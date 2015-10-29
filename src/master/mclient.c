/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/mclient.c $                                        */
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
#include <mclient.h>
#include <cblk.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>


/* Two mc_hndl_priv_t instances must not share any data, including
   the fixed caller supplied data that is identical for all duplicated
   handles. Sharing implies locking which the design prefers to avoid.
   Different mc handle can be run in parallel in different client threads 
   w/o locks or serialization.

   One mc_hndl_priv_t corresponds to one client connection to the server.
   Client code does not care/enforce how many connections can be made.
   It is for server to enforce.

   A connection is bound to a AFU context during registration. Multiple
   connections (mc handles) can be bound to the same context.

   This implementation does not prevent a child using a mc_hndl inherited
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
  ctx_hndl_t ctx_hndl;
  char master_dev_path[MC_PATHLEN];
  volatile struct sisl_host_map* p_mmio_map;
} mc_hndl_priv_t;


static __u64 gen_rand(mc_hndl_priv_t *p_mc_hndl_priv) {
    __u64 rand;
    __u64 p_low;

    asm volatile ( "mfspr %0, 268" : "=r"(rand) : ); // time base 

    // add in low 32 bits of malloc'ed address to top 32 bits of TB
    p_low = (((__u64)p_mc_hndl_priv) & 0xFFFFFFFF);
    rand ^= (p_low << 32);

    return rand;
}

/* 
 * Procedure: xfer_data
 *
 * Description: Perform a transfer operation for the given
 *              socket file descriptor.
 *
 * Parameters:
 *             fd:         Socket File Descriptor
 *             op:         Read or Write Operation
 *             buf:        Buffer to either read from or write to
 *             exp_size:   Size of data transfer
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
static int
xfer_data(int fd, int op, void *buf, ssize_t exp_size)
{
  int rc = 0;
  ssize_t        offset      = 0;
  ssize_t        bytes_xfer  = 0;
  ssize_t        target_size = exp_size;
  struct iovec   iov;
  struct msghdr  msg;

  while ( 1 )
  {
      // Set up IO vector for IO operation.
      memset(&msg, 0, sizeof(struct msghdr));
      iov.iov_base   = buf + offset;
      iov.iov_len    = target_size;
      msg.msg_iov    = &iov;
      msg.msg_iovlen = 1;

      // Check to see if we are sending or receiving data
      if ( op == XFER_OP_READ )
      {
        bytes_xfer = recvmsg(fd, &msg, MSG_WAITALL);
      } 
      else
      {
        bytes_xfer = sendmsg(fd, &msg, MSG_NOSIGNAL);
      }

      if ( -1 == bytes_xfer )
      {
        if ( EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
        {
          // just retry the whole request
          continue;
        }
        else
        {
          // connection closed by the other end
          rc = 1;
          break;
        }
      }
      else if ( 0 == bytes_xfer )
      {
        // connection closed by the other end
        rc = 1;
        break;
      }
      else if ( bytes_xfer == target_size )
      {
        // We have transfered all the bytes we wanted, we
	// can stop now.
        rc = 0;
	break;
      }
      else
      {
        // less than target size - partial  condition
        // set up to transfer for the remainder of the request
        offset += bytes_xfer;
        target_size = (target_size - bytes_xfer);
      }
  }

  return rc;
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

  // Create a socket file descriptor
  conn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn_fd < 0)
  {
    fprintf(stderr, "blk_connect: socket failed: %d (%d)\n",
            conn_fd, errno);
  }
  else
  {
    bzero(&svr_addr, sizeof(struct sockaddr_un));
    svr_addr.sun_family      = AF_UNIX;
    strcpy(svr_addr.sun_path, MC_SOCKET_DIR);
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
      fprintf(stderr, "block_connect: Connect failed: %d (%d)\n",
	      rc, errno);
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
blk_send_command(int conn_fd, mc_req_t *p_mc_req, mc_resp_t *p_mc_resp)
{
  int rc = 0;

  // Send the command block
  rc = xfer_data(conn_fd, XFER_OP_WRITE, (void *)p_mc_req, sizeof(*p_mc_req));
  if (rc)
  {
    fprintf(stderr, "blk_send_command: PID %d failed send: %d\n",
            getpid(), rc);
  }
  else
  {
    // Wait for the response
    rc = xfer_data(conn_fd, XFER_OP_READ, (void *)p_mc_resp, sizeof(*p_mc_resp));
    if (rc)
    {
      fprintf(stderr, "blk_send_command: PID %d failed read: %d\n",
              getpid(), rc);
    }
  }

  return rc;
}



int
mc_init()
{
  return 0;
}

int
mc_term()
{
  return 0;
}

// backend function used by mc_register after making a connection
// to server.
//
static int 
mc_register_back(mc_hndl_t mc_hndl, ctx_hndl_t ctx_hndl, 
		 __u8 mode)

{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  __u64 challenge;
  int rc;

  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  challenge = gen_rand(p_mc_hndl_priv);
  write_64(&p_mc_hndl_priv->p_mmio_map->mbox_w, challenge);

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCREG;
  mc_req.reg.client_pid = getpid();
  mc_req.reg.client_fd = p_mc_hndl_priv->conn_fd;
  mc_req.reg.mode = mode;
  mc_req.reg.ctx_hndl = ctx_hndl;
  mc_req.reg.challenge = challenge;

  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  // this is important: protect our context from someone trying to
  // claim as their's by locking out the mbox.
  write_64(&p_mc_hndl_priv->p_mmio_map->mbox_w, 0);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
  }

  return rc;
}

int 
mc_register(char *master_dev_path, ctx_hndl_t ctx_hndl, 
	    volatile __u64 *p_mmio_map, mc_hndl_t *p_mc_hndl)
{
  int conn_fd;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv;

  conn_fd = blk_connect(master_dev_path);
  if (conn_fd < 0) {
    fprintf(stderr, "socket failed: %d (%d)\n", conn_fd, errno);
    return -1; // errno set in  blk_connect()
  }

  p_mc_hndl_priv = (mc_hndl_priv_t *) malloc(sizeof(mc_hndl_priv_t));
  if (p_mc_hndl_priv == NULL) {
    close(conn_fd);
    fprintf(stderr, "cannot allocate client handle\n");
    errno = ENOMEM;
    return -1;
  }

  // init mc_hndl
  memset(p_mc_hndl_priv, 0, sizeof(*p_mc_hndl_priv));
  p_mc_hndl_priv->conn_fd = conn_fd;
  p_mc_hndl_priv->tag = 0;
  p_mc_hndl_priv->pid = getpid();
  strncpy(p_mc_hndl_priv->master_dev_path, master_dev_path, 
	  MC_PATHLEN - 1);
  p_mc_hndl_priv->ctx_hndl = ctx_hndl;
  p_mc_hndl_priv->p_mmio_map = (volatile struct sisl_host_map*) p_mmio_map;

  // initial MCREG of ctx_hndl (not a dup)
  rc = mc_register_back(p_mc_hndl_priv, ctx_hndl, MCREG_INITIAL_REG);

  if (rc != 0) {
    free(p_mc_hndl_priv);
    close(conn_fd);
    return rc; // errno is already set
  }

  *p_mc_hndl = p_mc_hndl_priv;
#if 0
  printf("%s success - registered ctx_hndl %d on client handle %p\n", 
	 __func__,
	 ctx_hndl, *p_mc_hndl);
#endif
  return 0;
}


int 
mc_hdup(mc_hndl_t mc_hndl,  mc_hndl_t *p_mc_hndl)
{
  int conn_fd;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv;
  mc_hndl_priv_t *p_mc_hndl_orig = (mc_hndl_priv_t *) mc_hndl;

  conn_fd = blk_connect(p_mc_hndl_orig->master_dev_path);
  if (conn_fd < 0) {
    fprintf(stderr, "socket failed: %d (%d)\n", conn_fd, errno);
    return -1; // errno set in  blk_connect()
  }

  p_mc_hndl_priv = (mc_hndl_priv_t *) malloc(sizeof(mc_hndl_priv_t));
  if (p_mc_hndl_priv == NULL) {
    close(conn_fd);
    fprintf(stderr, "cannot allocate client handle\n");
    errno = ENOMEM;
    return -1;
  }

  // init dup'ed mc_hndl
  memcpy(p_mc_hndl_priv, p_mc_hndl_orig, sizeof(*p_mc_hndl_priv));
  p_mc_hndl_priv->conn_fd = conn_fd;
  p_mc_hndl_priv->tag = 0;
  p_mc_hndl_priv->pid = getpid();

  // duplicate MCREG 
  rc = mc_register_back(p_mc_hndl_priv, p_mc_hndl_priv->ctx_hndl, 
			MCREG_DUP_REG);

  if (rc != 0) {
    free(p_mc_hndl_priv);
    close(conn_fd);
    return rc; // errno is already set
  }

  *p_mc_hndl = p_mc_hndl_priv;
#if 0
  printf("%s success - registered ctx_hndl %d on client handle %p\n", 
	 __func__,
	 p_mc_hndl_priv->ctx_hndl, *p_mc_hndl);
#endif
  return 0;
}



int mc_unregister(mc_hndl_t mc_hndl)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc = 0;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  /* Unregister with server if this is called by parent i.e. the 
     original registrant */
  if (p_mc_hndl_priv-> pid == getpid()) { 
      memset(&mc_req, '\0', sizeof(mc_req));
      memset(&mc_resp, '\0', sizeof(mc_resp));
      mc_req.header.size = sizeof(mc_req);
      mc_resp.header.size = sizeof(mc_resp);
      mc_req.header.tag = p_mc_hndl_priv->tag++;

      mc_req.header.command = CMD_MCUNREG;

      rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

      if (rc != 0) {
	  fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
	  errno = EIO; // transport error
	  rc = -1;
      }
      else if (mc_resp.header.tag != mc_req.header.tag) {
	  fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
		  mc_req.header.tag, mc_resp.header.tag);
	  errno = EIO; // this is a response for some other cmd
	  rc = -1;
      }
      else if (mc_resp.header.status != 0) {
	  fprintf(stderr, "%s failed - server errno: %d\n", __func__,
		  mc_resp.header.status);
	  errno = mc_resp.header.status;
	  rc = -1;
      }
      else {
	  rc = 0;
#if 0
	  printf("%s success - unregisterd client handle %p\n", __func__, mc_hndl);
#endif
      }
  }

  close(p_mc_hndl_priv->conn_fd);
  free(p_mc_hndl_priv);

  return rc;
}

