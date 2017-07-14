/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_block_eras_client.h $                      */
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
#ifndef _H_CFLASH_BLK_ERAS_CLIENT
#define _H_CFLASH_BLK_ERAS_CLIENT


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
#include <limits.h>
#include <capiblock.h>

/***************************************************************************
  This file defines IPC messages to a block library daemon (thread).

  The daemon listens on a unique per PID (process id) UNIX socket. The
  name of the socket can be derived from the PID as follows:
      corresponding socket = CFLSH_BLK_ERAS_SOCKET_DIR/PID
      If CFLSH_BLK_ERAS_SOCKET_DIR is /tmp, and the PID is 12345 
       the socket is /tmp/12345

  A client wishing to do direct IPC with the block library daemon selects the
  correct socket name to connect to based on the disk. 
***************************************************************************/

#define CFLSH_BLK_ERAS_SOCKET_DIR "/etc/cxlflash/data/cblk_eras/"

#define XFER_OP_READ      1      // Read from the Blk-MC Socket
#define XFER_OP_WRITE     2      // Write to the Blk-MC Socket

typedef enum {
    CFLSH_BLK_CMD_ERAS_REG                  = 1, // Register an pid 
    CFLSH_BLK_CMD_ERAS_UNREG                = 2, // Unregister a pid

    CFLSH_BLK_CMD_ERAS_SET_TRACE            = 3, // Set trace information.
                                                 // (i.e. verbosity etc).
    CFLSH_BLK_CMD_ERAS_GET_STATS            = 4, // Get statistics
    CFLSH_BLK_CMD_ERAS_LIVE_DUMP            = 5, // Do a live dump
    CFLSH_BLK_ERAS_LAST_ITEM                = 6, // Last entry         
} cflsh_blk_eras_cmd_t;



typedef struct cflsh_eras_req_header
{
  int   version; // 0
  cflsh_blk_eras_cmd_t   command;
  int   size;
  int   tag;     // command tag to identify the active request
} cflsh_blk_eras_req_header_t;


typedef struct cflsh_blk_eras_resp_header
{
  int  version; // 0
  cflsh_blk_eras_cmd_t  command; // same as command in the request
  int  size;
  int  tag;     // same as tag in request
  int      status;  // 0: success, otherwise set to a errno value
} cflsh_blk_eras_resp_header_t;

typedef struct eras_req
{
  cflsh_blk_eras_req_header_t  header;

  union {
    // The client sends a eras_req with the challenge field set to something
    // the server can use to validate that the client is the true owner of
    // the AFU context it wants to register. The specifics of this protocol
    // is left to the implementation.
    //
    // ERAS_REG must be the first command on a new connection. If the registation
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
	uint64_t challenge;
    } reg;

    // After MCUNREG, the client can send a new MCREG on the existing 
    // connection or close it.
    //
    struct {
    } unreg;

    struct {
	int        verbosity;
	char       filename[PATH_MAX];   /* Trace filename              */
	uint64_t   flags;
#define CBLK_TRC_CLIENT_SET_VERBOSITY 0x1   /* Change trace verbosity */
#define CBLK_TRC_CLIENT_SET_FILENAME  0x2   /* Change trace filename */

    } trace;

    struct {
	char       reason[PATH_MAX];
	uint64_t   flags;
    } live_dump;

    struct {
	char       disk_name[PATH_MAX];
	uint64_t   flags;
    } stat;


  };
} cflsh_blk_eras_req_t;

typedef struct mc_resp {
  cflsh_blk_eras_resp_header_t  header;

  union {
    struct {
    } reg;

    struct {
    } unreg;

    struct {
	chunk_stats_t stat;
    } stat;

  };

} cflsh_blk_eras_resp_t;

/****************************************************************************
 * Each user process needs two interfaces to use the AFU: 
 *
 *   1. a host transport MMIO map to issue commands directly to the AFU.
 *      The client opens the regular AFU device, registers its process 
 *      context with the AFU driver (CXL_IOCTL_START_WORK ioctl) and 
 *      then mmaps a 64KB host transport that has registers to issue
 *      commands and setup responses.
 *
 *   2. an interface to the eras daemon via a cflsh_blk_eras_hndl_t.
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

/* cflsh_blk_eras_hndl_t is the client end point for communication to the
 * ERAS daemon. After a fork(), the child must call mc_unregister
 * on any inherited mc handles. It is a programming error to pass
 * an inherited mc handle to any other API in the child.
 */
typedef void* cflsh_blk_eras_hndl_t;


/* eras_init & eras_term are called once per process */
int cblk_eras_init();
int cblk_eras_term();




/*
 * eras_register outputs a client handle (cflsh_blk_eras_hndl_t) when successful.
 * The handle is bound to the specified process.
 * Subsequent API calls using the handle apply to that process.
 * The specified context handle must be owned by the
 * requesting user process.
 *
 * The cflsh_blk_eras_hndl_t is a client endpoint for communication with the eras
 * server. It can be used to set up resources only for the single process.
 *
 *
 * Client is responsible for serialization when the same handle is 
 * shared by multiple threads.
 *
 * Different threads can work on different mc handles in parallel w/o 
 * any serialization.
 *
 *
 * Inputs:
 *   eras_dev_path - e.g. Process ID string
 *                     The path must be to the eras PID
 *                     user opened the corresponding regular device.
 *
 * Output:
 *   p_cflsh_blk_eras_hndl       - The output cflsh_blk_eras_hndl_t is written using this pointer.
 *                     The cflsh_blk_eras_hndl_t is passed in all subsequent calls.
 *
 */
int cblk_eras_register(char *eras_dev_path, cflsh_blk_eras_hndl_t *p_cflsh_blk_eras_hndl);


/* eras_unregister invalidates the handle. All resource
 * handles under the input handle are released if this cflsh_blk_eras_hndl is 
 * the only reference to those resources.
 *
 * Inputs:
 *   cflsh_blk_eras_hndl  - a cflsh_blk_eras_hndl that specifies a process
 *                          to unregister
 *
 * Output:
 *   none
 */
int cblk_eras_unregister(cflsh_blk_eras_hndl_t cflsh_blk_eras_hndl);


/* cblk_eras_set_trace: Update trace settings: verbosity, filename
 * 
 *
 * Inputs:
 *   cflsh_blk_eras_hndl - mc handle that specifies a process
 *
 *   flags               - flags: see flags defineds for trace union
 *                         of cflsh_blk_eras_resp_t type.
 *
 * Output:
 */
int cblk_eras_set_trace(cflsh_blk_eras_hndl_t cflsh_blk_eras_hndl, int verbosity, char *filename,
		    uint64_t flags);


/* cblk_eras_live_dump: Live dump block library for this process.
 * 
 *
 * Inputs:
 *   cflsh_blk_eras_hndl - mc handle that specifies a process
 *
 *   flags               - flags
 *
 * Output:
 */
int cblk_eras_live_dump(cflsh_blk_eras_hndl_t cflsh_blk_eras_hndl, char *reason, 
			uint64_t flags);


/* cblk_eras_get_stats: Get statistics for the specified disk
 * 
 *
 * Inputs:
 *   cflsh_blk_eras_hndl  - mc handle that specifies a process
 *
 *   flags                - flags
 *
 * Output:
 */
int cblk_eras_get_stat(cflsh_blk_eras_hndl_t cflsh_blk_eras_hndl, 
		       char *disk_name, chunk_stats_t *stat,
		       uint64_t flags);


#endif  /* ifndef _H_CFLASH_BLK_ERAS_CLIENT */

