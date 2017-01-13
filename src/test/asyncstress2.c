/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/asyncstress2.c $                                     */
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
#include <sys/time.h>
#include <ticks.h>

//#define PRI(wh, args...) fprintf(wh, ##args)
#define PRI(wh, args...) 
#define EPRI(wh, args...) fprintf(wh, ##args)

char      *run_name = "Unnamed";
char 	  *dev_name = "/dev/cxl/afu0.0s"; /* name of device including dev */
int       num_ops = 1024;         /* no of operations per thread */
int       num_asyncs = 128;         /* no of outstanding any given at time */
int	  num_threads = 1;	     /* no of threads */
int       num_blocks = 1000000;        /* 0..num_blocks-1 lbas to be reads */
int       virt_blks = 1;
char      *collect = NULL;
int       read_block  = 100;
int       write_block = 0;
int       wait_inorder = 0;

#define PSTAT(retry, err, none, thru, rop_min, rop_max, rop_sum, rop_cnt, wop_min, wop_max, wop_sum, wop_cnt, nspt) \
  printf("%s,d,%s,n,%d,a,%d,t,%d,b,%d,v,%d,c,%s,r,%d,w,%d,o,%d,retry,%d,err,%d,none,%d,thru,%g,rmin,%lu,rmax,%lu,ravg,%lu,wmin,%lu,wmax,%lu,wavg,%lu,\n", \
	 run_name ,							\
	 dev_name ,							\
	 num_ops ,							\
	 num_asyncs ,							\
	 num_threads ,							\
	 num_blocks ,							\
	 virt_blks ,							\
	 collect ,							\
	 read_block  ,							\
	 write_block ,							\
	 wait_inorder,							\
	 retry, err, none,						\
	 thru,								\
	 rop_cnt ? ((uint64_t)(1e-3 * nspt * rop_min)) : 0,		\
	 rop_cnt ? ((uint64_t)(1e-3 * nspt * rop_max)) : 0,		\
	 rop_cnt ? ((uint64_t)(1e-3 * nspt * (rop_sum / rop_cnt))) : 0,	\
	 wop_cnt ? ((uint64_t)(1e-3 * nspt * wop_min)) : 0,		\
	 wop_cnt ? ((uint64_t)(1e-3 * nspt * wop_max)) : 0,		\
	 wop_cnt ? ((uint64_t)(1e-3 * nspt * (wop_sum / wop_cnt))) : 0	\
)

