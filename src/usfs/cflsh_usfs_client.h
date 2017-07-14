/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/mclient.h $                                       */
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
#ifndef _H_CFLASH_USFS_CLIENT
#define _H_CFLASH_USFS_CLIENT


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>


/***************************************************************************
  This file defines IPC messages to the master context deamon.

  The daemon listens on a unique per AFU UNIX socket. The
  name of the socket can be derived from the disk as follows:
      disk_device = /dev/sg4
      corresponding socket = CFLSH_USFS_MASTER_SOCKET_DIR/dev/sg4
      If CFLSH_USFS_MASTER_SOCKET_DIR is /tmp, the socket is /tmp/dev/sg4

  A client wishing to do direct IPC with the master selects the
  correct socket name to connect to based on the disk. 
***************************************************************************/

#define CFLSH_USFS_MASTER_SOCKET_DIR "/etc/cxlflash/data"

#define XFER_OP_READ      1      // Read from the Blk-MC Socket
#define XFER_OP_WRITE     2      // Write to the Blk-MC Socket

/* Commad opcodes in IPC requests */
#define CFLSH_USFS_CMD_MASTER_REG    1 // Register an filesystem + context handle with master
#define CFLSH_USFS_CMD_MASTER_UNREG  2 // Unregister a context handle with master

#define CFLSH_USFS_CMD_MASTER_LOCK   3 // Acquire a shared lock for some filesystem critical resource
                                       // (i.e. free block table, inode table, journal etc).
#define CFLSH_USFS_CMD_MASTER_UNLOCK 4 // Release a shared lock for some filesystem critical resource


typedef enum {
    CFLSH_USFS_MASTER_FS_LOCK          = 1,  /* Filesystem lock       */
    CFLSH_USFS_MASTER_FREE_TABLE_LOCK  = 2,  /* Free block table lock */
    CFLSH_USFS_MASTER_INODE_TABLE_LOCK = 3,  /* Inode table lock      */
    CFLSH_USFS_MASTER_JOURNAL_LOCK     = 4,  /* Journal lock          */
    CFLSH_USFS_MASTER_LAST_ITEM        = 5,  /* Journal lock          */
} cflsh_usfs_master_lock_t;



typedef struct cflsh_master_req_header
{
  int   version; // 0
  int   command;
  int   size;
  int   tag;     // command tag to identify the active request
} cflsh_usfs_master_req_header_t;


typedef struct cflsh_usfs_master_resp_header
{
  int  version; // 0
  int  command; // same as command in the request
  int  size;
  int  tag;     // same as tag in request
  int      status;  // 0: success, otherwise set to a errno value
} cflsh_usfs_master_resp_header_t;

typedef struct master_req
{
  cflsh_usfs_master_req_header_t  header;

  union {
    // The client sends a MASTER_REG with the challenge field set to something
    // the server can use to validate that the client is the true owner of
    // the AFU context it wants to register. The specifics of this protocol
    // is left to the implementation.
    //
    // Since the server remembers the disk (implicitly tied to connection) and 
    // the registered ctx_hndl, these are not needed to be sent again in
    // subsequent IPC messages.
    //
    // MASTER_REG must be the first command on a new connection. If the registation
    // fails, the MCREG can be retried on the same connection any number of
    // times. Once successful, subsequent MCREGs are failed until after a 
    // MCUNREG.
    //
    struct {
      pid_t            client_pid;
      int              client_fd;
      int              mode;
#define MCREG_INITIAL_REG         0x1   // fresh registration
#define MCREG_DUP_REG             0x0   // dup
//      ctx_hndl_t       ctx_hndl;
	uint64_t challenge;
    } reg;

    // After MCUNREG, the client can send a new MCREG on the existing 
    // connection or close it.
    //
    struct {
    } unreg;

    struct {
	cflsh_usfs_master_lock_t lock;
	uint64_t            flags;
    } lock;


  };
} cflsh_usfs_master_req_t;

typedef struct mc_resp {
  cflsh_usfs_master_resp_header_t  header;

  union {
    struct {
    } reg;

    struct {
    } unreg;

    struct {
	int foo;
//      res_hndl_t       res_hndl;
    } open;

    struct {
    } close;

    struct {
    } dup;

  };

} cflsh_usfs_master_resp_t;

/****************************************************************************
 * Each user process needs two interfaces to use the AFU: 
 *
 *   1. a host transport MMIO map to issue commands directly to the AFU.
 *      The client opens the regular AFU device, registers its process 
 *      context with the AFU driver (CXL_IOCTL_START_WORK ioctl) and 
 *      then mmaps a 64KB host transport that has registers to issue
 *      commands and setup responses.
 *
 *   2. an interface to the master context daemon via a cflsh_usfs_master_hndl_t.
 *      This is used to create & size resource handles which are equivalent
 *      to virtual disks. The resource handle is written into the command 
 *      block (IOARCB) sent to the AFU using interface #1.
 *
 * This header file describes the second interface. See sislite.h for the
 * first interface.
 *
 * User must create interface #1 first. Parameters returned by #1 are 
 * required to establish interface #2.
 *
 * All routines described here return 0 on success and -1 on failure.
 * errno is set on failure.                                                
 ***************************************************************************/

/* Max pathlen - e.g. for AFU device path */
#define MC_PATHLEN       64

/* Permission Flags */
#define MC_RDONLY        0x1u
#define MC_WRONLY        0x2u
#define MC_RDWR          0x3u

/* cflsh_usfs_master_hndl_t is the client end point for communication to the
 * master. After a fork(), the child must call mc_unregister
 * on any inherited mc handles. It is a programming error to pass
 * an inherited mc handle to any other API in the child.
 */
