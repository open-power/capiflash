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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/errno.h>

#include "ct.h"
#include "ut.h"
#include "vi.h"

#include "arkdb.h"
#include "ark.h"
#include "arp.h"
#include "am.h"

#include <arkdb_trace.h>

int ark_wait_tag(_ARK *_arkp, int tag, int *errcode, int64_t *res)
{
  int rc = 0;
  rcb_t *rcbp = &(_arkp->rcbs[tag]);

  pthread_mutex_lock(&(rcbp->alock));
  while (rcbp->stat != A_COMPLETE)
  {
    pthread_cond_wait(&(rcbp->acond), &(rcbp->alock));
  }

  if (rcbp->stat == A_COMPLETE)
  {
    *res = rcbp->res;
    *errcode = rcbp->rc;
    rcbp->stat = A_NULL;
    tag_bury(_arkp->rtags, tag);
  }
  else
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "tag = %d rc = %d", tag, rc);
  }
  
  pthread_mutex_unlock(&(rcbp->alock));

  return rc;
}

int ark_anyreturn(_ARK *_arkp, int *tag, int64_t *res) {
  int i;
  int astart = _arkp->astart;
  int nasyncs = _arkp->nasyncs;
  int itag = -1;

  for (i = 0; i < _arkp->nasyncs; i++) {
    itag = (astart + i) % nasyncs;
    if (_arkp->rcbs[itag].stat == A_COMPLETE) {
      //KV_TRC_IO(pAT, pAT, "Found completion tag %d", itag);
      *tag = itag;
      *res = _arkp->rcbs[(astart+i) % nasyncs].res;
      KV_TRC_IO(pAT, "arp %p res %"PRIi64"", _arkp->rcbs+itag,
                                             _arkp->rcbs[itag].res);
      _arkp->rcbs[itag].stat = A_NULL;
      tag_bury(_arkp->rtags, itag);
      return 0;
    }
  }
  _arkp->astart++;
  _arkp->astart %= nasyncs;
  return EINVAL;
}

int64_t ark_take_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t n)
{
  int64_t rc = 0;

  pthread_mutex_lock(&_arkp->mainmutex);

  if (bl_left(_arkp->bl) < n)
  {
    BL *bl = NULL;
    int ea_rc = 0;
    int64_t cursize = _arkp->size;
    int64_t atleast = n * _arkp->bsize;
    int64_t newsize = cursize + atleast + (_arkp->grow * _arkp->bsize);

    uint64_t newbcnt = newsize / _arkp->bsize;

    bl = bl_resize(_arkp->bl, newbcnt, _arkp->bl->w);

    if (bl == NULL) {
      rc = -1;
    }
    else
    {
      ea_rc = ea_resize(_arkp->ea, _arkp->bsize, newbcnt);
      if (ea_rc) {
        rc = -1;
      } else {
        _arkp->size = newsize;
	_arkp->bl = bl;
      }
    }    
  }

  if (rc != -1)
  {
    uint64_t blk = bl_take(_arkp->bl, n);
    if (blk > 0) {
      _arkp->blkused += n;
      rc = blk;
    }
  }
  else
  {
    KV_TRC_FFDC(pAT, "Failed to obtain blocks requested = %"PRIu64"", n);
  }

  pthread_mutex_unlock(&_arkp->mainmutex);

  if (rc != -1)
  {
    stats->blk_cnt += n;
  }

  return rc;
}

void ark_drop_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t blk) {

  pthread_mutex_lock(&_arkp->mainmutex);
  int now = bl_drop(_arkp->bl, blk);
  _arkp->blkused -= now;
  pthread_mutex_unlock(&_arkp->mainmutex);
  stats->blk_cnt -= now;
  KV_TRC_IO(pAT, "blkused %ld new %d", _arkp->blkused, now);
}


