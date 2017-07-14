//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/block/cflsh_block_eras_serv.c $
//
//  IBM CONFIDENTIAL
//
//  COPYRIGHT International Business Machines Corp. 2014
//
//  p1
//
//  Object Code Only (OCO) source materials
//  Licensed Internal Code Source Materials
//  IBM Surelock Licensed Internal Code
//
//  The source code for this program is not published or other-
//  wise divested of its trade secrets, irrespective of what has
//  been deposited with the U.S. Copyright Office.
//
//  Origin: 30
//
//  IBM_PROLOG_END
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/poll.h>
#ifndef _AIX
#include <sys/epoll.h>
#else
#include <poll.h>
#endif /* !_AIX */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define CFLSH_BLK_FILENUM 0x0900

#include "cflash_block_eras_serv.h"
#include "cflash_block_eras_client.h"
#include "cflash_block_inline.h"



/***************************************************************************
 *                        Block Library ERAS Server
 *
 * The functions of the block library ERAS server are:
 *
 * 1. 
 * 2. 
 *
 *
 * A single instance of the ERAS server serves a process. 
 *
 *
 ***************************************************************************/

static cblk_serv_disk_t cblk_serv_disk;  


conn_info_t*
cblk_serv_alloc_connection(cblk_serv_disk_t *p_disk, int fd)
{
    int i;

    CBLK_TRACE_LOG_FILE(9,"allocating a new connection\n");

    for (i = 0; i < MAX_CONNS; i++) {
	if (p_disk->conn_tbl[i].fd == -1) { /* free entry */
	    p_disk->conn_tbl[i].fd = fd;
	    p_disk->conn_tbl[i].rx = cblk_serv_rx_reg;

#ifdef _AIX
	    /*
	     * AIX uses the same index into both the 
	     * conn_tbl and events table.
	     */
	    p_disk->events[i].fd = fd;
	    p_disk->events[i].events = POLLIN|POLLMSG;
	    p_disk->num_poll_events++;
#endif /* _AIX */
	    break;
	}
    }

    return (i == MAX_CONNS) ? NULL : &p_disk->conn_tbl[i];
}

/* 
 * Procedure: cblk_serv_free_connection
 *
 * Description: Free connection
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
void
cblk_serv_free_connection(cblk_serv_disk_t *p_disk, conn_info_t *p_conn_info)
{
    CBLK_TRACE_LOG_FILE(9,"freeing connection %p\n", 
	    p_conn_info);

#ifdef _AIX
    int i;


    for (i = 0; i < MAX_CONNS; i++) {
	if (&(p_disk->conn_tbl[i]) == p_conn_info) { /* matching entry */

	    /*
	     * AIX uses the same index into both the 
	     * conn_tbl and events table.
	     */
	    p_disk->events[i].fd = 0;
	    p_disk->events[i].events = 0;
	    p_disk->num_poll_events--;
	    break;
	}
    }

#endif /* _AIX */

    p_conn_info->fd = -1;
}


/* 
 * Procedure: cblk_undo_eras_serv_init
 *
 * Description: Cleanup/undo cblk_eras_serv_init
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
void cblk_undo_eras_serv_init(cblk_serv_disk_t *p_disk, enum undo_level level)
{

    switch(level) 
    {
    case UNDO_ALL:
    case UNDO_EPOLL_ADD:
    case UNDO_EPOLL_CREATE:
#ifndef _AIX
	close(p_disk->epfd);
#endif

    case UNDO_BIND_SOCK:
	unlink(p_disk->svr_addr.sun_path);
    case UNDO_OPEN_SOCK:
	close(p_disk->listen_fd);
    default:
	break;
    }
}


/* 
 * Procedure: cblk_eras_serv_init
 *
 * Description: Initializes data structure for server thread
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_eras_serv_init(cblk_serv_disk_t *p_disk, char *path)
{
    int i;
    int rc;
    mode_t oldmask;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    char *env_eras_socket = getenv("CFLSH_BLK_ERAS_SOCKET");
#ifndef _AIX
    struct epoll_event     epoll_event;
#endif /* !_AIX */
    enum undo_level level = UNDO_NONE;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

    memset(p_disk, 0, sizeof(*p_disk));

    //?? TODO: Is this MC_PATHLEN correct?

    strncpy(p_disk->eras_dev_path, path, MC_PATHLEN - 1);
    p_disk->name = strrchr(p_disk->eras_dev_path, '/') + 1;
    pthread_mutex_init(&p_disk->mutex, &mattr);
    pthread_cond_init(&p_disk->thread_event, &cattr);

    for (i = 0; i < MAX_CONNS; i++) {
	p_disk->conn_tbl[i].fd = -1;
    }

    // keep AFU accessed data in RAM as much as possible
