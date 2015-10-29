/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/mclient_test.c $                                   */
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

#include <mclient.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <cxl.h>


#define B_DONE   0x01
#define B_ERROR  0x02
#define NUM_RRQ_ENTRY 128
#define NUM_CMDS 63     /* max is NUM_RRQ_ENTRY
			 * must be <= 64 due to the select_mask
			 * must be (NUM_CMDS*n == nchunk), where n is 1, 2, ... etc
			 */

#define CL_SIZE             128             /* Processor cache line size */
#define CL_SIZE_MASK        0x7F            /* Cache line size mask */
#define DATA_SEED           0xdead000000000000ull

#define RETRY_CNT         5 

// use mmap for rcb+data. munmap/remap in loops
// hopefully that will cause seg/page faults.
//
struct ctx {
    /* Stuff requiring alignment go first. */
    
    /* Command & data for AFU commands issued by test. */
    char rbuf[NUM_CMDS][0x1000];    // 4K read data buffer (page aligned)
    char wbuf[NUM_CMDS][0x1000];    // 4K write data buffer (page aligned)
    __u64 rrq_entry[NUM_RRQ_ENTRY]; // 128B RRQ (page aligned)

    struct afu_cmd {
	sisl_ioarcb_t rcb;  // IOARCB (cache line aligned)
	sisl_ioasa_t sa;    // IOASA follows RCB
	pthread_mutex_t mutex; 
	pthread_cond_t cv;

	__u8 cl_pad[CL_SIZE - 
		    ((sizeof(sisl_ioarcb_t) +
		      sizeof(sisl_ioasa_t) +
		      sizeof(pthread_mutex_t) +
		      sizeof(pthread_cond_t)) & CL_SIZE_MASK)];
    } cmd[NUM_CMDS];

    // AFU interface
    int afu_fd; 
    struct cxl_ioctl_start_work work;
    char event_buf[0x1000];  /* Linux cxl event buffer (interrupts) */
    volatile struct sisl_host_map *p_host_map;
    ctx_hndl_t ctx_hndl; 

    __u64 *p_hrrq_start;
    __u64 *p_hrrq_end;
    volatile __u64 *p_hrrq_curr;
    unsigned int toggle;
    unsigned int flags;
#define CTX_STOP  0x1

    // MC client interface
    mc_hndl_t mc_hndl;
    res_hndl_t res_hndl;
} __attribute__ ((aligned (0x1000)));

enum test_mode {
  TEST_DEFAULT = 0,
  TEST_BALLOC,   /* 1: stress block allocator */
  TEST_BAD_EA    /* 2: send bad EA to AFU */
};

enum test_mode mode = TEST_DEFAULT;    /* test mode  */
char *afu_path;   /* points to argv[] string */
pid_t pid;
int nloops = -1;
int rand_fd = -1;
struct ctx myctx[2]; // myctx[0] -> child, myctx[1] -> parent
#define PARENT_INDX   1
#define CHILD_INDX    0


void manual_mode();
void *ctx_intr_rx(void *arg);
int ctx_init(struct ctx *p_ctx, char *dev_path);
void send_write(struct ctx *p_ctx, __u64 start_lba, __u64 stride, int just_send);
void send_read(struct ctx *p_ctx, __u64 start_lba, __u64 stride, int just_send);
void rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba);
void rw_cmp_buf_cloned(struct ctx *p_ctx, __u64 start_lba);
void send_cmd(struct ctx *p_ctx, __u64 sel_mask);
void wait_resp(struct ctx *p_ctx, __u64 sel_mask);
__u64 check_status(struct ctx *p_ctx);
void fill_buf(__u64* p_buf, unsigned int len);
int cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len);
int cmp_buf_cloned(__u64* p_buf, unsigned int len);
__u64 gen_rand();
void balloc_test(char *afu_path, char *afu_path_m);
void bad_ea_test(char *afu_path, char *afu_path_m);
void halt_test();

void
usage(char *prog)
{
    printf("Usage: %s [-m mode] [-n nloops] afu_dev_path\n", prog);
    printf("e. g.: %s -m 2 -n 10 /dev/cxl/afu0.0s\n", prog);
}

void
get_parameters(int argc, char** argv)
{
  extern int   optind;         /* for getopt function */
  extern char  *optarg;        /* for getopt function */
  int ch;

  while ((ch = getopt(argc,argv,"m:n:h")) != EOF) {
    switch (ch) {
    case 'm':       /* decimal */
        mode = atoi(optarg);
        break;
    case 'n':       /* decimal: -1 is forever */
        nloops = atoi(optarg);
        break;
    case 'h':
        usage(argv[0]);
        exit(0);
    default:
        usage(argv[0]);
        exit(-1);
    }
  }

  if ((argc - optind) != 1) { /* number of afus specified in cmd line */
      usage(argv[0]);
      exit(-1);
  }

  afu_path = argv[optind];
}


