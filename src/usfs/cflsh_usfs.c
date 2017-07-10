//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/usfs/cflsh_usfs.c $
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

#define CFLSH_USFS_FILENUM 0x0100

#include "cflsh_usfs_internal.h"
#include "cflsh_usfs_protos.h"
#include "cflsh_usfs_client.h"
#ifdef _AIX
#include "cflsh_usfs.h"
#include "cflsh_usfs_admin.h"
#else
#include <cflsh_usfs.h>
#include <cflsh_usfs_admin.h>
#endif


#ifndef _AIX
#include <revtags.h>
REVISION_TAGS(ufs);
#ifdef CFLASH_LITTLE_ENDIAN_HOST
#ident "$Debug: LITTLE_ENDIAN $"
#else
#ident "$Debug: BIG_ENDIAN $"
#endif
#ifdef _MASTER_LOCK
#ident "$Debug: MASTER_LOCK $"
#endif
#endif

cusfs_global_t cusfs_global;

char  *cusfs_log_filename = NULL;              /* Trace log filename       */
                                               /* This traces internal     */
                                               /* activities               */

int cusfs_log_verbosity = 0;                   /* Verbosity of traces in   */
                                               /* log.                     */

FILE *cusfs_logfp = NULL;                      /* File pointer for         */
                                               /* trace log                */
FILE *cusfs_dumpfp = NULL;                     /* File pointer  for        */
                                               /* dumpfile                 */

int cusfs_dump_level;                          /* Run time level threshold */
                                               /* required to initiate     */
                                               /* live dump.               */

int cusfs_notify_log_level;                    /* Run time level threshold */
                                               /* notify/error logging     */



int dump_sequence_num;                         /* Dump sequence number     */
 

pthread_mutex_t  cusfs_log_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t  cflsh_usfs_init_lock = PTHREAD_MUTEX_INITIALIZER;

uint32_t num_thread_logs = 0;     /* Number of thread log files */
  
#ifndef _AIX
/* 
 * This __attribute constructor is not valid for the AIX xlc 
 * compiler. 
 */
static void _cflsh_usfs_init(void) __attribute__((constructor));
static void _cflsh_usfs_free(void) __attribute__((destructor));
#else 
static int cflsh_usfs_initialize = 0;
static int cflsh_usfs_init_finish = 0;
static int cflsh_usfs_term = 0;

#endif

/*
 * NAME:        _cflsh_usfs_init
 *
 * FUNCTION:    Internal intializer/constructor
 *              for the CAPI flash block library.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NONE
 *              
 */

void _cflsh_usfs_init(void)
{

    char *env_sigsegv = getenv("CFLSH_USFS_SIGSEGV_DUMP");
    char *env_sigusr1 = getenv("CFLSH_USFS_SIGUSR1_DUMP");
    char *env_dump_level = getenv("CFLSH_USFS_DUMP_LEVEL");
    char *env_notify_log_level = getenv("CFLSH_USFS_NOTIFY_LOG_LEVEL");
    int rc;

    /*
     * Require that the inode structure fits evenly into a 4K
     * disk block.
     */

    CFLSH_USFS_COMPILE_ASSERT((4096 % sizeof(cflsh_usfs_inode_t)) == 0);

    
#ifdef _LINUX_MTRACE
    mtrace();
#endif


    CUSFS_RWLOCK_INIT(cusfs_global.global_lock);

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);



    rc = pthread_atfork(cusfs_prepare_fork,cusfs_parent_post_fork,cusfs_child_post_fork);

#ifndef _AIX
    if (rc) {
	fprintf(stderr,"pthread_atfork failed rc = %d, errno = %d\n",rc,errno);

    }
#endif

    /*
     * ?? Initialize global data structs
     */

    cusfs_setup_trace_files(FALSE);


    if (!cusfs_valid_endianess()) {
	
	 CUSFS_TRACE_LOG_FILE(1,"This program is compiled for different endianness then the host is is running");
    }

    CUSFS_TRACE_LOG_FILE(2,"cusfs_global = %p",&cusfs_global);

    if (env_sigsegv) {

	/*
	 * The caller has requested that on SIGSEGV
	 * signals (segmentation fault), that we dump
	 * data just prior the application crash (and
	 * potentially coredump). This will provide
	 * more data to aid in analyzing the core dump.
	 */

	if (cusfs_setup_sigsev_dump()) {
	    CUSFS_TRACE_LOG_FILE(1,"failed to set up sigsev dump handler");
	}

    }

    if (env_sigusr1) {

	/*
	 * The caller has requested that on SIGUSR1
	 * signals that we dump data.
	 */

	if (cusfs_setup_sigusr1_dump()) {
	    CUSFS_TRACE_LOG_FILE(1,"failed to set up sigusr1 dump handler");
	}

    }

    if (env_dump_level) {

	if (cusfs_setup_dump_file()) {
	    CUSFS_TRACE_LOG_FILE(1,"failed to set up dump file");

	} else {

	    cusfs_dump_level = atoi(env_dump_level);
	}

    }


    if (env_notify_log_level) {

	cusfs_notify_log_level = atoi(env_notify_log_level);

    }

    cusfs_global.caller_pid = getpid();

    /*
     * Determine host type
     */


    cusfs_global.os_type = cusfs_get_os_type();

    cflsh_usfs_master_init();

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    return;
}


/*
 * NAME:        _cflsh_usfs_free
 *
 * FUNCTION:    Free library resources.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NONE
 *              
 */

void _cflsh_usfs_free(void)
{

    cflsh_usfs_master_term();

    if (cusfs_global.flags & CFLSH_G_SYSLOG) {

	closelog();

    }


    if (num_thread_logs) {

	free(cusfs_global.thread_logs);
    }

#ifdef _LINUX_MTRACE
    muntrace();
#endif

    

    if (cusfs_global.process_name) {

	free(cusfs_global.process_name);
    }


    CUSFS_TRACE_LOG_FILE(3,"\nLIBRARY STATISTICS ...");

#ifdef CFLASH_LITTLE_ENDIAN_HOST
    CUSFS_TRACE_LOG_FILE(3,"Little Endian");
#else
    CUSFS_TRACE_LOG_FILE(3,"Big Endian");
#endif 
     
    CUSFS_TRACE_LOG_FILE(3,"cusfs_log_verbosity              0x%x",cusfs_log_verbosity);
#if !defined(__64BIT__) && defined(_AIX)
    CUSFS_TRACE_LOG_FILE(3,"32-bit app support              ");
#else
    CUSFS_TRACE_LOG_FILE(3,"64-bit app support              ");

#endif
    CUSFS_TRACE_LOG_FILE(3,"flags                           0x%x",cusfs_global.flags);
    CUSFS_TRACE_LOG_FILE(3,"next_cusfs_id                    0x%llx",cusfs_global.next_cusfs_id);
    CUSFS_TRACE_LOG_FILE(3,"num_active_cufs                 0x%x",cusfs_global.num_active_cufs);
    CUSFS_TRACE_LOG_FILE(3,"num_max_active_cufs             0x%x",cusfs_global.num_max_active_cufs);
    CUSFS_TRACE_LOG_FILE(3,"num_bad_cusfs_ids                0x%x",cusfs_global.num_bad_cusfs_ids);

    return;

}


#ifdef _AIX
/*
 * NAME:        cflsh_usfs_init
 *
 * FUNCTION:    Call library initializing code.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NONE
 *              
 */

void cflsh_usfs_init(void)
{
    /*
     * The first thread will invoke this routine, but
     * but subsequent threads must not be allowed to 
     * proceed until the _cusfs_init has completed.
     *
     * NOTE: the use of both a lock and a fetch_and_or
     *       is overkill here. A better solution would be
     *       to find a way to use fetch_and_or and only using
     *       the lock while the _cusfs_init.
     */

    //pthread_mutex_lock(&cflsh_usfs_init_lock);

    if (fetch_and_or(&cflsh_usfs_initialize,1)) {



        /*
         * If cusfs_initialize is set, then
         * we have done all the cflash block code
         * initialization. As result we can return
         * now provided we ensure any thread that is
         * doing initializatoin for this library
         * has completed.
         */

        while (!cflsh_usfs_init_finish) {

            usleep(1);

        }

        return;
    }

    /*
     * We get here if the cflash_block code has not been
     * initialized yet. 
     */

    _cflsh_usfs_init();
        
    fetch_and_or(&cflsh_usfs_init_finish,1);

    //pthread_mutex_unlock(&cflsh_usfs_init_lock);
    return;
}

#endif /* _AIX */

/*
 * NAME:        cusfs_init
 *
 * FUNCTION:    Call library initializing code.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NONE
 *              
 */

int cusfs_init(void *arg,uint64_t flags)
{

    /*
     * Today, this routine is a no-op, but it provides
     * future expandability options should that ever be needed.
     */

    return 0;
}


/*
 * NAME:        cusfs_term
 *
 * FUNCTION:    Free library resources.
 *
 *
 * INPUTS:      NONE
 *              
 *
 * RETURNS:     NONE
 *              
 */

int cusfs_term(void *arg,uint64_t flags)
{
#ifdef _AIX
    if (fetch_and_or(&cflsh_usfs_term,1)) {



        /*
         * If cflsh_blk_term is set, then
         * we have done all the cflash block code
         * cleanup. As result we can return
         * now.
         */
        return 0;
    }

    /*
     * We get here if the cflsh UFS code has not been
     * terminated yet. 
     */

    _cflsh_usfs_free();

#endif /* AIX */
    return 0;
}



/*
 * NAME:        cusfs_create_fs
 *
 * FUNCTION:    Create a CAPI flash user space filesystem
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

int cusfs_create_fs(char *device_name, int flags)
{
    int rc = 0;
    int local_flags = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
    uid_t uid;
    gid_t gid;
    mode_t mode;


    CUSFS_TRACE_LOG_FILE(5,"device_name = %s",
			device_name);
    if (device_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Null device_name");

	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_get_cufs(device_name,0);

    if (cufs == NULL) {

	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    if (flags & CUSFS_FORCE_CREATE_FS) {
	local_flags = CFLSH_USFS_FORCE_SB_CREATE;
    }

    
    if (cusfs_create_superblock(cufs,local_flags)) {

	cusfs_release_cufs(cufs,0);
	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }


    if (cusfs_create_free_block_table(cufs,0)) {


	cusfs_release_cufs(cufs,0);
	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    cufs->superblock.flags |= CFLSH_USFS_SB_FT_VAL;

    if (cusfs_create_inode_table(cufs,0)) {


	cusfs_release_cufs(cufs,0);
	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }


    cufs->superblock.flags |= CFLSH_USFS_SB_IT_VAL;

    if (cusfs_create_journal(cufs,0)) {


	cusfs_release_cufs(cufs,0);
	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    cufs->superblock.flags |= CFLSH_USFS_SB_JL_VAL;

    /* 
     * Create root directory 
     */

    uid = getuid();
    
    gid = getgid();

    mode = S_IRWXU | S_IRWXG | S_IRWXO;

    file = cusfs_create_data_obj(cufs,"/",CFLSH_USFS_INODE_FILE_DIRECTORY,0,mode,uid,gid,NULL,CFLSH_USFS_ROOT_DIR);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to create root directory on disk %s",
				cufs->device_name);
    }

    cufs->superblock.flags |= CFLSH_USFS_SB_ROOT_DIR;

    if (cusfs_update_superblock(cufs,0)) {


	cusfs_release_cufs(cufs,0);
	

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }



    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    return rc;
}

