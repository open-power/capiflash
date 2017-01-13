/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/tools/blk_test.c 1.2         */
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
#ifdef _AIX
static char sccsid[] = "%Z%%M%   %I%  %W% %G% %U%";
#endif


/*
 * COMPONENT_NAME: (sysxcflashusfs) CAPI Flash userspace filesystem
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
 * Set FUSE version to the level that corresponds to the 2.9.5 
 * version that we built this against.
 */

#define FUSE_USE_VERSION 26


#define _FILE_OFFSET_BITS 64

#ifndef _AIX
#define _GNU_SOURCE
#endif /* !_AIX */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _MACOSX 
#include <malloc.h>
#endif /* !_MACOS */
#include <unistd.h>
#include <inttypes.h>
#ifdef _AIX
#include <sys/aio.h>
#else
#include <aio.h>
#endif
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <utime.h>
#include <fuse.h>
#include <cflsh_usfs.h>
#include <cflsh_usfs_admin.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef TIMELEN
#define   TIMELEN  26           /* Linux does have a define for the minium size of the a timebuf */
                                /* However linux man pages say it is 26                          */
#endif 



static char *device_name = NULL; /* point to device name */



/*
 * NOTE:   The paths passed to each of the callback functions
 *         are relative to the root directory of that filesystem
 */



/*
 * NOTE3:  The return codes of the call back fuse functions needs
 *         to either be 0 (good completion) or -errno for erro.
 */


