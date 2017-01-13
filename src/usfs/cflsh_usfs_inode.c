/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_block.c 1.8           */
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

#define CFLSH_USFS_FILENUM 0x0400

#include "cflsh_usfs_internal.h"
#include "cflsh_usfs_protos.h"


/*
 * NAME:        cusfs_get_inode_stats
 *
 * FUNCTION:    Get inode table statistics
 *
 *
 * INPUTS:
 *
 *
 * RETURNS:     0 Success, Otherwise failure
 *              
 *              
 */

int cusfs_get_inode_stats(cflsh_usfs_t *cufs,uint64_t *total_inodes, uint64_t *available_inodes,
			 int flags)
{
    cflash_offset_t offset;
    uint64_t num_blocks_read;
    uint64_t rc = 0;
    cflsh_usfs_inode_table_stats_t *inode_stat;


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (total_inodes == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (available_inodes == NULL) {

	errno = EINVAL;
	return -1;
    }


    CUSFS_LOCK(cufs->inode_table_lock);


    /*
     * Get inode table statistics
     */

    offset = CFLSH_USFS_INODE_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    inode_stat = (cflsh_usfs_inode_table_stats_t *) cufs->inode_tbl_buf;


    if ((inode_stat->start_marker != CLFSH_USFS_IT_SM) ||
	(inode_stat->end_marker != CLFSH_USFS_IT_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted inode table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    offset,inode_stat->start_marker,inode_stat->end_marker);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    *total_inodes = inode_stat->total_inodes;
    *available_inodes = inode_stat->available_inodes;



    CUSFS_UNLOCK(cufs->inode_table_lock);
    return 0;
}


/*
 * NAME:        cusfs_get_inode
 *
 * FUNCTION:    Get free inode and mark as allocated.
 *
 *
 * INPUTS:
 *              
 *
 * NOTES:       This code assumes the following about the blocks
 *              array:
 *
 *                0-11:     Are direct inode pointers
 *                12:       Is a single indirect inode pointer
 *                13:       Is a double indirect inode pointer
 *                14:       Is a triple indirect inode pointer.
 *
 *
 * RETURNS:     NULL failure, Otherwise success
 *              
 *              
 */

cflsh_usfs_inode_t *cusfs_get_inode(cflsh_usfs_t *cufs, uint64_t parent_directory_inode_num,
				  cflsh_usfs_inode_file_type_t inode_type,
				  mode_t mode_flags, uid_t uid, gid_t gid, 
				  uint64_t blocks[], int num_blocks, uint64_t file_size,
				  char *sym_link_name,
				  int flags)
{
    cflsh_usfs_inode_t *inode = NULL;
    int buf_size;
    char *buf;
    int i;
    uint64_t remaining_size;
    cflash_offset_t offset;
    uint64_t num_blocks_read;
    uint64_t rc = 0;
    cflsh_usfs_inode_table_stats_t *inode_stat;
    uint64_t current_byte_offset;
    cflsh_usfs_inode_t *inode_ptr;
    uint64_t buf_offset;


    if (cufs == NULL) {


	errno = EINVAL;
	return inode;
    }

    if (blocks == NULL) {

	errno = EINVAL;
	return inode;
    }


    CUSFS_LOCK(cufs->inode_table_lock);

    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }



    buf_size = cufs->inode_tbl_buf_size;

    remaining_size = cufs->superblock.inode_table_size * cufs->disk_block_size;

    offset = cufs->superblock.inode_table_start_lba;
    
    if (buf_size % cufs->disk_block_size) {

	CUSFS_TRACE_LOG_FILE(1,"buf_size = 0x%x, is not a multiple of device block size",
			    buf_size,cufs->disk_block_size);
    }

    num_blocks_read = MIN(buf_size/cufs->disk_block_size,cufs->max_xfer_size);


    current_byte_offset = 0;

    while (remaining_size > 0) {

	
	bzero(cufs->inode_tbl_buf,cufs->inode_tbl_buf_size);

	if (remaining_size < buf_size) {


	    num_blocks_read = remaining_size/cufs->disk_block_size;


	    if (remaining_size % cufs->disk_block_size) {

		num_blocks_read++;
	    }

	    buf_size = remaining_size;
	}


	
	rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,num_blocks_read,0);

	if (rc < num_blocks_read) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"reaad of inode table at lba = 0x%llx failed with errno = %d",
				offset,errno);
	    
	    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

		CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				     cufs->device_name,errno);
	    }

	    CUSFS_UNLOCK(cufs->inode_table_lock);
	    return inode;
	}
	
	inode_ptr = cufs->inode_tbl_buf;

	while (inode_ptr < (cflsh_usfs_inode_t *)(cufs->inode_tbl_buf + buf_size)) {
	    
	    if (!(inode_ptr->flags & CFLSH_USFS_INODE_INUSE)) {
		
		
		if ((inode_ptr->start_marker != CLFSH_USFS_INODE_SM) ||
		    (inode_ptr->end_marker != CLFSH_USFS_INODE_EM)) {

		    CUSFS_TRACE_LOG_FILE(1,"Invalid inode markers inode_ptr = %p offset = 0x%llx",
					inode_ptr, offset);
		    continue;
		} 


		inode = (cflsh_usfs_inode_t *) malloc(sizeof(cflsh_usfs_inode_t));

		if (inode == NULL) {

		    CUSFS_TRACE_LOG_FILE(1,"malloc of inode failed with errno = %d",
					 errno);

	    
		    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

			CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
					     cufs->device_name,errno);
		    }

		    CUSFS_UNLOCK(cufs->inode_table_lock);
		    return inode;

		}


		inode_ptr->uid = uid;
		inode_ptr->gid = gid;


		if (inode_type == CFLSH_USFS_INODE_FILE_DIRECTORY) {

		    mode_flags |= S_IFDIR;

		} else if (inode_type == CFLSH_USFS_INODE_FILE_REGULAR) {

		    mode_flags |= S_IFREG;

		} else if (inode_type == CFLSH_USFS_INODE_FILE_SYMLINK) {

		    mode_flags |= S_IFLNK;

		} 

		inode_ptr->mode_flags = mode_flags;
		inode_ptr->file_size = file_size;
		inode_ptr->type = inode_type;
		inode_ptr->num_links = 1;
#if !defined(__64BIT__) && defined(_AIX)
		inode_ptr->atime = cflsh_usfs_time64(NULL);
#else
		inode_ptr->atime = time(NULL);
#endif /* not 32-bit AIX */
		inode_ptr->ctime = inode_ptr->atime;
		inode_ptr->mtime = inode_ptr->atime;


		inode_ptr->directory_inode = parent_directory_inode_num;

		if (inode_type != CFLSH_USFS_INODE_FILE_SYMLINK) {

		    num_blocks = MIN(num_blocks,CFLSH_USFS_NUM_TOTAL_INODE_PTRS);

		    for (i=0; i< num_blocks; i++) {

			inode_ptr->ptrs[i] = blocks[i];
		    }

		} else {

		    if (sym_link_name == NULL) {


			CUSFS_TRACE_LOG_FILE(1,"symlink is null");

			free(inode);
			if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

			    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
						 cufs->device_name,errno);
			}

			CUSFS_UNLOCK(cufs->inode_table_lock);
			return NULL;
		    }


		    strcpy(inode_ptr->sym_link_path,sym_link_name);
		}

		inode_ptr->flags =  CFLSH_USFS_INODE_INUSE;

		*inode = *inode_ptr;


		/* 
		 * Write back this inode, but only write this specific LBA
		 * data
		 */

		if (cusfs_get_inode_block_no_from_inode_index(cufs,inode->index,(uint64_t *)&offset)) {

		    CUSFS_TRACE_LOG_FILE(1,"failed to get lba for inode->index = 0x%llx",
					inode->index);

		    free(inode);
		    CUSFS_UNLOCK(cufs->inode_table_lock);

		    return NULL;;

		}
		
		/*
		 * Get pointer to beginning of this disk blocks data buffer
		 * with the updated inode
		 */
		
		buf_offset = (uint64_t)inode_ptr;

		buf_offset &= ~((uint64_t)cufs->disk_block_size - 1);

		buf = (char*)buf_offset;
	
		/*
		 * Write updated disk block containing this inode back to disk
		 */

		rc = cblk_write(cufs->chunk_id,buf,offset,1,0);
    
		if (rc < 1) {
	
		    CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
					offset,errno);

		    free(inode);
		    free(inode);
		    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

			CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
					     cufs->device_name,errno);
		    }
		    CUSFS_UNLOCK(cufs->inode_table_lock);

		    return NULL;
		}
	
		    
		*inode = *inode_ptr;


		break;
	    }
	    inode_ptr++;
	}

	if (inode) {

	    break;
	}
	
	remaining_size -= buf_size;

	current_byte_offset += buf_size;

			
	offset += num_blocks_read;

	
    } /* while */
    
    
    /*
     * ?? Add validation check that this lba is not the meta data
     *    at the beginning of the disk.
     */

    CUSFS_TRACE_LOG_FILE(5,"inode found = 0x%p on device = %s",
			inode,cufs->device_name);


    /*
     * Update inode table statistics
     */

    offset = CFLSH_USFS_INODE_TBL_STATS_OFFSET;

    num_blocks_read = 1;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return NULL;
    }

    inode_stat = (cflsh_usfs_inode_table_stats_t *) cufs->inode_tbl_buf;


    if ((inode_stat->start_marker != CLFSH_USFS_IT_SM) ||
	(inode_stat->end_marker != CLFSH_USFS_IT_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted inode table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			    offset,inode_stat->start_marker,inode_stat->end_marker);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return NULL;

    }
 
    inode_stat->available_inodes--;

    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,num_blocks_read,0);
    
    if (rc < num_blocks_read) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return NULL;
    }



    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    
    
    CUSFS_UNLOCK(cufs->inode_table_lock);
    return inode;
}


/*
 * NAME:        cusfs_release_inode
 *
 * FUNCTION:    Release an inode for use by others
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error.
 *              
 *              
 */

