/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_eras_client.c $                        */
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


#define CFLSH_BLK_FILENUM 0x0a00
#ifdef _AIX
#include "cflash_block_eras_client.h"
#else
#include <cflash_block_eras_client.h>
#endif 

#include "cflash_block_inline.h"



/* 

   Two cflsh_blk_hndl_priv_t instances must not share any data, including
   the fixed caller supplied data that is identical for all duplicated
   handles. Sharing implies locking which the design prefers to avoid.
   Different eras handle can be run in parallel in different client threads 
   w/o locks or serialization.

   One cflsh_blk_hndl_priv_t corresponds to one client connection to the server.
   Client code does not care/enforce how many connections can be made.
   It is for server to enforce.

   A connection is bound to a during registration. Multiple
   connections (eras handles) can be bound to the same process.

   This implementation does not prevent a child using a cflsh_blk_hndl inherited
   from its parent. Doing so is considered a programming error and the
   results are undefined.
*/
typedef struct
{
  int conn_fd; 
  uint8_t tag;

  /* save pid of registrant for limited special handling of child after
     a fork */
  pid_t pid;

  /* save off caller supplied parms for mc_hdup etc */
//  ctx_hndl_t ctx_hndl;
  char eras_dev_path[MC_PATHLEN];

} cflsh_blk_hndl_priv_t;


/* 
 * Procedure: uint64_t gen_rand
 *
 * Description: Generate random number
 *
 *
 * Parameters:
 *             
 *
 * Return:     
 */
static uint64_t gen_rand(cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv) 
{
    uint64_t rand;
    uint64_t p_low;

#ifdef TARGET_ARCH_x86_64
    rand = getticks();
#else
    asm volatile ( "mfspr %0, 268" : "=r"(rand) : ); // time base 
#endif

    // add in low 32 bits of malloc'ed address to top 32 bits of TB
    p_low = (((uint64_t)p_cflsh_blk_hndl_priv) & 0xFFFFFFFF);
    rand ^= (p_low << 32);

    return rand;
}


/***************************************************************************** 
 * Procedure: blk_connect
 *
 * Description: Connect to the server entity
 *
 * Parameters:
 *
 * Return:     0 or greater is the file descriptor for the connection
 *             -1 is error
 *****************************************************************************/
static int
blk_connect(char *mdev_path)
{
  struct sockaddr_un svr_addr;
  int conn_fd;
  int rc;
  int retry;
  char *env_eras_socket = getenv("CFLSH_BLK_ERAS_SOCKET");




  // Create a socket file descriptor
  conn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn_fd < 0)
  {
      CBLK_TRACE_LOG_FILE(1,"blk_connect: socket failed: %d (%d)\n",
			   conn_fd, errno);
  }
  else
  {
    bzero(&svr_addr, sizeof(struct sockaddr_un));
    svr_addr.sun_family      = AF_UNIX;

    if (env_eras_socket) {
	strcpy(svr_addr.sun_path, env_eras_socket);
    } else {
	strcpy(svr_addr.sun_path, CFLSH_BLK_ERAS_SOCKET_DIR);
    }
    strcat(svr_addr.sun_path, mdev_path);

    // Connect to the server entity
    for (retry = 0; retry < 3; retry++) {
      // retry to handle ECONNREFUSED, there may be too many pending 
      // connections on the server (SOMAXCONN).
      //
      rc = connect(conn_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
      if (rc == 0) {
	break;
      }
      usleep(100);
    }

    if (rc) {
      CBLK_TRACE_LOG_FILE(1,"block_connect: Connect failed: mdev_path=%s %d (%d)\n",
			  mdev_path,rc, errno);
      close(conn_fd);
      conn_fd = -1;
    }
  }

  return conn_fd;
}

/***************************************************************************** 
 * Procedure: blk_send_command
 *
 * Description: Send a command to the server entity
 *
 * Parameters:
 *             conn_fd:    Socket File Descriptor
 *             cmd:        Command to perform
 *             cmdblock:   Command request block
 *             cmdresp:    Command response block
 *
 * Return:     0, if successful
 *             non-zero otherwise
 *****************************************************************************/ 
