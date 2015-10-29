/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bv.c $                                                 */
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
#include <errno.h>

#include "am.h"
#include "ut.h"
#include "bv.h"

#define BPW 8
#define WRD uint8_t
#define SHF 3
#define MSK 7

/* #define BPW 16 */
/* #define WRD uint16_t */
/* #define SHF 4 */
/* #define MSK 15 */

/* #define BPW 32 */
/* #define WRD uint32_t */
/* #define SHF 5 */
/* #define MSK 31 */

/* #define BPW 64 */
/* #define WRD uint64_t */
/* #define SHF 6 */
/* #define MSK 63 */

#define BVINLINE // inline
#define BVSAFE   // if (i<0 & i>=bv->n) exit(987);
  
BV *bv_new(uint64_t n) {
  uint64_t nw = divup(n, BPW);
  BV *p = am_malloc(sizeof(BV) + nw * sizeof(WRD));
  if (p == NULL)
  {
    errno = ENOMEM;
  }
  else
  {
    memset(p->bits,0x00, nw * sizeof(WRD));
    p->n = n;
    p->nw = nw;
  }
  return p;
}
void bv_delete(BV *bv) {
  am_free(bv);
}

BVINLINE 
int bv_get(BV *bv, uint64_t i) {
  BVSAFE
  WRD *v = (WRD*)(bv->bits);
  uint64_t w = i>>SHF;
  int b = i & MSK;
  return 1 & (v[w]>>b);
}
BVINLINE 
void bv_set(BV *bv, uint64_t i) {
  BVSAFE
  WRD *v = (WRD*) (bv->bits);
  uint64_t w = i>>SHF;
  int b = i & MSK;
  WRD m = 1;
  m <<= b;
  v[w] |= m;
}
BVINLINE 
void bv_clr(BV *bv, uint64_t i) {
  BVSAFE
  WRD *v = (WRD*) (bv->bits);
  uint64_t w = i>>SHF;
  int b = i & MSK;
  WRD m = 1;
  m <<= b;
  m ^= -1;
  v[w] &= m;
}

int popcount[256] = 
  {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
   3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
  };

uint64_t bv_cnt(BV *bv) {
  uint64_t cnt = 0;
  WRD *v = (WRD*) (bv->bits);
  int i;
  if (BPW==8) {
    for (i=0; i<bv->nw; i++) cnt += popcount[v[i]];
  } else if (BPW==16) {
    for (i=0; i<bv->nw; i++) cnt += (popcount[v[i] & MSK] +  popcount[v[i] >> (BPW/2)]);
  } else if (BPW==64) {
    cnt = -2;
  } else {
    cnt = -BPW;
  }
  return cnt;
}

int bv_bpw() { return BPW; }
