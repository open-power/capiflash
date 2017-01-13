/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test.h $                               */
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
#ifndef __CFLASH_TEST_H__
#define __CFLASH_TEST_H__

// If MANUAL flag enabled then semi automated test also get build
// For jenkins run, we won't build manual tests by default !
//#define MANUAL

// AIX_MANUAL : under this tag test cases are MANUAL in AIX but automated in Linux
// Like - EEH test cases.
#ifndef _AIX

#define AIX_MANUAL

#endif

// Incase we define MANUAL then all the test case either MANUAL or AIX_MANUAL will be
/// enabled and available to user.

#ifdef MANUAL

#define AIX_MANUAL

#endif

#include <asmrw.h>
#include<stdbool.h>
#ifdef _AIX
#include <scsi.h>
#include <scdisk_capi.h>
#define DK_QEF_ALL_RESOURCE 0x0
#else
#include <scsi/cxlflash_ioctl.h>
typedef __u64 dev64_t; //no use in Linux, its dummy
//wil be removed once defined in Linux scope
//it will be dummy for Linux, these are useful in AIX
#define DK_UDF_ASSIGN_PATH 0x0000000000000001LL
#define DK_UVF_ASSIGN_PATH  0x0000000000000001LL
#define DK_UVF_ALL_PATHS  0x0000000000000002LL
#define DK_CAPI_REATTACHED 0x0000000000000001LL
#define DK_AF_ASSIGN_AFU 0x0000000000000002LL

// dummy definition to compile cflash_test_scen.c on linux
// TBD: cleanup once cflash_ioctl.h is updated
#define DK_VF_LUN_RESET 0x50
#define DK_RF_IOCTL_FAILED 0x70
#define DK_RF_REATTACHED 0x0
#define DK_VF_HC_TUR 0x0
#define DK_VF_HC_INQ 0x0

#define DK_RF_LUN_NOT_FOUND 0x1
#define DK_RF_ATTACH_NOT_FOUND 0x2
#define DK_RF_DUPLICATE 0x3
#define DK_RF_CHUNK_ALLOC 0x4
#define DK_RF_PATH_ASSIGNED 0x5

#endif /*_AIX */

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
#include <stdbool.h>
#include <inttypes.h>
#include <cxl.h>

#define MC_PATHLEN       64

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define CFLASH_ADAP_POLL_INDX  0
#define CFLASH_DISK_POLL_INDX  1

#define CFLSH_NUM_INTERRUPTS 4

// below flags are going to be used in do_write_or_read()
// depending on the user need 

#define WRITE 1
#define READ  2

#define RES_HNDLR_MASK 0xffffffff
#define CTX_HNDLR_MASK 0xffffffff

#define FDISKS_ALL 1
#define FDISKS_SAME_ADPTR 2
#define FDISKS_DIFF_ADPTR 3
#define FDISKS_SHARED 4

//IOCTL
#define MAX_PATH 8 //check later & change based on that
#define SCSI_VERSION_0 0 //Check where is defined in scsi headers
#define NMASK 0x10 //(1<<NMASK)=0x10000, number of 4K blocks in one chunk
#define NUM_BLOCKS 0x10000 //0x10000 4k blocks, 256MB size

#define LUN_VIRTUAL 0x1 //flag to create VLUNs
#define LUN_DIRECT 0x2 //flag to create PLUN

//Remove these macros in future & decide dynamically
#define MAX_FDISK 20 //max 20 flash disks, can increase in future
#define MAX_PATHS 8 //A flash disk can have max 8 paths, can change in future

// not implemented in AIX
#ifndef WCOREDUMP
#define WCOREDUMP(x) ((x) & 0x80)
#endif

#define MAX_TRY_WAIT 100
#define MC_BLOCK_DELAY_ROOM 1000

#define ENABLE_HEXDUMP 0 //0 for disable hexdump
#define MMIO_MAP_SIZE 64*1024


#define BLOCK_SIZE 0x1000 //default 4K block size

//max allow ctx handler
#ifdef _AIX
#define MAX_OPENS 494
#else
#define MAX_OPENS 508
#define MAX_OPENS_PVM 502
#endif

//max allowd VLUNS
#define MAX_VLUNS 1024