int cusfs_release_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags)
{
    int rc = 0;
    cflash_offset_t offset;
    cflsh_usfs_inode_t *inode_ptr;
    cflsh_usfs_inode_table_stats_t *inode_stat;
    int inode_index_in_block;
    int num_inodes_per_block;
    uint64_t index;


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (inode == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Node valid inode passed for  disk %s",
			    cufs->device_name);
	errno = EINVAL;
	return -1;
    }

    if (cusfs_get_inode_block_no_from_inode_index(cufs,inode->index,(uint64_t *)&offset)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get lba for inode->index = 0x%llx",
			    inode->index);
	return -1;

    }

    CUSFS_LOCK(cufs->inode_table_lock);

    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    
	
    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"read of inode table at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }
	

    /*
     * Get offset in sector for this inode
     */

    num_inodes_per_block = cufs->disk_block_size / sizeof(cflsh_usfs_inode_t);

    inode_index_in_block =  inode->index % num_inodes_per_block;

    inode_ptr = cufs->inode_tbl_buf;

    if (inode_ptr[inode_index_in_block].index != inode->index) {

	CUSFS_TRACE_LOG_FILE(1,"inode on disk different from ours inode_ptr->index = 0x%llx, inode->index = 0x%llx",
			     inode_ptr->index,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    if (inode_ptr[inode_index_in_block].type != inode->type) {

	CUSFS_TRACE_LOG_FILE(1,"inode types do not match inode_ptr->type = %d, inode->type = %d, inode->index = 0x%llx",
			     inode_ptr->type,inode->type,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    if (inode_ptr[inode_index_in_block].num_links > 1) {

	inode_ptr[inode_index_in_block].num_links--;

	
    } else {

	index = inode_ptr->index;

	bzero(&inode_ptr[inode_index_in_block],sizeof(*inode_ptr));

	inode_ptr[inode_index_in_block].start_marker = CLFSH_USFS_INODE_SM;

	inode_ptr[inode_index_in_block].index = index;

	inode_ptr[inode_index_in_block].end_marker = CLFSH_USFS_INODE_EM;
	
    }

    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }


    free(inode);

    CUSFS_TRACE_LOG_FILE(5,"inode freed = 0x%p on device = %s",
			inode,cufs->device_name);



    /*
     * Update inode table statistics
     */

    offset = CFLSH_USFS_INODE_TBL_STATS_OFFSET;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    inode_stat = (cflsh_usfs_inode_table_stats_t *) cufs->inode_tbl_buf;


    if ((inode_stat->start_marker != CLFSH_USFS_IT_SM) ||
	(inode_stat->end_marker != CLFSH_USFS_IT_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted inode table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			     offset,inode_stat->start_marker,inode_stat->end_marker);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    inode_stat->available_inodes++;

    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }


    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }

    CUSFS_UNLOCK(cufs->inode_table_lock);
    return rc;
}


/*
 * NAME:        cusfs_release_inode_and_data_blocks
 *
 * FUNCTION:    Release an inode for use by others.
 *              Also free all associated data blocks for this inode.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error.
 *              
 *              
 */

int cusfs_release_inode_and_data_blocks(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags)
{
    int rc = 0;
    cflash_offset_t offset;
    cflsh_usfs_inode_table_stats_t *inode_stat;
    cflsh_usfs_inode_t *inode_ptr;
    int inode_index_in_block;
    int num_inodes_per_block;
    uint64_t index;
    int i;


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (inode == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Node valid inode passed for  disk %s",
			    cufs->device_name);
	errno = EINVAL;
	return -1;
    }

    if (cusfs_get_inode_block_no_from_inode_index(cufs,inode->index,(uint64_t *)&offset)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get lba for inode->index = 0x%llx",
			    inode->index);
	return -1;

    }

    CUSFS_LOCK(cufs->inode_table_lock);

    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }


	
    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"read of inode table at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }
	

    /*
     * Get offset in sector for this inode
     */

    num_inodes_per_block = cufs->disk_block_size / sizeof(cflsh_usfs_inode_t);

    inode_index_in_block = inode->index % num_inodes_per_block;

    inode_ptr = cufs->inode_tbl_buf;

    if (inode_ptr[inode_index_in_block].index != inode->index) {

	CUSFS_TRACE_LOG_FILE(1,"inode on disk different from ours inode_ptr->index = 0x%llx, inode->index = 0x%llx",
			     inode_ptr->index,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    if (inode_ptr[inode_index_in_block].type != inode->type) {

	CUSFS_TRACE_LOG_FILE(1,"inode types do not match inode_ptr->type = %d, inode->type = %d, inode->index = 0x%llx",
			     inode_ptr->type,inode->type,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    if (inode_ptr[inode_index_in_block].num_links > 1) {

	inode_ptr[inode_index_in_block].num_links--;
	
    } else {

	index = inode_ptr[inode_index_in_block].index;

	if (inode_ptr[inode_index_in_block].type != CFLSH_USFS_INODE_FILE_SYMLINK) {

	    for (i=0; i< CFLSH_USFS_NUM_DIRECT_INODE_PTRS; i++) {

		if (inode_ptr[inode_index_in_block].ptrs[i]) {

		    cusfs_release_data_blocks(cufs,inode_ptr[inode_index_in_block].ptrs[i],1,0);
		}
	    }

	    if (inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_SINGLE_INDIRECT]) {

		cusfs_remove_indirect_inode_ptr(cufs,inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_SINGLE_INDIRECT],0);
	    }
	    if (inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_DOUBLE_INDIRECT]) {

		cusfs_remove_indirect_inode_ptr(cufs,inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_DOUBLE_INDIRECT],
					       CFLSH_USFS_INODE_FLG_INDIRECT_ENTRIES);
	    }
	    if (inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_TRIPLE_INDIRECT]) {

		cusfs_remove_indirect_inode_ptr(cufs,inode_ptr[inode_index_in_block].ptrs[CFLSH_USFS_INDEX_TRIPLE_INDIRECT],
					       CFLSH_USFS_INODE_FLG_D_INDIRECT_ENTRIES);
	    }


	}

	bzero(&inode_ptr[inode_index_in_block],sizeof(*inode_ptr));

	inode_ptr[inode_index_in_block].start_marker = CLFSH_USFS_INODE_SM;

	inode_ptr[inode_index_in_block].index = index;

	inode_ptr[inode_index_in_block].end_marker = CLFSH_USFS_INODE_EM;
	
    }

    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    } else {

	rc = 0;
    }


    free(inode);

    CUSFS_TRACE_LOG_FILE(5,"inode freed = 0x%p on device = %s",
			inode,cufs->device_name);



    /*
     * Update inode table statistics
     */

    offset = CFLSH_USFS_INODE_TBL_STATS_OFFSET;

    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    inode_stat = (cflsh_usfs_inode_table_stats_t *) cufs->inode_tbl_buf;


    if ((inode_stat->start_marker != CLFSH_USFS_IT_SM) ||
	(inode_stat->end_marker != CLFSH_USFS_IT_EM)) {

	CUSFS_TRACE_LOG_FILE(1,"corrupted inode table stats at lba = 0x%llx sm =  0x%llx em =  0x%llx",
			     offset,inode_stat->start_marker,inode_stat->end_marker);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }
 
    inode_stat->available_inodes--;

    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);
    
    if (rc < 1) {
	
	CUSFS_TRACE_LOG_FILE(1,"write of free block table stats at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }


    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }


    CUSFS_UNLOCK(cufs->inode_table_lock);
    return rc;
}

/*
 * NAME:        cusfs_get_inode_from_index
 *
 * FUNCTION:    Get the inode structure from disk for
 *              the specified inode index (number).
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     NULL failure, otherwise success
 *              
 *              
 */

cflsh_usfs_inode_t *cusfs_get_inode_from_index(cflsh_usfs_t *cufs, uint64_t inode_num,int flags)
{
    int rc = 0;
    cflash_offset_t offset;
    cflsh_usfs_inode_t *inode_ptr;
    cflsh_usfs_inode_t *inode = NULL;
    int inode_index_in_block;
    int num_inodes_per_block;



    if (cufs == NULL) {


	errno = EINVAL;
	return NULL;
    }


    if (cusfs_get_inode_block_no_from_inode_index(cufs,inode_num,(uint64_t *)&offset)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get lba for inode->index = 0x%llx",
			    inode_num);
	return NULL;

    }

    CUSFS_LOCK(cufs->inode_table_lock);

    
	
    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"read of inode table at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return NULL;
    }
	

    /*
     * Get offset in sector for this inode
     */

    num_inodes_per_block = cufs->disk_block_size / sizeof(cflsh_usfs_inode_t);

    inode_index_in_block = inode_num % num_inodes_per_block;

    inode_ptr = cufs->inode_tbl_buf;

    if (inode_ptr[inode_index_in_block].index != inode_num) {

	CUSFS_TRACE_LOG_FILE(1,"inode index 0x%llx, does not match index 0x%llx requested",
			    inode[inode_index_in_block].index,inode_num);
	CUSFS_UNLOCK(cufs->inode_table_lock);
	return NULL;
    }

    
    inode = (cflsh_usfs_inode_t *) malloc(sizeof(cflsh_usfs_inode_t));

    if (inode == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of inode failed with errno = %d",
			    errno);

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return inode;
    }

    *inode = inode_ptr[inode_index_in_block];


    CUSFS_TRACE_LOG_FILE(5,"inode = 0x%p found on device = %s",
			inode,cufs->device_name);



    CUSFS_UNLOCK(cufs->inode_table_lock);
    return inode;
}

/*
 * NAME:        cusfs_update_inode
 *
 * FUNCTION:    Update a inode
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise error.
 *              
 *              
 */

int cusfs_update_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags)
{
    int rc = 0;
    cflash_offset_t offset;
    cflsh_usfs_inode_t *inode_ptr;
    int inode_index_in_block;
    int num_inodes_per_block;



    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (inode == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Node valid inode passed for  disk %s",
			    cufs->device_name);
	errno = EINVAL;
	return -1;
    }

    if (cusfs_get_inode_block_no_from_inode_index(cufs,inode->index,(uint64_t *)&offset)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get lba for inode->index = 0x%llx",
			    inode->index);
	return -1;

    }

    CUSFS_LOCK(cufs->inode_table_lock);


    if (cflsh_usfs_master_lock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    
	
    rc = cblk_read(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"read of inode table at lba = 0x%llx failed with errno = %d",
			     offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }
	

    /*
     * Get offset in sector for this inode
     */

    num_inodes_per_block = cufs->disk_block_size / sizeof(cflsh_usfs_inode_t);

    inode_index_in_block = inode->index % num_inodes_per_block;

    inode_ptr = cufs->inode_tbl_buf;

    if (inode_ptr[inode_index_in_block].index != inode->index) {

	CUSFS_TRACE_LOG_FILE(1,"inode on disk different from ours inode_ptr->index = 0x%llx, inode->index = 0x%llx",
			    inode_ptr->index,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;

    }

 
    if (inode_ptr[inode_index_in_block].type != inode->type) {

	CUSFS_TRACE_LOG_FILE(1,"inode types do not match inode_ptr->type = %d, inode->type = %d, inode->index = 0x%llx",
			    inode_ptr->type,inode->type,inode->index);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }

    /*
     * Update inode change time
     */

#if !defined(__64BIT__) && defined(_AIX)
    inode->ctime = cflsh_usfs_time64(NULL);
#else
    inode->ctime = time(NULL);
#endif /* not 32-bit AIX */


    inode_ptr[inode_index_in_block] = *inode;
	
    rc = cblk_write(cufs->chunk_id,cufs->inode_tbl_buf,offset,1,0);

    if (rc < 1) {
	    
	CUSFS_TRACE_LOG_FILE(1,"write of inode table at lba = 0x%llx failed with errno = %d",
			    offset,errno);
	
	if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
				 cufs->device_name,errno);
	}

	CUSFS_UNLOCK(cufs->inode_table_lock);
	return -1;
    }




    CUSFS_TRACE_LOG_FILE(5,"updated inode = 0x%p on device = %s",
			inode,cufs->device_name);



    if (cflsh_usfs_master_unlock(cufs->master_handle,CFLSH_USFS_MASTER_INODE_TABLE_LOCK,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to unlock master lock for disk %s with errno = %d",
			     cufs->device_name,errno);
    }
    CUSFS_UNLOCK(cufs->inode_table_lock);
    return 0;
}