void ctx_rrq_intr(struct ctx *p_ctx) {
    struct afu_cmd *p_cmd;

    // process however many RRQ entries that are ready
    while ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
	   p_ctx->toggle) {
	p_cmd = (struct afu_cmd*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));

	pthread_mutex_lock(&p_cmd->mutex);
	p_cmd->sa.host_use_b[0] |= B_DONE;
	pthread_cond_signal(&p_cmd->cv);
	pthread_mutex_unlock(&p_cmd->mutex);
      
	if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end) {
	    p_ctx->p_hrrq_curr++; /* advance to next RRQ entry */
	}
	else { /* wrap HRRQ  & flip toggle */
	    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
	    p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
	}
    }
}

void ctx_sync_intr(struct ctx *p_ctx) {
    __u64 reg;
    __u64 reg_unmasked;

    reg = read_64(&p_ctx->p_host_map->intr_status);
    reg_unmasked = (reg & SISL_ISTATUS_UNMASK);

    if (reg_unmasked == 0) {
	fprintf(stderr,
		"%d: spurious interrupt, intr_status 0x%016lx, ctx %d\n", 
		pid, reg, p_ctx->ctx_hndl);
	return;
    }

    if (reg_unmasked == SISL_ISTATUS_PERM_ERR_RCB_READ &&
	(p_ctx->flags & CTX_STOP)) {
	// ok - this is a signal to stop this thread
    }
    else if (mode == TEST_BAD_EA) {
	fprintf(stderr,
		"%d: intr_status 0x%016lx, ctx %d\n", 
		pid, reg, p_ctx->ctx_hndl);
    }
    else {
	fprintf(stderr,
		"%d: unexpected interrupt, intr_status 0x%016lx, ctx %d, halting test...\n", 
		pid, reg, p_ctx->ctx_hndl);
	halt_test();
    }

    write_64(&p_ctx->p_host_map->intr_clear, reg_unmasked);

    return;
}

void *ctx_intr_rx(void *arg) {
  struct cxl_event *p_event;
  int len;
  struct ctx   *p_ctx = (struct ctx*) arg;

  while (!(p_ctx->flags & CTX_STOP)) {
      //
      // read afu fd & block on any interrupt
      len = read(p_ctx->afu_fd, &p_ctx->event_buf[0], 
		 sizeof(p_ctx->event_buf));

      if (len < 0) {
	  fprintf(stderr, "afu has been reset, exiting...\n");
	  exit(-1);
      }

      p_event = (struct cxl_event *)&p_ctx->event_buf[0];
      while (len >= sizeof(p_event->header)) {
	  if (p_event->header.type == CXL_EVENT_AFU_INTERRUPT) {
	      switch(p_event->irq.irq) {
	      case SISL_MSI_RRQ_UPDATED:
		  ctx_rrq_intr(p_ctx);
		  break;

	      case SISL_MSI_SYNC_ERROR:
		  ctx_sync_intr(p_ctx);
		  break;

	      default:
		  fprintf(stderr, "%d: unexpected irq %d, ctx %d, halting test...\n",
			  pid, p_event->irq.irq, p_ctx->ctx_hndl);
		  halt_test();
		  break;
	      }

	  }
	  else if (p_event->header.type == CXL_EVENT_DATA_STORAGE &&
		   (p_ctx->flags & CTX_STOP)) {
	      // this is a signal to terminate this thread
	  }
	  else if (p_event->header.type == CXL_EVENT_DATA_STORAGE &&
		   (mode == TEST_BAD_EA)) {
	      // this is expected, but no need to print since we print
	      // the sync_intr status that accompanies the DSI
	  }
	  else {
	      fprintf(stderr, "%d: unexpected event %d, ctx %d, halting test...\n",
		      pid, p_event->header.type, p_ctx->ctx_hndl);
	      halt_test();
	  }

	  len -= p_event->header.size;
	  p_event = (struct cxl_event *)
	      (((char*)p_event) + p_event->header.size);
      }
  }

  return NULL;
}


