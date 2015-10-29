/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/mc.c $                                               */
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
#include <sislite.h>
#include <afu.h> 

/* 
 * Basic program to send IO to the AFU in physical LBA mode.
 * Uses libafu.so so that it can be run on HW or in sim.
 * Sim provides its own libafu.so.
 */

#define USE_INTR 1

#ifdef SIM
#include "sim_pthread.h"
#else
#include <pthread.h>
#endif

#define MIN(a,b) ((a)<(b) ? (a) : (b))

#ifdef SIM
#define NUM_CMDS 8
#else
#define NUM_CMDS 1024
#endif

#define B_DONE   0x01
#define B_ERROR  0x02

// in libafu, the mapped address is to the base of the MMIO space,
// MY_OFFSET gets to the 64K space for my_ctxt. 
// Offset is in 4byte words.
//
#define MY_OFFSET (0x4000*(my_ctxt))
#define MY_CTRL (0x4000*512 + 0x20*(my_ctxt))

#ifdef TARGET_ARCH_PPC64EL
static inline void write_64(volatile __u64 *addr, __u64 val)
{
    __u64 zero = 0; 
    asm volatile ( "stdbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
}

static inline void write_32(volatile __u32 *addr, __u32 val)
{
    __u32 zero = 0;
    asm volatile ( "stwbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
}
#endif

struct ctx {
  __u64 *p_hrrq_start;
  __u64 *p_hrrq_end;
  volatile __u64 *p_hrrq_curr;
  unsigned int toggle;
  int fd;
};

struct data_buf {
  char buf[0x1000];
} __attribute__ ((aligned (0x1000)));


struct hrrq {
  __u64 entry[NUM_CMDS];
};

sisl_iocmd_t      cmd[NUM_CMDS];
struct data_buf   data[NUM_CMDS];
struct hrrq       rrq;
pthread_mutex_t   mutex;
pthread_cond_t    cv;
struct ctx        ctx;
struct afu        *p_afu;
int               recv_fn_kill = 0;
__u16             my_ctxt;

void send_cmd(sisl_iocmd_t *p_cmd);
void wait_resp(sisl_iocmd_t *p_cmd);


void *recv_fn(void *p_arg) {
  struct cxl_event event;

  struct ctx *p_ctx = (struct ctx *) p_arg;
  sisl_iocmd_t *p_cmd;

  // init 
  p_ctx->p_hrrq_start = &rrq.entry[0];
  p_ctx->p_hrrq_end = &rrq.entry[NUM_CMDS-1];
  p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
  p_ctx->toggle = 1;
  p_ctx->fd = p_afu->fd;

  printf("startng receive function\n"); fflush(stdout);
  while (recv_fn_kill == 0) {
    // read fd here & block OR just poll
#ifdef SIM
    sched_yield();
#elif (USE_INTR)
    read(p_ctx->fd, &event, sizeof(event));
    if (event.header.type ==CXL_EVENT_AFU_INTERRUPT) {
      printf("recd afu_intr %d\n", event.irq.irq); fflush(stdout);
    }
#else
    sleep(1);
#endif

    while ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
	   p_ctx->toggle) {
      p_cmd = (sisl_iocmd_t*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));
#ifdef SIM
      printf("got response for ea=%p\n",p_cmd); fflush(stdout);
#endif
      pthread_mutex_lock(&mutex);
      p_cmd->sa.host_use[0] |= B_DONE;
      pthread_cond_signal(&cv);
      pthread_mutex_unlock(&mutex);
      
      if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end) {
	/* advance to next RRQ entry */
	p_ctx->p_hrrq_curr++;
      }
      else { /* wrap HRRQ  & flip toggle */
	p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
	p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
      }
    }
  }
  return NULL;
}

