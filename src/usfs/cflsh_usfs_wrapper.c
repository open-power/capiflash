//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/usfs/cflsh_usfs_wrapper.c $
//
//  IBM CONFIDENTIAL
//
//  COPYRIGHT International Business Machines Corp. 2016
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
//  IBM_PROLOG_END_TAG                                                     */
#ifdef _AIX
static char sccsid[] = "%Z%%M%   %I%  %W% %G% %U%";
#endif

/*
 * COMPONENT_NAME: (sysxcflashusfs) CAPI Flash user space filesystem library
 *
 * FUNCTIONS: 
 *
 * ORIGINS: 27
 *
 *                  -- (                            when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS 
 * (C) COPYRIGHT International Business Machines Corp. 2015
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

/*
 * This source file provides shim where all user space common requests
 * typically for files or directories are intercepted and then either:
 *
 *     - Passed through to the original interface       
 *
 *     - Routed to this library for a user space filesystem. These items
 *       are determined if a path is provided, since it must have the 
 *       disk special filename prepended onto the filesystem path with a ":"
 *       separating them.  File descriptors returned for these operations (open,
 *       creat) are saved in a hash table to that subsequent calls with the file
 *       descriptor can be intercepted and routed to this library. 
 *
 * To leverage this shim one must use an environment variable
 * to preload this library first. This environment variable specifies
 * the absolute path of this library. For Linux the environment variable
 * is LD_PRELOAD. For AIX 32-bit is LDR_PRELOAD and AIX 64-bit LDR_PRELOAD64.
 * For AIX the library member functiones of shr.0 or shr_64.o must be specified
 * for LDR_PRELOAD or LDR_PROLOAD64, respectively. Thus one one would have
 * something like LDR_PRELOAD="/lib/libcflsh_usfs.a(shr.o)" .... The double quotes
 * must be used along with the parentheses.
 *
 * Also for AIX, one must run the following command: chdev -l sys0 -a enhanced_RBAC=false
 * and reboot. 
 *
 * NOTES; To determine what wrapper functions may be required for an application one
 *        can use the ltrace call on linux. For AIX, one should use the truss -u libc.a
 *
 *
 */

#define CFLSH_USFS_FILENUM 0x0600

#ifndef _AIX
#define _GNU_SOURCE
#endif /* !_AIX */

#include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <memory.h>
#include <time.h>
#include <utime.h>
#ifdef _AIX
#include <sys/aio.h>
#else
#include <aio.h>
#endif
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#endif /* !_AIX && !_MACOSX */


#include "cflsh_usfs_wrapper.h"

#ifdef _AIX
#include "cflsh_usfs.h"
#include "cflsh_usfs_admin.h"
#else
#include <cflsh_usfs.h>
#include <cflsh_usfs_admin.h>
#endif

#ifdef _AIX
#define CUSFS_RET_AIOCB(_aiocbp_,_rc_,_errno_)   \
do {                                             \
                                                 \
    (_aiocbp_)->aio_errno = (_errno_);           \
                                                 \
    (_aiocbp_)->aio_return = (_rc_);             \
                                                 \
} while (0)

#else

#define CUSFS_RET_AIOCB(_aiocbp_,_rc_,_errno_)   \
do {                                             \
                                                 \
    (_aiocbp_)->__error_code = (_errno_);        \
                                                 \
    (_aiocbp_)->__return_value = (_rc_);         \
                                                 \
} while (0)


#endif /* !_AIX */


cusfs_gwrapper_t cusfs_gwrapper;

pthread_mutex_t  cusfs_gwrapper_lock = PTHREAD_MUTEX_INITIALIZER;

static int (*real_fcntl)(int fd,int cmd,...) = NULL;

static int (*real_dup)(int fd) = NULL;

static int (*real_dup2)(int old_fd, int new_fd) = NULL;

static int (*real_posix_fadvise)(int fd, off_t offset, off_t len, int advice) = NULL;

static int (*real_posix_fallocate)(int fd, off_t offset, off_t len) = NULL;

static int (*real_posix_fallocate64)(int fd, off64_t offset, off64_t len) = NULL;

#ifndef _AIX
static int (*real_posix_fadvise64)(int fd, off64_t offset, off64_t len, int advice) = NULL;
#endif /* !_AIX */

static int (*real_statfs)(char *pathname, struct statfs  *statfs) = NULL;


static int (*real_statfs64)(char *pathname, struct statfs64  *statfs) = NULL;



static int (*real_mkdir)(char *path,mode_t mode_flags) = NULL;

static int (*real_rmdir)(char *pathname) = NULL;

#ifdef _AIX
static DIR64 *(*real_opendir64)(char *directory_name) = NULL;
#else
static DIR *(*real_opendir)(char *directory_name) = NULL;
#endif /* !_AIX */

#ifdef _AIX
static int (*real_closedir64)(DIR64 *directory) = NULL;
#else
static int (*real_closedir)(DIR *directory_linux) = NULL;
#endif /* !_AIX */

#ifdef _AIX
static int (*real_readdir64_r)(DIR64 *directory, struct dirent64 *entry,struct dirent64 **result) = NULL;
static struct dirent64 *(*real_readdir64)(DIR64 *directory) = NULL;
#else
static int (*real_readdir_r)(DIR *directory_linux, struct dirent *entry,struct dirent **result) = NULL;

#endif
static struct dirent *(*real_readdir)(DIR *directory) = NULL;

#ifdef _NOT_YET
#ifdef _AIX
static offset_t (*real_telldir64)(DIR64 *directory) = NULL;
#else
static off_t (*real_telldir)(DIR *directory_linux) = NULL;
#endif 


#ifdef _AIX
static void (*real_seekdir64)(DIR64 *directory, offset_t location) = NULL;
#else
static void (*real_seekdir)(DIR *directory_linux, off_t location) = NULL;
#endif 
#endif /* NOT_YET */

static int (*real_rename)(char *from_pathname,char *to_pathname) = NULL;

static int (*real_creat)(char *path,mode_t mode) = NULL;

static int (*real_open)(char *path,int flags,...) = NULL;

static int (*real_close)(int fd) = NULL;

static ssize_t (*real_read)(int fd, void *buffer,size_t nbytes) = NULL;

static ssize_t (*real_write)(int fd, void *buffer,size_t nbytes) = NULL;

static int (*real_fsync)(int fd) = NULL;

static off_t (*real_lseek)(int fd, off_t Offset, int Whence) = NULL;

static off64_t (*real_lseek64)(int fd, off64_t Offset, int Whence) = NULL;

#ifdef _AIX
static offset_t (*real_llseek)(int fd, offset_t Offset, int Whence) = NULL;

#endif /* AIX */


static int (*real_fstat)(int fd,struct stat *buffer) = NULL;

#ifdef _AIX
static int (*real_fstatx)(int fd, struct stat *buffer, int length, int command) = NULL;
static int (*real_fstat64)(int fd,struct stat64 *buffer) = NULL;

#endif /* AIX */

#ifdef _AIX
static int (*real_statx)(char *path, struct stat *buffer, int length, int command) = NULL;
static int (*real_stat64)(char *path, struct stat64 *buffer) = NULL;
#else
static int (*real_stat)(char *path, struct stat *buffer) = NULL;
#endif /* !AIX */

#ifdef _AIX
static int (*real_fstat64x)(int fd,struct stat64x *buffer) = NULL;
static int (*real_stat64x)(char *path, struct stat64x *buffer) = NULL;


#else
static int (*real_xstat)(int version, const char *path, struct stat *buffer) = NULL;
static int (*real_fxstat)(int version, int fd, struct stat *buffer) = NULL;

static int (*real_xstat64)(int version, const char *path, struct stat64 *buffer) = NULL;
static int (*real_fxstat64)(int version, int fd, struct stat64 *buffer) = NULL;