int ctx_init(struct ctx *p_ctx, char *dev_path) 
{
    void *map;
    __u32 proc_elem;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    int i;

    // general init, no resources allocated

    pthread_mutexattr_init(&mattr); 
    pthread_condattr_init(&cattr);

    // must clear RRQ memory for reinit in child
    memset(&p_ctx->rrq_entry[0], 0, sizeof(p_ctx->rrq_entry));
    p_ctx->flags = 0;

    for (i = 0; i < NUM_CMDS; i++) {
	pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
	pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
    }

    // open non-master device
    p_ctx->afu_fd = open(dev_path, O_RDWR);
    if (p_ctx->afu_fd < 0) {
	fprintf(stderr, "open failed: device %s, errno %d\n", dev_path, errno);
	return -1;
    }

    // enable the AFU. This must be done before mmap.
    p_ctx->work.num_interrupts = 4;
    p_ctx->work.flags = CXL_START_WORK_NUM_IRQS;
    if (ioctl(p_ctx->afu_fd, CXL_IOCTL_START_WORK, &p_ctx->work) != 0) {
	close(p_ctx->afu_fd);
	fprintf(stderr, "start command failed on AFU, errno %d\n", errno);
	return -1;
    }
    if (ioctl(p_ctx->afu_fd, CXL_IOCTL_GET_PROCESS_ELEMENT, 
	      &proc_elem) != 0) {
	fprintf(stderr, "get_process_element failed, errno %d\n", errno);
	return -1;
    }

    // mmap host transport MMIO space of this context 
    // the map must be accessible by forked child for clone access
    // checks.
    // so do not madvise(map, 0x10000, MADV_DONTFORK);
    //
    map = mmap(NULL, 0x10000,  // 64KB 
	       PROT_READ|PROT_WRITE, MAP_SHARED, p_ctx->afu_fd, 0);
    if (map == MAP_FAILED) {
	fprintf(stderr, "mmap failed, errno %d\n", errno);
	close(p_ctx->afu_fd);
	return -1;
    }

    // copy frequently used fields into p_ctx
    p_ctx->ctx_hndl = proc_elem; // ctx_hndl is 16 bits in CAIA
    p_ctx->p_host_map = (volatile struct sisl_host_map *) map;

    // initialize RRQ pointers
    p_ctx->p_hrrq_start = &p_ctx->rrq_entry[0];
    p_ctx->p_hrrq_end = &p_ctx->rrq_entry[NUM_RRQ_ENTRY - 1];
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
    p_ctx->toggle = 1;

    printf("p_host_map %p, ctx_hndl %d, rrq_start %p\n", 
	    p_ctx->p_host_map, p_ctx->ctx_hndl, p_ctx->p_hrrq_start);

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++) {
	p_ctx->cmd[i].rcb.msi = SISL_MSI_RRQ_UPDATED;
	p_ctx->cmd[i].rcb.rrq = 0x0;
	p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }

    // set up RRQ in AFU
    write_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
    write_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);

    // set LISN#, it is always sent to the context that wrote IOARRIN
    write_64(&p_ctx->p_host_map->ctx_ctrl, SISL_MSI_SYNC_ERROR);
    write_64(&p_ctx->p_host_map->intr_mask, SISL_ISTATUS_MASK);

    return 0;
}

void ctx_close(struct ctx *p_ctx, int stop_rrq_thread) 
{
    __u64 room = 0;

    if (stop_rrq_thread) {
	p_ctx->flags |= CTX_STOP;
	asm volatile ( "lwsync" : : ); // make flags visible & 
	                               // let any IOARRIN writes complete
	do {
	    room = read_64(&p_ctx->p_host_map->cmd_room);
	} while (room == 0);

	// this MMIO will send 2 interrupts (a DSI and a AFU sync error)
	// and wake up the rrq thread if it is blocked on a read. 
	// After it unblocks, the thread will terminate.
	write_64(&p_ctx->p_host_map->ioarrin, 0xdeadbeef);
	sleep(1);
    }

    munmap((void*)p_ctx->p_host_map, 0x10000);
    close(p_ctx->afu_fd); // afu_fd is closed and the ctx freed only 
                          // if there is no read pending on the afu_fd.
                          // so, the rrq thread must not be blocked on
                          // a read at this point, else close will fail.
}

