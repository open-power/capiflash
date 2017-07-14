/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_kern_mc.c $                            */
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
/* ----------------------------------------------------------------------------
 *
 * This file contains the linux specific code for the block library.
 * For other OSes, this file should not be linked in and instead replaced
 * with the analogous OS specific file.  
 *     
 * ----------------------------------------------------------------------------
 */ 


#define CFLSH_BLK_FILENUM 0x0500
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"
#include "cflash_block_protos.h"
#ifdef _AIX
#include <sys/devinfo.h>
#include <procinfo.h>
#include <sys/scdisk_capi.h>
#include "cflash_block_aix.h"

typedef struct dk_capi_attach dk_capi_attach_t;
typedef struct dk_capi_detach dk_capi_detach_t;
typedef struct dk_capi_udirect dk_capi_udirect_t;
typedef struct dk_capi_uvirtual dk_capi_uvirtual_t;
typedef struct dk_capi_resize dk_capi_resize_t;
typedef struct dk_capi_release dk_capi_release_t;
typedef struct dk_capi_exceptions dk_capi_exceptions_t;
typedef struct dk_capi_log dk_capi_log_t;
typedef struct dk_capi_verify dk_capi_verify_t;
typedef struct dk_capi_recover_context dk_capi_recover_context_t; 
#else 
#include <cxl.h>
#include <libudev.h>

#include <scsi/cxlflash_ioctl.h>
#ifdef DK_CXLFLASH_CONTEXT_SQ_CMD_MODE
#ident "$Debug: SQ_CMD_MODE $"
#endif
#ifndef DK_CAPI_ATTACH 
/*
 * Create common set of defines/types across
 * OSes to improve code readibility.
 */
#define DK_CAPI_ATTACH           DK_CXLFLASH_ATTACH
#define DK_CAPI_DETACH           DK_CXLFLASH_DETACH
#define DK_CAPI_USER_DIRECT      DK_CXLFLASH_USER_DIRECT
#define DK_CAPI_USER_VIRTUAL     DK_CXLFLASH_USER_VIRTUAL
#define DK_CAPI_VLUN_RESIZE      DK_CXLFLASH_VLUN_RESIZE
#define DK_CAPI_VLUN_CLONE       DK_CXLFLASH_VLUN_CLONE
#define DK_CAPI_RELEASE          DK_CXLFLASH_RELEASE
#define DK_CAPI_RECOVER_CTX      DK_CXLFLASH_RECOVER_AFU
#define DK_CAPI_VERIFY           DK_CXLFLASH_VERIFY

typedef struct dk_cxlflash_attach dk_capi_attach_t;
typedef struct dk_cxlflash_detach dk_capi_detach_t;
typedef struct dk_cxlflash_udirect dk_capi_udirect_t;
typedef struct dk_cxlflash_uvirtual dk_capi_uvirtual_t;
typedef struct dk_cxlflash_resize dk_capi_resize_t;
typedef struct dk_cxlflash_clone dk_capi_clone_t;
typedef struct dk_cxlflash_release dk_capi_release_t;
typedef struct dk_cxlflash_exceptions dk_capi_exceptions_t;
typedef struct dk_cxlflash_verify dk_capi_verify_t;
typedef struct dk_cxlflash_recover_afu dk_capi_recover_context_t;

#define return_flags   hdr.return_flags

#define DK_RF_REATTACHED DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET

#endif /* DK_CAPI_ATTACH  */

#endif /* _AIX */


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

#ifdef _AIX
    if (arch_type) {

	/*
	 * If architecture type is set, then 
	 * evaluate it.
	 */

	switch (arch_type) {
	  case DK_ARCH_SISLITE:
	      /*
	       * Original SISLITE (default) type
	       */
	    chunk_type = CFLASH_BLK_CHUNK_SIS_LITE;
	    break;
#ifdef DK_ARCH_SISLITE_SQ
	  case DK_ARCH_SISLITE_SQ:
	      /*
	       * Sislite type that uses submission
	       * queue (SQ).
	       */
	    chunk_type = CFLASH_BLK_CHUNK_SIS_LITE_SQ;
	    break;
#endif /*  DK_ARCH_SISLITE_SQ */

	  default:
	    return CFLASH_BLK_CHUNK_NONE;
	}
	

	return chunk_type;
    }
#endif
    
    /*
     * For now we only support one chunk type:
     * the SIS lite type.
     */

    chunk_type = CFLASH_BLK_CHUNK_SIS_LITE;

    return chunk_type;
}

#ifndef _AIX

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_extract_udid_from_vpd83_file
 *                  
 * FUNCTION:  Parse the inquiry page 83 file in sysfs
 *            
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
char *cblk_extract_udid_from_vpd83_file(char *vpd_pg83_filename)
{


#define CFLASH_BLOCK_BUF_SIZE   256
#define CFLASH_WWID_SIZE   256

    FILE *fp;
    char buf[CFLASH_BLOCK_BUF_SIZE];
    char *udid = NULL;
    char wwid[CFLASH_WWID_SIZE];
    int length;
    int rc;


    CBLK_TRACE_LOG_FILE(9,"vpd_pg83 name = %s",vpd_pg83_filename);



    /* 
     * The data read out of the vpd_pg83 file is the
     * inquiry data of page 0x83, which is the
     * Device Identification VPD page.
     */

    if ((fp = fopen(vpd_pg83_filename,"r")) == NULL) {

	CBLK_TRACE_LOG_FILE(1,"failed to open = %s",
			    vpd_pg83_filename);
	return udid;
    }


    length = fread(buf,1,CFLASH_BLOCK_BUF_SIZE,fp);

    if (length < sizeof(struct inqry_pg83_data)) {



	CBLK_TRACE_LOG_FILE(1,"failed to read page 0x83 header, read %d bytes, but expected *%d bytes, errno = %d",
			    length,sizeof(struct inqry_pg83_data),errno);

	fclose(fp);

	return udid;
    }

    bzero(wwid,CFLASH_WWID_SIZE);


    rc = cflash_process_scsi_inquiry_dev_id_page(buf,length,wwid);

    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"failed to parse page 0x83 with rc = %d, and errno = %d",
			    rc,errno);
    } else {


	udid = strdup(wwid);
    }

    fclose (fp);
    return udid;


}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_find_other_paths_to_device
 *                  
 * FUNCTION:  Walk all cflash disks in the same and looks for those
 *            that have the same udid (universal device ID).
 *            
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
int cblk_find_other_paths_to_device(char *subsystem_name, char *orig_udid, 
				    char *cur_devname, char *dev_name_list[],
				    int size_devname_list)
{

    struct udev *udev_lib;
    struct udev_device *device, *parent;
    struct udev_enumerate *device_enumerate;
    struct udev_list_entry *device_list, *device_entry;
    const char *device_mode = NULL;
    char *parent_name = NULL;
    const char *path = NULL;
    char vpd_pg83_filename[PATH_MAX];
    char *udid = NULL;
    char *devname = NULL;
    int num_matches_found = 0;


    CBLK_TRACE_LOG_FILE(9,"subsystem_path = %s, cur_devname = %s",subsystem_name,cur_devname);
	
	
    /*
     * Extract cur_devname with absolute path removed
     */

    cur_devname = rindex(cur_devname,'/');

    cur_devname++;


    udev_lib = udev_new();

    if (udev_lib == NULL) {
	CBLK_TRACE_LOG_FILE(1,"udev_new failed with errno = %d",errno);


	return 0;
	
    }


    device_enumerate = udev_enumerate_new(udev_lib);

    if (device_enumerate == NULL)  {

	CBLK_TRACE_LOG_FILE(1,"udev_enumerate_new failed with errno = %d",errno);


	return 0;
	
    }


    udev_enumerate_add_match_subsystem(device_enumerate,subsystem_name);

    udev_enumerate_scan_devices(device_enumerate);

    device_list = udev_enumerate_get_list_entry(device_enumerate);


    for (device_entry = device_list; device_entry != NULL; device_entry = udev_list_entry_get_next(device_entry)) {

	path = udev_list_entry_get_name(device_entry);

	if (path == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"udev_list_entry_get_name failed with errno = %d",errno);
	    continue;
	}
	

	CBLK_TRACE_LOG_FILE(9,"path = %s",path);
	
	
	/*
	 * Extract filename with absolute path removed
	 */

	devname = rindex(path,'/');
	devname++;

	if (!(strcmp(devname,cur_devname))) {

	    /*
	     * Skip over our device name
	     */

	    continue;
	}

	device = udev_device_new_from_syspath(udev_lib,path);

	if (device == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"udev_device_new_from_subsystem_syspath failed with errno = %d",errno);

	    continue;

	}

	device_mode = udev_device_get_sysattr_value(device,"device/mode");


	if (device_mode == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"no mode for this device ");


	    continue;

	} else {


	    if (strcmp(device_mode,"superpipe")) {

		CBLK_TRACE_LOG_FILE(1,"Device is not in superpipe mode = %s",device_mode);

		continue;
	    }

	    CBLK_TRACE_LOG_FILE(9,"disk in superpipe mode found");

	    parent =  udev_device_get_parent(device);

	    if (parent == NULL) {

		CBLK_TRACE_LOG_FILE(1,"udev_device_get_parent failed with errno = %d",errno);

		continue;;

	    }

	    parent_name = (char *)udev_device_get_devpath(parent);
    
	    CBLK_TRACE_LOG_FILE(9,"parent name = %s",parent_name);

	    sprintf(vpd_pg83_filename,"/sys%s/vpd_pg83",parent_name);

	    udid = cblk_extract_udid_from_vpd83_file(vpd_pg83_filename);

	    if (!(strcmp(udid,orig_udid))) {


		devname = rindex(path,'/');
		devname++;

		CBLK_TRACE_LOG_FILE(9,"found device %s with same udid = %s",devname,vpd_pg83_filename);


		if (num_matches_found < size_devname_list) {

		    dev_name_list[num_matches_found] = malloc(PATH_MAX);

		    if (dev_name_list[num_matches_found] == NULL) {
			CBLK_TRACE_LOG_FILE(1,"malloc for dev_name = %s on %d entry failed with errno = %d",
					    devname,num_matches_found,errno);
			break;
		    }

		    bzero(dev_name_list[num_matches_found],PATH_MAX);

		    sprintf(dev_name_list[num_matches_found],"/dev/%s",devname);

		    
		} else {

		    /*
		     * We have exceeded the size of
		     * this list
		     */

		    break;
		}

		num_matches_found++;

		
	       
	    }

	    
	}
	
	
    }

    CBLK_TRACE_LOG_FILE(9,"num_matches_found = %d",num_matches_found);

    return num_matches_found;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_find_full_parent_name
 *                  
 * FUNCTION:  Find parent name, that also includes child name portion
 *            for this device.
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
char *cblk_find_full_parent_name(char *device_name)
{

    char *parent_name = NULL;
    const char *device_mode = NULL;
    char *subsystem_name = NULL;
    char *devname = NULL;


    struct udev *udev_lib;
    struct udev_device *device, *parent;


    udev_lib = udev_new();

    if (udev_lib == NULL) {
	CBLK_TRACE_LOG_FILE(1,"udev_new failed with errno = %d",errno);


	return parent_name;
	
    }



    /*
     * Extract filename with absolute path removed
     */

    devname = rindex(device_name,'/');

    if (devname == NULL) {

	devname = device_name;
    } else {

	devname++;

	if (devname == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"invalid device name = %s",devname);


	    return parent_name;
	}


    }

    CBLK_TRACE_LOG_FILE(9,"device name = %s",devname);

    if (!(strncmp(devname,"sd",2))) {

	subsystem_name = "block";

    } else if (!(strncmp(devname,"sg",2))) {

	subsystem_name = "scsi_generic";
    } else {

	CBLK_TRACE_LOG_FILE(1,"invalid device name = %s",devname);


	return parent_name;
    }


    CBLK_TRACE_LOG_FILE(9,"subsystem name = %s",subsystem_name);

    device = udev_device_new_from_subsystem_sysname(udev_lib,subsystem_name,devname);

    if (device == NULL) {

	CBLK_TRACE_LOG_FILE(1,"udev_device_new_from_subsystem_sysname failed with errno = %d",errno);

	return parent_name;

    }

    CBLK_TRACE_LOG_FILE(9,"device found for subsystem name = %s",subsystem_name);

    device_mode = udev_device_get_sysattr_value(device,"device/mode");


    if (device_mode == NULL) {

	CBLK_TRACE_LOG_FILE(1,"no mode for this device ");


	return NULL;

    } else {


	if (strcmp(device_mode,"superpipe")) {

	    CBLK_TRACE_LOG_FILE(1,"Device is not in superpipe mode = %s",device_mode);

	    return NULL;
	}
    }

    CBLK_TRACE_LOG_FILE(9,"device_mode found for subsystem name = %s",subsystem_name);

    parent =  udev_device_get_parent(device);

    if (parent == NULL) {

	CBLK_TRACE_LOG_FILE(1,"udev_device_get_parent failed with errno = %d",errno);

	return parent_name;

    }


    CBLK_TRACE_LOG_FILE(9,"parent found for subsystem name = %s",subsystem_name);

    parent_name = (char *)udev_device_get_devpath(parent);


    CBLK_TRACE_LOG_FILE(9,"parent name = %s",parent_name);

    /* udev is setting errno to non-zero, but there is no real error */
    errno=0;
    return (parent_name);

}
#endif /* !_AIX */


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

#ifdef _AIX
char *cblk_find_parent_dev(char *device_name)
{

    return NULL;
}
#else

char *cblk_find_parent_dev(char *device_name)
{
    char *parent_name = NULL;
    char *child_part = NULL;




    parent_name = cblk_find_full_parent_name(device_name);

    if (parent_name == NULL) {

	return parent_name;
    }



    /*
     * This parent name string will actually have sysfs directories
     * associated with the connection information of the child. We
     * need to get the base name of parent device that is not associated with
     * child device.  Find child portion of parent name string
     * and insert null terminator there to remove it.
     */

    child_part = strstr(parent_name,"/host");


    if (child_part) {


	child_part[0] = '\0';
    }

    return parent_name;
}

#endif /* !AIX */




/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_get_udid
 *                  
 * FUNCTION:  Get device's unique identifier
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

#ifdef _AIX
char *cblk_get_udid(char *device_name)
{

    return NULL;
}
#else

char *cblk_get_udid(char *device_name)
{
    char vpd_pg83_filename[PATH_MAX];
    char *udid = NULL;
    char *parent_name = NULL;


    parent_name = cblk_find_full_parent_name(device_name);


    sprintf(vpd_pg83_filename,"/sys%s/vpd_pg83",parent_name);

    udid = cblk_extract_udid_from_vpd83_file(vpd_pg83_filename);


    return udid;
}

#endif /* !AIX */



