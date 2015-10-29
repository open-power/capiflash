/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test.c $                                   */
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
#include "mc_test.h"
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define MMIO_MAP_SIZE 64*1024

extern char master_dev_path[MC_PATHLEN];
extern char afu_path[MC_PATHLEN];

int mc_permute(void *arg);

extern int g_error;
extern pid_t pid;

//Routine to call mc APIs in order.
int mc_permute(void *arg)
{
  ctx_p p_ctx = (ctx_p)arg;
  int rc = 0;
  mc_stat_t l_mc_stat;

  rc = mc_register(master_dev_path, p_ctx->ctx_hndl, (volatile __u64 *)p_ctx->p_host_map, &p_ctx->mc_hndl_p->mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
    g_error = -1;
    return -1;
  }

  rc = mc_open(p_ctx->mc_hndl_p->mc_hndl, MC_RDWR, &p_ctx->mc_hndl_p->res_hndl_p->res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl rc %d\n", rc);
    g_error = -1;
    return -1;
  }

  rc = mc_stat(p_ctx->mc_hndl_p->mc_hndl, p_ctx->mc_hndl_p->res_hndl_p->res_hndl, &l_mc_stat);
  debug("Size of new lba = %lu\n", l_mc_stat.size);

  rc = mc_close(p_ctx->mc_hndl_p->mc_hndl, p_ctx->mc_hndl_p->res_hndl_p->res_hndl );
  if(rc != 0) {
    fprintf(stderr, "mc_close() failed: Error closing res_hndl %d\n", p_ctx->mc_hndl_p->res_hndl_p->res_hndl);
    g_error = -1;
    return -1;
  }

  rc = mc_unregister(p_ctx->mc_hndl_p->mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_unregister() failed: Error unregistering for mc_hndl %p\n", p_ctx->mc_hndl_p->mc_hndl);
    g_error = -1;
    return -1;
  }

  return 0;
}

int mc_register_tst()
{
  struct ctx myctx;
  int rc = 0;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  debug("mc_register() success: registered mc_hndl %p for ctx_hndl %d\n", myctx.mc_hndl, myctx.ctx_hndl);

  ctx_close(&myctx);
  mc_term();
  return 0;
}

int mc_unregister_tst()
{
  struct ctx myctx;
  int rc = 0;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc_hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  rc = mc_unregister(myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_unregister() failed: Error unregistering mc_hndl %p\n", myctx.mc_hndl);
    mc_term();
    return -1;
  }

  debug("mc_unregister() success: unregistered mc_hndl for ctx_hndl %d\n", myctx.ctx_hndl);

  ctx_close(&myctx);
  mc_term();
  return 0;
}

