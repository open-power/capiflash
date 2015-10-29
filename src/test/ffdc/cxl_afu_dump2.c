/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/cxl_afu_dump2.c $                               */
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "afu.h"
#include "afu_fc.h"
#ifdef SIM
#include "sim_pthread.h"
#else
#include <pthread.h>
#endif


void print_64(FILE *f, int a, uint64_t v) {
    if (v != 0xFFFFFFFFFFFFFFFF)
    fprintf(f,"%x : %lx\n",a,v);
}

void dump_range(FILE *f, struct afu *afu, int lwr, int upr) {
  uint64_t r;
  fprintf(f,"# mmio address range 0x%07x to 0x%07x\n",lwr, upr);
  while (lwr < upr) {
    // skip heartbeat reg otherwise mserv reports a mismatch
    if( lwr != 0x2010020 ) { 
      afu_mmio_read_dw(afu,(lwr >> 2),(__u64 *)&r);
      print_64(f,lwr,r);
    }
    lwr+=8;
  }
}

void dump_count(FILE *f, struct afu *afu, int base, int n) {
  dump_range(f,afu,base,base+(n*8));
}

void dump_ctxt(FILE *f, struct afu* afu, int ctxt) {
  int i;
  uint64_t r;
  dump_count(f,afu,0x10000*ctxt,16);
}

void dump_cpc(FILE *f, struct afu* afu, int ctxt) {
  dump_count(f,afu,0x2000000+(16*8*ctxt),16);
}
void dump_gbl(FILE *f, struct afu *afu) {
  dump_range(f,afu,0x2010000,0x2012000);
}
void dump_afu_dbg(FILE *f, struct afu *afu){
  dump_range(f,afu,0x2040000,0x2060000);
}
void dump_fc_dbg(FILE *f, struct afu* afu, int fcp) {
  dump_range(f,afu,0x2060000+(fcp*0x20000),0x2060000+(fcp*0x20000)+0x20000);
}

void dump_perf (FILE *f, struct afu * afu) {
  dump_range(f,afu,0x2018000,0x2020000);
}

void print_version(FILE *f, struct afu *afu) 
{
  uint64_t r;
  afu_mmio_read_dw(afu,0x804200,&r);
  if (r == -1l) {
    fprintf(f, "# Version number not implemented\n");
  }
  else {
    char version[9];
    int i;
    for (i=0; i<8; i++) {
      version[7-i] = (r & 0xFFl);
      r = r >> 8;
    }
    version[8] = 0;
    fprintf(f,"# AFU Version = %s\n",version);
  }
}


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: %s <path-to-afu>\n",argv[0]);
    exit(2);
  }
  struct afu* afu = afu_map(argv[1]);
  FILE *f = stdout;
  print_version(f,afu);
  int i;
  for(i=0; i<512; i++) dump_ctxt(f,afu,i);
  for(i=0; i<512; i++) dump_cpc(f,afu,i);
  dump_gbl(f,afu);
  dump_perf(f,afu);
  dump_afu_dbg(f,afu);
  dump_fc_dbg(f,afu,0);
  dump_fc_dbg(f,afu,1);
}