/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_attach_path
 *                  
 * FUNCTION:  Attaches the current process to a chunk
 *            for a specific adapter path
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
 *              0        - Success
 *              nonzero  - Failure   
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_chunk_attach_path(cflsh_chunk_t *chunk, int path_index,int mode, 
			    dev64_t adap_devno,
			    int *cleanup_depth, int assign_path, 
			    cflsh_afu_in_use_t in_use)
{

    int rc = 0;
    dk_capi_attach_t disk_attach;
#ifdef _MASTER_CONTXT   
    uint32_t block_size = 0;

#endif /* _MASTER_CONTXT */



    bzero(&disk_attach,sizeof(disk_attach));

#ifndef BLOCK_FILEMODE_ENABLED

#ifdef _AIX

    if ((assign_path) && (!in_use)) {

	disk_attach.flags |= DK_AF_ASSIGN_AFU;

	//TODO: ?? This can lead to muliple paths with the same adap_devno. Is that a problem?

    } else {
	disk_attach.devno = chunk->path[path_index]->afu->adap_devno;
    }
    disk_attach.num_exceptions = CFLASH_BLOCK_EXCEP_QDEPTH; 


    disk_attach.num_interrupts = CBLK_GET_NUM_INTERRUPTS(chunk,path_index);

    if (in_use) {
	disk_attach.flags |= DK_AF_REUSE_CTX;
	disk_attach.ctx_token = chunk->path[path_index]->afu->contxt_id;
	CBLK_TRACE_LOG_FILE(6,"Reuse contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);
    }

#else 

    // TODO:?? What do we do for Linux for two paths on same AFU?
    

    disk_attach.num_interrupts = CBLK_GET_NUM_INTERRUPTS(chunk,path_index);

#ifdef DK_CXLFLASH_ATTACH
    disk_attach.hdr.flags = mode & O_ACCMODE;                                  
#else
    disk_attach.flags = mode & O_ACCMODE;
#endif

    if (in_use) {
	disk_attach.context_id = chunk->path[path_index]->afu->contxt_id;
#ifdef DK_CXLFLASH_ATTACH
	disk_attach.hdr.flags |= DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#else
	disk_attach.flags |= DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
	CBLK_TRACE_LOG_FILE(6,"Reuse contxt_id = 0x%x",chunk->path[path_index]->afu->contxt_id);
    }

#endif /* !_AIX */


    // TODO:?? Is this needed disk_attach.flags = CXL_START_WORK_NUM_IRQS;
    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_ATTACH,&disk_attach);
    
    if (rc) {


	
	CBLK_TRACE_LOG_FILE(1,"Unable to attach errno = %d, return_flags = 0x%llx, num_active_chunks = 0x%x",
			    errno,disk_attach.return_flags,cflsh_blk.num_active_chunks);




	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	cblk_release_path(chunk,(chunk->path[path_index]));

	chunk->path[path_index] = NULL;
	return -1;
	
    }    



#ifdef  _MASTER_CONTXT  

    block_size = disk_attach.block_size;

	
    CBLK_TRACE_LOG_FILE(5,"block_size = %d, flags = 0x%llx",
			block_size, disk_attach.return_flags);



#ifndef _AIX

    /*
     * For AIX we determine block size from IOCINFO, 
     * Thus we should just verify here for all paths
     * that the block size returned is consistent.
     */

    if (path_index == 0) {

    
	/*
	 * Only save block size on first path.
	 */


	if (block_size) {
	    chunk->blk_size_mult = CAPI_FLASH_BLOCK_SIZE/block_size;
	} else {
	    chunk->blk_size_mult = 8;
	}

    } else {
#endif

	/*
	 * Verify block size is the same for this path
	 */

	
	if (block_size) {

	    if (chunk->blk_size_mult != CAPI_FLASH_BLOCK_SIZE/block_size) {

		return -1;
	    }
	    
	} else {

	    if (chunk->blk_size_mult != 8) {

		cblk_release_path(chunk,(chunk->path[path_index]));

		chunk->path[path_index] = NULL;
		return -1;
	    }
	}

#ifndef _AIX
    }
#endif




#endif /* _MASTER_CONTXT */ 


    if (in_use) {
#ifndef _AIX

	if (!(chunk->flags & CFLSH_CHNK_VLUN)) {
	    chunk->stats.max_transfer_size = disk_attach.max_xfer;
	} else {

	    /*
	     * Virtual luns only support a transfer size of 1 sector
	     */

	    chunk->stats.max_transfer_size = 1;
	}

	chunk->num_blocks_lun = disk_attach.last_lba + 1;

	CBLK_TRACE_LOG_FILE(5,"last_lba = 0x%llx",chunk->num_blocks_lun);
#endif 
	return rc;
    }
#ifdef _AIX
    chunk->path[path_index]->afu->adap_devno = disk_attach.devno;
#endif /* AIX */

    chunk->path[path_index]->afu->poll_fd = disk_attach.adap_fd;

    CBLK_TRACE_LOG_FILE(5,"chunk->fd = %d,adapter fd = %d, poll_fd = %d",
			chunk->fd,disk_attach.adap_fd,
			chunk->path[path_index]->afu->poll_fd);


#else

    chunk->path[path_index]->afu->poll_fd = chunk->fd;
    disk_attach.num_interrupts = 0;
    
#endif /* BLOCK_FILEMODE_ENABLED */
  
  
#ifndef  _MASTER_CONTXT  



    chunk->path[path_index]->afu->mmap_size = CAPI_FLASH_REG_SIZE;


#endif /* !_MASTER_CONTXT */    
    
#ifdef _AIX
    chunk->path[path_index]->afu->contxt_id = disk_attach.ctx_token;
    chunk->path[path_index]->afu->contxt_handle = 0xffffffff & disk_attach.ctx_token;
#else
    chunk->path[path_index]->afu->contxt_id = disk_attach.context_id;
    chunk->path[path_index]->afu->contxt_handle = 0xffffffff & disk_attach.context_id;

#ifdef DK_CXLFLASH_CONTEXT_SQ_CMD_MODE
    if (disk_attach.return_flags & DK_CXLFLASH_CONTEXT_SQ_CMD_MODE) {


	/*
	 * driver is indicating this AFU supports SQ. So switch
	 * the chunk/afu type to CFLASH_BLK_CHUNK_SIS_LITE_SQ)
	 */

	CBLK_TRACE_LOG_FILE(9,"set,DK_CXLFLASH_CONTEXT_SQ_CMD_MODE contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);

       
	
	if (cblk_update_path_type(chunk,chunk->path[path_index],CFLASH_BLK_CHUNK_SIS_LITE_SQ)) {


	    CBLK_TRACE_LOG_FILE(1,"failed to update chunk path type for SQ for contxt_id = 0x%llx",
				chunk->path[path_index]->afu->contxt_id);

       
	
	    cblk_release_path(chunk,(chunk->path[path_index]));

	    chunk->path[path_index] = NULL;

	    return -1;
	}

    }
#endif /*DK_CXLFLASH_CONTEXT_SQ_CMD_MODE  */


#ifdef DK_CXLFLASH_APP_CLOSE_ADAP_FD
    if (disk_attach.return_flags & DK_CXLFLASH_APP_CLOSE_ADAP_FD) {

	CBLK_TRACE_LOG_FILE(9,"DK_CXLFLASH_APP_CLOSE_ADAP_FD set, contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);
	chunk->path[path_index]->flags |= CFLSH_PATH_CLOSE_POLL_FD;

    }
#endif /* DK_CXLFLASH_APP_CLOSE_ADAP_FD */

#endif /* !AIX */




    CBLK_TRACE_LOG_FILE(6,"contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);


#ifdef _AIX


    chunk->path[path_index]->afu->mmio_mmap = disk_attach.mmio_start;

    chunk->path[path_index]->afu->mmio = chunk->path[path_index]->afu->mmio_mmap;

    chunk->path[path_index]->afu->mmap_size = disk_attach.mmio_size;
#else 


    *cleanup_depth = 35;

    chunk->path[path_index]->afu->mmap_size = disk_attach.mmio_size;

    if (!(chunk->flags & CFLSH_CHNK_VLUN)) {
	chunk->stats.max_transfer_size = disk_attach.max_xfer;
    } else {
	/*
	 * Virtual luns only support a transfer size of 1 sector
	 */

	chunk->stats.max_transfer_size = 1;
    }

    chunk->num_blocks_lun = disk_attach.last_lba + 1;

    CBLK_TRACE_LOG_FILE(5,"last_lba = 0x%llx",chunk->num_blocks_lun);

    
#endif   
    
    chunk->path[path_index]->flags |= CFLSH_PATH_ATTACH;

    return rc;
}



/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_attach_process_map_path
 *                  
 * FUNCTION:  Attaches the current process to a chunk and
 *            maps the MMIO space for a specific adapter path
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
 *              0        - Success
 *              nonzero  - Failure   
 *     
 * ----------------------------------------------------------------------------
 */ 
int  cblk_chunk_attach_process_map_path(cflsh_chunk_t *chunk, int path_index,int mode, 
					dev64_t adap_devno,  char *path_dev_name, int fd, int arch_type,
					int *cleanup_depth, int assign_path)
{

    int rc = 0;
    cflsh_block_chunk_type_t chunk_type;
    cflsh_path_t *path = NULL;
    cflsh_afu_in_use_t in_use = CFLASH_AFU_NOT_INUSE;
    int share = FALSE;
    int create_new_path = FALSE;


    if (chunk == NULL) {
	
	return (-1);
    }


    /*
     * TODO:?? Can we consolidate this code segment below
     *      with the routine cblk_chunk_reuse_attach_mpio.
     *      There seems to be considerable overlap and maybe
     *      this could lead to simplified code.
     */

    if (chunk->path[path_index] == NULL) {

	/*
	 * For the case of forking a process, the
	 * child's path structures will already exist,
	 * and we need to preserve some of the data.
	 * So only get a path if one is not allocated.
	 */

	chunk_type = cblk_get_chunk_type(chunk->dev_name,arch_type);

	/*
	 * We are going to attempt to create a
	 * new path here.
	 */

	create_new_path = TRUE;

	if (chunk->flags & CFLSH_CHNK_SHARED) {

	    share = TRUE;
	}

	path = cblk_get_path(chunk,adap_devno,path_dev_name,chunk_type,chunk->num_cmds,&in_use,share); 


	if (path == NULL) {

	    return (-1);
	}

	if (path->afu == NULL) {

	    return (-1);
	}


	chunk->path[path_index] = path;

	chunk->path[path_index]->fd = fd;

	path->path_index = path_index;
		
	if (in_use == CFLASH_AFU_MPIO_INUSE) {

	    /*
	     * If this AFU is already attached for this lun,
	     * via another path, then do not do another attach:
	     * Just reuse the AFU.
	     */

	    chunk->path[path_index]->flags |= CFLSH_PATH_ACT;

	} else if (in_use == CFLASH_AFU_SHARE_INUSE) {

	    /*
	     * If this AFU is already attached for a different
	     * lun, but not this lun, then we need to do another
	     * attach using the REATTACH flag if possible.
	     */

	    rc = cblk_chunk_attach_path(chunk,path_index,mode,adap_devno,cleanup_depth,assign_path,in_use);

	    if (!rc) {

		chunk->path[path_index]->flags |= CFLSH_PATH_ACT;
	    }

	    return rc;
	}


    } else if (!(chunk->flags & CFLSH_CHNK_VLUN) &&
	       (chunk->path[path_index]->afu_share_type == CFLASH_AFU_SHARE_INUSE)) {
	
	/*
	 * If this after a fork, and a physical lun
	 * that is using shared AFU. then do 
	 * a shared attach.
	 */

	rc = cblk_chunk_attach_path(chunk,path_index,mode,adap_devno,cleanup_depth,assign_path,CFLASH_AFU_SHARE_INUSE);

	if (!rc) {

	    chunk->path[path_index]->flags |= CFLSH_PATH_ACT;
	}

	return rc;

    }



    rc = cblk_chunk_attach_path(chunk,path_index,mode,adap_devno,cleanup_depth,assign_path,in_use);

    if (rc) {

	return rc;
    }

#ifndef _AIX


    chunk->path[path_index]->afu->mmio_mmap = mmap(NULL,chunk->path[path_index]->afu->mmap_size,PROT_READ|PROT_WRITE, MAP_SHARED,
			    chunk->path[path_index]->afu->poll_fd,0);
    
    if (chunk->path[path_index]->afu->mmio_mmap == MAP_FAILED) {
	CBLK_TRACE_LOG_FILE(1,"mmap of mmio space failed errno = %d, mmio_size = 0x%llx",
			    errno,(uint64_t)chunk->path[path_index]->afu->mmap_size);


	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	
	if (create_new_path) {

	    /*
	     * Only release path and its resources, if we created
	     * one. In the case where it may have already existed
	     * (such as after fork), we want to leave the path
	     * resources in place.
	     */

	    cblk_release_path(chunk,(chunk->path[path_index]));
	    chunk->path[path_index] = NULL;
	} else {

	    /*
	     * For a path that was not just created now in this
	     * routine, mark it has inactive.
	     */

	    chunk->path[path_index]->flags &= ~CFLSH_PATH_ACT;
	}

	return -1;
    } 

    chunk->path[path_index]->afu->mmio = chunk->path[path_index]->afu->mmio_mmap;
    

    if (fcntl(chunk->path[path_index]->afu->poll_fd,F_SETFL,O_NONBLOCK) == -1) {

	/*
	 * Ignore error for now
	 */

	CBLK_TRACE_LOG_FILE(1,"fcntl failed with  errno = %d",errno);
	
    }

#endif

    *cleanup_depth = 40;  

    CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx, path_index = %d, afu = %p",
			(uint64_t)chunk->path[path_index]->afu->mmio_mmap,path_index,chunk->path[path_index]->afu);

    /*
     * Set up response queue
     */
    

    bzero((void *)chunk->path[path_index]->afu->p_hrrq_start ,chunk->path[path_index]->afu->size_rrq);

    chunk->path[path_index]->afu->p_hrrq_curr = chunk->path[path_index]->afu->p_hrrq_start;

    /*
     * Since the host RRQ is
     * bzeroed. The toggle bit in the host
     * RRQ that initially indicates we
     * have a new RRQ will need to be 1.
     */


    chunk->path[path_index]->afu->toggle = 1;
        
    chunk->path[path_index]->afu->num_issued_cmds = 0;

    chunk->cmd_curr = chunk->cmd_start;

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {

	/*
	 * If this AFU is using a Submission Queue (SQ)
	 * then, reinitialize the SQ.
	 */ 
	bzero((void *)chunk->path[path_index]->afu->p_sq_start ,chunk->path[path_index]->afu->size_sq);

	chunk->path[path_index]->afu->p_sq_curr = chunk->path[path_index]->afu->p_sq_start;


    }

    if (CBLK_ADAP_SETUP(chunk,path_index)) {

	
	if (create_new_path) {

	    /*
	     * Only release path and its resources, if we created
	     * one. In the case where it may have already existed
	     * (such as after fork), we want to leave the path
	     * resources in place.
	     */

	    cblk_release_path(chunk,(chunk->path[path_index]));

	    chunk->path[path_index] = NULL;
	} else {

	    /*
	     * For a path that was not just created now in this
	     * routine, mark it has inactive.
	     */

	    chunk->path[path_index]->flags &= ~CFLSH_PATH_ACT;
	}
	return -1;
    }
    
    if (!rc) {

	chunk->path[path_index]->flags |= CFLSH_PATH_ACT;

	if (chunk->path[path_index]->afu->flags & CFLSH_AFU_UNNAP_SPT) {

	    /*
	     * If this AFU supports Unmap sectors
	     */
	    
	    if (path_index == 0) {

		/*
		 * If first path supports unmap then tentatively
		 * setup the chunk indicating this is true.
		 * if subsequent paths do not support Unmap, then
		 * we'll clear this chunk flag.
		 */

		chunk->flags |= CFLSH_CHNK_UNNAP_SPT;

	    } 

	} else if (chunk->flags & CFLSH_CHNK_UNNAP_SPT) {

	    /*
	     * If this AFU does not support unmap, but we
	     * currently have the chunk indicating it does,
	     * then clear that flag. We will only allow
	     * unmap operations if all paths/afus support it.
	     */


	    chunk->flags &= ~CFLSH_CHNK_UNNAP_SPT;
	}
    }
    return rc;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_reuse_attach_mpio
 *                  
 * FUNCTION:  Reuses an attach thru the same AFU
 *            for MPIO.  We can not do a reatach
 *            for the same disk and process.  If this is a different
 *            disk sharing the same AFU, then a reattach is needed.
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
int  cblk_chunk_reuse_attach_mpio(cflsh_chunk_t *chunk,int mode, dev64_t adap_devno,
				  char *path_dev_name, int fd, int arch_type,
				  int matching_path_index,int path_index, int *cleanup_depth)
{
    int rc = 0;
    cflsh_block_chunk_type_t chunk_type;
    cflsh_afu_in_use_t in_use;
    cflsh_path_t *path = NULL;
    int share = FALSE;



    chunk_type = cblk_get_chunk_type(chunk->dev_name,arch_type);


    if (chunk->flags & CFLSH_CHNK_SHARED) {

	share = TRUE;
    }

    path = cblk_get_path(chunk,adap_devno,path_dev_name,chunk_type,chunk->num_cmds,&in_use,share); 

    if (path == NULL) {
			    
	return (-1);
    }
			
    if (path->afu == NULL) {
			    
	return (-1);
    }
			
    chunk->path[path_index] = path;

    chunk->path[path_index]->fd = fd;
			
    path->path_index = path_index;
			
    if (in_use == CFLASH_AFU_MPIO_INUSE) {

	/*
	 * If this AFU is already attached for this lun,
	 * via another path, then do not do another attach:
	 * Just reuse the AFU.
	 */

	chunk->path[path_index]->flags |= CFLSH_PATH_ACT;

    } else if (in_use == CFLASH_AFU_SHARE_INUSE) {


	/*
	 * If this AFU is already attached for a different
	 * lun, but not this lun, then we need to do another
	 * attach using the REATTACH flag.
	 */


	rc = cblk_chunk_attach_path(chunk,path_index,mode,adap_devno,cleanup_depth,FALSE,in_use);
			    
	if (!rc) {
				
	    chunk->path[path_index]->flags |= CFLSH_PATH_ACT;
	}


    } else {

	if (share) {


	    CBLK_TRACE_LOG_FILE(1,"Invalid path_index Assigned path_id  = %d and path_index = %d",
				chunk->path[path_index]->path_id, path_index);

	    cblk_release_path(chunk,(chunk->path[path_index]));
	    chunk->path[path_index] = NULL;

	    return (-1);
	}
    }

    return rc;
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
    int path_index = 0;
    dev64_t adap_devno = 0;
    int i,j;                                /* general counter */
    int previous_path_found;
#ifdef _AIX
    struct devinfo iocinfo;
    int first_reserve_path = -1;
    int first_good_path = -1;
    int second_good_path = -1;

    struct cflash_paths {
	struct dk_capi_paths path;
	struct dk_capi_path_info  paths[CFLSH_BLK_MAX_NUM_PATHS-1];
    } disk_paths;
    
    struct dk_capi_path_info *path_info = NULL; 
    uint32_t block_size = 0;
    dev64_t  prim_path_devno;
    int  prim_path_id = -1;
    cflsh_block_chunk_type_t chunk_type;
#else
    char *path_dev_list[CFLSH_BLK_MAX_NUM_PATHS];
    int num_paths = 0;
    char *afu_name = NULL;
    int open_flags;
    int fd;

#endif /* _AIX */

    int rc = 0;
    int assign_afu = FALSE;

    
    if (chunk == NULL) {
	
	return (-1);
    }
 
#ifdef _AIX
    bzero(&iocinfo,sizeof(iocinfo));

    bzero(&disk_paths,sizeof(disk_paths));
    disk_paths.path.path_count = CFLSH_BLK_MAX_NUM_PATHS;

#endif /* AIX */

    if (chunk->udid == NULL) {
	chunk->udid = cblk_get_udid(chunk->dev_name);
    }

    CBLK_TRACE_LOG_FILE(9,"chunk->udid = %s",chunk->udid);


    // Set cur_path to path 0 in the chunk;

    chunk->cur_path = 0;


#ifndef BLOCK_FILEMODE_ENABLED

#ifdef _AIX

    rc = ioctl(chunk->fd,IOCINFO,&iocinfo);

    
    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"Iocinfo failed errno = %d",errno);

	/* 
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	
	return -1;
	
    }  

    if (iocinfo.devtype != DD_SCDISK) {

	CBLK_TRACE_LOG_FILE(1,"Invalid devtype = 0x%x",iocinfo.devtype);

    }


    if (!((iocinfo.flags &  DF_IVAL) &&
	(iocinfo.un.scdk64.flags & DF_CFLASH))) {
	
	CBLK_TRACE_LOG_FILE(1,"Not a CAPI flash disk");
	return -1;

    }

    block_size = iocinfo.un.scdk64.blksize;

	
    CBLK_TRACE_LOG_FILE(5,"block_size = %d",block_size);

    if (block_size) {
	chunk->blk_size_mult = CAPI_FLASH_BLOCK_SIZE/block_size;
    } else {
	chunk->blk_size_mult = 8;
	block_size = 512;
    }


    if (!(chunk->flags & CFLSH_CHNK_VLUN)) {
	chunk->stats.max_transfer_size = iocinfo.un.scdk64.lo_max_request;

	if (iocinfo.flags  & DF_LGDSK) {

	    chunk->stats.max_transfer_size |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
	
	}

	/*
	 * Convert max transfer size from bytes to blocks (sectors)
	 */

	chunk->stats.max_transfer_size /= block_size;

    } else {
	/*
	 * Virtual luns only support a transfer size of 1 sector
	 */

	chunk->stats.max_transfer_size = 1;
    }


    chunk->num_blocks_lun = iocinfo.un.scdk64.lo_numblks;


    if (iocinfo.flags  & DF_LGDSK) {

	chunk->num_blocks_lun |= (uint64_t)(iocinfo.un.scdk64.hi_numblks << 32);
	
    }
	
    // TODO: ?? Remove this, since we plan to only support 4K luns

    chunk->num_blocks_lun = chunk->num_blocks_lun/chunk->blk_size_mult;

    
    CBLK_TRACE_LOG_FILE(5,"last_lba = 0x%llx",chunk->num_blocks_lun);




    rc = ioctl(chunk->fd,DK_CAPI_QUERY_PATHS,&disk_paths);
    
    if (rc) {
	
	
	CBLK_TRACE_LOG_FILE(1,"Query paths errno = %d, return_flags = 0x%llx",
			    errno,disk_paths.path.return_flags);


	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	
	return -1;
	
    }    

    if (disk_paths.path.returned_path_count == 0) {

	CBLK_TRACE_LOG_FILE(1,"Unable to Query Path errno = %d",errno);


	/*
	 * Cleanup depth is set correctly on entry to this routine
	 * So it does not need to be adjusted for this failure
	 */
	
	return -1;
    }


    if (disk_paths.path.returned_path_count > CFLSH_BLK_MAX_NUM_PATHS) {


	CBLK_TRACE_LOG_FILE(5,"Returned more paths that we provided issued = %d returned = %d",
			    CFLSH_BLK_MAX_NUM_PATHS,disk_paths.path.returned_path_count);


	/*
	 * This call indicated they were more paths available then 
	 * what we provided. Attempt to continue with the returned
	 * proper subset of paths.
	 */

	disk_paths.path.returned_path_count = CFLSH_BLK_MAX_NUM_PATHS;
	
    }


    CBLK_TRACE_LOG_FILE(5,"number of paths found = %d",disk_paths.path.returned_path_count);


    /*
     * find first reserved path
     */

    path_info = disk_paths.path.path_info;

    for (i = 0;i<disk_paths.path.returned_path_count;i++) {

	if ((path_info[i].flags & DK_CPIF_RESERVED) &&
	    !(path_info[i].flags & DK_CPIF_FAILED)) {
	    first_reserve_path = i;

	    break;
	}
    }


    
    
    if (first_reserve_path > -1 ) {

	if (chunk->flags & CFLSH_CHNK_NO_RESRV) {

	    /*
	     * We have detected a path with reservations, but 
	     * we opened the disk with no reservations. This should
	     * not happen. So fail.
	     */

	    CBLK_TRACE_LOG_FILE(1,"path_info %d has a reservation, but there should not be any reserves",i);

	    /*
	     * Cleanup depth is set correctly on entry to this routine
	     * So it does not need to be adjusted for this failure
	     */
	

	    return -1;
	
	}


	if (chunk->flags & CFLSH_CHNK_MPIO_FO) {

	    /*
	     * We have detected a path with reservations, but 
	     * we opened the disk with MPIO.So fail.
	     */

	    CBLK_TRACE_LOG_FILE(1,"path_info %d has a reservation, but using MPIO",i);

	    /*
	     * Cleanup depth is set correctly on entry to this routine
	     * So it does not need to be adjusted for this failure
	     */
	

	    return -1;
	
	}



	/*
	 * Use the first path reservations
	 */
	adap_devno = path_info[first_reserve_path].devno;

	prim_path_id = path_info[first_reserve_path].path_id;

	CBLK_TRACE_LOG_FILE(6,"First reserve path_id  = %d and path_index = %d",
			    chunk->path[path_index]->path_id, path_index);
	
	
    } else {


	if ((chunk->flags & CFLSH_CHNK_MPIO_FO) &&
	    !(chunk->flags & CFLSH_CHNK_VLUN)) {


	    /*
	     * If we are using MPIO for physical luns, then attach and map all
	     * paths. First let the disk driver assign the primary path (with
	     * the hope it will load balance these assignments) which will have
	     * path_index of 0. Then attach to the remaining paths.
	     * 
	     * NOTE: For AIX which uses MPIO, the path's file descriptor
	     *       is identical to the chunk file descriptor, since the
	     *       the disk driver (and device) are multi-path aware.
	     */

	    if (cblk_chunk_attach_process_map_path(chunk,0,mode,0,chunk->dev_name,chunk->fd,0,cleanup_depth,TRUE)) {

		return -1;
	    }
	    



	    prim_path_devno = chunk->path[path_index]->afu->adap_devno;

	    
	
	    /*
	     * Find primary AFU selected for us. It
	     * may be associated with multiple paths.
	     * Thus we need to find all of them for this AFU.
	     * In the loop below, path_index indicates the current
	     * chunk->path[path_index] that is being processed/created.
	     * The primary path will will be path_index of 0.
	     */

	    for (i = 0;i<disk_paths.path.returned_path_count;i++) {

		if ((!(path_info[i].flags & DK_CPIF_FAILED)) &&
		    (path_info[i].devno == prim_path_devno)) {

		    /*
		     * If this is not a failed path, and it is 
		     * the one associated with the primary path, then
		     * save off the path information.
		     */

		    if (path_index > 0) {

			/*
			 * If path_index is greater than 0, then we
			 * have already chosen the primary path for this AFU 
			 * (path_index of 0 will always be primary path).
			 * However if this AFU has multiple ports (paths), then
			 * we would like to use those as the non-primary paths for
			 * MPIO.  
			 */

			if (chunk->path[path_index]) {

			    CBLK_TRACE_LOG_FILE(1,"Invalid path_index Assigned path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);

			    continue;
			}

			/*
			 * 
			 * NOTE: For AIX which uses MPIO, the path's file descriptor
			 *       is identical to the chunk file descriptor, since the
			 *       the disk driver (and device) are multi-path aware.
			 */

			if (cblk_chunk_reuse_attach_mpio(chunk,mode,path_info[i].devno,NULL,
							 chunk->fd,
							 path_info[i].architecture,
							 0,path_index,cleanup_depth)) {


			    continue;
			}

		    } else {

			/*
			 * Trace tentative primary path_id for debug purposes
			 */

			CBLK_TRACE_LOG_FILE(5,"Tentative Primary path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);
		    }


		    /*
		     * Since we had the driver assign this path, we need to determine
		     * its chunk type.
		     */

		    chunk_type = cblk_get_chunk_type(chunk->dev_name,path_info[i].architecture);

		    if (cblk_update_path_type(chunk,chunk->path[path_index],chunk_type)) {

			return -1;
		    }
		    
		    chunk->path[path_index]->path_id = path_info[i].path_id;

		    chunk->path[path_index]->path_id_mask = 1 << path_info[i].path_id;


		    /*
		     * Only one port is associated with this path
		     */

		    chunk->path[path_index]->num_ports = 1;

		    CBLK_TRACE_LOG_FILE(9,"For same (primary) AFU, assigned path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);

		    path_index++;
		    




		}
	    }

	    if (path_index == 0) {

		CBLK_TRACE_LOG_FILE(1,"Could not find selected path, num_paths = %d",
				    disk_paths.path.returned_path_count);
		return -1;
	    }

	    for (i = 0;i<disk_paths.path.returned_path_count;i++) {

		if ((!(path_info[i].flags & DK_CPIF_FAILED)) &&
		    (path_info[i].devno != prim_path_devno)) {

		    /*
		     * If this is not a failed path, and it is not
		     * associated with the primary path, then
		     * attach to this path. First we need to see if
		     * we are already attached to this AFU before attempting 
		     * an attach. 
		     */

		    previous_path_found = FALSE;
		    for (j=0;j< path_index; j++) {

			if (chunk->path[j]->afu->adap_devno == path_info[i].devno) {



			    /*
			     * 
			     * NOTE: For AIX which uses MPIO, the path's file descriptor
			     *       is identical to the chunk file descriptor, since the
			     *       the disk driver (and device) are multi-path aware.
			     */
			    
			    if (cblk_chunk_reuse_attach_mpio(chunk,mode,path_info[i].devno,NULL,
							     chunk->fd,
							     path_info[i].architecture,
							 j,path_index,cleanup_depth)) {


				break;
			    }



			    previous_path_found = TRUE;

			    break;
			}
		    }

		    if (!previous_path_found) {



			/*
			 * 
			 * NOTE: For AIX which uses MPIO, the path's file descriptor
			 *       is identical to the chunk file descriptor, since the
			 *       the disk driver (and device) are multi-path aware.
			 */
			if (cblk_chunk_attach_process_map_path(chunk,path_index,mode,path_info[i].devno,NULL,
							       chunk->fd,path_info[i].architecture,
							       cleanup_depth,FALSE)) {

			    break;
			}
	    
			chunk->path[path_index]->afu->adap_devno = path_info[i].devno;

		    }

		    chunk->path[path_index]->path_id = path_info[i].path_id;

		    chunk->path[path_index]->path_id_mask = 1 << path_info[i].path_id;


		    /*
		     * Only one port is associated with this path
		     */

		    chunk->path[path_index]->num_ports = 1;

		    CBLK_TRACE_LOG_FILE(9,"Attached path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);
		    path_index++;


		}
	    }


	    CBLK_TRACE_LOG_FILE(5,"rc = %d num_paths = %d",
				rc, chunk->num_paths);
	    return rc;
	
	} else {

	    /*
	     * We are not doing MPIO so let driver choose the path.
	     */

	    assign_afu = TRUE;
	    

	    if (chunk->flags & CFLSH_CHNK_SHARED) {

		/*
		 * If we are also sharing contexts, then pick the first non-failed
		 * path.  The subsequent code will use this non-failed path as
		 * an opportunity to share the context. If sharing is possible the assign_afu
		 * option will be overrdden in favor fo sharing. Otherwise if no sharing is 
		 * possible the disk driver will assign the path.
		 *
		 * There is no guarantee that this disk has its paths ordered the same
		 * as another disk on this AFU. Thus the opportunity to share a context
		 * will only occur if we happen to pick the same path. For now this
		 * seems acceptable. Perhaps in the future we may find reasons
		 * to increase this likelihood.
		 */

		for (i = 0;i<disk_paths.path.returned_path_count;i++) {
		    
		    if (!(path_info[i].flags & DK_CPIF_FAILED)) {
			
			first_good_path = i;
			break;
		    }
		}
		if (first_good_path > -1) {
		    adap_devno = path_info[first_good_path].devno;
		}
		
	    }

#ifdef _REMOVE
	    if (first_good_path > -1) {

		chunk->path[path_index]->afu->adap_devno = path_info[first_good_path].devno;

		chunk->path[path_index]->path_id_mask = 1 << path_info[first_good_path].path_id;

		if (chunk->flags & CFLSH_CHNK_VLUN) {
		    /*
		     * If we are using virtual luns, then see if
		     * there is another path thru this same adapter/AFU
		     * for this device. If so we can enable the AFU
		     * to use both.
		     */

		    for (i = 0;i <disk_paths.path.returned_path_count;i++) {

			if ((path_info[i].devno == chunk->path[path_index]->afu->adap_devno) &&
			    (i != first_good_path)) {

			    second_good_path = i;
			    break;
			}
		    }

		    if (second_good_path > -1) {

			/*
			 * If we found a second valid path on this same adapter,
			 * then add this path to the path id mask.
			 */

			chunk->path[path_index]->path_id_mask |= 1 << path_info[second_good_path].path_id;
		    }

		}

	    } else {


		CBLK_TRACE_LOG_FILE(1,"No good paths returned");

		/*
		 * Cleanup depth is set correctly on entry to this routine
		 * So it does not need to be adjusted for this failure
		 */
	

		return -1;

	    }
	    chunk->path[path_index]->path_id =  path_info[first_good_path].path_id;

#endif /* REMOVE */
	}

    }

#else

    if ((chunk->flags & CFLSH_CHNK_MPIO_FO) &&
	!(chunk->flags & CFLSH_CHNK_VLUN) &&
	(chunk->udid)) {


	/*
	 * If we are using MPIO for physical luns, and we have a 
	 * unique device ID (UDID), then attach and map all
	 * paths. First attach for the primary path.
	 * Then attach to the remaining paths.
	 *
	 * NOTE: For linux the primary path will have the same
	 *       file descriptor as the chunk. All other paths will
	 *       have different file descriptors.
	 */

	if (cblk_chunk_attach_process_map_path(chunk,0,mode,0,
					       chunk->dev_name,chunk->fd,0,
					       cleanup_depth,TRUE)) {


	    return -1;
	}

	/*
	 * Now find all remaining paths (special files) for this
	 * same lun.
	 */

	bzero(path_dev_list,CFLSH_BLK_MAX_NUM_PATHS);

	num_paths = cblk_find_other_paths_to_device("scsi_generic",chunk->udid,chunk->dev_name,
						  path_dev_list,CFLSH_BLK_MAX_NUM_PATHS);

	/*
	 * Advance to the next potential path index
	 */

	path_index = 1;
	for (i = 0; i < num_paths; i++) {

	    CBLK_TRACE_LOG_FILE(9,"path found %d = %s",i,path_dev_list[i]);


	    /* ?? Need support for other architectures */

	    /*
	     * ?? path file descriptor needs to be passed
	     * to the leaf routines to do the attach ioctls.
	     */

	    /*
	     * Attach to this path. First we need to see if
	     * we are already attached to this AFU before attempting 
	     * an attach. 
	     */

	    previous_path_found = FALSE;
	    for (j=0;j< path_index; j++) {

		afu_name = cblk_find_parent_dev(path_dev_list[i]);


		if (!strcmp(chunk->path[j]->afu->master_name,afu_name)) {


		    CBLK_TRACE_LOG_FILE(9,"reusing attach for  %s for path_index = %d",
					    path_dev_list[i],path_index);
			    
		    // ?? Need way to use same flags as original open

		    open_flags = O_RDWR | O_NONBLOCK;   

		    fd = open(path_dev_list[i],open_flags);

		    if (fd  < 0) {

			CBLK_TRACE_LOG_FILE(1,"Unable to open device %s for path_index = %d errno = %d",
					    path_dev_list[i],path_index,errno);

			continue;
		    }

		    if (cblk_chunk_reuse_attach_mpio(chunk,mode,0,path_dev_list[i],fd,CFLASH_BLK_CHUNK_SIS_LITE,
						     j,path_index,cleanup_depth)) {


			break;
		    }



		    previous_path_found = TRUE;

		    break;
		}
	    }

	    if (!previous_path_found) {

		/* 
		 * If no paths are using this same AFU, then attach now
		 */

		

		CBLK_TRACE_LOG_FILE(9,"opening/attaching for new AFU  %s for path_index = %d",
				    path_dev_list[i],path_index);

		// ?? Need way to use same flags as original open

		open_flags = O_RDWR | O_NONBLOCK;   

		fd = open(path_dev_list[i],open_flags);

		if (fd  < 0) {

		    CBLK_TRACE_LOG_FILE(1,"Unable to open device %s for path_index = %d errno = %d",
					path_dev_list[i],path_index,errno);

		    continue;
		}

		if (cblk_chunk_attach_process_map_path(chunk,path_index,mode,0,path_dev_list[i],
						       fd,0,
						       cleanup_depth,FALSE)) {

		    break;
		}
	    }


	    /*
	     * Only one port is associated with this path
	     */

	    chunk->path[path_index]->num_ports = 1;

	    CBLK_TRACE_LOG_FILE(9,"Attached path dev_name  = %s, path_index = %d",
				chunk->path[path_index]->dev_name,path_index);
	    path_index++;

	}


	
	CBLK_TRACE_LOG_FILE(5,"rc = %d num_paths = %d",
				rc, chunk->num_paths);
	return rc;
    }



#endif /* !_AIX */




    
#endif /* BLOCK_FILEMODE_ENABLED */
    
    if (cblk_chunk_attach_process_map_path(chunk,path_index,mode,adap_devno,chunk->dev_name,chunk->fd,0,cleanup_depth,assign_afu)) {

	return -1;
    }

#ifdef _AIX
    if (assign_afu) {


	/*
	 * Find primary path selected for us.
	 */

	prim_path_devno = chunk->path[path_index]->afu->adap_devno;
	
	if ((prim_path_id > -1) &&
	    !(chunk->flags & CFLSH_CHNK_VLUN)) {

	    /*
	     * We already know the path id for this
	     * path. Thus if this a physical lun, then
	     * save off the relevant information now.
	     */

	    chunk->path[path_index]->path_id = prim_path_id;

	    chunk->path[path_index]->path_id_mask |= 1 << prim_path_id;

	    if (chunk->path[path_index]->num_ports == 0) {


		chunk_type = cblk_get_chunk_type(chunk->dev_name,path_info[i].architecture);

		if (cblk_update_path_type(chunk,chunk->path[path_index],chunk_type)) {

		    return -1;
		}
	    }
		
	    chunk->path[path_index]->num_ports++;

	    CBLK_TRACE_LOG_FILE(9,"Assigned path_id  = %d and path_index = %d",
				chunk->path[path_index]->path_id, path_index);

	} else {

	    /*
	     * Find the associated path_id that we were assigned.
	     * In actuality we can only narrow the search to the
	     * adapter/AFU. For multiple paths from the same adapter/AFU
	     * we have no scheme yet to determine which of the multiple path_ids
	     * thru that adapter/AFU is correct. Thus we will just pick one
	     * now. When we do the USER_DIRECT we will ask the disk driver
	     * to assign a path from this adapter/AFU and then we'll adjust the
	     * the path_ids at that time (in case the one we choose now is
	     * not the one assigned).
	     */
	    for (i = 0;i<disk_paths.path.returned_path_count;i++) {

		if ((!(path_info[i].flags & DK_CPIF_FAILED)) &&
		    (path_info[i].devno == prim_path_devno)) {

		    /*
		     * If this is not a failed path, and it is 
		     * the one associated with the primary path, then
		     * save off the path information.
		     */

		    chunk->path[path_index]->path_id = path_info[i].path_id;

		    chunk->path[path_index]->path_id_mask |= 1 << path_info[i].path_id;


		    if (chunk->path[path_index]->num_ports == 0) {


			chunk_type = cblk_get_chunk_type(chunk->dev_name,path_info[i].architecture);

			if (cblk_update_path_type(chunk,chunk->path[path_index],chunk_type)) {

			    return -1;
			}
		    }

		    /*	
		     * For virtual luns determine how many ports
		     * on this AFU are associated with this path.
		     */
		    chunk->path[path_index]->num_ports++;

		    CBLK_TRACE_LOG_FILE(9,"Assigned path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);


		    if (chunk->flags & CFLSH_CHNK_VLUN) {

			/*
			 * Get all paths to get the full path_id mask set.
			 * The last path found will be the path_id set.
			 */

			continue;
		    } else {

			/*
			 * For physical mode only use the first found path_id
			 */

			break;
		    }

		}
	    }
	}
    } else {


	chunk->path[path_index]->path_id_mask = 1 << path_info[first_reserve_path].path_id;

	chunk->path[path_index]->path_id =  path_info[first_reserve_path].path_id;

    }
#else
    chunk->path[path_index]->num_ports++;

#endif /* AIX */


    return rc;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_detach_path
 *                  
 * FUNCTION:  Detaches the current process for the current adapter path
 *            
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
void  cblk_chunk_detach_path (cflsh_chunk_t *chunk, int path_index,int force)
{ 


    dk_capi_detach_t disk_detach;
    int rc;



    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_DETACH NULL path");
	return;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_DETACH NULL afu");
	return;
    }


    if (!(chunk->path[path_index]->flags & CFLSH_PATH_ACT)) {

	/*
	 * Path is not active. Just return.
	 */

	return;
    }

    
    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);
    
    if ((chunk->path[path_index]->flags & CFLSH_PATH_ATTACH) ||
	(force)) {


	/*
	 * Only detach on the paths, for which we did an attach
	 */
	
	if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
	
	    /*
	     * If path is in a halted state then fail 
	     * from this routine
	     */
	
	    CBLK_TRACE_LOG_FILE(5,"afu halted, failing detach path afu->flags = 0x%x",
				chunk->path[path_index]->afu->flags);
	    
	    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);
	    return;

	
	
	}


	/*
	 * Since we are reusing contexts, it is fine to detach 
	 * even if this is not last path to uses this AFU.
	 */


	bzero(&disk_detach,sizeof(disk_detach));

#ifdef _AIX
	disk_detach.devno = chunk->path[path_index]->afu->adap_devno;
#endif /* AIX */

#ifdef _AIX
	disk_detach.ctx_token = chunk->path[path_index]->afu->contxt_id;
#else
	disk_detach.context_id = chunk->path[path_index]->afu->contxt_id;
#endif /* !AIX */

	rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_DETACH,&disk_detach);
    

	if (rc) {
	
	    CBLK_TRACE_LOG_FILE(1,"DK_CAPI_DETACH failed with rc = %d, errno = %d, return_flags = 0x%llx, chunk_id = %d, chunk->flags = 0x%x",
				rc,errno,disk_detach.return_flags,chunk->index, chunk->flags,
				chunk->path[path_index]->afu->contxt_id);

	    CBLK_TRACE_LOG_FILE(1,"DK_CAPI_DETACH failed (cont) chunk->in_use = %d, chunk->dev_name = %s, path_index = %d,  contxt_id = 0x%llx",
				chunk->in_use, chunk->dev_name,path_index,
				chunk->path[path_index]->afu->contxt_id);
	
	
	}  else {


	
	    CBLK_TRACE_LOG_FILE(9,"DK_CAPI_DETACH success withchunk_id = %d, chunk->flags = 0x%x",
				chunk->index, chunk->flags,
				chunk->path[path_index]->afu->contxt_id);

	    CBLK_TRACE_LOG_FILE(8,"DK_CAPI_DETACH success (cont) chunk->in_use = %d, chunk->dev_name = %s, path_index = %d,  contxt_id = 0x%llx",
				chunk->in_use, chunk->dev_name,path_index,
				chunk->path[path_index]->afu->contxt_id);
	
	
	}

	if (chunk->path[path_index]->flags & CFLSH_PATH_CLOSE_POLL_FD) {

	    close(chunk->path[path_index]->afu->poll_fd);
	}


    }

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);


#ifndef _AIX


    if ((path_index) &&
	(chunk->path[path_index]->fd >= 0)) {

	/*
	 * The primary path index's (0),
	 * file descriptor is same as
	 * the chunk's file descriptor,
	 * Thus we do not close the primary
	 * path index's file descriptor 
	 * here.
	 */

	CBLK_TRACE_LOG_FILE(9,"closing = %s, path_index = %d",
			    chunk->path[path_index]->dev_name,path_index);
	
	close(chunk->path[path_index]->fd);

	chunk->path[path_index]->fd = -1;
    }
#endif /* !_AIX */


    chunk->path[path_index]->flags &= ~CFLSH_PATH_ACT;
    return;

}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_detach
 *                  
 * FUNCTION:  Detaches the current process.
 *            
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
    int i;

    for (i=0; i< chunk->num_paths;i++) {

	cblk_chunk_detach_path(chunk,i,force);
    }

    return;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_umap_path
 *                  
 * FUNCTION:  Unmaps the MMIO space for an adapter path
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
void  cblk_chunk_unmap_path (cflsh_chunk_t *chunk,int path_index,int force)
{ 



    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"NULL path");
	return;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"NULL afu");
	return;
    }


    /*
     * TODO:?? Logic is needed to ensure this does not pull the
     *         rug out from other chunks using this AFU.
     */

    if (!(chunk->path[path_index]->flags & CFLSH_PATH_ACT)) {

	/*
	 * Path is not active. Just return.
	 */

	return;
    }


    
    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if ((chunk->path[path_index]->afu->ref_count == 1) ||
	(force)) {


	/*
	 * Only unmap on the last entity to use this afu unless
	 * force is set.
	 */
	
	if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
	    
	    /*
	     * If path is in a halted state then fail 
	     * from this routine
	     */
	    
	
	    CBLK_TRACE_LOG_FILE(5,"afu halted, failing unmap path afu->flags = 0x%x",
			    chunk->path[path_index]->afu->flags);
	    
	    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);
	    return;
	    
	}


