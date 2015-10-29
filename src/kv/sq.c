/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/sq.c $                                                 */
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
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "am.h"

#include "sq.h"

#include <arkdb_trace.h>

int  sq_sizeof(int n, int m) {return sizeof(SQ) + n*m;}
int  sq_count(SQP sq)        {return sq->cnt;}
int  sq_emptyp(SQP sq)       {return sq->cnt==0;}
int  sq_fullp(SQP sq)        {return sq->cnt==sq->n;}

SQP sq_init(SQP sq, int n, int m) {
  if (sq==NULL)  sq = am_malloc(sq_sizeof(n,m));

  if (sq==NULL)
  {
      errno = ENOMEM;
      KV_TRC_FFDC(pAT, "FFDC: sq %p n %d, m %d, ENOMEM", sq, n, m);
      goto exception;
  }

  sq->n = n;
  sq->m = m;
  sq->head = 0;
  sq->tail = 0;
  sq->cnt  = 0;
  KV_TRC(pAT, "sq %p n %d m %d", sq, n, m);

exception:
  return sq;
}

// return count of items enqueued (0 or 1)
int sq_enq(SQP sq, void *v) {
  if (sq_fullp(sq))
  {
      KV_TRC_DBG(pAT, "QFULL: sq %p sq->n %d v %p", sq, sq->n, v);
      return 0;
  }
  int h = sq->head;
  memcpy(sq->queue+(h*sq->m), v, sq->m);
  sq->head = (h+1)%sq->n;
  sq->cnt++;
  return 1;
}

// return count of items dequeued (0 or 1)
int sq_deq(SQP sq, void *v) {
  if (sq_emptyp(sq))
  {
      KV_TRC_FFDC(pAT, "FFDC: sq %p sq->n %d v %p", sq, sq->n, v);
      return 0;
  }
  int t = sq->tail;
  memcpy(v,sq->queue+(t*sq->m), sq->m);
  sq->tail = (t+1)%sq->n;
  sq->cnt--;
  return 1;
}

