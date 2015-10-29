/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/tg.c $                                                 */
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
#include "am.h"

#include "tg.h"

int tg_sizeof(int n) {
  return sizeof(TG) + n*sizeof(int) + n*sizeof(char);
}

TGP tg_init(TGP tg, int n) {
  if (tg==NULL) {
    tg = am_malloc(tg_sizeof(n));
  }
  tg->size = n;
  tg->left = n;
  (void)pthread_mutex_init(&(tg->tg_lock), NULL);
  tg->avail = ((char *)&(tg->stack[0])) + n * sizeof(int);
  int i;
  for(i=0; i<n; i++) {
    tg->avail[i] = 1;
    tg->stack[i] = (n-1) - i;
  }
  return tg;
}

int tg_left(TGP tg) {
  if(tg!=NULL)
    return tg->left;
  else
    return -1;
}

int tg_get(TGP tg) {
  int tag;
  pthread_mutex_lock(&(tg->tg_lock));
  if (tg->left > 0) {
    tag = tg->stack[tg->left-1];
    tg->avail[tag] = 0;
    tg->left--;
  } else {
    tag = -1;
  }
  pthread_mutex_unlock(&(tg->tg_lock));
  return tag;
}

void tg_return(TGP tg, int tag) {
  pthread_mutex_lock(&(tg->tg_lock));
  if (tag<0 || tag>=tg->size) {
    fprintf(stderr,"Bad tag return %d\n", tag);
    pthread_mutex_unlock(&(tg->tg_lock));
    exit(1);
  }
  if (tg->avail[tag]==0) {
    tg->stack[tg->left] = tag;
    tg->avail[tag]= 1;
    tg->left++;
  }
  pthread_mutex_unlock(&(tg->tg_lock));
}