/*
 * NAME:        cusfs_get_indirect_inode_ptr
 *
 * FUNCTION:    Get a data block to be set up for indirect
 *              inode pointers.
 *
 *
 * INPUTS:
 *              
 *
 *
 *
 * RETURNS:     0 failure, otherwise success.
 *              
 *              
 */

uint64_t cusfs_get_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				     uint64_t blocks[], int *num_blocks, 
				     int flags)
{
    int rc = 0;
    int i = 0;
    uint64_t block_num;
    uint64_t disk_block_num;
    int requested_num_blocks = 1;
    uint64_t *disk_blocks;
    char *buf;
    int buf_size;
    int num_disk_blocks;


    if (cufs == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (blocks == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (num_blocks == NULL) {


	errno = EINVAL;
	return 0;
    }    

    buf_size = cufs->fs_block_size;

    buf = malloc(buf_size);

    if (buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of buf for indirect inode pointer failed with errno = %d for disk %s",
			    errno, cufs->device_name);

	return 0;
    }


    bzero(buf,buf_size);

    block_num = cusfs_get_data_blocks(cufs,&requested_num_blocks,0);

    if (num_blocks == 0) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to get data block on disk %s, with errno %d",
			    cufs->device_name, errno);
	return 0;
    }
    

    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,block_num,&disk_block_num)) {

	cusfs_release_data_blocks(cufs,block_num,requested_num_blocks,0);
		
	free(buf);
	return 0;
    }

    disk_blocks = (uint64_t *)buf;

    *num_blocks = MIN(*num_blocks,cufs->fs_block_size/sizeof(uint64_t));

    for (i = 0; i < *num_blocks; i++) {

	disk_blocks[i] = blocks[i];
    }
	    
    //?? locking??
	    

	    
    num_disk_blocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_write(cufs->chunk_id,buf,disk_block_num,num_disk_blocks,0);

    if (rc < num_disk_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"write of of inode indirect pointer %d at lba = 0x%llx failed with errno = %d",
			    i,disk_block_num,errno);
	cusfs_release_data_blocks(cufs,block_num,requested_num_blocks,0);	
	
	block_num = 0;
    }

    
    free(buf);


    return block_num;
}


/*
 * NAME:        cusfs_update_indirect_inode_ptr
 *
 * FUNCTION:    Update a data block that is already set up for indirect
 *              inode pointers.
 *
 *
 * INPUTS:
 *              
 *
 *
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_update_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				   uint64_t inode_ptr_block_no,
					uint64_t blocks[], int *num_blocks, 
					int flags)
{
    int rc = 0;
    int i = 0;
    uint64_t block_num = inode_ptr_block_no;
    uint64_t disk_block_num;
    uint64_t *disk_blocks;
    char *buf;
    int buf_size;
    int num_disk_blocks;


    if (cufs == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (blocks == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (num_blocks == NULL) {


	errno = EINVAL;
	return 0;
    }    

    buf_size = cufs->fs_block_size;

    buf = malloc(buf_size);

    if (buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of buf for indirect inode pointer failed with errno = %d for disk %s",
			    errno, cufs->device_name);

	return 0;
    }

    bzero(buf,buf_size);

    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,block_num,&disk_block_num)) {

	free(buf);
	return 0;
    }




    disk_blocks = (uint64_t *) buf;

    *num_blocks = MIN(*num_blocks,cufs->fs_block_size/sizeof(uint64_t));

    for (i = 0; i < *num_blocks; i++) {

	disk_blocks[i] = blocks[i];
    }
	    
    //?? locking??
	    

	    
    num_disk_blocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_write(cufs->chunk_id,buf,disk_block_num,num_disk_blocks,0);

    if (rc < num_disk_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"write of of inode indierct pointer %d at lba = 0x%llx failed with errno = %d",
			    i,disk_block_num,errno);
	
	block_num = 0;
    }

    
    free(buf);


    return rc;
}


/*
 * NAME:        cusfs_add_blocks_to_indirect_inode_ptr
 *
 * FUNCTION:    Update data block that is already set up for indirect
 *              inode pointers to add more data block pointers.
 *
 *
 * INPUTS:
 *              
 *
 *
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_add_blocks_to_indirect_inode_ptr(cflsh_usfs_t *cufs, 
					  uint64_t inode_ptr_block_no,
					uint64_t blocks[], int *num_blocks, 
					int flags)
{
    int rc = 0;
    int i,j;
    uint64_t block_num = inode_ptr_block_no;
    uint64_t disk_block_num;
    uint64_t *disk_blocks;
    char *buf;
    int buf_size;
    int num_disk_blocks;
    int num_inode_ptrs;


    if (cufs == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (blocks == NULL) {


	errno = EINVAL;
	return 0;
    } 

    if (num_blocks == NULL) {


	errno = EINVAL;
	return 0;
    }    

    buf_size = cufs->fs_block_size;

    buf = malloc(buf_size);

    if (buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of buf for indirect inode pointer failed with errno = %d for disk %s",
			    errno, cufs->device_name);

	return 0;
    }

    bzero(buf,buf_size);

    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,block_num,&disk_block_num)) {

	free(buf);
	return 0;
    }


	    
    num_disk_blocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,buf,disk_block_num,num_disk_blocks,0);

    if (rc < num_disk_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"read of inode indierct pointer at lba = 0x%llx failed with errno = %d",
			    disk_block_num,errno);
	block_num = 0;
    }



    disk_blocks = (uint64_t *) buf;

    /*
     * Find first unused data inode_ptrs
     * in this data block.
     */
    
    num_inode_ptrs = cufs->fs_block_size/sizeof(uint64_t);

    for (i = 0; i < num_inode_ptrs;i++) {

	if (disk_blocks[i] == 0x00LL) {

	    break;
	}
    }
    

    *num_blocks = MIN(*num_blocks,(cufs->fs_block_size/sizeof(uint64_t) - i));

    for (j = i; j < *num_blocks; j++) {

	disk_blocks[j] = blocks[j];
    }
	    
	    

    // ?? cusfs_release_data_blocks(cufs,block_num,requested_num_blocks,0);



    rc = cblk_write(cufs->chunk_id,buf,disk_block_num,num_disk_blocks,0);

    if (rc < num_disk_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"write of inode indierct pointer %d at lba = 0x%llx failed with errno = %d",
			    i,disk_block_num,errno);
	
	block_num = 0;
    }

    
    free(buf);


    return rc;
}

/*
 * NAME:        cusfs_remove_indirect_inode_ptr
 *
 * FUNCTION:    Remove a data block that is already set up for indirect
 *              inode pointers. This can also remove all the data blocks
 *              referenced from this indirect inode pointer.
 *
 *
 * INPUTS:
 *              
 *
 *
 *
 * RETURNS:     0 success, otherwise failure.
 *              
 *              
 */

int cusfs_remove_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				   uint64_t inode_ptr_block_no, 
				   int flags)
{
    int rc = 0;
    int i = 0;
    uint64_t block_num = inode_ptr_block_no;
    uint64_t *disk_blocks;
    uint64_t disk_block_num;
    char *buf;
    int buf_size;
    int num_disk_blocks;
    int num_inode_ptrs;


    if (cufs == NULL) {


	errno = EINVAL;
	return 0;
    } 

    buf_size = cufs->fs_block_size;

    buf = malloc(buf_size);

    if (buf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of buf for indirect inode pointer failed with errno = %d for disk %s",
			    errno, cufs->device_name);

	return 0;
    }

    bzero(buf,buf_size);

    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,block_num,&disk_block_num)) {

	free(buf);
	return 0;
    }

	    
    num_disk_blocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,buf,disk_block_num,num_disk_blocks,0);

    if (rc < num_disk_blocks) {

	CUSFS_TRACE_LOG_FILE(1,"read of inode indierct pointer %d at lba = 0x%llx failed with errno = %d",
			    i,disk_block_num,errno);
	
	block_num = 0;
    }


    disk_blocks = (uint64_t *) buf;

    num_inode_ptrs = cufs->fs_block_size/sizeof(uint64_t);


    for (i = 0; i < num_inode_ptrs;i++) {

	if (disk_blocks[i]) {

	    if (flags & CFLSH_USFS_INODE_FLG_INDIRECT_ENTRIES) {
		cusfs_remove_indirect_inode_ptr(cufs, disk_blocks[i],0);
	    } else if (flags & CFLSH_USFS_INODE_FLG_D_INDIRECT_ENTRIES) {
		cusfs_remove_indirect_inode_ptr(cufs, disk_blocks[i],
					       CFLSH_USFS_INODE_FLG_INDIRECT_ENTRIES);
	    } else {
		cusfs_release_data_blocks(cufs,disk_blocks[i],1,0);
	    }
	}
    }
	    
    //?? locking??
	    

    cusfs_release_data_blocks(cufs,inode_ptr_block_no,1,0);
    
    free(buf);


    return 0;
}

/*
 * NAME:        cusfs_get_inode_block_no_from_inode_index
 *
 * FUNCTION:    Read the disk sector (in terms of cufs->disk_block_size)
 *              which contains the inode with the inode_index specfied.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, non-zero otherwise.
 *              
 *              
 */

