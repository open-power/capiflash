/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_iv.c $                                        */
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>


#include "cl.h"
#include "iv.h"

int tst_iv_entry(int argc, char **argv)
{
  char *anon[] = {NULL,NULL,NULL,NULL};
  int iter = 15;
  uint64_t n = 10;
  uint64_t m = 56;
  int test = 1;
  int seed = 1234;
  int def = 1234;
  CL args[] = {{"-n", &n, AR_INT64, "array size"},
	       {"-m", &m, AR_INT64, "integer size"},
	       {"-i", &iter, AR_INT, "iterations"},
	       {"-t", &test, AR_INT, "test to run"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {NULL, NULL, 0, NULL}};

  int echo = 1;
  (void)cl_parse(argc,argv,args,anon,echo);

  srand48(seed);

  IV *iv = iv_new(n, m);
  uint64_t mask = 1;
  mask <<= m;
  mask -= 1;

  uint64_t errs = 0;

  printf("Running test %d\n", test);
  switch (test) {
  case 0: 
    {
      uint64_t *arr = malloc(n * sizeof(uint64_t));
      memset(arr, 0x00, n * sizeof(uint64_t));
      int i;
      for(i=0; i<n; i++) {
	uint64_t val = lrand48();
	val <<= 32;
	val |= lrand48();
	val &= mask;
	arr[i] = val;
	iv_set(iv,i,val);
      }
      for(i=0; i<iter; i++) {
	int op = lrand48() % 2;
	uint64_t ind = lrand48() % n;
	switch (op) {
	case 0: 
	  {
	    uint64_t v0 = arr[ind];
	    uint64_t v1 = iv_get(iv,ind);
	    //printf("Checking %"PRIu64"->%llx %llx\n", ind, v0, v1);
	    if (v0!=v1) errs++;
	    break;
	  }
	case 1:
	  {
	    uint64_t val = lrand48();
	    val <<= 32;
	    val |= lrand48();
	    val &= mask;
	    arr[ind] = val;
	    //	    printf("Setting %"PRIu64"->%"PRIu64"\n", ind, val);
	    iv_set(iv,ind,val);
	    break;
	  }
	}
      }
      break;
    }
  case 1:
    {
      uint64_t i;
      for (i=0; i<n; i++) {
	iv_set(iv,i,i+1);
      }
      for (i=0; i<n; i++) {
	uint64_t v0 = iv_get(iv,i & mask);
	if (v0!=((i+1)&mask)) {
	  printf("Mismatch %"PRIu64": %"PRIu64" != %"PRIu64"\n", i, i & mask, v0);
	  errs++;
	} else {
	  printf("Match %"PRIu64": %"PRIu64" != %"PRIu64"\n", i, i & mask, v0);
	}
      }
      break;
    }
  case 2:
    {
      uint64_t i;
      uint64_t v = def;
      for (i=0; i<n; i++) {
	iv_set(iv,i,v);
      }
      for (i=0; i<n; i++) {
	uint64_t v0 = iv_get(iv,i);
	if (v0!=v) {
	  errs++;
	}
      }
    }
  default:
    {
      printf("Unknown test %d\n", test);
    }
  }
  printf("Error count = %"PRIu64"\n", errs);

  return errs;
}