#ifndef _AIX
#define RECO_DISK_SIZE 500
#endif

#ifndef _AIX
#define B_DONE   0x01
#define B_ERROR  0x02
#endif

#define NUM_RRQ_ENTRY 4
#define NUM_CMDS 4

#define CL_SIZE 128       /* Processor cache line size */
#define CL_SIZE_MASK 0x7F /* Cache line size mask */

#define DEBUG 0 //0 disable, 1 some level, 2 complete

#define CHECK_RC(rc, msg) \
        if(rc){\
        fprintf(stderr,"%d: Failed @ %s:%d:%s(): rc = %d : %s\n",\
        pid,__FILE__, __LINE__, __func__,rc, msg);\
        g_error=-1;\
        return rc;}

#define CHECK_RC_EXIT(rc, msg) \
        if(rc){\
        fprintf(stderr,"%d: Failed @ %s:%d:%s(): rc = %d : %s\n",\
        pid,__FILE__, __LINE__, __func__,rc, msg);\
        exit(rc);}

#define debug\
        if(DEBUG >= 1) printf

#define debug_2\
        if(DEBUG ==2) printf

#define TESTCASE_SKIP(_reason) \
        printf("[  SKIPPED ] %s\n", _reason);

// all required for EEH automation  

#define MAXBUFF 400
#define MAXNP   10
#define KEYLEN  5

typedef struct eehCmd 
{
    char cmdToRun[MAXBUFF];
    int  ieehLoop;
    int eehSync;
    pthread_mutex_t eeh_mutex;
    pthread_cond_t eeh_cv;

} eehCmd_t;

int diskToPCIslotConv( char *, char * );
int prepEEHcmd(char *, char * );
void * do_trigger_eeh_cmd( void * );
int is_UA_device( char * disk_name );

//Context structure.

struct ctx
{
    /* Stuff requiring alignment go first. */

    /* Command & data for AFU commands issued by test. */
    char rbuf[NUM_CMDS][0x1000];    // 4K read data buffer (page aligned)
    char wbuf[NUM_CMDS][0x1000];    // 4K write data buffer (page aligned)
    __u64 rrq_entry[NUM_RRQ_ENTRY]; // 128B RRQ (page aligned)
    sisl_ioarcb_t sq_entry[NUM_RRQ_ENTRY+1];

    struct afu_cmd
    {
        volatile sisl_ioarcb_t rcb;  // IOARCB (cache line aligned)
        volatile sisl_ioasa_t sa;    // IOASA follows RCB
        pthread_mutex_t mutex;
        pthread_cond_t cv;

        __u8 cl_pad[CL_SIZE -
                ((sizeof(sisl_ioarcb_t) +
                      sizeof(sisl_ioasa_t) +
                      sizeof(pthread_mutex_t) +
                      sizeof(pthread_cond_t)) & CL_SIZE_MASK)];
    } cmd[NUM_CMDS];

    // AFU interface
    int fd;
    char dev[MC_PATHLEN];
    struct cxl_ioctl_start_work work;
    char event_buf[0x1000];
    volatile struct sisl_host_map *p_host_map;
    __u64 mmio_size;

    __u64 *p_hrrq_start;
    __u64 *p_hrrq_end;
    volatile __u64 *p_hrrq_curr;

    //SQ support 
    sisl_ioarcb_t *p_sq_start;
    sisl_ioarcb_t *p_sq_end;
    volatile sisl_ioarcb_t *p_sq_curr;

    unsigned int toggle;


    //for IOARCB request
    res_hndl_t res_hndl;
    ctx_hndl_t ctx_hndl;

