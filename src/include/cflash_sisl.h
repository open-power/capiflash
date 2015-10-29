/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_sisl.h $                                   */
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


/******************************************************************************
 * COMPONENT_NAME: sysxcflash
 *
 * FUNCTION: CAPI Flash sislite specification definitions.
 *
 * FUNCTIONS:   NONE
 *
 *****************************************************************************/

#ifndef _SISLITE_H
#define _SISLITE_H

#include <unistd.h>
#if 0
#include <inttypes.h>
#endif
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif /* !_AIX && ! _MACOSX */
#if defined(_AIX) || defined(_MACOSX)
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef short __s16;
typedef unsigned int __u32;
typedef int __s32;
typedef unsigned long long __u64;
#endif /* AIX */

/************************************************************************/
/* Sislite AFU Alignment                                                */
/************************************************************************/
#define SISL_BYTE_ALIGN 16
#define SISL_ALIGN_MASK 0xFFFFFFFFFFFFFFF0ULL /* mask off the offset   */


/************************************************************************/
/* Sislite Macros                                                       */
/************************************************************************/
#define SISL_PER_CONTEXT_MMIO_SIZE 0x10000    /* Per context MMIO space*/
#define SISL_TOTAL_MMIO_SIZE       0X20A0000  /* Total MMIO space      */

#define SISL_NUM_FC_PORTS   2
#define SISL_MAX_CONTEXT  512           /* how many contexts per afu */

/* make the fp byte */
#define SISL_RHT_FP(fmt, perm) (((fmt) << 4) | (perm))

/* make the fp byte for a clone from a source fp and clone flags
 * flags must be only 2 LSB bits. 
 */
#define SISL_RHT_FP_CLONE(src_fp, clone_flags) ((src_fp) & (0xFC | (clone_flags)))

/* extract the perm bits from a fp */
#define SISL_RHT_PERM(fp) ((fp) & 0x3)

#define SISL_RHT_PERM_READ  0x01u
#define SISL_RHT_PERM_WRITE 0x02u

/* AFU Sync Commands */
#define SISL_AFU_SYNC_CMD 0xC0

/* AFU Sync Mode byte */
#define SISL_AFU_LW_SYNC 0x0u
#define SISL_AFU_HW_SYNC 0x1u
#define SISL_AFU_GSYNC   0x2u

/* AFU Task Management Function Commands */
#define SISL_TMF_LUN_RESET    0x1
#define SISL_TMF_CLEAR_ACA    0x2

/************************************************************************/
/* IOARCB: 64 bytes, min 16 byte alignment required                     */
/************************************************************************/

typedef __u16 ctx_hndl_t;
typedef __u32 res_hndl_t;


/***************************************************************************
 *
 * Below this point are data structures that make up the adapter API.
 *
 * We do not want the compiler to insert any extra padding in these
 * structures.
 *
 ***************************************************************************/
#pragma pack(1)


/*
 * IOARCB strcuture for AFU 
 */
typedef struct sisl_ioarcb_s { 
    __u16 ctx_id;  /* ctx_hndl_t */
    __u16 req_flags;
#define SISL_REQ_FLAGS_RES_HNDL       0x8000u /* bit 0 (MSB) */
#define SISL_REQ_FLAGS_PORT_LUN_ID    0x0000u

#define SISL_REQ_FLAGS_SUP_UNDERRUN   0x4000u /* bit 1 */

#define SISL_REQ_FLAGS_TIMEOUT_SECS   0x0000u /* bits 8,9 */
#define SISL_REQ_FLAGS_TIMEOUT_MSECS  0x0040u
#define SISL_REQ_FLAGS_TIMEOUT_USECS  0x0080u
#define SISL_REQ_FLAGS_TIMEOUT_CYCLES 0x00C0u

#define SISL_REQ_FLAGS_TMF            0x0004u /* bit 13 */
#define SISL_REQ_FLAGS_AFU_CMD        0x0002u /* bit 14 */

#define SISL_REQ_FLAGS_HOST_WRITE     0x0001u /* bit 15 (LSB) */
#define SISL_REQ_FLAGS_HOST_READ      0x0000u

    union {
	__u32 res_hndl;  /* res_hndl_t */
	__u32 port_sel;  /* this is a selection mask:
			  * 0x1 -> port#0 can be selected,
			  * 0x2 -> port#1 can be selected.
			  * Can be bitwise ORed.
			  */
    };
    __u64 lun_id;
    __u32 data_len;  /* 4K for read/write */
    __u32 ioadl_len;
    union {
	__u64 data_ea; /* min 16 byte aligned */
	__u64 ioadl_ea;
    };
    __u8 msi; /* LISN to send on RRQ write */
#define SISL_MSI_PSL_XLATE         0  /* reserved for PSL */
#define SISL_MSI_SYNC_ERROR        1  /* recommended for AFU sync error */
#define SISL_MSI_RRQ_UPDATED       2  /* recommended for IO completion */
#define SISL_MSI_ASYNC_ERROR       3  /* master only - for AFU async error */
    /* The above LISN allocation permits user contexts to use 3 interrupts.
     * Only master needs 4. This saves IRQs on the system.
     */

    __u8 rrq;  /* 0 for a single RRQ */
    __u16 timeout; /* in units specified by req_flags */
    __u32 rsvd1;
    __u8 cdb[16]; /* must be in big endian */
    __u64 rsvd2;
} sisl_ioarcb_t;



