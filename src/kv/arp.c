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
int ark_wait_tag(_ARK *_arkp, int tag, int *errcode, int64_t *res)
{
  int rc = 0;
  rcb_t *rcbp = &(_arkp->rcbs[tag]);

  KV_TRC(pAT, "WAIT    tag:%d", tag);

  pthread_mutex_lock(&(rcbp->alock));
  while (rcbp->stat != A_COMPLETE)
  {
    pthread_cond_wait(&(rcbp->acond), &(rcbp->alock));
  }

  KV_TRC_EXT3(pAT, "WAKEUP  tag:%d", tag);

  if (rcbp->stat == A_COMPLETE)
  {
      *res       = rcbp->res;
      *errcode   = rcbp->rc;
      rcbp->stat = A_NULL;
      tag_bury(_arkp->rtags, tag);
      KV_TRC_DBG(pAT, "TAG_CMP tag:%d", tag);
  }
  else
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "tag = %d rc = %d", tag, rc);
  }

  pthread_mutex_unlock(&(rcbp->alock));

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_anyreturn(_ARK *_arkp, int *tag, int64_t *res)
{
  int i;
  int astart = _arkp->astart;
  int nasyncs = _arkp->nasyncs;
  int itag = -1;

  for (i = 0; i < _arkp->nasyncs; i++)
  {
      itag = (astart + i) % nasyncs;
      if (_arkp->rcbs[itag].stat == A_COMPLETE)
      {
          *tag = itag;
          *res = _arkp->rcbs[(astart+i) % nasyncs].res;
          _arkp->rcbs[itag].stat = A_NULL;
          tag_bury(_arkp->rtags, itag);
          return 0;
      }
  }
  _arkp->astart++;
  _arkp->astart %= nasyncs;
  return EINVAL;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t ark_take_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t n)
{
  int64_t  i   = 0;
  int64_t  rc  = 0;
  uint64_t blk = 0;

  pthread_mutex_lock(&_arkp->mainmutex);

  /* if there are enough blocks in the unmap list, use them */
  if (_arkp->blu->count >= n)
  {
      /* move n blocks 1 at a time from unmap list */
      for (i=0; i<n; i++)
      {
          blk = bls_rem(_arkp->blu);
          bl_drop(_arkp->bl, blk);
      }
      blk = bl_take(_arkp->bl, n);
      KV_TRC_EXT3(pAT, "TK_UM   blk:%ld n:%ld bl:%ld blu:%ld",
                  blk, n, _arkp->bl->count, _arkp->blu->count);
      goto blu_done;
  }

  bl_check_take(_arkp->bl, n);

  if (bl_left(_arkp->bl) < n)
  {
      BL      *bl      = NULL;
      BL      *blu     = NULL;
      int64_t  cursize = _arkp->size;
      int64_t  atleast = n * _arkp->bsize;
      int64_t  newsize = cursize + atleast + (_arkp->grow * _arkp->bsize);
      uint64_t newbcnt = newsize / _arkp->bsize;

      KV_TRC(pAT, "TK_RESZ n:%ld cur:%ld new:%ld",
                  n, cursize/_arkp->bsize, newbcnt);

      rc = ea_resize(_arkp->ea, _arkp->bsize, newbcnt);
      if (rc)
      {
          rc = -1;
          goto error;
      }
      bl = bl_resize(_arkp->bl, newbcnt, _arkp->bl->w);
      if (bl == NULL)
      {
          rc = -1;
          goto error;
      }
      blu = bl_resize(_arkp->blu, newbcnt, _arkp->blu->w);
      if (blu == NULL)
      {
          rc = -1;
          goto error;
      }

      _arkp->size = newsize;
      _arkp->bl   = bl;
      _arkp->blu  = blu;
  }

  blk = bl_take(_arkp->bl, n);
  if (blk <= 0)
  {
      rc = -1;
      goto error;
  }

blu_done:
  rc = blk;

  _arkp->blkused += n;
  stats->blk_cnt += n;

  KV_TRC_EXT3(pAT, "BL_STAT INC blk:%ld nblks:%ld used:%ld cnt:%ld",
              blk, n, _arkp->blkused, stats->blk_cnt);

error:
  pthread_mutex_unlock(&_arkp->mainmutex);

  if (rc <= 0) {KV_TRC_FFDC(pAT, "ERROR");}

  KV_TRC_EXT3(pAT, "BL_TAKE blk:%ld used:%ld", rc, _arkp->blkused);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_drop_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t blk)
{
  int64_t  nblks = 0;
  int      i     = 0;
  uint64_t tblk  = 0;

  if (blk <= 0) {KV_TRC_FFDC(pAT, "bad blk:%ld", blk); return;}

  pthread_mutex_lock(&_arkp->mainmutex);

  nblks = bl_drop(_arkp->bl, blk);
  if (nblks == -1)
  {
      KV_TRC_FFDC(pAT, "ERROR   unable to free chain for blk:%ld", blk);
  }
  else
  {
      _arkp->blkused -= nblks;
      stats->blk_cnt -= nblks;
      KV_TRC_EXT3(pAT, "BL_STAT DEC blk:%ld nblks:%ld used:%ld cnt:%ld",
                  blk, nblks, _arkp->blkused, stats->blk_cnt);
  }

  /*------------------------------------------------------------------------
   * if unmap is supported, mv UNMAP blks from bl to blu
   *----------------------------------------------------------------------*/
  if (_arkp->ea->unmap)
  {
      for (i=0; i<nblks; i++)
      {
          /* take single blk */
          if ((tblk=bl_take(_arkp->bl,1)) <= 0)
          {
              KV_TRC_FFDC(pAT, "ERROR   bl_take i:%d nblks:%ld", i, nblks);
              break;
          }
          KV_TRC_EXT3(pAT, "UM_bl   TAKE bl:%p i:%d blk:%ld nblks:%ld",
                      _arkp->bl, i, tblk, nblks);

          /* drop single blk to blu */
          if (bls_add(_arkp->blu,tblk) <= 0)
          {
              KV_TRC_EXT3(pAT, "UM_blu  DROP bl:%p i:%d blk:%ld nblks:%ld",
                          _arkp->blu, i, tblk, nblks);
              bl_drop(_arkp->bl,tblk);
              KV_TRC_FFDC(pAT, "ERROR   bl_drop i:%d blk:%ld", i, tblk);
              break;
          }
          KV_TRC_EXT3(pAT, "UM_blu  DROP blk:%ld", tblk);
      }
  }

  pthread_mutex_unlock(&_arkp->mainmutex);

  KV_TRC_EXT3(pAT, "BL_DROP blk:%ld used:%ld", blk, _arkp->blkused);
}

