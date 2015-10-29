/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ll.c $                                                 */
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
#include <stdlib.h>
#include <errno.h>
#include "am.h"
#include "ll.h"

LL *ll_cons(void *dat, LL *ll) {
  LL *ret = am_malloc(sizeof(LL));
  if (ret == NULL)
  {
    errno = ENOMEM;
  }
  ret->car = dat;
  ret->cdr = ll;
  return ret;
}

void *ll_car(LL *ll) {return ll->car; }
LL   *ll_cdr(LL *ll) {return ll->cdr; }

int ll_len(LL *ll) {
  int ret = 0;
  while (ll) {
    ret++;
    ll = ll->cdr;
  }
  return ret;
}