struct sisl_rc { 
    __u8  flags;
#define SISL_RC_FLAGS_SENSE_VALID         0x80u
#define SISL_RC_FLAGS_FCP_RSP_CODE_VALID  0x40u
#define SISL_RC_FLAGS_OVERRUN             0x20u
#define SISL_RC_FLAGS_UNDERRUN            0x10u

    __u8  afu_rc;
#define SISL_AFU_RC_RHT_INVALID           0x01u /* user error */
#define SISL_AFU_RC_RHT_UNALIGNED         0x02u /* should never happen */
#define SISL_AFU_RC_RHT_OUT_OF_BOUNDS     0x03u /* user error */
#define SISL_AFU_RC_RHT_DMA_ERR           0x04u /* see afu_extra
						   may retry if afu_retry is off
						   possible on master exit
						*/
#define SISL_AFU_RC_RHT_RW_PERM           0x05u /* no RW perms, user error */
#define SISL_AFU_RC_LXT_UNALIGNED         0x12u /* should never happen */
#define SISL_AFU_RC_LXT_OUT_OF_BOUNDS     0x13u /* user error */
#define SISL_AFU_RC_LXT_DMA_ERR           0x14u /* see afu_extra
						   may retry if afu_retry is off
						   possible on master exit
						*/
#define SISL_AFU_RC_LXT_RW_PERM           0x15u /* no RW perms, user error */

#define SISL_AFU_RC_NOT_XLATE_HOST        0x1au /* possible when master exited */

    /* NO_CHANNELS means the FC ports selected by dest_port in
     * IOARCB or in the LXT entry are down when the AFU tried to select 
     * a FC port. If the port went down on an active IO, it will set
     * fc_rc to =0x54(NOLOGI) or 0x57(LINKDOWN) instead.
     */
#define SISL_AFU_RC_NO_CHANNELS           0x20u /* see afu_extra, may retry */
#define SISL_AFU_RC_CAP_VIOLATION         0x21u /* either user error or
						   afu reset/master restart
						*/
#define SISL_AFU_RC_SYNC_IN_PROGRESS      0x22u /* AFU Sync issued when previous
                                                   sync is in progress
						*/
#define SISL_AFU_RC_OUT_OF_DATA_BUFS      0x30u /* always retry */
#define SISL_AFU_RC_DATA_DMA_ERR          0x31u /* see afu_extra
						   may retry if afu_retry is off
						*/
#define SISL_AFU_RC_RCB_TIMEOUT_PRE_FC    0x50u /* RCB timeout before reaching
                                                   FC layers inside AFU
                                                */
#define SISL_AFU_RC_RCB_TIMEOUT_POST_FC   0x51u /* RCB timeout after reaching
                                                   FC layers inside AFU
                                                */

    __u8  scsi_rc;  /* SCSI status byte, retry as appropriate */
#define SISL_SCSI_RC_CHECK                0x02u
#define SISL_SCSI_RC_BUSY                 0x08u