/*
 * NAME:        cusfs_query_fs
 *
 * FUNCTION:    Query informatin on filesystem
 *              for the specified disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_query_fs(char *device_name, struct cusfs_query_fs *query,int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_super_block_t cusfs_super_block;

    CUSFS_TRACE_LOG_FILE(5,"device_name = %s",
			device_name);
    if (device_name == NULL) {


	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_get_cufs(device_name,0);


    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {


	return -1;
    }

    bzero(query,sizeof(*query));
    bzero(&cusfs_super_block,sizeof(cusfs_super_block));

    rc = cusfs_query_superblock(cufs,&cusfs_super_block,0);

    if (!rc) {


	if (cusfs_super_block.start_marker != CLFSH_USFS_SB_SM) {

	    /*
	     * Not valid super block for this host
	     */

	    CUSFS_TRACE_LOG_FILE(1,"Not valid super block start_marker = 0x%llx",
				 cusfs_super_block.start_marker);


	    if (CFLASH_REV64(cusfs_super_block.start_marker) == CLFSH_USFS_SB_SM) {

		CUSFS_TRACE_LOG_FILE(1,"Actually superblock is valid for different endianness host",
				     cusfs_super_block.start_marker);
		query->flags |= CUSFS_QRY_DIF_ENDIAN;
		query->os_type = CFLASH_REV32(cusfs_super_block.os_type);
		query->fs_block_size = CFLASH_REV64(cusfs_super_block.fs_block_size);
		query->disk_block_size = CFLASH_REV64(cusfs_super_block.disk_block_size);
		query->num_blocks = CFLASH_REV64(cusfs_super_block.num_blocks);
		query->free_block_table_size = CFLASH_REV64(cusfs_super_block.free_block_table_size);
		query->inode_table_size = CFLASH_REV64(cusfs_super_block.inode_table_size);


		if (sizeof(time_t) == 8) {

		    query->create_time = CFLASH_REV64(cusfs_super_block.create_time);
		    query->write_time = CFLASH_REV64(cusfs_super_block.write_time);
		    query->mount_time = CFLASH_REV64(cusfs_super_block.mount_time);
		    query->fsck_time = CFLASH_REV64(cusfs_super_block.fsck_time);


		} else  {

		    query->create_time = CFLASH_REV32(cusfs_super_block.create_time);
		    query->write_time = CFLASH_REV32(cusfs_super_block.write_time);
		    query->mount_time = CFLASH_REV32(cusfs_super_block.mount_time);
		    query->fsck_time = CFLASH_REV32(cusfs_super_block.fsck_time);

		}

#ifdef _AIX
		rc = ECORRUPT;
#else
		rc = EBADR;
#endif /* ! AIX */




	    } else {

		rc =  ENXIO;

	    }
	}  else {


	    query->os_type = cusfs_super_block.os_type;
	    query->fs_block_size = cusfs_super_block.fs_block_size;
	    query->disk_block_size = cusfs_super_block.disk_block_size;
	    query->num_blocks = cusfs_super_block.num_blocks;
	    query->free_block_table_size = cusfs_super_block.free_block_table_size;
	    query->inode_table_size = cusfs_super_block.inode_table_size;
	    query->create_time = cusfs_super_block.create_time;
	    query->write_time = cusfs_super_block.write_time;
	    query->mount_time = cusfs_super_block.mount_time;
	    query->fsck_time = cusfs_super_block.fsck_time;

	}

    }

    cusfs_release_cufs(cufs,0);

    if (rc) {
	errno = rc;

	rc = -1;

    }
    return rc;
}

/*
 * NAME:        cusfs_statfs
 *
 * FUNCTION:    Return statfs informatin on filesystem
 *              for the specified disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL for failure, otherwise success.
 *              
 *              
 */

int cusfs_statfs(char *path, struct statfs  *statfs)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_super_block_t cusfs_super_block;
    uint64_t num_inodes;
    uint64_t num_avail_inodes;
    uint64_t num_free_blocks;
    uint64_t num_avail_free_blocks;
    char device_name[PATH_MAX];
    char *filename;


    CUSFS_TRACE_LOG_FILE(5,"device_name = %s",
			device_name);

    if (path == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"path is null");
	errno = EINVAL;
	return -1;
    }

    /*
     * Copy path, since we're
     * going to modify it.
     */

    strcpy(device_name,path);

    filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename) {

	    filename[0] = '\0';
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_get_cufs(device_name,0);


    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for device_name = %s",
			    device_name);


	return -1;
    }

    bzero(statfs,sizeof(*statfs));
    bzero(&cusfs_super_block,sizeof(cusfs_super_block));

    rc = cusfs_query_superblock(cufs,&cusfs_super_block,0);
   
    if (!rc) {

	if (cusfs_super_block.start_marker != CLFSH_USFS_SB_SM) {

	    /*
	     * Not valid super block for this host
	     */

	    CUSFS_TRACE_LOG_FILE(1,"Not valid super block start_marker = 0x%llx",
				 cusfs_super_block.start_marker);


	    cusfs_release_cufs(cufs,0);
	    errno = ENXIO;
	    return -1;

	}
#ifdef _AIX
	statfs->f_version = cusfs_super_block.version;
	statfs->f_name_max = PATH_MAX;
#endif 

	//??statfs->f_type
	statfs->f_bsize = cusfs_super_block.fs_block_size;
	statfs->f_blocks = cusfs_super_block.num_blocks; //?? should this be in fs_block_size?
	bcopy(&(cusfs_super_block.fs_unique_id.val1),
	       &(statfs->f_fsid), 
	       sizeof(cusfs_super_block.fs_unique_id.val1));

    }

    rc = cusfs_get_free_table_stats(cufs,&num_free_blocks,&num_avail_free_blocks,0);

    if (!rc) {

	statfs->f_bfree = num_free_blocks;
	statfs->f_bavail = num_avail_free_blocks;
    }


    rc = cusfs_get_inode_stats(cufs,&num_inodes,&num_avail_inodes,0);


    if (!rc) {

	statfs->f_files = num_inodes;
	statfs->f_ffree = num_avail_inodes;
    }



    cusfs_release_cufs(cufs,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}


/*
 * NAME:        cusfs_statfs64
 *
 * FUNCTION:    Return statfs informatin on filesystem
 *              for the specified disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL for failure, otherwise success.
 *              
 *              
 */

int cusfs_statfs64(char *path, struct statfs64  *statfs)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_super_block_t cusfs_super_block;
    uint64_t num_inodes;
    uint64_t num_avail_inodes;
    uint64_t num_free_blocks;
    uint64_t num_avail_free_blocks;
    char device_name[PATH_MAX];
    char *filename;

    if (device_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"device_name is null");
	errno = EINVAL;
	return -1;
    }

    /*
     * Copy path, since we're
     * going to modify it.
     */

    strcpy(device_name,path);

    filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename) {

	    filename[0] = '\0';
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_get_cufs(device_name,0);


    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for device_name = %s",
			    device_name);

	return -1;
    }

    bzero(statfs,sizeof(*statfs));
    bzero(&cusfs_super_block,sizeof(cusfs_super_block));

    rc = cusfs_query_superblock(cufs,&cusfs_super_block,0);
   
    if (!rc) {

	if (cusfs_super_block.start_marker != CLFSH_USFS_SB_SM) {

	    /*
	     * Not valid super block for this host
	     */

	    CUSFS_TRACE_LOG_FILE(1,"Not valid super block start_marker = 0x%llx",
				 cusfs_super_block.start_marker);


	    cusfs_release_cufs(cufs,0);
	    errno = ENXIO;
	    return -1;

	}
#ifdef _AIX
	statfs->f_version = cusfs_super_block.version;
	statfs->f_name_max = PATH_MAX;
#endif 
	//??statfs->f_type
	statfs->f_bsize = cusfs_super_block.fs_block_size;
	statfs->f_blocks = cusfs_super_block.num_blocks; //?? should this be in fs_block_size?
	bcopy(&(cusfs_super_block.fs_unique_id.val1),
	       &(statfs->f_fsid), 
	       sizeof(cusfs_super_block.fs_unique_id.val1));


    }

    rc = cusfs_get_free_table_stats(cufs,&num_free_blocks,&num_avail_free_blocks,0);

    if (!rc) {

	statfs->f_bfree = num_free_blocks;
	statfs->f_bavail = num_avail_free_blocks;
    }


    rc = cusfs_get_inode_stats(cufs,&num_inodes,&num_avail_inodes,0);


    if (!rc) {

	statfs->f_files = num_inodes;
	statfs->f_ffree = num_avail_inodes;
    }


    cusfs_release_cufs(cufs,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}


/*
 * NAME:        cusfs_remove_fs
 *
 * FUNCTION:    Remove a CAPI flash user space filesystem
 *              on a disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL for failure, otherwise success.
 *              
 *              
 */

int cusfs_remove_fs(char *device_name, int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;


    CUSFS_TRACE_LOG_FILE(5,"device_name = %s",
			device_name);

    if (device_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"device_name is null");

	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_find_fs_for_disk(device_name,0);

    if (cufs == NULL) {
	
	CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
			    device_name);
	
	
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	errno = EINVAL;
	return -1;
	
    }


    if (cusfs_remove_superblock(cufs,0)) {

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

#if !defined(_MASTER_LOCK_CLIENT) && defined(_MASTER_LOCK)

    rc = cusfs_cleanup_sem(cufs->master_handle);
    if ( rc == -1 )
    {
        CUSFS_TRACE_LOG_FILE(1,"cleanup failed for file system for this disk %s",
                            device_name);
        CUSFS_RWUNLOCK(cusfs_global.global_lock);
        return rc;
    }

#endif

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    cusfs_release_cufs(cufs,0);
    return rc;
}


/*
 * NAME:        cusfs_fsck
 *
 * FUNCTION:    Run filesystem check and repair
 *              on a filesystem on the specified disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL for failure, otherwise success.
 *              
 *              
 */

int cusfs_fsck(char *device_name, int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;


    CUSFS_TRACE_LOG_FILE(5,"device_name = %s",
			device_name);

    if (device_name == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"device_name is null");
	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_find_fs_for_disk(device_name,0);

    if (cufs == NULL) {
	
	CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
			    device_name);
	
	errno = EINVAL;
	
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return rc;
	
    }

    //?? 

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    cusfs_release_cufs(cufs,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}



/*
 * NAME:        cusfs_mkdir
 *
 * FUNCTION:    Make a directory on in CAPI flash user space filesystem
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

int cusfs_mkdir(char *path,mode_t mode_flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    char device_name[PATH_MAX];
    char filename2[PATH_MAX];
    char *filename;
    char *parent_filename;
    char *directory_name;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_data_obj_t *parent_dir;
    uid_t uid;
    gid_t gid;




    CUSFS_TRACE_LOG_FILE(5,"pathxs = %s",
			path);
    if (path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"path is null");
	errno = EINVAL;
	return -1;
    } 


    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);
    
    if (file) {

	errno = EEXIST;

	CUSFS_TRACE_LOG_FILE(1,"filename already exists %s",path);

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;

    }

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    /*
     * File does not exist yet
     * Need to determine if the file system
     * exists.
     */

    strcpy(device_name,path);

	
    filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
			    device_name);

	errno = EINVAL;
	return rc;
    }

    filename[0] = '\0';
    filename++;

    cufs = cusfs_find_fs_for_disk(device_name,0);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
			    device_name);

	errno = EINVAL;

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return rc;
    }


    /*
     * Get parent filename of this file
     */

    strcpy(filename2,filename);

    directory_name = strrchr(filename2,'/');

    directory_name[0] = '\0';

    directory_name++;

    if (!strcmp(filename2,"")) {

	parent_filename = "/";
    } else {
	parent_filename = filename2;
    }


    if (directory_name == NULL) {

	errno = EINVAL;
	return -1;
    }    

    parent_dir = cusfs_lookup_data_obj(cufs,parent_filename,0);

    if (parent_dir == NULL) {

	return -1;
    }
    
    uid = getuid();
    
    gid = getgid();

    if (mode_flags == 0) {
	/*
	 * If no mode_flags were specified,
	 * then use the mode flags of the parent directory
	 */

	mode_flags = parent_dir->inode->mode_flags & (S_IRWXU|S_IRWXG|S_IRWXO);

    }

    CUSFS_LOCK(parent_dir->lock);

    file = cusfs_create_data_obj(cufs,directory_name,CFLSH_USFS_INODE_FILE_DIRECTORY,
				parent_dir->inode->index,mode_flags,uid,gid,NULL,0);
    

    if (file == NULL) {

	rc = errno;
    }

    CUSFS_UNLOCK(parent_dir->lock);

    /*
     * Put absolute path in
     * file struct
     */

    strcpy(device_name,path);

	
    filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename) {

	strcpy(file->filename,filename);
    }


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    if (rc) {

	errno = rc;

	rc = -1;
    }
    return rc;
}

/*
 * NAME:        cusfs_rmdir
 *
 * FUNCTION:    Remove a diectory from CAPI flash user space filesystem
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

int cusfs_rmdir(char *pathname)
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;


    CUSFS_TRACE_LOG_FILE(5,"pathname = %s",
			pathname);
    if (pathname == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"pathname is null");
	errno = EINVAL;
	return -1;
    } 

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(pathname,0);

	
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find file for pathname = %s",
			    pathname);
	errno = ENOENT;
	return -1;

    }

    cufs = file->fs;


    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for pathname = %s",
			    pathname);
	errno = EINVAL;
	
	return -1;
    }    

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	CUSFS_TRACE_LOG_FILE(1,"pathname = %s is not a directory",
			    pathname);
	errno = ENOTDIR;
	
	return -1;
	
    }

    rc = cusfs_free_data_obj(cufs,file,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        CUSFS_GET_FILE_HASH
 *
 * FUNCTION:    Find file from index in the hash table
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

static inline cflsh_usfs_data_obj_t *CUSFS_GET_FILE_HASH(int file_index, int check_rdy)
{
    cflsh_usfs_data_obj_t *file = NULL;
    cflsh_usfs_t *cufs;


    file = cusfs_global.file_hash[file_index & (MAX_NUM_CUSFS_FILE_HASH - 1)];


    while (file) {

        if ( (ulong)file & CUSFS_BAD_ADDR_MASK ) {

            CUSFS_TRACE_LOG_FILE(1,"Corrupted file address = 0x%llx, index = 0x%x", 
				 (uint64_t)file, (file_index & (MAX_NUM_CUSFS_FILE_HASH - 1)));

            cusfs_global.num_bad_file_ids++;
            file = NULL;

            break;

        }


	CUSFS_TRACE_LOG_FILE(9,"file address = 0x%llx, index = 0x%x, name = %s", 
			    (uint64_t)file, file->index, file->filename);

        if (file->index == file_index) {

            /*
             * Found the specified chunk. Let's
             * validate it.
             */

            if (CFLSH_EYECATCH_FILE(file)) {
                /*
                 * Invalid chunk
                 */
                
                CUSFS_TRACE_LOG_FILE(1,"Invalid file, index = %d",
                                    file_index);
                file = NULL;
            } 
