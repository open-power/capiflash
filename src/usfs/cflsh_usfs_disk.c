/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflash_block.c 1.8           */
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

#define CFLSH_USFS_FILENUM 0x0200

#include "cflsh_usfs_internal.h"
#include "cflsh_usfs_protos.h"
#include <sys/utsname.h>

/*
 * NAME:        cusfs_generate_fs_unique_id
 *
 * FUNCTION:    Get the disk block number for the filesystem block number.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 failure, Otherwise success
 *              
 *              
 */

cflsh_usfs_fs_id_t cusfs_generate_fs_unique_id(void)
{
    cflsh_usfs_fs_id_t fs_serial_num;
    struct  utsname uname_arg;
    int rc;
    uint *word_ptr;
    uint seed = 0;

    bzero(&fs_serial_num, sizeof(fs_serial_num));


    /*
     * The first 64-bits will contain machine id
     */


    rc = uname(&uname_arg);

    if (rc < 0) {

	return fs_serial_num;
    }


    CUSFS_TRACE_LOG_FILE(9,"uname: machined id = %s",
			 uname_arg.machine);


    sscanf(uname_arg.machine,"%16"PRIX64"",&fs_serial_num.val1);

    if (fs_serial_num.val1 == 0LL) {

	/*
	 * If serial number does not decode as number,
	 * then try to use the ASCII hex values instead.
	 */

	word_ptr = (uint *)&uname_arg.machine;

	fs_serial_num.val1 = *word_ptr;
    }


    /*
     * The second 64-bits will contain the time in seconds,
     * since epoch.
     */
#if !defined(__64BIT__) && defined(_AIX)
    fs_serial_num.val2 = (uint64_t)cflsh_usfs_time64(NULL);
#else 
    fs_serial_num.val2 = (uint64_t)time(NULL);
#endif /* not 32-bit AIX */

    /*
     * The third 64-bit will contain a random number 
     */

    word_ptr = (uint *)&fs_serial_num.val3;

    *word_ptr = rand_r(&seed);

    seed = *word_ptr + 5;

    word_ptr++;

 

    *word_ptr = rand_r(&seed);
    
    /*
     * The fourth 64-bit will be zero for now.
     */

    CUSFS_TRACE_LOG_FILE(9,"fs_serial num = 0x%llx 0x%llx 0x%llx 0x%llx",
			 fs_serial_num.val1,fs_serial_num.val2,fs_serial_num.val3,fs_serial_num.val4);

    return fs_serial_num;
}
	

/*
 * NAME:        cusfs_get_disk_block_no_from_fs_block_no
 *
 * FUNCTION:    Get the disk block number for the filesystem block number.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_get_disk_block_no_from_fs_block_no(cflsh_usfs_t *cufs, uint64_t fs_block_no, uint64_t *disk_block_no)
{
    int rc = 0;



    if (fs_block_no < cufs->superblock.fs_start_data_block) {

	CUSFS_TRACE_LOG_FILE(1,"Invalid fs_block_no = 0x%llx  fs_start_data_lba = 0x%llx for disk %s",
			    fs_block_no,cufs->superblock.fs_start_data_block, cufs->device_name);

	errno = ERANGE;
	return -1;

    }



    *disk_block_no = (fs_block_no * cufs->fs_block_size)/cufs->disk_block_size;


    if (*disk_block_no < cufs->superblock.start_data_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_block_no = 0x%llx  start_data_lba = 0x%llx for disk %s",
			    *disk_block_no,cufs->superblock.start_data_blocks,cufs->device_name);

	errno = ERANGE;
	return -1;
    }


    if (*disk_block_no > cufs->num_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_block_no = 0x%llx  last disk lba = 0x%llx for disk %s",
			    *disk_block_no,cufs->num_blocks,cufs->device_name);

	errno = ERANGE;
	return -1;
    }



    return rc;
}


/*
 * NAME:        cusfs_get_free_block_table_size
 *
 * FUNCTION:    Get size of free block table in blocks
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     size of free block table in blocks
 *              
 *              
 */

uint64_t cusfs_get_free_block_table_size(cflsh_usfs_t *cufs, 
					uint64_t num_blocks,
					int flags)
{
    uint64_t num_disk_blocks_free_blk_tbl = 0;


    /* 
     * If the end of the disk is not a full fs_block_size
     * then we ignore that last sectors up to fs_block_size
     *
     * Since each bit in the free block table represents a block of
     * size fs_block_size.  Lets first determine the
     * size in bytes this free block table. The free block data
     * table will represent all blocks on the disk include the metadata
     * We'll then mark the metadata blocks as in use.
     */





    /*
     * Determine how many disk blocks (of cufs->disk_block_size) are needed to represent 
     * this free table. Each bit in the free table corresponds to one
     * filesystem block size (cufs->fs_block_size). Thus num_inode_data_blocks is
     * a preliminary value for the number of bits in the free block table.
     * 
     */

    
    num_disk_blocks_free_blk_tbl = cufs->num_inode_data_blocks/(8*cufs->disk_block_size);

    if (cufs->num_inode_data_blocks % (8*cufs->disk_block_size)) {

	num_disk_blocks_free_blk_tbl++;
    }



    return num_disk_blocks_free_blk_tbl;
}



/*
 * NAME:        cusfs_create_superblock
 *
 * FUNCTION:    Create superblock for this filesystem
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_create_superblock(cflsh_usfs_t *cufs, int flags)
{
    cflsh_usfs_super_block_t *cusfs_super_block;
    int rc;
    char timebuf[TIMELEN+1];
    char ctime_string[TIMELEN+1];
    char wtime_string[TIMELEN+1];
    char mtime_string[TIMELEN+1];
    char ftime_string[TIMELEN+1];




    cusfs_super_block =  &cufs->superblock;

    bzero(cusfs_super_block, sizeof(*cusfs_super_block));


    cusfs_super_block->start_marker = CLFSH_USFS_SB_SM;

    cusfs_super_block->end_marker = CLFSH_USFS_SB_EM;
    

    cusfs_super_block->fs_unique_id = cusfs_generate_fs_unique_id();

#ifdef _NOT_YET
    if (cusfs_super_block->fs_unique_id.val1 == 0LL) {

	CUSFS_TRACE_LOG_FILE(1,"Invalid FS unique_id");
	return -1;
    }
#endif

    cusfs_super_block->fs_block_size = cufs->fs_block_size;

    cusfs_super_block->disk_block_size = cufs->disk_block_size;

    cusfs_super_block->os_type = cusfs_global.os_type;

    cusfs_super_block->num_blocks = cufs->num_blocks;

    if (cusfs_super_block->num_blocks < 
	(CFLSH_USFS_FREE_BLOCK_TABLE_LBA + 2*cufs->fs_block_size)) {

	CUSFS_TRACE_LOG_FILE(1,"Disk is too small for filesystem num_blocks = 0x%llx needds to be 0xllx",
                            cusfs_super_block->num_blocks,
			    (CFLSH_USFS_FREE_BLOCK_TABLE_LBA + 2*cufs->fs_block_size));
	return -1;

    }

    cusfs_super_block->journal_start_lba = CFLSH_USFS_JOURNAL_START_TABLE_LBA;

    cusfs_super_block->journal_table_size = CFLSH_USFS_JOURNAL_TABLE_SIZE;

    cusfs_super_block->free_block_table_stats_lba = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    cusfs_super_block->free_block_table_start_lba = CFLSH_USFS_FREE_BLOCK_TABLE_LBA;

    cusfs_super_block->free_block_table_size = cusfs_get_free_block_table_size(cufs,
							  cufs->num_blocks,
							  0);

    cusfs_super_block->inode_table_stats_lba = CFLSH_USFS_INODE_TBL_STATS_OFFSET;
    cusfs_super_block->inode_table_start_lba = CFLSH_USFS_FREE_BLOCK_TABLE_LBA +
	cusfs_super_block->free_block_table_size;

    
    cusfs_super_block->inode_table_size = (cufs->num_inode_data_blocks * sizeof(cflsh_usfs_inode_t))/cufs->disk_block_size;

    if ((cufs->num_inode_data_blocks * sizeof(cflsh_usfs_inode_t)) % cufs->disk_block_size) {

	cusfs_super_block->inode_table_size++;
    }

    cusfs_super_block->num_inodes_per_disk_block = cufs->disk_block_size/sizeof(cflsh_usfs_inode_t);


    cusfs_super_block->start_data_blocks = (cusfs_super_block->inode_table_start_lba + cusfs_super_block->inode_table_size + cufs->fs_block_size) & ~(cufs->fs_block_size - 1);

    cusfs_super_block->fs_start_data_block = (cusfs_super_block->start_data_blocks * cufs->disk_block_size)/cufs->fs_block_size;

    /*
     * Set create and write time now
     */
#if !defined(__64BIT__) && defined(_AIX)
    cusfs_super_block->create_time = cflsh_usfs_time64(NULL);
    cusfs_super_block->write_time = cflsh_usfs_time64(NULL);
#else
    cusfs_super_block->create_time = time(NULL);
    cusfs_super_block->write_time = time(NULL);