#ifndef _AIX
	if (chunk->path[path_index]->afu->mmap_size == 0) {

	    /*
	     * Nothing to unmap.
	     */

	    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);
	    return;
	}



	if (munmap(chunk->path[path_index]->afu->mmio_mmap,chunk->path[path_index]->afu->mmap_size)) {



	    /*
	     * Don't return here on error. Continue
	     * to close
	     */
	    CBLK_TRACE_LOG_FILE(2,"munmap failed with errno = %d",
				errno);
	}

#endif


	chunk->path[path_index]->afu->mmio = 0;
	chunk->path[path_index]->afu->mmio_mmap = 0;
	chunk->path[path_index]->afu->mmap_size = 0;

    }


    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_umap
 *                  
 * FUNCTION:  Unmaps the MMIO space.
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
void  cblk_chunk_unmap (cflsh_chunk_t *chunk, int force)
{ 

    int i;

    for (i=0; i< chunk->num_paths;i++) {

	cblk_chunk_unmap_path(chunk,i, force);
    }

}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_get_mc_phys_disk_resource_path
 *                  
 * FUNCTION:  Get master context (MC) resources for phyiscal
 *            disk for a specific path, which 
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
int  cblk_chunk_get_mc_phys_disk_resources_path(cflsh_chunk_t *chunk, 
						int path_index)
{
    int rc = 0;
    dk_capi_udirect_t disk_physical; 
#ifdef _AIX
    int i;
    uint32_t path_id_msk;
#endif




    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null path passed, path_index = %d",path_index);
	return -1;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null afu path passed, path_index = %d",path_index);
	return -1;
    }

    bzero(&disk_physical,sizeof(disk_physical));

