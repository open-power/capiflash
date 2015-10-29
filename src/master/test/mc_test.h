/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test.h $                                   */
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


#ifndef __MC_H__
#define __MC_H__

#include <mclient.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <cxl.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

// not implemented in AIX
#ifndef WCOREDUMP
#define WCOREDUMP(x) ((x) & 0x80)
#endif

#define MAX_TRY_WAIT 100
#define MC_BLOCK_DELAY_ROOM 1000

#define ENABLE_HEXDUMP 0 //0 for disable hexdump
#define MMIO_MAP_SIZE 64*1024

#define LBA_BLK 8 //1 for 4K blk size & 8 for 512 BLK

//Flags to be used while sending
//send_read & send_write 
#define PLBA 0x0
#define VLBA 0x1
#define NO_XLATE 0x2

//max allow ctx handler
#define MAX_OPENS 507 

//max allow RHT per context
#define MAX_NUM_THREADS 16 /*hardcoded 16 MAX_RHT_CONTEXT*/
#define MAX_RES_HANDLE 16 /*hardcoded 16 MAX_RHT_CONTEXT */

#define B_DONE   0x01
#define B_ERROR  0x02
#define NUM_RRQ_ENTRY 16

#define NUM_CMDS 16     /* max is NUM_RRQ_ENTRY */

#define CL_SIZE 128       /* Processor cache line size */
#define CL_SIZE_MASK 0x7F /* Cache line size mask */

#define DEBUG 0 //0 disable, 1 some level, 2 complete

#define CHECK_RC(rc, msg);\
	if(rc){\
	fprintf(stderr,"Failed @ %s:%d:%s rc = %d on %s\n",\
	__FILE__, __LINE__, __func__,rc, msg);\
	return rc;}\
	
#define CHECK_RC_EXIT(rc, msg);\
	if(rc){\
	fprintf(stderr,"Failed @ %s:%d:%s rc = %d on %s\n",\
	__FILE__, __LINE__, __func__,rc, msg);\
	exit(rc);}/

#define debug\
	if(DEBUG >= 1) printf

#define debug_2\
	if(DEBUG ==2) printf

	//fprintf(stderr, "\n%s:%d:%s:", __FILE__, __LINE__, __func__);
struct res_hndl_s {
  res_hndl_t res_hndl;
  __u64 size;
};

struct mc_hndl_s {
  mc_hndl_t mc_hndl;
  struct res_hndl_s *res_hndl_p;
};

//Context structure.
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
	char event_buf[0x1000];
    volatile struct sisl_host_map *p_host_map;
    ctx_hndl_t ctx_hndl;

    __u64 *p_hrrq_start;
    __u64 *p_hrrq_end;
    volatile __u64 *p_hrrq_curr;
    unsigned int toggle;

    // MC client interface
    mc_hndl_t mc_hndl;
    res_hndl_t res_hndl;
    struct mc_hndl_s *mc_hndl_p; //For handling multiple mc_hndl and res_hndl
} __attribute__ ((aligned (0x1000)));

typedef struct ctx * ctx_p;

struct ctx_alloc {
  struct ctx    ctx;
  __u8    page_pad[0x1000 - (sizeof(struct ctx) & 0xFFF)];
};

struct pthread_alloc {
    pthread_t rrq_thread;
};

struct rwbuf{
	char wbuf[NUM_CMDS][0x2000]; //8K for EA alignment
	char rbuf[NUM_CMDS][0x2000]; //8K for EA alignment
}__attribute__ ((aligned (0x1000)));

struct rwshmbuf{
	char wbuf[1][0x1000];
	char rbuf[1][0x1000];
}__attribute__ ((aligned (0x1000)));

