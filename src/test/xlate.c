/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/xlate.c $                                            */
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

// set this to 1 to run w/libafu or in sim
#define LIBAFU 0

#include <sislite.h>
#include <cxl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define REMAP_FREQ        0xF  /* must be all ones, 
				  higher value is fewer remaps */
#define MC_RHT_NMASK      16                  /* in bits */
#define MC_CHUNK_SIZE     (1 << MC_RHT_NMASK) /* in LBAs, see mclient.h */
#define MC_CHUNK_SHIFT    MC_RHT_NMASK  /* shift to go from LBA to chunk# */
#define MC_CHUNK_OFF_MASK (MC_CHUNK_SIZE - 1) /* apply to LBA get offset */

#if LIBAFU
#include <afu.h> 
/* offset in afu_mmio_write_dw is in words and from the base of the entire
   map */
#define MMIO_WRITE_64(addr, val) \
  afu_mmio_write_dw(p_afu, ((__u64)(addr) - (__u64)p_afu->ps_addr)/4, (val))

#define MMIO_READ_64(addr, p_val) \
  afu_mmio_read_dw(p_afu, ((__u64)(addr) - (__u64)p_afu->ps_addr)/4, (p_val))


/* fuctions to byte reverse SCSI CDB on LE host */
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
#else
#include <mclient.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#define MMIO_WRITE_64(addr, val) write_64((addr), (val))
#define MMIO_READ_64(addr, p_val) *(p_val) = read_64((addr))
#endif


/*
  Stand alone program to test LBA translation. Does not require
  master context/daemon.
 */

#define B_DONE   0x01
#define B_ERROR  0x02
#define NUM_RRQ_ENTRY 64
#define NUM_CMDS 64     /* max is NUM_RRQ_ENTRY */
#define LUN_INDEX     1 /* lun index to use, should be something other than 0
			   used by mserv */

#define CL_SIZE             128             /* Processor cache line size */
#define CL_SIZE_MASK        0x7F            /* Cache line size mask */
#define DATA_SEED           0xdead000000000000ull
#define CHUNK_BASE          0x100 /* chunk# 0x100, i.e. RLBA=0x1000000 */

struct ctx {
    /* Stuff requiring alignment go first. */
    
    /* Command & data for AFU commands issued by test. */
    char rbuf[NUM_CMDS][0x1000];    // 4K read data buffer (page aligned)
    char wbuf[NUM_CMDS][0x1000];    // 4K write data buffer (page aligned)
    char rbufm[NUM_CMDS][0x1000];   // 4K read data buffer (page aligned)
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
    volatile struct sisl_ctrl_map *p_ctrl_map;
    volatile struct surelock_afu_map *p_afu_map;
    ctx_hndl_t ctx_hndl; 

    __u64 *p_hrrq_start;
    __u64 *p_hrrq_end;
    volatile __u64 *p_hrrq_curr;
    unsigned int toggle;

    // LBA xlate
    sisl_rht_entry_t rht;
    sisl_lxt_entry_t lxt[NUM_CMDS]; // each cmd targets 1 chunk
    
} __attribute__ ((aligned (0x1000)));

typedef void (*read_fcn_t)(struct ctx *p_ctx, __u64 start_lba, __u64 stride);

