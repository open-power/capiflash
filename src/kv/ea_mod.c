/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ea_mod.c $                                             */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ark.h"
#include "bl.h"
#include "ea.h"
#include "capiblock.h"
#include "am.h"

#include <test/fvt_kv_inject.h>
#include <arkdb_trace.h>

int ea_async_io_schedule(_ARK *_arkp, int32_t tid, tcb_t *tcbp, iocb_t *iocbp)
{
  EA       *ea             = iocbp->ea;
  int32_t   rc             = 0;
  int32_t   harvest        = 0;
  int64_t   i              = 0;
  uint8_t  *p_addr         = NULL;
  uint8_t  *m_addr         = NULL;

  if ( EA_STORE_TYPE_MEMORY == ea->st_type )
  {
    // With memory, since we will be doing a simple
    // memcpy, there really isn't anything to "wait"
    // for in terms of IO completion.  However, to keep
    // it somewhat similar in design, we will perform the
    // memcpy inline here, but the "wait" IO task will
    // still be scheduled.  When it pops, no processing
    // will be done, instead the "complete" status
    // will be set and the IO will be considered
    // done.

    // Loop through the block list to issue the IO
    for ( i = 0; i < iocbp->nblks; i++ )
    {
      p_addr = ((uint8_t *)(iocbp->addr)) + (i * ea->bsize);

      // For in-memory store, we issue the memcpy.  There 
      // is no need to do any asynchronous "waiting" for 
      // IO completion.
      m_addr = ea->st_memory + (iocbp->blist[i].blkno * ea->bsize);

      if ( ARK_EA_READ == iocbp->op )
      {

        if (FVT_KV_READ_ERROR_INJECT)
        {
            FVT_KV_CLEAR_READ_ERROR; 
            rc = -(EIO);
            KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
	    break;
        }

	if ( memcpy(p_addr, m_addr, ea->bsize) == NULL )
	{
          rc = -(errno);
	  break;
	}
      }
      else
      {

        if (FVT_KV_WRITE_ERROR_INJECT)
        {
            FVT_KV_CLEAR_WRITE_ERROR; 
            rc = -(EIO);
            KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
	    break;
        }

	if ( memcpy(m_addr, p_addr, ea->bsize) == NULL )
	{
          rc = -(errno);
	  break;
	}
      }
    }

    if ( rc >= 0)
    {
      rc = iocbp->nblks;
    }

    am_free(iocbp->blist);
  }
  else
  {
    // The idea here is to try and issue all the IO's
    // for this operation.  However, we can run into
    // a situation where the block layer runs out of
    // command IO buffer space and starts returning
    // EAGAIN.  If that happens, we need to stop issuing
    // IO's and instead look to harvest the completion
    // of any outstanding ops, so we schedule a TASK
    // to do that.  This can get a bit tricky
    
    for (i = iocbp->start; i < iocbp->nblks; i++)
    {
      p_addr = ((uint8_t *)iocbp->addr) + (i * ea->bsize);

      // Call out to the block layer and retrive a block
      // Do an async op for a single block and tell the block
      // layer to wait if there are no available command
      // blocks.  Upon return, we can either get an error
      // (rc == -1), the data will be available (rc == number
      // of blocks read), or IO has been scheduled (rc == 0).
      if ( iocbp->op == ARK_EA_READ )
      {
        if (FVT_KV_READ_ERROR_INJECT)
        {
          FVT_KV_CLEAR_READ_ERROR;
          rc = -1;
	  errno = EIO;
          KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
        }
	else
	{
        KV_TRC_IO(pAT, "RD: id:%d blkno:%"PRIi64" tag:%d",
                ea->st_flash,
                iocbp->blist[i].blkno,
                iocbp->blist[i].a_tag);
          rc = cblk_aread(ea->st_flash, p_addr, iocbp->blist[i].blkno, 1, 
                          &(iocbp->blist[i].a_tag), NULL, 
                          0);
        }
      }
      else
      {
        if (FVT_KV_WRITE_ERROR_INJECT)
        {
          FVT_KV_CLEAR_WRITE_ERROR;
          rc = -1;
	  errno = EIO;
          KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
        }
	else
	{
        KV_TRC_IO(pAT, "WR: id:%d blkno:%"PRIi64" tag:%d",
                ea->st_flash,
                iocbp->blist[i].blkno,
                iocbp->blist[i].a_tag);
          rc = cblk_awrite(ea->st_flash, p_addr, iocbp->blist[i].blkno, 1, 
                        &(iocbp->blist[i].a_tag), NULL, 
                        0);
        }
      }

      if ( rc == -1 )
      {
        if ( errno != EAGAIN )
	{
          // Something bad went wrong.  We need to fail the
	  // IO operation and the KV command itself.  We schedule
	  // an ARK_IO_HARVEST task and set an error in the status
	  // field.
          //rc = -(errno);
          rc = -(EIO);
        }
	else
	{
          rc = 0;
	}

	break;
      }
      else if ( rc > 0 )
      {
        // Data has already been returned so we don't need
	// to harvest the completion for this block
	iocbp->blist[i].a_tag = -1;
      }
      else
      {
        // IO was scheduled and we need to check back
	// later for the completion.
	harvest = 1;
      }
    }

    iocbp->new_start = i;

    if ( harvest )
    {
      // Schedule the task to harvest the outstanding
      // IO operations.
      iocbp->io_errno = rc;
      rc = 0;
    }
    else
    {
      if ( i == iocbp->nblks )
      {
        // The IO has completed and we have all the data
        // Schedule an ARK_IO_DONE task.
        rc = iocbp->nblks;
      }
    }
  }

  return rc;
}

