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

#ifndef _H_CFLASH_USFS_ADMIN
#define _H_CFLASH_USFS_ADMIN

#ifndef _AIX
#define _GNU_SOURCE
#endif /* !_AIX */

#include <stdio.h>
#include <dirent.h>
#include <sys/statfs.h>

/************************************************************************/
/* cufs_create_fs flags                                                 */
/************************************************************************/

#define CUSFS_FORCE_CREATE_FS   0x00000001



struct cusfs_query_fs {
    int version;
    int flags;
#define CUSFS_QRY_DIF_ENDIAN   0x1  /* Filesystem has different  */
				    /* endianness that this host  */
    int os_type;
    int reserved;
    uint64_t fs_block_size;
    uint64_t disk_block_size;
    uint64_t num_blocks;
    uint64_t free_block_table_size;
    uint64_t inode_table_size;
    time_t create_time;
    time_t write_time;
    time_t mount_time;
    time_t fsck_time;
};

/*
 * Administrative CAPI Flash User Space File System 
 * commands.
 */


int cusfs_create_fs(char *device_name, int flags);
int cusfs_query_fs(char *device_name, struct cusfs_query_fs *qry, int flags);
int cusfs_statfs(char *pathname, struct statfs  *statfs);

int cusfs_statfs64(char *pathname, struct statfs64  *statfs);

int cusfs_remove_fs(char *device_name, int flags);
int cusfs_fsck(char *device_name, int flags);
int cusfs_mkdir(char *pathname, mode_t mode_flags);
int cusfs_rmdir(char *pathname);

#ifdef _AIX
DIR64 *cusfs_opendir64( char *directory_name);

int cusfs_closedir64(DIR64 *directory_pointer);

int cusfs_readdir64_r(DIR64 *directory_pointer,struct dirent64 *entry, 
			      struct dirent64 ** result);

struct dirent64 *cusfs_readdir64(DIR64 *directory_pointer);
offset_t cusfs_telldir64(DIR64 *directory_pointer);

void cusfs_seekdir64(DIR64 *directory_pointer, offset_t location);
#else
DIR *cusfs_opendir( char *directory_name);

int cusfs_closedir(DIR *directory_pointer);

int cusfs_readdir_r(DIR *directory_pointer,struct dirent *entry, 
			      struct dirent ** result);

off_t cusfs_telldir(DIR *directory_pointer);

void cusfs_seekdir(DIR *directory_pointer, off_t location);

#endif /* !_AIX */

struct dirent *cusfs_readdir(DIR *directory_pointer);



//cflsh_ufs_data_obj_t *cufs_create_file(cflsh_ufs_t *cufs, char *full_pathname,
//				       mode_t mode_flags, uid_t uid, gid_t gid, int flags);

#define CFLSH_USFS_LINK_SYM_FLAG  0x1

int cusfs_create_link(char *orig_path, char *path, 
		     mode_t mode_flags,  uid_t uid, gid_t gid, 
		     int flags);
int cusfs_remove_file(char *pathname, int flags);

int cusfs_list_files(char *pathname, int flags);
int cusfs_rename(char *from_pathname, 
		 char *to_pathname);

#endif /* _H_CFLSH_USFS_ADNIN */
