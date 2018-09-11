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

#include <ark.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_set_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t   *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t   *rcbp       = &(scbp->rcbs[tcbp->rtag]);
  iocb_t  *iocbp      = &(scbp->iocbs[rcbp->ttag]);
  uint64_t new_vbsize = 0;
  uint8_t *new_vb     = NULL;
  int32_t  rc         = 0;
  uint8_t  lck        = 0;

  scbp->poolstats.ops_cnt += 1;
  tcbp->bytes              = 0;

  if (DUMP_KV)
  {
      char buf[256]={0};
      sprintf(buf, "HEX_KEY tid:%3d ttag:%3d pos:%6ld",
              tid, tcbp->ttag, rcbp->posI);
      KV_TRC_HEX(pAT, 4, buf, rcbp->key, rcbp->klen);
      sprintf(buf, "HEX_VAL tid:%3d ttag:%3d pos:%6ld",
              tid, tcbp->ttag, rcbp->posI);
      KV_TRC_HEX(pAT, 4, buf, rcbp->val, rcbp->vlen);
  }

  // Let's check to see if the value is larger than
  // what can fit inline in the bucket.
  if (rcbp->vlen > _arkp->vlimit)
  {
    // Determine the number of blocks needed to hold the value
    tcbp->vblkcnt = divceil(rcbp->vlen, _arkp->bsize);

    // Ok, the value will not fit inline in the has bucket.
    // Let's see if the per task value buffer is currently
    // large enough to hold this value.  If not, we will
    // need to grow it.
    if (rcbp->vlen > tcbp->vbsize)
    {
      // Calculate the new size of the value buffer.  The
      // size will be based on the block size, rounded up
      // to ensure enough space to hold value. We record
      // the size in a new variable and wait to make sure
      // the realloc succeeds before updating vbsize
      void *old_vb = tcbp->vb_orig;
      new_vbsize = tcbp->vblkcnt * _arkp->bsize;
      am_free(tcbp->vb_orig);
      new_vb = am_malloc(new_vbsize + ARK_ALIGN);
      KV_TRC_DBG(pAT, "RE_VB   tid:%3d ttag:%3d pos:%6ld vlen:%5ld "
                      "newsz:%ld old:%p new:%p",
          tid, tcbp->ttag, rcbp->posI, rcbp->vlen, new_vbsize, old_vb, new_vb);
      if (NULL == new_vb)
      {
        rcbp->res   = -1;
        rcbp->rc    = ENOMEM;
        iocbp->state = ARK_CMD_DONE;
        KV_TRC_FFDC(pAT, "realloc failed, ttag = %d rc = %d", tcbp->ttag, rc);
        goto ark_set_start_err;
      }
      tcbp->vbsize  = new_vbsize;
      tcbp->vb_orig = new_vb;
      tcbp->vb      = ptr_align(tcbp->vb_orig);
    }
    memcpy(tcbp->vb, rcbp->val, rcbp->vlen);
  }
  else
  {
    KV_TRC(pAT, "INLINE  tid:%3d ttag:%3d pos:%6ld vlen:%5ld",
           tid, tcbp->ttag, rcbp->posI, rcbp->vlen);
    tcbp->vval = (uint8_t *)rcbp->val;
  }

  HASH_GET(scbp->ht, rcbp->posI, &lck, &tcbp->hblk);

  if (0 == tcbp->hblk)
  {
    KV_TRC(pAT, "1ST_KEY tid:%3d ttag:%3d pos:%6ld", tid, tcbp->ttag, rcbp->posI);
    // This is going to be the first key in this bucket.
    // Initialize the bucket data/header
    bt_clear(tcbp->inb);
    bt_clear(tcbp->oub);
    tcbp->bytes += tcbp->oub->len; // add BT header if its the first

    // Call to the next phase of the SET command since
    // we do not need to read in the hash bucket since
    // this will be the first key in it.
    ark_set_process(_arkp, tid, tcbp);
    return;
  }
  else
  {
    // Look to see how many blocks are currently in use
    // for this hash bucket.
    tcbp->blen = bl_len(scbp->bl, tcbp->hblk);

    if (kv_inject_flags && HTC_INUSE(scbp,rcbp->posI))
    {
        HTC_FREE(scbp,rcbp->posI);
    }

    if (tcbp->blen*_arkp->bsize > tcbp->inb_size)
    {
        KV_TRC_DBG(pAT, "RE_INB  tid:%3d ttag:%3d pos:%6ld old:%ld new:%ld",
           tid, tcbp->ttag, rcbp->posI, tcbp->inb_size, tcbp->blen*_arkp->bsize);
        rc = bt_realloc(&tcbp->inb, &tcbp->inb_orig, tcbp->blen*_arkp->bsize);
        if (0 != rc)
        {
          rcbp->res    = -1;
          rcbp->rc     = rc;
          iocbp->state = ARK_CMD_DONE;
          KV_TRC_FFDC(pAT, "bt_reallocif failed, ttag:%3d rc:%d errno:%d",
                  tcbp->ttag, rc, errno);
          goto ark_set_start_err;
        }
        tcbp->inb_size = tcbp->blen*_arkp->bsize;
    }

    scbp->poolstats.io_cnt  += tcbp->blen;
    scbp->poolstats.hcl_cnt += 1;
    scbp->poolstats.hcl_tot += tcbp->blen;

    if (HTC_HIT(scbp, rcbp->posI))
    {
        ++scbp->htc_hits;
        KV_TRC(pAT,"HTC_HIT tid:%3d ttag:%3d pos:%6ld hits:%ld",
               tid, tcbp->ttag, rcbp->posI, scbp->htc_hits);
        HTC_GET(scbp, rcbp->posI, tcbp->inb);
        ark_set_process(_arkp, tid, tcbp);
        return;
    }

    // Create a list of the blocks that will be read in.
    // We do this upfront so we only have to access the block
    // list once.
    if (bl_rechain(&tcbp->aiol,scbp->bl,tcbp->hblk,tcbp->blen,tcbp->aiolN,scbp->offset))
    {
      rcbp->res    = -1;
      rcbp->rc     = ENOMEM;
      iocbp->state = ARK_CMD_DONE;
      KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
      goto ark_set_start_err;
    }
    tcbp->aiolN = tcbp->blen;

    KV_TRC(pAT, "RD_HASH tid:%3d ttag:%3d pos:%6ld hblk:%6ld nblks:%ld",
           tid, tcbp->ttag, rcbp->posI, tcbp->hblk, tcbp->blen);

    ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_READ,
                     (void *)tcbp->inb, tcbp->aiol, tcbp->blen,
                     0, tcbp->ttag, ARK_SET_PROCESS);
}

