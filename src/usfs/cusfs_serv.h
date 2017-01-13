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


#include <signal.h>
#include <time.h>


#ifndef _AIX
#include <sys/epoll.h>
#else 
#include <poll.h>
#endif /* !_AIX */
#include <sys/socket.h>
#include <sys/un.h>
#include "cflsh_usfs_client.h"


/********************************************************************
 * Queueing Macros
 *
 * The CUSFS_Q/DQ_NODE macros enqueue and dequeue nodes to the head
 * or tail of a doubly-linked list.  They assume that 'next' and 'prev'
 * are the names of the queueing pointers.  
 *
 *******************************************************************/

/*
 * Give a node and the head and tail pointer for a list, enqueue
 * the node at the head of the list.  Assumes the list is
 * doubly-linked and NULL-terminated at both ends, and that node
 * is non-NULL.  Casts to void allow commands of different data
 * types than the list to be queued into the list.  This is useful
 * if one data type is a subset of another.
 */
#define CUSFS_Q_NODE_HEAD(head, tail, node,_node_prev,_node_next) \
do {                                                            \
                                                                \
    /* If node is valid  */                                     \
    if ((node) != NULL)                                         \
    {                                                           \
        (node)->_node_prev = NULL;                              \
        (node)->_node_next = (head);                            \
                                                                \
        if ((head) == NULL) {                                   \
                                                                \
            /* List is empty; 'node' is also the tail */        \
            (tail) = (node);                                    \
                                                                \
        } else {                                                \
                                                                \
            /* List isn't empty;old head must point to 'node' */ \
            (head)->_node_prev = (node);                        \
        }                                                       \
                                                                \
        (head) = (node);                                        \
    }                                                           \
                                                                \
} while (0)



/*
 * Give a node and the head and tail pointer for a list, enqueue
 * the node at the tail of the list.  Assumes the list is
 * doubly-linked and NULL-terminated at both ends, and that node
 * is non-NULL.  Casts to void allow commands of different data
 * types than the list to be queued into the list.  This is useful
 * if one data type is a subset of another.
 */
#define CUSFS_Q_NODE_TAIL(head, tail, node,_node_prev,_node_next) \
do {                                                            \
                                                                \
    /* If node is valid  */                                     \
    if ((node) != NULL)                                         \
    {                                                           \
                                                                \
        (node)->_node_prev = (tail);                            \
        (node)->_node_next = NULL;                              \
                                                                \
        if ((tail) == NULL) {                                   \
                                                                \
            /* List is empty; 'node' is also the head */        \
            (head) = (node);                                    \
                                                                \
        } else {                                                \
                                                                \
            /* List isn't empty;old tail must point to 'node' */ \
            (tail)->_node_next = (node);                        \
        }                                                       \
                                                                \
        (tail) = (node);                                        \
    }                                                           \
                                                                \
                                                                \
                                                                \
} while (0)


/*
 * Given a node and the head and tail pointer for a list, dequeue
 * the node from the list.  Assumes the list is doubly-linked and
 * NULL-terminated at both ends, and that node is non-NULL.
 *
 * Casts to void allow commands of different data types than the
 * list to be dequeued into the list.  This is useful if one data
 * type is a subset of another.
 */
#define CUSFS_DQ_NODE(head, tail, node,_node_prev,_node_next)    \
do {                                                            \
    /* If node is valid  */                                     \
    if ((node) != NULL)                                         \
    {                                                           \
        /* If node was head, advance the head to node's next*/  \
        if ((head) == (node))                                   \
        {                                                       \
            (head) = ((node)->_node_next);                      \
        }                                                       \
                                                                \
        /* If node was tail, retract the tail to node's prev */ \
        if ((tail) == (node))                                   \
        {                                                       \
            (tail) = ((node)->_node_prev);                      \
        }                                                       \
                                                                \
        /* A follower's predecessor is now node's predecessor*/ \
        if ((node)->_node_next)                                 \
        {                                                       \
            (node)->_node_next->_node_prev = ((node)->_node_prev); \
        }                                                       \
                                                                \
        /* A predecessor's follower is now node's follower */   \
        if ((node)->_node_prev)                                 \
        {                                                       \
            (node)->_node_prev->_node_next = ((node)->_node_next); \
        }                                                       \
                                                                \
        (node)->_node_next = NULL;                              \
        (node)->_node_prev = NULL;                              \
    }                                                           \
                                                                \
                                                                \
} while(0)


/* Sizing parms: same context can be registered multiple times. 
   Therefore we allow MAX_CONNS > MAX_CONTEXT.
*/