    //for IOCTL
    uint16_t path_id;
    dev64_t devno;
    uint64_t flags;
    uint64_t return_flags;
    uint64_t block_size; /*a block size in bytes*/
    uint64_t chunk_size; /*size in blocks of one chunk */
    uint64_t last_lba; /* last lba of dev or vlun */
    uint64_t verify_last_lba; /* last lba of dev or vlun returned by verify ioctl */
    uint64_t last_phys_lba; /* last lba of physical disk */
    uint64_t lun_size; /* requested size in blocks */
    uint64_t req_size; /* New requested size, blocks */
    uint64_t hint;     /* Reasons for verify DK_HINT_*  */
    uint64_t reason;   /* Reason code for error */
    uint16_t version;
    uint64_t new_ctx_token; /* returned new_ctx_token by recover ioctl */
    uint64_t context_id;
    uint64_t rsrc_handle;
    uint32_t adap_fd;
    uint32_t new_adap_fd;
    uint32_t path_id_mask;
    uint64_t exceptions;
    char sense_data[512];
    int dummy_sense_flag;
    int close_adap_fd_flag; /* set when attach returns APP_CLOSE_ADAP_FD */
    int sq_mode_flag;       /* flag will be set when AFU is in SQ mode */
    uint64_t adap_except_type;
    uint64_t adap_except_time;
    uint64_t adap_except_data;
    uint64_t adap_except_count;
    char verify_sense_data[SISL_SENSE_DATA_LEN];
    
    __u64 st_lba;
    uint8_t return_path_count;
    __u64 max_xfer;

    //internal use
    __u32 blk_len; /*blocks requested to RW */
    __u64 unused_lba; /*hold avaibale disk blocks */
} __attribute__((aligned(0x1000)));

typedef struct ctx * ctx_p;

typedef
enum
{
    CASE_PLUN = 1,
    CASE_VLUN,
}validate_en_t;

struct validatePckt
{
   // we will continue to add member required to validate
   uint64_t expt_return_flags;
   uint64_t expt_last_lba;
   validate_en_t    obCase;
   struct ctx *ctxPtr;

};

struct ctx_alloc
{
    struct ctx    ctx;
    __u8    page_pad[0x1000 - (sizeof(struct ctx) & 0xFFF)];
};

struct pthread_alloc
{
    pthread_t rrq_thread;
};

struct rwbuf
{
    char wbuf[NUM_CMDS][0x2000]; //8K for EA alignment
    char rbuf[NUM_CMDS][0x2000]; //8K for EA alignment
}__attribute__((aligned(0x1000)));

struct rwshmbuf
{
    char wbuf[1][0x1000];
    char rbuf[1][0x1000];
}__attribute__((aligned(0x1000)));


//struct for large transfer size
struct rwlargebuf
{
    char *wbuf[NUM_CMDS];
    char *rbuf[NUM_CMDS];
};

struct flash_disk
{
    char dev[MC_PATHLEN];
    __u16 path_count;
    dev64_t devno[MAX_PATH];
    uint16_t path_id[MAX_PATH];
	uint32_t path_id_mask[MAX_PATH];

};

#ifndef _AIX
typedef
enum {
    CFLASH_HOST_UNKNOWN   = 0,  /* Unknown host type                */
    CFLASH_HOST_NV        = 1,  /* Bare Metal (or No virtualization */
    CFLASH_HOST_PHYP      = 2,  /* pHyp host type                   */
    CFLASH_HOST_KVM       = 3,  /* KVM host type                    */
} cflash_host_type_t;

cflash_host_type_t     host_type ;

#endif