int mc_open(mc_hndl_t mc_hndl, __u64 flags, res_hndl_t *p_res_hndl)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCOPEN;
  mc_req.open.flags = flags;

  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
    *p_res_hndl = mc_resp.open.res_hndl;
#if 0
    printf("%s success - opened res_hndl %d\n",  __func__, *p_res_hndl);
#endif
  }

  return rc;
}


int
mc_close(mc_hndl_t mc_hndl, res_hndl_t res_hndl)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCCLOSE;
  mc_req.close.res_hndl = res_hndl;

  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
#if 0
    printf("%s success - closed res_hndl %d\n", __func__, res_hndl);
#endif
  }

  return rc;
}

int mc_size(mc_hndl_t mc_hndl, res_hndl_t res_hndl,
	    __u64 new_size, __u64 *p_actual_new_size) 
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCSIZE;
  mc_req.size.res_hndl = res_hndl;
  mc_req.size.new_size = new_size;
  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
    *p_actual_new_size = mc_resp.size.act_new_size;
#if 0
    printf("%s success - res_hndl %d new size is 0x%lx\n", 
	   __func__,
	   res_hndl, *p_actual_new_size);
#endif
  }

  return rc;
}

int
mc_xlate_lba(mc_hndl_t mc_hndl, res_hndl_t res_hndl,
	     __u64 v_lba, __u64 *p_lba)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCXLATE_LBA;
  mc_req.xlate_lba.res_hndl = res_hndl;
  mc_req.xlate_lba.v_lba = v_lba;
  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
    *p_lba = mc_resp.xlate_lba.p_lba;