int mc_open_tst(int cmd)
{
  struct ctx myctx;
  pthread_t thread;
  int rc = 0, res = 0;
  __u64 size = 16;
  __u64 actual_size;
  __u64 start_lba = 0;
  __u64 stride = 0x1000;
  mc_stat_t l_mc_stat;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  pid=getpid();

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  pthread_create(&thread, NULL, ctx_rrq_rx, &myctx);

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc-hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  //Negative test 4: Do IO without doing opening any resource.
  if(cmd == 4) {
    debug("Time to attempt IO without mc_open().\n");

    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on read-only vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on write-only vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = rc;
  }

  //Positive test 1: Do mc_open with MC_RDWR flag
  if(cmd == 1) {
    rc = mc_open(myctx.mc_hndl, MC_RDWR, &myctx.res_hndl);
    if(rc != 0) {
      fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
      return -1;
    }
    debug("mc_open() success: opened res_hndl %d for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);

    res = 0;
  }

  //Negative test 2: Do mc_open with MC_RDONLY and do IO read write. Expect write fail
  if(cmd == 2) {
    rc =  mc_open(myctx.mc_hndl, MC_RDONLY, &myctx.res_hndl);
    if(rc != 0) {
      fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
      return -1;
    }
    debug("mc_open() success: opened res_hndl %d for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);

    //Setting some size
    rc = mc_size(myctx.mc_hndl, myctx.res_hndl, size, &actual_size);
    if(rc != 0) {
      fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu.\n", size);
      mc_term();
      return -1;
    }

	rc = mc_stat(myctx.mc_hndl, myctx.res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");
    debug("Size of new lba = %lu\n", actual_size);

	stride = (1 << l_mc_stat.nmask);
    debug("%d : Time to do IO on 0x%lX sized vdisks.\n", pid, actual_size);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on read-only vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc != 0) {
      fprintf(stderr, "send_read() success: rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = 0x5;
  }

  //Negative test 3: mc_open with MC_WRONLY and do IO read write. Expect read fail.
  if(cmd == 3) {
    rc =  mc_open(myctx.mc_hndl, MC_WRONLY, &myctx.res_hndl);
    if(rc != 0) {
      fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
      return -1;
    }
    debug("mc_open() success: opened res_hndl %d for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);

    //Setting some size
    rc = mc_size(myctx.mc_hndl, myctx.res_hndl, size, &actual_size);
    if(rc != 0) {
      fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu.\n", size);
      mc_term();
      return -1;
    }

    debug("Size of new lba = %lu\n", actual_size);

    debug("Time to do IO on 0x%lX sized vdisks.\n", actual_size);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc != 0) {
      fprintf(stderr, "send_write() failed: rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on write-only vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = 0x5;
  }

  pthread_cancel(thread);
  mc_close(myctx.mc_hndl,myctx.res_hndl);
  mc_unregister(myctx.mc_hndl);

  ctx_close(&myctx);
  mc_term();
  return res;
}

int mc_close_tst(int cmd)
{
  struct ctx myctx;
  pthread_t thread;
  int rc = 0, res = 0;
  __u64 size = 10;
  __u64 actual_size;
  __u64 start_lba = 0;
  __u64 stride;
  mc_stat_t l_mc_stat;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "context initialization failed.\n");
    mc_term();
    return -1;
  }

  pthread_create(&thread, NULL, ctx_rrq_rx, &myctx);

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc-hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  rc = mc_open(myctx.mc_hndl, MC_RDWR, &myctx.res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
    return -1;
  }
  debug("mc_open() success: opened res_hndl %d for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);

  rc = mc_size(myctx.mc_hndl, myctx.res_hndl, size, &actual_size);
  if(rc != 0) {
    fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu, rc %d\n",size, rc);
    mc_term();
    return -1;
  }

	rc = mc_stat(myctx.mc_hndl, myctx.res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");

	stride = (1 << l_mc_stat.nmask);
  //Positive test 1: Checking mc_close.
  if(cmd == 1) {
    rc = mc_close(myctx.mc_hndl,myctx.res_hndl);
    if(rc != 0) {
      fprintf(stderr, "mc_close() failed: rc %d\n", rc);
      return -1;
    }
    debug("mc_close() success: res_hndl %d closed for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);
    res = 0;
  }

  pid = getpid();
  //Negative test 2: Doing read write after freeing res_hndl.
  if(cmd == 2) {
    rc = mc_close(myctx.mc_hndl,myctx.res_hndl);
    if(rc != 0) {
      fprintf(stderr, "mc_close() failed: rc %d\n", rc);
      return -1;
    }
    debug("mc_close() success: res_hndl %d closed for mc_hndl %p\n", myctx.res_hndl, myctx.mc_hndl);

    debug("Doing IO on freed res_hndl %d\n", myctx.res_hndl);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = 0x5;
  }

  pthread_cancel(thread);
  mc_unregister(myctx.mc_hndl);
  ctx_close(&myctx);
  mc_term();
  return res;
}

int mc_size_tst(int cmd)
{
  struct ctx myctx;
  pthread_t thread;
  int rc = 0, res = 0;
  pid_t pid;
  __u64 size = 0;
  __u64 nlba = 0;
  __u64 actual_size;
  __u64 start_lba = 0;
  __u64 stride = 0x1000;
  __u64 chunk[] = {10, 15, 23, 45, 90, 55, 40, 5, 20 };
  mc_stat_t l_mc_stat;

  int i = 0;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  pthread_create(&thread, NULL, ctx_rrq_rx, &myctx);

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc-hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  rc = mc_open(myctx.mc_hndl, MC_RDWR, &myctx.res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
    mc_term();
    return -1;
  }

  rc = mc_stat(myctx.mc_hndl,  myctx.res_hndl, &l_mc_stat);
  if(rc != 0) {
    fprintf(stderr, "mc_stat() failed: rc %d\n",rc);
    mc_term();
    return -1;
  }
  size = l_mc_stat.size;
  if(size != 0) {
    fprintf(stderr, "Wrong size value. Expected = 0, Output = %lu \n",size);
    mc_term();
    return -1;
  }

  debug("Size of new lba = %lu\n", size);

  pid=getpid();

  //Negative test 2: Doing IO on a zero size vdisk
  if(cmd == 2) {
    debug("Doing IO on NULL sized vdisks.\n");
    rc = send_write(&myctx, 0, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on NULL vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, 0, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on NULL vdisk should fail. rc %d\n", rc);
      mc_term();
      return -1;
    }

	return rc;
  }
  /*This loop does mc_size for various chunk sizes and performs in range
    and out of range IO*/
  for(i = 0; i<sizeof(chunk)/sizeof(__u64); i++) {
    rc = mc_size(myctx.mc_hndl, myctx.res_hndl, chunk[i], &actual_size);
    if(rc != 0) {
      fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu. rc %d\n",chunk[i], rc);
      mc_term();
      return -1;
    }

    rc = mc_stat(myctx.mc_hndl,  myctx.res_hndl, &l_mc_stat);
    if(rc != 0) {
      fprintf(stderr, "mc_stat() failed: rc %d\n",rc);
      mc_term();
      return -1;
    }

	size = l_mc_stat.size;
    if(size != actual_size) {
      fprintf(stderr, "Unequal size values. Expected %lu Actual %lu\n", actual_size, size);
      mc_term();
      return -1;
    }

    debug("mc_size() success: size requested %lu size allocated %lu\n", chunk[i], actual_size);
    debug("mc_stat() success: size expected %lu size returned %lu\n",actual_size, size);

    nlba = chunk[i] * (1 << l_mc_stat.nmask);

    //Negative test 3: Doing write in the out of range LBA. Expecting failure.
    if(cmd == 3) {
      debug("Doing out of range IO i.e. on LBA = %lX. Expecting failure.\n", nlba);
      rc = send_write(&myctx, nlba, stride, pid, VLBA);
      if(rc == 0) {
        fprintf(stderr, "send_read() on NULL vdisk should fail %d\n", rc);
        mc_term();
        return -1;
      }

      res = rc;
    }

    //Positive test 1: Doing IO on vdisk along with mc_size().
    if(cmd == 1) {
      debug("Time to do in-range IO on %lu size vdisk\n", actual_size);

      for(start_lba = 0; start_lba <nlba; start_lba+=(NUM_CMDS*stride)) {
        mc_stat(myctx.mc_hndl, myctx.res_hndl, &l_mc_stat);
		size = l_mc_stat.size;

        debug("Doing IO on vdisk size 0x%lX at start_lba 0x%lX nlba = 0x%lX \n", size, start_lba, size*stride);
        rc = send_write(&myctx, start_lba, stride, pid, VLBA);
        if(rc) {
          fprintf(stderr, "send_write() failed: write failed for start_lba 0X%lX for vidsk size 0X%lX, rc 0X%X\n", start_lba, actual_size, rc);
          mc_term();
          return -1;
        }

        rc = send_read(&myctx, start_lba, stride, VLBA);
        if(rc) {
          fprintf(stderr, "send_read() failed: read failed for start_lba 0X%lX for vidsk size 0X%lX, rc 0X%X\n", start_lba, actual_size, rc);
          mc_term();
          return -1;
        }

        rc = rw_cmp_buf(&myctx, start_lba);
        if(rc) {
          fprintf(stderr, "rw_cmp_buf() failed: Unequal read write buffers for vlba 0X%lX, rc 0X%X\n", start_lba, rc );
          mc_term();
          return -1;
        }
      }

      res = 0;
    }
  }

  //Negative test 4: set vdisk size zero and do read write.
  if(cmd == 4) {
    rc = mc_size(myctx.mc_hndl, myctx.res_hndl, 0, &actual_size);
    if(rc != 0) {
      fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu, rc %d\n", 0l, rc);
      mc_term();
      return -1;
    }

    rc = send_write(&myctx, 0, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on NULL vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, 0, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on NULL vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = 4;
  }

  pthread_cancel(thread);
  mc_close(myctx.mc_hndl,myctx.res_hndl);
  mc_unregister(myctx.mc_hndl);
  ctx_close(&myctx);
  mc_term();

  return res;
}

int mc_xlate_tst(int cmd)
{
  struct ctx myctx;
  pthread_t thread;
  __u64 size[] = {16, 64};
  __u64 nlba[2] = {0};
  __u64 plba[2];
  __u64 actual_size[2];
  __u64 stride = LBA_BLK;
  mc_stat_t l_mc_stat;

  //Positive test 1: Do write and read on VLBA. Compare.
  int rd_flag = VLBA;
  int wr_flag = VLBA;
  int rc = 0, res = 0;

  //Extra resource handles
  res_hndl_t myres_hndl[2];

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");


  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  pthread_create(&thread, NULL, ctx_rrq_rx, &myctx);

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc-hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  //Opening on first res_hndl.
  rc = mc_open(myctx.mc_hndl, MC_RDWR, &myres_hndl[0]);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
    mc_term();
    return -1;
  }

  //Opening on second res_hndl
  rc = mc_open(myctx.mc_hndl, MC_RDWR, &myres_hndl[1]);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl, rc);
    mc_term();
    return -1;
  }

  //Sizing first res_hndl
  rc = mc_size(myctx.mc_hndl, myres_hndl[0], size[0], &actual_size[0]);
  if(rc != 0) {
    fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu, rc %d.\n", size[0], rc);
    mc_term();
    return -1;
  }

  debug("size for res_hndl1 %d = %lu \n", myres_hndl[0], actual_size[0]);
  rc = mc_stat(myctx.mc_hndl, myres_hndl[0], &l_mc_stat);
  CHECK_RC(rc, "mc_stat");
  nlba[0] = actual_size[0] * (1 << l_mc_stat.nmask);

  //Sizing second res_hndl
  rc = mc_size(myctx.mc_hndl, myres_hndl[1], size[1], &actual_size[1]);
  if(rc != 0) {
    fprintf(stderr, "mc_size() failed: Unable to resize vdisk to %lu, rc %d.\n", size[1], rc);
    mc_term();
    return -1;
  }

  rc = mc_stat(myctx.mc_hndl, myres_hndl[1], &l_mc_stat);
  CHECK_RC(rc, "mc_stat");

  debug("size for res_hndl2 %d = %lu \n", myres_hndl[1], actual_size[1]);
  nlba[1] = actual_size[1] * (1 << l_mc_stat.nmask);

  pid = getpid();

  int j = 0;
  while(j<2) {
    //Creating lba range array.
    __u64 start_lba[] = {0, nlba[j]/2, nlba[j]/2-1, nlba[j]/2+1, nlba[j]-(NUM_CMDS*LBA_BLK)};

    myctx.res_hndl = myres_hndl[j];

    int i = 0;
    //Doing mc_xlate_lba on each lba range followed by read write.
    for(i = 0; i < 5; i++) {
      rc = mc_xlate_lba(myctx.mc_hndl, myctx.res_hndl, start_lba[i], &plba[j]);
      if(rc != 0) {
        fprintf(stderr, "mc_xlate_lba() failed: Unable to find physical lba for vlba 0x%lX.\n", start_lba[i]);
        mc_term();
        return -1;
      }

      debug("mc_xlate_lba() success: plba value 0x%lX for vlba 0x%lX.\n",plba[j], start_lba[i]);

      //Positive test 2: Do write on VLBA and read on PLBA. Compare.
      if(cmd ==2) {
        wr_flag = VLBA;
        rd_flag = PLBA;
      }

      //Positive test 3: Do write on PLBA and read on VLBA. Compare.
      if(cmd == 3) {
        wr_flag = PLBA;
        rd_flag = VLBA;
      }

      debug("Doing IO on vdisk under res_hndl %d.\n", myctx.res_hndl);
      rc = send_write(&myctx, start_lba[i], stride, pid, wr_flag);
      if(rc) {
        fprintf(stderr, "send_write() failed: write failed for start_lba 0X%lX for vidsk size 0X%lX, rc 0X%X\n", start_lba[i], actual_size[j], rc);
		pthread_cancel(thread);
        mc_term();
        return rc;
      }

      rc = send_read(&myctx, start_lba[i], stride, rd_flag);
      if(rc) {
        fprintf(stderr, "send_read() failed: read failed for start_lba 0X%lX for vidsk size 0X%lX, rc 0X%X\n", start_lba[i], actual_size[j], rc);
		pthread_cancel(thread);
        mc_term();
        return rc;
      }

      rc = rw_cmp_buf(&myctx, start_lba[i]);
      if(rc) {
        fprintf(stderr, "rw_cmp_buf() failed: Unequal read write buffers for vlba 0x%lX, rc %d\n", start_lba[i], rc );
		pthread_cancel(thread);
        mc_term();
        return rc;
      }

      res = 0;
    }

    j++;
  }

  pthread_cancel(thread);
  mc_close(myctx.mc_hndl,myctx.res_hndl);
  mc_unregister(myctx.mc_hndl);

  ctx_close(&myctx);
  mc_term();
  return res;
}

int mc_hdup_tst(int cmd)
{
  struct ctx myctx;
  pthread_t thread;
  int rc = 0, res = 0;
  pid_t pid;
  __u64 size = 0;
  __u64 nlba = 0;
  __u64 actual_size;
  __u64 start_lba = 0;
  __u64 stride = 0x1000;
  __u64 chunk[] = {10, 15, 23, 45, 90, 55, 40, 5, 20 };
  mc_stat_t l_mc_stat;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  rc = ctx_init(&myctx);
  if(rc != 0) {
    fprintf(stderr, "contex initialization failed.\n");
    mc_term();
    return -1;
  }

  pthread_create(&thread, NULL, ctx_rrq_rx, &myctx);

  //Allocating two mc_hndl and two res_hndls under each mc_hndl. 
  myctx.mc_hndl_p = (struct mc_hndl_s *)malloc(2*sizeof(struct mc_hndl_s));
  myctx.mc_hndl_p[0].res_hndl_p = (struct res_hndl_s *)malloc(2*sizeof(struct res_hndl_s));
  myctx.mc_hndl_p[1].res_hndl_p = (struct res_hndl_s *)malloc(2*sizeof(struct res_hndl_s));

  rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl_p[0].mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_register() failed: Error registering mc-hndl for ctx_hndl %d, rc %d\n", myctx.ctx_hndl, rc );
    mc_term();
    return -1;
  }

  //Failing now. Error path
  //Calling mc_register once more with same values and expecting a failure this time.
  /*rc = mc_register(master_dev_path, myctx.ctx_hndl, (volatile __u64 *)myctx.p_host_map, &myctx.mc_hndl_p->mc_hndl);
  if(rc == 0) {
    fprintf(stderr, "mc_register() succes: Expecting failure. mc_register() with same values should fail for subsequent call but first. rc %d\n", rc);
    mc_term();
    return -1;
  }*/

  pid = getpid();

  //Duping the mc_hndl
  rc = mc_hdup(myctx.mc_hndl_p[0].mc_hndl, &myctx.mc_hndl_p[1].mc_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_hdup() failed: Error duping mc_hndl %p for ctx_hndl %d, rc %d\n",
      myctx.mc_hndl_p[0].mc_hndl, myctx.ctx_hndl, rc);
    mc_term();
    return -1;
  }

  //Opening two resources using original mc_hndl.
  rc = mc_open(myctx.mc_hndl_p[0].mc_hndl, MC_RDWR, &myctx.mc_hndl_p[0].res_hndl_p[0].res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl_p[0].mc_hndl, rc);
    mc_term();
    return -1;
  }

  debug("res_hndl %d created for first mc_hndl %p\n", myctx.mc_hndl_p[0].res_hndl_p[0].res_hndl, myctx.mc_hndl_p[0].mc_hndl);

  rc = mc_open(myctx.mc_hndl_p[0].mc_hndl, MC_RDWR, &myctx.mc_hndl_p[0].res_hndl_p[1].res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl_p[0].mc_hndl, rc);
    mc_term();
    return -1;
  }

  debug("res_hndl %d created for first mc_hndl %p\n", myctx.mc_hndl_p[0].res_hndl_p[1].res_hndl, myctx.mc_hndl_p[0].mc_hndl);

  //Opening two more resources using duplicated mc_hndl
  rc = mc_open(myctx.mc_hndl_p[1].mc_hndl, MC_RDWR, &myctx.mc_hndl_p[1].res_hndl_p[0].res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl_p[0].mc_hndl, rc);
    mc_term();
    return -1;
  }

  debug("res_hndl %d created for second mc_hndl %p\n", myctx.mc_hndl_p[1].res_hndl_p[0].res_hndl, myctx.mc_hndl_p[1].mc_hndl);

  rc = mc_open(myctx.mc_hndl_p[0].mc_hndl, MC_RDWR, &myctx.mc_hndl_p[1].res_hndl_p[1].res_hndl);
  if(rc != 0) {
    fprintf(stderr, "mc_open() failed: Error opening res_hndl for mc_hndl %p, rc %d\n", myctx.mc_hndl_p[0].mc_hndl, rc);
    mc_term();
    return -1;
  }

  debug("res_hndl %d created for second mc_hndl %p\n", myctx.mc_hndl_p[1].res_hndl_p[1].res_hndl, myctx.mc_hndl_p[0].mc_hndl);

  int i = 0;
  int j = 0;
  int k = 0;

  debug("Time to do IO\n");

  while(i<2)
  {
    j = 0;
    while(j<2){
      myctx.res_hndl = myctx.mc_hndl_p[i].res_hndl_p[j].res_hndl;
      debug("Choosing res_hndl %d\n", myctx.res_hndl);
      k = 0;
      while(k<5)
      {
        //Size get size verification on original and duplicate mc_hndls
        mc_size(myctx.mc_hndl_p[0].mc_hndl, myctx.res_hndl, chunk[k], &actual_size);
        mc_stat(myctx.mc_hndl_p[1].mc_hndl, myctx.res_hndl, &l_mc_stat);
		size = l_mc_stat.size;

        if(size != actual_size) {
          fprintf(stderr, "different size values for res_hndl %d on original mc_hndl %p and duplicate mc_hndl %p\n",
          myctx.res_hndl, myctx.mc_hndl_p[0].mc_hndl, myctx.mc_hndl_p[1].mc_hndl);
          fprintf(stderr, "Expected size: 0x%lX actual size: 0x%lX\n", actual_size, size);
          ctx_close(&myctx);
          mc_term();
          return -1;
        }

        debug("Expecte size 0x%lX actual size 0x%lX \n", actual_size, size);

        nlba = chunk[k] * (1 << l_mc_stat.nmask);
        //Positive test 1: Allocate resource on both mc_hndl. Do write on original and read from duplicate. Compare.
        //Positive test 2: Allocate resource on both mc_hndl. Do write on duplicate and read from original. Compare.
        if(cmd == 1 || cmd == 2) {
          //IO Loop
          for(start_lba = 0; start_lba <nlba; start_lba+=(NUM_CMDS*stride)) {
            if(cmd == 1){
              //Setting primary mc_hndl for this context to original for writing
              debug("Choosing original mc_hndl %p for write\n", myctx.mc_hndl_p[0].mc_hndl);
              myctx.mc_hndl = myctx.mc_hndl_p[0].mc_hndl;
            }

            if(cmd == 2) {
              //Setting primary mc_hndl for this context to duplicate for writing
              debug("Choosing duped mc_hndl %p for write\n", myctx.mc_hndl_p[1].mc_hndl);
              myctx.mc_hndl = myctx.mc_hndl_p[1].mc_hndl;
            }

            debug("Doing write on vdisk size 0x%lX at start_lba 0x%lX nlba = 0x%lX \n", size, start_lba, size*stride);
            rc = send_write(&myctx, start_lba, stride, pid, VLBA);
            if(rc) {
              fprintf(stderr, "send_write() failed: write failed for start_lba %lX for vidsk size %lu, rc %d\n", start_lba, actual_size, rc);
              mc_term();
              return -1;
            }

            if(cmd == 1) {
              //Setting primary mc_hndl for this context to duplicate for reading
              debug("Choosing duped mc_hndl %p for read\n", myctx.mc_hndl_p[1].mc_hndl);
              myctx.mc_hndl = myctx.mc_hndl_p[1].mc_hndl;
            }

            if(cmd == 2) {
              //Setting primary mc_hndl for this context to original for reading
              debug("Choosing original mc_hndl %p for read\n", myctx.mc_hndl_p[0].mc_hndl);
              myctx.mc_hndl = myctx.mc_hndl_p[0].mc_hndl;
            }

            debug("Doing from on vdisk size 0x%lX at start_lba 0x%lX nlba = 0x%lX \n", size, start_lba, size*stride);
            rc = send_read(&myctx, start_lba, stride, VLBA);
            if(rc) {
              fprintf(stderr, "send_read() failed: read failed for start_lba 0X%lX for vidsk size 0X%lX, rc 0X%X\n", start_lba, actual_size, rc);
              mc_term();
              return -1;
            }

            rc = rw_cmp_buf(&myctx, start_lba);
            if(rc) {
              fprintf(stderr, "rw_cmp_buf() failed: Unequal read write buffers for vlba 0x%lX, rc %d\n", start_lba, rc );
              mc_term();
              return -1;
            }
          }
        }
        k++;
      }
      j++;
    }
    i++;
  }

  res = 0;

  //Negative test 3: Close res_hndl from original and duplicated mc_hndl. Expect IO failure.
  if(cmd == 3) {
    //Close res_hndl 0 through duplicate mc_hndl and try IO through original mc_hndl
    myctx.res_hndl = myctx.mc_hndl_p[0].res_hndl_p[0].res_hndl;
    mc_close(myctx.mc_hndl_p[1].mc_hndl, myctx.mc_hndl_p[0].res_hndl_p[0].res_hndl);
    myctx.mc_hndl = myctx.mc_hndl_p[0].mc_hndl;

    debug("Doing IO on freed res_hndl %d\n", myctx.res_hndl);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    //Now try IO through duplicate mc_hndl
    myctx.mc_hndl = myctx.mc_hndl_p[1].mc_hndl;

    debug("Doing IO on freed res_hndl %d\n", myctx.res_hndl);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }


    //Close res_hndl 3 through original mc_hndl and try IO through duplicate mc_hndl
    myctx.res_hndl = myctx.mc_hndl_p[1].res_hndl_p[1].res_hndl;
    mc_close(myctx.mc_hndl_p[0].mc_hndl, myctx.mc_hndl_p[1].res_hndl_p[1].res_hndl);
    myctx.mc_hndl = myctx.mc_hndl_p[1].mc_hndl;

    debug("Doing IO on freed res_hndl %d\n", myctx.res_hndl);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    //Now try IO through original mc_hndl
    debug("Doing IO on freed res_hndl %d\n", myctx.res_hndl);
    rc = send_write(&myctx, start_lba, stride, pid, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_write() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    rc = send_read(&myctx, start_lba, stride, VLBA);
    if(rc == 0) {
      fprintf(stderr, "send_read() on freed res_hndl vdisk should fail, rc %d\n", rc);
      mc_term();
      return -1;
    }

    res = 3;
  }

  pthread_cancel(thread);
  ctx_close(&myctx);
  mc_term();

  return res;
}

