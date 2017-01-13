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

// if successful returns length of value
void ark_get_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t            *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp     = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t            *iotcbp   = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t           *iocbp    = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t    *bl_array = NULL;
  int32_t           rc       = 0;

  scbp->poolstats.ops_cnt+=1;

  // Now that we have the hash entry, get the block
  // that holds the control information for the entry.
  tcbp->hblk = HASH_LBA(HASH_GET(_arkp->ht, rcbp->pos));

  // If there is no control block for this hash
  // entry, then the key is not present in the hash.
  // Set the error
  if ( tcbp->hblk == 0 )
  {
    KV_TRC_FFDC(pAT, "rc = ENOENT key %p, klen %"PRIu64" ttag:%d",
                 rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->res   = -1;
    rcbp->rc = ENOENT;
    tcbp->state = ARK_CMD_DONE;
    goto ark_get_start_err;
  }

  // Set up the in-buffer to read in the hash bucket
  // that contains the key
  tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);
  rc = bt_growif(&(tcbp->inb), &(tcbp->inb_orig), &(tcbp->inblen), 
                  (tcbp->blen * _arkp->bsize));
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "bt_growif failed ttag:%d", tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = rc;
    tcbp->state = ARK_CMD_DONE;
    goto ark_get_start_err;
  }

  // Create a chain of blocks to be passed to be read
  bl_array = bl_chain(_arkp->bl, tcbp->hblk, tcbp->blen);
  if (bl_array == NULL)
  {
    KV_TRC_FFDC(pAT, "bl_chain failed ttag:%d", tcbp->ttag);
    rcbp->rc = ENOMEM;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
    goto ark_get_start_err;
  }

  scbp->poolstats.io_cnt += tcbp->blen;

  KV_TRC(pAT, "RD_HASH tid:%d ttag:%3d", tid, tcbp->ttag);
  ea_async_io_init(_arkp, ARK_EA_READ, (void *)tcbp->inb, bl_array,
                   tcbp->blen, 0, tcbp->ttag, ARK_GET_PROCESS);

  if (iocbp->ea->st_type == EA_STORE_TYPE_MEMORY)
  {
      ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
      ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
      if (iotcbp->state == ARK_GET_PROCESS) {ark_get_process(_arkp, tid, tcbp);}
  }

ark_get_start_err:

  return;
}

void ark_get_process(_ARK *_arkp, int tid, tcb_t  *tcbp)
{
  scb_t            *scbp        = &(_arkp->poolthreads[tid]);
  rcb_t            *rcbp        = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t            *iotcbp      = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t           *iocbp       = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t    *bl_array    = NULL;
  uint8_t          *new_vb      = NULL;
  uint64_t          vblk        = 0;
  uint64_t          new_vbsize  = 0;

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
      if (tcbp->vvlen > tcbp->vbsize) {

        new_vbsize = (tcbp->blen * _arkp->bsize);
        new_vb = am_realloc(tcbp->vb_orig, new_vbsize);
        if ( new_vb == NULL )
        {
          KV_TRC_FFDC(pAT, "am_realloc failed ttag:%d", tcbp->ttag);
          rcbp->rc = ENOMEM;
          rcbp->res = -1;
          tcbp->state = ARK_CMD_DONE;
          goto ark_get_process_err;
        }

        // The realloc succeeded.  Set the new size, original
        // variable buffer, and adjusted variable buffer
        tcbp->vbsize = new_vbsize;
        tcbp->vb_orig = new_vb;
        tcbp->vb = ptr_align(tcbp->vb_orig);
      }

      // Create the block chain to be used for the IO
      bl_array = bl_chain(_arkp->bl, vblk, tcbp->blen);
      if (bl_array == NULL)
      {
        KV_TRC_FFDC(pAT, "bl_chain failed ttag:%d", tcbp->ttag);
        rcbp->rc = ENOMEM;
        rcbp->res = -1;
        tcbp->state = ARK_CMD_DONE;
        goto ark_get_process_err;
      }

      scbp->poolstats.io_cnt += tcbp->blen;

      KV_TRC(pAT, "RD_VAL  tid:%d ttag:%3d vlen:%5ld",
             tid, tcbp->ttag, rcbp->vlen);
      // Schedule the READ of the key's value into the
      // variable buffer.
      ea_async_io_init(_arkp, ARK_EA_READ, (void *)tcbp->vb,
                       bl_array, tcbp->blen, 0, tcbp->ttag, ARK_GET_FINISH);

      if (iocbp->ea->st_type == EA_STORE_TYPE_MEMORY)
      {
          ea_async_io_schedule(_arkp, tid, iotcbp, iocbp);
          ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp);
          if (iotcbp->state == ARK_GET_FINISH) {ark_get_finish(_arkp,tid,tcbp);}
      }
}
    else
    {
        ark_get_finish(_arkp, tid, tcbp);
    }
  } 
  else
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: key %p, klen %"PRIu64" ttag:%d",
                  rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->rc = ENOENT;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
  }

ark_get_process_err:

  return;
}

void ark_get_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  rcb_t  *rcbp  = &(_arkp->rcbs[tcbp->rtag]);

  // We've read in the variable buffer.  Now we copy it
  // into the passed in buffer.
  if ((rcbp->voff + rcbp->vlen) <= tcbp->vvlen)
  {
    memcpy(rcbp->val, (tcbp->vb + rcbp->voff), rcbp->vlen);
  }
  else
  {
    memcpy(rcbp->val, (tcbp->vb + rcbp->voff), (tcbp->vvlen - rcbp->voff));
  }
  rcbp->res = tcbp->vvlen;

  tcbp->state = ARK_CMD_DONE;

  return;
}