//    mlock(p_disk->rht, sizeof(p_disk->rht));
    level = UNDO_MLOCK;

    pthread_condattr_destroy(&cattr);
    pthread_mutexattr_destroy(&mattr);


    // Create a socket to be used for listening to connections
    p_disk->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (p_disk->listen_fd < 0) {
	CBLK_TRACE_LOG_FILE(1,"socket failed, errno %d\n", errno);
	cblk_undo_eras_serv_init(p_disk, level);
	return -1;
    }
    level = UNDO_OPEN_SOCK;

    // Bind the socket to the file
    bzero(&p_disk->svr_addr, sizeof(struct sockaddr_un));
    p_disk->svr_addr.sun_family      = AF_UNIX;

    if (env_eras_socket) {
	strcpy(p_disk->svr_addr.sun_path, env_eras_socket);
    } else {
	strcpy(p_disk->svr_addr.sun_path, CFLSH_BLK_ERAS_SOCKET_DIR);
    }
    strcat(p_disk->svr_addr.sun_path, p_disk->eras_dev_path);
    cblk_serv_mkdir_p(p_disk->svr_addr.sun_path); // make intermediate directories
    unlink(p_disk->svr_addr.sun_path);
    // create socket with rwx for group but no perms for others
    oldmask = umask(007); 
    rc = bind(p_disk->listen_fd, (struct sockaddr *)&p_disk->svr_addr, 
	      sizeof(p_disk->svr_addr));
    umask(oldmask); // set umask back to default
    if (rc) {
	CBLK_TRACE_LOG_FILE(1,"bind failed, rc %d, errno %d\n", rc, errno);
	cblk_undo_eras_serv_init(p_disk, level);
	return -1;
    }
    level = UNDO_BIND_SOCK;

    // do not listen on the socket at this point since we do not
    // want clients to be able to connect yet.

#ifndef _AIX
    /*
     * NOTE: This functionality is currently broken in AIX, which
     *       does not support epoll.  So if this is needed in AIX
     *       some analog must be added.
     */

    // Create the epoll array to be used for waiting on events.
    p_disk->epfd = epoll_create(MAX_CONN_TO_POLL);
    if (p_disk->epfd == -1) {
	CBLK_TRACE_LOG_FILE(1,"epoll_create failed, errno %d\n", errno);
	cblk_undo_eras_serv_init(p_disk, level);
	return -1;
    }
    level = UNDO_EPOLL_CREATE;

    // Add the listening file descriptor to the epoll array
    p_disk->conn_tbl[0].fd = p_disk->listen_fd;
    memset(&epoll_event, 0, sizeof(struct epoll_event));
    epoll_event.events = EPOLLIN;
    epoll_event.data.ptr = &p_disk->conn_tbl[0];
    rc = epoll_ctl(p_disk->epfd, EPOLL_CTL_ADD, p_disk->listen_fd, &epoll_event);
    if (rc) {
	CBLK_TRACE_LOG_FILE(1,"epoll_ctl failed for ADD, rc %d, errno %d\n", 
		rc, errno);
	cblk_undo_eras_serv_init(p_disk, level);
	return -1;
    }
    level = UNDO_EPOLL_ADD;
#else

    memset(p_disk->events,0,sizeof(struct pollfd) * MAX_CONN_TO_POLL);

    // Add the listening file descriptor to the epoll array
    p_disk->conn_tbl[0].fd = p_disk->listen_fd;
    p_disk->events[0].fd = p_disk->listen_fd;
    p_disk->events[0].events = POLLIN|POLLMSG;
    p_disk->num_poll_events++;
#endif /* !_AIX */

    return 0;
}