#ifdef _NOT_YET
	    else if ((!(file->flags & CFLSH_FILE_RDY)) &&
                       (check_rdy)) {


                /*
                 * This chunk is not ready
                 */
                
                CUSFS_TRACE_LOG_FILE(1,"chunk not ready, file_index = %d",
                                    file_index);
                file = NULL;
            } 
#endif

            break;
        }

        file = file->fnext;


    }


    if (file) {

	cufs = file->fs;

	if (cufs) {


       
	    if ( (ulong)cufs & CUSFS_BAD_ADDR_MASK ) {

		CUSFS_TRACE_LOG_FILE(1,"Corrupted cufs address = 0x%llx for file %s", 
				    (uint64_t)cufs, file->filename);

		cusfs_global.num_bad_cusfs_ids++;

		errno = EFAULT;

		file = NULL;

	    }  
 
	} else {
	    errno = EINVAL;

	    return NULL;
	}
    }


    return file;

}


/*
 * NAME:        cusfs_opendir64/cusfs_opendir
 *
 * FUNCTION:    Opens a a directory 
 *              on a disk. 
 * 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL on error, otherwise success.
 *              
 *              
 */
#ifdef _AIX
DIR64 *cusfs_opendir64(char *directory_name)
#else
DIR *cusfs_opendir(char *directory_name)
#endif /* !_AIX */
{
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
#ifdef _AIX
    DIR64 *directory;
#else
    cflsh_usfs_DIR_t *directory;
#endif /* _AIX */


    if (directory_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory_name = NULL");

	errno = EINVAL;
	return NULL;
    } 

    CUSFS_TRACE_LOG_FILE(5,"directory_name = %s",
			directory_name);


    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(directory_name,0);

	
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory not found for directory_name = %s",
			    directory_name);

	errno = ENOENT;
	return NULL;

    }

    cufs = file->fs;


    if (cufs == NULL) {

	errno = EINVAL;
	
	return NULL;
    }    

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	errno = ENOTDIR;
	
	return NULL;
	
    }


    directory = malloc(sizeof(*directory));


    if (directory == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc directory failed for directory_name = %s on disk %s with errno = %d ",
			    directory_name,cufs->device_name,errno);
	return NULL;
    }

    bzero(directory,sizeof(*directory));


    directory->dd_fd = file->index;

    directory->dd_blksize = cufs->fs_block_size;

    directory->dd_loc = 0;

    directory->dd_curoff = 0;



    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    cusfs_add_file_to_hash(file,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

#ifdef _AIX 
    return directory;
#else
    return (DIR *)directory;
#endif
}

#ifdef _AIX
/*
 * NAME:        cusfs_opendir
 *
 * FUNCTION:    Opens a a directory 
 *              on a disk. For AIX, this routine use 
 *              the opendir64 routine.
 * 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL on error, otherwise success.
 *              
 *              
 */
DIR *cusfs_opendir(char *directory_name)
{
    DIR64 *directory64;
    DIR *directory;

    directory64 = cusfs_opendir64(directory_name);

    if (directory64 == NULL) {

	return NULL;
    }

    directory = malloc(sizeof(*directory));


    if (directory == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc directory failed for directory_name = %s with errno = %d ",
			    directory_name,errno);
	return NULL;
    }

    bzero(directory,sizeof(*directory));


    directory->dd_fd = directory64->dd_fd;

    directory->dd_blksize =  directory64->dd_blksize;

    directory->dd_loc = 0;

    directory->dd_curoff = 0;

    free(directory64);

    return (directory);

}
#endif

/*
 * NAME:        cusfs_closedir64/cusfs_closedir
 *
 * FUNCTION:    Closes a a directory
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
#ifdef _AIX
int cusfs_closedir64(DIR64 *directory)
#else
int cusfs_closedir(DIR *directory_linux)
#endif /* !_AIX */
{

    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
    int fd;
#ifndef _AIX
    cflsh_usfs_DIR_t *directory = (cflsh_usfs_DIR_t *)directory_linux;
#endif


    if (directory == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory_name = NULL");
	errno = EBADF;
	return 0;
    }

    fd = directory->dd_fd;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory not found on disk");

	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;

    } 

    CUSFS_TRACE_LOG_FILE(9,"directory %s on disk %s",
			file->filename,cufs->device_name);

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {


	CUSFS_TRACE_LOG_FILE(1,"file %s not directory on disk %s",
			    file->filename,cufs->device_name);
	errno = ENOTDIR;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	
	return -1;
	
    }

    cusfs_remove_file_from_hash(file,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    free (directory);

    CUSFS_TRACE_LOG_FILE(5,"rc = 0");

    return 0;
}

#ifdef _AIX
/*
 * NAME:        cusfs_closedir
 *
 * FUNCTION:    Closes a a directory
 *              on a disk. On AIX this routine
 *              relies on cusfs_closedir64.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int cusfs_closedir(DIR *directory)
{
    DIR64 *directory64;
    int rc = 0;

    directory64 = malloc(sizeof(*directory64));


    if (directory64 == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc directory64 failed with errno = %d ",
			     errno);
	return NULL;
    }

    bzero(directory64,sizeof(*directory64));


    directory64->dd_fd = directory->dd_fd;

    directory64->dd_blksize =  directory->dd_blksize;

    rc = cusfs_closedir64(directory64);

    free(directory64);

    return rc;
}
#endif /* _AIX */

/*
 * NAME:        cusfs_readdir64_r/cusfs_readdir_r
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
#ifdef _AIX
int cusfs_readdir64_r(DIR64 *directory, struct dirent64 *entry,struct dirent64 **result)
#else
int cusfs_readdir_r(DIR *directory_linux, struct dirent *entry,struct dirent **result)
#endif
{
    int rc = 0;
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
    cflash_offset_t offset;
#ifdef _AIX
    int namelen = MIN((CFLSH_USFS_DIR_MAX_NAME-1),_D_NAME_MAX);
#else
    cflsh_usfs_DIR_t *directory = (cflsh_usfs_DIR_t *)directory_linux;
    int namelen = MIN((CFLSH_USFS_DIR_MAX_NAME - 1),256);  /* Linux hard codes this 256 */
							  /* size in dirent.h          */
#endif
    int fd;
    cflsh_usfs_directory_entry_t disk_directory_entry;

    if (directory == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory = NULL on disk");

	errno = EBADF;
	return 0;
    }

    fd = directory->dd_fd;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory not found on disk");

	errno = EINVAL;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"file system  not found on disk");
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	CUSFS_TRACE_LOG_FILE(1,"file %s is not directory on disk %s",
			    file->filename,cufs->device_name);

	errno = ENOTDIR;

	return -1;
	
    }
    

    offset = directory->dd_loc;

    CUSFS_TRACE_LOG_FILE(1,"directory %s with offset 0x%llx on disk %s",
			file->filename,offset,cufs->device_name);



    if (cusfs_get_next_directory_entry(cufs,file,&disk_directory_entry,&offset,0)) {


	CUSFS_TRACE_LOG_FILE(1,"No more directory entries in directory %s on disk %s",
			    file->filename,cufs->device_name);
	*result = NULL;
	return 0; 
    }



    /*
     * Fill in entry with the current directory entry item
     */

    
    if (!(disk_directory_entry.flags & CFLSH_USFS_DIR_VAL)) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid directory entry in %s on disk %s",
			    file->filename,cufs->device_name);
	*result = NULL;
	return 0; 
    }

    entry->d_ino = disk_directory_entry.inode_number;



    directory->dd_loc = offset;
    directory->dd_curoff = offset;


    entry->d_reclen = sizeof(disk_directory_entry);
#ifdef _AIX
    entry->d_namlen = namelen;
    entry->d_offset = offset;
#else
    entry->d_off = offset;
#endif


    strncpy(entry->d_name,disk_directory_entry.filename,namelen);

    entry->d_name[namelen] = '\0';
    

    //?? update atime on directory entry


    *result = entry;

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d, offset = 0x%llx, dir->dd_loc = 0x%llx",
			rc, errno,offset,directory->dd_loc);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d");
    return rc;
}

/*
 * NAME:        cusfs_readdir64/cufs_readdir
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
#ifdef _AIX
struct dirent64 *cusfs_readdir64(DIR64 *directory)
#else
struct dirent *cusfs_readdir(DIR *directory)
#endif /* !AIX */
{
    int rc = 0;  
#ifdef _AIX 
    struct dirent64 entry;
    struct dirent64 *result = &entry;

    rc = cusfs_readdir64_r(directory,&entry,&result);
#else

    struct dirent entry;
    struct dirent *result = &entry;

    rc = cusfs_readdir_r(directory,&entry,&result);

#endif

    if ((rc) ||
	(result == NULL)) {

	return NULL;
    }

    result = malloc(sizeof(*result));


    if (result) {

	*result = entry;

    }
#ifdef _AIX
    CUSFS_TRACE_LOG_FILE(5,"result = %p, name = %s, offset = 0x%llx",
			 result,entry.d_name,entry.d_offset);
#else 
    CUSFS_TRACE_LOG_FILE(5,"result = %p, name = %s, offset = 0x%llx",
			 result,entry.d_name,entry.d_off);
#endif

    return (result);
}

#ifdef _AIX
/*
 * NAME:        cusfs_readdir
 *
 * FUNCTION:    Returns a pointer to the next directory entry.
 *              On AIX this is done with cusfs
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
struct dirent *cusfs_readdir(DIR *directory)
{

    DIR64 *directory64;
    struct dirent64 *entry64;
    struct dirent *entry = NULL;
    int rc = 0;

    directory64 = malloc(sizeof(*directory64));


    if (directory64 == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc directory64 failed with errno = %d ",
			     errno);
	return NULL;
    }

    bzero(directory64,sizeof(*directory64));


    directory64->dd_fd = directory->dd_fd;

    directory64->dd_blksize =  directory->dd_blksize;

    directory64->dd_loc =  directory->dd_loc;

    directory64->dd_curoff =  directory->dd_curoff;

    entry64 = cusfs_readdir64(directory64);

    if (entry64 == NULL) {
	
	free(directory64);
	return entry;
    }
	
    entry = malloc(sizeof(*entry));


    if (entry == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc entry failed with errno = %d ",
			     errno);
	free(directory64);
	return entry;
    }

    bzero(entry,sizeof(*entry));

    entry->d_reclen = entry64->d_reclen;

    entry->d_namlen = entry64->d_namlen;
    entry->d_offset =  entry64->d_offset;

    strncpy(entry->d_name,entry64->d_name,entry64->d_namlen);

    directory->dd_loc =  directory64->dd_loc;

    directory->dd_curoff =  directory64->dd_curoff;

    free(directory64);

    CUSFS_TRACE_LOG_FILE(5,"entry = %p, name = %s, errno = %d, offset = 0x%llx",
			 entry,entry->d_name,entry->d_offset);
    return entry;
}

#endif /* _AIX */

