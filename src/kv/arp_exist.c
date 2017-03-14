/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp_exist.c $                                          */
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ut.h>
#include <vi.h>
#include <arkdb.h>
#include <ark.h>
#include <arp.h>
#include <am.h>
#include <kv_inject.h>
#include <arkdb_trace.h>

#include <errno.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if successful returns length of value
void ark_exist_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp     = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t         *iotcbp   = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t        *iocbp    = &(_arkp->iocbs[rcbp->ttag]);
  int32_t        rc       = 0;

  scbp->poolstats.ops_cnt += 1;

  // Now that we have the hash entry, get the block
  // that holds the control information for the entry.
  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  if (DUMP_KV) {KV_TRC_HEX(pAT, 9, "exi_key ", rcbp->key, rcbp->klen);}

  // If there is no control block for this hash
  // entry, then the key is not present in the hash.
  // Set the error
  if (tcbp->hblk == 0)
  {
    KV_TRC(pAT, "ENOENT tid:%d ttag:%3d key:%p klen:%ld pos:%ld",
           tid, tcbp->ttag, rcbp->key, rcbp->klen, rcbp->pos);
    rcbp->res   = -1;
    rcbp->rc    = ENOENT;
    tcbp->state = ARK_CMD_DONE;
    goto ark_exist_start_err;
  }

  // Set up the in-buffer to read in the hash bucket
  // that contains the key
  tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);

  if (tcbp->blen*_arkp->bsize > tcbp->inb_size)
  {
      KV_TRC(pAT, "RE_INB  tid:%d ttag:%3d old:%ld new:%ld",
             tid, tcbp->ttag, tcbp->inb_size, tcbp->blen*_arkp->bsize);
      rc = bt_realloc(&(tcbp->inb), &(tcbp->inb_orig), tcbp->blen*_arkp->bsize);
      if (rc != 0)
      {
          KV_TRC_FFDC(pAT, "bt_realloc failed tcbp:%p ttag:%d", tcbp, tcbp->ttag);
          rcbp->res   = -1;
          rcbp->rc    = rc;
          tcbp->state = ARK_CMD_DONE;
          goto ark_exist_start_err;
      }
      tcbp->inb_size = tcbp->blen*_arkp->bsize;
  }

  if (kv_inject_flags) {HTC_FREE(_arkp->htc[rcbp->pos]);}

  scbp->poolstats.io_cnt += tcbp->blen;

  if (HTC_HIT(_arkp->htc[rcbp->pos], tcbp->blen))
  {
      ++_arkp->htc_hits;
      KV_TRC(pAT, "HTC_HIT tid:%d ttag:%3d pos:%ld", tid, tcbp->ttag,rcbp->pos);
      HTC_GET(_arkp->htc[rcbp->pos], tcbp->inb, tcbp->blen*_arkp->bsize);
      ark_exist_finish(_arkp, tid, tcbp);
      return;
  }

  // Create a chain of blocks to be passed to be read
  if (bl_rechain(&tcbp->aiol, _arkp->bl, tcbp->hblk, tcbp->blen, tcbp->aiolN))
  {
    rcbp->res   = -1;
    rcbp->rc    = ENOMEM;
    tcbp->state = ARK_CMD_DONE;
    KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
    goto ark_exist_start_err;
  }
  tcbp->aiolN = tcbp->blen;

  KV_TRC(pAT, "RD_HASH tid:%d ttag:%3d", tid, tcbp->ttag);
  ea_async_io_init(_arkp, ARK_EA_READ, (void *)tcbp->inb, tcbp->aiol,
                   tcbp->blen, 0, tcbp->ttag, ARK_EXIST_FINISH);

  if (MEM_FASTPATH)
  {
      ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
      ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
      if (iotcbp->state == ARK_EXIST_FINISH) {ark_exist_finish(_arkp,tid,tcbp);}
  }

ark_exist_start_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_exist_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  rcb_t *rcbp = &(_arkp->rcbs[tcbp->rtag]);

  KV_TRC(pAT, "INB_GET tid:%d ttag:%3d tot:%ld used:%ld",
               tid, tcbp->ttag, tcbp->inb_size, tcbp->inb->len);

  // Find the key position in the read in bucket
  rcbp->res = bt_exists(tcbp->inb, rcbp->klen, rcbp->key);
  if (rcbp->res == BT_FAIL)
  {
    KV_TRC(pAT, "ENOENT tid:%d ttag:%3d key:%p klen:%ld pos:%ld",
           tid, tcbp->ttag, rcbp->key, rcbp->klen, rcbp->pos);
    rcbp->rc    = ENOENT;
    rcbp->res   = -1;
    tcbp->state = ARK_CMD_DONE;
  }

  tcbp->state = ARK_CMD_DONE;
  return;
}

