/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/vi.c $                                                 */
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

#include "vi.h"

uint64_t vi_enc64(uint64_t n, uint8_t *buf) {
  uint64_t len = 0;
  int done = 0;
  while (!done) {
    if (n<128) {
      buf[len++] = n & 0x000000000000007F;
      done = 1;
    } else {
      buf[len++] = (n & 0x000000000000007F) | 0x0000000000000080;
      n >>= 7;
    }
  }
  return len;
}


uint64_t vi_dec64(uint8_t *buf, uint64_t *n) {
  uint64_t m = 0;
  int len = 0;
  int done = 0;
  uint64_t tmp = 0;
  while (!done) {
    if (buf[len] & 0x80) {
      tmp = buf[len] & 0x7F;
    } else {
      tmp = buf[len];
      done = 1;
    }
    m |= tmp << (len * 7);
    len++;
  }
  *n = m;
  return len;
}