/* 
 * Procedure: cblk_serv_term
 *
 * Description: Terminate interfaces with ERAS server
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_serv_term(cblk_serv_disk_t *p_disk)
{
    cblk_undo_eras_serv_init(p_disk, UNDO_ALL);

    return 0;
}

// all do_xxx functions return 0 or an errno value
//

/*
 * NAME:        cblk_serv_do_register
 *
 * FUNCTION:    Register a user with the ERAS server
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              challenge   - Challenge to validate if requester owns
 *                            the context it is trying to register.
 * OUTPUTS:
 *              none
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful:
 *               a. Sets CTX_CAP
 *               c. Clears all RHT entries effectively making 
 *                  all resource handles invalid.
 *               d. goes to cblk_serv_rx_ready state
 *
 * If registation fails, stay in cblk_serv_rx_reg state to allow client retries.
 */
int
cblk_serv_do_register(cblk_serv_disk_t        *p_disk, 
	       conn_info_t  *p_conn_info,
	       uint64_t        challenge)
{


    CBLK_TRACE_LOG_FILE(9,"client_pid=%d client_fd=%d\n",
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd);



    /* a fresh registration will cause all previous registrations,
       if any, to be forcefully canceled. This is important since 
       a client can close the context (AFU) but not unregister the 
       mc_handle. A new owner of the same context must be able to 
       mc_register by forcefully unregistering the previous owner.
    */
    if (p_conn_info->mode == MCREG_INITIAL_REG) {
#ifdef _NOT_YET
	//?? Need to ensure someone does not double register.
	for (i = 0; i < MAX_CONNS; i++) {
	    if (p_disk->conn_tbl[i].p_ctx_info == p_ctx_info) {
		cblk_serv_do_register(p_disk, &p_disk->conn_tbl[i]);
	    }
	}
#endif /* _NOT_YET */

    }

    p_conn_info->rx = cblk_serv_rx_ready; /* it is now registered, go to ready state */
    return 0;

}

/*
 * NAME:        cblk_serv_do_unregister
 *
 * FUNCTION:    Unregister a user with the ERAS server
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *
 * OUTPUTS:
 *              none
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful:
 *               a. RHT_START, RHT_CNT & CTX_CAP registers for the 
 *                  context are cleared
 *               b. There is no need to clear RHT entries since
 *                  RHT_CNT=0.
 *               c. goes to cblk_serv_rx_reg state to allow re-registration
 */
int
cblk_serv_do_unregister(cblk_serv_disk_t        *p_disk, 
			       conn_info_t  *p_conn_info)
{



    CBLK_TRACE_LOG_FILE(9,"client_pid=%d client_fd=%d",
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd);

    //?? Need lock to unlock all locks for this client.
    p_conn_info->rx = cblk_serv_rx_reg; /* client can now send another MCREG */
    
    return 0;
} 




/*
 * NAME:        cblk_serv_do_set_trace
 *
 * FUNCTION:    Changes the trace verbosity.
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              flags       - Permission flags
 *
 * OUTPUTS:
 *              
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful, the RHT entry contains
 *               a. non-zero NMASK, format & permission bits.
 *               b. LXT_START & LXT_CNT are still zeroed. For all purposes,
 *                  the resource handle is opened in SW, but invalid in HW
 *                  due to 0 size.
 *
 *              A zero NMASK means the RHT entry is free/closed.
 */
