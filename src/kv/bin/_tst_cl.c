/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/_tst_cl.c $                                       */
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

#include "cl.h"
#include "ut.h"
#include "ll.h"



int main(int argc, char **argv) {
  
  char *anon[] = {"foo","bar","doo","dah",NULL,NULL,NULL,NULL};
  int tcnt = 1;
  int bcnt = 128;
  int bsize = 4096;
  int excl = 0;
  int echo = 1;
  int seed = 1234;
  int iter = 1000;
  CL args[] = {{"-t", &tcnt, AR_INT, "thread count"},
	       {"-bc", &bcnt, AR_INT, "block count per thread"},
	       {"-bs", &bsize, AR_INT, "block size"},
	       {"-e", &excl, AR_FLG, "exclusive regions"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {"-n", &iter, AR_INT, "iterations per thread"},
	       {NULL, NULL, 0, NULL}};
  int acnt = cl_parse(argc,argv,args,anon,echo);


  exit(6 == acnt);
}

