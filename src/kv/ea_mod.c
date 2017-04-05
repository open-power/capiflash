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

#include <ark.h>
#include <bl.h>
#include <ea.h>
#include <am.h>
#include <ticks.h>

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

  KV_TRC_IO(pAT, "IO_BEG: tid:%d ttag:%3d start:%4"PRIu64" "
                 "issT:%3d cmpT:%3d nblks:%3"PRIu64" addr:%p",
                 tid, iocbp->tag, iocbp->start,
                 iocbp->issT, iocbp->cmpT, iocbp->nblks, iocbp->addr);

  ARK_SYNC_EA_READ(iocbp->ea);
  errno           = 0;
  iocbp->io_error = 0;
  iocbp->stime    = getticks();

  if (iocbp->op == ARK_EA_READ) {ot="IO_RD";}
  else                          {ot="IO_WR";}

  for (i=iocbp->start; i<iocbp->nblks; i++)
  {
      if (ea->st_type == EA_STORE_TYPE_MEMORY)
      {
          p_addr       = ((uint8_t *)(iocbp->addr)) + (i * ea->bsize);
          m_addr       = ea->st_memory + (iocbp->blist[i].blkno * ea->bsize);
          iocbp->issT += 1; ++_arkp->issT;

          if (ARK_EA_READ == iocbp->op) {prc = memcpy(p_addr,m_addr,ea->bsize);}
          else                          {prc = memcpy(m_addr,p_addr,ea->bsize);}

          if (check_sched_error_injects(iocbp->op)) {prc=NULL;}

          // if memcpy failed, fail the iocb
          if (prc == NULL)
          {
              rc=FALSE;
              KV_TRC_FFDC(pAT,"IO_ERR: tid:%d ttag:%3d issT:%3d cmpT:%3d "
                              "nblks:%3ld blkno:%"PRIi64" errno:%d",
                              tid, iocbp->tag, iocbp->issT, iocbp->cmpT,
                              iocbp->nblks, iocbp->blist[i].blkno, errno);
              if (!errno) {KV_TRC_FFDC(pAT, "IO:     UNSET_ERRNO"); errno=EIO;}
              iocbp->io_error = errno;
              break;
          }
          iocbp->blist[i].a_tag.tag = i;
      }
      else // r/w to hw
      {
          p_addr       = ((uint8_t *)iocbp->addr) + (i * ea->bsize);
          iocbp->issT += 1; ++_arkp->issT;

          if (check_sched_error_injects(iocbp->op))
          {
              arc = -1;
          }
          else if ( iocbp->op == ARK_EA_READ )
          {
              iocbp->rd = 1;
              arc = cblk_cg_aread(ea->st_flash, p_addr, iocbp->blist[i].blkno,1,
                              &(iocbp->blist[i].a_tag), NULL, iocbp->aflags);
          }
          else
          {
              arc = cblk_cg_awrite(ea->st_flash, p_addr,iocbp->blist[i].blkno,1,
                              &(iocbp->blist[i].a_tag), NULL, iocbp->aflags);
          }

          if      (arc == 0) {rc=FALSE;} // good status
          else if (arc <  0)
          {
              rc=FALSE;
              if (errno == EAGAIN)
              {
                  iocbp->issT -= 1; --_arkp->issT;
                  // return, and an ark thread will re-schedule this iocb
                  KV_TRC(pAT,"IO:     tid:%d ttag:%3d EAGAIN "
                             "issT:%3d cmpT:%3d nblks:%3ld blkno:%"PRIi64"",
                             tid, iocbp->tag, iocbp->issT, iocbp->cmpT,
                             iocbp->nblks, iocbp->blist[i].blkno);
                  break;
              }
              // Something bad went wrong, fail the iocb
              KV_TRC_FFDC(pAT,"IO_ERR: tid:%d ttag:%3d issT:%3d cmpT:%3d "
                              "nblks:%3ld blkno:%"PRIi64" errno:%d",
                              tid, iocbp->tag, iocbp->issT, iocbp->cmpT,
                              iocbp->nblks, iocbp->blist[i].blkno, errno);
              if (!errno) {KV_TRC_FFDC(pAT, "IO:     UNSET_ERRNO"); errno=EIO;}
              iocbp->cmpT    += 1;
              iocbp->io_error = errno;
              break;
          }
          else if (arc > 0)
          {
              KV_TRC(pAT,"IO_CMP: tid:%d ttag:%3d a_tag:%4d IMMEDIATE "
                         "issT:%3d cmpT:%3d nblks:%3ld blkno:%"PRIi64"",
                         tid, iocbp->tag, iocbp->blist[i].a_tag.tag,
                         iocbp->issT, iocbp->cmpT, iocbp->nblks,
                         iocbp->blist[i].blkno);
              ++iocbp->issT;
              ++iocbp->cmpT;
              iocbp->blist[i].a_tag.tag = -1; // mark as harvested
          }
      }

      KV_TRC_IO(pAT, "%s:  tid:%d ttag:%3d a_tag:%4d issT:%3d cmpT:%3d "
                     "nblks:%3ld blkno:%5"PRIi64"", ot,tid, iocbp->tag,
                     iocbp->blist[i].a_tag.tag, iocbp->issT, iocbp->cmpT,
                     iocbp->nblks, iocbp->blist[i].blkno);
  }

  /* if a cmd schedule failed OR cmds are waiting for harvest, call harvest */
  if ((iocbp->io_error) ||
      (iocbp->issT && (iocbp->issT != iocbp->cmpT)))
  {
      iotcbp->state = ARK_IO_HARVEST;
      iocbp->start  = i;
  }
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
  int32_t   hrvst  = (iocbp->aflags & CBLK_ARESULT_NO_HARVEST)==0;
  int32_t   rc     = FALSE;
  uint64_t  status = 0;

  for (i=0; i<iocbp->nblks; i++)
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
          if (iocbp->blist[i].a_tag.tag == -1) {continue;}

          arc = cblk_cg_aresult(ea->st_flash,
                                &iocbp->blist[i].a_tag, &status, iocbp->aflags);
      }

      if (check_harv_error_injects(iocbp->op)) {arc=-1;}

      if (arc == 0 && iocbp->issT > iocbp->cmpT)
      {
          KV_TRC_DBG(pAT,"IO:     tid:%d ttag:%3d a_tag:%4d issT:%3d cmpT:%3d "
                         "nblks:%3ld blkno:%5ld hrvst:%d WAIT_NOT_CMP",
                         tid, iocbp->tag, iocbp->blist[i].a_tag.tag,
                         iocbp->issT, iocbp->cmpT, iocbp->nblks,
                         iocbp->blist[i].blkno, hrvst);
          ++iocbp->hmissN;
          break;
      }

      ++iocbp->cmpT; --_arkp->issT;

      iocbp->lat = UDELTA(iocbp->stime, _arkp->ns_per_tick);

      if (arc < 0)
      {
          KV_TRC_FFDC(pAT, "IO_ERR: tid:%d ttag:%3d a_tag:%4d issT:%3d "
                           "cmpT:%3d nblks:%3ld lat:%d errno=%d",
                           tid, iocbp->tag, iocbp->blist[i].a_tag.tag,
                           iocbp->issT, iocbp->cmpT, iocbp->nblks,
                           iocbp->lat, errno);
          if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=EIO;}
          iocbp->io_error = errno;
      }
      else
      {
          iocbp->hmissN=0;
          KV_TRC_IO(pAT,"IO_CMP: tid:%d ttag:%3d a_tag:%4d issT:%3d "
                        "cmpT:%3d nblks:%3ld blkno:%5ld rd:%d lat:%d",
                        tid, iocbp->tag, iocbp->blist[i].a_tag.tag,
                        iocbp->issT, iocbp->cmpT, iocbp->nblks,
                        iocbp->blist[i].blkno, iocbp->rd, iocbp->lat);
      }

      iocbp->blist[i].a_tag.tag = -1; // mark as harvested
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
          KV_TRC_FFDC(pAT, "IO:     tid:%d ttag:%3d ERROR_DONE rc:%d",
                      tid, iocbp->tag, iorcbp->rc);
      }
      else
      {
          // IOs outstanding, harvest the remaining IOs for this iocb
          KV_TRC_FFDC(pAT,"IO:    tid:%d ttag:%3d ERROR_RE_HARVEST "
                          "issT:%3d cmpT:%3d nblks:%3ld",
                          tid, iocbp->tag, iocbp->issT, iocbp->cmpT,
                          iocbp->nblks);
      }
  }
  // if all IO has completed successfully for this iocb, done
  else if (iocbp->cmpT == iocbp->nblks)
  {
      rc=TRUE;
      iotcbp->state = iocbp->io_done;
      KV_TRC(pAT, "IO_END: tid:%d ttag:%3d SUCCESS cmpT:%d",
             tid, iocbp->tag, iocbp->cmpT);
  }
  // if more blks need a harvest
  else if (iocbp->cmpT < iocbp->issT)
  {
      iotcbp->state = ARK_IO_HARVEST;
      KV_TRC_IO(pAT,"IO:     tid:%d ttag:%3d hrvst:%4d "
                    "issT:%3d cmpT:%3d nblks:%3ld RE_HARVEST_PART",
                tid, iocbp->tag, hrvst, iocbp->issT, iocbp->cmpT, iocbp->nblks);
  }
  // if more blks need an IO, schedule
  else if (iocbp->issT < iocbp->nblks)
  {
      iotcbp->state = ARK_IO_SCHEDULE;
      KV_TRC_IO(pAT,"IO:     tid:%d ttag:%3d hrvst:%4d "
                    "issT:%3d cmpT:%3d, nblks:%3"PRIi64" RE_SCHEDULE",
                tid, iocbp->tag, hrvst, iocbp->issT, iocbp->cmpT, iocbp->nblks);
  }
  else
  {
      // all IOs have been issued but not all are completed, do harvest
      KV_TRC_IO(pAT,"IO:     tid:%d ttag:%3d hrvst:%4d "
                    "issT:%3d cmpT:%3d nblks:%3ld RE_HARVEST",
                tid, iocbp->tag, hrvst, iocbp->issT, iocbp->cmpT, iocbp->nblks);
  }
  return rc;
}