int
cblk_serv_do_set_trace(cblk_serv_disk_t        *p_disk, 
			      conn_info_t  *p_conn_info, 
			      int verbosity,
			      char *filename,
			      uint64_t  flags)
{
    int old_verbosity = cblk_log_verbosity;
    char env_verbosity[PATH_MAX];
    char env_trc_filename[PATH_MAX];
    char env_use_syslog[PATH_MAX];
    int setup_trace_files = FALSE;




    if (flags & CBLK_TRC_CLIENT_SET_FILENAME) {

	/*
	 * Change trace filename
	 */

	

	/*
	 * Update environment variable for trace filename to match
	 * this setting in case we need to call cblk_setup_trace_files
	 */


	sprintf(env_trc_filename,"CFLSH_BLK_TRACE=%s",filename);

	if (putenv(env_trc_filename)) {

	    CBLK_TRACE_LOG_FILE(1,"putenv failed with string = %s",env_trc_filename);
	}

	setup_trace_files = TRUE;
	
#ifndef _AIX

	
	if (filename) {

	    /*
	     * If a filename is specified then turn off
	     * syslog
	     */

	    sprintf(env_use_syslog,"CFLSH_BLK_TRC_SYSLOG=OFF");

	} else {
	    /*
	     * If a filename is not specified then turn on
	     * syslog
	     */
	    sprintf(env_use_syslog,"CFLSH_BLK_TRC_SYSLOG=ON");
	}

	if (putenv(env_use_syslog)) {

	    CBLK_TRACE_LOG_FILE(1,"putenv failed with string = %s",env_use_syslog);
	}

#endif /* !_AIX */

    }


    if (flags & CBLK_TRC_CLIENT_SET_VERBOSITY) {


	/* 
	 * Change trace verbosity
	 */



	cblk_log_verbosity = verbosity;

	/*
	 * Update environment variable for verbosity to match
	 * this setting in case we need to call cblk_setup_trace_files
	 */


	sprintf(env_verbosity,"CFLSH_BLK_TRC_VERBOSITY=%d",verbosity);


	if (putenv(env_verbosity)) {

	    CBLK_TRACE_LOG_FILE(1,"putenv failed with string = %s",env_verbosity);
	}


	if ((old_verbosity == 0) &&
	    (verbosity)) {


	    /*
	     * If verbosity is 0, and we changing it to a non-zero value,
	     * then we need to setup the trace files.
	     */


	    setup_trace_files = TRUE;

	} else {

	    CBLK_TRACE_LOG_FILE(1,"changed verbosity to %d of trace file",verbosity);
	}
    }


    if (setup_trace_files) {

	/*
	 * We need to setup trace files for this change
	 */

	cblk_setup_trace_files(0);
    }



    return 0;
} 



/*
 * NAME:        cblk_serv_do_live_dump
 *
 * FUNCTION:    Do a live dump of the library for this process.
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              flags       - Permission flags
 *
 * OUTPUTS:
 *              p_res_hndl  - Pointer to allocated resource handle
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful, the RHT entry contains
 *               a. non-zero NMASK, format & permission bits.
 *               b. LXT_START & LXT_CNT are still zeroed. For all purposes,
 *                  the resource handle is opened in SW, but invalid in HW
 *                  due to 0 size.
 *
 *              A zero NMASK means the RHT entry is free/closed.
 */
int
cblk_serv_do_live_dump(cblk_serv_disk_t        *p_disk, 
			      conn_info_t  *p_conn_info, 
			      char         *reason,
			      uint64_t     flags)
{
    char reason_str[100];


    sprintf(reason_str,"eras_Socket:%s",reason);

    if (cblk_setup_dump_file()) {

	return -1;
    }

    cblk_dump_debug_data(reason_str,__FILE__,__FUNCTION__,__LINE__,__DATE__);


    return 0;
} 


/*
 * NAME:        cblk_serv_do_get_stats
 *
 * FUNCTION:    Get this processes, block statistics.
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              flags       - Permission flags
 *
 * OUTPUTS:
 *              p_res_hndl  - Pointer to allocated resource handle
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful, the RHT entry contains
 *               a. non-zero NMASK, format & permission bits.
 *               b. LXT_START & LXT_CNT are still zeroed. For all purposes,
 *                  the resource handle is opened in SW, but invalid in HW
 *                  due to 0 size.
 *
 *              A zero NMASK means the RHT entry is free/closed.
 */
int
cblk_serv_do_get_stats(cblk_serv_disk_t        *p_disk, 
		       conn_info_t  *p_conn_info, 
		       char *disk_name,
		       chunk_stats_t *stat,
		       uint64_t     flags)
{
    int i;
    cflsh_chunk_t *chunk;
    int found = FALSE;
    


    // ?? TODO: Taking this global lock could impact performance
    
    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);

    for (i=0;i<MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;

	while (chunk) {

	    if (!strcmp(chunk->dev_name,disk_name)) {

		CFLASH_BLOCK_LOCK(chunk->lock);

		/* 
		 * Copy stats back to caller
		 */


		bcopy((void *)&(chunk->stats),(void *)stat, sizeof(chunk->stats));

		CFLASH_BLOCK_UNLOCK(chunk->lock);

		found = TRUE;

		break;
	    }

	    chunk = chunk->next;

	} /* while */


	if (found) {

	    break;
	}

    } /* for */

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

 

    return 0;
} 