#endif /* not 32-bit AIX */

    bzero(cufs->buf,cufs->disk_block_size);

    rc = cblk_read(cufs->chunk_id,cufs->buf,CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }


    cusfs_super_block = cufs->buf;

    if (cusfs_super_block->start_marker == CLFSH_USFS_SB_SM) {


	CUSFS_TRACE_LOG_FILE(5,"superblock at lba = 0x%llx already exists!!",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);

	CUSFS_TRACE_LOG_FILE(5,"version = 0x%x, status = 0x%x. os_type = 0x%x flags = 0x%x",
                            cusfs_super_block->version,cusfs_super_block->status,
			    cusfs_global.os_type,cusfs_super_block->flags);

	CUSFS_TRACE_LOG_FILE(5,"disk_block_size = 0x%x, fs_block_size = 0x%x. num_blocks = 0x%llx",
                            cusfs_super_block->disk_block_size,cusfs_super_block->fs_block_size,
			    cusfs_super_block->num_blocks);


	CUSFS_TRACE_LOG_FILE(5,"inode_table_start_lba = 0x%llx, inode_size = 0x%llx",
                            cusfs_super_block->inode_table_start_lba,
			    cusfs_super_block->inode_table_size);

	CUSFS_TRACE_LOG_FILE(5,"free_block_table_start = 0x%llx, free_block_table_size = 0x%llx",
                            cusfs_super_block->free_block_table_start_lba,
			    cusfs_super_block->free_block_table_size);


	CUSFS_TRACE_LOG_FILE(5,"journal_start_lba = 0x%llx, journal_table_size = 0x%llx",
                            cusfs_super_block->journal_start_lba,
			    cusfs_super_block->journal_table_size);


	/*
	 * ctime_r adds a newline character at the end. So strip
	 * that off for all times here to make traces presentable.
	 */
#if !defined(__64BIT__) && defined(_AIX)

	sprintf(ctime_string,"%s",ctime64_r(&(cusfs_super_block->create_time),timebuf));

	ctime_string[strlen(ctime_string) - 1] = '\0';

	sprintf(wtime_string,"%s",ctime64_r(&(cusfs_super_block->write_time),timebuf));

	wtime_string[strlen(wtime_string) - 1] = '\0';

	sprintf(mtime_string,"%s",ctime64_r(&(cusfs_super_block->mount_time),timebuf));

	mtime_string[strlen(mtime_string) - 1] = '\0';

	sprintf(ftime_string,"%s",ctime64_r(&(cusfs_super_block->fsck_time),timebuf));

	ftime_string[strlen(ftime_string) - 1] = '\0';
#else

	sprintf(ctime_string,"%s",ctime_r(&(cusfs_super_block->create_time),timebuf));

	ctime_string[strlen(ctime_string) - 1] = '\0';

	sprintf(wtime_string,"%s",ctime_r(&(cusfs_super_block->write_time),timebuf));

	wtime_string[strlen(wtime_string) - 1] = '\0';

	sprintf(mtime_string,"%s",ctime_r(&(cusfs_super_block->mount_time),timebuf));

	mtime_string[strlen(mtime_string) - 1] = '\0';

	sprintf(ftime_string,"%s",ctime_r(&(cusfs_super_block->fsck_time),timebuf));

	ftime_string[strlen(ftime_string) - 1] = '\0';
#endif



	CUSFS_TRACE_LOG_FILE(5,"Creation time %s, Last write time %s, mount time %s, fsck_time %s",
			    ctime_string,wtime_string,mtime_string,ftime_string);


	if (!(flags & CFLSH_USFS_FORCE_SB_CREATE)) {
	    errno = EEXIST;
	    return -1;
	}
	
    }

    if (CFLASH_REV64(cusfs_super_block->start_marker) == CLFSH_USFS_SB_SM) {


	CUSFS_TRACE_LOG_FILE(5,"superblock at lba = 0x%llx already exists buf for different endian host",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);

	CUSFS_TRACE_LOG_FILE(5,"version = 0x%x, status = 0x%x. os_type = 0x%x flags = 0x%x",
                            CFLASH_REV32(cusfs_super_block->version),
			    CFLASH_REV32(cusfs_super_block->status),
			    CFLASH_REV32(cusfs_global.os_type),
			    CFLASH_REV32(cusfs_super_block->flags));

	CUSFS_TRACE_LOG_FILE(5,"disk_block_size = 0x%x, fs_block_size = 0x%x. num_blocks = 0x%llx",
                            CFLASH_REV32(cusfs_super_block->disk_block_size),
			    CFLASH_REV64(cusfs_super_block->fs_block_size),
			    CFLASH_REV64(cusfs_super_block->num_blocks));


	CUSFS_TRACE_LOG_FILE(5,"inode_table_start_lba = 0x%llx, inode_size = 0x%llx",
                            CFLASH_REV64(cusfs_super_block->inode_table_start_lba),
			    CFLASH_REV64(cusfs_super_block->inode_table_size));

	CUSFS_TRACE_LOG_FILE(5,"free_block_table_start = 0x%llx, free_block_table_size = 0x%llx",
                            CFLASH_REV64(cusfs_super_block->free_block_table_start_lba),
			    CFLASH_REV64(cusfs_super_block->free_block_table_size));


	CUSFS_TRACE_LOG_FILE(5,"journal_start_lba = 0x%llx, journal_table_size = 0x%llx",
                            CFLASH_REV64(cusfs_super_block->journal_start_lba),
			    CFLASH_REV64(cusfs_super_block->journal_table_size));



	if (!(flags & CFLSH_USFS_FORCE_SB_CREATE)) {
#ifdef _AIX
	    errno = ECORRUPT;
#else
	    errno = EBADR;
#endif /* ! AIX */
	    return -1;
	}
	
	
    }

    bzero(cufs->buf,cufs->disk_block_size);

    *cusfs_super_block = cufs->superblock;


    rc = cblk_write(cufs->chunk_id,cufs->buf, CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }

    cufs->flags |= CFLSH_USFS_FLG_SB_VAL;

    CUSFS_TRACE_LOG_FILE(5,"Superblock successfully written to disk %s",
	cufs->device_name);

    return 0;
}


/*
 * NAME:        cusfs_update_superblock
 *
 * FUNCTION:    Update superblock for this filesystem
 *
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_update_superblock(cflsh_usfs_t *cufs, int flags)
{
    cflsh_usfs_super_block_t *cusfs_super_block;
    int rc;


    bzero(cufs->buf,cufs->disk_block_size);

    rc = cblk_read(cufs->chunk_id,cufs->buf,CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }



    cusfs_super_block =  &cufs->superblock;

    /*
     * Update write time
     */

#if !defined(__64BIT__) && defined(_AIX)
    cusfs_super_block->write_time = cflsh_usfs_time64(NULL);
#else
    cusfs_super_block->write_time = time(NULL);
#endif /* not 32-bit AIX */


    cusfs_super_block = cufs->buf;

    *cusfs_super_block = cufs->superblock;


    rc = cblk_write(cufs->chunk_id,cufs->buf, CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }


    CUSFS_TRACE_LOG_FILE(5,"Superblock successfully updated and written to disk %s",
	cufs->device_name);

    return 0;
}


/*
 * NAME:        cusfs_remove_superblock
 *
 * FUNCTION:    Remove superblock from this disk\
 *              and thus remove the filesystem.
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_remove_superblock(cflsh_usfs_t *cufs, int flags)
{
    int rc;


    bzero(cufs->buf,cufs->disk_block_size);


    rc = cblk_write(cufs->chunk_id,cufs->buf, CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }


    CUSFS_TRACE_LOG_FILE(5,"Superblock successfully removed and written to disk %s",
	cufs->device_name);

    return 0;
}


/*
 * NAME:        cusfs_validate_superblock
 *
 * FUNCTION:    Validate an existing superblock for this filesystem
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_validate_superblock(cflsh_usfs_t *cufs, int flags)
{
    cflsh_usfs_super_block_t *cusfs_super_block;
    int rc;
    char timebuf[TIMELEN+1];
    char ctime_string[TIMELEN+1];
    char wtime_string[TIMELEN+1];

#if !defined(__64BIT__) && defined(_AIX)
    time64_t rev_ctime, rev_wtime;
#else
    time_t rev_ctime, rev_wtime;
#endif

    bzero(cufs->buf,cufs->disk_block_size);

    rc = cblk_read(cufs->chunk_id,cufs->buf,CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }


    cusfs_super_block = cufs->buf;



    CUSFS_TRACE_LOG_FILE(5,"version = 0x%x, status = 0x%x. os_type = 0x%x flags = 0x%x",
			cusfs_super_block->version,cusfs_super_block->status,
			cusfs_super_block->os_type,cusfs_super_block->flags);

    CUSFS_TRACE_LOG_FILE(5,"disk_block_size = 0x%x, fs_block_size = 0x%llx, num_blocks = 0x%llx",
			cusfs_super_block->disk_block_size,cusfs_super_block->fs_block_size,
			cusfs_super_block->num_blocks);


    CUSFS_TRACE_LOG_FILE(5,"inode_table_start_lba = 0x%llx, inode_size = 0x%llx",
			cusfs_super_block->inode_table_start_lba,
			cusfs_super_block->inode_table_size);

    CUSFS_TRACE_LOG_FILE(5,"free_block_table_start = 0x%llx, free_block_table_size = 0x%llx",
			cusfs_super_block->free_block_table_start_lba,
			cusfs_super_block->free_block_table_size);


    CUSFS_TRACE_LOG_FILE(5,"journal_start_lba = 0x%llx, journal_table_size = 0x%llx",
			cusfs_super_block->journal_start_lba,
			cusfs_super_block->journal_table_size);


    CUSFS_TRACE_LOG_FILE(9,"fs_serial num = 0x%llx 0x%llx 0x%llx 0x%llx",
			 cusfs_super_block->fs_unique_id.val1,
			 cusfs_super_block->fs_unique_id.val2,
			 cusfs_super_block->fs_unique_id.val3,
			 cusfs_super_block->fs_unique_id.val4);

    //?? Need check for endianness maybe just look at cusfs_super_block->start_marker ?


    if (cusfs_super_block->start_marker != CLFSH_USFS_SB_SM) {

	CUSFS_TRACE_LOG_FILE(1,"Not valid super block start_marker = 0x%llx",
			    cusfs_super_block->start_marker);

	CUSFS_TRACE_LOG_FILE(1,"expected tart_marker = 0x%llx",
			    CLFSH_USFS_SB_SM);

	errno = EINVAL;
	if (CFLASH_REV64(cusfs_super_block->start_marker) == CLFSH_USFS_SB_SM) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"Actually superblock is valid for different endianness host",
				cusfs_super_block->start_marker);

	    CUSFS_TRACE_LOG_FILE(1,"version = 0x%x, status = 0x%x, os_type = 0x%x flags = 0x%x",
				CFLASH_REV32(cusfs_super_block->version),
				CFLASH_REV32(cusfs_super_block->status),
				CFLASH_REV32(cusfs_super_block->os_type),
				CFLASH_REV32(cusfs_super_block->flags));

	    CUSFS_TRACE_LOG_FILE(5,"disk_block_size = 0x%x, fs_block_size = 0x%llx, num_blocks = 0x%llx",
				CFLASH_REV32(cusfs_super_block->disk_block_size),
				CFLASH_REV64(cusfs_super_block->fs_block_size),
				CFLASH_REV64(cusfs_super_block->num_blocks));


	    CUSFS_TRACE_LOG_FILE(5,"inode_table_start_lba = 0x%llx, inode_size = 0x%llx",
				CFLASH_REV64(cusfs_super_block->inode_table_start_lba),
				CFLASH_REV64(cusfs_super_block->inode_table_size));

	    CUSFS_TRACE_LOG_FILE(5,"free_block_table_start = 0x%llx, free_block_table_size = 0x%llx",
				CFLASH_REV64(cusfs_super_block->free_block_table_start_lba),
				CFLASH_REV64(cusfs_super_block->free_block_table_size));


	    CUSFS_TRACE_LOG_FILE(5,"journal_start_lba = 0x%llx, journal_table_size = 0x%llx",
				CFLASH_REV64(cusfs_super_block->journal_start_lba),
				CFLASH_REV64(cusfs_super_block->journal_table_size));



		rev_ctime = CFLASH_REV64(cusfs_super_block->create_time);
		rev_wtime = CFLASH_REV64(cusfs_super_block->write_time);


	    

	    /*
	     * ctime_r adds a newline character at the end. So strip
	     * that off for all times here to make traces presentable.
	     */
#if !defined(__64BIT__) && defined(_AIX)

	    sprintf(ctime_string,"%s",ctime64_r(&rev_ctime,timebuf));

	    ctime_string[strlen(ctime_string) - 1] = '\0';

	    sprintf(wtime_string,"%s",ctime64_r(&rev_wtime,timebuf));

	    wtime_string[strlen(wtime_string) - 1] = '\0';
#else
	    sprintf(ctime_string,"%s",ctime_r(&rev_ctime,timebuf));

	    ctime_string[strlen(ctime_string) - 1] = '\0';

	    sprintf(wtime_string,"%s",ctime_r(&rev_wtime,timebuf));

	    wtime_string[strlen(wtime_string) - 1] = '\0';
#endif

	    CUSFS_TRACE_LOG_FILE(5,"Creation time %s, last write time %s",
				ctime_string,wtime_string);


#ifdef _AIX
	    errno = ECORRUPT;
