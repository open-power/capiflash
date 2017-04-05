/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_vi.c $                                        */
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
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>

#include "cl.h"
#include "ut.h"
#include "ll.h"

#include "vi.h"


int tst_vi_entry(int argc, char **argv) {
  
  char *anon[] = {"foo","bar","doo","dah",NULL,NULL,NULL,NULL};
  int echo = 1;
  int seed = 1234;
  int iter = 1000;
  int prt = 0;
  CL args[] = {{"-p", &prt, AR_FLG, "print flag"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {"-n", &iter, AR_INT, "iterations"},
	       {NULL, NULL, 0, NULL}};
  (void)cl_parse(argc,argv,args,anon,echo);


  uint64_t *arr = malloc(iter * sizeof(uint64_t));
  uint64_t *res = malloc(iter * sizeof(uint64_t));

  srand48(seed);

  uint64_t h,l;
  int i;
  for (i=0; i<iter; i++) {
    h = lrand48();
    l = lrand48();
    arr[i] = (h<<32) | l;
    switch ((lrand48() % 9)) {
    case 0: arr[i] = 0; break;
    case 1: arr[i] &= 0xFF; break;
    case 2: arr[i] &= 0xFFFF; break;
    case 3: arr[i] &= 0xFFFFFF; break;
    case 4: arr[i] &= 0xFFFFFFFF; break;
    case 5: arr[i] &= 0xFFFFFFFFFFLL; break;
    case 6: arr[i] &= 0xFFFFFFFFFFFFLL; break;
    case 7: arr[i] &= 0xFFFFFFFFFFFFFFLL; break;
    case 8: arr[i] &= 0xFFFFFFFFFFFFFFFFLL; break;
    }
    if (prt) printf("arr[%d] = %"PRIu64"\n", i, arr[i]);
  }

  unsigned char *buf = malloc(iter * 9);
  int pos = 0;

  for(i=0; i<iter; i++) {
    pos += vi_enc64(arr[i],buf + pos);
  }
  printf("%"PRIu64"bytes encoded in %"PRIu32" bytes\n", (uint64_t)iter*sizeof(uint64_t), pos);

  int err = 0;
  pos = 0;
  for(i=0; i<iter; i++) {
    pos += vi_dec64(buf+pos, res+i);
    if (prt) printf("arr[%d] = %"PRIu64"     res[%d] = %"PRIu64"\n", i, arr[i], i, res[i]);
    if(arr[i] != res[i]) err++;
  }
  printf("error count = %d\n", err);

  return err;
}


