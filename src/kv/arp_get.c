/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp_get.c $                                            */
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
void ark_get_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t   *scbp  = &(_arkp->poolthreads[tid]);
  rcb_t   *rcbp  = &(scbp->rcbs[tcbp->rtag]);
  iocb_t  *iocbp = &(scbp->iocbs[rcbp->ttag]);
  int32_t  rc    = 0;
  uint8_t  lck   = 0;

  scbp->poolstats.ops_cnt+=1;

  if (DUMP_KV)
  {
      char buf[256]={0};
      sprintf(buf, "HEX_KEY tid:%3d ttag:%3d pos:%6ld",
              tid, tcbp->ttag, rcbp->posI);
      KV_TRC_HEX(pAT, 4, buf, rcbp->key, rcbp->klen);
  }

  // Now that we have the hash entry, get the block
  // that holds the control information for the entry.
  HASH_GET(scbp->ht, rcbp->posI, &lck, &tcbp->hblk);

  // If there is no control block for this hash
  // entry, then the key is not present in the hash.
  // Set the error
  if (tcbp->hblk == 0)
  {
    KV_TRC(pAT, "ENOENT  tid:%3d ttag:%3d pos:%6ld key:%p klen:%ld",
                 tid, tcbp->ttag, rcbp->posI, rcbp->key, rcbp->klen);
    rcbp->res    = -1;
    rcbp->rc     = ENOENT;
    iocbp->state = ARK_CMD_DONE;
    goto ark_get_start_err;
  }

  // Set up the in-buffer to read in the hash bucket
  // that contains the key
  tcbp->blen = bl_len(scbp->bl, tcbp->hblk);

  if (kv_inject_flags && HTC_INUSE(scbp,rcbp->posI))
  {
      HTC_FREE(scbp,rcbp->posI);
  }

  if (tcbp->blen*_arkp->bsize > tcbp->inb_size)
  {
      KV_TRC_DBG(pAT, "RE_INB  tid:%3d ttag:%3d pos:%6ld old:%ld new:%ld",
                 tid, tcbp->ttag, rcbp->posI, tcbp->inb_size,
                 tcbp->blen*_arkp->bsize);
      rc = bt_realloc(&(tcbp->inb), &(tcbp->inb_orig), tcbp->blen*_arkp->bsize);
      if (rc != 0)
      {
          KV_TRC_FFDC(pAT, "bt_realloc failed ttag:%d", tcbp->ttag);
          rcbp->res    = -1;
          rcbp->rc     = rc;
          iocbp->state = ARK_CMD_DONE;
          goto ark_get_start_err;
      }
      tcbp->inb_size = tcbp->blen*_arkp->bsize;
  }

  scbp->poolstats.io_cnt  += tcbp->blen;
  scbp->poolstats.hcl_cnt += 1;
  scbp->poolstats.hcl_tot += tcbp->blen;

  if (HTC_HIT(scbp, rcbp->posI))
  {
      ++scbp->htc_hits;
      HTC_GET(scbp, rcbp->posI, tcbp->inb);
      KV_TRC(pAT, "HTC_HIT tid:%3d ttag:%3d pos:%6ld len:%ld",
             tid, tcbp->ttag,rcbp->posI, tcbp->inb->len);
      ark_get_process(_arkp, tid, tcbp);
      return;
  }

  // Create a chain of blocks to be passed to be read
  if (bl_rechain(&tcbp->aiol, scbp->bl,tcbp->hblk,tcbp->blen,tcbp->aiolN,scbp->offset))
  {
    rcbp->res    = -1;
    rcbp->rc     = ENOMEM;
    iocbp->state = ARK_CMD_DONE;
    KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
    goto ark_get_start_err;
  }
  tcbp->aiolN = tcbp->blen;

  KV_TRC(pAT, "RD_HASH tid:%3d ttag:%3d pos:%6ld blk#:%5ld hblk:%6ld nblks:%ld",
         tid, tcbp->ttag, rcbp->posI, tcbp->aiol[0].blkno,tcbp->hblk,tcbp->blen);

  ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_READ, (void *)tcbp->inb,
                   tcbp->aiol, tcbp->blen, 0, tcbp->ttag, ARK_GET_PROCESS);