#else
	    errno = EBADR;
#endif /* ! AIX */


	} 

	return -1;

    }


    if (cusfs_super_block->end_marker != CLFSH_USFS_SB_EM) {

	CUSFS_TRACE_LOG_FILE(1,"Not valid super block end_marker = 0x%llx",
			    cusfs_super_block->end_marker);

	errno = EINVAL;

	return -1;

    }

    /*
     * ctime_r adds a newline character at the end. So strip
     * that off for all times here to make traces presentable.
     */

#if !defined(__64BIT__) && defined(_AIX)
  
    sprintf(ctime_string,"%s",ctime64_r(&(cusfs_super_block->create_time),timebuf));

    ctime_string[strlen(ctime_string) - 1] = '\0';

    sprintf(wtime_string,"%s",ctime64_r(&(cusfs_super_block->write_time),timebuf));

    wtime_string[strlen(wtime_string) - 1] = '\0';


    CUSFS_TRACE_LOG_FILE(5,"Creation time %s, Last write time %s, mount time %s, fsck_time %s",
			ctime_string,wtime_string,
			ctime64_r(&(cusfs_super_block->mount_time),timebuf),
			ctime64_r(&(cusfs_super_block->fsck_time),timebuf));
#else
	    
    sprintf(ctime_string,"%s",ctime_r(&(cusfs_super_block->create_time),timebuf));

    ctime_string[strlen(ctime_string) - 1] = '\0';

    sprintf(wtime_string,"%s",ctime_r(&(cusfs_super_block->write_time),timebuf));

    wtime_string[strlen(wtime_string) - 1] = '\0';


    CUSFS_TRACE_LOG_FILE(5,"Creation time %s, Last write time %s, mount time %s, fsck_time %s",
			ctime_string,wtime_string,
			ctime_r(&(cusfs_super_block->mount_time),timebuf),
			ctime_r(&(cusfs_super_block->fsck_time),timebuf));

#endif

    if (cusfs_super_block->os_type != cusfs_global.os_type) {

	CUSFS_TRACE_LOG_FILE(1,"os_type differences: our os_type  = 0x%x, but super block's os_type = 0x%x",
			    cusfs_global.os_type,
			    cusfs_super_block->os_type);

	errno = EINVAL;

	return -1;

	
    }

    if (cusfs_super_block->disk_block_size != cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"disk_block_size differences: our disk_block_size  = 0x%x, but super block's disk_block_size = 0x%x",
			    cufs->disk_block_size,
			    cusfs_super_block->disk_block_size);

	errno = EINVAL;

	return -1;
	
    }


    if (cusfs_super_block->fs_block_size != cufs->fs_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"fs_block_size differences: our disk_block_size  = 0x%x, but super block's fs_block_size = 0x%x",
			    cufs->fs_block_size,
			    cusfs_super_block->fs_block_size);

	errno = EINVAL;

	return -1;
	
    }

    if (cusfs_super_block->num_blocks != cufs->num_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"num_blocks differences: our num_blocks  = 0x%llx, but super block's num_blocks = 0x%llx",
			    cufs->num_blocks,
			    cusfs_super_block->num_blocks);

	errno = EINVAL;

	return -1;
	
    }

    cufs->superblock = *cusfs_super_block;

    return 0;
}



/*
 * NAME:        cusfs_query_superblock
 *
 * FUNCTION:    Query for a superblock on the specified disk.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_query_superblock(cflsh_usfs_t *cufs, cflsh_usfs_super_block_t *superblock,int flags)
{
    cflsh_usfs_super_block_t *cusfs_super_block;
    int rc;

    bzero(cufs->buf,cufs->disk_block_size);

    rc = cblk_read(cufs->chunk_id,cufs->buf,CFLSH_USFS_PRIMARY_SB_OFFSET,1,0);

    if (rc < 1) {

	CUSFS_TRACE_LOG_FILE(1,"read of superblock at lba = 0x%llx failed with errno = %d",
                            CFLSH_USFS_PRIMARY_SB_OFFSET,errno);
	return -1;
    }


    cusfs_super_block = cufs->buf;

    *superblock = *cusfs_super_block;



    return 0;
}


/*
 * NAME:        cusfs_create_free_block_table
 *
 * FUNCTION:    Create the block free table map for this disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_create_free_block_table(cflsh_usfs_t *cufs, int flags)
{
    uint64_t remaining_size;
    cflash_offset_t offset;
    uint64_t num_blocks_write;
    uint64_t byte_offset_data_start;
    uint64_t bit_offset_in_byte_data_start;
    uint64_t current_byte_offset;
    uint64_t end_meta_data_offset;
    uint64_t num_used_free_blocks = 0;
    cflsh_usfs_free_block_stats_t *block_stat;
    char *bptr;
    int buf_size;
    uint64_t i;
    int rc = 0;



    CUSFS_LOCK(cufs->free_table_lock);


    /*
     * Determine which fs block corresponds to the first data block
     */

    

    buf_size = cufs->buf_size;

    remaining_size = cufs->superblock.free_block_table_size * cufs->disk_block_size;

    offset = cufs->superblock.free_block_table_start_lba;
    
    if (buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    buf_size,cufs->disk_block_size);
    }

    num_blocks_write = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);


    /*
     * Since the free block table includes the meta data blocks at the
     * beginning of the disk, determine the offset in the free block
     * table where the "real" data blocks start.
     */

    
    byte_offset_data_start = cufs->superblock.fs_start_data_block/CUSFS_BITS_PER_BYTE;

    bit_offset_in_byte_data_start = cufs->superblock.fs_start_data_block % CUSFS_BITS_PER_BYTE;

    current_byte_offset = 0;

    while (remaining_size > 0) {

	
	bzero(cufs->buf,cufs->buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_write = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_write++;
	    }

	    buf_size = remaining_size;
	}


	
	if ((current_byte_offset <= byte_offset_data_start) ||
	    (((byte_offset_data_start + 1) == current_byte_offset) && 
	     (bit_offset_in_byte_data_start))) {

	    
	    /*
	     * Since the free block table also includes
	     * the filesystem meta data blocks at the 
	     * beginning of the disk we need to mark 
	     * those as in use.
	     */


	    end_meta_data_offset = MIN((buf_size + current_byte_offset),byte_offset_data_start);


	    for (bptr = cufs->buf; bptr < (char *)(cufs->buf + end_meta_data_offset); bptr++) {

		*bptr = 0xff;
		num_used_free_blocks += 8;


	    }

	    if (end_meta_data_offset < (buf_size + current_byte_offset)) {

		for (i=0;i< bit_offset_in_byte_data_start; i++) {

		    *bptr |= (0x01 << ((CUSFS_BITS_PER_BYTE - 1) - i));
		    num_used_free_blocks++;
		}
	    }


	}


	
	rc = cblk_write(cufs->chunk_id,cufs->buf,offset,num_blocks_write,0);

	if (rc < num_blocks_write) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"write of free block table at lba = 0x%llx failed with errno = %d",
				offset,errno);
	    CUSFS_UNLOCK(cufs->free_table_lock);
	    return -1;
	}

	remaining_size -= buf_size;

	current_byte_offset += buf_size;

			
	offset += num_blocks_write;


    } /* while */


    /* 
     * Create free block table statistics area
     */

    bzero (cufs->buf,cufs->disk_block_size);

    block_stat = (cflsh_usfs_free_block_stats_t *) cufs->buf;

    block_stat->start_marker = CLFSH_USFS_FB_SM;

    block_stat->end_marker = CLFSH_USFS_FB_EM;

    block_stat->total_free_blocks = cufs->superblock.free_block_table_size * cufs->disk_block_size * 8;

    block_stat->available_free_blocks = block_stat->total_free_blocks - num_used_free_blocks;
    
    num_blocks_write = 1;

    offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;
    rc = cblk_write(cufs->chunk_id,cufs->buf,offset,num_blocks_write,0);
    
    if (rc < num_blocks_write) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }


    cufs->flags |= CFLSH_USFS_FLG_FT_VAL;

    /* should we update superblock with this status ?? */

    CUSFS_TRACE_LOG_FILE(5,"Free block table successfully written starting at 0%llx to disk %s",
			cufs->superblock.free_block_table_start_lba,cufs->device_name);


    CUSFS_UNLOCK(cufs->free_table_lock);

    return 0;
}

/*
 * NAME:        cusfs_create_inode_table
 *
 * FUNCTION:    Create the inode table for this disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_create_inode_table(cflsh_usfs_t *cufs, int flags)
{
    int rc = 0;
    uint64_t remaining_size;
    cflash_offset_t offset;
    uint64_t num_blocks_write;
    uint64_t inode_index = 0;
    uint64_t total_num_inodes = 0;
    int buf_size;
    int i,j;
    int num_inodes_per_disk_block;
    cflsh_usfs_inode_t *inode;
    cflsh_usfs_inode_table_stats_t *inode_stat;

    CUSFS_LOCK(cufs->inode_table_lock);



    buf_size = cufs->buf_size;

    offset = cufs->superblock.inode_table_start_lba;

    remaining_size = cufs->num_inode_data_blocks * cufs->disk_block_size;

    /*
     * Ignore end of disk block if it can not fit an inode entry
     */

    num_inodes_per_disk_block = cufs->disk_block_size/sizeof(cflsh_usfs_inode_t);
    
    if (buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    cufs->buf_size,cufs->disk_block_size);
    }

    num_blocks_write = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);

    while (remaining_size > 0) {


	bzero(cufs->buf,cufs->buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_write = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_write++;
	    }

	    buf_size = remaining_size;
	}
	
	inode = cufs->buf;

	for (j=0; j < num_blocks_write; j++) {

	    inode = cufs->buf + (j * cufs->disk_block_size);

	    for (i=0; i < num_inodes_per_disk_block; i++) {

		inode[i].start_marker = CLFSH_USFS_INODE_SM;

		inode[i].index = inode_index++;

		inode[i].end_marker = CLFSH_USFS_INODE_EM;

		total_num_inodes++;
	    
	    }
	}
	

	rc = cblk_write(cufs->index,cufs->buf,offset,num_blocks_write,0);

	if (rc < num_blocks_write) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
				offset,errno);
	    

	    CUSFS_UNLOCK(cufs->inode_table_lock);
	    return -1;
	}

	remaining_size -= buf_size;

			
	offset += num_blocks_write;




    } /* while */


    /* 
     * Create inode table statistics area
     */

    bzero (cufs->buf,cufs->disk_block_size);

    inode_stat = (cflsh_usfs_inode_table_stats_t *) cufs->buf;

    inode_stat->start_marker = CLFSH_USFS_IT_SM;

    inode_stat->end_marker = CLFSH_USFS_IT_EM;

    inode_stat->total_inodes = total_num_inodes;

    inode_stat->available_inodes = total_num_inodes;
    
    num_blocks_write = 1;

    offset = CFLSH_USFS_INODE_TBL_STATS_OFFSET;
    rc = cblk_write(cufs->chunk_id,cufs->buf,offset,num_blocks_write,0);
    
    if (rc < num_blocks_write) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of inode table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	

	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }


    cufs->flags |= CFLSH_USFS_FLG_IT_VAL;

    /* should we update superblock with this status ?? */

    CUSFS_TRACE_LOG_FILE(5,"Inode table successfully written starting at 0%llx to disk %s",
			cufs->superblock.inode_table_start_lba,cufs->device_name);


    CUSFS_UNLOCK(cufs->inode_table_lock);

    return 0;
}