/*
 * NAME:        cusfs_telldir64/cusfs_telldir
 *
 * FUNCTION:    Returns the current location in the specfied directory.
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
#ifdef _AIX
offset_t cusfs_telldir64(DIR64 *directory)
#else
off_t cusfs_telldir(DIR *directory_linux)
#endif 
{
#ifdef _AIX
    offset_t rc = 0;
#else
    off_t rc = 0;
#endif
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
    int fd;
#ifndef _AIX
    cflsh_usfs_DIR_t *directory = (cflsh_usfs_DIR_t *)directory_linux;
#endif

    if (directory == NULL) {

	errno = EBADF;
	return -1;
    }

    fd = directory->dd_fd;


    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	errno = EINVAL;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	errno = ENOTDIR;

	
	return -1;
	
    }


    rc = directory->dd_curoff;


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);	

    return rc;
}


/*
 * NAME:        cusfs_seekdir64/cusfs_seekdir
 *
 * FUNCTION:    Sets the position of the next readdir64_r operation.
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
#ifdef _AIX
void cusfs_seekdir64(DIR64 *directory, offset_t location)
#else
void cusfs_seekdir(DIR *directory_linux, off_t location)
#endif 
{
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file;
    int fd;
#ifndef _AIX
    cflsh_usfs_DIR_t *directory = (cflsh_usfs_DIR_t *)directory_linux;
#endif

    if (directory == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory = NULL on disk");
	errno = EBADF;
	return;
    }


    fd = directory->dd_fd;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"directory not found on disk");

	errno = EINVAL;
	return;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"file system  not found on disk");

	errno = EINVAL;
	return;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	CUSFS_TRACE_LOG_FILE(1,"file %s is not directory on disk %s",
			    file->filename,cufs->device_name);

	errno = ENOTDIR;

	
	return;
	
    }


    directory->dd_curoff = location;
    directory->dd_loc = location;



    return;
}


/*
 * NAME:        cusfs_create_file
 *
 * FUNCTION:    Create new file in filesystem
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, otherwise success
 *              
 *              
 */

cflsh_usfs_data_obj_t *cusfs_create_file(cflsh_usfs_t *cufs, char *full_pathname,
	       mode_t mode_flags, uid_t uid, gid_t gid, int flags)
{

    char filename[PATH_MAX];
    char *parent_filename;
    char *end_parent_filename;
    cflsh_usfs_data_obj_t *file = NULL;
    cflsh_usfs_data_obj_t *parent_dir;
    


    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"file system is null");

	errno = EINVAL;
	return file;
    }    

    /*
     * ?? Do we have to handle disk_name:pathname here
     */

    if (full_pathname == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"full_pathaname is null on disk %s",
			    cufs->device_name);
	errno = EINVAL;
	return file;
    }  


    /*
     * Get parent filename of this file
     */

    strcpy(filename,full_pathname);

    end_parent_filename = strrchr(filename,'/');

    end_parent_filename[0] = '\0';

    end_parent_filename++;

    if (!strcmp(filename,"")) {

	parent_filename = "/";
    } else {
	parent_filename = filename;
    }

    parent_dir = cusfs_lookup_data_obj(cufs,parent_filename,0);

    if (parent_dir == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"parent_directory not found for %s on disk %s",
			    parent_filename,cufs->device_name);

	return file;
    }


    CUSFS_LOCK(parent_dir->lock);

    file = cusfs_create_data_obj(cufs,end_parent_filename,CFLSH_USFS_INODE_FILE_REGULAR,
				parent_dir->inode->index,mode_flags,uid,gid,NULL,0);

    CUSFS_UNLOCK(parent_dir->lock);

    if (file  == NULL) {

	return NULL;
    }


    strcpy(file->filename,full_pathname);

    CUSFS_TRACE_LOG_FILE(5,"Created file = %s with inode num = 0x%llx of type %d on disk %s, ",
			full_pathname,file->inode->index,file->inode->type, cufs->device_name);

    return file;
}


/*
 * NAME:        cusfs_create_link
 *
 * FUNCTION:    create a hard/sym link
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_create_link(char *orig_path, char *path, 
	      mode_t mode_flags,  uid_t uid, gid_t gid, 
	      int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs;
    char parent_filename[PATH_MAX];
    char *end_parent_filename;
    cflsh_usfs_data_obj_t *link = NULL;
    cflsh_usfs_data_obj_t *file = NULL;
    cflsh_usfs_data_obj_t *parent_dir;

   

    /*
     * ?? Do we have to handle disk_name:pathname here
     */

    if (orig_path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"orig_path is null");

	errno = EINVAL;
	return -1;
    }  



    if (path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"path is null");

	errno = EINVAL;
	return -1;
    }  

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_find_fs_from_path(orig_path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for orig_path = %s",
			    orig_path);

	errno = EINVAL;
	return -1;
    } 

    /*
     * Get old parent filename of this file
     */

    strcpy(parent_filename,orig_path);

    end_parent_filename = strrchr(path,'/');

    end_parent_filename[0] = '\0';

    
    end_parent_filename++;

    parent_dir = cusfs_lookup_data_obj(cufs,path,0);

    if (parent_dir == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"parent_directory not found for %s on disk %s",
			    path,cufs->device_name);

	errno = EINVAL;
	return -1;
    }


    if (flags & CFLSH_USFS_LINK_SYM_FLAG) {
	
	/*
	 * This is a symbolic link
	 */

	
	CUSFS_LOCK(parent_dir->lock);

	link = cusfs_create_data_obj(cufs,end_parent_filename,CFLSH_USFS_INODE_FILE_SYMLINK,
				    parent_dir->inode->index,mode_flags,uid,gid,
				    orig_path,
				    0);

	CUSFS_UNLOCK(parent_dir->lock);
	


    } else {
	
	/*
	 * This is a hard link
	 *
	 *
	 * Get original file this hard link will reference
	 */


	file = cusfs_lookup_data_obj(cufs,orig_path,0);

	if (file == NULL) {

	    free(parent_dir);

	    return -1;
	}

	file->data_buf_size = cufs->fs_block_size;

	file->data_buf = malloc(file->data_buf_size);


	if (file->data_buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to to malloc file->data_bu of size 0x%x for file = %s on disk %s",
				file->data_buf_size,file->filename,cufs->device_name);


	    cusfs_free_file(file);
	    free(parent_dir);

	    return -1;

	}

	if (file->inode->type == CFLSH_USFS_INODE_FILE_DIRECTORY) {


	    CUSFS_TRACE_LOG_FILE(1,"Failed because tried to hard link a directory %s on disk %s",
				orig_path,cufs->device_name);
	    cusfs_free_file(file);
	    free(parent_dir);

	    return -1;
	}
	
	CUSFS_LOCK(file->lock);


	CUSFS_LOCK(parent_dir->lock);

	link = cusfs_create_data_obj(cufs,end_parent_filename,CFLSH_USFS_INODE_FILE_REGULAR,
				    parent_dir->inode->index,mode_flags,uid,gid,
				    NULL,0);

	CUSFS_UNLOCK(parent_dir->lock);

	link = cusfs_alloc_file(cufs,end_parent_filename,0);

	if (link == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"malloc of file %s for failed for disk %s",
				end_parent_filename,cufs->device_name);
	    CUSFS_UNLOCK(file->lock);
	    cusfs_free_file(file);
	    free(parent_dir);

	    return -1;
	}


	CUSFS_LOCK(parent_dir->lock);
	if (cusfs_add_directory_entry(cufs,parent_dir->inode->index,file->inode,end_parent_filename,0)) {


		CUSFS_TRACE_LOG_FILE(1,"Failed to add directory entry to parent inode 0x%llx for file = %s on disk %s",
				    parent_dir->inode->index,end_parent_filename,cufs->device_name);

		CUSFS_UNLOCK(file->lock);
		cusfs_free_file(file);
		free(parent_dir);
		cusfs_free_file(link);

		return -1;
	}

	CUSFS_UNLOCK(parent_dir->lock);

	/*
	 * Increment count on original inode
	 */

	file->inode->num_links++;
	
	cusfs_update_inode(cufs,file->inode,0);

	
	CUSFS_UNLOCK(file->lock);


    }

    CUSFS_TRACE_LOG_FILE(5,"Created link = %s on disk %s",
			link->filename,cufs->device_name);
    cusfs_free_file(link);

    cusfs_free_file(file);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_remove_file
 *
 * FUNCTION:    Remove a file from CAPI flash user space filesystem
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

int cusfs_remove_file(char *pathname, int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs;
    cflsh_usfs_data_obj_t *file;
   

    /*
     * ?? Do we have to handle disk_name:pathname here
     */

    if (pathname == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"pathname is null");
	errno = EINVAL;
	return -1;
    } 

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cufs = cusfs_find_fs_from_path(pathname,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for pathname = %s",
			    pathname);

	errno = EINVAL;
	return -1;
    } 

    if (!strcmp(pathname,"/")) {

	CUSFS_TRACE_LOG_FILE(1,"Trying to remove root filesystem / failed for disk %s",
			   cufs->device_name);
	errno = EINVAL;
	return -1;
    }

    file = cusfs_lookup_data_obj(cufs,pathname,0);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"file not found for %s on disk %s",
			    pathname,cufs->device_name);

	return -1;
    }
   

    if (file->inode->type == CFLSH_USFS_INODE_FILE_DIRECTORY) {

	CUSFS_TRACE_LOG_FILE(1,"file %s is a directory on disk %s",
			    file->filename,cufs->device_name);
	errno = EISDIR;
	return -1;
	
    }
	
    rc = cusfs_free_data_obj(cufs,file,0);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_list_files
 *
 * FUNCTION:    List files from a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_list_files(char *pathname, int flags)
{
    int rc = 0;
    cflsh_usfs_t *cufs;
    cflsh_usfs_data_obj_t *file;
    

    /*
     * ?? Do we have to handle disk_name:pathname here
     */

    if (pathname == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"pathname is null");

	errno = EINVAL;
	return -1;
    } 

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    cufs = cusfs_find_fs_from_path(pathname,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for pathname = %s",
			    pathname);

	errno = EINVAL;
	return -1;
    }  

    file = cusfs_lookup_data_obj(cufs,pathname,0);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"file not found for %s on disk %s",
			    pathname,cufs->device_name);

	errno = ENOENT;
	return -1;
    }

    if (file->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	CUSFS_TRACE_LOG_FILE(1,"file %s is not a directory on disk %s",
			    file->filename,cufs->device_name);

	errno = ENOTDIR;

	return -1;
    }

    CUSFS_LOCK(file->lock);
    rc = cusfs_list_files_directory(cufs,file->inode,file->filename,flags);

    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_rename
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

int cusfs_rename(char *from_pathname,
		 char *to_pathname)
{
    int rc = 0;
    cflsh_usfs_t *cufs;
    cflsh_usfs_data_obj_t *file;


    /*
     * ?? Do we have to handle disk_name:pathname here
     */

    if (from_pathname == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"from_pathname is null");

	errno = EINVAL;
	return -1;
    }

    if (to_pathname == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"to_pathname is null");

	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = cusfs_open_disk_filename(from_pathname,0);

    cufs = file->fs;


    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"could not find filesystem for pathname = %s",
			   from_pathname);
	errno = EINVAL;
	return -1;
    } 

    CUSFS_LOCK(file->lock);
    rc = cusfs_mv_data_obj(cufs,file,to_pathname,0);

    CUSFS_UNLOCK(file->lock);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}




/*
 * NAME:        cusfs_open
 *
 * FUNCTION:    Open a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_open(char *path, int flags, mode_t mode)
{
    int rc = -1;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;
    char device_name[PATH_MAX];
    char *filename;
    uid_t uid;
    gid_t gid;

#ifdef _AIX
    if (flags & (O_EFSON|O_EFSOFF|O_RAW)) {


	/*
	 * We do not support encrypted filesystems
	 */

	CUSFS_TRACE_LOG_FILE(1,"Unsupported flags = 0x%x",flags);
	errno = EINVAL;

	return -1;
    }
#endif /* _AIX */

    CUSFS_TRACE_LOG_FILE(5,"path = %s, flags = %d, pid = %d",
			 path,flags,getpid());

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);
    
    if ((file == NULL) &&
	(flags & O_CREAT)) {


	CUSFS_TRACE_LOG_FILE(9,"file %s does not exist, creating...",
			    path);
	
	/*
	 * File does not exist yet.
	 * Only create it if the 0_CREATE flag
	 * is specified.
	 * 
	 * Need to first get file system
	 * object.
         */

	strcpy(device_name,path);

	
	filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

	if (filename == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
				device_name);

	    errno = EINVAL;
	    return rc;
	}

	filename[0] = '\0';
	filename++;

	cufs = cusfs_find_fs_for_disk(device_name,0);

	if (cufs == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
				device_name);

	    errno = EINVAL;

	    CUSFS_RWUNLOCK(cusfs_global.global_lock);
	    return rc;
	}


	/*
	 * If we get here,
	 * then the filesystem exits
	 */

	uid = getuid();


	/*
	 * ?? TODO: If parent directory has S_ISGID set, then
	 * group id should be set to the parent directories.
	 * group id.
	 */

	gid = getgid();

	file = cusfs_create_file(cufs,filename,mode,uid,gid,0);

	if (file == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to create file %s for this disk %s",
				filename,device_name);

	    rc = -1;
	} else {

	
	    if (file->disk_lbas == NULL) {
	
		if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

		    rc = -1;

		} else {

		    rc = file->index;
		}

	    } else {
		rc = file->index;
	    }

	}



    } else {

	cufs = file->fs;

	CUSFS_TRACE_LOG_FILE(9,"file %s already exists",
			    path);
	
	if (file->disk_lbas == NULL) {
	
	    if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

		rc = -1;

	    } else {

		 rc = file->index;
	    }

	} else {
	    rc = file->index;
	}
    }

    if (flags & O_SYNC) {
	file->flags |= CFLSH_USFS_SYNC_IO;
    }

    if (flags & O_DIRECT) {
	file->flags |= CFLSH_USFS_DIRECT_IO;
    }

    cusfs_add_file_to_hash(file,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}