#ifdef _AIX

    /*
     * For AIX physical need to do this ioctl per path
     */

    if ((path_index == 0) &&
	(chunk->flags & CFLSH_CHNK_NO_RESRV)) {


	/*
	 * If this is the primary path and we are not
	 * using resevations (If we are using reservations,
	 * then we should have already chosen the path id
	 * for the primary path), then let
	 * the driver assign the optimum path.
	 */

	disk_physical.flags = DK_UDF_ASSIGN_PATH;
    } else {
	disk_physical.path_id_mask = chunk->path[path_index]->path_id_mask;
    }


    disk_physical.devno = chunk->path[path_index]->afu->adap_devno;
    
    disk_physical.ctx_token = chunk->path[path_index]->afu->contxt_id;
#else
    disk_physical.context_id = chunk->path[path_index]->afu->contxt_id;
#endif /* !AIX */

	



    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_USER_DIRECT,&disk_physical);

    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_USER_DIRECT failed with errno = %d, return_flags = 0x%llx",
			    errno,disk_physical.return_flags);

	return -1;
    }

    chunk->path[path_index]->sisl.resrc_handle = 0xffffffff & disk_physical.rsrc_handle;

#ifdef _AIX
    if ((disk_physical.flags & DK_UDF_ASSIGN_PATH) &&
	(path_index == 0)) {
	
	/*
	 * If we asked the driver to assign the path,
	 * then save it off now. This should only be done
	 * for path_index of 0 (primary path).
	 */
	/*
	 * See if this path_id mask is being used for a different path 
	 * to this same AFU. If so then swap them now. 
	 * Thus the primary path will have this
	 * path_id_mask only.
	 */

	for (i = 1; i < chunk->num_paths; i++) {

	    if ((chunk->path[i]->afu->adap_devno == chunk->path[path_index]->afu->adap_devno) &&
		(disk_physical.path_id_mask == chunk->path[i]->path_id_mask)) {

		/*
		 * Exchange path_id_mask and path_id
		 */

		chunk->path[i]->path_id_mask = chunk->path[path_index]->path_id_mask;

		chunk->path[i]->path_id = chunk->path[path_index]->path_id;

		break;
	    }
	}

	chunk->path[path_index]->path_id_mask = disk_physical.path_id_mask;

	/*
	 * Get corresponding path_id from this
	 * path_id_mask
	 */

	i = 0;
	path_id_msk = chunk->path[path_index]->path_id_mask;
	while (path_id_msk > 1) {

	    path_id_msk = path_id_msk >> 1;
	    i++;
	}
	
	chunk->path[path_index]->path_id = i;

	CBLK_TRACE_LOG_FILE(5,"Primary path_id  = %d and path_index = %d",
					chunk->path[path_index]->path_id, path_index);

	chunk->stats.primary_path_id = chunk->path[path_index]->path_id;
	
    }