/*
 * NAME:        cusfs_create_journal
 *
 * FUNCTION:    Create the filesystem journal for this disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_create_journal(cflsh_usfs_t *cufs, int flags)
{
    int rc = 0;
    uint64_t remaining_size;
    cflash_offset_t offset;
    uint64_t num_blocks_write;
    int buf_size;
    int need_header = TRUE;
    cflsh_usfs_journal_hdr_t *journal_hdr;


    CUSFS_LOCK(cufs->journal_lock);

    buf_size = cufs->buf_size;

    offset = cufs->superblock.journal_start_lba;

    remaining_size = cufs->superblock.journal_table_size * cufs->disk_block_size;

    if (cufs->buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    buf_size,cufs->disk_block_size);
    }

    num_blocks_write = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);

    while (remaining_size > 0) {


	bzero(cufs->buf,cufs->buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_write = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_write++;
	    }

	    buf_size = remaining_size;
	}


	if (need_header) {

	    journal_hdr = cufs->buf;
	    need_header = FALSE;

	    journal_hdr->start_marker = CLFSH_USFS_JOURNAL_SM;

	    /*
	     * To compute nuber of entries, we first need to get the number of bytes needed
	     * after the header. Since the header does contain the first entry we need to adjust this.
	     */
	     
	    journal_hdr->num_entries = (remaining_size - sizeof(*journal_hdr) + sizeof(cflsh_usfs_directory_entry_t))/sizeof(cflsh_usfs_directory_entry_t);
	    cufs->superblock.num_journal_entries = journal_hdr->num_entries;

	}


	rc = cblk_write(cufs->index,cufs->buf,offset,num_blocks_write,0);

	if (rc < num_blocks_write) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
				offset,errno);
	    
	    CUSFS_UNLOCK(cufs->journal_lock);

	    return -1;
	}

	remaining_size -= buf_size;

			
	offset += num_blocks_write;




    } /* while */



    cufs->flags |= CFLSH_USFS_FLG_IT_VAL;

    /* should we update superblock with this status ?? */


    CUSFS_TRACE_LOG_FILE(5,"Journal successfully written starting at 0%llx to disk %s",
			cufs->superblock.journal_start_lba,cufs->device_name);


    CUSFS_UNLOCK(cufs->journal_lock);

    return 0;
}



/*
 * NAME:        cusfs_add_journal_event
 *
 * FUNCTION:    Add a journal event to be written 
 *              to the journal for this filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_add_journal_event(cflsh_usfs_t *cufs, 
			   cflsh_journal_entry_type_t journal_type,
			   uint64_t block_no, uint64_t inode_num,
			   uint64_t old_file_size, uint64_t new_file_size,
			   uint64_t directory_inode_num,
			   int flags)
{
    int rc = 0;
    cflsh_usfs_journal_ev_t *journal;

    journal = malloc(sizeof(*journal));

    if (journal == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc journal entry with errno = %d for disk %s",
			    errno,cufs->device_name);
	return -1;
    }

    bzero(journal,sizeof(*journal));

    journal->journal_entry.start_marker = CLFSH_USFS_JOURNAL_E_SM;

    journal->journal_entry.type = journal_type;
    
    switch (journal_type) {

    case CFLSH_USFS_JOURNAL_E_ADD_BLK:
    case CFLSH_USFS_JOURNAL_E_FREE_BLK:
	journal->journal_entry.blk.block_no = block_no;
	break;
    case CFLSH_USFS_JOURNAL_E_GET_INODE:
    case CFLSH_USFS_JOURNAL_E_FREE_INODE:
	journal->journal_entry.inode.inode_num = inode_num;
	break;
    case CFLSH_USFS_JOURNAL_E_ADD_INODE_DBLK:
    case CFLSH_USFS_JOURNAL_E_FREE_INODE_DBLK:
	journal->journal_entry.inode_dblk.inode_num = inode_num;
	journal->journal_entry.inode_dblk.block_num = block_no;
	break;
    case CFLSH_USFS_JOURNAL_E_INODE_FSIZE:
	journal->journal_entry.inode_fsize.old_file_size = old_file_size;
	journal->journal_entry.inode_fsize.new_file_size = new_file_size;
	break;
    case CFLSH_USFS_JOURNAL_E_FREE_DIR_E:
	journal->journal_entry.dir.directory_inode_num = directory_inode_num;
	journal->journal_entry.dir.file_inode_num = inode_num;
	break;
    default:
	CUSFS_TRACE_LOG_FILE(1,"Unknown journal type = %d for disk %s",
			    journal_type,cufs->device_name);
	free(journal);
	return -1;

    }



    CUSFS_LOCK(cufs->lock);
    CUSFS_Q_NODE_TAIL(cufs->add_journal_ev_head,cufs->add_journal_ev_tail,journal,
		     prev,next);   
    
    CUSFS_UNLOCK(cufs->lock);

    CUSFS_TRACE_LOG_FILE(5,"Journal event of type %d add for disk %s",
			journal_type, cufs->device_name);


    return rc;
}


/*
 * NAME:        cusfs_flush_journal_events
 *
 * FUNCTION:    Add a journal event to be written 
 *              to the journal for this filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */
int cusfs_flush_journal_events(cflsh_usfs_t *cufs, int flags)
{
    int rc = 0;

    if (cufs->add_journal_ev_head == NULL) {

	/*
	 * No events to flush
	 */
	return 0;
    }

    return rc;
}

/*
 * NAME:        cusfs_get_free_table_stats
 *
 * FUNCTION:    Get get free table statistics
 *
 *
 * INPUTS:
 *
 *
 * RETURNS:     0 Success, Otherwise failure
 *              
 *              
 */

int cusfs_get_free_table_stats(cflsh_usfs_t *cufs,uint64_t *total_blocks, uint64_t *available_blocks,
			 int flags)
{
    cflash_offset_t offset;
    uint64_t num_blocks_read;
    uint64_t rc = 0;
    cflsh_usfs_free_block_stats_t *block_stat;


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (total_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (available_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }


    CUSFS_LOCK(cufs->free_table_lock);


    /*
     * Get inode table statistics
     */

    offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    block_stat = (cflsh_usfs_free_block_stats_t *) cufs->inode_tbl_buf;


    if ((block_stat->start_marker != CLFSH_USFS_FB_SM) ||
	(block_stat->end_marker != CLFSH_USFS_FB_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted free block table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    offset,block_stat->start_marker,block_stat->end_marker);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    *total_blocks = block_stat->total_free_blocks;
    *available_blocks = block_stat->available_free_blocks;



    CUSFS_UNLOCK(cufs->free_table_lock);
    return 0;
}

/*
 * NAME:        cusfs_get_data_blocks
 *
 * FUNCTION:    Get free contiguous data blocks and mark as allocated. 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 failure, lba (in filesystem blocks) if successful.
 *              
 *              
 */

uint64_t cusfs_get_data_blocks(cflsh_usfs_t *cufs, int *num_blocks,int flags)
{
    uint64_t lba = 0;
    uint64_t remaining_size;
    cflash_offset_t offset;
    cflsh_usfs_free_block_stats_t *block_stat;
    uint64_t num_blocks_read;
    uint64_t current_byte_offset;
    int buf_size;
    uint64_t byte_offset;
    int i,j;
    char *byte;
    uint64_t rc;
    int num_blocks_found = 0;
    int leave_loop = FALSE;

    if ((num_blocks == NULL) ||
	(*num_blocks == 0)) {

	return 0;
    }



    if (cufs == NULL) {


	errno = EINVAL;
	return 0;
    }

	
    CUSFS_LOCK(cufs->free_table_lock);

    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }


    /*
     * Read free block table from disk until we find a free bit (LBA).
     */

    

    buf_size = cufs->free_blk_tbl_buf_size;

    remaining_size = cufs->superblock.free_block_table_size * cufs->disk_block_size;

    offset = cufs->superblock.free_block_table_start_lba;
    
    if (buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    buf_size,cufs->disk_block_size);
    }

    num_blocks_read = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);


    current_byte_offset = 0;

    while (remaining_size > 0) {

	
	bzero(cufs->free_blk_tbl_buf,cufs->buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_read = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_read++;
	    }

	    buf_size = remaining_size;
	}


	
	rc = cblk_read(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,num_blocks_read,0);

	if (rc < num_blocks_read) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"reaad of free block table at lba = 0x%llx failed with errno = %d",
				offset,errno);
    
	    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

		CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				     cufs->device_name,errno);
	    }

	    CUSFS_UNLOCK(cufs->free_table_lock);
	    return 0;
	}
	
	
	byte = cufs->free_blk_tbl_buf;
	
	for (i = 0; i < buf_size; i++) {
	    
	    if (((*byte) & 0xff) != 0xff) {
		

		if (!num_blocks_found) {
		
		    byte_offset = current_byte_offset + (byte - (char *)cufs->free_blk_tbl_buf);
		    
		    
		    /*
		     * Determine LBA offset to the first bit of this
		     * byte.
		     */
		    
		    
		    lba = byte_offset * CUSFS_BITS_PER_BYTE;
		    
		    CUSFS_TRACE_LOG_FILE(9,"Found byte that has freeblock at = 0x%llx and lba >= 0x%llx",
					 byte_offset,lba);

		}
		
		for (j=0;j< CUSFS_BITS_PER_BYTE; j++) {

		    if (!( (*byte) & (0x01 << ((CUSFS_BITS_PER_BYTE - 1) - j)))) {


			/*
			 * Mark block now as in use and write
			 * it back.
			 */

			*byte |=  0x01 << ((CUSFS_BITS_PER_BYTE - 1) - j);

			num_blocks_found++;
			if (*num_blocks == num_blocks_found) {
			    leave_loop = TRUE;
			    break;
			}
		    } else if (num_blocks_found) {
			
			/*
			 * We are only returning contiguous blocks.
			 * So if we encounter a non-free block, then
			 * give up.
			 */
			leave_loop = TRUE;
			
			break;
		    }

		    if (!num_blocks_found) {
			lba++;
		    }
		} /* inner for */

		if (leave_loop) {

		    break;
		}
	    } else if (num_blocks_found) {
		
		/*
		 * We are only returning contiguous blocks.
		 * So if we encounter a non-free block, then
		 * give up.
		 */

		leave_loop = TRUE;
		break;
	    }
	    
	    byte++;
	    
	}  /* outer for loop */

	
	if (lba) {
	    
	    rc = cblk_write(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,
			    num_blocks_read,0);
	    
	    if (rc < num_blocks_read) {
		
		
		CUSFS_TRACE_LOG_FILE(1,"update write of free block table at lba = 0x%llx failed with errno = %d",
				     offset,errno);
		
		if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {
		    
		    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
					 cufs->device_name,errno);
		}
		
		CUSFS_UNLOCK(cufs->free_table_lock);
		return 0;
	    }
	    
	    if ((*num_blocks == num_blocks_found) ||
		(leave_loop)) {
		break;
	    }

	}
	
	
	remaining_size -= buf_size;

	current_byte_offset += buf_size;

			
	offset += num_blocks_read;

	
    } /* while */
    
    CUSFS_TRACE_LOG_FILE(5,"Found data block = 0x%llx and num_blocks = 0x%x on device = %s",
			lba,num_blocks_found,cufs->device_name);


    *num_blocks = num_blocks_found;
    /*
     * Update free block table statistics
     */

    offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    rc = cblk_read(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }

    block_stat = (cflsh_usfs_free_block_stats_t *) cufs->free_blk_tbl_buf;


    if ((block_stat->start_marker != CLFSH_USFS_FB_SM) ||
	(block_stat->end_marker != CLFSH_USFS_FB_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted free block table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    offset,block_stat->start_marker,block_stat->end_marker);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;

    }
 
    block_stat->available_free_blocks -= *num_blocks;

    rc = cblk_write(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }



    
    /*
     * ?? Add validation check that this lba is not the meta data
     *    at the beginning of the disk.
     */

    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    

    CUSFS_UNLOCK(cufs->free_table_lock);

    return lba;
}