void *ctx_rrq_rx(void *arg);
int  ctx_init(struct ctx *p_ctx, char *dev_path);
void send_writep(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
void send_readp(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
void send_readv(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
void send_readm(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
void rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
void send_cmd(struct ctx *p_ctx);
void wait_resp(struct ctx *p_ctx);
void fill_buf(__u64* p_buf, unsigned int len, __u64 vlba, __u64 plba);
int  cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len);
__u64 gen_rand();
struct ctx* remap();


char *afu_path;   /* points to argv[] string */
pid_t pr_id;
read_fcn_t read_fcn = send_readv; // default is read virtual
__u64 lun_id = 0x0;
__u64 chunk_base = CHUNK_BASE;
int rand_fd;
int ctx_fd;
char ctx_file[32];
struct ctx *p_ctx;

#if LIBAFU
struct afu *p_afu;
#endif

// run multiple instances separated by 0x100 e.g.
// xlate -c 0x100 ...
// xlate -c 0x200 ...
// etc
void
usage(char *prog)
{
    printf("Usage: %s [-c chunk_base] [-l lun_id] [-p] master_dev_path\n", prog);
    printf("e. g.: %s -c 0x100 -l 0x1000000000000 /dev/cxl/afu0.0m\n", prog);
    printf("       -p runs reads & writes in physical LBAs\n");
    printf("       default is write physical, read virtual\n");
}

void
get_parameters(int argc, char** argv)
{
  extern int   optind;         /* for getopt function */
  extern char  *optarg;        /* for getopt function */
  int ch;

  while ((ch = getopt(argc,argv,"pl:c:h")) != EOF) {
    switch (ch) {
    case 'p' :      /* use physical read (default is virtual) */
	read_fcn = send_readp;
	break;

    case 'l' :      /* LUN_ID to use */
	sscanf(optarg, "%lx", &lun_id);
	break;

    case 'c' :      /* chunk_base to use */
	sscanf(optarg, "%lx", &chunk_base);
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
      exit(-11);
  }

  afu_path = argv[optind];
}



void *ctx_rrq_rx(void *arg) {
  struct ctx   *p_ctx = (struct ctx*) arg;
  struct afu_cmd *p_cmd;
  int len;

  while (1) {
      //
      // read afu fd block on any interrupt
      len = read(p_ctx->afu_fd, &p_ctx->event_buf[0], 
		 sizeof(p_ctx->event_buf));

      if (len == -EIO) {
	  fprintf(stderr, "afu has been reset, exiting...\n");
	  exit(-1);
      }

      // process however many RRQ entries that are ready
      // not checking the event type
      while ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
	     p_ctx->toggle) {
	  p_cmd = (struct afu_cmd*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));

	  pthread_mutex_lock(&p_cmd->mutex);
	  p_cmd->sa.host_use[0] |= B_DONE;
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

  return NULL;
}

// dev_path must be master device
// dev_path is not used when running w/libafu - see afu.c 
int ctx_init(struct ctx *p_ctx, char *dev_path) 
{

#if !(LIBAFU)
    void *map;
    __u32 proc_elem;
#endif
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    int i;
    __u64 reg;

    // general init, no resources allocated
    memset(p_ctx, 0, sizeof(*p_ctx));
    pthread_mutexattr_init(&mattr); // check rc on there ?
    pthread_condattr_init(&cattr);

    for (i = 0; i < NUM_CMDS; i++) {
	pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
	pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
    }
#if LIBAFU
    if ((p_afu = afu_map()) == NULL) {
      printf("Cannot open AFU using libafu\n");
      return -1;
    }
    // copy stuff into p_ctx 
    p_ctx->afu_fd = p_afu->fd;
    p_ctx->work = p_afu->work;
    p_ctx->p_afu_map = (volatile struct surelock_afu_map *) p_afu->ps_addr;
    p_ctx->ctx_hndl =  p_afu->process_element;
#else
    // open master device
    p_ctx->afu_fd = open(dev_path, O_RDWR);
    if (p_ctx->afu_fd < 0) {
	fprintf(stderr, "open failed: device %s, errno %d", dev_path, errno);
	return -1;
    }

    // enable the AFU. This must be done before mmap.
    p_ctx->work.num_interrupts = 4;
    p_ctx->work.flags = CXL_START_WORK_NUM_IRQS;
    if (ioctl(p_ctx->afu_fd, CXL_IOCTL_START_WORK, &p_ctx->work) != 0) {
	fprintf(stderr, "start command failed on AFU, errno %d\n", errno);
	return -1;
    }
    if (ioctl(p_ctx->afu_fd, CXL_IOCTL_GET_PROCESS_ELEMENT, 
	      &proc_elem) != 0) {
	fprintf(stderr, "get_process_element failed, errno %d\n", errno);
	return -1;
    }

    // mmap entire MMIO space of this AFU
    map = mmap(NULL, sizeof(struct surelock_afu_map),
	       PROT_READ|PROT_WRITE, MAP_SHARED, p_ctx->afu_fd, 0);
    if (map == MAP_FAILED) {
	fprintf(stderr, "mmap failed, errno %d\n", errno);
	return -1;
    }
    p_ctx->p_afu_map = (volatile struct surelock_afu_map *) map;
    p_ctx->ctx_hndl = proc_elem; // ctx_hndl is 16 bits in CAIA
#endif

    // copy frequently used fields into p_ctx
    p_ctx->p_host_map = &p_ctx->p_afu_map->hosts[p_ctx->ctx_hndl].host;
    p_ctx->p_ctrl_map = &p_ctx->p_afu_map->ctrls[p_ctx->ctx_hndl].ctrl;

    // initialize RRQ pointers
    p_ctx->p_hrrq_start = &p_ctx->rrq_entry[0];
    p_ctx->p_hrrq_end = &p_ctx->rrq_entry[NUM_RRQ_ENTRY - 1];
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
    p_ctx->toggle = 1;

    printf("p_host_map %p, ctx_hndl %d, rrq_start %p\n", 
	    p_ctx->p_host_map, p_ctx->ctx_hndl, p_ctx->p_hrrq_start);

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++) {
	p_ctx->cmd[i].rcb.msi = 0x2;
	p_ctx->cmd[i].rcb.rrq = 0x0;
	p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }

    // set up RRQ in AFU
    MMIO_WRITE_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
    MMIO_WRITE_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);

    // program FC_PORT LUN Tbl
    MMIO_WRITE_64(&p_ctx->p_afu_map->global.fc_port[0][LUN_INDEX], lun_id);
    MMIO_WRITE_64(&p_ctx->p_afu_map->global.fc_port[1][LUN_INDEX], lun_id);

    // AFU configuration
    MMIO_READ_64(&p_ctx->p_afu_map->global.regs.afu_config, &reg);
    reg |= 0x7F00;        // enable auto retry
    MMIO_WRITE_64(&p_ctx->p_afu_map->global.regs.afu_config, reg);

    // turn off PSL page-mode translation, use in-order translation
    // MMIO_WRITE_64((__u64*)&p_ctx->p_afu_map->global.page1[0x28], 0);

    // set up my own CTX_CAP to allow real mode, host translation
    // tbls, allow read/write cmds
    MMIO_READ_64(&p_ctx->p_ctrl_map->mbox_r, &reg);
    asm volatile ( "eieio" : : );
    MMIO_WRITE_64(&p_ctx->p_ctrl_map->ctx_cap, 
	     SISL_CTX_CAP_REAL_MODE | SISL_CTX_CAP_HOST_XLATE |
	     SISL_CTX_CAP_WRITE_CMD | SISL_CTX_CAP_READ_CMD);

    // set up LBA xlate
    //
    for (i = 0; i < NUM_CMDS; i++) {
	// LUN_INDEX & select both ports, use r/w perms from RHT
	p_ctx->lxt[i].rlba_base 
	    = (((chunk_base + i) << MC_CHUNK_SHIFT) | (LUN_INDEX << 8) | 0x33);
    }
    p_ctx->rht.lxt_start = &p_ctx->lxt[0];
    p_ctx->rht.lxt_cnt = NUM_CMDS;
    p_ctx->rht.nmask = MC_RHT_NMASK;
    p_ctx->rht.fp = SISL_RHT_FP(0u, 0x3); /* format 0 & RW perms */

    // make tables visible to AFU before MMIO
    asm volatile ( "lwsync" : : ); 

    // make  MMIO registers for this context point to the single entry
    // RHT. The RHT is under this context.
    MMIO_WRITE_64(&p_ctx->p_ctrl_map->rht_start,
	     (__u64)&p_ctx->rht);
    MMIO_WRITE_64(&p_ctx->p_ctrl_map->rht_cnt_id,
	     SISL_RHT_CNT_ID((__u64)1,  
			     (__u64)(p_ctx->ctx_hndl)));
    return 0;
}

void ctx_close(struct ctx *p_ctx) 
{
#if LIBAFU
    afu_unmap(p_afu);
#else
    munmap((void*)p_ctx->p_afu_map, sizeof(struct surelock_afu_map));
    close(p_ctx->afu_fd);
#endif
}

__u64 xlate_lba(__u64 vlba) {
    __u64 chunk_id = (vlba >> MC_CHUNK_SHIFT);
    __u64 chunk_off = (vlba & MC_CHUNK_OFF_MASK);
    __u64 rlba_base;

    if (chunk_id < NUM_CMDS) {
	rlba_base = ((chunk_base + chunk_id) << MC_CHUNK_SHIFT);
	return (rlba_base | chunk_off);
    }
    else {
	return -1; // error
    }
}

// writes wbuf using physical LBA
void send_writep(struct ctx *p_ctx, __u64 start_lba, __u64 stride) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba, plba;

    for (i = 0; i < NUM_CMDS; i++) {
	vlba = start_lba + i*stride;
	plba = xlate_lba(vlba);

	fill_buf((__u64*)&p_ctx->wbuf[i][0], 
		 sizeof(p_ctx->wbuf[i])/sizeof(__u64), vlba, plba);

	p_ctx->cmd[i].rcb.lun_id = lun_id;
	p_ctx->cmd[i].rcb.port_sel = 0x3; // either FC port
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[i]);
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID |
				       SISL_REQ_FLAGS_HOST_WRITE);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	write_64(p_u64, plba); // physical LBA#
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[i].sa.ioasc = 0;
    }

    if ((gen_rand() & REMAP_FREQ) == 0) {
      p_ctx = remap();
    }

    send_cmd(p_ctx);
    wait_resp(p_ctx);
}