/*
 * NAME:        cusfs_fuse_convert_path
 *
 * FUNCTION:    Convert a fuse relative path into one that contains
 *              the device_name.
 *
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
static char *cusfs_fuse_convert_path(const char *path)
{
    char *disk_pathname;

    disk_pathname = malloc(strlen(path) + strlen(device_name) + 1);

    if (disk_pathname == NULL) {

	return NULL;
    }

    sprintf(disk_pathname,"%s%s%s",device_name,
	    CUSFS_DISK_NAME_DELIMINATOR,path);
    
    fprintf(stderr,"disk_pathname = %s\n",disk_pathname);
    return disk_pathname;
}

/*
 * NAME:        cusfs_fuse_getattr
 *
 * FUNCTION:    Get statistics of a file
 *	        Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 *              ignored.      The 'st_ino' field is ignored except if the 'use_ino'
 *              mount option is given.
 *
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_getattr(const char *pathname, struct stat *stats)
{
    int rc = 0;
    char *disk_pathname;
#ifdef _AIX
    struct stat64 stats64;

    

    bzero(&stats64,sizeof(stats64));
#endif


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

#ifdef _AIX
    rc = cusfs_stat64(disk_pathname,&stats64);

    stats->st_ino = stats64.st_ino;
    stats->st_mode = stats64.st_mode;
    stats->st_nlink = stats64.st_nlink;
    stats->st_uid = stats64.st_uid;
    stats->st_gid = stats64.st_gid;
    stats->st_atime = stats64.st_atime;
    stats->st_ctime = stats64.st_ctime;
    stats->st_mtime = stats64.st_mtime;
    stats->st_blksize = stats64.st_blksize;
    stats->st_blocks = stats64.st_blocks;
#ifndef __64BIT__

    stats->st_size = (int)stats64.st_size;
#else
   stats->st_ssize = (int)stats64.st_size;
#endif

#else
    rc = cusfs_stat(disk_pathname,stats);
#endif
    
    free(disk_pathname);

    
    fprintf(stderr,"getattr: rc = %d, errno = %d\n",rc,errno);

    if (rc) {

	rc = -errno;
    }
    return rc;
}

    
/*
 * NAME:        cusfs_fuse_mkdir
 *
 * FUNCTION:    Make a directory
 *
 *
 * NOTE:        The mode argument may not have the type specification
 *              bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 *              correct directory type bits use  mode|S_IFDIR
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_mkdir(const char *pathname, mode_t mode)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    rc = cusfs_mkdir(disk_pathname,mode);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}

    

/*
 * NAME:        cusfs_fuse_rmdir
 *
 * FUNCTION:    Remove a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_rmdir(const char *pathname)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    rc = cusfs_rmdir(disk_pathname);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}


/*
 * NAME:        cusfs_fuse_unlink
 *
 * FUNCTION:    Remove a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_unlink(const char *pathname)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return ENOMEM;
    }

    rc = cusfs_unlink(disk_pathname);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}	

/*
 * NAME:        cusfs_fuse_rename
 *
 * FUNCTION:    Rename a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_rename(const char *from_pathname, const char *to_pathname)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(from_pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    rc = cusfs_rename(disk_pathname,(char *)to_pathname);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
      
/*
 * NAME:        cusfs_fuse_link
 *
 * FUNCTION:    Sym link a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_symlink(const char *from_pathname, const char *to_pathname)
{
    int rc = 0;
    uid_t uid;
    gid_t gid;
    char *disk_pathname;
    mode_t mode = (S_IRWXU|S_IRWXG|S_IRWXO);


    disk_pathname = cusfs_fuse_convert_path(from_pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    uid = getuid();
    
    gid = getgid();

    rc = cusfs_create_link(disk_pathname,(char *)to_pathname,mode,uid,gid,CFLSH_USFS_LINK_SYM_FLAG);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}

 

/*
 * NAME:        cusfs_fuse_link
 *
 * FUNCTION:    Link a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_link(const char *from_pathname, const char *to_pathname)
{
    int rc = 0;
    uid_t uid;
    gid_t gid;
    char *disk_pathname;
    mode_t mode = (S_IRWXU|S_IRWXG|S_IRWXO);


    disk_pathname = cusfs_fuse_convert_path(from_pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    uid = getuid();
    
    gid = getgid();

    rc = cusfs_create_link(disk_pathname,(char *)to_pathname,mode,uid,gid,0);

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}

      
/*
 * NAME:        cusfs_fuse_chmod
 *
 * FUNCTION:    Change mode of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_chmod(const char *path, mode_t mode)
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);

	free(disk_pathname);
	return -ENOMEM;
	    
    }

    rc = cusfs_fchmod(fd,mode);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }


    free(disk_pathname);

    if (rc) {

	rc = -errno;
    }
    return rc;
}

         
/*
 * NAME:        cusfs_fuse_chown
 *
 * FUNCTION:    Change owner/group of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_chown(const char *path, uid_t uid, gid_t gid)
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);

	free(disk_pathname);
	return -errno;
	    
    }

    rc = cusfs_fchown(fd,uid,gid);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }



    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
        
/*
 * NAME:        cusfs_fuse_truncate
 *
 * FUNCTION:    Change file size
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_truncate(const char *path, off_t file_size)
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);

	free(disk_pathname);
	return -errno;
	    
    }

    rc = cusfs_ftruncate(fd,file_size);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }



    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
   

/*
 * NAME:        cusfs_fuse_utime
 *
 * FUNCTION:    Change time of file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_utime(const char *path, struct utimbuf *timebuf)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }


    rc = cusfs_utime(disk_pathname,timebuf);



    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
  
#ifdef _NOT_YET
      
/*
 * NAME:        cusfs_fuse_utimens
 *
 * FUNCTION:    Change time of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_utimes(const char *path, const struct timespec tv[2])
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);

	free(disk_pathname);
	return -errno;
	    
    }

    rc = cusfs_futimens(fd,tv);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }



    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
 
#endif

        
/*
 * NAME:        cusfs_fuse_ftruncate
 *
 * FUNCTION:    Change file size
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_ftruncate(const char *path, off_t file_size,struct fuse_file_info *finfo)
{

    int fd;
    int rc = 0;

    if (finfo == NULL) {
	errno = EINVAL;
	return -1;
    }

    fd = finfo->fh;

    rc = cusfs_ftruncate(fd,file_size);
    if (rc) {

	rc = -errno;
    }

    return rc;
}

        
/*
 * NAME:        cusfs_fuse_open
 *
 * FUNCTION:    Open a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_open(const char *path, struct fuse_file_info *finfo)
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    if (finfo == NULL) {

	return -EINVAL;
    }


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed %s with errno = %d\n",
		disk_pathname,errno);
	rc = -errno;
	    
    }

    finfo->fh = fd;

    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }

    return rc;
}
       
/*
 * NAME:        cusfs_fuse_create
 *
 * FUNCTION:    Open a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_create(const char *path, mode_t mode,struct fuse_file_info *finfo)
{
    int rc = 0;
    int fd;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    if (finfo == NULL) {

	return -EINVAL;
    }


    fd = cusfs_creat(disk_pathname,mode);

    if (fd < 0) {

	fprintf(stderr,"Creat failed %s with errno = %d\n",
		disk_pathname,errno);
	rc = -errno;
	    
    }

    finfo->fh = fd;


    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}

          
/*
 * NAME:        cusfs_fuse_read
 *
 * FUNCTION:    read a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_read(const char *path, char *buffer, size_t nbytes, off_t offset, 
		   struct fuse_file_info *finfo)
{
    int fd;
    int rc = 0;

    if (finfo == NULL) {

	return -EINVAL;
    }

    fd = finfo->fh;

    rc = cusfs_lseek(fd,offset,0);

    if (rc != offset) {

	return -errno;
    }

    rc = cusfs_read(fd,buffer,nbytes);
    if (rc < 0) {

	rc = -errno;
    }

    return rc;
}
        
/*
 * NAME:        cusfs_fuse_write
 *
 * FUNCTION:    read a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_write(const char *path, const char *buffer, size_t nbytes, off_t offset, 
		   struct fuse_file_info *finfo)
{
    int fd;
    int rc;

    if (finfo == NULL) {
	errno = EINVAL;
	return -1;
    }

    fd = finfo->fh;

    rc = cusfs_lseek(fd,offset,0);

    if (rc != offset) {

	return -errno;
    }

    rc = cusfs_write(fd,(char *)buffer,nbytes);
    if (rc < 0) {

	rc = -errno;
    }

    return rc;
}

          
/*
 * NAME:        cusfs_fuse_statfs
 *
 * FUNCTION:    Gets stats for a filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_statfs(const char *path, struct statvfs *statvfs)
{
    int rc;
    struct statfs statfs;

    //? add statvfs to library to do this directly

    if (statvfs == NULL) {
	errno = EINVAL;
	return -1;
    }

    bzero(&statfs,sizeof(statfs));

    bzero(statvfs,sizeof(*statvfs));

    rc = cusfs_statfs(device_name,&statfs);

    if (rc == 0) {

	statvfs->f_blocks = statfs.f_blocks;
	statvfs->f_bfree = statfs.f_bfree;
	statvfs->f_bavail = statfs.f_bavail;
	statvfs->f_blocks = statfs.f_blocks;
	statvfs->f_files = statfs.f_files;
	statvfs->f_ffree = statfs.f_ffree;
	statvfs->f_favail = statvfs->f_ffree;

    }


    if (rc) {

	rc = -errno;
    }
    return rc;
}

       
/*
 * NAME:        cusfs_fuse_flush
 *
 * FUNCTION:    flush data for a file
 *
 * NOTES:       Currently this just does the same thing as fsync.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_flush(const char *path, struct fuse_file_info *finfo)
{
    int fd;
    int rc = 0;

    if (finfo == NULL) {

	return -EINVAL;
    }

    fd = finfo->fh;

    rc = cusfs_fsync(fd);

    if (rc) {

	rc = -errno;
    }
    return rc;
}
        
/*
 * NAME:        cusfs_fuse_release
 *
 * FUNCTION:    release(close) an open file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_release(const char *path, struct fuse_file_info *finfo)
{
    int fd;
    int rc = 0;

    if (finfo == NULL) {

	return -EINVAL;
    }

    fd = finfo->fh;

    rc = cusfs_close(fd);

    if (rc) {

	rc = -errno;
    }
    return rc;
}
    

/*
 * NAME:        cusfs_fuse_fsync
 *
 * FUNCTION:    Sync a file's contents
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_fsync(const char *path, int data_sync,struct fuse_file_info *finfo)
{
    int fd;
    int rc = 0;

    if (finfo == NULL) {

	return -EINVAL;
    }

    fd = finfo->fh;

    rc = cusfs_fsync(fd);

    if (rc) {

	rc = -errno;
    }
    return rc;
}


       
/*
 * NAME:        cusfs_fuse_opendir
 *
 * FUNCTION:    Open a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_opendir(const char *path, struct fuse_file_info *finfo)
{
    int rc = 0;
#ifdef _AIX
    DIR64 *directory;
#else
    DIR *directory;
#endif 
    char *disk_pathname;


    if (finfo == NULL) {

	return -EINVAL;
    }

    disk_pathname = cusfs_fuse_convert_path(path);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

#ifdef _AIX
    directory = cusfs_opendir64(disk_pathname);

#else
    directory = cusfs_opendir(disk_pathname);
#endif

    if (directory == NULL) {

	fprintf(stderr,"opendir returned null pointer with errno = %d\n",errno);
	rc = -errno;
    }

    finfo->fh = (uint64_t)directory;


    free(disk_pathname);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
    
         
/*
 * NAME:        cusfs_fuse_releasedir
 *
 * FUNCTION:    Close a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_releasedir(const char *path,
		      struct fuse_file_info *finfo)
{
    int rc = 0;
#ifdef _AIX
    DIR64 *directory;
#else
    DIR *directory;
#endif 

    if (finfo == NULL) {
	errno = EINVAL;
	return -1;
    }

#ifdef _AIX
    directory = (DIR64 *) finfo->fh;

    if (directory) {
	rc= cusfs_closedir64(directory);
    }

#else
    directory = (DIR *) finfo->fh;
    if (directory) {
	rc = cusfs_closedir(directory);
    }
#endif

    if (rc) {

	rc = -errno;
    }
    return rc;
}
    
      
   
         
/*
 * NAME:        cusfs_fuse_readdir
 *
 * FUNCTION:    Read a directory entry
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_readdir(const char *path, void *buf,
		       fuse_fill_dir_t filler, off_t offset,
		      struct fuse_file_info *finfo)
{
    int rc = 0;
    char stat_name[PATH_MAX];
#ifdef _AIX
    DIR64 *directory;
    struct dirent64 file;
    struct dirent64 *result = &file;
    struct stat64 stats64;
#else
    DIR *directory;
    struct dirent file;
    struct dirent *result = &file;
#endif 
    struct stat stats;
    off_t nextoff;


    if (finfo == NULL) {

	return -EINVAL;
    }

#ifdef _AIX
    directory = (DIR64 *) finfo->fh;
#else
    directory = (DIR *) finfo->fh;
#endif

    while (TRUE) {
#ifdef _AIX
	rc = cusfs_readdir64_r(directory,&file,&result);
	nextoff = file.d_offset;
	bzero(&stats64,sizeof(stats64));
#else
	rc = cusfs_readdir_r(directory,&file,&result);
	nextoff = file.d_off;
#endif

	if (rc) {


	    return -errno;
	}

	if (result == NULL) {

	    return 0;
	}

	bzero(&stats,sizeof(stats));

	sprintf(stat_name,"%s%s%s%s",device_name,
		CUSFS_DISK_NAME_DELIMINATOR,path,file.d_name);	


#ifdef _AIX
	rc = cusfs_stat64(stat_name,&stats64);

	stats.st_ino = stats64.st_ino;
	stats.st_mode = stats64.st_mode;
	stats.st_nlink = stats64.st_nlink;
	stats.st_uid = stats64.st_uid;
	stats.st_gid = stats64.st_gid;
	stats.st_atime = stats64.st_atime;
	stats.st_ctime = stats64.st_ctime;
	stats.st_mtime = stats64.st_mtime;
	stats.st_blksize = stats64.st_blksize;
	stats.st_blocks = stats64.st_blocks;
#ifndef __64BIT__

	stats.st_size = (int)stats64.st_size;
#else
	stats.st_ssize = (int)stats64.st_size;
#endif
#else
	rc = cusfs_stat(stat_name,&stats);
#endif
	
	if (rc) {

	    return -errno;
	}

	//?? For AIX stat64 is not correct here.

	if (filler(buf, file.d_name, &stats, nextoff)) {

	    /*
	     * Buffer is full
	     */
	    break;
	}


    }


    return 0;
}
    

