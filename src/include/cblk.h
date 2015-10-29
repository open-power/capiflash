/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cblk.h $                                          */
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
#ifndef _CBLK_H
#define _CBLK_H

#include <inttypes.h>
#include <sislite.h>
#include <mclient.h>

/***************************************************************************
  This file defines IPC messages to the master context deamon.

  The daemon listens on a unique per AFU UNIX socket. The
  name of the socket can be derived from the path name to the
  master AFU device node as follows:
      master_device = /dev/cxl/afu0.0m
      corresponding socket = MC_SOCKET_DIR/dev/cxl/afu0.0m
      If MC_SOCKET_DIR is /tmp, the socket is /tmp/dev/cxl/afu0.0m

  A client wishing to do direct IPC with the master selects the
  correct socket name to connect to based on the target AFU. All 
  messages exchanged on that connection are executed by the daemon
  in the context of the AFU implied by the socket name.
***************************************************************************/

#define MC_SOCKET_DIR "/opt/ibm/capikv/data"

#define XFER_OP_READ      1      // Read from the Blk-MC Socket
#define XFER_OP_WRITE     2      // Write to the Blk-MC Socket

/* Commad opcodes in IPC requests */
#define CMD_MCREG         1      // Register an AFU + context handle with MC
#define CMD_MCUNREG       2      // Unregister a context handle with MC

#define CMD_MCOPEN        3      // Get a new & zero sized Resource Handle
#define CMD_MCCLOSE       4      // Free a Resouce Handle

#define CMD_MCSIZE        5      // Resize a Resouce Handle (grow or shrink)
#define CMD_UNUSED        6

#define CMD_MCXLATE_LBA   7      // Translate Virtual LBA to Physical LBA

#define CMD_MCCLONE       8      // Clone Resource Handles from a context

#define CMD_MCDUP         9      // Dup a one context to another

#define CMD_MCSTAT        10     // query a Resource Handle

#define CMD_MCNOTIFY      11     // Notify master of certain events

typedef struct mc_req_header
{
  uint8_t   version; // 0
  uint8_t   command;
  uint8_t   size;
  uint8_t   tag;     // command tag to identify the active request
} mc_req_header_t;


typedef struct mc_resp_header
{
  uint8_t  version; // 0
  uint8_t  command; // same as command in the request
  uint8_t  size;
  uint8_t  tag;     // same as tag in request
  int      status;  // 0: success, otherwise set to a errno value
} mc_resp_header_t;

typedef struct mc_req
{
  mc_req_header_t  header;

  union {
    // The client sends a MCREG with the challenge field set to something
    // the server can use to validate that the client is the true owner of
    // the AFU context it wants to register. The specifics of this protocol
    // is left to the implementation.
    //
    // Since the server remembers the afu (implicitly tied to connection) and 
    // the registered ctx_hndl, these are not needed to be sent again in
    // subsequent IPC messages.
    //
    // MCREG must be the first command on a new connection. If the registation
    // fails, the MCREG can be retried on the same connection any number of
    // times. Once successful, subsequent MCREGs are failed until after a 
    // MCUNREG.
    //
    struct {
      pid_t            client_pid;
      int              client_fd;
      __u8             mode;
#define MCREG_INITIAL_REG         0x1   // fresh registration
#define MCREG_DUP_REG             0x0   // dup
      ctx_hndl_t       ctx_hndl;
      __u64            challenge;
    } reg;

    // After MCUNREG, the client can send a new MCREG on the existing 
    // connection or close it.
    //
    struct {
    } unreg;

    struct {
      __u64            flags;
    } open;

    struct {
      res_hndl_t       res_hndl;
    } close;

    struct {
      res_hndl_t       res_hndl;
      __u64            new_size;   //  size in chunks
    } size;

    struct {
      res_hndl_t       res_hndl;
      __u64            v_lba;
    } xlate_lba;

    struct {
      ctx_hndl_t       ctx_hndl_src;
      __u64            flags;
      __u64            challenge;
    } clone;

    struct {
      ctx_hndl_t       ctx_hndl_cand;
      __u64            challenge;
    } dup;

    struct {
      res_hndl_t       res_hndl; 
    } stat;

    mc_notify_t notify;

  };
} mc_req_t;


typedef struct mc_resp
{
  mc_resp_header_t  header;

  union {
    struct {
    } reg;

    struct {
    } unreg;

    struct {
      res_hndl_t       res_hndl;
    } open;

    struct {
    } close;

    struct {
      __u64            act_new_size;
    } size;

    struct {
      __u64            p_lba;
    } xlate_lba;

    struct {
    } clone;

    struct {
    } dup;

    mc_stat_t stat;

    struct {
    } notify;

  };

} mc_resp_t;


#endif
