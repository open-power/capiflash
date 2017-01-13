//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/master/cusfs_serv.c $
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

#include <cusfs_serv.h>
#include "cflsh_usfs_client.h"
#include <prov.h>

#ifndef _AIX
#include <revtags.h>
REVISION_TAGS(cusfs_serv);
#endif

// LOG_ERR(3), LOG_WARNING(4), LOG_NOTICE(5), LOG_INFO(6) & LOG_DEBUG(7)
//
// TRACE_x versions: fmt must be of the form: "%s: xxxx" - the
// leading %s is the disk name
//
// trace_x versions: used when the log is not specific to 
// a particular disk or the disk name is not available
//
#define TRACE_0(lvl, p_disk, fmt) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name); }

#define TRACE_1(lvl, p_disk, fmt, A) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name, A); }

#define TRACE_2(lvl, p_disk, fmt, A, B) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name, A, B); }

#define TRACE_3(lvl, p_disk, fmt, A, B, C) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name, A, B, C); }

#define TRACE_4(lvl, p_disk, fmt, A, B, C, D)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name, A, B, C, D); }

#define TRACE_5(lvl, p_disk, fmt, A, B, C, D, E)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_disk)->name, A, B, C, D, E); }

// these do not have a disk name
#define trace_0(lvl, fmt) \
    if (trc_lvl > lvl) { syslog(lvl, fmt); }

#define trace_1(lvl, fmt, A) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A); }

#define trace_2(lvl, fmt, A, B) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B); }

#define trace_3(lvl, fmt, A, B, C) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B, C); }

unsigned int trc_lvl = LOG_INFO; // by default, log NOTICE and lower levels.
                                 // lower level is higher severity

/***************************************************************************
 *                        Master Context Server
 *
 * The functions of the master context are:
 *
 * 1. virtualize physical LUN(s) for user contexts. Users send IO to a
 *    resource handle that identifies a virtual LUN. Resource handles
 *    are mapped to physical LBAs by the master.
 * 2. control what a user context is allowed or not allowed to do.
 *
 * There are several key functions that are not provided by the master:
 *
 * 1. Does not handle different linux disks (paths) with same filesystem.
 *
 *
 * A single instance of the master serves a host. 
 *
 *
 ***************************************************************************/

typedef struct {
    struct capikv_ini *p_ini;
    struct afu_alloc *p_disk_a;  //??
} global_t;

global_t gb;  /* cusfs_serv globals */