ark_set_start_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_set_process(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t           *scbp        = &(_arkp->poolthreads[tid]);
  rcb_t           *rcbp        = &(scbp->rcbs[tcbp->rtag]);
  iocb_t          *iocbp       = &(scbp->iocbs[rcbp->ttag]);
  uint64_t         oldvlen     = 0;
  uint64_t         oldvdf      = 0;
  uint64_t         oldvblkcnt  = 0;
  int64_t          exivlen     = 0;
  int32_t          rc          = 0;
  uint64_t         new_btsize  = 0;

  if (0 == tcbp->hblk) {tcbp->old_btsize = 0;}
  else                 {tcbp->old_btsize = tcbp->inb->len;}

  KV_TRC_DBG(pAT, "INB_GET tid:%3d ttag:%3d pos:%6ld tot:%ld used:%ld",
                  tid, tcbp->ttag, rcbp->posI, tcbp->inb_size, tcbp->old_btsize);

  // Let's see if we need to grow the out buffer
  if ((exivlen=bt_get_vdf(tcbp->inb,rcbp->klen,rcbp->key,(uint8_t*)&oldvdf))>=0)
  {
      if (exivlen <= tcbp->inb->max) {oldvdf=0;}
      new_btsize = tcbp->inb->len -
                   (exivlen <= tcbp->inb->max ? exivlen : 0) +
                   (rcbp->vlen <= tcbp->inb->max ? rcbp->vlen : 0);
      KV_TRC_DBG(pAT, "BT_EXI: tid:%3d ttag:%3d pos:%6ld new:%ld cur:%ld "
                      "used:%ld exivlen:%ld vlen:%ld ",
                      tid, tcbp->ttag, rcbp->posI, new_btsize, tcbp->inb_size,
                      tcbp->inb->len, exivlen, rcbp->vlen);
  }
  else
  {
      new_btsize = tcbp->inb->len + BT_KV_LEN_SZ + rcbp->klen +
              (rcbp->vlen > tcbp->inb->max ? tcbp->inb->def : rcbp->vlen);
      KV_TRC_DBG(pAT, "BT_ADD: tid:%3d ttag:%3d pos:%6ld new:%ld cur:%ld "
                      "used:%ld kl:%ld vl:%ld",
                      tid, tcbp->ttag, rcbp->posI, new_btsize, tcbp->inb_size,
                      tcbp->inb->len,rcbp->klen,rcbp->vlen);
  }

  new_btsize = divceil(new_btsize, _arkp->bsize) * _arkp->bsize;
  if (new_btsize > tcbp->oub_size)
  {
      KV_TRC_DBG(pAT, "RE_OUB  tid:%3d ttag:%3d pos:%6ld old:%ld new:%ld",
                 tid, tcbp->ttag, rcbp->posI, tcbp->oub_size, new_btsize);
      rc = bt_realloc(&(tcbp->oub), &(tcbp->oub_orig), new_btsize);
      if (rc != 0)
      {
          KV_TRC_FFDC(pAT, "bt_reallocif rc != 0, ttag:%3d", tcbp->ttag);
          rcbp->res   = -1;
          rcbp->rc    = rc;
          iocbp->state = ARK_CMD_DONE;
          goto ark_set_process_err;
      }
      tcbp->oub_size = new_btsize;
  }

  if (exivlen>0 && oldvdf)
  {
      KV_TRC_DBG(pAT, "INB_VDF tid:%3d ttag:%3d pos:%6ld oldvdf:%ld",
                 tid, tcbp->ttag, rcbp->posI, oldvdf);
  }

  /* if replacing an existing key/value, evaluate drop/take and vdf */
  if (exivlen>0)
  {
      oldvblkcnt = divceil(exivlen, _arkp->bsize);

      /* replace vdf with inline */
      if (exivlen > tcbp->inb->max && rcbp->vlen <= tcbp->inb->max)
      {
          ark_drop_pool(_arkp, tid, oldvdf);
          KV_TRC(pAT, "REP_VAL tid:%3d ttag:%3d pos:%6ld exivl:%ld vl:%ld "
                      "VDF>INLINE",
                  tid, tcbp->ttag, rcbp->posI, exivlen, rcbp->vlen);
      }
      /* replace inline with vdf */
      else if (exivlen <= tcbp->inb->max && rcbp->vlen > tcbp->inb->max)
      {
          tcbp->vblk = ark_take_pool(_arkp, tid, tcbp->vblkcnt);
          if (-1 == tcbp->vblk)
          {
            rcbp->res   = -1;
            rcbp->rc    = ENOSPC;
            iocbp->state = ARK_CMD_DONE;
            KV_TRC_FFDC(pAT, "FFDC    tid:%3d ttag:%3d pos:%6ld ark_take_pool "
                             "%ld failed rc = %d",
                             tid, tcbp->ttag, rcbp->posI, tcbp->vblkcnt, rc);
            goto ark_set_process_err;
          }
          tcbp->vval   = (uint8_t *)&(tcbp->vblk);
          KV_TRC(pAT, "REP_VAL tid:%3d ttag:%3d pos:%6ld exivl:%ld vl:%ld "
                      "INLINE>VDF",
                  tid, tcbp->ttag, rcbp->posI, exivlen, rcbp->vlen);
      }
      /* replace inline with inline */
      else if (exivlen <= tcbp->inb->max && rcbp->vlen <= tcbp->inb->max)
      {
          KV_TRC(pAT, "REP_VAL tid:%3d ttag:%3d pos:%6ld exivl:%ld vl:%ld "
                      "REP_INLINE",
                      tid, tcbp->ttag, rcbp->posI, exivlen, rcbp->vlen);
      }
      /* reuse the existing vdf */
      else if (tcbp->vblkcnt == oldvblkcnt)
      {
          KV_TRC(pAT, "REP_VAL tid:%3d ttag:%3d pos:%6ld vdf:%ld old:%ld new:%ld"
                      " REUSE_VDF",
                      tid, tcbp->ttag, rcbp->posI, oldvdf, oldvblkcnt, tcbp->vblkcnt);
          tcbp->vval   = (uint8_t*)&oldvdf;
          tcbp->vblk   = oldvdf;
      }
      /* replace old vdf with different size vdf */
      else
      {
          ark_drop_pool(_arkp, tid, oldvdf);
          tcbp->vblk = ark_take_pool(_arkp, tid, tcbp->vblkcnt);
          if (-1 == tcbp->vblk)
          {
            rcbp->res    = -1;
            rcbp->rc     = ENOSPC;
            iocbp->state = ARK_CMD_DONE;
            KV_TRC_FFDC(pAT, "FFDC    tid:%3d ttag:%3d pos:%6ld ark_take_pool "
                             "%ld failed rc = %d",
                             tid, tcbp->ttag, rcbp->posI, tcbp->vblkcnt, rc);
            goto ark_set_process_err;
          }
          tcbp->vval   = (uint8_t *)&(tcbp->vblk);
          KV_TRC(pAT, "REP_VAL tid:%3d ttag:%3d pos:%6ld old:%ld new:%ld REPLACE_VDF",
                      tid, tcbp->ttag, rcbp->posI, oldvblkcnt, tcbp->vblkcnt);
      }
  }
  /* new key with non-inlined value */
  else if (rcbp->vlen > tcbp->inb->max)
  {
      KV_TRC(pAT, "NEW_VAL tid:%3d ttag:%3d pos:%6ld vl:%ld max:%d",
                      tid, tcbp->ttag, rcbp->posI, rcbp->vlen, tcbp->inb->max);
      tcbp->vblk = ark_take_pool(_arkp, tid, tcbp->vblkcnt);
      if (-1 == tcbp->vblk)
      {
        rcbp->res    = -1;
        rcbp->rc     = ENOSPC;
        iocbp->state = ARK_CMD_DONE;
        KV_TRC_FFDC(pAT, "FFDC    tid:%3d ttag:%3d pos:%6ld ark_take_pool "
                         "%ld failed rc = %d",
                         tid, tcbp->ttag, rcbp->posI, tcbp->vblkcnt, rc);
        goto ark_set_process_err;
      }
      tcbp->vval = (uint8_t *)&(tcbp->vblk);
  }

  // modify bucket
  tcbp->new_key = bt_set(tcbp->oub, tcbp->inb, rcbp->klen, rcbp->key,
                         rcbp->vlen, tcbp->vval, &oldvdf, &oldvlen);

  /* calc byte difference for poolstats.byte_cnt */
  if (exivlen>0)
  {
      /* replace vdf with inline */
      if (exivlen > tcbp->inb->max && rcbp->vlen <= tcbp->inb->max)
      {
          tcbp->bytes += tcbp->oub->len - tcbp->inb->len - exivlen;
      }
      /* replace inline with vdf */
      else if (exivlen <= tcbp->inb->max && rcbp->vlen > tcbp->inb->max)
      {
          tcbp->bytes += tcbp->oub->len - tcbp->inb->len + rcbp->vlen;
      }
      /* replace inline with inline */
      else if (exivlen <= tcbp->inb->max && rcbp->vlen <= tcbp->inb->max)
      {
          tcbp->bytes += tcbp->oub->len - tcbp->inb->len;
      }
      /* replace vdf with vdf */
      else
      {
          tcbp->bytes += rcbp->vlen - exivlen;
      }
  }
  /* new key with non-inlined value */
  else if (rcbp->vlen > tcbp->inb->max)
  {
      tcbp->bytes += tcbp->oub->len - tcbp->inb->len + rcbp->vlen;
  }
  /* new key with inlined value */
  else
  {
      tcbp->bytes += tcbp->oub->len - tcbp->inb->len;
  }

  KV_TRC_DBG(pAT, "OUB_SET tid:%3d ttag:%3d pos:%6ld cnt:%d tot:%ld used:%ld "
                  "ovdf:%ld ovl:%ld add_bytes:%ld",
                  tid, tcbp->ttag, rcbp->posI, tcbp->oub->cnt, tcbp->oub_size,
                  tcbp->oub->len, oldvdf, oldvlen, tcbp->bytes);

  if (tcbp->new_key < 0)
  {
      KV_TRC_FFDC(pAT, "FFDC    tid:%3d ttag:%3d pos:%6ld bt_set failed "
                       "%ld failed rc = %d",
                       tid, tcbp->ttag, rcbp->posI, tcbp->vblkcnt,tcbp->new_key);
    rcbp->rc    = ENOMEM;
    rcbp->res   = -1;
    iocbp->state = ARK_CMD_DONE;
    goto ark_set_process_err;
  }

  /* if value is not inlined */
  if (rcbp->vlen > tcbp->oub->max)
  {
    KV_TRC(pAT, "WR_VAL  tid:%3d ttag:%3d pos:%6ld vlen:%5ld vblkcnt:%ld",
           tid, tcbp->ttag, rcbp->posI, rcbp->vlen, tcbp->vblkcnt);

    if (bl_rechain(&tcbp->aiol,scbp->bl,tcbp->vblk,tcbp->vblkcnt,tcbp->aiolN,scbp->offset))
    {
      rcbp->res    = -1;
      rcbp->rc     = ENOMEM;
      iocbp->state = ARK_CMD_DONE;
      KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
      goto ark_set_process_err;
    }
    tcbp->aiolN = tcbp->vblkcnt;

    scbp->poolstats.io_cnt += tcbp->vblkcnt;

    ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_WRITE, (void*)tcbp->vb,
                     tcbp->aiol, tcbp->vblkcnt, 0, tcbp->ttag, ARK_SET_WRITE);
  }
  else
  {
    ark_set_write(_arkp, tid, tcbp);
  }

