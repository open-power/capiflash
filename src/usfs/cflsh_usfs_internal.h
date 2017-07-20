/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_usfs_internal.h 1.6  */
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
/* %Z%%M%       %I%  %W% %G% %U% */


/*
 * COMPONENT_NAME: (sysxcflashusfs) CAPI Flash user space file system library
 *
 * FUNCTIONS: Data structure used by library for its various internal
 *            operations.
 *
 * ORIGINS: 27
 *
 *                  -- (                            when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 2015
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#ifndef _H_CFLASH_USFS_INT
#define _H_CFLASH_USFS_INT

#ifndef _AIX
#define _GNU_SOURCE
#endif /* !_AIX */

#include "cflsh_usfs_disk.h"
#include "cflsh_usfs_wrapper.h"


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif



/************************************************************************/
/* Compile options                                                      */
/************************************************************************/
#ifdef _NOT_YET
#define _MASTER_LOCK 1  /* require master to coordinate locking of meta */
			/* data in filesystem                           */
#endif /* NOT_YET */



/************************************************************************/
/* Userspace filesyste library limits                                   */
/************************************************************************/
#define MAX_NUM_THREAD_LOGS    4095    /* Maximum number of thread logs allowed per  */
				       /* process                                    */

#define CUSFS_BITS_PER_BYTE      8      


#define CUSFS_PAGE_ALIGN      0xfffffffffffff000LL
/************************************************************************/
/* CAPI user space filesyste library eye catchers                       */
/************************************************************************/
#define CFLSH_USFS_EYEC_GLOBAL   __EYEC4('c','u','G','L')    /* cuGL */
#define CFLSH_USFS_EYEC_FS       __EYEC4('c','u','F','S')    /* cuFS */
#define CFLSH_USFS_EYEC_FL       __EYEC4('c','u','F','L')    /* cuFL */
#define CFLSH_USFS_EYEC_AIO      __EYEC4('c','u','A','I')    /* cuAI */

#define CFLSH_EYECATCH_AIO(aio) ((aio)->eyec != CFLSH_USFS_EYEC_AIO)

#define CFLSH_EYECATCH_FILE(file) ((file)->eyec != CFLSH_USFS_EYEC_FL)

#define CFLSH_EYECATCH_FS(cufs) ((cufs)->eyec != CFLSH_USFS_EYEC_FS)

#define CFLSH_EYECATCH_GLOBAL(global) ((global)->eyec != CFLSH_USFS_EYEC_GLOBAL)

/************************************************************************/
/* Miscellaneous                                                        */
/************************************************************************/

#define CFLSH_USFS_WAIT_AIO_BG_DLY  10   /* Time to delay in microseconds*/
				         /* for background thread to wait*/
					 /* for AIO to drain.            */

#define CFLSH_USFS_WAIT_AIO_DELAY   100  /* Time to delay in microseconds*/
				         /* to wait for AIO to drain.    */

#define CFLSH_USFS_WAIT_AIO_RETRY  100000/* Retries to wait for AIO to   */
					 /* drain.                       */

#define CFLSH_USFS_WAIT_READ_AHEAD   100 /* Time to delay in microseconds*/
				         /* for read ahead to complete   */

#define CFLSH_USFS_WAIT_READ_AHEAD_RETRY 600000 /* Retries to wait for  */
					 /* read ahead to complete.      */

#define CFLSH_USFS_PTHREAD_SIG   0x1    /* Send a pthread_cond_signal    */

#define CFLSH_USFS_ONLY_ASYNC_CMPLT 0x1 /* only look for async completion */

#define CFLSH_USFS_ALLOC_READ_AHEAD 1   /* Allocate Read Ahead Buffer   */

/************************************************************************/
/* Delays/Retries                                                       */
/************************************************************************/

#define CFLSH_USFS_RDWR_WRITE  0x001     /* Indicates write      */
#define CFLSH_USFS_RDWR_DIRECT 0x002     /* Indicates direct I/O */
					 /* to user's buffer.    */

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))


#ifndef _AIX
/*
 * fetch_and_or and fetch_and_add are not available for linux user space.
 * It appears that __sync_fetch_and_or, and __sync_fetch_and_add are doing the
 * required functionality, respectively.
 */

#define fetch_and_or(ptr,val) (__sync_fetch_and_or(ptr,val))

