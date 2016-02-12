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

/**
 *******************************************************************************
 * \file
 * \brief
 *   init/schedule/harvest functions for async IO
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "ark.h"
#include "bl.h"
#include "ea.h"
#include "am.h"

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include "capiblock.h"
#endif

#include <kv_inject.h>
#include <arkdb_trace.h>
#include <errno.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/**
 *******************************************************************************
 * \brief
 *  return TRUE if all IOs for the iocb are successfully completed, else FALSE
 ******************************************************************************/
int ea_async_io_schedule(_ARK   *_arkp,
                         int32_t tid,
                         tcb_t  *iotcbp,
                         iocb_t *iocbp)
{
  EA       *ea     = iocbp->ea;
  int32_t   rc     = TRUE;
  int32_t   arc    = 0;
  void     *prc    = 0;
  int64_t   i      = 0;
  uint8_t  *p_addr = NULL;
  uint8_t  *m_addr = NULL;
  char     *ot     = NULL;

  KV_TRC_IO(pAT, "IO_BEG: SCHEDULE_START: tid:%d ttag:%d start:%"PRIu64" "
                 "nblks:%"PRIu64" issT:%d cmpT:%d",
                 tid, iocbp->tag, iocbp->start, iocbp->nblks,
                 iocbp->issT, iocbp->cmpT);

  ARK_SYNC_EA_READ(iocbp->ea);

  if (iocbp->op == ARK_EA_READ) {ot="IO_RD";}
  else                          {ot="IO_WR";}

  for (i=iocbp->start; i<iocbp->nblks; i++)
  {
      if (ea->st_type == EA_STORE_TYPE_MEMORY)
      {
          p_addr = ((uint8_t *)(iocbp->addr)) + (i * ea->bsize);
          m_addr = ea->st_memory + (iocbp->blist[i].blkno * ea->bsize);

          if (ARK_EA_READ == iocbp->op) {prc = memcpy(p_addr,m_addr,ea->bsize);}
          else                          {prc = memcpy(m_addr,p_addr,ea->bsize);}

          if (check_sched_error_injects(iocbp->op)) {prc=NULL;}

          // if memcpy failed, fail the iocb
          if (prc == NULL)
          {
              rc=FALSE;
              KV_TRC_FFDC(pAT,"IO_ERR: tid:%d ttag:%d blkno:%"PRIi64""
                              " errno:%d", tid, iocbp->tag,
                              iocbp->blist[i].blkno, errno);
              if (!errno) {KV_TRC_FFDC(pAT, "IO:     UNSET_ERRNO"); errno=EIO;}
              iocbp->io_error = errno;
              break;
          }
          ++iocbp->issT;
          iocbp->blist[i].a_tag = i;
      }
      else // r/w to hw
      {
          p_addr = ((uint8_t *)iocbp->addr) + (i * ea->bsize);

          if (check_sched_error_injects(iocbp->op))
          {
              arc=-1;
          }
          else if ( iocbp->op == ARK_EA_READ )
          {
              arc = cblk_aread(ea->st_flash, p_addr, iocbp->blist[i].blkno, 1,
                              &(iocbp->blist[i].a_tag), NULL, 0);
          }
          else
          {
              arc = cblk_awrite(ea->st_flash, p_addr, iocbp->blist[i].blkno, 1,
                               &(iocbp->blist[i].a_tag), NULL, 0);
          }

          if (arc == 0)    // good status
          {
              ++iocbp->issT; rc=FALSE;
          }
          else if (arc < 0)
          {
              rc=FALSE;
              if (errno == EAGAIN)
              {
                  // return, and an ark thread will re-schedule this iocb
                  KV_TRC_DBG(pAT,"IO:    RW_EAGAIN: tid:%d ttag:%d "
                                 "blkno:%"PRIi64"",
                                 tid, iocbp->tag, iocbp->blist[i].blkno);
                  break;
              }
              // Something bad went wrong, fail the iocb
              KV_TRC_FFDC(pAT,"IO_ERR: tid:%d ttag:%d blkno:%"PRIi64""
                              " errno:%d", tid, iocbp->tag,
                              iocbp->blist[i].blkno, errno);
              if (!errno) {KV_TRC_FFDC(pAT, "IO:     UNSET_ERRNO"); errno=EIO;}
              iocbp->io_error = errno;
              break;
          }
          else if (arc > 0)
          {
              KV_TRC_IO(pAT,"IO_CMP: IMMEDIATE: tid:%d ttag:%d a_tag:%d "
                            "blkno:%"PRIi64"",
                            tid, iocbp->tag,
                            iocbp->blist[i].a_tag, iocbp->blist[i].blkno);
              ++iocbp->issT;
              ++iocbp->cmpT;
              iocbp->blist[i].a_tag = -1; // mark as harvested
          }
      }

      KV_TRC_IO(pAT, "%s:  tid:%2d ttag:%4d a_tag:%4d blkno:%5"PRIi64"", ot,tid,
                iocbp->tag, iocbp->blist[i].a_tag, iocbp->blist[i].blkno);

  }

  iotcbp->state = ARK_IO_HARVEST;
  iocbp->start  = i;
  ARK_SYNC_EA_UNLOCK(iocbp->ea);

  return rc;
}

/**
 *******************************************************************************
 * \brief
 *  return TRUE if the IOs for the iocb are successfully completed, else FALSE
 ******************************************************************************/
