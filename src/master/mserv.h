/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/mserv.h $                                          */
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
#ifndef _MSERVE_H
#define _MSERVE_H

#include <sislite.h>
#include <block_alloc.h>
#include <mclient.h>
#include <signal.h>
#include <time.h>

#include <cxl.h>
#ifndef _AIX
#include <sys/epoll.h>
#else 
#include <poll.h>
#endif /* !_AIX */
#include <sys/socket.h>
#include <sys/un.h>

/*
 * Terminology: use afu (and not adapter) to refer to the HW. 
 * Adapter is the entire slot and includes PSL out of which
 * only the AFU is visible to user space.
 */

/* Chunk size parms: note sislite minimum chunk size is
   0x10000 LBAs corresponding to a NMASK or 16.
*/
#define MC_RHT_NMASK      16                  /* in bits */
#define MC_CHUNK_SIZE     (1 << MC_RHT_NMASK) /* in LBAs, see mclient.h */
#define MC_CHUNK_SHIFT    MC_RHT_NMASK  /* shift to go from LBA to chunk# */
#define MC_CHUNK_OFF_MASK (MC_CHUNK_SIZE - 1) /* apply to LBA get offset 
						 into a chunk */

/* Sizing parms: same context can be registered multiple times. 
   Therefore we allow MAX_CONNS > MAX_CONTEXT.
*/
#define MAX_CONTEXT  SURELOCK_MAX_CONTEXT /* num contexts per afu */
#define MAX_RHT_PER_CONTEXT 16            /* num resource hndls per context */
#define MAX_CONNS (MAX_CONTEXT*2)         /* num client connections per AFU */
#define MAX_CONN_TO_POLL 64               /* num fds to poll once */
#define NUM_RRQ_ENTRY    16               /* for master issued cmds */
#define NUM_CMDS         16               /* must be <= NUM_RRQ_ENTRY */
#define NUM_FC_PORTS     SURELOCK_NUM_FC_PORTS  /* ports per AFU */



/* LXT tables are allocated dynamically in groups. This is done to
   avoid a malloc/free overhead each time the LXT has to grow
   or shrink.

   Based on the current lxt_cnt (used), it is always possible to 
   know how many are allocated (used+free). The number of allocated 
   entries is not stored anywhere.

   The LXT table is re-allocated whenever it needs to cross into 
   another group.
*/
#define LXT_GROUP_SIZE          8
#define LXT_NUM_GROUPS(lxt_cnt) (((lxt_cnt) + 7)/8) /* alloc'ed groups */

/* port online retry intervals */
#define FC_PORT_STATUS_RETRY_CNT 100             // 100 100ms retries = 10 seconds
#define FC_PORT_STATUS_RETRY_INTERVAL_US 100000  // microseconds


/* flags in IOA status area for host use */
#define B_DONE       0x01
#define B_ERROR      0x02  /* set with B_DONE */
#define B_TIMEOUT    0x04  /* set with B_DONE & B_ERROR */

/* AFU command timeout values */
#define MC_DISCOVERY_TIMEOUT 5 /* 5 secs */
#define MC_AFU_SYNC_TIMEOUT  5 /* 5 secs */

/* AFU command retry limit */
#define MC_RETRY_CNT         5 /* sufficient for SCSI check and
				  certain AFU errors */

/* AFU command room retry limit */
#define MC_ROOM_RETRY_CNT    10 

/* AFU heartbeat periodic timer */
#define MC_HB_PERIOD 5 /* 5 secs */

/* FC CRC clear periodic timer */
#define MC_FC_PERIOD  300  /* 5 mins */
#define MC_CRC_THRESH 100  /* threshold in 5 mins */

#define CL_SIZE             128             /* Processor cache line size */
#define CL_SIZE_MASK        0x7F            /* Cache line size mask */

struct scsi_inquiry_page_83_hdr
{
    __u8  peri_qual_dev_type;
    __u8  page_code;
    __u16 adtl_page_length; /* not counting 4 byte hdr */
    /* Identification Descriptor list */
};

struct scsi_inquiry_p83_id_desc_hdr
{
    __u8  prot_code;       /* Protocol Identifier & Code Set */
#define TEXAN_PAGE_83_DESC_PROT_CODE             0x01u
    __u8  assoc_id;        /* PIV/Association/Identifier type */
#define TEXAN_PAGE_83_ASSC_ID_LUN_WWID           0x03u
    __u8 reserved;
    __u8 adtl_id_length;
    /* Identifier Data */
};