/*
 * NAME:        cusfs_creat
 *
 * FUNCTION:    Create a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_creat(char *path, mode_t mode)
{
    int rc = -1;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;
    char device_name[PATH_MAX];
    char *filename;
    uid_t uid;
    gid_t gid;




    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			path);

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);
    
    if (file == NULL) {
	
	CUSFS_TRACE_LOG_FILE(9,"file %s does not exist, creating...",
			    path);
	
	/*
	 * File does not exist yet
	 * Need to determine if the file system
	 * exists.
         */

	strcpy(device_name,path);

	
	filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

	if (filename == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
				device_name);

	    errno = EINVAL;
	    return rc;
	}

	filename[0] = '\0';
	filename++;

	cufs = cusfs_find_fs_for_disk(device_name,0);

	if (cufs == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
				device_name);

	    errno = EINVAL;

	    CUSFS_RWUNLOCK(cusfs_global.global_lock);
	    return rc;
	}


	/*
	 * If we get here,
	 * then the filesystem exits
	 */

	uid = getuid();

	gid = getgid();

	file = cusfs_create_file(cufs,filename,mode,uid,gid,0);

	if (file == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to create file %s for this disk %s",
				filename,device_name);

	    rc = -1;
	} else {

	    rc = file->index;
	}



    } else {

	rc = file->index;
    }

    cusfs_add_file_to_hash(file,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;

}


/*
 * NAME:        cusfs_close
 *
 * FUNCTION:    Close a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_close(int fd)
{
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);

	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;

    } 

 
    CUSFS_LOCK(file->lock);

    if (cusfs_wait_for_aio_for_file(cufs,file,0)) {


	CUSFS_TRACE_LOG_FILE(5,"wait failed for closed file = %s of type %d on disk %s, ",
			     file->filename,file->inode->type, cufs->device_name);

	CUSFS_UNLOCK(file->lock);
	errno = EBUSY;
	return -1;
    }

    cusfs_flush_inode_timestamps(cufs,file);

    CUSFS_UNLOCK(file->lock);

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    cusfs_remove_file_from_hash(file,0);


    CUSFS_TRACE_LOG_FILE(5,"closed file = %s of type %d on disk %s, ",
			file->filename,file->inode->type, cufs->device_name);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    cusfs_display_stats(cufs,3);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}


/*
 * NAME:        cusfs_rdwr
 *
 * FUNCTION:    Read/Write a file of the specified size.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

ssize_t _cusfs_rdwr(int fd, struct aiocb64 *aiocbp, void *buffer,size_t nbytes,int flags)
{
    ssize_t rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    size_t nblocks;
    uint64_t cur_nblocks;
    uint64_t offset;
#ifdef _REMOVE
    int level;
#endif
    int read_ahead_retry;
    int aio_flags;
    void *buf;
    size_t buf_size;
    size_t nblocks_in_bytes;
    cflsh_usfs_disk_lba_e_t *disk_lbas;
    uint64_t num_disk_lbas;
    int i;
#ifdef _REMOVE
    int tag;
    uint64_t status;
#endif
    size_t bytes_transferred = 0;
    int copy_buf = FALSE;    /* Whether we need to copy data Copy data for */
			     /*	the caller, when I/O is not block aligned.  */


    

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if ((aiocbp) && (buffer == NULL)) {

	buffer = (void *)aiocbp->aio_buf;
    }

    if (buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Buffer is NULL for fd = %d",fd);
	errno = EINVAL;
	return -1;
    }

    if (aiocbp) {

	offset = aiocbp->aio_offset;
    } else {

	offset = file->offset;
    }


    if (nbytes == 0x0LL) {

	return 0;
    }


    CUSFS_TRACE_LOG_FILE(5,"%s%s file = %s of type %d, nbytes = 0x%llx, on disk %s, ",
			 (aiocbp ? "Async ":""),
			((flags & CFLSH_USFS_RDWR_WRITE) ? "Write":" Read"),
			file->filename,file->inode->type, nbytes,
			cufs->device_name);


    CUSFS_LOCK(cufs->lock);
    CUSFS_LOCK(file->lock);


    if ((offset + nbytes) > file->inode->file_size) {

	if (!(flags & CFLSH_USFS_RDWR_WRITE)) {

	    /*
	     * Do not allow read beyond end of file.
	     */

	    if (file->inode->file_size > offset) {

		nbytes = file->inode->file_size - offset;

	    } else {

	
		CUSFS_TRACE_LOG_FILE(1,"read beyond end of disk %s for file %",
				    cufs->device_name,file->filename);
		CUSFS_UNLOCK(file->lock);

		CUSFS_UNLOCK(cufs->lock);
		return -1;
		
	    }

	} else {

	    /* 
	     * This is a write beyond the current end of the file.
	     * Add more blocks if necessary to increase file size.
	     */

	    nblocks = (offset + nbytes)/cufs->fs_block_size;

	    if ((offset + nbytes) % cufs->fs_block_size) {

		nblocks++;

	    }

#ifdef _REMOVE
	    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&cur_nblocks,&level,0)) {
		
		
	
		CUSFS_UNLOCK(file->lock);

		CUSFS_UNLOCK(cufs->lock);
		return -1;
	    }

#endif /* REMOVE */

	    cur_nblocks = file->num_disk_lbas;

	    if (cur_nblocks <  nblocks) {

		/*
		 * We currently do not have enough blocks. 
		 * So allocate them now.
		 */

		nblocks -= cur_nblocks;
		
		CUSFS_TRACE_LOG_FILE(9,"For file = %s on disk %s need to add 0x%llx blocks ",
				    file->filename,cufs->device_name,nblocks);

		if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,file->inode,nblocks,0)) {

		    CUSFS_UNLOCK(file->lock);

		    CUSFS_UNLOCK(cufs->lock);
		    return -1;
		}

		free(file->disk_lbas);

		if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

		    /*
		     * This failure makes file unusable.  
		     * ?? TODO: Do we need something more here
		     */

		    CUSFS_UNLOCK(file->lock);

		    CUSFS_UNLOCK(cufs->lock);
		    return -1;
		}
	    }

	    /*
	     * Update Inode with new file
	     * size.
	     */

	    file->inode->file_size = offset + nbytes;

	    cusfs_update_inode(cufs,file->inode,0);

	}
    }


    if (cusfs_get_disk_lbas_for_transfer(cufs, file, offset,
					 nbytes, &disk_lbas,&num_disk_lbas,0)) {
	
	CUSFS_TRACE_LOG_FILE(1,"For file = %s on disk %s failed to get inode pointers offset = 0x%llx",
			    file->filename,cufs->device_name,offset);
	
	
	
	CUSFS_UNLOCK(file->lock);

	CUSFS_UNLOCK(cufs->lock);
	return -1;

    }


#ifdef _REMOVE 
    if (cusfs_get_disk_lbas_from_inode_ptr_for_transfer(cufs, file->inode, offset,
						       nbytes, &disk_lbas,&num_disk_lbas,0)) {
	
	CUSFS_TRACE_LOG_FILE(1,"For file = %s on disk %s failed to get inode pointers offset = 0x%llx",
			    file->filename,cufs->device_name,offset);
	
	
	
	CUSFS_UNLOCK(file->lock);

	CUSFS_UNLOCK(cufs->lock);
	return -1;

    }
#endif /* _REMOVE */



