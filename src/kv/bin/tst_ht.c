/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_ht.c $                                        */
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>

#include "cl.h"
#include "ht.h"

int *vld =  NULL;
uint64_t *blk =  NULL;
HT  *ht =  NULL;

int iter = 10;
int hcnt = 10;
int mcnt = 34;
int seed = 1234;
int prt = 0;

int check_ht() {
  int i;
  int evld = 0;
  int eblk = 0;
  int v;
  uint64_t b;
  for(i=0; i<hcnt; i++) {
    v = ht_vldp(ht,i);
    if (vld[i]!=v) evld++;
    if (v) {
      b = ht_get(ht,i);
      if (blk[i]!=b) eblk++;
    }
  }
  if (evld) printf("Valid errors %d\n", evld);
  if (eblk) printf("Block errors %d\n", eblk);
  return evld  + eblk;
}

int tst_ht_entry(int argc, char **argv)
{
  //int i,j,k,l;

    int rc1,rc2;

  char *anon[] = {NULL,NULL,NULL,NULL};
  CL args[] = {{"-n", &iter, AR_INT, "iterations"},
	       {"-h", &hcnt, AR_INT, "hash entries"},
	       {"-m", &mcnt, AR_INT, "value size"},
	       {"-p", &prt,  AR_FLG, "print table"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {NULL, NULL, 0, NULL}};

  int echo = 1;
  (void)cl_parse(argc,argv,args,anon,echo);

  uint64_t mask = 1;
  mask <<= mcnt;
  mask -= 1;

  srand48(seed);

  vld = malloc(hcnt * sizeof(int));
  blk = malloc(hcnt * sizeof(uint64_t));

  bzero(vld,(hcnt * sizeof(int)));
  bzero(blk,(hcnt * sizeof(uint64_t)));

  ht = ht_new(hcnt,mcnt);

  rc1=check_ht();
  if (rc1) printf("Failed initial check\n");


  if (ht->n != hcnt) printf("Fail : initial size bad\n");
  uint64_t i;
  for(i=0; i<iter; i++) {

    uint64_t p = lrand48() % hcnt;
    
    int sw = lrand48() % 6;
    switch (sw) {
    case 0:

      vld[p] = 1;
      blk[p] = lrand48();
      blk[p] <<= 32;
      blk[p] |= lrand48();
      blk[p] &= mask;

      ht_set(ht,p,blk[p]);
      break;
    case 1:
      ht_clr(ht,p);
      vld[p] = 0;
      break;


    }
  }
  rc2=check_ht();
  if (rc2 == 0)
    printf("Final check passed\n");
  else
    printf("Final check failed %d\n", rc2);
    
  if (prt) {
    for(i=0; i<hcnt; i++) {
      if (ht_vldp(ht,i)) {
	printf("+ %"PRIu64": %"PRIu64"\n", i, ht_get(ht,i));
      } else {
	printf("- %"PRIu64":\n", i);
      }
    }
  }
  return (rc1+rc2);
}

  