/* mc_max_open_tst creates maximum number threads each holding one context
 * handle. Then mc_permute is called per thread to call various mc APIs in
 * order.
 * 
 * Inputs:
 *   NONE
 *
 * Output:
 *   0 Success
 *  -1 Failure
 */

int mc_max_open_tst()
{
  struct ctx *p_ctx;
  struct mc_hndl_s mc_hndl_a[MAX_OPENS];
  struct res_hndl_s res_hndl_a[MAX_OPENS];
  pthread_t *threads_a;

  int i;
  int rc = 0;

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");

  /*Aligning context structure for each thread at 0x1000 multiple address 
    so that each of the thread can work on their own portions of memory 
    regions without overlapping.
  */
  rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*MAX_OPENS);
  if(rc != 0) {
    fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
    return -1;
  }

  threads_a = (pthread_t *)malloc(sizeof(pthread_t) * MAX_OPENS);

  //Creating MAX_OPEN threads and call ctx_init
  for(i = 0; i < MAX_OPENS; i++) {
    rc = pthread_create(&(threads_a[i]), NULL, (void *)&ctx_init_thread, (void *)&p_ctx[i]);
    if(rc) {
      fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
      return -1;
    }
  }

  //joining
  for(i = 1; i < MAX_OPENS; i++) {
    pthread_join(threads_a[i], NULL);
  }

  //Creating MAX_OPEN threads and call mc_permute.
  for(i = 0; i < MAX_OPENS; i++) {
    p_ctx[i].mc_hndl_p = &mc_hndl_a[i];
    p_ctx[i].mc_hndl_p->res_hndl_p = &res_hndl_a[i];
    pthread_create(&(threads_a[i]), NULL, (void *)&mc_permute, (void *)&p_ctx[i]);
    if(rc) {
      fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
      return -1;
    }
  }

  //joining again
  for(i = 0; i < MAX_OPENS; i++) {
    pthread_join(threads_a[i], NULL);
  }

  //Closing all opened contexts.
  for(i = 0; i < MAX_OPENS; i++) {
    rc = pthread_create(&(threads_a[i]), NULL, (void *)&ctx_close_thread, (void *)&p_ctx[i]);
    if(rc) {
      fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
      return -1;
    }
  }

  for(i = 0; i < MAX_OPENS; i++) {
    pthread_join(threads_a[i], NULL);
  }
	rc = g_error;
	g_error = 0;

  free(p_ctx);
  free(threads_a);
  
  mc_term();

  return rc;
}

