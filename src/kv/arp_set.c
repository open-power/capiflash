/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp_set.c $                                            */
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

//#include "ct.h"
#include "ut.h"
#include "vi.h"

#include "arkdb.h"
#include "ark.h"
#include "arp.h"
#include "am.h"

#include <arkdb_trace.h>
#include <errno.h>

// This is the entry into ark_set.  The task has
// been pulled off the task queue.
void ark_set_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t            *scbp        = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp        = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t            *iotcbp      = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t           *iocbp       = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t    *bl_array    = NULL;
  uint64_t          vlen        = rcbp->vlen;
  uint64_t          new_vbsize  = 0;
  int32_t           rc          = 0;
  uint8_t           *new_vb     = NULL;

  scbp->poolstats.ops_cnt+=1;

   // Let's check to see if the value is larger than
  // what can fit inline in the bucket.
  if ( vlen > _arkp->vlimit )
  {
    // Determine the number of blocks needed to hold the value
    tcbp->vblkcnt = divceil(vlen, _arkp->bsize);

    // Ok, the value will not fit inline in the has bucket.
    // Let's see if the per task value buffer is currently
    // large enough to hold this value.  If not, we will
    // need to grow it.
    if ( vlen > tcbp->vbsize )
    {
      // Calculate the new size of the value buffer.  The
      // size will be based on the block size, rounded up
      // to ensure enough space to hold value. We record
      // the size in a new variable and wait to make sure
      // the realloc succeeds before updating vbsize
      new_vbsize = tcbp->vblkcnt * _arkp->bsize;
      KV_TRC_DBG(pAT, "VB_REALLOC tid:%d ttag:%3d vlen:%5ld new_vbsize:%ld",
                 tid, tcbp->ttag, vlen, new_vbsize);
      new_vb = am_realloc(tcbp->vb_orig, new_vbsize);
      if ( NULL == new_vb )
      {
        rcbp->res = -1;
        rcbp->rc = ENOMEM;
        tcbp->state = ARK_CMD_DONE;
        KV_TRC_FFDC(pAT, "realloc failed, ttag = %d rc = %d", tcbp->ttag, rc);
        goto ark_set_start_err;
      }
      tcbp->vbsize  = new_vbsize;
      tcbp->vb_orig = new_vb;
      tcbp->vb = ptr_align(tcbp->vb_orig);
    }

    memcpy(tcbp->vb, rcbp->val, vlen);

    // Get/Reserve the required number of blocks.  Pass along
    // the per server thread pool stats so that they can 
    // be updated
    tcbp->vblk = ark_take_pool(_arkp, &(scbp->poolstats), tcbp->vblkcnt);
    if ( -1 == tcbp->vblk )
    {
      rcbp->res = -1;
      rcbp->rc = ENOSPC;
      tcbp->state = ARK_CMD_DONE;
      KV_TRC_FFDC(pAT, "ark_take_pool %ld failed, ttag = %d rc = %d",
                  tcbp->vblkcnt, tcbp->ttag,rc);
      goto ark_set_start_err;
    }

    tcbp->vval = (uint8_t *)&(tcbp->vblk);
  }
  else
  {
    KV_TRC(pAT, "INLINE  tid:%d ttag:%3d vlen:%5ld", tid, tcbp->ttag, vlen);
    tcbp->vval = (uint8_t *)rcbp->val;
  }

  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  if ( 0 == tcbp->hblk )
  {
    KV_TRC(pAT, "1ST_KEY tid:%d ttag:%3d", tid, tcbp->ttag);
    // This is going to be the first key in this bucket.
    // Initialize the bucket data/header
    rc = bt_init(tcbp->inb);
    if (rc)
    {
        KV_TRC_FFDC(pAT, "bt_init failed ttag:%3d", tcbp->ttag);
        rcbp->res = -1;
        rcbp->rc  = rc;
        tcbp->state = ARK_CMD_DONE;
        goto ark_set_start_err;
    }

    // Call to the next phase of the SET command since
    // we do not need to read in the hash bucket since
    // this will be the first key in it.
    ark_set_process(_arkp, tid, tcbp);
  }
  else
  {
    // Look to see how many blocks are currently in use
    // for this hash bucket.
    tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);

    // Now that we have the number of blocks, check to see
    // if we need to grow the in-buffer.  If so, then
    // grow the in-buffer to the appropriate size.
    rc = bt_growif(&(tcbp->inb), &(tcbp->inb_orig), &(tcbp->inblen),
                     (tcbp->blen * _arkp->bsize));
    if ( 0 != rc )
    {
      rcbp->res = -1;
      rcbp->rc = rc;
      tcbp->state = ARK_CMD_DONE;
      KV_TRC_FFDC(pAT, "bt_growif failed, ttag:%3d rc:%d errno:%d",
              tcbp->ttag, rc, errno);
      goto ark_set_start_err;
    }

    // Create a list of the blocks that will be read in.
    // We do this upfront so we only have to access the block
    // list once.
    bl_array = bl_chain(_arkp->bl, tcbp->hblk, tcbp->blen);
    if ( NULL == bl_array )
    {
      rcbp->res = -1;
      rcbp->rc = ENOMEM;
      tcbp->state = ARK_CMD_DONE;
      KV_TRC_FFDC(pAT, "bl_chain failed, ttag = %d rc = %d", tcbp->ttag, rc);
      goto ark_set_start_err;
    }
    scbp->poolstats.io_cnt += tcbp->blen;

    KV_TRC(pAT, "RD_HASH tid:%d ttag:%3d", tid, tcbp->ttag);
    ea_async_io_init(_arkp, ARK_EA_READ,
                     (void *)tcbp->inb, bl_array, tcbp->blen,
                     0, tcbp->ttag, ARK_SET_PROCESS_INB);

    if (_arkp->ea->st_type == EA_STORE_TYPE_MEMORY)
    {
        ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
        ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
        ark_set_process_inb (_arkp, tid, tcbp);
    }
}

