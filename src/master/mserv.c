/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/mserv.c $                                          */
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

#include <mserv.h>
#include <block_alloc.h>
#include <cblk.h>
#include <afu_fc.h>
#include <prov.h>

#ifndef _AIX
#include <revtags.h>
REVISION_TAGS(mserv);
#endif

// LOG_ERR(3), LOG_WARNING(4), LOG_NOTICE(5), LOG_INFO(6) & LOG_DEBUG(7)
//
// TRACE_x versions: fmt must be of the form: "%s: xxxx" - the
// leading %s is the afu name
//
// trace_x versions: used when the log is not specific to 
// a particular AFU or the AFU name is not available
//
#define TRACE_0(lvl, p_afu, fmt) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name); }

#define TRACE_1(lvl, p_afu, fmt, A) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name, A); }

#define TRACE_2(lvl, p_afu, fmt, A, B) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name, A, B); }

#define TRACE_3(lvl, p_afu, fmt, A, B, C) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name, A, B, C); }

#define TRACE_4(lvl, p_afu, fmt, A, B, C, D)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name, A, B, C, D); }

#define TRACE_5(lvl, p_afu, fmt, A, B, C, D, E)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, (p_afu)->name, A, B, C, D, E); }

// these do not have a afu name
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
 * 3. field AFU wide errors that cannot be attributed to a user context
 *
 * There are several key functions that are not provided by the master:
 *
 * 1. load balancing across AFUs
 * 2. handle user errors including SCSI errors
 * 3. persistence of resources & LBA allocation across reboots/restarts
 * 4. recovery from an AFU reset. All users and the master must restart 
 *    if the AFU had to be reset.
 *
 *
 * The master does not have the most up to date information on who
 * owns a particular AFU context at any time (the kernel driver does).
 * It must also work with misbehaving applications that close the AFU
 * context while leaving the registration with the master open. The
 * AFU provides HW support to deal with such scenarios by automatically
 * clearing certain states during context establishment and/or termination.
 *
 * A single instance of the master serves an AFU set. The AFU set is any
 * number of AFUs that are connected to the same storage and share the
 * same LUN(s) for use by the master to virtualize on. Each AFU in the
 * set must have the same volumes (SCSI LUNs) mapped in. However, there is
 * no requirement that they be mapped using the same LUN_ID even though
 * that is the recommended configuration.
 *
 * The master will run when some AFUs in the set are not functional as
 * long as each of the remaining functional AFUs see the same LUNs that 
 * the master is configured to use.
 *
 ***************************************************************************/

typedef struct {
    struct capikv_ini *p_ini;
    struct afu_alloc *p_afu_a;
    timer_t timer_hb;
    timer_t timer_fc;
} global_t;

global_t gb;  /* mserv globals */


conn_info_t*
alloc_connection(afu_t *p_afu, int fd)
{
    int i;

    TRACE_0(LOG_INFO, p_afu,
	    "%s: allocating a new connection\n");

    for (i = 0; i < MAX_CONNS; i++) {
	if (p_afu->conn_tbl[i].fd == -1) { /* free entry */
	    p_afu->conn_tbl[i].fd = fd;
	    p_afu->conn_tbl[i].rx = rx_mcreg;
	    break;
	}
    }

    return (i == MAX_CONNS) ? NULL : &p_afu->conn_tbl[i];
}

void
free_connection(afu_t *p_afu, conn_info_t *p_conn_info)
{
    TRACE_1(LOG_INFO, p_afu, "%s: freeing connection %p\n", 
	    p_conn_info);

    p_conn_info->fd = -1;
}