#if 0
    printf("%s success - res_hndl %d v_lba 0x%lx, p_lba 0x%lx\n", 
	   __func__,
	   res_hndl, v_lba, *p_lba);
#endif
  }

  return rc;
}


int mc_clone(mc_hndl_t mc_hndl, mc_hndl_t mc_hndl_src, 
	     __u64 flags)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  __u64 challenge;

  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;
  mc_hndl_priv_t *p_mc_hndl_priv_src = (mc_hndl_priv_t *) mc_hndl_src;

  challenge = gen_rand(p_mc_hndl_priv);
  write_64(&p_mc_hndl_priv_src->p_mmio_map->mbox_w, challenge);

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCCLONE;
  mc_req.clone.ctx_hndl_src = p_mc_hndl_priv_src->ctx_hndl;
  mc_req.clone.flags = flags;
  mc_req.clone.challenge = challenge;
  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  // lock mbox
  write_64(&p_mc_hndl_priv_src->p_mmio_map->mbox_w, 0);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
#if 0
    printf("%s success - cloned context %d\n",  __func__, 
	   p_mc_hndl_priv_src->ctx_hndl);
#endif
  }

  return rc;
}

int mc_dup(mc_hndl_t mc_hndl, mc_hndl_t mc_hndl_cand)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  __u64 challenge;

  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;
  mc_hndl_priv_t *p_mc_hndl_priv_cand = (mc_hndl_priv_t *) mc_hndl_cand;

  challenge = gen_rand(p_mc_hndl_priv);
  write_64(&p_mc_hndl_priv_cand->p_mmio_map->mbox_w, challenge);

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCDUP;
  mc_req.dup.ctx_hndl_cand = p_mc_hndl_priv_cand->ctx_hndl;
  mc_req.dup.challenge = challenge;

  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  // lock mbox
  write_64(&p_mc_hndl_priv_cand->p_mmio_map->mbox_w, 0);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
