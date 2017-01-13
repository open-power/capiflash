/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_linux.c $                              */
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
/* ----------------------------------------------------------------------------
 *
 * This file contains the linux specific code for the block library.
 * For other OSes, this file should not be linked in and instead replaced
 * with the analogous OS specific file.  
 *     
 * ----------------------------------------------------------------------------
 */ 


#define CFLSH_BLK_FILENUM 0x0400
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"
#include "cflash_block_protos.h"
#include <cxl.h>
#ifdef  _MASTER_CONTXT
#include <mclient.h>
#endif /* _MASTER_CONTXT */

#ifdef _USE_LIB_AFU
#include <afu.h> 
#endif /* _USE_LIB_AFU */
#ifdef _USE_LIB_AFU
struct afu *p_afu = NULL;
#endif /* _USE_LIB_AFU */

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_init_mc_interface
 *                  
 * FUNCTION:  Initialize master context (MC) interfaces for this process.
 *          
 *
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_init_mc_interface(void)
{
    char *lun = getenv("CFLSH_BLK_LUN_ID");



    if (lun) {
	cblk_lun_id = strtoul(lun,NULL,16);
    }
   

#ifdef  _MASTER_CONTXT

     mc_init();

#endif /* _MASTER_CONTXT */

    
    return;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_cleanup_mc_interface
 *                  
 * FUNCTION:  Initialize master context (MC) interfaces for this process.
 *          
 *
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_cleanup_mc_interface(void)
{


 
#ifdef  _MASTER_CONTXT   

     mc_term();

#endif /* _MASTER_CONTXT */


    
    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_get_os_chunk_type
 *                  
 * FUNCTION:  Get OS specific chunk types
 *          
 *
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
cflsh_block_chunk_type_t   cblk_get_os_chunk_type(const char *path, int arch_type)
{
    cflsh_block_chunk_type_t chunk_type;


    
    /*
     * For now we only support one chunk type:
     * the SIS lite type.
     */

    chunk_type = CFLASH_BLK_CHUNK_SIS_LITE;

    return chunk_type;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_find_parent_dev
 *                  
 * FUNCTION:  Find parent string of lun.
 *          
 *
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
char *cblk_find_parent_dev(char *device_name)
{

    return NULL;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_attach_process
 *                  
 * FUNCTION:  Attaches the current process to a chunk and
 *            maps the MMIO space.
 *                                                 
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_chunk_attach_process_map (cflsh_chunk_t *chunk, int mode, int *cleanup_depth)
{
#ifndef _USE_LIB_AFU 
    struct cxl_ioctl_start_work start_work;
#ifndef _MASTER_CONTXT
#ifdef _NOT_YET   
    uint64_t port_lun_table_start;
#endif /* _NOT_YET */
#endif
#endif
    cflsh_path_t *path = NULL;
    cflsh_afu_in_use_t in_use;
    int share = FALSE;

#ifdef _MASTER_CONTXT   
    int end;
#endif /* _MASTER_CONTXT */
    int rc = 0;
    cflsh_block_chunk_type_t chunk_type;

    
    if (chunk == NULL) {
	
	return (-1);
    }


    if (chunk->path[chunk->cur_path] == NULL) {

	/*
	 * For the case of forking a process, the
	 * child's path structures will already exist,
	 * and we need to preserve some of the data.
	 * So only get a path if one is not allocated.
	 */

	chunk_type = cblk_get_chunk_type(chunk->dev_name,0);



	/*
	 * For non-kernel MC, only use one path.
	 */

	chunk->cur_path = 0;

	if (chunk->flags & CFLSH_CHNK_SHARED) {

	    share = TRUE;
	}


	path = cblk_get_path(chunk,0,NULL,chunk_type,chunk->num_cmds,&in_use, share); 


	if (path == NULL) {

	    return (-1);
	}

	chunk->path[chunk->cur_path] = path;

	path->path_index = chunk->cur_path;

    }
    
    /*
     * For GA1, the poll filedescriptor
     * and the one used for open/ioctls are
     * the same.
     */
    chunk->path[chunk->cur_path]->afu->poll_fd = chunk->fd;
 
#ifdef _USE_LIB_AFU
    afu_start(p_afu);

    // set up RRQ
    // these funcs have syncs in them. offset is in 4-byte words.
    // assume problem space starts with the "Host transport MMIO regs"
    // in SISLite p 7.
    afu_mmio_write_dw(p_afu, 10, (uint64_t)chunk->path[chunk->cur_path]->afu->p_hrrq_start); // START_EA
    afu_mmio_write_dw(p_afu, 12, (uint64_t)chunk->path[chunk->cur_path]->afu->p_hrrq_end);   // END_EA


#else

    bzero(&start_work,sizeof(start_work));

    start_work.flags = CXL_START_WORK_NUM_IRQS;
    start_work.num_interrupts = 4;
    
    
#ifndef BLOCK_FILEMODE_ENABLED
    rc = ioctl(chunk->fd,CXL_IOCTL_START_WORK,&start_work);
    
    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"Unable to attach errno = %d",errno);
	perror("cblk_open: Unable to attach");

	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
	
    }    
    rc = ioctl(chunk->fd,CXL_IOCTL_GET_PROCESS_ELEMENT,&chunk->path[chunk->cur_path]->afu->contxt_handle);
    
    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"Unable to attach errno = %d",errno);
	perror("cblk_open: Unable to attach");

	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	
	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
	
    }
#else
    start_work.num_interrupts = 0;
    
#endif /* BLOCK_FILEMODE_ENABLED */
    
    
    CBLK_TRACE_LOG_FILE(6,"contxt_handle = 0x%x",chunk->path[chunk->cur_path]->afu->contxt_handle);
    
#ifdef  _MASTER_CONTXT  
    end = strlen(chunk->dev_name);
    if (chunk->dev_name[end-1] != 'm') {

	/*
	 * If this is not the master special file then only
	 * use the MMIO space for one non-master user.
	 */

	chunk->path[chunk->cur_path]->afu->mmap_size = CFLASH_MMIO_CONTEXT_SIZE;
    } else {
	/*
	 * If this is the master special file then 
	 * request the adapter's full MMIO address space.
	 */

	chunk->path[chunk->cur_path]->afu->mmap_size = CAPI_FLASH_REG_SIZE;
    }

#else

    chunk->path[chunk->cur_path]->afu->mmap_size = CAPI_FLASH_REG_SIZE;
#endif /* !_MASTER_CONTXT */    
    
    chunk->path[chunk->cur_path]->afu->mmio_mmap = mmap(NULL,chunk->path[chunk->cur_path]->afu->mmap_size,PROT_READ|PROT_WRITE, MAP_SHARED,
			    chunk->fd,0);
    
    if (chunk->path[chunk->cur_path]->afu->mmio_mmap == MAP_FAILED) {
	CBLK_TRACE_LOG_FILE(1,"mmap of mmio space failed errno = %d",errno);
	perror ("blk_open: mmap of mmio space failed");

	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */

	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
    }
 
    *cleanup_depth = 40;   
    
    CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx",(uint64_t)chunk->path[chunk->cur_path]->afu->mmio_mmap);
#ifdef  _MASTER_CONTXT  


    /*
     * If we have a separate master context, then we will not be opening the master context
     * special file. Thus the MMIO space returned is just for this process.
     */
    
    /*
     * TODO: ?? Currently we are using the master special file with master context and this
     *        requires us to treat the MMIO space the same as if the code was compiled without
     *       _MASTER_CONTEXT. At some future time, we need to disable this and only allow
     *       the non-master file.
     */

    end = strlen(chunk->dev_name);
    if (chunk->dev_name[end-1] != 'm') {

	/*
	 * If this is not the master special file then the
	 * returned mmap MMIO space is just this user's MMIO space.
	 */

	chunk->path[chunk->cur_path]->afu->mmio = chunk->path[chunk->cur_path]->afu->mmio_mmap;
    } else {
	/*
	 * If this is the master special file then the
	 * returned mmap MMIO space is everyone's MMIO space.
	 * Thus we need to find our MMIO space in this by using
	 * the process_element.
	 */

	chunk->path[chunk->cur_path]->afu->mmio = chunk->path[chunk->cur_path]->afu->mmio_mmap + CFLASH_MMIO_CONTEXT_SIZE * chunk->path[chunk->cur_path]->afu->contxt_handle;

    }

#else     


    /*
     * If we are using the master context special file,
     * then each call to mmap for the same CAPI adapter should return the beginning
     * of that adapter's MMIO space, we need to get to the MMIO space for this seecific
     * process (context), which is achieved by multiplying the returned process_element
     * number by the size of each context space. That gives the offset from the
     * returned chunk->path[chunk->cur_path].mmio_mapp address.
     */
    chunk->path[chunk->cur_path]->afu->mmio = chunk->path[chunk->cur_path]->afu->mmio_mmap + CFLASH_MMIO_CONTEXT_SIZE * chunk->path[chunk->cur_path]->afu->contxt_handle;

#endif /* !_MASTER_CONTXT */
    /*
     * Set up response queue
     */
    
    if (CBLK_ADAP_SETUP(chunk,chunk->cur_path)) {


	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
    }

#ifndef  _MASTER_CONTXT  
#ifndef BLOCK_FILEMODE_ENABLED

#ifdef _NOT_YET
    /*
     * Set up lun table for each FC port 0
     */

    port_lun_table_start = CAPI_AFU_GLOBAL_OFFSET + offsetof(struct sisl_global_map,fc_port_0[0]);


    
    if (CBLK_SETUP_BAD_MMIO_SIGNAL(chunk,chunk->cur_path,port_lun_table_start+8)) {
	
	/*
	 * If we get here then the MMIO below
	 * failed indicating the adapter either
	 * is being reset or encountered a UE.
	 */
	
	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
    }
    
    CBLK_TRACE_LOG_FILE(6,"chunk->path[chunk->cur_path].mmio = 0x%llx, port_lun_table_start = 0x%llx",chunk->path[chunk->cur_path]->afu->mmio,port_lun_table_start);


    out_mmio64 (chunk->path[chunk->cur_path]->afu->mmio + port_lun_table_start, cblk_lun_id);

    CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,chunk->cur_path); 



    /*
     * Set up lun table for each FC port 1
     */

    port_lun_table_start = CAPI_AFU_GLOBAL_OFFSET + offsetof(struct sisl_global_map,fc_port_1[0]);


    
    if (CBLK_SETUP_BAD_MMIO_SIGNAL(chunk,chunk->cur_path,port_lun_table_start+8)) {
	
	/*
	 * If we get here then the MMIO below
	 * failed indicating the adapter either
	 * is being reset or encountered a UE.
	 */
	
	cblk_release_path(chunk,(chunk->path[chunk->cur_path]));

	chunk->path[chunk->cur_path] = NULL;
	return -1;
    }
    
    CBLK_TRACE_LOG_FILE(6,"chunk->path[chunk->cur_path].mmio = 0x%llx, port_lun_table_start = 0x%llx",chunk->path[chunk->cur_path]->afu->mmio,port_lun_table_start);


    out_mmio64 (chunk->path[chunk->cur_path]->afu->mmio + port_lun_table_start, cblk_lun_id);

    CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,chunk->cur_path);
 
