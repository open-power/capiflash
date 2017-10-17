/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/queue.c $                                              */
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

#include <inttypes.h>
#include <am.h>
#include <queue.h>
#include <errno.h>

queue_t *queue_new(uint32_t n) {
  queue_t *q = am_malloc(sizeof(queue_t) + n * sizeof(int32_t));
  q->n = n;
  q->c = 0;
  q->h = 0;
  q->t = 0;
  q->waiters = 0;
  pthread_mutex_init(&q->m,NULL);
  pthread_cond_init(&q->cond,NULL);
  return q;
}

void queue_free(queue_t *q)
{
  am_free(q);
  return;
}

int queue_enq(queue_t *q, int32_t v) {
  int rc = ENOSPC;
  pthread_mutex_lock(&q->m);
  if (q->c < q->n) {
    int h = q->h;
    q->q[h++] = v;
    h %= q->n;
    q->c++;
    q->h = h;
    rc = 0;
    if (q->waiters > 0)
    {
      queue_wakeup(q);
    }
  }
  pthread_mutex_unlock(&q->m);
  return rc;
}
int queue_enq_unsafe(queue_t *q, int32_t v) {
  int rc = ENOSPC;
  if (q->c < q->n) {
    int h = q->h;
    q->q[h++] = v;
    h %= q->n;
    q->c++;
    q->h = h;
    rc = 0;
    if (q->waiters > 0)
    {
      queue_wakeup(q);
    }
  }
  return rc;
}
int queue_deq(queue_t *q, int32_t *v) {
  int rc = EAGAIN;
  pthread_mutex_lock(&q->m);
  if (q->c > 0) {
    int t = q->t;
    *v = q->q[t++];
    t %= q->n;
    q->c--;
    q->t = t;
    rc = 0;
  }
  pthread_mutex_unlock(&q->m);
  return rc;
}

int queue_deq_unsafe(queue_t *q, int32_t *v) {
  int rc = EAGAIN;
  if (q->c > 0) {
    int t = q->t;
    *v = q->q[t++];
    t %= q->n;
    q->c--;
    q->t = t;
    rc = 0;
  }
  return rc;
}

void queue_lock(queue_t *q)
{
  pthread_mutex_lock(&(q->m));
  return;
}

void queue_unlock(queue_t *q)
{
  pthread_mutex_unlock(&(q->m));
  return;
}

void queue_wakeup(queue_t *q)
{
  pthread_cond_broadcast(&(q->cond));
  return;
}

void queue_wait(queue_t *q)
{
  q->waiters++;

  pthread_cond_wait(&(q->cond), &(q->m));

  q->waiters--;

  return; 
}