conn_info_t*
alloc_connection(cusfs_serv_disk_t *p_disk, int fd)
{
    int i;

    TRACE_0(LOG_INFO, p_disk,
	    "%s: allocating a new connection\n");

    for (i = 0; i < MAX_CONNS; i++) {
	if (p_disk->conn_tbl[i].fd == -1) { /* free entry */
	    p_disk->conn_tbl[i].fd = fd;
	    p_disk->conn_tbl[i].rx = rx_mcreg;

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

void
free_connection(cusfs_serv_disk_t *p_disk, conn_info_t *p_conn_info)
{
    TRACE_1(LOG_INFO, p_disk, "%s: freeing connection %p\n", 
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


void undo_master_init(cusfs_serv_disk_t *p_disk, enum undo_level level)
{

    switch(level) 
    {
    case UNDO_AFU_ALL:
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


int master_init(cusfs_serv_disk_t *p_disk, struct capikv_ini_elm *p_elm)
{
    int i;
    int rc;
    mode_t oldmask;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    char *env_master_socket = getenv("CFLSH_USFS_MASTER_SOCKET");
#ifndef _AIX
    struct epoll_event     epoll_event;
#endif /* !_AIX */
    enum undo_level level = UNDO_NONE;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

    memset(p_disk, 0, sizeof(*p_disk));

    strncpy(p_disk->master_dev_path, p_elm->afu_dev_pathm, MC_PATHLEN - 1);
    p_disk->name = strrchr(p_disk->master_dev_path, '/') + 1;
    pthread_mutex_init(&p_disk->mutex, &mattr);
    pthread_mutex_init(&p_disk->err_mutex, &mattr);
    pthread_cond_init(&p_disk->err_cv, &cattr);

    for (i = 0; i < MAX_CONNS; i++) {
	p_disk->conn_tbl[i].fd = -1;
    }

    // keep AFU accessed data in RAM as much as possible
//    mlock(p_disk->rht, sizeof(p_disk->rht));
    level = UNDO_MLOCK;

    pthread_condattr_destroy(&cattr);
    pthread_mutexattr_destroy(&mattr);

    // open master device ??
    p_disk->afu_fd = open(p_disk->master_dev_path, O_RDWR);
    if (p_disk->afu_fd < 0) {
	TRACE_1(LOG_ERR, p_disk, "%s: open failed, errno %d\n", errno);
	undo_master_init(p_disk, level);
	return -1;
    }
    level = UNDO_AFU_OPEN;

    // enable the AFU. This must be done before mmap.


    // Create a socket to be used for listening to connections
    p_disk->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (p_disk->listen_fd < 0) {
	TRACE_1(LOG_ERR, p_disk, "%s: socket failed, errno %d\n", errno);
	undo_master_init(p_disk, level);
	return -1;
    }
    level = UNDO_OPEN_SOCK;

    // Bind the socket to the file
    bzero(&p_disk->svr_addr, sizeof(struct sockaddr_un));
    p_disk->svr_addr.sun_family      = AF_UNIX;

    if (env_master_socket) {
	strcpy(p_disk->svr_addr.sun_path, env_master_socket);
    } else {
	strcpy(p_disk->svr_addr.sun_path, CFLSH_USFS_MASTER_SOCKET_DIR);
    }
    strcat(p_disk->svr_addr.sun_path, p_disk->master_dev_path);
    mkdir_p(p_disk->svr_addr.sun_path); // make intermediate directories
    unlink(p_disk->svr_addr.sun_path);
    // create socket with rwx for group but no perms for others
    oldmask = umask(007); 
    rc = bind(p_disk->listen_fd, (struct sockaddr *)&p_disk->svr_addr, 
	      sizeof(p_disk->svr_addr));
    umask(oldmask); // set umask back to default
    if (rc) {
	TRACE_2(LOG_ERR, p_disk, "%s: bind failed, rc %d, errno %d\n", rc, errno);
	undo_master_init(p_disk, level);
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
	TRACE_1(LOG_ERR, p_disk, "%s: epoll_create failed, errno %d\n", errno);
	undo_master_init(p_disk, level);
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
	TRACE_2(LOG_ERR, p_disk, "%s: epoll_ctl failed for ADD, rc %d, errno %d\n", 
		rc, errno);
	undo_master_init(p_disk, level);
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


int cusfs_serv_disk_term(cusfs_serv_disk_t *p_disk)
{
    undo_master_init(p_disk, UNDO_AFU_ALL);

    return 0;
}

// all do_xxx functions return 0 or an errno value
//

/*
 * NAME:        do_master_register
 *
 * FUNCTION:    Register a user AFU context with master.
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
 *               d. goes to rx_ready state
 *
 * If registation fails, stay in rx_mcreg state to allow client retries.
 */
int
do_master_register(cusfs_serv_disk_t        *p_disk, 
	       conn_info_t  *p_conn_info,
	       uint64_t        challenge)
{


    TRACE_3(LOG_INFO, p_disk, "%s: %s, client_pid=%d client_fd=%d\n",
	    __func__,
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
		do_master_unregister(p_disk, &p_disk->conn_tbl[i]);
	    }
	}
#endif /* _NOT_YET */

    }

    p_conn_info->rx = rx_ready; /* it is now registered, go to ready state */
    return 0;

}

/*
 * NAME:        do_master_unregister
 *
 * FUNCTION:    Unregister a user AFU context with master.
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
 *               c. goes to rx_mcreg state to allow re-registration
 */
int
do_master_unregister(cusfs_serv_disk_t        *p_disk, 
		 conn_info_t  *p_conn_info)
{



    TRACE_3(LOG_INFO, p_disk, "%s: %s, client_pid=%d client_fd=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd);

    //?? Need lock to unlock all locks for this client.
    p_conn_info->rx = rx_mcreg; /* client can now send another MCREG */
    
    return 0;
} 




/*
 * NAME:        do_master_lock
 *
 * FUNCTION:    Get a shared filesystem lock
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
do_master_lock(cusfs_serv_disk_t        *p_disk, 
	   conn_info_t  *p_conn_info, 
	   cflsh_usfs_master_lock_t lock,
	   uint64_t     flags)
{
    int rc = 0;

    if (lock >= CFLSH_USFS_MASTER_LAST_ITEM) {

	errno = EINVAL;
	return -1;
    }

    if (p_disk->locks[lock].client_pid == 0) {

	p_disk->locks[lock].client_pid = p_conn_info->client_pid;
    } else {
	
	CUSFS_Q_NODE_TAIL(p_disk->locks[lock].head,p_disk->locks[lock].tail,p_conn_info,prev,next);
	rc = EBUSY;
    }


    TRACE_4(LOG_INFO, p_disk, "%s: %s, client_pid=%d client_fd=%d lock=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    lock);

    return rc;
} 


/*
 * NAME:        do_master_unlock
 *
 * FUNCTION:    Release a shared filesystem lock
 *              
 *
 * INPUTS:
 *              p_disk       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              res_hndl    - resource handle to close
 *
 * OUTPUTS:
 *              none
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              When successful, the RHT entry is cleared.
 */
int
do_master_unlock(cusfs_serv_disk_t        *p_disk, 
	     conn_info_t  *p_conn_info,
	     cflsh_usfs_master_lock_t lock,
	     uint64_t     flags)
{
    conn_info_t  *p_conn_info_next;
    cflsh_usfs_master_resp_t  mc_resp;
    int rc = 0;




    if (p_disk->locks[lock].client_pid == p_conn_info->client_pid) {

	p_disk->locks[lock].client_pid = 0;
	p_conn_info_next = p_disk->locks[lock].head;

	if (p_conn_info_next) {
	    CUSFS_DQ_NODE(p_disk->locks[lock].head,p_disk->locks[lock].tail,p_conn_info,prev,next);
	    
	    // Send response back to the client.

	    p_conn_info_next->mc_resp.header.status = 0;

	    rc = xfer_data(p_conn_info_next->fd, XFER_OP_WRITE, 
			       (void *)&(p_conn_info_next->mc_resp), sizeof(mc_resp));
	    if (rc) {
		TRACE_1(LOG_ERR, p_disk,
			"%s: write of cmd response failed, rc %d\n", rc);

	    }
	}
    }

    TRACE_4(LOG_INFO, p_disk, "%s: %s, client_pid=%d client_fd=%d lock=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    lock);


    return 0;
}


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

// rx fcn on a fresh connection waiting a MCREG.
// On receipt of a MCREG, it will go to the rx_ready state where
// all cmds except a MCREG is accepted.
//
int rx_mcreg(cusfs_serv_disk_t *p_disk, conn_info_t *p_conn_info,
	     cflsh_usfs_master_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CFLSH_USFS_CMD_MASTER_REG: 
	// first command on a fresh connection
	// copy input fields to connection info 
	p_conn_info->client_pid = p_req->reg.client_pid;
	p_conn_info->client_fd = p_req->reg.client_fd;



	status = do_master_register(p_disk, p_conn_info, p_req->reg.challenge);

	break;

    default:
	// fail everything else
	TRACE_1(LOG_ERR, p_disk, "%s: bad command code %d in wait_mcreg state\n",
		p_req->header.command);
	status = EINVAL;
	break;
    }

    return status;
}

int rx_ready(cusfs_serv_disk_t *p_disk, conn_info_t *p_conn_info, 
	     cflsh_usfs_master_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CFLSH_USFS_CMD_MASTER_UNREG:
	(void) do_master_unregister(p_disk, p_conn_info);
	status = 0;
	break;

    case CFLSH_USFS_CMD_MASTER_LOCK:
	status = do_master_lock(p_disk, p_conn_info, 
			    p_req->lock.lock,
			    p_req->lock.flags);
	break;

    case CFLSH_USFS_CMD_MASTER_UNLOCK:
	status = do_master_unlock(p_disk, p_conn_info, 
			    p_req->lock.lock,
			    p_req->lock.flags);
	break;

    default :
	TRACE_1(LOG_ERR, p_disk, "%s: bad command code %d in ready state\n",
		p_req->header.command);
	status = EINVAL;
	break;
    }

    return status;
}

void *afu_ipc_rx(void *arg) {
    int                    conn_fd;
    int                    rc;
    int                    i;
    int                    nready;
    socklen_t              len;
    struct sockaddr_un     cli_addr;
#ifndef _AIX
    struct epoll_event     epoll_event;
#endif /* !_AIX */
    cflsh_usfs_master_req_t  mc_req;
    cflsh_usfs_master_resp_t  mc_resp;
    conn_info_t            *p_conn_info;
    conn_info_t            *p_conn_info_new;

    cusfs_serv_disk_t                  *p_disk = (cusfs_serv_disk_t *) arg;

    // The listen is delayed until everything is ready. This prevent
    // clients from connecting to the server until it reaches this
    // point.
    rc = listen(p_disk->listen_fd, SOMAXCONN);
    if ( rc ) {
	TRACE_2(LOG_ERR, p_disk, "%s: listen failed, rc %d, errno %d\n", rc, errno);
	exit(-1);
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
			    MAX_CONN_TO_POLL, -1);
#else
	nready = poll(&p_disk->events,p_disk->num_poll_events,1);

#endif /* _AIX */


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
		TRACE_3(LOG_ERR, p_disk, 
			"%s: connection is bad, %d (0x%08X), client fd %d\n", 
			p_conn_info->fd, p_disk->events[i].events, 
			p_conn_info->client_fd);


#ifndef _AIX
		rc = epoll_ctl(p_disk->epfd, EPOLL_CTL_DEL, p_conn_info->fd, 
			       &p_disk->events[i]);
		if (rc) {
		    TRACE_2(LOG_ERR, p_disk,
			    "%s: epoll_ctl failed for DEL: %d (%d)\n", rc, errno);
		}

#endif /* !_AIX */

		close(p_conn_info->fd);
		if (p_conn_info->p_ctx_info != NULL) { /* if registered */
		    (void) do_master_unregister(p_disk, p_conn_info);
		}
		free_connection(p_disk, p_conn_info);
		
		continue;
	    }
      
	    // Is this the listening FD...would mean a new connection
	    if (p_conn_info->fd == p_disk->listen_fd) {
		len = sizeof(cli_addr);
		conn_fd = accept(p_disk->listen_fd, (struct sockaddr *)&cli_addr, &len); 
		if (conn_fd == -1) {
		    TRACE_2(LOG_ERR, p_disk,
			    "%s: accept failed, fd %d, errno %d\n", conn_fd, errno);
		    continue;
		}

		p_conn_info_new = alloc_connection(p_disk, conn_fd); 
		if (p_conn_info_new == NULL) {
		    TRACE_0(LOG_ERR, p_disk, 
			    "%s: too many connections, closing new connection\n");
		    close(conn_fd);
		    continue;
		}

#ifndef _AIX
		// Add the connection to the watched array
		epoll_event.events = EPOLLIN;
		epoll_event.data.ptr = p_conn_info_new;
		rc = epoll_ctl(p_disk->epfd, EPOLL_CTL_ADD, conn_fd, &epoll_event);
		if (rc == -1) {
		    TRACE_1(LOG_ERR, p_disk, 
			    "%s: epoll_ctl ADD failed, errno %d\n", errno);
		    close(conn_fd);
		    continue;
		}
#endif /* !_AIX */

	    }
	    else {
		// We can now assume that we have data to be read from a client
		// Read the entire command
		rc = xfer_data(p_conn_info->fd, XFER_OP_READ, 
			       (void *)&mc_req, sizeof(mc_req));
		if (rc) {
		    TRACE_1(LOG_ERR, p_disk, "%s: read of cmd failed, rc %d\n", rc);


#ifdef _AIX
		    /*
		     * This indicates the connection was closed on the
		     * other end. Let's close our connection and clean 
		     * up.
		     */

		    close(p_conn_info->fd);
		    if (p_conn_info->p_ctx_info != NULL) { /* if registered */
			(void) do_master_unregister(p_disk, p_conn_info);
		    }
		    free_connection(p_disk, p_conn_info);
#endif /* _AIX */
		    continue;
		}

		TRACE_4(LOG_INFO, p_disk,
			"%s: command code=%d, tag=%d, size=%d, conn_info=%p\n", 
			mc_req.header.command,
			mc_req.header.tag,
			mc_req.header.size,
			p_conn_info);

		// process command and fill in response while protecting from
		// other threads accessing the disk

		    
	        p_conn_info->mc_resp.header.command  = mc_req.header.command;
	        p_conn_info->mc_resp.header.tag  = mc_req.header.tag;
	        p_conn_info->mc_resp.header.size = sizeof(mc_resp);

		if (p_conn_info->rx) {
		    pthread_mutex_lock(&p_disk->mutex);

		    p_conn_info->mc_resp.header.status =  
			(*p_conn_info->rx)(p_disk, p_conn_info, &mc_req, &mc_resp);

		    pthread_mutex_unlock(&p_disk->mutex);
		} else {

		    
		    TRACE_0(LOG_ERR, p_disk,
			    "%s: rx function pointer is null");
		}


	        if (p_conn_info->mc_resp.header.status!= EBUSY) {

		    // Send response back to the client.
		    rc = xfer_data(p_conn_info->fd, XFER_OP_WRITE, 
			       (void *)&(p_conn_info->mc_resp), sizeof(mc_resp));
		    if (rc) {
			TRACE_1(LOG_ERR, p_disk,
				"%s: write of cmd response failed, rc %d\n", rc);
			continue;
		    }
		}
	    }
	}
    }

    return NULL;
}




int mkdir_p(char *file_path) // path must be absolute and must end on a file
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




void usage(char *prog) {
#ifndef _AIX
    fprintf(stderr, "usage-a: %s [-v level] /dev/sgN ....\n", prog);
    fprintf(stderr, "       : %s /dev/sg5\n", prog);
#else
    fprintf(stderr, "usage-a: %s [-v level] /dev/hdiskN ....\n", prog);
    fprintf(stderr, "       : %s /dev/hdisk5\n", prog);
#endif /* !_AIX */
    fprintf(stderr, "usage-b: %s [-i ini_file] [-v level]\n", prog);
    fprintf(stderr, "       : %s -i myafu.ini\n", prog);
    fprintf(stderr, "       specify any number of disk paths (that have filesystems) in usage-a\n");
    fprintf(stderr, "       level is trace level in decimal from 0(least) to 8(all)\n");
}

// the caller to validate the returned struct
// 
// need:  domain sock dir?
//
struct capikv_ini *parse_cmd_line(int argc, char **argv)
{
    int get_char;
    __u64 lun_id = 0; /* default lun_id to use */
    int nafu;
    int nbytes;
    int i;
    char *ini_path = NULL;
    int fd;
    struct stat sbuf;
    void *map;
    struct capikv_ini_elm *p_elm;
    struct capikv_ini *p_ini;

    while ((get_char = getopt(argc, argv, "i:l:v:h")) != EOF)
    {
        switch (get_char)
        {
	case 'v' :      /* trace level in decimal */
	    sscanf(optarg, "%d", &trc_lvl);
	    break;

	case 'i' :      /* path to init file */
	    ini_path = optarg;
	    break;

	case 'h' :      /* usage help */
	    usage(argv[0]);
	    exit(0);

	default:
	    usage(argv[0]);
	    exit(-1);
	}
    }

    if (ini_path == NULL) {
	nafu = argc - optind; /* number of afus specified in cmd line */

	nbytes = sizeof(*p_ini) + 
	    ((nafu > 1) ? (nafu - 1)*sizeof(*p_elm) : 0);

	p_ini = (struct capikv_ini *) malloc(nbytes);
	if (p_ini == NULL) {
	    trace_1(LOG_ERR, "cannot allocate %d bytes\n", nbytes);
	    return NULL;
	}

	memset(p_ini, 0, nbytes);
	p_ini->sini_marker = 0x53494e49;
	p_ini->nelm = nafu;
	p_ini->size = sizeof(*p_elm);

	for (i = 0; i < nafu; i++) {
	    p_elm = &p_ini->elm[i];

	    // for this mode, the user enters the master dev path.
	    // also assume wwpns are already programmed off-line and
	    // master should leave them alone
	    //
	    p_elm->elmd_marker = 0x454c4d44;
	    strcpy(&p_elm->afu_dev_pathm[0], argv[i + optind]);
	    p_elm->lun_id = lun_id;
	}
    }
    else {
	fd = open(ini_path, O_RDONLY);
	if (fd < 0) {
	    trace_2(LOG_ERR, "open of %s failed, errno %d\n", ini_path, errno);
	    return NULL;
	}

	if (fstat(fd, &sbuf) != 0) {
	    trace_2(LOG_ERR, "fstat failed on %s, errno %d\n", ini_path, errno);
	    close(fd);
	    return NULL;
	}

	map = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
	    trace_1(LOG_ERR, "mmap failed, errno %d\n", errno);
	    close(fd);
	    return NULL;
	}

	p_ini = (struct capikv_ini *) map;
    }

    return p_ini;
}


int
main(int argc, char *argv[])
{
    int i;
    int rc;
    cusfs_serv_disk_t *p_disk;
    sigset_t set;
    struct capikv_ini_elm *p_elm;
    struct capikv_ini *p_ini;
    struct afu_alloc *p_disk_a;




    openlog("cusfs_serv", LOG_PERROR, LOG_USER);

    p_ini = parse_cmd_line(argc, argv);

    /* some sanity checks */
    if (p_ini == NULL || 
	p_ini->nelm == 0 ||
	p_ini->sini_marker != 0x53494e49) {
	trace_0(LOG_ERR, "bad input parameters, exiting...\n");
	usage(argv[0]); // additional errors already logged by parse_cmd_line()
	exit(-1);
    }
    gb.p_ini = p_ini;

    sigemptyset(&set);
    sigaddset(&set, SIGRTMIN);
    /* signal mask is inherited by all threads created from now on */
    pthread_sigmask(SIG_BLOCK, &set, NULL); 

    rc = posix_memalign((void**)&p_disk_a, 0x1000, 
			sizeof(struct afu_alloc) * p_ini->nelm);
    if (rc != 0) {
	trace_1(LOG_ERR, "cannot allocate AFU structs, rc %d\n", rc);
	exit(-1);
    }
    gb.p_disk_a = p_disk_a;

    p_elm = &p_ini->elm[0];
    for (i = 0; i < p_ini->nelm; i++) {
        p_disk = &p_disk_a[i].disk;
        rc = master_init(p_disk, p_elm);
        if (rc != 0) {
            trace_2(LOG_ERR, "error instantiating afu %s, rc %d\n", 
                    p_elm->afu_dev_pathm, rc);
            exit(-1);
        }

        // advance to next element using correct size
        p_elm = (struct capikv_ini_elm*) (((char*)p_elm) + p_ini->size);
    }

    /* When all went well, start IPC threads to take requests */
    for (i = 0; i < p_ini->nelm; i++) {
	pthread_create(&p_disk_a[i].disk.ipc_thread, NULL, afu_ipc_rx, 
		       &p_disk_a[i].disk);
    }




    for (i = 0; i < p_ini->nelm; i++) {
	pthread_join(p_disk_a[i].disk.ipc_thread, NULL);
	cusfs_serv_disk_term(&p_disk_a[i].disk);
    }




    closelog();
 
    return 0;
}