/*
 * NAME:        cusfs_get_data_block_list
 *
 * FUNCTION:    Get a list of data blocks and mark as allocated. 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_get_data_blocks_list(cflsh_usfs_t *cufs, uint64_t *num_blocks, uint64_t **block_list, int flags)
{

    uint64_t lba = 0;
    uint64_t remaining_size;
    cflash_offset_t offset;
    cflsh_usfs_free_block_stats_t *block_stat;
    uint64_t num_blocks_read;
    uint64_t current_byte_offset;
    int buf_size;
    uint64_t byte_offset;
    int i,j;
    int update_write_needed;
    char *byte;
    void *blk_stat_buf;
    uint64_t rc;
    uint64_t num_blocks_found = 0;
    int leave_loop = FALSE;
    uint64_t *local_block_list;

    if ((num_blocks == NULL) ||
	(*num_blocks == 0)) {

	return -1;
    }



    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }


    blk_stat_buf = malloc(cufs->disk_block_size);

    if (blk_stat_buf == NULL) {

	return -1;
    }
	
    CUSFS_TRACE_LOG_FILE(9,"Looking for 0x%llx num_blocks on device = %s",
			*num_blocks,cufs->device_name);

    CUSFS_LOCK(cufs->free_table_lock);


    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }


    /*
     * Get free block table statistics
     */

    offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    rc = cblk_read(cufs->chunk_id,blk_stat_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }

    block_stat = (cflsh_usfs_free_block_stats_t *) blk_stat_buf;


    if ((block_stat->start_marker != CLFSH_USFS_FB_SM) ||
	(block_stat->end_marker != CLFSH_USFS_FB_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted free block table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    offset,block_stat->start_marker,block_stat->end_marker);

	free(blk_stat_buf);
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {
		    
	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}
		
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;

    }
 

    if (*num_blocks > block_stat->available_free_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"Not enough free blocks available, available free blocks = 0x%llx number requested  0x%llx",
			     block_stat->available_free_blocks,*num_blocks);

	free(blk_stat_buf);
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {
		    
	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}
		
	CUSFS_UNLOCK(cufs->free_table_lock);

	errno = ENOMEM;
	return -1;

    }

    local_block_list = malloc((sizeof(uint64_t) * (*num_blocks)));


    if (local_block_list == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc block_list of size 0x%llx",
			     (sizeof(uint64_t) * (*num_blocks)));

	free(blk_stat_buf);
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {
		    
	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}
		
	CUSFS_UNLOCK(cufs->free_table_lock);

	errno = ENOMEM;
	return -1;
    }

    bzero(local_block_list,(sizeof(uint64_t) * (*num_blocks)));

    /*
     * Read free block table from disk until we find a free bit (LBA).
     */

    

    buf_size = cufs->free_blk_tbl_buf_size;

    remaining_size = cufs->superblock.free_block_table_size * cufs->disk_block_size;

    offset = cufs->superblock.free_block_table_start_lba;
    
    if (buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    buf_size,cufs->disk_block_size);
    }

    num_blocks_read = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);


    current_byte_offset = 0;

    while (remaining_size > 0) {

	
	bzero(cufs->free_blk_tbl_buf,cufs->buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_read = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_read++;
	    }

	    buf_size = remaining_size;
	}

	update_write_needed = FALSE;
	
	rc = cblk_read(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,num_blocks_read,0);

	if (rc < num_blocks_read) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"read of free block table at lba = 0x%llx failed with errno = %d",
				offset,errno);
    
	    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

		CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				     cufs->device_name,errno);
	    }

	    free(blk_stat_buf);
	    free(local_block_list);
	    CUSFS_UNLOCK(cufs->free_table_lock);
	    return -1;
	}
	
	
	byte = cufs->free_blk_tbl_buf;
	
	for (i = 0; i < buf_size; i++) {
	    
	    if (((*byte) & 0xff) != 0xff) {
		


		
		byte_offset = current_byte_offset + (byte - (char *)cufs->free_blk_tbl_buf);
		    
		    
		/*
		 * Determine LBA offset to the first bit of this
		 * byte.
		 */
		    
		    
		lba = byte_offset * CUSFS_BITS_PER_BYTE;
		    
		CUSFS_TRACE_LOG_FILE(9,"Found byte that has freeblock at = 0x%llx and lba >= 0x%llx",
				     byte_offset,lba);


		for (j=0;j< CUSFS_BITS_PER_BYTE; j++) {

		    if (!( (*byte) & (0x01 << ((CUSFS_BITS_PER_BYTE - 1) - j)))) {


			/*
			 * Mark block now as in use and write
			 * it back.
			 */

			*byte |=  0x01 << ((CUSFS_BITS_PER_BYTE - 1) - j);

			local_block_list[num_blocks_found++] = lba;

			CUSFS_TRACE_LOG_FILE(9,"Found lba 0x%llx",
					     lba);

			update_write_needed = TRUE;
			if (*num_blocks == num_blocks_found) {
			    leave_loop = TRUE;
			    break;
			}
		    } 

		    lba++;

		} /* inner for */

		if (leave_loop) {

		    break;
		}
	    } 

	    byte++;
	    
	}  /* outer for loop */

	
	if (update_write_needed) {
	    
	    rc = cblk_write(cufs->chunk_id,cufs->free_blk_tbl_buf,offset,
			    num_blocks_read,0);
	    
	    if (rc < num_blocks_read) {
		
		
		CUSFS_TRACE_LOG_FILE(1,"update write of free block table at lba = 0x%llx failed with errno = %d",
				     offset,errno);
		
		if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {
		    
		    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
					 cufs->device_name,errno);
		}
		free(blk_stat_buf);
		free(local_block_list);
		CUSFS_UNLOCK(cufs->free_table_lock);
		return -1;
	    }
	    
	    if ((*num_blocks == num_blocks_found) ||
		(leave_loop)) {
		break;
	    }

	}
	
	
	remaining_size -= buf_size;

	current_byte_offset += buf_size;

			
	offset += num_blocks_read;

	
    } /* while */
    
    CUSFS_TRACE_LOG_FILE(5,"Found data block = 0x%llx and num_blocks = 0x%llx on device = %s",
			lba,num_blocks_found,cufs->device_name);



    *num_blocks = num_blocks_found;
    /*
     * Update free block table statistics
     */

    offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    block_stat->available_free_blocks -= *num_blocks;

    rc = cblk_write(cufs->chunk_id,blk_stat_buf,offset,num_blocks_read,0);

    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	
	free(local_block_list);
	rc = -1;
    } else {

	rc = 0;
    }

    *block_list = local_block_list;

    free(blk_stat_buf);

    
    /*
     * ?? Add validation check that this lba is not the meta data
     *    at the beginning of the disk.
     */

    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    

    CUSFS_UNLOCK(cufs->free_table_lock);

    return rc;
}


/*
 * NAME:        cusfs_release_data_blocks
 *
 * FUNCTION:    Release contiguous data blocks and mark as unallocated. 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error
 *              
 *              
 */

int cusfs_release_data_blocks(cflsh_usfs_t *cufs, uint64_t lba,int num_blocks,int flags)
{
    int rc = 0;
    uint64_t lba_offset;
    uint64_t byte_offset;
    uint64_t byte_offset_in_block;
    uint64_t bit_offset;
    char *byte;
    cflsh_usfs_free_block_stats_t *block_stat;
    

    if (num_blocks == 0) {

	CUSFS_TRACE_LOG_FILE(1,"Num blocks = 0 for free_block = 0x%llx,for  disk %s",
			    lba,cufs->device_name);

	errno = EINVAL;
	return -1;
    }


    if (num_blocks > 1) {

	/*
	 * currently we don't support multiple blocks
	 */

	errno = EINVAL;
	return -1;

    }


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    /*
     * Determine bit offset of the this LBA
     * in the free block table.
     */

    byte_offset = lba/CUSFS_BITS_PER_BYTE;

    bit_offset = lba % CUSFS_BITS_PER_BYTE; 

    lba_offset = byte_offset/cufs->disk_block_size;

    if (byte_offset % cufs->disk_block_size) {

	lba_offset++;

	byte_offset_in_block = byte_offset % cufs->disk_block_size;
    } else {
	byte_offset_in_block = 0;
    }
	
    CUSFS_LOCK(cufs->free_table_lock);

    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }

    /*
     * Read disk block containing this LBA byte/bit
     */

    rc = cblk_read(cufs->chunk_id,cufs->free_blk_tbl_buf,lba_offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"read of free block table at lba = 0x%llx failed with errno = %d",
			     lba_offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }

    byte = cufs->free_blk_tbl_buf + byte_offset_in_block;

    *byte &= ~(0x01 << ((CUSFS_BITS_PER_BYTE - 1) - bit_offset));
	
    /*
     * Write updated disk block containing this LBA byte/bit now indicating
     * it is free.
     */

    rc = cblk_write(cufs->chunk_id,cufs->free_blk_tbl_buf,lba_offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table at lba = 0x%llx failed with errno = %d",
			    lba_offset,errno);

	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_FREE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }
	

    
    CUSFS_TRACE_LOG_FILE(9,"lba = 0x%llx marked free ",lba);

    /*
     * Update free block table statistics
     */

    lba_offset = CFLSH_USFS_FBLK_TBL_STATS_OFFSET;

    rc = cblk_read(cufs->chunk_id,cufs->free_blk_tbl_buf,lba_offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    lba_offset,errno);
	// cflsh_usfs_master_unregister(??,??,??,??);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }

    block_stat = (cflsh_usfs_free_block_stats_t *) cufs->free_blk_tbl_buf;


    if ((block_stat->start_marker != CLFSH_USFS_FB_SM) ||
	(block_stat->end_marker != CLFSH_USFS_FB_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted free block table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    lba_offset,block_stat->start_marker,block_stat->end_marker);
	// cflsh_usfs_master_unregister(??,??,??,??);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;

    }
 
    block_stat->available_free_blocks += num_blocks;

    rc = cblk_write(cufs->chunk_id,cufs->free_blk_tbl_buf,lba_offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    lba_offset,errno);
	
	// cflsh_usfs_master_unregister(??,??,??,??);
	CUSFS_UNLOCK(cufs->free_table_lock);
	return -1;
    }


    // cflsh_usfs_master_unregister(??,??,??,??);
    CUSFS_UNLOCK(cufs->free_table_lock);

    return rc;
}