typedef void* cflsh_usfs_master_hndl_t;


/* mc_init & mc_term are called once per process */
int cflsh_usfs_master_init();
int cflsh_usfs_master_term();

/* master lock initilization data */

#ifdef _MASTER_LOCK

typedef struct
{
   key_t  cusfs_fs_key;
   int     cusfs_fs_semid ;
} cusfs_fs_sem_t ;

typedef struct
{
   key_t  cusfs_frtb_key;
   int     cusfs_frtb_semid ;
} cusfs_frtb_sem_t ;

typedef struct
{
   key_t  cusfs_intb_key;
   int     cusfs_intb_semid ;
} cusfs_intb_sem_t ;


typedef struct
{
   key_t  cusfs_jrn_key;
   int     cusfs_jrn_semid ;
} cusfs_jrn_sem_t ;

typedef struct cusfs_serv_sem_s
{
    char master_dev_path[MC_PATHLEN];
    char *name;
    cusfs_fs_sem_t     fs_sem ;
    cusfs_frtb_sem_t  frtb_sem;
    cusfs_intb_sem_t  intb_sem;
    cusfs_jrn_sem_t    jrn_sem;

    /* client IPC */

} __attribute__ ((aligned (0x1000))) cusfs_serv_sem_t;

key_t cusfs_key_gen( char * file_name , cflsh_usfs_master_lock_t cufs_lock);

int cusfs_create_sem( key_t cufs_sem_key );

int cusfs_rm_sem( int  cufs_sem_id );

int  cusfs_cleanup_sem ( cflsh_usfs_master_hndl_t cflsh_usfs_hndl );

#endif

/*
 * master_register outputs a client handle (cflsh_usfs_master_hndl_t) when successful.
 * The handle is bound to the specified AFU and context handle.
 * Subsequent API calls using the handle apply to that context
 * and that AFU. The specified context handle must be owned by the
 * requesting user process.
 *
 * The cflsh_usfs_master_hndl_t is a client endpoint for communication with the master
 * context. It can be used to set up resources only for the single context 
 * on a single AFU bound to the mc handle.
 *
 * Users opening multiple AFU contexts must create multiple mc handles by
 * registering each context.
 *
 * Client is responsible for serialization when the same mc handle is 
 * shared by multiple threads.
 *
 * Different threads can work on different mc handles in parallel w/o 
 * any serialization.
 *
 * It is possible to mc_register the same (AFU + ctx_hndl) multiple times
 * creating multiple mc endpoints (cflsh_usfs_master_hndl) bound to the same context.
 * However, to accomplish this, the mc_hdup function must be used to 
 * duplicate an existing cflsh_usfs_master_hndl. Calling mc_register more than once with 
 * the same parameters will cancel all but the most recent registation.
 *
 * Inputs:
 *   master_dev_path - e.g. /dev/cxl/afu0.0m
 *                     The path must be to the master AFU device even if the
 *                     user opened the corresponding regular device.
 *
 * Output:
 *   p_cflsh_usfs_master_hndl       - The output cflsh_usfs_master_hndl_t is written using this pointer.
 *                     The cflsh_usfs_master_hndl_t is passed in all subsequent calls.
 *
 */
int cflsh_usfs_master_register(char *master_dev_path, cflsh_usfs_master_hndl_t *p_cflsh_usfs_master_hndl);


/* master_unregister invalidates the mc handle. All resource
 * handles under the input mc handle are released if this cflsh_usfs_master_hndl is 
 * the only reference to those resources.
 *
 * Note that after a dup (see below), two contexts can be referencing 
 * the same resource handles. In that case, unregistering one context 
 * must not release resource handles since they are still referenced 
 * by the other context.
 *
 * Inputs:
 *   cflsh_usfs_master_hndl         - a cflsh_usfs_master_hndl that specifies a (context + AFU)
 *                     to unregister
 *
 * Output:
 *   none
 */
int cflsh_usfs_master_unregister(cflsh_usfs_master_hndl_t cflsh_usfs_master_hndl);


/* mc_open creates a zero sized virtual LBA space or a virtual disk
 * on the AFU and context bound to the mc handle.
 *
 * Inputs:
 *   cflsh_usfs_master_hndl         - mc handle that specifies a (context + AFU)
 *
 *   flags           - permission flags (see #defines)
 *
 * Output:
 *   p_res_hndl      - A resource handle allocated by master is written using
 *                     this pointer. The handle is valid only in the scope of
 *                     the AFU context bound to the input cflsh_usfs_master_hndl or any 
 *                     contexts dup'ed to it. User must not mix up resource 
 *                     handles from different contexts. Resource handle A for 
 *                     AFU context 1 and handle B for AFU context 2 can have the 
 *                     same internal representation, but they refer to two
 *                     completely different virtual LBA spaces.
 */
int cflsh_usfs_master_lock(cflsh_usfs_master_hndl_t cflsh_usfs_master_hndl, cflsh_usfs_master_lock_t lock,
			   uint64_t flags);


/* mc_close deallocates any resources (LBAs) allocated to a virtual 
 * disk if this context is the last reference to those resources.
 *
 * Inputs:
 *   cflsh_usfs_master_hndl         - client handle that specifies a (context + AFU)
 *   res_hndl        - resource handle identifying the virtual disk 
 *                     to close
 *
 * Output:
 *   none
 */
int cflsh_usfs_master_unlock(cflsh_usfs_master_hndl_t cflsh_usfs_master_hndl, cflsh_usfs_master_lock_t lock,
			     uint64_t flags);


#endif  /* ifndef _H_CFLASH_USFS_CLIENT */