int afu_init(afu_t *p_afu, struct capikv_ini_elm *p_elm)
{
    int i, j;
    int rc;
    __u64 reg;
    __u32 proc_elem;
    void *map;
    char version[16];
    mode_t oldmask;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    struct sigevent sigev;
#ifndef _AIX
    struct epoll_event     epoll_event;
#endif /* !_AIX */
    enum undo_level level = UNDO_NONE;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

    memset(p_afu, 0, sizeof(*p_afu));

    strncpy(p_afu->master_dev_path, p_elm->afu_dev_pathm, MC_PATHLEN - 1);
    p_afu->name = strrchr(p_afu->master_dev_path, '/') + 1;
    pthread_mutex_init(&p_afu->mutex, &mattr);
    pthread_mutex_init(&p_afu->err_mutex, &mattr);
    pthread_cond_init(&p_afu->err_cv, &cattr);

    for (i = 0; i < MAX_CONNS; i++) {
	p_afu->conn_tbl[i].fd = -1;
    }
    for (i = 0; i < MAX_CONTEXT; i++) {
	p_afu->rht_info[i].rht_start = &p_afu->rht[i][0];
    }

    // keep AFU accessed data in RAM as much as possible
    mlock(p_afu->rht, sizeof(p_afu->rht));
    level = UNDO_MLOCK;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGRTMIN; /* must use a queued signal */

    for (i = 0; i < NUM_CMDS; i++) {
	pthread_mutex_init(&p_afu->cmd[i].mutex, &mattr);
	pthread_cond_init(&p_afu->cmd[i].cv, &cattr);

	sigev.sigev_value.sival_ptr = &p_afu->cmd[i];
	rc = timer_create(CLOCK_REALTIME, &sigev, &p_afu->cmd[i].timer);
	if (rc != 0) {
	    TRACE_1(LOG_ERR, p_afu, "%s: timer_create failed, errno %d\n", errno);
	    for (j = 0; j < i; j++) {
		timer_delete(p_afu->cmd[j].timer);
	    }
	    undo_afu_init(p_afu, level);
	    return -1;
	}
    }
    level = UNDO_TIMER;

    pthread_condattr_destroy(&cattr);
    pthread_mutexattr_destroy(&mattr);

    // open master device
    p_afu->afu_fd = open(p_afu->master_dev_path, O_RDWR);
    if (p_afu->afu_fd < 0) {
	TRACE_1(LOG_ERR, p_afu, "%s: open failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    level = UNDO_AFU_OPEN;

    // enable the AFU. This must be done before mmap.
    p_afu->work.num_interrupts = 4;
    p_afu->work.flags = CXL_START_WORK_NUM_IRQS;
    if (ioctl(p_afu->afu_fd, CXL_IOCTL_START_WORK, &p_afu->work) != 0) {
	TRACE_1(LOG_ERR, p_afu, "%s: START_WORK failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    if (ioctl(p_afu->afu_fd, CXL_IOCTL_GET_PROCESS_ELEMENT, 
	      &proc_elem) != 0) {
	TRACE_1(LOG_ERR, p_afu, "%s: GET_PROCESS_ELEMENT failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    level = UNDO_AFU_START;

    // mmap entire MMIO space of AFU
    map = mmap(NULL, sizeof(struct surelock_afu_map),
	       PROT_READ|PROT_WRITE, MAP_SHARED, p_afu->afu_fd, 0);
    if (map == MAP_FAILED) {
	TRACE_1(LOG_ERR, p_afu, "%s: mmap failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    p_afu->p_afu_map = (volatile struct surelock_afu_map *) map;

    for (i = 0; i < MAX_CONTEXT; i++) {
	p_afu->ctx_info[i].p_ctrl_map = &p_afu->p_afu_map->ctrls[i].ctrl;

	// disrupt any clients that could be running 
	// e. g. clients that survived a master restart
	write_64(&p_afu->ctx_info[i].p_ctrl_map->rht_start, 0);
	write_64(&p_afu->ctx_info[i].p_ctrl_map->rht_cnt_id, 0);
	write_64(&p_afu->ctx_info[i].p_ctrl_map->ctx_cap, 0);
    }
    level = UNDO_AFU_MMAP;

    // copy frequently used fields into p_afu 
    p_afu->ctx_hndl =  (__u16)proc_elem; // ctx_hndl is 16 bits in CAIA
    p_afu->p_host_map = &p_afu->p_afu_map->hosts[p_afu->ctx_hndl].host;
    p_afu->p_ctrl_map = &p_afu->p_afu_map->ctrls[p_afu->ctx_hndl].ctrl;

    // initialize RRQ pointers
    p_afu->p_hrrq_start = &p_afu->rrq_entry[0];
    p_afu->p_hrrq_end = &p_afu->rrq_entry[NUM_RRQ_ENTRY - 1];
    p_afu->p_hrrq_curr = p_afu->p_hrrq_start;
    p_afu->toggle = 1;

    memset(&version[0], 0, sizeof(version));
    // don't byte reverse on reading afu_version, else the string form
    // will be backwards
    reg = p_afu->p_afu_map->global.regs.afu_version;
    memcpy(&version[0], &reg, 8);
    TRACE_2(LOG_NOTICE, p_afu, "%s: afu version %s, ctx_hndl %d\n", 
	    version, p_afu->ctx_hndl);

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++) {
	p_afu->cmd[i].rcb.ctx_id = p_afu->ctx_hndl;
	p_afu->cmd[i].rcb.msi = SISL_MSI_RRQ_UPDATED;
	p_afu->cmd[i].rcb.rrq = 0x0;
    }

    // set up RRQ in AFU for master issued cmds
    write_64(&p_afu->p_host_map->rrq_start, (__u64) p_afu->p_hrrq_start);
    write_64(&p_afu->p_host_map->rrq_end, (__u64) p_afu->p_hrrq_end);

    // AFU configuration
    reg = read_64(&p_afu->p_afu_map->global.regs.afu_config);
    reg |= 0x7F00;        // enable auto retry
    // leave others at default: 
    // CTX_CAP write protected, mbox_r does not clear on read and
    // checker on if dual afu
    write_64(&p_afu->p_afu_map->global.regs.afu_config, reg);

    // global port select: select either port
    write_64(&p_afu->p_afu_map->global.regs.afu_port_sel, 0x3);

    for (i = 0; i < NUM_FC_PORTS; i++) {
	// program FC_PORT LUN Tbl
	write_64(&p_afu->p_afu_map->global.fc_port[i][0], p_elm->lun_id);

	// unmask all errors (but they are still masked at AFU)
	write_64(&p_afu->p_afu_map->global.fc_regs[i][FC_ERRMSK/8], 0);

	// clear CRC error cnt & set a threshold
	(void) read_64(&p_afu->p_afu_map->global.fc_regs[i][FC_CNT_CRCERR/8]);
	write_64(&p_afu->p_afu_map->global.fc_regs[i][FC_CRC_THRESH/8], 
		 MC_CRC_THRESH);

	// set WWPNs. If already programmed, p_elm->wwpn[i] is 0
	if (p_elm->wwpn[i] != 0 &&
	    afu_set_wwpn(p_afu, i, &p_afu->p_afu_map->global.fc_regs[i][0],
			 p_elm->wwpn[i])) {
	    TRACE_1(LOG_ERR, p_afu, "%s: failed to set WWPN on port %d\n", i);
	    undo_afu_init(p_afu, level);
	    return -1;
	}

	// record the lun_id to be used in discovery later
	p_afu->lun_info[i].lun_id = p_elm->lun_id;
    }

    // set up master's own CTX_CAP to allow real mode, host translation
    // tbls, afu cmds and non-read/write GSCSI cmds.
    // First, unlock ctx_cap write by reading mbox
    //
    (void) read_64(&p_afu->p_ctrl_map->mbox_r); // unlock ctx_cap
    asm volatile ( "eieio" : : );
    write_64(&p_afu->p_ctrl_map->ctx_cap, 
	     SISL_CTX_CAP_REAL_MODE | SISL_CTX_CAP_HOST_XLATE |
	     SISL_CTX_CAP_AFU_CMD | SISL_CTX_CAP_GSCSI_CMD);

    // init heartbeat
    p_afu->hb = read_64(&p_afu->p_afu_map->global.regs.afu_hb);

    // Create a socket to be used for listening to connections
    p_afu->listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (p_afu->listen_fd < 0) {
	TRACE_1(LOG_ERR, p_afu, "%s: socket failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    level = UNDO_OPEN_SOCK;

    // Bind the socket to the file
    bzero(&p_afu->svr_addr, sizeof(struct sockaddr_un));
    p_afu->svr_addr.sun_family      = AF_UNIX;
    strcpy(p_afu->svr_addr.sun_path, MC_SOCKET_DIR);
    strcat(p_afu->svr_addr.sun_path, p_afu->master_dev_path);
    mkdir_p(p_afu->svr_addr.sun_path); // make intermediate directories
    unlink(p_afu->svr_addr.sun_path);
    // create socket with rwx for group but no perms for others
    oldmask = umask(007); 
    rc = bind(p_afu->listen_fd, (struct sockaddr *)&p_afu->svr_addr, 
	      sizeof(p_afu->svr_addr));
    umask(oldmask); // set umask back to default
    if (rc) {
	TRACE_2(LOG_ERR, p_afu, "%s: bind failed, rc %d, errno %d\n", rc, errno);
	undo_afu_init(p_afu, level);
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
    p_afu->epfd = epoll_create(MAX_CONN_TO_POLL);
    if (p_afu->epfd == -1) {
	TRACE_1(LOG_ERR, p_afu, "%s: epoll_create failed, errno %d\n", errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    level = UNDO_EPOLL_CREATE;

    // Add the listening file descriptor to the epoll array
    p_afu->conn_tbl[0].fd = p_afu->listen_fd;
    memset(&epoll_event, 0, sizeof(struct epoll_event));
    epoll_event.events = EPOLLIN;
    epoll_event.data.ptr = &p_afu->conn_tbl[0];
    rc = epoll_ctl(p_afu->epfd, EPOLL_CTL_ADD, p_afu->listen_fd, &epoll_event);
    if (rc) {
	TRACE_2(LOG_ERR, p_afu, "%s: epoll_ctl failed for ADD, rc %d, errno %d\n", 
		rc, errno);
	undo_afu_init(p_afu, level);
	return -1;
    }
    level = UNDO_EPOLL_ADD;
#else

    memset(p_afu->events,0,sizeof(struct pollfd) * MAX_CONN_TO_POLL);

    // Add the listening file descriptor to the epoll array
    p_afu->conn_tbl[0].fd = p_afu->listen_fd;
    p_afu->events[0].fd = p_afu->listen_fd;
    p_afu->events[0].events = POLLIN|POLLMSG;
    p_afu->num_poll_events++;
#endif /* !_AIX */

    return 0;
}

void undo_afu_init(afu_t *p_afu, enum undo_level level)
{
    int i;

    switch(level) 
    {
    case UNDO_AFU_ALL:
    case UNDO_EPOLL_ADD:
    case UNDO_EPOLL_CREATE:
#ifndef _AIX
	close(p_afu->epfd);
#endif
    case UNDO_BIND_SOCK:
	unlink(p_afu->svr_addr.sun_path);
    case UNDO_OPEN_SOCK:
	close(p_afu->listen_fd);
    case UNDO_AFU_MMAP:
	munmap((void*)p_afu->p_afu_map, sizeof(struct surelock_afu_map));

    case UNDO_AFU_START:	
    case UNDO_AFU_OPEN:	
	close(p_afu->afu_fd);
    case UNDO_TIMER:
	for (i = 0; i < NUM_CMDS; i++) {
	    timer_delete(p_afu->cmd[i].timer);
	}
    case UNDO_MLOCK:
	munlock(p_afu->rht, sizeof(p_afu->rht));
    default:
	break;
    }
}


int afu_term(afu_t *p_afu)
{
    undo_afu_init(p_afu, UNDO_AFU_ALL);

    return 0;
}

void afu_err_intr_init(afu_t *p_afu)
{
    int i;

    /* global async interrupts: AFU clears afu_ctrl on context exit
       if async interrupts were sent to that context. This prevents
       the AFU form sending further async interrupts when there is
       nobody to receive them.
    */
    // mask all
    write_64(&p_afu->p_afu_map->global.regs.aintr_mask, -1ull);
    // set LISN# to send and point to master context
    write_64(&p_afu->p_afu_map->global.regs.afu_ctrl,
	     ((__u64)((p_afu->ctx_hndl << 8) | SISL_MSI_ASYNC_ERROR)) << 40);
    // clear all
    write_64(&p_afu->p_afu_map->global.regs.aintr_clear, -1ull);
    // unmask bits that are of interest 
    // note: afu can send an interrupt after this step
    write_64(&p_afu->p_afu_map->global.regs.aintr_mask, SISL_ASTATUS_MASK);
    // clear again in case a bit came on after previous clear but before unmask
    write_64(&p_afu->p_afu_map->global.regs.aintr_clear, -1ull);

    // now clear FC errors
    for (i = 0; i < NUM_FC_PORTS; i++) {
	write_64(&p_afu->p_afu_map->global.fc_regs[i][FC_ERROR/8], (__u32)-1);
	write_64(&p_afu->p_afu_map->global.fc_regs[i][FC_ERRCAP/8], 0);
    }


    // sync interrupts for master's IOARRIN write
    // note that unlike asyncs, there can be no pending sync interrupts 
    // at this time (this is a fresh context and master has not written 
    // IOARRIN yet), so there is nothing to clear.
    // 
    // set LISN#, it is always sent to the context that wrote IOARRIN
    write_64(&p_afu->p_host_map->ctx_ctrl, SISL_MSI_SYNC_ERROR);
    write_64(&p_afu->p_host_map->intr_mask, SISL_ISTATUS_MASK);
}

// online means the FC link layer has sync and has completed the link layer
// handshake. It is ready for login to start.
void set_port_online(volatile __u64 *p_fc_regs)
{
    __u64 cmdcfg;

    cmdcfg = read_64(&p_fc_regs[FC_MTIP_CMDCONFIG/8]);
    cmdcfg &= (~FC_MTIP_CMDCONFIG_OFFLINE);  // clear OFF_LINE
    cmdcfg |= (FC_MTIP_CMDCONFIG_ONLINE); // set ON_LINE
    write_64(&p_fc_regs[FC_MTIP_CMDCONFIG/8], cmdcfg);
}

void set_port_offline(volatile __u64 *p_fc_regs)
{
    __u64 cmdcfg;

    cmdcfg = read_64(&p_fc_regs[FC_MTIP_CMDCONFIG/8]);
    cmdcfg &= (~FC_MTIP_CMDCONFIG_ONLINE); // clear ON_LINE
    cmdcfg |= (FC_MTIP_CMDCONFIG_OFFLINE);  // set OFF_LINE
    write_64(&p_fc_regs[FC_MTIP_CMDCONFIG/8], cmdcfg);
}

// returns 1 - went online
// wait_port_xxx will timeout when cable is not pluggd in
int wait_port_online(volatile __u64 *p_fc_regs,
		     useconds_t delay_us,
		     unsigned int nretry)
{
    __u64 status;

    do {
	usleep(delay_us);
	status = read_64(&p_fc_regs[FC_MTIP_STATUS/8]);
    } while ((status & FC_MTIP_STATUS_MASK) != FC_MTIP_STATUS_ONLINE &&
	     nretry--);

    return ((status & FC_MTIP_STATUS_MASK) == FC_MTIP_STATUS_ONLINE);
}

// returns 1 - went offline
int wait_port_offline(volatile __u64 *p_fc_regs,
		      useconds_t delay_us,
		      unsigned int nretry)
{
    __u64 status;

    do {
	usleep(delay_us);
	status = read_64(&p_fc_regs[FC_MTIP_STATUS/8]);
    } while ((status & FC_MTIP_STATUS_MASK) != FC_MTIP_STATUS_OFFLINE &&
	     nretry--);

    return ((status & FC_MTIP_STATUS_MASK) == FC_MTIP_STATUS_OFFLINE);
}

// this function can block up to a few seconds
int afu_set_wwpn(afu_t *p_afu, int port, volatile __u64 *p_fc_regs,
		 __u64 wwpn)
{
    int ret = 0;
    
    set_port_offline(p_fc_regs);

    if (!wait_port_offline(p_fc_regs, FC_PORT_STATUS_RETRY_INTERVAL_US,
			   FC_PORT_STATUS_RETRY_CNT)) {
	TRACE_1(LOG_ERR, p_afu, "%s: wait on port %d to go offline timed out\n", 
		port);
	ret = -1; // but continue on to leave the port back online
    }

    if (ret == 0) {
	write_64(&p_fc_regs[FC_PNAME/8], wwpn);
    }

    set_port_online(p_fc_regs);

    if (!wait_port_online(p_fc_regs, FC_PORT_STATUS_RETRY_INTERVAL_US,
			  FC_PORT_STATUS_RETRY_CNT)) {
	TRACE_1(LOG_ERR, p_afu, "%s: wait on port %d to go online timed out\n", 
		port);
	ret = -1;
    }

    return ret;
}

// this function can block up to a few seconds
void afu_link_reset(afu_t *p_afu, int port, volatile __u64 *p_fc_regs)
{
    __u64 port_sel;

    // first switch the AFU to the other links, if any
    port_sel = read_64(&p_afu->p_afu_map->global.regs.afu_port_sel);
    port_sel &= ~(1 << port);
    write_64(&p_afu->p_afu_map->global.regs.afu_port_sel, port_sel);
    afu_sync(p_afu, 0, 0, AFU_GSYNC);

    set_port_offline(p_fc_regs);
    if (!wait_port_offline(p_fc_regs, FC_PORT_STATUS_RETRY_INTERVAL_US,
			   FC_PORT_STATUS_RETRY_CNT)) {
	TRACE_1(LOG_ERR, p_afu, "%s: wait on port %d to go offline timed out\n", 
		port);
    }

    set_port_online(p_fc_regs);
    if (!wait_port_online(p_fc_regs, FC_PORT_STATUS_RETRY_INTERVAL_US,
			  FC_PORT_STATUS_RETRY_CNT)) {
	TRACE_1(LOG_ERR, p_afu, "%s: wait on port %d to go online timed out\n", 
		port);
    }

    // switch back to include this port
    port_sel |= (1 << port);
    write_64(&p_afu->p_afu_map->global.regs.afu_port_sel, port_sel);
    afu_sync(p_afu, 0, 0, AFU_GSYNC);

}


// all do_xxx functions return 0 or an errno value
//

/*
 * NAME:        do_mc_register
 *
 * FUNCTION:    Register a user AFU context with master.
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
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
 *               b. Sets RHT_START & RHT_CNT registers for the 
 *                  registered context
 *               c. Clears all RHT entries effectively making 
 *                  all resource handles invalid.
 *               d. goes to rx_ready state
 *
 * If registation fails, stay in rx_mcreg state to allow client retries.
 */
int
do_mc_register(afu_t        *p_afu, 
	       conn_info_t  *p_conn_info,
	       __u64        challenge)
{
    ctx_info_t *p_ctx_info;
    __u64 reg;
    int i;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (p_conn_info->ctx_hndl < MAX_CONTEXT) {
	p_ctx_info = &p_afu->ctx_info[p_conn_info->ctx_hndl];

	/* This code reads the mbox w/o knowing if the requester is the
	   true owner of the context it wants to register. The read has
	   no side effect and does not affect the true owner if this is
	   a fraudulent registration attempt.
	*/
	reg = read_64(&p_ctx_info->p_ctrl_map->mbox_r);

	if (reg == 0 || /* zeroed mbox is a locked mbox */
	    challenge != reg) {
 	    return EACCES; /* return Permission denied */
	}

	if (p_conn_info->mode == MCREG_DUP_REG &&
	    p_ctx_info->ref_cnt == 0) {
	    return EINVAL; /* no prior registration to dup */
	}

	/* a fresh registration will cause all previous registrations,
	   if any, to be forcefully canceled. This is important since 
	   a client can close the context (AFU) but not unregister the 
	   mc_handle. A new owner of the same context must be able to 
	   mc_register by forcefully unregistering the previous owner.
	*/
	if (p_conn_info->mode == MCREG_INITIAL_REG) {
	    for (i = 0; i < MAX_CONNS; i++) {
		if (p_afu->conn_tbl[i].p_ctx_info == p_ctx_info) {
		    do_mc_unregister(p_afu, &p_afu->conn_tbl[i]);
		}
	    }

	    if (p_ctx_info->ref_cnt != 0) {
		TRACE_0(LOG_ERR, p_afu, 
			"%s: internal error: p_ctx_info->ref_cnt != 0");
	    }

	    /* This context is not duped and is in a group by itself. */
	    p_ctx_info->p_next = p_ctx_info;
	    p_ctx_info->p_forw = p_ctx_info;

	    /* restrict user to read/write cmds in translated mode.
	       User has option to choose read and/or write permissions
	       again in mc_open.
	     */
	    write_64(&p_ctx_info->p_ctrl_map->ctx_cap,
		     SISL_CTX_CAP_READ_CMD | SISL_CTX_CAP_WRITE_CMD);
	    asm volatile ( "eieio" : : ); 
	    reg = read_64(&p_ctx_info->p_ctrl_map->ctx_cap);
	    
	    /* if the write failed, the ctx must have been closed since
	       the mbox read and the ctx_cap register locked up.
	       fail the registration */
	    if (reg != (SISL_CTX_CAP_READ_CMD | SISL_CTX_CAP_WRITE_CMD)) {
		return EAGAIN;
	    }

	    /* the context gets a dedicated RHT tbl unless it is dup'ed
	       later. */
	    p_ctx_info->p_rht_info = &p_afu->rht_info[p_conn_info->ctx_hndl];
	    p_ctx_info->p_rht_info->ref_cnt = 1;
	    memset(p_ctx_info->p_rht_info->rht_start, 0, 
		   sizeof(sisl_rht_entry_t)*MAX_RHT_PER_CONTEXT);
	    /* make clearing of the RHT visible to AFU before MMIO */
	    asm volatile ( "lwsync" : : ); 

	    /* set up MMIO registers pointing to the RHT */
	    write_64(&p_ctx_info->p_ctrl_map->rht_start,
		     (__u64)p_ctx_info->p_rht_info->rht_start);
	    write_64(&p_ctx_info->p_ctrl_map->rht_cnt_id,
		     SISL_RHT_CNT_ID((__u64)MAX_RHT_PER_CONTEXT,  
				     (__u64)(p_afu->ctx_hndl)));
	}

	p_conn_info->p_ctx_info = p_ctx_info;
	p_ctx_info->ref_cnt++;
	p_conn_info->rx = rx_ready; /* it is now registered, go to ready state */
	return 0;
    }
    else {
	return EINVAL;
    }
}

/*
 * NAME:        do_mc_unregister
 *
 * FUNCTION:    Unregister a user AFU context with master.
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
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
do_mc_unregister(afu_t        *p_afu, 
		 conn_info_t  *p_conn_info)
{
    int i;
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (p_ctx_info->ref_cnt-- == 1) { /* close the context */
	/* for any resource still open, dealloate LBAs and close if 
	   nobody else is using it. */
	if (p_ctx_info->p_rht_info->ref_cnt-- == 1) {
	    for (i = 0; i < MAX_RHT_PER_CONTEXT; i++) {
		do_mc_close(p_afu, p_conn_info, i); // will this p_conn_info work ?
	    }
	}

	/* clear RHT registers for this context */
	write_64(&p_ctx_info->p_ctrl_map->rht_start, 0);
	write_64(&p_ctx_info->p_ctrl_map->rht_cnt_id, 0);

	/* drop all capabilities */
	write_64(&p_ctx_info->p_ctrl_map->ctx_cap, 0);
    }

    p_conn_info->p_ctx_info = NULL;
    p_conn_info->rx = rx_mcreg; /* client can now send another MCREG */
    
    return 0;
} 




/*
 * NAME:        do_mc_open
 *
 * FUNCTION:    Create a virtual LBA space of 0 size
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
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
do_mc_open(afu_t        *p_afu, 
	   conn_info_t  *p_conn_info, 
	   __u64        flags,
	   res_hndl_t   *p_res_hndl)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;
    sisl_rht_entry_t *p_rht_entry = NULL;
    int i;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    /* find a free RHT entry */
    for (i = 0; i <  MAX_RHT_PER_CONTEXT; i++) {
	if (p_rht_info->rht_start[i].nmask == 0) {
	    p_rht_entry = &p_rht_info->rht_start[i];
	    break;
	}
    }

    /* if we did not find a free entry, reached max opens allowed per
       context */
    if (p_rht_entry == NULL) {
	return EMFILE; // too many mc_opens
    }

    p_rht_entry->nmask = MC_RHT_NMASK;
    p_rht_entry->fp = SISL_RHT_FP(0u, flags & 0x3); /* format 0 & perms */
    *p_res_hndl = (p_rht_entry - p_rht_info->rht_start);

    return 0;
} 


/*
 * NAME:        do_mc_close
 *
 * FUNCTION:    Close a virtual LBA space setting it to 0 size and
 *              marking the res_hndl as free/closed.
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
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
do_mc_close(afu_t        *p_afu, 
	    conn_info_t  *p_conn_info,
	    res_hndl_t   res_hndl)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;
    sisl_rht_entry_t *p_rht_entry;
    __u64 act_new_size;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (res_hndl < MAX_RHT_PER_CONTEXT) {
	p_rht_entry = &p_rht_info->rht_start[res_hndl];

	if (p_rht_entry->nmask == 0) { /* not open */
	    return EINVAL;
	}

	/* set size to 0, this will clear LXT_START and LXT_CNT fields
	   in the RHT entry */
	do_mc_size(p_afu, p_conn_info, res_hndl, 0u, &act_new_size); // p_conn good ?

	p_rht_entry->nmask = 0;
	p_rht_entry->fp = 0;

	/* now the RHT entry is all cleared */
    }
    else {
	return EINVAL;
    }

    return 0;
}


/*
 * NAME:        do_mc_size
 *
 * FUNCTION:    Resize a resource handle by changing the RHT entry and LXT
 *              Tbl it points to. Synchronize all contexts that refer to
 *              the RHT.
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              res_hndl    - resource handle to resize
 *              new_size    - new size in chunks
 *
 * OUTPUTS:
 *              p_act_new_size   - pointer to actual new size in chunks
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 *              Setting new_size=0 will clear LXT_START and LXT_CNT fields
 *              in the RHT entry.
 */
int
do_mc_size(afu_t        *p_afu, 
	   conn_info_t  *p_conn_info,
	   res_hndl_t   res_hndl,
	   __u64        new_size,
	   __u64        *p_act_new_size)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;
    sisl_rht_entry_t *p_rht_entry;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (res_hndl < MAX_RHT_PER_CONTEXT) {
	p_rht_entry = &p_rht_info->rht_start[res_hndl];

	if (p_rht_entry->nmask == 0) { /* not open */
	    return EINVAL;
	}

	if (new_size > p_rht_entry->lxt_cnt) {
	    grow_lxt(p_afu, 
		     p_conn_info->ctx_hndl,
		     res_hndl,
		     p_rht_entry,
		     new_size - p_rht_entry->lxt_cnt,
		     p_act_new_size);

	}
	else if (new_size < p_rht_entry->lxt_cnt) {
	    shrink_lxt(p_afu, 
		       p_conn_info->ctx_hndl,
		       res_hndl,
		       p_rht_entry,
		       p_rht_entry->lxt_cnt - new_size,
		       p_act_new_size);
	}
	else {
	    *p_act_new_size = new_size;
	    return 0;
	}
    }
    else {
	return EINVAL;
    }

    return 0;
} 

int
grow_lxt(afu_t            *p_afu, 
	 ctx_hndl_t       ctx_hndl_u,
	 res_hndl_t       res_hndl_u,
	 sisl_rht_entry_t *p_rht_entry,
	 __u64            delta,
	 __u64            *p_act_new_size)
{
    sisl_lxt_entry_t *p_lxt, *p_lxt_old;
    unsigned int av_size;
    unsigned int ngrps, ngrps_old;
    aun_t aun; /* chunk# allocated by block allocator */
    int i;

    /* check what is available in the block allocator before re-allocating
       LXT array. This is done up front under the mutex which must not be
       released until after allocation is complete. 
    */
    pthread_mutex_lock(&p_afu->p_blka->mutex);
    av_size = ba_space(&p_afu->p_blka->ba_lun);
    if (av_size < delta) {
	delta = av_size;
    }

    p_lxt_old = p_rht_entry->lxt_start;
    ngrps_old = LXT_NUM_GROUPS(p_rht_entry->lxt_cnt);
    ngrps = LXT_NUM_GROUPS(p_rht_entry->lxt_cnt + delta);

    if (ngrps != ngrps_old) {
	/* realloate to fit new size */
	p_lxt = (sisl_lxt_entry_t  *) malloc(sizeof(sisl_lxt_entry_t) *
					     LXT_GROUP_SIZE * ngrps);
	if (p_lxt == NULL) {
	    pthread_mutex_unlock(&p_afu->p_blka->mutex);
	    return ENOMEM;
	}
	/* copy over all old entries */
	memcpy(p_lxt, p_lxt_old, 
	       sizeof(sisl_lxt_entry_t)*p_rht_entry->lxt_cnt);
    }
    else {
	p_lxt = p_lxt_old;
    }

    /* nothing can fail from now on */
    *p_act_new_size = p_rht_entry->lxt_cnt + delta;

    /* add new entries to the end */
    for (i = p_rht_entry->lxt_cnt; i < *p_act_new_size; i++) {
	/* Due to the earlier check of available space, ba_alloc cannot
	   fail here. If it did due to internal error, leave a rlba_base
	   of -1u which will likely be a invalid LUN (too large).
	*/
	aun = ba_alloc(&p_afu->p_blka->ba_lun);
	if (aun == (aun_t) -1 ||
	    aun >= p_afu->p_blka->nchunk) {
	    TRACE_2(LOG_ERR, p_afu, 
		    "%s: ba_alloc error: allocated chunk# 0x%lx, max 0x%lx",
		    aun, p_afu->p_blka->nchunk - 1);
	}

	/* lun_indx = 0, select both ports, use r/w perms from RHT */
	p_lxt[i].rlba_base = ((aun << MC_CHUNK_SHIFT) | 0x33); 
    }

    pthread_mutex_unlock(&p_afu->p_blka->mutex);

    asm volatile ( "lwsync" : : );  /* make lxt updates visible */

    /* Now sync up AFU - this can take a while */
    p_rht_entry->lxt_start = p_lxt; /* even if p_lxt didn't change */
    asm volatile ( "lwsync" : : ); 

    p_rht_entry->lxt_cnt = *p_act_new_size;
    asm volatile ( "lwsync" : : ); 

    afu_sync(p_afu, ctx_hndl_u, res_hndl_u, AFU_LW_SYNC);

    /* free old lxt if reallocated */
    if (p_lxt != p_lxt_old) {
	free(p_lxt_old);
    }
    // sync up AFU on each context in the doubly linked list
    return 0;
} 

int
shrink_lxt(afu_t            *p_afu, 
	   ctx_hndl_t       ctx_hndl_u,
	   res_hndl_t       res_hndl_u,
	   sisl_rht_entry_t *p_rht_entry,
	   __u64            delta,
	   __u64            *p_act_new_size)
{
    sisl_lxt_entry_t *p_lxt, *p_lxt_old;
    unsigned int ngrps, ngrps_old;
    aun_t aun; /* chunk# allocated by block allocator */
    int i;

    p_lxt_old = p_rht_entry->lxt_start;
    ngrps_old = LXT_NUM_GROUPS(p_rht_entry->lxt_cnt);
    ngrps = LXT_NUM_GROUPS(p_rht_entry->lxt_cnt - delta);

    if (ngrps != ngrps_old) {
	/* realloate to fit new size unless new size is 0 */
	if (ngrps) {
	    p_lxt = (sisl_lxt_entry_t  *) malloc(sizeof(sisl_lxt_entry_t) *
						 LXT_GROUP_SIZE * ngrps);
	    if (p_lxt == NULL) {
		return ENOMEM;
	    }
	    /* copy over old entries that will remain */
	    memcpy(p_lxt, p_lxt_old, 
		   sizeof(sisl_lxt_entry_t)*(p_rht_entry->lxt_cnt - delta));
	}
	else {
	    p_lxt = NULL;
	}
    }
    else {
	p_lxt = p_lxt_old;
    }

    /* nothing can fail from now on */
    *p_act_new_size = p_rht_entry->lxt_cnt - delta;

    /* Now sync up AFU - this can take a while */
    p_rht_entry->lxt_cnt = *p_act_new_size;
    asm volatile ( "lwsync" : : ); /* also makes lxt updates visible */

    p_rht_entry->lxt_start = p_lxt; /* even if p_lxt didn't change */
    asm volatile ( "lwsync" : : ); 

    afu_sync(p_afu, ctx_hndl_u, res_hndl_u, AFU_HW_SYNC);

    /* free LBAs allocated to freed chunks */
    pthread_mutex_lock(&p_afu->p_blka->mutex);
    for (i = delta - 1; i >= 0; i--) {
	aun = (p_lxt_old[*p_act_new_size + i].rlba_base >> MC_CHUNK_SHIFT);
	ba_free(&p_afu->p_blka->ba_lun, aun);
    }
    pthread_mutex_unlock(&p_afu->p_blka->mutex);

    /* free old lxt if reallocated */
    if (p_lxt != p_lxt_old) {
	free(p_lxt_old);
    }

    // sync up AFU on each context in the doubly linked list!!!

    return 0;
} 

/*
 * NAME:        clone_lxt
 *
 * FUNCTION:    clone a LXT table
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
 *              ctx_hndl_u  - context that owns the destination LXT
 *              res_hndl_u  - res_hndl of the destination LXT
 *              p_rht_entry - destination RHT to clone into
 *              p_rht_entry_src - source RHT to clone from
 *
 * OUTPUTS:
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * NOTES:
 */
int
clone_lxt(afu_t            *p_afu, 
	  ctx_hndl_t       ctx_hndl_u,
	  res_hndl_t       res_hndl_u,
	  sisl_rht_entry_t *p_rht_entry,
	  sisl_rht_entry_t *p_rht_entry_src)
{
    sisl_lxt_entry_t *p_lxt;
    unsigned int ngrps;
    aun_t aun; /* chunk# allocated by block allocator */
    int i, j;

    ngrps = LXT_NUM_GROUPS(p_rht_entry_src->lxt_cnt);

    if (ngrps) {
	/* alloate new LXTs for clone */
	p_lxt = (sisl_lxt_entry_t  *) malloc(sizeof(sisl_lxt_entry_t) *
					     LXT_GROUP_SIZE * ngrps);
	if (p_lxt == NULL) {
	    return ENOMEM;
	}
	/* copy over */
	memcpy(p_lxt, p_rht_entry_src->lxt_start, 
	       sizeof(sisl_lxt_entry_t)*p_rht_entry_src->lxt_cnt);

	/* clone the LBAs in block allocator via ref_cnt */
	pthread_mutex_lock(&p_afu->p_blka->mutex);
	for (i = 0; i < p_rht_entry_src->lxt_cnt; i++) {
	    aun = (p_lxt[i].rlba_base >> MC_CHUNK_SHIFT);
	    if (ba_clone(&p_afu->p_blka->ba_lun, aun) == -1) {
		/* free the clones already made */
		for (j = 0; j < i; j++) {
		    aun = (p_lxt[j].rlba_base >> MC_CHUNK_SHIFT);
		    ba_free(&p_afu->p_blka->ba_lun, aun);
		}
		pthread_mutex_unlock(&p_afu->p_blka->mutex);
		free(p_lxt);
		return EIO;
	    }
	}
	pthread_mutex_unlock(&p_afu->p_blka->mutex);
    }
    else {
	p_lxt = NULL;
    }

    asm volatile ( "lwsync" : : );  /* make lxt updates visible */

    /* Now sync up AFU - this can take a while */
    p_rht_entry->lxt_start = p_lxt; /* even if p_lxt is NULL */
    asm volatile ( "lwsync" : : ); 

    p_rht_entry->lxt_cnt = p_rht_entry_src->lxt_cnt;
    asm volatile ( "lwsync" : : ); 

    afu_sync(p_afu, ctx_hndl_u, res_hndl_u, AFU_LW_SYNC);

    // sync up AFU on each context in the doubly linked list
    return 0;
}


/*
 * NAME:        do_mc_xlate_lba
 *
 * FUNCTION:    Query the physical LBA mapped to a virtual LBA
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              res_hndl    - resource handle to query on
 *              v_lba       - virtual LBA on res_hndl
 *
 * OUTPUTS:
 *              p_p_lba     - pointer to output physical LBA
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 */
int
do_mc_xlate_lba(afu_t        *p_afu, 
		conn_info_t* p_conn_info,
		res_hndl_t   res_hndl,
		__u64        v_lba,
		__u64        *p_p_lba)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;
    sisl_rht_entry_t *p_rht_entry;
    __u64 chunk_id, chunk_off, rlba_base;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (res_hndl < MAX_RHT_PER_CONTEXT) {
	p_rht_entry = &p_rht_info->rht_start[res_hndl];

	if (p_rht_entry->nmask == 0) { /* not open */
	    return EINVAL;
	}

	chunk_id = (v_lba >> MC_CHUNK_SHIFT);
	chunk_off = (v_lba & MC_CHUNK_OFF_MASK);

	if (chunk_id < p_rht_entry->lxt_cnt) {
	    rlba_base = 
		(p_rht_entry->lxt_start[chunk_id].rlba_base & (~MC_CHUNK_OFF_MASK));
	    *p_p_lba = (rlba_base | chunk_off);
	}
	else {
	    return EINVAL;
	}
    }
    else {
	return EINVAL;
    }

    return 0;
}

/*
 * NAME:        do_mc_clone
 *
 * FUNCTION:    clone by making a snapshot copy of another context
 *
 * INPUTS:
 *              p_afu        - Pointer to afu struct
 *              p_conn_info  - Pointer to connection the request came in
 *                             This is also the target of the clone.
 *              ctx_hndl_src - AFU context to clone from
 *              challenge    - used to validate access to ctx_hndl_src
 *              flags        - permissions for the cloned copy
 *
 * OUTPUTS:
 *              None
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 * clone effectively does what open and size do. The destination
 * context must be in pristine state with no resource handles open.
 *
 */
// todo dest ctx must be unduped
int
do_mc_clone(afu_t        *p_afu, 
	    conn_info_t  *p_conn_info, 
	    ctx_hndl_t   ctx_hndl_src,
	    __u64        challenge,
	    __u64        flags)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;

    ctx_info_t *p_ctx_info_src;
    rht_info_t *p_rht_info_src;
    __u64 reg;
    int i, j;
    int rc;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    /* verify there is no open resource handle in the target context
       of the clone.
    */
    for (i = 0; i <  MAX_RHT_PER_CONTEXT; i++) {
	if (p_rht_info->rht_start[i].nmask != 0) {
	    return EINVAL;
	}
    }

    /* do not clone yourself */
    if (p_conn_info->ctx_hndl == ctx_hndl_src) {
	return EINVAL;
    }

    if (ctx_hndl_src < MAX_CONTEXT) {
	p_ctx_info_src = &p_afu->ctx_info[ctx_hndl_src];
	p_rht_info_src = &p_afu->rht_info[ctx_hndl_src];
    }
    else {
	return EINVAL;
    }

    reg = read_64(&p_ctx_info_src->p_ctrl_map->mbox_r);

    if (reg == 0 || /* zeroed mbox is a locked mbox */
	challenge != reg) {
	return EACCES; /* return Permission denied */
    }

    /* this loop is equivalent to do_mc_open & do_mc_size
     * Not checking if the source context has anything open or whether
     * it is even registered.
     */
    for (i = 0; i <  MAX_RHT_PER_CONTEXT; i++) {
	p_rht_info->rht_start[i].nmask = 
	    p_rht_info_src->rht_start[i].nmask;
    	p_rht_info->rht_start[i].fp = 
	    SISL_RHT_FP_CLONE(p_rht_info_src->rht_start[i].fp, flags & 0x3);

	rc = clone_lxt(p_afu, p_conn_info->ctx_hndl, i,
		       &p_rht_info->rht_start[i],
		       &p_rht_info_src->rht_start[i]);
	if (rc != 0) {
	    for (j = 0; j < i; j++) {
		do_mc_close(p_afu, p_conn_info, j);
	    }

	    p_rht_info->rht_start[i].nmask = 0;
	    p_rht_info->rht_start[i].fp = 0;

	    return rc;
	}
    }

    return 0;
} 

/*
 * NAME:        do_mc_dup
 *
 * FUNCTION:    dup 2 contexts by linking their RHTs
 *
 * INPUTS:
 *              p_afu         - Pointer to afu struct
 *              p_conn_info   - Pointer to connection the request came in
 *                              This is the context to dup to (target)
 *              ctx_hndl_cand - This is the context to dup from source)
 *              challenge     - used to validate access to ctx_hndl_cand
 *
 * OUTPUTS:
 *              None
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 */
// dest ctx must be unduped and with no open res_hndls
int
do_mc_dup(afu_t        *p_afu, 
	  conn_info_t  *p_conn_info, 
	  ctx_hndl_t   ctx_hndl_cand,
	  __u64        challenge)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;

    ctx_info_t *p_ctx_info_cand;
    //rht_info_t *p_rht_info_cand;
    __u64 reg;
    int i; //, j;
    //int rc;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    /* verify there is no open resource handle in the target context
       of the clone.
    */
    for (i = 0; i <  MAX_RHT_PER_CONTEXT; i++) {
	if (p_rht_info->rht_start[i].nmask != 0) {
	    return EINVAL;
	}
    }

    /* do not dup yourself */
    if (p_conn_info->ctx_hndl == ctx_hndl_cand) {
	return EINVAL;
    }

    if (ctx_hndl_cand < MAX_CONTEXT) {
	p_ctx_info_cand = &p_afu->ctx_info[ctx_hndl_cand];
	//p_rht_info_cand = &p_afu->rht_info[ctx_hndl_cand];
    }
    else {
	return EINVAL;
    }

    reg = read_64(&p_ctx_info_cand->p_ctrl_map->mbox_r);

    if (reg == 0 || /* zeroed mbox is a locked mbox */
	challenge != reg) {
	return EACCES; /* return Permission denied */
    }


    return EIO; // todo later!!!
} 