/***************************************************************************** 
 * Procedure: cblk_serv_xfer_data
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
cblk_serv_xfer_data(int fd, int op, void *buf, ssize_t exp_size)
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

/* 
 * Procedure: cblk_serv_rx_reg
 *
 * Description: rx fcn on a fresh connection waiting a MCREG.
 *              On receipt of a MCREG, it will go to the 
 *              cblk_serv_rx_ready state where all cmds except a 
 *              MCREG is accepted.
 *
 *              
 *
 * Return:     0, if successful
 *             non-zero otherwise
 */
int cblk_serv_rx_reg(cblk_serv_disk_t *p_disk, conn_info_t *p_conn_info,
	     cflsh_blk_eras_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CFLSH_BLK_CMD_ERAS_REG: 
	// first command on a fresh connection
	// copy input fields to connection info 
	p_conn_info->client_pid = p_req->reg.client_pid;
	p_conn_info->client_fd = p_req->reg.client_fd;



	status = cblk_serv_do_register(p_disk, p_conn_info, p_req->reg.challenge);

	break;

    default:
	// fail everything else
	CBLK_TRACE_LOG_FILE(1,"bad command code %d in wait_mcreg state\n",
		p_req->header.command);
	status = EINVAL;
	break;
    }

    return status;
}

int cblk_serv_rx_ready(cblk_serv_disk_t *p_disk, conn_info_t *p_conn_info, 
	     cflsh_blk_eras_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CFLSH_BLK_CMD_ERAS_UNREG:
	(void) cblk_serv_do_unregister(p_disk, p_conn_info);
	status = 0;
	break;

    case CFLSH_BLK_CMD_ERAS_SET_TRACE:
	status = cblk_serv_do_set_trace(p_disk, p_conn_info, 
					       p_req->trace.verbosity,
					       p_req->trace.filename,
					       p_req->trace.flags);
	break;
    case CFLSH_BLK_CMD_ERAS_LIVE_DUMP:
	status = cblk_serv_do_live_dump(p_disk, p_conn_info, 
					       p_req->live_dump.reason,
					       p_req->live_dump.flags);
	break;	

    case CFLSH_BLK_CMD_ERAS_GET_STATS:
	status = cblk_serv_do_get_stats(p_disk, p_conn_info, 
					p_req->stat.disk_name,
					&p_resp->stat.stat,
					p_req->stat.flags);
	break;	


    default :
	CBLK_TRACE_LOG_FILE(1,"bad command code %d in ready state\n",
		p_req->header.command);
	status = EINVAL;
	break;
    }

    return status;
}

