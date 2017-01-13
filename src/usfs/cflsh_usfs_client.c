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


#define CFLSH_USFS_FILENUM 0x0500

#include "cflsh_usfs_internal.h"
#include "cflsh_usfs_protos.h"


/* Two cflsh_usfs_hndl_priv_t instances must not share any data, including
   the fixed caller supplied data that is identical for all duplicated
   handles. Sharing implies locking which the design prefers to avoid.
   Different master handle can be run in parallel in different client threads 
   w/o locks or serialization.

   One cflsh_usfs_hndl_priv_t corresponds to one client connection to the server.
   Client code does not care/enforce how many connections can be made.
   It is for server to enforce.

   A connection is bound to a during registration. Multiple
   connections (master handles) can be bound to the same process.

   This implementation does not prevent a child using a cflsh_usfs_hndl inherited
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
  char master_dev_path[MC_PATHLEN];

} cflsh_usfs_hndl_priv_t;

#ifdef _MASTER_LOCK
/*
 * NAME:cusfs_key_gen()
 *
 * FUNCTION:
 *    cusfs_key_gen() will generate key with help
 *    The key will be unique to each disk, resources.
 * RETURN:
 *
 */

key_t cusfs_key_gen( char * file_name , cflsh_usfs_master_lock_t cufs_lock )
{
    key_t   key_value;

    key_value = ftok(file_name, cufs_lock );

    if ( (key_t)-1 == key_value )
    {
       CUSFS_TRACE_LOG_FILE(1, "%s: ftok() failed, errno %d\n", file_name,errno);
       return (-1);
    }

    return key_value;
}

/*
 * NAME:cusfs_create_sem()
 *
 * FUNCTION:
 *    cusfs_create_sem() will create semphore if it does not exist
 *    Or get the semaphore for existing one.
 *    On successful case, it will return semid to its caller.
 * RETURN:
 *
 */

int cusfs_create_sem( key_t cufs_sem_key )
{
    int rc;

    int  cufs_sem_id;

    cufs_sem_id = semget( cufs_sem_key , 1 , S_IRUSR |
                              S_IWUSR | S_IRGRP | S_IWGRP | IPC_CREAT | IPC_EXCL);
    if ( -1 == cufs_sem_id )
    {
        /* It is already created by other process; 
           So Just need to get semid now */

        cufs_sem_id = semget ( cufs_sem_key , 1 , S_IRUSR |
                                       S_IWUSR | S_IRGRP | S_IWGRP ); 
        /* if failed to get semid ; then inform caller */
        if ( -1 == cufs_sem_id ) 
           return -1;
     }

    else
    {
        /* Initialize the semaphore ; 
           Process created the sem for First time*/

        rc = semctl(cufs_sem_id,0,SETVAL,1);
        if(rc == -1)
        {
           return -1 ;
        }
    }

    return cufs_sem_id;
}

/*
 * NAME:cusfs_rm_sem()
 *
 * FUNCTION:
 *    cusfs_rm_sem() will delete semphore.
 *    On successful case, it will return semid to its caller.
 * RETURN:
 *
 */

int cusfs_rm_sem( int  cufs_sem_id  )
{
    int rc = 0;

    rc = semctl( cufs_sem_id, 1, IPC_RMID );
    if(rc == -1)
    {
       //TRACE_1
       return -1 ;
    }

    return rc;
}

/*
 * NAME:cusfs_cleanup_sem()
 *
 * FUNCTION:
 *    cusfs_cleanup_sem() will cleanup all the  semphore.
 *    On successful case, it will return 0 else -1.
 * RETURN:
 *
 */

int  cusfs_cleanup_sem ( cflsh_usfs_master_hndl_t cflsh_usfs_hndl )
{
   int rc = 0;
   cusfs_serv_sem_t *p_disk = ( cusfs_serv_sem_t *) cflsh_usfs_hndl;

   rc = cusfs_rm_sem( p_disk->fs_sem.cusfs_fs_semid );
   if ( rc == -1 )
   {
      goto x_err;
   }
   rc = cusfs_rm_sem( p_disk->frtb_sem.cusfs_frtb_semid );
   if ( rc == -1 )
   {
      goto x_err;
   }
   rc = cusfs_rm_sem( p_disk->intb_sem.cusfs_intb_semid );
   if ( rc == -1 )
   {
      goto x_err;
   }
   rc = cusfs_rm_sem( p_disk->jrn_sem.cusfs_jrn_semid );
   if ( rc == -1 )
   {
      goto x_err;
   }

x_err:
  return rc; /* Need to improve the return code later */
}

#endif