/*
 * NAME:        cusfs_add_directory_entry
 *
 * FUNCTION:    Add a directory entry for the specified
 *              inode
 *
 * NOTES:       Assume caller has directory data object's lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
int cusfs_add_directory_entry(cflsh_usfs_t *cufs,uint64_t directory_inode_num,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags)
{
    int rc = 0;
    char *buf;
    char *buf_new_fs_block;
    int buf_size;
    int old_directory_entry_offset;
    cflsh_usfs_inode_t *parent_inode;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;
    int added_entry;
    cflsh_usfs_data_obj_t *directory;

    if (strlen(filename) >= CFLSH_USFS_DIR_MAX_NAME) {

	CUSFS_TRACE_LOG_FILE(1,"filename = %s too long %d bytes",
			    filename,strlen(filename));

	errno = EINVAL;
	return -1;

    }

    directory = cusfs_find_data_obj_from_inode(cufs,directory_inode_num,0);

    if (directory == NULL) {

	    
	parent_inode = cusfs_get_inode_from_index(cufs,directory_inode_num,0);

	if (parent_inode == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Failed to get parent inode 0x%llx for file = %s of type %d on disk %s",
				directory_inode_num,filename,inode->type, cufs->device_name);


	    return -1;
	}


	buf_size = parent_inode->file_size;

	buf = malloc(buf_size);

	if (buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc data buffer for parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
				directory_inode_num,filename,inode->type, cufs->device_name, errno);



	    return -1;
	}


    } else {


	parent_inode = directory->inode;

	buf = directory->data_buf;

	buf_size = directory->data_buf_size;
    }



    if (cusfs_read_data_object(cufs,parent_inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    directory_inode_num,filename,inode->type, cufs->device_name, errno);

	if (directory == NULL) {
	    free(buf);
	}

	return -1;

    }

    /*
     * Add file entry to directory.
     *
     * NOTE: Since directory entries can span
     *      a filesystem block, we need to 
     *      ensure that a potential directory entry is
     *      is contained in the allocated blocks for this
     *      filesystem.  We chose to allow directory entries
     *      to span blocks, since the filename string can
     *      be very large in some OS (AIX allows 1024). We
     *      could have chosen to truncate that to 256, but
     *      decided to allow this option instead.
     */

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_entry = directory_hdr->entry;

    added_entry = FALSE;

    while (directory_entry  < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	if (!(directory_entry->flags & CFLSH_USFS_DIR_VAL)) {

	    directory_entry->flags |= CFLSH_USFS_DIR_VAL;
	    directory_entry->inode_number = inode->index;
	    directory_entry->type = inode->type;
	    strcpy(directory_entry->filename,filename);
	    added_entry = TRUE;
	    break;
	}

	directory_entry++;
    }


    if (!added_entry) {

	/*
	 * Need to add 1 data block to this directory.
	 * Then we reallocate the data buffer and
	 * add the directory where we left off in our
	 * search above.
	 */

	CUSFS_TRACE_LOG_FILE(5,"Could not find unused directory entry for = %s of type %d on disk %s, errno = %d",
			    filename,inode->type, cufs->device_name, errno);



	if (cusfs_add_fs_blocks_to_inode_ptrs(cufs,parent_inode,1,0)) {


	    return -1;
	}


	CUSFS_TRACE_LOG_FILE(9,"Added block for  directory entry for = %s of type %d on disk %s, errno = %d",
			    filename,inode->type, cufs->device_name, errno);


	/*
	 * Save offset of the directory entry 
	 * where we left off above. Since realloc
	 * can change the location of the memory referenced
	 * by buf, we will need this offset to return
	 * to where we left off.
	 */

	old_directory_entry_offset = (char *)directory_entry - buf;


	/*
	 * Update inode file size to reflect this
	 * this addition. Below we will update
	 * inode on the disk block.
	 */

	parent_inode->file_size += cufs->fs_block_size;

	buf_size = parent_inode->file_size;

	buf = realloc(buf,buf_size);

	if (buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to realloc after directory expansion parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    directory_inode_num,filename,inode->type, cufs->device_name, errno);
	    if (directory == NULL) {
		free(buf);
	    }

	    return -1;

	}


	/*
	 * Initialize newly allocated filesystem
	 * data block.
	 */

	buf_new_fs_block = buf - cufs->fs_block_size;

	bzero(buf_new_fs_block,cufs->fs_block_size);

	directory_entry = (cflsh_usfs_directory_entry_t *)(buf + old_directory_entry_offset);


	/*
	 * Add new directory entry where we left
	 * off.
	 */

	directory_entry->flags |= CFLSH_USFS_DIR_VAL;
	directory_entry->inode_number = inode->index;
	directory_entry->type = inode->type;
	strcpy(directory_entry->filename,filename);


    }

    if (cusfs_write_data_object(cufs,parent_inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to write parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    directory_inode_num,filename,inode->type, cufs->device_name, errno);


	if (directory == NULL) {
	    free(buf);
	}

	return -1;

    }

    /*
     * Update directory modification
     * time and maybe file_size if changed above.
     */

#if !defined(__64BIT__) && defined(_AIX)
    parent_inode->mtime = cflsh_usfs_time64(NULL);
#else
    parent_inode->mtime = time(NULL);
#endif /* not 32-bit AIX */

    cusfs_update_inode(cufs,parent_inode,0);

    if (directory == NULL) {
	free(buf);
    }

    CUSFS_TRACE_LOG_FILE(9,"filename = %s of type %d on disk %s, rc = %d",
			filename,inode->type, cufs->device_name, rc);

    return rc;
}



/*
 * NAME:        cusfs_find_inode_num_from_directory_by_name
 *
 * FUNCTION:    Find inode number for the filename
 *              in the specified directory.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL - error, otherwise success
 *              
 *              
 */
cflsh_usfs_inode_t *cusfs_find_inode_num_from_directory_by_name(cflsh_usfs_t *cufs,
						    char *name,char *buf, 
						    int buf_size,int flags)
{
    cflsh_usfs_inode_t *inode = NULL;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;


    if (strlen(name) >= CFLSH_USFS_DIR_MAX_NAME) {

	CUSFS_TRACE_LOG_FILE(1,"filename = %s too long %d bytes",
			    name,strlen(name));

	errno = EINVAL;
	return NULL;

    }

    /*
     * Remove file entry to directory
     */

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_entry = directory_hdr->entry;

    while (directory_entry < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	if ((directory_entry->flags & CFLSH_USFS_DIR_VAL) &&
	    !(strcmp(directory_entry->filename,name)))  {

	    CUSFS_TRACE_LOG_FILE(9,"Found directory entry to be requested with filename = %s of type 0x%x",
				directory_entry->filename,directory_entry->type);

	    inode = cusfs_get_inode_from_index(cufs,directory_entry->inode_number,0);


	    break;
	}

	directory_entry++;
    }

    return inode;
}

/*
 * NAME:        cusfs_remove_directory_entry
 *
 * FUNCTION:    Remove a directory entry for the specified
 *              inode
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */
int cusfs_remove_directory_entry(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags)
{
    int rc = 0;
    char *buf;
    int buf_size;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;
    int remove_entry;
    cflsh_usfs_data_obj_t *directory;
    cflsh_usfs_inode_t *parent_inode = NULL;


    if (strlen(filename) >= CFLSH_USFS_DIR_MAX_NAME) {

	CUSFS_TRACE_LOG_FILE(1,"filename = %s too long %d bytes",
			    filename,strlen(filename));

	errno = EINVAL;
	return -1;

    }

    directory = cusfs_find_data_obj_from_inode(cufs,inode->directory_inode,0);

    if (directory == NULL) {

	    
	parent_inode = cusfs_get_inode_from_index(cufs,inode->directory_inode,0);

	if (parent_inode == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Failed to get parent inode 0x%llx for file = %s of type %d on disk %s",
				inode->directory_inode,filename,inode->type, cufs->device_name);


	    return -1;
	}


	buf_size = parent_inode->file_size;

	buf = malloc(buf_size);

	if (buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc data buffer for parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
				inode->directory_inode,filename,inode->type, cufs->device_name, errno);



	    return -1;
	}


    } else {

	/*
	 * Only lock the directory structure
	 * if it is in use
	 */

	CUSFS_LOCK(directory->lock);
	parent_inode = directory->inode;

	buf = directory->data_buf;

	buf_size = directory->data_buf_size;
    }



    if (cusfs_read_data_object(cufs,parent_inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    inode->directory_inode,filename,inode->type, cufs->device_name, errno);

	if (directory == NULL) {
	    free(buf);
	} else {
	    CUSFS_UNLOCK(directory->lock);
	}

	return -1;

    }

    /*
     * Remove file entry to directory
     */

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_entry = directory_hdr->entry;

    remove_entry = FALSE;

    while (directory_entry < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	if ((directory_entry->flags & CFLSH_USFS_DIR_VAL) &&
	    (directory_entry->inode_number == inode->index)) {

	    //?? Add more validation of entry

	    CUSFS_TRACE_LOG_FILE(9,"Found directory entry to be removed with filename = %s of type 0x%x",
				directory_entry->filename,directory_entry->type);


	    bzero(directory_entry,sizeof(*directory_entry));
	    remove_entry = TRUE;
	    break;
	}

	directory_entry++;
    }

    if (!remove_entry) {


	CUSFS_TRACE_LOG_FILE(1,"Could not find directory entry to remove for = %s of type %d on disk %s, errno = %d",
			    filename,inode->type, cufs->device_name, errno);


	if (directory == NULL) {
	    free(buf);
	} else {
	    CUSFS_UNLOCK(directory->lock);
	}

	return -1;

    }

    if (cusfs_write_data_object(cufs,parent_inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to write parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    inode->directory_inode,filename,inode->type, cufs->device_name, errno);



	if (directory == NULL) {
	    free(buf);
	} else {
	    CUSFS_UNLOCK(directory->lock);
	}

	return -1;

    }

    /*
     * Update directory modification
     * time.
     */

#if !defined(__64BIT__) && defined(_AIX)
    parent_inode->mtime = cflsh_usfs_time64(NULL);
#else
    parent_inode->mtime = time(NULL);
#endif /* not 32-bit AIX */

    cusfs_update_inode(cufs,parent_inode,0);

    if (directory == NULL) {
	free(buf);
    } else {
	CUSFS_UNLOCK(directory->lock);
    }

    return rc;
}


/*
 * NAME:        cusfs_check_directory_empty
 *
 * FUNCTION:    Remove a directory entry for the specified
 *              inode
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     1 if directory is empty, 0 otherwise.
 *              
 *              
 */