int cusfs_get_inode_block_no_from_inode_index(cflsh_usfs_t *cufs, uint64_t inode_index,uint64_t *lba)
{
    int rc = 0;


    *lba = inode_index * sizeof(cflsh_usfs_inode_t)/cufs->disk_block_size + cufs->superblock.inode_table_start_lba;

    if ((*lba < cufs->superblock.inode_table_start_lba) ||
	(*lba > (cufs->superblock.inode_table_start_lba + cufs->superblock.inode_table_size)))	 {

	CUSFS_TRACE_LOG_FILE(1,"Invalid inode lba = 0x%llx  inode_start_lba = 0x%llx, inode_end_lba = 0x%llx",
			    *lba,cufs->superblock.inode_table_start_lba,
			    (cufs->superblock.inode_table_start_lba + cufs->superblock.inode_table_size));

	errno = ERANGE;
	return -1;
    }
    return rc;
}


/*
 * NAME:        cusfs_get_inode_ptr_from_file_offset
 *
 * FUNCTION:    Determine which inode pointer in the inode is
 *              associated with the byte offset in the file.
 *
 *
 * INPUTS:
 *              ?? Remove this routine?
 *
 * RETURNS:     -1 failure, otherwise success
 *              
 *              
 */

int cusfs_get_inode_ptr_from_file_offset(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
					uint64_t offset, int *indirect_inode_ptr, int flags)
{
    int inode_ptr_index = -1;
    uint64_t start_single_indirect_inode_offset;

    if (cufs == NULL) {

	errno = EINVAL;
	return inode_ptr_index;

    } 

    if (file == NULL) {

	errno = EINVAL;
	return inode_ptr_index;
    }

    inode_ptr_index = offset/cufs->fs_block_size;


    if (inode_ptr_index >= CFLSH_USFS_NUM_DIRECT_INODE_PTRS) {

	start_single_indirect_inode_offset = CFLSH_USFS_NUM_DIRECT_INODE_PTRS * cufs->fs_block_size;


	*indirect_inode_ptr = (offset - start_single_indirect_inode_offset)/ cufs->fs_block_size;
	
	if (*indirect_inode_ptr > cufs->fs_block_size/(sizeof(uint64_t))) {
	    
	    
	    /*
	     * ?? TODO: This code does not handle double and triple indirection in inode ponters
	     */

	    return -1;
	}

	inode_ptr_index = CFLSH_USFS_INDEX_SINGLE_INDIRECT;

	

						       
    }
    

	
    return inode_ptr_index;
}

/*
 * NAME:        cusfs_rdwr_block_inode_pointers
 *
 * FUNCTION:    Read/Write in/out fs block size block of inode pointers.
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
int cusfs_rdwr_block_inode_pointers(cflsh_usfs_t *cufs, 
				   cflsh_usfs_inode_t *inode,
				   void *buffer,
				   uint64_t fs_lba, int nblocks, int flags)
{
    int rc = 0;
    uint64_t disk_lba;
    

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    fs_lba,inode->type, cufs->device_name);
	
	
	return -1;
    }
    if (flags & CFLSH_USFS_RDWR_WRITE) {
	rc = cblk_write(cufs->chunk_id,buffer,disk_lba,nblocks,0);
    } else {
	rc = cblk_read(cufs->chunk_id,buffer,disk_lba,nblocks,0);
    }
    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"%s at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     ((flags & CFLSH_USFS_RDWR_WRITE) ? "Write":" Read"),
			    disk_lba,inode->type, cufs->device_name);
	
	
	return -1;
    }
    
    return 0;

}


/*
 * NAME:        cusfs_get_disk_lbas_from_inode_ptr_for_transfer
 *
 * FUNCTION:    For the specified transfer, get a list of
 *              disk LBA ranges that are associated.
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_get_disk_lbas_from_inode_ptr_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, 
						   uint64_t offset, size_t num_bytes, 
						   cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
						   int flags)
{
    int rc = 0;
    int i,j;
    uint64_t start_fs_lba;
    int single_index =0;
    int double_index = 0;
    int triple_index = 0;
    uint64_t start_inode_ptr;
    uint64_t start_offset_in_inode_ptr;
    uint64_t end_offset_in_inode_ptr;
    uint64_t num_inode_ptrs;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_single_block_list = NULL;
    uint64_t *inode_double_block_list = NULL;
    uint64_t *inode_triple_block_list = NULL;
    uint64_t start_disk_lba;
    cflsh_usfs_disk_lba_e_t *lba_list;
    uint64_t num_lbas, num_lbas2;
    int nblocks;
    


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    /*
     * First determine out of the whole collection
     * inode pointers (this is independent of whether
     * these are direct or indirect inode pointers), 
     * which one is the starting
     * inode pointer and which one is the ending
     * inode pointer. In this initial work,	
     * we are only counting inode pointers that directly
     * point to data blocks. Thus if a an indirect inode pointer
     * is involved, this calculation is not directly looking
     * indirect inode pointers, but instead looking at direct
     * inode pointers that are referenced by the indirect
     * inode pointers.
     */


    start_inode_ptr = offset/cufs->fs_block_size;

    start_offset_in_inode_ptr = offset % cufs->fs_block_size;


    num_inode_ptrs = num_bytes/cufs->fs_block_size;

    if (num_bytes % cufs->fs_block_size) {
	num_inode_ptrs++;
    }

    if ((offset % cufs->fs_block_size) &&
	(offset/cufs->fs_block_size != (offset+num_bytes)/cufs->fs_block_size)) {

	/*
	 * If the offset is not aligned on a filesystem block and
	 * the start and end of the transfer are not on the same
	 * filesystem block, then we need to span an additional
	 * filesystem block.
	 */
	num_inode_ptrs++;
    }

    *num_disk_lbas = num_inode_ptrs;

    
    end_offset_in_inode_ptr = (offset + num_bytes) % cufs->fs_block_size;


    /*
     * First create list of inode pointer in fs_block_size LBAs,
     * then we'll convert the fs_block_size LBAs to disk_block_size
     * LBAs in the middle inode pointers. And we'll adjust the
     * the start and ending offset to the closest disk_block_size
     * lba.
     */

    
    lba_list = malloc(sizeof(cflsh_usfs_disk_lba_e_t) * num_inode_ptrs);

    if (lba_list == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Malloc of disk_lbas of for num_inode_ptrs = %d of on disk %s failed with errno %d ",
			    cufs->device_name,errno);

	
	
	return -1;
    }

    CUSFS_TRACE_LOG_FILE(9,"type %d on disk %s, offset = 0x%llx, start_inode_ptr = 0x%llx, num_inode_ptrs = 0x%llx",
			 inode->type, cufs->device_name,offset,start_inode_ptr,num_inode_ptrs);
    
    /*
     * This code assumes each inode pointer to points to exactly one fs_block_size
     * block.
     */

    
    for (i = start_inode_ptr,j=0; i < (start_inode_ptr + num_inode_ptrs); i++,j++) {

	
	lba_list[j].num_lbas = cufs->fs_block_size/cufs->disk_block_size;


	if (i < CFLSH_USFS_NUM_DIRECT_INODE_PTRS) {

	    start_fs_lba = inode->ptrs[i];
		
	    CUSFS_TRACE_LOG_FILE(9,"start_fs_lba = 0x%llx, i = 0x%x ",
				 start_fs_lba,i,cufs->device_name);
	    

	} else {

	    
	    if (inode_single_block_list == NULL) {

		inode_single_block_list = malloc(cufs->fs_block_size);

		if (inode_single_block_list == NULL) {

		    CUSFS_TRACE_LOG_FILE(1,"Malloc of single_indirect of for num_inode_ptrs = %d on disk %s failed with errno %d ",
					cufs->device_name,errno);
	    
		    return -1;
		}

		/*
		 * Read in single indirect inode ptrs
		 */

		if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_single_block_list,
						   inode->ptrs[CFLSH_USFS_INDEX_SINGLE_INDIRECT],
							       nblocks,0)) {

		    if (inode_single_block_list) {
			free(inode_single_block_list);
		    }
		    if (inode_double_block_list) {
			free(inode_double_block_list);
		    }
		    if (inode_triple_block_list) {
			free(inode_triple_block_list);
		    }
		    free(lba_list);
		    return -1;
		}


	    }
	    
	    
	    /*
	     * Since we flatten out the inode pointers to just
	     * the direct ones eliminating the indirect varieties
	     */

	    if ((i >= CFLSH_USFS_NUM_DIRECT_INODE_PTRS) &&
		(i < (num_inode_ptrs_per_fs_block + CFLSH_USFS_NUM_DIRECT_INODE_PTRS))) {

		/*
		 * These inode pointers are in the single inode pointer list
		 */

		
		start_fs_lba = inode_single_block_list[i - CFLSH_USFS_NUM_DIRECT_INODE_PTRS];


		CUSFS_TRACE_LOG_FILE(9,"start_fs_lba = 0x%llx, i = 0x%x on disk %s",
				     start_fs_lba,i,cufs->device_name);
		
	    } else if ((i >= num_inode_ptrs_per_fs_block + CFLSH_USFS_NUM_DIRECT_INODE_PTRS) &&
		      (i < (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block  + CFLSH_USFS_NUM_DIRECT_INODE_PTRS))) {


		/*
		 * These inode pointers are in the double inode pointer list
		 */
		
		if (inode_double_block_list == NULL) {

		    double_index = 0;
		    single_index = 0;

		    inode_double_block_list = malloc(cufs->fs_block_size);

		    if (inode_double_block_list == NULL) {

			CUSFS_TRACE_LOG_FILE(1,"Malloc of single_indirect of for num_inode_ptrs = %d on disk %s failed with errno %d ",
					    cufs->device_name,errno);
	    
			return -1;
		    }

		    /*
		     * Read in double indirect inode ptrs
		     */
		    
		    
		    if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_double_block_list,
						 inode->ptrs[CFLSH_USFS_INDEX_DOUBLE_INDIRECT],
						 nblocks,0)) {
			
			if (inode_single_block_list) {
			    free(inode_single_block_list);
			}
			if (inode_double_block_list) {
			    free(inode_double_block_list);
			}
			if (inode_triple_block_list) {
			    free(inode_triple_block_list);
			}
			free(lba_list);
			return -1;
		    }

		}
		
		if (single_index == 0) {
		    /*
		     * Read in another block of single indirect inode pointers
		     */
		    
		    if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_single_block_list,
						 inode_double_block_list[double_index],
						 nblocks,0)) {
			
			if (inode_single_block_list) {
			    free(inode_single_block_list);
			}
			if (inode_double_block_list) {
			    free(inode_double_block_list);
			}
			if (inode_triple_block_list) {
			    free(inode_triple_block_list);
			}
			free(lba_list);
			return -1;
		    }

		}
		
		start_fs_lba = inode_single_block_list[single_index];
		

		CUSFS_TRACE_LOG_FILE(9,"start_fs_lba = 0x%llx, i = 0x%x on disk %s",
				     start_fs_lba,i,cufs->device_name);
		
		if (single_index < num_inode_ptrs_per_fs_block) {

		    single_index++;
		} else {

		    single_index = 0;

		    double_index++;
		}


	    } else if ((i >= (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block  + CFLSH_USFS_NUM_DIRECT_INODE_PTRS)) &&
		       (i < (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block  + CFLSH_USFS_NUM_DIRECT_INODE_PTRS))) {


		/*
		 * These inode pointers are in the triple inode pointer list
		 */


		if (inode_triple_block_list == NULL) {

		    triple_index = 0;
		    double_index = 0;
		    single_index = 0;

		    inode_triple_block_list = malloc(cufs->fs_block_size);

		    if (inode_triple_block_list == NULL) {

			CUSFS_TRACE_LOG_FILE(1,"Malloc of triple_indirect of for num_inode_ptrs = %d on disk %s failed with errno %d ",
					    cufs->device_name,errno);
	    
			return -1;
		    }

		    /*
		     * Read in triple indirect inode ptrs
		     */
		    
		    
		    if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_triple_block_list,
						 inode->ptrs[CFLSH_USFS_INDEX_TRIPLE_INDIRECT],
						 nblocks,0)) {
			
			if (inode_single_block_list) {
			    free(inode_single_block_list);
			}
			if (inode_double_block_list) {
			    free(inode_double_block_list);
			}
			if (inode_triple_block_list) {
			    free(inode_triple_block_list);
			}
			free(lba_list);
			return -1;
		    }

		}
		
		
		if (double_index == 0) {
		    /*
		     * Read in another block of single indirect inode pointers
		     */
		    
		    if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_double_block_list,
						 inode_triple_block_list[triple_index],
						 nblocks,0)) {
			
			if (inode_single_block_list) {
			    free(inode_single_block_list);
			}
			if (inode_double_block_list) {
			    free(inode_double_block_list);
			}
			if (inode_triple_block_list) {
			    free(inode_triple_block_list);
			}
			free(lba_list);
			return -1;
		    }

		}
		
		if (single_index == 0) {
		    /*
		     * Read in another block of single indirect inode pointers
		     */
		    
		    if (cusfs_rdwr_block_inode_pointers(cufs,inode,inode_single_block_list,
						 inode_double_block_list[double_index],
						 nblocks,0)) {
			
			if (inode_single_block_list) {
			    free(inode_single_block_list);
			}
			if (inode_double_block_list) {
			    free(inode_double_block_list);
			}
			if (inode_triple_block_list) {
			    free(inode_triple_block_list);
			}
			free(lba_list);
			return -1;
		    }

		}

		start_fs_lba = inode_single_block_list[single_index];
		
		
		CUSFS_TRACE_LOG_FILE(9,"start_fs_lba = 0x%llx, i = 0x%x on disk %s",
				     start_fs_lba,i,cufs->device_name);

		if (single_index < num_inode_ptrs_per_fs_block) {

		    single_index++;
		} else {

		    single_index = 0;
		    
		    if (double_index < num_inode_ptrs_per_fs_block) {
			
			double_index++;
		    } else {
			
			double_index = 0;
			
			triple_index++;
		    }
		}


	    } else {

		CUSFS_TRACE_LOG_FILE(1,"Invalid inode pointer %d in main inode of type %d on disk %s",
				    i,inode->type, cufs->device_name);

	
		if (inode_single_block_list) {
		    free(inode_single_block_list);
		}
		if (inode_double_block_list) {
		    free(inode_double_block_list);
		}
		if (inode_triple_block_list) {
		    free(inode_triple_block_list);
		}
		free(lba_list);
		return -1;
		
	    }
	}
	

	if (start_fs_lba == 0) {

	
	    CUSFS_TRACE_LOG_FILE(1,"LBA of 0 found for index i = %d  on device , ",
				 i,cufs->device_name);

	    
	    if (inode_single_block_list) {
		free(inode_single_block_list);
	    }
	    if (inode_double_block_list) {
		free(inode_double_block_list);
	    }
	    if (inode_triple_block_list) {
		free(inode_triple_block_list);
	    }
	    free(lba_list);
	    return -1;
	}

	

	/*
	 * Convert block number from and filesystem block size
	 * block number, to a disk block size block number.
	 */

	if (cusfs_get_disk_block_no_from_fs_block_no(cufs,start_fs_lba,&start_disk_lba)) {

	
	    CUSFS_TRACE_LOG_FILE(1,"type %d on disk %s failed to get disk blocks for 0x%llx, ",
				inode->type, cufs->device_name,start_fs_lba);

	
	    if (inode_single_block_list) {
		free(inode_single_block_list);
	    }
	    if (inode_double_block_list) {
		free(inode_double_block_list);
	    }
	    if (inode_triple_block_list) {
		free(inode_triple_block_list);
	    }
	    free(lba_list);
	    return -1;
	}


	    
	lba_list[j].start_lba = start_disk_lba;

	num_lbas =  lba_list[j].num_lbas;

	if ((i == start_inode_ptr) &&
	    (start_offset_in_inode_ptr)) {

	    /*
	     * Adjust the transfer's start lba to the closest disk lba in this
	     * fs lba.
	     */
	    start_disk_lba += start_offset_in_inode_ptr/cufs->disk_block_size;

	    lba_list[j].num_lbas -= (start_disk_lba - lba_list[j].start_lba);

	    lba_list[j].start_lba = start_disk_lba;
	} 

	if ((i == (num_inode_ptrs - 1)) &&
	    (end_offset_in_inode_ptr)) {


	    /*
	     * Adjust the transfer's end lba to the closest disk lba in this
	     * fs lba
	     */
	    
	    num_lbas2 = end_offset_in_inode_ptr/cufs->disk_block_size;

	    if (end_offset_in_inode_ptr % cufs->disk_block_size) {
		num_lbas2++;
	    }

	    if (num_lbas2 < num_lbas) {

		lba_list[j].num_lbas -= (num_lbas - num_lbas2);
	    }

	}
	
    }
    
    *disk_lbas = lba_list;

    if (inode_single_block_list) {
	free(inode_single_block_list);
    }
    if (inode_double_block_list) {
	free(inode_double_block_list);
    }
    if (inode_triple_block_list) {
	free(inode_triple_block_list);
    }
    return rc;
}