ark_set_process_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_set_write(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t            *scbp      = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp      = &(scbp->rcbs[tcbp->rtag]);
  iocb_t           *iocbp     = &(scbp->iocbs[rcbp->ttag]);
  uint64_t          blkcnt    = 0;

  // write oub to new blocks
  blkcnt = divceil(tcbp->oub->len, _arkp->bsize);

  /* if a key block chain exists but it isn't the correct size, drop it */
  if (tcbp->hblk && blkcnt != tcbp->blen)
  {
      ark_drop_pool(_arkp, tid, tcbp->hblk);
      tcbp->hblk = 0;
  }

  /* if a key block chain doesn't exist, take one */
  if (tcbp->hblk == 0)
  {
      tcbp->nblk = ark_take_pool(_arkp, tid, blkcnt);
      if (tcbp->nblk == -1)
      {
        KV_TRC_FFDC(pAT, "ark_take_pool %ld failed ttag:%3d",
                    blkcnt, tcbp->ttag);
        rcbp->rc = ENOSPC;
        rcbp->res = -1;
        iocbp->state = ARK_CMD_DONE;
        goto ark_set_write_err;
      }
  }
  else
  {
      tcbp->nblk = tcbp->hblk;
  }

  if (bl_rechain(&tcbp->aiol, scbp->bl, tcbp->nblk, blkcnt, tcbp->aiolN,scbp->offset))
  {
    rcbp->res    = -1;
    rcbp->rc     = ENOMEM;
    iocbp->state = ARK_CMD_DONE;
    KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
    goto ark_set_write_err;
  }
  tcbp->aiolN = blkcnt;

  KV_TRC(pAT, "WR_KEY  tid:%3d ttag:%3d pos:%6ld klen:%5ld nblk:%ld blkcnt:%ld",
         tid, tcbp->ttag, rcbp->posI, rcbp->klen, tcbp->nblk, blkcnt);

  scbp->poolstats.io_cnt += blkcnt;

  ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_WRITE, (void *)tcbp->oub,
                   tcbp->aiol, blkcnt, 0, tcbp->ttag, ARK_SET_FINISH);