// read into rbuf using virtual LBA 
void send_readv(struct ctx *p_ctx, __u64 start_lba, __u64 stride) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba;

    for (i = 0; i < NUM_CMDS; i++) {
	memset(&p_ctx->rbuf[i][0], 0xB, sizeof(p_ctx->rbuf[i]));

	p_ctx->cmd[i].rcb.res_hndl = 0; // only 1 resource open at RHT[0]
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbuf[i]);
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_RES_HNDL |
				       SISL_REQ_FLAGS_HOST_READ);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbuf[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	vlba = start_lba + i*stride;

	write_64(p_u64, vlba); // virtual LBA#
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[i].sa.ioasc = 0;
    }

    if ((gen_rand() & REMAP_FREQ) == 0) {
      p_ctx = remap();
    }

    send_cmd(p_ctx);
    wait_resp(p_ctx);
}

// read into rbuf using physical LBA 
void send_readp(struct ctx *p_ctx, __u64 start_lba, __u64 stride) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba, plba;

    for (i = 0; i < NUM_CMDS; i++) {
	memset(&p_ctx->rbuf[i][0], 0xB, sizeof(p_ctx->rbuf[i]));

	p_ctx->cmd[i].rcb.lun_id = lun_id;
	p_ctx->cmd[i].rcb.port_sel = 0x3; // either FC port
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbuf[i]);
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID |
				       SISL_REQ_FLAGS_HOST_READ);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbuf[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	vlba = start_lba + i*stride;
	plba = xlate_lba(vlba);

	write_64(p_u64, plba); // physical LBA#
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[i].sa.ioasc = 0;
    }

    if ((gen_rand() & REMAP_FREQ) == 0) {
      p_ctx = remap();
    }

    send_cmd(p_ctx);
    wait_resp(p_ctx);
}