int cusfs_check_directory_empty(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags)
{
    char *buf;
    int buf_size;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;
    int found_entry = 0;
    cflsh_usfs_data_obj_t *directory;


    if (inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {


	CUSFS_TRACE_LOG_FILE(1,"This inode is not a directory %s of type %d on disk %s, errno = %d",
			    filename,inode->type, cufs->device_name, errno);



	return 0;
    }



    directory = cusfs_find_data_obj_from_inode(cufs,inode->index,0);

    if (directory == NULL) {

	    
	buf_size = inode->file_size;

	buf = malloc(buf_size);

	if (buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc data buffer for parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
				inode->index,filename,inode->type, cufs->device_name, errno);



	    return 0;
	}


    } else {



	buf = directory->data_buf;

	buf_size = directory->data_buf_size;
    }



    if (cusfs_read_data_object(cufs,inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    inode->index,filename,inode->type, cufs->device_name, errno);

	if (directory == NULL) {
	    free(buf);
	}

	return 0;

    }

    /*
     * See if any entries exist in this directory
     */

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_entry = directory_hdr->entry;

    found_entry = FALSE;

    while (directory_entry < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	if (directory_entry->flags & CFLSH_USFS_DIR_VAL) {


	    CUSFS_TRACE_LOG_FILE(9,"Found directory entry with filename = %s of type 0x%x",
				directory_entry->filename,directory_entry->type);


	    found_entry = TRUE;
	    break;
	}

	directory_entry++;
    }

    if (directory == NULL) {
	free(buf);
    }

    return !found_entry;
}


/*
 * NAME:        cusfs_list_files_directory
 *
 * FUNCTION:    List all files in a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error
 *              
 *              
 */
int cusfs_list_files_directory(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags)
{
    int rc = 0;
    char *buf;
    int buf_size;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;
    cflsh_usfs_data_obj_t *directory;
    char time_string[TIMELEN+1];
    char timebuf[TIMELEN+1];


    if (inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {


	CUSFS_TRACE_LOG_FILE(1,"This inode is not a directory %s of type %d on disk %s, errno = %d",
			    filename,inode->type, cufs->device_name, errno);


	errno = EINVAL;
	return -1;
    }



    directory = cusfs_find_data_obj_from_inode(cufs,inode->index,0);

    if (directory == NULL) {

	    
	buf_size = inode->file_size;

	buf = malloc(buf_size);

	if (buf == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc data buffer for parent inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
				inode->index,filename,inode->type, cufs->device_name, errno);



	    return -1;
	}


    } else {



	buf = directory->data_buf;

	buf_size = directory->data_buf_size;
    }



    if (cusfs_read_data_object(cufs,inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    inode->index,filename,inode->type, cufs->device_name, errno);

	if (directory == NULL) {
	    free(buf);
	}

	return -1;

    }

    /*
     * See if any entries exist in this directory
     */

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_entry = directory_hdr->entry;

    /*
     * ?? TODO: This printing out of data needs to move to caller
     *          Instead we may need to provide data for ls to work
     */

    printf("mode\tuid\tgid\tmtime                   \tsize\tinode\tfilename\n");

    while (directory_entry < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	if (directory_entry->flags & CFLSH_USFS_DIR_VAL) {

	    inode = cusfs_get_inode_from_index(cufs,directory_entry->inode_number,0);

	    if (inode) {
		
#if !defined(__64BIT__) && defined(_AIX)
		sprintf(time_string,"%s",ctime64_r(&(inode->mtime),timebuf));
#else
		sprintf(time_string,"%s",ctime_r(&(inode->mtime),timebuf));
#endif  
		/*
		 * Remove newline from time_string
		 */

		time_string[strlen(time_string) - 1] = '\0';

		printf("%o\t%d\t%d\t%s\t0x%"PRIX64"\t0x%"PRIX64"\t%s\n",
		       inode->mode_flags, inode->uid, inode->gid,
		       time_string,
		       inode->file_size,inode->index,directory_entry->filename);

		free(inode);
	    } else {
		printf("0x%"PRIX64"  %s\n",directory_entry->inode_number,directory_entry->filename);
	    }
	}

	directory_entry++;
    }

    if (directory == NULL) {
	free(buf);
    }

    return rc;
}



/*
 * NAME:        cusfs_get_next_directory_entry
 *
 * FUNCTION:    Get the next directory entry after file->offset.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error
 *              
 *              
 */
int cusfs_get_next_directory_entry(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *directory, 
			    cflsh_usfs_directory_entry_t *return_dir_entry, cflash_offset_t *offset,
			    int flags)
{
    int rc = 0;
    char *buf;
    int buf_size;
    int found_entry = FALSE;
    cflash_offset_t next_rel_offset;
    cflsh_usfs_directory_t *directory_hdr;
    cflsh_usfs_directory_entry_t *directory_entry;


    if (directory == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"No directory specified for device %s",
			    cufs->device_name);


	errno = EINVAL;
	return -1;
    }
    
    CUSFS_TRACE_LOG_FILE(9,"directory %s with offset 0x%llx on disk %s",
			directory->filename,offset,cufs->device_name);


    if (directory->inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {


	CUSFS_TRACE_LOG_FILE(1,"This inode is not a directory %s of type %d on disk %s",
			    directory->filename,directory->inode->type, cufs->device_name);


	errno = EINVAL;
	return -1;
    }


    buf_size = directory->inode->file_size;

    if (*offset > directory->inode->file_size) {

	CUSFS_TRACE_LOG_FILE(1,"This offset 0x%llx is beyond the end of this directory %s on this disk",
			    *offset,directory->filename,cufs->device_name);

	errno = EINVAL;
	return -1;

    }

    buf = malloc(buf_size);

    if (buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc data buffer for directory = %s on disk %s, errno = %d",
			    directory->filename,cufs->device_name, errno);

	return -1;
    }


    CUSFS_LOCK(directory->lock);

    if (cusfs_read_data_object(cufs,directory->inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read inode data 0x%llx for file = %s of type %d on disk %s, errno = %d",
			    directory->inode->index,directory->filename,directory->inode->type, 
			    cufs->device_name);

	free(buf);
	CUSFS_UNLOCK(directory->lock);
	return -1;

    }

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    if (directory_hdr->start_marker != CLFSH_USFS_DIRECTORY_SM) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid directory header, start_marker =  0x%llx for file = %s of type %d on disk %s, errno = %d",
			    directory_hdr->start_marker,directory->inode->index,directory->filename,directory->inode->type, 
			    cufs->device_name);

	free(buf);
	CUSFS_UNLOCK(directory->lock);
	return -1;
    }

    directory_entry = directory_hdr->entry;

    CUSFS_TRACE_LOG_FILE(7,"directory_entry %p, buf %p, buf_size = 0x%x, sizeof directory_entry = 0x%x",
			directory_entry,buf,buf_size,sizeof(*directory_entry));

    next_rel_offset = (cflash_offset_t)((char *)directory_entry - buf);

    while (directory_entry < (cflsh_usfs_directory_entry_t *)((buf + buf_size) - sizeof(*directory_entry))) {
	    
	CUSFS_TRACE_LOG_FILE(9,"directory entry loop = %s with inode 0x%llx in directory %s at offset 0x%llx on disk %s",
			    directory_entry->filename,directory_entry->inode_number, directory->filename,*offset,
			    cufs->device_name);


	/*
	 * Advance offset to next
	 * directory entry
	 */
	next_rel_offset += sizeof(*directory_entry);


	if (directory_entry >= (cflsh_usfs_directory_entry_t *)(buf + *offset)) {

	    if (directory_entry->flags & CFLSH_USFS_DIR_VAL) {

		*return_dir_entry = *directory_entry;

		found_entry = TRUE;

		CUSFS_TRACE_LOG_FILE(9,"directory entry found = %s with inode 0x%llx in directory %s on disk %s",
				    directory_entry->filename,directory_entry->inode_number, directory->filename,
				    cufs->device_name);
		/*
		 * Advance offset to next
		 * directory entry
		 */
		*offset = next_rel_offset;
		break;
	    }
	}


	directory_entry++;
    }
    
    free(buf);

#if !defined(__64BIT__) && defined(_AIX)
    directory->inode->atime = cflsh_usfs_time64(NULL);
#else
    directory->inode->atime = time(NULL);
#endif /* not 32-bit AIX */
    cusfs_update_inode(cufs,directory->inode,0);
    CUSFS_UNLOCK(directory->lock);

    if (!found_entry) {

	CUSFS_TRACE_LOG_FILE(5,"No entries found in directory %s with offset 0x%llx on disk %s",
			    directory->filename,offset,cufs->device_name);
	rc = -1;
    } 

    return rc;
}





/*
 * NAME:        cusfs_clear_data_obj
 *
 * FUNCTION:    Clear data object will all zeroes for
 *              the associated inode
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 succesa, otherwise failure
 *              
 *              
 */
int cusfs_clear_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode,int flags)
{
    int rc = 0;
    char *buf;
    int buf_size;


    buf_size = inode->file_size;

    buf = malloc(buf_size);

    if (buf == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc inode data buffer for 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);

	return -1;
    }



    if (cusfs_read_data_object(cufs,inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read inode data 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);

	free(buf);

	return -1;

    }

    /*
     * ?? TODO: This code may need to iterate a block at a time
     *          to avoid large buffers in the future.
     */

    bzero(buf,buf_size);

    if (cusfs_write_data_object(cufs,inode,buf,&buf_size,0)) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to write inode data 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);



	rc = -1;

    }

    free(buf);

    return rc;
}

/*
 * NAME:        cusfs_initialize_directory
 *
 * FUNCTION:    Initialize a directory
 *              
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 succesa, otherwise failure
 *              
 *              
 */
int cusfs_initialize_directory(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode,int flags)
{
    int rc = 0;
    char *buf;
    int buf_size;
    cflsh_usfs_directory_t *directory_hdr;


    if (inode->type != CFLSH_USFS_INODE_FILE_DIRECTORY) {

	return -1;
    }

    buf_size = inode->file_size;

    buf = malloc(buf_size);

    if (buf == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc inode data buffer for 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);

	return -1;
    }



    if (cusfs_read_data_object(cufs,inode,buf,&buf_size,0)) {



	CUSFS_TRACE_LOG_FILE(1,"Failed to read inode data 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);

	free(buf);

	return -1;

    }

    /*
     * ?? TODO: This code may need to iterate a block at a time
     *          to avoid large buffers in the future.
     */

    bzero(buf,buf_size);

    directory_hdr = (cflsh_usfs_directory_t *)buf;

    directory_hdr->start_marker = CLFSH_USFS_DIRECTORY_SM;

    if (cusfs_write_data_object(cufs,inode,buf,&buf_size,0)) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to write inode data 0x%llx of type %d on disk %s, errno = %d",
			    inode->index,inode->type, cufs->device_name, errno);



	rc = -1;

    }

    free(buf);

    return rc;
}


/*
 * NAME:        cusfs_create_data_obj
 *
 * FUNCTION:    Creates a filesystem data object of 
 *              of inode_type (file, directory, symlink etc).
 *
 * NOTES:       Assumes caller has parent directory's lock.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, Otherwise success
 *              
 *              
 */

