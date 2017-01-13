/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_usfs_wrapper.h 1.1     */
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
 * COMPONENT_NAME: (sysxcflashusfs) CAPI Flash user space file system library
 *
 * FUNCTIONS: Data structure used by library for its various internal
 *            operations.
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

#ifndef _H_CFLASH_USFS_WRAPPER
#define _H_CFLASH_USFS_WRAPPER

#ifndef _AIX
/************************************************************************/
/* Linux special internal DIR structure implementation.                 */
/* Linux hides the internals of this structure                          */
/* and wants it opaque. So for linux we will use                        */
/* our own internal implementation. There are only                      */
/* two key aspects we need to contain in this                           */
/* structure, the file descriptor and the offset                        */
/* in the directory.                                                    */
/************************************************************************/
typedef struct cflsh_usfs_DIR_s {
    int       dd_fd;       /* file descriptor of directory       */
    uint64_t  dd_blksize; /* This is the filesystems block size */
    uint64_t  dd_size;
    uint64_t  dd_loc;      /* logical offset in directory        */
    uint64_t  dd_curoff;   /* real offset in directory           */

} cflsh_usfs_DIR_t;


#endif /* !_AIX */

#define CFLSH_USFS_DISK_DELIMINATOR  ':'

#define MAX_NUM_CUSFS_WRAP_FDS 16

#define MAX_NUM_CUSFS_WRAP_FDS_HASH (MAX_NUM_CUSFS_WRAP_FDS-1)

#define CFLSH_USFS_WRAPPER_INITIAL_FD 512 /* Intial filedescriptor value */
					  /* used by this library        */



/************************************************************************/
/* cflsh_usfs_wrap_fd - The data structure file descriptors associated  */
/*                      with this userspace filesystem.                 */
/* .                                                                    */
/************************************************************************/
typedef struct cflsh_usfs_wrap_fd_s {
    struct cflsh_usfs_wrap_fd_s *prev;   /* Previous filesystem in list */
    struct cflsh_usfs_wrap_fd_s *next;   /* Next filesystem in list     */
    int   fd;                            /* file descriptor;            */
    int   usfs_fd;                       /* File descriptor used by USFS*/
} cflsh_usfs_wrap_fd_t;

/************************************************************************/
/* cusfs_gwrapper - Global wrapper library data structure per process   */
/************************************************************************/

typedef struct cusfs_gwrapper_s {
    int flags;                        /* Global flags for this chunk */

    cflsh_usfs_wrap_fd_t *fd_hash[MAX_NUM_CUSFS_WRAP_FDS];

//?? Not yet    cusfs_lock_t   lock;
} cusfs_gwrapper_t;

extern cusfs_gwrapper_t cusfs_gwrapper;



#endif /* _H_CFLASH_USFS_WRAPPER */