/*
 * NAME:        do_mc_stat
 *
 * FUNCTION:    Query the current information on a resource handle
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              res_hndl    - resource handle to query
 *
 * OUTPUTS:
 *              p_mc_stat   - pointer to output stat information
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 */
int
do_mc_stat(afu_t        *p_afu, 
	   conn_info_t  *p_conn_info, 
	   res_hndl_t   res_hndl,
	   mc_stat_t    *p_mc_stat)
{
    ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    rht_info_t *p_rht_info = p_ctx_info->p_rht_info;
    sisl_rht_entry_t *p_rht_entry;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

    if (res_hndl < MAX_RHT_PER_CONTEXT) {
	p_rht_entry = &p_rht_info->rht_start[res_hndl];

	if (p_rht_entry->nmask == 0) { /* not open */
	    return EINVAL;
	}

	p_mc_stat->blk_len = p_afu->p_blka->ba_lun.lba_size;
	p_mc_stat->nmask = p_rht_entry->nmask;
	p_mc_stat->size = p_rht_entry->lxt_cnt;
	p_mc_stat->flags = SISL_RHT_PERM(p_rht_entry->fp);
    }
    else {
	return EINVAL;
    }

    return 0;
}

/*
 * NAME:        do_mc_notify
 *
 * FUNCTION:    Send an even to master
 *
 * INPUTS:
 *              p_afu       - Pointer to afu struct
 *              p_conn_info - Pointer to connection the request came in
 *              p_mc_stat   - pointer to input event data
 *
 * OUTPUTS:
 *
 * RETURNS:
 *              0           - Success
 *              errno       - Failure
 *
 */