cflsh_usfs_data_obj_t *cusfs_create_data_obj(cflsh_usfs_t *cufs, 
					   char *filename, 
					   cflsh_usfs_inode_file_type_t inode_type,   
					   uint64_t parent_directory_inode_num,
					   mode_t mode_flags, uid_t uid, gid_t gid,
					   char *sym_link_name,
					   int flags)
{
    int num_blocks = 1;
    uint64_t block_num  = 0;
    cflsh_usfs_data_obj_t *file;
    cflsh_usfs_inode_t *inode;
    uint64_t file_size;
    int alloc_flags = 0;



    if (cufs == NULL) {


	errno = EINVAL;
	return NULL;
    }


    CUSFS_TRACE_LOG_FILE(9,"file = %s of type %d on disk %s",
			filename,inode_type, cufs->device_name);


    if (inode_type == CFLSH_USFS_INODE_FILE_REGULAR) {
	alloc_flags = CFLSH_USFS_ALLOC_READ_AHEAD;
    }

    file = cusfs_alloc_file(cufs,filename,alloc_flags);

    if (file == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"malloc of file %s of type %d for failed for disk %s",
			    filename,inode_type, cufs->device_name);
	return NULL;
    }

    if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {

	/*
	 * If this is a symlink then we store the filename in
	 * union sharing the same space as the inode_ptrs
	 */

	block_num = cusfs_get_data_blocks(cufs,&num_blocks,0);

	if (num_blocks == 0) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for file = %s of type %d on disk %s",
				filename,inode_type, cufs->device_name);
	    
	    cusfs_free_file(file);
	    return NULL;
	}

    }

    /*
     * For directory inodes always provide a file
     * size of the filesystem block size.
     * For other inodes, set the file size to 0
     * even though cusfs_get_inode will allocate
     * one filesystem block size.
     */
    if (inode_type == CFLSH_USFS_INODE_FILE_DIRECTORY) {

	file_size = cufs->fs_block_size;

    } else {

	file_size = 0;

    }

    inode = cusfs_get_inode(cufs,parent_directory_inode_num,inode_type,
			   mode_flags,uid,gid,&block_num,num_blocks,file_size,
			   sym_link_name,
			   0);

    if (inode == NULL) {

	if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {
	    cusfs_release_data_blocks(cufs,block_num,num_blocks,0);
	}


	CUSFS_TRACE_LOG_FILE(1,"Failed to get inode for file = %s of type %d on disk %s",
			    filename,inode_type, cufs->device_name);
	cusfs_free_file(file);

	return NULL;
    }

    file->inode = inode;

    CUSFS_TRACE_LOG_FILE(9,"inode->index = 0x%llx on disk %s",
			inode->index, cufs->device_name);

    if (flags & CFLSH_USFS_ROOT_DIR) {

	file->flags |= CFLSH_USFS_ROOT_DIR;

	inode->flags |= CFLSH_USFS_INODE_ROOT_DIR;

	cusfs_update_inode(cufs,inode,0);
	
    } else {

	CUSFS_TRACE_LOG_FILE(9,"Adding inode->index = 0x%llx with filename %s",
			    inode->index, filename);
	if (cusfs_add_directory_entry(cufs,parent_directory_inode_num,inode,filename,0)) {


		CUSFS_TRACE_LOG_FILE(1,"Failed to add directory entry to parent inode 0x%llx for file = %s of type %d on disk %s",
				    parent_directory_inode_num,filename,inode_type, cufs->device_name);


		if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {
		    cusfs_release_inode(cufs,inode,0);
		}

		cusfs_release_data_blocks(cufs,block_num,num_blocks,0);
		cusfs_free_file(file);


		return NULL;
	}

    }

    if (inode_type == CFLSH_USFS_INODE_FILE_DIRECTORY) {

	/*
	 * For new directory, we need to bzero its data blocks
	 */

	if (cusfs_initialize_directory(cufs,inode,0)) {

	    if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {
		cusfs_release_inode(cufs,inode,0);
	    }

	    cusfs_release_data_blocks(cufs,block_num,num_blocks,0);
	    cusfs_free_file(file);


	    return NULL;
	}
    }

    file->data_buf_size = cufs->fs_block_size;

    file->data_buf = malloc(file->data_buf_size);


    if (file->data_buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to to malloc file->data_bu of size 0x%x for file = %s of type %d on disk %s",
			    file->data_buf_size,filename,inode_type, cufs->device_name);


	if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {
	    cusfs_release_inode(cufs,inode,0);
	}

	cusfs_release_data_blocks(cufs,block_num,num_blocks,0);
	cusfs_free_file(file);


	return NULL;

    }

    if (file) {
	    
	CUSFS_LOCK(cufs->lock);
	CUSFS_Q_NODE_TAIL(cufs->filelist_head,cufs->filelist_tail,file,prev,next);
	CUSFS_UNLOCK(cufs->lock);

    }

    CUSFS_TRACE_LOG_FILE(5,"Created file = %s of type %d on disk %s",
			filename,inode_type, cufs->device_name);


    return file;
}




/*
 * NAME:        cusfs_free_data_obj
 *
 * FUNCTION:    Free a filesystem data object 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_free_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *file,int flags)
{
    int rc = 0;

    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (file == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null file object passed for disk %s",
			    cufs->device_name);
	return -1;
    }

    if (file->inode == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null file->inode for file %s object passed for disk %s",
			    file->filename,cufs->device_name);
	return -1;
    }

    CUSFS_LOCK(file->lock);

    if (file->inode->type == CFLSH_USFS_INODE_FILE_DIRECTORY) {


	if (file->inode->flags & CFLSH_USFS_INODE_ROOT_DIR) {


	    CUSFS_TRACE_LOG_FILE(1,"Can not remove root directory file %s object passed for disk %s",
			    file->filename,cufs->device_name);
	    CUSFS_UNLOCK(file->lock);
	    return -1;
	}



	if ((cusfs_check_directory_empty(cufs,file->inode,file->filename,0)) == 0) {

	    

	    CUSFS_TRACE_LOG_FILE(1,"directory file %s object is not empty for disk %s",
			    file->filename,cufs->device_name);
	    CUSFS_UNLOCK(file->lock);
	    return -1;

	}

	/* ??TODO2: Need a force flag to allow this directory to be deleted
	   and delete the hierarchy underneath.
	*/

    }


    // Remove from parent directory

    if (cusfs_remove_directory_entry(cufs,file->inode,file->filename,0)) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to remove file %s of type %s from directory 0x%llx on disk %s",
			    file->filename,file->inode->type, file->inode->directory_inode,cufs->device_name);

	CUSFS_UNLOCK(file->lock);
	return -1;
    }


    cusfs_release_inode_and_data_blocks(cufs,file->inode,0);

    CUSFS_UNLOCK(file->lock);

    CUSFS_LOCK(cufs->lock);

    CUSFS_DQ_NODE(cufs->filelist_head,cufs->filelist_tail,file,prev,next);

    CUSFS_UNLOCK(cufs->lock);

    
    cusfs_free_file(file);


    return rc;
}


/*
 * NAME:        cusfs_move_data_obj
 *
 * FUNCTION:    Move a data object in a filesystem
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_mv_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *file,char *new_full_pathname,int flags)
{
    int rc = 0;
    char full_path[PATH_MAX];
    char *new_filename;
    cflsh_usfs_data_obj_t *new_parent_dir;
    
	

    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (file == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null file object passed for disk %s",
			    cufs->device_name);
	return -1;
    }

    if (file->inode == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null file->inode for file %s object passed for disk %s",
			    file->filename,cufs->device_name);
	return -1;
    }

    if (new_full_pathname == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null new_full_pathname is null for file %s passed for disk %s",
			    file->filename,cufs->device_name);
	return -1;
    }



    strcpy(full_path,new_full_pathname);

    /*
     * Get parent directory of new path
     */

    new_filename = strrchr(full_path,'/');

    new_filename[0] = '\0';

    new_filename++;

    
    new_parent_dir = cusfs_lookup_data_obj(cufs,full_path,0);

    if (new_parent_dir == NULL) {

	
	CUSFS_TRACE_LOG_FILE(1,"Failed to find new directory %s for disk %s",
			    new_parent_dir,file->filename,cufs->device_name);
	return -1;
    }


    /*
     * First add file to new location. Then remove it from
     * old location.
     */

    CUSFS_LOCK(new_parent_dir->lock);
    if (cusfs_add_directory_entry(cufs,new_parent_dir->inode->index,file->inode,new_filename,0)) {

	
	CUSFS_TRACE_LOG_FILE(1,"Failed to aadd file %s to new directory  for disk %s",
			    file->filename,cufs->device_name);
	CUSFS_UNLOCK(new_parent_dir->lock);
	return -1;
    }

    CUSFS_UNLOCK(new_parent_dir->lock);


    if (cusfs_remove_directory_entry(cufs,file->inode,file->filename,0)) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to remove file %s to new directory  for disk %s",
			    file->filename,cufs->device_name);
	return -1;
    }

    strcpy(file->filename,new_filename);


    return rc;
}

/*
 * NAME:        cusfs_find_root_inode
 *
 * FUNCTION:    This code code finds and returns the inode
 *              of the root directory of this filesystem.
 *
 *
 * INPUTS:
 *              
 *
 * NOTES:       The root directory inode should be the first 
 *              inode in the inode table. However this code
 *              allows it to be anywhere in this disk block
 *              of inodes.
 *
 *
 * RETURNS:     NULL failure, Otherwise success
 *              
 *              
 */

cflsh_usfs_inode_t *cusfs_find_root_inode(cflsh_usfs_t *cufs, int flags)
{
    cflsh_usfs_inode_t *inode = NULL;
    cflsh_usfs_inode_t *inode_ptr = NULL;
    int buf_size;
    int rc = 0;

    if (cufs == NULL) {


	errno = EINVAL;
	return inode;
    }


    CUSFS_LOCK(cufs->inode_table_lock);

    /*
     * Only read the first disk block of the inode table for the root directoy.
     */

    buf_size = cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,cufs->superblock.inode_table_start_lba,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"reaad of inode table at lba = 0x%llx failed with errno = %d",
			    cufs->superblock.inode_table_start_lba,errno);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return inode;

    }

	
    inode_ptr = cufs->inode_tbl_buf;

    while (inode_ptr < (cflsh_usfs_inode_t *)(cufs->inode_tbl_buf + buf_size)) {
	    
	if ((inode_ptr->flags & CFLSH_USFS_INODE_INUSE) &&
	    (inode_ptr->flags & CFLSH_USFS_INODE_ROOT_DIR)) {
		
		
	    if ((inode_ptr->start_marker != CLFSH_USFS_INODE_SM) ||
		(inode_ptr->end_marker != CLFSH_USFS_INODE_EM)) {

		CUSFS_TRACE_LOG_FILE(1,"Invalid inode markers inode_ptr = %p offset = 0x%llx",
				    inode_ptr, (cufs->inode_tbl_buf + buf_size));

		CUSFS_UNLOCK(cufs->inode_table_lock);
		return inode;
	    } 


	    inode = (cflsh_usfs_inode_t *) malloc(sizeof(cflsh_usfs_inode_t));

	    if (inode == NULL) {

		CUSFS_TRACE_LOG_FILE(1,"malloc of inode failed with errno = %d",
				    errno);

		CUSFS_UNLOCK(cufs->inode_table_lock);
		return inode;

	    }



	    *inode = *inode_ptr;

		break;
	}
	inode_ptr++;
    }


    /*
     * ?? Add validation check that this lba is not the meta data
     *    at the beginning of the disk.
     */

    CUSFS_TRACE_LOG_FILE(5,"root inode found = 0x%p inode->index = 0x%llx on device = %s",
			inode,inode->index,cufs->device_name);



    CUSFS_UNLOCK(cufs->inode_table_lock);
    return inode;
}