/* 
 * A resource handle table (RHT) can be pointed to by multiple contexts.
 * This happens when one context is duped to another. 
 * W/o dup, each context has its own resource handles that is visible 
 * only from that context.
 *
 * The rht_info refers to all resource handles of a context and not to
 * a particular RHT entry or a single resource handle.
 */
typedef struct rht_info 
{
    sisl_rht_entry_t *rht_start; /* initialized at startup */
    int ref_cnt;   /* num ctx_infos pointing to me */
} rht_info_t;


/* Single AFU context can be pointed to by multiple client connections. 
 * The client can create multiple endpoints (mc_hndl_t) to the same 
 * (context + AFU).
 */
typedef struct ctx_info
{
    volatile struct sisl_ctrl_map *p_ctrl_map; /* initialized at startup */

    /* The rht_info pointer is initialized when the context is first 
       registered, can be changed on dup.
    */
    rht_info_t *p_rht_info;

    /* all dup'ed contexts are in a doubly linked circular list */
    struct ctx_info *p_forw; 
    struct ctx_info *p_next;

    int ref_cnt;   /* num conn_infos pointing to me */
} ctx_info_t;

/* forward decls */
struct capikv_ini_elm;
typedef struct afu afu_t;
typedef struct conn_info conn_info_t;
typedef struct mc_req mc_req_t;
typedef struct mc_resp mc_resp_t;
typedef int (*rx_func)(afu_t *p_afu, conn_info_t *p_conn_info, 
		       mc_req_t *p_req, mc_resp_t *p_resp);


/* On accept, the server allocates a connection info. 
   A connection can be associated with at most one AFU context.
 */
typedef struct conn_info
{
    int   fd;              /* accepted fd on server, -1 means entry is free */
    pid_t client_pid;      /* client PID - trace use only */
    int   client_fd;       /* client's socket fd - trace use only */
    ctx_hndl_t ctx_hndl;   /* AFU context this connection is bound to */
    __u8       mode;       /* registration mode - see cblk.h */
    rx_func   rx;          /* IPC receive fcn depends on state */
    ctx_info_t *p_ctx_info;/* ptr to bound context 
			      or NULL if not registered */
} conn_info_t;


/* LUN discovery results are in lun_info */
typedef struct lun_info
{
    __u64       lun_id;    /* from cmd line/cfg file */
    __u32       flags;     /* housekeeping */
    
    struct {
	__u8        wwid[16];  /* LUN WWID from page 0x83 (NAA-6) */
	__u64       max_lba;   /* from read cap(16) */
	__u32       blk_len;   /* from read cap(16) */
    } li;

#define LUN_INFO_VALID   0x01
} lun_info_t;


/* Block Alocator can be shared between AFUs */
typedef struct blka
{
    ba_lun_t    ba_lun;    /* single LUN for SureLock */
    __u64       nchunk;    /* number of chunks */
    pthread_mutex_t mutex;
} blka_t;


enum undo_level {
    UNDO_NONE = 0,
    UNDO_MLOCK,
    UNDO_TIMER,
    UNDO_AFU_OPEN,
    UNDO_AFU_START,
    UNDO_AFU_MMAP,
    UNDO_OPEN_SOCK,
    UNDO_BIND_SOCK,
    UNDO_LISTEN,
    UNDO_EPOLL_CREATE,
    UNDO_EPOLL_ADD,
    UNDO_AFU_ALL /* must be last */
};