#endif /* !_AIX */

#ifdef _AIX 
static int (*real_readlink)(char *path, char *buffer, size_t buffer_size) = NULL;

#else
static ssize_t (*real_readlink)(char *path, char *buffer, size_t buffer_size) = NULL;
#endif /* !_AIX */

static int (*real_access)(char *path, int mode) = NULL;

static int (*real_fchmod)(int fd,mode_t mode) = NULL;

static int (*real_fchown)(int fd, uid_t uid, gid_t gid) = NULL;

static int (*real_ftruncate)(int fd, off_t length) = NULL;

static int (*real_ftruncate64)(int fd, off64_t length) = NULL;

static int (*real_link)(char *orig_path, char *path) = NULL;

static int (*real_unlink)(char *path) = NULL;

static int (*real_utime)(char *path,struct utimbuf *times) = NULL;

//??__futimens

static int (*real_aio_read64)(struct aiocb64 *aiocbp) = NULL;
static int (*real_aio_read)(struct aiocb *aiocbp) = NULL;

static int (*real_aio_write64)(struct aiocb64 *aiocbp) = NULL;
static int (*real_aio_write)(struct aiocb *aiocbp) = NULL;

static int (*real_aio_error64)(struct aiocb64 *aiocbp) = NULL;
static int (*real_aio_error)(struct aiocb *aiocbp) = NULL;

static ssize_t (*real_aio_return64)(struct aiocb64 *aiocbp) = NULL;
static size_t (*real_aio_return)(struct aiocb *aiocbp) = NULL;


static int (*real_aio_fsync64)(int op, struct aiocb64 *aiocbp) = NULL;
static int (*real_aio_fsync)(int op, struct aiocb *aiocbp) = NULL;

static int (*real_aio_cancel64)(int fd, struct aiocb64 *aiocbp) = NULL;
static int (*real_aio_cancel)(int fd, struct aiocb *aiocbp) = NULL;

static int (*real_aio_suspend64)(const struct aiocb64 *const list[], int nent,
                        const struct timespec *timeout) = NULL;

static int (*real_aio_suspend)(const struct aiocb *const list[], int nent,
                        const struct timespec *timeout) = NULL;

// #include <sys/statvfs.h>
//??int statvfs(const char *path, struct statvfs *buf);

/*
 * NAME:        CUSFS_GET_WRAPPER_FD_HASH
 *
 * FUNCTION:    Find wrapper fd from file descriptor in the hash table
 *
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              NULL = No chunk found.
 *              pointer = chunk found.
 *              
 */

static inline cflsh_usfs_wrap_fd_t *CUSFS_GET_WRAPPER_FD_HASH(int fd)
{
    cflsh_usfs_wrap_fd_t *fd_wrapper = NULL;


    pthread_mutex_lock(&cusfs_gwrapper_lock);
    fd_wrapper = cusfs_gwrapper.fd_hash[fd & MAX_NUM_CUSFS_WRAP_FDS_HASH];


    while (fd_wrapper) {

        if (fd_wrapper->fd == fd) {


            break;
        }

        fd_wrapper = fd_wrapper->next;

    }

    pthread_mutex_unlock(&cusfs_gwrapper_lock);
    return fd_wrapper;

}




/*
 * NAME:        CUSFS_IS_USFS_PATH
 *
 * FUNCTION:    Determine if the path specified is for a
 *              userspace filesystem.
 *
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              NULL = No chunk found.
 *              pointer = chunk found.
 *              
 */

static inline int CUSFS_IS_USFS_PATH(char *path)
{
    int rc = 0;


    if ((strchr(path,CFLSH_USFS_DISK_DELIMINATOR))
#ifndef BLOCK_FILEMODE_ENABLED
#ifdef _AIX
	&& (!strncmp(path,"/dev/hdisk",10))
#else
	&& ((!strncmp(path,"/dev/sd",7)) || (!strncmp(path,"/dev/sg",7)))
#endif
#endif /* !BLOCK_FILEMODE_ENABLED */
      )
    {

	/*
	 * This path starts with "/dev/" and
	 * has a ":" in it.
	 */

	//?? TODO: Should we do anything special here for filemode.
	rc = 1;


    }


    return rc;
}


/*
` * NAME:       cusfs_add_fd_wrapper_to_hash
 *
 * FUNCTION:    Add fd_wrapper to global wrapper hash table
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
void cusfs_add_fd_wrapper_to_hash(cflsh_usfs_wrap_fd_t *fd_wrapper, int flags)
{
    cflsh_usfs_wrap_fd_t *tmp_fd_wrapper = NULL;



    pthread_mutex_lock(&cusfs_gwrapper_lock);

    if (cusfs_gwrapper.fd_hash[fd_wrapper->fd & MAX_NUM_CUSFS_WRAP_FDS_HASH] == NULL) {

	cusfs_gwrapper.fd_hash[fd_wrapper->fd & MAX_NUM_CUSFS_WRAP_FDS_HASH] = fd_wrapper;

    } else {

	tmp_fd_wrapper = cusfs_gwrapper.fd_hash[fd_wrapper->fd & MAX_NUM_CUSFS_WRAP_FDS_HASH];

	while (tmp_fd_wrapper) {


	    if (tmp_fd_wrapper->next == NULL) {

		tmp_fd_wrapper->next = fd_wrapper;

	        fd_wrapper->prev = tmp_fd_wrapper;

		break;

	    }

	    tmp_fd_wrapper = tmp_fd_wrapper->next;
	}
    }

    pthread_mutex_unlock(&cusfs_gwrapper_lock);

    return;

}


/*
 * NAME:        cusfs_remove_fd_wrapper_from_hash
 *
 * FUNCTION:    Remove fd_wrapper from hash
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
void cusfs_remove_fd_wrapper_from_hash(cflsh_usfs_wrap_fd_t *fd_wrapper, int flags)
{
    if (fd_wrapper == NULL) {

	return;
    }

    pthread_mutex_lock(&cusfs_gwrapper_lock);

    if (fd_wrapper->prev) {
	fd_wrapper->prev->next = fd_wrapper->next;
	
    } else {
	
	cusfs_gwrapper.fd_hash[fd_wrapper->fd & MAX_NUM_CUSFS_WRAP_FDS_HASH] = fd_wrapper->next;
    }
    
    if (fd_wrapper->next) {
	fd_wrapper->next->prev = fd_wrapper->prev;
    }

    pthread_mutex_unlock(&cusfs_gwrapper_lock);    
    
    return;

}

/*
 * NAME:        cufs_alloc_fd_wrapper
 *
 * FUNCTION:    Allocate an fd_wrapper structure
 *              
 *
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */

int cufs_alloc_fd_wrapper(int *fd)
{
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    
    if (CUSFS_GET_WRAPPER_FD_HASH(*fd+CFLSH_USFS_WRAPPER_INITIAL_FD) != NULL) {

	/*
	 * if this fd is already in the hash list, then don't 
	 * add it again.
	 */

	return 0;

    }


    if (fd == NULL) {

	return -1;
    }

    fd_wrapper = malloc(sizeof(*fd_wrapper));

    if (fd_wrapper == NULL) {

	return -1;

    }

    bzero(fd_wrapper,sizeof(*fd_wrapper));


    /*
     * ??TODO: There is an issue that this code is not taking into account
     *         that this library could generate the same file descriptors as other
     *
     *         The current work around for this now is to add an offset CFLSH_USFS_WRAPPER_INITIAL_FD
     *         to all file descriptors returned by the library.
     */

    fd_wrapper->usfs_fd = *fd;

    *fd += CFLSH_USFS_WRAPPER_INITIAL_FD;
    fd_wrapper->fd = *fd;

    cusfs_add_fd_wrapper_to_hash(fd_wrapper,0);

    return 0;
}