#endif /* !AIX */



    CBLK_TRACE_LOG_FILE(6,"USER_DIRECT ioctl success rsrc handle = 0x%x for path_index = %d",
			chunk->path[path_index]->sisl.resrc_handle,path_index);
	
    return rc;
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
    dk_capi_uvirtual_t disk_virtual;
    int i;

    
    if (chunk == NULL) {
	
	return (-1);
    }

    if (chunk->path[chunk->cur_path] == NULL) {


	return -1;
    }

    if (chunk->path[chunk->cur_path]->afu == NULL) {


	return -1;
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


    if (chunk->flags & CFLSH_CHNK_VLUN) {

	/*
	 * Get a virtual lun of size 0 for the specified AFU and context.
	 */

	bzero(&disk_virtual,sizeof(disk_virtual));

#ifdef _AIX
	disk_virtual.devno = chunk->path[chunk->cur_path]->afu->adap_devno;
	disk_virtual.path_id_mask = chunk->path[chunk->cur_path]->path_id_mask;
	disk_virtual.ctx_token = chunk->path[chunk->cur_path]->afu->contxt_id;

	// TODO:? Need to set scrub on virtual lun, when ioctl defines it.
#else
	disk_virtual.context_id = chunk->path[chunk->cur_path]->afu->contxt_id;

	if (chunk->flags & CFLSH_CHNK_VLUN_SCRUB) {
	
	    disk_virtual.hdr.flags |= DK_CXLFLASH_UVIRTUAL_NEED_WRITE_SAME;
	}
#endif /* !AIX */

#ifdef _AIX
	disk_virtual.vlun_size = 0; 
#else
	disk_virtual.lun_size = 0; 
#endif /* !_AIX */

	/*
	 * Note: For virtual luns, we do not support multi-pathing.
	 *       Thus we only use the chunk file descriptor for this ioctl.
	 */

	rc = ioctl(chunk->fd,DK_CAPI_USER_VIRTUAL,&disk_virtual);

	if (rc) {

	    CBLK_TRACE_LOG_FILE(1,"DK_CAPI_USER_VIRTUAL failed with errno = %d, return_flags = 0x%llx",
			    errno,disk_virtual.return_flags);

	    cblk_chunk_free_mc_device_resources(chunk);

	    return -1;
	}

	chunk->path[chunk->cur_path]->sisl.resrc_handle = 0xffffffff & disk_virtual.rsrc_handle;

#ifndef _AIX
#ifdef DK_CXLFLASH_ALL_PORTS_ACTIVE

	if (disk_virtual.return_flags & DK_CXLFLASH_ALL_PORTS_ACTIVE) {

	    /*
	     * Multiple ports on this AFU are used for this virtual lun.
	     */

	    chunk->path[chunk->cur_path]->num_ports++;

	}
	CBLK_TRACE_LOG_FILE(6,"USER_VIRTUAL num_ports =  0x%x",chunk->path[chunk->cur_path]->num_ports);
#endif 

#endif
	
	CBLK_TRACE_LOG_FILE(6,"USER_VIRTUAL ioctl success rsrc handle = 0x%x",
			    chunk->path[chunk->cur_path]->sisl.resrc_handle);
	
    } else {

	/*
	 * Get a physical lun for all requested specified AFU and context.
	 *
	 * It should be noted that at attach time, we determined the maximum
	 * number of paths allowed for this. So now we just ensure that each
	 * has a valid attachment.
	 */

	for (i=0;i < chunk->num_paths; i++) {

	    if (cblk_chunk_get_mc_phys_disk_resources_path(chunk,i)) {

	   
		cblk_chunk_free_mc_device_resources(chunk);
		
		return -1;
	    }
	}
	
	chunk->num_blocks = chunk->num_blocks_lun;
	CBLK_TRACE_LOG_FILE(5,"last_lba = 0x%llx",chunk->num_blocks_lun);
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
    dk_capi_resize_t disk_resize;
#ifdef _MASTER_CONTXT   



    bzero(&disk_resize,sizeof(disk_resize));

    if (nblocks != -1) {

	/*
	 * Caller is requesting a specific amount of space
	 */

	if (nblocks < chunk->vlun_max_num_blocks) {

	    /*
	     * If the amount of space requested is is still within the current
	     * space allocated by the MC from the last size request, then just
	     * return the size requested by the caller.
	     */

	    CBLK_TRACE_LOG_FILE(5,"blocks already exist so just use them");
	    chunk->num_blocks =  nblocks;
	    return 0;
	}

	

    } 


    if (chunk->path[chunk->cur_path] == NULL) {

	
	CBLK_TRACE_LOG_FILE(1,"NULL path");
	return -1;
    }
 

#ifdef _AIX
    disk_resize.devno = chunk->path[chunk->cur_path]->afu->adap_devno;
    disk_resize.ctx_token = chunk->path[chunk->cur_path]->afu->contxt_id;
    disk_resize.vlun_size = nblocks;
#else
    disk_resize.context_id = chunk->path[chunk->cur_path]->afu->contxt_id;
    disk_resize.req_size = nblocks;
#endif /* !AIX */


    disk_resize.rsrc_handle = chunk->path[chunk->cur_path]->sisl.resrc_handle;



    /*
     * Note: For virtual luns, we do not support multi-pathing.
     *       Thus we only use the chunk file descriptor for this ioctl.
     */

    rc = ioctl(chunk->fd,DK_CAPI_VLUN_RESIZE,&disk_resize);
    



    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_VLUN_RESIZE failed with rc = %d, errno = %d, size = 0x%llx, return_flags = 0x%llx",
			    rc, errno,(uint64_t)nblocks,disk_resize.return_flags);

	if (errno == 0) {

	    errno = ENOMEM;
	}
	return -1;
    }


    
    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_VLUN_RESIZE succeed with size = 0x%llx and actual_size = 0x%llx",
			(uint64_t)nblocks, disk_resize.last_lba);

    if ((nblocks != -1) &&
	((disk_resize.last_lba + 1) < nblocks)) {


	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_VLUN_RESIZE returned smaller actual size = 0x%llx then requested = 0x%llx",
			    disk_resize.last_lba,(uint64_t)nblocks);

	errno = ENOMEM;

	return -1;
    }


    /*
     * Save off the actual amount of space the MC allocated, which may be more than
     * what the user requested. 
     */

    chunk->vlun_max_num_blocks = disk_resize.last_lba + 1;

    if (nblocks == -1) {

	nblocks = chunk->vlun_max_num_blocks;
    }
#else

  
    /*
     *       This is temporary code for
     *       early development to allow virtual
     *       luns.  Eventually the MC will provision
     *       this. For now the block layer will use
     *       a very simplistic and flawed approach
     *       that leads to inefficient memory usage
     *       and fragmentation. However it is hoped
     *       this flawed approach is sufficient until
     *       the MC can provide the real functionality.
     *       When the MC does add this functionality,
     *       this code can be removed if needed.
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
 *         This routine is not valid for MPIO physical luns.
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
#ifdef _AIX
#define   DK_CAPI_VLUN_CLONE   0x8B  
    errno = EINVAL;
    return -1;
#else
    int rc = 0;
#ifdef  _MASTER_CONTXT  
#ifdef _NOT_YET
    uint64_t chunk_flags;
#endif
    int cleanup_depth;
    dk_capi_clone_t disk_clone;
    dk_capi_detach_t disk_detach;
    res_hndl_t old_resrc_handle; 
    void *old_mmio_mmap;
    size_t old_mmap_size;
    int old_adap_fd;
    uint64_t old_contxt_id;


    /*
     * Cloning only works for virtual luns. In addition,
     * virtual luns can not use shared contexts.
     */

    if (!(chunk->flags & CFLSH_CHNK_VLUN)) {

	/*
	 * Clone is only supported on virtual luns
	 */


	CBLK_TRACE_LOG_FILE(1,"physical lun attempted to clone flags = 0x%x",chunk->flags);
	return -1;
	
    }

    if (chunk->path[chunk->cur_path] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null path passed, cur_path = %d",chunk->cur_path);
	return -1;
    }

    if (chunk->path[chunk->cur_path]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null afu passed, cur_path = %d",chunk->cur_path);
	return -1;
    }


    bzero(&disk_clone,sizeof(disk_clone));
    bzero(&disk_detach,sizeof(disk_detach));


    
    /*
     * It should be noted the chunk is not a fully functional chunk
     * from this process' perspective after a fork. It has enough information that should allow
     * us to clone it into a new chunk using the same chunk id and chunk structure.
     * So first save off relevant information about the old chunk before unregistering 
     * it.
     */



    old_resrc_handle = chunk->path[chunk->cur_path]->sisl.resrc_handle;
    old_mmio_mmap = chunk->path[chunk->cur_path]->afu->mmio_mmap;
    old_mmap_size = chunk->path[chunk->cur_path]->afu->mmap_size;
    old_contxt_id = chunk->path[chunk->cur_path]->afu->contxt_id;
    old_adap_fd = chunk->path[chunk->cur_path]->afu->poll_fd;

    /*
     * If we have a dedicated thread per chunk
     * for interrupts, then stop it now.
     */

    cblk_open_cleanup_wait_thread(chunk);

    cleanup_depth = 30;


    if (cblk_chunk_attach_process_map(chunk,mode,&cleanup_depth)) {

	CBLK_TRACE_LOG_FILE(1,"Unable to attach, errno = %d",errno);


	  
	cblk_chunk_open_cleanup(chunk,cleanup_depth);
	free(chunk);
	    
	return -1;

    }
    

    rc = munmap(old_mmio_mmap,old_mmap_size);

    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"munmap failed with rc = %d errno = %d",
			    rc,errno);

	    
	cblk_chunk_open_cleanup(chunk,cleanup_depth);


	return -1;
    }


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

    cleanup_depth = 50;

#ifdef _NOT_YET
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
#endif /* _NOT_YET */


#ifndef _AIX
#ifdef DK_CXLFLASH_VLUN_CLONE
   disk_clone.hdr.flags = mode & O_ACCMODE;
#else
   disk_clone.flags = mode & O_ACCMODE;
#endif 
   disk_clone.context_id_src = old_contxt_id;
   disk_clone.context_id_dst = chunk->path[chunk->cur_path]->afu->contxt_id;
   disk_clone.adap_fd_src = old_adap_fd;


#endif 

   /*
    * Note: For virtual luns, we do not support multi-pathing.
    *       Thus we only use the chunk file descriptor for this ioctl.
    */

    rc = ioctl(chunk->fd,DK_CAPI_VLUN_CLONE,&disk_clone);

    if (rc) {

	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_CLONE ioctl failed with rc = %d, errno = %d",
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


    if (rc) {

	/* 
	 * If any of the above operations fail then
	 * exit out this code. 
	 */


	CBLK_TRACE_LOG_FILE(1,"close failed with rc = %d errno = %d",
			    rc,errno);

    }


    
    if (chunk->path[chunk->cur_path]->flags & CFLSH_PATH_CLOSE_POLL_FD) {
	
	close(old_adap_fd);
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
    
#else

    rc = EINVAL;
#endif  /* _COMMON_INTRPT_THREAD */

#endif /* _MASTER_CONTXT */
    return rc;

#endif /* ! AIX */
}



/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_free_mc_device_resources_path
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
void  cblk_chunk_free_mc_device_resources_path(cflsh_chunk_t *chunk, int path_index)
{
    dk_capi_release_t disk_release;
#ifdef  _MASTER_CONTXT   
    int rc = 0;
#endif /* _MASTER_CONTXT */



    if (chunk == NULL) {
	
	return;
    }
 

    if (chunk->path[path_index] == NULL) {

	
	CBLK_TRACE_LOG_FILE(1,"NULL path");
	return;
    }

    if (!(chunk->path[path_index]->flags & CFLSH_PATH_ACT)) {

	/*
	 * Path is not active. Just return.
	 */

	return;
    }

    bzero(&disk_release,sizeof(disk_release));


#ifdef  _MASTER_CONTXT   

    /*
     * Free resources for this lun.
     */

    if (chunk->path[path_index]->afu->contxt_id == 0) {
	/*
	 * There is nothing to do here, exit
	 */

	return;

    }




#ifdef _AIX
    disk_release.devno = chunk->path[path_index]->afu->adap_devno;
    disk_release.ctx_token = chunk->path[path_index]->afu->contxt_id;
#else
    disk_release.context_id = chunk->path[path_index]->afu->contxt_id;
#endif /* !AIX */


    disk_release.rsrc_handle = chunk->path[path_index]->sisl.resrc_handle;



    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_RELEASE,&disk_release);
    

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_RELEASE failed with rc = %d, errno = %d, return_flags = 0x%llx, contxt_id = 0x%llx",
			    rc, errno,disk_release.return_flags,
			    chunk->path[path_index]->afu->contxt_id);

	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_RELEASE failed with rsrc_hndl = 0x%llx, chunk->index = %d",
			    chunk->path[path_index]->sisl.resrc_handle,
			    chunk->index);
	return;
    } else {

	CBLK_TRACE_LOG_FILE(9,"DK_CAPI_RELEASE success rsrc_hndl = 0x%llx, chunk->index = %d, contxt_id = 0x%llx",
			    chunk->path[path_index]->sisl.resrc_handle,
			    chunk->index,
			    chunk->path[path_index]->afu->contxt_id);


    }


    chunk->path[path_index]->afu->master.mc_handle = 0;

#endif /* _MASTER_CONTXT */

    return;
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
    int i;


    for (i = 0; i < chunk->num_paths; i++) {

	cblk_chunk_free_mc_device_resources_path(chunk,i);
    }
    

    return;
}
 
#ifndef _AIX
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
cflash_cmd_err_t cblk_process_nonafu_intrpt_cxl_events(cflsh_chunk_t *chunk,int path_index,
						       struct cxl_event *cxl_event)
{
    int rc = CFLASH_CMD_FATAL_ERR;
    uint64_t intrpt_status;
    cflash_block_check_os_status_t reset_status = CFLSH_BLK_CHK_OS_NO_RESET;