#endif /* _NOT_YET */

#endif /* !BLOCK_FILEMODE_ENABLED */
#endif /* !_MASTER_CONTXT */

#endif /* _USE_LIB_AFU */

    return rc;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_detach
 *                  
 * FUNCTION:  Detaches the current process.
 *            maps the MMIO space.
 *                                                 
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_chunk_detach (cflsh_chunk_t *chunk,int force)
{ 


    return;

}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_umap
 *                  
 * FUNCTION:  Attaches the current process to a chunk and
 *            maps the MMIO space.
 *                                                 
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_chunk_unmap (cflsh_chunk_t *chunk,int force)
{ 

#ifdef _USE_LIB_AFU

    afu_unmap(p_afu);
#else

    if (chunk->path[chunk->cur_path] == NULL) {

	/*
	 * Nothing to unmap.
	 */

	return;

    }


    if (chunk->path[chunk->cur_path]->afu->mmap_size == 0) {

	/*
	 * Nothing to unmap.
	 */

	return;
    }


    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[chunk->cur_path]->afu);

    if ((chunk->path[chunk->cur_path]->afu->ref_count == 1) ||
	(force)) {


	/*
	 * Only unmap on the last entity to use this afu. unless
	 * force is set.
	 */

	if (munmap(chunk->path[chunk->cur_path]->afu->mmio_mmap,chunk->path[chunk->cur_path]->afu->mmap_size)) {



	    /*
	     * Don't return here on error. Continue
	     * to close
	     */
	    CBLK_TRACE_LOG_FILE(2,"munmap failed with errno = %d",
				errno);
	}


#endif /* !_USE_LIB_AFU */

	chunk->path[chunk->cur_path]->afu->mmio = 0;
	chunk->path[chunk->cur_path]->afu->mmio_mmap = 0;
	chunk->path[chunk->cur_path]->afu->mmap_size = 0;

    }

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[chunk->cur_path]->afu);
}

  

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_get_mc_device_resources
 *                  
 * FUNCTION:  Get master context (MC) resources, which 
 *            include device information to allow 
 *            the device to be accessed for read/writes.  
 *          
 *
 * NOTES:  This routine assumes the caller has the chunk lock.
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_chunk_get_mc_device_resources(cflsh_chunk_t *chunk, 
					int *cleanup_depth)
{
    int rc = 0;
#ifdef _MASTER_CONTXT   
    char *env_block_size  = getenv("CFLSH_BLK_SIZE");
    uint32_t block_size = 0;
    mc_stat_t  mstat;
#ifndef BLOCK_FILEMODE_ENABLED
    int end;
#endif /* !BLOCK_FILEMODE_ENABLED */

#endif /* _MASTER_CONTXT */
    
    if (chunk == NULL) {
	
	return (-1);
    }
    
    if (chunk->path[chunk->cur_path] == NULL) {

	return (-1);
    }

	 
#ifndef _MASTER_CONTXT 
  
    /*
     * We can not be locked when we issue
     * commands, since they will do a lock.
     * Thus we would deadlock here.
     */
    
    CFLASH_BLOCK_UNLOCK(chunk->lock);
    
    if (cblk_get_lun_id(chunk)) {
	
	CFLASH_BLOCK_LOCK(chunk->lock);
	CBLK_TRACE_LOG_FILE(5,"cblk_get_lun_id failed errno = %d",
			    errno);
	
	return -1;
    }
    
    if (cblk_get_lun_capacity(chunk)) {
	
	CFLASH_BLOCK_LOCK(chunk->lock);
	CBLK_TRACE_LOG_FILE(5,"cblk_get_lun_capacity failed errno = %d",
			    errno);
	
	return -1;
    }
    
    CFLASH_BLOCK_LOCK(chunk->lock);

#else

    /*
     * TODO: ?? We probably need to eventually fail here
     *       if a special file with a suffix of 'm' is being
     *       used since this indicate we (the user) are opening
     *       the master special file.
     */



#ifndef BLOCK_FILEMODE_ENABLED

    end = strlen(chunk->dev_name);

    if (end < 1) {

	CFLASH_BLOCK_LOCK(chunk->lock);
	CBLK_TRACE_LOG_FILE(5,"adapter name two short %s",chunk->dev_name);
	
	return -1;
    }

    if (chunk->dev_name[end-1] != 'm') {

	sprintf(chunk->path[chunk->cur_path]->afu->master_name,"%s",chunk->dev_name);


	if (chunk->dev_name[end-1] == 's') {
	    chunk->path[chunk->cur_path]->afu->master_name[end-1] = '\0';
	} 
	strcat(chunk->path[chunk->cur_path]->afu->master_name,"m");
    } else {

#endif /* BLOCK_FILEMODE_ENABLED */

	sprintf(chunk->path[chunk->cur_path]->afu->master_name,"%s",chunk->dev_name);

#ifndef BLOCK_FILEMODE_ENABLED
    }

#endif /* BLOCK_FILEMODE_ENABLED */

    /*
     * Get a client handle for the specified AFU and context.
     */

    rc = mc_register(chunk->path[chunk->cur_path]->afu->master_name,chunk->path[chunk->cur_path]->afu->contxt_handle,chunk->path[chunk->cur_path]->afu->mmio,
		&(chunk->path[chunk->cur_path]->afu->master.mc_handle));

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_register failed with rc = %d, errno = %d for path = %s",
			    rc, errno,chunk->path[chunk->cur_path]->afu->master_name);
	return -1;
    }


    /*
     * Get a virtual lun for the specified AFU and context (via
     * the client handle).
     */

    rc = mc_open(chunk->path[chunk->cur_path]->afu->master.mc_handle,MC_RDWR,
		 &(chunk->path[chunk->cur_path]->sisl.resrc_handle));

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_open failed with rc = %d, errno = %d",
			    rc, errno);

	
	/*
	 * Free resources for the old context and AFU.
	 */
	rc = mc_unregister(chunk->path[chunk->cur_path]->afu->master.mc_handle);
 
	if (rc) {
	
	    CBLK_TRACE_LOG_FILE(5,"mc_unregister failed with rc = %d, errno = %d",
				rc, errno);
	}

	return -1;
    }

    if (env_block_size) {

	/*
	 * If environment for block size is set, then
	 * use it.
	 */
	  
	block_size = atoi(env_block_size);

	if (block_size == CAPI_FLASH_BLOCK_SIZE) {
	    /*
	     * If the device is reporting back 4K block size,
	     */
	    chunk->blk_size_mult = 1;
	} else {
	    /*
	     * If the device is reporting back an non-4K block size,
	     * then determine appropriate multiple
	     */

	  
	    if (block_size) {
		chunk->blk_size_mult = CAPI_FLASH_BLOCK_SIZE/block_size;
	    } else {
		chunk->blk_size_mult = 8;
	    }
	}
    } else {

	/*
	 * Issue mc_stat call to get the device's block
	 * size and the chunk/page size used by the MC
	 * for that device.
	 */

	bzero(&mstat,sizeof(mstat));

	rc = mc_stat(chunk->path[chunk->cur_path]->afu->master.mc_handle,
		     chunk->path[chunk->cur_path]->sisl.resrc_handle,&mstat);

	if (rc) {
	
	    CBLK_TRACE_LOG_FILE(1,"mc_stat failed with rc = %d, errno = %d",
				rc, errno);

	    cblk_chunk_free_mc_device_resources(chunk);

	    return -1;
	}

	block_size = mstat.blk_len;

	
	CBLK_TRACE_LOG_FILE(5,"mc_stat succeeded with block_size = %d, MC chunk/page size = %d, size= 0x%llx, flags = 0x%llx",
			    block_size, mstat.nmask,mstat.size,mstat.flags);
	if (block_size) {
	    chunk->blk_size_mult = CAPI_FLASH_BLOCK_SIZE/block_size;
	} else {
	    chunk->blk_size_mult = 8;
	}

	/*
	 * The chunk/page size is determined by mstat.nmask
	 * and using it as a bit shifter.
	 */

	chunk->path[chunk->cur_path]->afu->master.mc_page_size = 1 << mstat.nmask;

	if (chunk->path[chunk->cur_path]->afu->master.mc_page_size == 0) {

	    CBLK_TRACE_LOG_FILE(5,"mc_stat returned an invalid MC chunk/page size = %d",
				mstat.nmask);

	    cblk_chunk_free_mc_device_resources(chunk);

	    return -1;
	}
    }

