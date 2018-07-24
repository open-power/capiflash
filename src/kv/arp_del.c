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

#include <ark.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if success returns size of value deleted
void ark_del_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp       = &(scbp->rcbs[tcbp->rtag]);
  iocb_t        *iocbp      = &(scbp->iocbs[rcbp->ttag]);
  int32_t        rc         = 0;

  scbp->poolstats.ops_cnt += 1;

  // Acquire the block that contains the hash entry
  // control information.
  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  if (DUMP_KV)
  {
      char buf[256]={0};
      sprintf(buf, "HEX_KEY tid:%d ttag:%3d pos:%6ld",
              tid, tcbp->ttag, rcbp->pos);
      KV_TRC_HEX(pAT, 4, buf, rcbp->key, rcbp->klen);
  }

  // If there is no block, that means there are no
  // entries in the hash entry, which means they
  // key in question is not in the store.
  if (tcbp->hblk == 0)
  {
    KV_TRC(pAT, "ENOENT  tid:%d ttag:%3d pos:%6ld key:%p klen:%ld pos:%6ld",
           tid, tcbp->ttag, rcbp->pos, rcbp->key, rcbp->klen, rcbp->pos);
    rcbp->res    = -1;
    rcbp->rc     = ENOENT;
    iocbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }

  // Check to see if we need to grow the in buffer
  // to hold the hash entry
  tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);
  if (tcbp->blen <= 0)
  {
    KV_TRC(pAT, "ENOENT  tid:%d ttag:%3d pos:%6ld key:%p klen:%ld pos:%6ld",
           tid, tcbp->ttag, rcbp->pos, rcbp->key, rcbp->klen, rcbp->pos);
    rcbp->res   = -1;
    rcbp->rc    = ENOENT;
    iocbp->state = ARK_CMD_DONE;
    goto ark_del_start_err;
  }

  if (tcbp->blen*_arkp->bsize > tcbp->inb_size)
  {
      KV_TRC_DBG(pAT, "RE_INB  tid:%d ttag:%3d pos:%6ld old:%ld new:%ld",
                 tid, tcbp->ttag, rcbp->pos, tcbp->inb_size,
                 tcbp->blen*_arkp->bsize);
      rc = bt_realloc(&(tcbp->inb), &(tcbp->inb_orig), tcbp->blen*_arkp->bsize);
      if (rc != 0)
      {
          KV_TRC_FFDC(pAT, "bt_realloc failed ttag:%d", tcbp->ttag);
          rcbp->res = -1;
          rcbp->rc = rc;
          iocbp->state = ARK_CMD_DONE;
          goto ark_del_start_err;
      }
      tcbp->inb_size = tcbp->blen*_arkp->bsize;
  }

  if (kv_inject_flags && HTC_INUSE(_arkp,_arkp->htc[rcbp->pos]))
  {
      HTC_FREE(_arkp->htc[rcbp->pos]);
  }

  scbp->poolstats.io_cnt += tcbp->blen;

  if (HTC_HIT(_arkp, rcbp->pos, tcbp->blen))
  {
      ++scbp->htc_hits;
      KV_TRC(pAT, "HTC_GET tid:%d ttag:%3d pos:%6ld", tid,tcbp->ttag,rcbp->pos);
      HTC_GET(_arkp->htc[rcbp->pos], tcbp->inb, tcbp->blen*_arkp->bsize);
      ark_del_process(_arkp, tid, tcbp);
      return;
  }

  // Create a list of blocks that will be read in
  if (bl_rechain(&tcbp->aiol, _arkp->bl, tcbp->hblk, tcbp->blen, tcbp->aiolN))
  {
    rcbp->res   = -1;
    rcbp->rc    = ENOMEM;
    iocbp->state = ARK_CMD_DONE;
    KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
    goto ark_del_start_err;
  }
  tcbp->aiolN = tcbp->blen;

  KV_TRC(pAT, "RD_HASH tid:%d ttag:%3d pos:%6ld", tid, tcbp->ttag, rcbp->pos);

  // Schedule the IO to read the hash entry from storage
  ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_READ, (void *)tcbp->inb,
                   tcbp->aiol, tcbp->blen, 0, tcbp->ttag, ARK_DEL_PROCESS);

