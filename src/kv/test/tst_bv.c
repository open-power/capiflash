/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_bv.c $                                        */
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
#include <inttypes.h>

#include <cl.h>
#include <bv.h>

int tst_bv_entry(int argc, char **argv)
{
  //int i,j,k,l;

  char *anon[] = {NULL,NULL,NULL,NULL};
  int iter = 10;
  int bcnt = 8;
  int seed = 1234;
  CL args[] = {{"-b", &bcnt, AR_INT, "block count per thread"},
	       {"-n", &iter, AR_INT, "iterations per thread"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {NULL, NULL, 0, NULL}};

  int echo = 1;
  (void)cl_parse(argc,argv,args,anon,echo);

  srand48(seed);

  BV *bv = bv_new(bcnt);

  char *bchk = malloc(bcnt);
  
  int i;
  for(i=0; i<bcnt; i++) bchk[i] = 0;

  for(i=0; i<iter; i++) {
    int b = lrand48() % bcnt;

    if (bchk[b]) {
      bchk[b] = 0;
      bv_clr(bv,b);
    } else {
      bchk[b] = 1;
      bv_set(bv,b);
    }
  }
  int errs = 0;
  for(i=0; i<bcnt; i++) {
    if (bchk[i] != bv_get(bv,i)) errs++;
  }
  printf("Error count = %d, # bits set = %"PRIu64"\n", errs, bv_cnt(bv));

  return errs;
}