/* mc_open_close_tst initializes all possible contexts and then closes a random
 * number of contexts. It then reopens same number of contexts and checks if any
 * duplicate context Id is not generated.
 * 
 * Inputs:
 *   NONE
 *
 * Output:
 *   0 Success
 *  -1 Failure
 */

int mc_open_close_tst()
{
  struct ctx *ctx_a;
  bool  ctx_hndl_a[MAX_OPENS+1]; //Keep track of which context is open/close.
  int ctx_idx_a[MAX_OPENS+1]; //Stores indexes of ctx_a.
  bool not_stress = true;
  int loop = 5;
  int i = 0;
  int rc = 0;
  int j=1;

	char *str = getenv("LONG_RUN");
    if((str != NULL) && !strcmp(str, "TRUE")) {
		not_stress = false;
    }

  if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init() failed.\n");
    return -1;
  }

  debug("mc_init() success.\n");
 
  rc = posix_memalign((void **)&ctx_a, 0x1000, sizeof(struct ctx)*MAX_OPENS+1);
  if(rc != 0) {
    fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
    return -1;
  }

  
  memset(ctx_idx_a, 0, sizeof(ctx_idx_a));
  memset(ctx_hndl_a, 0, sizeof(ctx_hndl_a));

	if(not_stress) {
		while(--loop >= 0) {
			for(i = 0; i < MAX_OPENS/2; i++) {
				rc = ctx_init(&ctx_a[i]);
				CHECK_RC(rc , "ctx_init");
			}
			for(i = 0; i < MAX_OPENS/2; i++) {
				ctx_close(&ctx_a[i]);
				CHECK_RC(rc, "ctx_close");
			}
		}
		free(ctx_a);
		return 0;
	}
  
  while(j <= MAX_OPENS) {
    ctx_init(&ctx_a[j]);
    if(ctx_hndl_a[ctx_a[j].ctx_hndl]) {
      fprintf(stderr, "Duplicate context Id generated %d \n", ctx_a[j].ctx_hndl);
      mc_term();
      return -1;
    }

    ctx_hndl_a[ctx_a[j].ctx_hndl] = true;
    debug("context generated %d\n", ctx_a[j].ctx_hndl);
    j++;
  }

  //Number of contexts to be closed.
  srand(time(NULL));
  int num_ctx = rand()%MAX_OPENS+1;
  
  debug("Will close %d contexts now.\n", num_ctx);
 
  j=0;
  while(j < num_ctx) {
    i = rand()%MAX_OPENS+1; //random index of the context to be closed.
    //Preventing multiple generation of same number.
    while(!ctx_hndl_a[ctx_a[i].ctx_hndl]) {
      i = rand()%MAX_OPENS+1;
    }

    ctx_idx_a[j] = i;
    int ctx_hndl_temp = ctx_a[i].ctx_hndl; 
    ctx_close(&ctx_a[i]);
    ctx_hndl_a[ctx_hndl_temp] = false;
    debug("context closed %d\n", ctx_hndl_temp);

    j++;
  }

  j = 0;
  while(j < num_ctx) {
    i = ctx_idx_a[j];
    ctx_init(&ctx_a[i]);
    if(ctx_hndl_a[ctx_a[i].ctx_hndl]) {
      fprintf(stderr, "Duplicate context generated %d\n",ctx_a[i].ctx_hndl);
      mc_term();
      return -1;
    }

    ctx_hndl_a[ctx_a[i].ctx_hndl] = true;
    debug("Context re-generated %d\n", ctx_a[i].ctx_hndl);
    j++;
  }

  j=1;
  //free everything.
  while(j <= MAX_OPENS) {
    int ctx_hndl_temp = ctx_a[j].ctx_hndl; 
    ctx_close(&ctx_a[j]);
    ctx_hndl_a[ctx_hndl_temp] = false;
    debug("context closed %d\n", ctx_hndl_temp);

    j++;
  }

	free(ctx_a);
  mc_term();
  return 0;
}