// read into rbufm using physical LBA - used in miscompare debug only
void send_readm(struct ctx *p_ctx, __u64 start_lba, __u64 stride) {
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba, plba;

    for (i = 0; i < NUM_CMDS; i++) {
	memset(&p_ctx->rbufm[i][0], 0xB, sizeof(p_ctx->rbufm[i]));

	p_ctx->cmd[i].rcb.lun_id = lun_id;
	p_ctx->cmd[i].rcb.port_sel = 0x3; // either FC port
	p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbufm[i]);
	p_ctx->cmd[i].rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID |
				       SISL_REQ_FLAGS_HOST_READ);
	p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbufm[i][0];

	memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
	p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	vlba = start_lba + i*stride;
	plba = xlate_lba(vlba);

	write_64(p_u64, plba); // physical LBA#
	p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
	write_32(p_u32, 8); // 8 LBAs for 4K

	p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[i].sa.ioasc = 0;
    }

    send_cmd(p_ctx);
    wait_resp(p_ctx);
}

// compare wbuf & rbuf
void rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba, __u64 stride) {
    int i;
    char buf[32];
    int read_fd, write_fd, readm_fd;

    for (i = 0; i < NUM_CMDS; i++) {
	if (cmp_buf((__u64*)&p_ctx->rbuf[i][0], (__u64*)&p_ctx->wbuf[i][0], 
		    sizeof(p_ctx->rbuf[i])/sizeof(__u64))) {
	    printf("%d: miscompare at start_vlba 0x%lx, chunk# %d\n", 
		   pr_id, start_lba, i);
	    fflush(stdout);

	    send_readm(p_ctx, start_lba, stride); // sends NUM_CMDS reads

	    sprintf(buf, "read.%d", pr_id);
	    read_fd = open(buf, O_RDWR|O_CREAT, 0600);
	    sprintf(buf, "write.%d", pr_id);
	    write_fd = open(buf, O_RDWR|O_CREAT, 0600);
	    sprintf(buf, "readm.%d", pr_id);
	    readm_fd = open(buf, O_RDWR|O_CREAT, 0600);

	    write(read_fd,  &p_ctx->rbuf[i][0], sizeof(p_ctx->rbuf[i]));
	    write(write_fd, &p_ctx->wbuf[i][0], sizeof(p_ctx->wbuf[i]));
	    write(readm_fd, &p_ctx->rbufm[i][0], sizeof(p_ctx->rbufm[i]));

	    close(read_fd);
	    close(write_fd);
	    close(readm_fd);

	    while(1); // stop IOs and stay quiet
	}
    }
}

