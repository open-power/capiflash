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
#include <ark.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
BT *bt_new(uint64_t dlen, uint64_t vmx, uint64_t vdf, BT **bt_orig)
{
    BT *bt = NULL;

  *bt_orig = am_malloc(sizeof(BT) + dlen + ARK_ALIGN);
  if (*bt_orig == NULL)
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory dlen %ld vmx %ld vdf %ld, errno = %d",
                dlen, vmx, vdf, errno);
  }
  else
  {
    bt       = (BT *)ptr_align(*bt_orig);
    bt->len  = sizeof(BT);
    bt->cnt  = 0;
    bt->max  = vmx;
    bt->def  = vdf;
  }
  KV_TRC_DBG(pAT, "BT_NEW: bt:%p bt_orig:%p len:%ld tot:%ld vmx:%ld vdf:%ld",
             bt, *bt_orig, (uint64_t)bt->len, dlen, vmx, vdf);
  return bt;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int bt_realloc(BT **bt, BT **bt_orig, uint64_t size)
{
  BT       *btret = NULL;
  BT       *btnew = NULL;
  int       rc    = 0;

  btnew = am_realloc(*bt_orig, size + ARK_ALIGN);
  if (!btnew)
  {
      rc = errno;
      KV_TRC_FFDC(pAT, "bt realloc failed, errno:%d", rc);
  }
  else
  {
      btret       = (BT*)ptr_align(btnew);
      *bt_orig    = btnew;
      *bt         = btret;
      KV_TRC_DBG(pAT, "REALLOC bt:%p orig:%p size:%ld", *bt, *bt_orig, size);
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bt_clear(BT *bt)
{
  if (bt)
  {
      bt->cnt = 0;
      bt->len = sizeof(BT);
  }
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bt_delete(BT *bt)
{
  if (bt)
  {
      KV_TRC_DBG(pAT, "free bt:%p", bt);
      am_free(bt);
  }
  else {KV_TRC_FFDC(pAT, "bt NULL");}
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bt_exists(BT *bt, uint64_t kl, uint8_t *k)
{
  uint64_t i;
  uint8_t *buf = bt->data;

  for (i=0; i<bt->cnt; i++)
  {
    uint64_t klen = 0;
    uint64_t vlen = 0;

    buf += vi_dec64(buf, &klen);
    buf += vi_dec64(buf, &vlen);
    if (kl==klen && memcmp(buf, k, klen)==0) {return vlen;}
    buf += klen + (vlen > bt->max ? bt->def : vlen);
  }
  return BT_FAIL;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int bt_set(BT       *bto,  BT       *bti,
           uint64_t  kl,   uint8_t  *k,
           uint64_t  vl,   uint8_t  *v,
           uint64_t *ov,   uint64_t *ovl)
{
    uint8_t *src   = bti->data;
    uint8_t *dst   = bto->data;
    uint64_t bytes = 0;

    if (!bto || !bti)
    {
        KV_TRC_FFDC(pAT, "FFDC    bto:%p bti:%p", bto, bti);
        return -1;
    }

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

    for(i=0; i<bti->cnt; i++)
    {
        src += vi_dec64(src, &pkl);
        src += vi_dec64(src, &pvl);
        if (pkl==kl && memcmp(src,k,kl)==0)
        {
            add  = 0;
            *ovl = pvl;
            if (pvl>bti->max) {memcpy(ov,src+pkl,bti->def);}
            src += (pkl + (pvl > bti->max ? bti->def : pvl));
        }
        else
        {
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

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bt_del(BT *bto, BT *bti, uint64_t kl, uint8_t *k)
{
  uint8_t *src   = bti->data;
  uint8_t *dst   = bto->data;
  uint64_t pkl   = 0;
  uint64_t pvl   = 0;
  int64_t  i     = 0;
  int64_t  ret   = -1;
  int      del   = 0;
  uint64_t bytes = 0;

  for(i=0 ; i<bti->cnt; i++)
  {
      src += vi_dec64(src, &pkl);
      src += vi_dec64(src, &pvl);

      if (pkl==kl && memcmp(src,k,kl)==0)
      {
          del   = 1;
          src  += (pkl + (pvl > bti->max ? bti->def : pvl));
          bytes = bti->len - sizeof(BT) - (src - bti->data);
          memcpy(dst, src, bytes);
          dst  += bytes;
          ret   = pvl;
          break;
      }
      else
      {
          bytes = pkl + (pvl > bti->max ? bti->def : pvl);
          dst  += vi_enc64(pkl,dst);
          dst  += vi_enc64(pvl,dst);
          memcpy(dst,src,bytes);
          src  += bytes;
          dst  += bytes;
      }
  }
  bto->len = sizeof(BT) + (dst - bto->data);
  bto->cnt = bti->cnt -del;
  bto->def = bti->def;
  bto->max = bti->max;

  if (ret == -1)
  {
    errno = EINVAL;
    KV_TRC_FFDC(pAT, "Key not found bto %p bti %p, kl %ld k %p, rc = %d",
            bto, bti, kl, k, errno);
  }
  return ret;
}

/**
 *******************************************************************************
 * \brief
 * same as bt_del except returns the val if vlen > vlimit as this ref may be
 * used to to delete other things ref will be filled with def bytes
 * this could replace bt_del at the cost of the extra parameter
 ******************************************************************************/
int64_t bt_del_def(BT *bto, BT *bti, uint64_t kl, uint8_t *k, uint8_t *ref, uint64_t *ovlen)
{
  uint8_t *src = bti->data;
  uint8_t *dst = bto->data;
  uint64_t pkl;
  uint64_t pvl;
  int64_t i;
  int64_t ret = -1;
  int del = 0;
  uint64_t bytes = 0;

  for (i=0 ; i<bti->cnt; i++)
  {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0)
    {
      if (ref)
      {
        if (pvl > bti->max) {memcpy(ref, src + pkl, bti->def);}
        else                {memset(ref, 0x00, bti->def);}
      }
      *ovlen = pvl;
      del = 1;
      src += (pkl + (pvl > bti->max ? bti->def : pvl));
      bytes = bti->len - ((uint64_t)src - (uint64_t)bti);
      memcpy(dst, src, bytes);
      dst += bytes;
      ret = pvl;
      i=bti->cnt;
    }
    else
    {
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

  if (ret == -1)
  {
    errno = EINVAL;
    KV_TRC_FFDC(pAT, "Key not found bto %p bti %p, kl %ld k %p ref %p, rc = %d",
            bto, bti, kl, k, ref, errno);
  }

  return ret;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bt_find(BT *bti, uint64_t kl, uint8_t *k, uint8_t **v)
{
  uint8_t *src = bti->data;
  uint64_t pkl;
  uint64_t pvl;
  int64_t  i;

  for(i=0 ; i<bti->cnt; i++)
  {
    src += vi_dec64(src, &pkl);
    src += vi_dec64(src, &pvl);
    if (pkl==kl && memcmp(src,k,kl)==0)
    {
      src += kl;
      *v = src;
      return pvl;
    }
    else
    {
      src += pkl;
      src += (pvl>bti->max ? bti->def : pvl);
    }
  }

  return -1;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bt_get(BT *bti, uint64_t kl, uint8_t *k, uint8_t *v)
{
  uint8_t *pv  = NULL;
  uint64_t pvl = -1;

  if ((pvl=bt_find(bti, kl, k, &pv)) > 0)
  {
      if (v && pv) {memcpy(v, pv, (pvl>bti->max ? bti->def : pvl));}
  }

  return pvl;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bt_get_vdf(BT *bti, uint64_t kl, uint8_t *k, uint8_t *vdf)
{
  uint8_t *pv  = NULL;
  uint64_t pvl = -1;

  if ((pvl=bt_find(bti, kl, k, &pv)) > 0)
  {
      if (vdf && pv) {memcpy(vdf, pv, bti->def);}
  }
  return pvl;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bt_rawdump(void *p, int bytes)
{
    uint32_t  i;
    uint8_t  *pp=(uint8_t*)p;

    for (i=0; i<bytes; i++) {printf("%02x",pp[i]);}
    printf("\n");
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bt_dump(BT *bt)
{
  uint64_t kl;
  uint64_t vl;
  uint64_t i,j;

  printf("    ----\n    Bkt: len:%ld cnt:%d max:%d def:%d\n",
          (uint64_t)bt->len, bt->cnt, bt->max, bt->def);

  if (bt->cnt==0)
  {
    printf("      --empty--\n");
  }
  else
  {
    uint8_t *buf = bt->data;

    for(i=0; i<bt->cnt; i++)
    {
      buf += vi_dec64(buf, &kl);
      buf += vi_dec64(buf, &vl);

      printf("      i:%ld: kl:%ld vl:%ld =>'", i,kl,vl);

      for(j=0; j<kl; j++) {printf("%02x",buf[j]);}
      buf += kl;
      printf("'->'");

      for(j=0; j<(vl>bt->max ? bt->def : vl); j++) {printf("%02x",buf[j]);}
      buf += (vl>bt->max ? bt->def : vl);
      printf("'\n");
    }
    printf("    ----\n");
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bt_cstr(BT *bt)
{
  uint64_t kl;
  uint64_t vl;
  uint64_t i,j;

  printf("    ----\n    Bkt: len:%ld tot:%ld cnt:%d max:%d def:%d\n",
                  (uint64_t)bt->len, sizeof(bt), bt->cnt, bt->max, bt->def);

  if (bt->cnt==0)
  {
    printf("      --empty--\n");
  }
  else
  {
    uint8_t *buf = bt->data;
    for (i=0; i<bt->cnt; i++)
    {
      buf += vi_dec64(buf, &kl);
      buf += vi_dec64(buf, &vl);
      printf("    i:%ld: klen:%ld vlen:%ld '", i,kl, vl);
      for (j=0; j<kl; j++) printf("%c",buf[j]);
      buf += kl;
      printf("'->'");
      for (j=0; j<(vl>bt->max ? bt->def : vl); j++) printf("%c",buf[j]);
      buf += (vl>bt->max ? bt->def : vl);
      printf("'\n");
    }
    printf("    ----\n");
  }
}