typedef
enum
{
    TEST_SPIO_VLUN = 1,
    TEST_SPIO_0_VLUN,
    TEST_SPIO_A_PLUN,
    TEST_SPIO_ALL_PLUN,
    TEST_SPIO_ALL_VLUN,
    TEST_SPIO_VLUN_PLUN,
	TEST_SPIO_NORES_AFURC,
    TEST_MC_SIZE_REGRESS,
    TEST_MC_REGRESS_CTX_CRT_DSTR,
    TEST_MC_REGRESS_CTX_CRT_DSTR_IO,
    TEST_MC_REGRESS_RESOURCE,
    TEST_MC_TWO_CTX_RD_WRTHRD,
    TEST_MC_TWO_CTX_RDWR_SIZE,
    TEST_MC_ONE_CTX_TWO_THRD,
    TEST_MC_ONE_CTX_RD_WRSIZE,
    TEST_MC_MAX_RES_HNDL,
    TEST_ONE_UNIT_SIZE,
    TEST_MAX_CTX_RES_UNIT,
    TEST_MAX_CTX_RES_LUN_CAP,
    TEST_MC_MAX_SIZE,
    TEST_MC_SPIO_VLUN_ATCH_DTCH,
    TEST_MC_SPIO_PLUN_ATCH_DTCH,
    MC_TEST_RWBUFF_GLOBAL,
    MC_TEST_RWBUFF_HEAP,
    MC_TEST_RWBUFF_SHM,
    MC_TEST_RW_SIZE_PARALLEL,
    MC_TEST_GOOD_ERR_AFU_DEV,
    TEST_MC_RW_CLS_RSH,
    TEST_MC_RW_CLOSE_CTX,
    TEST_MC_RW_CLOSE_DISK_FD,
    TEST_MC_RW_UNMAP_MMIO,
    TEST_MC_IOARCB_EA_ALGNMNT_16,
    TEST_MC_IOARCB_EA_ALGNMNT_128,
    TEST_MC_IOARCB_EA_INVLD_ALGNMNT,
    TEST_LARGE_TRANSFER_IO,
    TEST_LARGE_TRNSFR_BOUNDARY,
    MAX_CTX_RCVR_EXCEPT_LAST_ONE,
    MAX_CTX_RCVR_LAST_ONE_NO_RCVR,
    /*** DK_CAPI_QUERY_PATH ****/
    TEST_DCQP_VALID_PATH_COUNT,
    TEST_IOCTL_INVALID_VERSIONS,
    TEST_DCQP_INVALID_PATH_COUNT,
    TEST_DCQP_DUAL_PATH_COUNT,
    TEST_DCQP_DK_CPIF_RESERVED,
    TEST_DCQP_DK_CPIF_FAILED,
    /**** DK_CAPI_ATTACH ***/
    TEST_DCA_OTHER_DEVNO,
    TEST_DCA_INVALID_DEVNO,
    TEST_DCA_INVALID_INTRPT_NUM,
    TEST_DCA_VALID_VALUES,
    TEST_DCA_INVALID_FLAGS,
    TEST_DCA_CALL_TWICE,
    TEST_DCA_CALL_DIFF_DEVNO_MULTIPLE,
    TEST_DCA_REUSE_CTX_FLAG,
    TEST_DCA_REUSE_CTX_FLAG_NEW_PLUN_DISK,
    TEST_DCA_REUSE_CTX_NEW_VLUN_DISK,
    TEST_DCA_REUSE_CTX_ALL_CAPI_DISK,
    TEST_DCA_REUSE_CTX_OF_DETACH_CTX,
    TEST_DCA_REUSE_CTX_OF_RELASED_IOCTL,
    TEST_DCA_REUSE_CTX_NEW_DISK_AFTER_EEH,
    /*** DK_CAPI_RECOVER_CTX ***/
    TEST_DCRC_NO_EEH,
    TEST_DCRC_DETACHED_CTX,
    TEST_DCRC_EEH_VLUN,
    TEST_DCRC_EEH_PLUN_MULTI_VLUN,
    TEST_DCRC_EEH_VLUN_RESUSE_CTX,
    TEST_DCRC_EEH_PLUN_RESUSE_CTX,
    TEST_DCRC_EEH_VLUN_RESIZE,
    TEST_DCRC_EEH_VLUN_RELEASE,
    TEST_DCRC_INVALID_DEVNO,
    TEST_DCRC_INVALID_FLAG,
    TEST_DCRC_INVALID_REASON,
    TEST_DCRC_IO_EEH_VLUN,
    TEST_DCRC_IO_EEH_PLUN,
    /*** DK_CAPI_USER_DIRECT ***/
    TEST_DCUD_INVALID_DEVNO_VALID_CTX,
    TEST_DCUD_INVALID_CTX_VALID_DEVNO,
    TEST_DCUD_INVALID_CTX_INVALID_DEVNO,
    TEST_DCUD_VALID_CTX_VALID_DEVNO,
    TEST_DCUD_PATH_ID_MASK_VALUES,
    TEST_DCUD_FLAGS,
    TEST_DCUD_TWICE_SAME_CTX_DEVNO,
    TEST_DCUD_VLUN_ALREADY_CREATED_SAME_DISK,
    TEST_DCUD_PLUN_ALREADY_CREATED_SAME_DISK,
    TEST_DCUD_VLUN_CREATED_DESTROYED_SAME_DISK,
    TEST_DCUD_IN_LOOP,
    TEST_DCUD_BAD_PATH_ID_MASK_VALUES,
    /*** DK_CAPI_USER_VIRTUAL ***/
    TEST_DCUV_INVALID_DEVNO_VALID_CTX,
    TEST_DCUV_INVALID_CTX_INVALID_DEVNO,
    TEST_DCUV_VALID_DEVNO_INVALID_CTX,
    TEST_DCUV_INVALID_FLAG_VALUES,
    TEST_DCUV_INVALID_VLUN_SIZE,
    TEST_DCUV_LUN_VLUN_SIZE_ZERO,
    TEST_DCUV_PLUN_ALREADY_CREATED_SAME_DISK,
    TEST_DCUV_VLUN_ALREADY_CREATED_SAME_DISK,
    TEST_DCUV_NO_FURTHER_VLUN_CAPACITY,
    TEST_DCUV_MTPLE_VLUNS_SAME_CAPACITY_SAME_DISK,
    TEST_DCUV_TWICE_SAME_CTX_DEVNO,
    TEST_DCUV_VLUN_MAX,
    TEST_DCUV_VLUN_SIZE_MORE_THAN_DISK_SIZE,
    TEST_DCUV_PLUN_CREATED_DESTROYED_SAME_DISK,
    TEST_DCUV_WITH_CTX_OF_PLUN,
    TEST_DCUD_WITH_CTX_OF_VLUN,
    TEST_DCUV_PATH_ID_MASK_VALUES,
    TEST_DCUV_INVALID_PATH_ID_MASK_VALUES,
    TEST_DCUV_IN_LOOP,
    /*** DK_CAPI_VLUN_RESIZE ***/
    TEST_DCVR_INVALID_DEVNO,
    TEST_DCVR_INVALID_CTX_DEVNO,
    TEST_DCVR_INVALID_CTX,
    TEST_DCVR_NO_VLUN,
    TEST_DCVR_ON_PLUN,
    TEST_DCVR_GT_DISK_SIZE,
    TEST_DCVR_NOT_FCT_256MB,
    TEST_DCVR_EQ_CT_VLUN_SIZE,
    TEST_DCVR_LT_CT_VLUN_SIZE,
    TEST_DCVR_GT_CT_VLUN_SIZE,
    TEST_DCVR_EQ_DISK_SIZE_NONE_VLUN,
    TEST_DCVR_EQ_DISK_SIZE_OTHER_VLUN,
    TEST_DCVR_INC_256MB,
    TEST_DCVR_DEC_256MB,
    TEST_DCVR_GT_CT_VLUN_LT_256MB,
    TEST_DCVR_LT_CT_VLUN_LT_256MB,
    TEST_DCVR_INC_DEC_LOOP,
	G_MC_test_DCVR_ZERO_Vlun_size,
    /*** DK_CAPI_RELEASE ***/
    TEST_DCR_INVALID_DEVNO,
    TEST_DCR_INVALID_DEVNO_CTX,
    TEST_DCR_INVALID_CTX,
    TEST_DCR_NO_VLUN,
    TEST_DCR_PLUN_AGIAN,
    TEST_DCR_VLUN_AGIAN,
    TEST_DCR_MULTP_VLUN,
	TEST_DCR_VLUN_INV_REL,
    /*** DK_CAPI_DETACH ***/
    TEST_DCD_INVALID_CTX_DEVNO,
    TEST_DCD_INVALID_DEVNO,
    TEST_DCD_INVALID_CTX,
    TEST_DCD_TWICE_ON_PLUN,
    TEST_DCD_TWICE_ON_VLUN,
    /*** DK_CAPI_VERIFY ***/
    TEST_DCV_INVALID_DEVNO,
    TEST_DCV_INVALID_FLAGS,
    TEST_DCV_INVALID_RES_HANDLE,
    TEST_DCV_UNEXPECTED_ERR,
    TEST_DCV_NO_ERR,
    TEST_DCV_UNEXPECTED_ERR_VLUN,
    TEST_DCV_VLUN_RST_FlAG,
    TEST_DCV_VLUN_TUR_FLAG,
    TEST_DCV_VLUN_INQ_FLAG,
    TEST_DCV_VLUN_HINT_SENSE,
    TEST_DCV_PLUN_RST_FlAG,
    TEST_DCV_PLUN_TUR_FLAG,
    TEST_DCV_PLUN_INQ_FLAG,
    TEST_DCV_PLUN_HINT_SENSE,
    TEST_DCV_PLUN_RST_FlAG_EEH,
    /********** DK_CAPI_LOG_EVENT ********/
    TEST_DCLE_VALID_VALUES,
    TEST_DCLE_DK_LF_TEMP,
    TEST_DCLE_DK_LF_PERM,
    TEST_DCLE_DK_FL_HW_ERR,
    TEST_DCLE_DK_FL_SW_ERR,
    /*********** ERR CASE **************/
    TEST_VSPIO_EEHRECOVERY,
    TEST_DSPIO_EEHRECOVERY,
    TEST_IOCTL_FCP,
    TEST_MMIO_ERRCASE1,
    TEST_MMIO_ERRCASE2,
    TEST_MMIO_ERRCASE3,
    TEST_SPIO_KILLPROCESS,
    TEST_SPIO_EXIT,
    TEST_IOCTL_SPIO_ERRCASE,
    TEST_CFDISK_CTXS_DIFF_DEVNO,
    TEST_ATTACH_REUSE_DIFF_PROC,
    TEST_DETACH_DIFF_PROC,
    TEST_FC_PR_RESET_VLUN,
    TEST_FC_PR_RESET_PLUN,
    /*********** EXCP CASE *************/
    EXCP_VLUN_DISABLE,
    EXCP_PLUN_DISABLE,
    EXCP_VLUN_VERIFY,
    EXCP_PLUN_VERIFY,
    EXCP_VLUN_INCREASE,
    EXCP_VLUN_REDUCE,
    EXCP_PLUN_UATTENTION,
    EXCP_VLUN_UATTENTION,
    EXCP_EEH_SIMULATION,
    EXCP_INVAL_DEVNO,
    EXCP_INVAL_CTXTKN,
    EXCP_INVAL_RSCHNDL,
    EXCP_DISK_INCREASE,
    