ark_get_start_err:
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_get_process(_ARK *_arkp, int tid, tcb_t  *tcbp)
{
  scb_t            *scbp        = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp        = &(scbp->rcbs[tcbp->rtag]);
  iocb_t           *iocbp       = &(scbp->iocbs[rcbp->ttag]);
  uint8_t          *new_vb      = NULL;
  uint64_t          vblk        = 0;
  uint64_t          new_vbsize  = 0;

  KV_TRC_DBG(pAT, "INB_GET tid:%3d ttag:%3d pos:%6ld tot:%ld used:%ld",
             tid, tcbp->ttag, rcbp->posI, tcbp->inb_size, tcbp->inb->len);

  if (!HTC_INUSE(scbp,rcbp->posI) && HTC_AVAIL(scbp,tcbp->inb->len))
  {
      ++scbp->htcN;
      KV_TRC(pAT, "HTC_NEW tid:%3d ttag:%3d pos:%6ld htcN:%d len:%ld",
             tid, tcbp->ttag,rcbp->posI, scbp->htcN, tcbp->inb->len);
      HTC_SET(scbp, rcbp->posI, tcbp->inb);
  }

  // Find the key position in the read in bucket
  tcbp->vvlen = bt_get(tcbp->inb, rcbp->klen, rcbp->key, tcbp->vb);
  if (tcbp->vvlen >= 0)
  {
    // The key was found in the bucket. Now check to see
    // if the length of the value requires us to read into
    // the variable buffer.
    if (tcbp->vvlen > _arkp->vlimit)
    {
      vblk = *((uint64_t *)(tcbp->vb));

      // Determine the number of blocks needed for
      // the value
      tcbp->blen = divceil(tcbp->vvlen, _arkp->bsize);

      // Check to see if the current size of the variable
      // buffer is big enough to hold the value.
      if (tcbp->vvlen > tcbp->vbsize)
      {
        void *old_vb = tcbp->vb_orig;
        new_vbsize   = (tcbp->blen * _arkp->bsize);
        am_free(tcbp->vb_orig);
        new_vb = am_malloc(new_vbsize + ARK_ALIGN);
        KV_TRC_DBG(pAT, "RE_VB   tid:%3d ttag:%3d pos:%6ld old:%ld new:%ld "
                         "old:%p new:%p",
             tid, tcbp->ttag, rcbp->posI, tcbp->vbsize, new_vbsize, old_vb, new_vb);
        if (!new_vb)
        {
          KV_TRC_FFDC(pAT, "am_malloc failed ttag:%d", tcbp->ttag);
          rcbp->rc     = ENOMEM;
          rcbp->res    = -1;
          iocbp->state = ARK_CMD_DONE;
          goto ark_get_process_err;
        }

        // The realloc succeeded.  Set the new size, original
        // variable buffer, and adjusted variable buffer
        tcbp->vbsize  = new_vbsize;
        tcbp->vb_orig = new_vb;
        tcbp->vb      = ptr_align(tcbp->vb_orig);
      }

      // Create the block chain to be used for the IO
      if (bl_rechain(&tcbp->aiol, scbp->bl, vblk, tcbp->blen,tcbp->aiolN,scbp->offset))
      {
        rcbp->res    = -1;
        rcbp->rc     = ENOMEM;
        iocbp->state = ARK_CMD_DONE;
        KV_TRC_FFDC(pAT, "bl_rechain failed, ttag:%d", tcbp->ttag);
        goto ark_get_process_err;
      }
      tcbp->aiolN = tcbp->blen;

      scbp->poolstats.io_cnt += tcbp->blen;

      KV_TRC(pAT, "RD_VAL  tid:%3d ttag:%3d pos:%6ld vlen:%5ld",
             tid, tcbp->ttag, rcbp->posI, rcbp->vlen);
      // Schedule the READ of the key's value into the
      // variable buffer.
      ea_async_io_init(_arkp, scbp->ea, iocbp, ARK_EA_READ, (void *)tcbp->vb,
                       tcbp->aiol, tcbp->blen, 0, tcbp->ttag, ARK_GET_FINISH);
    }
    else
    {
        KV_TRC(pAT, "INLINE  tid:%3d ttag:%3d pos:%6ld vlen:%5ld",
               tid, tcbp->ttag, rcbp->posI, rcbp->vlen);
        ark_get_finish(_arkp, tid, tcbp);
        return;
    }
  }
  else
  {
    KV_TRC(pAT, "ENOENT  tid:%3d ttag:%3d pos:%6ld key:%p klen:%ld",
                tid, tcbp->ttag, rcbp->posI, rcbp->key, rcbp->klen);
    rcbp->rc    = ENOENT;
    rcbp->res   = -1;
    iocbp->state = ARK_CMD_DONE;
  }

ark_get_process_err:

  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_get_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t  *scbp  = &(_arkp->poolthreads[tid]);
  rcb_t  *rcbp  = &(_arkp->poolthreads[tid].rcbs[tcbp->rtag]);
  iocb_t *iocbp = &(scbp->iocbs[rcbp->ttag]);

  if (DUMP_KV)
  {
      char buf[256]={0};
      sprintf(buf, "HEX_VAL tid:%3d ttag:%3d pos:%6ld",
              tid, tcbp->ttag, rcbp->posI);
      KV_TRC_HEX(pAT, 4, buf, tcbp->vb, rcbp->vlen);
  }

  // We've read in the variable buffer. Now we copy it to the passed in buffer
  if ((rcbp->voff + rcbp->vlen) <= tcbp->vvlen)
  {
    memcpy(rcbp->val, (tcbp->vb + rcbp->voff), rcbp->vlen);
  }
  else
  {
    memcpy(rcbp->val, (tcbp->vb + rcbp->voff), (tcbp->vvlen - rcbp->voff));
  }

  rcbp->res    = tcbp->vvlen;
  iocbp->state = ARK_CMD_DONE;

  return;
}