typedef struct afu
{
    /* Stuff requiring alignment go first. */

    /*
     * Command & data for AFU commands.
     * master sends only 1 command at a time except WRITE SAME.
     * Therefore, only 1 data buffer shared by all commands suffice.
     */
    char buf[0x1000];    /* 4K AFU data buffer (page aligned) */
    __u64 rrq_entry[NUM_RRQ_ENTRY]; /* 128B RRQ (page aligned) */
    struct afu_cmd {
	sisl_ioarcb_t rcb;  /* IOARCB (cache line aligned) */
	sisl_ioasa_t sa;    /* IOASA must follow IOARCB */
	pthread_mutex_t mutex; 
	pthread_cond_t cv;  /* for signalling responses */
	timer_t timer;

	__u8 cl_pad[CL_SIZE - 
		    ((sizeof(sisl_ioarcb_t) +
		      sizeof(sisl_ioasa_t) +
		      sizeof(pthread_mutex_t) +
		      sizeof(pthread_cond_t) +
		      sizeof(timer_t)) & CL_SIZE_MASK)];

    } cmd[NUM_CMDS];

#define AFU_INIT_INDEX   0  // first cmd is used in init/discovery,
                            // free for other use thereafter
#define AFU_SYNC_INDEX   (NUM_CMDS - 1) // last cmd is rsvd for afu sync

    /* Housekeeping data */
    char master_dev_path[MC_PATHLEN]; /* e. g. /dev/cxl/afu1.0m */
    conn_info_t conn_tbl[MAX_CONNS]; /* conn_tbl[0] is rsvd for listening */
    ctx_info_t ctx_info[MAX_CONTEXT];
    rht_info_t rht_info[MAX_CONTEXT];
    char *name;  /* ptr to last component in master_dev_path, e.g. afu1.0m */
    pthread_mutex_t mutex; /* for anything that needs serialization
			      e. g. to access afu */
    pthread_mutex_t err_mutex; /* for signalling error thread */
    pthread_cond_t err_cv;
    int err_flag;
#define E_SYNC_INTR   0x1  /* synchronous error interrupt */
#define E_ASYNC_INTR  0x2  /* asynchronous error interrupt */

    /* AFU Shared Data */
    sisl_rht_entry_t rht[MAX_CONTEXT][MAX_RHT_PER_CONTEXT];
    /* LXTs are allocated dynamically in groups */

    /* AFU HW */
    int                    afu_fd;
    struct cxl_ioctl_start_work work;
    char event_buf[0x1000];  /* Linux cxl event buffer (interrupts) */
    volatile struct surelock_afu_map *p_afu_map;  /* entire MMIO map */
    volatile struct sisl_host_map *p_host_map; /* master's sislite host map */
    volatile struct sisl_ctrl_map *p_ctrl_map; /* master's control map */

    ctx_hndl_t ctx_hndl; /* master's context handle */
    __u64 *p_hrrq_start;
    __u64 *p_hrrq_end;
    volatile __u64 *p_hrrq_curr;
    unsigned int toggle;
    __u64 room;
    __u64 hb;

    /* client IPC */
    int                    listen_fd;
    int                    epfd;
    struct sockaddr_un     svr_addr;
#ifndef _AIX
    struct epoll_event     events[MAX_CONN_TO_POLL]; /* ready events */
#else
    struct pollfd          events[MAX_CONN_TO_POLL]; /* ready events */
    int                    num_poll_events;
#endif /* !_AIX */

    /* LUN discovery: one lun_info per path */
    lun_info_t             lun_info[NUM_FC_PORTS];

    /* shared block allocator with other AFUs */
    blka_t             *p_blka;

    /* per AFU threads */
    pthread_t ipc_thread;
    pthread_t rrq_thread;
    pthread_t err_thread;

} __attribute__ ((aligned (0x1000))) afu_t;

struct afu_alloc {
    afu_t   afu;
    __u8    page_pad[0x1000 - (sizeof(afu_t) & 0xFFF)];
};


typedef struct asyc_intr_info {
    __u64 status;
    char *desc;
    __u8 port;
    __u8 action;
#define CLR_FC_ERROR   0x01
#define LINK_RESET     0x02
} asyc_intr_info_t;



conn_info_t *alloc_connection(afu_t *p_afu, int fd);
void free_connection(afu_t *p_afu, conn_info_t *p_conn_info);
int afu_init(afu_t *p_afu, struct capikv_ini_elm *p_elm);
void undo_afu_init(afu_t *p_afu, enum undo_level level);
int afu_term(afu_t *p_afu);
void afu_err_intr_init(afu_t *p_afu);


void set_port_online(volatile __u64 *p_fc_regs);
void set_port_offline(volatile __u64 *p_fc_regs);
int wait_port_online(volatile __u64 *p_fc_regs,
		     useconds_t delay_us,
		     unsigned int nretry);
int wait_port_offline(volatile __u64 *p_fc_regs,
		      useconds_t delay_us,
		      unsigned int nretry);
int afu_set_wwpn(afu_t *p_afu, int port, 
		 volatile __u64 *p_fc_regs,
		 __u64 wwpn);
void afu_link_reset(afu_t *p_afu, int port, 
		    volatile __u64 *p_fc_regs);


