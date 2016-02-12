/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bt.c $                                                 */
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <arkdb_trace.h>

#include "am.h"
#include "vi.h"
#include "bt.h"
#include <errno.h>

#include <arkdb_trace.h>

BT *bt_new(uint64_t dlen, uint64_t vmx,  uint64_t vdf, uint64_t *btsize, BT **bt_orig) {
  BT *bt = NULL;

  *bt_orig = am_malloc(sizeof(BT) + dlen);
  if (*bt_orig == NULL) {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory dlen %ld vmx %ld vdf %ld, errno = %d",
            dlen, vmx, vdf, errno);
    if (btsize != NULL)
    {
      *btsize = 0;
    }
  }
  else
  {
    bt = (BT *)ptr_align(*bt_orig);
    bt->len = sizeof(BT);
    bt->cnt = 0;
    bt->max = vmx;
    bt->def = vdf;
    bt->dlen = dlen;
    if (btsize != NULL)
    {
      *btsize = sizeof(BT) + dlen;
    }
  }
  KV_TRC(pAT, "bt %p dlen %ld vmx %ld vdf %ld",
          bt, dlen, vmx, vdf);
  return bt;
}

int bt_growif(BT **bt, BT **bt_orig, uint64_t *lim, uint64_t size) {
  BT *btret = NULL;
  int rc = 0;

  // We need to include the size of the BT header struct
  // This is a bit of a hack, but we know that a
  // struct BT will fit within a block size of 4K
  uint64_t newsize = (size + 4096);

  // If the sie of bt is already sufficient, do nothing
  // and just return success.
  if (newsize > *lim) {
    btret = am_realloc(*bt_orig, newsize);
    if (btret == NULL)
    {
      KV_TRC_FFDC(pAT, "bt realloc failed");
      // An error was encountered with realloc
      // Do not lose the original bt, just retur
      // the error
      rc = errno;
    }
    else
    {
      // Realloc succeeded, set bt to the new
      // space
      *bt_orig = btret;
      *bt = (BT *)ptr_align(btret);
      (*bt)->dlen = newsize - sizeof(BT);
      *lim = newsize;
    }
  } 

  return rc;
}

int bt_init(BT *bt) {
  if (bt) {
    bt->len = sizeof(BT);
    bt->cnt = 0;
    return BT_OK;
  } else {
    errno = EINVAL;
    KV_TRC_FFDC(pAT, "bt %p, errno = %d", bt, errno);
    return BT_FAIL;
  }
}


void bt_delete(BT *bt)
{
  if (bt) {am_free(bt); KV_TRC_DBG(pAT, "bt %p", bt);}
  else    {KV_TRC_FFDC(pAT, "bt NULL");}
}

int64_t bt_exists(BT *bt, uint64_t kl, uint8_t *k) {
  uint64_t i;
  //uint64_t off = 0;
  uint8_t *buf = bt->data;
  for(i=0; i<bt->cnt; i++) {
    uint64_t klen = 0;
    uint64_t vlen = 0;
    //uint64_t bytes = 0;
    buf += vi_dec64(buf, &klen);
    buf += vi_dec64(buf, &vlen);
    if (kl==klen && memcmp(buf, k, klen)==0) {
      return vlen;
    }
    buf += klen + (vlen > bt->max ? bt->def : vlen);
  }
  errno = EINVAL;
  KV_TRC_FFDC(pAT, "bt %p kl %ld, k %p, rc = %d", bt, kl, k, errno);
  return BT_FAIL;
}

int bt_set(BT *bto,BT *bti, uint64_t kl, uint8_t *k, uint64_t vl, uint8_t *v, uint64_t *ovlen) {
  uint8_t *src = bti->data;
  uint8_t *dst = bto->data;
  uint64_t bytes = 0;
  dst += vi_enc64(kl, dst);
  dst += vi_enc64(vl, dst);
  memcpy(dst, k, kl);
  dst += kl;
  bytes = vl > bti->max ? bti->def : vl;
  memcpy(dst, v, bytes);
  dst += bytes;
  int add = 1;
  uint64_t pkl;
  uint64_t pvl;
  int64_t i;
  //  uint64_t inc = 0;
  for(i=0 ; i<bti->cnt; i++) {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0) {
      add = 0;
      *ovlen = pvl;
      src += (pkl + (pvl > bti->max ? bti->def : pvl));
      bytes = bti->len - sizeof(BT) - (src - bti->data);
      memcpy(dst, src, bytes);
      dst += bytes;
      i=bti->cnt;
    } else {
      bytes = pkl + (pvl>bti->max ? bti->def : pvl);
      dst += vi_enc64(pkl,dst);
      dst += vi_enc64(pvl,dst);
      memcpy(dst,src, bytes);
      src += bytes;
      dst += bytes;
    }
  }
  bto->len = sizeof(BT) + (dst - bto->data);
  bto->cnt = bti->cnt + add;
  bto->def = bti->def;
  bto->max = bti->max;
  return add;
}

