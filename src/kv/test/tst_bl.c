/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_bl.c $                                        */
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
#include <bl.h>
#include <cl.h>

int tst_bl(int iter, int bcnt, int ccnt, int cmax,int grow,int hold ,int resz,int seed,int prnt,int dprt,int w)
{
  int i;

  int *blks = (int*)malloc(bcnt * sizeof(int));
  for (i=0; i<bcnt; i++) blks[i] = 0;
  
  int64_t *chains = (int64_t*)malloc(ccnt * sizeof(int64_t));
  for (i=0; i<ccnt; i++) chains[i] = -1;
  
  BL *bl = bl_new(bcnt,w);
  
  int fails = 0;
  int holds = 0;

  int ntakes = 0;
  int nfails = 0;
  int ndrops = 0;

  if (hold) bl_hold(bl);

  for (i = 0; i<iter; i++) {

    //if (prnt && i%prnt==0) printf("%d:\n",i);

    if (dprt && iter%dprt==0) bl_dot(bl,i,blks,ccnt, chains);

    int ch = lrand48() % ccnt;

    if (chains[ch]==-1) {
      //if (prnt && i%prnt==0) printf("%d:Take\n",i);
     unsigned int n = 1 + lrand48()%cmax;
      int b = bl_take(bl,n);
      if (prnt && i%prnt==0) printf("%d:Take %d = @%d (%"PRIi64", %"PRIi64")\n", i, n, b, bl->count, bl_len(bl,bl->head));
      if (0<=b) {
    ntakes++;
    int64_t j;
    for (j = b; j>0; j = bl_next(bl,j)) {
      blks[j]++;
    }
    chains[ch] = b;
      } else {
    nfails++;
    fails++;
    if (resz && fails >= resz) {
      printf("Resizing: %"PRIi64" + %d\n", bl->n, grow);
      BL *rbl = bl_resize(bl, bl->n+grow, bl->w);
      if (!rbl) {
        printf("Resize failed, exiting\n");
        return 1;
      } else {
        bl = rbl;
        fails = 0;
        blks = (int*)realloc(blks, (bcnt + grow)*sizeof(int));
        if (!blks) {
          printf("Realloc for blks failed\n");
          return 1;
        }
        int j;
        for(j=0; j<grow; j++) blks[bcnt+j] = 0;
        bcnt += grow;
      }
      printf("New size (%"PRIi64", %"PRIi64")\n",  bl->count, bl_len(bl,bl->head));
    }
      }
    } else {
      //if (prnt && i%prnt==0) printf("%d:Drop\n",i);
      ndrops++;
      int64_t b = chains[ch];
      int64_t n = bl_len(bl,b);
      int64_t j;
      for (j = chains[ch]; j>0; j = bl_next(bl,j)) {
        blks[j]--;
      }
      bl_drop(bl,b);
      if (prnt && i%prnt==0)
    printf("%d:                                                          Drop %"PRIi64" = @%"PRIi64" (%"PRIi64",%"PRIi64")\n",
               i, n, b, bl->count, bl_len(bl,bl->head));
      chains[ch] = -1;
      if (hold && holds == hold) {
    bl_release(bl);
    bl_hold(bl);
    holds = 0;
      } else {
    holds++;
      }
    }
  }
  if (hold) bl_release(bl);

  if (dprt) bl_dot(bl,iter,blks,ccnt, chains);

  printf("Done: take %d, drops %d, fails %d\n", ntakes, ndrops, nfails);


  int *infree = (int*)malloc(bcnt * sizeof(int));
  int *inuse  = (int*)malloc(bcnt * sizeof(int));
  int *inhold = (int*)malloc(bcnt * sizeof(int));
  for(i=0; i<bcnt; i++) infree[i] = inuse[i] = inhold[i] = 0;

  int64_t b = bl->head;
  while (0<b) {
    infree[b]++;
    b = bl_next(bl,b);
  }

  b = bl->hold;
  while (0<b) {
    inhold[b]++;
    b = bl_next(bl,b);
  }
  

  for(i=0; i<ccnt; i++) {
    b = chains[i];
    while (0<b) {
      inuse[b]++;
      b = bl_next(bl,b);
    }
  }

  int err0 = 0;
  int err1 = 0;
  int err2 = 0;
  int err3 = 0;
  int err4 = 0;
  for (i=1; i<bcnt; i++) {
    if (infree[i]+inuse[i]+inhold[i] != 1) err0++;
    if (infree[i]+inuse[i]+inhold[i] == 0) err1++;
    if (infree[i]+inuse[i]+inhold[i] >  1) err2++;
    if (blks[i]==0 && infree[i]==0 && inhold[i]==0) err3++;
    if (blks[i]==1 && inuse[i]==0) err4++;
  }
  if (err0)
    printf("Error 0 = %d\n", err0);
  else
    printf("Pass: all present\n");

  if (err1)  printf("Error 1 = %d\n", err1);
  if (err2)  printf("Error 2 = %d\n", err2);
  
  if (err3) 
    printf("Error 3 = %d\n", err3);
  else 
    printf("Pass: free blocks in the correct place\n");

  if (err4) 
    printf("Error 4 = %d\n", err4);
  else 
    printf("Pass: in use blocks in the correct place\n");


  /* int errs; */
  /* for(i=0; i<bcnt; i++) if (blks[i]!=0 && blks[i]!=1) errs++; */
  
  /* if (errs!=0)  */
  /*   printf("Test 1 failed, Error some blks (%d) or neither zero or one\n", errs); */
  /* else */
  /*   printf("Test 1 passed\n"); */

  /* if (errs) exit(0); */

  /* int zeros = 0; */
  /* int ones  = 0; */
  /* for(i=0; i<bcnt; i++)  */
  /*   if (blks[i]==0 && blks[i]!=1) zeros++; */
  /*   else if (blks[i]==1) ones++; */
  /*   else printf("Error Block %d = %d\n", i, blks[i]); */

  /* if (ones + zeros == bcnt)  */
  /*   printf("Test 2 passed\n"); */
  /* else */
  /*   printf("Test 2 failed, %d + %d != %d\n", ones, zeros, bcnt); */


  /* int fcnt = bl_len(bl,bl->head); */
  /* int ncnt = 0; */
  /* for(i=0; i<ccnt; i++) */
  /*   if (chains[i] >= 0) */
  /*     ncnt += bl_len(bl,chains[i]); */
  /* if (fcnt != zeros || ncnt != ones) */
  /*   printf("Test 3 failed, %d != %d | %d != %d\n", fcnt, zeros, ncnt, ones); */
  /* else */
  /*   printf("Test 3 passed\n"); */

  /* errs = 0; */
  /* for (i=0; i<bcnt; i++)  */
  /*   if (blks[i]==0)  */
  /*     if (bl_cnt(bl, bl->head, i)!=1) */
  /*    errs++; */
  /* if (errs) */
  /*   printf("Test 4 failed, errors = %d\n", errs); */
  /* else */
  /*   printf("Test 4 passed\n"); */



  /* for(i=0; i<bcnt; i++) blks[i]=0; */
  /* int64_t b = bl->head; */
  /* while (0<=b) { */
  /*   blks[b]++; */
  /*   b=bl_next(bl,b); */
  /* } */
  /* for(i=0; i<ccnt; i++) { */
  /*   b = chains[i]; */
  /*   while (0<=b) { */
  /*     blks[b]++; */
  /*     b=bl_next(bl,b); */
  /*   } */
  /* } */
  /* errs = 0; */
  /* for (i=0; i<bcnt; i++) { */
  /*   if (blks[i] != 1) { */
  /*     printf("Block %d occurs %d times\n", i, blks[i]); */
  /*     errs++; */
  /*   } */
  /* } */
  /* if (errs) */
  /*   printf("Test 5 failed, errs = %d\n", errs); */
  /* else */
  /*   printf("Test 5 passed\n"); */

    


  /* errs = 0; */
  /* int j; */
  /* for (i=0; i<bcnt; i++)  */
  /*   if (blks[i]==1) { */
  /*     int cnt = 0; */
  /*     for(j=0; j<ccnt; j++) */
  /*    if (chains[j]>=0) { */
  /*      //printf("bl_cnt(bl,chains[%d],%d) -> %"PRIi64"\n",j,i,  bl_cnt(bl,chains[j],i)); */
  /*      cnt += bl_cnt(bl,chains[j],i); */
  /*    } */
  /*     if (cnt!=1) errs++; */
  /*   } */
  /* if (errs) */
  /*   printf("Test 5 failed, errs = %d\n", errs); */
  /* else */
  /*   printf("Test 5 passed\n"); */

  
  bl_delete(bl);
  free(blks);
  free(chains);

  return (err0+err1+err2+err3+err4);
}

