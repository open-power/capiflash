/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test_perf.c $                                          */
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
#include <malloc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <arkdb.h>

#define MAX_KEYS        100000
#define MAX_STRING_LEN  16

typedef struct t_data 
{
  int tid;
  int start;
  int end;
} t_data_t;

int run = 0;
int numthrs = 4;
char **keys = NULL;
ARK *arkp = NULL;

void set_cb(int errcode, uint64_t dt, int64_t res)
{
  if (errcode != 0)
  {
    printf("Set failed for key %"PRIi64"\n", dt);
  }

  return;
}

void *thread_run(void *arg)
{
  int rc = 0;
  int i  = 0;
  int tid = 0;
  uint64_t retry_cnt = 0;
  //int nasyncs = MAX_KEYS / numthrs;
  t_data_t *data = (t_data_t *)arg;
  // ARC *arcp = NULL;

  tid = data->tid;

#if 0
  rc = ark_connect_verbose(&arcp, arkp, nasyncs);
  if (rc != 0)
  {
    printf("Thread %d: failed ark_connect (%d)\n", tid, rc);
    return NULL;
  }
#endif

  while ( run == 0 );
  
  for (i = data->start; i <= data->end; i++)
  {
    do
    {
      rc = ark_set_async_cb(arkp, strlen(keys[i]), (void*)keys[i], 
                            strlen(keys[i]), (void*)keys[i], 
                            &set_cb, i);
      if ( rc == EAGAIN )
      {
        retry_cnt++;
      }
    } while (rc == EAGAIN);

    if (rc != 0)
    {
      printf("Thread %d: Failed to add key %d (%d)\n", tid, i, rc);
      break;
    }
  }

  printf("Thread %d is done: %ld\n", tid, retry_cnt);

  return NULL;
}

int main(int argc, char **argv) {

  int i = 0;
  int rc = 0;
  int thrs = 0;
  int64_t key_cnt = 0;
  uint64_t ops = 0;
  uint64_t ios = 0;
  uint64_t act = 0;
  int64_t total_us = 0;
  int64_t total_sec = 0;
  pthread_t *threads = NULL;
  t_data_t  *datap = NULL;
  struct timeval stop, start;

  keys = (char **)malloc(MAX_KEYS * sizeof(char *));
  if (keys == NULL)
  {
    printf("malloc failed for keys\n");
    return ENOMEM;
  }

  for (i = 0; i < MAX_KEYS; i++)
  {
    keys[i] = (char *)malloc(MAX_STRING_LEN);
    snprintf(keys[i], MAX_STRING_LEN, "%d-hello", i);
  }

  rc = ark_create("/dev/cxl/afu0.0s", &arkp, ARK_KV_VIRTUAL_LUN);
  if (rc != 0)
  {
    printf("Error opening ark (%d)\n", rc);
    return rc;
  }

  threads = (pthread_t *)malloc(numthrs * sizeof(pthread_t));
  datap = (t_data_t *)malloc(numthrs * sizeof(t_data_t));

  for (thrs = 0; thrs < numthrs; thrs++)
  {
    datap[thrs].tid = thrs;
    datap[thrs].start = thrs * (MAX_KEYS / numthrs);
    datap[thrs].end = ((thrs + 1) * (MAX_KEYS / numthrs)) - 1;
    if (datap[thrs].end >= MAX_KEYS)
    {
      datap[thrs].end = MAX_KEYS - 1;
    }

    printf("App Thread %d: start %d, end %d\n", thrs, datap[thrs].start, datap[thrs].end);

    rc = pthread_create(&(threads[thrs]), NULL, thread_run, &datap[thrs]);
    if (rc != 0)
    {
      printf("Failed to create pthread %d (%d)\n", i, rc);
      return rc;
    }
  }

  gettimeofday(&start, NULL);

  run = 1;

  do
  {
    key_cnt = 0;
    rc = ark_count(arkp, &key_cnt);
    if (rc != 0)
    {
      printf("ark_count failed %d\n", rc);
    }
  } while (key_cnt < MAX_KEYS);

  gettimeofday(&stop, NULL);

  rc = ark_stats(arkp, &ops, &ios);
  if (rc != 0)
  {
    printf("ark_stats failed %d\n", rc);
    return rc;
  }

  rc = ark_actual(arkp, &act);
  if (rc != 0)
  {
    printf("ark_actual failed %d\n", rc);
    return rc;
  }

  for (thrs = 0; thrs < numthrs; thrs++)
  {
    (void)pthread_join(threads[thrs], NULL);
  }

  printf("Added all keys\n");

  free(threads);
  free(datap);

  rc = ark_delete(arkp);
  if (rc != 0)
  {
    printf("Error deleting ark (%d)\n", rc);
    return rc;
  }

  total_us = ((stop.tv_sec * 1000000) + stop.tv_usec) - 
             ((start.tv_sec * 1000000) + start.tv_usec);
  total_sec = total_us / 1000000;

  printf("K/V ops: %"PRIu64", ios: %"PRIu64"\n", ops, ios);
  printf("usecs %"PRIi64", secs: %"PRIi64"\n", total_us, total_sec);
  printf("K/V ops/usec: %6.0f, ios/usec: %.0f\n", (float)ops/(float)total_us, 
                      (float)ios/(float)total_us);
  printf("K/V ops/sec: %6.0f, ios/sec: %.0f\n", (float)ops/(float)total_sec, 
                      (float)ios/(float)total_sec);

  return 0;
}