static int
blk_send_command(int conn_fd, cflsh_blk_eras_req_t *p_mc_req, cflsh_blk_eras_resp_t *p_mc_resp)
{
  int rc = 0;

  // Send the command block
  rc = cblk_serv_xfer_data(conn_fd, XFER_OP_WRITE, (void *)p_mc_req, sizeof(*p_mc_req));
  if (rc)
  {
    CBLK_TRACE_LOG_FILE(1,"blk_send_command: PID %d failed send: %d\n",
            getpid(), rc);
  }
  else
  {
    // Wait for the response
    rc = cblk_serv_xfer_data(conn_fd, XFER_OP_READ, (void *)p_mc_resp, sizeof(*p_mc_resp));
    if (rc)
    {
      CBLK_TRACE_LOG_FILE(1, "blk_send_command: PID %d failed read: %d\n",
              getpid(), rc);
    }
  }

  return rc;
}


/* 
 * Procedure: cflsh_blk_eras_register_back
 *
 * Description: backend function used by cflsh_blk_eras_register 
 *              after making a connection to server.
 *
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
static int 
cflsh_blk_eras_register_back(cflsh_blk_eras_hndl_t cflsh_blk_hndl, 
				int mode)

{
  cflsh_blk_eras_req_t mc_req;
  cflsh_blk_eras_resp_t mc_resp;
  uint64_t challenge;
  int rc;

  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) cflsh_blk_hndl;

  challenge = gen_rand(p_cflsh_blk_hndl_priv);


  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_blk_hndl_priv->tag++;

  mc_req.header.command = CFLSH_BLK_CMD_ERAS_REG;
  mc_req.reg.client_pid = getpid();
  mc_req.reg.client_fd = p_cflsh_blk_hndl_priv->conn_fd;
  mc_req.reg.mode = mode;
  mc_req.reg.challenge = challenge;

  rc = blk_send_command(p_cflsh_blk_hndl_priv->conn_fd, &mc_req, &mc_resp);


  if (rc != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - transport error rc: %d, errno = %d\n", rc, errno);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CBLK_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n", mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - server errno: %d\n", mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
  }

  return rc;
}


/* 
 * Procedure: cflsh_blk_eras_init
 *
 * Description: Init interfaces with ERAS server.
 *
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int
cblk_eras_init()
{
  return 0;
}

/* 
 * Procedure: cflsh_blk_eras_term
 *
 * Description: Terminate interfaces with ERAS server
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int
cblk_eras_term()
{
  return 0;
}

/* 
 * Procedure: cblk_eras_register
 *
 * Description: Register a client with the ERAS Server
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int 
cblk_eras_register(char *eras_dev_path, cflsh_blk_eras_hndl_t *p_cflsh_blk_hndl)
{

  int conn_fd;
  int rc;
  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv;

  conn_fd = blk_connect(eras_dev_path);
  if (conn_fd < 0) {
    CBLK_TRACE_LOG_FILE(1,"socket failed:%s %d (%d)\n", eras_dev_path,conn_fd, errno);
    return -1; // errno set in  blk_connect()
  }

  p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) malloc(sizeof(cflsh_blk_hndl_priv_t));
  if (p_cflsh_blk_hndl_priv == NULL) {
    close(conn_fd);
    CBLK_TRACE_LOG_FILE(1,"cannot allocate client handle\n");
    errno = ENOMEM;
    return -1;
  }

  // init cflsh_blk_hndl
  memset(p_cflsh_blk_hndl_priv, 0, sizeof(*p_cflsh_blk_hndl_priv));
  p_cflsh_blk_hndl_priv->conn_fd = conn_fd;
  p_cflsh_blk_hndl_priv->tag = 0;
  p_cflsh_blk_hndl_priv->pid = getpid();
  strncpy(p_cflsh_blk_hndl_priv->eras_dev_path, eras_dev_path, 
	  MC_PATHLEN - 1);


  // initial MCREG of ctx_hndl (not a dup)
  rc = cflsh_blk_eras_register_back(p_cflsh_blk_hndl_priv, MCREG_INITIAL_REG);

  if (rc != 0) {
    free(p_cflsh_blk_hndl_priv);
    close(conn_fd);
    return rc; // errno is already set
  }

  *p_cflsh_blk_hndl = p_cflsh_blk_hndl_priv;

  CBLK_TRACE_LOG_FILE(9,"success - on client handle %p\n", 
		       *p_cflsh_blk_hndl);
  return rc;

}



/* 
 * Procedure: cblk_eras_unregister
 *
 * Description: Unregister a client with the ERAS server
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_eras_unregister(cflsh_blk_eras_hndl_t cflsh_blk_hndl)
{

  cflsh_blk_eras_req_t mc_req;
  cflsh_blk_eras_resp_t mc_resp;
  int rc = 0;
  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) cflsh_blk_hndl;

  /* Unregister with server if this is called by parent i.e. the 
     original registrant */
  if (p_cflsh_blk_hndl_priv-> pid == getpid()) { 
      memset(&mc_req, '\0', sizeof(mc_req));
      memset(&mc_resp, '\0', sizeof(mc_resp));
      mc_req.header.size = sizeof(mc_req);
      mc_resp.header.size = sizeof(mc_resp);
      mc_req.header.tag = p_cflsh_blk_hndl_priv->tag++;

      mc_req.header.command = CFLSH_BLK_CMD_ERAS_UNREG;

      rc = blk_send_command(p_cflsh_blk_hndl_priv->conn_fd, &mc_req, &mc_resp);

      if (rc != 0) {
	  CBLK_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n", rc);
	  errno = EIO; // transport error
	  rc = -1;
      }
      else if (mc_resp.header.tag != mc_req.header.tag) {
	  CBLK_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n", 
			       mc_req.header.tag, mc_resp.header.tag);
	  errno = EIO; // this is a response for some other cmd
	  rc = -1;
      }
      else if (mc_resp.header.status != 0) {
	  CBLK_TRACE_LOG_FILE(1,"failed - server errno: %d\n", 
			       mc_resp.header.status);
	  errno = mc_resp.header.status;
	  rc = -1;
      }
      else {
	  rc = 0;

	  CBLK_TRACE_LOG_FILE(9,"success - unregisterd client handle %p\n", cflsh_blk_hndl);

      }
  }

  close(p_cflsh_blk_hndl_priv->conn_fd);
  free(p_cflsh_blk_hndl_priv);
  return rc;

}