#define fetch_and_add(ptr,val) (__sync_fetch_and_add(ptr,val))


#endif /* !_AIX */


/************************************************************************/
/* Tracing/logging data structues and maroc                             */
/************************************************************************/
extern char  *cusfs_log_filename;
extern int cusfs_log_verbosity;
extern FILE *cusfs_logfp;
extern FILE *cusfs_dumpfp;
extern int cusfs_dump_level;
extern int cusfs_notify_log_level;
extern int dump_sequence_num;   

extern pthread_mutex_t  cusfs_log_lock;

extern uint32_t num_thread_logs;     /* Number of thread log files */


/*
 * For thread trace tables, using pthread_t does not work
 * work well since the values can be very large (64-bit)
 * and there is not an obvious way to hash them into something
 * more manageable. So for thread trace table we use
 * gettid for non-AIX and pthread_getthreadid_np. pthread_getthreadid_np
 * is a non-portible pthread interface to get thread sequence numbers,
 * It is available in some BSD unixes, but not AIX nor Linux.
 * However AIX does support a close derivative pthread_getunique_np, which
 * can be used to implement it. For linux we use gettid via the syscall
 * interface.
 */
#ifdef _AIX 

inline int pthread_getthreadid_np(void) 
{
    int tid = 0;
    pthread_t my_pthread = pthread_self();

    if (pthread_getunique_np(&my_pthread,&tid)) {

        tid = 0;
    }

    return tid;
}



#define CFLSH_BLK_GETTID     (pthread_getthreadid_np()) 
#else
#define CFLSH_BLK_GETTID     (syscall(SYS_gettid))
#endif /* ! AIX */

/*
 * Trace/log the information to common file. If thread tracing
 * is also on then log information to the hashed thread log file.
 */
