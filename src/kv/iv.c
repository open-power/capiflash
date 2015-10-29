/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/iv.c $                                                 */
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
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "am.h"

#include "ut.h"
#include "iv.h"
#include <arkdb_trace.h>


#define IVINLINE // inline
#define IVSAFE   // if (i<0 & i>=bv->n) exit(987);
  
IV *iv_new(uint64_t n, uint64_t m) {

  uint64_t bits  = n * m;
  uint64_t words = divup(bits, 64);
  uint64_t bytes = sizeof(IV) + words * sizeof(uint64_t);

  IV *iv = am_malloc(bytes);
  if (iv == NULL)
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "FFDC: n %"PRIu64" m %"PRIu64", errno = %d", n, m, errno);
  }
  else
  {
    memset(iv,0x00, bytes);
    iv->n = n;
    iv->m = m;
    iv->bits = bits;
    iv->words = words;
    iv->mask = 1;
    iv->mask <<= m;
    iv->mask -= 1;
    iv->bar = 64 - m;
  }

  KV_TRC(pAT, "iv %p n %"PRIu64" m %"PRIu64"", iv, n, m);
  return iv;
}

IV *iv_resize(IV *piv, uint64_t n, uint64_t m) {

  uint64_t bits  = n * m;
  uint64_t words = divup(bits, 64);
  uint64_t bytes = sizeof(IV) + words * sizeof(uint64_t);

  IV *iv = am_realloc(piv,bytes);
  if (iv == NULL)
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "FFDC: iv %p n %"PRIu64" m %"PRIu64", errno = %d", piv, n, m, errno);
  }
  else
  {
    iv->n = n;
    iv->m = m;
    iv->bits = bits;
    iv->words = words;
    iv->mask = 1;
    iv->mask <<= m;
    iv->mask -= 1;
    iv->bar = 64 - m;
  }

  KV_TRC_DBG(pAT, "iv %p n %"PRIu64" m %"PRIu64"", piv, n, m);
  return iv;
}

void iv_set(IV *iv, uint64_t i, uint64_t v) {
  uint64_t pos = i * iv->m;
  uint64_t w = pos >> 6;
  uint64_t b = pos & 63;
  uint64_t shift;
  uint64_t msk0;
  uint64_t msk1;
  uint64_t val;


  v &= iv->mask;
  if (b <= iv->bar) {
    shift = iv->bar - b;
    msk1   = iv->mask << shift;
    msk0  = ~msk1;
    val   = v << shift;
    val |= (iv->data[w] & msk0);
    iv->data[w] =  val;
  } else {
    shift = b - iv->bar;
    msk1  = iv->mask >> shift;
    msk0  = ~msk1;
    val   = v >> shift;
    val |= (iv->data[w] & msk0);
    iv->data[w] = val;
    shift = 64 - (b - iv->bar);
    msk1  = iv->mask << shift;
    msk0  = ~msk1;
    val   = v << shift;
    val |= (iv->data[w+1] & msk0);
    iv->data[w+1] = val;
  }
}
  
uint64_t iv_get(IV *iv, uint64_t i) {
  uint64_t bp = i * iv->m;
  uint64_t w = bp >> 6;
  uint64_t b = bp & 63;
  uint64_t shift;
  uint64_t val0;
  uint64_t val1;
  uint64_t msk0;
  uint64_t msk1;
  uint64_t val;
  if (b <= iv->bar) {
    shift = iv->bar - b;
    val = iv->mask & (iv->data[w] >> shift);
  } else {
    shift = b - iv->bar;
    msk0 = iv->mask>>shift;
    msk1 = iv->mask >> (iv->m-shift);
    val0 = (msk0  & iv->data[w]) << shift;
    val1 = msk1 & (iv->data[w+1] >> (64-shift));
    val = val0 | val1;
  }
  return val;
}

/* void iv_resize(IV *iv, uint64_t n) { */
/*   uint64_t bits  = n * iv->m; */
/*   uint64_t words = divup(bits, 64); */
/*   uint64_t bytes = sizeof(IV) + words * sizeof(uint64_t); */
/*   IV *iv = realloc(bytes); */
  
void    iv_delete(IV *iv) {
    KV_TRC(pAT, "iv %p", iv);
    am_free(iv);
}