typedef
enum {
	TEST_MC_REG =1,
	TEST_MC_UNREG,
	TEST_MC_OPEN,
	TEST_MC_OPEN_ERROR1,
	TEST_MC_OPEN_ERROR2,
	TEST_MC_OPEN_ERROR3,
	TEST_MC_CLOSE,
	TEST_MC_CLOSE_ERROR1,
	TEST_MC_SIZE,
	TEST_MC_SIZE_ERROR2,
	TEST_MC_SIZE_ERROR3,
	TEST_MC_SIZE_ERROR4,
	TEST_MC_GETSIZE,
	TEXT_MC_XLAT_IO,
	TEXT_MC_XLAT_IO_V2P,
	TEXT_MC_XLAT_IO_P2V,
	TEST_MC_HDUP_ORG_IO,
	TEST_MC_HDUP_DUP_IO,
	TEST_MC_HDUP_ERROR1,
	TEST_MC_DUP,
	TEST_MC_CLONE_RDWR,
	TEST_MC_CLONE_READ,
	TEST_MC_CLONE_WRITE,
	TEST_MC_CLN_O_RDWR_CLN_RD,
	TEST_MC_CLN_O_RDWR_CLN_WR,
	TEST_MC_CLN_O_RD_CLN_RD,
	TEST_MC_CLN_O_RD_CLN_WR,
	TEST_MC_CLN_O_WR_CLN_RD,
	TEST_MC_CLN_O_WR_CLN_WR,
	TEST_MC_CLN_O_RD_CLN_RDWR,
	TEST_MC_CLN_O_WR_CLN_RDWR,
	TEST_MC_MAX_SIZE,
	TEST_MC_MAX_CTX_N_RES,
	TEST_MC_MAX_RES_HNDL,
	TEST_MC_MAX_CTX_HNDL,
	TEST_MC_MAX_CTX_HNDL2,
	TEST_MC_CHUNK_REGRESS,
	TEST_MC_CHUNK_REGRESS_BOTH_AFU,
	TEST_MC_SIGNAL,
	TEST_ONE_UNIT_SIZE,
	TEST_MC_LUN_SIZE,
	TEST_MC_PLBA_OUT_BOUND,
	TEST_MC_INVALID_OPCODE,
	TEST_MC_IOARCB_EA_NULL,
	TEST_MC_IOARCB_INVLD_FLAG,
	TEST_MC_IOARCB_INVLD_LUN_FC,
	TEST_MC_ONE_CTX_TWO_THRD,
	TEST_MC_ONE_CTX_RD_WRSIZE,
	TEST_MC_TWO_CTX_RD_WRTHRD,
	TEST_MC_TWO_CTX_RDWR_SIZE,
	TEST_MC_LUN_DISCOVERY,
	TEST_MC_TEST_VDISK_IO,
	TEST_MC_RW_CLS_RSH,
	TEST_MC_UNREG_MC_HNDL,
	TEST_MC_RW_CLOSE_CTX,
	TEST_MC_CLONE_MANY_RHT,
	TEST_AUN_WR_PLBA_RD_VLBA,
	TEST_GOOD_CTX_ERR_CTX,
	TEST_GOOD_CTX_ERR_CTX_CLS_RHT,
	TEST_GOOD_CTX_ERR_CTX_UNREG_MCH,
	TEST_MC_IOARCB_EA_ALGNMNT_16,
	TEST_MC_IOARCB_EA_ALGNMNT_128,
	TEST_MC_MNY_CTX_ONE_RRQ_C_NULL,
	TEST_MC_ALL_AFU_DEVICES,
	TEST_MC_IOARCB_EA_INVLD_ALGNMNT,
	MC_TEST_RWBUFF_GLOBAL,
	MC_TEST_RW_SIZE_PARALLEL,
	MC_TEST_RWBUFF_SHM,
	MC_TEST_GOOD_ERR_AFU_DEV,
	TEST_MC_AFU_RC_CAP_VIOLATION,
	TEST_MC_REGRESS_RESOURCE,
	TEST_MC_REGRESS_CTX_CRT_DSTR,
	TEST_MC_REGRESS_CTX_CRT_DSTR_IO,
	TEST_MC_SIZE_REGRESS,
}mc_test_t;