void *cflash_block_eras_thread(void *arg) 
{
    void *ret_code = NULL;
    int                    conn_fd;
    int                    rc;
    int                    i;
    int                    nready;
    socklen_t              len;
    struct sockaddr_un     cli_addr;
#ifndef _AIX
    struct epoll_event     epoll_event;
#endif /* !_AIX */
    cflsh_blk_eras_req_t  mc_req;
    cflsh_blk_eras_resp_t  mc_resp;
    conn_info_t            *p_conn_info;
    conn_info_t            *p_conn_info_new;

    cblk_serv_disk_t       *p_disk = (cblk_serv_disk_t *) arg;

    // The listen is delayed until everything is ready. This prevent
    // clients from connecting to the server until it reaches this
    // point.
    rc = listen(p_disk->listen_fd, SOMAXCONN);
    if ( rc ) {
	CBLK_TRACE_LOG_FILE(1,"listen failed, rc %d, errno %d\n", rc, errno);
	return ret_code;
    }

    while (1) { 


#ifndef _AIX

	//
	// Wait for an event on any of the watched file descriptors
	// block for ever if no events.
	//
	// Note we poll all file descriptors in the epoll structure
	// but receive only up to MAX_CONN_TO_POLL ready fds at a time.
	//
	nready = epoll_wait(p_disk->epfd, &p_disk->events[0], 
			    MAX_CONN_TO_POLL, 1);
#else
	nready = poll(&p_disk->events,p_disk->num_poll_events,1);

#endif /* _AIX */


	if (p_disk->flags & CBLK_SRV_THRD_EXIT) {

	    /*
	     * Thread needs to exit
	     */
	    
	    break;
	}


	if ((nready == -1) && (errno == EINTR)) {
	    continue;
	}



#ifndef _AIX

	for (i = 0; i < nready; i++) {
	    p_conn_info = (conn_info_t*) p_disk->events[i].data.ptr;
#else 

	if (nready == 0) {
	    continue;
	}

	for (i = 0; i < p_disk->num_poll_events; i++) {


	    if (p_disk->events[i].revents == 0) {

		continue;
	    }
	    p_conn_info = &(p_disk->conn_tbl[i]);

#endif /* _AIX */

#ifndef _AIX
            if (p_disk->events[i].events & (EPOLLHUP | EPOLLERR))  {
#else
            if (p_disk->events[i].revents & (POLLHUP | POLLERR))  {
#endif /* _AIX */


		// Something bad happened with the connection.  Assume the
		// worse and remove the file descriptor from the array of
		// watched FD's
		CBLK_TRACE_LOG_FILE(1,
			"connection is bad, %d (0x%08X), client fd %d\n", 
			p_conn_info->fd, p_disk->events[i].events, 
			p_conn_info->client_fd);


#ifndef _AIX
		rc = epoll_ctl(p_disk->epfd, EPOLL_CTL_DEL, p_conn_info->fd, 
			       &p_disk->events[i]);
		if (rc) {
		    CBLK_TRACE_LOG_FILE(1, 
			    "epoll_ctl failed for DEL: %d (%d)\n", rc, errno);
		}

#endif /* !_AIX */

		close(p_conn_info->fd);
		if (p_conn_info->p_ctx_info != NULL) { /* if registered */
		    (void) cblk_serv_do_unregister(p_disk, p_conn_info);
		}
		cblk_serv_free_connection(p_disk, p_conn_info);
		
		continue;
	    }
      
	    // Is this the listening FD...would mean a new connection
	    if (p_conn_info->fd == p_disk->listen_fd) {
		len = sizeof(cli_addr);
		conn_fd = accept(p_disk->listen_fd, (struct sockaddr *)&cli_addr, &len); 
		if (conn_fd == -1) {
		    CBLK_TRACE_LOG_FILE(1,
			    "accept failed, fd %d, errno %d\n", conn_fd, errno);
		    continue;
		}

		p_conn_info_new = cblk_serv_alloc_connection(p_disk, conn_fd); 
		if (p_conn_info_new == NULL) {
		    CBLK_TRACE_LOG_FILE(1,
			    "too many connections, closing new connection\n");
		    close(conn_fd);
		    continue;
		}

#ifndef _AIX
		// Add the connection to the watched array
		epoll_event.events = EPOLLIN;
		epoll_event.data.ptr = p_conn_info_new;
		rc = epoll_ctl(p_disk->epfd, EPOLL_CTL_ADD, conn_fd, &epoll_event);
		if (rc == -1) {
		    CBLK_TRACE_LOG_FILE(1,
			    "epoll_ctl ADD failed, errno %d\n", errno);
		    close(conn_fd);
		    continue;
		}
#endif /* !_AIX */

	    }
	    else {
		// We can now assume that we have data to be read from a client
		// Read the entire command
		rc = cblk_serv_xfer_data(p_conn_info->fd, XFER_OP_READ, 
			       (void *)&mc_req, sizeof(mc_req));
		if (rc) {
		    CBLK_TRACE_LOG_FILE(1,"read of cmd failed, rc %d\n", rc);


#ifdef _AIX
		    /*
		     * This indicates the connection was closed on the
		     * other end. Let's close our connection and clean 
		     * up.
		     */

		    close(p_conn_info->fd);
		    if (p_conn_info->p_ctx_info != NULL) { /* if registered */
			(void) cblk_serv_do_unregister(p_disk, p_conn_info);
		    }
		    cblk_serv_free_connection(p_disk, p_conn_info);
#endif /* _AIX */
		    continue;
		}

		CBLK_TRACE_LOG_FILE(1,
			"command code=%d, tag=%d, size=%d, conn_info=%p\n", 
			mc_req.header.command,
			mc_req.header.tag,
			mc_req.header.size,
			p_conn_info);

		// process command and fill in response while protecting from
		// other threads accessing the disk

		    
	        mc_resp.header.command  = mc_req.header.command;
	        mc_resp.header.tag  = mc_req.header.tag;
	        mc_resp.header.size = sizeof(mc_resp);

		if (p_conn_info->rx) {
		    pthread_mutex_lock(&p_disk->mutex);

		    p_conn_info->mc_resp.header.status =  
			(*p_conn_info->rx)(p_disk, p_conn_info, &mc_req, &mc_resp);

		    pthread_mutex_unlock(&p_disk->mutex);
		} else {

		    
		    CBLK_TRACE_LOG_FILE(1,
			    "rx function pointer is null");
		}


	        if (p_conn_info->mc_resp.header.status!= EBUSY) {

		    // Send response back to the client.
		    rc = cblk_serv_xfer_data(p_conn_info->fd, XFER_OP_WRITE, 
			       (void *)&(mc_resp), sizeof(mc_resp));
		    if (rc) {
		        CBLK_TRACE_LOG_FILE(1,
				"write of cmd response failed, rc %d\n", rc);
			continue;
		    }
		}
	    }
	}
    }

    return ret_code;
}




/*
 * NAME:        cblk_serv_mkdir_p
 *
 * FUNCTION:    Make intermediate directories for socket file.
 *
 *
 * INPUTS:
 *              Flag  
 *
 * RETURNS:
 *              0 - Success, otherwise error
 *              
 */
int cblk_serv_mkdir_p(char *file_path) // path must be absolute and must end on a file
{
    struct stat s;
    char *p;
    char *last = NULL;

    if (file_path[0] != '/') {
	return -1;
    }

    if ((last = strrchr(file_path, '/'))) {
	*last = '\0'; // get rid of the last component
    }

    for (p = (file_path + 1); *p; p++) {
	if (*p == '/') {
	    *p = '\0';
	    if (stat(file_path, &s) == -1) {
	        // create dirs as traversable by group members and no one else
		mkdir(file_path, 0710); 
	    }
	    *p = '/';
	}
    }

    if (stat(file_path, &s) == -1) {
        // create dirs as traversable by group members and no one else
	mkdir(file_path, 0710); 
    }

    if (last) { // restore original file_path
	*last = '/';
    }

    return 0;
}




/*
 * NAME:        cblk_start_eras_thread
 *
 * FUNCTION:    Start background ERAS thread to monitor for IPC 
 *              requests to adjust tracing, live dump and collect
 *              statistics.
 *
 *
 * INPUTS:
 *              Flag  
 *
 * RETURNS:
 *              0 - Success, otherwise error
 *              
 */
int cblk_start_eras_thread(int flags)
{

    int rc;
    char filename[PATH_MAX];
    int pthread_rc;


    sprintf(filename,"%d",getpid());




    rc = cblk_eras_serv_init(&cblk_serv_disk, filename);
    if (rc != 0) {
	CBLK_TRACE_LOG_FILE(1,"error instantiating initing eras thread %s, rc %d\n", 
		filename, rc);
	errno = EAGAIN;
	rc = -1;

    }

    /* When all went well, start IPC threads to take requests */
    pthread_rc = pthread_create(&cblk_serv_disk.ipc_thread, NULL, cflash_block_eras_thread, 
				&cblk_serv_disk);
    

    if (pthread_rc) {
	    
		    
	CBLK_TRACE_LOG_FILE(1,"pthread_create failed rc = %d,errno = %d ",
			    pthread_rc,errno);

	errno = EAGAIN;
	rc = -1;
    }



    if (!rc) {

	CBLK_TRACE_LOG_FILE(9,"started eras thread %s\n", 
		filename);
    }

 
    return rc;
}

/*
 * NAME:        cblk_stop_eras_thread
 *
 * FUNCTION:    If we are running with a single common interrupt thread
 *              per chunk, then this routine terminates that thread
 *              and waits for completion.
 *
 *
 * INPUTS:
 *              chunk - Chunk to be cleaned up.
 *
 * RETURNS:
 *              NONE
 *              
 */
void cblk_stop_eras_thread(int flags) 
{

    int pthread_rc = 0;
    void *status;


    

    cblk_serv_disk.flags |= CBLK_SRV_THRD_EXIT;

    pthread_rc = pthread_cond_signal(&(cblk_serv_disk.thread_event));
	
    if (pthread_rc) {
	    
	CBLK_TRACE_LOG_FILE(1,"pthread_cond_signal failed rc = %d,errno = %d",
			    pthread_rc,errno);
    }


    pthread_join(cblk_serv_disk.ipc_thread, &status);
    cblk_serv_term(&cblk_serv_disk);

    cblk_serv_disk.flags &= ~CBLK_SRV_THRD_EXIT;

    //?? TODO unlink socket file for this server

    return;
}