#define CUSFS_TRACE_LOG_FILE(verbosity,msg, ...)                         \
do {                                                                    \
   if ((cusfs_log_filename != NULL) &&                                   \
       (verbosity <= cusfs_log_verbosity)) {                             \
      pthread_mutex_lock(&cusfs_log_lock);                               \
      if (cusfs_global.flags & CFLSH_G_SYSLOG) {                           \
          /*                                                            \
           * If we are using syslog                                     \
           */                                                           \
        cusfs_trace_log_data_syslog(__FILE__,(char *)__FUNCTION__, __LINE__, \
                                   msg,## __VA_ARGS__);       \
                                                                        \
      } else {                                                          \
          /*                                                            \
           * If we are not using syslog, but tracing to files           \
           */                                                           \
          cusfs_trace_log_data_ext(&(cusfs_global.trace_ext),               \
                             cusfs_logfp,                                \
                             __FILE__,(char *)__FUNCTION__,__LINE__,    \
                             msg,## __VA_ARGS__);                       \
          if (num_thread_logs) {                                        \
              int thread_index = CFLSH_BLK_GETTID & cusfs_global.thread_log_mask; \
                                                                        \
                                                                        \
              if ((thread_index >=0) &&                                 \
                  (thread_index < num_thread_logs)) {                   \
                                                                        \
                      cusfs_global.thread_logs[thread_index].ext_arg.log_number = cusfs_global.trace_ext.log_number; \
                      cusfs_trace_log_data_ext(&(cusfs_global.thread_logs[thread_index].ext_arg), \
                                  cusfs_global.thread_logs[thread_index].logfp,  \
                                 __FILE__,(char *)__FUNCTION__,         \
                                __LINE__,msg,## __VA_ARGS__);           \
                                                                        \
              }                                                         \
          }                                                             \
      }                                                                 \
                                                                        \
      pthread_mutex_unlock(&cusfs_log_lock);                             \
   }                                                                    \
} while (0)

typedef struct cflsh_thread_log_s {
    trace_log_ext_arg_t ext_arg;
    FILE *logfp;
} cflsh_thread_log_t;



/************************************************************************/
/* Live dump macro                                                      */
/************************************************************************/

#define CUSFS_LIVE_DUMP_THRESHOLD(level,msg)                             \
do {                                                                    \
                                                                        \
    if (level <= cusfs_dump_level)  {                                    \
        cusfs_dump_debug_data(msg,__FILE__,__FUNCTION__,__LINE__,__DATE__);  \
    }                                                                   \
                                                                        \
} while (0)

#ifdef _REMOVE
/************************************************************************/
/* Notify/Log macro: This is only used when we want to control          */
/* amount of logging.                                                   */
/************************************************************************/

#define CUSFS_NOTIFY_LOG_THRESHOLD(level,chunk,path_index,error_num,out_rc,reason, cmd) \
do {                                                                    \
                                                                        \
    if (level <= cusfs_notify_log_level) {                               \
        cusfs_notify_mc_err((chunk),(path_index),(error_num),(out_rc),(reason),(cmd));  \
    }                                                                   \
                                                                        \
} while (0)

#endif /* _REMOVE */
/************************************************************************/
/* OS specific LWSYNC macros                                            */
/************************************************************************/

#ifdef _AIX
#define CUSFS_LWSYNC()        asm volatile ("lwsync")
#elif _MACOSX
#define CUSFS_LWSYNC()        asm volatile ("mfence")
#else
#define CUSFS_LWSYNC()        __asm__ __volatile__ ("lwsync")
#endif 



/***************************************************************************
 *
 * cusfs_lock
 *
 * Wrapper structure for mutex locks and accompanying macros
 *
 ***************************************************************************/
typedef
struct cusfs_lock_s {
    pthread_mutex_t   plock;
    pthread_t         thread;   /* Thread id of lock holder                */
    int               count;    /* Non-zero when someone holding this lock */
    char              *filename;/* Filename string of lock taker           */
    uint              file;     /* File number of lock taker               */
    uint              line;     /* Line number of lock taker               */
} cusfs_lock_t;

/* 
 * Initialize the mutex lock fields
 *
 * cflsh_lock - cflsh_blk_lock_t structure
 */
#define CUSFS_LOCK_INIT(cflsh_lock)                               \
do {                                                                     \
    pthread_mutex_init(&((cflsh_lock).plock), NULL);                     \
    (cflsh_lock).count      = 0;                                         \
    (cflsh_lock).file       = 0;                                         \
    (cflsh_lock).line       = 0;                                         \
    (cflsh_lock).thread     = 0;                                         \
} while (0)

#define CUSFS_LOCK_IFREE(cflsh_lock)                              \
do {                                                                     \
    pthread_mutex_destroy(&((cflsh_lock).plock));                        \
    (cflsh_lock).count      = 0;                                         \
    (cflsh_lock).file       = 0;                                         \
    (cflsh_lock).line       = 0;                                         \
    (cflsh_lock).thread     = NULL;                                      \
} while (0)

#define CUSFS_LOCK(cflsh_lock)                                   \
do {                                                                    \
                                                                        \
    pthread_mutex_lock(&((cflsh_lock).plock));                          \
                                                                        \
    if (cusfs_log_verbosity >= 5) {                                      \
        (cflsh_lock).thread = pthread_self();                           \
    }                                                                   \
    (cflsh_lock).file   = CFLSH_USFS_FILENUM;                            \
    (cflsh_lock).filename = __FILE__;                                   \
    (cflsh_lock).line   = __LINE__;                                     \
                                                                        \
    (cflsh_lock).count++;                                               \
                                                                        \
} while (0)


#define CUSFS_UNLOCK(cflsh_lock)                                 \
do {                                                                    \
                                                                        \
    (cflsh_lock).count--;                                               \
                                                                        \
    (cflsh_lock).thread = 0;                                            \
    (cflsh_lock).filename = NULL;                                       \
    (cflsh_lock).file   = 0;                                            \
    (cflsh_lock).line   = 0;                                            \
                                                                        \
    pthread_mutex_unlock(&((cflsh_lock).plock));                        \
                                                                        \
} while (0)





/***************************************************************************
 *
 * cflsh_blk_rwlock
 *
 * Wrapper structure for mutex rwlocks and accompanying macros
 *
 ***************************************************************************/
typedef
struct cusfs_rwlock_s {
    pthread_rwlock_t  plock;
    pthread_t         thread;   /* Thread id of lock holder                */
    int               count;    /* Non-zero when someone holding this lock */
    char              *filename;/* Filename string of lock taker           */
    uint              file;     /* File number of lock taker               */
    uint              line;     /* Line number of lock taker               */
} cflsh_blk_rwlock_t;

#ifdef _USE_RW_LOCK
/* 
 * Initialize the mutex read/write lock fields
 *
 * cflsh_lock - cflsh_blk_rwlock_t structure
 */
#define CUSFS_RWLOCK_INIT(cflsh_lock)                             \
do {                                                                     \
    pthread_rwlock_init(&((cflsh_lock).plock), NULL);                    \
    (cflsh_lock).count      = 0;                                         \
    (cflsh_lock).file       = 0;                                         \
    (cflsh_lock).line       = 0;                                         \
    (cflsh_lock).thread     = 0;                                         \
} while (0)

#define CUSFS_RWLOCK_IFREE(cflsh_lock)                            \
do {                                                                     \
    pthread_rwlock_destroy(&((cflsh_lock).plock));                       \
    (cflsh_lock).count      = 0;                                         \
    (cflsh_lock).file       = 0;                                         \
    (cflsh_lock).line       = 0;                                         \
    (cflsh_lock).thread     = NULL;                                      \
} while (0)

#define CUSFS_RD_RWLOCK(cflsh_lock)                              \
do {                                                                    \
                                                                        \
    pthread_rwlock_rdlock(&((cflsh_lock).plock));                       \
                                                                        \
    if (cusfs_log_verbosity >= 5) {                                      \
        (cflsh_lock).thread = pthread_self();                           \
    }                                                                   \
    (cflsh_lock).file   = CFLSH_USFS_FILENUM;                            \
    (cflsh_lock).filename = __FILE__;                                   \
    (cflsh_lock).line   = __LINE__;                                     \
                                                                        \
    (cflsh_lock).count++;                                               \
                                                                        \
} while (0)


#define CUSFS_WR_RWLOCK(cflsh_lock)                              \
do {                                                                    \
                                                                        \
    pthread_rwlock_wrlock(&((cflsh_lock).plock));                       \
                                                                        \
    if (cusfs_log_verbosity >= 5) {                                      \
        (cflsh_lock).thread = pthread_self();                           \
    }                                                                   \
    (cflsh_lock).file   = CFLSH_USFS_FILENUM;                            \
    (cflsh_lock).filename = __FILE__;                                   \
    (cflsh_lock).line   = __LINE__;                                     \
                                                                        \
    (cflsh_lock).count++;                                               \
                                                                        \
} while (0)

#define CUSFS_RWUNLOCK(cflsh_lock)                               \
do {                                                                    \
                                                                        \
    (cflsh_lock).count--;                                               \
                                                                        \
    (cflsh_lock).thread = 0;                                            \
    (cflsh_lock).filename = NULL;                                       \
    (cflsh_lock).file   = 0;                                            \
    (cflsh_lock).line   = 0;                                            \
                                                                        \
    pthread_rwlock_unlock(&((cflsh_lock).plock));                       \
                                                                        \
} while (0)

#else
/*
 * Do not use read/write locks use mutex locks instead
 */
#define CUSFS_RWLOCK_INIT(cflsh_lock)   CUSFS_LOCK_INIT(cflsh_lock) 
#define CUSFS_RWLOCK_IFREE(cflsh_lock)  CUSFS_LOCK_IFREE(cflsh_lock) 
#define CUSFS_RD_RWLOCK(cflsh_lock)     CUSFS_LOCK(cflsh_lock) 
#define CUSFS_WR_RWLOCK(cflsh_lock)     CUSFS_LOCK(cflsh_lock) 
#define CUSFS_RWUNLOCK(cflsh_lock)      CUSFS_UNLOCK(cflsh_lock) 

#endif                        



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



/************************************************************************/
/* AIOCB defines and macros                                             */
/************************************************************************/

#define CUSFS_AIOCB32_FLG   1 /* Indicates this is an aiocb and not aiocb64 */

#ifdef _AIX
#define CUSFS_RET_AIOCB(_aiocbp_,_rc_,_errno_)   \
do {                                             \
                                                 \
    (_aiocbp_)->aio_errno = (_errno_);           \
                                                 \
    (_aiocbp_)->aio_return = (_rc_);             \
                                                 \
} while (0)

#else

#define CUSFS_RET_AIOCB(_aiocbp_,_rc_,_errno_)   \
do {                                             \
                                                 \
    (_aiocbp_)->__error_code = (_errno_);        \
                                                 \
    (_aiocbp_)->__return_value = (_rc_);         \
                                                 \
} while (0)


#endif /* !_AIX */




typedef enum {
    CFLSH_USFS_AIO_E_INVAL = 0x0,  /* Invalid operation          */
    CFLSH_USFS_AIO_E_READ  = 0x1,  /* Read operations            */
    CFLSH_USFS_AIO_E_WRITE = 0x2,  /* Write operations           */
} cflsh_usfs_aio_op_t;

/************************************************************************/
/* cflsh_usfs_aio_entry - The data structure for a async I/O requests   */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/************************************************************************/
typedef struct cflsh_usfs_aio_entry_s {
    struct cflsh_usfs_aio_entry_s *free_prev;/* Previous free async    */
					     /* I/O                    */
    struct cflsh_usfs_aio_entry_s *free_next;/* Next free async        */
					     /* I/O                    */
    struct cflsh_usfs_aio_entry_s *act_prev; /* Previous active async  */
					     /* I/O.                   */ 
    struct cflsh_usfs_aio_entry_s *act_next; /* Next active async I/O  */
    struct cflsh_usfs_aio_entry_s *cmplt_prev;/* Previous completed    */
					      /* async I/O.            */
    struct cflsh_usfs_aio_entry_s *cmplt_next;/* Next completed        */
					      /* async I/O.            */
    struct cflsh_usfs_aio_entry_s *file_prev;/* Previous async I/O     */
					     /* for this file.         */
    struct cflsh_usfs_aio_entry_s *file_next;/* Next async I/O         */
					     /* for this file          */

    int flags;  
#define CFLSH_USFS_AIO_E_AIOCB 0x0001 /* This aio_entry is associated */
				      /* with an aiocb.               */
#define CFLSH_USFS_AIO_E_AIOCB64 0x0002 /* This aio_entry is associated */
				      /* with an aiocb64.               */
#define CFLSH_USFS_AIO_E_LBUF  0x0004 /* This aio_entry is using the  */
				      /* local_buffer.                */
#define CFLSH_USFS_AIO_E_ISSUED 0x0008/* Command has been issued      */
    cflsh_usfs_aio_op_t  op;          /* Operation                    */
    int state;
    uint64_t aio_offset;               /* aiocb's aio_offset          */
    uint64_t aio_nbytes;               /* aiocb's aio_nbytes          */
    struct aiocb64 *aiocbp;            /* Pointer to associated aiocb */
    struct sigevent aio_sigevent;      /* aiocb's aio_sigevent        */
    struct cflsh_usfs_data_obj_s *file;/* Associated file structure   */
    void *caller_buffer;               /* Buffer provided by caller   */
				       /* to this library.            */
    uint64_t caller_buffer_len;        /* Length of caller buffer in  */
				       /* bytes.                      */
    void *local_buffer;                /* Our local (internal) buffer */
    uint64_t local_buffer_len;         /* Length of local buffer in   */
				       /* bytes.                      */
    uint64_t local_buffer_len_used;    /* Amount of bytes used by this*/
				       /* Async I/O of the            */
				       /* local_buffer.               */
    int tag;                           /* tag returned by cblk_aread  */
				       /* or cblk_awrite.             */
    int index;                         /* Index in async pool         */
    size_t nblocks;                    /* Size of transfer in blocks  */
    uint64_t offset;                   /* Offset in file              */
    uint64_t start_lba;                /* Starting LBA for this async */
    cblk_arw_status_t cblk_status;     /* Status for this command     */
    
    eye_catch4b_t  eyec;           /* Eye catcher                      */

} cflsh_usfs_aio_entry_t;


struct cflsh_usfs_data_read_ahead {
    int flags;
#define CFLSH_USFS_D_RH_ACTIVE 0x0001 /* Read ahead request is active*/
    int  tag;                     /* Tag of async I/O                */
    uint64_t start_offset;        /* Starting disk LBA of read ahead */
    uint64_t start_lba;           /* Starting disk LBA of read ahead */
    uint64_t nblocks;             /* Size of read ahead in blocks    */
    void *local_buffer;           /* Our local (internal) buffer     */
    uint64_t local_buffer_len;    /* Length of local buffer in       */
				  /* bytes.                          */
    uint64_t local_buffer_len_used;/* Amount of bytes used by this   */
				 /* Request                          */

    cblk_arw_status_t cblk_status;/* Status for this command         */

};



/************************************************************************/
/* cflsh_usfs_data_obj - The data structure for a data object in the    */
/*                      in the filesystem (i.e. file, directory,        */
/*                      symlink, etc).                                  */
/*                                                                      */
/************************************************************************/
typedef struct cflsh_usfs_data_obj_s {
    struct cflsh_usfs_data_obj_s *prev;  /* Previous file in cufs list  */
    struct cflsh_usfs_data_obj_s *next;  /* Next file in cufs list      */
    struct cflsh_usfs_data_obj_s *fprev; /* Previous file in global list*/
    struct cflsh_usfs_data_obj_s *fnext; /* Next file in global list    */
    int flags;
#define CFLSH_USFS_ROOT_DIR  0x00001
#define CFLSH_USFS_DIRECT_IO 0x00002     /* This file was open with   */
					 /* O_DIRECT flag set.        */
#define CFLSH_USFS_SYNC_IO   0x00004     /* This file was open with   */
					 /* O_SYNC flag set.          */
#define CFLSH_USFS_IN_HASH   0x00008     /* In file hash table        */


    cflsh_usfs_inode_file_type_t file_type;
    cflsh_usfs_inode_t *inode;
    int index;
    FILE *fp;
    char filename[PATH_MAX];           /* Filename                     */

    cflash_offset_t  offset;           /* offset in this file          */

    void *data_buf;                    /* Data buffer of file          */
    int  data_buf_size;                /* Data buffer size             */
    cusfs_lock_t   lock;
    
    struct cflsh_usfs_s   *fs;

    cflsh_usfs_disk_lba_e_t *disk_lbas; /* Disk LBAs for this file */
    uint64_t num_disk_lbas;             /* Number of entries in the*/
				        /* disk_lbas list.         */ 
    struct cflsh_usfs_data_read_ahead seek_ahead; /* Read ahead on seek */
    struct {
	int flags;
#define CFLSH_USFS_D_INODE_MTIME 0x0001 /* Modification time for */
				    /* inode needs to be updated */
				    /* to this value.            */
#define CFLSH_USFS_D_INODE_ATIME 0x0002 /* Last access  time for */
				    /* inode needs to be updated */
				    /* to this value.            */
#if !defined(__64BIT__) && defined(_AIX)
	time64_t mtime;             /* Last time file content   */
				    /* was modified.            */
	time64_t atime;             /* Last time file was       */
				    /* accessed.                */ 
#else
	time_t mtime;               /* Last time file content   */
				    /* was modified.            */
	time_t atime;               /* Last time file was       */
				    /* accessed.                */ 
#endif  

    } inode_updates;

    cflsh_usfs_aio_entry_t *head_act; /* Head of active async   */
                                      /* I/O for this file.     */
    cflsh_usfs_aio_entry_t *tail_act; /* Tail of active async   */
                                      /* I/O for this file.     */
    eye_catch4b_t  eyec;           /* Eye catcher                      */


} cflsh_usfs_data_obj_t;



/************************************************************************/
/* CUFS hashing defines                                                */
/************************************************************************/
#define CUSFS_BAD_ADDR_MASK  0xfff     /* CUFS' should be allocated page aligned   */
                                       /* this mask allows us to check for bad       */
                                       /* chunk addresses                            */

#define MAX_NUM_CUSFS_HASH       64
#define MAX_NUM_CUSFS_FILE_HASH  64

#define CUSFS_HASH_MASK      0x0000003f

typedef struct cflsh_usfs_async_thread_s {
    struct cflsh_usfs_s *cufs;   /* Cufs associated with this  */
                                 /* thread.                    */

} cflsh_usfs_async_thread_t;


struct cflsh_usfs_stat {
    struct cflsh_usfs_s *cufs;      /* Cufs associated with this       */
                                    /* thread.                         */
    uint64_t num_areads;            /* Total number of async reads     */
                                    /* issued via cblk_aread interface */
    uint64_t num_awrites;           /* Total number of async writes    */
                                    /* issued via cblk_awrite interface*/
    uint32_t num_act_areads;        /* Current number of reads active  */
                                    /* via cblk_read interface         */
    uint32_t num_act_awrites;       /* Current number of writes active */
                                    /* via cblk_write interface        */
    uint32_t max_num_act_awrites;   /* High water mark on the maximum  */
                                    /* number of asyync writes active  */
                                    /* at once.                        */
    uint32_t max_num_act_areads;    /* High water mark on the maximum  */
    uint64_t num_error_areads;      /* Total number of async reads     */
                                    /* failed via cblk_aread interface */
    uint64_t num_error_awrites;     /* Total number of async writes    */
                                    /* failed via cblk_awrite interface*/
    uint64_t num_success_threads;   /* Total number of pthread_creates */
                                    /* that succeed.                   */
    uint64_t num_failed_threads;    /* Total number of pthread_creates */
                                    /* that failed.                    */
    uint64_t num_active_threads;    /* Current number of threads       */
                                    /* running.                        */

 
};



/************************************************************************/
/* cflsh_usfs - The data structure for a file system                     */
/*             for this process                                         */
/* .                                                                    */
/************************************************************************/
typedef struct cflsh_usfs_s {
    struct cflsh_usfs_s *prev;      /* Previous filesystem in list     */
    struct cflsh_usfs_s *next;      /* Next filesystem in list         */
    uint8_t in_use;                 /* This filessytem is in use       */
    int flags;                      /* Flags for this filesystem       */
#define CFLSH_USFS_FLG_SB_VAL 0x0001 /* Superblock valid               */
#define CFLSH_USFS_FLG_FT_VAL 0x0002 /* Free block table valid         */
#define CFLSH_USFS_FLG_IT_VAL 0x0004 /* Inode table valid              */
#define CFLSH_USFS_FLG_FAIL_FORK 0x0008 /* The child process this      */
				    /* file system is associated with  */
				    /* encountered errors on the fork  */
    int        index;               /* Index of this cufs              */
    chunk_id_t chunk_id;            /* Chunk index number of disk      */
				    /* for this filesystem             */

    
    uint64_t   fs_block_size;       /* Block size referenced by inodes */
    uint64_t   num_blocks;          /* Size of file system in blocks   */
    uint64_t   num_inode_data_blocks; /* Number of data blocks (where  */
				      /* the block size is inode block */
				      /* size, which is the block_size */ 
				      /* field in this struct.         */
    uint64_t   size_free_blk_entries; /* size of free block table      */
				      /* entries in bytes              */

    uint32_t   disk_block_size;      /* Block size used by disk         */

    int max_requests;
    int max_xfer_size;              /* Maxium transfer size in blocks  */
    void *buf;                      /* Buffer for this filesystem      */
    int buf_size;                   /* Size of memory referenced by buf*/
    void *free_blk_tbl_buf;
    int free_blk_tbl_buf_size;
    void *inode_tbl_buf;
    int inode_tbl_buf_size;
    chunk_stats_t disk_stats;
    cflsh_usfs_super_block_t superblock;
    //?? stats   
    cflsh_usfs_data_obj_t *root_directory; /* Root directory of filesystem */

    cflsh_usfs_data_obj_t *filelist_head; /* Head of queue of files in  */
					 /* use on this filesystem     */

    cflsh_usfs_data_obj_t *filelist_tail; /* Tail of queue of files in  */
					 /* use on this filesystem     */

    cflsh_usfs_journal_ev_t *add_journal_ev_head;/* Head of journal events   */
					   /* to be added for this         */
					   /* filesystem                   */

    cflsh_usfs_journal_ev_t *add_journal_ev_tail;/* Tail of journal events   */
					   /* to be added for this         */
					   /* filesystem                   */
    char device_name[PATH_MAX];
    char *udid;

    pthread_t thread_id;                   /* Async completion thread id   */
    int thread_flags;                      /* Flags passed to background   */
					   /* async I/O handler.           */
#define CFLSH_CUFS_EXIT_ASYNC  0x0001      /* Thread should exit           */
    pthread_cond_t thread_event;           /* Thread event for this file-  */
					   /* system.                      */
    struct cflsh_usfs_async_thread_s async_data;/* Data passed o           */
                                           /* async thread handler.        */

    cflsh_usfs_master_hndl_t master_handle;
    cusfs_lock_t   lock;
    cusfs_lock_t   free_table_lock;
    cusfs_lock_t   inode_table_lock;
    cusfs_lock_t   journal_lock;

    int async_pool_size;            /* Size of async I/O poll   */
				    /* in bytes.                */
    int async_pool_num_cmds;        /* Number of async I/O      */
				    /* commands in async_poll.  */
    cflsh_usfs_aio_entry_t *async_pool;/* Pool of async I/O     */
				       /* requests              */
    cflsh_usfs_aio_entry_t *head_free;/* Head of free async     */
                                      /* I/O                    */
    cflsh_usfs_aio_entry_t *tail_free;/* Tail of free async     */
                                      /* I/O                    */
    cflsh_usfs_aio_entry_t *head_act; /* Head of active async   */
                                      /* I/O                    */
    cflsh_usfs_aio_entry_t *tail_act; /* Tail of active async   */
                                      /* I/O                    */
    cflsh_usfs_aio_entry_t *head_cmplt; /* Head of complete     */
                                      /* asyc I/O               */
    cflsh_usfs_aio_entry_t *tail_cmplt; /* Tail of complete     */
                                      /* async I/O              */

    struct cflsh_usfs_stat  stats;  /* Statistics for this fs    */
    eye_catch4b_t  eyec;           /* Eye catcher                      */

} cflsh_usfs_t;

#ifndef _AIX
#define CFLSH_USFS_HOST_TYPE_FILE  "/proc/cpuinfo"



#endif  /* ! _AIX */

typedef
enum {
    CFLSH_USFS_HOST_UNKNOWN   = 0,  /* Unknown host type                */
    CFLSH_USFS_HOST_NV        = 1,  /* Bare Metal (or No virtualization */
                                    /* host type.                       */
    CFLSH_USFS_HOST_PHYP      = 2,  /* pHyp host type                   */
    CFLSH_USFS_HOST_KVM       = 3,  /* KVM host type                    */
} cflsh_usfs_host_type_t;


/************************************************************************/
/* cusfs_global - Global library data structure per process              */
/************************************************************************/

typedef struct cusfs_global_s {
#ifdef _USE_RW_LOCK
    cusfs_rwlock_t global_lock;
#else
    cusfs_lock_t   global_lock;
#endif
    int flags;                        /* Global flags for this chunk */
#define CFLSH_G_SYSLOG       0x0001   /* Use syslog for all tracing  */
    int next_cusfs_id;                 /* Id of next allocated        */
                                      /* filesystem.                 */
    int next_file_id;                 /* Id of next allocated        */
                                      /* file.                       */

    cflsh_usfs_host_type_t  host_type; /* Host type                   */
    cflsh_usfs_os_type_t   os_type;    /* Current OS type             */

    pid_t  caller_pid;                /* Process ID of caller of     */
                                      /* this library.               */

    char *process_name;               /* Name of process using this  */
                                      /* library if known.           */
    int num_active_cufs;              /* Number of active filesystems*/
    int num_max_active_cufs;          /* Maximum number of active    */
                                      /* filesystems seen at a time. */
    int num_bad_file_ids;             /* Number of times we see a    */
                                      /* a bad cufs id.              */
    int num_bad_cusfs_ids;             /* Number of times we see a    */
                                      /* a bad cufs id.              */

    cflsh_usfs_data_obj_t *file_hash[MAX_NUM_CUSFS_FILE_HASH];
    cflsh_usfs_t *hash[MAX_NUM_CUSFS_HASH];


    trace_log_ext_arg_t trace_ext;    /* Extended argument for trace */
    uint32_t thread_log_mask;         /* Mask used to hash thread    */
                                      /* logs into specific files.    */
    cflsh_thread_log_t *thread_logs;  /* Array of log files per thread*/  
            
#ifdef _SKIP_READ_CALL
    int         adap_poll_delay;/* Adapter poll delay time in  */
                                /* microseconds                */
#endif /* _SKIP_READ_CALL */
    eye_catch4b_t  eyec;              /* Eye catcher                  */


} cusfs_global_t;

extern cusfs_global_t cusfs_global;


/* Compile time check suitable for use in a function */
#define CFLSH_USFS_COMPILE_ASSERT(test)                             \
do {                                                            \
     struct __Fo0 { char v[(test) ? 1 : -1]; } ;                \
} while (0)



#endif /* _H_CFLASH_USFS_INT */