    __u8  fc_rc;    /* retry */
    /*
     * We should only see fc_rc=0x57 (LINKDOWN) or 0x54(NOLOGI)
     * for commands that are in flight when a link goes down or is logged out. 
     * If the link is down or logged out before AFU selects the port, either 
     * it will choose the other port or we will get afu_rc=0x20 (no_channel)
     * if there is no valid port to use.
     *
     * ABORTPEND/ABORTOK/ABORTFAIL/TGTABORT can be retried, typically these 
     * would happen if a frame is dropped and something times out.  
     * NOLOGI or LINKDOWN can be retried if the other port is up.  
     * RESIDERR can be retried as well.
     *
     * ABORTFAIL might indicate that lots of frames are getting CRC errors.  
     * So it maybe retried once and reset the link if it happens again.
     * The link can also be reset on the CRC error threshold interrupt.
     */
#define SISL_FC_RC_ABORTPEND              0x52 /* exchange timeout or abort request */
#define SISL_FC_RC_WRABORTPEND            0x53 /* due to write XFER_RDY invalid */
#define SISL_FC_RC_NOLOGI                 0x54 /* port not logged in, in-flight cmds */
#define SISL_FC_RC_NOEXP                  0x55 /* FC protocol error or HW bug */
#define SISL_FC_RC_INUSE                  0x56 /* tag already in use, HW bug */
#define SISL_FC_RC_LINKDOWN               0x57 /* link down, in-flight cmds */
#define SISL_FC_RC_ABORTOK                0x58 /* pending abort completed w/success */
#define SISL_FC_RC_ABORTFAIL              0x59 /* pending abort completed w/fail */
#define SISL_FC_RC_RESID                  0x5A /* ioasa underrun/overrun flags set */
#define SISL_FC_RC_RESIDERR               0x5B /* actual data len does not match SCSI
						  reported len, possbly due to dropped
						  frames */
#define SISL_FC_RC_TGTABORT               0x5C /* command aborted by target */

};


#define SISL_SENSE_DATA_LEN     20            /* Sense data length         */

/* 
 * IOASA: 64 bytes & must follow IOARCB, min 16 byte alignment required 
 */

typedef struct sisl_ioasa_s { 
    union {
	struct sisl_rc rc;
	__u32 ioasc;
#define SISL_IOASC_GOOD_COMPLETION        0x00000000u
    };
    __u32 resid;
    __u8  port;
    __u8  afu_extra;
    /* when afu_rc=0x04, 0x14, 0x31 (_xxx_DMA_ERR):
     * afu_exta contains PSL response code. Useful codes are:
     */
#define SISL_AFU_DMA_ERR_PAGE_IN      0x0A  /* AFU_retry_on_pagein SW_Implication
					     *  Enabled            N/A
					     *  Disabled           retry
					     */
#define SISL_AFU_DMA_ERR_INVALID_EA   0x0B  /* this is a hard error
					     * afu_rc              SW_Implication
					     * 0x04, 0x14          Indicates master exit.
					     * 0x31                user error.
					     */
    /* when afu rc=0x20 (no channels):
     * afu_extra bits [4:5]: available portmask,  [6:7]: requested portmask.
     */
#define SISL_AFU_NO_CLANNELS_AMASK(afu_extra) (((afu_extra) & 0x0C) >> 2)
#define SISL_AFU_NO_CLANNELS_RMASK(afu_extra) ((afu_extra) & 0x03)

    __u8 scsi_extra;
    __u8 fc_extra;
    __u8 sense_data[SISL_SENSE_DATA_LEN];

    union {
	__u64 host_use[4];
	__u8  host_use_b[32];
	__u64 next_cmd[4];
    };
} sisl_ioasa_t;


/* single request+response block: 128 bytes.
   cache line aligned for better performance.
*/
typedef struct sisl_iocmd_s { 
    sisl_ioarcb_t rcb;
    sisl_ioasa_t sa;
} sisl_iocmd_t __attribute__ ((aligned (128)));

#define SISL_RESP_HANDLE_T_BIT        0x1ull /* Toggle bit */
#define SISL_RESP_HANDLE_BADDR_MASK   0xFull /* Toggle bit */

/* MMIO space is required to support only 64-bit access */