#endif /* Master context */

    return rc;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_set_mc_size
 *                  
 * FUNCTION:  Request master context to provide the
 *            the specified storage for this chunk.
 *          
 *
 * NOTES:  This routine assumes the caller has the chunk lock.
 *
 *         This code assumes if the caller passes -1 for the 
 *         master context case, then it will return whatever
 *         space is available.
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:  0:        Good completion
 0           non-zero: Error 
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_chunk_set_mc_size(cflsh_chunk_t *chunk, size_t nblocks)
{
    int rc = 0;
#ifdef _MASTER_CONTXT   
    uint64_t size = 0;
    uint64_t actual_size = 0;


    if (chunk->path[chunk->cur_path] == NULL) {


	CBLK_TRACE_LOG_FILE(5,"chunk->path[chunk->cur_path] = NULL");

	errno = EINVAL;
	return -1;
    }


    if (chunk->path[chunk->cur_path]->afu->master.mc_page_size == 0) {


	CBLK_TRACE_LOG_FILE(5,"mc_page_size = 0");

	errno = EINVAL;
	return -1;
    }


    if (nblocks != -1) {

	/*
	 * Caller is requesting a specific amount of space
	 */

	if ((nblocks < chunk->path[chunk->cur_path]->afu->master.num_blocks) &&
	    ((chunk->path[chunk->cur_path]->afu->master.num_blocks * chunk->blk_size_mult) >= chunk->path[chunk->cur_path]->afu->master.mc_page_size) &&
	    (nblocks > (chunk->path[chunk->cur_path]->afu->master.num_blocks - (chunk->path[chunk->cur_path]->afu->master.mc_page_size/chunk->blk_size_mult)))) {

	    /*
	     * If the amount of space requested is is still within the current
	     * current MC page/chunk size, then just use the new value without
	     * calling mc_size again (since it is already allocated). There are three
	     * possibilities here: they are increasing space, descreasing space, or 
	     * requesting the same space (this last option is probably not very likely) , but 
	     * all are still within the same mc_page_size.
	     *
	     * NOTE: mc_page_size is in units of numnber of the device's block size
	     *       (ie. 512, or 4K) and num_blocks is in units of 4K.  chunk->blk_size_mult
	     *       is the multiple to go from the device's block size
	     *       to 4K (i.e. 1, 8).
	     * 
	     */

	    CBLK_TRACE_LOG_FILE(5,"blocks already exist so just use them");
	    chunk->num_blocks =  nblocks;
	    return 0;
	}

	
	/*
	 * if we get here then we need to request the MC for this
	 * specific size.
	 */


	if ((nblocks * chunk->blk_size_mult) % chunk->path[chunk->cur_path]->afu->master.mc_page_size) {

	    /*
	     * The size is not divisible by chunk->path[chunk->cur_path]->master.mc_page_size (i.e.
	     * it is not on a mc_page_size boundary. So round up 
	     * up to the next mc_page_size boundary.
	     * 
	     * NOTE: mc_page_size is minimum
	     * granularity of the MC in terms of the devices' block size),
	     */


	    size =  (nblocks * (chunk->blk_size_mult))/chunk->path[chunk->cur_path]->afu->master.mc_page_size + 1;
	} else {

	    /*
	     * This size is on a mc_page_size boundary.
	     */

	    size = (nblocks * (chunk->blk_size_mult))/chunk->path[chunk->cur_path]->afu->master.mc_page_size;
	}
    } else {

	/*
	 * caller is request all remaining space on this device,
	 * for this case we just pass the -1 thru.
	 */

	size = -1;
    }

    rc = mc_size(chunk->path[chunk->cur_path]->afu->master.mc_handle,
		 chunk->path[chunk->cur_path]->sisl.resrc_handle,
		 size,&actual_size);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_size failed with rc = %d, errno = %d, size = 0x%llx",
			    rc, errno,size);

	if (errno == 0) {

	    errno = ENOMEM;
	}
	return -1;
    }


    
    CBLK_TRACE_LOG_FILE(5,"mc_size succeed with size = 0x%llx and actual_size = 0x%llx",
			size, actual_size);

    if ((size != -1) &&
	(actual_size < size)) {


	CBLK_TRACE_LOG_FILE(1,"mc_size returned smaller actual size = 0x%llx then requested = 0x%llx",
			    actual_size,size);

	errno = ENOMEM;

	return -1;
    }


    /*
     * Save off the actual amount of space the MC allocated, which may be more than
     * what the user requested. Since mc_page_size is in unit of the device's block
     * size (chunk->blk_size_mult is the multiple to go from the device's block
     * size to 4K) and and our num_blocks is in units of 4K block sizes we need convert this 
     * size into 4K blocks.  
     */

    chunk->path[chunk->cur_path]->afu->master.num_blocks = (actual_size * chunk->path[chunk->cur_path]->afu->master.mc_page_size)/chunk->blk_size_mult;

    if (size == -1) {

	nblocks = chunk->path[chunk->cur_path]->afu->master.num_blocks;
    }