#define MAX_RHT_PER_CONTEXT 16            /* num resource hndls per context */
#define MAX_CONNS  512         /* num client connections per disk */
#define MAX_CONN_TO_POLL 64               /* num fds to poll once */
#define NUM_RRQ_ENTRY    16               /* for master issued cmds */
#define NUM_CMDS         16               /* must be <= NUM_RRQ_ENTRY */




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
struct capikv_ini_elm;
typedef struct cusfs_serv_disk_s cusfs_serv_disk_t;
typedef struct conn_info conn_info_t;



typedef cflsh_usfs_master_req_t mc_req_t;
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

    int (*rx)(cusfs_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
		      cflsh_usfs_master_req_t  *p_req, cflsh_usfs_master_resp_t *p_resp);
    struct conn_info *prev;
    struct conn_info *next;
    cflsh_usfs_master_resp_t  mc_resp;
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
    UNDO_AFU_OPEN,
    UNDO_OPEN_SOCK,
    UNDO_BIND_SOCK,
    UNDO_LISTEN,
    UNDO_EPOLL_CREATE,
    UNDO_EPOLL_ADD,
    UNDO_AFU_ALL /* must be last */
};

typedef struct cusfs_serv_disk_s {
    /* Stuff requiring alignment go first. */


    /* Housekeeping data */
    char master_dev_path[MC_PATHLEN]; /* e. g. /dev/sg5 */
    struct conn_info conn_tbl[MAX_CONNS]; /* conn_tbl[0] is rsvd for listening */

    char *name;  /* ptr to last component in master_dev_path, e.g. afu1.0m */
    pthread_mutex_t mutex; /* for anything that needs serialization
			      e. g. to access afu */
    pthread_mutex_t err_mutex; /* for signalling error thread */
    pthread_cond_t err_cv;
    int err_flag;
#define E_SYNC_INTR   0x1  /* synchronous error interrupt */
#define E_ASYNC_INTR  0x2  /* asynchronous error interrupt */

    /* AFU Shared Data */

    /* LXTs are allocated dynamically in groups */

    /* AFU HW */
    int                    afu_fd;

    char event_buf[0x1000];  /* Linux cxl event buffer (interrupts) */

    client_locks_t locks[CFLSH_USFS_MASTER_LAST_ITEM];
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

    /* per AFU threads */
    pthread_t ipc_thread;
    pthread_t rrq_thread;
    pthread_t err_thread;

} __attribute__ ((aligned (0x1000))) cusfs_serv_disk_t;


struct afu_alloc {
     cusfs_serv_disk_t disk;
    uint8_t page_pad[0x1000 - (sizeof(cusfs_serv_disk_t) & 0xFFF)];
};



conn_info_t *alloc_connection(cusfs_serv_disk_t *p_disk, int fd);
void free_connection(cusfs_serv_disk_t *p_disk, conn_info_t *p_conn_info);
int afu_init(cusfs_serv_disk_t *p_disk, struct capikv_ini_elm *p_elm);
void undo_afu_init(cusfs_serv_disk_t *p_disk, enum undo_level level);
int cusfs_serv_disk_term(cusfs_serv_disk_t *p_disk);
void afu_err_intr_init(cusfs_serv_disk_t *p_disk);



int do_master_register(cusfs_serv_disk_t        *p_disk, 
                      conn_info_t  *p_conn_info,
                      uint64_t challenge);

int do_master_unregister(cusfs_serv_disk_t        *p_disk, 
		     conn_info_t  *p_conn_info);

int do_master_lock(cusfs_serv_disk_t        *p_disk, 
	       conn_info_t  *p_conn_info, 
	      cflsh_usfs_master_lock_t lock,
	       uint64_t        flags);

int do_master_unlock(cusfs_serv_disk_t        *p_disk, 
		conn_info_t  *p_conn_info, 
	      cflsh_usfs_master_lock_t lock,
	       uint64_t        flags);

int xfer_data(int fd, int op, void *buf, ssize_t exp_size);

int rx_mcreg(cusfs_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
	     cflsh_usfs_master_req_t *p_req, mc_resp_t *p_resp);
int rx_ready(cusfs_serv_disk_t *p_disk, struct conn_info *p_conn_info, 
	     cflsh_usfs_master_req_t *p_req, mc_resp_t *p_resp);




void *afu_ipc_rx(void *arg);

int mkdir_p(char *file_path);


void periodic_hb();

void *sig_rx(void *arg);

#endif /* _MSERVE_H */