/* per context host transport MMIO  */
struct sisl_host_map {
    __u64 endian_ctrl;
#define SISL_ENDIAN_CTRL_BE             0x8000000000000080ull
    
__u64 intr_status; /* this sends LISN# programmed in ctx_ctrl.
			* Only recovery in a PERM_ERR is a context exit since
			* there is no way to tell which command caused the error.
			*/
#define SISL_ISTATUS_PERM_ERR_CMDROOM    0x0010ull /* b59, user error */
#define SISL_ISTATUS_PERM_ERR_RCB_READ   0x0008ull /* b60, user error */
#define SISL_ISTATUS_PERM_ERR_SA_WRITE   0x0004ull /* b61, user error */
#define SISL_ISTATUS_PERM_ERR_RRQ_WRITE  0x0002ull /* b62, user error */
    /* Page in wait accessing RCB/IOASA/RRQ is reported in b63. 
     * Same error in data/LXT/RHT access is reported via IOASA.
     */
#define SISL_ISTATUS_TEMP_ERR_PAGEIN     0x0001ull /* b63,  can be generated
						    * only when AFU auto retry is 
						    * disabled. If user can determine
						    * the command that caused the error, 
						    * it can be retried.
						    */
#define SISL_ISTATUS_UNMASK  (0x001Full)            /* 1 means unmasked */
#define SISL_ISTATUS_MASK    ~(SISL_ISTATUS_UNMASK) /* 1 means masked */

    __u64 intr_clear;
    __u64 intr_mask;
    __u64 ioarrin;   /* only write what cmd_room permits */
#define SISL_IOARRIN_CTX_RST  0x01
    __u64 rrq_start; /* start & end are both inclusive */
    __u64 rrq_end;   /* write sequence: start followed by end */
    __u64 cmd_room;
    __u64 ctx_ctrl;  /* least signiifcant byte or b56:63 is LISN# */
    __u64 mbox_w;    /* restricted use */
};

/* per context provisioning & control MMIO */
struct sisl_ctrl_map {
    __u64 rht_start;
    __u64 rht_cnt_id;
    /* both cnt & ctx_id args must be ull */
#define SISL_RHT_CNT_ID(cnt, ctx_id)  (((cnt) << 48) | ((ctx_id) << 32))

    __u64 ctx_cap; /* afu_rc below is when the capability is violated */
#define SISL_CTX_CAP_PROXY_ISSUE       0x8000000000000000ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_REAL_MODE         0x4000000000000000ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_HOST_XLATE        0x2000000000000000ull /* afu_rc 0x1a */
#define SISL_CTX_CAP_PROXY_TARGET      0x1000000000000000ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_AFU_CMD           0x0000000000000008ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_GSCSI_CMD         0x0000000000000004ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_WRITE_CMD         0x0000000000000002ull /* afu_rc 0x21 */
#define SISL_CTX_CAP_READ_CMD          0x0000000000000001ull /* afu_rc 0x21 */
    __u64 mbox_r;
};

/* single copy global regs */
struct sisl_global_regs {
    __u64 aintr_status;
    /* In surelock, each FC port/link gets a byte of status */
#define SISL_ASTATUS_FC0_OTHER    0x8000ull /* b48, other err, FC_ERRCAP[31:20] */
#define SISL_ASTATUS_FC0_LOGO     0x4000ull /* b49, target sent FLOGI/PLOGI/LOGO
					       while logged in */
#define SISL_ASTATUS_FC0_CRC_T    0x2000ull /* b50, CRC threshold exceeded */
#define SISL_ASTATUS_FC0_LOGI_R   0x1000ull /* b51, login state mechine timed out
					       and retrying */
#define SISL_ASTATUS_FC0_LOGI_F   0x0800ull /* b52, login failed, FC_ERROR[19:0] */
#define SISL_ASTATUS_FC0_LOGI_S   0x0400ull /* b53, login succeeded */
#define SISL_ASTATUS_FC0_LINK_DN  0x0200ull /* b54, link online to offline */
#define SISL_ASTATUS_FC0_LINK_UP  0x0100ull /* b55, link offline to online */

#define SISL_ASTATUS_FC1_OTHER    0x0080ull /* b56 */
#define SISL_ASTATUS_FC1_LOGO     0x0040ull /* b57 */
#define SISL_ASTATUS_FC1_CRC_T    0x0020ull /* b58 */
#define SISL_ASTATUS_FC1_LOGI_R   0x0010ull /* b59 */
#define SISL_ASTATUS_FC1_LOGI_F   0x0008ull /* b60 */
#define SISL_ASTATUS_FC1_LOGI_S   0x0004ull /* b61 */
#define SISL_ASTATUS_FC1_LINK_DN  0x0002ull /* b62 */
#define SISL_ASTATUS_FC1_LINK_UP  0x0001ull /* b63 */

#define SISL_ASTATUS_UNMASK       0xFFFFull              /* 1 means unmasked */
#define SISL_ASTATUS_MASK         ~(SISL_ASTATUS_UNMASK) /* 1 means masked */