int ark_enq_cmd(int cmd, _ARK *_arkp, uint64_t klen, void *key, 
                uint64_t vlen,void *val,uint64_t voff,
                void (*cb)(int errcode, uint64_t dt,int64_t res), 
		uint64_t dt, int pthr, int *ptag)
{
  int32_t   rtag = -1;
  int       rc = 0;
  int       pt = 0;
  rcb_t    *rcbp = NULL;

  if ( (_arkp == NULL) || ((klen > 0) && (key == NULL)))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: cmd %d, ark %p, \
                     key %p, klen %"PRIu64"", cmd, _arkp, key, klen);
    rc = EINVAL;
    goto ark_enq_cmd_err;
  }

  rc = tag_unbury(_arkp->rtags, &rtag);
  if (rc == 0)
  {
    rcbp = &(_arkp->rcbs[rtag]);
    rcbp->ark = _arkp;
    rcbp->cmd = cmd;
    rcbp->rtag = rtag;
    rcbp->ttag = -1;
    rcbp->stat = A_INIT;
    rcbp->klen = klen;
    rcbp->key = key;
    rcbp->vlen = vlen;
    rcbp->val = val;
    rcbp->voff = voff;
    rcbp->cb = cb;
    rcbp->dt = dt;
    rcbp->rc = 0;
    rcbp->hold = -1;
    rcbp->res = -1;

    // If the caller has asked to run on a specific thread, then
    // do so.  Otherwise, use the key to find which thread
    // will handle command
    if (pthr == -1)
    {
      rcbp->pos = hash_pos(_arkp->ht, key, klen);
      rcbp->sthrd = rcbp->pos / _arkp->npart;
      pt = rcbp->pos / _arkp->npart;
    }
    else
    {
      rcbp->sthrd = pthr;
      pt = pthr;
    }

    KV_TRC_IO(pAT, "%s%p enqueueing on P%d%s", C_Yellow, _arkp, rcbp->sthrd, C_Reset);

    queue_lock(_arkp->poolthreads[pt].rqueue);

    (void)queue_enq_unsafe(_arkp->poolthreads[pt].rqueue, rtag);

    queue_unlock(_arkp->poolthreads[pt].rqueue);

  }
  else
  {
    KV_TRC_DBG(pAT, "NO_TAG: %"PRIu64"", dt);
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

int ark_rand_pool(_ARK *_arkp, int id, tcb_t *tcbp) {
  // find the hashtable position
  rcb_t   *rcbp = &(_arkp->rcbs[tcbp->rtag]);
  int32_t  found = 0;
  int32_t  i = 0;
  int32_t  ea_rc = 0;
  int32_t  state = ARK_CMD_DONE;
  uint8_t  *buf = NULL;
  uint8_t  *pos = NULL;
  uint64_t hblk;
  uint64_t pkl;
  uint64_t pvl;
  uint64_t btsize;
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
                                 _arkp->vlimit, 8, &(btsize), &bt_orig);
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
  return state;
}

// arp->res returns the length of the key
// arp->val is for the ARI structure...both for
//          passing in and returning.
int ark_first_pool(_ARK *_arkp, int id, tcb_t *tcbp) {
  rcb_t   *rcbp = &(_arkp->rcbs[tcbp->rtag]);
  _ARI *_arip = NULL;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  state = ARK_CMD_DONE;
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
                                 _arkp->vlimit, 8, &(_arip->btsize),
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

  return state;
}

// arp->res returns the length of the key
// arp->val is for the ARI structure...both for
//          passing in and returning.
int ark_next_pool(_ARK *_arkp, int id, tcb_t  *tcbp) {
  rcb_t    *rcbp = &(_arkp->rcbs[tcbp->rtag]);
  _ARI     *_arip = (_ARI *)rcbp->val;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  state = ARK_CMD_DONE;
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
                           _arkp->vlimit, 8, &(_arip->btsize), 
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

  return state;
}


void
cleanup_task_memory(_ARK *_arkp, tcb_t *tcbp)
{

  if ((tcbp->inb != NULL) && (tcbp->inblen > _arkp->bsize))
  {
    //printf("Reducing in bucket\n");
    bt_delete(tcbp->inb);
    tcbp->inb = bt_new(0, _arkp->vlimit, sizeof(uint64_t), 
                                       &(tcbp->inblen),
                                       &(tcbp->inb_orig));
  }

  if ((tcbp->oub != NULL) && (tcbp->oublen > _arkp->bsize))
  {
    //printf("Reducing out bucket\n");
    bt_delete(tcbp->oub);
    tcbp->oub = bt_new(0, _arkp->vlimit, sizeof(uint64_t), 
                                       &(tcbp->oublen),
                                       &(tcbp->oub_orig));
  }

  if ((tcbp->vb != NULL) && (tcbp->vbsize > (_arkp->bsize * 256)))
  {
    //printf("Reducing variable buffer\n");
    am_free(tcbp->vb_orig);
    tcbp->vbsize = _arkp->bsize * 256;
    tcbp->vb_orig = am_malloc(tcbp->vbsize);
    if (tcbp->vb_orig != NULL)
    {
      tcbp->vb = ptr_align(tcbp->vb_orig);
    }
    else
    {
      // Clear out the vbsize.  When this buffer is used
      // for a command, the command will fail gracefully
      tcbp->vbsize = 0;
      tcbp->vb = NULL;
    }
  }
}

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
      break;
  }

  return init_state;
}

