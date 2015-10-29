/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ut.c $                                                 */
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
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#ifndef _AIX
#include <getopt.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#include "am.h"
#include "ut.h"

uint64_t divup(uint64_t n, uint64_t m) {
  return n/m + (n%m==0 ? 0 : 1);
}

uint64_t divceil(uint64_t n, uint64_t m) {
  return n/m + (n%m==0 ? 0 : 1);
}  

char *rndalpha(int n, int m) {
  int i;
  char *ret = am_malloc(n + 1);
  if (ret) {
    for (i=0; i<n; i++) {
      ret[i] = 'a' + lrand48()%m;
    }
    ret[n] = 0;
  }
  return ret;
}

void expandif(void **buf, uint64_t *len, uint64_t req) {
  if (req < *len) { 
    void *newp = am_realloc(*buf, req);
    if (newp) {
      *buf = newp;
      *len = req;
    }
  }
}


double time_diff(struct timeval x , struct timeval y)
{
    double x_ms , y_ms , diff;
     
    x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
     
    diff = (double)y_ms - (double)x_ms;
     
    return diff;
}

// return nanoseconds per tick
double time_per_tick(int n, int del) {
  int i;

  double *td = am_malloc(n * sizeof(double));
  double *tv = am_malloc(n * sizeof(double));

  struct timeval tvs;
  struct timeval tve;

  ticks  ts;
  ticks te;

  for (i=0; i<n; i++) {

    gettimeofday(&tvs, NULL);
    ts = getticks();

    usleep(del);

    te = getticks();
    gettimeofday(&tve, NULL);

    td[i] = elapsed(te,ts);
    tv[i] = time_diff(tvs,tve);
  }

  double sum = 0.0;

  for(i=0; i<n; i++) {

    sum +=  1000.0 * tv[i] / td[i];

    //printf("ticks, %15g, time, %15g, ticks/usec, %15g, nsec/tick, %15g\n", td[i], tv[i], td[i] / tv[i], 1000.0 * tv[i] / td[i]);
    
  }
  return sum / n ;

}  