    __u64 aintr_clear;
    __u64 aintr_mask;
    __u64 afu_ctrl;
#define SISL_CTRL_LOCAL_LUN             0x01ull    /*b63 */
    __u64 afu_hb;
    __u64 afu_scratch_pad;
    __u64 afu_port_sel;
#define SISL_PORT_SEL_PORT0             0x01ull    /*b63 */    
#define SISL_PORT_SEL_PORT1             0x02ull    /*b62 */
    __u64 afu_config;
#define SISL_CONFIG_MBOX_CLR_READ       0x00010ull /* b59 */
#define SISL_CONFIG_LE_MODE             0x00020ull /* b58 */
#define SISL_CONFIG_RRQ_PAGIN_RETRY     0x00100ull /* b55 */
#define SISL_CONFIG_IOASA_PAGIN_RETRY   0x00200ull /* b54 */
#define SISL_CONFIG_RESRC_ERR_RETRY     0x00400ull /* b53 */
#define SISL_CONFIG_DATA_PAGIN_RETRY    0x00800ull /* b52 */
#define SISL_CONFIG_RHT_PAGIN_TRYRY     0x01000ull /* b51 */
#define SISL_CONFIG_LXT_PAGIN_RETRY     0x02000ull /* b50 */
#define SISL_CONFIG_IOARCB_PAGIN_RETRY  0x04000ull /* b49 */
    __u64 rrin_read_to;                 /* RRIN read timeout */
    __u64 cont_retry_to;                /* Continue response retry timeout */
    __u64 rsvd[0xf6];
    __u64 afu_version;
};


struct sisl_global_map {
    union {
	struct sisl_global_regs regs;
	char page0[0x1000];                      /* page 0 */
    };

    char page1[0x1000];                        /* page 1 */
    __u64 fc_regs[SISL_NUM_FC_PORTS][512]; /* pages 2 & 3, see afu_fc.h */
    __u64 fc_port[SISL_NUM_FC_PORTS][512]; /* pages 4 & 5 (lun tbl) */

    char page6_15[0xA000];
};


struct surelock_afu_map {
    union {
	struct sisl_host_map host;
	char harea[0x10000]; /* 64KB each */
    } hosts[SISL_MAX_CONTEXT];

    union {
	struct sisl_ctrl_map ctrl;
	char carea[128];    /* 128B each */
    } ctrls[SISL_MAX_CONTEXT];

    union {
	struct sisl_global_map global;
	char garea[0x10000]; /* 64KB single block */
    };
    char reserved[0x20000];
    char afu_dbg[0x20000];
    char fc0_dbg[0x20000];
    char fc1_dbg[0x20000];
};


/* LBA translation control blocks */

typedef struct sisl_lxt_entry {
    __u64 rlba_base; /* bits 0:47 is base
		      * b48:55 is lun index
		      * b58:59 is write & read perms
		      * (if no perm, afu_rc=0x15)
		      * b60:63 is port_sel mask
		      */

} sisl_lxt_entry_t;


typedef struct sisl_vrht_entry {
    sisl_lxt_entry_t  *lxt_start;
    __u32  lxt_cnt;
    __u16  rsvd;
    __u8   fp;  /* format & perm nibbles.
		 * (if no perm, afu_rc=0x05)
		 */
    __u8   nmask;
} sisl_vrht_entry_t __attribute__ ((aligned (16)));

typedef struct sisl_prht_entry {
    __u64  lun_id;
    __u8   valid;
    __u8   rsvd[5];
    __u8   fp;  /* format & perm nibbles.
		 * (if no perm, afu_rc=0x05)
		 */
    __u8   pmask;
} sisl_prht_entry_t __attribute__ ((aligned (16)));

typedef union sisl_rht_entry {
     sisl_vrht_entry_t vrht;
     sisl_prht_entry_t prht;
} sisl_rht_entry_t;



#pragma pack(pop)


#endif /* _SISLITE_H */