int main(int argc, char **argv) {


  int a;
  int rc;
  chunk_ext_arg_t ext = 0;
  
  for(a=1; a<argc; a++) {
    if (argv[a][0]=='-') {
      switch (argv[a][1]) {
      case 'p': run_name     = argv[++a]; break;
      case 'd': dev_name     = argv[++a]; break;
      case 'n': num_ops      = atoi(argv[++a]); break;
      case 'a': num_asyncs   = atoi(argv[++a]); break;
      case 'b': num_blocks   = atoi(argv[++a]); break;
      case 't': num_threads  = atoi(argv[++a]); break;
      case 'v': virt_blks    = atoi(argv[++a]); break;
      case 'c': collect      = argv[++a]; break;
      case 'r': read_block   = atoi(argv[++a]); break;
      case 'w': write_block  = atoi(argv[++a]); break;
      case 'o': wait_inorder = atoi(argv[++a]); break;
      case 'h':
	printf("usage: asyncstress <args>\n");
	printf("-d <name>  'name of device, usually /dev/cxl/af0.0s or /dev/cxl/afu1.0s'\n");
	printf("-b <int> '# of blocks to run the test over, i.e. lba=0..b-1\n");
	printf("-n <int> '# of ops to run (combination of reads and writes total'\n");
	printf("-a <int> '# of outstanding ops to maintain'\n");
	printf("-r <int> '# for read  ratio, read  ratio is r/(r+w)'\n");
	printf("-w <int> '# for write ratio, write ratio is w/(r+w)'\n");
	printf("-t <int> '# threads (not implemented yet)'\n");
	printf("-v <bool> '0/1 to use virtual or physical lba (not implemented'\n");
	printf("-o <bool> '0/1 to retire ops inorder/any order'\n");
	printf("-c <fname> 'collect statistics in file 'fname''\n");
	printf("-p <name> 'arbitrary name echoed in the stats line to help identify run stats\n");
	exit(1);
      default: 
	printf("Unknown arg '%s'\n", argv[a]);
	exit(1);
      }
    }
  }
  printf("asyncstress -d %s -n %d -a %d -t %d -b %d -v %d -c %s\n", 
	 dev_name, num_ops, num_asyncs, num_threads, num_blocks, virt_blks, collect);

  double ns_per_tick = time_per_tick(1000, 100);
  printf("%g ns/tick\n", ns_per_tick);

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

  int      *st_type    = NULL;
  uint64_t *st_start   = NULL;
  uint64_t *st_issue   = NULL;
  uint64_t *st_result  = NULL;
  uint64_t *st_retire  = NULL;
  if (collect) {
    st_type   = malloc(num_ops * sizeof(int));
    st_start  = malloc(num_ops * sizeof(uint64_t));
    st_issue  = malloc(num_ops * sizeof(uint64_t));
    st_result = malloc(num_ops * sizeof(uint64_t));
    st_retire = malloc(num_ops * sizeof(uint64_t));
    bzero(st_type, num_ops * sizeof(int));
    bzero(st_start, num_ops * sizeof(uint64_t));
    bzero(st_issue, num_ops * sizeof(uint64_t));
    bzero(st_result, num_ops * sizeof(uint64_t));
    bzero(st_retire, num_ops * sizeof(uint64_t));
  }

  int i;
  uint64_t status;
  size_t lsize;
  rc = cblk_get_lun_size(id, &lsize, 0);
  printf("lsize = %ld, rc, = %d\n", lsize, rc);

  rc = cblk_set_size(id, num_blocks, 0);
  printf("bsize = %d, rc, = %d\n", num_blocks, rc);

  int *wait_order = malloc(num_asyncs * sizeof(int));
  int wait_pos = 0;
  int wait_next = 0;

  int *rtag_buf    = malloc(num_asyncs * sizeof(int));
  void **rbuf      = malloc(num_asyncs * sizeof(void *));

  int *buf_stack = malloc(num_asyncs * sizeof(int));

  int *op_type = malloc(num_asyncs * sizeof(int));
  uint64_t *op_start = malloc(num_asyncs * sizeof(uint64_t));
  uint64_t *op_stop  = malloc(num_asyncs * sizeof(uint64_t));
  uint64_t *op_num   = malloc(num_asyncs * sizeof(uint64_t));

  uint64_t rop_min = UINT64_C(0x7fffffffffffffff);
  uint64_t rop_max = 0;
  uint64_t rop_sum = 0;
  uint64_t rop_cnt = 0;
  uint64_t wop_min = UINT64_C(0x7fffffffffffffff);
  uint64_t wop_max = 0;
  uint64_t wop_sum = 0;
  uint64_t wop_cnt = 0;

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

  int ops_issued   = 0;
  int ops_retired  = 0;
  int ops_inflight = 0;

  int op_retry = 0;
  int op_err = 0;
  int op_none = 0;

  int rw = read_block + write_block;

  uint64_t tmp_start, tmp_issue, tmp_result, tmp_retire;

  uint64_t base = getticks();

  uint64_t start_ticks = base;
  while (ops_retired < num_ops) {
    int rtag;
    int use_buf;
    while (ops_issued < num_ops && ops_inflight < num_asyncs) {
      int blk_num = lrand48() % num_blocks;
      use_buf = buf_stack[next_buf++];
      rc = -1;

      while (rc) {
	tmp_start = getticks();
	if (read_block && !write_block) {
	  rc = cblk_aread(id, rbuf[use_buf], blk_num, 1, &rtag,NULL,CBLK_ARW_WAIT_CMD_FLAGS);
	  op_type[rtag] = 0;
	  //	  rop_cnt++;
	} else if (write_block && !read_block) {
	  rc = cblk_awrite(id, rbuf[use_buf], blk_num, 1, &rtag,NULL,CBLK_ARW_WAIT_CMD_FLAGS);
	  op_type[rtag] = 1;
	  //wop_cnt++;
	} else {
	  int rnd = lrand48() % rw;
	  if (rnd < read_block) {
	    rc = cblk_aread(id, rbuf[use_buf], blk_num, 1, &rtag,NULL,CBLK_ARW_WAIT_CMD_FLAGS);
	    op_type[rtag] = 0;
	    //rop_cnt++;
	  } else {
	    rc = cblk_awrite(id, rbuf[use_buf], blk_num, 1, &rtag,NULL,CBLK_ARW_WAIT_CMD_FLAGS);
	    op_type[rtag] = 1;
	    //wop_cnt++;
	  }
	}
	tmp_issue = getticks();
	if (rc==0) {

	  if (collect) {
	    st_type[ops_issued] = op_type[rtag];
	    st_start[ops_issued] = tmp_start;
	    st_issue[ops_issued] = tmp_issue;
	    op_num[rtag] = ops_issued;
	  }

	  op_start[rtag] = tmp_start;
	  wait_order[wait_pos++] = rtag;
	  wait_pos %= num_asyncs;
	  rtag_buf[rtag] = use_buf;
	  ops_inflight++;

	  ops_issued++;
	  
	  PRI(stdout,"read %d using buffer %d tag %d\n", ops_issued, use_buf, rtag);
	} else {
	  op_retry++;
	  PRI(stdout,"ELSE %d aread buf %d rc = %d\n", read_retry, use_buf, rc);
	}
      }
    }
    uint64_t op_diff = 0;
    tmp_result = getticks();
    if (wait_inorder) {
      rtag = wait_order[wait_next++];
      rc = cblk_aresult(id, &rtag,&status,CBLK_ARESULT_BLOCKING);
      wait_next %= num_asyncs;
    } else {
      rc = cblk_aresult(id, &rtag,&status,CBLK_ARESULT_BLOCKING | CBLK_ARESULT_NEXT_TAG);
    }
    tmp_retire = getticks();
    if (rc==0) {
      op_none++;
      PRI(stdout,"ELSE %d aresult rc = %d, tag = %d\n", result_none,rc, rtag);
    } else {
      if (rc < 0) {
	op_err++;
	PRI(stdout,"ELSE %d aresult rc = %d, tag = %d\n", result_err,rc, rtag);
      } else {
	op_stop[rtag] = getticks();
	st_result[op_num[rtag]] = tmp_result;
	st_retire[op_num[rtag]] = tmp_retire;
	op_diff = op_stop[rtag] - op_start[rtag];
	if (op_type[rtag]) {
	  wop_cnt++;
	  wop_sum += op_diff;
	  if (op_diff>wop_max) wop_max = op_diff;
	  if (op_diff<wop_min) wop_min = op_diff;
	} else {
	  rop_cnt++;
	  rop_sum += op_diff;
	  if (op_diff>rop_max) rop_max = op_diff;
	  if (op_diff<rop_min) rop_min = op_diff;
	}
      }
      ops_retired++;
      ops_inflight--;
      use_buf = rtag_buf[rtag];
      buf_stack[--next_buf] = use_buf;
      PRI(stdout,"result tag %d, rc %d\n",rtag, rc); 
    } 
  }
  uint64_t stop_ticks = getticks();
  
  double thru = 1e9 * num_ops / (ns_per_tick * elapsed(stop_ticks, start_ticks));
  PSTAT(op_retry, op_err, op_none, thru, rop_min, rop_max, rop_sum, rop_cnt, wop_min, wop_max, wop_sum, wop_cnt, ns_per_tick);
  //printf("retries = %d, result_none = %d, result_err = %d\n", read_retry, result_none, result_err);
  cblk_close(id,0);

  cblk_term(NULL,0);
  if (collect) {

    FILE *F = fopen(collect, "w");
    fprintf(F, "RW, Start, Issue, Result, Retire\n");

    for(i=0; i<num_ops; i++) {
      fprintf(F,"%d, %f, %f, %f, %f\n", 
	     st_type[i], 
	     1e-3 * ns_per_tick * (double)(st_start[i]-base), 
	     1e-3 * ns_per_tick * (double)(st_issue[i]-base), 
	     1e-3 * ns_per_tick * (double)(st_result[i]-base), 
	     1e-3 * ns_per_tick * (double)(st_retire[i]-base)//,
	     //	     1e-3 * ns_per_tick * (double)(st_retire[i] - st_start[i]), 
	     //1e-3 * ns_per_tick * (double)(st_issue[i] - st_start[i]), 
	     //1e-3 * ns_per_tick * (double)(st_retire[i] - st_result[i]) 
	     );
    }
    fclose(F);
  }

  exit(0);
}