// writes using virtual LBAs
void send_write(struct ctx *p_ctx, __u64 start_lba, __u64 stride, int just_send) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba;
    __u64 sel_mask;

    for (i = 0; i < NUM_CMDS; i++) {
	fill_buf((__u64*)&p_ctx->wbuf[i][0], 
		 sizeof(p_ctx->wbuf[i])/sizeof(__u64));

	p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_RES_HNDL |
				       SISL_REQ_FLAGS_HOST_WRITE);
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[i]);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	vlba = start_lba + i*stride;

	write_64(p_u64, vlba); // virtual LBA# 
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use_b[1] = 0;    // reset retry cnt
    }

    sel_mask = ((NUM_CMDS == 64) ? 
		(-1ull) : (1ull << NUM_CMDS) - 1); // select each command

    if (just_send) {
	send_cmd(p_ctx, sel_mask);
	return;
    }

    do {
	send_cmd(p_ctx, sel_mask);
	wait_resp(p_ctx, sel_mask);
    } while ((sel_mask = check_status(p_ctx)) != 0);

    for (i = 0; i < NUM_CMDS; i++) {
	if (p_ctx->cmd[i].sa.host_use_b[0] & B_ERROR) { 
	    fprintf(stderr, "%d: write failed, ctx %d, halting test...\n", pid,
		    p_ctx->ctx_hndl);
	    halt_test();
	}
    }
}

// reads using virtual LBA
void send_read(struct ctx *p_ctx, __u64 start_lba, __u64 stride, int just_send) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba;
    __u64 sel_mask;

    for (i = 0; i < NUM_CMDS; i++) {
	memset(&p_ctx->rbuf[i][0], 0xB, sizeof(p_ctx->rbuf[i]));

	p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_RES_HNDL |
				       SISL_REQ_FLAGS_HOST_READ);
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbuf[i]);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbuf[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	vlba = start_lba + i*stride;

	write_64(p_u64, vlba); // virtual LBA#
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use_b[1] = 0;    // reset retry cnt
    }

    sel_mask = ((NUM_CMDS == 64) ? 
		(-1ull) : (1ull << NUM_CMDS) - 1); // select each command

    if (just_send) {
	send_cmd(p_ctx, sel_mask);
	return;
    }

    do {
	send_cmd(p_ctx, sel_mask);
	wait_resp(p_ctx, sel_mask);
    } while ((sel_mask = check_status(p_ctx)) != 0);

    for (i = 0; i < NUM_CMDS; i++) {
	if (p_ctx->cmd[i].sa.host_use_b[0] & B_ERROR) { 
	    fprintf(stderr, "%d:read failed, ctx %d, halting test...\n", pid,
		    p_ctx->ctx_hndl);
	    halt_test();
	}
    }
}

void rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba) {
    int i;
    char buf[32];
    int read_fd, write_fd;
    for (i = 0; i < NUM_CMDS; i++) {
	if (cmp_buf((__u64*)&p_ctx->rbuf[i][0], (__u64*)&p_ctx->wbuf[i][0], 
		    sizeof(p_ctx->rbuf[i])/sizeof(__u64))) {
	    sprintf(buf, "read.%d", pid);
	    read_fd = open(buf, O_RDWR|O_CREAT);
	    sprintf(buf, "write.%d", pid);
	    write_fd = open(buf, O_RDWR|O_CREAT);

	    write(read_fd, &p_ctx->rbuf[i][0], sizeof(p_ctx->rbuf[i]));
	    write(write_fd, &p_ctx->wbuf[i][0], sizeof(p_ctx->wbuf[i]));

	    close(read_fd);
	    close(write_fd);

	    fprintf(stderr, 
		    "%d: miscompare at start_lba 0x%lx, ctx %d, halting test...\n", 
		    pid, start_lba, p_ctx->ctx_hndl);
	    halt_test();
	}
    }
}

void rw_cmp_buf_cloned(struct ctx *p_ctx, __u64 start_lba) {
    int i;

    for (i = 0; i < NUM_CMDS; i++) {
	if (cmp_buf_cloned((__u64*)&p_ctx->rbuf[i][0],
		    sizeof(p_ctx->rbuf[i])/sizeof(__u64))) {
	    fprintf(stderr, 
		    "%d: clone miscompare at start_lba 0x%lx, ctx %d, halting test...\n", 
		    pid, start_lba, p_ctx->ctx_hndl);
	    halt_test();
	}
    }
}


void send_cmd(struct ctx *p_ctx, __u64 sel_mask) {
    int i;
    __u64 room = 0;

    for (i = 0; i < NUM_CMDS; i++) {
	if (sel_mask & (1ull << i)) {
	    p_ctx->cmd[i].sa.host_use_b[0] = 0; // 0 means active
	    p_ctx->cmd[i].sa.ioasc = 0; // clear
	}
    }

    /* make memory updates visible to AFU before MMIO */
    asm volatile ( "lwsync" : : ); 

    for (i = 0; i < NUM_CMDS; i++) {
	if (sel_mask & (1ull << i)) {
	    if (room == 0) {
		asm volatile ( "eieio" : : ); // let IOARRIN writes complete
		do {
		    room = read_64(&p_ctx->p_host_map->cmd_room);
		} while (room == 0);
	    }

	    // write IOARRIN
	    write_64(&p_ctx->p_host_map->ioarrin,
		     (__u64)&p_ctx->cmd[i].rcb);
	    room--;
	}
    }

}