int
do_mc_notify(afu_t        *p_afu, 
	   conn_info_t    *p_conn_info, 
	   mc_notify_t    *p_mc_notify)
{
    //ctx_info_t *p_ctx_info = p_conn_info->p_ctx_info;
    //rht_info_t *p_rht_info = p_ctx_info->p_rht_info;

    TRACE_4(LOG_INFO, p_afu, "%s: %s, client_pid=%d client_fd=%d ctx_hdl=%d\n",
	    __func__,
	    p_conn_info->client_pid, 
	    p_conn_info->client_fd, 
	    p_conn_info->ctx_hndl);

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
int rx_mcreg(afu_t *p_afu, conn_info_t *p_conn_info,
	     mc_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CMD_MCREG: 
	// first command on a fresh connection
	// copy input fields to connection info 
	p_conn_info->client_pid = p_req->reg.client_pid;
	p_conn_info->client_fd = p_req->reg.client_fd;
	p_conn_info->ctx_hndl = p_req->reg.ctx_hndl;
	p_conn_info->mode = p_req->reg.mode;

	status = do_mc_register(p_afu, p_conn_info, p_req->reg.challenge);

	break;

    default:
	// fail everything else
	TRACE_1(LOG_ERR, p_afu, "%s: bad command code %d in wait_mcreg state\n",
		p_req->header.command);
	status = EINVAL;
	break;
    }

    return status;
}