//??    buf_size = nbytes;


    bytes_transferred = 0;

    for (i = 0; i < num_disk_lbas; i++) {

	nblocks = MIN(disk_lbas[i].num_lbas,cufs->max_xfer_size);


	while (nblocks) {
	    
	    
	    nblocks_in_bytes = nblocks * cufs->disk_block_size;
	    
	    buf_size = nblocks_in_bytes;
	    
	    CUSFS_TRACE_LOG_FILE(9,"nblocks = 0x%llx buf_size = 0x%llx, i = 0x%x",
				 nblocks,buf_size,i);

	    if ((offset % cufs->disk_block_size) ||
		(((offset + nbytes) % cufs->disk_block_size) &&
		 (offset/cufs->disk_block_size == (offset + nbytes)/cufs->disk_block_size))) {

		/*
		 * If the start of the transfer is not aligned on a disk block
		 * boundary, then we need to use our internal copy buffer.
		 *
		 * Similarly if the end of the transfer is not disk block aligned, but it
		 * in the current nblocks where are doing I/O then we need to
		 * use our internal copy buffer.
		 */


		if (offset % cufs->disk_block_size) {

		    /*
		     * This request does not start on
		     * disk block boundary and it is
		     * in the nblocks being transferred now.
		     */
		    buf_size -= (offset % cufs->disk_block_size);

		}

		if (((offset + nbytes) % cufs->disk_block_size) &&
		    (offset/cufs->disk_block_size == (offset + nbytes)/cufs->disk_block_size)) {

		    /*
		     * This request does not end on
		     * disk block boundary and it is in the
		     * nblocks being transferred now.
		     */

		    buf_size -= (nblocks_in_bytes - ((offset + nbytes) % cufs->disk_block_size));

		}
		copy_buf = TRUE;

		buf = file->data_buf;
	    } else {

		copy_buf = FALSE;
		buf = (buffer + bytes_transferred);
	    }

	    CUSFS_TRACE_LOG_FILE(9,"nblocks = 0x%llx buf_size = 0x%llx, i = 0x%x, copy_buf = %d",
				 nblocks,buf_size,i,copy_buf);

	    aio_flags = 0;
	    if (flags & CFLSH_USFS_RDWR_WRITE) {
		
		if (copy_buf) {

		    /*
		     * For this case we need to do a read/modify/write
		     * First read in the full data blocks
		     */

		    rc = cblk_read(cufs->chunk_id,buf,disk_lbas[i].start_lba,nblocks,0);

		    if (rc < 0) {


			free(disk_lbas);

			CUSFS_UNLOCK(file->lock);

			CUSFS_UNLOCK(cufs->lock);
			return -1;
		    }

		    if (rc < nblocks) {

			free(disk_lbas);

			CUSFS_UNLOCK(file->lock);

			CUSFS_UNLOCK(cufs->lock);
			
			if (aiocbp) {

			    errno = EINVAL;
			    return -1;
			} else {
			    return bytes_transferred;
			}

		    }
		    

		    bcopy((buffer + bytes_transferred),buf,buf_size);
		}

		if (((cufs->flags & (CFLSH_USFS_DIRECT_IO|CFLSH_USFS_SYNC_IO)) ||
		     (cufs->async_pool_num_cmds == 0)) &&
		    (aiocbp == NULL)) {
		    /*
		     * We are not allowed to do async I/O  for this write.
		     * So issue a synchronous write.
		     */

		    rc = cblk_write(cufs->chunk_id,buf,disk_lbas[i].start_lba,nblocks,0);
		} else {
		    /*
		     * Issue asynchronous write
		     */

		    if (!copy_buf && aiocbp) {
			aio_flags = CFLSH_USFS_RDWR_DIRECT;
		    }

		    aio_flags |= CFLSH_USFS_RDWR_WRITE;

		    rc = cusfs_issue_aio(cufs,file,disk_lbas[i].start_lba,aiocbp,buf,buf_size,aio_flags);

		    
		}
	    } else {


		if ((file->seek_ahead.flags & CFLSH_USFS_D_RH_ACTIVE) &&
		    (file->seek_ahead.start_offset <= offset) &&
		    ((file->seek_ahead.nblocks * cufs->disk_block_size + file->seek_ahead.start_offset) >= (offset + buf_size))) {

		    /*
		     * If we issued an async read ahead for these lbas, then
		     * check for completion of that async read now.
		     */
		    
		    CUSFS_TRACE_LOG_FILE(9,"read ahead found. start_lba = 0x%llx,nblocks = 0x%llx",
					 file->seek_ahead.start_lba,file->seek_ahead.nblocks);

		    read_ahead_retry = 0;

		    while (read_ahead_retry++ < CFLSH_USFS_WAIT_READ_AHEAD_RETRY) {
			if ((file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_SUCCESS) ||
			    (file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_FAIL) ||
			    (file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_INVALID)) {

			    /*
			     * Read ahead completed.
			     */

			
			    file->seek_ahead.flags &= ~CFLSH_USFS_D_RH_ACTIVE;

			    if (file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_FAIL) {


				rc = -1;
				errno = file->seek_ahead.cblk_status.fail_errno;
			    } else if (file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_INVALID) {

				rc = -1;
				errno = EINVAL;
			    } else {



				if (file->seek_ahead.cblk_status.blocks_transferred) {

				    /*
				     * ?? This code is not handling the case where the transfer returned
				     *    success, but had a resid.
				     */

				    rc = buf_size/cufs->disk_block_size;

				}
				/*
				 * Set copy buff to allow the code below
				 * copy out the data.
				 */
				copy_buf = TRUE;

				buf = file->seek_ahead.local_buffer + (offset - file->seek_ahead.start_offset);


			    }

			    break;
			} else {

			    /*
			     * We need to wait for completion
			     */

			    usleep(CFLSH_USFS_WAIT_READ_AHEAD);
			}

			
		    }

		    if (read_ahead_retry >= CFLSH_USFS_WAIT_READ_AHEAD_RETRY) {

			rc = -1;
			errno = ETIMEDOUT;
		    }
		    
		    
		    CUSFS_TRACE_LOG_FILE(9,"read ahead found. rc = 0x%llx, errno = %d, read_ahead_retry = %d",
					 rc, errno, read_ahead_retry);

		} else {
		    
		    if (aiocbp) {
			/*
			 * Issue asynchronous read
			 */
			if (!copy_buf) {
			    aio_flags = CFLSH_USFS_RDWR_DIRECT;
			}
			rc = cusfs_issue_aio(cufs,file,disk_lbas[i].start_lba,aiocbp,buf,buf_size,aio_flags);
			
		    } else {
			
			/*
			 * Issue synchronous read
			 */
			rc = cblk_read(cufs->chunk_id,buf,disk_lbas[i].start_lba,
				       nblocks,0);
#ifdef _REMOVE
			rc = cblk_aread(cufs->chunk_id,buf,disk_lbas[i].start_lba,
					nblocks,&tag,NULL,0);
			
			CUSFS_TRACE_LOG_FILE(9,"rc = 0x%llx,nblocks = 0x%llx ",
					     rc,nblocks);
			if (rc == 0) {
			    CUSFS_TRACE_LOG_FILE(9,"rc = 0x%llx,nblocks = 0x%llx ",
						 rc,nblocks);
			    rc = cblk_aresult(cufs->chunk_id,&tag,&status,CBLK_ARESULT_BLOCKING);
			    
			    CUSFS_TRACE_LOG_FILE(9,"rc = 0x%llx,nblocks = 0x%llx errno = %d",
						 rc,nblocks,errno);
			    
			}
#endif
			
		    }
		}

	    }
	

	    if (rc < 0) {


		free(disk_lbas);

		CUSFS_UNLOCK(file->lock);

		CUSFS_UNLOCK(cufs->lock);
		return -1;
	    }

	    if (rc < nblocks) {


		CUSFS_TRACE_LOG_FILE(1,"%s%s of file = %s of type %d on disk %s failed to disk at lba 0x%llx, ",
			 (aiocbp ? "Async ":""),
				    ((flags & CFLSH_USFS_RDWR_WRITE) ? "Write":" Read"),
				    file->filename,file->inode->type, cufs->device_name,disk_lbas[i].start_lba);

	
		/*
		 * Return the total number of bytes read.
		 */

		if (!(flags & CFLSH_USFS_RDWR_WRITE)) {
		    if (copy_buf) {


			nblocks_in_bytes = rc * cufs->disk_block_size;
			
			buf_size = nblocks_in_bytes;


			if (offset % nblocks_in_bytes) {

			    /*
			     * This request does not start on
			     * disk block boundary and it is
			     * in the nblocks being transferred now.
			     */
			    buf_size -= (offset % nblocks_in_bytes);

			}

			if (((offset + nbytes) % nblocks_in_bytes) &&
			    (offset/nblocks_in_bytes == (offset + nbytes)/nblocks_in_bytes)) {

			    /*
			     * This request does not end on
			     * disk block boundary and it is in the
			     * nblocks being transferred now.
			     */

			    buf_size -= (nblocks_in_bytes - ((offset + nbytes) % nblocks_in_bytes));

			}

			bcopy(buf,(buffer + bytes_transferred),buf_size);
		    }
		}


		bytes_transferred += rc * cufs->disk_block_size;

		offset += rc * cufs->disk_block_size;

		free(disk_lbas);

		CUSFS_UNLOCK(file->lock);

		CUSFS_UNLOCK(cufs->lock);
		if (aiocbp) {
		    return 0;
		} else {
		    return bytes_transferred;
		}
	    }
	    
	    if (!(flags & CFLSH_USFS_RDWR_WRITE)) {
		if (copy_buf) {
		    
		    bcopy(buf,(buffer + bytes_transferred),buf_size);
		}
	    }


	    offset += buf_size;

	    bytes_transferred += buf_size;

	    if (nblocks > cufs->max_xfer_size) {

		
		CUSFS_TRACE_LOG_FILE(1,"nblocks too large = 0x%llx buf_size = 0x%llx, nbytes = 0x%llx",
				     nblocks,buf_size,nbytes);
		free(disk_lbas);

		CUSFS_UNLOCK(file->lock);

		CUSFS_UNLOCK(cufs->lock);

#ifndef ESYSERROR
		errno = ELIBBAD;
#else
		errno = ESYSERROR;
#endif

		return -1;

	    } else {
		nblocks = MIN((disk_lbas[i].num_lbas - nblocks),cufs->max_xfer_size);
	    }

	    if (bytes_transferred >= nbytes) {

		
		CUSFS_TRACE_LOG_FILE(5,"nblocks = 0x%llx buf_size = 0x%llx, nbytes = 0x%llx",
				     nblocks,buf_size,nbytes);
			
		if (bytes_transferred) {

		    rc = bytes_transferred;

		} else {

		    errno = EINVAL;
		    rc = -1;
		} 

		break;
	    }

	} /* while */
	

    } /* for */


    free(disk_lbas);

    CUSFS_TRACE_LOG_FILE(5,"%s%s success of file = %s of bytes 0x%llx, offset 0x%llx on disk %s, ",
			 (aiocbp ? "Async ":""),
			((flags & CFLSH_USFS_RDWR_WRITE) ? "Write":" Read"),
			file->filename,bytes_transferred, offset,cufs->device_name);

    if (aiocbp == NULL) {

	file->offset = offset;
    }



    if (flags & CFLSH_USFS_RDWR_WRITE) {

	file->inode_updates.flags |= CFLSH_USFS_D_INODE_MTIME;
#if !defined(__64BIT__) && defined(_AIX)
	file->inode_updates.mtime = cflsh_usfs_time64(NULL);
#else
	file->inode_updates.mtime = time(NULL);
#endif /* not 32-bit AIX */
    } else {

	file->inode_updates.flags |= CFLSH_USFS_D_INODE_ATIME;
#if !defined(__64BIT__) && defined(_AIX)
	file->inode_updates.atime = cflsh_usfs_time64(NULL);
#else
	file->inode_updates.atime = time(NULL);
#endif /* not 32-bit AIX */

    }

    
    CUSFS_UNLOCK(file->lock);

    CUSFS_UNLOCK(cufs->lock);

    if (aiocbp) {
	return 0;
    } else {
	return bytes_transferred;
    }

}



/*
 * NAME:        cusfs_read
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

ssize_t cusfs_read(int fd, void *buffer,size_t nbytes)
{

    return (_cusfs_rdwr(fd,NULL,buffer,nbytes,0));
}

/*
 * NAME:        cusfs_write
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

ssize_t cusfs_write(int fd, void *buffer,size_t nbytes)
{

    return (_cusfs_rdwr(fd,NULL,buffer,nbytes, CFLSH_USFS_RDWR_WRITE));
}


/*
 * NAME:        cusfs_fsync
 *
 * FUNCTION:    Write changes in a file to permanent storage
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_fsync(int fd)
{

    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);

	errno = EINVAL;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 


    CUSFS_TRACE_LOG_FILE(5,"fsync file = %s of type %d on disk %s, ",
			file->filename,file->inode->type, cufs->device_name);

    CUSFS_LOCK(file->lock);


    cusfs_flush_inode_timestamps(cufs,file);

    if (cusfs_wait_for_aio_for_file(cufs,file,CFLSH_USFS_ONLY_ASYNC_CMPLT)) {

	rc = -1;
    }

    CUSFS_UNLOCK(file->lock);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

/*
 * NAME:        cusfs_lseek
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

off_t cusfs_lseek(int fd, off_t Offset, int Whence)
{
    off_t rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

#if !defined(__64BIT__)  && defined(_AIX) 

    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s Offset = 0x%x of type %d on disk %s, ",
			file->filename,Offset,file->inode->type, cufs->device_name);
#else
    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s Offset = 0x%llx of type %d on disk %s, ",
			file->filename,Offset,file->inode->type, cufs->device_name);
#endif /* 64-bit */

    CUSFS_LOCK(file->lock);

    switch (Whence) {

      case SEEK_SET:
	if (Offset > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset = Offset;
	break;
      case SEEK_CUR:
	if ((file->offset + Offset) > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset += Offset;
	break;
      case SEEK_END:
	file->offset = file->inode->file_size;
	break;
      default:
	errno = EINVAL;
	return -1;
    }
#if !defined(__64BIT__)  && defined(_AIX)    
    rc = file->offset & 0xffffffff;
#endif /* AIX and 32-bit */

    /*
     * Issue read ahead at this offset
     */
    cusfs_seek_read_ahead(cufs,file,file->offset,0);

    CUSFS_UNLOCK(file->lock);

#if !defined(__64BIT__)  && defined(_AIX) 
    
    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s rc = 0x%x of type %d on disk %s, ",
			file->filename,rc,file->inode->type, cufs->device_name);
#else
    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s rc = 0x%llx of type %d on disk %s, ",
			file->filename,rc,file->inode->type, cufs->device_name);
#endif /* 64-bit */
    return rc;
}

#ifdef _AIX
/*
 * NAME:        cusfs_llseek
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

offset_t cusfs_llseek(int fd, offset_t Offset ,int Whence)
{
    offset_t rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EBADF;
	return -1;

    } 


    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s Offset = 0x%llx of type %d on disk %s, ",
			file->filename,Offset,file->inode->type, cufs->device_name);

    CUSFS_LOCK(file->lock);

    switch (Whence) {

      case SEEK_SET:
	if (Offset > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset = Offset;
	break;
      case SEEK_CUR:
	if ((file->offset + Offset) > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset += Offset;
	break;
      case SEEK_END:
	file->offset = file->inode->file_size;
	break;
      default:
	errno = EINVAL;
	return -1;
    }
    
    rc = file->offset;

    /*
     * Issue read ahead at this offset
     */
    cusfs_seek_read_ahead(cufs,file,file->offset,0);

    CUSFS_UNLOCK(file->lock);
    
    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s rc = 0x%llx of type %d on disk %s, ",
			file->filename,rc,file->inode->type, cufs->device_name);
    return rc;        
}


#endif /* AIX */

/*
 * NAME:        cusfs_lseek64
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

off64_t cusfs_lseek64(int fd, off64_t Offset, int Whence)
{  

    off64_t rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EBADF;
	return -1;

    } 


    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s Offset = 0x%llx of type %d on disk %s, ",
			file->filename,Offset,file->inode->type, cufs->device_name);

    CUSFS_LOCK(file->lock);

    switch (Whence) {

      case SEEK_SET:
	if (Offset > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset = Offset;
	break;
      case SEEK_CUR:
	if ((file->offset + Offset) > file->inode->file_size) {

	    errno = EINVAL;
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	file->offset += Offset;
	break;
      case SEEK_END:
	file->offset = file->inode->file_size;
	break;
      default:
	errno = EINVAL;
	return -1;
    }
    
    rc = file->offset;

    /*
     * Issue read ahead at this offset
     */
    cusfs_seek_read_ahead(cufs,file,file->offset,0);

    CUSFS_UNLOCK(file->lock);
    
    CUSFS_TRACE_LOG_FILE(5,"lseek file = %s rc = 0x%llx of type %d on disk %s, ",
			file->filename,rc,file->inode->type, cufs->device_name);
    return rc; 
}


/*
 * NAME:        cusfs_fstat
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

int cusfs_fstat(int fd,struct stat *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);


    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }

    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}




/*
 * NAME:        cusfs_fstat64
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

int cusfs_fstat64(int fd,struct stat64 *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;
	

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Stat buffer is null for fd = %d",fd);
	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);


    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }


    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

#ifdef _AIX

/*
 * NAME:        cusfs_fstat64x
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

int cusfs_fstat64x(int fd,struct stat64x *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;
	

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Stat buffer is null for fd = %d",fd);
	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);


    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }


    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

#endif /* _AIX */