void wait_resp(struct ctx *p_ctx, __u64 sel_mask) {
    int i;

    for (i = 0; i < NUM_CMDS; i++) {
	if (sel_mask & (1ull << i)) {
	    pthread_mutex_lock(&p_ctx->cmd[i].mutex);
	    while (p_ctx->cmd[i].sa.host_use_b[0] != B_DONE) {
		pthread_cond_wait(&p_ctx->cmd[i].cv, &p_ctx->cmd[i].mutex);
	    }
	    pthread_mutex_unlock(&p_ctx->cmd[i].mutex);

	    if (p_ctx->cmd[i].sa.ioasc) {	
		fprintf(stderr,
			"%d:CMD 0x%x failed, IOASC = flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n", 
			pid,
			p_ctx->cmd[i].rcb.cdb[0],
			p_ctx->cmd[i].sa.rc.flags,
			p_ctx->cmd[i].sa.rc.afu_rc, 
			p_ctx->cmd[i].sa.rc.scsi_rc, 
			p_ctx->cmd[i].sa.rc.fc_rc);
	    }
	}
    }
}


// returns a select mask of cmd indicies to retry 
// 0 means all indices completed
__u64 check_status(struct ctx *p_ctx) 
{
    int i;
    __u64 ret = 0;

    // check_status does not take a sel_mask and examines each
    // command every time. If the cmd was successful, ioasc
    // will remain 0.
    for (i = 0; i < NUM_CMDS; i++) {

	if (p_ctx->cmd[i].sa.ioasc == 0) {
	    continue;
	}

	p_ctx->cmd[i].sa.host_use_b[0] |= B_ERROR;


	if (!(p_ctx->cmd[i].sa.host_use_b[1]++ < RETRY_CNT)) {
	    continue;
	}

	switch (p_ctx->cmd[i].sa.rc.afu_rc)
	{
	/* 1. let afu choose another FC port or retry tmp buf full */
	case SISL_AFU_RC_NO_CHANNELS:
	case SISL_AFU_RC_OUT_OF_DATA_BUFS:
	    usleep(100); // 100 microsec
	    ret |= (1ull << i);
	    break;
	    

	/* 2. check for master exit, if so user must restart */
	case SISL_AFU_RC_RHT_DMA_ERR:
	case SISL_AFU_RC_LXT_DMA_ERR:
	    // afu_extra=1 when mserv exited and restarted using same ctx
	    // new mserv (on same ctx) gets CXL_EVENT_DATA_STORAGE events
	    // the above is when mserv did not clear capabiliy on start 
	    // allowing AFU to read RHT set up by exited mserv.
	    //
	    // if did not restart at all or rstarted on another ctx, afu_extra=11
	    //
	    if (p_ctx->cmd[i].sa.afu_extra == SISL_AFU_DMA_ERR_INVALID_EA) {
		fprintf(stderr, "%d: master may have exited, halting test...\n", pid);
		halt_test();
	    }
	    break;
	case SISL_AFU_RC_NOT_XLATE_HOST:
	    fprintf(stderr, "%d: master may have exited, halting test...\n", pid);
	    halt_test();


	/* 3. check for afu reset and/or master restart */
	case SISL_AFU_RC_CAP_VIOLATION:
	    fprintf(stderr, "%d: afu reset or master may have restated, halting test...\n", pid);
	    halt_test();


	/* 4. any other afu_rc is a user error, just fail the cmd */

	/* 5. if no afu_rc, then either scsi_rc and/or fc_rc is set
	 *    retry all scsi_rc and fc_rc after a small delay
	 */
	case 0: 
	    usleep(100); // 100 microsec
	    ret |= (1ull << i);
	    break;
	}
    }


    return ret;
}

void halt_test() {
    while (1);
}

/********************************************************************
 *  NAME        : main
 *
 *
 *  RETURNS     :
 *     0 = success.
 *    !0 = failure.
 *
 *********************************************************************/