#else

  
    /*
     * TODO: ?? This is temporary code to for
     *       early development to allow virtual
     *       luns.  Eventually the MC will provision
     *       this. For now the block layer will use
     *       a very simplistic and flawed approach
     *       that leads to inefficient memory usage
     *       and fragmentation. However it is hoped
     *       this flawed approach is sufficient until
     *       the MC can provide the real functionality.
     *       When the MC does add this functionality,
     *       this code can be removed.
     */

    
    if ((nblocks + cflsh_blk.next_chunk_starting_lba)  > chunk->num_blocks_lun) {


	CBLK_TRACE_LOG_FILE(1,"set_size failed with EINVAL, nblocks = 0x%llx, next_lba = 0x%llx num_blocks_lun = 0x%llx",
			    (uint64_t)nblocks,(uint64_t)cflsh_blk.next_chunk_starting_lba,(uint64_t)chunk->num_blocks_lun);
	errno = EINVAL;
	return -1;
    }
    

    if (chunk->num_blocks) {


	/*
	 * If chunk->num_blocks is non-zero then this
	 * is a resize. 
	 */

	if (cflsh_blk.next_chunk_starting_lba ==
	    (chunk->start_lba + chunk->num_blocks)) {


	    /*
	     * If chunk->num_blocks is non-zero then this
	     * is a resize. If this is the last chunk on this physical disk,
	     * then set the next_chunk_start_lba to our chunk's
	     * starting LBA. For this case we do not need
	     * to update our start_lba since it is correct.
	     */
	    cflsh_blk.next_chunk_starting_lba = chunk->start_lba;

	} else {

	    /*
	     * The current implementation is very inefficient
	     * and has fragmentation issues. In this case
	     * it will move the chunk past the other chunks
	     * on this physical lun. All previous data will be
	     * lossed
	     */
	    chunk->start_lba = cflsh_blk.next_chunk_starting_lba;
	}
    } else {

	/*
	 * This is the first allocation of blocks
	 * for this chunk.
	 */

	chunk->start_lba = cflsh_blk.next_chunk_starting_lba;
    }


    cflsh_blk.next_chunk_starting_lba += nblocks;
    
    /*
     * TODO:  End of virtual lun hack
     */    
    