#if 0
    printf("%s success - duped context %d\n",  __func__, 
	   p_mc_hndl_priv_cand->ctx_hndl);
#endif
  }

  return rc;
}

int mc_stat(mc_hndl_t mc_hndl, res_hndl_t res_hndl,
	    mc_stat_t *p_mc_stat)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCSTAT;
  mc_req.stat.res_hndl = res_hndl;
  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
    *p_mc_stat = mc_resp.stat;
#if 0
    printf("%s success - res_hndl %d size is 0x%lx\n", 
	   __func__,
	   res_hndl, *p_size);
#endif
  }

  return rc;
}

int mc_notify(mc_hndl_t mc_hndl, mc_notify_t *p_mc_notify)
{
  mc_req_t mc_req;
  mc_resp_t mc_resp;
  int rc;
  mc_hndl_priv_t *p_mc_hndl_priv = (mc_hndl_priv_t *) mc_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_mc_hndl_priv->tag++;

  mc_req.header.command = CMD_MCNOTIFY;
  mc_req.notify = *p_mc_notify;
  rc = blk_send_command(p_mc_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    fprintf(stderr, "%s failed - transport error rc: %d\n", __func__, rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    fprintf(stderr, "%s failed - tag mismatch: exp(%d), actual(%d)\n", __func__,
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    fprintf(stderr, "%s failed - server errno: %d\n", __func__,
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
#if 0
    printf("%s success - res_hndl %d size is 0x%lx\n", 
	   __func__,
	   res_hndl, *p_size);
#endif
  }

  return rc;
}