void *pool_function(void *arg) {
  PT          *pt    = (PT*)arg;
  int          id    = pt->id;
  _ARK        *_arkp = pt->ark;
  scb_t       *scbp  = &(_arkp->poolthreads[id]);
  rcb_t       *iorcbp  = NULL;
  rcb_t       *rcbp  = NULL;
  rcb_t       *rcbtmp  = NULL;
  rcb_t       *ractp = NULL;
  tcb_t       *iotcbp  = NULL;
  tcb_t       *tcbp  = NULL;
  tcb_t       *tactp = NULL;
  iocb_t      *iocbp = NULL;
  queue_t     *rq    = scbp->rqueue;
  queue_t     *tq    = scbp->tqueue;
  queue_t     *ioq   = scbp->ioqueue;
  int32_t      io_rc = 0;
  int32_t      iocount = 0;
  int32_t      i     = 0;
  int32_t      reqrc = EAGAIN;
  int32_t      tskrc = EAGAIN;
  int32_t      io_status = 0;
  int32_t      tskact = 0;
  int32_t      reqact = 0;
  int32_t      iotask = 0;
  int32_t      reqtag = -1;
  int32_t      tsktag = 0;
  uint64_t     hval   = 0;
  uint64_t     hlba   = 0;

#if 0
  struct timespec timewait;
  struct timeval now;

#define ARK_KV_GC_TIME_SEC  120
  gettimeofday(&now, NULL);
  timewait.tv_sec = now.tv_sec + ARK_KV_GC_TIME_SEC;
  timewait.tv_nsec = now.tv_usec * 1000UL;
  _arkp->poolthreads[id].dogc = 1;
#endif

  KV_TRC(pAT, "id %d started, ark->nactive %d", id, _arkp->nactive);

  // Run until the thread state is EXIT or the global
  // state, ark_exit, is not set showing we are shutting
  // down the ark db.
  while ((scbp->poolstate != PT_EXIT))
  {

    // First we check to see if there are any outstanding
    // I/O's that need to be harvested.  Because the IO
    // queue is only manipulated by the owning thread, we
    // don't need to worry about any serialization issues here
    iocount = queue_count(ioq);
    for (i = 0; i < iocount; i++)
    {
      queue_deq_unsafe(ioq, &iotask);
      iocbp = &(_arkp->iocbs[iotask]);
      iotcbp = &(_arkp->tcbs[iotask]);
      iorcbp = &(_arkp->rcbs[iotcbp->rtag]);

      // Call out to IO completion routine
      io_status = ea_async_io_harvest(_arkp, id, iotcbp, iocbp);
      if (io_status > 0 )
      {
        iotcbp->state = iocbp->io_done;
        (void)queue_enq_unsafe(tq, iotask);
      }
      else if (io_status == 0)
      {
        io_rc = ea_async_io_mod(_arkp, iocbp->op, 
                               (void *)iocbp->addr, iocbp->blist, iocbp->nblks, 
                               iocbp->start, iotask, iocbp->io_done);
        if ( io_rc < 0 )
        {
          iorcbp->res = -1;
          iorcbp->rc = -io_rc;
          iotcbp->state = ARK_CMD_DONE;
	  (void)queue_enq_unsafe(tq, iotask);
        }
        else if (io_rc == 0)
        {
          iotcbp->state = ARK_IO_HARVEST;
          (void)queue_enq_unsafe(ioq, iotask);
        }
        else
        {
          iotcbp->state = iocbp->io_done;
	  (void)queue_enq_unsafe(tq, iotask);
        }
      }
      else
      {
        iotcbp->state = ARK_CMD_DONE;
        iorcbp->rc = -(io_status);
        (void)queue_enq_unsafe(tq, iotask);
      }
    }

    // Now we check the request queue and try to pull off
    // as many requests as possible and queue them up
    // in the task queue
    queue_lock(rq);

    if ( (queue_empty(rq)) && (reqtag == -1) )
    {
      if ( queue_empty(ioq) && queue_empty(tq) )
      {
        // We have reached a point where there is absolutely
	// no work for this worker thread to do.  So we
	// go to sleep waiting for new requests to come in
	queue_wait(rq);

#if 0
	// If we've come out of here, it means work has been
	// placed on the request queue.
        p_rc = pthread_cond_timedwait(&(_arkp->poolthreads[id].poolcond), 
                                      &(_arkp->poolthreads[id].poolmutex),
                                      &timewait);
        // Every ARK_KV_GC_TIME_SEC, check to see if we need
        // to reduce the size of the vb buffer back to it's original
        // size.  This is to help keep memory footprint down
        if ( (p_rc == ETIMEDOUT) && (_arkp->poolthreads[id].dogc) )
        {
          gettimeofday(&now, NULL);
          timewait.tv_sec = now.tv_sec + ARK_KV_GC_TIME_SEC;
          timewait.tv_nsec = now.tv_usec * 1000UL;

          if (_arkp->poolthreads[id].vbsize > (_arkp->bsize * 1024))
          {
            am_free(_arkp->poolthreads[id].vb_orig);

            _arkp->poolthreads[id].vbsize = _arkp->bsize * 1024;
            _arkp->poolthreads[id].vb_orig = 
                                    am_malloc(_arkp->poolthreads[id].vbsize);
            if (_arkp->poolthreads[id].vb_orig != NULL)
            {
              _arkp->poolthreads[id].vb = 
                                  ptr_align(_arkp->poolthreads[id].vb_orig);
            }
            else
            {
              // Clear out the vbsize.  When this buffer is used
              // for a command, the command will fail gracefully
              _arkp->poolthreads[id].vbsize = 0;
              _arkp->poolthreads[id].vb = NULL;
            }
          }
        }
#endif
      }
    }

    while (((reqrc == EAGAIN) && !(queue_empty(rq))) || 
           ((reqrc == 0) && (!tag_empty(_arkp->ttags))))
    {
        if (reqrc == EAGAIN)
        {
          reqrc = queue_deq_unsafe(rq, &reqtag);
          if ( reqrc == 0 )
          {
            rcbp = &(_arkp->rcbs[reqtag]);
            rcbp->rtag = reqtag;
            rcbp->ttag = -1;
            rcbp->hold = -1;
          }
        }

        if (reqrc == 0)
        {
          hval = HASH_GET(_arkp->ht, rcbp->pos);
          if (HASH_LCK(hval))
          {
            tsktag = HASH_TAG(hval);
            tcbp = &(_arkp->tcbs[tsktag]);
	    rcbtmp = &(_arkp->rcbs[tcbp->rtag]);
	    while (rcbtmp->hold != -1)
	    {
	      rcbtmp = &(_arkp->rcbs[rcbtmp->hold]);
	    }
	    scbp->holds++;
	    rcbtmp->hold = reqtag;
	    reqrc = EAGAIN;
	    reqtag = -1;
	    rcbp = NULL;
          }
	  else
	  {
	    tskrc = tag_unbury(_arkp->ttags, &tsktag);
	    if (tskrc == 0)
	    {
              tcbp = &(_arkp->tcbs[tsktag]);
	    }
	    else
	    {
              tcbp = NULL;
	    }

	    if (tcbp)
	    {
              tcbp->rtag = reqtag;
	      rcbp->ttag = tsktag;
	      rcbp->hold = -1;
	      tcbp->state = init_task_state(_arkp, tcbp);
	      tcbp->sthrd = rcbp->sthrd;
	      tcbp->ttag = tsktag;
	      tcbp->new_key = 0;
              hlba = HASH_LBA(hval);
	      HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tsktag, hlba));
	      (void)queue_enq_unsafe(tq, tsktag);
	      reqtag = -1;
	      reqrc = EAGAIN;
	      tskrc = EAGAIN;
	      rcbp = NULL;
	      tcbp = NULL;
            }
          }
        }
    }

    queue_unlock(rq);

    while(!queue_empty(tq))
    {
      (void)queue_deq_unsafe(tq, &tskact);
      tactp = &(_arkp->tcbs[tskact]);
      reqact = tactp->rtag;
      ractp = &(_arkp->rcbs[reqact]);

      switch (tactp->state)
      {
        case ARK_SET_START :
        {
          scbp->poolstats.ops_cnt++;
          tactp->state = ark_set_start(_arkp, id, tactp);
          break;
        }
        case ARK_SET_PROCESS_INB :
        {
          tactp->state = ark_set_process_inb(_arkp, id, tactp);
          break;
        }
        case ARK_SET_WRITE :
        {
          tactp->state = ark_set_write(_arkp, id, tactp);
          break;
        }
        case ARK_SET_FINISH :
        {
          tactp->state = ark_set_finish(_arkp, id, tactp);
          break;
        }
	case ARK_GET_START :
        {
          scbp->poolstats.ops_cnt++;
          tactp->state = ark_get_start(_arkp, id, tactp);
          break;
        }
	case ARK_GET_PROCESS :
        {
          tactp->state = ark_get_process(_arkp, id, tactp);
          break;
        }
	case ARK_GET_FINISH :
        {
          tactp->state = ark_get_finish(_arkp, id, tactp);
          break;
        }
	case ARK_DEL_START :
        {
          scbp->poolstats.ops_cnt++;
          tactp->state = ark_del_start(_arkp, id, tactp);
          break;
        }
	case ARK_DEL_PROCESS :
        {
          tactp->state = ark_del_process(_arkp, id, tactp);
          break;
        }
	case ARK_DEL_FINISH :
        {
          tactp->state = ark_del_finish(_arkp, id, tactp);
          break;
        }
	case ARK_EXIST_START :
        {
          scbp->poolstats.ops_cnt++;
          tactp->state = ark_exist_start(_arkp, id, tactp);
          break;
        }
	case ARK_EXIST_FINISH :
        {
          tactp->state = ark_exist_finish(_arkp, id, tactp);
          break;
        }
	case ARK_RAND_START :
        {
          tactp->state = ark_rand_pool(_arkp, id, tactp);
          break;
        }
	case ARK_FIRST_START :
        {
          tactp->state = ark_first_pool(_arkp, id, tactp);
          break;
        }
	case ARK_NEXT_START :
        {
          tactp->state = ark_next_pool(_arkp, id, tactp);
          break;
        }
	default :
	{
          // The only state left is ARK_CMD_DONE, so we
	  // just break out and end the task
          break;
	}
      }

      if (tactp->state == ARK_CMD_DONE)
      {
        if (ractp->hold == -1)
        {
          hlba = HASH_LBA(HASH_GET(_arkp->ht, ractp->pos));
          HASH_SET(_arkp->ht, ractp->pos, HASH_MAKE(0, 0, hlba));
          if ( ractp->cb != NULL )
	  {
            (ractp->cb)(ractp->rc, ractp->dt, ractp->res);
            ractp->stat = A_NULL;
	    (void)tag_bury(_arkp->rtags, reqact);
          }
	  else
	  {
            pthread_mutex_lock(&(ractp->alock));
            ractp->stat = A_COMPLETE;
            pthread_cond_broadcast(&(ractp->acond));
            pthread_mutex_unlock(&(ractp->alock));
	  }
	  cleanup_task_memory(_arkp, tactp);
	  (void)tag_bury(_arkp->ttags, tskact);
        }
	else
	{
	  tactp->rtag = ractp->hold;
	  ractp->hold = -1;
	  if ( ractp->cb != NULL)
	  {
            (ractp->cb)(ractp->rc, ractp->dt, ractp->res);
            ractp->stat = A_NULL;
	    (void)tag_bury(_arkp->rtags, reqact);
          }
	  else
	  {
            pthread_mutex_lock(&(ractp->alock));
            ractp->stat = A_COMPLETE;
            pthread_cond_broadcast(&(ractp->acond));
            pthread_mutex_unlock(&(ractp->alock));
	  }

          scbp->holds--;
          ractp = &(_arkp->rcbs[tactp->rtag]);
	  ractp->ttag = tactp->ttag;
	  tactp->rtag = ractp->rtag;
	  tactp->state = init_task_state(_arkp, tactp);
	  (void)queue_enq_unsafe(tq, tskact);
        }
      }
      else
      {
        (void)queue_enq_unsafe(ioq, tskact);
      }
    }
  }
  KV_TRC(pAT, "pool thread %d exiting, nactive %d", id, _arkp->nactive);
  return NULL;
}

