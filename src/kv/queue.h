/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/queue.h $                                              */
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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdlib.h>
#include <pthread.h>

typedef struct _queue {
  uint32_t n;
  uint32_t c;
  uint32_t h;
  uint32_t t;
  uint32_t waiters;
  pthread_mutex_t m;
  pthread_cond_t cond;
  int32_t q[];
} queue_t;


queue_t *queue_new(uint32_t n);
void queue_free(queue_t *q);

int queue_enq(queue_t *q, int32_t v);
int queue_deq(queue_t *q, int32_t *v);

int queue_enq_unsafe(queue_t *q, int32_t v);
int queue_deq_unsafe(queue_t *q, int32_t *v);

void queue_lock(queue_t *q);
void queue_unlock(queue_t *q);

void queue_wait(queue_t *q);
void queue_wakeup(queue_t *q);

#define Q_EMPTY(q) ((q)->c==0)
#define Q_FULL(q) ((q)->c==(q)->n)
#define Q_CNT(q) ((q)->c)

#endif