int
main(int argc, char *argv[])
{
    int rc;
    __u64 act_new_size;
    __u64 start_lba;
    char master_dev_path[MC_PATHLEN];
    pthread_t thread;
    pid_t new_pid;
    mc_stat_t stat;

    int parent = 1; // start as parent and fork
    __u64 npass = 0;
    __u64 stride = 0; // stop gcc warnings
    __u64 nlba = 0;   // stop gcc warnings
    unsigned long nchunk = NUM_CMDS*2;
    struct ctx *p_ctx = &myctx[PARENT_INDX];
    mc_hndl_t mc_hndl;

    get_parameters(argc, argv);
    strcpy(master_dev_path, afu_path); 
    master_dev_path[strlen(master_dev_path) - 1] = 0; // drop "s" suffix
    strcat(master_dev_path, "m"); // add master suffix

    rand_fd = open("/dev/urandom", O_RDONLY);
    if (rand_fd < 0) {
	fprintf(stderr, "cannot open random device, errno %d\n", errno);
	exit(-1);
    }

    if (mc_init() != 0) {
	fprintf(stderr, "mc_init failed\n");
	exit(-1);
    }

    if (mode == TEST_BALLOC) {
	balloc_test(afu_path, master_dev_path);
	return 0;
    }
    else if (mode == TEST_BAD_EA) {
	bad_ea_test(afu_path, master_dev_path);
	return 0;
    }

    // TEST_DEFAULT mode
    memset(p_ctx, 0, sizeof(*p_ctx));

reinit:
    printf("instantiating ctx on %s...\n", afu_path);
    rc = ctx_init(p_ctx, afu_path);
    if (rc != 0) {
	fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	exit(-1);
    }
    pthread_create(&thread, NULL, ctx_intr_rx, p_ctx);

    rc = mc_register(master_dev_path, p_ctx->ctx_hndl, 
		     (volatile __u64 *) p_ctx->p_host_map, 
		     &mc_hndl);
    if (rc != 0) {
	fprintf(stderr, "error registering ctx_hndl %d, rc %d\n", 
		p_ctx->ctx_hndl, rc);
	exit(-1);
    }

    rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
    if (rc != 0) {
	fprintf(stderr, "error in mc_hdup, rc %d\n", rc);
	exit(-1);
    }

    mc_unregister(mc_hndl); // we will use the p_ctx->mc_hndl

    if (parent) { // mc_open + mc_size
      rc = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
	if (rc != 0) {
	    fprintf(stderr, "error opening res_hndl rc %d\n", rc);
	    exit(-1);
	}

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &stat);
	if (rc != 0) {
	    fprintf(stderr, "error in stat of res_hndl rc %d\n", rc);
	    exit(-1);
	}

	stride = (1 << stat.nmask); // set to chunk size
	                            // or use 8 to test all LBAs


	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
		     nchunk, &act_new_size);
	if (rc != 0 || nchunk != act_new_size) {
	    fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
	    exit(-1);
	}

	nlba = nchunk*(1 << stat.nmask); 

	pid = getpid(); // pid is used to create unique data patterns
	                // now it has parent's pid

	for (start_lba = 0; start_lba < nlba; start_lba += (NUM_CMDS*stride)) {
	    send_write(p_ctx, start_lba, stride, 0);
	}

	// leave data written by parent to be read and compared by
	// child after a clone.

	// In a real application, it is very important that parent & child 
	// coordinate access to cloned data since they are on the same LBAs.
	// This is until AFU implements copy-on-write.
	//
	new_pid = fork();
	if (new_pid) {
	    sleep(30); // let child clone before parent closes the 
	              // resources
	    mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	    mc_unregister(p_ctx->mc_hndl);
	    ctx_close(p_ctx, 1);

	    printf("ppid = %d, cpid = %d\n", pid, new_pid); fflush(stdout);
	    new_pid = wait(&rc);
	    printf("%d terminated, signalled %s, signo %d\n",
		   new_pid, WIFSIGNALED(rc) ? "yes" : "no", WTERMSIG(rc));
	    fflush(stdout);
	}
	else {
	    parent = 0;
	    // copy data bufs and res_hndl to child.
	    // this is an artifact of this test and not something for 
	    // real user app
	    //
	    myctx[CHILD_INDX] = myctx[PARENT_INDX]; 
	    p_ctx = &myctx[CHILD_INDX];  // point to child's

	    goto reinit; // but do not clear p_ctx since we need res_hndl+data
	}
    }
    else { // mc_clone which is equivalent to mc_open + mc_size
	rc = mc_clone(p_ctx->mc_hndl, myctx[PARENT_INDX].mc_hndl, MC_RDWR);
	if (rc != 0) {
	    fprintf(stderr, "error cloning rc %d\n", rc);
	    exit(-1);
	}

	// Once cloned, close the inherited interfaces from parent: 
	// 1) the mc handle, 2) the parent's context.
	//
	// child must unregister any inherited mc handle. 
	// child must not call any API using the inherited handle other
	// than mc_clone & mc_unregister
	//
	rc = mc_unregister(myctx[PARENT_INDX].mc_hndl);

	// now close parent's AFU context
	ctx_close(&myctx[PARENT_INDX], 0);

	// first read what parent wrote
	for (start_lba = 0; start_lba < nlba; start_lba += (NUM_CMDS*stride)) {
	    send_read(p_ctx, start_lba, stride, 0);
	    rw_cmp_buf_cloned(p_ctx, start_lba);
	}
	printf("%d:clone compare success\n", pid); fflush(stdout);

	pid = getpid(); // use child pid from now on for new data patterns
	                // to write, must be after cloned cmp

	// child loops in write, read & compare.
	// Note that parent had closed its handle & context, but that does
	// not affect child's cloned copy.
	//
	while (nloops--) {
	    for (start_lba = 0; start_lba < nlba; start_lba += (NUM_CMDS*stride)) {
		send_write(p_ctx, start_lba, stride, 0);
		send_read(p_ctx, start_lba, stride, 0);
		rw_cmp_buf(p_ctx, start_lba);
	    }

	    if ((npass++ & 0x3F) == 0) {
		printf("%d:completed pass %ld\n", pid, npass>>6); fflush(stdout);
	    }
	}

	// queue a bunch of cmds just before we exit but do not wait for responses
	// must send both reads and writes
	//
	if (gen_rand() & 0x1) {
	    send_write(p_ctx, 0, stride, 1);
	}
	else {
	    send_read(p_ctx, 0, stride, 1);
	}

	if (gen_rand() & 0x2) {
	    exit(0); // get out fast w/o closing ctx etc
	}
	else {
	    // do not mc_unregister by intent. 
	    ctx_close(p_ctx, 1);
	}
    }

    pthread_join(thread, NULL);
    mc_term();

    return 0;
}