/*
 * NAME:        cusfs_get_all_disk_lbas_from_inode
 *
 * FUNCTION:    For the specified inode, get a list of
 *              all disk LBA ranges that are associated with it.
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_get_all_disk_lbas_from_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
						   int flags)
{
    int level;
    int rc;
    size_t num_bytes;

    CUSFS_TRACE_LOG_FILE(9,"inode->type %d on disk = %s",
			 inode->type, cufs->device_name);


    if (cusfs_num_data_blocks_in_inode(cufs,inode,num_disk_lbas,&level,0)) {

	*num_disk_lbas = 0;
	return -1;
    }

    num_bytes = (*num_disk_lbas) * cufs->fs_block_size;

    CUSFS_TRACE_LOG_FILE(9,"num_disk_lbas = 0x%llx",
			 *num_disk_lbas);

    rc = cusfs_get_disk_lbas_from_inode_ptr_for_transfer(cufs,inode,0,num_bytes,disk_lbas,num_disk_lbas,0);

    CUSFS_TRACE_LOG_FILE(9,"rc = %d, errno = %d",
			rc, errno);
    return rc;
}


/*
 * NAME:        cusfs_get_inode_ptr_for_transfer
 *
 * FUNCTION:    Determine which inode pointer in the inode are
 *              associated with this transfer
 *
 *
 * INPUTS:      ?? Remove thie routine?
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_get_inode_ptr_from_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
					uint64_t offset, size_t num_bytes, 
					int *start_inode_ptr_index,
					int *start_inode_ptr_byte_offset,
					int *end_inode_ptr_index,
					int *end_inode_ptr_byte_offset,
					int *num_inode_ptr_indices,
					int flags)
{
    int indirect_inode_ptr;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (file == NULL) {

	errno = EINVAL;
	return -1;
    }

    if ((start_inode_ptr_index == NULL) ||
	(end_inode_ptr_index == NULL)) {


	errno = EINVAL;
	return -1;
    }


    *start_inode_ptr_index = cusfs_get_inode_ptr_from_file_offset(cufs,file,offset,&indirect_inode_ptr,0);

    if (*start_inode_ptr_index < 0) {

	
	    CUSFS_TRACE_LOG_FILE(1,"For file = %s on disk %s failed to get starting inode_ptr for offset = 0x%llx, ",
				file->filename,cufs->device_name,offset);

	
	    
	    return -1;
    }

    /*
     * ?? TODO: Currently this code discards the value of indirect_inode_ptr
     */

    *start_inode_ptr_byte_offset = offset % cufs->fs_block_size;



    *end_inode_ptr_index = cusfs_get_inode_ptr_from_file_offset(cufs,file,(offset + num_bytes),&indirect_inode_ptr,0);

    if (*end_inode_ptr_index < 0) {

	
	    CUSFS_TRACE_LOG_FILE(1,"For file = %s on disk %s failed to get starting inode_ptr for offset = 0x%llx, ",
				file->filename,cufs->device_name,(offset + num_bytes));

	
	    
	    return -1;
    }

    /*
     * ?? TODO: Currently this code discards the value of indirect_inode_ptr
     */

    *end_inode_ptr_byte_offset = offset % cufs->fs_block_size;



    *num_inode_ptr_indices = end_inode_ptr_index - start_inode_ptr_index + 1;


    
    return 0;
}