int mc_max_open_tst();
int mc_open_close_tst();
int mc_register_tst();
int mc_unregister_tst();
int mc_open_tst(int);
int mc_size_tst(int);
int mc_xlate_tst(int);
int mc_hdup_tst(int cmd);
int mc_max_vdisk_thread();
int test_mc_clone_api(__u32 flags);
int test_mc_clone_error(__u32 oflg, __u32 cnflg);
int test_mc_max_size();
int test_max_ctx_n_res();
int mc_test_engine(mc_test_t);
int test_one_aun_size();
int test_mc_clone_read();
int test_mc_clone_write();
int test_mc_lun_size(int cmd);
int test_mc_dup_api();
int mc_close_tst();
int mc_test_chunk_regress(int cmd);
int mc_test_chunk_regress_long();
int mc_test_chunk_regress_both_afu();
int test_mc_xlate_error(int);
int test_vdisk_io();
int ctx_init(struct ctx *p_ctx);
int ctx_init_thread(void *);
int test_lun_discovery(int cmd);
int test_onectx_twothrd(int cmd);
int test_mc_reg_error(int cmd);
int test_two_ctx_two_thrd(int cmd);
int test_mc_invalid_ioarcb(int cmd);
int test_rw_close_hndl(int cmd);
int test_mc_clone_many_rht();
int test_good_ctx_err_ctx(int cmd);
int test_mc_ioarcb_ea_alignment(int cmd);
int check_mc_null_params(int cmd);
int test_many_ctx_one_rrq_curr_null();
int test_all_afu_devices();
int mc_test_rwbuff_global();
int test_mc_rwbuff_shm();
int test_mc_rw_size_parallel();
int test_mc_good_error_afu_dev();
int test_mc_regress_ctx_crt_dstr(int cmd);
int test_mc_size_error(int cmd);
int test_mc_null_params(int cmd);
int test_mc_inter_prcs_ctx(int cmd);
int mc_test_ctx_regress(int cmd);
void ctx_close(struct ctx *p_ctx);
void ctx_close_thread(void *);
int get_fvt_dev_env();

int test_init(struct ctx *p_ctx);
void *ctx_rrq_rx(void *arg);
int send_write(struct ctx *p_ctx, __u64 start_lba,
		__u64 stride, __u64 data,__u32 flags);
int send_single_write(struct ctx *p_ctx, __u64 vlba, __u64 data);
int send_read(struct ctx *p_ctx, __u64 start_lba, __u64 stride, __u32 flags);
int send_single_read(struct ctx *p_ctx, __u64 vlba);
int rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba);
int rw_cmp_buf_cloned(struct ctx *p_ctx, __u64 start_lba);
int cmp_buf_cloned(__u64* p_buf, unsigned int len);
int rw_cmp_single_buf(struct ctx *p_ctx, __u64 vlba);
int send_cmd(struct ctx *p_ctx);
int wait_resp(struct ctx *p_ctx);
int wait_single_resp(struct ctx *p_ctx);
void fill_buf(__u64* p_buf, unsigned int len, __u64 data);
int cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len);
int send_report_luns(struct ctx *p_ctx, __u32 port_sel,
		__u64 **lun_ids,__u32 *nluns);
int send_read_capacity(struct ctx *p_ctx, __u32 port_sel,
                __u64 lun_id, __u64 *lun_capacity, __u64 *blk_len);
int check_status(sisl_ioasa_t *p_ioasa);
void send_single_cmd(struct ctx *p_ctx);
int send_rw_rcb(struct ctx *p_ctx, struct rwbuf *p_rwb,
				__u64 start_lba, __u64 stride,
				int align, int where);
int send_rw_shm_rcb(struct ctx *p_ctx, struct rwshmbuf *p_rwb,
                __u64 vlba);
void hexdump(void *data, long len, const char *hdr);

#endif