/* 
 * Procedure: cblk_eras_set_trace
 *
 * Description: Request the specified shared lock.
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_eras_set_trace(cflsh_blk_eras_hndl_t cflsh_blk_hndl,
			int verbosity,
			char *filename,
			uint64_t flags)
{
  cflsh_blk_eras_req_t mc_req;
  cflsh_blk_eras_resp_t mc_resp;
  int rc;
  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) cflsh_blk_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_blk_hndl_priv->tag++;

  mc_req.header.command = CFLSH_BLK_CMD_ERAS_SET_TRACE;
  mc_req.trace.verbosity = verbosity;
  if (filename) {
      strcpy(mc_req.trace.filename,filename);
  }
  mc_req.trace.flags = flags;

  rc = blk_send_command(p_cflsh_blk_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n",rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CBLK_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n",
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - server errno: %d\n",
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;


    CBLK_TRACE_LOG_FILE(9,"success - verbosity %d, flags 0x%x, filename %s\n",  
			verbosity,flags,filename);

  }

  return rc;
}


/* 
 * Procedure: cblk_live_dump
 *
 * Description: Request the specified shared lock.
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_eras_live_dump(cflsh_blk_eras_hndl_t cflsh_blk_hndl,
			char *reason,
			uint64_t flags)
{
  cflsh_blk_eras_req_t mc_req;
  cflsh_blk_eras_resp_t mc_resp;
  int rc;
  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) cflsh_blk_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_blk_hndl_priv->tag++;

  mc_req.header.command = CFLSH_BLK_CMD_ERAS_LIVE_DUMP;
  strcpy(mc_req.live_dump.reason,reason);
  mc_req.live_dump.flags = flags;

  rc = blk_send_command(p_cflsh_blk_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n",rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CBLK_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n",
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - server errno: %d\n",
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;


    CBLK_TRACE_LOG_FILE(9,"success - reason %s\n",  reason);

  }

  return rc;
}

/* 
 * Procedure: cblk_eras_live_dump
 *
 * Description: Request the specified shared lock.
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_eras_get_stat(cflsh_blk_eras_hndl_t cflsh_blk_hndl, 
		       char *disk_name, chunk_stats_t *stat,
		       uint64_t flags)
{
  cflsh_blk_eras_req_t mc_req;
  cflsh_blk_eras_resp_t mc_resp;
  int rc;
  cflsh_blk_hndl_priv_t *p_cflsh_blk_hndl_priv = (cflsh_blk_hndl_priv_t *) cflsh_blk_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_blk_hndl_priv->tag++;

  mc_req.header.command = CFLSH_BLK_CMD_ERAS_GET_STATS;
  strcpy(mc_req.stat.disk_name,disk_name);
  mc_req.stat.flags = flags;

  rc = blk_send_command(p_cflsh_blk_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n",rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CBLK_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n",
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CBLK_TRACE_LOG_FILE(1,"failed - server errno: %d\n",
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
    *stat = mc_resp.stat.stat;

    CBLK_TRACE_LOG_FILE(9,"success - disk_name %s\n",  disk_name);

  }

  return rc;
}