// len in __u64
void fill_buf(__u64* p_buf, unsigned int len)
{
  static __u64 data = DATA_SEED;
  int i;

  for (i = 0; i < len; i += 2) {
    p_buf[i] = pid;
    p_buf[i + 1] = data++;
  }
}

// len in __u64
int cmp_buf_cloned(__u64* p_buf, unsigned int len)
{
  static __u64 data = DATA_SEED;
  int i;

  for (i = 0; i < len; i += 2) {
    if (!(p_buf[i] == pid && p_buf[i + 1] == data++)) {
      return -1;
    }
  }
  return 0;
}

// len in __u64
int cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len)
{
  return memcmp(p_buf1, p_buf2, len*sizeof(__u64));
}


__u64 gen_rand() {
    __u64 rand;

    if (read(rand_fd, &rand, sizeof(rand)) != sizeof(rand)) {
	fprintf(stderr, "cannot read random device, errno %d\n", errno);
	exit(-1);
    }
    return rand;
}


void balloc_test(char *afu_path, char *afu_path_m)
{

#define NUM_RH 16

    int rc;
    __u64 act_new_size;
    pthread_t thread_a;
    pthread_t thread_b;
    struct ctx *p_ctx_a = &myctx[0];
    struct ctx *p_ctx_b = &myctx[1];
    res_hndl_t rh[NUM_RH];
    int i;
    unsigned int req_nchunk;

    pid = getpid(); 

    while (nloops--) {
	// init ctx_a
	rc = ctx_init(p_ctx_a, afu_path);
	if (rc != 0) {
	    fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	    exit(-1);
	}
	pthread_create(&thread_a, NULL, ctx_intr_rx, p_ctx_a);
	rc = mc_register(afu_path_m, p_ctx_a->ctx_hndl, 
			 (volatile __u64 *) p_ctx_a->p_host_map, 
			 &p_ctx_a->mc_hndl);
	if (rc != 0) {
	    fprintf(stderr, "error registering ctx_hndl %d, rc %d\n", 
		    p_ctx_a->ctx_hndl, rc);
	    exit(-1);
	}

	// init ctx_b
	rc = ctx_init(p_ctx_b, afu_path);
	if (rc != 0) {
	    fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	    exit(-1);
	}
	pthread_create(&thread_b, NULL, ctx_intr_rx, p_ctx_b);
	rc = mc_register(afu_path_m, p_ctx_b->ctx_hndl, 
			 (volatile __u64 *) p_ctx_b->p_host_map, 
			 &p_ctx_b->mc_hndl);
	if (rc != 0) {
	    fprintf(stderr, "error registering ctx_hndl %d, rc %d\n", 
		    p_ctx_b->ctx_hndl, rc);
	    exit(-1);
	}

	// open & size ctx_a
	for (i = 0; i < NUM_RH; i++) {
	    rc = mc_open(p_ctx_a->mc_hndl, MC_RDWR, &rh[i]);
	    if (rc != 0) {
		fprintf(stderr, "error opening res_hndl rc %d\n", rc);
		exit(-1);
	    }

	    //req_nchunk = 0x71c0/NUM_RH; // fixed & based on lun size
	    req_nchunk = (gen_rand() & 0x7FF);

	    rc = mc_size(p_ctx_a->mc_hndl, rh[i], req_nchunk, &act_new_size);
	    if (rc != 0) {
		fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
		exit(-1);
	    }
	    else if (req_nchunk !=  act_new_size) {
		printf("sized rh %d to 0x%lx chunks (req 0x%x)\n", i, act_new_size,
		       req_nchunk);
	    }
	}

	// clone ctx_a into ctx_b -  ok to fail
	rc = mc_clone(p_ctx_b->mc_hndl, p_ctx_a->mc_hndl, MC_RDWR);

	// close ctx_a
	for (i = 0; i < NUM_RH; i++) {
	    mc_close(p_ctx_a->mc_hndl, rh[i]);
	}
	mc_unregister(p_ctx_a->mc_hndl);
	ctx_close(p_ctx_a, 1);

	// close ctx_b if clone was successful
	if (rc == 0) {
	    for (i = 0; i < NUM_RH; i++) {
		mc_close(p_ctx_b->mc_hndl, rh[i]);
	    }
	}
	mc_unregister(p_ctx_b->mc_hndl);
	ctx_close(p_ctx_b, 1);
    }
}

