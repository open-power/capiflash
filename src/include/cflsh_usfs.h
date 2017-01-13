/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_ufs.h 1.6  */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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
/* %Z%%M%       %I%  %W% %G% %U% */


/*
 * COMPONENT_NAME: (sysxcflashufs) CAPI Flash user space file system library
 *
 * FUNCTIONS: API for users of this library.
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

#ifndef _H_CFLASH_USFS
#define _H_CFLASH_USFS


#include <sys/stat.h>
#ifdef _AIX
#include <sys/access.h>
#endif /* AIX */


#define CUSFS_DISK_NAME_DELIMINATOR ":"
/*
 * Typical CAPI Flash User Space File System 
 * API.
 */
int cusfs_init(void *arg,uint64_t flags);
int cusfs_term(void *arg,uint64_t flags);
int cusfs_creat(char *path, mode_t mode);
int cusfs_open(char *path, int flags, mode_t mode);
int cusfs_close(int fd);
ssize_t cusfs_read(int fd, void *buffer,size_t nbytes);
ssize_t cusfs_write(int fd, void *buffer,size_t nbytes);
int cusfs_fsync(int fd);
off_t cusfs_lseek(int fd, off_t Offset, int Whence);
off64_t cusfs_lseek64(int fd, off64_t Offset, int Whence);
#ifdef _AIX
offset_t cusfs_llseek(int fd, offset_t Offset ,int Whence);
#endif /* _AIX */

int cusfs_fstat(int fd,struct stat *Buffer);
off_t cusfs_lseek(int fd, off_t Offset, int Whence);

int cusfs_fstat64(int fd,struct stat64 *Buffer);

#ifdef _AIX
int cusfs_fstat64x(int fd,struct stat64x *Buffer);
int cusfs_stat64x(char *path,struct stat64x *Buffer);
int cusfs_lstat64x(char *path,struct stat64x *Buffer);
#endif /* _AIX */

int cusfs_stat64(char *path,struct stat64 *Buffer);
int cusfs_lstat64(char *path,struct stat64 *Buffer);

int cusfs_fstat(int fd,struct stat *Buffer);
int cusfs_stat(char *path,struct stat *Buffer);
int cusfs_lstat(char *path,struct stat *Buffer);

int cusfs_readlink(const char *path, char *buffer, size_t buffer_size);
int cusfs_access(char *path, int mode);
int cusfs_fchmod(int fd, mode_t mode);
int cusfs_fchown(int fd, uid_t uid, gid_t group);
int cusfs_ftruncate(int fd, off_t length);
int cusfs_ftruncate64(int fd, off64_t length);
int cusfs_posix_fadvise(int fd, off_t offset, off_t length, int advise);
#ifndef _AIX
int cusfs_posix_fadvise64(int fd, off64_t offset, off64_t length, int advise);
#endif /* !_AIX */

int cusfs_link(char *path1, char *path2);
int cusfs_unlink(char *path);
int cusfs_utime(char *path,struct utimbuf *times);
#ifdef _NOT_YET
int cusfs_futimens(int fd,struct timespec *times)
#endif
int cusfs_aio_read64(struct aiocb64 *aiocbp);
int cusfs_aio_read(struct aiocb *aiocbp);
int cusfs_aio_write64(struct aiocb64 *aiocbp);
int cusfs_aio_write(struct aiocb *aiocbp);
int cusfs_aio_error64(struct aiocb64 *aiocbp);
int cusfs_aio_error(struct aiocb *aiocbp);
int cusfs_aio_return64(struct aiocb64 *aiocbp);
int cusfs_aio_return(struct aiocb *aiocbp);
int cusfs_aio_fsync64(int op,struct aiocb64 *aiocbp);
int cusfs_aio_fsync(int op,struct aiocb *aiocbp);
int cusfs_aio_cancel64(int fd, struct aiocb64 *aiocbp);
int cusfs_aio_cancel(int fd, struct aiocb *aiocbp);
int cusfs_aio_suspend64(const struct aiocb64 *const list[], int nent,
                        const struct timespec *timeout);
int cusfs_aio_suspend(const struct aiocb *const list[], int nent,
                        const struct timespec *timeout);
#endif /* _H_CFLASH_USFS */