ark_set_write_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_set_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t     *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t     *rcbp     = &(scbp->rcbs[tcbp->rtag]);
  iocb_t    *iocbp    = &(scbp->iocbs[rcbp->ttag]);

  HASH_SET(scbp->ht, rcbp->posI, 1, tcbp->nblk);

  if (HTC_AVAIL(scbp, tcbp->oub->len))
  {
      if (HTC_INUSE(scbp,rcbp->posI))
      {
          KV_TRC(pAT, "HTC_SET tid:%3d ttag:%3d pos:%6ld htc_tot:%8ld sz:%ld",
                 tid, tcbp->ttag, rcbp->posI, scbp->htc_tot, tcbp->oub->len);
          HTC_SET(scbp, rcbp->posI, tcbp->oub);
      }
      else
      {
          ++scbp->htcN;
          KV_TRC(pAT, "HTC_NEW tid:%3d ttag:%3d pos:%6ld htc_tot:%8ld sz:%ld",
                 tid, tcbp->ttag, rcbp->posI, scbp->htc_tot, tcbp->oub->len);
          HTC_SET(scbp, rcbp->posI, tcbp->oub);
      }
  }
  else if (HTC_INUSE(scbp,rcbp->posI))
  {
      --scbp->htcN;
      ++scbp->htc_disc;
      KV_TRC(pAT, "HTCFREE tid:%3d ttag:%3d pos:%6ld htc_tot:%8ld disc:%ld",
                  tid, tcbp->ttag, rcbp->posI, scbp->htc_tot, scbp->htc_disc);
      HTC_FREE(scbp,rcbp->posI);
  }

  scbp->poolstats.kv_cnt   += tcbp->new_key;
  scbp->poolstats.byte_cnt += tcbp->bytes;
  rcbp->res                 = rcbp->vlen;
  iocbp->state              = ARK_CMD_DONE;

  KV_TRC(pAT, "HASHSET tid:%3d ttag:%3d pos:%6ld nblk:%5ld bytes:%ld "
              "byte_cnt:%ld",
              tid, tcbp->ttag, rcbp->posI, tcbp->nblk, tcbp->bytes,
              scbp->poolstats.byte_cnt);
  return;
}