int64_t bt_del(BT *bto, BT *bti, uint64_t kl, uint8_t *k) {
  uint8_t *src = bti->data;
  uint8_t *dst = bto->data;
  uint64_t pkl;
  uint64_t pvl;
  int64_t i;
  int64_t ret = -1;
  int del = 0;
  uint64_t bytes = 0;

  //  uint64_t inc = 0;
  for(i=0 ; i<bti->cnt; i++) {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0) {
      del = 1;
      src += (pkl + (pvl > bti->max ? bti->def : pvl));
      bytes = bti->len - sizeof(BT) - (src - bti->data);
      memcpy(dst, src, bytes);
      dst += bytes;
      ret = pvl;
      i=bti->cnt;
    } else {
      bytes = pkl + (pvl>bti->max ? bti->def : pvl);
      dst += vi_enc64(pkl,dst);
      dst += vi_enc64(pvl,dst);
      memcpy(dst,src, bytes);
      src += bytes;
      dst += bytes;
    }
  }
  bto->len = sizeof(BT) + (dst - bto->data);
  bto->cnt = bti->cnt -del;
  bto->def = bti->def;
  bto->max = bti->max;

  if ( ret == -1 )
  {
    errno = EINVAL;
    KV_TRC_FFDC(pAT, "Key not found bto %p bti %p, kl %ld k %p, rc = %d",
            bto, bti, kl, k, errno);
  }
  return ret;
}
// same as bt_del except returns the val if vlen > vlimit as this ref may be used to 
// to delete other things ref will be filled with def bytes
// this could replace bt_del at the cost of the extra parameter
int64_t bt_del_def(BT *bto, BT *bti, uint64_t kl, uint8_t *k, uint8_t *ref, uint64_t *ovlen) {
  uint8_t *src = bti->data;
  uint8_t *dst = bto->data;
  uint64_t pkl;
  uint64_t pvl;
  int64_t i;
  int64_t ret = -1;
  int del = 0;
  uint64_t bytes = 0;
  //  uint64_t inc = 0;
  for(i=0 ; i<bti->cnt; i++) {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0) {
      if (ref) {
        if (pvl > bti->max)
          memcpy(ref, src + pkl, bti->def);
        else
          memset(ref, 0x00, bti->def);
      }
      *ovlen = pvl;
      del = 1;
      src += (pkl + (pvl > bti->max ? bti->def : pvl));
      bytes = bti->len - ((uint64_t)src - (uint64_t)bti);
      memcpy(dst, src, bytes);
      dst += bytes;
      ret = pvl;
      i=bti->cnt;
    } else {
      bytes = pkl + (pvl>bti->max ? bti->def : pvl);
      dst += vi_enc64(pkl,dst);
      dst += vi_enc64(pvl,dst);
      memcpy(dst,src, bytes);
      src += bytes;
      dst += bytes;
    }
  }
  bto->len = sizeof(BT) + (dst - bto->data);
  bto->cnt = bti->cnt - del;
  bto->def = bti->def;
  bto->max = bti->max;

  if ( ret == -1 )
  {
    errno = EINVAL;
    KV_TRC_FFDC(pAT, "Key not found bto %p bti %p, kl %ld k %p ref %p, rc = %d",
            bto, bti, kl, k, ref, errno);
  }

  return ret;
}




int64_t bt_get(BT *bti, uint64_t kl, uint8_t *k, uint8_t *v) {
  uint8_t *src = bti->data;
  uint64_t pkl;
  uint64_t pvl;
  int64_t i;
  //  uint64_t inc = 0;
  for(i=0 ; i<bti->cnt; i++) {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0) {
      src += kl;
      memcpy(v,src,(pvl>bti->max ? bti->def : pvl));
      return pvl;
    } else {
      src += pkl;
      src += (pvl>bti->max ? bti->def : pvl);
    }
  }

  errno = EINVAL;
  KV_TRC_FFDC(pAT, "bti %p, kl %ld k %p, rc = %d",
          bti, kl, k, errno);
  return -1;
}



void bt_dump(BT *bt) {
  uint64_t kl;
  uint64_t vl;
  uint64_t i,j;
  printf("    ----\n    Bkt: %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64"\n", bt->len, bt->cnt, bt->max, bt->def);
  if (bt->cnt==0) {
    printf("      --empty--\n");
  } else {
    uint8_t *buf = bt->data;
    for(i=0; i<bt->cnt; i++) {
      buf += vi_dec64(buf, &kl);
      buf += vi_dec64(buf, &vl);
      printf("      %"PRIu64": %"PRIu64" %"PRIu64" '", i,kl, vl);
      for(j=0; j<kl; j++) printf("%02x",buf[j]);
      buf += kl;
      printf("'->'");
      for(j=0; j<(vl>bt->max ? bt->def : vl); j++) printf("%02x",buf[j]);
      buf += vl;
      printf("'\n");
    }
    printf("    ----\n");
  }
}

void bt_cstr(BT *bt) {
  uint64_t kl;
  uint64_t vl;
  uint64_t i,j;
  printf("    ----\n    Bkt: %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64"\n", bt->len, bt->cnt, bt->max, bt->def);
  if (bt->cnt==0) {
    printf("      --empty--\n");
  } else {
    uint8_t *buf = bt->data;
    for(i=0; i<bt->cnt; i++) {
      buf += vi_dec64(buf, &kl);
      buf += vi_dec64(buf, &vl);
      printf("    %"PRIu64": %"PRIu64" %"PRIu64" '", i,kl, vl);
      for(j=0; j<kl; j++) printf("%c",buf[j]);
      buf += kl;
      printf("'->'");
      for(j=0; j<(vl>bt->max ? bt->def : vl); j++) printf("%c",buf[j]);
      buf += vl;
      printf("'\n");
    }
    printf("    ----\n");
  }
}
