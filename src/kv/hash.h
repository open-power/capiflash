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
#define __HASHH__
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct _hash {
  uint64_t n;
  uint64_t h[];
} hash_t;

hash_t *hash_new(uint64_t n);
void hash_free(hash_t *hash);

uint64_t hash_hash(uint8_t *buf, uint64_t n);
uint64_t hash_pos(hash_t *hash, uint8_t *buf, uint64_t n);

#define HASH_GET(htb, pos) ((htb)->h[pos])
#define HASH_SET(htb, pos, val) ((htb)->h[pos] = val)

#define HASH_MAKE(lck,tag,lba) ((((uint64_t)(lck))<<56) | (((uint64_t)(tag))<<40) | ((uint64_t)(lba)))
#define HASH_LCK(x) ((x)>>56)
#define HASH_TAG(x) (0x000000000000FFFFULL & ((x)>>40))
#define HASH_LBA(x) (0x000000FFFFFFFFFFULL & (x))


#endif
