/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ht.h $                                                 */
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

#ifndef __HT_H__
#define __HT_H__

#include <stdint.h>

#include "bv.h"
#include "iv.h"


typedef struct _ht {
  uint64_t  n;
  uint64_t  m;
  BV *valid;
  IV *value;
} HT;

uint64_t ht_hash(uint8_t *buf, uint64_t n);
uint64_t ht_pos(HT *ht, uint8_t *buf, uint64_t n);

HT  *ht_new(uint64_t n, uint64_t m);
void ht_delete(HT *ht);

void      ht_set(HT *ht, uint64_t pos, uint64_t val);
uint64_t ht_get(HT *ht, uint64_t pos);

void      ht_clr(HT *ht, uint64_t pos);

int      ht_vldp(HT *ht, uint64_t pos);

uint64_t ht_cnt(HT *ht);

#endif