#endif /* Master context */


    chunk->num_blocks =  nblocks;
    return rc;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_mc_clone
 *                  
 * FUNCTION:  Requests master context to clone
 *            an existing AFU + context to this context
 *            on the same AFU. This is needed whenever 
 *            a process has forked to reenable access
 *            to the chunks from the parent process in the child
 *            process.
 *          
 *
 * NOTES:  This routine assumes the caller has the chunk lock.
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:  0:        Good completion
 0           non-zero: Error 
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_mc_clone(cflsh_chunk_t *chunk,int mode, int flags)
{
    int rc = 0;
#ifdef  _MASTER_CONTXT    
    int open_flags;
    uint64_t chunk_flags;
    int cleanup_depth;
    mc_hndl_t old_mc_handle;
    res_hndl_t old_resrc_handle; 
    int old_fd;
    void *old_mmio_mmap;
    size_t old_mmap_size;
 

    if (chunk->path[chunk->cur_path] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path == NULL");
	return -1;
    }

    /*
     * It should be noted the chunk is not a fully functional chunk
     * from this process' perspective after a fork. It has enough information that should allow
     * us to clone it into a new chunk using the same chunk id and chunk structure.
     * So first save off relevant information about the old chunk before unregistering 
     * it.
     */

    old_mc_handle = chunk->path[chunk->cur_path]->afu->master.mc_handle;
    old_resrc_handle = chunk->path[chunk->cur_path]->sisl.resrc_handle;
    old_fd = chunk->fd;
    old_mmio_mmap = chunk->path[chunk->cur_path]->afu->mmio_mmap;
    old_mmap_size = chunk->path[chunk->cur_path]->afu->mmap_size;

    /*
     * If we have a dedicated thread per chunk
     * for interrupts, then stop it now.
     */

    cblk_open_cleanup_wait_thread(chunk);

    open_flags = O_RDWR | O_NONBLOCK;   /* ??TODO Try without O_CLOEXEC */

    chunk->fd = open(chunk->dev_name,open_flags);
    if (chunk->fd < 0) {

	CBLK_TRACE_LOG_FILE(1,"Unable to open device errno = %d",errno);
	perror("cblk_open: Unable to open device");

	cblk_chunk_open_cleanup(chunk,cleanup_depth);
	free(chunk);

	    
	return -1;
    }

    cleanup_depth = 30;

    if (cblk_chunk_attach_process_map(chunk,mode,&cleanup_depth)) {

	CBLK_TRACE_LOG_FILE(1,"Unable to attach, errno = %d",errno);
	perror("cblk_open: Unable to open device");

	  
	cblk_chunk_open_cleanup(chunk,cleanup_depth);
	free(chunk);
	    
	return -1;

    }
    
    cleanup_depth = 40;

#ifdef _COMMON_INTRPT_THREAD

    /*
     * If we are using a common interrupt thread per chunk,
     * then restart it now.
     */

    if (cblk_start_common_intrpt_thread(chunk)) {


	CBLK_TRACE_LOG_FILE(1,"cblk_start_common_intrpt thread failed with errno= %d",
			    errno);

	    
	cblk_chunk_open_cleanup(chunk,cleanup_depth);


	return -1;
    }

    cleanup_depth = 45;
    
#endif  /* _COMMON_INTRPT_THREAD */

    /*
     * Get a client handle for the specified AFU and context.
     */

    rc = mc_register(chunk->path[chunk->cur_path]->afu->master_name,chunk->path[chunk->cur_path]->afu->contxt_handle,chunk->path[chunk->cur_path]->afu->mmio_mmap,
		&(chunk->path[chunk->cur_path]->afu->master.mc_handle));

    if (rc) {
	

	CBLK_TRACE_LOG_FILE(1,"mc_register for clone failed with rc = %d, errno = %d for path = %s",
			    rc,errno,chunk->path[chunk->cur_path]->afu->master_name);
	
	cblk_chunk_open_cleanup(chunk,cleanup_depth);

	return -1;
    }

    cleanup_depth = 50;


    switch (mode & O_ACCMODE) {

      case O_RDONLY:
	chunk_flags  = MC_RDONLY;
	break;
      case O_WRONLY:
	chunk_flags  = MC_WRONLY;
	break;
      case O_RDWR:
	chunk_flags  = MC_RDWR;
	break;
      default:
	chunk_flags  = MC_RDONLY;
    }


    CBLK_TRACE_LOG_FILE(5,"mc_clone chunk_flags 0x%x",
			chunk_flags);



    rc = mc_clone(chunk->path[chunk->cur_path]->afu->master.mc_handle,old_mc_handle,chunk_flags);


    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"mc_clone failed with rc = %d, errno = %d",
			    rc, errno);

	if (errno == 0) {

	    errno = EINVAL;
	}
	cblk_chunk_open_cleanup(chunk,cleanup_depth);

	return -1;

    }
   
    /*
     * We reuse the original resource handle after an mc_clone
     */

    chunk->path[chunk->cur_path]->sisl.resrc_handle = old_resrc_handle;




    /*
     * Free resources for the old context and AFU.
     */
    rc = mc_unregister(old_mc_handle);
 
    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_unregister after clone failed with rc = %d, errno = %d",
			    rc, errno);

    }
    
    rc = munmap(old_mmio_mmap,old_mmap_size);

    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"munmap failed with rc = %d errno = %d",
			    rc,errno);

    }

    cleanup_depth = 20;
    rc = close(old_fd);


    if (rc) {

	/* 
	 * If any of the above operations fail then
	 * exit out this code. 
	 */


	CBLK_TRACE_LOG_FILE(1,"close failed with rc = %d errno = %d",
			    rc,errno);

    }


