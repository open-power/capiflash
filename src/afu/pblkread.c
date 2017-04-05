/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/pblkread.c $                                         */
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
// some things i do occasionally
//for c in $(seq 0 9 191); do sudo cpufreq-set -c $c -r -g performance; done
//echo 0 > /proc/sys/kernel/numa_balancing
//echo 1 > /sys/class/cxl/afu0.0m/device/reset
//sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img ../../obj/tests/pblkread 




/* // simple i adapter */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c  8 ../../obj/tests/pblkread -n 20000000 -s /dev/cxl/afu0.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 16 ../../obj/tests/pblkread -n 20000000 -s /dev/cxl/afu0.0s & */

/*   // add the second adapter */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 88 ../../obj/tests/pblkread -n 20000000 -d /dev/cxl/afu1.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 96 ../../obj/tests/pblkread -n 20000000 -d /dev/cxl/afu1.0s & */

/*   //next six run both adapaters */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c  8 ../../obj/tests/pblkread -n 20000000 -s /dev/cxl/afu0.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 16 ../../obj/tests/pblkread -n 20000000 -s /dev/cxl/afu0.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 24 ../../obj/tests/pblkread -n 20000000 -s /dev/cxl/afu0.0s & */

/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 88 ../../obj/tests/pblkread -n 20000000 -d /dev/cxl/afu1.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 96 ../../obj/tests/pblkread -n 20000000 -d /dev/cxl/afu1.0s & */
/* sudo LD_LIBRARY_PATH=/home/capideveloper/capiflash/img taskset -c 104 ../../obj/tests/pblkread -n 20000000 -d /dev/cxl/afu1.0s & */



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <capiblock.h>
#include <limits.h>
#include <pthread.h>
#include <cflash_tools_user.h>

//#define PRI(wh, args...) fprintf(wh, ##args)
#define PRI(wh, args...) 

char 	  *dev_name = "/dev/cxl/afu0.0m"; /* name of device including dev */
int       num_ops = 1024;         /* no of operations per thread */
int       num_asyncs = 128;         /* no of outstanding any given at time */
int	  num_threads = 16;	     /* no of threads */
int       num_blocks = 1000000;        /* 0..num_blocks-1 lbas to be reads */
int       virt_blks = 1;
int       history = 0;

int main(int argc, char **argv) {

  int a;
  int rc;

  chunk_ext_arg_t ext = 0;
  
  for(a=1; a<argc; a++) {
    if (argv[a][0]=='-') {
      switch (argv[a][1]) {
      case 'd': dev_name     = argv[a+1]; a++; break;
      case 'n': num_ops      = atoi(argv[a+1]); a++; break;
      case 'a': num_asyncs   = atoi(argv[a+1]); a++; break;
      case 'b': num_blocks   = atoi(argv[a+1]); a++; break;
      case 't': num_threads  = atoi(argv[a+1]); a++; break;
      case 'v': virt_blks    = atoi(argv[a+1]); a++; break;
      case 'h': history      = atoi(argv[a+1]); a++; break;
      default: printf("Unknown arg '%s'\n", argv[a]);
      }
    }
  }
  printf("pblkread -d %s -n %d -a %d -t %d -b %d -v %d -h %d\n", 
	 dev_name, num_ops, num_asyncs, num_threads, num_blocks, virt_blks, history);

  rc = cblk_init(NULL,0);

  if (rc) {

      fprintf(stderr,"cblk_init failed with rc = %d and errno = %d\n",
	      rc,errno);
      exit(1);

  }
  int id = cblk_open(dev_name,num_asyncs,O_RDWR,ext,virt_blks ? CBLK_OPN_VIRT_LUN : 0);
  if (id==NULL_CHUNK_ID) {
    fprintf(stderr,"Device open failed errno = %d\n", errno);
    cblk_term(NULL,0);
    exit(errno);
  } else {
    printf("Device open success\n");
  }

  int i;
  uint64_t status;
  size_t lsize;
  rc = cblk_get_lun_size(id, &lsize, 0);
  printf("lsize = %ld, rc, = %d\n", lsize, rc);

  rc = cblk_set_size(id, num_blocks, 0);
  printf("bsize = %d, rc, = %d\n", num_blocks, rc);

  int *rtag_buf    = malloc(num_asyncs * sizeof(int));
  void **rbuf      = malloc(num_asyncs * sizeof(void *));

  int *buf_stack = malloc(num_asyncs * sizeof(int));

  for (i=0; i<num_asyncs; i++) {
    if(posix_memalign(&rbuf[i], 4096, 4096)) {
      fprintf(stderr,"posix_memalign failed, exiting\n");
      cblk_close(id,0);
      cblk_term(NULL,0);
      exit(0);
    }
    rtag_buf[i]   = -1;
    buf_stack[i] = i;
  }
  int next_buf = 0;

  int reads_issued   = 0;
  int reads_retired  = 0;
  int reads_inflight = 0;

  int read_retry = 0;
  int result_err = 0;
  int result_none = 0;

  while (reads_retired < num_ops) {
    int rtag;
    int use_buf;
    while (reads_issued < num_ops && reads_inflight < num_asyncs) {
      int blk_num = lrand48() % num_blocks;
      use_buf = buf_stack[next_buf++];
      rc = -1;
      while (rc) {
	rc = cblk_aread(id, rbuf[use_buf], blk_num, 1, &rtag,NULL,0);//CBLK_ARW_WAIT_CMD_FLAGS);
	if (rc==0) {
	  rtag_buf[rtag] = use_buf;
	  reads_issued++;
	  reads_inflight++;
	  PRI(stdout,"read %d using buffer %d tag %d\n", reads_issued, use_buf, rtag);
	} else {
	  read_retry++;
	  PRI(stdout,"ELSE %d aread buf %d rc = %d\n", read_retry, use_buf, rc);
	}
      }
    }
    //    while (rc!=1) {
    rc = cblk_aresult(id, &rtag,&status,CBLK_ARESULT_BLOCKING | CBLK_ARESULT_NEXT_TAG);
      if (rc > 0) {
	reads_retired++;
	reads_inflight--;
	use_buf = rtag_buf[rtag];
	buf_stack[--next_buf] = use_buf;
	PRI(stdout,"result tag %d, rc %d\n",rtag, rc); 
      } else if (rc==0) {
	result_none++;
	PRI(stdout,"ELSE %d aresult rc = %d, tag = %d\n", result_none,rc, rtag);
      } else {
	result_err++;
	PRI(stdout,"ELSE %d aresult rc = %d, tag = %d\n", result_err,rc, rtag);
      }
      //}
  }

  printf("retries = %d, result_none = %d, result_err = %d\n", read_retry, result_none, result_err);
  cblk_close(id,0);
  cblk_term(NULL,0);
  exit(0);
}