// do not touch memory to make remap effective
void send_cmd(struct ctx *p_ctx) {
    int cnt = NUM_CMDS;
    int i;
    __u64 room;

    asm volatile ( "lwsync" : : ); /* make memory updates visible to AFU */

    while (cnt) {
        asm volatile ( "eieio" : : ); // let IOARRIN writes complete
        MMIO_READ_64(&p_ctx->p_host_map->cmd_room, &room);
	for (i = 0; i < room; i++) {
	    // write IOARRIN
	    MMIO_WRITE_64(&p_ctx->p_host_map->ioarrin, 
		     (__u64)&p_ctx->cmd[cnt - 1].rcb);
	    if (cnt-- == 1) break;
	}
    }
}

void wait_resp(struct ctx *p_ctx) {
    int i;

    for (i = 0; i < NUM_CMDS; i++) {
	pthread_mutex_lock(&p_ctx->cmd[i].mutex);
	while (!(p_ctx->cmd[i].sa.host_use[0] & B_DONE)) {
	    pthread_cond_wait(&p_ctx->cmd[i].cv, &p_ctx->cmd[i].mutex);
	}
	pthread_mutex_unlock(&p_ctx->cmd[i].mutex);

	if (p_ctx->cmd[i].sa.ioasc) {	
	    printf("%d:IOASC = flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n", 
		   pr_id,
		   p_ctx->cmd[i].sa.rc.flags,
		   p_ctx->cmd[i].sa.rc.afu_rc, 
		   p_ctx->cmd[i].sa.rc.scsi_rc, 
		   p_ctx->cmd[i].sa.rc.fc_rc);
	    fflush(stdout);
	}
    }

}