    errno = EIO;


    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path == NULL");
	return -1;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu == NULL");
	return -1;
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
	CBLK_TRACE_LOG_FILE(6,"contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);
	CBLK_TRACE_LOG_FILE(6,"mmio_map = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio_mmap);
	CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio);
	CBLK_TRACE_LOG_FILE(6,"mmap_size = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmap_size);
	CBLK_TRACE_LOG_FILE(6,"hrrq_start = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_start);
	CBLK_TRACE_LOG_FILE(6,"hrrq_end = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_end);
	CBLK_TRACE_LOG_FILE(6,"cmd_start = 0x%llx",(uint64_t)chunk->cmd_start);
	CBLK_TRACE_LOG_FILE(6,"cmd_end = 0x%llx",(uint64_t)chunk->cmd_end);

	intrpt_status = CBLK_GET_INTRPT_STATUS(chunk,path_index,&reset_status);
	CBLK_TRACE_LOG_FILE(6,"intrpt_status = 0x%llx",intrpt_status);

	CBLK_TRACE_LOG_FILE(6,"num_active_cmds = 0x%x\n",chunk->num_active_cmds);
	
	
	
	if (!(cblk_chk_cmd_bad_page(chunk,cxl_event->fault.addr))) {
	    
	    /*
	     * If a command matching this bad address is not found, then log a general
	     * error here.
	     */
	    cblk_notify_mc_err(chunk,path_index,0x508,cxl_event->fault.addr, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

	    
	    CBLK_TRACE_LOG_FILE(1,"Bad page addr = 0x%llx not found",cxl_event->fault.addr);
	}

	break;
    case CXL_EVENT_AFU_ERROR:
	chunk->stats.num_capi_afu_errors++;
	CBLK_TRACE_LOG_FILE(1,"CXL_EVENT_AFU_ERROR error = 0x%llx, flags = 0x%x",
			    cxl_event->afu_error.error,cxl_event->afu_error.flags);
	
	cblk_notify_mc_err(chunk,path_index,0x500,cxl_event->afu_error.error,CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

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
	

	cblk_notify_mc_err(chunk,path_index,0x501,cxl_event->header.type, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

    } /* switch */

    return rc;
}

#endif /* !_AIX */

#ifdef _AIX
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
				       size_t *transfer_size, struct pollfd poll_list[])
{
    int rc = 0;
    int eeh_event = 0;
#ifndef BLOCK_FILEMODE_ENABLED
    dk_capi_exceptions_t dk_exceptions;
    int i;
    int original_primary_path;
    int found_new_path = FALSE;
#endif

 
#ifdef BLOCK_FILEMODE_ENABLED

    chunk->stats.num_capi_afu_intrpts++;

    rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,(int)CFLSH_BLK_INTRPT_STATUS,cmd_complete,transfer_size);

#else


    if (cmd == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmd is null");


	return -1;
    }

    if (cmd_complete == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmd_complete is null");


	return -1;
    }

    if (transfer_size == NULL) {

	CBLK_TRACE_LOG_FILE(1,"transfer_size is null");


	return -1;
    }
    
    CBLK_TRACE_LOG_FILE(7,"poll_list[CFLASH_ADAP_POLL_INDX].revents = 0x%x,  poll_list[CFLASH_DISK_POLL_INDX].revents = 0x%x",
			poll_list[CFLASH_ADAP_POLL_INDX].revents,poll_list[CFLASH_DISK_POLL_INDX].revents);

    if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLPRI) {

	/*
	 * Adapter exception has occurred.
	 */

	if ((*cmd) && ((*cmd)->cmdi)) {

	    CBLK_TRACE_LOG_FILE(6," cmd = 0x%llx lba = 0x%llx flags = 0x%x, cmd->cmdi->buf = 0x%llx",
				*cmd,(*cmd)->cmdi->lba,(*cmd)->cmdi->flags,(*cmd)->cmdi->buf);

	}

	do {



	    chunk->stats.num_capi_data_st_errs++;

	    bzero(&dk_exceptions, sizeof dk_exceptions);
	    dk_exceptions.devno = chunk->path[path_index]->afu->adap_devno;
	    dk_exceptions.ctx_token = chunk->path[path_index]->afu->contxt_id;
	    dk_exceptions.rsrc_handle = chunk->path[path_index]->sisl.resrc_handle;
	    dk_exceptions.flags = DK_QEF_ADAPTER;
	    
	    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_QUERY_EXCEPTIONS,&dk_exceptions);
	    
	    
	    if (rc) {
		
		CBLK_TRACE_LOG_FILE(1,"DK_CAPI_QUERY_EXCEPTIONS failed with rc = %d, errno = %d, return_flags = 0x%llx\n",
				    rc,errno,dk_exceptions.return_flags);
		
		CBLK_NOTIFY_LOG_THRESHOLD(9,chunk,path_index,0x50a,errno,CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);


		break;
	    }
	    
	    
	    CBLK_TRACE_LOG_FILE(5,"Adapter exceptions = 0x%llx adap_except_type = 0x%llx adap_except_time = 0x%llx",
				dk_exceptions.exceptions,dk_exceptions.adap_except_type,dk_exceptions.adap_except_time);
	    
	    CBLK_TRACE_LOG_FILE(5,"Adapter adap_except_data = 0x%llx adap_except_count = 0x%llx",
				dk_exceptions.adap_except_data,dk_exceptions.adap_except_count);

	    switch (dk_exceptions.adap_except_type) {
	      case DK_AET_BAD_PF:    

		/*
		 * Bad page fault
		 */
		
		CBLK_TRACE_LOG_FILE(6,"contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);
		CBLK_TRACE_LOG_FILE(6,"mmio_map = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio_mmap);
		CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio);
		CBLK_TRACE_LOG_FILE(6,"mmap_size = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmap_size);
		CBLK_TRACE_LOG_FILE(6,"hrrq_start = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_start);
		CBLK_TRACE_LOG_FILE(6,"hrrq_end = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_end);
		CBLK_TRACE_LOG_FILE(6,"cmd_start = 0x%llx",(uint64_t)chunk->cmd_start);
		CBLK_TRACE_LOG_FILE(6,"cmd_end = 0x%llx",(uint64_t)chunk->cmd_end);

		if (!(cblk_chk_cmd_bad_page(chunk,dk_exceptions.adap_except_data))) {

		    /*
		     * If a command matching this bad aaddress is not found, then log a general
		     * error here.
		     */
		    cblk_notify_mc_err(chunk,path_index,0x505,dk_exceptions.adap_except_data, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);
		}
		break;

	      case  DK_AET_EEH_EVENT:
		/*
		 * EEH has occurred. process EEH after all exceptions have been queride
		 */

		CBLK_TRACE_LOG_FILE(6,"EEH exception reported, contxt_id = 0x%llx",chunk->path[path_index]->afu->contxt_id);
		eeh_event++;
		break;

	      case DK_AET_AFU_ERROR:

		/*
		 * AFU erro detected, ignore return code
		 */

		CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,NULL,(int)CFLSH_BLK_INTRPT_STATUS,NULL,NULL);

		break;

	      default:
		CBLK_TRACE_LOG_FILE(1,"Unknown Adapter exceptions = 0x%llx adap_except_type = 0x%llx adap_except_time = 0x%llx",
				dk_exceptions.exceptions,dk_exceptions.adap_except_type,dk_exceptions.adap_except_time);

		cblk_notify_mc_err(chunk,path_index,0x506,dk_exceptions.adap_except_type, CFLSH_BLK_NOTIFY_ADAP_ERR,NULL);

	    }
	} while (dk_exceptions.adap_except_count);


	if (eeh_event) {

	    CBLK_TRACE_LOG_FILE(7,"EEH rcovery");

	    cblk_check_os_adap_err(chunk,path_index);

	}
    }
    
    if (poll_list[CFLASH_DISK_POLL_INDX].revents & POLLPRI) {

	/*
	 * Disk exception has occurred
	 */

	
	bzero(&dk_exceptions, sizeof dk_exceptions);
	dk_exceptions.devno = chunk->path[path_index]->afu->adap_devno;
	dk_exceptions.ctx_token = chunk->path[path_index]->afu->contxt_id;
	dk_exceptions.rsrc_handle = chunk->path[path_index]->sisl.resrc_handle;

	    
	rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_QUERY_EXCEPTIONS,&dk_exceptions);

	
	if (rc) {
	
	    CBLK_TRACE_LOG_FILE(1,"DK_CAPI_QUERY_EXCEPTIONS failed with rc = %d, errno = %d, return_flags = 0x%llx\n",
				rc,errno,dk_exceptions.return_flags);

	    
	    cblk_notify_mc_err(chunk,path_index,0x502,errno, CFLSH_BLK_NOTIFY_DISK_ERR,NULL);

	
	}
	
	CBLK_TRACE_LOG_FILE(5,"Disk exceptions = 0x%llx,adap_except_type = 0x%llx,adap_except_time = 0x%llx",
			    dk_exceptions.exceptions,dk_exceptions.adap_except_type,dk_exceptions.adap_except_time);

	CBLK_TRACE_LOG_FILE(5,"Disk adap_except_data = 0x%llx adap_except_count = 0x%llx",
			    dk_exceptions.adap_except_data,dk_exceptions.adap_except_count);
	
	if (dk_exceptions.exceptions & DK_CE_VLUN_TRUNCATED) {
	    /*
	     * This Virtual LUN is now smaller.
	     * Save off the new size.  
	     */

	    CBLK_TRACE_LOG_FILE(1,"Lun has shrunk: return_flags = 0x%llx, new size = 0x%llx\n",
				dk_exceptions.return_flags,dk_exceptions.last_lba);
	    

	    chunk->vlun_max_num_blocks = dk_exceptions.last_lba;

	    chunk->num_blocks = dk_exceptions.last_lba;

	    
	    cblk_notify_mc_err(chunk,path_index,0x507,dk_exceptions.last_lba,CFLSH_BLK_NOTIFY_DISK_ERR,NULL);

	    

	} else if (dk_exceptions.exceptions & DK_CE_PATH_LOST) {
	    
	    /*
	     * Someone entity has done a verify on this disk and that
	     * operation detected a path was lost.  This exception only
	     * tells us a path was lost, but not which path it was. Even 
	     * if we are not running MPIO, it is possible this notification
	     * is a path used by another process that was lost. So we need
	     * verify we have have a valid path. 
	     */

	    if (cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_DISK_ERR,NULL,NULL)) {
		
		/*
		 * Verification failed on primary path. Look for another good path 
		 * if it exists.
		 */
		
		original_primary_path = chunk->cur_path;

		for (i = 0; i < chunk->num_paths; i++) {


		    if (i == original_primary_path) {

			continue;
		    }

		    /*
		     * Change primary path to this one and see if it is functional
		     */

		    chunk->cur_path = i;

		    if (!(cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_DISK_ERR,NULL,NULL))) {

			/*
			 * We found a good path,
			 */
			found_new_path = TRUE;
			break;
		    }
		
		} /* for */


		if (!found_new_path) {

		    /*
		     * We did not find a valid path for this
		     * device.
		     */

		    cblk_notify_mc_err(chunk,path_index,0x509,chunk->num_paths,CFLSH_BLK_NOTIFY_DISK_ERR,NULL);

		    rc = EIO;
		
		    chunk->flags |= CFLSH_CHUNK_FAIL_IO;

		    /*
		     * Clear halted state so that subsequent commands
		     * do not hang, but are failed immediately.
		     */

		    chunk->flags &= ~CFLSH_CHNK_HALTED;
		
		    /*
		     * Issue reset context to fail any active I/O.
		     */
		
		
		    cblk_chunk_free_mc_device_resources(chunk);
		
		    cblk_chunk_unmap(chunk,TRUE);
		    cblk_chunk_detach(chunk,TRUE);
		    close(chunk->fd);
		
		    cblk_fail_all_cmds(chunk);

		}
	    }

	    
	    
	
	} else {

	    if ((dk_exceptions.exceptions & DK_CE_VERIFY_IN_PROGRESS) &&
		!(dk_exceptions.exceptions & DK_CE_VERIFY_SUCCEEDED)) {

		// TODO: ?? Should we just do verify and block until completes?

		cblk_halt_all_cmds(chunk, path_index, FALSE);
		
		

	    } else if (dk_exceptions.exceptions & DK_CE_VERIFY_SUCCEEDED) {

		
		CFLASH_BLOCK_UNLOCK(chunk->lock);
		cblk_reset_context_shared_afu(chunk->path[path_index]->afu);
		CFLASH_BLOCK_LOCK(chunk->lock);


	    }


	}

	

	if ((dk_exceptions.exceptions & DK_CE_SIZE_CHANGE) &&
	    !(chunk->flags & CFLSH_CHUNK_FAIL_IO)) {

	    if (cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_DISK_ERR,NULL,NULL)) {
		
		/*
		 * Verification failed
		 */
		
		rc = EIO;
		
		chunk->flags |= CFLSH_CHUNK_FAIL_IO;


		/*
		 * Clear halted state so that subsequent commands
		 * do not hang, but are failed immediately.
		 */

		chunk->flags &= ~CFLSH_CHNK_HALTED;
		
		/*
		 * Issue reset context to fail any active I/O.
		 */
		
		
		cblk_chunk_free_mc_device_resources(chunk);
		
		cblk_chunk_unmap(chunk,TRUE);
		cblk_chunk_detach(chunk,TRUE);
		close(chunk->fd);
		
		cblk_fail_all_cmds(chunk);
	    }
	}
    }

    
    if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLMSG) {

	/*
	 * Adapter error interrupt 
	 */

	chunk->stats.num_capi_afu_intrpts++;

	rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,CFLSH_BLK_INTRPT_STATUS,cmd_complete,transfer_size);
    } 

    if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLIN) {

	/*
	 * Adapter interrupt for command completion has occurred
	 */

	chunk->stats.num_capi_afu_intrpts++;


	rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,CFLSH_BLK_INTRPT_CMD_CMPLT,cmd_complete,transfer_size);
    } 

#ifdef _ERROR_INTR_MODE

    /*
     * In Error Interrupt Mode, we should never get POLLIN to indicate command
     * complete. The code invoking poll, expects it to time-out and then just check the
     * RRQ via the call to CBLK_PROCESS_ADAP_CONVERT_INTRPT. Thus in error interrupt
     * mode, it is possible error events may get posted and this routine
     * will be invoked instead of CBLK_PROCESS_ADAP_CONVERT_INTRPT. Since we
     * also want to handle command completions in that case, lets check for them
     * now to ensure they are processed.
     */

    rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,CFLSH_BLK_INTRPT_CMD_CMPLT,cmd_complete,
					  transfer_size);
#endif /* !_ERROR_INTR_MODE */

#endif /* !BLOCK_FILEMODE_ENABLED */

    /*
     * TODO: ?? Currently we are just returning the last rc seen,
     *       Is this the corect choice.
     */


    return rc;
}


#else 
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
				       size_t *transfer_size, struct pollfd *poll_list)
{
    int rc = 0;
    int read_bytes = 0;
    int process_bytes  = 0;
    uint8_t read_buf[CAPI_FLASH_BLOCK_SIZE];
    struct cxl_event *cxl_event = (struct cxl_event *)read_buf;
    cflash_block_check_os_status_t reset_status;
 
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

	    CBLK_TRACE_LOG_FILE(7,"possible UE recovery cmd = 0x%llx",(uint64_t)*cmd);

	    reset_status = cblk_check_os_adap_err(chunk,path_index);

	    if ((reset_status != CFLSH_BLK_CHK_OS_NO_RESET) &&
		(reset_status != CFLSH_BLK_CHK_OS_RESET_FAIL)) {


		/*
		 * If adapter reset succeeded, is pending or is indeterminant,
		 * then return good completion here to allow this state machine
		 * to proceed. Otherwise fail this, with the existing errno.
		 */
		errno = 0;

		return 0;
	    }
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

#ifdef _ERROR_INTR_MODE

    /*
     * In Error Interrupt Mode, we should never get events to indicate command
     * complete. The code invoking poll, expects it to time-out and then just check the 
     * RRQ via the call to CBLK_PROCESS_ADAP_CONVERT_INTRPT. Thus in error interrupt
     * mode, it is possible error events may get posted and this routine
     * will be invoked instead of CBLK_PROCESS_ADAP_CONVERT_INTRPT. Since we
     * also want to handle command completions in that case, lets check for them
     * now to ensure they are processed.
     */

    rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,cmd,CFLSH_BLK_INTRPT_CMD_CMPLT,cmd_complete,
					  transfer_size);
#endif /* !_ERROR_INTR_MODE */


    /*
     * TODO: ?? Currently we are just returning the last rc seen,
     *       Is this the corect choice.
     */


    return rc;

}
#endif /* !_AIX */


/*
 * NAME:        cblk_check_os_adap_err_failure_cleanup
 *
 * FUNCTION:    If when doing the check os adapter error recovery
 *              we suffer a fatal, then clean up by failing all I/O
 *              to all associated paths of this AFU.
 *              This routine assumes the caller is holding only the global lock.
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
void cblk_check_os_adap_err_failure_cleanup(cflsh_chunk_t *chunk, cflsh_afu_t *afu)
{

    cflsh_path_t *path;
    int pthread_rc = 0;
    cflsh_chunk_t *tmp_chunk;
    int path_index;




    afu->flags &= ~(CFLSH_AFU_HALTED|CFLSH_AFU_RECOV);
	
	
    pthread_rc = pthread_cond_broadcast(&(afu->resume_event));
	
    if (pthread_rc) {
	    
	CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for AFU resume_event rc = %d,errno = %d",
			    pthread_rc,errno);
    }	

    path = afu->head_path;
	
    CFLASH_BLOCK_UNLOCK(afu->lock);



    chunk->flags |= CFLSH_CHUNK_FAIL_IO;


    /*
     * Clear halted state so that subsequent commands
     * do not hang, but are failed immediately.
     */

    chunk->flags &= ~CFLSH_CHNK_HALTED;

    cblk_chunk_free_mc_device_resources(chunk);

    cblk_chunk_unmap(chunk,TRUE);
    cblk_chunk_detach(chunk,TRUE);
    close(chunk->fd);

	
    /*
     * For each chunk associated with this path:
     *
     *   - Put the chunk in a failed state
     *   - fail all active I/O
     *   - Wakeup any threads waiting to issue
     *     commands for this. Since the chunk is
     *     in a failed state, these should all fail
     *     immediately.
     */

    while (path) {


	tmp_chunk = path->chunk;

	if (tmp_chunk == NULL) {
	    CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
	    continue;
	}

	CFLASH_BLOCK_LOCK(tmp_chunk->lock);

	if (tmp_chunk == chunk) {

	    
	    chunk->flags &= ~CFLSH_CHNK_RECOV_AFU;

	    chunk->recov_thread_id = 0;
	}

	path_index = path->path_index;


	tmp_chunk->flags |= CFLSH_CHUNK_FAIL_IO;


	/*
	 * Clear halted state so that subsequent commands
	 * do not hang, but are failed immediately.
	 */

	tmp_chunk->flags &= ~CFLSH_CHNK_HALTED;

	cblk_fail_all_cmds(tmp_chunk);

	pthread_rc = pthread_cond_broadcast(&(tmp_chunk->path[path_index]->resume_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for resume_event rc = %d,errno = %d",
				pthread_rc,errno);
	}

	CFLASH_BLOCK_UNLOCK(tmp_chunk->lock);

	path = path->next;
    }

    return;
}