int do_mc_register(afu_t        *p_afu, 
		   conn_info_t  *p_conn_info,
		   __u64        challenge);

int do_mc_unregister(afu_t        *p_afu, 
		     conn_info_t  *p_conn_info);

int do_mc_open(afu_t        *p_afu, 
	       conn_info_t  *p_conn_info, 
	       __u64        flags,
	       res_hndl_t   *p_res_hndl);

int do_mc_close(afu_t        *p_afu, 
		conn_info_t  *p_conn_info,
		res_hndl_t   res_hndl);

int do_mc_size(afu_t        *p_afu, 
	       conn_info_t  *p_conn_info,
	       res_hndl_t   res_hndl,
	       __u64        new_size,
	       __u64        *p_act_new_size);

int do_mc_xlate_lba(afu_t        *p_afu, 
		    conn_info_t* p_conn_info,
		    res_hndl_t   res_hndl,
		    __u64        v_lba,
		    __u64        *p_p_lba);

int do_mc_clone(afu_t        *p_afu, 
		conn_info_t  *p_conn_info, 
		ctx_hndl_t   ctx_hndl_src,
		__u64        challenge,
		__u64        flags);

int do_mc_dup(afu_t        *p_afu, 
	      conn_info_t  *p_conn_info, 
	      ctx_hndl_t   ctx_hndl_cand,
	      __u64        challenge);

int do_mc_stat(afu_t        *p_afu, 
	       conn_info_t  *p_conn_info, 
	       res_hndl_t   res_hndl,
	       mc_stat_t    *p_mc_stat);

int do_mc_notify(afu_t        *p_afu, 
		 conn_info_t    *p_conn_info, 
		 mc_notify_t    *p_mc_notify);

int grow_lxt(afu_t            *p_afu, 
	     ctx_hndl_t       ctx_hndl_u,
	     res_hndl_t       res_hndl_u,
	     sisl_rht_entry_t *p_rht_entry,
	     __u64            delta,
	     __u64            *p_act_new_size);

int shrink_lxt(afu_t            *p_afu, 
	       ctx_hndl_t       ctx_hndl_u,
	       res_hndl_t       res_hndl_u,
	       sisl_rht_entry_t *p_rht_entry,
	       __u64            delta,
	       __u64            *p_act_new_size);

int clone_lxt(afu_t            *p_afu, 
	      ctx_hndl_t       ctx_hndl_u,
	      res_hndl_t       res_hndl_u,
	      sisl_rht_entry_t *p_rht_entry,
	      sisl_rht_entry_t *p_rht_entry_src);



int xfer_data(int fd, int op, void *buf, ssize_t exp_size);

int rx_mcreg(afu_t *p_afu, struct conn_info *p_conn_info, 
	     mc_req_t *p_req, mc_resp_t *p_resp);
int rx_ready(afu_t *p_afu, struct conn_info *p_conn_info, 
	     mc_req_t *p_req, mc_resp_t *p_resp);


asyc_intr_info_t *find_ainfo(__u64 status);
void afu_rrq_intr(afu_t *p_afu);
void afu_sync_intr(afu_t *p_afu);
void afu_async_intr(afu_t *p_afu);
void notify_err(afu_t *p_afu, int err_flag);

void *afu_ipc_rx(void *arg);
void *afu_rrq_rx(void *arg);
void *afu_err_rx(void *arg);

int mkdir_p(char *file_path);
void send_cmd(afu_t *p_afu, struct afu_cmd *p_cmd);
void wait_resp(afu_t *p_afu, struct afu_cmd *p_cmd);

int find_lun(afu_t *p_afu, lun_info_t *p_lun_info,
	     __u32 port_sel);
int read_cap16(afu_t *p_afu, lun_info_t *p_lun_info,
	       __u32 port_sel);
int page83_inquiry(afu_t *p_afu, lun_info_t *p_lun_info,
		   __u32 port_sel);
int afu_sync(afu_t *p_afu, ctx_hndl_t ctx_hndl_u, 
	     res_hndl_t res_hndl_u, __u8 mode);

void print_wwid(__u8 *p_wwid, char *ascii_buf);

void periodic_hb();
void periodic_fc();
void *sig_rx(void *arg);

void timer_start(timer_t timer, time_t sec, int rep);
void timer_stop(timer_t timer);

#endif /* _MSERVE_H */