int ea_async_io_harvest(_ARK   *_arkp,
                        int32_t tid,
                        tcb_t  *iotcbp,
                        iocb_t *iocbp,
                        rcb_t  *iorcbp)
{
  EA       *ea     = iocbp->ea;
  int32_t   i      = 0;
  int32_t   arc    = 0;
  int32_t   rc     = FALSE;
  uint64_t  status = 0;
  scb_t    *scbp   = &(_arkp->poolthreads[tid]);
  queue_t  *rq     = scbp->rqueue;
  queue_t  *tq     = scbp->tqueue;
  queue_t  *ioq    = scbp->ioqueue;

  for (i=0; i<iocbp->issT; i++)
  {
      if (EA_STORE_TYPE_MEMORY == ea->st_type)
      {
          // the IO has already been done in the schedule function,
          // so mark it completed
          arc = 1;
      }
      else
      {
          // skip previously harvested cmd
          if (iocbp->blist[i].a_tag == -1) {continue;}

          arc = cblk_aresult(ea->st_flash, &(iocbp->blist[i].a_tag), &status,0);
      }

      if (check_harv_error_injects(iocbp->op)) {arc=-1;}

      if (arc == 0)
      {
          KV_TRC_DBG(pAT,"IO:     WAIT_NOT_CMP: tid:%d ttag:%d a_tag:%d "
                         "blkno:%"PRIi64"",
                         tid, iocbp->tag, iocbp->blist[i].a_tag,
                         iocbp->blist[i].blkno);
          ++iocbp->hmissN;

          // if nothing to do and the first harvest missed, usleep
          if (queue_empty(rq) && queue_empty(tq) && queue_count(ioq)<=8 &&
              iocbp->hmissN==1 &&
              _arkp->ea->st_type != EA_STORE_TYPE_MEMORY)
          {
              usleep(50);
              KV_TRC_DBG(pAT,"IO:     USLEEP");
          }
          break;
      }

      if (arc < 0)
      {
          KV_TRC_FFDC(pAT, "IO_ERR: tid:%d ttag:%d errno=%d",
                           tid, iocbp->tag, errno);
          if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=EIO;}
          iocbp->io_error = errno;
      }
      else
      {
          KV_TRC_IO(pAT,"IO_CMP: tid:%2d ttag:%4d a_tag:%4d blkno:%5"PRIi64"",
                    tid, iocbp->tag,
                    iocbp->blist[i].a_tag, iocbp->blist[i].blkno);
      }

      ++iocbp->cmpT;
      iocbp->blist[i].a_tag = -1; // mark as harvested
  }

  if (iocbp->io_error)
  {
      // if all cmds that were issued (success or fail) have been
      // completed for this iocb, then fail this iocb
      if (iocbp->issT == iocbp->cmpT)
      {
          iorcbp->res    = -1;
          iorcbp->rc     = iocbp->io_error;
          iotcbp->state  = ARK_CMD_DONE;
          am_free(iocbp->blist);
          KV_TRC_FFDC(pAT, "IO:     ERROR_DONE: tid:%d ttag:%d rc:%d",
                      tid, iocbp->tag, iorcbp->rc);
      }
      else
      {
          // IOs outstanding, harvest the remaining IOs for this iocb
          KV_TRC_FFDC(pAT,"IO:    ERROR_RE_HARVEST: tid:%d ttag:%d "
                          "iocbp->issT:%d iocbp->cmpT:%d",
                          tid, iocbp->tag, iocbp->issT, iocbp->cmpT);
      }
  }
  // if all IO has completed successfully for this iocb, done
  else if (iocbp->cmpT == iocbp->nblks)
  {
      rc=TRUE;
      am_free(iocbp->blist);
      iotcbp->state = ARK_IO_DONE;
      KV_TRC_IO(pAT, "IO_END: SUCCESS tid:%d ttag:%d cmpT:%d",
                tid, iocbp->tag, iocbp->cmpT);
  }
  // if more blks need an IO, schedule
  else if (iocbp->issT < iocbp->nblks)
  {
      iotcbp->state = ARK_IO_SCHEDULE;
      KV_TRC_IO(pAT,"IO:     RE_SCHEDULE: tid:%d ttag:%d "
                    "iocbp->issT:%d iocbp->nblks:%"PRIi64" ",
                    tid, iocbp->tag, iocbp->issT, iocbp->nblks);
  }
  else
  {
      // all IOs have been issued but not all are completed, do harvest
      KV_TRC_IO(pAT,"IO:     RE_HARVEST: tid:%d ttag:%d "
                    "iocbp->cmpT:%d iocbp->issT:%d",
                    tid, iocbp->tag, iocbp->cmpT, iocbp->issT);
  }
  return rc;
}

/**
 *******************************************************************************
 * \brief
 *  init iocb struct for IO
 ******************************************************************************/
void ea_async_io_init(_ARK *_arkp, int op, void *addr, ark_io_list_t *blist,
                      int64_t nblks, int start, int32_t tag, int32_t io_done)
{
  iocb_t *iocbp  = &(_arkp->iocbs[tag]);

  iocbp->ea       = _arkp->ea;
  iocbp->op       = op;
  iocbp->addr     = addr;
  iocbp->blist    = blist;
  iocbp->nblks    = nblks;
  iocbp->start    = start;
  iocbp->issT     = 0;
  iocbp->cmpT     = 0;
  iocbp->hmissN   = 0;
  iocbp->io_error = 0;
  iocbp->io_done  = io_done;
  iocbp->tag      = tag;

  return;
}