/**
 *******************************************************************************
 * \brief
 *  schedule/harvest/complete unmaps
 ******************************************************************************/
void process_unmapQ(_ARK *_arkp, int id, int iss)
{
    scb_t   *scbp  = &(_arkp->poolthreads[id]);
    iocb_t  *iocbp = NULL;
    queue_t *usq   = scbp->uscheduleQ;
    queue_t *uhq   = scbp->uharvestQ;
    int32_t  utag  = 0;
    int32_t  max   = ARK_UQ_SIZE;
    int      i     = 0;
    uint64_t blk   = 0;

    if (!_arkp->blu->count && !usq->c && !uhq->c) {return;}

    max = _arkp->blu->count>max ? max : _arkp->blu->count;

    /*------------------------------------------------------------------------
     * process blu and queue UNMAPs
     *----------------------------------------------------------------------*/
    for (i=0; iss && i<max; i++)
    {
        _arkp->utime = getticks();
        if (EAGAIN == tag_unbury(_arkp->utags, &utag)) {break;}

        if (i==0)
        {
            KV_TRC_IO(pAT, "UM_Q    tid:%d max:%d blu:%ld",
                      id, max, _arkp->blu->count);
        }

        iocb_t        *iocbp = &(_arkp->ucbs[utag]);
        ark_io_list_t *aiol = NULL;

        if (!(aiol=am_malloc(sizeof(ark_io_list_t))))
        {
            tag_bury(_arkp->utags, utag);
            break;
        }

        if ((blk=bls_rem(_arkp->blu)) <= 0)
        {
            tag_bury(_arkp->utags, utag);
            break;
        }
        aiol[0].blkno     = blk;
        aiol[0].a_tag.tag = -1;
        ea_async_io_init(_arkp, iocbp, ARK_EA_UNMAP, (void *)_arkp->ea->zbuf,
                         aiol, 1, 0, utag, ARK_CMD_DONE);
        queue_enq_unsafe(usq, utag);
    }

    /*------------------------------------------------------------------------
     * harvest UNMAPs
     *----------------------------------------------------------------------*/
    max=uhq->c;
    for (i=0; i<max; i++)
    {
        if (queue_deq_unsafe(uhq, &utag)) {break;}

        iocbp = &(_arkp->ucbs[utag]);

        if (iocbp->state==ARK_IO_HARVEST) {ea_async_io_harvest(_arkp,id,iocbp);}

        if      (iocbp->state==ARK_IO_SCHEDULE) {queue_enq_unsafe(usq, utag);}
        else if (iocbp->state==ARK_IO_HARVEST)  {queue_enq_unsafe(uhq, utag);}
        else if (iocbp->state==ARK_CMD_DONE)
        {
            pthread_mutex_lock(&_arkp->mainmutex);
            if (bl_drop(_arkp->bl, iocbp->blist[0].blkno) <= 0)
               {KV_TRC_FFDC(pAT, "ERROR   bl_drop:%ld",iocbp->blist[0].blkno);}
            pthread_mutex_unlock(&_arkp->mainmutex);

            am_free(iocbp->blist);
            tag_bury(_arkp->utags, iocbp->tag);
            _arkp->um_opsT += 1;
            _arkp->um_latT += iocbp->lat;
        }
    }

    /*------------------------------------------------------------------------
     * schedule UNMAPs
     *----------------------------------------------------------------------*/
    max=usq->c;
    for (i=0; i<max; i++)
    {
        if (queue_deq_unsafe(usq, &utag)) {break;}

        iocbp = &(_arkp->ucbs[utag]);

        if (iocbp->state==ARK_IO_SCHEDULE) {ea_async_io_schedule(_arkp,id,iocbp);}

        if      (iocbp->state==ARK_IO_SCHEDULE) {queue_enq_unsafe(usq, utag);}
        else if (iocbp->state==ARK_IO_HARVEST)  {queue_enq_unsafe(uhq, utag);}
        else if (iocbp->state==ARK_CMD_DONE)
        {
            pthread_mutex_lock(&_arkp->mainmutex);
            if (bl_drop(_arkp->bl, iocbp->blist[0].blkno) <= 0)
               {KV_TRC_FFDC(pAT, "ERROR   bl_drop:%ld",iocbp->blist[0].blkno);}
            pthread_mutex_unlock(&_arkp->mainmutex);

            am_free(iocbp->blist);
            tag_bury(_arkp->utags, iocbp->tag);
            _arkp->um_opsT += 1;
            _arkp->um_latT += iocbp->lat;
        }
        if (iocbp->eagain) {iocbp->eagain=0; break;}
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_enq_cmd(int cmd, _ARK *_arkp, uint64_t klen, void *key,
                uint64_t vlen,void *val,uint64_t voff,
                void (*cb)(int errcode, uint64_t dt,int64_t res),
                uint64_t dt, int pthr, int *ptag)
{
  int32_t   rtag = -1;
  int       rc = 0;
  int       pt = 0;
  rcb_t    *rcbp = NULL;
  queue_t  *rq   = NULL;

  if ( (_arkp == NULL) || ((klen > 0) && (key == NULL)))
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL cmd:%d ark:%p key:%p klen:%ld",
                cmd, _arkp, key, klen);
    rc = EINVAL;
    goto ark_enq_cmd_err;
  }

  rc = tag_unbury(_arkp->rtags, &rtag);
  if (rc == 0)
  {
    rcbp        = &(_arkp->rcbs[rtag]);
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

    // If the caller has asked to run on a specific thread, then
    // do so.  Otherwise, use the key to find which thread
    // will handle command
    if (pthr == -1)
    {
      rcbp->pos   = hash_pos(_arkp->ht, key, klen);
      rcbp->sthrd = rcbp->pos / _arkp->npart;
      pt          = rcbp->pos / _arkp->npart;
    }
    else
    {
      rcbp->sthrd = pthr;
      pt = pthr;
    }

    rq = _arkp->poolthreads[pt].reqQ;
    queue_lock(rq);
    queue_enq_unsafe(rq, rtag);
    queue_unlock(rq);

    KV_TRC_DBG(pAT, "IO_NEW  tid:%d rtag:%d cmd:%d pos:%7ld kl:%ld vl:%ld rq:%p"
                    " RQ_ENQ NEW_REQ ",
                 rcbp->sthrd, rtag, cmd, rcbp->pos, rcbp->klen, rcbp->vlen, rq);
  }
  else
  {
    KV_TRC_DBG(pAT, "NO_TAG: %lx rc:%d", dt, rc);
  }

ark_enq_cmd_err:

  // If the caller is interested in the tag, give it back to
  // them.  If they aren't, no need to set it
  if (ptag != NULL)
  {
    *ptag = rtag;
  }
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_rand_pool(_ARK *_arkp, int id, tcb_t *tcbp)
{
  // find the hashtable position
  rcb_t   *rcbp  = &(_arkp->rcbs[tcbp->rtag]);
  iocb_t  *iocbp = &(_arkp->iocbs[tcbp->ttag]);
  int32_t  found = 0;
  int32_t  i = 0;
  int32_t  ea_rc = 0;
  uint8_t  *buf = NULL;
  uint8_t  *pos = NULL;
  uint64_t hblk;
  uint64_t pkl;
  uint64_t pvl;
  int64_t  blen = 0;
  void     *kbuf = rcbp->key;
  BT       *bt = NULL;
  BT       *bt_orig = NULL;
  ark_io_list_t *bl_array = NULL;
  scb_t    *scbp = &(_arkp->poolthreads[id]);

  // Check to see if this thead has any keys to begin with.
  // If it doesn't reset the rlast field and return immediately.
  if (scbp->poolstats.kv_cnt == 0)
  {
    rcbp->res = -1;
    scbp->rlast = -1;
    rcbp->rc = EAGAIN;
    goto ark_rand_pool_err;
  }

  // Check to see if we start off at the beginning
  // of the thread's monitored hash buckets or
  // if we need to pick up where we left off last time
  i = scbp->rlast;
  if ( i == -1 )
  {
    i = id;
  }

  // Now that we have the starting point, loop
  // through the buckets for this thread and find
  // the first bucket with blocks
  for (; (i < _arkp->hcount) && (!found); i += _arkp->nthrds)
  {

    hblk = HASH_LBA(HASH_GET(_arkp->ht, i));

    // Found an entry with blocks
    if (hblk>0)
    {
      blen = bl_len(_arkp->bl, hblk);
      bt = bt_new(divceil(blen * _arkp->bsize, 
                                 _arkp->bsize) * _arkp->bsize, 
                                 _arkp->vlimit, 8, &bt_orig);
      if (bt == NULL)
      {
        rcbp->res = -1;
        rcbp->rc = ENOMEM;
        KV_TRC_FFDC(pAT, "Bucket create failed blen %"PRIi64"", blen);
        goto ark_rand_pool_err;
      }

      buf = (uint8_t *)bt;
      bl_array = bl_chain(_arkp->bl, hblk, blen);
      if (bl_array == NULL)
      {
        rcbp->res = -1;
        rcbp->rc = ENOMEM;
        KV_TRC_FFDC(pAT, "Can not create block list %"PRIi64"", blen);
        bt_delete(bt_orig);
        goto ark_rand_pool_err;
      }

      ea_rc = ea_async_io(_arkp->ea, ARK_EA_READ, (void *)buf, 
                          bl_array, blen, _arkp->nthrds);
      if (ea_rc != 0)
      {
        rcbp->res = -1;
        rcbp->rc = ea_rc;
        free(bl_array);
        bt_delete(bt_orig);
        bt = NULL;
        KV_TRC_FFDC(pAT, "IO failure for READ rc = %d", ea_rc);
        goto ark_rand_pool_err;
      }

      free(bl_array);

      found = 1;
    }
  }

  if (found)
  {
    // Start position at the beginning of the bucket
    pos = bt->data;

    // If we've made it here, we have our bucket, whether it be 
    // a new bucket from a new hash table entry, or the old 
    // bucket that still has keys that haven't been processed.
    pos += vi_dec64(pos, &pkl);
    pos += vi_dec64(pos, &pvl);

    // Record the true length of the key
    rcbp->res = pkl;

    // Copy the key into the buffer for the passed in length
    memcpy(kbuf, pos, (pkl > rcbp->klen? rcbp->klen : pkl));

    // Delete the bucket since we are now done with it
    bt_delete(bt_orig);

    // This will be the starting point on the next call to ark_random
    // for this thread
    scbp->rlast = i + _arkp->nthrds;
  }
  else
  {
    rcbp->res = -1;

    // We didn't find a key here, so return EAGAIN so that
    // ark_random can determine if it wants to try on the
    // next pool thread.  Also reset the start point
    // for this thread
    scbp->rlast = -1;
    rcbp->rc = EAGAIN;
  }

ark_rand_pool_err:
  iocbp->state = ARK_CMD_DONE;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *  arp->res returns the length of the key\n
 *  arp->val is for the ARI structure...both for passing in and returning.
 ******************************************************************************/
void ark_first_pool(_ARK *_arkp, int id, tcb_t *tcbp)
{
  rcb_t   *rcbp  = &(_arkp->rcbs[tcbp->rtag]);
  iocb_t  *iocbp = &(_arkp->iocbs[tcbp->ttag]);
  _ARI *_arip = NULL;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  found = 0;
  uint8_t  *buf = NULL;
  uint8_t  *pos = NULL;
  uint64_t hblk;
  uint64_t pkl;
  uint64_t pvl;
  int64_t  blen = 0;
  void     *kbuf = rcbp->key;
  ark_io_list_t *bl_array = NULL;

  _arip = (_ARI *)rcbp->val;
  _arip->ark = _arkp;
  _arip->bt = NULL;
  _arip->hpos = 0;
  _arip->key = 0;
  _arip->pos = NULL;

  // Now that we have the starting point, loop
  // through the buckets for this thread and find
  // the first bucket with blocks
  for (i = id; (i < _arkp->hcount) && (!found); i += _arkp->nthrds)
  {
    hblk = HASH_LBA(HASH_GET(_arkp->ht, i));

    // Found an entry with blocks
    if (hblk>0)
    {
      // Remember the hash table entry.  Will start from
      // this point.
      _arip->hpos = i;        

      // Allocate a new bucket
      blen = bl_len(_arkp->bl, hblk);
      _arip->bt = bt_new(divceil(blen * _arkp->bsize, 
                                 _arkp->bsize) * _arkp->bsize, 
                                 _arkp->vlimit, 8,
                                 &(_arip->bt_orig));
      if (_arip->bt == NULL)
      {
        rcbp->rc = ENOMEM;
        rcbp->res = -1;
        goto ark_first_pool_err;
      }

      buf = (uint8_t *)_arip->bt;

      bl_array = bl_chain(_arkp->bl, hblk, blen);
      if (bl_array == NULL)
      {
        rcbp->rc = ENOMEM;
        bt_delete(_arip->bt_orig);
        _arip->bt_orig = NULL;
        _arip->bt = NULL;
        rcbp->res = -1;
        goto ark_first_pool_err;
      }

      // Iterate over all the blocks and read them from
      // the storage into the bucket.
      rc = ea_async_io(_arkp->ea, ARK_EA_READ, (void *)buf, 
                       bl_array, blen, _arkp->nthrds);
      if (rc != 0)
      {
        KV_TRC_FFDC(pAT, "FFDC, errno = %d", errno);
        free(bl_array);
        bt_delete(_arip->bt_orig);
        _arip->bt_orig = NULL;
        _arip->bt = NULL;
        rcbp->rc = rc;
        rcbp->res = -1;
        goto ark_first_pool_err;
      }

      free(bl_array);

      found = 1;
    }
  }

  // If bt == NULL, that either means we didn't find a
  // hash bucket with a key or we ran into an error.
  if (_arip->bt == NULL) {
    if (rc == 0)
    {
      // If rc == 0, that means we didn't find a hash
      // table.  Set the error EAGAIN so the caller
      // can retry on a different thread.
      rcbp->rc = EAGAIN;
    }
    rcbp->res = -1;
    goto ark_first_pool_err;
  }

  // Look for the first key in the bucket
  pos = _arip->bt->data;
  pos += vi_dec64(pos, &pkl);
  pos += vi_dec64(pos, &pvl);

  // Record the true length of the key
  rcbp->res = pkl;

  // Copy the key into the buffer for the passed in length
  memcpy(kbuf, pos, (pkl > rcbp->klen) ? rcbp->klen : pkl);

  // Are we done with this bucket or do we remember which key we
  // left off on for next call.
  if (_arip->bt->cnt > 1)
  {
    _arip->key++;
  }
  else
  {
    // If there are no more keys in this hash bucket
    // then jump to the next monitored hash bucket for this thread.
    _arip->hpos += _arkp->nthrds;
    _arip->key = 0;
  }

ark_first_pool_err:

  if (_arip->bt != NULL)
  {
    bt_delete(_arip->bt_orig);
    _arip->bt_orig = NULL;
    _arip->bt = NULL;
  }

  iocbp->state = ARK_CMD_DONE;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *  arp->res returns the length of the key\n
 *  arp->val is for the ARI structure...both for passing in and returning.
 ******************************************************************************/
void ark_next_pool(_ARK *_arkp, int id, tcb_t *tcbp)
{
  rcb_t    *rcbp  = &(_arkp->rcbs[tcbp->rtag]);
  iocb_t   *iocbp = &(_arkp->iocbs[tcbp->ttag]);
  _ARI     *_arip = (_ARI *)rcbp->val;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  found = 0;
  int32_t  kcnt = 0;
  uint8_t  *buf = NULL;
  uint8_t  *pos = NULL;
  uint64_t hblk;
  uint64_t pkl;
  uint64_t pvl;
  int64_t  blen = 0;
  void     *kbuf = rcbp->key;
  ark_io_list_t *bl_array = NULL;
  
  rcbp->res = -1;

  // We can be in a few different scenarios when we enter
  // here.
  //
  // 1. We are starting out with a new thread, so we
  //    start from the beginning of it's monitored
  //    hash buckets.
  //    _arip->key == 0;
  // 2. We are back with the same thread as before, but
  //    we are starting a new hash bucket.
  //    _arip->key == 0;
  // 3. We are back with the same thread as before, but
  //    we are in the middle of a hash bucket.
  //    _arip->key > 0;
  //
  // Previously, we would cache the hash bucket in the ARI
  // structure.  But, because the ark library now supports
  // multi-threading, the hash bucket can change inbetween
  // calls to ark_first and ark_next and ark_next.  So
  // now we must fetch the hash bucket on each invocation.
  //

  // Look for the first hash table entry that has blocks
  // in it.  If hpos is zero, we are just starting out
  // with a new pool thread, so start with that pool
  // thread's first hash bucket.
  i = _arip->hpos;
  if (_arip->hpos == 0)
  {
    i = id;
  }

  for (; (i < _arkp->hcount) && (!found); i += _arkp->nthrds) {
    hblk = HASH_LBA(HASH_GET(_arkp->ht, i));

    // Found an entry with blocks
    if (hblk>0)
    {
      // Remember the hash table entry.  Will start from
      // this point next time.
      _arip->hpos = i;        

      // Allocate a new bucket
      blen = bl_len(_arkp->bl, hblk);
      _arip->bt = bt_new(divceil(blen * _arkp->bsize, 
                                 _arkp->bsize) * _arkp->bsize, 
                           _arkp->vlimit, 8,
                           &(_arip->bt_orig));
      if (_arip->bt == NULL)
      {
        rcbp->rc = ENOMEM;
        goto ark_next_pool_err;
      }

      buf = (uint8_t *)_arip->bt;
      bl_array = bl_chain(_arkp->bl, hblk, blen);
      if (bl_array == NULL)
      {
        rcbp->rc = ENOMEM;
        bt_delete(_arip->bt_orig);
        _arip->bt_orig = NULL;
        _arip->bt = NULL;
        goto ark_next_pool_err;
      }

      rc = ea_async_io(_arkp->ea, ARK_EA_READ, (void *)buf, 
                          bl_array, blen, _arkp->nthrds);
      if (rc != 0)
      {
        free(bl_array);
        KV_TRC_FFDC(pAT, "FFDC, ENOENT errno = %d", errno);
        bt_delete(_arip->bt_orig);
        _arip->bt_orig = NULL;
        _arip->bt = NULL;
        rcbp->rc = rc;
        goto ark_next_pool_err;
      }

      free(bl_array);

      // The hash bucket we just read in, does it have
      // enough keys that we can get the "_arip->key"
      // entry?  If not, then release this hash bucket
      // and look to the next one
      if (_arip->key < _arip->bt->cnt)
      {
        found = 1;
      }
      else
      {
        // This hash bucket has changed and no longer
        // contains the same amount of keys.  Move
        // to the next bucket and start with the first (0)
        // key.
        bt_delete(_arip->bt_orig);
        _arip->bt_orig = NULL;
        _arip->bt = NULL;
        _arip->key = 0;
      }
    }
    else
    {
      // This could have been the bucket we left off on
      // last time...if so, then we need to reset
      // _arip->key to start at the beginning of
      // the next valid hash bucket.
      _arip->key = 0;
    }
  }

  // If bt is NULL, then we either hit an error above, or
  // we didn't find a key.
  if (_arip->bt == NULL) {

    // If rc == 0, that means we didn't find a key.  Set
    // rc to EAGAIN so that the caller can try with the next
    // thread.
    if (rc == 0)
    {
      _arip->key = 0;
      _arip->hpos = 0;
      rcbp->rc = EAGAIN;
    }
    rcbp->res = -1;
    goto ark_next_pool_err;
  }

  // We have our bucket and now we need to find the next key.
  pos = _arip->bt->data;

  kcnt = 0;
  do
  {
    pos += vi_dec64(pos, &pkl);
    pos += vi_dec64(pos, &pvl);

    // Have you found our key place in the bucket?
    if (kcnt == _arip->key)
    {
      break;
    }
    else
    {
      // Move to the next key/value pair in the bucket.
      // By here, we have made sure that we have enough keys
      // to find the _arip->key entry because of the check above.
      pos += (pkl + (pvl > _arip->bt->max ? _arip->bt->def : pvl));
      kcnt++;
    }
  } while (1);

  // Record the true length of the key
  rcbp->res = pkl;

  // Copy the key into the buffer for the passed in length
  memcpy(kbuf, pos, (pkl > rcbp->klen) ? rcbp->klen : pkl);

  // Are we done with this bucket or do we remember which key we
  // left off on for next call.
  _arip->key++;
  if (_arip->key == _arip->bt->cnt)
  {
    // If there are no more keys in this hash bucket
    // then jump to the next monitored hash bucket for this thread.
    _arip->hpos += _arkp->nthrds;
    _arip->key = 0;
  }

ark_next_pool_err:

  if (_arip->bt != NULL)
  {
    bt_delete(_arip->bt_orig);
    _arip->bt_orig = NULL;
    _arip->bt = NULL;
  }

  iocbp->state = ARK_CMD_DONE;
}

/**
 *******************************************************************************
 * \brief
 *  reduce memory footprint if possible after an io is complete
 ******************************************************************************/
void cleanup_task_memory(_ARK *_arkp, int tid)
{
    int    i    = 0;
    tcb_t *tcbp = NULL;

    KV_TRC(pAT, "CHECK_CLEAN");
    for (i=0; i<ARK_MAX_TASK_OPS; i++)
    {
        tcbp = _arkp->tcbs + i;

        if (tcbp->aiol && tcbp->aiolN > ARK_MIN_AIOL)
        {
            KV_TRC(pAT,"REDUCE IOL: tid:%d %ld to %d",
                    tid, tcbp->aiolN, ARK_MIN_AIOL);
            tcbp->aiolN = ARK_MIN_AIOL;
            tcbp->aiol=am_realloc(tcbp->aiol,sizeof(ark_io_list_t)*tcbp->aiolN);
            if (!tcbp->aiol)
            {
                KV_TRC(pAT,"FFDC    tid:%d %ld to %d",
                        tid, tcbp->aiolN, ARK_MIN_AIOL);
            }
        }
        if (tcbp->inb && tcbp->inb_size > _arkp->min_bt)
        {
            KV_TRC(pAT,"REDUCE INB: tid:%d %ld to %d",
                    tid, tcbp->inb_size, _arkp->min_bt);
            bt_realloc(&tcbp->inb, &tcbp->inb_orig, _arkp->min_bt);
            bt_clear(tcbp->inb);
            tcbp->inb_size = _arkp->min_bt;
        }
        if (tcbp->oub && tcbp->oub_size > _arkp->min_bt)
        {
            KV_TRC(pAT,"REDUCE OUB: tid:%d %ld to %d",
                    tid, tcbp->oub_size, _arkp->min_bt);
            bt_realloc(&tcbp->oub, &tcbp->oub_orig, _arkp->min_bt);
            bt_clear(tcbp->oub);
            tcbp->oub_size = _arkp->min_bt;
        }

        if (tcbp->vb && tcbp->vbsize > _arkp->bsize*ARK_MIN_VB)
        {
            KV_TRC(pAT,"REDUCE VAB: tid:%d %ld to %ld",
                    tid, tcbp->vbsize, _arkp->bsize*ARK_MIN_VB);
            tcbp->vbsize  = _arkp->bsize*ARK_MIN_VB;
            tcbp->vb_orig = am_realloc(tcbp->vb_orig, tcbp->vbsize + ARK_ALIGN);
            if (tcbp->vb_orig) {tcbp->vb = ptr_align(tcbp->vb_orig);}
            else
            {
                KV_TRC_FFDC(pAT,"REDUCE VAB: tid:%d %ld to %ld",
                            tid, tcbp->vbsize, _arkp->bsize*ARK_MIN_VB);
                tcbp->vbsize  = 0;
                tcbp->vb      = NULL;
            }
        }
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int init_task_state(_ARK *_arkp, tcb_t *tcbp)
{
  rcb_t *rcbp = &(_arkp->rcbs[tcbp->rtag]);
  int init_state = 0;

  switch (rcbp->cmd)
  {
    case K_GET :
    {
      init_state = ARK_GET_START;
      break;
    }
    case K_SET :
    {
      init_state = ARK_SET_START;
      break;
    }
    case K_DEL :
    {
      init_state = ARK_DEL_START;
      break;
    }
    case K_EXI :
    {
      init_state = ARK_EXIST_START;
      break;
    }
    case K_RAND :
    {
      init_state = ARK_RAND_START;
      break;
    }
    case K_FIRST :
    {
      init_state = ARK_FIRST_START;
      break;
    }
    case K_NEXT :
    {
      init_state = ARK_NEXT_START;
      break;
    }
    default :
      // we should never get here
      KV_TRC_FFDC(pAT, "FFDC, bad rcbp->cmd: %d", rcbp->cmd);
      break;
  }

  return init_state;
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
  queue_t *usq    = scbp->uscheduleQ;
  queue_t *uhq    = scbp->uharvestQ;
  int32_t  i      = 0;
  uint64_t hval   = 0;
  uint64_t hlba   = 0;
  uint64_t tmiss  = 0;
  uint32_t MAX_POLL=0;
  uint64_t perfT  = 0;
  uint64_t verbT  = 0;
  ticks    iticks = 0;

  KV_TRC_DBG(pAT, "start tid:%d nactive:%d", id, _arkp->nactive);

  _arkp->utime = getticks();

  /*----------------------------------------------------------------------------
   * Run until the thread state is EXIT
   *--------------------------------------------------------------------------*/
  while (scbp->poolstate != PT_EXIT)
  {
      /*------------------------------------------------------------------------
       * harvest IOs
       *----------------------------------------------------------------------*/
      MAX_POLL = queue_count(hq);
      if (MAX_POLL) {MAX_POLL = MAX_POLL<10 ? 1 : MAX_POLL/10;}

      for (i=0; i<MAX_POLL; i++)
      {
          iocb_t *iocbp = NULL;
          int32_t ttag  = 0;

          if (queue_deq_unsafe(hq, &ttag) != 0) {continue;}
          iocbp = &(_arkp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "HQ_DEQ  tid:%d ttag:%3d state:%d",
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
          iocbp = &(_arkp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "SQ_DEQ  tid:%d ttag:%3d state:%d",
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

      if (_arkp->ea->unmap)
      {
          process_unmapQ(_arkp, id, 300<MDELTA(_arkp->utime,_arkp->ns_per_tick));
      }

      /*------------------------------------------------------------------------
       * check the request queue and try to pull off
       * as many requests as possible and queue them up
       * in the task queue
       *----------------------------------------------------------------------*/
      queue_lock(rq);
      if (!rq->c && !tq->c && !sq->c && !hq->c && !cq->c && !usq->c && !uhq->c)
      {
          ticks delta;
          iticks = getticks();
          // We have reached a point where there is absolutely
          // no work for this worker thread to do.  So we
          // go to sleep waiting for new requests to come in
          queue_wait(rq);
          delta = MDELTA(iticks,_arkp->ns_per_tick);
          if (delta > ARK_CLEANUP_DELAY) {cleanup_task_memory(_arkp,id);}
          KV_TRC_DBG(pAT, "IO:     tid:%d IDLE_THREAD iticks:%ld", id, delta);
      }

      int rN=rq->c;
      for (i=0; i<rN; i++)
      {
          rcb_t  *rcbp  = NULL;
          tcb_t  *tcbp  = NULL;
          iocb_t *iocbp = NULL;
          int32_t rtag  = 0;
          int32_t ttag  = 0;
          int32_t tskrc = EAGAIN;

          if (queue_deq_unsafe(rq, &rtag)) {break;}

          KV_TRC_EXT3(pAT, "RQ_DEQ  tid:%d rtag:%3d", id, rtag);
          rcbp       = &(_arkp->rcbs[rtag]);
          rcbp->rtag = rtag;
          rcbp->ttag = -1;
          hval = HASH_GET(_arkp->ht, rcbp->pos);
          if (HASH_LCK(hval))
          {
              queue_enq_unsafe(rq, rtag);
              KV_TRC_DBG(pAT,"IO_RERQ tid:%2d rtag:%3d HASH_LCK", id, rtag);
              continue;
          }

          tskrc = tag_unbury(_arkp->ttags, &ttag);
          if (tskrc == EAGAIN)
          {
              KV_TRC_DBG(pAT,"IO_RERQ tid:%2d rtag:%3d NO_TAG", id, rtag);
              queue_enq_unsafe(rq, rtag);
              break;
          }
          if (tskrc)
          {
              KV_TRC_DBG(pAT,"IO_RERQ tid:%2d rtag:%3d rc:%d", id, rtag, tskrc);
              queue_enq_unsafe(rq, rtag);
              break;
          }

          tcbp            = &(_arkp->tcbs[ttag]);
          iocbp           = &(_arkp->iocbs[ttag]);
          tcbp->rtag      = rtag;
          rcbp->ttag      = ttag;
          iocbp->state    = init_task_state(_arkp, tcbp);
          iocbp->io_error = 0;
          tcbp->sthrd     = rcbp->sthrd;
          tcbp->ttag      = ttag;
          tcbp->new_key   = 0;
          hlba = HASH_LBA(hval);
          HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, ttag, hlba));
          (void)queue_enq_unsafe(tq, ttag);
          KV_TRC_DBG(pAT, "IO:     tid:%d ttag:%3d rtag:%d TQ_ENQ START ",
                     id, ttag, rtag);
      }
      queue_unlock(rq);

      /*------------------------------------------------------------------------
       * usleep if appropriate
       *----------------------------------------------------------------------*/
      if (!rq->c && !tq->c && !sq->c && !cq->c &&
          ((tmiss>100 && hq->c>1 && hq->c<32) || (tmiss>10  && hq->c>32))
         )
      {
          usleep(10);
          KV_TRC(pAT,"IO:     tmiss:%ld hq:%d USLEEP", tmiss, hq->c);
          tmiss=0;
      }

      /* do calcs for avg QD */
      _arkp->QDT += rq->c + cq->c + tq->c + sq->c + hq->c;
      _arkp->QDN += 1;

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

          tcbp       = &(_arkp->tcbs[ttag]);
          iocbp      = &(_arkp->iocbs[ttag]);
          tcbp->ttag = ttag;
          s          = iocbp->state;

          KV_TRC_EXT3(pAT, "TQ_DEQ  tid:%d ttag:%3d state:%d ", id, ttag, s);

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
          else if (s==ARK_RAND_START)     {ark_rand_pool      (_arkp,id,tcbp);}
          else if (s==ARK_FIRST_START)    {ark_first_pool     (_arkp,id,tcbp);}
          else if (s==ARK_NEXT_START)     {ark_next_pool      (_arkp,id,tcbp);}

          /*--------------------------------------------------------------------
           * schedule the first IO right way
           *------------------------------------------------------------------*/
          if (iocbp->state == ARK_IO_SCHEDULE)
          {
              iocbp = &(_arkp->iocbs[ttag]);
              ea_async_io_schedule(_arkp, id, iocbp);
          }

          /*--------------------------------------------------------------------
           * requeue task
           *------------------------------------------------------------------*/
          if (iocbp->state == ARK_IO_SCHEDULE)
          {
              KV_TRC_EXT3(pAT, "SQ_ENQ  tid:%d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(sq,ttag);
          }
          else if (iocbp->state == ARK_IO_HARVEST)
          {
              KV_TRC_EXT3(pAT, "HQ_ENQ  tid:%d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(hq,ttag);
          }
          else if (iocbp->state == ARK_CMD_DONE)
          {
              KV_TRC_EXT3(pAT, "CQ_ENQ  tid:%d ttag:%3d state:%d ",
                          id, ttag, iocbp->state);
              queue_enq_unsafe(cq,ttag);
          }
          else
          {
              KV_TRC_EXT3(pAT, "TQ_ENQ  tid:%d ttag:%3d state:%d ",
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

          tcbp  = &(_arkp->tcbs[ttag]);
          rtag = tcbp->rtag;
          rcbp  = &(_arkp->rcbs[rtag]);
          iocbp = &(_arkp->iocbs[ttag]);

          KV_TRC_EXT3(pAT, "CQ_DEQ  tid:%d ttag:%3d state:%d ",
                      id, ttag, iocbp->state);

          if (iocbp->state == ARK_CMD_DONE)
          {
              uint32_t lat = UDELTA(rcbp->stime, _arkp->ns_per_tick);
              UPDATE_LAT(_arkp, rcbp, lat);

              if (iocbp->io_error)
              {
                   rcbp->res = -1;
                   rcbp->rc  = iocbp->io_error;
              }
              hlba = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));
              HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(0, 0, hlba));
              if (rcbp->cb)
              {
                  KV_TRC(pAT, "CMP_CB  tid:%d ttag:%3d rtag:%3d cmd:%d "
                              "rc:%d res:%7ld lat:%6d",
                              id, ttag, rcbp->rtag, rcbp->cmd,
                              rcbp->rc,rcbp->res,lat);
                  (rcbp->cb)(rcbp->rc, rcbp->dt, rcbp->res);
                  rcbp->stat = A_NULL;
                  (void)tag_bury(_arkp->rtags, rtag);
              }
              else
              {
                  KV_TRC(pAT, "CMP_SIG tid:%d ttag:%3d rtag:%3d cmd:%d "
                              "rc:%d res:%7ld lat:%6d",
                              id, ttag, rcbp->rtag, rcbp->cmd,
                              rcbp->rc,rcbp->res,lat);
                  pthread_mutex_lock(&(rcbp->alock));
                  rcbp->stat = A_COMPLETE;
                  pthread_cond_signal(&(rcbp->acond));
                  pthread_mutex_unlock(&(rcbp->alock));
              }
              (void)tag_bury(_arkp->ttags, ttag);
          }
          else
          {
              KV_TRC_EXT3(pAT, "FFDC CQ tid:%d ttag:%3d state:%d UNKNOWN_STATE",
                      id, ttag, iocbp->state);
          }
      }

      /*------------------------------------------------------------------------
       * performance and dynamic tracing
       *----------------------------------------------------------------------*/
      if (pAT->verbosity >= 1 && SDELTA(perfT, _arkp->ns_per_tick) > 1)
      {
          uint64_t opsT = _arkp->set_opsT + _arkp->get_opsT + _arkp->exi_opsT +\
                          _arkp->del_opsT;
          uint64_t latT = _arkp->set_latT + _arkp->get_latT + _arkp->exi_latT +\
                          _arkp->del_latT;
          uint64_t lat=0,set_lat=0,get_lat=0,exi_lat=0,del_lat=0,um_lat=0,QDA=0;
          if (opsT)            {lat     = latT/opsT;}
          if (_arkp->set_opsT) {set_lat = _arkp->set_latT/_arkp->set_opsT;}
          if (_arkp->get_opsT) {get_lat = _arkp->get_latT/_arkp->get_opsT;}
          if (_arkp->exi_opsT) {exi_lat = _arkp->exi_latT/_arkp->exi_opsT;}
          if (_arkp->del_opsT) {del_lat = _arkp->del_latT/_arkp->del_opsT;}
          if (_arkp->um_opsT)  {um_lat  = _arkp->um_latT/_arkp->um_opsT;}
          if (_arkp->QDT)      {QDA     = _arkp->QDT/_arkp->QDN;}
          KV_TRC_PERF1(pAT,"IO:     tid:%d PERF   opsT:%7ld lat:%7ld "
                           "opsT(%7ld %7ld %7ld %7ld %7ld) "
                           "lat(%7ld %6ld %6ld %6ld %6ld) QD:%4ld issT:%4d",
                           id, opsT, lat,
                           _arkp->set_opsT, _arkp->get_opsT, _arkp->exi_opsT,
                           _arkp->del_opsT, _arkp->um_opsT,
                           set_lat, get_lat, exi_lat, del_lat, um_lat,
                           QDA, _arkp->issT);
          KV_TRC_PERF2(pAT,"IO:     tid:%d QUEUES rq:%3d tq:%3d sq:%4d hq:%4d "
                           "cq:%4d usq:%4d uhq:%4d",
                           id, rq->c, tq->c, sq->c, hq->c, cq->c, usq->c, uhq->c);
          _arkp->set_latT = 0;
          _arkp->get_latT = 0;
          _arkp->exi_latT = 0;
          _arkp->del_latT = 0;
          _arkp->set_opsT = 0;
          _arkp->get_opsT = 0;
          _arkp->exi_opsT = 0;
          _arkp->del_opsT = 0;
          _arkp->QDT      = 0;
          _arkp->QDN      = 0;
          _arkp->htc_hits = 0;
          _arkp->htc_disc = 0;
          perfT           = getticks();

          KV_TRC_PERF2(pAT, "PSTATS  kv:%8ld ops:%8ld ios:%8ld bytes:%8ld blks:%8ld",
                       scbp->poolstats.kv_cnt   + _arkp->pers_stats.kv_cnt,
                       scbp->poolstats.ops_cnt,
                       scbp->poolstats.io_cnt,
                       scbp->poolstats.byte_cnt + _arkp->pers_stats.byte_cnt,
                       scbp->poolstats.blk_cnt  + _arkp->pers_stats.blk_cnt);

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

  /* allow unmaps to finish */
  while (_arkp->ea->unmap && (_arkp->blu->count || usq->c || uhq->c))
      {process_unmapQ(_arkp, id, 1);}

  KV_TRC_PERF2(pAT,"IO:     tid:%d QUEUES rq:%3d tq:%3d sq:%4d hq:%4d "
                   "cq:%4d usq:%4d uhq:%4d",
                   id, rq->c, tq->c, sq->c, hq->c, cq->c, usq->c, uhq->c);

  KV_TRC_FFDC(pAT, "PSTATS  kv:%8ld ops:%8ld ios:%8ld bytes:%8ld blks:%8ld",
               scbp->poolstats.kv_cnt   + _arkp->pers_stats.kv_cnt,
               scbp->poolstats.ops_cnt,
               scbp->poolstats.io_cnt,
               scbp->poolstats.byte_cnt + _arkp->pers_stats.byte_cnt,
               scbp->poolstats.blk_cnt  + _arkp->pers_stats.blk_cnt);

  KV_TRC(pAT, "exiting tid:%d nactive:%d", id, _arkp->nactive);
  return NULL;
}