int tst_bl_entry(int argc, char **argv)
{
    char *anon[] = {NULL,NULL,NULL,NULL};
    int iter = 10;
    int bcnt = 8;
    int ccnt = 16;
    int cmax = 6;
    int grow = 8;
    int hold = 0;
    int resz = 0;
    int seed = 1234;
    int prnt = 0;
    int dprt = 0;
    int w = 34;
    CL args[] = {{"-d", &dprt, AR_INT, "dot print"},
             {"-g", &grow, AR_INT, "amount to grow by"},
             {"-p", &prnt, AR_INT, "print (alot)"},
             {"-b", &bcnt, AR_INT, "block count per thread"},
             {"-c", &ccnt, AR_INT, "chain count"},
             {"-l", &cmax, AR_INT, "max chain length"},
             {"-s", &seed, AR_INT,  "random seed"},
             {"-r", &resz, AR_INT, "resize after n take fails"},
             {"-n", &iter, AR_INT,  "iterations per thread"},
             {"-w", &w,    AR_INT, "lba width"},
             {"-j", &hold, AR_INT, "only release after hold # of drops"},
             {NULL, NULL, 0, NULL}};

    int echo = 1;

  (void)cl_parse(argc,argv,args,anon,echo);

  //function call to the tst_bl function and return code of 0 = success and else for fail
   return tst_bl(iter,bcnt,ccnt,cmax,grow,hold,resz,seed,prnt,dprt,w);
}
