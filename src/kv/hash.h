/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/hash.h  $                                              */
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

#ifndef __HASH_H__
#define __HASH_H__

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <iv.h>

#include <errno.h>

typedef struct
{
  pthread_rwlock_t l;
  uint64_t         n;
  uint64_t         m;
  uint64_t         lba_mask;
  IV              *iv;
} hash_t;

#define HASH_SET(htp, _pos, _lck, _lba) hash_set(htp,_pos,_lck,_lba)
#define HASH_GET(htp, _pos, _lck, _lba) hash_get(htp,_pos,_lck,_lba)
#define HASH_POS(_htN, _buf, _blen)     hash_pos(_htN,_buf,_blen)

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline uint64_t hash_hash(uint8_t *buf, uint64_t n)
{
  uint64_t sum = 0;
  uint64_t i;
  for (i=0; i<n; i++) sum = sum * 65559 + buf[i];
  return sum;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline uint64_t hash_pos(uint64_t htN, uint8_t *buf, uint64_t blen)
{
    return hash_hash(buf, blen) % htN;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline hash_t *hash_new(uint64_t n, uint64_t m)
{
  hash_t *ht = (hash_t*)am_malloc(sizeof(hash_t));
  if (!ht)
  {
    KV_TRC_FFDC(pAT, "n %ld m %ld ENOMEM", n, m);
    errno = ENOMEM;
  }
  else
  {
    pthread_rwlock_init(&ht->l, NULL);
    ht->n        = n;
    ht->m        = m;
    ht->lba_mask = 1;
    ht->lba_mask = (ht->lba_mask<<(m-1))-1;
    ht->iv     = iv_new(n,m);
    KV_TRC(pAT, "n %ld m %ld lba_mask:%16lx", n, m, ht->lba_mask);
  }
  return ht;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline void hash_free(hash_t *ht)
{
  KV_TRC(pAT, "ht %p iv %p", ht, ht->iv);
  if (ht)
  {
      iv_delete(ht->iv);
      am_free(ht);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline void hash_set(hash_t  *ht,
                            uint64_t pos,
                            uint8_t  lck,
                            uint64_t lba)
{
  uint64_t val = (((uint64_t)lck)<<(ht->m-1)) | (uint64_t)lba;
  pthread_rwlock_wrlock(&ht->l);
  iv_set(ht->iv, pos, val);
  pthread_rwlock_unlock(&ht->l);
  KV_TRC_DBG(pAT, "HASHSET iv[%ld]=%lx iv[0]=%lx", pos,val,ht->iv->data[0]);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline void hash_get(hash_t   *ht,
                            uint64_t  pos,
                            uint8_t  *lck,
                            uint64_t *lba)
{
    int64_t v=0;
    pthread_rwlock_rdlock(&ht->l);
    if ((v=iv_get(ht->iv,pos)) < 0)
    {
        KV_TRC_FFDC(pAT, "invalid pos:%ld", pos);
    }
    pthread_rwlock_unlock(&ht->l);
    *lck = v >> (ht->m-1);
    *lba = v & ht->lba_mask;
    KV_TRC_DBG(pAT, "HASHGET iv[%ld]=%lx lck:%x lba:%ld", pos,v,*lck,*lba);
    return;
}

#endif
