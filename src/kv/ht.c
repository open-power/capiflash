/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ht.c $                                                 */
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

uint64_t ht_hash(uint8_t *buf, uint64_t n) {
  uint64_t sum = 0;
  uint64_t i;
  for (i=0; i<n; i++) sum = sum * 65559 + buf[i];
  return sum;
}

uint64_t ht_pos(HT *ht, uint8_t *buf, uint64_t n) {
  return ht_hash(buf, n) % ht->n;
}


HT *ht_new(uint64_t n, uint64_t m) {
  HT *ht = am_malloc(sizeof(HT));
  if ( ht == NULL )
  {
      KV_TRC_FFDC(pAT, "n %ld m %ld ENOMEM", n, m);
    errno = ENOMEM;
  }
  else
  {
    ht->n = n;
    ht->m = m;
    ht->valid = bv_new(n);
    ht->value = iv_new(n,m);
    KV_TRC(pAT, "n %ld m %ld", n, m);
  }
  return ht;
}

void ht_delete(HT *ht) {
  KV_TRC(pAT, "ht %p valid %p value %p", ht, ht->valid, ht->value);
  bv_delete(ht->valid);
  iv_delete(ht->value);
  am_free(ht);
}


void ht_set(HT *ht, uint64_t pos, uint64_t val) {
  bv_set(ht->valid, pos);
  iv_set(ht->value, pos, val);
} 

int64_t ht_get(HT *ht, uint64_t pos)
{
    int64_t v;
    if ((v=iv_get(ht->value,pos)) < 0)
    {
        KV_TRC_FFDC(pAT, "invalid pos:%ld", pos);
    }
    return v;
}

void     ht_clr(HT *ht, uint64_t pos) {
    KV_TRC(pAT, "valid %p pos %ld", ht->valid, pos);
  bv_clr(ht->valid,pos);
}

int      ht_vldp(HT *ht, uint64_t pos) {
  return bv_get(ht->valid, pos);
}

