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

#include <sys/errno.h>

//#include "ct.h"
#include "ut.h"
#include "vi.h"

#include "arkdb.h"
#include "ark.h"
#include "arp.h"
#include "am.h"

#include <arkdb_trace.h>

// This is the entry into ark_set.  The task has
// been pulled off the task queue.
int ark_set_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t             *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t             *rcbp       = &(_arkp->rcbs[tcbp->rtag]);
  ark_io_list_t    *bl_array    = NULL;
  uint64_t          vlen        = rcbp->vlen;
  uint64_t          new_vbsize  = 0;
  int32_t           rc          = 0;
  int32_t           state       = ARK_CMD_DONE;
  uint8_t           *new_vb     = NULL;

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
      new_vb = am_realloc(tcbp->vb_orig, new_vbsize);
      if ( NULL == new_vb )
      {
        rcbp->res = -1;
        rcbp->rc = ENOMEM;
	state = ARK_CMD_DONE;
        goto ark_set_start_err;
      }
      tcbp->vbsize = new_vbsize;
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
      state = ARK_CMD_DONE;
      goto ark_set_start_err;
    }

    tcbp->vval = (uint8_t *)&(tcbp->vblk);
  }
  else
  {
    tcbp->vval = (uint8_t *)rcbp->val;
  }

  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  if ( 0 == tcbp->hblk )
  {
    // This is going to be the first key in this bucket.
    // Initialize the bucket data/header
    bt_init(tcbp->inb);

    // Call to the next phase of the SET command since
    // we do not need to read in the hash bucket since
    // this will be the first key in it.
    state = ark_set_process(_arkp, tid, tcbp);
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
      state = ARK_CMD_DONE;
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
      state = ARK_CMD_DONE;
      goto ark_set_start_err;
    }

    // Here is where we schedule the IO for the hash bucket
    // This will not wait for the the IO to complete, it will
    // instead issue the asynchronouse IO and then set up
    // the task and queue it so that the IO completion can be
    // done later.
    rc = ea_async_io_mod(_arkp, ARK_EA_READ, 
                               (void *)tcbp->inb, bl_array, tcbp->blen, 
                               0, tcbp->ttag, ARK_SET_PROCESS_INB);
    
    // Upon successful return, the IO has been issued and the TASK
    // block has been set up so that IO completion can be checked
    // later.
    if ( rc < 0 )
    {
      rcbp->res = -1;
      rcbp->rc = -rc;
      state = ARK_CMD_DONE;
      goto ark_set_start_err;
    }
    else if (rc == 0)
    {
      state = ARK_IO_HARVEST;
    }
    else
    {
      state = ark_set_process_inb(_arkp, tid, tcbp);
    }
  }

ark_set_start_err:

  return state;
}

int ark_set_process_inb(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t     *scbp    = &(_arkp->poolthreads[tid]);
  int32_t    state   = ARK_CMD_DONE;

  ark_drop_pool(_arkp, &(scbp->poolstats), tcbp->hblk);

  state = ark_set_process(_arkp, tid, tcbp);

  return state;
}