#else 

    /*
     * This is the case when there is no Master Context
     */

#ifdef _COMMON_INTRPT_THREAD

    /*
     * If we are using a common interrupt thread per chunk,
     * and we are not using master context, then the fork will not
     * forked our interrupt thread. So we need to start it now.
     */

    if (cblk_start_common_intrpt_thread(chunk)) {


	CBLK_TRACE_LOG_FILE(1,"cblk_start_common_intrpt thread failed with errno= %d",
			    errno);

	    
	return -1;
    }
    
#endif  /* _COMMON_INTRPT_THREAD */

#endif /* _MASTER_CONTXT */
    return rc;

}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_free_mc_device_resources
 *                  
 * FUNCTION:  Free master context (MC) resources.  
 *          
 *
 * NOTES:  This routine assumes the caller has the chunk lock.
 *                                                                         
 *
 * CALLED BY:
 *    
 *
 * INTERNAL PROCEDURES CALLED:
 *      
 *     
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *     
 *      
 *
 * RETURNS:     
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_chunk_free_mc_device_resources(cflsh_chunk_t *chunk)
{
#ifdef  _MASTER_CONTXT   
    int rc = 0;
#endif /* _MASTER_CONTXT */


    if (chunk == NULL) {
	
	return;
    }

    if (chunk->path[chunk->cur_path] == NULL) {
	/*
	 * There is nothing to do here, exit
	 */

	return;

    }

#ifdef  _MASTER_CONTXT   

    /*
     * Free resources for this virtual lun.
     */

    if (chunk->path[chunk->cur_path]->afu->master.mc_handle == 0) {
	/*
	 * There is nothing to do here, exit
	 */

	return;

    }

    rc = mc_close(chunk->path[chunk->cur_path]->afu->master.mc_handle,
		  chunk->path[chunk->cur_path]->sisl.resrc_handle);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_close failed with rc = %d, errno = %d",
			    rc, errno);
	return;
    }

    /*
     * Free resources for this context and AFU.
     */
    rc = mc_unregister(chunk->path[chunk->cur_path]->afu->master.mc_handle);
 
    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_unregister failed with rc = %d, errno = %d",
			    rc, errno);
	return;
    }

    chunk->path[chunk->cur_path]->afu->master.mc_handle = 0;

#endif /* _MASTER_CONTXT */

    

    return;
}
 
/*
 * NAME:        cblk_process_nonafu_intrpt_cxl_events
 *
 * FUNCTION:    This routine process non-AFU interrupt CAPI
 *              events.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              ioasa    - I/O Adapter status response
 *
 * RETURNS:
 *             -1  - Fatal error
 *              0  - Ignore error (consider good completion)
 *              1  - Retry recommended
 *              
 */
