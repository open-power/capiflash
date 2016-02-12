/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp_del.c $                                            */
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

#include "ut.h"
#include "vi.h"

#include "arkdb.h"
#include "ark.h"
#include "arp.h"
#include "am.h"

#include <arkdb_trace.h>
#include <errno.h>

// if success returns size of value deleted
void ark_del_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp       = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t         *iotcbp     = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t        *iocbp      = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t *bl_array   = NULL;
  int32_t        rc         = 0;

  // Acquire the block that contains the hash entry
  // control information.
  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  // If there is no block, that means there are no
  // entries in the hash entry, which means they
  // key in question is not in the store.
  if ( tcbp->hblk == 0 )
  {
    KV_TRC_FFDC(pAT, "rc = ENOENT: key %p, klen %"PRIu64" ttag:%d",
                 rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = ENOENT;
    tcbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }

  // Check to see if we need to grow the in buffer
  // to hold the hash entry
  tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);
  if (tcbp->blen <= 0)
  {
    KV_TRC_FFDC(pAT, "rc = ENOENT: key %p, klen %"PRIu64" ttag:%d",
                 rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->res   = -1;
    rcbp->rc    = ENOENT;
    tcbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }
  rc = bt_growif(&(tcbp->inb), &(tcbp->inb_orig), &(tcbp->inblen), 
                  (tcbp->blen * _arkp->bsize));
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "bt_growif failed ttag:%d", tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = rc; 
    tcbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }

  // Create a list of blocks that will be read in
  bl_array = bl_chain(_arkp->bl, tcbp->hblk, tcbp->blen);
  if (bl_array == NULL)
  {
    KV_TRC_FFDC(pAT, "bl_chain failed ttag:%d", tcbp->ttag);
    rcbp->rc = ENOMEM;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }

  tcbp->old_btsize = tcbp->inb->len;
  scbp->poolstats.io_cnt += tcbp->blen;

  KV_TRC_IO(pAT, "read hash entry ttag:%d", tcbp->ttag);
  // Schedule the IO to read the hash entry from storage
  ea_async_io_init(_arkp, ARK_EA_READ, (void *)tcbp->inb, bl_array,
                       tcbp->blen, 0, tcbp->ttag, ARK_DEL_PROCESS);
  if (ea_async_io_schedule(_arkp, tid, iotcbp, iocbp) &&
      ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp))
  {
      ark_del_process(_arkp, tid, tcbp);
  }

ark_del_start_err:

  return;
}

void ark_del_process(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp       = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t         *iotcbp     = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t        *iocbp      = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t *bl_array   = NULL;
  int32_t        rc         = 0;
  uint64_t       blkcnt     = 0;
  uint64_t       oldvlen    = 0;
  int64_t        dblk       = 0;

  rc = bt_growif(&(tcbp->oub), &(tcbp->oub_orig), &(tcbp->oublen), 
                  (tcbp->blen * _arkp->bsize));
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "bt_growif failed ttag:%d", tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = rc;
    tcbp->state = ARK_CMD_DONE;
    goto ark_del_process_err;
  }

  // Delete the key and value from the hash entry
  // and save off the length of the value
  rcbp->res = bt_del_def(tcbp->oub, tcbp->inb, rcbp->klen, 
                          rcbp->key, (uint8_t*)&dblk, &oldvlen);
  
  if (rcbp->res >= 0)
  {
    // Return the blocks of the hash entry back to
    // the free list
    ark_drop_pool(_arkp, &(scbp->poolstats), tcbp->hblk);
    if (dblk > 0)
    { 
      // Return the blocks used to store the value if it
      // wasn't stored in the hash entry
      ark_drop_pool(_arkp, &(scbp->poolstats), dblk);
    }

    // Are there entries in the hash bucket.
    if (tcbp->oub->cnt > 0)
    {

      // Determine how many blocks will be needed for the
      // out buffer and then get them from the free
      // block list
      blkcnt = divceil(tcbp->oub->len, _arkp->bsize);
      tcbp->nblk = ark_take_pool(_arkp, &(scbp->poolstats), blkcnt);
      if (tcbp->nblk == -1)
      {
        KV_TRC_FFDC(pAT, "ark_take_pool %ld failed ttag:%d",
                    blkcnt, tcbp->ttag);
        rcbp->rc = ENOSPC;
        rcbp->res = -1;
        tcbp->state = ARK_CMD_DONE;
        goto ark_del_process_err;
      }

      // Create a list of the blocks to write.
      bl_array = bl_chain(_arkp->bl, tcbp->nblk, blkcnt);
      if (bl_array == NULL)
      {
        KV_TRC_FFDC(pAT, "bl_chain failed ttag:%d", tcbp->ttag);
        rcbp->rc = ENOMEM;
        rcbp->res = -1;
        tcbp->state = ARK_CMD_DONE;
        goto ark_del_process_err;
      }

      scbp->poolstats.io_cnt += blkcnt;
      scbp->poolstats.byte_cnt -= (tcbp->old_btsize + oldvlen);
      scbp->poolstats.byte_cnt += tcbp->oub->len;

      KV_TRC_IO(pAT, "write updated hash entry ttag:%d", tcbp->ttag);
      // Schedule the WRITE IO of the updated hash entry.
      ea_async_io_init(_arkp, ARK_EA_WRITE, (void *)tcbp->oub,
                       bl_array, blkcnt, 0, tcbp->ttag, ARK_DEL_FINISH);
      if (ea_async_io_schedule(_arkp, tid, iotcbp, iocbp) &&
          ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp))
      {
          ark_del_finish(_arkp, tid, tcbp);
      }
    }
    else
    {
      scbp->poolstats.byte_cnt -= (tcbp->old_btsize + oldvlen);
      scbp->poolstats.byte_cnt += tcbp->oub->len;
      scbp->poolstats.kv_cnt--;

      KV_TRC_IO(pAT, "no write required for hash entry ttag:%d", tcbp->ttag);
      // Nothing left in this hash entry, so let's clear out
      // the hash entry control block to show there is no
      // data in the store for this hash entry
      HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, 0));

      rcbp->rc = 0;
      tcbp->state = ARK_CMD_DONE;
    }
  }
  else
  {
    KV_TRC_FFDC(pAT, "rc = ENOENT: key %p, klen %"PRIu64" ttag:%d",
                 rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->rc = ENOENT;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
  }

ark_del_process_err:

  return;
}

void ark_del_finish(_ARK *_arkp, int32_t tid, tcb_t *tcbp)
{
  rcb_t  *rcbp  = &(_arkp->rcbs[tcbp->rtag]);
  scb_t  *scbp  = &(_arkp->poolthreads[tid]);

  scbp->poolstats.kv_cnt--;

  // Update the starting hash entry block with the
  // new block info.
  HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, tcbp->nblk));

  tcbp->state = ARK_CMD_DONE;
  return;
}