int ark_set_process(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t            *scbp       = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp       = &(_arkp->rcbs[tcbp->rtag]);
  ark_io_list_t   *bl_array    = NULL;
  uint64_t         oldvlen     = 0;
  int32_t          rc          = 0;
  int32_t          state       = ARK_CMD_DONE;

  // Let's see if we need to grow the out buffer
  tcbp->old_btsize = tcbp->inb->len;
  rc = bt_growif(&(tcbp->oub), &(tcbp->oub_orig), &(tcbp->oublen),
                       divceil((tcbp->blen * _arkp->bsize) + 
		           (rcbp->klen + _arkp->vlimit + 16), 
			   _arkp->bsize) * _arkp->bsize);
  if (rc != 0)
  {
    rcbp->res = -1;
    rcbp->rc = rc;
    state = ARK_CMD_DONE;
    goto ark_set_process_err;
  }

  // modify bucket
  tcbp->new_key = bt_set(tcbp->oub, tcbp->inb, rcbp->klen, rcbp->key, 
                         rcbp->vlen, tcbp->vval, &oldvlen);

  // write the value to the value allocated blocks
  if (rcbp->vlen > _arkp->vlimit) {

    bl_array = bl_chain(_arkp->bl, tcbp->vblk, tcbp->vblkcnt);
    if (bl_array == NULL)
    {
      rcbp->rc = ENOMEM;
      rcbp->res = -1;
      state = ARK_CMD_DONE;
      goto ark_set_process_err;
    }

    scbp->poolstats.byte_cnt -= oldvlen;
    scbp->poolstats.byte_cnt += rcbp->vlen;
    scbp->poolstats.io_cnt += tcbp->vblkcnt;

    rc = ea_async_io_mod(_arkp, ARK_EA_WRITE, (void *)tcbp->vb, 
                     bl_array, tcbp->vblkcnt, 0, tcbp->ttag, ARK_SET_WRITE);
    if (rc < 0)
    {
      rcbp->rc = -rc;
      rcbp->res = -1;
      state = ARK_CMD_DONE;
      goto ark_set_process_err;
    }
    else if (rc == 0)
    {
      state = ARK_IO_HARVEST;
    }
    else
    {
      state = ark_set_write(_arkp, tid, tcbp);
    }
  }
  else
  {
    state = ark_set_write(_arkp, tid, tcbp);
  }

ark_set_process_err:

  return state;
}

int ark_set_write(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  rcb_t            *rcbp      = &(_arkp->rcbs[tcbp->rtag]);
  scb_t            *scbp      = &(_arkp->poolthreads[tid]);
  ark_io_list_t    *bl_array  = NULL;
  uint64_t          blkcnt    = 0;
  int32_t           rc        = 0;
  int32_t           state     = ARK_CMD_DONE;

  // write obuf to new blocks
  blkcnt = divceil(tcbp->oub->len, _arkp->bsize);
  tcbp->nblk = ark_take_pool(_arkp, &(scbp->poolstats), blkcnt);
  if (tcbp->nblk == -1)
  {
    rcbp->rc = ENOSPC;
    rcbp->res = -1;
    state = ARK_CMD_DONE;
    goto ark_set_write_err;
  }

  bl_array = bl_chain(_arkp->bl, tcbp->nblk, blkcnt);
  if (bl_array == NULL)
  {
    rcbp->rc = ENOMEM;
    rcbp->res = -1;
    state = ARK_CMD_DONE;
    goto ark_set_write_err;
  }

  scbp->poolstats.byte_cnt -= tcbp->old_btsize;
  scbp->poolstats.byte_cnt += tcbp->oub->len;
  scbp->poolstats.io_cnt += blkcnt;

  rc = ea_async_io_mod(_arkp, ARK_EA_WRITE, (void *)tcbp->oub, 
                   bl_array, blkcnt, 0, tcbp->ttag, ARK_SET_FINISH);
  if (rc < 0)
  {
    rcbp->rc = -rc;
    rcbp->res = -1;
    state = ARK_CMD_DONE;
    goto ark_set_write_err;
  }
  else if (rc == 0)
  {
    state = ARK_IO_HARVEST;
  }
  else
  {
    state = ark_set_finish(_arkp, tid, tcbp);
  }

ark_set_write_err:

  return state;
}

int ark_set_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t     *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t     *rcbp     = &(_arkp->rcbs[tcbp->rtag]);
  int32_t    state    = ARK_CMD_DONE;

  HASH_SET(_arkp->ht, rcbp->pos, HASH_MAKE(1, tcbp->ttag, tcbp->nblk));

  rcbp->res = rcbp->vlen;
  if ( 1 == tcbp->new_key )
  {
    scbp->poolstats.kv_cnt++;
  }

  return state;
}