#ifdef _MASTER_LOCK_CLIENT
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
static uint64_t gen_rand(cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv) 
{
    uint64_t rand;
    uint64_t p_low;

    asm volatile ( "mfspr %0, 268" : "=r"(rand) : ); // time base 

    // add in low 32 bits of malloc'ed address to top 32 bits of TB
    p_low = (((uint64_t)p_cflsh_usfs_hndl_priv) & 0xFFFFFFFF);
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
  char *env_master_socket = getenv("CFLSH_USFS_MASTER_SOCKET");


  // Create a socket file descriptor
  conn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (conn_fd < 0)
  {
      CUSFS_TRACE_LOG_FILE(1,"blk_connect: socket failed: %d (%d)\n",
			   conn_fd, errno);
  }
  else
  {
    bzero(&svr_addr, sizeof(struct sockaddr_un));
    svr_addr.sun_family      = AF_UNIX;

    if (env_master_socket) {
	strcpy(svr_addr.sun_path, env_master_socket);
    } else {
	strcpy(svr_addr.sun_path, CFLSH_USFS_MASTER_SOCKET_DIR);
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
      CUSFS_TRACE_LOG_FILE(1,"block_connect: Connect failed: %d (%d)\n",
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
blk_send_command(int conn_fd, cflsh_usfs_master_req_t *p_mc_req, cflsh_usfs_master_resp_t *p_mc_resp)
{
  int rc = 0;

  // Send the command block
  rc = xfer_data(conn_fd, XFER_OP_WRITE, (void *)p_mc_req, sizeof(*p_mc_req));
  if (rc)
  {
    CUSFS_TRACE_LOG_FILE(1,"blk_send_command: PID %d failed send: %d\n",
            getpid(), rc);
  }
  else
  {
    // Wait for the response
    rc = xfer_data(conn_fd, XFER_OP_READ, (void *)p_mc_resp, sizeof(*p_mc_resp));
    if (rc)
    {
      CUSFS_TRACE_LOG_FILE(1, "blk_send_command: PID %d failed read: %d\n",
              getpid(), rc);
    }
  }

  return rc;
}


// backend function used by cflsh_usfs_master_register after making a connection
// to server.
//
static int 
cflsh_usfs_master_register_back(cflsh_usfs_master_hndl_t cflsh_usfs_hndl, 
				int mode)

{
  cflsh_usfs_master_req_t mc_req;
  cflsh_usfs_master_resp_t mc_resp;
  uint64_t challenge;
  int rc;

  cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv = (cflsh_usfs_hndl_priv_t *) cflsh_usfs_hndl;

  challenge = gen_rand(p_cflsh_usfs_hndl_priv);


  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_usfs_hndl_priv->tag++;

  mc_req.header.command = CFLSH_USFS_CMD_MASTER_REG;
  mc_req.reg.client_pid = getpid();
  mc_req.reg.client_fd = p_cflsh_usfs_hndl_priv->conn_fd;
  mc_req.reg.mode = mode;
  mc_req.reg.challenge = challenge;

  rc = blk_send_command(p_cflsh_usfs_hndl_priv->conn_fd, &mc_req, &mc_resp);


  if (rc != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - transport error rc: %d, errno = %d\n", rc, errno);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CUSFS_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n", mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - server errno: %d\n", mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;
  }

  return rc;
}

#endif /* _MASTER_LOCK_CLIENT */
/* 
 * Procedure: cflsh_usfs_master_init
 *
 * Description: Init interfaces with master.

 *
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int
cflsh_usfs_master_init()
{
  return 0;
}

/* 
 * Procedure: cflsh_usfs_master_term
 *
 * Description: Terminate interfaces with master
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int
cflsh_usfs_master_term()
{
  return 0;
}

/* 
 * Procedure: cflsh_usfs_master_register
 *
 * Description: Register a client with the master
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int 
cflsh_usfs_master_register(char *master_dev_path, cflsh_usfs_master_hndl_t *p_cflsh_usfs_hndl)
{
#ifdef _MASTER_LOCK
#ifdef _MASTER_LOCK_CLIENT
  int conn_fd;
  int rc;
  cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv;

  conn_fd = blk_connect(master_dev_path);
  if (conn_fd < 0) {
    CUSFS_TRACE_LOG_FILE(1,"socket failed: %d (%d)\n", conn_fd, errno);
    return -1; // errno set in  blk_connect()
  }

  p_cflsh_usfs_hndl_priv = (cflsh_usfs_hndl_priv_t *) malloc(sizeof(cflsh_usfs_hndl_priv_t));
  if (p_cflsh_usfs_hndl_priv == NULL) {
    close(conn_fd);
    CUSFS_TRACE_LOG_FILE(1,"cannot allocate client handle\n");
    errno = ENOMEM;
    return -1;
  }

  // init cflsh_usfs_hndl
  memset(p_cflsh_usfs_hndl_priv, 0, sizeof(*p_cflsh_usfs_hndl_priv));
  p_cflsh_usfs_hndl_priv->conn_fd = conn_fd;
  p_cflsh_usfs_hndl_priv->tag = 0;
  p_cflsh_usfs_hndl_priv->pid = getpid();
  strncpy(p_cflsh_usfs_hndl_priv->master_dev_path, master_dev_path, 
	  MC_PATHLEN - 1);


  // initial MCREG of ctx_hndl (not a dup)
  rc = cflsh_usfs_master_register_back(p_cflsh_usfs_hndl_priv, MCREG_INITIAL_REG);

  if (rc != 0) {
    free(p_cflsh_usfs_hndl_priv);
    close(conn_fd);
    return rc; // errno is already set
  }

  *p_cflsh_usfs_hndl = p_cflsh_usfs_hndl_priv;

  CUSFS_TRACE_LOG_FILE(9,"success - on client handle %p\n", 
		       *p_cflsh_usfs_hndl);
  return rc;

#else /* _MASTER_LOCK_CLIENT */

  int rc = 0;

  cusfs_serv_sem_t *p_disk =NULL ;

  if ( posix_memalign((void *)&(p_disk),4096,
                               sizeof( cusfs_serv_sem_t)))
  {
    CUSFS_TRACE_LOG_FILE(1,"cannot allocate p_disk handle \n");
    errno = ENOMEM;
    return -1;
  }

  strncpy(p_disk->master_dev_path, master_dev_path, MC_PATHLEN - 1);

  p_disk->fs_sem.cusfs_fs_key=cusfs_key_gen( p_disk->master_dev_path, \
                                      CFLSH_USFS_MASTER_FS_LOCK);
  if( p_disk->fs_sem.cusfs_fs_key == -1 )
  {
     CUSFS_TRACE_LOG_FILE(1,"cannot get fs key\n");
     rc =-1;
     goto init_err;
  }

  p_disk->frtb_sem.cusfs_frtb_key = cusfs_key_gen( p_disk->master_dev_path, \
                                                   CFLSH_USFS_MASTER_FREE_TABLE_LOCK);
  if ( p_disk->frtb_sem.cusfs_frtb_key == -1 )
  {
     rc = -1;
     CUSFS_TRACE_LOG_FILE(1,"cannot get free table key\n");
     goto init_err;
  }

  p_disk->intb_sem.cusfs_intb_key =  cusfs_key_gen( p_disk->master_dev_path, \
                                                      CFLSH_USFS_MASTER_INODE_TABLE_LOCK );
  if ( p_disk->intb_sem.cusfs_intb_key == -1 )
  {
       rc = -1;
       CUSFS_TRACE_LOG_FILE(1,"cannot get intb  key\n");
       goto init_err;
   }

   p_disk->jrn_sem.cusfs_jrn_key =  cusfs_key_gen( p_disk->master_dev_path, \
                                                   CFLSH_USFS_MASTER_JOURNAL_LOCK );
   if ( p_disk->jrn_sem.cusfs_jrn_key == -1 )
   {
      rc = -1;
      CUSFS_TRACE_LOG_FILE(1,"cannot get jrn key\n");
      goto init_err;
   }

   p_disk->fs_sem.cusfs_fs_semid = cusfs_create_sem( p_disk->fs_sem.cusfs_fs_key ) ;
   if ( p_disk->fs_sem.cusfs_fs_semid  == -1 )
   {
      rc = -1;
      goto init_err;
   }

   p_disk->frtb_sem.cusfs_frtb_semid = cusfs_create_sem( p_disk->frtb_sem.cusfs_frtb_key ) ;
   if ( p_disk->frtb_sem.cusfs_frtb_semid  == -1 )
   {
      rc = -1;
      goto init_err;
   }

   p_disk->intb_sem.cusfs_intb_semid = cusfs_create_sem( p_disk->intb_sem.cusfs_intb_key ) ;
   if ( p_disk->intb_sem.cusfs_intb_semid  == -1 )
   {
        rc =-1;
        goto init_err;
   }

   p_disk->jrn_sem.cusfs_jrn_semid = cusfs_create_sem( p_disk->jrn_sem.cusfs_jrn_key ) ;
   if( p_disk->jrn_sem.cusfs_jrn_semid  == -1 )
   {
       rc =-1;
       goto init_err;
   }
init_err:
   *p_cflsh_usfs_hndl = p_disk;
   return rc;

#endif      // END of MASTER_LOCK_CLIENT
#else 
   return 0;
#endif     // END of MASTER_LOCK
}



/* 
 * Procedure: cflsh_usfs_master_unregister
 *
 * Description: Unregister a client with the master
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cflsh_usfs_master_unregister(cflsh_usfs_master_hndl_t cflsh_usfs_hndl)
{
#ifdef _MASTER_LOCK_CLIENT
  cflsh_usfs_master_req_t mc_req;
  cflsh_usfs_master_resp_t mc_resp;
  int rc = 0;
  cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv = (cflsh_usfs_hndl_priv_t *) cflsh_usfs_hndl;

  /* Unregister with server if this is called by parent i.e. the 
     original registrant */
  if (p_cflsh_usfs_hndl_priv-> pid == getpid()) { 
      memset(&mc_req, '\0', sizeof(mc_req));
      memset(&mc_resp, '\0', sizeof(mc_resp));
      mc_req.header.size = sizeof(mc_req);
      mc_resp.header.size = sizeof(mc_resp);
      mc_req.header.tag = p_cflsh_usfs_hndl_priv->tag++;

      mc_req.header.command = CFLSH_USFS_CMD_MASTER_UNREG;

      rc = blk_send_command(p_cflsh_usfs_hndl_priv->conn_fd, &mc_req, &mc_resp);

      if (rc != 0) {
	  CUSFS_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n", rc);
	  errno = EIO; // transport error
	  rc = -1;
      }
      else if (mc_resp.header.tag != mc_req.header.tag) {
	  CUSFS_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n", 
			       mc_req.header.tag, mc_resp.header.tag);
	  errno = EIO; // this is a response for some other cmd
	  rc = -1;
      }
      else if (mc_resp.header.status != 0) {
	  CUSFS_TRACE_LOG_FILE(1,"failed - server errno: %d\n", 
			       mc_resp.header.status);
	  errno = mc_resp.header.status;
	  rc = -1;
      }
      else {
	  rc = 0;

	  CUSFS_TRACE_LOG_FILE(9,"success - unregisterd client handle %p\n", cflsh_usfs_hndl);

      }
  }

  close(p_cflsh_usfs_hndl_priv->conn_fd);
  free(p_cflsh_usfs_hndl_priv);
  return rc;
#else 
  return 0;
#endif /* !_MASTER_LOCK_CLIENT */

}

/* 
 * Procedure: cflsh_usfs_master_lock
 *
 * Description: Request the specified shared lock.
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cflsh_usfs_master_lock(cflsh_usfs_master_hndl_t cflsh_usfs_hndl,
			   cflsh_usfs_master_lock_t lock,
			   uint64_t flags)
{
#ifdef _MASTER_LOCK
#ifdef _MASTER_LOCK_CLIENT
  cflsh_usfs_master_req_t mc_req;
  cflsh_usfs_master_resp_t mc_resp;
  int rc;
  cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv = (cflsh_usfs_hndl_priv_t *) cflsh_usfs_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_usfs_hndl_priv->tag++;

  mc_req.header.command = CFLSH_USFS_CMD_MASTER_LOCK;
  mc_req.lock.lock = lock;
  mc_req.lock.flags = flags;

  rc = blk_send_command(p_cflsh_usfs_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n",rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CUSFS_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n",
	   mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - server errno: %d\n",
	   mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;


    CUSFS_TRACE_LOG_FILE(9,"success - locked %d\n",  lock);

  }

  return rc;
#else

  int rc = 0;
  int lock_semid = 0;
  struct sembuf cflash_sembuf;

  cusfs_serv_sem_t *p_disk = ( cusfs_serv_sem_t *) cflsh_usfs_hndl;

  switch ( lock )
  {
   case CFLSH_USFS_MASTER_FS_LOCK:
       lock_semid = p_disk->fs_sem.cusfs_fs_semid;
       break;

   case CFLSH_USFS_MASTER_FREE_TABLE_LOCK:
       lock_semid = p_disk->frtb_sem.cusfs_frtb_semid;
       break;

   case CFLSH_USFS_MASTER_INODE_TABLE_LOCK:
       lock_semid = p_disk->intb_sem.cusfs_intb_semid;
       break;

   case CFLSH_USFS_MASTER_JOURNAL_LOCK:
       lock_semid = p_disk->jrn_sem.cusfs_jrn_semid;
       break;

   default:
       /* No lock sem registered */
       break;
  }

  cflash_sembuf.sem_num = 0;
  cflash_sembuf.sem_op = -1;
  cflash_sembuf.sem_flg = IPC_NOWAIT;

  rc = semop(lock_semid, &cflash_sembuf, 1);

  if ( ( rc != 0) && ( errno == EINTR || errno == EAGAIN ))
  {
      while(1) 
      {
          rc = semop(lock_semid, &cflash_sembuf, 1);
          if(rc ==0)
          {
             break;
          }
       }
  }

  if(rc ==0)
    CUSFS_TRACE_LOG_FILE(9,"success - locked %d\n",  lock);


  return rc;