    /*CLONE */
    TEST_DK_CAPI_CLONE,
G_ioctl_7_1_119,
E_ioctl_7_1_120,
E_ioctl_7_1_174,
E_ioctl_7_1_175,
E_ioctl_7_1_180,
E_ioctl_7_1_1801,
G_ioctl_7_1_181,
E_ioctl_7_1_182,
G_ioctl_7_1_187,
E_ioctl_7_1_212,
E_ioctl_7_1_213,
E_ioctl_7_1_215,
E_ioctl_7_1_216,
G_ioctl_7_1_188,
G_ioctl_7_1_189,
E_ioctl_7_1_190,
G_ioctl_7_1_191,
G_ioctl_7_1_192,
G_ioctl_7_1_193_1,
G_ioctl_7_1_193_2,
G_ioctl_7_1_196,
E_ioctl_7_1_197,
E_ioctl_7_1_198,
E_ioctl_7_1_209,
E_ioctl_7_1_210,
E_ioctl_7_1_211,
E_ioctl_7_1_214,	
E_test_SCSI_CMDS,
E_TEST_CTX_RESET,
M_TEST_7_5_13_1,
M_TEST_7_5_13_2,
G_TEST_MAX_CTX_PLUN,
G_TEST_MAX_CTX_ONLY,
G_TEST_MAX_CTX_0_VLUN,
E_CAPI_LINK_DOWN,
G_ioctl_7_1_203,
G_TEST_MAX_VLUNS,
G_TEST_MAX_CTX_IO_NOFLG,
E_TEST_MAX_CTX_CRSS_LIMIT,
}mc_test_t;