/*
 * NAME:        cblk_check_os_adap_err
 *
 * FUNCTION:    Inform adapter driver that it needs to check if this
 *              is a fatal error that requires a reset.
 *              This routine assumes the caller is holding chunk->lock.
 *              
 *
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 * RETURNS:
 *              status on Recovery
 *              
 *              
 */
cflash_block_check_os_status_t cblk_check_os_adap_err(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;
    cflash_block_check_os_status_t status = CFLSH_BLK_CHK_OS_NO_RESET;
    cflsh_afu_t *afu;
    cflsh_path_t *path;
    int tmp_path_index;
    cflsh_chunk_t *tmp_chunk;
    dk_capi_recover_context_t disk_recover;
    int pthread_rc = 0;
#ifndef _AIX
    int old_adap_fd;
#endif
    void *old_mmio;
    size_t old_mmio_size;
    

    if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

	return status;
    }

    afu = chunk->path[path_index]->afu;


    if (afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"AFU is null");
    }
    

    if (chunk->flags & CFLSH_CHNK_RECOV_AFU) {

	/*
	 * If a RECOV AFU is being done from this
	 * chunk now, then there is nothing further
	 * to do here.
	 */

	CBLK_TRACE_LOG_FILE(9,"AFU recovery is already active, for chunk->index = %d, chunk->dev_name = %s, path_index = %d,chunk->flags = 0x%x,afu->flags = 0x%x",
			    chunk->index,chunk->dev_name,path_index,chunk->flags,chunk->path[path_index]->afu->flags);

	
	CBLK_TRACE_LOG_FILE(9,"num_active_cmds = %d, pid = 0x%llx",
			    chunk->num_active_cmds,(uint64_t)cflsh_blk.caller_pid);

	if (chunk->recov_thread_id == CFLSH_BLK_GETTID) {
	    CBLK_TRACE_LOG_FILE(9,"This is the recovery thread waiting for recovery num_active_cmds = %d",
				chunk->num_active_cmds);
	}

	return CFLSH_BLK_CHK_OS_RESET_PEND;

    } else {

	chunk->flags |= CFLSH_CHNK_RECOV_AFU;


	/*
	 * Save off a unique thread id
	 * for this thread, since it will drive
	 * the error recovery.
	 */

	chunk->recov_thread_id = CFLSH_BLK_GETTID;
    }

    /*
     * Set the halted flag for this chunk, since
     * we're about to unlock.
     */

    chunk->flags |= CFLSH_CHNK_HALTED;

    chunk->stats.num_capi_adap_chck_err++;

    /*
     * Since we are going to grab the global lock as
     * well as some other locks, we need to release this chunk
     * lock, because the lock ordering scheme followed by this
     * library requires Global lock first, then chunk lock.
     */

    CFLASH_BLOCK_UNLOCK(chunk->lock);


    /*
     * Grab the global lock here, since we could
     * have multiple chunk paths sharing this AFU.
     */

    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);


    
    CFLASH_BLOCK_LOCK(afu->lock);

    if (chunk->path[path_index]->flags & CFLSH_PATH_A_RST) {

	/*
	 * It is possible in the case of a shared AFU among different chunk paths,
	 * that multiple ones may detect this adapter error and decide to issue
	 * the Recover Adapter ioctl. We need to ensure that is only done once
	 * for this case. Thus when teh Recover Adapter ioctl is done, we will set
	 * the each of the associated path flags with CFLSH_PATH_A_RST. This allows
	 * them to detect this case and avoid issuing the ioctl. The thread that did do
	 * the ioctl, will have updated all AFU fields and halted/resumes all I/Os to
	 * this AFU. Thus these subsequent chunks, should not need to do anything except
	 * possibly wake up block requests.
	 */
	
	CBLK_TRACE_LOG_FILE(5,"afu recovery done via another path to this shared AFU");

	chunk->flags &= ~(CFLSH_CHNK_HALTED|CFLSH_CHNK_RECOV_AFU);

	chunk->recov_thread_id = 0;

	chunk->path[path_index]->flags &= ~CFLSH_PATH_A_RST;

	CFLASH_BLOCK_UNLOCK(afu->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
    
	CFLASH_BLOCK_LOCK(chunk->lock);

	pthread_rc = pthread_cond_broadcast(&(chunk->path[chunk->cur_path]->resume_event));
	

	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for resume_event rc = %d,errno = %d",
				pthread_rc,errno);
	}
	

	return CFLSH_BLK_CHK_OS_RESET;
    }


    /*
     * If we got here, then there was not other thread that
     * processed this recover adapter ioctl. So let's issue
     * it along with the associated work needed.
     */


    afu->flags |= CFLSH_AFU_HALTED|CFLSH_AFU_RECOV;

    path = afu->head_path;
    
    /*
     * Since we are going to be grabbing each chunk lock for chunk's
     * that have paths that use this AFU, we need to release the AFU
     * lock. This is because lock order convention in this library
     * is that chunk lock must be grabbed before AFU lock.
     * We should be safe iterating over the paths in this AFU,
     * since we still have the global lock.
     */

    CFLASH_BLOCK_UNLOCK(afu->lock);



    /* 
     * Mark all chunks that have paths associated with this AFU
     * in the halted state to stop any new I/O from being attempted
     * until we complete the Recover Adapter ioctl.
     */

    while (path) {


	tmp_chunk = path->chunk;

	if (tmp_chunk == NULL) {
	    CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
	    continue;
	}

	CFLASH_BLOCK_LOCK(tmp_chunk->lock);

	tmp_chunk->flags |= CFLSH_CHNK_HALTED;
	CFLASH_BLOCK_UNLOCK(tmp_chunk->lock);

	path = path->next;
    }


    /*
     * Now that all chunks have been "halted" that have
     * paths associated with this AFU, let's grab the
     * AFU lock again for this ioctl, since will be first
     * reading values for the AFU, and the potential after the
     * ioctl updating those values.
     */

    CFLASH_BLOCK_LOCK(afu->lock);


    bzero(&disk_recover,sizeof(disk_recover));

    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER initiated, for chunk->index = %d, chunk->dev_name = %s, path_index = %d,chunk->flags = 0x%x",
			chunk->index,chunk->dev_name,path_index,chunk->flags);

    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER old afu->contxt_id = 0x%llx, old_adap_fd = %d",
			chunk->path[path_index]->afu->contxt_id,chunk->path[path_index]->afu->poll_fd);


    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER old mmio = 0x%llx, old mmio_size = 0x%llx",
			chunk->path[path_index]->afu->mmio,chunk->path[path_index]->afu->mmap_size);


    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER num_active_cmds = %d, pid = 0x%llx",
			chunk->num_active_cmds,(uint64_t)cflsh_blk.caller_pid);



#ifdef _AIX
    disk_recover.devno = chunk->path[path_index]->afu->adap_devno;

    disk_recover.ctx_token = chunk->path[path_index]->afu->contxt_id;
#else

    disk_recover.context_id = chunk->path[path_index]->afu->contxt_id;
#endif /* _AIX */



    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_RECOVER_CTX,&disk_recover);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_RECOVER failed with rc = %d, errno = %d, return_flags = 0x%llx\n",
			    rc,errno,disk_recover.return_flags);

	


	/*
	 * If this ioctl failed, then we have no recourse other
	 * than to fail everything. So mark all 
	 * associated chunks in failed
	 * state and fail all their I/O.
	 *
	 * NOTE: cblk_check_os_adap_err_failure_cleanup
	 *       unlocks the afu->lock.
	 */
	    
	cblk_notify_mc_err(chunk,path_index,0x503,errno, CFLSH_BLK_NOTIFY_ADAP_ERR,NULL);

	CBLK_LIVE_DUMP_THRESHOLD(1,"0x503");


	cblk_check_os_adap_err_failure_cleanup(chunk,afu);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	return status; 

    } else {


	if (disk_recover.return_flags & DK_RF_REATTACHED) {
	
	    /*
	     * The ioctl succeeded and an AFU reset has been done.
	     * We need to process the updated information from this
	     */

	    CBLK_TRACE_LOG_FILE(9,"DK_CAPI_RECOVER reattached old afu->contxt_id = 0x%llx, new_adap_fd = %d",
				chunk->path[path_index]->afu->contxt_id,chunk->path[path_index]->afu->poll_fd);



	    status = CFLSH_BLK_CHK_OS_RESET_SUCC;

	    path = afu->head_path;
	
	    CFLASH_BLOCK_UNLOCK(afu->lock);


	    /*
	     * For each chunk associated with this path:
	     *
	     *   - Mark the chunk's path to indicate adapter 
	     *     reset occurred.
	     *   - Halt all active I/O
	     */

	    while (path) {


		tmp_chunk = path->chunk;

		if (tmp_chunk == NULL) {
		    CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
		    continue;
		}

		CFLASH_BLOCK_LOCK(tmp_chunk->lock);

		tmp_path_index = path->path_index;
		cblk_halt_all_cmds(tmp_chunk,tmp_path_index, FALSE);

		path->flags |= CFLSH_PATH_A_RST;


		/*
		 * Clear any poison bits we can for this
		 * chunk.
		 */

		cblk_clear_poison_bits_chunk(tmp_chunk,tmp_path_index,FALSE);

		CFLASH_BLOCK_UNLOCK(tmp_chunk->lock);

		path = path->next;
	    }


	    CFLASH_BLOCK_LOCK(afu->lock);

	    /*
	     * Extract new AFU information
	     */

#ifdef _AIX
	    chunk->path[path_index]->afu->contxt_id = disk_recover.new_ctx_token;
	    chunk->path[path_index]->afu->contxt_handle = 0xffffffff & disk_recover.new_ctx_token;
	    chunk->path[path_index]->afu->poll_fd = disk_recover.new_adap_fd;


	    chunk->path[path_index]->afu->mmio_mmap = disk_recover.mmio_start;

	    chunk->path[path_index]->afu->mmio = chunk->path[path_index]->afu->mmio_mmap;

	    chunk->path[path_index]->afu->mmap_size = disk_recover.mmio_size;

	    
	    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER reattached new afu->contxt_id = 0x%llx, new_adap_fd = %d",
				chunk->path[path_index]->afu->contxt_id,chunk->path[path_index]->afu->poll_fd);


	    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER mmio = 0x%llx, mmio_size = 0x%llx",
				chunk->path[path_index]->afu->mmio,chunk->path[path_index]->afu->mmap_size);



	    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER reattached pid = 0x%llx",
				(uint64_t)cflsh_blk.caller_pid);



#else

	    old_mmio = chunk->path[path_index]->afu->mmio_mmap;

	    old_adap_fd = chunk->path[path_index]->afu->poll_fd;

	    old_mmio_size = chunk->path[path_index]->afu->mmap_size;

	    chunk->path[path_index]->afu->contxt_id = disk_recover.context_id;
	    chunk->path[path_index]->afu->contxt_handle = 0xffffffff & disk_recover.context_id;
	    chunk->path[path_index]->afu->poll_fd = disk_recover.adap_fd;


	    chunk->path[path_index]->afu->mmap_size = disk_recover.mmio_size;




	    
	    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER reattached new afu->contxt_id = 0x%llx, new_adap_fd = %d",
				chunk->path[path_index]->afu->contxt_id,chunk->path[path_index]->afu->poll_fd);

#ifdef DK_CXLFLASH_CONTEXT_SQ_CMD_MODE
	    if (disk_recover.return_flags & DK_CXLFLASH_CONTEXT_SQ_CMD_MODE) {


		/*
		 * driver is indicating this AFU supports SQ. See if this matches
		 * our original setting.
		 */

		if (!(chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ)) {

		    /*
		     * The driver is now indicating the AFU supports SQ, but
		     * we were not set up for that. So fail this.
		     */


		    cblk_check_os_adap_err_failure_cleanup(chunk,afu);

		    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
		    return CFLSH_BLK_CHK_OS_RESET_FAIL;
		}

	    }
#endif /*DK_CXLFLASH_CONTEXT_SQ_CMD_MODE  */



	    /*
	     * We do not need to unmap the old MMIO space, since the recover adapter ioctl did this
	     * for us. So we just need to map the new MMIO space here.
	     */

	    chunk->path[path_index]->afu->mmio_mmap = mmap(NULL,chunk->path[path_index]->afu->mmap_size,PROT_READ|PROT_WRITE, MAP_SHARED,
							   chunk->path[path_index]->afu->poll_fd,0);
    

	    if (chunk->path[path_index]->afu->mmio_mmap == MAP_FAILED) {

		CBLK_TRACE_LOG_FILE(1,"mmap of mmio space failed for dev_name = %s, errno = %d, mmio_size = 0x%llx",
				    chunk->dev_name,errno,(uint64_t)chunk->path[path_index]->afu->mmap_size);

		/*
		 * Setup failed, mark chunk in failed
		 * state and fail all I/O.
		 *
		 * NOTE: cblk_check_os_adap_err_failure_cleanup
		 *       unlocks the afu->lock.
		 */


		cblk_check_os_adap_err_failure_cleanup(chunk,afu);

		CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
		return CFLSH_BLK_CHK_OS_RESET_FAIL;

	    } 

	    chunk->path[path_index]->afu->mmio = chunk->path[path_index]->afu->mmio_mmap;
	    
	    
	    if (fcntl(chunk->path[path_index]->afu->poll_fd,F_SETFL,O_NONBLOCK) == -1) {
		
		/*
		 * Ignore error for now
		 */
		
		CBLK_TRACE_LOG_FILE(1,"fcntl failed with  errno = %d",errno);
		
	    }



	    if (munmap(old_mmio,old_mmio_size)) {
		
		
		
		/*
		 * Don't return here on error. Continue
		 * to close
		 */
		CBLK_TRACE_LOG_FILE(2,"munmap failed with errno = %d",
				    errno);
	    }

	    CBLK_TRACE_LOG_FILE(5,"DK_CAPI_RECOVER mmio = 0x%llx, mmio_size = 0x%llx",
				chunk->path[path_index]->afu->mmio,chunk->path[path_index]->afu->mmap_size);
	    
	    
	    if (chunk->path[path_index]->flags & CFLSH_PATH_CLOSE_POLL_FD) {
		
		close(old_adap_fd);
	    }
	    