#endif /* !_MASTER_LOCK_CLIENT */
#else
  return 0; 
#endif /* !_MASTER_LOCK */
}


/* 
 * Procedure: cflsh_usfs_master_unlock
 *
 * Description: Request to release a shared lock
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int
cflsh_usfs_master_unlock(cflsh_usfs_master_hndl_t cflsh_usfs_hndl,
			   cflsh_usfs_master_lock_t lock,
			   uint64_t flags)
{
#ifdef _MASTER_LOCK
#ifdef _MASTER_LOCK_CLIENT
  cflsh_usfs_master_req_t mc_req;
  cflsh_usfs_master_resp_t mc_resp;
  int rc;
  cflsh_usfs_hndl_priv_t *p_cflsh_usfs_hndl_priv = (cflsh_usfs_hndl_priv_t *) cflsh_usfs_hndl;

  memset(&mc_req, '\0', sizeof(mc_req));
  memset(&mc_resp, '\0', sizeof(mc_resp));
  mc_req.header.size = sizeof(mc_req);
  mc_resp.header.size = sizeof(mc_resp);
  mc_req.header.tag = p_cflsh_usfs_hndl_priv->tag++;

  mc_req.header.command = CFLSH_USFS_CMD_MASTER_UNLOCK;
  mc_req.lock.lock = lock;
  mc_req.lock.flags = flags;

  rc = blk_send_command(p_cflsh_usfs_hndl_priv->conn_fd, &mc_req, &mc_resp);

  if (rc != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - transport error rc: %d\n", rc);
    errno = EIO; // transport error
    rc = -1;
  }
  else if (mc_resp.header.tag != mc_req.header.tag) {
    CUSFS_TRACE_LOG_FILE(1,"failed - tag mismatch: exp(%d), actual(%d)\n",
			 mc_req.header.tag, mc_resp.header.tag);
    errno = EIO; // this is a response for some other cmd
    rc = -1;
  }
  else if (mc_resp.header.status != 0) {
    CUSFS_TRACE_LOG_FILE(1,"failed - server errno: %d\n",
			 mc_resp.header.status);
    errno = mc_resp.header.status;
    rc = -1;
  }
  else {
    rc = 0;

    CUSFS_TRACE_LOG_FILE(9,"success - unlocked %d\n", lock);

  }

  return rc;
#else
  int rc = 0;
  int lock_semid = 0;
  struct sembuf cflash_sembuf;

  cusfs_serv_sem_t *p_disk = ( cusfs_serv_sem_t *) cflsh_usfs_hndl;

  switch ( lock )
  {
  case CFLSH_USFS_MASTER_FS_LOCK:
       lock_semid = p_disk->fs_sem.cusfs_fs_semid;
       break;

  case CFLSH_USFS_MASTER_FREE_TABLE_LOCK:
       lock_semid = p_disk->frtb_sem.cusfs_frtb_semid;
       break;

  case CFLSH_USFS_MASTER_INODE_TABLE_LOCK:
       lock_semid = p_disk->intb_sem.cusfs_intb_semid;
       break;

  case CFLSH_USFS_MASTER_JOURNAL_LOCK:
       lock_semid = p_disk->jrn_sem.cusfs_jrn_semid;
       break;

  default:
       /* No lock sem registered */
       break;

  }
  cflash_sembuf.sem_num = 0;
  cflash_sembuf.sem_op = 1;
  cflash_sembuf.sem_flg = IPC_NOWAIT;

  rc = semop(lock_semid, &cflash_sembuf, 1);

  if ( ( rc != 0) && ( errno == EINTR || errno == EAGAIN ))
  {
     while(1) 
     {
          rc = semop(lock_semid, &cflash_sembuf, 1);
          if(rc ==0)
          {  
             break;
          }
     }
  }

  if(rc ==0)
    CUSFS_TRACE_LOG_FILE(9,"success - unlocked %d\n", lock);
   
  return rc;
 
#endif /* !_MASTER_LOCK_CLIENT */
#else
  return 0; /* !_MASTER_LOCK */
#endif
}
