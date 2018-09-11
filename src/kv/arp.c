/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp.c $                                                */
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
 *   ark thread pool functions
 ******************************************************************************/
#include <ark.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_wait_cmd(_ARK *_arkp, rcb_t *rcbp, int *errcode, int64_t *res)
{
  scb_t   *scbp  = &(_arkp->poolthreads[rcbp->sthrd]);
  int rc = 0;

  pthread_mutex_lock(&(rcbp->alock));
  while (rcbp->stat != A_COMPLETE)
  {
    pthread_cond_wait(&(rcbp->acond), &(rcbp->alock));
  }

  if (rcbp->stat == A_COMPLETE)
  {
      *res       = rcbp->res;
      *errcode   = rcbp->rc;
      rcbp->stat = A_NULL;
      tag_bury(scbp->rtags, rcbp->rtag);
  }
  else
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "tag = %d rc = %d", rcbp->rtag, rc);
  }

  pthread_mutex_unlock(&(rcbp->alock));

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t ark_take_pool(_ARK *_arkp, int id, uint64_t n)
{
  scb_t       *scbp  = &(_arkp->poolthreads[id]);
  ark_stats_t *stats = &scbp->poolstats;
  int64_t      rc    = 0;
  int64_t      blk   = 0;

  bl_check_take(scbp->bl, n);

  if (bl_left(scbp->bl) < n && _arkp->flags & ARK_KV_VIRTUAL_LUN)
  {
      BL      *bl      = NULL;
      int64_t  cursize = scbp->size;
      int64_t  atleast = n * _arkp->bsize;
      int64_t  newsize = cursize + atleast + (_arkp->grow * _arkp->bsize);
      uint64_t newbcnt = newsize / _arkp->bsize;

      KV_TRC(pAT, "TK_RESZ tid:%3d n:%ld cur:%ld new:%ld",
                  id, n, cursize/_arkp->bsize, newbcnt);

      rc = ea_resize(scbp->ea, _arkp->bsize, newbcnt);
      if (rc)
      {
          rc = -1;
          KV_TRC_FFDC(pAT, "TK_RESZ tid:%3d rc:%ld n:%ld cur:%ld new:%ld ERROR",
                      id, rc, n, scbp->bcount, newbcnt);
          goto error;
      }
      bl = bl_resize(scbp->bl, newbcnt, scbp->bl->w);
      if (bl == NULL)
      {
          rc = -1;
          KV_TRC_FFDC(pAT, "TK_RESZ tid:%3d rc:%ld n:%ld cur:%ld new:%ld ERROR",
                      id, rc, n, scbp->bcount, newbcnt);
          goto error;
      }

      scbp->size = newsize;
      scbp->bl   = bl;
  }

  blk = bl_take(scbp->bl, n);
  if (blk <= 0)
  {
      rc = -1;
      KV_TRC_FFDC(pAT, "TK_POOL tid:%3d blk:%ld n:%ld bcount:%ld blk_cnt:%ld ERROR",
                  id, blk, n, scbp->bcount, stats->blk_cnt);
      goto error;
  }

  rc              = blk;
  stats->blk_cnt += n;

  KV_TRC_DBG(pAT, "TK_TAKE tid:%3d blk:%ld n:%ld bcount:%ld blk_cnt:%ld",
              id, blk, n, scbp->bcount, stats->blk_cnt);

error:
  if (rc <= 0) {KV_TRC_FFDC(pAT, "ERROR   tid:%3d blk:%ld", id, blk);}

  KV_TRC_EXT3(pAT, "TK_DONE tid:%3d blk:%ld blk_cnt:%ld", id,rc,stats->blk_cnt);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_drop_pool(_ARK *_arkp, int id, uint64_t blk)
{
  scb_t       *scbp  = &(_arkp->poolthreads[id]);
  ark_stats_t *stats = &scbp->poolstats;
  int64_t      nblks = 0;

  if (blk <= 0) {KV_TRC_FFDC(pAT, "bad blk:%ld", blk); return;}

  nblks = bl_drop(scbp->bl, blk);
  if (nblks == -1)
  {
      KV_TRC_FFDC(pAT, "ERROR   unable to free chain for blk:%ld", blk);
  }
  else
  {
      stats->blk_cnt -= nblks;
      KV_TRC_DBG(pAT, "TK_DROP tid:%3d blk:%ld n:%ld bcount:%ld blk_cnt:%ld",
                  id, blk, nblks, scbp->bcount, stats->blk_cnt);
  }

  KV_TRC_EXT3(pAT, "TK_DROP tid:%3d blk:%ld blk_cnt:%ld",id,blk,stats->blk_cnt);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_enq_cmd(int cmd, _ARK *_arkp,
                uint64_t klen, void *key,
                uint64_t vlen, void *val, uint64_t voff,
                void (*cb)(int,uint64_t,int64_t),
                uint64_t dt, int pthr, int *p, rcb_t **rcb)
{
  scb_t    *scbp  = NULL;
  int32_t   rtag  = -1;
  int       rc    = 0;
  int       pt    = 0;
  rcb_t    *rcbp  = NULL;
  queue_t  *rq    = NULL;
  uint64_t  pos   = 0;

  if (!_arkp || (klen>0 && !key))
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL cmd:%d ark:%p key:%p klen:%ld",
                cmd, _arkp, key, klen);
    rc = EINVAL;
    goto ark_enq_cmd_err;
  }

  if (pthr == -1)
  {
      /* use the key to find which thread will handle command */
      pos  = HASH_POS(_arkp->hcount, key, klen);
      pt   = pos / _arkp->hcount_per_thd;
      scbp = &(_arkp->poolthreads[pt]);
      KV_TRC_DBG(pAT, "UNBURY  tid:%3d scbp:%p pos:%7ld posI:%7ld",
                      pt, scbp, pos, pos - (_arkp->hcount_per_thd * pt));
  }
  else
  {
      /* use the thread specified for this cmd */
      pt   = pthr;
      scbp = &(_arkp->poolthreads[pt]);
      KV_TRC_DBG(pAT, "UNBURY  tid:%3d scbp:%p", pt, scbp);
  }

  rc = tag_unbury(scbp->rtags, &rtag);
  if (rc == 0)
  {
    rcbp        = &scbp->rcbs[rtag];
    rcbp->stime = getticks();
    rcbp->ark   = _arkp;
    rcbp->cmd   = cmd;
    rcbp->rtag  = rtag;
    rcbp->ttag  = -1;
    rcbp->stat  = A_INIT;
    rcbp->klen  = klen;
    rcbp->key   = key;
    rcbp->vlen  = vlen;
    rcbp->val   = val;
    rcbp->voff  = voff;
    rcbp->cb    = cb;
    rcbp->dt    = dt;
    rcbp->rc    = 0;
    rcbp->res   = -1;
    rcbp->sthrd = pt;
    rcbp->thdI  = pt;

    if (cmd == K_RAND)
    {
        pos         = (pt*scbp->hcount) + scbp->rand_htI;
        rcbp->htI   = scbp->rand_htI;
        rcbp->posI  = scbp->rand_htI;
        rcbp->btI   = scbp->rand_btI;
        rcbp->state = ARK_RAND_FINISH;
        KV_TRC_DBG(pAT,"K_RAND  rcb:%p thdI:%d htI:%ld btI:%d",
                 rcbp,_arkp->rand_thdI, scbp->rand_htI, scbp->rand_btI);
        if (p && *p==1) {*p=0; scbp->poolstats.ops_cnt+=1;}
    }
    else if (cmd == K_NEXT)
    {
        _ARI *arip = val;
        pos        = (pt*scbp->hcount) + arip->htI;
        rcbp->posI = arip->htI;
        rcbp->htI  = arip->htI;
        rcbp->btI  = arip->btI;
        if (arip->getN)
        {
            rcbp->vbep = arip->vbep;
            rcbp->end  = (uint8_t*)key + klen;
            rcbp->getN = arip->getN;
            rcbp->cnt  = arip->cnt;
            rcbp->full = 0;
        }
        rcbp->state = ARK_NEXT_FINISH;
        KV_TRC_DBG(pAT,"K_NEXT  rcb:%p thdI:%d htI:%ld btI:%ld buf:%p "
                        "buflen:%ld vbep:%p getN:%d",
                 rcbp, arip->thdI, arip->htI, arip->btI, key, klen,
                 rcbp->vbep, rcbp->getN);
        if (p && *p==1) {*p=0; scbp->poolstats.ops_cnt+=1;}
    }
    else
    {
        rcbp->posI = pos - (scbp->hcount * pt);
    }

    if (rcb) {*rcb=rcbp;}

    rq = scbp->reqQ;

    KV_TRC_DBG(pAT, "IO_NEW  tid:%3d rtag:%d cmd:%d pos:%7ld posI:%7ld kl:%ld vl:%ld "
                    "val:%p rq:%p RQ_ENQ NEW_REQ ",
         rcbp->sthrd,rtag,cmd,pos,rcbp->htI,rcbp->klen,rcbp->vlen,rcbp->val,rq);

    queue_lock(rq);
    queue_enq_unsafe(rq, rtag);
    queue_unlock(rq);
  }
  else
  {
    KV_TRC_DBG(pAT, "NO_TAG: %lx rc:%d", dt, rc);
  }

ark_enq_cmd_err:

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_next_pool(_ARK *_arkp, int id, tcb_t *tcbp)
{
  ark_io_list_t *bl_array = NULL;
  scb_t         *scbp     = &(_arkp->poolthreads[id]);
  rcb_t         *rcbp     = &(scbp->rcbs[tcbp->rtag]);
  iocb_t        *iocbp    = &(scbp->iocbs[tcbp->ttag]);
  int32_t        ea_rc    = 0;
  uint8_t       *buf      = NULL;
  uint64_t       hblk     = 0;
  uint64_t       pkl      = 0;
  uint64_t       pvl      = 0;
  int64_t        blen     = 0;
  BT            *bt       = NULL;
  BT            *bt_orig  = NULL;
  uint8_t        lck      = 0;
  int            i        = 0;
  uint8_t       *next     = NULL;

  KV_TRC_DBG(pAT, "START   tid:%3d rcbp:%p thdI:%d htI:%ld btI:%ld cnt:%ld",
                  id,rcbp,rcbp->thdI,rcbp->htI,rcbp->btI,rcbp->cnt);

  rcbp->res = -1;
  rcbp->rc  = EAGAIN;

  // if this thread has no keys
  if (scbp->poolstats.kv_cnt == 0)
  {
      KV_TRC_DBG(pAT, "EAGAIN  tid:%3d thdI:%d htI:%ld btI:%ld kv_cnt=0",
                      id,rcbp->thdI,rcbp->htI,rcbp->btI);
      goto done;
  }

  // Now that we have the starting point, loop
  // through the buckets for this thread and find
  // the first bucket with blocks
  for (; rcbp->htI<scbp->hcount; rcbp->htI++)
  {
      HASH_GET(scbp->ht, rcbp->htI, &lck, &hblk);

      if (hblk<1) {KV_TRC_DBG(pAT, "skip htI:%ld", rcbp->htI); continue;}
      else        {break;}
  }

  if (rcbp->htI == scbp->hcount)
  {
      KV_TRC_DBG(pAT, "NXT_THD tid:%3d htI:%ld", id, rcbp->htI);
      goto done;
  }

  // Found an entry with blocks
  blen = bl_len(scbp->bl, hblk);
  bt   = bt_new(divceil(blen * _arkp->bsize, _arkp->bsize) * _arkp->bsize,
                _arkp->vlimit, 8, &bt_orig);
  if (!bt)
  {
      rcbp->res = -1;
      rcbp->rc  = ENOMEM;
      KV_TRC_FFDC(pAT, "Bucket create failed blen %ld", blen);
      goto done;
  }

  buf      = (uint8_t *)bt;
  bl_array = bl_chain(scbp->bl, hblk, blen, scbp->offset);
  if (!bl_array)
  {
      rcbp->res = -1;
      rcbp->rc  = ENOMEM;
      bt        = NULL;
      KV_TRC_FFDC(pAT, "Can not create block list %ld", blen);
      goto done;
  }

  scbp->poolstats.io_cnt += blen;

  ea_rc = ea_async_io(scbp->ea, ARK_EA_READ, buf, bl_array, blen);
  am_free(bl_array);
  if (ea_rc != 0)
  {
      rcbp->res = -1;
      rcbp->rc  = ea_rc;
      bt        = NULL;
      KV_TRC_FFDC(pAT, "IO failure for READ rc = %d", ea_rc);
      goto done;
  }

  KV_TRC_DBG(pAT, "foundBT tid:%3d htI:%ld btI:%ld bt->cnt:%d",
                  id, rcbp->htI, rcbp->btI, bt->cnt);

  uint8_t *addr = bt->data;

  // move to the current key index
  for (i=0; i<rcbp->btI; i++)
  {
      addr += vi_dec64(addr, &pkl);
      addr += vi_dec64(addr, &pvl);
      addr += pkl;
      addr += (pvl>bt->max ? bt->def : pvl);
  }

  if (rcbp->getN)
  {
      // while more keys and room in buf
      while (rcbp->btI < bt->cnt)
      {
          addr += vi_dec64(addr, &pkl);
          addr += vi_dec64(addr, &pvl);
          rcbp->vbep->len = pkl;
          memcpy(rcbp->vbep->p, addr, rcbp->vbep->len);
          next = (uint8_t*)BMP_VBE(rcbp->vbep);
          rcbp->vbep = (vbe_t*)next;
          rcbp->cnt += 1;
          rcbp->btI += 1;
          if (next >= rcbp->end) {rcbp->full=1; break;}
          addr += pkl;
          addr += (pvl>bt->max ? bt->def : pvl);
      }
  }
  else
  {
      addr += vi_dec64(addr, &pkl);
      addr += vi_dec64(addr, &pvl);
      memcpy(rcbp->key, addr, (pkl > rcbp->klen? rcbp->klen : pkl));
      rcbp->res  = pkl;
      rcbp->cnt += 1;
      rcbp->btI += 1;
  }

  rcbp->rc     = 0;
  rcbp->bt_cnt = bt->cnt;
  if (rcbp->btI==bt->cnt) {bt_delete(bt_orig); bt_orig=NULL;}

done:
  if (bt_orig)  {bt_delete(bt_orig);}
  KV_TRC_DBG(pAT, "DONE    tid:%3d thdI:%d htI:%ld btI:%ld rc:%d cnt:%ld",
                  id,rcbp->thdI,rcbp->htI,rcbp->btI, rcbp->rc, rcbp->cnt);
  iocbp->state = rcbp->state;
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_rand_finish(_ARK *_arkp, int id, tcb_t *tcbp)
{
    scb_t  *scbp  = &_arkp->poolthreads[id];
    rcb_t  *rcbp  = &scbp->rcbs[tcbp->rtag];
    iocb_t *iocbp = &(scbp->iocbs[tcbp->ttag]);

    if (rcbp->rc == EAGAIN)
    {
        _arkp->rand_thdI = (id+1) % _arkp->nthrds;
        scbp->rand_htI   = 0;
        scbp->rand_btI   = 0;
        KV_TRC_DBG(pAT,"NEXT_TD tid:%3d thdI:%d htI:%ld btI:%d",
             id,_arkp->rand_thdI, scbp->rand_htI, scbp->rand_btI);
    }
    else if (rcbp->btI == rcbp->bt_cnt)
    {
        uint64_t hte     = ((id*scbp->hcount)+rcbp->htI+1) % _arkp->hcount;
        _arkp->rand_thdI = hte / scbp->hcount;
        scbp->rand_htI   = hte % scbp->hcount;
        scbp->rand_btI   = 0;
        KV_TRC_DBG(pAT,"NEXT_HT tid:%3d thdI:%d hte:%ld htI:%ld btI:%d",
             id,_arkp->rand_thdI, hte, scbp->rand_htI, scbp->rand_btI);
    }
    else
    {
        scbp->rand_htI = rcbp->htI;
        scbp->rand_btI  = rcbp->btI;
        KV_TRC_DBG(pAT,"NEXT_BT tid:%3d thdI:%d htI:%ld btI:%d",
             id,_arkp->rand_thdI, scbp->rand_htI, scbp->rand_btI);
    }
    iocbp->state = ARK_CMD_DONE;
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_first_finish(_ARK *_arkp, int id, tcb_t *tcbp)
{
    scb_t  *scbp  = &_arkp->poolthreads[id];
    rcb_t  *rcbp  = &scbp->rcbs[tcbp->rtag];
    iocb_t *iocbp = &(scbp->iocbs[tcbp->ttag]);
    _ARI  *arip   = rcbp->val;

    if (rcbp->rc == EAGAIN)
    {
        // move to the next thread
        arip->thdI = id+1;
        arip->htI  = 0;
        arip->btI  = 0;
        KV_TRC_DBG(pAT,"NEXT_TD tid:%3d thdI:%d htI:%ld btI:%ld",
             id,arip->thdI, arip->htI, arip->btI);
    }
    else if (rcbp->btI == rcbp->bt_cnt)
    {
        uint64_t hte = ((id*scbp->hcount)+rcbp->htI+1) % _arkp->hcount;
        arip->thdI   = hte / scbp->hcount;
        arip->htI    = hte % scbp->hcount;
        arip->btI    = 0;
        KV_TRC_DBG(pAT,"NEXT_HT tid:%3d thdI:%d htI:%ld btI:%ld",
             id,arip->thdI, arip->htI, arip->btI);
    }
    else
    {
        arip->htI = rcbp->htI;
        arip->btI = rcbp->btI;
        KV_TRC_DBG(pAT,"NEXT_BT tid:%3d thdI:%d htI:%ld btI:%ld",
             id,arip->thdI, arip->htI, arip->btI);
    }
    iocbp->state = ARK_CMD_DONE;
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_next_finish(_ARK *_arkp, int id, tcb_t *tcbp)
{
    scb_t  *scbp  = &_arkp->poolthreads[id];
    rcb_t  *rcbp  = &scbp->rcbs[tcbp->rtag];
    iocb_t *iocbp = &(scbp->iocbs[tcbp->ttag]);
    _ARI  *arip   = rcbp->val;

    if (rcbp->getN)
    {
        rcbp->res = rcbp->cnt;
        if (rcbp->full) {arip->full=1;}
    }

    if (rcbp->rc == EAGAIN)
    {
        arip->thdI = id+1;
        arip->htI  = 0;
        arip->btI  = 0;
        KV_TRC_DBG(pAT,"NEXT_TD tid:%3d thdI:%d htI:%ld btI:%ld full:%d",
             id,arip->thdI, arip->htI, arip->btI, arip->full);
    }
    else if (rcbp->btI == rcbp->bt_cnt)
    {
        uint64_t hte = (id*scbp->hcount)+rcbp->htI+1;
        arip->thdI   = hte / scbp->hcount;
        arip->htI    = hte % scbp->hcount;
        arip->btI    = 0;
        KV_TRC_DBG(pAT,"NEXT_HT tid:%3d thdI:%d htI:%ld btI:%ld full:%d",
             id,arip->thdI, arip->htI, arip->btI, arip->full);
    }
    else
    {
        arip->htI = rcbp->htI;
        arip->btI = rcbp->btI;
        KV_TRC_DBG(pAT,"NEXT_BT tid:%3d thdI:%d htI:%ld btI:%ld full:%d",
             id,arip->thdI, arip->htI, arip->btI, arip->full);
    }
    iocbp->state = ARK_CMD_DONE;
    return;
}

/**
 *******************************************************************************
 * \brief
 *  reduce memory footprint if possible after an io is complete
 ******************************************************************************/
void tcb_cleanup(_ARK *_arkp, int tid, int ttag)
{
    int    i    = 0;
    scb_t *scbp = &(_arkp->poolthreads[tid]);
    tcb_t *tcbp = &scbp->tcbs[ttag];

    KV_TRC_DBG(pAT, "CHK_CLN tid:%3d",tid);
    if (tcbp->aiol && tcbp->aiolN > ARK_MIN_AIOL)
    {
        void *old   = tcbp->aiol;
        tcbp->aiolN = ARK_MIN_AIOL;
        tcbp->aiol  = am_malloc(sizeof(ark_io_list_t)*tcbp->aiolN);
        am_free(old);
        KV_TRC(pAT,"RED_IOL tid:%3d ttag:%3d %ld to %d old:%p new:%p",
                tid, i, tcbp->aiolN, ARK_MIN_AIOL, old, tcbp->aiol);
        if (!tcbp->aiol)
        {
            KV_TRC(pAT,"FFDC    tid:%3d ttag:%3d %ld to %d",
                    tid, i, tcbp->aiolN, ARK_MIN_AIOL);
        }
    }
    if (tcbp->inb_orig && tcbp->inb_size > _arkp->min_bt)
    {
        KV_TRC(pAT,"RED_INB tid:%3d ttag:%3d %ld to %d",
                tid, i, tcbp->inb_size, _arkp->min_bt);
        bt_delete(tcbp->inb_orig);
        tcbp->inb = bt_new(_arkp->min_bt,_arkp->vlimit,VDF_SZ,&tcbp->inb_orig);
        if (!tcbp->inb)
        {
            KV_TRC(pAT,"RED_INB tid:%3d ttag:%3d sz:%d", tid,i,_arkp->min_bt);
        }
        tcbp->inb_size = _arkp->min_bt;
    }
    if (tcbp->oub_orig && tcbp->oub_size > _arkp->min_bt)
    {
        KV_TRC(pAT,"RED_OUB tid:%3d ttag:%3d %ld to %d",
                        tid, i, tcbp->oub_size, _arkp->min_bt);
        bt_delete(tcbp->oub_orig);
        tcbp->oub = bt_new(_arkp->min_bt,_arkp->vlimit,VDF_SZ,&tcbp->oub_orig);
        if (!tcbp->oub)
        {
            KV_TRC(pAT,"RED_INB tid:%3d ttag:%3d sz:%d", tid,i,_arkp->min_bt);
        }
        tcbp->oub_size = _arkp->min_bt;
    }
    if (tcbp->vb_orig && tcbp->vbsize > _arkp->bsize*ARK_MIN_VB)
    {
        void    *old   = tcbp->vb_orig;
        uint64_t oldsz = tcbp->vbsize;
        tcbp->vbsize   = _arkp->bsize*ARK_MIN_VB;
        tcbp->vb_orig  = am_malloc(tcbp->vbsize + ARK_ALIGN);
        am_free(old);
        KV_TRC(pAT,"RED_VB  tid:%3d ttag:%3d %ld to %ld old:%p new:%p",
                tid, i, oldsz, tcbp->vbsize, old, tcbp->vb_orig);
        if (tcbp->vb_orig) {tcbp->vb = ptr_align(tcbp->vb_orig);}
        else
        {
            KV_TRC_FFDC(pAT,"RED_VB  tid:%3d ttag:%3d %ld to %ld",
                        tid, i, tcbp->vbsize, _arkp->bsize*ARK_MIN_VB);
            tcbp->vbsize = 0;
            tcbp->vb     = NULL;
        }
    }
    KV_TRC_DBG(pAT, "END_CLN tid:%3d",tid);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int get_task_state(int32_t cmd)
{
  int tstate = 0;

  switch (cmd)
  {
    case K_GET:   {tstate = ARK_GET_START;   break;}
    case K_SET:   {tstate = ARK_SET_START;   break;}
    case K_DEL:   {tstate = ARK_DEL_START;   break;}
    case K_EXI:   {tstate = ARK_EXIST_START; break;}
    case K_RAND:  {tstate = ARK_RAND_START;  break;}
    case K_FIRST: {tstate = ARK_FIRST_START; break;}
    case K_NEXT:  {tstate = ARK_NEXT_START;  break;}
    default:
      // we should never get here
      KV_TRC_FFDC(pAT, "FFDC, bad rcbp->cmd: %d", cmd);
      break;
  }

  return tstate;
}

/**
 *******************************************************************************
 * \brief
 *   calc and log the perf stats for the previous window of time
 ******************************************************************************/
static __inline__ void PERF_LOG(_ARK *_arkp, scb_t *scbp, int id)
{
    if (SDELTA(scbp->perflogtime, _arkp->ns_per_tick) < 1) {return;}

    uint64_t opsT = scbp->set_opsT + scbp->get_opsT + scbp->exi_opsT +\
                    scbp->del_opsT;
    uint64_t latT = scbp->set_latT + scbp->get_latT + scbp->exi_latT +\
                    scbp->del_latT;
    uint64_t lat=0,set_lat=0,get_lat=0,exi_lat=0,del_lat=0,QDA=0;
    if (opsT)            {lat     = latT/opsT;}
    if (scbp->set_opsT) {set_lat = scbp->set_latT/scbp->set_opsT;}
    if (scbp->get_opsT) {get_lat = scbp->get_latT/scbp->get_opsT;}
    if (scbp->exi_opsT) {exi_lat = scbp->exi_latT/scbp->exi_opsT;}
    if (scbp->del_opsT) {del_lat = scbp->del_latT/scbp->del_opsT;}
    if (scbp->QDT)      {QDA     = scbp->QDT/scbp->QDN;}
    KV_TRC_PERF1(pAT,"IO_PERF tid:%3d opsT:%7ld lat:%7ld "
                     "opsT(%7ld %7ld %7ld %7ld) "
                     "lat(%7ld %6ld %6ld %6ld) QDA:%4ld issT:%4d",
                     id, opsT, lat,
                     scbp->set_opsT, scbp->get_opsT, scbp->exi_opsT,
                     scbp->del_opsT,
                     set_lat, get_lat, exi_lat, del_lat,
                     QDA, scbp->issT);

    KV_TRC_PERF2(pAT,"IO_QS   tid:%3d rq:%3d tq:%3d sq:%4d hq:%4d "
                     "cq:%4d",
                     id, scbp->reqQ->c, scbp->taskQ->c, scbp->scheduleQ->c,
                     scbp->harvestQ->c, scbp->cmpQ->c);
    uint64_t hcl_tot = scbp->poolstats.hcl_tot;
    double   cl      = (double)hcl_tot/(double)scbp->poolstats.hcl_cnt;
    KV_TRC_PERF2(pAT,"PSTATS  tid:%3d kv:%8ld ops:%8ld ios:%8ld cl:%5.1lf "
                     "bytes:%10ld blks:%8ld avail:%8ld htc_tot:%8ld hit:%9ld "
                     "disc:%8ld",
                 id,
                 scbp->poolstats.kv_cnt,
                 scbp->poolstats.ops_cnt,
                 scbp->poolstats.io_cnt,
                 cl,
                 scbp->poolstats.byte_cnt,
                 scbp->poolstats.blk_cnt,
                 scbp->bl->n-1 - (scbp->bl->top-scbp->bl->count),
                 scbp->htc_tot, scbp->htc_hits, scbp->htc_disc);

    /* reset all counters to recalc for the next window of time */
    scbp->set_latT  = 0;
    scbp->get_latT  = 0;
    scbp->exi_latT  = 0;
    scbp->del_latT  = 0;
    scbp->set_opsT  = 0;
    scbp->get_opsT  = 0;
    scbp->exi_opsT  = 0;
    scbp->del_opsT  = 0;
    scbp->QDT       = 0;
    scbp->QDN       = 0;
    scbp->htc_hits  = 0;
    scbp->htc_disc  = 0;

    scbp->perflogtime = getticks();
}

/**
 *******************************************************************************
 * \brief
 *   function run by each ark thread
 ******************************************************************************/
void *pool_function(void *arg)
{
  PT      *pt     = (PT*)arg;
  int      id     = pt->id;
  _ARK    *_arkp  = pt->ark;
  scb_t   *scbp   = &(_arkp->poolthreads[id]);
  queue_t *rq     = scbp->reqQ;
  queue_t *cq     = scbp->cmpQ;
  queue_t *tq     = scbp->taskQ;
  queue_t *sq     = scbp->scheduleQ;
  queue_t *hq     = scbp->harvestQ;
  int32_t  i      = 0;
  uint64_t tmiss  = 0;
  uint32_t MAX_POLL=0;
  uint64_t verbT  = 0;
  ticks    iticks = 0;

  KV_TRC_DBG(pAT, "start tid:%3d nactive:%d", id, _arkp->nactive);

  /*----------------------------------------------------------------------------
   * Run until the thread state is EXIT
   *--------------------------------------------------------------------------*/
  while (scbp->poolstate != PT_EXIT)
  {
      /*------------------------------------------------------------------------
       * harvest IOs
       *----------------------------------------------------------------------*/
      MAX_POLL = Q_CNT(hq);
      if (MAX_POLL) {MAX_POLL = MAX_POLL<10 ? 1 : MAX_POLL/10;}

      for (i=0; i<MAX_POLL; i++)
      {
          iocb_t *iocbp = NULL;
          int32_t ttag  = 0;

          if (queue_deq_unsafe(hq, &ttag) != 0) {continue;}
          iocbp = &(scbp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "HQ_DEQ  tid:%3d ttag:%3d state:%d",
                     id, ttag, iocbp->state);

          if (iocbp->state == ARK_IO_HARVEST)
          {
              ea_async_io_harvest(_arkp, id, iocbp);
              if (iocbp->hmissN) {++tmiss;}
          }

          // place the iocb on a queue for the next step
          if      (iocbp->state==ARK_IO_SCHEDULE) {queue_enq_unsafe(sq, ttag);}
          else if (iocbp->state==ARK_IO_HARVEST)  {queue_enq_unsafe(hq, ttag);}
          else if (iocbp->state==ARK_CMD_DONE)    {queue_enq_unsafe(cq, ttag);}
          else                                    {queue_enq_unsafe(tq, ttag);}
      }

      /*------------------------------------------------------------------------
       * schedule IOs
       *----------------------------------------------------------------------*/
      int sN=sq->c;
      for (i=0; i<sN; i++)
      {
          iocb_t *iocbp = NULL;
          int32_t ttag  = 0;

          if (queue_deq_unsafe(sq, &ttag) != 0) {continue;}
          iocbp = &(scbp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "SQ_DEQ  tid:%3d ttag:%3d state:%d",
                     id, ttag, iocbp->state);

          if (iocbp->state == ARK_IO_SCHEDULE)
              {ea_async_io_schedule(_arkp, id, iocbp);}

          // place the iocb on a queue for the next step
          if      (iocbp->state==ARK_IO_SCHEDULE) {queue_enq_unsafe(sq, ttag);}
          else if (iocbp->state==ARK_IO_HARVEST)  {queue_enq_unsafe(hq, ttag);}
          else if (iocbp->state==ARK_CMD_DONE)    {queue_enq_unsafe(cq, ttag);}
          else                                    {queue_enq_unsafe(tq, ttag);}
          if (iocbp->eagain) {iocbp->eagain=0; break;}
      }

      /*------------------------------------------------------------------------
       * check the request queue and try to pull off
       * as many requests as possible and queue them up
       * in the task queue
       *----------------------------------------------------------------------*/
      queue_lock(rq);
      if (!rq->c && !tq->c && !sq->c && !hq->c && !cq->c)
      {
          ticks delta;
          iticks = getticks();
          // We have reached a point where there is absolutely
          // no work for this worker thread to do.  So we
          // go to sleep waiting for new requests to come in
          queue_wait(rq);
          delta = MDELTA(iticks,_arkp->ns_per_tick);
          KV_TRC_DBG(pAT, "IO:     tid:%3d IDLE_THREAD iticks:%ld", id, delta);
      }

      int rN=rq->c;
      for (i=0; i<rN; i++)
      {
          rcb_t   *rcbp  = NULL;
          tcb_t   *tcbp  = NULL;
          iocb_t  *iocbp = NULL;
          int32_t  rtag  = 0;
          int32_t  ttag  = 0;
          int32_t  tskrc = EAGAIN;
          uint64_t hblk  = 0;
          uint8_t  lck   = 0;

          if (queue_deq_unsafe(rq, &rtag)) {break;}

          rcbp       = &(scbp->rcbs[rtag]);
          rcbp->rtag = rtag;
          rcbp->ttag = -1;
          KV_TRC_EXT3(pAT, "RQ_DEQ  tid:%3d rtag:%3d pos:%7ld", id, rtag, rcbp->posI);
          HASH_GET(scbp->ht, rcbp->posI, &lck, &hblk);
          if (lck)
          {
              queue_enq_unsafe(rq, rtag);
              KV_TRC_DBG(pAT,"IO_RERQ tid:%3d rtag:%3d pos:%7ld HASH_LOCK",
                              id, rtag, rcbp->posI);
              continue;
          }

          tskrc = tag_get(scbp->ttags, &ttag);
          if (tskrc == EAGAIN)
          {
              KV_TRC_DBG(pAT,"IO_RERQ tid:%3d rtag:%3d NO_TAG", id, rtag);
              queue_enq_unsafe(rq, rtag);
              break;
          }
          if (tskrc)
          {
              KV_TRC_DBG(pAT,"IO_RERQ tid:%3d rtag:%3d rc:%d", id, rtag, tskrc);
              queue_enq_unsafe(rq, rtag);
              break;
          }

          tcbp            = &(scbp->tcbs[ttag]);
          iocbp           = &(scbp->iocbs[ttag]);
          tcbp->rtag      = rtag;
          rcbp->ttag      = ttag;
          iocbp->state    = get_task_state(rcbp->cmd);
          iocbp->io_error = 0;
          tcbp->sthrd     = rcbp->sthrd;
          tcbp->ttag      = ttag;
          tcbp->new_key   = 0;
          HASH_SET(scbp->ht, rcbp->posI, 1, hblk);
          (void)queue_enq_unsafe(tq, ttag);
          KV_TRC_DBG(pAT, "IO:     tid:%3d ttag:%3d rtag:%d TQ_ENQ START ",
                     id, ttag, rtag);
      }
      queue_unlock(rq);

      /* do calcs for avg QD */
      scbp->QDT += rq->c + cq->c + tq->c + sq->c + hq->c;
      scbp->QDN += 1;

      /*------------------------------------------------------------------------
       * initiate or continue tasks
       *----------------------------------------------------------------------*/
      int tN=tq->c;
      for (i=0; i<tN; i++)
      {
          tcb_t  *tcbp  = NULL;
          iocb_t *iocbp = NULL;
          int32_t ttag  = 0;
          int     s     = 0;

          if (queue_deq_unsafe(tq, &ttag) != 0) {continue;}

          tcbp       = &(scbp->tcbs[ttag]);
          iocbp      = &(scbp->iocbs[ttag]);
          tcbp->ttag = ttag;
          s          = iocbp->state;

          KV_TRC_EXT3(pAT, "TQ_DEQ  tid:%3d ttag:%3d state:%d ", id, ttag, s);

          if      (s==ARK_SET_START)      {ark_set_start      (_arkp,id,tcbp);}
          else if (s==ARK_SET_PROCESS)    {ark_set_process    (_arkp,id,tcbp);}
          else if (s==ARK_SET_WRITE)      {ark_set_write      (_arkp,id,tcbp);}
          else if (s==ARK_SET_FINISH)     {ark_set_finish     (_arkp,id,tcbp);}
          else if (s==ARK_GET_START)      {ark_get_start      (_arkp,id,tcbp);}
          else if (s==ARK_GET_PROCESS)    {ark_get_process    (_arkp,id,tcbp);}
          else if (s==ARK_GET_FINISH)     {ark_get_finish     (_arkp,id,tcbp);}
          else if (s==ARK_DEL_START)      {ark_del_start      (_arkp,id,tcbp);}
          else if (s==ARK_DEL_PROCESS)    {ark_del_process    (_arkp,id,tcbp);}
          else if (s==ARK_DEL_FINISH)     {ark_del_finish     (_arkp,id,tcbp);}
          else if (s==ARK_EXIST_START)    {ark_exist_start    (_arkp,id,tcbp);}
          else if (s==ARK_EXIST_FINISH)   {ark_exist_finish   (_arkp,id,tcbp);}
          else if (s==ARK_RAND_START)     {ark_next_pool      (_arkp,id,tcbp);}
          else if (s==ARK_FIRST_START)    {ark_next_pool      (_arkp,id,tcbp);}
          else if (s==ARK_NEXT_START)     {ark_next_pool      (_arkp,id,tcbp);}
          else if (s==ARK_RAND_FINISH)    {ark_rand_finish    (_arkp,id,tcbp);}
          else if (s==ARK_FIRST_FINISH)   {ark_first_finish   (_arkp,id,tcbp);}
          else if (s==ARK_NEXT_FINISH)    {ark_next_finish    (_arkp,id,tcbp);}

          /*--------------------------------------------------------------------
           * schedule the first IO right way
           *------------------------------------------------------------------*/
          if (iocbp->state == ARK_IO_SCHEDULE)
          {
              iocbp = &(scbp->iocbs[ttag]);
              ea_async_io_schedule(_arkp, id, iocbp);
          }

          /*--------------------------------------------------------------------
           * requeue task
           *------------------------------------------------------------------*/
          if (iocbp->state == ARK_IO_SCHEDULE)
          {
              KV_TRC_EXT3(pAT, "SQ_ENQ  tid:%3d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(sq,ttag);
          }
          else if (iocbp->state == ARK_IO_HARVEST)
          {
              KV_TRC_EXT3(pAT, "HQ_ENQ  tid:%3d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(hq,ttag);
          }
          else if (iocbp->state == ARK_CMD_DONE)
          {
              KV_TRC_EXT3(pAT, "CQ_ENQ  tid:%3d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(cq,ttag);
          }
          else
          {
              KV_TRC_EXT3(pAT, "TQ_ENQ  tid:%3d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              (void)queue_enq_unsafe(tq, ttag);
          }
      }

      /*------------------------------------------------------------------------
       * complete requests
       *----------------------------------------------------------------------*/
      int cN=cq->c;
      for (i=0; i<cN; i++)
      {
          rcb_t  *rcbp = NULL;
          tcb_t  *tcbp = NULL;
          iocb_t *iocbp = NULL;
          int32_t rtag = 0;
          int32_t ttag = 0;

          if (queue_deq_unsafe(cq, &ttag) != 0) {continue;}

          tcbp  = &(scbp->tcbs[ttag]);
          rtag = tcbp->rtag;
          rcbp  = &(scbp->rcbs[rtag]);
          iocbp = &(scbp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "CQ_DEQ  tid:%3d ttag:%3d state:%d ",
                      id, ttag, iocbp->state);

          if (iocbp->state == ARK_CMD_DONE)
          {
              uint64_t hblk = 0;
              uint8_t  lck  = 0;
              uint32_t lat  = UDELTA(rcbp->stime, _arkp->ns_per_tick);
              UPDATE_LAT(scbp, rcbp, lat);

              if (iocbp->io_error)
              {
                   rcbp->res = -1;
                   rcbp->rc  = iocbp->io_error;
              }
              HASH_GET(scbp->ht, rcbp->posI, &lck, &hblk);
              HASH_SET(scbp->ht, rcbp->posI, 0, hblk);

              if (rcbp->cb)
              {
                  KV_TRC(pAT, "CMP_CB  tid:%3d ttag:%3d rtag:%3d cmd:%d "
                              "rc:%d res:%7ld lat:%6d",
                              id, ttag, rcbp->rtag, rcbp->cmd,
                              rcbp->rc,rcbp->res,lat);
                  (rcbp->cb)(rcbp->rc, rcbp->dt, rcbp->res);
                  rcbp->stat = A_NULL;
                  tag_bury(scbp->rtags, rtag);
              }
              else
              {
                  KV_TRC(pAT, "CMP_SIG tid:%3d ttag:%3d rtag:%3d cmd:%d "
                              "rc:%d res:%7ld lat:%6d",
                              id, ttag, rcbp->rtag, rcbp->cmd,
                              rcbp->rc,rcbp->res,lat);
                  pthread_mutex_lock(&(rcbp->alock));
                  rcbp->stat = A_COMPLETE;
                  pthread_cond_signal(&(rcbp->acond));
                  pthread_mutex_unlock(&(rcbp->alock));
              }
              tcb_cleanup(_arkp,id,ttag);
              tag_put(scbp->ttags, ttag);
          }
          else
          {
              KV_TRC_EXT3(pAT, "FFDC CQ tid:%3d ttag:%3d state:%d UNKNOWN_STATE",
                      id, ttag, iocbp->state);
          }
      }

      /*------------------------------------------------------------------------
       * performance and dynamic tracing
       *----------------------------------------------------------------------*/
      if (pAT->verbosity >= 1)
      {
          PERF_LOG(_arkp, scbp, id);

          /* check for dynamic verbosity every 5 seconds */
          if (SDELTA(verbT, _arkp->ns_per_tick) > 5)
          {
              struct stat sbuf;
              FILE  *fp        = NULL;
              char   verbS[2]  = {0};
              int    verbI     = 0;

              verbT = getticks();

              if (stat(ARK_VERB_FN,&sbuf) == 0)
              {
                  if ((fp=fopen(ARK_VERB_FN,"r")))
                  {
                      if (fgets(verbS, 2, fp))
                      {
                          verbI = atoi(verbS);
                          if (verbI > 0 && verbI < 10) {pAT->verbosity=verbI;}
                          fclose(fp);
                      }
                  }
              }
          }
      }
  }

  scbp->perflogtime = 0;
  PERF_LOG(_arkp, scbp, id);

  KV_TRC(pAT, "exiting tid:%3d nactive:%d", id, _arkp->nactive);
  return NULL;
}
