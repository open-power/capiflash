/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/pathlength_test.c $                                  */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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


#include <sislite.h>
#include <cxl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <poll.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define MAX_WAIT_CNT      1000
#define MC_RHT_NMASK      16                  /* in bits */
#define MC_CHUNK_SIZE     (1 << MC_RHT_NMASK) /* in LBAs, see mclient.h */
#define MC_CHUNK_SHIFT    MC_RHT_NMASK  /* shift to go from LBA to chunk# */
#define MC_CHUNK_OFF_MASK (MC_CHUNK_SIZE - 1) /* apply to LBA get offset */


#include <mclient.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#define MMIO_WRITE_64(addr, val) write_64((addr), (val))
#define MMIO_READ_64(addr, p_val) *(p_val) = read_64((addr))



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

	__u8 cl_pad[CL_SIZE - 
		    ((sizeof(sisl_ioarcb_t) +
		      sizeof(sisl_ioasa_t)) & CL_SIZE_MASK)];
    } cmd[NUM_CMDS];

    int cur_cmd_index;
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

int  ctx_init(struct ctx *p_ctx, char *dev_path);
int async_read(struct ctx *p_ctx, __u64 start_lba);
int async_poll(struct ctx *p_ctx,int *status);




char *afu_path;   /* points to argv[] string */
pid_t pid;
__u64 lun_id = 0x0;
__u64 read_lba = 0x0;
__u64 chunk_base = CHUNK_BASE;
int ctx_fd;
char ctx_file[32];
struct ctx *p_ctx;


void usage(char *prog)
{
    printf("Usage: %s [-l lun_id] [-a lba ] master_dev_path\n", prog);
    printf("e. g.: %s -l 0x1000000000000 /dev/cxl/afu0.0m\n", prog);
}

void
get_parameters(int argc, char** argv)
{
  extern int   optind;         /* for getopt function */
  extern char  *optarg;        /* for getopt function */
  int ch;

  while ((ch = getopt(argc,argv,"pl:c:h")) != EOF) {
    switch (ch) {
    case 'a' :      /* lba to use */
	sscanf(optarg, "%lx", &read_lba);
	break;

    case 'l' :      /* LUN_ID to use */
	sscanf(optarg, "%lx", &lun_id);
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




// dev_path must be master device
// dev_path is not used when running w/libafu - see afu.c 
int ctx_init(struct ctx *p_ctx, char *dev_path) 
{


    void *map;
    __u32 proc_elem;

    int i;
    __u64 reg;

    // general init, no resources allocated
    memset(p_ctx, 0, sizeof(*p_ctx));

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
	//p_ctx->cmd[i].rcb.msi = 0x2;
	p_ctx->cmd[i].rcb.rrq = 0x0;
	p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }

    p_ctx->cur_cmd_index = 0;

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

    munmap((void*)p_ctx->p_afu_map, sizeof(struct surelock_afu_map));
    close(p_ctx->afu_fd);

}

// read into rbuf using virtual LBA 
inline int async_read(struct ctx *p_ctx, __u64 start_lba) {
    __u64 *p_u64;
    __u64 room;



    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.res_hndl = 0; // only 1 resource open at RHT[p_ctx->cur_cmd_index]
    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.data_len = sizeof(p_ctx->rbuf[p_ctx->cur_cmd_index]);
    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.req_flags = (SISL_REQ_FLAGS_RES_HNDL |
				   SISL_REQ_FLAGS_HOST_READ);
    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.data_ea = (__u64) &p_ctx->rbuf[p_ctx->cur_cmd_index][0];

    /*
     *
     *
     *                        READ(16) Command                                 
     *  +=====-======-======-======-======-======-======-======-======+        
     *  |  Bit|   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |        
     *  |Byte |      |      |      |      |      |      |      |      |        
     *  |=====+=======================================================|        
     *  | 0   |                Operation Code (88h)                   |        
     *  |-----+-------------------------------------------------------|        
     *  | 1   |                    | DPO  | FUA  | Reserved    |RelAdr|        
     *  |-----+-------------------------------------------------------|        
     *  | 2   | (MSB)                                                 |        
     *  |-----+---                                                 ---|        
     *  | 3   |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 4   |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 5   |             Logical Block Address                     |        
     *  |-----+---                                                 ---|        
     *  | 6   |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 7   |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 8   |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 9   |                                                 (LSB) |        
     *  |-----+-------------------------------------------------------|        
     *  | 10  | (MSB)                                                 |        
     *  |-----+---                                                 ---|        
     *  | 11  |                                                       |        
     *  |-----+---              Transfer Length                    ---|        
     *  | 12  |                                                       |        
     *  |-----+---                                                 ---|        
     *  | 13  |                                                 (LSB) |        
     *  |-----+-------------------------------------------------------|        
     *  | 14  |                    Reserved                           |        
     *  |-----+-------------------------------------------------------|        
     *  | 15  |                    Control                            |        
     *  +=============================================================+        
     *      
     */
    memset(&p_ctx->cmd[p_ctx->cur_cmd_index].rcb.cdb[p_ctx->cur_cmd_index], 0, sizeof(p_ctx->cmd[p_ctx->cur_cmd_index].rcb.cdb));
    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.cdb[p_ctx->cur_cmd_index] = 0x88; // read(16)
    p_u64 = (__u64*)&p_ctx->cmd[p_ctx->cur_cmd_index].rcb.cdb[2];

    write_64(p_u64, start_lba); // virtual LBA#

    p_ctx->cmd[p_ctx->cur_cmd_index].rcb.cdb[13] = 0x8; 

    p_ctx->cmd[p_ctx->cur_cmd_index].sa.host_use[p_ctx->cur_cmd_index] = 0; // 0 means active
    p_ctx->cmd[p_ctx->cur_cmd_index].sa.ioasc = 0;
    

    asm volatile ( "lwsync" : : ); /* make memory updates visible to AFU */


    MMIO_READ_64(&p_ctx->p_host_map->cmd_room, &room);

    if (room) {

	/*
	 * AFU can accept this command
	 */


	// write IOARRIN
	MMIO_WRITE_64(&p_ctx->p_host_map->ioarrin, 
		      (__u64)&p_ctx->cmd[p_ctx->cur_cmd_index].rcb);


	
	if (p_ctx->cur_cmd_index < NUM_CMDS) {
	    p_ctx->cur_cmd_index++; /* advance to next RRQ entry */
	}
	else { /* wrap */
	    p_ctx->cur_cmd_index = 0;;
	}

	return 0;
    }

    return EBUSY;

}


// check for completion of read
inline int async_poll(struct ctx *p_ctx,int *status) {
  struct afu_cmd *p_cmd;

  
  
  if ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
      p_ctx->toggle) {
      
      p_cmd = (struct afu_cmd*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));
      
      if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end) {
	  p_ctx->p_hrrq_curr++; /* advance to next RRQ entry */
      }
      else { /* wrap HRRQ  & flip toggle */
	  p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
	  p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
      }
      
      *status = p_cmd->sa.ioasc;
      
      return TRUE;
      
  }
  

  return FALSE;
}