ark_del_start_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_del_process(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp       = &(scbp->rcbs[tcbp->rtag]);
  iocb_t        *iocbp      = &(scbp->iocbs[rcbp->ttag]);
  uint64_t       blkcnt     = 0;
  uint64_t       exivlen    = 0;
  uint64_t       oldvlen    = 0;
  int64_t        dblk       = 0;
  uint64_t       new_btsize = 0;
  int32_t        rc         = 0;

  tcbp->old_btsize = tcbp->inb->len;
  tcbp->bytes      = 0;

  KV_TRC_DBG(pAT, "INB_GET tid:%d ttag:%3d pos:%6ld tot:%ld used:%ld",
             tid, tcbp->ttag, rcbp->pos, tcbp->inb_size, tcbp->inb->len);

  if ((exivlen=bt_exists(tcbp->inb, rcbp->klen, rcbp->key)) >= 0 &&
      tcbp->inb->len > 1)
  {
      new_btsize = tcbp->inb->len - rcbp->klen - BT_KV_LEN_SZ -
                   (exivlen <= tcbp->inb->max ? exivlen : tcbp->inb->def);
      new_btsize = divceil(new_btsize, _arkp->bsize) * _arkp->bsize;
      if (new_btsize > tcbp->oub_size)
      {
          KV_TRC_DBG(pAT, "RE_OUB  tid:%d ttag:%3d pos:%6ld old:%ld new:%ld",
                     tid, tcbp->ttag, rcbp->pos, tcbp->oub_size, new_btsize);
          rc = bt_realloc(&(tcbp->oub), &(tcbp->oub_orig), new_btsize);
          if (rc != 0)
          {
              KV_TRC_FFDC(pAT, "bt_reallocif rc != 0, ttag:%3d", tcbp->ttag);
              rcbp->res   = -1;
              rcbp->rc    = rc;
              iocbp->state = ARK_CMD_DONE;
              goto ark_del_process_err;
          }
          tcbp->oub_size = new_btsize;
      }

    // Delete the key and value from the hash entry
    // and save off the length of the value
    rcbp->res = bt_del_def(tcbp->oub, tcbp->inb, rcbp->klen,
                           rcbp->key, (uint8_t*)&dblk, &oldvlen);

    KV_TRC(pAT,"EXISTS  hblk:%ld dblk:%ld", tcbp->hblk, dblk);

    // Return the blocks of the hash entry back to
    // the free list
    ark_drop_pool(_arkp, tid, tcbp->hblk);
    if (dblk > 0)
    {
      // Return the blocks used to store the value if it
      // wasn't stored in the hash entry
      ark_drop_pool(_arkp, tid, dblk);
      tcbp->bytes += oldvlen;
    }

    // Are there entries in the hash bucket.
    if (tcbp->oub->cnt > 0)
    {
      // Determine how many blocks will be needed for the
      // out buffer and then get them from the free
      // block list
      blkcnt     = divceil(tcbp->oub->len, _arkp->bsize);
      tcbp->nblk = ark_take_pool(_arkp, tid, blkcnt);
      if (tcbp->nblk == -1)
      {
        KV_TRC_FFDC(pAT, "ark_take_pool %ld failed ttag:%d",
                    blkcnt, tcbp->ttag);
        rcbp->rc    = ENOSPC;
        rcbp->res   = -1;
        iocbp->state = ARK_CMD_DONE;
        goto ark_del_process_err;
      }

      // Create a list of the blocks to write.
      if (bl_rechain(&tcbp->aiol, _arkp->bl, tcbp->nblk, blkcnt, tcbp->aiolN))
      {
        rcbp->res   = -1;
        rcbp->rc    = ENOMEM;
        iocbp->state = ARK_CMD_DONE;
        KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
        goto ark_del_process_err;
      }
      tcbp->aiolN = blkcnt;

      scbp->poolstats.io_cnt += blkcnt;
      tcbp->bytes            += tcbp->old_btsize;
      tcbp->bytes            -= tcbp->oub->len;

      KV_TRC(pAT, "WR_HASH tid:%d ttag:%3d pos:%6ld bytes:%ld",
                  tid, tcbp->ttag, rcbp->pos, tcbp->bytes);

      // Schedule the WRITE IO of the updated hash entry.
      ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_WRITE, (void *)tcbp->oub,
                       tcbp->aiol, blkcnt, 0, tcbp->ttag, ARK_DEL_FINISH);
    }
    else
    {
      // Nothing left in this hash entry, so let's clear out
      // the hash entry control block to show there is no
      // data in the store for this hash entry
      HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, 0));

      tcbp->bytes += tcbp->old_btsize;
      rcbp->rc     = 0;
      ark_del_finish(_arkp, tid, tcbp);
    }
  }
  else
  {
    KV_TRC(pAT, "ENOENT tid:%d ttag:%3d pos:%6ld key:%p klen:%ld pos:%6ld",
           tid, tcbp->ttag, rcbp->pos, rcbp->key, rcbp->klen, rcbp->pos);
    rcbp->rc     = ENOENT;
    rcbp->res    = -1;
    iocbp->state = ARK_CMD_DONE;
  }

ark_del_process_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_del_finish(_ARK *_arkp, int32_t tid, tcb_t *tcbp)
{
  scb_t  *scbp  = &(_arkp->poolthreads[tid]);
  rcb_t  *rcbp  = &(scbp->rcbs[tcbp->rtag]);
  iocb_t *iocbp = &(scbp->iocbs[rcbp->ttag]);

  scbp->poolstats.kv_cnt   -= 1;
  scbp->poolstats.byte_cnt -= tcbp->bytes;

  if (tcbp->oub->cnt > 0)
  {
    // Update the starting hash entry block with the
    // new block info.
    HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, tcbp->nblk));
    KV_TRC(pAT, "HASHSET tid:%d ttag:%3d pos:%6ld bytes:%ld byte_cnt:%ld",
           tid, tcbp->ttag, rcbp->pos, tcbp->bytes, scbp->poolstats.byte_cnt);

    if (HTC_INUSE(_arkp,_arkp->htc[rcbp->pos]))
    {
        HTC_PUT(_arkp->htc[rcbp->pos], tcbp->oub, tcbp->blen*_arkp->bsize);
    }
  }
  else
  {
      KV_TRC(pAT, "HASHSET tid:%d ttag:%3d pos:%6ld bytes:%ld byte_cnt:%ld "
                  "NO_WRITE_REQD",
             tid, tcbp->ttag, rcbp->pos, tcbp->bytes, scbp->poolstats.byte_cnt);

      if (HTC_INUSE(_arkp,_arkp->htc[rcbp->pos]))
      {
          --scbp->htcN;
          ++scbp->htc_disc;
          KV_TRC(pAT, "HTCFREE tid:%d ttag:%3d pos:%6ld htcN:%d disc:%ld",
                  tid, tcbp->ttag, rcbp->pos, scbp->htcN, scbp->htc_disc);
          HTC_FREE(_arkp->htc[rcbp->pos]);
      }
  }

  iocbp->state = ARK_CMD_DONE;
  return;
}