/*
 * NAME:        cusfs_stat64
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

int cusfs_stat64(char *path, struct stat64 *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;
	

    CUSFS_TRACE_LOG_FILE(5,"path = %s, pid = %d",
			path,getpid());
    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for path = %s",path);
	errno = ENOENT;

	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for path = %s",path);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Stat buffer is null for path = %s",path);
	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);

    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }


    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

#ifdef _AIX

/*
 * NAME:        cusfs_stat64x
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

int cusfs_stat64x(char *path, struct stat64x *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;
	

    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			path);
    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for path = %s",path);
	errno = ENOENT;

	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for path = %s",path);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Stat buffer is null for path = %s",path);
	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);

    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }


    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}


#endif /* _AIX */

/*
 * NAME:        cusfs_stat
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
int cusfs_stat(char *path, struct stat *Buffer)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;
	

    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			path);
    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for path = %s",path);
	errno = ENOENT;

	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for path = %s",path);
	errno = EINVAL;
	return -1;

    } 

    if (Buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Stat buffer is null for path = %s",path);
	errno = EFAULT;
	return -1;

    }

    bzero(Buffer,sizeof(*Buffer));

    CUSFS_LOCK(file->lock);

    cusfs_flush_inode_timestamps(cufs,file);

    Buffer->st_mode = file->inode->mode_flags;

    Buffer->st_uid = file->inode->uid;

    Buffer->st_gid = file->inode->gid;

    Buffer->st_size = file->inode->file_size;

    Buffer->st_atime = file->inode->atime;
   
    Buffer->st_ctime = file->inode->ctime;
   
    Buffer->st_mtime = file->inode->mtime;

    Buffer->st_nlink = file->inode->num_links;

    Buffer->st_blksize = cufs->fs_block_size;


    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0) == 0) {

	
	Buffer->st_blocks = num_data_blocks;
    }


    Buffer->st_ino = file->inode->index;
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}


/*
 * NAME:        cusfs_lstat64
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
int cusfs_lstat64(char *path, struct stat64 *Buffer)
{
    //?? Add real support for lstat

    return(cusfs_stat64(path,Buffer));
}

#ifdef _AIX
/*
 * NAME:        cusfs_lstat64x
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
int cusfs_lstat64x(char *path, struct stat64x *Buffer)
{
    //?? Add real support for lstat

    return(cusfs_stat64x(path,Buffer));
}

#endif /* _AIX */

/*
 * NAME:        cusfs_lstat
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

int cusfs_lstat(char *path, struct stat *Buffer)
{  
    //?? Add real support for lstat


    return(cusfs_stat(path,Buffer));
}


/*
 * NAME:        cusfs_readlink
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
int cusfs_readlink(const char *path, char *buffer, size_t buffer_size)
{
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;


    if (buffer == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"buffer is null for path = %s",path);
	errno = EFAULT;
	return -1;

    }


    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			 path);
    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename((char *)path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for path = %s",path);
	errno = ENOENT;

	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for path = %s",path);
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_SYMLINK) {


	CUSFS_TRACE_LOG_FILE(1,"path = %s is not symlink",path);
	errno = EINVAL;
	return -1;
    }

    if (strlen(file->inode->sym_link_path) > buffer_size) {

	CUSFS_TRACE_LOG_FILE(1,"symlink %d is larger than buffer size %d for path = %s is not symlink",
			     strlen(file->inode->sym_link_path),buffer_size,path);
	errno = ERANGE;
	return -1;
    }


    bzero(buffer,buffer_size);


    strcpy(buffer,file->inode->sym_link_path);



    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			 rc, errno);

    return rc;
}


/*
 * NAME:        cusfs_access
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

int cusfs_access(char *path, int mode)
{ 
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    char device_name[PATH_MAX];
    char *filename;
	

    CUSFS_TRACE_LOG_FILE(5,"path = %s, mode = 0x%x",
			path,mode);
    CUSFS_WR_RWLOCK(cusfs_global.global_lock);

    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(9,"file % does not exist, creating...",
			    path);
	
	/*
	 * File does not exist yet
	 * Need to determine if the file system
	 * exists.
         */

	strcpy(device_name,path);

	
	filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

	if (filename == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
				device_name);

	    errno = EINVAL;
	    return rc;
	}

	filename[0] = '\0';
	filename++;

	cufs = cusfs_find_fs_for_disk(device_name,0);

	if (cufs == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
				device_name);

	    errno = EINVAL;

	    rc = -1;
	} else {


	    /*
	     * Filesystem exists, but file does not
	     */

	    errno = ENOENT;
	    rc = -1;
	}

    } else {


	cufs = file->fs;

	if (cufs == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"No filesystem found for path = %s",path);
	    errno = EINVAL;
	    return -1;

	} 

	if ((mode & R_OK) &&
	    !(file->inode->mode_flags & (S_IRUSR|S_IRGRP|S_IROTH))) {

	    CUSFS_TRACE_LOG_FILE(5,"path %s does not have read access",path);
	    rc = -1;
	    errno = EACCES;
	} else if ((mode & W_OK) &&
	    !(file->inode->mode_flags & (S_IWUSR|S_IWGRP|S_IWOTH))) {

	    CUSFS_TRACE_LOG_FILE(5,"path %s does not have write access",path);
	    rc = -1;
	    errno = EROFS;
	} else if ((mode & X_OK) &&
	    !(file->inode->mode_flags & (S_IXUSR|S_IXGRP|S_IXOTH))) {

	    CUSFS_TRACE_LOG_FILE(5,"path %s does not have execute access",path);
	    rc = -1;
	    errno = EACCES;
	}

    }


    CUSFS_TRACE_LOG_FILE(5,"path = %s, rc = %d, errno = %d",
			path,rc, errno);


    return rc;

}


/*
 * NAME:        cusfs_fchmod
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

int cusfs_fchmod(int fd,mode_t mode)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    }

    CUSFS_LOCK(file->lock);

    file->inode->mode_flags = (file->inode->mode_flags & S_IFMT) | mode;
	
    cusfs_update_inode(cufs,file->inode,0);
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

/*
 * NAME:        cusfs_fchown
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

int cusfs_fchown(int fd, uid_t uid, gid_t gid)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    CUSFS_LOCK(file->lock);

    if (uid != -1) {
	file->inode->uid = uid;
    }


    if (gid != -1) {
	file->inode->gid = gid;
    }
	
    cusfs_update_inode(cufs,file->inode,0);
    
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

/*
 * NAME:        cusfs_ftruncate
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

int cusfs_ftruncate(int fd, off_t length)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;

    uint64_t length_in_blocks;
    uint64_t delta_blocks;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    } 

    if (length <= 0) {
	
	return 0;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_REGULAR) {

	CUSFS_TRACE_LOG_FILE(1,"%s Not regular file, type = %d",
			    file->filename,file->inode->type);
	errno = EINVAL;

	return -1;
    }

    length_in_blocks = length/cufs->fs_block_size;

    if (length % cufs->fs_block_size) {
	length_in_blocks++;
    }

    CUSFS_LOCK(file->lock);

 
    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0)) {

	CUSFS_UNLOCK(file->lock);
	return -1;
	
    }
   

    if (num_data_blocks > length_in_blocks) {

	/*
	 * Remove data blocks from file
	 */

	delta_blocks = num_data_blocks - length_in_blocks;
	if (cusfs_remove_fs_blocks_from_inode_ptrs(cufs,file->inode,delta_blocks,0)) {


	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}


    } else if (num_data_blocks < length_in_blocks) {

	/*
	 * Add data blocks to a file
	 */
	
	delta_blocks =  length_in_blocks - num_data_blocks;

	if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,file->inode,delta_blocks,0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
    } 

    
    file->inode->file_size = length;
    cusfs_update_inode(cufs,file->inode,0);
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}


/*
 * NAME:        cusfs_ftruncate64
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

int cusfs_ftruncate64(int fd, off64_t length)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;

    uint64_t length_in_blocks;
    uint64_t delta_blocks;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    } 

    if (length <= 0) {
	
	return 0;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_REGULAR) {

	CUSFS_TRACE_LOG_FILE(1,"%s Not regular file, type = %d",
			    file->filename,file->inode->type);
	errno = EINVAL;

	return -1;
    }

    length_in_blocks = length/cufs->fs_block_size;

    if (length % cufs->fs_block_size) {
	length_in_blocks++;
    }

    CUSFS_LOCK(file->lock);

 
    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0)) {

	CUSFS_UNLOCK(file->lock);
	return -1;
	
    }
   

    if (num_data_blocks > length_in_blocks) {

	/*
	 * Remove data blocks from file
	 */

	delta_blocks = num_data_blocks - length_in_blocks;
	if (cusfs_remove_fs_blocks_from_inode_ptrs(cufs,file->inode,delta_blocks,0)) {


	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}

    } else if (num_data_blocks < length_in_blocks) {

	/*
	 * Add data blocks to a file
	 */
	
	delta_blocks =  length_in_blocks - num_data_blocks;

	if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,file->inode,delta_blocks,0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
    } 

    
    file->inode->file_size = length;
    cusfs_update_inode(cufs,file->inode,0);
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}


/*
 * NAME:        cusfs_posix_fadvise
 *
 * FUNCTION:    Provides advisory information about a file.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int cusfs_posix_fadvise(int fd, off_t offset, off_t length, int advise)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;

    uint64_t length_in_blocks;
    uint64_t delta_blocks;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    } 

    if (length <= 0) {
	
	return 0;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_REGULAR) {

	CUSFS_TRACE_LOG_FILE(1,"%s Not regular file, type = %d",
			    file->filename,file->inode->type);
	errno = EINVAL;

	return -1;
    }

    length_in_blocks = (offset + length)/cufs->fs_block_size;

    if (length % cufs->fs_block_size) {
	length_in_blocks++;
    }

    CUSFS_LOCK(file->lock);

 
    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0)) {

	CUSFS_UNLOCK(file->lock);
	return -1;
	
    }
   
    /*
     * Currently the only thing we use advise for is
     * as a suggestion of how large the file
     * might need to be.
     */

    if (num_data_blocks < length_in_blocks) {

	/*
	 * Add data blocks to a file
	 */
	
	delta_blocks =  length_in_blocks - num_data_blocks;

	if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,file->inode,delta_blocks,0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
    } 

    
    file->inode->file_size = length;
    cusfs_update_inode(cufs,file->inode,0);
    CUSFS_UNLOCK(file->lock);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d, length = 0x%llx, advice = 0x%x",
			 rc, errno,length,advise);

    return rc;
}


#ifndef _AIX

/*
 * NAME:        cusfs_posix_fadvise64
 *
 * FUNCTION:    Provides advisory information about a file.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

int cusfs_posix_fadvise64(int fd, off64_t offset, off64_t length, int advise)
{  
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs;
    uint64_t num_data_blocks;
    int level;

    uint64_t length_in_blocks;
    uint64_t delta_blocks;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    } 

    if (length <= 0) {
	
	return 0;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for fd = %d",fd);
	errno = EINVAL;
	return -1;

    } 

    if (file->inode->type != CFLSH_USFS_INODE_FILE_REGULAR) {

	CUSFS_TRACE_LOG_FILE(1,"%s Not regular file, type = %d",
			    file->filename,file->inode->type);
	errno = EINVAL;

	return -1;
    }

    length_in_blocks = (offset + length)/cufs->fs_block_size;

    if (length % cufs->fs_block_size) {
	length_in_blocks++;
    }

    CUSFS_LOCK(file->lock);

 
    if (cusfs_num_data_blocks_in_inode(cufs,file->inode,&num_data_blocks,&level,0)) {

	CUSFS_UNLOCK(file->lock);
	return -1;
	
    }
   
    /*
     * Currently the only thing we use advise for is
     * as a suggestion of how large the file
     * might need to be.
     */


    if (num_data_blocks < length_in_blocks) {

	/*
	 * Add data blocks to a file
	 */
	
	delta_blocks =  length_in_blocks - num_data_blocks;

	if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,file->inode,delta_blocks,0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
	
	free(file->disk_lbas);

	if (cusfs_get_all_disk_lbas_from_inode(cufs,file->inode,&(file->disk_lbas),&(file->num_disk_lbas),0)) {

	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}
    } 

    
    file->inode->file_size = length;
    cusfs_update_inode(cufs,file->inode,0);
    CUSFS_UNLOCK(file->lock);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d, length = 0x%llx, advice = 0x%x",
			 rc, errno,length,advise);

    return rc;
}

#endif /* !_AIX */

