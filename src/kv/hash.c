/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/hash.c $                                               */
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

hash_t *hash_new(uint64_t n) {
  hash_t *hash = am_malloc(sizeof(hash_t) + n * sizeof(uint64_t));
  if (hash)
  {
      bzero(hash, sizeof(hash_t) + n * sizeof(uint64_t));
      hash->n = n;
  }
  else
      {KV_TRC_FFDC(pAT, "HASH malloc FAILED");}
  return hash;
}

void hash_free(hash_t *hash)
{
  if (hash) {am_free(hash);}
  else      {KV_TRC_FFDC(pAT, "hash_free NULL");}
  return;
}

uint64_t hash_hash(uint8_t *buf, uint64_t n) {
  uint64_t sum = 0;
  uint64_t i;
  for (i=0; i<n; i++) sum = sum * 65559 + buf[i];
  return sum;
}

uint64_t hash_pos(hash_t *hash, uint8_t *buf, uint64_t n) {
  return hash_hash(buf, n) % hash->n;
}
