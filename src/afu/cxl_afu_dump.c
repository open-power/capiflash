/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/cxl_afu_dump.c $                                */
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
#include <string.h>
#include <libcxl.h>
#include "afu_fc.h"
#ifdef SIM
#include "sim_pthread.h"
#else
#include <pthread.h>
#endif

#define DEBUG 0
#define debug(fmt, ...) do { if (DEBUG) fprintf(stderr,fmt,__VA_ARGS__); fflush(stderr);} while (0)
#define debug0(fmt)     do { if (DEBUG) fprintf(stderr,fmt); fflush(stderr);} while (0)

uint64_t portmask=0;

void print_64(FILE *f, int a, uint64_t v) {
    if (v != 0xFFFFFFFFFFFFFFFF)
    fprintf(f,"%x : %lx\n",a,v);
}

void dump_range(FILE *f, struct cxl_afu_h *afu, int lwr, int upr) {
  uint64_t r;
  __u64 addr;

  fprintf(f,"# mmio address range 0x%07x to 0x%07x\n",lwr, upr);
  while (lwr < upr) {
    // skip heartbeat reg otherwise mserv reports a mismatch
    if( lwr != 0x2010020 ) { 
      addr = 4 * (__u64) ((lwr >> 2) & ~0x1);     // Force 8byte align
      cxl_mmio_read64(afu,addr,(__u64 *)&r);
      print_64(f,lwr,r);
    }
    lwr+=8;
  }
}

void dump_count(FILE *f, struct cxl_afu_h *afu, int base, int n) {
  dump_range(f,afu,base,base+(n*8));
}

void dump_ctxt(FILE *f, struct cxl_afu_h* afu, int ctxt) {
  dump_count(f,afu,0x10000*ctxt,16);
}

void dump_cpc(FILE *f, struct cxl_afu_h* afu, int ctxt) {
  dump_count(f,afu,0x2000000+(16*8*ctxt),16);
}
void dump_gbl(FILE *f, struct cxl_afu_h *afu) {
  dump_range(f,afu,0x2010000,0x2012000);
}
void dump_afu_dbg(FILE *f, struct cxl_afu_h *afu){
  dump_range(f,afu,0x2040000,0x2060000);
}
void dump_fc_dbg(FILE *f, struct cxl_afu_h* afu, int fcp) {
  dump_range(f,afu,0x2060000+(fcp*0x20000),0x2060000+(fcp*0x20000)+0x20000);
}

void dump_perf (FILE *f, struct cxl_afu_h * afu) {
  dump_range(f,afu,0x2018000,0x2020000);
}

void get_str(char *out, __u64 in)
{
  int i;
  for (i=0; i<8; i++) {
    out[i] = (in & 0xFFl);
    in = in >> 8;
  }
  out[8] = 0;
}

void print_version(FILE *f, struct cxl_afu_h *afu) 
{
  uint64_t r;
  __u64 addr;
  char version[9];

  addr = 4 * (__u64) (0x804200 & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r == -1l) {
    fprintf(f, "# AFU Version number not implemented\n");
  }
  else {
    int i;
    for (i=0; i<8; i++) {
      version[7-i] = (r & 0xFFl);
      r = r >> 8;
    }
    version[8] = 0;
    fprintf(f,"AFU Version     = %s\n",version);
  }

  fprintf(f,"\n");
  addr = 4 * (__u64) ((0x2060708>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l) {
    char version[9];
    get_str(version, r);
    fprintf(f,"  NVMe0 Version = %s\n",version);
  }
  addr = 4 * (__u64) ((0x2060710>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe0 NEXT    = %s\n",version);
  }
  addr = 4 * (__u64) ((0x2060700>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l) {fprintf(f,"  NVMe0 STATUS  = 0x%lx\n", r);}

  fprintf(f,"\n");
  addr = 4 * (__u64) ((0x2013708>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe1 Version = %s\n",version);
  }
  addr = 4 * (__u64) ((0x2080710>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe1 NEXT    = %s\n",version);
  }
  addr = 4 * (__u64) ((0x2080700>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l) {fprintf(f,"  NVMe1 STATUS  = 0x%lx\n", r);}

  if (portmask != 0xf) {goto done;}

  fprintf(f,"\n");
  addr = 4 * (__u64) ((0x20A0708>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe2 Version = %s\n",version);
  }
  addr = 4 * (__u64) ((0x20A0710>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe2 NEXT    = %s\n",version);
  }
  addr = 4 * (__u64) ((0x20A0700>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l) {fprintf(f,"  NVMe2 STATUS  = 0x%lx\n", r);}

  fprintf(f,"\n");
  addr = 4 * (__u64) ((0x20C0708>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe3 Version = %s\n",version);
  }
  addr = 4 * (__u64) ((0x20C0710>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l)  {
    get_str(version, r);
    fprintf(f,"  NVMe3 NEXT    = %s\n",version);
  }
  addr = 4 * (__u64) ((0x20C0700>>2) & ~0x1);     // Force 8byte align
  cxl_mmio_read64(afu,addr,&r);
  if (r != -1l) {fprintf(f,"  NVMe3 STATUS  = 0x%lx\n", r);}

done:
  return;
}

int main(int argc, char **argv) {
  struct cxl_ioctl_start_work *work;
  long reported = 0;

  if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
    printf("usage: %s <path-to-afu> [-v]\n",argv[0]);
    exit(2);
  }

  struct cxl_afu_h *afu = cxl_afu_open_dev(argv[1]);
  if (!afu) {
    perror ("malloc");
    return 0;
  }
  debug ("Allocated memory at 0x%016lx for AFU\n", (__u64) afu);

  fprintf(stderr,"Creating and opening AFU file descriptor %s\n",argv[1]);
  work = (struct cxl_ioctl_start_work*) cxl_work_alloc();
  if (work == NULL) {
      perror("cxl_work_alloc");
      return 1;
  }
  if (cxl_afu_attach_work(afu, work)) {
      debug0 ("Start command to AFU failed\n");
      perror("cxl_afu_attach_work(master)");
      return 1;
  }
  else {
    debug0 ("Start command succeeded on AFU\n");
  }
  if (cxl_mmio_map(afu, CXL_MMIO_BIG_ENDIAN) != 0) {
    perror ("mmap_problem_state_registers");
    return 0;
  }

  cxl_get_mmio_size(afu, &reported);
  debug ("mmio_size 0x%lx for AFU\n", reported);

  debug0 ("Installing libcxl SIGBUS handler\n");
  cxl_mmio_install_sigbus_handler();

  __u64 addr = 4 * (0x80400C & ~ 0x1);
  cxl_mmio_read64(afu,addr,(__u64 *)&portmask);

  FILE *f = stdout;
  print_version(f,afu);
  if (argv[2] && strncmp(argv[2],"-v",2)==0) {return 0;}

  int i;
  for(i=0; i<512; i++) dump_ctxt(f,afu,i);
  for(i=0; i<512; i++) dump_cpc(f,afu,i);
  dump_gbl(f,afu);
  dump_perf(f,afu);
  dump_afu_dbg(f,afu);
  dump_fc_dbg(f,afu,0);
  dump_fc_dbg(f,afu,1);
  if (portmask==0xf)
  {
    dump_fc_dbg(f,afu,2);
    dump_fc_dbg(f,afu,3);
  }
  return 0;
}