int ea_async_io_harvest(_ARK *_arkp, int32_t tid, tcb_t *tcbp, iocb_t *iocbp)
{
  EA        *ea     = iocbp->ea;
  int32_t    i      = 0;
  int32_t    rc     = 0;
  int32_t    a_rc   = 0;
  uint64_t   status = 0;
  
  // Loop through all potential async IO's waiting for
  // completions
  for (i = iocbp->start; i < iocbp->new_start; i++)
  {
    // If a_tag is -1, that means we don't need to wait
    // for any data...just move on to the next one
    if ( iocbp->blist[i].a_tag == -1 )
    {
      continue;
    }

    do
    {
      // Make call to harvest the IO completion.
      a_rc = cblk_aresult(ea->st_flash, &(iocbp->blist[i].a_tag), 
                           &status, CBLK_ARESULT_BLOCKING);

      KV_TRC_IO(pAT, "RT: id:%d blkno:%"PRIi64" tag:%d status:%"PRIi64"",
              ea->st_flash,
              iocbp->blist[i].blkno,
              iocbp->blist[i].a_tag,
              status);
      if (a_rc == -1)
      {
        if (rc == 0)
        {
          //rc = -errno;
          rc = -(EIO);
        }
      }
    } while (a_rc == 0);
  }

  // If we've harvested all IO's or if we got
  // an error, then the IO itself
  // is complete.  Schedule the ARK_IO_DONE task.
  if ( (iocbp->io_errno < 0) || (rc < 0) || (iocbp->new_start == iocbp->nblks) )
  {
    if ( iocbp->io_errno < 0 )
    {
      rc = iocbp->io_errno;
    }
    else if ( (rc == 0) && (iocbp->new_start == iocbp->nblks) )
    {
      rc = iocbp->nblks;
    }
    am_free(iocbp->blist);
  }
  else
  {
    // We are not done so we have to go back and
    // schedule some more IOs
    iocbp->start = iocbp->new_start;
    rc = 0;
  }

  return rc;
}

int ea_async_io_mod(_ARK *_arkp, int op, void *addr, ark_io_list_t *blist, 
                int64_t nblks, int start, int32_t tag, int32_t io_done)
{
  int     status = 0;
  iocb_t *iocbp = &(_arkp->iocbs[tag]);
  tcb_t  *tcbp  = &(_arkp->tcbs[tag]);

  // We really shouldn't run into an error here since all we
  // are doing is filling in the iocb_t structure for the
  // given tag and setting the state to ARK_IO_SCHEDULE and
  // queueing it up on the task queue. 
  iocbp->ea       = _arkp->ea;
  iocbp->op       = op;
  iocbp->addr     = addr;
  iocbp->blist    = blist;
  iocbp->nblks    = nblks;
  iocbp->start    = start;
  iocbp->new_start = 0;
  iocbp->io_errno = 0;
  iocbp->io_done  = io_done;
  iocbp->tag      = tag;

  // Set the state to start the IO
  // tcbp->state     = ARK_IO_SCHEDULE;

  ARK_SYNC_EA_READ(iocbp->ea);

  // Queue up the task now.
  // (void)sq_enq(_arkp->tkq[tid], (void *)&task);
  status = ea_async_io_schedule(_arkp, 0, tcbp, iocbp);

  ARK_SYNC_EA_UNLOCK(iocbp->ea);

  return status;
}