/*
 * NAME:        cusfs_fuse_readlink
 *
 * FUNCTION:    Read symbollic link filename
 *
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_readlink(const char *pathname, char *buffer, size_t buffer_size)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    rc = cusfs_readlink(disk_pathname,buffer,buffer_size);

    
    free(disk_pathname);

    
    if (rc) {

	rc = -errno;
    }
    return rc;
}
      

/*
 * NAME:        cusfs_fuse_access
 *
 * FUNCTION:    Determines accessibility of a file
 *
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_access(const char *pathname, int mode)
{
    int rc = 0;
    char *disk_pathname;


    disk_pathname = cusfs_fuse_convert_path(pathname);

    if (disk_pathname == NULL) {

	return -ENOMEM;
    }

    rc = cusfs_access(disk_pathname,mode);

    
    free(disk_pathname);

    
    fprintf(stderr,"access: rc = %d, errno = %d\n",rc,errno);
    if (rc) {

	rc = -errno;
    }
    return rc;
}
      

/*
 * NAME:        cusfs_fuse_fgetattr
 *
 * FUNCTION:    Get statistics of a file
 *	        Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 *              ignored.      The 'st_ino' field is ignored except if the 'use_ino'
 *              mount option is given.
 *
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int cusfs_fuse_fgetattr(const char *pathname, struct stat *stats,
		       struct fuse_file_info *finfo)
{    
    int fd;
    int rc = 0;

    if (finfo == NULL) {
	errno = EINVAL;
	return -1;
    }

    fd = finfo->fh;


    rc = cusfs_fstat(fd,stats);

    fprintf(stderr,"access: rc = %d, errno = %d\n",rc,errno);

    if (rc) {

	rc = -errno;
    }
   
    return rc;
}


/************************************************************************/
/* Fuse operations supported                                            */
/************************************************************************/