/*
 * NAME:        cusfs_find_disk_lbas_in_inode_ptr_for_transfer
 *
 * FUNCTION:    For the specified inode pointer index, determine
 *              which disk lba's are associated with this transfer
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_find_disk_lbas_in_inode_ptr_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
						  int inode_ptr_index,
						  uint64_t *start_disk_lba, 
						  uint64_t *num_disk_blocks,
						  int start_inode_ptr,
						  int end_inode_ptr,
						  int transfer_start_offset,
						  int transfer_end_offset, 
						  int flags)
{
    uint64_t end_lba;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (file == NULL) {

	errno = EINVAL;
	return -1;
    }

    if ((start_disk_lba == NULL) ||
	(num_disk_blocks == NULL)) {


	errno = EINVAL;
	return -1;
    }


    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,file->inode->ptrs[inode_ptr_index],start_disk_lba)) {

	
	CUSFS_TRACE_LOG_FILE(1,"file = %s of type %d on disk %s failed to get disk blocks for 0x%llx, ",
			    file->filename,file->inode->type, cufs->device_name,file->inode->ptrs[inode_ptr_index]);

	
	    
	return -1;
    }


    end_lba = *start_disk_lba + cufs->fs_block_size/cufs->disk_block_size;


    if (inode_ptr_index == start_inode_ptr) {

	*start_disk_lba += transfer_start_offset/cufs->disk_block_size;
    }

    if (inode_ptr_index == end_inode_ptr) {

	*num_disk_blocks = transfer_end_offset/cufs->disk_block_size;
	if (transfer_end_offset % cufs->disk_block_size ) {
	    (*num_disk_blocks)++;
	} 
    }else {

	*num_disk_blocks = end_lba - *start_disk_lba + 1;

	    
    }

    return 0;

}

/*
 * NAME:        cusfs_num_data_blocks_single_indirect_inode_ptr
 *
 * FUNCTION:    Determine the number of data blocks currently
 *              referenced by this single indirect inode pointer
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_num_data_blocks_single_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t single_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_single_block_list = NULL;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }


    if (single_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_single_block_list = malloc(cufs->fs_block_size);

    if (inode_single_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of single_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,single_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    single_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_single_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    CUSFS_TRACE_LOG_FILE(9,"inode->type %d nblocks = 0x%llx, disk_lba = 0x%llx,on disk = %s",
			 inode->type, nblocks,disk_lba,cufs->device_name);

    rc = cblk_read(cufs->chunk_id,inode_single_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_single_block_list);
	return -1;
    }
    
    CUSFS_TRACE_LOG_FILE(9,"inode->type %d rc = 0x%llx, num_inode_ptrs_per_fs_block = 0x%llx,on disk = %s",
			 inode->type, rc,num_inode_ptrs_per_fs_block,cufs->device_name);


    for (i = 0; i < num_inode_ptrs_per_fs_block;i++) {

	if (inode_single_block_list[i] == 0x0LL) {

	    *num_fs_blocks = i;
	    break;
	}
	    
    }


    CUSFS_TRACE_LOG_FILE(9,"*num_fs_blocks = 0x%llx",
			 *num_fs_blocks);

    free(inode_single_block_list);
    return 0;



}



/*
 * NAME:        cusfs_num_data_blocks_double_indirect_inode_ptr
 *
 * FUNCTION:    Determine the number of data blocks currently
 *              referenced by this double indirect inode pointer
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_num_data_blocks_double_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t double_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_double_block_list = NULL;
    uint64_t num_indirect_blocks;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (double_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }

    *num_fs_blocks = 0;


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_double_block_list = malloc(cufs->fs_block_size);

    if (inode_double_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of double_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,double_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    double_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_double_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_double_block_list);
	return -1;
    }
    

    for (i = 0; i < num_inode_ptrs_per_fs_block;i++) {


	if (cusfs_num_data_blocks_single_indirect_inode_ptr(cufs,inode,
							   inode_double_block_list[i],
							   &num_indirect_blocks,0)) {


	    rc = -1;
	    *num_fs_blocks += num_indirect_blocks;
	    break;
	    

	}

	if (num_indirect_blocks < num_inode_ptrs_per_fs_block) {

	    *num_fs_blocks += num_indirect_blocks;
	    break;
	}

	
	*num_fs_blocks += num_indirect_blocks;
	    
    }



    free(inode_double_block_list);
    return 0;



}



/*
 * NAME:        cusfs_num_data_blocks_triple_indirect_inode_ptr
 *
 * FUNCTION:    Determine the number of data blocks currently
 *              referenced by this triple indirect inode pointer
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_num_data_blocks_triple_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t triple_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_triple_block_list = NULL;
    uint64_t num_indirect_blocks;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (triple_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }

    *num_fs_blocks = 0;


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_triple_block_list = malloc(cufs->fs_block_size);

    if (inode_triple_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of triple_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,triple_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    triple_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_triple_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_triple_block_list);
	return -1;
    }
    

    for (i = 0; i < num_inode_ptrs_per_fs_block;i++) {


	if (cusfs_num_data_blocks_single_indirect_inode_ptr(cufs,inode,
							   inode_triple_block_list[i],
							   &num_indirect_blocks,0)) {


	    rc = -1;
	    *num_fs_blocks += num_indirect_blocks;
	    break;
	    

	}

	if (num_indirect_blocks < num_inode_ptrs_per_fs_block) {

	    *num_fs_blocks += num_indirect_blocks;
	    break;
	}

	
	*num_fs_blocks += num_indirect_blocks;
	    
    }



    free(inode_triple_block_list);
    return 0;



}



/*
 * NAME:        cusfs_num_data_blocks_in_inode
 *
 * FUNCTION:    Determine the number of data blocks currently
 *              referenced by this triple indirect inode pointer
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_num_data_blocks_in_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
				  uint64_t *num_fs_blocks, int *level,int flags)
{


    int rc = 0;
    uint64_t num_data_blocks;
    int i;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t num_indirect_blocks;


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);

    num_data_blocks = 0;


    CUSFS_TRACE_LOG_FILE(9,"inode->type %d on disk = %s",
			 inode->type, cufs->device_name);

    for (i = 0; i < CFLSH_USFS_NUM_TOTAL_INODE_PTRS; i++) {

	if (i < CFLSH_USFS_NUM_DIRECT_INODE_PTRS) {

	    if (inode->ptrs[i] == 0x0LL) {

		*level = (CFLSH_USFS_NUM_DIRECT_INODE_PTRS -1);
		
		CUSFS_TRACE_LOG_FILE(9,"i = 0x%x, level = 0x%x ",
				 i,*level);
		break;
	    }

	    num_data_blocks++;

	} else if (i < CFLSH_USFS_INDEX_DOUBLE_INDIRECT) {

	    if (inode->ptrs[i] == 0x0LL) {
		
		*level = (CFLSH_USFS_NUM_DIRECT_INODE_PTRS -1);
		CUSFS_TRACE_LOG_FILE(9,"i = 0x%x, level = 0x%x ",
				 i,*level);
		break;
	    }

	    if (cusfs_num_data_blocks_single_indirect_inode_ptr(cufs,inode,
							       inode->ptrs[i],
							       &num_indirect_blocks,0)) {

		rc = -1;
		break;
	    }
	
	    num_data_blocks += num_indirect_blocks;


	    if (num_indirect_blocks < num_inode_ptrs_per_fs_block) {

		
		*level = CFLSH_USFS_INDEX_SINGLE_INDIRECT;
		CUSFS_TRACE_LOG_FILE(9,"i = 0x%x, level = 0x%x ",
				 i,*level);
		break;
	    }

	} else if (i < CFLSH_USFS_INDEX_TRIPLE_INDIRECT) {



	    if (inode->ptrs[i] == 0x0LL) {
		
		*level = CFLSH_USFS_INDEX_SINGLE_INDIRECT;
		CUSFS_TRACE_LOG_FILE(9,"i = 0x%x, level = 0x%x ",
				 i,*level);
		break;
	    }

	    if (cusfs_num_data_blocks_double_indirect_inode_ptr(cufs,inode,
							       inode->ptrs[i],
							       &num_indirect_blocks,0)) {

		rc = -1;

		break;
	    }

	    num_data_blocks += num_indirect_blocks;


	    if (num_indirect_blocks < num_inode_ptrs_per_fs_block) {

		*level = CFLSH_USFS_INDEX_DOUBLE_INDIRECT;
		break;
	    }

	} else if (i == CFLSH_USFS_INDEX_TRIPLE_INDIRECT) {


	    if (inode->ptrs[i] == 0x0LL) {
		
		*level = CFLSH_USFS_INDEX_DOUBLE_INDIRECT;
		CUSFS_TRACE_LOG_FILE(9,"i = 0x%x, level = 0x%x ",
				 i,*level);
		break;
	    }

	    if (cusfs_num_data_blocks_triple_indirect_inode_ptr(cufs,inode,
							       inode->ptrs[i],
							       &num_indirect_blocks,0)) {
		rc = -1;

		break;
	    }

	    
	    *level = CFLSH_USFS_INDEX_TRIPLE_INDIRECT;
	    num_data_blocks += num_indirect_blocks;

	}

    } /* for */


    *num_fs_blocks = num_data_blocks;

    CUSFS_TRACE_LOG_FILE(9,"num_data_blocks = 0x%llx, level = 0x%x ",
			 num_data_blocks,*level);
    return rc;
}
/*
 * NAME:        cusfs_get_next_inode_data_block
 *
 * FUNCTION:    Add up to num_fs_lbas data blocks to this
 *              single indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 failure, otherwise success
 *              
 *              
 */

uint64_t cusfs_get_next_inode_data_block(cflsh_usfs_t *cufs,  uint64_t **block_list, 
					 uint64_t *block_list_index,uint64_t num_needed_blocks, int flags)
{
    uint64_t block_list_num;
    uint64_t *local_block_list;

    if (cufs == NULL) {

	errno = EINVAL;
	return 0;

    }     

    if (block_list_index == NULL) {

	errno = EINVAL;
	return 0;

    }     

    if (num_needed_blocks < 1) {


	errno = EINVAL;
	return 0;
    }

    
    if (*block_list == NULL) {


	block_list_num = num_needed_blocks;

	*block_list_index = 0;

	if (cusfs_get_data_blocks_list(cufs, &block_list_num, block_list,0)) {


	    CUSFS_TRACE_LOG_FILE(1,"failed to get requested blocks 0x%llx  on disk %s",
				 num_needed_blocks, cufs->device_name);
	    return 0;
	}

	if (block_list_num < num_needed_blocks) {

	    CUSFS_TRACE_LOG_FILE(1,"failed to get requested blocks 0x%llx, insteaed got 0x%llx  on disk %s",
				 num_needed_blocks, block_list_num,cufs->device_name);
	    free(*block_list);
	    return 0;
	}

    } else {


	if (*block_list_index >= num_needed_blocks) {

	    CUSFS_TRACE_LOG_FILE(1,"block_list_index = 0x%llx is too large *block_list_num = 0x%llx on disk %s",
				 *block_list_index,num_needed_blocks, cufs->device_name);

	    errno = EINVAL;
	    return 0;


	}

    }

    local_block_list = *block_list;

    CUSFS_TRACE_LOG_FILE(9,"local_block_list[0x%llx] = 0x%llx  on disk %s",
			 block_list_index,local_block_list[*block_list_index], cufs->device_name);

    return (local_block_list[(*block_list_index)++]);
}