// when runing w/libafu, the master device is hard coded in afu.c to
// /dev/cxl/afu0.0m. The cmd line path is not used.
//
int
main(int argc, char *argv[])
{
    int rc;
    void *map;
    int status;
    __u64 start_build_ic = 0;
    __u64 finish_build_ic = 0;
    __u64 start_status_ic = 0;
    __u64 finish_status_ic = 0;


    struct sched_param  sched;
    int sched_policy;


    get_parameters(argc, argv);

    pid = getpid(); // pid used to create unique data patterns 
                    // or ctx file for mmap

    sched_policy = sched_getscheduler(0);

    if (sched_policy < 0) {

	fprintf(stderr,"getscheduler failed with errno = %d\n",errno);
	exit(-1);
    }

    bzero((void *)&sched,sizeof(sched));


    sprintf(ctx_file, "ctx.%d", pid);
    unlink(ctx_file);
    ctx_fd = open(ctx_file, O_RDWR|O_CREAT);
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

    printf("instantiating ctx on %s...\n", afu_path);
    rc = ctx_init(p_ctx, afu_path);
    if (rc != 0) {
	fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
	exit(-1);
    }


    memset(&p_ctx->rbuf[p_ctx->cur_cmd_index][0], 0xB, sizeof(p_ctx->rbuf[p_ctx->cur_cmd_index]));


    /*
     * Get instruction count (using PMC5 register at offset 775) 
     * before building and queuing a command
     */

    asm volatile ( "mfspr %0, 775" : "=r"(start_build_ic) : ); 

    rc = async_read(p_ctx, read_lba);

    if (rc) {

	printf("failed to issue command rc = %d\n",rc);
    }

    /*
     * Get instruction count (using PMC5 register at offset 775) 
     * after building and queuing a command
     */
    asm volatile ( "mfspr %0, 775" : "=r"(finish_build_ic) : ); 


    printf("start_build_ic = 0x%lx,finish_build_ic = 0x%lx, diff = 0x%lx\n",
	   start_build_ic, 
	   finish_build_ic, (finish_build_ic-start_build_ic));

  
    usleep(250);
  
    /*
     * Get instruction count (using PMC5 register at offset 775) 
     * before processing command completion
     */
  
    asm volatile ( "mfspr %0, 775" : "=r"(start_status_ic) : ); 
  
    rc = async_poll(p_ctx, &status);


      
    /*
     * Get instruction count (using PMC5 register at offset 775) 
     * after processing command completion
     */
    asm volatile ( "mfspr %0, 775" : "=r"(finish_status_ic) : ); 
      
      
    printf("start_status_ic = 0x%lx,finish_status_ic = 0x%lx, diff = 0x%lx\n",
	   start_status_ic, 
	   finish_status_ic, (finish_status_ic-start_status_ic));

    if (rc) {

	// Command completed


	printf("command completed with status = 0x%x\n",status);
    }


    return 0;
}