int validateFunction(struct validatePckt *);
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
int test_large_trnsfr_boundary();
int test_large_transfer();
int max_ctx_rcvr_except_last_one();
int max_ctx_rcvr_last_one_no_rcvr();


int open_dev(char*, int);
int mc_test_engine(mc_test_t);
int ctx_init(struct ctx *p_ctx);
int ctx_init2(struct ctx *p_ctx, char *dev, __u64 flags, dev64_t devno);
int ctx_reinit(struct ctx *p_ctx);
int ctx_close(struct ctx *p_ctx);
void ctx_close_thread(void *);
int get_fvt_dev_env();
int cflash_interrupt_number(void );

int test_init(struct ctx *p_ctx);
void *ctx_rrq_rx(void *arg);
int send_write(struct ctx *p_ctx, __u64 start_lba,
                __u64 stride, __u64 data);
int send_single_write(struct ctx *p_ctx, __u64 vlba, __u64 data);
int send_read(struct ctx *p_ctx, __u64 start_lba, __u64 stride);
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
int check_status(volatile sisl_ioasa_t *p_ioasa);
void send_single_cmd(struct ctx *p_ctx);
int send_rw_rcb(struct ctx *p_ctx, struct rwbuf *p_rwb,
                 __u64 start_lba, __u64 stride,
                int align, int where);
