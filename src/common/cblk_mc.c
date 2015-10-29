/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/cblk_mc.c $                                        */
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
#include "cblk.h"

/***************************************************************************** 
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
 *****************************************************************************/
int
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
        bytes_xfer = sendmsg(fd, &msg, 0);
      }

      if ( -1 == bytes_xfer )
      {
        if ( EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
        {
          fprintf(stderr, "xfer_data: Operation(%d) fail errno %d\n",
                  op, errno);

          // just retry the whole request
          continue;
        }
        else
        {
          fprintf(stderr, "xfer_data: Operation(%d) Connection closed %d(rc %d)\n",
                  op, errno, rc);
          // connection closed by the other end
          rc = 1;
          break;
        }
      }
      else if ( 0 == bytes_xfer )
      {
        fprintf(stderr, "xfer_data: Operation(%d) 0 Bytes Transfered %d(rc %d)\n",
                op, errno, rc);
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
        fprintf(stderr, "xfer_data: Operation(%d) Partial Transfered %d(Total %d)\n",
                op, bytes_xfer, target_size);
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
int
blk_connect()
{
  struct sockaddr_un svr_addr;
  int conn_fd = -1;
  int rc = 0;

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

    // This can be changed to any file
    strcpy(svr_addr.sun_path, "/tmp/surelock");

    // Connect to the server entity.
    rc = connect(conn_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
    if ( rc )
    {
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
int
blk_send_command(int conn_fd, void *cmdblock, void *cmdresp)
{
  CReqBlkHeader_p  cmd_hdr;
  CRespBlkHeader_p cmdresp_hdr;
  int rc = 0;

  cmd_hdr = (CReqBlkHeader_t *)cmdblock;
  cmd_hdr->creqh_pid = getpid();
  cmd_hdr->creqh_pthid = pthread_self();

  cmdresp_hdr = (CRespBlkHeader_p)cmdresp;

  // Send the command block
  rc = xfer_data(conn_fd, 0, (void *)cmdblock, cmd_hdr->creqh_cmdsize);
  if (rc)
  {
    fprintf(stderr, "blk_send_command: Child %d failed send: %d\n",
            getpid(), rc);
  }
  else
  {
    // Wait for the response
    rc = xfer_data(conn_fd, 1, (void *)cmdresp, cmdresp_hdr->cresh_respsize);
    if (rc)
    {
      fprintf(stderr, "blk_send_command: Child %d failed read: %d\n",
              getpid(), rc);
    }
  }

  return rc;
}



/************MC Block Allocator Command Functions*****************************/

/*************************************************************************
 * NAME: blk_sendmcregister
 *
 * FUNCTION:
 *    This will send a MCREG cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcregister(dev_t adap_devno, char *adap_name,
                    ctx_handle_t *ctx_handle)
{
  CReqBlk_MCRegister_t cmd_mcreg;
  CRespBlk_MCRegister_t cmdresp_mcreg;
  int rc;

  memset(&cmd_mcreg, '\0', sizeof(CReqBlk_MCRegister_t));
  memset(&cmdresp_mcreg, '\0', sizeof(CRespBlk_MCRegister_t));

  cmd_mcreg.header.creqh_command = CMDBLK_CMD_MCREG;
  cmd_mcreg.header.creqh_cmdsize = sizeof(CReqBlk_MCRegister_t);

  cmdresp_mcreg.header.cresh_respsize = sizeof(CRespBlk_MCRegister_t);

  cmd_mcreg.adap_devno = adap_devno;
  strcpy(cmd_mcreg.adap_name, adap_name);

  rc = blk_send_command(conn_fd, &cmd_mcreg, &cmdresp_mcreg);

  if (cmdresp_mcreg.header.cresh_status != 0 ||
      cmdresp_mcreg.ctx_handle < 0) {
    printf("blk_sendmcreg: MCREG failed - status: %x \n",
           cmdresp_mcreg.header.cresh_status);
    return(cmdresp_mcreg.header.cresh_status);
  }

  printf("blk_sendmcreg: MCREG Success - ctx_id %x\n",
         CTX_ID(cmdresp_mcreg.ctx_handle));

  *ctx_handle = cmdresp_mcreg.ctx_handle;

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcbainit
 *
 * FUNCTION:
 *    This will send a MCBAINIT cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcbainit(size_t lunsize)
{
  CReqBlk_MCBaInit_t cmd_mcbainit;
  CRespBlk_MCBaInit_t cmdresp_mcbainit;
  lungrp_info_t lungrp;
  lungrp_vec_p lunptr;
  int rc;

  memset(&cmd_mcbainit, '\0', sizeof(CReqBlk_MCBaInit_t));
  memset(&cmdresp_mcbainit, '\0', sizeof(CRespBlk_MCBaInit_t));

  cmd_mcbainit.header.creqh_command = CMDBLK_CMD_MCBAINIT;
  cmd_mcbainit.header.creqh_cmdsize = sizeof(CReqBlk_MCBaInit_t);

  cmdresp_mcbainit.header.cresh_respsize = sizeof(CRespBlk_MCBaInit_t);

  lungrp.total_luns = 1;
  
  lunptr = (lungrp_vec_p) malloc(sizeof(lungrp_vec_t));

  if (lunptr == NULL) {
      printf("blk_sendmcbainit: malloc lunptr failed ENOMEM\n");
      return(ENOMEM);
  }

  lunptr->lun_id   = 0x10000;
  lunptr->wwpn     = 0x20000;
  lunptr->lun_size = lunsize;
  
  lungrp.lunptr    = lunptr;

  cmd_mcbainit.lungrp = &lungrp;

  rc = blk_send_command(conn_fd, &cmd_mcbainit, &cmdresp_mcbainit);

  if (cmdresp_mcbainit.header.cresh_status != 0 ||
      cmdresp_mcbainit.ba_handle == NULL) {
    printf("blk_sendmcbainit: MCBAINIT failed - status: %x ba_handle %llx\n",
           cmdresp_mcbainit.header.cresh_status, cmdresp_mcbainit.ba_handle);
  } else {
    printf("blk_sendmcbainit: MCBAINIT Success - ba_handle %llx\n",
           cmdresp_mcbainit.ba_handle);
  }

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcbaclose
 *
 * FUNCTION:
 *    This will send a MCBACLOSE cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcbaclose(ba_handle_t ba_handle)
{
  CReqBlk_MCBaClose_t cmd_mcbaclose;
  CRespBlk_MCBaClose_t cmdresp_mcbaclose;
  int rc;

  memset(&cmd_mcbaclose, '\0', sizeof(CReqBlk_MCBaClose_t));
  memset(&cmdresp_mcbaclose, '\0', sizeof(CRespBlk_MCBaClose_t));

  cmd_mcbaclose.header.creqh_command = CMDBLK_CMD_MCBACLOSE;
  cmd_mcbaclose.header.creqh_cmdsize = sizeof(CReqBlk_MCBaClose_t);

  cmdresp_mcbaclose.header.cresh_respsize = sizeof(CRespBlk_MCBaClose_t);

  cmd_mcbaclose.ba_handle = ba_handle;

  rc = blk_send_command(conn_fd, &cmd_mcbaclose, &cmdresp_mcbaclose);

  if (cmdresp_mcbaclose.header.cresh_status != 0) {
    printf("blk_sendmcbaclose: MCBACLOSE failed - status: %x ba_handle %llx\n",
           cmdresp_mcbaclose.header.cresh_status, cmd_mcbaclose.ba_handle);
  } else {
    printf("blk_sendmcbaclose: MCBACLOSE Success - ba_handle %llx\n",
           cmd_mcbaclose.ba_handle);
  }
}

/*************************************************************************
 * NAME: blk_sendmcopen
 *
 * FUNCTION:
 *    This will send a MCOPEN cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcopen(dev_t adap_devno, char *adap_name,
               ctx_handle_t ctx_handle)
{
  CReqBlk_MCOpen_t cmd_mcopen;
  CRespBlk_MCOpen_t cmdresp_mcopen;
  ctx_info_p ctx_ptr;
  res_handle_info_p res_info_ptr, tmp_res_info_ptr;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  if (ctx_id < 0 || ctx_id > CTX_LIST_SIZE) {
    printf("blk_sendmcopen: ctx_id Invalid ctx_id %x- EINVAL \n",
           ctx_id);
    return(EINVAL);
  }

  memset(&cmd_mcopen, '\0', sizeof(CReqBlk_MCOpen_t));
  memset(&cmdresp_mcopen, '\0', sizeof(CRespBlk_MCOpen_t));

  cmd_mcopen.header.creqh_command = CMDBLK_CMD_MCOPEN;
  cmd_mcopen.header.creqh_cmdsize = sizeof(CReqBlk_MCOpen_t);

  cmdresp_mcopen.header.cresh_respsize = sizeof(CRespBlk_MCOpen_t);

  cmd_mcopen.adap_devno = adap_devno;
  cmd_mcopen.ctx_handle = ctx_handle;

  strcpy(cmd_mcopen.adap_name, adap_name);

  rc = blk_send_command(conn_fd, &cmd_mcopen, &cmdresp_mcopen);

  if (cmdresp_mcopen.header.cresh_status != 0 ||
      cmdresp_mcopen.r_handle < 0) {
    printf("blk_sendmcopen: MCOPEN failed - status: %x \n",
           cmdresp_mcopen.header.cresh_status);
    return(cmdresp_mcopen.header.cresh_status);
  }

  printf("blk_sendmcopen: MCOPEN Success - r_handle %x\n",
         cmdresp_mcopen.r_handle);

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcclose
 *
 * FUNCTION:
 *    This will send a MCCLOSE cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcclose(dev_t adap_devno, ctx_handle_t ctx_handle,
                res_handle_t r_handle)
{
  CReqBlk_MCClose_t cmd_mcclose;
  CRespBlk_MCClose_t cmdresp_mcclose;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  memset(&cmd_mcclose, '\0', sizeof(CReqBlk_MCClose_t));
  memset(&cmdresp_mcclose, '\0', sizeof(CRespBlk_MCClose_t));

  cmd_mcclose.header.creqh_command = CMDBLK_CMD_MCCLOSE;
  cmd_mcclose.header.creqh_cmdsize = sizeof(CReqBlk_MCClose_t);

  cmdresp_mcclose.header.cresh_respsize = sizeof(CRespBlk_MCClose_t);

  cmd_mcclose.adap_devno = adap_devno;
  cmd_mcclose.r_handle   = r_handle;

  rc = blk_send_command(conn_fd, &cmd_mcclose, &cmdresp_mcclose);

  if (cmdresp_mcclose.header.cresh_status != 0) {
    printf("blk_sendmcclose: MCCLOSE failed - status: %x \n",
           cmdresp_mcclose.header.cresh_status);
    return(cmdresp_mcclose.header.cresh_status);
  }

  printf("blk_sendmcclose: MCCLOSE Success ctx_id %x r_handle %x\n",
         ctx_id, r_handle);

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcresize
 *
 * FUNCTION:
 *    This will send a MCRESIZE cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcresize(dev_t adap_devno, ctx_handle_t ctx_handle,
                 res_handle_t r_handle, size_t r_size)
{
  CReqBlk_MCResize_t cmd_mcresize;
  CRespBlk_MCResize_t cmdresp_mcresize;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  memset(&cmd_mcresize, '\0', sizeof(CReqBlk_MCResize_t));
  memset(&cmdresp_mcresize, '\0', sizeof(CRespBlk_MCResize_t));

  cmd_mcresize.header.creqh_command = CMDBLK_CMD_MCRESIZE;
  cmd_mcresize.header.creqh_cmdsize = sizeof(CReqBlk_MCResize_t);

  cmdresp_mcresize.header.cresh_respsize = sizeof(CRespBlk_MCResize_t);

  cmd_mcresize.ctx_handle = ctx_handle;
  cmd_mcresize.adap_devno = adap_devno;
  cmd_mcresize.r_handle   = r_handle;
  cmd_mcresize.r_size     = r_size;

  rc = blk_send_command(conn_fd, &cmd_mcresize, &cmdresp_mcresize);

  if (cmdresp_mcresize.header.cresh_status != 0) {
    printf("blk_sendmcresize: MCRESIZE failed - status: %x \n",
           cmdresp_mcresize.header.cresh_status);
    return(cmdresp_mcresize.header.cresh_status);
  }

  printf("blk_sendmcresize: MCRESIZE Success status: %x\n",
         cmdresp_mcresize.header.cresh_status);

  if (cmdresp_mcresize.new_r_size != r_size) {
    printf("blk_sendmcresize: Partial Size Alloc Size: %x Req Size: %x\n",
           cmdresp_mcresize.new_r_size, r_size);
  }

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcgetsize
 *
 * FUNCTION:
 *    This will send a MCRESIZE cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcgetsize(dev_t adap_devno, ctx_handle_t ctx_handle,
                  res_handle_t r_handle, size_t *size)
{
  CReqBlk_MCGetsize_t cmd_mcgetsize;
  CRespBlk_MCGetsize_t cmdresp_mcgetsize;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  memset(&cmd_mcgetsize, '\0', sizeof(CReqBlk_MCGetsize_t));
  memset(&cmdresp_mcgetsize, '\0', sizeof(CRespBlk_MCGetsize_t));

  cmd_mcgetsize.header.creqh_command = CMDBLK_CMD_MCGETSIZE;
  cmd_mcgetsize.header.creqh_cmdsize = sizeof(CReqBlk_MCGetsize_t);

  cmdresp_mcgetsize.header.cresh_respsize = sizeof(CRespBlk_MCGetsize_t);

  cmd_mcgetsize.ctx_handle = ctx_handle;
  cmd_mcgetsize.adap_devno = adap_devno;
  cmd_mcgetsize.r_handle   = r_handle;

  rc = blk_send_command(conn_fd, &cmd_mcgetsize, &cmdresp_mcgetsize);

  if (cmdresp_mcgetsize.header.cresh_status != 0) {
    printf("blk_sendmcgetsize: MCGETSIZE failed - status: %x \n",
           cmdresp_mcgetsize.header.cresh_status);
    return(cmdresp_mcgetsize.header.cresh_status);
  }

  printf("blk_sendmcgetsize: MCGETSIZE Success status: %x\n",
         cmdresp_mcgetsize.header.cresh_status);

  *size = cmdresp_mcgetsize.r_size;

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmcclone
 *
 * FUNCTION:
 *    This will send a MCCLONE cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmcclone(dev_t adap_devno, ctx_handle_t ctx_handle,
                res_handle_t r_handle, res_handle_t *new_r_handle)
{
  CReqBlk_MCClone_t cmd_mcclone;
  CRespBlk_MCClone_t cmdresp_mcclone;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  memset(&cmd_mcclone, '\0', sizeof(CReqBlk_MCClone_t));
  memset(&cmdresp_mcclone, '\0', sizeof(CRespBlk_MCClone_t));

  cmd_mcclone.header.creqh_command = CMDBLK_CMD_MCCLONE;
  cmd_mcclone.header.creqh_cmdsize = sizeof(CReqBlk_MCClone_t);

  cmdresp_mcclone.header.cresh_respsize = sizeof(CRespBlk_MCClone_t);

  rc = blk_send_command(conn_fd, &cmd_mcclone, &cmdresp_mcclone);

  if (cmdresp_mcclone.header.cresh_status != 0) {
    printf("blk_sendmcclone: MCCLONE failed - status: %x \n",
           cmdresp_mcclone.header.cresh_status);
    return(cmdresp_mcclone.header.cresh_status);
  }

  printf("blk_sendmcclone: MCCLONE Success status: %x\n",
         cmdresp_mcclone.header.cresh_status);

  return(0);
}

/*************************************************************************
 * NAME: blk_sendmctxlba
 *
 * FUNCTION:
 *    This will send a MCTXLBA cmd to the MC process
 *
 * PARAMETERS: None
 *
 * RETURNS: None
 ************************************************************************/
int
blk_sendmctxlba(dev_t adap_devno, ctx_handle_t ctx_handle,
                res_handle_t r_handle,
                int64_t v_lba, int64_t *p_lba)
{
  CReqBlk_MCTxLBA_t cmd_mctxlba;
  CRespBlk_MCTxLBA_t cmdresp_mctxlba;
  ctx_id_t ctx_id = CTX_ID(ctx_handle);
  int rc;

  memset(&cmd_mctxlba, '\0', sizeof(CReqBlk_MCTxLBA_t));
  memset(&cmdresp_mctxlba, '\0', sizeof(CRespBlk_MCTxLBA_t));

  cmd_mctxlba.header.creqh_command = CMDBLK_CMD_MCTXLBA;
  cmd_mctxlba.header.creqh_cmdsize = sizeof(CReqBlk_MCTxLBA_t);

  cmdresp_mctxlba.header.cresh_respsize = sizeof(CRespBlk_MCTxLBA_t);

  cmd_mctxlba.ctx_handle = ctx_handle;
  cmd_mctxlba.adap_devno = adap_devno;
  cmd_mctxlba.r_handle   = r_handle;
  cmd_mctxlba.v_lba      = v_lba;

  rc = blk_send_command(conn_fd, &cmd_mctxlba, &cmdresp_mctxlba);

  if (cmdresp_mctxlba.header.cresh_status != 0) {
    printf("blk_sendmctxlba: MCTXLBA failed - status: %x \n",
           cmdresp_mctxlba.header.cresh_status);
    return(cmdresp_mctxlba.header.cresh_status);
  }

  printf("blk_sendmcgetsize: MCTXLBA Success status: %x\n",
         cmdresp_mctxlba.header.cresh_status);

  *p_lba = cmdresp_mctxlba.p_lba;

  return(0);
}

/************End of MC Block Allocator Command Functions**********************/