/*
 * bad RCB EA, bad RRQ EA, bad Data EA
 * queue many bad cmds and exit when nloops exhaust
 */
void bad_ea_test(char *afu_path, char *afu_path_m)
{
    int i;
    int rc;
    __u64 stride;
    __u64 act_new_size;
    __u64 sel_mask;
    __u64 room;
    pthread_t thread;
    mc_stat_t stat;
    unsigned long nchunk = NUM_CMDS*2;
    struct ctx *p_ctx = &myctx[0];


    pid = getpid(); 

    // init ctx
    rc = ctx_init(p_ctx, afu_path);
    if (rc != 0) {
	fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	exit(-1);
    }
    pthread_create(&thread, NULL, ctx_intr_rx, p_ctx);

    rc = mc_register(afu_path_m, p_ctx->ctx_hndl, 
		     (volatile __u64 *) p_ctx->p_host_map, 
		     &p_ctx->mc_hndl);
    if (rc != 0) {
	fprintf(stderr, "error registering ctx_hndl %d, rc %d\n", 
		p_ctx->ctx_hndl, rc);
	exit(-1);
    }

    rc = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
    if (rc != 0) {
	fprintf(stderr, "error opening res_hndl rc %d\n", rc);
	exit(-1);
    }

    rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
		 nchunk, &act_new_size);
    if (rc != 0 || nchunk != act_new_size) {
	fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
	exit(-1);
    }

    rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &stat);
    if (rc != 0) {
	fprintf(stderr, "error in stat of res_hndl rc %d\n", rc);
	exit(-1);
    }

    stride = (1 << stat.nmask); // set to chunk size

    send_write(p_ctx, 0, stride, 0);
    send_read(p_ctx, 0, stride, 0);
    rw_cmp_buf(p_ctx, 0);

    printf("%s: so far so good...\n", __func__);


    // set up bad RRQ in AFU, host pointers do not matter since nothing
    // will be written to RRQ
    write_64(&p_ctx->p_host_map->rrq_start, (__u64) (-1ull & (~7)));
    write_64(&p_ctx->p_host_map->rrq_end, (__u64) 0);

    for (i = 0; i < NUM_CMDS; i++) {
	// odd cmd indicies will report bad data_ea
	// even ones will report bad RRQ
	if (i & 0x1) {
	    p_ctx->cmd[i].rcb.data_ea = (__u64) 0x5555555500000000ull;
	}
    }
    sel_mask = ((NUM_CMDS == 64) ? 
		(-1ull) : (1ull << NUM_CMDS) - 1); // select each command

    while (nloops--) {
	sleep(1);

	asm volatile ( "eieio" : : ); // let any IOARRIN writes complete
	do {
	    room = read_64(&p_ctx->p_host_map->cmd_room);
	} while (room == 0);

	// bad RCB EA
	write_64(&p_ctx->p_host_map->ioarrin, 0xdeadbeef);

	send_cmd(p_ctx, sel_mask);
    }

    // exit w/o ctx_close when send_cmd called last has pending cmds
}


