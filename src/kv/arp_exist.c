/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp_exist.c $                                          */
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
void ark_exist_start(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  scb_t         *scbp     = &(_arkp->poolthreads[tid]);
  rcb_t         *rcbp     = &(_arkp->rcbs[tcbp->rtag]);
  tcb_t         *iotcbp   = &(_arkp->tcbs[rcbp->ttag]);
  iocb_t        *iocbp    = &(_arkp->iocbs[rcbp->ttag]);
  ark_io_list_t *bl_array = NULL;
  int32_t        rc       = 0;

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
    goto ark_exist_start_err;
  }

  // Set up the in-buffer to read in the hash bucket
  // that contains the key
  tcbp->blen = bl_len(_arkp->bl, tcbp->hblk);
  rc = bt_growif(&(tcbp->inb), &(tcbp->inb_orig), &(tcbp->inblen), 
                  (tcbp->blen * _arkp->bsize));
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "bt_growif failed tcbp:%p ttag:%d", tcbp, tcbp->ttag);
    rcbp->res = -1;
    rcbp->rc = rc;
    tcbp->state = ARK_CMD_DONE;
    goto ark_exist_start_err;
  }

  // Create a chain of blocks to be passed to be read
  bl_array = bl_chain(_arkp->bl, tcbp->hblk, tcbp->blen);
  if (bl_array == NULL)
  {
    KV_TRC_FFDC(pAT, "bl_chain failed tcbp:%p ttag:%d", tcbp, tcbp->ttag);
    rcbp->rc = ENOMEM;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
    goto ark_exist_start_err;
  }

  scbp->poolstats.io_cnt += tcbp->blen;

  KV_TRC_IO(pAT, "read hash entry ttag:%d", tcbp->ttag);
  ea_async_io_init(_arkp, ARK_EA_READ, (void *)tcbp->inb, bl_array,
                   tcbp->blen, 0, tcbp->ttag, ARK_EXIST_FINISH);
  if (ea_async_io_schedule(_arkp, tid, iotcbp, iocbp) &&
      ea_async_io_harvest (_arkp, tid, iotcbp, iocbp, rcbp))
  {
      ark_exist_finish(_arkp, tid, tcbp);
  }

ark_exist_start_err:

  return;
}

void ark_exist_finish(_ARK *_arkp, int tid, tcb_t *tcbp)
{
  rcb_t *rcbp = &(_arkp->rcbs[tcbp->rtag]);

  // Find the key position in the read in bucket
  rcbp->res = bt_exists(tcbp->inb, rcbp->klen, rcbp->key);
  if (rcbp->res == BT_FAIL)
  {
    KV_TRC_FFDC(pAT, "rc = ENOENT key %p, klen %"PRIu64" ttag:%d",
                  rcbp->key, rcbp->klen, tcbp->ttag);
    rcbp->rc = ENOENT;
    rcbp->res = -1;
    tcbp->state = ARK_CMD_DONE;
  }

  tcbp->state = ARK_CMD_DONE;
  return;
}

