/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/cl.c $                                                 */
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
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "cl.h"

char *ardes[] = {"<int>","<float>","<string>","","<int32>","<int64>", "<double>"};

int cl_parse(int argc, char **argv, CL *ar, char **anon, int echo) {
  int i, j;
  int acnt = 0;
  int arn = 0;
  int amax = 0;
  while (ar[arn].str!=NULL) arn++;
  while (anon[amax]!=NULL) amax++; 
  //printf("arn = %d, amax = %d\n", arn, amax);
  for (i=1; i<argc; i++) {
    int found = 0;
    for(j=0; j<arn; j++) {
      if (strcmp(argv[i],"--help")==0) goto help_exit;
      if (strcmp(argv[i], ar[j].str)==0) { 
        //printf(">>>> %s\n", argv[i]);
        found = 1;
        switch (ar[j].typ) {
        case AR_INT:
        case AR_INT32: {
          if (++i==argc) goto help_exit;
          *((int*)ar[j].val) = atoi(argv[i]);
          break;
        }
        case AR_INT64: {
          if (++i==argc) goto help_exit;
          *((int64_t*)ar[j].val) = atoll(argv[i]);
          break;
        }
        case AR_FLT: {
          if (++i==argc) goto help_exit;
          *((float*)ar[j].val) = atof(argv[i]);
          break;
        }
        case AR_DBL: {
          if (++i==argc) goto help_exit;
          *((double*)ar[j].val) = strtod(argv[i],0);
          break;
        }
        case AR_STR: {
          if (++i==argc) goto help_exit;
          *((char**)ar[j].val) = argv[i];
          break;
        }
        case AR_FLG: {
          *((int*)ar[j].val) -= 1;
          break;
        }
        default : goto help_exit;
        }
      }
    }
    if (!found) { 
      if (argv[i][0]=='-')
        goto help_exit;
      else {
        if (acnt < amax) {
          //printf("###### %s\n", argv[i]);
          found = 1;
          anon[acnt++] = argv[i];
        } else {
          goto help_exit;
        }
      } // end anonymous
    }
  }
  if (echo) {
    printf(" Running: %s with arguments\n", argv[0]); 
    j = 0;
    for(j=0; j<arn; j++) {
      switch (ar[j].typ) {
      case AR_INT:
      case AR_INT32: {
        printf("    %s %d : %s\n", ar[j].str, *(int32_t*)ar[j].val, ar[j].des);
        break;
      }
      case AR_INT64: {
        printf("    %s %"PRIi64" : %s\n", ar[j].str, *(int64_t*)ar[j].val, ar[j].des);
        break;
      }
      case AR_FLT: {
        printf("    %s %g : %s\n", ar[j].str, *(float*)ar[j].val, ar[j].des);
        break;
      }
      case AR_DBL: {
        printf("    %s %g : %s\n", ar[j].str, *(double*)ar[j].val, ar[j].des);
        break;
      }
      case AR_STR: {
        printf("    %s %s : %s\n", ar[j].str, *(char**)ar[j].val, ar[j].des);
        break;
      }
      case AR_FLG: {
        printf("    %s <%s> : %s\n", ar[j].str, (*(int*)ar[j].val) ? "true" : "false" , ar[j].des);
        break;
      }
      }
    }
    if (acnt==0) acnt = amax;
    if (acnt) printf(" Anonymous arguments:\n");
    for(j=0; j<acnt; j++) {
      printf("   %s\n", anon[j]);
    }
  }
  return acnt;
 help_exit:
  fprintf(stderr,"usage: %s <args>\n", argv[0]);
  for(j=0; j<arn; j++) { 
    fprintf(stderr, "   %s %s : %s\n", ar[j].str, ardes[ar[j].typ], ar[j].des);
  }
  fprintf(stderr, " <anon0> ... <anon%d>: max anonymous args cnt = %d\n", amax-1, amax);
  exit(1);
}

int csv_parse(char *buf, char **val, int *len, int n) {
  int m = 0;
  int i = 0;
  int cnt = 0;
  int instring = 0;
  while (buf[i]!=0) {
    int mark = buf[i]=='"';
    if (instring) {
      if (mark) {
        buf[i] = 0;
        len[m] = cnt;
        m++;
        instring = 0;
      } else {
        cnt++;
      }
    } else {
      if (mark) {
        val[m] = buf+i+1;
        cnt=0;
        instring = 1;
      }/*  else { */
      /*         1; */
      /* } */
    }
    i++;
  }
  return m;
}