/*
 * NAME:        cufs_free_fd_wrapper
 *
 * FUNCTION:    Free an fd_wrapper structure
 *              
 *
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
    

int cufs_free_fd_wrapper(cflsh_usfs_wrap_fd_t *fd_wrapper)
{


    cusfs_remove_fd_wrapper_from_hash(fd_wrapper,0);

    free(fd_wrapper);


    return 0;
}


/*
 * NAME:        statfs
 *
 * FUNCTION:    Get statistics on filesystems
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int statfs(const char *path, struct statfs  *statfs)
{ 
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_statfs((char *)path,statfs);

	
    } else {

	real_statfs = (int(*)(char*,struct statfs *))dlsym(RTLD_NEXT,"statfs");

	if (real_statfs) {

	    rc = real_statfs((char *)path,statfs);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}




/*
 * NAME:        statfs64
 *
 * FUNCTION:    Get statistics on filesystems
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int statfs64(const char *path, struct statfs64  *statfs)
{ 
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_statfs64((char *)path,statfs);

	
    } else {

	real_statfs64 = (int(*)(char*,struct statfs64 *))dlsym(RTLD_NEXT,"statfs64");

	if (real_statfs64) {

	    rc = real_statfs64((char *)path,statfs);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}




/*
 * NAME:        mkdir
 *
 * FUNCTION:    Make a directory 
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int mkdir(const char *path,mode_t mode_flags)
{ 
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_mkdir((char *)path,mode_flags);

	
    } else {

	real_mkdir = (int(*)(char*,unsigned int))dlsym(RTLD_NEXT,"mkdir");

	if (real_mkdir) {

	    rc = real_mkdir((char *)path,mode_flags);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        rmdir
 *
 * FUNCTION:    Remove a directory 
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int rmdir(const char *path)
{ 
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_rmdir((char *)path);

	
    } else {

	real_rmdir = (int(*)(char*))dlsym(RTLD_NEXT,"rmdir");

	if (real_rmdir) {

	    rc = real_rmdir((char *)path);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        fcntl
 *
 * FUNCTION:    performs controlling function
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int fcntl(int fd,int cmd,...)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    long arg;
    va_list ap;

    /*
     * fcntl has a most 3 arguments.
     * So we only need to try to extract
     * one argument here. We chose type long
     * which varies from 32-bit to 64-bit to
     * consistent with possible pointer here.
     */

    va_start(ap,cmd);
    arg = va_arg(ap,long);
    va_end(ap);

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	fprintf(stderr,"fcntl called with cmd = %d",cmd);

    } else {

	real_fcntl = (int(*)(int, int, ...))dlsym(RTLD_NEXT,"fcntl");

	if (real_fcntl) {

	    rc = real_fcntl(fd,cmd,arg);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        dup2
 *
 * FUNCTION:    Duplicating file descriptor
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int dup2(int old_fd,int new_fd)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(old_fd);

    if (fd_wrapper) {

	fprintf(stderr,"dup2 called with old_fd = %d, and new_fd = %d",old_fd,new_fd);

    } else {

	real_dup2 = (int(*)(int, int))dlsym(RTLD_NEXT,"dup2");

	if (real_dup2) {

	    rc = real_dup2(old_fd,new_fd);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        posix_fallocate
 *
 * FUNCTION:    Provides advisory information about a file.
 *              Currently this call ignores the advice.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int posix_fallocate(int fd,off_t offset, off_t len)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    off_t truncate_len = offset + len;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	fprintf(stderr,"posix_fallocated called with fd = %d",fd);
	/*
	 * Map this into a ftruncate request.
	 */

	rc = cusfs_ftruncate(fd_wrapper->usfs_fd,truncate_len);

    } else {

	real_posix_fallocate = (int(*)(int, off_t, off_t))dlsym(RTLD_NEXT,"posix_fallocate");

	if (real_posix_fallocate) {

	    rc = real_posix_fallocate(fd,offset, len);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        posix_fallocate64
 *
 * FUNCTION:    Provides advisory information about a file.
 *              Currently this call ignores the advice.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int posix_fallocate64(int fd,off64_t offset, off64_t len)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    off_t truncate_len = offset + len;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	fprintf(stderr,"posix_fallocated called with fd = %d",fd);
	/*
	 * Map this into a ftruncate request.
	 */

	rc = cusfs_ftruncate64(fd_wrapper->usfs_fd,truncate_len);

    } else {

	real_posix_fallocate64 = (int(*)(int, off64_t, off64_t))dlsym(RTLD_NEXT,"posix_fallocate64");

	if (real_posix_fallocate64) {

	    rc = real_posix_fallocate64(fd,offset, len);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        posix_fadvise
 *
 * FUNCTION:    Provides advisory information about a file.
 *              Currently this call ignores the advice.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int posix_fadvise(int fd,off_t offset, off_t len, int advice)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_posix_fadvise(fd_wrapper->usfs_fd,offset,len,advice);

    } else {

	real_posix_fadvise = (int(*)(int, off_t, off_t, int))dlsym(RTLD_NEXT,"posix_fadvise");

	if (real_posix_fadvise) {

	    rc = real_posix_fadvise(fd,offset, len, advice);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#ifndef _AIX


/*
 * NAME:        posix_fadvise64
 *
 * FUNCTION:    Provides advisory information about a file.
 *              Currently this call ignores the advice.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int posix_fadvise64(int fd,off64_t offset, off64_t len, int advice)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {


	rc = cusfs_posix_fadvise(fd_wrapper->usfs_fd,offset,len,advice);

    } else {

	real_posix_fadvise64 = (int(*)(int, off64_t, off64_t, int))dlsym(RTLD_NEXT,"posix_fadvise64");

	if (real_posix_fadvise64) {

	    rc = real_posix_fadvise64(fd,offset, len, advice);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#endif /* !_AIX */

/*
 * NAME:        dup
 *
 * FUNCTION:    Duplicating file descriptor
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 on error. Otherwise  success
 *              
 *              
 */

int dup(int fd)
{
    int rc = 0;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	fprintf(stderr,"dup called with fd = %d",fd);

    } else {

	real_dup = (int(*)(int))dlsym(RTLD_NEXT,"dup");

	if (real_dup) {

	    rc = real_dup(fd);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#ifdef _AIX
/*
 * NAME:        opendir64
 *
 * FUNCTION:    Opens a a directory 
 *              on a disk. 
 * 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

DIR64 *opendir64(const char *directory_name)
{
    DIR64 *rc;
#ifdef _AIX
    DIR64 *directory;
#else
    cflsh_usfs_DIR_t *directory;
#endif /* _AIX */

    if (CUSFS_IS_USFS_PATH((char *)directory_name)) {

	rc = cusfs_opendir64((char *)directory_name);

	if (rc) {

#ifdef _AIX
	    directory = rc;
#else
	    directory = (cflsh_usfs_DIR_t *)rc;
#endif

	    if (cufs_alloc_fd_wrapper(&(directory->dd_fd))) {

		rc = NULL;
	    }

	}
	
    } else {

	real_opendir64 = (DIR64 *(*)(char *))dlsym(RTLD_NEXT,"opendir64");

	if (real_opendir64) {

	    rc = real_opendir64((char *)directory_name);

	} else {

	    errno = EBADF;
	    rc = NULL;
	}
    }

    return rc;

}
#else
/*
 * NAME:        opendir
 *
 * FUNCTION:    Opens a a directory 
 *              on a disk. 
 * 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
DIR *opendir(const char *directory_name)
{
    DIR *rc = NULL;
#ifdef _AIX
    DIR *directory;
#else
    cflsh_usfs_DIR_t *directory;
#endif /* _AIX */


    if (CUSFS_IS_USFS_PATH((char *)directory_name)) {

	directory = (cflsh_usfs_DIR_t *)cusfs_opendir((char *)directory_name);

	if (directory) {

	    rc = (DIR *) directory;

	    if (cufs_alloc_fd_wrapper(&(directory->dd_fd))) {

		rc = NULL;
	    }

	}
	
    } else {

	real_opendir = (DIR *(*)(char *))dlsym(RTLD_NEXT,"opendir");

	if (real_opendir) {

	    rc = real_opendir((char *)directory_name);

	} else {

	    errno = EBADF;
	    rc = NULL;
	}
    }

    return rc;

}
#endif /* !_AIX */

#ifdef _AIX
/*
 * NAME:        closedir64
 *
 * FUNCTION:    Opens a a directory
 *              on a disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int closedir64(DIR64 *directory)
{
    int rc;
#ifndef _AIX
    cflsh_usfs_DIR_t *usfs_directory = (cflsh_usfs_DIR_t *)directory;
#else
    DIR64 *usfs_directory = directory;
#endif
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(usfs_directory->dd_fd);

    if (fd_wrapper) {

	usfs_directory->dd_fd = fd_wrapper->usfs_fd;

	rc = cusfs_closedir64((DIR64 *)usfs_directory);

	if (!rc) {

	    cufs_free_fd_wrapper(fd_wrapper);
	}
    } else {


	real_closedir64 = (int(*)(struct _dirdesc64*))dlsym(RTLD_NEXT,"closedir64");

	if (real_closedir64) {

	    rc = real_closedir64(directory);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#else

/*
 * NAME:        closedir
 *
 * FUNCTION:    Opens a a directory
 *              on a disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int closedir(DIR *directory)
{
    int rc;
#ifndef _AIX
    cflsh_usfs_DIR_t *usfs_directory = (cflsh_usfs_DIR_t *)directory;
#else
    DIR *usfs_directory = directory;
#endif
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(usfs_directory->dd_fd);

    if (fd_wrapper) {

	usfs_directory->dd_fd = fd_wrapper->usfs_fd;

	rc = cusfs_closedir((DIR *)usfs_directory);

	if (!rc) {

	    cufs_free_fd_wrapper(fd_wrapper);
	}
    } else {


	real_closedir = dlsym(RTLD_NEXT,"closedir");

	if (real_closedir) {

	    rc = real_closedir(directory);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
    
}
#endif /* !_AIX */


#ifdef _AIX
/*
 * NAME:        readdir64_r
 *
 * FUNCTION:    Returns a pointer to the next directory entry.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int readdir64_r(DIR64 *directory, struct dirent64 *entry,struct dirent64 **result)
{
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    DIR64 usfs_directory;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(directory->dd_fd);

    if (fd_wrapper) {

	usfs_directory = *directory;

	usfs_directory.dd_fd = fd_wrapper->usfs_fd;
	rc = cusfs_readdir64_r(&usfs_directory,entry,result);
	*directory = usfs_directory;
	directory->dd_fd = fd_wrapper->fd;
    } else {


	real_readdir64_r = (int(*)(struct _dirdesc64*,struct dirent64*,struct dirent64**))dlsym(RTLD_NEXT,"readdir64_r");

	if (real_readdir64_r) {

	    rc = real_readdir64_r(directory,entry,result);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        readdir64
 *
 * FUNCTION:    Returns a pointer to the next directory entry.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
struct dirent64 *readdir64(DIR64 *directory)
{
    struct dirent64 *result = NULL;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    DIR64 usfs_directory;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(directory->dd_fd);

    if (fd_wrapper) {


	usfs_directory = *directory;

	usfs_directory.dd_fd = fd_wrapper->usfs_fd;
	result = cusfs_readdir64(&usfs_directory);
	usfs_directory.dd_fd = fd_wrapper->fd;
	*directory = usfs_directory;

    } else {


	real_readdir64 = (struct dirent64 *(*)(DIR64 *))dlsym(RTLD_NEXT,"readdir64");

	if (real_readdir64) {

	    result = real_readdir64(directory);
	} else {

	    errno = EBADF;
	    result = NULL;
	}
    }

    return result;
}

#else
/*
 * NAME:        readdir_r
 *
 * FUNCTION:    Returns a pointer to the next directory entry.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int readdir_r(DIR *directory, struct dirent *entry,struct dirent **result)
{
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
#ifndef _AIX
    cflsh_usfs_DIR_t usfs_directory;
    cflsh_usfs_DIR_t *usfs_directory2 = (cflsh_usfs_DIR_t *)directory;
#else
    DIR usfs_directory;
#endif

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(usfs_directory2->dd_fd);

    if (fd_wrapper) {


#ifndef _AIX
	usfs_directory = *usfs_directory2;
#else
	usfs_directory = *directory;
#endif 

	
	usfs_directory.dd_fd = fd_wrapper->usfs_fd;
	rc = cusfs_readdir_r((DIR *)&usfs_directory,entry,result);

	usfs_directory.dd_fd = fd_wrapper->fd;
#ifndef _AIX
	*usfs_directory2 = usfs_directory;
#else
	*directory = usfs_directory;
#endif 

    } else {


	real_readdir_r = dlsym(RTLD_NEXT,"readdir_r");

	if (real_readdir_r) {

	    rc = real_readdir_r(directory,entry,result);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


#endif

/*
 * NAME:        readdir
 *
 * FUNCTION:    Returns a pointer to the next directory entry.
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
struct dirent *readdir(DIR *directory)
{
    struct dirent *result = NULL;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
#ifndef _AIX
    cflsh_usfs_DIR_t usfs_directory;
    cflsh_usfs_DIR_t *usfs_directory2 = (cflsh_usfs_DIR_t *)directory;
#else
    DIR usfs_directory;
    DIR *usfs_directory2 = directory;
#endif

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(usfs_directory2->dd_fd);

    if (fd_wrapper) {


	usfs_directory = *usfs_directory2;

	
	usfs_directory.dd_fd = fd_wrapper->usfs_fd;

	result = cusfs_readdir((DIR *)&usfs_directory);

	usfs_directory.dd_fd = fd_wrapper->fd;
#ifndef _AIX
	*usfs_directory2 = usfs_directory;
#else
	*directory = usfs_directory;
#endif 

    } else {


	real_readdir = (struct dirent *(*)(DIR *))dlsym(RTLD_NEXT,"readdir");

	if (real_readdir) {

	    result = real_readdir(directory);
	} else {

	    errno = EBADF;
	    result = NULL;
	}
    }

    return result;
}


/*
 * NAME:        rename
 *
 * FUNCTION:    Rename a file/directory to another name
 *              in the same filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int rename(const char *from_pathname,
	   const char *to_pathname)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)from_pathname)) {

	rc = cusfs_rename((char *)from_pathname,(char *)to_pathname);

	
    } else {

	real_rename = (int(*)(char*,char*))dlsym(RTLD_NEXT,"rename");

	if (real_rename) {

	    rc = real_rename((char *)from_pathname,(char *)to_pathname);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        creat
 *
 * FUNCTION:    Wrapper for creat
 *              
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
        
int creat(const char *path, mode_t mode)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_creat((char *)path,mode);

	if (rc >= 0) {


	    if (cufs_alloc_fd_wrapper(&rc)) {

		rc = -1;
	    }
	}
	
    } else {

	real_creat = (int(*)(char*,unsigned int))dlsym(RTLD_NEXT,"creat");

	if (real_creat) {

	    rc = real_creat((char *)path,mode);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;

}

/*
 * NAME:        creat64
 *
 * FUNCTION:    Wrapper for creat64
 *              
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
        
int creat64(const char *path, mode_t mode)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_creat((char *)path,mode);

	if (rc >= 0) {


	    if (cufs_alloc_fd_wrapper(&rc)) {

		rc = -1;
	    }
	}
	
    } else {

	real_creat = (int(*)(char*,unsigned int))dlsym(RTLD_NEXT,"creat64");

	if (real_creat) {

	    rc = real_creat((char *)path,mode);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;

}

/*
 * NAME:        open
 *
 * FUNCTION:    Wrapper for open
 *              
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
        
int open(const char *path, int flags, ...)
{
    int rc;
    mode_t mode;
    va_list ap;

    /*
     * open has a most 3 arguments.
     * So we only need to try to extract
     * one argument here. If that argument
     * exists it will be of type mode_t.
     */

    va_start(ap,flags);
    mode = va_arg(ap,mode_t);
    va_end(ap);

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_open((char *)path,flags,mode);
	
	if (rc >= 0) {

	    if (cufs_alloc_fd_wrapper(&rc)) {

		rc = -1;
	    }
	}
	
    } else {

	real_open = (int(*)(char*,int,...))dlsym(RTLD_NEXT,"open");

	if (real_open) {

	    rc = real_open((char *)path,flags,mode);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;

}

/*
 * NAME:        open64
 *
 * FUNCTION:    Wrapper for open64
 *              
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
        
int open64(const char *path, int flags, ...)
{
    int rc;
    mode_t mode;
    va_list ap;

    /*
     * open has a most 3 arguments.
     * So we only need to try to extract
     * one argument here. If that argument
     * exists it will be of type mode_t.
     */

    va_start(ap,flags);
    mode = va_arg(ap,mode_t);
    va_end(ap);

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_open((char *)path,flags,mode);
	
	if (rc >= 0) {

	    if (cufs_alloc_fd_wrapper(&rc)) {

		rc = -1;
	    }
	}
	
    } else {

	real_open = (int(*)(char*,int,...))dlsym(RTLD_NEXT,"open64");

	if (real_open) {

	    rc = real_open((char *)path,flags,mode);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;

}

/*
 * NAME:        close
 *
 * FUNCTION:    Wrapper for close
 *              
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 *              
 */
        
int close(int fd)
{
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_close(fd_wrapper->usfs_fd);

	if (!rc) {

	    cufs_free_fd_wrapper(fd_wrapper);
	}
    } else {


	real_close = (int(*)(int))dlsym(RTLD_NEXT,"close");

	if (real_close) {

	    rc = real_close(fd);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
    
}


/*
 * NAME:        read
 *
 * FUNCTION:    Read a file of the specified size.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

ssize_t read(int fd, void *buffer,size_t nbytes)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_read(fd_wrapper->usfs_fd,buffer,nbytes);

    } else {

	real_read = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT,"read");

	if (real_read) {

	    rc = real_read(fd,buffer,nbytes);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        write
 *
 * FUNCTION:    Write a file of the specified size.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

ssize_t write(int fd, const void *buffer,size_t nbytes)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_write(fd_wrapper->usfs_fd,(void *)buffer,nbytes);

    } else {


	real_write = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT,"write");

	if (real_write) {

	    rc = real_write(fd,(void *)buffer,nbytes);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}



/*
 * NAME:        fsync
 *
 * FUNCTION:    sync a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int fsync(int fd)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fsync(fd_wrapper->usfs_fd);

    } else {


	real_fsync = (int(*)(int))dlsym(RTLD_NEXT,"fsync");

	if (real_fsync) {

	    rc = real_fsync(fd);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        lseek
 *
 * FUNCTION:    Seek to a specified offset in the file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

off_t lseek(int fd, off_t offset, int whence)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_lseek(fd_wrapper->usfs_fd,offset,whence);

    } else {


	real_lseek = (off_t(*)(int,off_t,int))dlsym(RTLD_NEXT,"lseek");

	if (real_lseek) {

	    rc = real_lseek(fd,offset,whence);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#ifdef _AIX
/*
 * NAME:        llseek
 *
 * FUNCTION:    Seek to a specified offset in the file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

offset_t llseek(int fd, offset_t offset, int whence)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_llseek(fd_wrapper->usfs_fd,offset,whence);

    } else {


	real_llseek = (offset_t(*)(int,offset_t,int))dlsym(RTLD_NEXT,"llseek");

	if (real_llseek) {

	    rc = real_llseek(fd,offset,whence);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#endif /* AIX */

/*
 * NAME:        lseek64
 *
 * FUNCTION:    Seek to a specified offset in the file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

off64_t lseek64(int fd, off64_t offset, int whence)
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_lseek64(fd_wrapper->usfs_fd,offset,whence);

    } else {


	real_lseek64 = (off64_t(*)(int,off64_t,int))dlsym(RTLD_NEXT,"lseek64");

	if (real_lseek64) {

	    rc = real_lseek64(fd,offset,whence);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}



/*
 * NAME:        fstat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fstat(int fd,struct stat *buffer)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fstat = (int(*)(int,struct stat *))dlsym(RTLD_NEXT,"fstat");

	if (real_fstat) {

	    rc = real_fstat(fd,buffer);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}
#ifdef _AIX

/*
 * NAME:        fstatx
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fstatx(int fd,struct stat *buffer, int length, int command)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fstatx = (int(*)(int,struct stat *, int, int))dlsym(RTLD_NEXT,"fstatx");

	if (real_fstatx) {

	    rc = real_fstatx(fd,buffer,length,command);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}
/*
 * NAME:        fstat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fstat64(int fd,struct stat64 *buffer)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat64(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fstat64 = (int(*)(int,struct stat64 *))dlsym(RTLD_NEXT,"fstat64");

	if (real_fstat64) {

	    rc = real_fstat64(fd,buffer);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        fstat64x
 *
 * FUNCTION:    Returns fstat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fstat64x(int fd,struct stat64x *buffer)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat64x(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fstat64x = (int(*)(int,struct stat64x *))dlsym(RTLD_NEXT,"fstat64x");

	if (real_fstat64x) {

	    rc = real_fstat64x(fd,buffer);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#endif /* _AIX */

#ifdef _AIX
/*
 * NAME:        stat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int stat64(const char *path, struct stat64 *buffer)
#else
/*
 * NAME:        stat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int stat(const char *path, struct stat *buffer)
#endif
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {
#ifdef _AIX
	rc = cusfs_stat64((char *)path,buffer);
#else
	rc = cusfs_stat((char *)path,buffer);
#endif
	
	
    } else {
#ifdef _AIX

	real_stat64 = (int(*)(char *,struct stat64 *))dlsym(RTLD_NEXT,"stat64");

	if (real_stat64) {

	    rc = real_stat64((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
#else

	real_stat = (int(*)(char *,struct stat *))dlsym(RTLD_NEXT,"stat");

	if (real_stat) {

	    rc = real_stat((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

#endif

    }

    return rc;
}


#ifdef _AIX


/*
 * NAME:        statx
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int statx(const char *path, struct stat *buffer, int length, int command)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_stat((char *)path,buffer);

	
	
    } else {

	real_statx = (int(*)(char *,struct stat *,int, int))dlsym(RTLD_NEXT,"statx");

	if (real_statx) {

	    rc = real_statx((char *)path,buffer,length,command);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;
}
/*
 * NAME:        stat64x
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int stat64x(const char *path, struct stat64x *buffer)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_stat64x((char *)path,buffer);

	
	
    } else {


	/*
	 * Reuse same real_stat64x for both stat64x and lstat64x
	 */

	real_stat64x = (int(*)(char *,struct stat64x *))dlsym(RTLD_NEXT,"stat64x");

	if (real_stat64x) {

	    rc = real_stat64x((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;


}

/*
 * NAME:        lstat64x
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int lstat64x(const char *path, struct stat64x *buffer)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_lstat64x((char *)path,buffer);

	
	
    } else {


	/*
	 * Reuse same real_stat64x for both stat64x and lstat64x
	 */

	real_stat64x = (int(*)(char *,struct stat64x *))dlsym(RTLD_NEXT,"lstat64x");

	if (real_stat64x) {

	    rc = real_stat64x((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;


}

#endif /* _AIX */


#ifdef _AIX
/*
 * NAME:        lstat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int lstat64(const char *path, struct stat64 *buffer)
#else
/*
 * NAME:        lstat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int lstat(const char *path, struct stat *buffer)
#endif
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {
#ifdef _AIX
	rc = cusfs_lstat64((char *)path,buffer);
#else
	rc = cusfs_lstat((char *)path,buffer);
#endif
	
	
    } else {
#ifdef _AIX

	/*
	 * Reuse same real_stat64 for both stat64 and lstat64
	 */

	real_stat64 = (int(*)(char *,struct stat64 *))dlsym(RTLD_NEXT,"lstat64");

	if (real_stat64) {

	    rc = real_stat64((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
#else

	/*
	 * Reuse same real_stat for both stat and lstat
	 */

	real_stat = (int(*)(char *,struct stat *))dlsym(RTLD_NEXT,"lstat");

	if (real_stat) {

	    rc = real_stat((char *)path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

#endif

    }

    return rc;
}

#ifndef _AIX
/*
 * NAME:        __xstat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int __xstat(int version, const char *path, struct stat *buffer)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_stat((char *)path,buffer);
	
	
    } else {

	real_xstat = (int(*)(int, const char *,struct stat *))dlsym(RTLD_NEXT,"__xstat");

	if (real_xstat) {

	    rc = real_xstat(version,path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;
}

/*
 * NAME:        __lxstat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int __lxstat(int version, const char *path, struct stat *buffer)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_lstat((char *)path,buffer);
	
	
    } else {

	real_xstat = (int(*)(int, const char *,struct stat *))dlsym(RTLD_NEXT,"__lxstat");

	if (real_xstat) {

	    rc = real_xstat(version,path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;
}


/*
 * NAME:        __fxstat
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int __fxstat(int version,int fd,struct stat *buffer)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fxstat = (int(*)(int,int,struct stat *))dlsym(RTLD_NEXT,"__fxstat");

	if (real_fxstat) {

	    rc = real_fxstat(version,fd,buffer);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        __xstat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int __xstat64(int version, const char *path, struct stat64 *buffer)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_stat64((char *)path,buffer);
	
	
    } else {

	real_xstat64 = (int(*)(int, const char *,struct stat64 *))dlsym(RTLD_NEXT,"__xstat64");

	if (real_xstat64) {

	    rc = real_xstat64(version,path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;
}

/*
 * NAME:        __lxstat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */
int __lxstat64(int version, const char *path, struct stat64 *buffer)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_lstat64((char *)path,buffer);
	
	
    } else {

	real_xstat64 = (int(*)(int, const char *,struct stat64 *))dlsym(RTLD_NEXT,"__lxstat64");

	if (real_xstat64) {

	    rc = real_xstat64(version,path,buffer);

	} else {

	    errno = EBADF;
	    rc = -1;
	}

    }

    return rc;
}


/*
 * NAME:        __fxstat64
 *
 * FUNCTION:    Returns stat 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int __fxstat64(int version,int fd,struct stat64 *buffer)
{  
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fstat64(fd_wrapper->usfs_fd,buffer);

    } else {


	real_fxstat64 = (int(*)(int,int,struct stat64 *))dlsym(RTLD_NEXT,"__fxstat64");

	if (real_fxstat64) {

	    rc = real_fxstat64(version,fd,buffer);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

#endif /* !_AIX */


/*
 * NAME:        readlink
 *
 * FUNCTION:    Read the symlink filename
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

#ifdef _AIX
int 
#else
ssize_t
#endif
readlink(const char *path, char *buffer, size_t buffer_size)

{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_readlink((char *)path,buffer,buffer_size);
	
    } else {


	/*
	 * Reuse same real_stat for both stat and lstat
	 */

#ifdef _AIX
	real_readlink = (int(*)(char *,char *,size_t))dlsym(RTLD_NEXT,"readlink");
#else
	real_readlink = (ssize_t(*)(char *,char *,size_t))dlsym(RTLD_NEXT,"readlink");
#endif

	if (real_readlink) {

	    rc = real_readlink((char *)path,buffer, buffer_size);

	} else {

	    errno = EBADF;
	    rc = -1;
	}



    }

    return rc;
}


/*
 * NAME:        access
 *
 * FUNCTION:    Validate access to file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int access(const char *path, int mode)
{ 
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_access((char *)path,mode);

	
    } else {

	real_access = (int(*)(char *,int))dlsym(RTLD_NEXT,"access");

	if (real_access) {

	    rc = real_access((char *)path,mode);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        fchmod
 *
 * FUNCTION:    Change mode of file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fchmod(int fd,mode_t mode)
{   
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fchmod(fd_wrapper->usfs_fd,mode);

    } else {


	real_fchmod = (int(*)(int,mode_t))dlsym(RTLD_NEXT,"fchmod");

	if (real_fchmod) {

	    rc = real_fchmod(fd,mode);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
} 

/*
 * NAME:        fchown
 *
 * FUNCTION:    Change owner/group
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int fchown(int fd, uid_t uid, gid_t gid)
{   
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_fchown(fd_wrapper->usfs_fd,uid,gid);

    } else {


	real_fchown = (int(*)(int,uid_t,gid_t))dlsym(RTLD_NEXT,"fchown");

	if (real_fchown) {

	    rc = real_fchown(fd,uid,gid);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
} 

/*
 * NAME:        ftruncate
 *
 * FUNCTION:    Change the length of a regular file
 *              to the length specified by the caller.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int ftruncate(int fd, off_t length) 
{   
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_ftruncate(fd_wrapper->usfs_fd,length);

    } else {


	real_ftruncate = (int(*)(int,off_t))dlsym(RTLD_NEXT,"ftruncate");

	if (real_ftruncate) {

	    rc = real_ftruncate(fd,length);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
} 
 

/*
 * NAME:        ftruncate64
 *
 * FUNCTION:    Change the length of a regular file
 *              to the length specified by the caller.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int ftruncate64(int fd, off64_t length) 
{   
    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(fd);

    if (fd_wrapper) {

	rc = cusfs_ftruncate64(fd_wrapper->usfs_fd,length);

    } else {


	real_ftruncate64 = (int(*)(int,off64_t))dlsym(RTLD_NEXT,"ftruncate64");

	if (real_ftruncate64) {

	    rc = real_ftruncate64(fd,length);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
} 
 

/*
 * NAME:        link
 *
 * FUNCTION:    link a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int link(const char *orig_path, const char *path)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_link((char *)orig_path,(char *)path);

	
    } else {

	real_link = (int(*)(char*,char *))dlsym(RTLD_NEXT,"link");

	if (real_link) {

	    rc = real_link((char *)orig_path,(char *)path);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        unlink
 *
 * FUNCTION:    Unlink a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int unlink(const char *path)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_unlink((char *)path);

	
    } else {

	real_unlink = (int(*)(char*))dlsym(RTLD_NEXT,"unlink");

	if (real_unlink) {

	    rc = real_unlink((char *)path);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        utime
 *
 * FUNCTION:    set acces and modification time of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int utime(const char *path,const struct utimbuf *times)
{
    int rc;

    if (CUSFS_IS_USFS_PATH((char *)path)) {

	rc = cusfs_utime((char *)path,(struct utimbuf*)times);

	
    } else {

	real_utime = (int(*)(char*,struct utimbuf*))dlsym(RTLD_NEXT,"utime");

	if (real_utime) {

	    rc = real_utime((char *)path,(struct utimbuf*)times);

	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NOTE: For AIO interfaces below, AIX has two interfaces:
 *       Legacy and POSIX.  For Linux there is just posix.
 *       For AIX the POSIX has function names with the 
 *       prefix "_posix", with legacy without them.
 *       Linux's POSIX AIO does not have this posix
 *       prefix naming convention. We are only supporting
 *       POSIX AIO here for both AIX and Linux.
 */


/*
 * NAME:        aio_read64
 *
 * FUNCTION:    Issue an asynchronous read
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_read64(struct aiocb64 *aiocbp)
#else
int aio_read64(struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {


	/*
	 * Set initial state of AIOCB
	 */

	CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_read64(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_read64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_read64");
#else
	real_aio_read64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"aio_read64");
#endif

	if (real_aio_read64) {

	    rc = real_aio_read64(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_read
 *
 * FUNCTION:    Issue an asynchronous read
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

#ifdef _AIX
int _posix_aio_read(struct aiocb *aiocbp)
#else
int aio_read(struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {


	/*
	 * Set initial state of AIOCB
	 */

	CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = aiocbp;
#endif

	rc = cusfs_aio_read(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_read = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_read");
#else
	real_aio_read = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"aio_read");
#endif

	if (real_aio_read) {

	    rc = real_aio_read(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_write64
 *
 * FUNCTION:    Issue an asynchronous write
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_write64(struct aiocb64 *aiocbp)
#else
int aio_write64(struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;



    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {


	/*
	 * Set initial state of AIOCB
	 */

	CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif
	rc = cusfs_aio_write64(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_write64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_write64");
#else
	real_aio_write64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"aio_write64");
#endif

	if (real_aio_write64) {

	    rc = real_aio_write64(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_write
 *
 * FUNCTION:    Issue an asynchronous write
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_write(struct aiocb *aiocbp)
#else
int aio_write(struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {


	/*
	 * Set initial state of AIOCB
	 */

	CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = aiocbp;
#endif

	rc = cusfs_aio_write(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_write = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_write");
#else
	real_aio_write = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"aio_write");
#endif

	if (real_aio_write) {

	    rc = real_aio_write(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        aio_error64
 *
 * FUNCTION:    Check status on an aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_error64(struct aiocb64 *aiocbp)
#else
int aio_error64(const struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_error64(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_error64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_error64");
#else
	real_aio_error64 = (int(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"aio_error64");
#endif

	if (real_aio_error64) {

	    rc = real_aio_error64((struct aiocb64 *)aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        aio_error
 *
 * FUNCTION:    Check status on an aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_error(struct aiocb *aiocbp)
#else
int aio_error(const struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_error(&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_error = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_error");
#else
	real_aio_error = (int(*)(struct aiocb *))dlsym(RTLD_NEXT,"aio_error");
#endif

	if (real_aio_error) {

	    rc = real_aio_error((struct aiocb *)aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        aio_return64
 *
 * FUNCTION:    Return complete aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
ssize_t _posix_aio_return64(struct aiocb64 *aiocbp)
#else
ssize_t aio_return64(struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_return64(&aiocb_arg);

	/*
	 * Set errno and return code
	 * in aiocb here.
	 */

#ifdef _AIX
	CUSFS_RET_AIOCB(aiocbp,aiocb_arg.aio_return,aiocb_arg.aio_errno);
 
#else
	CUSFS_RET_AIOCB(aiocbp,aiocb_arg.__return_value,aiocb_arg.__error_code);

#endif /* _AIX */

    } else {

#ifdef _AIX
	real_aio_return64 = (ssize_t(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_return64");
#else
	real_aio_return64 = (ssize_t(*)(struct aiocb64 *))dlsym(RTLD_NEXT,"aio_return64");
#endif

	if (real_aio_return64) {

	    rc = real_aio_return64(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_return
 *
 * FUNCTION:    Return complete aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
ssize_t _posix_aio_return(struct aiocb *aiocbp)
#else
ssize_t aio_return(struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = aiocbp;
#endif

	rc = cusfs_aio_return(&aiocb_arg);

#ifdef _AIX
	CUSFS_RET_AIOCB(aiocbp,aiocb_arg.aio_return,aiocb_arg.aio_errno);
 
#else
	CUSFS_RET_AIOCB(aiocbp,aiocb_arg.__return_value,aiocb_arg.__error_code);

#endif /* _AIX */
    } else {

#ifdef _AIX
	real_aio_return = (size_t(*)(struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_return");
#else
	real_aio_return = (size_t(*)(struct aiocb *))dlsym(RTLD_NEXT,"aio_return");
#endif

	if (real_aio_return) {

	    rc = real_aio_return(aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        aio_fsync64
 *
 * FUNCTION:    Sync aio
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_fsync64(int op,struct aiocb64 *aiocbp)
#else
int aio_fsync64(int op,struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_fsync64(op,&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_fsync64 = (int(*)(int, struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_fsync64");
#else
	real_aio_fsync64 = (int(*)(int, struct aiocb64 *))dlsym(RTLD_NEXT,"aio_fsync64");
#endif

	if (real_aio_fsync64) {

	    rc = real_aio_fsync64(op,aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_fsync
 *
 * FUNCTION:    Sync aio
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_fsync(int op,struct aiocb *aiocbp)
#else
int aio_fsync(int op,struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = aiocbp;
#endif

	rc = cusfs_aio_fsync(op,&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_fsync = (int(*)(int,struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_fsync");
#else
	real_aio_fsync = (int(*)(int,struct aiocb *))dlsym(RTLD_NEXT,"aio_fsync");
#endif

	if (real_aio_fsync) {

	    rc = real_aio_fsync(op,aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_cancel64
 *
 * FUNCTION:    Cancel an iocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_cancel64(int fd, struct aiocb64 *aiocbp)
#else
int aio_cancel64(int fd, struct aiocb64 *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb64 aiocb_arg;

    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = (struct aiocb *)aiocbp;
#endif

	rc = cusfs_aio_cancel64(fd,&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_cancel64 = (int(*)(int, struct aiocb64 *))dlsym(RTLD_NEXT,"_posix_aio_cancel64");
#else
	real_aio_cancel64 = (int(*)(int, struct aiocb64 *))dlsym(RTLD_NEXT,"aio_cancel64");
#endif

	if (real_aio_cancel64) {

	    rc = real_aio_cancel64(fd,aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}


/*
 * NAME:        aio_cancel
 *
 * FUNCTION:    Cancel an aiocb.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_cancel(int fd, struct aiocb *aiocbp)
#else
int aio_cancel(int fd, struct aiocb *aiocbp)
#endif
{

    int rc;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    struct aiocb aiocb_arg;


    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(aiocbp->aio_fildes);

    if (fd_wrapper) {

	aiocb_arg = *aiocbp;
	aiocb_arg.aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
	aiocb_arg.aio_handle = (aio_handle_t)aiocbp;
#else
	aiocb_arg.__next_prio = aiocbp;
#endif

	rc = cusfs_aio_cancel(fd,&aiocb_arg);

    } else {

#ifdef _AIX
	real_aio_cancel = (int(*)(int, struct aiocb *))dlsym(RTLD_NEXT,"_posix_aio_cancel");
#else
	real_aio_cancel = (int(*)(int, struct aiocb *))dlsym(RTLD_NEXT,"aio_cancel");
#endif

	if (real_aio_cancel) {

	    rc = real_aio_cancel(fd,aiocbp);
	} else {

	    errno = EBADF;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        aio_suspend64
 *
 * FUNCTION:    Wait for an aiocb to complete.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_suspend64(const struct aiocb64 *const list[], int nent,
                        const struct timespec *timeout)
#else
int aio_suspend64(const struct aiocb64 *const list[], int nent,
                        const struct timespec *timeout)
#endif
{
    int rc;
    int i,j;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    int num_usfs_aiocbs = 0;
    int num_other_aiocbs = 0;
    struct aiocb64 **usfs_list = NULL;
    struct aiocb64 *aiocb_list;

    if (nent == 0) {

	return 0;
    }

#ifdef _AIX
    real_aio_suspend64 = (int(*)(const struct aiocb64 *const list[], int,
				     const struct timespec *))dlsym(RTLD_NEXT,"_posix_aio_suspend64");
#else
    real_aio_suspend64 = (int(*)(const struct aiocb64 *const list[], int,
				     const struct timespec *))dlsym(RTLD_NEXT,"aio_suspend64");
#endif

    for (i = 0; i < nent; i++) {

	if (list[i] != NULL) {
	    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);
	    
	    if (fd_wrapper) {
		num_usfs_aiocbs++;
	    } else {
		num_other_aiocbs++;
	    }
	}

    }

    if (num_other_aiocbs == 0) {

	/*
	 * If everything is for USFS, 
	 * then handle it. Thus we need
	 * to create a duplicate list of aiocbs, but
	 * each with the correct file descriptor for 
	 * this library.
	 */

	usfs_list = malloc(sizeof(struct aiocb64*) * num_usfs_aiocbs);

	if (usfs_list == NULL) {

	    errno = ENOMEM;
	    return -1;
	}

	aiocb_list = malloc(sizeof(struct aiocb64) * num_usfs_aiocbs);

	if (aiocb_list == NULL) {

	    errno = ENOMEM;

	    free(usfs_list);
	    return -1;
	}


	for (i = 0; i < num_usfs_aiocbs; i++) {

	    if (list[i] != NULL) {
	    
		usfs_list[i] = &(aiocb_list[i]);

		aiocb_list[i] = *(list[i]);

		fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);

		if (fd_wrapper == NULL) {


		    free(usfs_list);
		    free(aiocb_list);
		    return -1;
		}

		aiocb_list[i].aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
		aiocb_list[i].aio_handle = (aio_handle_t)(list[i]);
#else
		aiocb_list[i].__next_prio = (struct aiocb *)(list[i]);
#endif

	    }
	}


	rc = cusfs_aio_suspend64((const struct aiocb64 * const*)usfs_list,num_usfs_aiocbs,timeout);

	
	free(usfs_list);
	free(aiocb_list);

    } else if (num_usfs_aiocbs == 0) {

	/*
	 * If everything is non-USFS aiocbs, we route
	 * everything through to the non-USFS AIO.
	 */
	if (real_aio_suspend64) {

	    rc = real_aio_suspend64(list,nent,timeout);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    } else {

	struct aiocb64 *non_usfs_list[num_other_aiocbs];
	/*
	 * If there is a mixture of both, then we only 
	 * pass thru the non-USFS ones. This is not
	 * ideal, but the hope is that waiting on these
	 * will be sufficient for some USFS ones to complete.
	 */
	
	for (i = 0, j = 0; i < nent; i++) {
	    
	    if (list[i] != NULL) {
		fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);
		
		if (fd_wrapper == NULL) {
		    non_usfs_list[j++] = (struct aiocb64 *)list[i];
		}
	    }
	}

	if (real_aio_suspend64) {

	    rc = real_aio_suspend64((const struct aiocb64 * const*)non_usfs_list,num_other_aiocbs,timeout);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

	
    }

    return rc;
}

/*
 * NAME:        aio_suspend
 *
 * FUNCTION:    Wait for an aiocb to complete.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
#ifdef _AIX
int _posix_aio_suspend(const struct aiocb *const list[], int nent,
                        const struct timespec *timeout)
#else
int aio_suspend(const struct aiocb *const list[], int nent,
                        const struct timespec *timeout)
#endif
{

    int rc;
    int i,j;
    cflsh_usfs_wrap_fd_t *fd_wrapper;
    int num_usfs_aiocbs = 0;
    int num_other_aiocbs = 0;
    struct aiocb **usfs_list = NULL;
    struct aiocb *aiocb_list;

    if (nent == 0) {

	return 0;
    }


#ifdef _AIX
    real_aio_suspend = (int(*)(const struct aiocb *const list[], int,
				     const struct timespec *))dlsym(RTLD_NEXT,"_posix_aio_suspend");
#else
    real_aio_suspend = (int(*)(const struct aiocb *const list[], int,
				     const struct timespec *))dlsym(RTLD_NEXT,"aio_suspend");
#endif

    for (i = 0; i < nent; i++) {

	if (list[i] != NULL) {
	    fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);
	    
	    if (fd_wrapper) {
		num_usfs_aiocbs++;
	    } else {
		num_other_aiocbs++;
	    }
	}
    }

    if (num_other_aiocbs == 0) {

	/*
	 * If everything is for USFS, 
	 * then handle it. Thus we need
	 * to create a duplicate list of aiocbs, but
	 * each with the correct file descriptor for 
	 * this library.
	 */

	usfs_list = malloc(sizeof(struct aiocb*) * num_usfs_aiocbs);

	if (usfs_list == NULL) {

	    errno = ENOMEM;
	    return -1;
	}

	aiocb_list = malloc(sizeof(struct aiocb) * num_usfs_aiocbs);

	if (aiocb_list == NULL) {

	    errno = ENOMEM;

	    free(usfs_list);
	    return -1;
	}


	for (i = 0; i < num_usfs_aiocbs; i++) {

	    if (list[i] != NULL) {
	    
		usfs_list[i] = &(aiocb_list[i]);

		aiocb_list[i] = *(list[i]);

		fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);

		if (fd_wrapper == NULL) {


		    free(usfs_list);
		    free(aiocb_list);
		    return -1;
		}

		aiocb_list[i].aio_fildes = fd_wrapper->usfs_fd;
#ifdef _AIX
		aiocb_list[i].aio_handle = (aio_handle_t)(list[i]);
#else
		aiocb_list[i].__next_prio = (struct aiocb *)(list[i]);
#endif


	    }
	}


	rc = cusfs_aio_suspend((const struct aiocb * const*)usfs_list,num_usfs_aiocbs,timeout);

	
	free(usfs_list);
	free(aiocb_list);

    } else if (num_usfs_aiocbs == 0) {

	/*
	 * If everything is non-USFS aiocbs, we route
	 * everything through to the non-USFS AIO.
	 */
	if (real_aio_suspend) {

	    rc = real_aio_suspend(list,nent,timeout);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

    } else {

	struct aiocb *non_usfs_list[num_other_aiocbs];
	/*
	 * If there is a mixture of both, then we only 
	 * pass thru the non-USFS ones. This is not
	 * ideal, but the hope is that waiting on these
	 * will be sufficient for some USFS ones to complete.
	 */
	
	for (i = 0, j = 0; i < nent; i++) {
	    
	    if (list[i] != NULL) {
		fd_wrapper = CUSFS_GET_WRAPPER_FD_HASH(list[i]->aio_fildes);
		
		if (fd_wrapper == NULL) {
		    non_usfs_list[j++] = (struct aiocb *)list[i];
		}
	    }
	}

	if (real_aio_suspend) {

	    rc = real_aio_suspend((const struct aiocb * const*)non_usfs_list,num_other_aiocbs,timeout);
	} else {

	    errno = EBADF;
	    rc = -1;
	}

	
    }

    return rc;
}