#endif /* !AIX */

	    /*
	     * Clear AFU halted state, so that ADAP_SETUP
	     * can proceed.
	     */

	    
	    afu->flags &= ~CFLSH_AFU_HALTED;

	
	    CFLASH_BLOCK_UNLOCK(afu->lock);

    

	    if (CBLK_ADAP_SETUP(chunk,path_index)) {


		/*
		 * Setup failed, mark chunk in failed
		 * state and fail all I/O.
		 *
		 * NOTE: cblk_check_os_adap_err_failure_cleanup
		 *       unlocks the afu->lock.
		 */

		CBLK_TRACE_LOG_FILE(1,"ADAP_SETUP fails,  for chunk->index = %d, chunk->dev_name = %s, path_index = %d,chunk->flags = 0x%x",
			chunk->index,chunk->dev_name,path_index,chunk->flags);

		cblk_notify_mc_err(chunk,path_index,0x504,errno, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

		CBLK_LIVE_DUMP_THRESHOLD(1,"0x504");

		cblk_check_os_adap_err_failure_cleanup(chunk,afu);

		CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
		return CFLSH_BLK_CHK_OS_RESET_FAIL;
	    }


	    /*
	     * Since, we are re-using the same chunk, make sure
	     * to reset some fields.
	     */

	    chunk->cmd_curr = chunk->cmd_start;

	    CFLASH_BLOCK_LOCK(afu->lock);

	    afu->num_issued_cmds = 0;

	    chunk->path[path_index]->afu->p_hrrq_curr = chunk->path[path_index]->afu->p_hrrq_start;

	    chunk->path[path_index]->afu->toggle = 1;



	    bzero((void *)chunk->path[path_index]->afu->p_hrrq_start ,chunk->path[path_index]->afu->size_rrq);


	    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {

		/*
		 * If this AFU is using a Submission Queue (SQ)
		 * then reinitialize the SQ.
		 */ 
		bzero((void *)chunk->path[path_index]->afu->p_sq_start ,chunk->path[path_index]->afu->size_sq);

		chunk->path[path_index]->afu->p_sq_curr = chunk->path[path_index]->afu->p_sq_start;


	    }



	} else {


	    CBLK_TRACE_LOG_FILE(9,"DK_CAPI_RECOVER succeeded but no reattach afu->contxt_id = 0x%llx, new_adap_fd = %d",
				chunk->path[path_index]->afu->contxt_id,chunk->path[path_index]->afu->poll_fd);



	}


    }

    /*
     * Ensure the CFLSH_AFU_HALTED is cleared
     */

    afu->flags &= ~(CFLSH_AFU_HALTED|CFLSH_AFU_RECOV);

    pthread_rc = pthread_cond_broadcast(&(afu->resume_event));
	
    if (pthread_rc) {
	    
	CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for AFU resume_event rc = %d,errno = %d",
			    pthread_rc,errno);
    }	

    path = afu->head_path;
	
    CBLK_TRACE_LOG_FILE(9,"resume AFU events for afu->contxt_id = 0x%llx, adap_fd = %d",
			afu->contxt_id,afu->poll_fd);

    CFLASH_BLOCK_UNLOCK(afu->lock);


    while (path) {


	tmp_chunk = path->chunk;

	if (tmp_chunk == NULL) {
	    CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
	    continue;
	}

	CFLASH_BLOCK_LOCK(tmp_chunk->lock);

	tmp_path_index = path->path_index;

	
	/*
	 * NOTE: cblk_resume_all_halted_cmds clears CFLSH_CHNK_HALTED
	 *       and only resumes I/O that was halted by cblk_halt_all_cmds
	 */

	cblk_resume_all_halted_cmds(tmp_chunk, FALSE, tmp_path_index, FALSE);

	if (!(tmp_chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
	    /*
	     * It is possible that the common interrupt thread
	     * has given up on the current set of commands and is
	     * waiting for a signal to proceed. So signal to check 
	     * for I/O completions.
	     */

	    tmp_chunk->thread_flags |= CFLSH_CHNK_POLL_INTRPT;

	    pthread_rc = pthread_cond_signal(&(tmp_chunk->thread_event));
	
	    if (pthread_rc) {
	    
		CBLK_TRACE_LOG_FILE(1,"pthread_cond_signall failed rc = %d,errno = %d",
				    pthread_rc,errno);

		/*
		 * Ignore error and continue.
		 */
	    }
	}


	CFLASH_BLOCK_UNLOCK(tmp_chunk->lock);

	path = path->next;
    }


    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    CFLASH_BLOCK_LOCK(chunk->lock);

    /*
     * Since we issued the reset and fully processed it,
     * clear our reset flag. 
     */

    chunk->path[path_index]->flags &= ~CFLSH_PATH_A_RST;

    
    chunk->flags &= ~CFLSH_CHNK_RECOV_AFU;

    chunk->recov_thread_id = 0;

    CBLK_TRACE_LOG_FILE(9,"exiting routine for  afu->contxt_id = 0x%llx, pid = 0x%llx",
			chunk->path[path_index]->afu->contxt_id,
			(uint64_t)cflsh_blk.caller_pid);

    return status;
}

#ifdef _AIX
/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_get_program_name
 *                  
 * FUNCTION:  Finds the name of the process associated with our PID.
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
 * RETURNS:    NULL string - No process name found.
 *             string      - Process name found.
 *     
 * ----------------------------------------------------------------------------
 */ 

char *cblk_get_program_name(pid_t pid) 
{

#define MAX_FETCH_PROCS  100

    char *process_name = NULL;
    pid_t process_index = 0;
    int process_list_size;
    int i;
    int num_process;


    
#if defined(__64BIT__)

    /*
     * 64-bit application
     */

    /*
     * NOTE: AIX does not have a mechanism to get a process
     *       name via getproc (the 32-bit version of the
     *       getprocs64 call) for 32-bit applications. This
     *       is due to the reduced size of the 32-bit procinfo
     */

    struct procentry64 *process_list;



    process_list_size = sizeof(*process_list) * MAX_FETCH_PROCS;


    process_list = malloc(process_list_size);

    if (process_list == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Failed to allocate process list of size = %d,with errno = %d",
			    process_list_size,errno);

	return NULL;
    }


    do {


	bzero(process_list,process_list_size);

	num_process = getprocs64(process_list,sizeof(*process_list),
			       NULL,0,&process_index,MAX_FETCH_PROCS);


	if (num_process == 0) {

	    
	    CBLK_TRACE_LOG_FILE(5,"No processes returned from getprocs64. last index = %d",
				process_index);
	    break;
	}

	for (i=0;i < num_process; i++) {

	    if (pid == (pid_t)process_list[i].pi_pid) {

		/*
		 * We found the matching process.
		 * Now let's extract the process'
		 * name.
		 */

		process_name = strdup(process_list[i].pi_comm);
		break;

	    }
	}

	if (process_name) {

	    /*
	     * If we found the process name, then
	     * break out this loop.
	     */

	    break;
	}

	if (num_process <  MAX_FETCH_PROCS) {


	    /*
	     * There are no more process eleents
	     * to fetch.
	     */

	    CBLK_TRACE_LOG_FILE(5,"No more processes left to fetch. last index = %d",
				process_index);
	    break;
	}


    } while (num_process);


    
#endif /* 64-bit */

    CBLK_TRACE_LOG_FILE(5,"Our process name = %s",process_name);


    return process_name;
}

#endif /* _AIX */


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
void cblk_notify_mc_err(cflsh_chunk_t *chunk,  int path_index,int error_num,
			uint64_t out_rc,
			cflash_block_notify_reason_t reason,cflsh_cmd_mgm_t *cmd)
{
#ifdef  _MASTER_CONTXT
#ifdef _AIX
    int rc = 0;
    dk_capi_log_t disk_log;
    scsi_cdb_t *cdb = NULL;
    int process_name_len;
    struct cflash_block_log log_data;




    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path == NULL");
	return;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu == NULL");
	return;
    }
    


    bzero(&log_data,sizeof(log_data));

    /*
     * Build detail data for logging
     */

    log_data.app_indicator = CFLSH_BLK_LIB;

    log_data.errnum = error_num;

    log_data.rc = out_rc;

    log_data.afu_type = chunk->path[path_index]->afu->type;


    /*
     * Fill in global library values
     */



    log_data.cflsh_blk_flags = cflsh_blk.flags;

    log_data.num_active_chunks = cflsh_blk.num_active_chunks;
    log_data.num_max_active_chunks = cflsh_blk.num_max_active_chunks;



    /*
     * Fill in chunk general values
     */

    log_data.chunk_flags = chunk->flags;

    log_data.num_active_cmds = chunk->num_active_cmds;

    log_data.num_cmds = chunk->num_cmds;

    log_data.num_paths = chunk->num_paths;


    /*
     * Fill in path general values
     */

    log_data.path_flags = chunk->path[path_index]->flags;
    log_data.path_index = path_index;


    /*
     * Fill in AFU general values
     */

    log_data.afu_flags = chunk->path[path_index]->afu->flags;
    
    log_data.num_rrqs = chunk->path[path_index]->afu->num_rrqs;
    

    /*
     * Fill in statistics
     */


    log_data.num_act_reads = chunk->stats.num_act_reads;         
                                    
    log_data.num_act_writes = chunk->stats.num_act_writes;        
                                    
    log_data.num_act_areads = chunk->stats.num_act_areads;        
                                    
    log_data.num_act_awrites = chunk->stats.num_act_awrites;       
                                    
    log_data.max_num_act_writes = chunk->stats.max_num_act_writes;    
                                    
    log_data.max_num_act_reads = chunk->stats.max_num_act_reads;     
                                    
    log_data.max_num_act_awrites = chunk->stats.max_num_act_awrites;   
                                    
    log_data.max_num_act_areads = chunk->stats.max_num_act_areads;    

    log_data.num_cc_errors = chunk->stats.num_cc_errors;         
                                    
    log_data.num_afu_errors = chunk->stats.num_afu_errors;        
                                    
    log_data.num_fc_errors = chunk->stats.num_fc_errors;         
                                    
    log_data.num_errors = chunk->stats.num_errors;            

    log_data.num_reset_contexts = chunk->stats.num_reset_contexts;    

    log_data.num_reset_contxt_fails = chunk->stats.num_reset_contxt_fails;
                                    
    log_data.num_path_fail_overs = chunk->stats.num_path_fail_overs;   

    log_data.block_size = chunk->stats.block_size;     
       
    log_data.primary_path_id = chunk->path[0]->path_id;       

    log_data.num_no_cmd_room = chunk->stats.num_no_cmd_room;       
                                    
    log_data.num_no_cmds_free = chunk->stats.num_no_cmds_free;      
                                    
    log_data.num_no_cmds_free_fail = chunk->stats.num_no_cmds_free_fail; 
                                    
    log_data.num_fail_timeouts = chunk->stats.num_fail_timeouts;     
                                    
    log_data.num_capi_adap_chck_err = chunk->stats.num_capi_adap_chck_err;
                                    
    log_data.num_capi_adap_resets = chunk->stats.num_capi_adap_resets;  
                                    
    log_data.num_capi_data_st_errs = chunk->stats.num_capi_data_st_errs; 

    log_data.num_capi_afu_errors = chunk->stats.num_capi_afu_errors;   
                    
    log_data.mmio = (uint64_t)chunk->path[path_index]->afu->mmio;

    log_data.hrrq_start = (uint64_t)chunk->path[path_index]->afu->p_hrrq_start;

    log_data.cmd_start = (uint64_t)chunk->cmd_start;

    if (cmd) {


	cdb = CBLK_GET_CMD_CDB(chunk,cmd);

	if (cdb) {
	    bcopy(cdb,&log_data.failed_cdb,sizeof(*cdb));
	}


	if (cmd->cmdi) {
	
	    log_data.data_ea = (uint64_t)cmd->cmdi->buf;

	}

	log_data.data_len = (uint64_t)cmd->cmdi->nblocks;

	log_data.lba = (uint64_t)cmd->cmdi->lba;

	CBLK_COPY_ADAP_CMD_RESP(chunk,cmd,log_data.data,CFLASH_BLOCK_LOG_DATA_LEN);
	
	
    }


    bzero(&disk_log,sizeof(disk_log));

    /*
     * Log all errors are temporary. Only 
     * treat the adapter/disk error as software errors.
     * All others are treated as hardware errors
     */


    disk_log.flags = DK_LF_TEMP;
    if ((reason == CFLSH_BLK_NOTIFY_DISK_ERR) ||
	(reason == CFLSH_BLK_NOTIFY_ADAP_ERR) ||
	(reason == CFLSH_BLK_NOTIFY_SFW_ERR)) {

	disk_log.flags |= DK_LF_SW_ERR;
    } else {


	disk_log.flags |= DK_LF_HW_ERR;
    }

    disk_log.path_id = chunk->path[path_index]->path_id;
    disk_log.devno = chunk->path[path_index]->afu->adap_devno;

    disk_log.ctx_token = chunk->path[path_index]->afu->contxt_id;
    disk_log.rsrc_handle = chunk->path[path_index]->sisl.resrc_handle;


    if (cflsh_blk.process_name == NULL) {
	cflsh_blk.process_name = cblk_get_program_name(cflsh_blk.caller_pid);
    }

    
    process_name_len = MIN(strlen(cflsh_blk.process_name),DK_LOG_ASCII_SENSE_LEN);

    strncpy(disk_log.ascii_sense_data,cflsh_blk.process_name,process_name_len);


    disk_log.reason = reason;

    bcopy(&log_data,disk_log.sense_data,MIN(DK_LOG_SENSE_LEN,sizeof(log_data)));

	
    CBLK_TRACE_LOG_FILE(5,"Issuing DK_CAPI_LOG_EVENT chunk->dev_name = %s, reason = %d, error_num = 0x%x,process_name = %s",
			chunk->dev_name,reason,error_num,cflsh_blk.process_name);


    rc = ioctl(chunk->path[path_index]->fd,DK_CAPI_LOG_EVENT,&disk_log);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"DISK_CAPI_LOG_EVENT failed for chunk->dev_name = %s,with rc = %d, errno = %d, return_flags = 0x%llx\n",
			    chunk->dev_name,rc,errno,disk_log.return_flags);

    }


#else

    CBLK_TRACE_LOG_FILE(1,"LOG_EVENT reason %d error_num = 0x%x,for chunk->dev_name = %s, chunk index = %d\n",
			reason,error_num,chunk->dev_name,chunk->index);

#endif /* !AIX */


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
 *              0   - Good completion
 *              -1  - Error
 *              
 *              
 */
int cblk_verify_mc_lun(cflsh_chunk_t *chunk,  cflash_block_notify_reason_t reason, 
			  cflsh_cmd_mgm_t *cmd, 
			  struct request_sense_data *sense_data)
{
#ifdef  _MASTER_CONTXT

    int rc = 0;
    dk_capi_verify_t disk_verify;


    if (chunk->path[chunk->cur_path] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path == NULL");
	return -1;
    }

    bzero(&disk_verify,sizeof(disk_verify));
    
#ifdef _AIX
    disk_verify.path_id = chunk->path[chunk->cur_path]->path_id;

#endif

    if (sense_data) {

#ifdef _AIX
	bcopy(sense_data,disk_verify.sense_data,
	      MIN(sizeof(*sense_data),DK_VERIFY_SENSE_LEN));


	disk_verify.hint = DK_HINT_SENSE;
#else
	bcopy(sense_data,disk_verify.sense_data,
	      MIN(sizeof(*sense_data),DK_CXLFLASH_VERIFY_SENSE_LEN));


	disk_verify.hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
	disk_verify.context_id = chunk->path[chunk->cur_path]->afu->contxt_id;
	disk_verify.rsrc_handle = chunk->path[chunk->cur_path]->sisl.resrc_handle;

#endif /* ! AIX */

    }

    
    CBLK_TRACE_LOG_FILE(5,"Issuing DK_CAPI_VERIFY for chunk index = %d\n",
			chunk->index);

    /*
     *  NOTE: For linux we only do the verify on the path
     *        that saw the error. It is assumed the other paths
     *        would either see the same error (such as Unit Attention)
     *        and a do a verify for that path at that time or the other paths
     *        would get an event notification.
     */

    rc = ioctl(chunk->path[chunk->cur_path]->fd,DK_CAPI_VERIFY,&disk_verify);

    if (rc) {
	
	CBLK_TRACE_LOG_FILE(1,"DK_CAPI_VERIFY failed with rc = %d, errno = %d, return_flags = 0x%llx\n",
			    rc,errno,disk_verify.return_flags);


	/*
	 * If verify failed with DK_RF_PATH_LOST, and we are 
	 * running MPIO, then this will also be detected when we
	 * query exceptions for the disk. At that time we'll check
	 * for a valid path and change our primary path to that one.
	 */

	
	chunk->flags |= CFLSH_CHUNK_FAIL_IO;

	/*
	 * Clear halted state so that subsequent commands
	 * do not hang, but are failed immediately.
	 */

	chunk->flags &= ~CFLSH_CHNK_HALTED;


	/*
	 * If verify disk failed, the fall all commands back,
	 * but first we need to ensure the AFU drops them first.
	 */

	cblk_chunk_free_mc_device_resources(chunk);

	cblk_chunk_unmap(chunk,TRUE);

	cblk_chunk_detach(chunk,TRUE);

	close(chunk->fd);


	cblk_fail_all_cmds(chunk);


	return -1;

	
    } else  {

	chunk->num_blocks_lun =  disk_verify.last_lba + 1;

	CBLK_TRACE_LOG_FILE(5,"Last block returned = 0x%llx\n",
			    chunk->num_blocks_lun);

	if (!(chunk->flags & CFLSH_CHNK_VLUN)) {

	    /*
	     * If this chunk represents a physical lun, then update
	     * its number of valid blocks.
	     */

	    chunk->num_blocks = chunk->num_blocks_lun;

	}
    }

#endif /* _MASTER_CONTXT */

    return 0;
}