/*
 * NAME:        cusfs_add_fs_blocks_to_single_inode_ptr
 *
 * FUNCTION:    Add up to num_fs_lbas data blocks to this
 *              single indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_add_fs_blocks_to_single_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t single_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{
    int rc = 0;
    int fail_for = FALSE;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_single_block_list = NULL;
    int set_num_needed_blocks = FALSE;
    int num_needed_blocks = 0;               /* Number of disk blocks needed at this level */
    uint64_t *block_list = NULL;
    uint64_t  block_list_index = 0;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (single_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_single_block_list = malloc(cufs->fs_block_size);

    if (inode_single_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of single_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }

    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,single_indirect_inode_fs_lba,
						&disk_lba)) {
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    single_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_single_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_single_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);

	free(inode_single_block_list);
	return -1;
    }
    

    if (flags & CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK) {
    
	/*
	 * This the first time accessing this indirect
	 * block. So initialize it.
	 */

	bzero(inode_single_block_list,cufs->fs_block_size);

    }

    for (i = 0; i < num_inode_ptrs_per_fs_block && *num_fs_blocks > 0;i++) {

	if (inode_single_block_list[i] == 0x0LL) {

	    if (!set_num_needed_blocks) {

		set_num_needed_blocks = TRUE;

		num_needed_blocks = MIN((num_inode_ptrs_per_fs_block - i),*num_fs_blocks);

	    }

	    inode_single_block_list[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,
									 num_needed_blocks,0);

	    if (inode_single_block_list[i] == 0x0LL) {

		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		fail_for = TRUE;
		break;

	    }

	    (*num_fs_blocks)--;
	}
	    
    }


    if (block_list) {

	free(block_list);
    }

    rc = cblk_write(cufs->chunk_id,inode_single_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	rc = -1;

    } else {

	rc = 0;
    }

    if (fail_for) {

	rc = -1;
    }

    free(inode_single_block_list);
    return 0;
}


/*
 * NAME:        cusfs_add_fs_blocks_to_double_inode_ptr
 *
 * FUNCTION:    Add up to num_fs_lbas data blocks to this
 *              double indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_add_fs_blocks_to_double_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t double_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{
    int rc = 0;
    int fail_for = FALSE;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_double_block_list = NULL;
    int local_flags;
    int set_num_needed_blocks = FALSE;
    int num_needed_blocks = 0;               /* Number of disk blocks needed at this level */
    uint64_t *block_list = NULL;
    uint64_t  block_list_index = 0;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (double_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_double_block_list = malloc(cufs->fs_block_size);

    if (inode_double_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of double_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }

    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,double_indirect_inode_fs_lba,
						&disk_lba)) {
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    double_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_double_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);
    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);

	free(inode_double_block_list);
	return -1;
    }
    

    if (flags & CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK) {
    
	/*
	 * This the first time accessing this indirect
	 * block. So initialize it.
	 */

	bzero(inode_double_block_list,cufs->fs_block_size);

    }

    for (i = 0; i < num_inode_ptrs_per_fs_block && *num_fs_blocks > 0;i++) {

	if (inode_double_block_list[i] == 0x0LL) {

	    if (!set_num_needed_blocks) {

		set_num_needed_blocks = TRUE;
		
		num_needed_blocks = MIN((num_inode_ptrs_per_fs_block - i),*num_fs_blocks);

	    }

	    inode_double_block_list[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,
									 num_needed_blocks,0);

	    if (inode_double_block_list[i] == 0x0LL) {

		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		fail_for = TRUE;
		break;
		
	    }
	    
	    local_flags = CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK;
	} else {
	    
	    local_flags = 0;
	    
	}
	
	if (cusfs_add_fs_blocks_to_single_inode_ptr(cufs,inode,inode_double_block_list[i],num_fs_blocks,local_flags)) {
	    
	    fail_for = TRUE;
	    break;
	}
	
    }


    if (block_list) {

	free(block_list);
    }


    rc = cblk_write(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);
    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	rc = -1;
    } else {

	rc = 0;
    }


    if (fail_for) {

	rc = -1;
    }

    free(inode_double_block_list);
    return rc;
}


/*
 * NAME:        cusfs_add_fs_blocks_to_triple_inode_ptr
 *
 * FUNCTION:    Add up to num_fs_lbas data blocks to this
 *              triple indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_add_fs_blocks_to_triple_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t triple_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{
    int rc = 0;
    int fail_for = FALSE;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_triple_block_list = NULL;
    int local_flags;
    int set_num_needed_blocks = FALSE;
    int num_needed_blocks = 0;               /* Number of disk blocks needed at this level */
    uint64_t *block_list = NULL;
    uint64_t  block_list_index = 0;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (triple_indirect_inode_fs_lba == 0x0LL) {

	errno = EINVAL;
	return -1;
    }

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_triple_block_list = malloc(cufs->fs_block_size);

    if (inode_triple_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of triple_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }

    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,triple_indirect_inode_fs_lba,
						&disk_lba)) {
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    triple_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_triple_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);
    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);

	free(inode_triple_block_list);
	return -1;
    }
    



    if (flags & CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK) {
    
	/*
	 * This the first time accessing this indirect
	 * block. So initialize it.
	 */

	bzero(inode_triple_block_list,cufs->fs_block_size);

    }

    for (i = 0; i < num_inode_ptrs_per_fs_block && *num_fs_blocks > 0;i++) {

	if (inode_triple_block_list[i] == 0x0LL) {

	    if (!set_num_needed_blocks) {

		set_num_needed_blocks = TRUE;

		num_needed_blocks = MIN((num_inode_ptrs_per_fs_block - i),*num_fs_blocks);

	    }
	    
	    inode_triple_block_list[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,
									 num_needed_blocks,0);
	    
	    if (inode_triple_block_list[i] == 0x0LL) {
		
		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		fail_for = TRUE;
		break;
		
	    }
	    
	    local_flags = CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK;
	} else {
	    
	    local_flags = 0;
	    
	}
	
	if (cusfs_add_fs_blocks_to_double_inode_ptr(cufs,inode,inode_triple_block_list[i],num_fs_blocks,local_flags)) {
	    
	    fail_for = TRUE;
	    break;
	}
    }


    if (block_list) {

	free(block_list);
    }


    rc = cblk_write(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);
    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);
	
	rc = -1;
    }

    if (fail_for) {

	rc = -1;
    } else {

	rc = 0;
    }


    free(inode_triple_block_list);
    return rc;
}

/*
 * NAME:        cusfs_add_fs_blocks_to_inode_ptrs
 *
 * FUNCTION:    Add num_fs_blocks filesystem data blocks to
 *              the end of the inode pointers. This code
 *              will also get data blocks from
 *              the free block table for others to use.
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_add_fs_blocks_to_inode_ptrs(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
					  uint64_t num_fs_lbas,int flags)
{
    int rc = 0;
    int i;
    int local_flags;
    int first_unused_inode_ptr = 0;
    int num_needed_blocks = 1;               /* Number of disk blocks needed at this level */
    uint64_t *block_list = NULL;
    uint64_t  block_list_index = 0;
    uint64_t num_inode_ptrs_per_fs_block;


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_lbas < 1) {


	errno = EINVAL;
	return -1;
    }

    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);

    for (i = 0; i < CFLSH_USFS_NUM_TOTAL_INODE_PTRS && num_fs_lbas > 0; i++) {

	if (i < CFLSH_USFS_NUM_DIRECT_INODE_PTRS) {

	    if (inode->ptrs[i] == 0x0LL) {

		if (!first_unused_inode_ptr) {

		    first_unused_inode_ptr = i;

		    /*
		     * Determine number of data blocks needed
		     * at this level to satisfy the requested number
		     * fs blocks.
		     */

		    num_needed_blocks = MIN((CFLSH_USFS_NUM_DIRECT_INODE_PTRS - i + 1),num_fs_lbas);


		    if (num_fs_lbas > (num_inode_ptrs_per_fs_block + (CFLSH_USFS_NUM_DIRECT_INODE_PTRS - i))) {

			/*
			 * We also need a double indirect pointer
			 */
			num_needed_blocks++;

		    }

		    if (num_fs_lbas > (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block + num_inode_ptrs_per_fs_block + (CFLSH_USFS_NUM_DIRECT_INODE_PTRS - i))) {

			/*
			 * We also need a triple indirect pointer
			 */
			num_needed_blocks++;

		    }

		    
		}

		inode->ptrs[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,num_needed_blocks,0);

		if (inode->ptrs[i] == 0x0LL) {

		    CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
					inode->type, cufs->device_name);
		    break;
		}
		num_fs_lbas--;
	    }

	} else if (i < CFLSH_USFS_INDEX_DOUBLE_INDIRECT) {
	    
	    if (inode->ptrs[i] == 0x0LL) {
		

		if (!first_unused_inode_ptr) {
		    
		    num_needed_blocks = 1;

		    if (num_fs_lbas > num_inode_ptrs_per_fs_block) {

			/*
			 * We also need a double indirect pointer
			 */
			num_needed_blocks++;

		    }

		    if (num_fs_lbas > (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block)) {

			/*
			 * We also need a triple indirect pointer
			 */
			num_needed_blocks++;

		    }
		}

		inode->ptrs[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,num_needed_blocks,0);
		
		if (inode->ptrs[i] == 0x0LL) {
		    
		    CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
					inode->type, cufs->device_name);
		    break;
		}
		
		local_flags = CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK;
	    } else {
		
		local_flags = 0;
		
	    }
	    
	    if (cusfs_add_fs_blocks_to_single_inode_ptr(cufs,inode,inode->ptrs[i],&num_fs_lbas,local_flags)) {
		
		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		break;
	    }
	    

	} else if (i < CFLSH_USFS_INDEX_TRIPLE_INDIRECT) {


	    if (inode->ptrs[i] == 0x0LL) {


		if (!first_unused_inode_ptr) {
		    
		    num_needed_blocks = 1;

		    if (num_fs_lbas > (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block)) {

			/*
			 * We also need a triple indirect pointer
			 */
			num_needed_blocks++;

		    }

		}

		inode->ptrs[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,num_needed_blocks,0);

		if (inode->ptrs[i] == 0x0LL) {

		    CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
					inode->type, cufs->device_name);
		    break;
		}

		local_flags = CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK;
	    } else {

		local_flags = 0;

	    }
	    
	    if (cusfs_add_fs_blocks_to_double_inode_ptr(cufs,inode,inode->ptrs[i],&num_fs_lbas,local_flags)) {
		
		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		break;
	    }
	    

	} else  if (i == CFLSH_USFS_INDEX_TRIPLE_INDIRECT) {


	    if (inode->ptrs[i] == 0x0LL) {

		if (!first_unused_inode_ptr) {
		    
		    num_needed_blocks = 1;
		}

		inode->ptrs[i] = cusfs_get_next_inode_data_block(cufs,&block_list,&block_list_index,num_needed_blocks,0);

		if (inode->ptrs[i] == 0x0LL) {

		    CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
					inode->type, cufs->device_name);
		    break;
		}

		local_flags = CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK;
	    } else {

		local_flags = 0;

	    }
	    
	    if (cusfs_add_fs_blocks_to_triple_inode_ptr(cufs,inode,inode->ptrs[i],&num_fs_lbas,local_flags)) {
		
		CUSFS_TRACE_LOG_FILE(1,"Failed to get data block for indirect_inode_ptr for inode of type %d on disk %s",
				    inode->type, cufs->device_name);
		break;
	    }

	}
    }

    if (block_list) {

	free(block_list);
    }
    

    rc = cusfs_update_inode(cufs,inode,0);


    if ((rc == 0) &&
	(num_fs_lbas > 0)) {

	rc = -1;
    }
    return rc;
}