ark_set_start_err:

  return;
}

void ark_set_process_inb(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t *scbp = &(_arkp->poolthreads[tid]);

  ark_drop_pool(_arkp, &(scbp->poolstats), tcbp->hblk);

  ark_set_process(_arkp, tid, tcbp);

  return;
}

void ark_set_process(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t           *scbp        = &(_arkp->poolthreads[tid]);
  rcb_t           *rcbp        = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t           *iotcbp      = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t          *iocbp       = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t   *bl_array    = NULL;
  uint64_t         oldvlen     = 0;
  int32_t          rc          = 0;

  // Let's see if we need to grow the out buffer
  tcbp->old_btsize = tcbp->inb->len;
  rc = bt_growif(&(tcbp->oub), &(tcbp->oub_orig), &(tcbp->oublen),
                 (tcbp->blen * _arkp->bsize) +
                 (rcbp->klen + _arkp->vlimit));
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "bt_growif rc != 0, ttag:%3d", tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = rc;
    tcbp->state = ARK_CMD_DONE;
    goto ark_set_process_err;
  }

  // modify bucket
  if (tcbp->inb->cnt)
  {
      KV_TRC(pAT, "MOD_BT  tid:%d ttag:%3d klen:%5ld vlen:%5ld",
             tid, tcbp->ttag, rcbp->klen, rcbp->vlen);
  }
  tcbp->new_key = bt_set(tcbp->oub, tcbp->inb, rcbp->klen, rcbp->key,
                         rcbp->vlen, tcbp->vval, &oldvlen);

  // write the value to the value allocated blocks
  if (rcbp->vlen > _arkp->vlimit)
  {
    KV_TRC(pAT, "WR_VAL  tid:%d ttag:%3d vlen:%5ld",
           tid, tcbp->ttag, rcbp->vlen);

    bl_array = bl_chain(_arkp->bl, tcbp->vblk, tcbp->vblkcnt);
    if (bl_array == NULL)
    {
      KV_TRC_FFDC(pAT, "bl_chain failed ttag:%3d", tcbp->ttag);
      rcbp->rc = ENOMEM;
      rcbp->res = -1;
      tcbp->state = ARK_CMD_DONE;
      goto ark_set_process_err;
    }

    scbp->poolstats.byte_cnt -= oldvlen;
    scbp->poolstats.byte_cnt += rcbp->vlen;
    scbp->poolstats.io_cnt   += tcbp->vblkcnt;

    ea_async_io_init(_arkp, ARK_EA_WRITE, (void*)tcbp->vb,
                     bl_array, tcbp->vblkcnt, 0, tcbp->ttag, ARK_SET_WRITE);

    if (_arkp->ea->st_type == EA_STORE_TYPE_MEMORY)
    {
        ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
        ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
        if (iotcbp->state == ARK_SET_WRITE) {ark_set_write(_arkp, tid, tcbp);}
    }
  }
  else
  {
    ark_set_write(_arkp, tid, tcbp);
  }

ark_set_process_err:

  return;
}

void ark_set_write(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  rcb_t            *rcbp      = &(_arkp->rcbs[tcbp->rtag]);
  scb_t            *scbp      = &(_arkp->poolthreads[tid]);
  tcb_t            *iotcbp    = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t           *iocbp     = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t    *bl_array  = NULL;
  uint64_t          blkcnt    = 0;

  KV_TRC(pAT, "WR_KEY  tid:%d ttag:%3d klen:%5ld", tid, tcbp->ttag, rcbp->klen);

  // write obuf to new blocks
  blkcnt = divceil(tcbp->oub->len, _arkp->bsize);
  tcbp->nblk = ark_take_pool(_arkp, &(scbp->poolstats), blkcnt);
  if (tcbp->nblk == -1)
  {
    KV_TRC_FFDC(pAT, "ark_take_pool %ld failed ttag:%3d",
                blkcnt, tcbp->ttag);
    rcbp->rc = ENOSPC;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
    goto ark_set_write_err;
  }

  bl_array = bl_chain(_arkp->bl, tcbp->nblk, blkcnt);
  if (bl_array == NULL)
  {
    KV_TRC_FFDC(pAT, "bl_chain failed ttag:%3d", tcbp->ttag);
    rcbp->rc = ENOMEM;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
    goto ark_set_write_err;
  }

  scbp->poolstats.byte_cnt -= tcbp->old_btsize;
  scbp->poolstats.byte_cnt += tcbp->oub->len;
  scbp->poolstats.io_cnt   += blkcnt;

  ea_async_io_init(_arkp, ARK_EA_WRITE, (void *)tcbp->oub,
                   bl_array, blkcnt, 0, tcbp->ttag, ARK_SET_FINISH);

  if (_arkp->ea->st_type == EA_STORE_TYPE_MEMORY)
  {
      ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
      ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
      if (iotcbp->state == ARK_SET_FINISH) {ark_set_finish(_arkp, tid, tcbp);}
  }

ark_set_write_err:

  return;
}

void ark_set_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t     *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t     *rcbp     = &(_arkp->rcbs[tcbp->rtag]);

  KV_TRC(pAT, "HASHSET tid:%d ttag:%3d nblk:%5ld", tid, tcbp->ttag, tcbp->nblk);

  HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, tcbp->nblk));

  rcbp->res = rcbp->vlen;
  if ( 1 == tcbp->new_key )
  {
    scbp->poolstats.kv_cnt++;
  }
  tcbp->state = ARK_CMD_DONE;

  return;
}