int rx_ready(afu_t *p_afu, conn_info_t *p_conn_info, 
	     mc_req_t *p_req, mc_resp_t *p_resp)
{
    int status;

    switch (p_req->header.command)
    {
    case CMD_MCUNREG:
	(void) do_mc_unregister(p_afu, p_conn_info);
	status = 0;
	break;

    case CMD_MCOPEN:
	status = do_mc_open(p_afu, p_conn_info, 
			    p_req->open.flags, &p_resp->open.res_hndl);
	break;

    case CMD_MCCLOSE:
	status = do_mc_close(p_afu, p_conn_info, p_req->close.res_hndl);
	break;

    case CMD_MCSIZE:
	status = do_mc_size(p_afu, p_conn_info,
			    p_req->size.res_hndl,
			    p_req->size.new_size,
			    &p_resp->size.act_new_size);
	break;

    case CMD_MCXLATE_LBA:
	status = do_mc_xlate_lba(p_afu, p_conn_info,
				 p_req->xlate_lba.res_hndl,
				 p_req->xlate_lba.v_lba,
				 &p_resp->xlate_lba.p_lba);
	break;

    case CMD_MCCLONE:
	status = do_mc_clone(p_afu, p_conn_info,
			     p_req->clone.ctx_hndl_src,
			     p_req->clone.challenge,
			     p_req->clone.flags);
	break;

    case CMD_MCDUP:
	status = do_mc_dup(p_afu, p_conn_info,
			   p_req->dup.ctx_hndl_cand,
			   p_req->dup.challenge);
	break;

    case CMD_MCSTAT:
	status = do_mc_stat(p_afu, p_conn_info,
			    p_req->stat.res_hndl,
			    &p_resp->stat);
	break;

    case CMD_MCNOTIFY:
	status = do_mc_notify(p_afu, p_conn_info,
			      &p_req->notify);
	break;

    default :
	TRACE_1(LOG_ERR, p_afu, "%s: bad command code %d in ready state\n",
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
    mc_req_t               mc_req;
    mc_resp_t              mc_resp;
    conn_info_t            *p_conn_info;
    conn_info_t            *p_conn_info_new;

    afu_t                  *p_afu = (struct afu*) arg;

#ifndef _AIX

    // The listen is delayed until everything is ready. This prevent
    // clients from connecting to the server until it reaches this
    // point.
    rc = listen(p_afu->listen_fd, SOMAXCONN);
    if ( rc ) {
	TRACE_2(LOG_ERR, p_afu, "%s: listen failed, rc %d, errno %d\n", rc, errno);
	exit(-1);
    }

    while (1) { 
	//
	// Wait for an event on any of the watched file descriptors
	// block for ever if no events.
	//
	// Note we poll all file descriptors in the epoll structure
	// but receive only up to MAX_CONN_TO_POLL ready fds at a time.
	//
	nready = epoll_wait(p_afu->epfd, &p_afu->events[0], 
			    MAX_CONN_TO_POLL, -1);

	if ((nready == -1) && (errno == EINTR)) {
	    continue;
	}

	for (i = 0; i < nready; i++) {
	    p_conn_info = (conn_info_t*) p_afu->events[i].data.ptr;

	    if (p_afu->events[i].events & (EPOLLHUP | EPOLLERR))  {
		// Something bad happened with the connection or the client
	        // just closed its end. Remove the file descriptor from the 
	        // array of watched FD's

		TRACE_3(LOG_INFO, p_afu, 
			"%s: connection is bad, %d (0x%08X), client fd %d\n", 
			p_conn_info->fd, p_afu->events[i].events, 
			p_conn_info->client_fd);

		rc = epoll_ctl(p_afu->epfd, EPOLL_CTL_DEL, p_conn_info->fd, 
			       &p_afu->events[i]);
		if (rc) {
		    TRACE_2(LOG_ERR, p_afu,
			    "%s: epoll_ctl failed for DEL: %d (%d)\n", rc, errno);
		}

		close(p_conn_info->fd);
		if (p_conn_info->p_ctx_info != NULL) { /* if registered */
		    (void) do_mc_unregister(p_afu, p_conn_info);
		}
		free_connection(p_afu, p_conn_info);
		
		continue;
	    }
      
	    // Is this the listening FD...would mean a new connection
	    if (p_conn_info->fd == p_afu->listen_fd) {
		len = sizeof(cli_addr);
		conn_fd = accept(p_afu->listen_fd, (struct sockaddr *)&cli_addr, &len); 
		if (conn_fd == -1) {
		    TRACE_2(LOG_ERR, p_afu,
			    "%s: accept failed, fd %d, errno %d\n", conn_fd, errno);
		    continue;
		}

		p_conn_info_new = alloc_connection(p_afu, conn_fd); 
		if (p_conn_info_new == NULL) {
		    TRACE_0(LOG_ERR, p_afu, 
			    "%s: too many connections, closing new connection\n");
		    close(conn_fd);
		    continue;
		}

		// Add the connection to the watched array
		epoll_event.events = EPOLLIN;
		epoll_event.data.ptr = p_conn_info_new;
		rc = epoll_ctl(p_afu->epfd, EPOLL_CTL_ADD, conn_fd, &epoll_event);
		if (rc == -1) {
		    TRACE_1(LOG_ERR, p_afu, 
			    "%s: epoll_ctl ADD failed, errno %d\n", errno);
		    close(conn_fd);
		    continue;
		}
	    }
	    else {
		// We can now assume that we have data to be read from a client
		// Read the entire command
		rc = xfer_data(p_conn_info->fd, XFER_OP_READ, 
			       (void *)&mc_req, sizeof(mc_req));
		if (rc) {
		    TRACE_1(LOG_ERR, p_afu, "%s: read of cmd failed, rc %d\n", rc);
		    continue;
		}

		TRACE_4(LOG_INFO, p_afu,
			"%s: command code=%d, tag=%d, size=%d, conn_info=%p\n", 
			mc_req.header.command,
			mc_req.header.tag,
			mc_req.header.size,
			p_conn_info);

		// process command and fill in response while protecting from
		// other threads accessing the afu
		pthread_mutex_lock(&p_afu->mutex);
		mc_resp.header.status =  
		    (*p_conn_info->rx)(p_afu, p_conn_info, &mc_req, &mc_resp);
		pthread_mutex_unlock(&p_afu->mutex);

		// Send response back to the client.
		mc_resp.header.command  = mc_req.header.command;
		mc_resp.header.tag  = mc_req.header.tag;
		mc_resp.header.size = sizeof(mc_resp);
		rc = xfer_data(p_conn_info->fd, XFER_OP_WRITE, 
			       (void *)&mc_resp, sizeof(mc_resp));
		if (rc) {
		    TRACE_1(LOG_ERR, p_afu,
			    "%s: write of cmd response failed, rc %d\n", rc);
		    continue;
		}
	    }
	}
    }

#endif /* !_AIX */
    return NULL;
}


asyc_intr_info_t ainfo[] = {
    { SISL_ASTATUS_FC0_OTHER,    "fc 0: other error", 0, CLR_FC_ERROR | LINK_RESET },
    { SISL_ASTATUS_FC0_LOGO,     "fc 0: target initiated LOGO", 0, 0 },
    { SISL_ASTATUS_FC0_CRC_T,    "fc 0: CRC threshold exceeded", 0, LINK_RESET },
    { SISL_ASTATUS_FC0_LOGI_R,   "fc 0: login timed out, retrying", 0, 0 },
    { SISL_ASTATUS_FC0_LOGI_F,   "fc 0: login failed", 0, CLR_FC_ERROR },
    { SISL_ASTATUS_FC0_LOGI_S,   "fc 0: login succeeded", 0, 0 },
    { SISL_ASTATUS_FC0_LINK_DN,  "fc 0: link down", 0, 0 },
    { SISL_ASTATUS_FC0_LINK_UP,  "fc 0: link up", 0, 0 },

    { SISL_ASTATUS_FC1_OTHER,    "fc 1: other error", 1, CLR_FC_ERROR | LINK_RESET },
    { SISL_ASTATUS_FC1_LOGO,     "fc 1: target initiated LOGO", 1, 0 },
    { SISL_ASTATUS_FC1_CRC_T,    "fc 1: CRC threshold exceeded", 1, LINK_RESET },
    { SISL_ASTATUS_FC1_LOGI_R,   "fc 1: login timed out, retrying", 1, 0 },
    { SISL_ASTATUS_FC1_LOGI_F,   "fc 1: login failed", 1, CLR_FC_ERROR },
    { SISL_ASTATUS_FC1_LOGI_S,   "fc 1: login succeeded", 1, 0 },
    { SISL_ASTATUS_FC1_LINK_DN,  "fc 1: link down", 1, 0 },
    { SISL_ASTATUS_FC1_LINK_UP,  "fc 1: link up", 1, 0 },

    { 0x0,                       "", 0, 0 } /* terminator */
};

asyc_intr_info_t *find_ainfo(__u64 status)
{
    asyc_intr_info_t *p_info;

    for (p_info = &ainfo[0]; p_info->status; p_info++) {
	if (p_info->status == status) {
	    return p_info;
	}
    }

    return NULL;
}

void afu_rrq_intr(afu_t *p_afu) {
    struct afu_cmd *p_cmd;

    // process however many RRQ entries that are ready
    while ((*p_afu->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
	   p_afu->toggle) {
	p_cmd = (struct afu_cmd*)
	    ((*p_afu->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));

	pthread_mutex_lock(&p_cmd->mutex);
	p_cmd->sa.host_use_b[0] |= B_DONE;
	pthread_cond_signal(&p_cmd->cv);
	pthread_mutex_unlock(&p_cmd->mutex);
      
	if (p_afu->p_hrrq_curr < p_afu->p_hrrq_end) {
	    p_afu->p_hrrq_curr++; /* advance to next RRQ entry */
	}
	else { /* wrap HRRQ  & flip toggle */
	    p_afu->p_hrrq_curr = p_afu->p_hrrq_start;
	    p_afu->toggle ^= SISL_RESP_HANDLE_T_BIT;
	}
    }
}

void afu_sync_intr(afu_t *p_afu) {
    __u64 reg;
    __u64 reg_unmasked;

    reg = read_64(&p_afu->p_host_map->intr_status);
    reg_unmasked = (reg & SISL_ISTATUS_UNMASK);

    if (reg_unmasked == 0) {
	TRACE_1(LOG_ERR, p_afu, 
		"%s: spurious interrupt, intr_status 0x%016lx\n", reg);
	return;
    }

    TRACE_1(LOG_ERR, p_afu, 
	    "%s: unexpected interrupt, intr_status 0x%016lx\n", reg);

    write_64(&p_afu->p_host_map->intr_clear, reg_unmasked);

    return;
}

// this function can block up to a few seconds
void afu_async_intr(afu_t *p_afu) {
    int i;
    __u64 reg;
    __u64 reg_unmasked;
    asyc_intr_info_t *p_info;
    volatile struct sisl_global_map *p_global = &p_afu->p_afu_map->global;

    reg = read_64(&p_global->regs.aintr_status);
    reg_unmasked = (reg & SISL_ASTATUS_UNMASK);

    if (reg_unmasked == 0) {
	TRACE_1(LOG_ERR, p_afu, 
		"%s: spurious interrupt, aintr_status 0x%016lx\n", reg);
	return;
    }

    /* it is OK to clear AFU status before FC_ERROR */
    write_64(&p_global->regs.aintr_clear, reg_unmasked);

    /* check each bit that is on */
    for (i = 0; reg_unmasked; i++, reg_unmasked = (reg_unmasked >> 1)) {
	if ((reg_unmasked & 0x1) == 0 ||
	    (p_info = find_ainfo(1ull << i)) == NULL) {
	    continue;
	}

	TRACE_2(LOG_ERR, p_afu, "%s: %s, fc_status 0x%08lx\n",
		p_info->desc,
		read_64(&p_global->fc_regs[p_info->port][FC_STATUS/8]));

	// do link reset first, some OTHER errors will set FC_ERROR again
	// if cleared before or w/o a reset
	//
	if (p_info->action & LINK_RESET) {
	    TRACE_1(LOG_ERR, p_afu, "%s: fc %d: resetting link\n", 
		    p_info->port);
	    afu_link_reset(p_afu, p_info->port, 
			   &p_afu->p_afu_map->global.fc_regs[p_info->port][0]);
	}

	if (p_info->action & CLR_FC_ERROR) {
	    reg = read_64(&p_global->fc_regs[p_info->port][FC_ERROR/8]);

	    // since all errors are unmasked, FC_ERROR and FC_ERRCAP
	    // should be the same and tracing one is sufficient.
	    //
	    TRACE_2(LOG_ERR, p_afu, "%s: fc %d: clearing fc_error 0x%08lx\n", 
		    p_info->port, reg);
	    write_64(&p_global->fc_regs[p_info->port][FC_ERROR/8], reg);
	    write_64(&p_global->fc_regs[p_info->port][FC_ERRCAP/8], 0);
	}

    }

}

// notify error thread that it has work to do
void notify_err(afu_t *p_afu, int err_flag)
{
    pthread_mutex_lock(&p_afu->err_mutex);
    p_afu->err_flag |= err_flag;
    pthread_cond_signal(&p_afu->err_cv);
    pthread_mutex_unlock(&p_afu->err_mutex);
}

/*
 * This thread receives all interrupts, not just RRQ.
 *
 * But it processes only the RRQ. Error interrupts are forwarded to their
 * own thread for processing. Note that error processing may involve 
 * sending a command to the afu. Doing so from this thread and then blocking
 * for a response will be a deadlock. The rrq thread must remain free ALL 
 * the time to collect responses.
 *
 * The rrq thread wakes up the error thread via a cv. Ideally, each thread
 * could block on the afu_fd waiting for a particular irq#. If the kernel
 * were to support such a demultiplexed irq dispatch via read, the cv can 
 * be removed.
 *
 * NOTE: afu_rrq_rx must not block (on events) or sleep, else response
 *       delivery can delay/deadlock.
 */
void *afu_rrq_rx(void *arg) {
  struct cxl_event *p_event;
  int len;
  afu_t  *p_afu = (struct afu*) arg;

  while (1) {
      //
      // read afu fd & block on any interrupt
      len = read(p_afu->afu_fd, &p_afu->event_buf[0], 
		 sizeof(p_afu->event_buf));


      if (len < 0) {
	  TRACE_0(LOG_ERR, p_afu, "%s: afu has been reset, exiting...\n");
	  exit(-1);
      }

      p_event = (struct cxl_event *)&p_afu->event_buf[0];
      while (len >= sizeof(p_event->header)) {
	  if (p_event->header.type == CXL_EVENT_AFU_INTERRUPT) {
	      switch(p_event->irq.irq) {
	      case SISL_MSI_RRQ_UPDATED:
		  afu_rrq_intr(p_afu);
		  break;

	      case SISL_MSI_ASYNC_ERROR:
		  notify_err(p_afu, E_ASYNC_INTR);
		  break;

	      case SISL_MSI_SYNC_ERROR:
		  notify_err(p_afu, E_SYNC_INTR);
		  break;

	      default:
		  TRACE_1(LOG_ERR, p_afu, "%s: unexpected irq %d\n", p_event->irq.irq);
		  break;
	      }
	  }
	  else if (p_event->header.type == CXL_EVENT_DATA_STORAGE) {
	      TRACE_2(LOG_ERR, p_afu, 
		      "%s: CXL_EVENT_DATA_STORAGE addr = 0x%lx, dsisr = 0x%lx\n", 
		      p_event->fault.addr, p_event->fault.dsisr);
	  }
	  else {
	      TRACE_1(LOG_ERR, p_afu, "%s: unexpected event %d\n", 
		      p_event->header.type);
	  }

	  len -= p_event->header.size;
	  p_event = (struct cxl_event *)
	      (((char*)p_event) + p_event->header.size);
      }
  }

  return NULL;
}


void *afu_err_rx(void *arg) {
  afu_t  *p_afu = (struct afu*) arg;
  int err_flag;

  while (1) {
      //
      // block on error notification
      pthread_mutex_lock(&p_afu->err_mutex);
      while (p_afu->err_flag == 0) {
	  pthread_cond_wait(&p_afu->err_cv, &p_afu->err_mutex);
      }
      err_flag = p_afu->err_flag;
      p_afu->err_flag = 0;
      pthread_mutex_unlock(&p_afu->err_mutex);

      // about to access afu and/or send cmds, so protect from other
      // threads doing the same.
      //
      pthread_mutex_lock(&p_afu->mutex);

      if (err_flag & E_ASYNC_INTR) {
	  afu_async_intr(p_afu);
      }

      if (err_flag & E_SYNC_INTR) {
	  afu_sync_intr(p_afu);
      }
      
      pthread_mutex_unlock(&p_afu->mutex);

      // delay so someone else can grab p_afu->mutex
      // else it can starve out other threads when error 
      // interrupts flood the system
      //
      usleep(100000); // 100ms

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

void send_cmd(afu_t *p_afu, struct afu_cmd *p_cmd) {
    int nretry = 0;

    if (p_afu->room == 0) {
	asm volatile ( "eieio" : : ); // let IOARRIN writes complete
	do {
	    p_afu->room = read_64(&p_afu->p_host_map->cmd_room);
	    usleep(nretry);
	} while (p_afu->room == 0 && nretry++ < MC_ROOM_RETRY_CNT);
    }

    p_cmd->sa.host_use_b[0] = 0; // 0 means active
    p_cmd->sa.ioasc = 0;

    /* make memory updates visible to AFU before MMIO */
    asm volatile ( "lwsync" : : ); 

    timer_start(p_cmd->timer, p_cmd->rcb.timeout*2, 0);

    if (p_afu->room) {
	// write IOARRIN
	write_64(&p_afu->p_host_map->ioarrin, (__u64)&p_cmd->rcb);
    }
    else {
	TRACE_1(LOG_ERR, p_afu, "%s: no cmd_room to send 0x%x\n",
		p_cmd->rcb.cdb[0]);
	// let timer fire to complete the response
    }
}

void wait_resp(afu_t *p_afu, struct afu_cmd *p_cmd) {
    pthread_mutex_lock(&p_cmd->mutex);
    while (!(p_cmd->sa.host_use_b[0] & B_DONE)) {
	pthread_cond_wait(&p_cmd->cv, &p_cmd->mutex);
    }
    pthread_mutex_unlock(&p_cmd->mutex);

    timer_stop(p_cmd->timer); /* already stopped if timer fired */

    if (p_cmd->sa.ioasc != 0) {
	TRACE_5(LOG_ERR, p_afu,
		"%s: CMD 0x%x failed, IOASC: flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n",
		p_cmd->rcb.cdb[0],
		p_cmd->sa.rc.flags,
		p_cmd->sa.rc.afu_rc, 
		p_cmd->sa.rc.scsi_rc, 
		p_cmd->sa.rc.fc_rc);
    }
}

// do we need to retry AFU_CMDs (sync) on afu_rc = 0x30 ?
// can we not avoid that ?
// not retrying afu timeouts (B_TIMEOUT)
// returns 1 if the cmd should be retried, 0 otherwise
// sets B_ERROR flag based on IOASA
int check_status(sisl_ioasa_t *p_ioasa) 
{
    if (p_ioasa->ioasc == 0) {
	return 0;
    }

    p_ioasa->host_use_b[0] |= B_ERROR;

    if (!(p_ioasa->host_use_b[1]++ < MC_RETRY_CNT)) {
	return 0;
    }

    switch (p_ioasa->rc.afu_rc)
    {
    case SISL_AFU_RC_NO_CHANNELS:
    case SISL_AFU_RC_OUT_OF_DATA_BUFS:
	usleep(100); // 100 microsec
	return 1;

    case 0: 
	// no afu_rc, but either scsi_rc and/or fc_rc is set
	// retry all scsi_rc and fc_rc after a small delay
	usleep(100); // 100 microsec
	return 1;
    }

    return 0;
}

// lun_id must be set in p_lun_info
int find_lun(afu_t *p_afu, lun_info_t *p_lun_info,
	     __u32 port_sel) {
    __u32 *p_u32;
    __u32 len;
    __u64 *p_u64;
    __u64 lun_id;
    struct afu_cmd *p_cmd = &p_afu->cmd[AFU_INIT_INDEX];

    memset(&p_afu->buf[0], 0, sizeof(p_afu->buf));
    memset(&p_cmd->rcb.cdb[0], 0, sizeof(p_cmd->rcb.cdb));

    p_cmd->rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID | 
			    SISL_REQ_FLAGS_SUP_UNDERRUN |
			    SISL_REQ_FLAGS_HOST_READ);
    p_cmd->rcb.port_sel = port_sel;
    p_cmd->rcb.lun_id = 0x0; /* use lun_id=0 w/report luns */
    p_cmd->rcb.data_len = sizeof(p_afu->buf);
    p_cmd->rcb.data_ea = (__u64) &p_afu->buf[0];
    p_cmd->rcb.timeout = MC_DISCOVERY_TIMEOUT;

    p_cmd->rcb.cdb[0] = 0xA0; /* report luns */
    p_u32 = (__u32*)&p_cmd->rcb.cdb[6];
    write_32(p_u32, sizeof(p_afu->buf)); /* allocaiton length */
    p_cmd->sa.host_use_b[1] = 0; /* reset retry cnt */

    TRACE_3(LOG_INFO, p_afu,
	    "%s: sending cmd(0x%x) with RCB EA=%p data EA=0x%lx\n", 
	   p_cmd->rcb.cdb[0],
	   &p_cmd->rcb, 
	   p_cmd->rcb.data_ea);

    do {
	send_cmd(p_afu, p_cmd);
	wait_resp(p_afu, p_cmd);
    } while (check_status(&p_cmd->sa));

    if (p_cmd->sa.host_use_b[0] & B_ERROR) { 
	return -1;
    }

    // report luns success
    len = read_32((__u32*)&p_afu->buf[0]);
    p_u64 = (__u64*)&p_afu->buf[8]; /* start of lun list */

    while (len) {
	lun_id = read_64(p_u64++);
	if (lun_id == p_lun_info->lun_id) {
	    return 0;
	}
	len -= 8;
    }

    return -1;
}

int read_cap16(afu_t *p_afu, lun_info_t *p_lun_info,
	       __u32 port_sel) {
    __u32 *p_u32;
    __u64 *p_u64;
    struct afu_cmd *p_cmd = &p_afu->cmd[AFU_INIT_INDEX];

    memset(&p_afu->buf[0], 0, sizeof(p_afu->buf));
    memset(&p_cmd->rcb.cdb[0], 0, sizeof(p_cmd->rcb.cdb));

    p_cmd->rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID | 
			    SISL_REQ_FLAGS_SUP_UNDERRUN |
			    SISL_REQ_FLAGS_HOST_READ);
    p_cmd->rcb.port_sel = port_sel;
    p_cmd->rcb.lun_id = p_lun_info->lun_id;
    p_cmd->rcb.data_len = sizeof(p_afu->buf);
    p_cmd->rcb.data_ea = (__u64) &p_afu->buf[0];
    p_cmd->rcb.timeout = MC_DISCOVERY_TIMEOUT;

    p_cmd->rcb.cdb[0] = 0x9E; /* read cap(16) */
    p_cmd->rcb.cdb[1] = 0x10; /* service action */
    p_u32 = (__u32*)&p_cmd->rcb.cdb[10];
    write_32(p_u32, sizeof(p_afu->buf)); /* allocation length */
    p_cmd->sa.host_use_b[1] = 0; /* reset retry cnt */

    TRACE_3(LOG_INFO, p_afu,
	    "%s: sending cmd(0x%x) with RCB EA=%p data EA=0x%lx\n", 
	   p_cmd->rcb.cdb[0],
	   &p_cmd->rcb, 
	   p_cmd->rcb.data_ea);

    do {
	send_cmd(p_afu, p_cmd);
	wait_resp(p_afu, p_cmd);
    } while (check_status(&p_cmd->sa));

    if (p_cmd->sa.host_use_b[0] & B_ERROR) { 
	return -1;
    }

    // read cap success
    p_u64 = (__u64*)&p_afu->buf[0]; 
    p_lun_info->li.max_lba = read_64(p_u64);

    p_u32 = (__u32*)&p_afu->buf[8]; 
    p_lun_info->li.blk_len = read_32(p_u32);
    return 0;
}


int page83_inquiry(afu_t *p_afu, lun_info_t *p_lun_info,
		   __u32 port_sel) {
    __u16 *p_u16;
    __u16 length;
    struct scsi_inquiry_page_83_hdr *p_hdr;
    struct scsi_inquiry_p83_id_desc_hdr *p_desc;
    struct afu_cmd *p_cmd = &p_afu->cmd[AFU_INIT_INDEX];

    memset(&p_afu->buf[0], 0, sizeof(p_afu->buf));
    memset(&p_cmd->rcb.cdb[0], 0, sizeof(p_cmd->rcb.cdb));

    p_cmd->rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID | 
			    SISL_REQ_FLAGS_SUP_UNDERRUN |
			    SISL_REQ_FLAGS_HOST_READ);
    p_cmd->rcb.port_sel = port_sel;
    p_cmd->rcb.lun_id = p_lun_info->lun_id;
    p_cmd->rcb.data_len = sizeof(p_afu->buf);
    p_cmd->rcb.data_ea = (__u64) &p_afu->buf[0];
    p_cmd->rcb.timeout = MC_DISCOVERY_TIMEOUT;

    p_cmd->rcb.cdb[0] = 0x12; /* inquiry */
    p_cmd->rcb.cdb[1] = 0x1;  /* EVPD bit */
    p_cmd->rcb.cdb[2] = 0x83; /* page# */
    p_u16 = (__u16*)&p_cmd->rcb.cdb[3];
    write_16(p_u16, sizeof(p_afu->buf)); /* allocaiton length */
    p_cmd->sa.host_use_b[1] = 0; /* reset retry cnt */

    TRACE_3(LOG_INFO, p_afu,
	    "%s: sending cmd(0x%x) with RCB EA=%p data EA=0x%lx\n", 
	   p_cmd->rcb.cdb[0],
	   &p_cmd->rcb, 
	   p_cmd->rcb.data_ea);

    do {
	send_cmd(p_afu, p_cmd);
	wait_resp(p_afu, p_cmd);
    } while (check_status(&p_cmd->sa));

    if (p_cmd->sa.host_use_b[0] & B_ERROR) { 
	return -1;
    }

    // inquiry success
    p_hdr = (struct scsi_inquiry_page_83_hdr *)&p_afu->buf[0];
    length = read_16(&p_hdr->adtl_page_length);
    if (length > (sizeof(p_afu->buf) - 4)) {
	length = sizeof(p_afu->buf) - 4;
    }
    p_desc = (struct scsi_inquiry_p83_id_desc_hdr *)(p_hdr + 1);
    /* loop through data searching for LUN WWID entry */
    while (length >= sizeof(*p_desc)) {
	if (p_desc->prot_code == TEXAN_PAGE_83_DESC_PROT_CODE &&
	    p_desc->assoc_id == TEXAN_PAGE_83_ASSC_ID_LUN_WWID &&
	    p_desc->adtl_id_length >= 0x10) { /* NAA-6 id is 0x10 bytes */

	    memcpy(p_lun_info->li.wwid, (char*)(p_desc + 1), 
		   sizeof(p_lun_info->li.wwid));
	    return 0;
	}

	length -= (sizeof(struct scsi_inquiry_p83_id_desc_hdr) +
		   p_desc->adtl_id_length);

	p_desc = (struct scsi_inquiry_p83_id_desc_hdr *)
	    ((char *)p_desc +
	     sizeof(struct scsi_inquiry_p83_id_desc_hdr) +
	     p_desc->adtl_id_length);
    }

    return -1;
}

// add write_same !!! 
// must not use AFU_SYNC_INDEX when sending multiple cmds


// afu_sync can be called from interrupt thread and the main processing
// thread. Caller is responsible for any serialization.
// Also, it can be called even before/during discovery, so we must use
// a dedicated cmd not used by discovery.
//
// AFU takes only 1 sync cmd at a time.
// 
int afu_sync(afu_t *p_afu, ctx_hndl_t ctx_hndl_u, 
	     res_hndl_t res_hndl_u,
	     __u8 mode) {
    __u32 *p_u32;
    __u16 *p_u16;
    struct afu_cmd *p_cmd = &p_afu->cmd[AFU_SYNC_INDEX];

    memset(&p_cmd->rcb.cdb[0], 0, sizeof(p_cmd->rcb.cdb));

    p_cmd->rcb.req_flags = SISL_REQ_FLAGS_AFU_CMD;
    p_cmd->rcb.port_sel = 0x0; /* NA */
    p_cmd->rcb.lun_id = 0x0; /* NA */
    p_cmd->rcb.data_len = 0x0;
    p_cmd->rcb.data_ea = 0x0;
    p_cmd->rcb.timeout = MC_AFU_SYNC_TIMEOUT;

    p_cmd->rcb.cdb[0] = 0xC0; /* AFU Sync */
    p_cmd->rcb.cdb[1] = mode;
    p_u16 = (__u16*)&p_cmd->rcb.cdb[2];
    write_16(p_u16, ctx_hndl_u); /* context to sync up */
    p_u32 = (__u32*)&p_cmd->rcb.cdb[4];
    write_32(p_u32, res_hndl_u); /* res_hndl to sync up */

    send_cmd(p_afu, p_cmd);
    wait_resp(p_afu, p_cmd);

    if (p_cmd->sa.ioasc != 0 ||
	(p_cmd->sa.host_use_b[0] & B_ERROR)) // B_ERROR is set on timeout
    {
	return -1;
    }

    return 0;
}

/* Periodic functions are called from sig_rx and can only do simple 
 * MMIOs that 1) do not block/wait and 2) do not need p_afu->mutex. 
 * 
 * Needing to lock the mutex can cause sig_rx to wait for another
 * thread and delay/deadlock timer delivery. If locking is 
 * absolutely necessary in periodic processing, these functions
 * must be moved to another thread.
 * 
 */
void periodic_hb() 
{
    int i;
    afu_t *p_afu;
    __u64 hb_read;

    for (i = 0; i < gb.p_ini->nelm; i++) {
	p_afu = &gb.p_afu_a[i].afu;
	hb_read = read_64(&p_afu->p_afu_map->global.regs.afu_hb);
	p_afu->hb++;
	if (hb_read != p_afu->hb) {
	    TRACE_2(LOG_ERR, p_afu, "%s: hb_cnt 0x%lx, expected 0x%lx\n", 
		    hb_read, p_afu->hb);
	    p_afu->hb = hb_read; // re-sync
	}
    }
}

void periodic_fc() 
{
    int i;
    afu_t *p_afu;

    for (i = 0; i < gb.p_ini->nelm; i++) {
	p_afu = &gb.p_afu_a[i].afu;

	for (i = 0; i < NUM_FC_PORTS; i++) {
	    // clear CRC error cnt 
	    read_64(&p_afu->p_afu_map->global.fc_regs[i][FC_CNT_CRCERR/8]);
	}
    }
}


/* Note about timers:
 *
 * 1. All timers use the same signal. Since posix queues 1 signal per timer,
 *    this is not a problem.
 * 2. The signal must be a queued signal (one of realtime signals). If 2
 *    timers expire at the same time, we want 2 signals queued. Using a
 *    traditional signal will result in only 1 signal to the process.
 * 3. posix does not queue 1 signal per expiration of a timer, only 1
 *    for any number of expirations. Since this code uses 1 shot timer, 
 *    this fact is not applicable.
 * 4. There can be a delay between expiry and when the signal is accepted.
 *    During that delay, the command may have completed. When sig_rx runs,
 *    it may mark the next command using the same afu_cmd as timedout.
 *    The net is that a timeout will be detected but on a different command
 *    than the one that almost timed out (but returned). Detection of a 
 *    timeout is more important that pin pointing exactly which command 
 *    timed out. The adapter must be reset on any timeout.
 * 5. A single thread collects timer expiry for any number of AFU instances.
 *    This thread has the same function as the afu_intr_rx threads.
 *    A command can return via the RRQ or via a timeout.
 * 6. Same thread also performs simple periodic tasks across all AFUs.
 *
 * NOTE: sig_rx must not block (on events) or sleep, else timer delivery
 *       can delay/deadlock.
 */
void *sig_rx(void *arg) {
    sigset_t *p_set = (sigset_t *) arg;
    siginfo_t sig;
    struct afu_cmd *p_cmd;

    while (1) {
	if (sigwaitinfo(p_set, &sig) > 0) {
	    /* anyone can send us a signal, so make sure it is from a
	       timer expiry */
	    if (sig.si_code != SI_TIMER) {
		continue;
	    }
	    
	    if (sig.si_value.sival_ptr == &gb.timer_hb) {
		periodic_hb();
	    }
	    else if (sig.si_value.sival_ptr == &gb.timer_fc) {
		periodic_fc();
	    }
	    else {
		/* cmd that timed out */
		p_cmd = (struct afu_cmd*) sig.si_value.sival_ptr;

		trace_1(LOG_ERR, "command timeout, opcode 0x%x\n", 
			p_cmd->rcb.cdb[0]);
		pthread_mutex_lock(&p_cmd->mutex);
		p_cmd->sa.host_use_b[0] |= (B_DONE | B_ERROR | B_TIMEOUT);
		pthread_cond_signal(&p_cmd->cv);
		pthread_mutex_unlock(&p_cmd->mutex);
	    }
	}
    }

    return NULL;
}

// rep = 0 for 1 shot timer.
void timer_start(timer_t timer, time_t sec, int rep) 
{
    struct itimerspec it;

    memset(&it, 0, sizeof(it));
    it.it_value.tv_sec = sec; /* it_interval zeroed for 1 shot timer */
    if (rep) {
	it.it_interval.tv_sec = sec;
    }
    timer_settime(timer, 0, &it, NULL);
}


void timer_stop(timer_t timer) 
{
    struct itimerspec it;

    memset(&it, 0, sizeof(it)); /* zeroed it_value stops the timer */
    timer_settime(timer, 0, &it, NULL);
}

// ascii_buf must be "char[33] ascii_buf"
void print_wwid(__u8 *p_wwid, char *ascii_buf) {
    int i;

    for (i = 0; i < 16; i++) {
	sprintf(ascii_buf, "%02x", (unsigned int) p_wwid[i]);
	ascii_buf += 2;
    }

    *ascii_buf = 0;
}

void usage(char *prog) {
    fprintf(stderr, "usage-a: %s [-l LUN_ID] [-v level] /dev/cxl/afu0.0m ....\n", prog);
    fprintf(stderr, "       : %s -l 0x1000000000000 /dev/cxl/afu0.0m\n", prog);
    fprintf(stderr, "usage-b: %s [-i ini_file] [-v level]\n", prog);
    fprintf(stderr, "       : %s -i myafu.ini\n", prog);
    fprintf(stderr, "       specify any number of master AFU paths in usage-a\n");
    fprintf(stderr, "       LUN_ID is in hex and must match report luns\n");
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
	case 'l' :      /* LUN_ID to use in hex */
	    sscanf(optarg, "%lx", &lun_id);
	    break;

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
    int j;
    char ascii_buf[32+1];
    int rc;
    afu_t *p_afu;
    lun_info_t *p_lun_info_first = NULL;
    sigset_t set;
    struct capikv_ini_elm *p_elm;
    struct capikv_ini *p_ini;
    struct afu_alloc *p_afu_a;
    struct sigevent sigev;
    pthread_t sig_thread;
    blka_t blka;


    openlog("mserv", LOG_PERROR, LOG_USER);

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

    rc = posix_memalign((void**)&p_afu_a, 0x1000, 
			sizeof(struct afu_alloc) * p_ini->nelm);
    if (rc != 0) {
	trace_1(LOG_ERR, "cannot allocate AFU structs, rc %d\n", rc);
	exit(-1);
    }
    gb.p_afu_a = p_afu_a;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGRTMIN; /* must use a queued signal */

    sigev.sigev_value.sival_ptr = &gb.timer_hb;
    rc = timer_create(CLOCK_REALTIME, &sigev, &gb.timer_hb);
    if (rc != 0) {
	trace_1(LOG_ERR, "hb timer_create failed, errno %d\n", errno);
	exit(-1);
    }

    sigev.sigev_value.sival_ptr = &gb.timer_fc;
    rc = timer_create(CLOCK_REALTIME, &sigev, &gb.timer_fc);
    if (rc != 0) {
	trace_1(LOG_ERR, "fc timer_create failed, errno %d\n", errno);
	exit(-1);
    }

    /* start thread to handle all timeouts. However, at this point it
       is prepared to handle only command timer expiry during discovery.
       Hold off starting the periodic timers till the end.
    */
    pthread_create(&sig_thread, NULL, sig_rx, &set);

    p_elm = &p_ini->elm[0];
    for (i = 0; i < p_ini->nelm; i++) {
	p_afu = &p_afu_a[i].afu;
	rc = afu_init(p_afu, p_elm);
	if (rc != 0) {
	    trace_2(LOG_ERR, "error instantiating afu %s, rc %d\n", 
		    p_elm->afu_dev_pathm, rc);
	    exit(-1);
	}

	pthread_create(&p_afu->rrq_thread, NULL, afu_rrq_rx, 
		       p_afu);
	pthread_create(&p_afu->err_thread, NULL, afu_err_rx, 
		       p_afu);

	/* after creating afu_err_rx thread, unmask error interrupts */
	afu_err_intr_init(p_afu); 

	// advance to next element using correct size
	p_elm = (struct capikv_ini_elm*) (((char*)p_elm) + p_ini->size);
    }

    /* discover the LUN specified in cmd line or cfg file */
    for (i = 0; i < p_ini->nelm; i++) {
	p_afu = &p_afu_a[i].afu;

	for (j = 0; j < NUM_FC_PORTS; j++) { /* discover on each port */
	    if ((rc = find_lun(p_afu, &p_afu->lun_info[j], 1u << j)) == 0 &&
		(rc = read_cap16(p_afu, &p_afu->lun_info[j], 1u << j)) == 0 &&
		(rc = page83_inquiry(p_afu, &p_afu->lun_info[j], 1u << j)) == 0) {
		p_afu->lun_info[j].flags = LUN_INFO_VALID;
		TRACE_3(LOG_NOTICE, p_afu,
			"%s: found lun_id 0x%lx: max_lba 0x%lx, blk_len %d\n", 
			p_afu->lun_info[j].lun_id,
			p_afu->lun_info[j].li.max_lba,
			p_afu->lun_info[j].li.blk_len);
		print_wwid(&p_afu->lun_info[j].li.wwid[0], ascii_buf);
		TRACE_2(LOG_NOTICE, p_afu,
			"%s:       wwid %s, fc_port %d\n", ascii_buf, j);
	    }
	    else {
		TRACE_2(LOG_ERR, p_afu, 
			"%s: error in LUN discovery: lun_id 0x%lx, port %d\n",
			p_afu->lun_info[j].lun_id, j);
		continue; /* this is ok if FC cable is not connected etc */
	    }
	}
    }

    /* verfiy all valid LUNs are the same, else bail out. */
    for (i = 0; i < p_ini->nelm; i++) {
	p_afu = &p_afu_a[i].afu;
	for (j = 0; j < NUM_FC_PORTS; j++) {
	    if (!(p_afu->lun_info[j].flags & LUN_INFO_VALID)) {
		continue;
	    }
	    
	    if (p_lun_info_first == NULL) {
		p_lun_info_first = &p_afu->lun_info[j];
	    }
	    else if (memcmp(&p_lun_info_first->li, &p_afu->lun_info[j].li,
			    sizeof(p_lun_info_first->li))) {
		TRACE_0(LOG_ERR, p_afu,
			"%s: configuration error: different LUNs on FC ports\n");
		exit(1);
	    }
	}
    }

    /* make sure we have at least 1 valid LUN */
    if (p_lun_info_first == NULL) {
	trace_0(LOG_ERR, "configuration error: no working LUNs found\n");
	exit(1);
    }

    /* initialize block allocator and point from each AFU */
    memset(&blka, 0, sizeof(blka));
    pthread_mutex_init(&blka.mutex, NULL);

    blka.ba_lun.lun_id = p_lun_info_first->lun_id;
    blka.ba_lun.lsize = p_lun_info_first->li.max_lba + 1;
    blka.ba_lun.lba_size = p_lun_info_first->li.blk_len;
    blka.ba_lun.au_size = MC_CHUNK_SIZE;
    blka.nchunk = blka.ba_lun.lsize/MC_CHUNK_SIZE;
    rc = ba_init(&blka.ba_lun);

    if (rc != 0) {
	trace_1(LOG_ERR, "cannot init block_alloc, rc %d\n", rc);
	exit(-1);
    }

    for (i = 0; i < p_ini->nelm; i++) {
	p_afu = &p_afu_a[i].afu;
	p_afu->p_blka = &blka;
    }

    /* When all went well, start IPC threads to take requests */
    for (i = 0; i < p_ini->nelm; i++) {
	pthread_create(&p_afu_a[i].afu.ipc_thread, NULL, afu_ipc_rx, 
		       &p_afu_a[i].afu);
    }

    /* start timers to kick off periodic processing */
    timer_start(gb.timer_hb, MC_HB_PERIOD, 1);
    timer_start(gb.timer_fc, MC_FC_PERIOD, 1);

    for (i = 0; i < p_ini->nelm; i++) {
	pthread_join(p_afu_a[i].afu.ipc_thread, NULL);
	pthread_join(p_afu_a[i].afu.rrq_thread, NULL);
	pthread_join(p_afu_a[i].afu.err_thread, NULL);
	afu_term(&p_afu_a[i].afu);
    }

    timer_stop(gb.timer_hb);
    timer_stop(gb.timer_fc);

    pthread_join(sig_thread, NULL);

    timer_delete(gb.timer_hb);
    timer_delete(gb.timer_fc);

    closelog();
 
    return 0;
}
