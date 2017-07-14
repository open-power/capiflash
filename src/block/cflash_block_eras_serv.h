/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_eras_serv.h $                          */
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
#ifndef _H_CFLASH_BLOCK_ERAS
#define _H_CFLASH_BLOCK_ERAS


#include <signal.h>
#include <time.h>


#ifndef _AIX
#include <sys/epoll.h>
#else 
#include <poll.h>
#endif /* !_AIX */
#include <sys/socket.h>
#include <sys/un.h>
#include "cflash_block_eras_client.h"
#include "cflash_block_internal.h"



/* Sizing parms: same context can be registered multiple times. 
   Therefore we allow MAX_CONNS > MAX_CONTEXT.
*/

#define MAX_CONNS  512         /* num client connections per disk */
#define MAX_CONN_TO_POLL 64    /* num fds to poll once            */






/* flags in IOA status area for host use */
#define B_DONE       0x01
#define B_ERROR      0x02  /* set with B_DONE */
#define B_TIMEOUT    0x04  /* set with B_DONE & B_ERROR */



#define MC_CRC_THRESH 100  /* threshold in 5 mins */

#define CL_SIZE             128             /* Processor cache line size */
#define CL_SIZE_MASK        0x7F            /* Cache line size mask */

#ifdef _REMOVE
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

    int ref_cnt;   /* num ctx_infos pointing to me */
} rht_info_t;
#endif


/* Single AFU context can be pointed to by multiple client connections. 
 * The client can create multiple endpoints (mc_hndl_t) to the same 
 * (context + AFU).
 */
typedef struct ctx_info
{


    /* The rht_info pointer is initialized when the context is first 
       registered, can be changed on dup.
    */


    /* all dup'ed contexts are in a doubly linked circular list */
    struct ctx_info *p_forw; 
    struct ctx_info *p_next;

    int ref_cnt;   /* num conn_infos pointing to me */
} ctx_info_t;

/* forward decls */

typedef struct cblk_serv_disk_s cblk_serv_disk_t;
typedef struct conn_info conn_info_t;



typedef cflsh_blk_eras_req_t mc_req_t;
typedef struct mc_resp mc_resp_t;

/* On accept, the server allocates a connection info. 
   A connection can be associated with at most one AFU context.
 */
typedef struct conn_info
{
    int   fd;              /* accepted fd on server, -1 means entry is free */
    pid_t client_pid;      /* client PID - trace use only */
    int   client_fd;       /* client's socket fd - trace use only */

    int   mode;            /* registration mode - see cblk.h */

                           /* IPC receive rx fcn depends on state */

    int (*rx)(cblk_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
		      cflsh_blk_eras_req_t  *p_req, cflsh_blk_eras_resp_t *p_resp);
    struct conn_info *prev;
    struct conn_info *next;
    cflsh_blk_eras_resp_t  mc_resp;
    ctx_info_t *p_ctx_info;/* ptr to bound context 
			      or NULL if not registered */
} conn_info_t;

typedef struct client_locks_s {
    pid_t client_pid;
    
    conn_info_t *head;
    conn_info_t *tail;

} client_locks_t;


enum undo_level {
    UNDO_NONE = 0,
    UNDO_MLOCK,
    UNDO_OPEN_SOCK,
    UNDO_BIND_SOCK,
    UNDO_LISTEN,
    UNDO_EPOLL_CREATE,
    UNDO_EPOLL_ADD,
    UNDO_ALL /* must be last */
};

typedef struct cblk_serv_disk_s {
    /* Stuff requiring alignment go first. */


    /* Housekeeping data */
    char eras_dev_path[MC_PATHLEN]; /* e. g. /dev/sg5 */
    struct conn_info conn_tbl[MAX_CONNS]; /* conn_tbl[0] is rsvd for listening */

    char *name;  /* ptr to last component in eras_dev_path, e.g. afu1.0m */
    pthread_mutex_t mutex; /* for anything that needs serialization
			      e. g. to access afu */

    pthread_cond_t thread_event;
    int flags;
#define CBLK_SRV_THRD_EXIT  0x1 /* Thread should exit */

    /* LXTs are allocated dynamically in groups */


    char event_buf[0x1000];  /* Linux cxl event buffer (interrupts) */

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

    /* per process threads */
    pthread_t ipc_thread;

} __attribute__ ((aligned (0x1000))) cblk_serv_disk_t;



/********************************************************************
 * Function prototypes for server daemon  
 *
 *******************************************************************/

				     conn_info_t *cblk_serv_alloc_connection(cblk_serv_disk_t *p_disk, int fd);
void cblk_serv_free_connection(cblk_serv_disk_t *p_disk, conn_info_t *p_conn_info);
int cblk_serv_term(cblk_serv_disk_t *p_disk);

int cblk_serv_do_register(cblk_serv_disk_t *p_disk, 
			  conn_info_t  *p_conn_info,
			  uint64_t challenge);

int cblk_serv_do_unregister(cblk_serv_disk_t *p_disk, 
			    conn_info_t  *p_conn_info);

int cblk_serv_do_set_trace(cblk_serv_disk_t  *p_disk, 
			   conn_info_t  *p_conn_info, 
			   int          verbosity,	 
			   char *filename,   
			   uint64_t     flags);


int cblk_serv_do_live_dump(cblk_serv_disk_t *p_disk, 
			   conn_info_t  *p_conn_info, 
			   char         *reason,
			   uint64_t      flags);

int cblk_serv_do_get_stats(cblk_serv_disk_t        *p_disk, 
			   conn_info_t  *p_conn_info,
			   char *disk_name,
			   chunk_stats_t *stat, 
			   uint64_t        flags);

int cblk_serv_xfer_data(int fd, int op, void *buf, ssize_t exp_size);

int cblk_serv_rx_reg(cblk_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
		     cflsh_blk_eras_req_t *p_req, mc_resp_t *p_resp);
int cblk_serv_rx_ready(cblk_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
		       cflsh_blk_eras_req_t *p_req, mc_resp_t *p_resp);

void *cblk_eras_thread(void *arg);

int cblk_serv_mkdir_p(char *file_path);




#endif /* _H_CFLASH_BLOCK_ERAS */