/*
 * NAME:        cusfs_remove_fs_blocks_from_single_inode_ptr
 *
 * FUNCTION:    Remove up to num_fs_lbas data blocks from this
 *              single indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_remove_fs_blocks_from_single_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t single_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_single_block_list = NULL;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_single_block_list = malloc(cufs->fs_block_size);

    if (inode_single_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of single_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,single_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    single_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_single_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_single_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_single_block_list);
	return -1;
    }
    

    for (i = num_inode_ptrs_per_fs_block; i > 0 && *num_fs_blocks > 0;i--) {

	if (inode_single_block_list[i] != 0x0LL) {


	    cusfs_release_data_blocks(cufs,inode_single_block_list[i],1,0);

	    inode_single_block_list[i] = 0;

	    (*num_fs_blocks)--;
	}
	    
    }


    rc = cblk_write(cufs->chunk_id,inode_single_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	
	rc = -1;
    }

    free(inode_single_block_list);
    return rc;



}



/*
 * NAME:        cusfs_remove_fs_blocks_from_double_inode_ptr
 *
 * FUNCTION:    Remove up to num_fs_lbas data blocks from this
 *              double indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_remove_fs_blocks_from_double_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t double_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_double_block_list = NULL;


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_double_block_list = malloc(cufs->fs_block_size);

    if (inode_double_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of double_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,double_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    double_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_double_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_double_block_list);
	return -1;
    }
    

    for (i = num_inode_ptrs_per_fs_block; i > 0 && *num_fs_blocks > 0;i--) {

	if (inode_double_block_list[i] != 0x0LL) {

	    if (cusfs_remove_fs_blocks_from_single_inode_ptr(cufs,inode,
							    inode_double_block_list[i],num_fs_blocks,0)) {

		/*
		 * Do best effort to write out the updated double indirect 
		 * block.
		 */

		cblk_write(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);
		free(inode_double_block_list);
		return -1;
	    }

	    
	    inode_double_block_list[i] = 0;


	}
	    
    }


    rc = cblk_write(cufs->chunk_id,inode_double_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);
	
	rc = -1;
    }
    

    free(inode_double_block_list);
    return rc;

}





/*
 * NAME:        cusfs_remove_fs_blocks_from_triple_inode_ptr
 *
 * FUNCTION:    Remove up to num_fs_lbas data blocks from this
 *              triple indirect inode pointer.
 *              
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_remove_fs_blocks_from_triple_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t triple_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags)
{


    int rc = 0;
    int i;
    uint64_t disk_lba;
    int nblocks;
    uint64_t num_inode_ptrs_per_fs_block;
    uint64_t *inode_triple_block_list = NULL;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_blocks == NULL) {

	errno = EINVAL;
	return -1;
    }


    num_inode_ptrs_per_fs_block = cufs->fs_block_size/sizeof(uint64_t);


    inode_triple_block_list = malloc(cufs->fs_block_size);

    if (inode_triple_block_list == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Malloc of triple_indirect of size 0x%x, for num_inode_ptrs = %d on disk %s failed with errno %d ",
			    cufs->fs_block_size,cufs->device_name,errno);
	    
	return -1;
    }



    /*
     * Convert fs_block_size sector to disk_block_size sector
     */
    
    
    if (cusfs_get_disk_block_no_from_fs_block_no(cufs,triple_indirect_inode_fs_lba,
						&disk_lba)) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Failed to get disk blocks for fs_lba 0x%ll for inode of type %d on disk %s",
			    triple_indirect_inode_fs_lba,inode->type, cufs->device_name);
	
	free(inode_triple_block_list);
	return -1;
    }

    nblocks = cufs->fs_block_size/cufs->disk_block_size;

    rc = cblk_read(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Read at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			    disk_lba,inode->type, cufs->device_name);
	
	
	free(inode_triple_block_list);
	return -1;
    }
    

    for (i = num_inode_ptrs_per_fs_block; i > 0 && *num_fs_blocks > 0;i--) {

	if (inode_triple_block_list[i] != 0x0LL) {

	    if (cusfs_remove_fs_blocks_from_double_inode_ptr(cufs,inode,
							    inode_triple_block_list[i],num_fs_blocks,0)) {

		/*
		 * Do best effort to write out the updated triple indirect 
		 * block.
		 */
		cblk_write(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);
		free(inode_triple_block_list);
		return -1;
	    }

	    inode_triple_block_list[i] = 0;


	}
	    
    }


    rc = cblk_write(cufs->chunk_id,inode_triple_block_list,disk_lba,nblocks,0);

    
    if (rc < nblocks) {
	
	
	CUSFS_TRACE_LOG_FILE(1,"Write at lba 0x%llx faild for indirect_inode_ptrt %d for inode of type %d on disk %s",
			     disk_lba,inode->type, cufs->device_name);
	
	
	rc = -1;
    }
    

    free(inode_triple_block_list);
    return rc;



}


/*
 * NAME:        cusfs_remove_fs_blocks_to_inode_ptrs
 *
 * FUNCTION:    Remove num_fs_blocks filesystem data blocks from
 *              the end of the inode pointers. This code
 *              will also make this data blocks available
 *              in the free block table for others to use.
 *              blocks to an inode, by update the inode.
 *
 *
 * INPUTS:
 *             
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */

int cusfs_remove_fs_blocks_from_inode_ptrs(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
					  uint64_t num_fs_lbas,int flags)
{
    int rc = 0;
    int i;
    int level;
    uint64_t num_data_blocks;

    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (inode == NULL) {

	errno = EINVAL;
	return -1;
    }

    if (num_fs_lbas < 1) {


	errno = EINVAL;
	return -1;
    }


    /*
     * First determine how many data blocks are
     * being referenced from this inode.
     *
     * The maximum possible total of such inode pointers
     * is the direct inode pointers in the 
     * inode itself (CFLSH_USFS_NUM_DIRECT_INODE_PTRS) +
     * the number of inode pointers in the first single indirect
     * inode pointer (num_inode_ptrs_per_fs_block) + the
     * number of inode pointers in the double indirect 
     * inode pointer (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block)
     * + the number of inode pinters in the triple indirect inode pointer
     * (num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block * num_inode_ptrs_per_fs_block).
     */



    num_data_blocks = 0;

    if (cusfs_num_data_blocks_in_inode(cufs,inode,&num_data_blocks,&level,0)) {

	return -1;
    }

    if (num_data_blocks < 1) {

	CUSFS_TRACE_LOG_FILE(1,"No inode pointers in use for this inode on disk %s",
			    cufs->device_name);
	    
	errno = EINVAL;
	return -1;
    }
    
    if (num_fs_lbas > num_data_blocks) {


	CUSFS_TRACE_LOG_FILE(1,"num_fs_lbas 0x%x > max_possible_inode_ptrs 0x%x on disk %s",
			    num_fs_lbas, num_data_blocks,cufs->device_name);
	    
	errno = EINVAL;
	return -1;
    }


    switch (level) {

    case CFLSH_USFS_INDEX_TRIPLE_INDIRECT:
	
	/*
	 * Remove up to num_fs_lbas from the
	 * triple indirect blocks
	 */
	
	if (cusfs_remove_fs_blocks_from_triple_inode_ptr(cufs, inode,
							inode->ptrs[CFLSH_USFS_INDEX_TRIPLE_INDIRECT],
							&num_fs_lbas,0)) {
	    break;
	}
	     
	if (num_fs_lbas == 0) {
	    break;
	}
	
	/* Drop through */

    case CFLSH_USFS_INDEX_DOUBLE_INDIRECT:

	
	/*
	 * Remove up to num_fs_lbas from the
	 * double indirect blocks
	 */
	
	if (cusfs_remove_fs_blocks_from_double_inode_ptr(cufs, inode,
							inode->ptrs[CFLSH_USFS_INDEX_DOUBLE_INDIRECT],
							&num_fs_lbas,0)) {
	    break;
	}
	     
	if (num_fs_lbas == 0) {
	    break;
	}
	
	/* Drop through */
    case CFLSH_USFS_INDEX_SINGLE_INDIRECT:

	
	/*
	 * Remove up to num_fs_lbas from the
	 * double indirect blocks
	 */
	
	if (cusfs_remove_fs_blocks_from_single_inode_ptr(cufs, inode,
							inode->ptrs[CFLSH_USFS_INDEX_SINGLE_INDIRECT],
							&num_fs_lbas,0)) {
	    break;
	}
	     
	if (num_fs_lbas == 0) {
	    break;
	}
	
	/* Drop through */
	
    case (CFLSH_USFS_NUM_DIRECT_INODE_PTRS - 1):

	for (i = (CFLSH_USFS_NUM_DIRECT_INODE_PTRS - 1); i >=0; i--) {

	    
	    cusfs_release_data_blocks(cufs,inode->ptrs[i],1,0);

	    inode->ptrs[i] = 0;

	    num_fs_lbas--;
	}
	break;
    default:
	CUSFS_TRACE_LOG_FILE(1,"Invalid level %d for disk %s",
			    level,cufs->device_name);

	errno = EINVAL;
	return -1;

    }

    rc = cusfs_update_inode(cufs,inode,0);

    return rc;
}