int send_rw_lsize(struct ctx *p_ctx, struct rwlargebuf *p_rwb,
                   __u64 start_lba, __u32 blocks);
int send_rw_shm_rcb(struct ctx *p_ctx, struct rwshmbuf *p_rwb,
                     __u64 vlba);
void hexdump(void *data, long len, const char *hdr);
int get_flash_disks(struct flash_disk *disks, int type);
int generate_unexpected_error(void);

int test_all_ioctl_invalid_version(void);
int test_invalid_version_ioctl(int flag);
#ifdef _AIX
int test_dcqp_ioctl(int flag);
int test_dcqp_error_ioctl(int flag);
#endif
int test_dca_ioctl(int flag);
int test_dca_error_ioctl(int flag);
int test_dcrc_ioctl(int flag);
int test_dcud_error_ioctl(int flag);
int test_dcud_ioctl(int flag);
int test_dcuv_error_ioctl(int flag);
int test_dcuv_ioctl(int flag);
int test_dcvr_error_ioctl(int flag);
int test_dcvr_ioctl(int flag);
int test_dcd_ioctl(int flag);
int test_dcv_error_ioctl(int flag);
int test_dcv_ioctl(int flag);
int test_dcle_ioctl(int flag);
int test_dcr_ioctl(int flag);

int test_vSpio_eehRecovery(int);
int test_ioctl_fcp();
int test_mmio_errcase(int);
int test_detach_diff_proc();
int test_attach_reuse_diff_proc();
int test_cfdisk_ctxs_diff_devno();
int test_ioctl_spio_errcase();
int test_spio_exit();
int test_spio_killprocess();
int test_dSpio_eehRecovery(int);

int test_dcqexp_ioctl();
int test_dcqexp_invalid();

typedef struct do_io_thread_arg
{
    struct ctx *p_ctx;
    __u64 stride;
    int loopCount;
} do_io_thread_arg_t;

void * do_io_thread(void * );



int get_nonflash_disk(char *, dev64_t *);

//new functions for ioctl
#ifdef _AIX
int ioctl_dk_capi_query_path(struct ctx *p_ctx);
#endif
int ioctl_dk_capi_attach(struct ctx *p_ctx);
int ioctl_dk_capi_detach(struct ctx *p_ctx);
int ioctl_dk_capi_udirect(struct ctx *p_ctx);
int ioctl_dk_capi_uvirtual(struct ctx *p_ctx);
int ioctl_dk_capi_release(struct ctx *p_ctx);
int ioctl_dk_capi_vlun_resize(struct ctx *p_ctx);
int ioctl_dk_capi_verify(struct ctx *p_ctx);
int ioctl_dk_capi_log(struct ctx *p_ctx, char *s_data);
int ioctl_dk_capi_recover_ctx(struct ctx *p_ctx);
int ioctl_dk_capi_query_exception(struct ctx *p_ctx);
int ioctl_dk_capi_clone(struct ctx *p_ctx, uint64_t old_ctx_id,int src_adap_fd);

int close_res(struct ctx *p_ctx);
int create_res(struct ctx *p_ctx);
int mc_stat1(struct ctx *p_ctx, mc_stat_t *stat);
int mc_size1(struct ctx *p_ctx, __u64 chunk, __u64 *actual_size);
int create_resource(struct ctx *p_ctx, __u64 nlba,
                     __u64 flags, __u16 lun_type);