// usage: ./a.out num_cmds_active num_reps
//
int main(int argc, char **argv)
{
  int i;
  pthread_t thread;
  pthread_mutexattr_t mattr;
  pthread_condattr_t cattr;
  int num_cmds = 1;
  unsigned int num_reps = -1u;
  __u64 mbox;
  __u64 *p_u64;
  __u32 *p_u32;

  printf("sizeof ioarcb=%zu ioasa=%zu cmd=%zu rrq=%zu start_work=%zu\n",
	 sizeof(sisl_ioarcb_t),
	 sizeof(sisl_ioasa_t),
	 sizeof(sisl_iocmd_t),
	 sizeof(rrq),
	 sizeof(struct cxl_ioctl_start_work));

  fflush(stdout);

  if (argc > 1) num_cmds = MIN(NUM_CMDS, atoi(argv[1]));
  if (argc > 2) num_reps = atoi(argv[2]);

  pthread_mutexattr_init(&mattr);
  pthread_condattr_init(&cattr);

  pthread_mutex_init(&mutex, &mattr);
  pthread_cond_init(&cv, &cattr);

  // afu_map opens Corsa adapter, atatches the process and maps the
  // base of the MMIO space.
  if ((p_afu = afu_map()) == NULL) {
    printf("Cannot open AFU\n");
    exit(1);
  }

  my_ctxt =  p_afu->process_element;
  printf("Opened AFU.  PE = %d\n",my_ctxt); fflush(stdout);

  // set up RRQ
  // offset is in 4-byte words.
  afu_mmio_write_dw(p_afu, MY_OFFSET+10, (__u64)&rrq.entry[0]); // start_ea
  afu_mmio_write_dw(p_afu, MY_OFFSET+12, (__u64)&rrq.entry[NUM_CMDS-1]); // end_ea

  afu_mmio_read_dw (p_afu, MY_CTRL+6, &mbox); // mbox_r
  asm volatile ( "eieio" : : );
  afu_mmio_write_dw (p_afu, MY_CTRL+4, // ctx_cap
		     SISL_CTX_CAP_REAL_MODE | SISL_CTX_CAP_HOST_XLATE |
		     SISL_CTX_CAP_WRITE_CMD | SISL_CTX_CAP_READ_CMD);


  for (i = 0; i < NUM_CMDS; i++) {
    cmd[i].rcb.data_ea = (__u64) &data[i];
    cmd[i].rcb.data_len = 0x1000;
    cmd[i].rcb.msi = 0x2; // 0x0 for no interrupts
    cmd[i].rcb.rrq = 0x0;
    cmd[i].rcb.ctx_id = my_ctxt; 
    cmd[i].rcb.lun_id = 0x1000000000000ull;
    cmd[i].rcb.port_sel = 0x3; // either FC port
    cmd[i].rcb.cdb[0] = 0x88; // read(16)

    p_u64 = (__u64*)&cmd[i].rcb.cdb[2];
    p_u32 = (__u32*)&cmd[i].rcb.cdb[10];

#ifdef TARGET_ARCH_PPC64EL
    write_64(p_u64, i*8);
    write_32(p_u32, 8);
#else
   *p_u64 = i*8; // LBA#
   *p_u32 = 8;   // blksz=512 & 8 LBAs for 4K
#endif
  }

  recv_fn_kill = 0;
  pthread_create(&thread, NULL, recv_fn, &ctx);

#ifdef SIM
  sched_yield();
#endif

  while (num_reps) {
    for (i = 0; i < num_cmds; i++) {
      send_cmd(&cmd[i]);
    }
    printf("Sent %d cmds\n", num_cmds);
    fflush(stdout);

    for (i = 0; i < num_cmds; i++) {
      wait_resp(&cmd[i]);
      printf("cmd %d: flags=0x%x, port=%d, afu_rc=0x%x, scsi_rc=0x%x, fc_rc=0x%x\n",
	     i, cmd[i].sa.rc.flags, cmd[i].sa.port, cmd[i].sa.rc.afu_rc, 
	     cmd[i].sa.rc.scsi_rc, cmd[i].sa.rc.fc_rc); fflush(stdout);
    }
    printf("Recd %d cmds\n", num_cmds);
    fflush(stdout);
    num_reps--;
  }

  recv_fn_kill = 1; // this may or maynot terminate the recv thread
  pthread_join(thread, NULL);

  // ask the afu to terminate
  // afu_mmio_write_dw(p_afu,MY_OFFSET+4,0);
  //
  afu_unmap(p_afu);

  pthread_cond_destroy(&cv);
  pthread_mutex_destroy(&mutex);

  pthread_condattr_destroy(&cattr);
  pthread_mutexattr_destroy(&mattr);
  
  return 0;
}



void send_cmd(sisl_iocmd_t *p_cmd) {
  pthread_mutex_lock(&mutex);  
  p_cmd->sa.host_use[0] = 0; // 0 means active
  p_cmd->sa.ioasc = 0;

  // make RCB visible to AFU before MMIO
  asm volatile ( "lwsync" : : ); 

  // write IOARRIN
  afu_mmio_write_dw(p_afu, MY_OFFSET+8, (__u64) p_cmd); // IOARRIN
  printf("Sent EA cmd 0x%p data 0x%" PRIx64 "\n", p_cmd, p_cmd->rcb.data_ea);
  fflush(stdout);

  pthread_mutex_unlock(&mutex);
}

void wait_resp(sisl_iocmd_t *p_cmd) {
  pthread_mutex_lock(&mutex);
  while (p_cmd->sa.host_use[0] != B_DONE) {
    pthread_cond_wait(&cv, &mutex);
  }
  pthread_mutex_unlock(&mutex);
}