__u64 gen_rand() {
    __u64 rand;

    if (read(rand_fd, &rand, sizeof(rand)) != sizeof(rand)) {
	fprintf(stderr, "cannot read random device, errno %d\n", errno);
	exit(-1);
    }
    return rand;
}

struct ctx* remap() {
#if LIBAFU
  return p_ctx; // NOP
#else
  void *map;
  munmap((void*)p_ctx, sizeof(struct ctx));

  // ask for the mapping at the same address since there are pointers
  // in struct ctx or in the AFU's RHT/LXT that we do not want to 
  // reinitialize
  //
  map = mmap(p_ctx, sizeof(struct ctx),
	     PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, ctx_fd, 0);
  if (map == MAP_FAILED) {
    fprintf(stderr, "remap failed, errno %d\n", errno);
    exit(-1);
  }
  return (struct ctx *) map;
#endif
}


// when runing w/libafu, the master device is hard coded in afu.c to
// /dev/cxl/afu0.0m. The cmd line path is not used.
//
int
main(int argc, char *argv[])
{
    int rc;
#if LIBAFU
    struct ctx myctx;
#else
    void *map;
#endif
    pthread_t thread;

    __u64 start_lba;
    __u64 npass = 0;
    __u64 stride = MC_CHUNK_SIZE; // or use 8 to test all LBAs
    unsigned long nchunk = NUM_CMDS;
    __u64 nlba = nchunk*MC_CHUNK_SIZE;

    get_parameters(argc, argv);

    pr_id = getpid(); // pid used to create unique data patterns
                    // or ctx file for mmap

    rand_fd = open("/dev/urandom", O_RDONLY);
    if (rand_fd < 0) {
	fprintf(stderr, "cannot open random device, errno %d\n", errno);
	exit(-1);
    }

#if LIBAFU
    p_ctx = &myctx;
#else
    sprintf(ctx_file, "ctx.%d", pr_id);
    unlink(ctx_file);
    ctx_fd = open(ctx_file, O_RDWR|O_CREAT, 0600);
    if (ctx_fd < 0) {
	fprintf(stderr, "open failed: file %s, errno %d", ctx_file, errno);
	exit(-1);
    }

    // mmap a struct ctx 
    ftruncate(ctx_fd, sizeof(struct ctx));
    map = mmap(NULL, sizeof(struct ctx),
	       PROT_READ|PROT_WRITE, MAP_SHARED, ctx_fd, 0);
    if (map == MAP_FAILED) {
	fprintf(stderr, "mmap failed, errno %d\n", errno);
	exit(-1);
    }
    p_ctx = (struct ctx *) map;
#endif

    printf("instantiating ctx on %s...\n", afu_path);
    rc = ctx_init(p_ctx, afu_path);
    if (rc != 0) {
	fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	exit(-1);
    }
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);



    // when stride == 1 chunk, it sends NUM_CMDS writes followed by 
    // same numner of reads. each command targets a different chunk.
    //
    while (1) {
	for (start_lba = 0; start_lba < nlba; start_lba += (NUM_CMDS*stride)) {
	    send_writep(p_ctx, start_lba, stride);
	    (*read_fcn)(p_ctx, start_lba, stride);
	    rw_cmp_buf(p_ctx, start_lba, stride);
	}

	if ((npass++ & 0xFF) == 0) {
	    printf("%d:completed pass %ld\n", pr_id, npass>>8); fflush(stdout);
	}
    }

    pthread_join(thread, NULL);

    return 0;
}

// len in __u64
void fill_buf(__u64* p_buf, unsigned int len, __u64 vlba, __u64 plba)
{
  static __u64 data = DATA_SEED;
  int i;

  // the vlba & plba helps to see if right data is going to the right
  // place.
  for (i = 0; i < len; i += 4) {
    p_buf[i] = pr_id;
    p_buf[i + 1] = data++;
    p_buf[i + 2] = vlba;
    p_buf[i + 3] = plba;
  }
}


// len in __u64
int cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len)
{
  return memcmp(p_buf1, p_buf2, len*sizeof(__u64));
}