/*
 * NAME:        cusfs_link
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

int cusfs_link(char *orig_path, char *path)
{
    int rc = 0;
    cflsh_usfs_data_obj_t *link;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;
    char device_name[PATH_MAX];
    char *filename;
    uid_t uid;
    gid_t gid;
    mode_t mode;


    CUSFS_TRACE_LOG_FILE(5,"orig_path = %s, path",
			orig_path, path);

    if (orig_path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Orig_path is null");
	errno = EINVAL;
	return -1;
    }

    if (path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"path is null");
	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);


    file = cusfs_open_disk_filename(orig_path,0);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Could not find file for orig_path = %s",
			    file->filename);
	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    link = cusfs_open_disk_filename(path,0);
    
    if (link) {

	CUSFS_TRACE_LOG_FILE(1,"Filename %s for link already exists on disk %ss",
			    path,device_name);
	errno = EINVAL;
	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }


    /*
     * File does not exist yet
     * Need to determine if the file system
     * exists.
     */

    strcpy(device_name,path);

	
    filename = strchr(device_name,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
			    device_name);

	errno = EINVAL;
	return 01;
    }

    filename[0] = '\0';
    filename++;

    cufs = cusfs_find_fs_for_disk(device_name,0);

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Did not find a filesystem for this disk %s",
			    device_name);

	errno = EINVAL;

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    if (cufs != file->fs) {

	CUSFS_TRACE_LOG_FILE(1,"new file on disk %s in a different filesystem then original file on disk %s",
			    cufs->device_name,file->fs->device_name);

	errno = EINVAL;

	free(cufs);

	CUSFS_RWUNLOCK(cusfs_global.global_lock);
	return -1;
    }

    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    uid = getuid();

    gid = getgid();


    mode = S_IRWXU | S_IRWXG | S_IRWXO;

    rc = cusfs_create_link(orig_path,path,mode,uid,gid,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);

    return rc;
}

/*
 * NAME:        cusfs_unlink
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

int cusfs_unlink(char *path)
{
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;


    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			path);

    if (path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"path is null");
	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);


    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for %s",path);
	errno = ENOENT;


	return -1;
    }


    cufs = file->fs;


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 


    rc = cusfs_free_data_obj(cufs,file,0);

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_utime
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

int cusfs_utime(char *path,struct utimbuf *times)
{
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;

    CUSFS_TRACE_LOG_FILE(5,"path = %s",
			path);
    if (path == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"path is null");
	errno = EINVAL;
	return -1;
    }

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);


    file = cusfs_open_disk_filename(path,0);

    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    
    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for %s",path);
	errno = ENOENT;

	return -1;
    }


    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found for %s",path);
	errno = EINVAL;
	return -1;

    } 



    file->inode->mtime = times->modtime;
    file->inode->atime = times->actime;

    

    rc = cusfs_update_inode(cufs,file->inode,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

#ifdef _NOT_YET
/*
 * NAME:        cusfs_futimens
 *
 * FUNCTION:    set access and modification time of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_futimens(int fd,struct timespec times[2])
{
    int rc = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_t *cufs = NULL;

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);
    file = CUSFS_GET_FILE_HASH(fd,0);
    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    if (file == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No file found for fd = %d",fd);
	errno = EBADF;
	return -1;
    } 

    cufs = file->fs;

    if (cufs == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No filesystem found");
	errno = EINVAL;
	return -1;

    } 



    file->inode->mtime = times[0].tv_sec;
    file->inode->atime = times[1].tv_sec;

    

    rc = cusfs_update_inode(cufs,file->inode,0);


    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d,",
			rc, errno);
    return rc;
}
#endif



/*
 * NAME:        cusfs_aio_read64
 *
 * FUNCTION:    Read a file of the specified size asynchronously
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_read64(struct aiocb64 *aiocbp)
{

    /*
     * Set initial state of AIOCB
     */

    CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);
#ifdef _AIX
    aiocbp->aio_word1 = 0;
    if (aiocbp->aio_handle == NULL) {

	aiocbp->aio_handle = (aio_handle_t)aiocbp;
    }
#else
    aiocbp->__glibc_reserved[0] = 0;
    if (aiocbp->__next_prio == NULL) {
	aiocbp->__next_prio = (struct aiocb *)aiocbp;
    }
#endif 
    return (_cusfs_rdwr(aiocbp->aio_fildes,aiocbp,(void *)aiocbp->aio_buf,aiocbp->aio_nbytes,0));
}

/*
 * NAME:        cusfs_aio_read
 *
 * FUNCTION:    Read a file of the specified size asynchronously
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_read(struct aiocb *aiocbp)
{
    struct aiocb64 aiocbp64;


    /*
     * Set initial state of AIOCB
     */

    CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);


    aiocbp64.aio_fildes = aiocbp->aio_fildes;
    aiocbp64.aio_buf = aiocbp->aio_buf;
    aiocbp64.aio_nbytes = aiocbp->aio_nbytes;
    aiocbp64.aio_offset = aiocbp->aio_offset;
    aiocbp64.aio_sigevent = aiocbp->aio_sigevent;
#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp64.aio_handle = aiocbp;
    } else {
	aiocbp64.aio_handle = aiocbp->aio_handle;
    }
    aiocbp64.aio_word1 = CUSFS_AIOCB32_FLG;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp64.__next_prio = aiocbp;
    } else {
	aiocbp64.__next_prio = aiocbp->__next_prio;
    }
    
    aiocbp64.__glibc_reserved[0] = 0;
#endif

    return (cusfs_aio_read64(&aiocbp64));
}

/*
 * NAME:        cusfs_aio_write64
 *
 * FUNCTION:    Write a file of the specified size asynchronously
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_write64(struct aiocb64 *aiocbp)
{

    /*
     * Set initial state of AIOCB
     */

    CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);
#ifdef _AIX
    aiocbp->aio_word1 = 0;
    if (aiocbp->aio_handle == NULL) {

	aiocbp->aio_handle = (aio_handle_t)aiocbp;
    }
#else
    aiocbp->__glibc_reserved[0] = 0;
    if (aiocbp->__next_prio == NULL) {
	aiocbp->__next_prio = (struct aiocb *)aiocbp;
    }
#endif 
    return (_cusfs_rdwr(aiocbp->aio_fildes,aiocbp,(void *)aiocbp->aio_buf,aiocbp->aio_nbytes,CFLSH_USFS_RDWR_WRITE));
}

/*
 * NAME:        cusfs_aio_write
 *
 * FUNCTION:    Write a file of the specified size asynchronously
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_write(struct aiocb *aiocbp)
{
    struct aiocb64 aiocbp64;


    /*
     * Set initial state of AIOCB
     */

    CUSFS_RET_AIOCB(aiocbp,-1,EINPROGRESS);

    aiocbp64.aio_fildes = aiocbp->aio_fildes;
    aiocbp64.aio_buf = aiocbp->aio_buf;
    aiocbp64.aio_nbytes = aiocbp->aio_nbytes;
    aiocbp64.aio_offset = aiocbp->aio_offset;
    aiocbp64.aio_sigevent = aiocbp->aio_sigevent;
#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp64.aio_handle = aiocbp;
    } else {
	aiocbp64.aio_handle = aiocbp->aio_handle;
    }
    aiocbp64.aio_word1 = CUSFS_AIOCB32_FLG;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp64.__next_prio = aiocbp;
    } else {
	aiocbp64.__next_prio = aiocbp->__next_prio;
    }
    
    aiocbp64.__glibc_reserved[0] = 0;
#endif


    return (cusfs_aio_write64(&aiocbp64));
}

/*
 * NAME:        cusfs_aio_error64
 *
 * FUNCTION:    Get error status for the specified async I/O
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_error64(struct aiocb64 *aiocbp)
{

    int rc = 0;
    struct aiocb64 *aiocbp2;

#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = (struct aiocb64 *)aiocbp->aio_handle;
    }
    rc = aiocbp2->aio_errno;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = (struct aiocb64 *)aiocbp->__next_prio;
    }
    
    rc = aiocbp2->__error_code;
#endif




    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_aio_error
 *
 * FUNCTION:    Get error status for the specified async I/O
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_error(struct aiocb *aiocbp)
{

    int rc = 0;
    struct aiocb *aiocbp2;

#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = aiocbp->aio_handle;
    }
    rc = aiocbp2->aio_errno;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = aiocbp->__next_prio;
    }
    
    rc = aiocbp2->__error_code;
#endif



    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_aio_return64
 *
 * FUNCTION:    Return completion status for an aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_return64(struct aiocb64 *aiocbp)
{
    int rc = 0;
    struct aiocb64 *aiocbp2;

#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = (struct aiocb64 *)aiocbp->aio_handle;
    }
    errno = aiocbp2->aio_errno;
    rc = aiocbp->aio_return;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = (struct aiocb64 *)aiocbp->__next_prio;
    }
    
    errno = aiocbp2->__error_code;
    rc = aiocbp->__return_value;
#endif



    if (rc == 0) {

	CUSFS_TRACE_LOG_FILE(1,"rc = %d, errno = %d",
			rc, errno);
    }

    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}

/*
 * NAME:        cusfs_aio_return
 *
 * FUNCTION:    Return completion status for an aiocb
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_return(struct aiocb *aiocbp)
{
    int rc = 0;
    struct aiocb *aiocbp2;

#ifdef _AIX
    if (aiocbp->aio_handle == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = aiocbp->aio_handle;
    }
    errno = aiocbp2->aio_errno;
    rc = aiocbp->aio_return;
#else
    if (aiocbp->__next_prio == NULL) {

	aiocbp2 = aiocbp;
    } else {
	aiocbp2 = aiocbp->__next_prio;
    }
    
    errno = aiocbp2->__error_code;
    rc = aiocbp->__return_value;
#endif



    CUSFS_TRACE_LOG_FILE(5,"rc = %d, errno = %d",
			rc, errno);
    return (rc);
}

/*
 * NAME:        cusfs_aio_fsync64
 *
 * FUNCTION:    Write changes in a file to permanent storage
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_fsync64(int op,struct aiocb64 *aiocbp)
{

    return(cusfs_fsync(aiocbp->aio_fildes));

}

/*
 * NAME:        cusfs_aio_fsync
 *
 * FUNCTION:    Write changes in a file to permanent storage
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_fsync(int op,struct aiocb *aiocbp)
{

    return(cusfs_fsync(aiocbp->aio_fildes));

}


/*
 * NAME:        cusfs_aio_cancel64
 *
 * FUNCTION:    Cancel I/O operations for an aiocb.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_cancel64(int fd, struct aiocb64 *aiocbp)
{

    /*
     * Since we have no way to cancel requests,
     * we just want for them to be synced to the disk.
     */
    return (cusfs_fsync(fd));
}


/*
 * NAME:        cusfs_aio_cancel
 *
 * FUNCTION:    Cancel I/O operations for an aiocb.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_cancel(int fd, struct aiocb *aiocbp)
{

    /*
     * Since we have no way to cancel requests,
     * we just want for them to be synced to the disk.
     */
    return (cusfs_fsync(fd));
}

/*
 * NAME:        cusfs_aio_suspend64
 *
 * FUNCTION:    Wait for at least one of the operations to complete
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_suspend64(const struct aiocb64 *const list[], int nent,
                        const struct timespec *timeout)
{
    int i;
    int rc = 0;
    int retry = 0;
    int done = FALSE;

    CUSFS_TRACE_LOG_FILE(5,"nent = %d",nent);

    for (i= 0; i < nent; i++) {

	if (list[i] != NULL) {


	    while (retry < CFLSH_USFS_WAIT_AIO_RETRY) {
		rc = cusfs_aio_error64((struct aiocb64 *)list[i]);
		if (!rc) {

		    done = TRUE;
		    break;
		}

		if (rc < 0) {

		    break;
		}

		retry++;
		usleep(CFLSH_USFS_WAIT_AIO_DELAY);
	    }

	    if (done) {

		break;
	    }
	}
    }


    CUSFS_TRACE_LOG_FILE(5,"nent = %d, rc = %d",nent,rc);
    return rc;
}


/*
 * NAME:        cusfs_aio_suspend
 *
 * FUNCTION:    Wait for at least one of the operations to complete
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_aio_suspend(const struct aiocb *const list[], int nent,
                        const struct timespec *timeout)
{
    int i;
    int rc = 0;
    int retry = 0;
    int done = FALSE;

    CUSFS_TRACE_LOG_FILE(5,"nent = %d",nent);
    for (i= 0; i < nent; i++) {

	if (list[i] != NULL) {

	    while (retry < CFLSH_USFS_WAIT_AIO_RETRY) {
		rc = cusfs_aio_error((struct aiocb *)list[i]);
		if (!rc) {

		    done = TRUE;
		    break;
		}
		retry++;
		usleep(CFLSH_USFS_WAIT_AIO_DELAY);
	    }

	    if (done) {

		break;
	    }
	}
    }



    CUSFS_TRACE_LOG_FILE(5,"nent = %d, rc = %d",nent,rc);

    return rc;
}