cflash_cmd_err_t cblk_process_nonafu_intrpt_cxl_events(cflsh_chunk_t *chunk,int path_index,struct cxl_event *cxl_event)
{
    int rc = CFLASH_CMD_FATAL_ERR;
    uint64_t intrpt_status;

    /*
     * TODO: ?? More work is needed here. 
     */


    errno = EIO;

    if (chunk == NULL) {
	
	return rc;
    }

    if (chunk->path[path_index] == NULL) {
	/*
	 * There is nothing to do here, exit
	 */

	return rc;

    }

    switch (cxl_event->header.type) {
    case CXL_EVENT_RESERVED:
	chunk->stats.num_capi_reserved_errs++;
	CBLK_TRACE_LOG_FILE(1,"CXL_EVENT_RESERVED = size =  0x%x",
			    cxl_event->header.size);
	break;
    case CXL_EVENT_DATA_STORAGE:
	chunk->stats.num_capi_data_st_errs++;
	CBLK_TRACE_LOG_FILE(1,"CAPI_EVENT_DATA_STOARAGE addr = 0x%llx, dsisr = 0x%llx",
			    cxl_event->fault.addr,cxl_event->fault.dsisr);
	CBLK_TRACE_LOG_FILE(6,"contxt_handle = 0x%x",chunk->path[path_index]->afu->contxt_handle);
	CBLK_TRACE_LOG_FILE(6,"mmio_map = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio_mmap);
	CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio);
	CBLK_TRACE_LOG_FILE(6,"mmap_size = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmap_size);
	CBLK_TRACE_LOG_FILE(6,"hrrq_start = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_start);
	CBLK_TRACE_LOG_FILE(6,"hrrq_end = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_end);
	CBLK_TRACE_LOG_FILE(6,"cmd_start = 0x%llx",(uint64_t)chunk->cmd_start);
	CBLK_TRACE_LOG_FILE(6,"cmd_end = 0x%llx",(uint64_t)chunk->cmd_end);

	intrpt_status = CBLK_GET_INTRPT_STATUS(chunk,path_index);
	CBLK_TRACE_LOG_FILE(6,"intrpt_status = 0x%llx",intrpt_status);

	CBLK_TRACE_LOG_FILE(6,"num_active_cmds = 0x%x\n",chunk->num_active_cmds);
	



	break;
    case CXL_EVENT_AFU_ERROR:
	chunk->stats.num_capi_afu_errors++;
	CBLK_TRACE_LOG_FILE(1,"CXL_EVENT_AFU_ERROR error = 0x%llx, flags = 0x%x",
			    cxl_event->afu_error.error,cxl_event->afu_error.flags);
	
	cblk_notify_mc_err(chunk,path_index,0x400,cxl_event->afu_error.error,CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

	break;
	

    case CXL_EVENT_AFU_INTERRUPT:
	/*
	 * We should not see this, since the caller
	 * should have parsed these out.
	 */

	/* Fall thru */
    default:
	CBLK_TRACE_LOG_FILE(1,"Unknown CAPI EVENT type = %d, process_element = 0x%x",
			    cxl_event->header.type, cxl_event->header.process_element);
	

	cblk_notify_mc_err(chunk,path_index,0x401,cxl_event->header.type, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

    } /* switch */

    return rc;
}

/*
 * NAME:        cblk_read_os_specific_intrpt_event
 *
 * FUNCTION:    Reads an OS specific event for this interrupt
 *              
 *
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 *              cmd   - Cmd this routine will wait for completion.
 *
 * RETURNS:
 *              0 - Good completion, otherwise error.
 *              
 *              
 */
int cblk_read_os_specific_intrpt_event(cflsh_chunk_t *chunk, int path_index,cflsh_cmd_mgm_t **cmd,int *cmd_complete,
				       size_t *transfer_size,struct pollfd poll_list[])
{
    int rc = 0;
    int read_bytes = 0;
    int process_bytes  = 0;
    uint8_t read_buf[CAPI_FLASH_BLOCK_SIZE];
    struct cxl_event *cxl_event = (struct cxl_event *)read_buf;

 
#ifndef BLOCK_FILEMODE_ENABLED

    
    read_bytes = read(chunk->path[path_index]->afu->poll_fd,cxl_event,CAPI_FLASH_BLOCK_SIZE);

#else
    /*
     * For file mode fake an AFU interrupt
     */

    cxl_event->header.type = CXL_EVENT_AFU_INTERRUPT;

    read_bytes = sizeof(struct cxl_event);

    cxl_event->header.size = read_bytes;
    
    cxl_event->irq.irq = SISL_MSI_RRQ_UPDATED;

#endif /* BLOCK_FILEMODE_ENABLED */


    if (read_bytes < 0) {

	if (*cmd) {
	    CBLK_TRACE_LOG_FILE(5,"read event failed, with rc = %d errno = %d, cmd = 0x%llx, cmd_index = %d, lba = 0x%llx",
				read_bytes, errno,(uint64_t)*cmd, (*cmd)->index, (*cmd)->cmdi->lba);
	} else {
	    CBLK_TRACE_LOG_FILE(7,"read event failed, with rc = %d errno = %d, cmd = 0x%llx",
				read_bytes, errno,(uint64_t)*cmd);

	}

#ifdef _SKIP_POLL_CALL

	/*
	 * If we are not using the poll call,
	 * then since we are not blocking on the
	 * read, we need to delay here before
	 * re-reading again.
	 */

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	usleep(CAPI_POLL_IO_TIME_OUT * 1000);

	CFLASH_BLOCK_LOCK(chunk->lock);
#else


	if ((read_bytes == -1) && (errno == EAGAIN)) {

#ifdef _ERROR_INTR_MODE
	    /*
	     * When we poll with no time-out, we sometimes
	     * see read fail with EAGAIN. Do not consider
	     * this an error, but instead check the RRQ for 
	     * completions.
	     */

	    rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,
						  CFLSH_BLK_INTRPT_CMD_CMPLT,
						  cmd_complete, transfer_size);
	    return rc;

#else
	    /*
	     * Increment statistics
	     */

	    chunk->stats.num_capi_false_reads++;
#endif /* !_ERROR_INTR_MODE */
	}

#endif /* _SKIP_POLL_CALL */

	if ((read_bytes == -1) && (errno == EIO)) {

	    /*
	     * This most likely indicates the adapter
	     * is being reset.
	     */


	    chunk->stats.num_capi_adap_resets++;

	    cblk_notify_mc_err(chunk,path_index,0x402,0,CFLSH_BLK_NOTIFY_AFU_RESET,NULL);
	}

	return (-1);
    }


   
    if (read_bytes > CAPI_FLASH_BLOCK_SIZE) {
	
	/*
	 * If the number of read bytes exceeded the
	 * size of the buffer we supplied then truncate
	 * read_bytes to our buffer size.
	 */


	if (*cmd) {
	    CBLK_TRACE_LOG_FILE(1,"read event returned too large buffer size = %d errno = %d, cmd = 0x%llx, cmd_index = %d, lba = 0x%llx",
				read_bytes, errno,(uint64_t)cmd, (*cmd)->index, (*cmd)->cmdi->lba);
	} else {
	    CBLK_TRACE_LOG_FILE(1,"read event returned too large buffer size = %d errno = %d, cmd = 0x%llx",
				read_bytes, errno,(uint64_t)*cmd);

	}
	read_bytes = CAPI_FLASH_BLOCK_SIZE;
    }

    while (read_bytes > process_bytes) {

	/*
	 * The returned read data will have
	 * cxl event types. Unfortunately they
	 * are not using the common struct cxl_event
	 * structure for all in terms of size. Thus
	 * we need to read the header (common
	 * for all) and from the header's size
	 * field determine the size of the read
	 * entry.
	 */



	CBLK_TRACE_LOG_FILE(7,"cxl_event type = %d, size = %d",
			    cxl_event->header.type,cxl_event->header.size);


	if (cxl_event->header.size == 0) {


	    CBLK_TRACE_LOG_FILE(1,"cxl_event type = %d, invalid size = %d",
			    cxl_event->header.type,cxl_event->header.size);

	    errno = 5;
	    
	    return (-1);
	}


	process_bytes += cxl_event->header.size;


	if (cxl_event->header.type == CXL_EVENT_AFU_INTERRUPT) {


	    chunk->stats.num_capi_afu_intrpts++;

	    rc = CBLK_PROCESS_ADAP_INTRPT(chunk,cmd,(int)cxl_event->irq.irq,cmd_complete,transfer_size);
	} else {


	    rc = cblk_process_nonafu_intrpt_cxl_events(chunk,path_index,cxl_event);
	}

	cxl_event = (struct cxl_event *)(((char*)cxl_event) + cxl_event->header.size);

    }

    /*
     * TODO: ?? Currently we are just returning the last rc seen,
     *       Is this the corect choice.
     */


    return rc;

}


/*
 * NAME:        cblk_check_os_adap_err
 *
 * FUNCTION:    Inform adapter driver that it needs to check if this
 *              is a fatal error that requires a reset.
 *              
 *
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 * RETURNS:
 *              status
 *              
 *              
 */
cflash_block_check_os_status_t cblk_check_os_adap_err(cflsh_chunk_t *chunk,int path_index)
{
    int rc = 0;
    cflash_block_check_os_status_t status = CFLSH_BLK_CHK_OS_NO_RESET;

    
    chunk->stats.num_capi_adap_chck_err++;
    /*
     * TODO:?? This ioctl has been removed for now.
     */
    //rc = ioctl(chunk->fd,CXL_IOCTL_CHECK_ERROR,NULL);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"IOCL_CHECK_ERROR failed with rc = %d, errno = %d\n",
			    rc,errno);

	
    }

    cblk_notify_mc_err(chunk,path_index,0x403,0,CFLSH_BLK_NOTIFY_AFU_FREEZE,NULL);

    return status;
}

/*
 * NAME:        cblk_notify_mc_err
 *
 * FUNCTION:    Inform Master Context (MC) of this error.
 *              
 *
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 * RETURNS:
 *              None
 *              
 *              
 */
void cblk_notify_mc_err(cflsh_chunk_t *chunk,  int path_index,int error_num, uint64_t out_rc,
			cflash_block_notify_reason_t reason, 
			  cflsh_cmd_mgm_t *cmd)
{
#ifdef  _MASTER_CONTXT
    int rc = 0;
    mc_notify_t notify;

    if (chunk == NULL) {
	
	return;
    }

    if (chunk->path[path_index] == NULL) {
	/*
	 * There is nothing to do here, exit
	 */

	return;

    }

    bzero(&notify,sizeof(notify));


    switch (reason) {

    case CFLSH_BLK_NOTIFY_TIMEOUT:
	notify.event = MC_NOTIFY_CMD_TIMEOUT;
	notify.cmd_timeout.res_hndl = chunk->path[path_index]->sisl.resrc_handle;
	break;
    case CFLSH_BLK_NOTIFY_AFU_FREEZE:
	notify.event = MC_NOTIFY_AFU_EEH;
	break;
    case CFLSH_BLK_NOTIFY_AFU_ERROR:
	notify.event = MC_NOTIFY_AFU_ERR;
	break;
    case CFLSH_BLK_NOTIFY_AFU_RESET:
	notify.event = MC_NOTIFY_AFU_RST;
	break;
    case CFLSH_BLK_NOTIFY_SCSI_CC_ERR:
	notify.event = MC_NOTIFY_SCSI_SENSE;
	notify.scsi_sense.res_hndl = chunk->path[path_index]->sisl.resrc_handle;
	if (cmd) {

	    CBLK_COPY_ADAP_CMD_RESP(chunk,cmd,notify.scsi_sense.data,
				    MIN(sizeof(notify.scsi_sense.data),SISL_SENSE_DATA_LEN));
	}
	break;
    default:
	CBLK_TRACE_LOG_FILE(1,"Unknown reason %d\n",
			    reason);
	return;
    }
	
    rc = mc_notify(chunk->path[path_index]->afu->master.mc_handle,&notify);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"mc_notify failed with rc = %d, errno = %d\n",
			    rc,errno);

    }

#endif /* _MASTER_CONTXT */

    return;
}



/*
 * NAME:        cblk_verify_mc_lun
 *
 * FUNCTION:    Request MC to verify lun.
 *              
 *
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 * RETURNS:
 *              None
 *              
 *              
 */
int cblk_verify_mc_lun(cflsh_chunk_t *chunk,  cflash_block_notify_reason_t reason, 
			  cflsh_cmd_mgm_t *cmd, 
			  struct request_sense_data *sense_data)
{

    /*
     * For user space MC, this is just a notification
     */


    cblk_notify_mc_err(chunk,chunk->cur_path,0x404,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);
    return 0;
}