int vlun_resize(struct ctx *p_ctx, __u64 nlba);
int wait4all();
int wait4allOnlyRC();
int do_io(struct ctx *p_ctx, __u64 stride);
int do_io_nocompare(struct ctx *p_ctx, __u64 stride);
int do_large_io(struct ctx *p_ctx, struct rwlargebuf *rwbuf, __u64 size);
int do_poll_eeh(struct ctx *);
int do_eeh(struct ctx *);
int diskInSameAdapater( char * );
int diskInDiffAdapater( char * );
bool check_afu_reset(struct ctx *p_ctx);
#ifdef _AIX
int ioctl_dk_capi_query_path_check_flag(struct ctx *p_ctx,
                                        int flag1, int flag2);
int setRUnlimited();
char * diskWithoutDev(char * source , char * destination );
#endif
#ifndef _AIX
int turnOffTestCase( char * );
void setupFVT_ENV( void );
int diskSizeCheck( char * , float );
void identifyHostType( void );
int diskToWWID ( char * WWID);
int WWIDtoDisk ( char * WWID);
#endif
int test_spio_vlun(int);
int test_spio_plun();
int test_fc_port_reset_vlun();
int test_fc_port_reset_plun();
int test_spio_lun(char *dev, dev64_t devno,
                   __u16 lun_type, __u64 chunk);
int test_spio_pluns(int cmd);
int test_spio_vluns(int cmd);
int test_spio_direct_virtual();
int max_ctx_max_res(int cmd);
int test_spio_attach_detach(int cmd);
int test_clone_ioctl(int cmd);
__u64 get_disk_last_lba(char *dev, dev64_t devno, uint64_t *chunk_size);
int compare_size(uint64_t act, uint64_t exp);
int compare_flags(uint64_t act, uint64_t exp);
int traditional_io(int disk_num);
int mc_init();
int mc_term();
//int max_ctx_res(struct ctx *p_ctx);
int create_multiple_vluns(struct ctx *p_ctx);
int ioctl_7_1_119_120(int);
int ioctl_7_1_174_175(int);
int test_traditional_IO(int,int);
int ioctl_7_1_188(int);
int ioctl_7_1_191(int);
int ioctl_7_1_192(int);
int ioctl_7_1_196();
int ioctl_7_1_197();
int ioctl_7_1_198();
int ioctl_7_1_209();
int ioctl_7_1_210();
int ioctl_7_1_211();
int ioctl_7_1_215();
int ioctl_7_1_216();    
void *create_lun1(void *arg );
int create_direct_lun(struct ctx *p_ctx);
int capi_open_close(struct ctx *, char *);
int create_direct_lun(struct ctx *p_ctx);
int create_vluns_216(char *dev, 
                     __u16 lun_type, __u64 chunk, struct ctx *p_ctx);
int create_vlun_215(char *dev, dev64_t devno,
                     __u16 lun_type, __u64 chunk, struct ctx *p_ctx);
int ioctl_dk_capi_attach_reuse(struct ctx *, struct ctx * , __u16 );
int ctx_init_internal(struct ctx *p_ctx,
                       __u64 flags, dev64_t devno);
void handleSignal(int sigNo);
void sig_handle(int sig);
int test_scsi_cmds();
int do_write_or_read(struct ctx *p_ctx, __u64 stride, int do_W_R);
int allocate_buf(struct rwlargebuf *rwbuf, __u64 size);
void deallocate_buf(struct rwlargebuf *rwbuf);
int ioctl_dk_capi_attach_reuse_all_disk( );
int ioctl_dk_capi_attach_reuse_loop(struct ctx *p_ctx,struct ctx *p_ctx_1 );
int test_ctx_reset();
int set_spio_mode();
int ioctl_7_5_13(int ops);
int keep_doing_eeh_test(struct ctx *p_ctx);
int max_ctx_on_plun(int );
int max_ctx_n_res_nchan(int nchan);
int ioctl_7_1_193_1( int flag  );
int ioctl_7_1_193_2( int flag );
int max_ctx_n_res_190();
int ctx_init_reuse(struct ctx *p_ctx);
int max_ctx_res_190(struct ctx *p_ctx);
int ioctl_7_1_203();
int ioctl_7_1_190(  ); 
int call_attach_diff_devno();
int max_vlun_on_a_ctx();
void displayBuildinfo();
int get_max_res_hndl_by_capacity(char *dev);
int max_ctx_cross_limit();
#endif /*__CFLASH_TEST_H__ */