static struct fuse_operations cflsh_ufs_oper = {
    .getattr 		= cusfs_fuse_getattr,
    .mkdir              = cusfs_fuse_mkdir,
    .unlink             = cusfs_fuse_unlink,
    .rmdir              = cusfs_fuse_rmdir,
    .symlink            = cusfs_fuse_link,
    .rename             = cusfs_fuse_rename,
    .link               = cusfs_fuse_link,
    .chmod              = cusfs_fuse_chmod,
    .chown              = cusfs_fuse_chown,
    .truncate           = cusfs_fuse_truncate,
    .utime 		= cusfs_fuse_utime,
    .open               = cusfs_fuse_open,
    .read               = cusfs_fuse_read,
    .write              = cusfs_fuse_write,
    .statfs             = cusfs_fuse_statfs,
    .flush              = cusfs_fuse_flush,
    .release            = cusfs_fuse_release,
    .fsync              = cusfs_fuse_fsync,
    .opendir            = cusfs_fuse_opendir,
    .readdir            = cusfs_fuse_readdir,
    .releasedir         = cusfs_fuse_releasedir,
    .readlink		= cusfs_fuse_readlink,
    .access		= cusfs_fuse_access,
    .create             = cusfs_fuse_create,
    .ftruncate          = cusfs_fuse_ftruncate,
    .fgetattr		= cusfs_fuse_fgetattr,
};

void  cusfs_fuse_usage()
{

    fprintf(stderr,"usage: cusfs_fuse [ fuse_options ] device_name mount_point\n");
    return;
}

/*
 * NAME:        Main
 *
 * FUNCTION:    fuse_main turns this into a daemon.
 *
 * NOTES:       Last argument with no "-" is mount point.
 *              -o fsname=/dev/sg5:/ mount_point
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int main(int argc, char *argv[])
{
    int rc = 0;

    cusfs_init(NULL,0);

    if (argc < 3) {

	cusfs_fuse_usage();
	return EINVAL;
    }

    if ((argv[argc-2][0] == '-') ||
	(argv[argc-1][0] == '-')) {

	
	cusfs_fuse_usage();
	return EINVAL;
    }
    
    
    device_name = strdup(argv[argc-2]);

    if (device_name == NULL) {

	
	fprintf(stderr,"device_name is null");

	cusfs_fuse_usage();

	return EINVAL;
    }

    /*
     * Remove device name from the argument stream
     */

    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    rc = fuse_main(argc, argv, &cflsh_ufs_oper, NULL);

    cusfs_term(NULL,0);

    
    free(device_name);
    
    return rc;
}
