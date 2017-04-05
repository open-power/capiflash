
/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_int.c $                                */
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


#define CFLSH_BLK_FILENUM 0x0200
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"

#ifdef BLOCK_FILEMODE_ENABLED
#include <sys/stat.h> 
#endif
#ifdef _AIX
#include <sys/scsi.h>
#endif


char             cblk_filename[PATH_MAX];


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_setup_trace_files
 *                  
 * FUNCTION:  Set up trace files
 *                                                 
 *        
 *     
 *      
 *
 * RETURNS:  NONE
 *     
 * ----------------------------------------------------------------------------
 */
void cblk_setup_trace_files(int new_process)
{
    int i;
    char *env_verbosity = getenv("CFLSH_BLK_TRC_VERBOSITY");
    char *env_use_syslog = getenv("CFLSH_BLK_TRC_SYSLOG");
    char *env_trace_append = getenv("CFLSH_BLK_TRC_APPEND");
    char *log_pid  = getenv("CFLSH_BLK_TRACE_PID");
    char *env_num_thread_logs  = getenv("CFLSH_BLK_TRACE_TID");
    char *env_user     = getenv("USER");
    uint32_t thread_logs = 0;
    char filename[PATH_MAX];
    char *filename_ptr = filename;
    int open_flags = 0;


#ifndef _AIX
    cflsh_blk.flags |= CFLSH_G_SYSLOG;

    if (env_use_syslog) {

	if (strcmp(env_use_syslog,"ON")) {

	    /*
	     * Don't use syslog tracing. Instead 
	     * use tracing to a file.
	     */
	    cflsh_blk.flags &= ~CFLSH_G_SYSLOG;


	    /*
	     * By default for linux, enable appending of
	     * of log file if someone turns off the default
	     * of syslog.
	     */

	    open_flags |= TRACE_LOG_EXT_OPEN_APPEND_FLG;
	}
    }
#endif /* !_AIX */




    if (new_process)  {
	
	if ((log_pid == NULL) ||
	    (cflsh_blk.flags & CFLSH_G_SYSLOG)) {
	    
	    /*	
	     * If this is a new process (forked process)
	     * and we are not using traces per process,
	     * or we are logging via syslog
	     * then continue to the use the tracing 
	     * in place for the parent process.
	     */
	    
	    return;
	}

	strcpy(filename,cblk_log_filename);
    }

    if (env_trace_append) {


	if (!strcmp(env_trace_append,"ON")) {

	    /*
	     * Append to existing trace file. The default
	     * it to truncate and overwrite.
	     */
	    open_flags |= TRACE_LOG_EXT_OPEN_APPEND_FLG;
	} else {

	    open_flags &= ~TRACE_LOG_EXT_OPEN_APPEND_FLG;
	}

    }

    if (env_verbosity) {
	cblk_log_verbosity = atoi(env_verbosity);
	
    } else {
#ifdef _AIX
	cblk_log_verbosity = 0;
#else
	cblk_log_verbosity = 1;
#endif
    }

    cblk_log_filename = getenv("CFLSH_BLK_TRACE");
    if (cblk_log_filename == NULL)
    {
        sprintf(cblk_filename, "/tmp/%s.cflash_block_trc", env_user);
        cblk_log_filename = cblk_filename;
    }

    if ((log_pid) && !(cflsh_blk.flags & CFLSH_G_SYSLOG)) {
	
	/*
	 * Use different filename for each process, when
	 * not using syslogging.
	 */

	sprintf(cblk_filename,"%s.%d",cblk_log_filename,getpid());

	if ((new_process) &&
	    !strcmp(cblk_log_filename,filename)) {

	    /*
	     * If this is a new process (forked process)
	     * and the process trace filename is same as before,
	     * then return here, since we are already set up.
	     * This situation can occur if there are multiple chunks
	     * that are cloned after a fork. Only the first
	     * one would change the trace file.
	     */

	    return;
	}

	cblk_log_filename = cblk_filename;

    }

    bzero((void *)&(cflsh_blk.trace_ext),sizeof(trace_log_ext_arg_t));

    /*
     * We need to serialize access to this log file
     * while we are setting it up.
     */

    pthread_mutex_lock(&cblk_log_lock);

    if (cflsh_blk.flags & CFLSH_G_SYSLOG) {

	openlog("CXLBLK",LOG_PID,LOG_USER);


    } else if (cblk_log_verbosity) {

	if (setup_trace_log_file_ext(&cblk_log_filename,&cblk_logfp,cblk_log_filename,open_flags)) {

	    //fprintf(stderr,"Failed to set up tracing for filename = %s\n",cblk_log_filename);

	    /*
	     * Turn off tracing if this fails.
	     */
	    cblk_log_verbosity = 0;
	}
    }



    if ((env_num_thread_logs) && !(cflsh_blk.flags & CFLSH_G_SYSLOG)) {

	/*
	 * This indicates they want a trace log file per thread
	 * and we are not using syslog.
	 * We will still trace all threads in one common file,
	 * but also provide a thread log per thread too.
	 */

	if ((new_process) && (num_thread_logs)) {

	    /*
	     * If this is a new process (i.e. forked
	     * process), then we need to free up
	     * the resources from parent first.
	     */

	    free(cflsh_blk.thread_logs);
	}

	num_thread_logs = atoi(env_num_thread_logs);

	num_thread_logs = MIN(num_thread_logs, MAX_NUM_THREAD_LOGS);

	if (num_thread_logs) {

	    /*
	     * Allocate there array of thread_log file pointers:
	     */

	    cflsh_blk.thread_logs = (cflsh_thread_log_t *) malloc(num_thread_logs * sizeof(cflsh_thread_log_t));

	    if (cflsh_blk.thread_logs) {

		bzero((void *)cflsh_blk.thread_logs, num_thread_logs * sizeof(cflsh_thread_log_t));


		for (i=0; i< num_thread_logs;i++) {

		    sprintf(filename,"%s.%d",cblk_log_filename,i);

		    
		    if (setup_trace_log_file(&filename_ptr,&cflsh_blk.thread_logs[i].logfp,filename)) {

			fprintf(stderr,"Failed to set up tracing for filename = %s\n",filename);
			free(cflsh_blk.thread_logs);

			num_thread_logs = 0;
			break;
		    }
		    
		    cflsh_blk.thread_logs[i]. ext_arg.flags |= TRACE_LOG_NO_USE_LOG_NUM;

		} /* for */

		/*
		 * We need to create a mask to allow us to hash
		 * thread ids into the our various thread log files.
		 * Thus we need mask that is based on the number_thread_log 
		 * files.  We'll create a mask that is contains a 1 for
		 * every bit up to the highest bit used to represent the number
		 * thread log files.
		 */

		thread_logs = num_thread_logs;
		cflsh_blk.thread_log_mask = 0;

		while (thread_logs) {

		    cflsh_blk.thread_log_mask = (cflsh_blk.thread_log_mask << 1) | 1;
		    thread_logs >>=  1;

		} /* while */

	    } else {

		/*
		 * If we fail to allocate the thread trace log, then
		 * set num_thread_logs back to 0.
		 */
		num_thread_logs = 0;
	    }
	}
	
    }

    pthread_mutex_unlock(&cblk_log_lock);


    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_valid_endianess
 *                  
 * FUNCTION:  Determines the Endianess of the host that
 *            the binary is running on.
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
 * RETURNS:  1  Host endianess matches compile flags
 *           0  Host endianess is invalid based on compile flags
 *     
 * ----------------------------------------------------------------------------
 */
int cblk_valid_endianess(void)
{
    int rc = FALSE;
    short test_endian = 0x0102;
    char  *ptr;
    char  byte;

    ptr = (char *) &test_endian;

    byte = ptr[0];

    if (byte == 0x02) {
        
        /*
         * In a Little Endian host, the first indexed
         * byte will be 0x2
         */
#ifdef CFLASH_LITTLE_ENDIAN_HOST
	rc = TRUE;
#else
	rc = FALSE;
#endif /* !CFLASH_LITTLE_ENDIAN_HOST */
	

    } else {
        
        /*
         * In a Big Endian host, the first indexed
         * byte will be 0x1
         */

#ifdef CFLASH_LITTLE_ENDIAN_HOST
	rc = FALSE;
#else
	rc = TRUE;
#endif /* !CFLASH_LITTLE_ENDIAN_HOST */
	

    }



    return rc;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_get_host_type
 *                  
 * FUNCTION:  Determine if this OS is running on Bare Metal, pHyp
 *            or PowerKVM.
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


void cblk_get_host_type(void)
{
#ifdef _AIX

    /*
     * For AIX always assume pHyp platform type
     */

    cflsh_blk.host_type = CFLASH_HOST_PHYP;
    return;

#else
#define CFLASH_BLOCK_HOST_BUF_SIZE   256
    FILE *fp;
    char buf[CFLASH_BLOCK_HOST_BUF_SIZE];
    char *platform_line;

    /*
     * Set host type to unknown until we had a chance
     * to deterine otherwise.
     */

    cflsh_blk.host_type = CFLASH_HOST_UNKNOWN;

    if ((fp = fopen(CFLASH_BLOCK_HOST_TYPE_FILE,"r")) == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Can not determine host type, failed to open = %s",
			    CFLASH_BLOCK_HOST_TYPE_FILE);
	return;
    }


    
    while (fgets(buf,CFLASH_BLOCK_HOST_BUF_SIZE,fp)) {


	/*
	 * The /proc/cpuinfo file will have a line
	 * that starts with the format:
	 *
	 *     platform    :
	 *
	 * after the ":" will be the platform/host type
	 *
	 * So we first search for platform, then extract out 
	 * platform/host type.
	 */

	platform_line = strstr(buf,"platform");

	if ((platform_line) &&
	     (strstr(platform_line,":")) ) {

	    CBLK_TRACE_LOG_FILE(9,"Platform_line = %s",platform_line);


	    /*
	     * NOTE: The order below is import, because two of the 
	     * platform strings start with pSeries.
	     */

	    if (strstr(platform_line,"PowerNV")) {

		CBLK_TRACE_LOG_FILE(9,"BML host type");
		cflsh_blk.host_type = CFLASH_HOST_NV;

		break;
	    } else if (strstr(platform_line,"pSeries (emulated by qemu)")) {

		CBLK_TRACE_LOG_FILE(9,"PowerKVM host type");
		cflsh_blk.host_type = CFLASH_HOST_KVM;

	    } else if (strstr(platform_line,"pSeries")) {

		CBLK_TRACE_LOG_FILE(9,"pHyp host type");
		cflsh_blk.host_type = CFLASH_HOST_PHYP;

	    } else {
		CBLK_TRACE_LOG_FILE(1,"Unknown platform string= %s",
			    platform_line);
	    }

	    break;


	}

    }

    if (cflsh_blk.host_type == CFLASH_HOST_UNKNOWN) {

	CBLK_TRACE_LOG_FILE(9,"could not determine host type");
    }


    fclose (fp);
    
    return;
#endif
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_sigsev_handler
 *                  
 * FUNCTION:  Since a failing CAPI adapter, can generate SIGSEV
 *            for a now invalid MMIO address, let us collect some
 *            debug information here in this SIGSEGV hanndler
 *            to determine this.
 *                                                 
 *                                                                         
 *
 *      
 *
 * RETURNS:  NONE 
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_chunk_sigsev_handler (int signum, siginfo_t *siginfo, void *uctx)
{
    cflsh_chunk_t *chunk;
    int i,j;

    CBLK_TRACE_LOG_FILE(1,"si_code = %d, si_addr = 0x%p",
			siginfo->si_code,siginfo->si_addr);

    switch (siginfo->si_code) {
#ifdef _AIX
      case SEGV_MAPERR: 
	  CBLK_TRACE_LOG_FILE(1,"Address not mapped, address = 0x%p",
			      siginfo->si_addr);

	break;
#endif /* _AIX */
      case SEGV_ACCERR: 

	CBLK_TRACE_LOG_FILE(1,"Invalid permissions, address = 0x%p",
			    siginfo->si_addr);

	break;
      default:

	CBLK_TRACE_LOG_FILE(1,"Unknown si_code = %d, address = 0x%p",
		siginfo->si_code,siginfo->si_addr);
    }
    

    for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;
    

	while (chunk) {


	    for (j=0; j < chunk->num_paths;j++) {
		if ((chunk->flags & CFLSH_CHNK_SIGH) &&
		    (chunk->path[j]) &&
		    (chunk->path[j]->afu) &&
		    (chunk->path[j]->afu->mmio <= siginfo->si_addr) &&
		    (chunk->path[j]->upper_mmio_addr >= siginfo->si_addr)) {

		    longjmp(chunk->path[j]->jmp_mmio,1);
		}

	    } /* for */

	    chunk = chunk->next;

	} /* while */


    } /* for */

    
    /*
     * If we get here then SIGSEGV is mostly
     * likely not associated with a bad MMIO
     * address (due to adapter reset or 
     * UE. Issue default signal.
     */

    signal(signum,SIG_DFL);
    kill(getpid(),signum);

    return;
}

/*
 * NAME:        cblk_reopen_after_fork
 *
 * FUNCTION:    After a fork, this routine reuses an existing chunk
 *              for a physical lun and reopens it for use by the child.
 *
 * NOTES:       Assumes caller as the cflsh_blk.global_lock.
 *
 * INPUTS:
 *              path - Path of device to open
 *          
 *              flags - Flags for open
 *
 * RETURNS:
 *              None
 *              
 */

void cblk_reopen_after_fork(cflsh_chunk_t *chunk, int flags)
{
    int cleanup_depth;
#ifndef BLOCK_FILEMODE_ENABLED
    int mode = 0;
    int i;
#endif /* BLOCK_FILEMODE_ENABLED */
    cflsh_cmd_info_t *cmdi;
    cflsh_cmd_info_t *next_cmdi;
    
    errno = 0;
    
    /*
     * Reopen after fork only works for physical luns. 
     */
    
    if (chunk->flags & CFLSH_CHNK_VLUN) {
	
	
	
        CBLK_TRACE_LOG_FILE(1,"virtual lun attempted to reopen after fork flags = 0x%x",
			    chunk->flags);
        return;
        
    }

    CFLASH_BLOCK_LOCK(chunk->lock);
    
    CBLK_TRACE_LOG_FILE(9,"reopening for %s starting",chunk->dev_name);
    /*
     * Clear some statistics. We can not clear
     * all stats, because the code below will not
     * correctly reset some of them. Also the num_paths
     * is correct initially until we clean up below and
     * then it will get reset to the current number of paths.
     */

    chunk->stats.num_active_threads = 0;
    chunk->stats.num_reads = 0;
    chunk->stats.num_areads = 0;
    chunk->stats.num_writes = 0;
    chunk->stats.num_awrites = 0;


    cmdi = chunk->head_act;

    while (cmdi) {

	next_cmdi = cmdi->act_next;


	CBLK_FREE_CMDI(chunk,cmdi);
	    

	cmdi = next_cmdi;
    }

#ifndef BLOCK_FILEMODE_ENABLED

    if (chunk->flags & (CFLSH_CHNK_RD_AC|CFLSH_CHNK_WR_AC))  {
	mode = O_RDWR;

    } else if (chunk->flags & CFLSH_CHNK_RD_AC)  {

	mode = O_RDONLY;
    } else if (chunk->flags & CFLSH_CHNK_WR_AC)  {

	mode = O_WRONLY;
    }

    /*
     * Unmap all mmio space in use
     */

    cblk_chunk_unmap(chunk,FALSE);

    

    CBLK_TRACE_LOG_FILE(9,"reopening successful for %s",chunk->dev_name);
    cleanup_depth = 30;


    for (i=0;i < chunk->num_paths;i++) {


	if (cblk_chunk_attach_process_map_path(chunk,i,mode,
					       chunk->path[i]->afu->adap_devno,
					       chunk->dev_name,
					       chunk->path[i]->fd,0,
					       &cleanup_depth,FALSE)) {

	    CBLK_TRACE_LOG_FILE(1,"Unable to attach errno = %d",errno);
	    chunk->path[i]->flags &= ~CFLSH_PATH_ACT;

	    chunk->flags |= CFLSH_CHUNK_FAIL_IO;

	    return;
	}
    }
    
    CBLK_TRACE_LOG_FILE(9,"reopening attach/map successful for %s",chunk->dev_name);
    
#endif /* ! BLOCK_FILEMODE_ENABLED */

    cleanup_depth = 40;
    
#ifdef _COMMON_INTRPT_THREAD
    
    
    if (cblk_start_common_intrpt_thread(chunk)) {
	
	
	CBLK_TRACE_LOG_FILE(1,"cblk_start_common_intrpt thread failed with errno= %d",
			    errno);
	
	
	cblk_chunk_open_cleanup(chunk,cleanup_depth);
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	
	free(chunk);
	
	
	return;
    }
    
    cleanup_depth = 45;
#endif 
    
#ifndef BLOCK_FILEMODE_ENABLED
    CBLK_TRACE_LOG_FILE(9,"started background interrupt thread for child for %s",chunk->dev_name);
    if (cblk_chunk_get_mc_device_resources(chunk,&cleanup_depth)) {
	
	
	CBLK_TRACE_LOG_FILE(5,"cblk_get_device_info failed errno = %d",
			    errno);
	
	cblk_chunk_open_cleanup(chunk,cleanup_depth);
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	
	free(chunk);
	
	
	return;
	
    }
#endif /* ! BLOCK_FILEMODE_ENABLED */

    CFLASH_BLOCK_UNLOCK(chunk->lock);

    CBLK_TRACE_LOG_FILE(5,"reopening for %s completed",chunk->dev_name);

    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_prepare_fork
 *                  
 * FUNCTION:  If a process using this library does a fork, then 
 *            this routine will be invoked
 *            prior to fork to the library into a consistent state
 *            that will be preserved across fork.
 *                                                 
 *                                                                         
 *
 *      
 *
 * RETURNS:  NONE 
 *     
 * ----------------------------------------------------------------------------
 */ 
void cblk_prepare_fork (void)
{
    cflsh_chunk_t *chunk = NULL;
    cflsh_afu_t *afu = NULL;
    int i;


    pthread_mutex_lock(&cblk_log_lock);

    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);


    for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;
    

	while (chunk) {

	    CFLASH_BLOCK_LOCK(chunk->lock);
	    chunk = chunk->next;

	} /* while */


    } /* for */

    afu = cflsh_blk.head_afu;


    while (afu) {


	CFLASH_BLOCK_LOCK(afu->lock);

	afu = afu->next;
    }
    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_parent_post_fork
 *                  
 * FUNCTION:  If a process using this library does a fork, then 
 *            this  routine will be run on the parent after fork
 *            to release locks.
 *                                                 
 *                                                                         
 *
 *      
 *
 * RETURNS:  NONE 
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_parent_post_fork (void)
{
    cflsh_chunk_t *chunk = NULL;
    cflsh_afu_t *afu = NULL;
    int i;
    int rc;



    for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;
    

	while (chunk) {

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    chunk = chunk->next;

	} /* while */


    } /* for */


    afu = cflsh_blk.head_afu;


    while (afu) {


	CFLASH_BLOCK_UNLOCK(afu->lock);

	afu = afu->next;
    }


    rc = pthread_mutex_unlock(&cblk_log_lock);

    if (rc) {

	/*
	 * Find the first chunk do a notify
	 * against it.
	 */

	for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	    chunk = cflsh_blk.hash[i].head;
    

	    if (chunk) {

		break;
	    }


	} /* for */
	
	if (chunk) {

	    cblk_notify_mc_err(chunk,0,0x205,rc, CFLSH_BLK_NOTIFY_SFW_ERR,NULL);

	}
    }
    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
    return;
}



/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_child_post_fork
 *                  
 * FUNCTION:  If a process using this library does a fork, then 
 *            this  routine will be run on the child after fork
 *            to release locks.
 *                                                 
 *                                                                         
 *
 *      
 *
 * RETURNS:  NONE 
 *     
 * ----------------------------------------------------------------------------
 */ 
void  cblk_child_post_fork (void)
{
    cflsh_chunk_t *chunk = NULL;
    cflsh_afu_t *afu = NULL;
    int i;
    int rc;


    for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;
    

	while (chunk) {

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    chunk = chunk->next;

	} /* while */


    } /* for */


    afu = cflsh_blk.head_afu;


    while (afu) {


	afu->num_issued_cmds = 0;
	afu->cmd_room = 0;
	CFLASH_BLOCK_UNLOCK(afu->lock);

	afu = afu->next;
    }

    rc = pthread_mutex_unlock(&cblk_log_lock);

    if (rc) {


	/*
	 * Find the first chunk do a notify
	 * against it.
	 */

	for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {

	    chunk = cflsh_blk.hash[i].head;
    

	    if (chunk) {

		break;
	    }


	} /* for */

	if (chunk) {

	    cblk_notify_mc_err(chunk,0,0x206,rc, CFLSH_BLK_NOTIFY_SFW_ERR,NULL);

	}
    } else {
	
	/*
	 * Since we forked the child, get its new PID
	 * and clear its old process name if known.
	 */
	cflsh_blk.caller_pid = getpid();
	
	
	if (cflsh_blk.process_name) {
	    
	    free(cflsh_blk.process_name);
	}
	
	/*
	 * Since we forked the child, if we have tracing turned
	 * on for a trace file per process, then we need to
	 * open the new file for this child's PID.  The routine
	 * cblk_setup_trace_files will handle the situation
	 * where multiple chunks are cloned and using the same
	 * new trace file.
	 */
	
	cblk_setup_trace_files(TRUE);
	
	CBLK_TRACE_LOG_FILE(9,"child_fork");


	for (i=0; i < MAX_NUM_CHUNKS_HASH; i++) {
	    
	    chunk = cflsh_blk.hash[i].head;
	    
	    
	    while (chunk) {
		
		if (!(chunk->flags & CFLSH_CHNK_VLUN)) {
		    
		    /*
		     * Only handle physical luns here. Virtual luns
		     * are handled separately with an explicit call
		     * to cblk_clone_after_fork. 
		     *
		     * For physical luns we will reuse the chunk.
		     */
		    
		    cblk_reopen_after_fork(chunk,0);
		}

		chunk = chunk->next;
		
	    } /* while */
	    
	    
	} /* for */


 
	CBLK_TRACE_LOG_FILE(9,"child_fork");
    }

	

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    return;
}





/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_init_cache
 *                  
 * FUNCTION:  Initialize cache for a chunk
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
void  cblk_chunk_init_cache (cflsh_chunk_t *chunk, size_t nblocks)
{
    cflsh_cache_line_t  *line;
    uint                n;
    
    
    if (chunk == NULL) {
	
	return;
    }
    
    if (nblocks == 0) {
	
	
	return;
    }
    

    chunk->cache_size = MIN(nblocks,cblk_cache_size);

    if (chunk->cache_size == 0) {

	return;
    }


    CBLK_TRACE_LOG_FILE(5,"cache_size",chunk->cache_size);

    chunk->cache = (cflsh_cache_line_t *) malloc(chunk->cache_size * sizeof(cflsh_cache_line_t));
    
    if (chunk->cache == (cflsh_cache_line_t *) NULL) {

	CBLK_TRACE_LOG_FILE(1,"Could not allocate cache with size = %d\n",
			    chunk->cache_size);
	fprintf (stderr,
		 "Could not allocate cache with size = %d\n",
		 chunk->cache_size);
	return;
    }
    
    bzero(chunk->cache,(chunk->cache_size * sizeof(cflsh_cache_line_t)));

    chunk->cache_buffer = NULL;
    if ( posix_memalign((void *)&(chunk->cache_buffer),4096,
			(CAPI_FLASH_BLOCK_SIZE * chunk->cache_size))) {
		
	CBLK_TRACE_LOG_FILE(1,"posix_memalign failed cache_size = %d,errno = %d",
			    chunk->cache_size,errno);
	

	free(chunk->cache);

	chunk->cache = NULL;

	return;

	
    }

    bzero(chunk->cache_buffer,(CAPI_FLASH_BLOCK_SIZE * chunk->cache_size));

    for (line = chunk->cache; line < &chunk->cache[chunk->cache_size]; line++) {
	for (n = 0; n < CFLSH_BLK_NSET; n++) {
	    
	    line->entry[n].data = chunk->cache_buffer + (n * CAPI_FLASH_BLOCK_SIZE);
		
	    line->entry[n].valid = 0;
	    line->entry[n].next = n + 1;
	    line->entry[n].prev = n - 1;
	}
    }

    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_free_cache
 *                  
 * FUNCTION:  free cache for a chunk
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
void  cblk_chunk_free_cache (cflsh_chunk_t *chunk)
{
    
    if (chunk == NULL) {
	
	return;
    }
    
    if (chunk->cache_size == 0) {
	
	
	return;
    }
    
    
    CBLK_TRACE_LOG_FILE(5,"cache_size",chunk->cache_size);

    free(chunk->cache_buffer);

    free(chunk->cache);

    return;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_chunk_flush_cache
 *                  
 * FUNCTION:  Flush a chunk's cache.
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
void cblk_chunk_flush_cache (cflsh_chunk_t *chunk)
{
    cflsh_cache_line_t *line;
    int             n;
    

	
    CBLK_TRACE_LOG_FILE(5,"cache_size",chunk->cache_size);

    for (line = chunk->cache; line < &chunk->cache[chunk->cache_size]; line++) {
	for (n = 0; n < CFLSH_BLK_NSET; n++) {
	    line->entry[n].valid = 0;
	}
    }
    
    
    return;
}


/*
 * NAME:        cblk_get_chunk_type
 *
 * FUNCTION:    Returns chunk type of the specified device.
 *              
 *
 *
 *
 * INPUTS:
 *              device path name
 *
 * RETURNS:
 *              command type
 *              
 */
cflsh_block_chunk_type_t cblk_get_chunk_type(const char *path, int arch_type)
{
    
    cflsh_block_chunk_type_t chunk_type;
#ifdef BLOCK_FILEMODE_ENABLED
    char *env_arch_type = getenv("CFLSH_BLK_ARCH_TYPE");


    if (env_arch_type) {

	chunk_type = atoi(env_arch_type);

	return chunk_type;
    } 
#endif /* BLOCK_FILEMODE_ENABLED */


    if (arch_type) {

	/*
	 * If architecture type is set, then 
	 * evaluate it for the specific OS..
	 */

	return (cblk_get_os_chunk_type(path,arch_type));

    }



    /*
     * For now we only support one chunk type:
     * the SIS lite type.
     */

    chunk_type = CFLASH_BLK_CHUNK_SIS_LITE;

    return chunk_type;
}


/*
 * NAME:        cblk_set_fcn_ptrs
 *
 * FUNCTION:    Sets function pointers for a chunk based on
 *              it chunk type.
 *              
 *
 *
 *
 * INPUTS:
 *              chunk
 *
 * RETURNS:
 *              0 - Good completion
 *                  Otherwise an erro.
 *              
 */
int  cblk_set_fcn_ptrs(cflsh_path_t *path)
{
    int rc = 0;

    if (path == NULL) {

	errno = EFAULT;

	return -1;
    }

    switch (path->type) {
    case CFLASH_BLK_CHUNK_SIS_LITE:
    case CFLASH_BLK_CHUNK_SIS_LITE_SQ:

	/*
	 * SIS Lite adapter/AFU type
	 */

	rc = cblk_init_sisl_fcn_ptrs(path);

	break;

    default:
	errno = EINVAL;

	rc = -1;

    }
    
    return rc;
}


/*
 * NAME:        cblk_alloc_hrrq_afu
 *
 * FUNCTION:    This routine allocates and initializes
 *              the RRQ per AFU.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0        - Success
 *              nonzero  - Failure
 *              
 */
int cblk_alloc_hrrq_afu(cflsh_afu_t *afu, int num_cmds)
{
    int rc = 0;


    if (num_cmds == 0) {

	CBLK_TRACE_LOG_FILE(1,"num_cmds = 0");

	return -1;

    }

    if (afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"No AFU provided");

	return -1;

    }


    if (afu->p_hrrq_start) {

	//TODO:?? CBLK_TRACE_LOG_FILE(5,"RRQ allocated already");

	// TODO:?? Maybe this should be failed eventually.
	return 0;
    }

    /*
     * Align start/end RRQ on cacheline boundary. 
     *
     * NOTE: We will pad out the end of the rrq to     
     *       to the next cacheline. This is needed
     *       in case we encounter a UE on the RRQ.
     *       The dcbz interface to clear the poison bits
     *       works only on cachelines. Thus we can not
     *       allow someone else to potentiallly share the
     *       cacheline with the last RRQ entry pointers.
     */

    afu->size_rrq = sizeof(*(afu->p_hrrq_start)) * num_cmds;

    
    if ((afu->size_rrq) % CBLK_CACHE_LINE_SIZE ) {


	afu->size_rrq = ((afu->size_rrq)/CBLK_CACHE_LINE_SIZE  + 1) * CBLK_CACHE_LINE_SIZE;
    }



    if ( posix_memalign((void *)&(afu->p_hrrq_start),CBLK_CACHE_LINE_SIZE,afu->size_rrq)) {

		    
	CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for rrq errno= %d, size_rrq = %d",errno,afu->size_rrq);

	return -1;

    }

    bzero((void *)afu->p_hrrq_start ,afu->size_rrq);


    afu->p_hrrq_end = afu->p_hrrq_start + (num_cmds - 1);

    afu->p_hrrq_curr = afu->p_hrrq_start;


    /*
     * Since the host RRQ is
     * bzeroed. The toggle bit in the host
     * RRQ that initially indicates we
     * have a new RRQ will need to be 1.
     */


    afu->toggle = 1;

    return rc;
}


/*
 * NAME:        cblk_free_hrrq_afu
 *
 * FUNCTION:    This routine frees the 
 *              RRQ per AFU.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0        - Success
 *              nonzero  - Failure
 *              
 */
int cblk_free_hrrq_afu(cflsh_afu_t *afu)
{


    if (afu == NULL) {

	return 0;

    }


    if (afu->p_hrrq_start == NULL) {

	return 0;
    }


    free(afu->p_hrrq_start);

    afu->p_hrrq_start = NULL;
    afu->p_hrrq_end = NULL;

    return 0;
}



/*
 * NAME:        cblk_alloc_sq_afu
 *
 * FUNCTION:    This routine allocates and initializes
 *              the SQ (submission queue) per AFU.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0        - Success
 *              nonzero  - Failure
 *              
 */
int cblk_alloc_sq_afu(cflsh_afu_t *afu, int num_cmds)
{
    int rc = 0;


    if (num_cmds == 0) {

	CBLK_TRACE_LOG_FILE(1,"num_cmds = 0");

	return -1;

    }

    if (afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"No AFU provided");

	return -1;

    }


    if (afu->p_sq_start) {

	//TODO:?? CBLK_TRACE_LOG_FILE(5,"SQ allocated already");

	// TODO:?? Maybe this should be failed eventually.
	return 0;
    }

    /*
     * Align start SQ on page boundary and end of SQ on
     * cache line boundary.
     *
     * NOTE: We will pad out the end of the sq to     
     *       to the next cacheline. This is being
     *       done to prevent the end of the SQ from being
     *       shared by some host memory that could be written
     *       from the AFU. For that case then that cache
     *       line could potentially encounter a UE. Thus
     *       we can not share the cache line of the last
     *       SQ entry.
     */

    afu->size_sq = sizeof(*(afu->p_sq_start)) * (num_cmds + 1);

    
    if ((afu->size_sq) % CBLK_CACHE_LINE_SIZE ) {


	afu->size_sq = ((afu->size_sq)/CBLK_CACHE_LINE_SIZE  + 1) * CBLK_CACHE_LINE_SIZE;
    }



    if ( posix_memalign((void *)&(afu->p_sq_start),4096,afu->size_sq)) {

		    
	CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for rrq errno= %d, size_rrq = %d",errno,afu->size_sq);

	return -1;

    }

    bzero((void *)afu->p_sq_start ,afu->size_sq);


    afu->p_sq_end = afu->p_sq_start + (num_cmds);

    afu->p_sq_curr = afu->p_sq_start;



    return rc;
}


/*
 * NAME:        cblk_free_sq_afu
 *
 * FUNCTION:    This routine frees the 
 *              SQ (submission queu)  per AFU.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0        - Success
 *              nonzero  - Failure
 *              
 */
int cblk_free_sq_afu(cflsh_afu_t *afu)
{


    if (afu == NULL) {

	return 0;

    }


    if (afu->p_sq_start == NULL) {

	return 0;
    }


    free(afu->p_sq_start);

    afu->p_sq_start = NULL;
    afu->p_sq_end = NULL;

    return 0;
}


/*
 * NAME:        cblk_get_afu
 *
 * FUNCTION:    This routine checks if an AFU structure
 *              already exists for the specified AFU. Otherwise
 *              it will allocate one.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *              For MPIO we allow sharing of AFUs from the same
 *              same chunk but different paths to the same disk.
 *              Otherwise sharing of contexts is needed.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */

cflsh_afu_t *cblk_get_afu(cflsh_path_t *path, char *dev_name, dev64_t adap_devno,cflsh_block_chunk_type_t type,int num_cmds,
			  cflsh_afu_in_use_t *in_use, int share)
{
    cflsh_afu_t *afu = NULL;
    char *afu_name = NULL;
    cflsh_path_t *tmp_path;
    int mpio_shared;                        /* MPIO shared AFU */
    int pthread_rc;



    if (in_use == NULL) {

	return NULL;
    }


    afu_name = cblk_find_parent_dev(dev_name);

    *in_use = CFLASH_AFU_NOT_INUSE;

    afu = cflsh_blk.head_afu;


    while (afu) {
	
#ifdef _AIX


	if ((adap_devno) &&
	    (afu->adap_devno == adap_devno) &&
	    (afu->type == type)) {
	    

	    
	    tmp_path = afu->head_path;

	    mpio_shared = FALSE;

	    while (tmp_path) {

		if (tmp_path->chunk == path->chunk) {

		    
		    mpio_shared = TRUE;
		    break;
		}
		tmp_path = tmp_path->next;
	    }


	    if (mpio_shared) {


		afu->ref_count++;
	    
		*in_use = CFLASH_AFU_MPIO_INUSE;

		break;
	    } else if ((share) &&
		       (afu->flags & CFLSH_AFU_SHARED)) {

		afu->ref_count++;
	    
		*in_use = CFLASH_AFU_SHARE_INUSE;

		break;
	    }
	}
	
#else

#ifdef _KERNEL_MASTER_CONTXT

	if (afu_name == NULL) {

	    /*
	     * Fail if there is no AFU name
	     */

	    return NULL;
	}
#endif /* _KERNEL_MASTER_CONTXT */

	if ((share) &&
	    (afu_name) &&
	    (!strcmp(afu->master_name,afu_name)) &&
	    (afu->flags & CFLSH_AFU_SHARED) &&
	    (afu->type == type)) {
	    
	    
	    afu->ref_count++;
	    
	    *in_use = CFLASH_AFU_SHARE_INUSE;
	    break;
	}

	
	if ((afu_name) &&
	    (!strcmp(afu->master_name,afu_name)) &&
	    (afu->type == type)) {
	    
	    
	    
	    tmp_path = afu->head_path;

	    mpio_shared = FALSE;

	    while (tmp_path) {

		if (tmp_path->chunk == path->chunk) {

		    
		    mpio_shared = TRUE;
		}
		tmp_path = tmp_path->next;
	    }

	    if (mpio_shared) {


		afu->ref_count++;
	    
		*in_use = CFLASH_AFU_MPIO_INUSE;

		break;
	    } else if ((share) &&
		       (afu->flags & CFLSH_AFU_SHARED)) {

		afu->ref_count++;
	    
		*in_use = CFLASH_AFU_SHARE_INUSE;

		break;
	    }


	}
#endif
	    
	    

	afu = afu->next;
    }

    if (afu == NULL) {

	/*
	 * If no path was found, then
	 * allocate one.
	 */

	/*
	 * Align on 4K boundary to make it easier
	 * for debug purposes.
	 */

	if ( posix_memalign((void *)&afu,4096,
			    (sizeof(*afu)))) {

		    
	    CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for afu, errno= %d",errno);
	


	    afu = NULL;

	} else {

	    
	    bzero((void *) afu,sizeof (*afu));
	    afu->ref_count++;

	    if (share) {

		afu->flags |= CFLSH_AFU_SHARED;
	    }

	    afu->type = type;

	    afu->num_rrqs = num_cmds;

#ifdef _AIX
	    afu->adap_devno = adap_devno;
#endif /* AIX */

	    if (cblk_alloc_hrrq_afu(afu,num_cmds)) {
		free(afu);

		return NULL;
	    }

	    if (type == CFLASH_BLK_CHUNK_SIS_LITE_SQ) {

		/*
		 * This AFU is using a Submission Queue (SQ)
		 * to issue IOARCBs to it. Let's allocate
		 * the SQ now.
		 */


		if (cblk_alloc_sq_afu(afu,num_cmds)) {


		    cblk_free_hrrq_afu(afu);
		    free(afu);

		    return NULL;
		}

		afu->flags |= CFLSH_AFU_SQ;

		afu->hrrq_to_ioarcb_off = sizeof(sisl_ioarcb_t);
	    }

	    pthread_rc = pthread_cond_init(&(afu->resume_event),NULL);
    
	    if (pthread_rc) {
	    
		CBLK_TRACE_LOG_FILE(1,"pthread_cond_init failed for resume_event rc = %d errno= %d",
				    pthread_rc,errno);

		cblk_free_hrrq_afu(afu);
		free(afu);

		return NULL;

	    }




	    if (afu_name) {
		strncpy(afu->master_name,afu_name,MIN(strlen(afu_name),
						      PATH_MAX));
	    }

	    CFLASH_BLOCK_LOCK_INIT(afu->lock);

	    afu->eyec = CFLSH_EYEC_AFU;

	    path->afu_share_type = *in_use;
	    CBLK_Q_NODE_TAIL(cflsh_blk.head_afu,cflsh_blk.tail_afu,afu,prev,next);
	    
	}

    }

    CBLK_Q_NODE_TAIL(afu->head_path,afu->tail_path,path,prev,next);

    return afu;
}


/*
 * NAME:        cblk_update_afu_type
 *
 * FUNCTION:    This routine updates the AFU type if 
 *              possible.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0         -   Success
 *              otherwise -  Failure
 *              
 */

int cblk_update_afu_type(cflsh_afu_t *afu,cflsh_block_chunk_type_t type)
{
    int rc = 0;
    
    if (afu == NULL) {

	errno = EINVAL;

	return -1;

    }


    if (afu->ref_count == 1) {

	/*
	 * Update AFU only if ref_count
	 * is 1.  Otherwise, it most likely
	 * is already in use.
	 */

	
	if (cblk_alloc_hrrq_afu(afu,afu->num_rrqs)) {


	    errno = ENOMEM;

	    return -1;
	}

	afu->type = type;

	if ((type == CFLASH_BLK_CHUNK_SIS_LITE_SQ) &&
	    !(afu->flags & CFLSH_AFU_SQ)) {


	    /*
	     * This AFU is using a Submission Queue (SQ)
	     * to issue IOARCBs to it and we have not
	     * yet allocated the SWQ. So let's do that
	     * now.
	     */

	
	    CBLK_TRACE_LOG_FILE(9,"switching to SQ mode");

	    if (cblk_alloc_sq_afu(afu,afu->num_rrqs)) {


		cblk_free_hrrq_afu(afu);
		errno = ENOMEM;

		return -1;
	    }

	    afu->flags |= CFLSH_AFU_SQ;

	    afu->hrrq_to_ioarcb_off = sizeof(sisl_ioarcb_t);


	}
    }

    return rc;

}

/*
 * NAME:        cblk_release_afu
 *
 * FUNCTION:    This routine checks if the afu is used by others
 *              if not, then it frees the afu structure;
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */

void cblk_release_afu(cflsh_path_t *path,cflsh_afu_t *afu)
{
    if ((afu) && (path)) {

	CBLK_DQ_NODE(afu->head_path,afu->tail_path,path,prev,next);

	afu->ref_count--;
	if (!(afu->ref_count)) {
	    
	    CBLK_DQ_NODE(cflsh_blk.head_afu,cflsh_blk.tail_afu,afu,prev,next);

	    if (afu->head_complete) {

		CBLK_TRACE_LOG_FILE(1,"AFU has commands on completion list");
		
	    }
	    cblk_free_hrrq_afu(afu);
	    cblk_free_sq_afu(afu);
	    free(afu);
	}

    }

    return;

}


/*
 * NAME:        cblk_get_path
 *
 * FUNCTION:    This routine checks if a path structure
 *              already exists for the specified path. Otherwise
 *              it will allocate one.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */

cflsh_path_t *cblk_get_path(cflsh_chunk_t *chunk, dev64_t adap_devno,char *path_name,cflsh_block_chunk_type_t type,int num_cmds,
			    cflsh_afu_in_use_t *in_use, int share)
{
    cflsh_path_t *path = NULL;
    int pthread_rc;
    int j;




    /*
     * Align on 4K boundary to make it easier
     * for debug purposes.
     */

    if ( posix_memalign((void *)&path,4096,
			(sizeof(*path)))) {

		    
	CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for path, errno= %d",errno);
	


	path = NULL;

    } else {

	    
	bzero((void *) path,sizeof (*path));

	path->type = type;

	path->chunk = chunk; 

	if (cblk_set_fcn_ptrs(path)) {

	    CBLK_TRACE_LOG_FILE(1,"Failed to set up function pointers. errno= %d",errno);

	    free(path);

	    return NULL;

	}


	path->afu = cblk_get_afu(path,chunk->dev_name,adap_devno,type,num_cmds,in_use,share);

	if (path->afu == NULL) {

	    free(path);

	    return NULL;
	}

	pthread_rc = pthread_cond_init(&(path->resume_event),NULL);
    
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(1,"pthread_cond_init failed for resume_event rc = %d errno= %d",
				pthread_rc,errno);

	    cblk_release_afu(path,path->afu);
	    free(path);

	    return NULL;

	}

#ifndef _AIX
	if (path_name) {
	    strcpy(path->dev_name,path_name);
	}
#endif /* _AIX */

	path->eyec = CFLSH_EYEC_PATH;   

	/*
	 * Ensure the chunk's num_cmds does
	 * not exceed the AFU's number of rrq
	 * elements. If num_cmds is larger than
	 * num_rrqs, then set num_cmds to num_rrqs,
	 * This also requires that there are only 
	 * num_rrqs command sin the chunk free list.
	 */


	if (chunk->num_cmds > path->afu->num_rrqs) {

	    
	    for (j = chunk->num_cmds; j < path->afu->num_rrqs; j++) {

		CBLK_DQ_NODE(chunk->head_free,chunk->tail_free,&(chunk->cmd_info[j]),free_prev,free_next);
	    }


	    chunk->num_cmds = path->afu->num_rrqs;
	}

	chunk->num_paths++;
	    
    }

    


    return path;
}


/*
 * NAME:        cblk_release_path
 *
 * FUNCTION:    This routine checks if the path is used by others
 *              if not, then it frees the path structure;
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */

void cblk_release_path(cflsh_chunk_t *chunk, cflsh_path_t *path)
{
    if (path) {


	cblk_release_afu(path,path->afu);
	path->afu = NULL;

#ifndef _AIX	
	bzero(path->dev_name,PATH_MAX);
#endif /* !_AIX */

	path->eyec = 0;
	free(path);
	chunk->num_paths--;

    }

    return;

}

/*
 * NAME:        cblk_update_path_type
 *
 * FUNCTION:    This routine updates a path's type when
 *              more information is known.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              0         -   Success
 *              otherwise -  Failure
 *              
 */

int cblk_update_path_type(cflsh_chunk_t *chunk, cflsh_path_t *path, cflsh_block_chunk_type_t type)
{
    int rc = 0;
    int sq_flags;


    if (path == NULL) {


	errno = EINVAL;

	return -1;
    }

    sq_flags = path->afu->flags & CFLSH_AFU_SQ;

    rc = cblk_update_afu_type(path->afu,type);

    if (!sq_flags &&
	(path->afu->mmio) &&
	(path->afu->flags & CFLSH_AFU_SQ)) {
	/*
	 * If we changed from not having an SQ for this AFU
	 * to now having one, then we need to register
	 * the SQ with the AFU. This can happen
	 * in situations where we determine the
	 * AFU type after we have already done
	 * the initial setup.
	 */

	rc = CBLK_ADAP_SQ_SETUP(chunk,path->path_index);

    }


    return rc;
}



/*
 * NAME:        cblk_get_chunk
 *
 * FUNCTION:    This routine gets chunk of the specified
 *              command type.
 *
 * NOTE:        This routine assumes the caller
 *              has the cflsh_blk.global_lock.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */
chunk_id_t cblk_get_chunk(int flags,int max_num_cmds)
{
    chunk_id_t ret_chunk_id = NULL_CHUNK_ID;
    cflsh_chunk_t *chunk = NULL;
    cflsh_chunk_hash_t *p_hash = NULL;
    int j;


#ifdef BLOCK_FILEMODE_ENABLED
    char *max_transfer_size_blocks = getenv("CFLSH_BLK_MAX_XFER");
#endif /* BLOCK_FILEMODE_ENABLED */



    if (max_num_cmds <= 0) {
      /*
       * If max_num_cmds not passed 
       * then use our default size.
       */
      max_num_cmds = NUM_CMDS;
    } else if (max_num_cmds > MAX_NUM_CMDS) {
      /*
       * If max_num_cmds is larger than
       * our upper limit then fail this request.
       */

      errno = ENOMEM;
      return ret_chunk_id;
    }

    /*
     * Align on 4K boundary, so that we can use
     * the low order bits for eyecatcher ir hashing
     * if we decide to pass back a modified pointer
     * to the user. Currently we are not doing this,
     * but depending on the efficiency of the hash table
     * we may need to in the future.
     */

    if ( posix_memalign((void *)&chunk,4096,
			(sizeof(*chunk)))) {

		    
	CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for chunk, errno= %d",errno);
	

	return ret_chunk_id;

    }
    
    /*
     * Initialize chunk for use;
     */


    if (flags & CFLSH_BLK_CHUNK_SET_UP) {

	
	bzero((void *) chunk,sizeof (*chunk));


	CFLASH_BLOCK_LOCK_INIT(chunk->lock);

	chunk->num_cmds = max_num_cmds;

	/*
	 * Align commands on cacheline boundary.
	 */

	if ( posix_memalign((void *)&(chunk->cmd_start),128,
			    (sizeof(*(chunk->cmd_start)) * chunk->num_cmds))) {

		    
	    CBLK_TRACE_LOG_FILE(1,"Failed posix_memalign for cmd_start errno= %d",errno);

	    // ?? TODO maybe should return special type of error.
	    free(chunk);
	    
	    return ret_chunk_id;
	}

	bzero((void *)chunk->cmd_start ,
	      (sizeof(*(chunk->cmd_start)) * chunk->num_cmds));

	chunk->cmd_curr = chunk->cmd_start;

	chunk->cmd_end = chunk->cmd_start + chunk->num_cmds;

	chunk->in_use = TRUE;

	/*
	 * Alocate command infos for each command
	 */

	chunk->cmd_info = malloc(sizeof(cflsh_cmd_info_t) * chunk->num_cmds);

	if (chunk->cmd_info == NULL) {


	    // ?? TODO maybe should return special type of error.

	    free(chunk->cmd_start);
	    chunk->cmd_start = NULL;
	    free(chunk);
	    
	    return ret_chunk_id;

	}

	bzero((void *)chunk->cmd_info,(sizeof(cflsh_cmd_info_t) * chunk->num_cmds));

	for (j = 0; j < chunk->num_cmds; j++) {
	    chunk->cmd_start[j].index = j;
	    chunk->cmd_start[j].cmdi = &chunk->cmd_info[j];
	    chunk->cmd_info[j].index = j;
	    chunk->cmd_info[j].chunk = chunk;
	    chunk->cmd_info[j].eyec = CFLSH_EYEC_INFO;

	    CBLK_Q_NODE_TAIL(chunk->head_free,chunk->tail_free,&(chunk->cmd_info[j]),free_prev,free_next);
	}



	CFLASH_BLOCK_LOCK_INIT((chunk->lock));




	cflsh_blk.num_active_chunks++;
	cflsh_blk.num_max_active_chunks = MAX(cflsh_blk.num_active_chunks,cflsh_blk.num_max_active_chunks);


	chunk->index = cflsh_blk.next_chunk_id++;

	ret_chunk_id = chunk->index;

	chunk->stats.block_size = CAPI_FLASH_BLOCK_SIZE;

	chunk->stats.max_transfer_size = 1;

#ifdef BLOCK_FILEMODE_ENABLED

	/*
	 * For filemode let user adjust maximum transfer size
	 */

	if (max_transfer_size_blocks) {
	    chunk->stats.max_transfer_size = atoi(max_transfer_size_blocks);
	} 
#endif /* BLOCK_FILEMODE_ENABLED */


	/*
	 * Insert chunk into hash list
	 */

	chunk->eyec = CFLSH_EYEC_CHUNK;

	p_hash = &cflsh_blk.hash[chunk->index & CHUNK_HASH_MASK];

    CFLASH_BLOCK_WR_RWLOCK(p_hash->lock);

	if (p_hash->tail)
	{
	    chunk->prev        = p_hash->tail;
	    p_hash->tail->next = chunk;
	}
	if (!p_hash->head) {p_hash->head = chunk;}
	p_hash->tail = chunk;

	CFLASH_BLOCK_RWUNLOCK(p_hash->lock);
    }

    if (ret_chunk_id == NULL_CHUNK_ID) {

	CBLK_TRACE_LOG_FILE(1,"no chunks found , num_active = 0x%x",cflsh_blk.num_active_chunks);
	errno = ENOSPC;
    }

    return ret_chunk_id;
}





/*
 * NAME:        cblk_process_cmd
 *
 * FUNCTION:    Processes the status of a command
 *              that the AFU has completed. 
 *              
 * Environment: This routine assumes the chunk lock is held
 *              and afu mutex
 *              lock was conditionally taken by 
 *              CFLASH_BLOCK_AFU_SHARE_LOCK.
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 *              cmd   - Cmd which just completed
 *
 * RETURNS:
 *             -1  - Fatal error
 *              0  - Ignore error (consider good completion)
 *              1  - Retry recommended
 *              
 *              
 */

cflash_cmd_err_t cblk_process_cmd(cflsh_chunk_t *chunk,int path_index, cflsh_cmd_mgm_t *cmd)
{
    cflash_cmd_err_t rc = CFLASH_CMD_IGNORE_ERR;
    int rc2 = 0;
    size_t transfer_size = 0;
    int pthread_rc;
    int i;
    


    if (CBLK_INVALID_CMD_CMDI(chunk,cmd,cmd->cmdi,__FUNCTION__)) {

	CBLK_TRACE_LOG_FILE(1,"cmd/cmdi is invalid for chunk = %s, path_index = 0x%x chunk->flags = 0x%x",
			    chunk->dev_name,path_index, chunk->flags);

	for (i = 0; i< chunk->num_cmds;i++) {

	    if (cmd == &chunk->cmd_start[i]) {

		
		CBLK_TRACE_LOG_FILE(1,"cmd/cmdi (cont) cmd found with index = %d, cmd->index = %d, cmdi = %p",
				    i,cmd->index,&(chunk->cmd_info[i]));

		break;
	    }
	}
	
        errno = EINVAL;
	rc = CFLASH_CMD_FATAL_ERR;
	return rc;
    }


    if (!cmd->cmdi->in_use) {

	/*
	 * This command has already been
	 * processed.
	 */
 	CBLK_TRACE_LOG_FILE(1,"!!---CMD WAS ALREADY COMPLETED:  cmd->cmdi->lba = 0x%llx, cmd->cmdi->retry_count %d, flags = 0x%x",cmd->cmdi->lba,cmd->cmdi->retry_count,cmd->cmdi->flags);
	CBLK_TRACE_LOG_FILE(1,"!!---CMD WAS ALREADY COMPLETED2:  cmd = 0x%llx on chunk->index = %d",(uint64_t)cmd,chunk->index);
	return CFLASH_CMD_IGNORE_ERR;
    }
    
    CBLK_TRACE_LOG_FILE(8,"cmd = 0x%llx",(uint64_t)cmd);

    if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

	CBLK_TRACE_LOG_FILE(1,"chunk/path/AFU is bad");
	
        errno = EINVAL;
	rc = CFLASH_CMD_FATAL_ERR;
	return rc;


    }

    if (chunk->path[path_index]->afu->num_issued_cmds) {
	fetch_and_add(&(chunk->path[path_index]->afu->num_issued_cmds),-1);

    } else {
	CBLK_TRACE_LOG_FILE(1,"bad num_issued_cmds field, cmd = 0x%llx,path_index = %d, ",
			    (uint64_t)cmd,path_index);
    }

    if (CBLK_COMPLETE_STATUS_ADAP_CMD(chunk,cmd)) {

	/*
	 * Command completed with an error
	 */

	cmd->cmdi->transfer_size = 0;

	rc = CBLK_PROCESS_ADAP_CMD_ERR(chunk,cmd);

	if ((rc == CFLASH_CMD_RETRY_ERR) ||
	    (rc == CFLASH_CMD_DLY_RETRY_ERR)) {

	    CBLK_TRACE_LOG_FILE(5,"retry recommended for  cmd->cmdi->lba = 0x%llx, cmd->cmdi->retry_count = %d on chunk->index = %d",
				cmd->cmdi->lba,cmd->cmdi->retry_count,chunk->index);

	    if (cmd->cmdi->retry_count < CAPI_CMD_MAX_RETRIES) {

		/*
		 * Retry command
		 */

		if (rc == CFLASH_CMD_DLY_RETRY_ERR) {
		    /*
		     * This is a retry after a delay.
		     *
		     * NOTE: Currently we are just
		     *       sleeping here for the delay.
		     *       Under the current implementation, this is 
		     *       approach is acceptable. However if we
		     *       we may need to change this in the future.
		     *
		     *       For this current approach we need
		     *       to unlock (and allow other threads
		     *       to progress) while we sleep. 
		     */

		    CBLK_TRACE_LOG_FILE(5,"retry with delayrecommended for  cmd->cmdi->lba = 0x%llx, cmd->cmdi->retry_count %d chunk->index = %d",
					cmd->cmdi->lba,cmd->cmdi->retry_count,chunk->index);
		    CFLASH_BLOCK_UNLOCK(chunk->lock);
		    sleep(CAPI_SCSI_IO_RETRY_DELAY);
		    CFLASH_BLOCK_LOCK(chunk->lock);

		    
		}

		CBLK_INIT_ADAP_CMD_RESP(chunk,cmd);

		/*
		 * Update command possibly for new path or context
		 */

		if (CBLK_UPDATE_PATH_ADAP_CMD(chunk,cmd,0)) {

		    CBLK_TRACE_LOG_FILE(1,"CBLK_UPDATE_PATH_ADAP_CMD failed");

		    rc = CFLASH_CMD_FATAL_ERR;
		}

		cmd->cmdi->retry_count++;

		cmd->cmdi->cmd_time = time(NULL);

		/*
		 * Since the caller used CFLASH_BLOCK_AFU_SHARE_LOCK,
		 * we need to use the CFLASH_BLOCK_AFU_SHARE_UNLOCK here.
		 */

		CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

		rc2 = CBLK_ISSUE_CMD(chunk,cmd,cmd->cmdi->buf,
					cmd->cmdi->lba,cmd->cmdi->nblocks,CFLASH_ISSUE_RETRY);


		/*
		 * Since we are going to potentially wait on a resume event,
		 * we need to explicitly take the afu->lock, instead
		 * of conditionally taking the lock as is done in
		 * CFLASH_BLOCK_AFU_SHARE_LOCK. We will explicitly release
		 * it after the waiting and if necessary do the conditional
		 * taking of it later via CFLASH_BLOCK_AFU_SHARE_LOCK,
		 * Since this error recovery, this should not impact good path
		 */

		CFLASH_BLOCK_LOCK(chunk->path[path_index]->afu->lock);
		
		if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

		    CBLK_TRACE_LOG_FILE(1,"chunk/path/AFU is bad");
	
		}

		if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
		    
		    /*
		     * If path is in a halted state then wait for it to 
		     * resume. Since we are waiting for the AFU resume
		     * event, that afu-lock will be released, but our chunk
		     * lock will not be released. So do that now.
		     */
		    
		    CFLASH_BLOCK_UNLOCK(chunk->lock);
		    
		    
		    pthread_rc = pthread_cond_wait(&(chunk->path[path_index]->afu->resume_event),
						   &(chunk->path[path_index]->afu->lock.plock));
		    
		    /*
		     * Chunk lock must be acquired first to prevent deadlock.
		     */
		    
		    CFLASH_BLOCK_UNLOCK(chunk->path[path_index]->afu->lock);          
		    CFLASH_BLOCK_LOCK(chunk->lock);

		    /*
		     * Conditionally acquire the afu lock to match the
		     * lock state we entered this routine.
		     */
		    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu); 

		    if (pthread_rc) {
			
			
			
			
			CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed for resume_event rc = %d errno = %d",
					    pthread_rc,errno);
			
			cmd->cmdi->status = EIO;

			errno = EIO;
			
			rc = CFLASH_CMD_FATAL_ERR;
			return rc;
		    }

		    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
			
			/*
			 * Give up if the AFU is still halted.
			 */
			
                        
			CBLK_TRACE_LOG_FILE(5,"afu halted again afu->flag = 0x%x",
					    chunk->path[path_index]->afu->flags);
					    
			
			cmd->cmdi->status = EIO;

			errno = EIO;
			
			rc = CFLASH_CMD_FATAL_ERR;
			return rc;
		    }
		    
		} else {


		    /*
		     * Conditionally acquire the afu lock to match the
		     * lock state we entered this routine.
		     */
		    CFLASH_BLOCK_UNLOCK(chunk->path[path_index]->afu->lock);
		    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu); 
		}



		if (rc2) {

		    /*
		     * If we failed to issue this command for
		     * retry then give up on it now.
		     */

		    CBLK_TRACE_LOG_FILE(8,"retry issue failed with rc2 = 0x%x cmd->cmdi->lba = 0x%llx chunk->index = %d",
					rc2,cmd->cmdi->lba,chunk->index);
		    cmd->cmdi->status = EIO;

		    errno = EIO;

		    rc = CFLASH_CMD_FATAL_ERR;
		    return rc;
		} else {
	
		    chunk->stats.num_retries++;
		    CBLK_TRACE_LOG_FILE(8,"retry issue succeeded cmd->cmdi->in_use= 0x%x cmd->cmdi->lba = 0x%llx chunk->index = %d",
					cmd->cmdi->in_use,cmd->cmdi->lba,chunk->index);
		    return rc;
		}



	    } else {


		/*
		 * If we exceeded retries then
		 * give up on it now.
		 */

		errno = EIO;
		cmd->cmdi->status = EIO;

		rc = CFLASH_CMD_FATAL_ERR;
	    }

	} /* rc == CFLASH_CMD_RETRY_ERR */


    } else {

	/*
	 * No serious error was seen, but we could
	 * have an underrun.
	 */


	cmd->cmdi->status = 0;

	if (!cmd->cmdi->transfer_size_bytes) {

	    /*
	     * If this transfer is not in bytes, then it will
	     * be in blocks, which indicate this is a read/write.
	     * As result, if all data was transferred, then
	     * we should save this to the cache.
	     */


	    if (cmd->cmdi->transfer_size_bytes == cmd->cmdi->nblocks) {

		CBLK_SAVE_IN_CACHE(chunk,cmd->cmdi->buf,cmd->cmdi->lba,
				   cmd->cmdi->nblocks);
	    }
	}

    }


    cmd->cmdi->flags |= CFLSH_PROC_CMD;


#ifdef _COMMON_INTRPT_THREAD

    if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    

	/*
	 * Signal any one waiting for this specific command
	 */

	pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for hread_event rc = %d,errno = %d, chunk->index = %d",
				pthread_rc,errno,chunk->index);
	}
	    

	/*
	 * Signal any one waiting for any command to complete.
	 */

	pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
				pthread_rc,errno,chunk->index);
	}
    }

    
#endif

    if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

	CBLK_TRACE_LOG_FILE(1,"chunk/path/AFU is bad");
	
	return rc;


    }

    if (CBLK_INVALID_CMD_CMDI(chunk,cmd,cmd->cmdi,__FUNCTION__)) {

	CBLK_TRACE_LOG_FILE(1,"cmd/cmdi is invalid");
	
	for (i = 0; i< chunk->num_cmds;i++) {

	    if (cmd == &chunk->cmd_start[i]) {

		
		CBLK_TRACE_LOG_FILE(1,"cmd/cmdi (cont) cmd found with index = %d, cmd->index = %d, cmdi = %p",
				    i,cmd->index,&(chunk->cmd_info[i]));

		break;
	    }
	}
	
	return rc;
    }

    CBLK_TRACE_LOG_FILE(8,"cmd->cmdi->in_use= 0x%x cmd->cmdi->lba = 0x%llx, chunk->index = %d cmd->cmdi->flags = 0x%x",
			cmd->cmdi->in_use,cmd->cmdi->lba,chunk->index,cmd->cmdi->flags);

    CBLK_TRACE_LOG_FILE(8,"chunk->dev_name = %s, chunk->flags = 0x%x, path_index = %d",
			chunk->dev_name,chunk->flags,path_index);

    if (((rc != CFLASH_CMD_RETRY_ERR) &&
	(rc != CFLASH_CMD_DLY_RETRY_ERR)) &&
	(chunk->cmd_info[cmd->index].flags & CFLSH_CMD_INFO_USTAT)) {

	CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);
	
    }


    return rc;
}



/*
 * NAME:        cblk_find_free_cmd
 *
 * FUNCTION:    Finds the first free command. Assumes caller has chunk lock.
 *
 *
 * INPUTS:
 *              chunk - The chunk to which a free
 *                      command is needed.
 *          
 *              cmd   - Pointer to found command.
 *
 * RETURNS:
 *              0         - Command was found.
 *              otherwise - error
 *              
 */

int cblk_find_free_cmd(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t **cmd,int flags)
{
    int rc = -1;
    int found = FALSE;
    int num_in_use = 0;
    int pthread_rc;
    int loop_cnt = 0;
    cflsh_cmd_info_t *cmdi;


    if (chunk == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Invalid chunk");
	errno = EINVAL;
	return -1;

    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */
	
	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk");
	errno = EINVAL;
	return -1;
    }

    /*
     * The head of the free queue will be the command
     * on the free list the longest. So use that if
     * it is available.
     */

    cmdi = chunk->head_free;


	
    if (cmdi == NULL)  {  
	
	chunk->stats.num_no_cmds_free++;

        if (flags & CFLASH_WAIT_FREE_CMD) {

	    /*
	     * We do not have any free commands
	     * available.  So we need to wait for 
	     * a free command.
	     */


	    while ((!found) && (loop_cnt++ < CFLASH_BLOCK_MAX_CMD_WAIT_RETRIES))  {

		CFLASH_BLOCK_UNLOCK(chunk->lock);

		usleep(CFLASH_BLOCK_FREE_CMD_WAIT_DELAY);

		CFLASH_BLOCK_LOCK(chunk->lock);

	    
		if (CFLSH_EYECATCH_CHUNK(chunk)) {
		    /*
		     * Invalid chunk. Exit now.
		     */

		    cflsh_blk.num_bad_chunk_ids++;
		    CBLK_TRACE_LOG_FILE(1,"Invalid chunk");
		    errno = EINVAL;
		    return -1;
		}

		CBLK_TRACE_LOG_FILE(1,"No free command found num_cmds = %d "
		                      "num_active_cmds = %d, num_in_use = %d flags:%x",
				    chunk->num_cmds, chunk->num_active_cmds, num_in_use, flags);
		
		
		cmdi = chunk->head_free;

		    
		if (cmdi == NULL)  {  
		    chunk->stats.num_no_cmds_free++;
		    
		    rc = -1;
		    
		    errno = EBUSY;
		    
		    
		    CBLK_TRACE_LOG_FILE(1,"No free command found num_active_cmds = %d",chunk->num_active_cmds);
		    
		} else {

		    found = TRUE;
		}
		

	    } /* while */

	    if (!found)  {


		rc = -1;

		errno = EBUSY;
	    
	    
		chunk->stats.num_no_cmds_free_fail++;
		CBLK_TRACE_LOG_FILE(1,"Giving up No free command found num_active_cmds = %d",chunk->num_active_cmds);
		return rc;
	    }
        } else {


	    /*
	     * The caller does not want us
	     * wait for a command. So fail now.
	     */
	    rc = -1;

	    errno = EBUSY;
	    
	    chunk->stats.num_no_cmds_free_fail++;
	    
	    CBLK_TRACE_LOG_FILE(1,"No free command found num_active_cmds = %d num_in_use = %d",
				chunk->num_active_cmds,num_in_use);
	    return rc;
	}

    }

    if (chunk->cmd_start == NULL) {
	CBLK_TRACE_LOG_FILE(1,"cmd_start is NULL, chunk->flags =0x%x",chunk->flags);
	errno = EINVAL;
	return -1;
    }


    if (cmdi == NULL) {
	CBLK_TRACE_LOG_FILE(1,"cmdi is NULL, chunk->flags =0x%x",chunk->flags);
	errno = EINVAL;
	return -1;
    }

    if ((cmdi < &(chunk->cmd_info[0])) ||
	(cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	CBLK_TRACE_LOG_FILE(1,"cmdi = %p with index %d is invalid, chunk->num_cmds = %d, chunk->flags =0x%x",
			    cmdi,cmdi->index, chunk->num_cmds,chunk->flags);
	errno = EINVAL;
	return -1;

    }

    if (CFLSH_EYECATCH_CMDI(cmdi)) {

	CBLK_TRACE_LOG_FILE(1,"Invalid eyecatcher cmdi = %p is invalid, chunk->num_cmds = %d, chunk->flags =0x%x",
			    cmdi,cmdi->index, chunk->num_cmds,chunk->flags);
	errno = EINVAL;
	return -1;

    }


    if (cmdi->index > chunk->num_cmds) {
	CBLK_TRACE_LOG_FILE(1,"cmdi = %p index is too large = %d, chunk->num_cmds = %d, chunk->flags =0x%x",
			    cmdi,cmdi->index, chunk->num_cmds);


	CBLK_TRACE_LOG_FILE(1,"user_tag = 0x%x, *user_status = %p, path_index = %d",
			    cmdi->user_tag,cmdi->user_status,cmdi->path_index,chunk->flags);
	errno = EINVAL;
	return -1;
    }

    *cmd = &(chunk->cmd_start[cmdi->index]);

    if (*cmd == NULL) {
	CBLK_TRACE_LOG_FILE(1,"cmd is NULL");
	errno = EINVAL;
	return -1;
    }

    if (((*cmd) < chunk->cmd_start) ||
	((*cmd) > chunk->cmd_end)) {



	CBLK_TRACE_LOG_FILE(1,"cmd = %p with index = %d, is invalid, chunk->num_cmds = %d, chunk->flags =0x%x",
			    *cmd,cmdi->index, chunk->num_cmds,chunk->flags);
	errno = EINVAL;
	return -1;

    }

    bzero((void *)(*cmd),sizeof (**cmd));

    pthread_rc = pthread_cond_init(&(cmdi->thread_event),NULL);
    
    if (pthread_rc) {
	
	CBLK_TRACE_LOG_FILE(1,"pthread_cond_init failed rc = %d errno= %d, cmdi = %p",
			    pthread_rc,errno,cmdi);
	rc = -1;

	return rc;
	
    }

#ifndef _COMMON_INTRPT_THREAD
    cmdi->async_data = NULL;
#endif /* !_COMMON_INTRPT_THREAD */

    cmdi->flags = 0;
    cmdi->path_index = chunk->cur_path;
    cmdi->retry_count = 0;
    cmdi->state = 0;
    cmdi->transfer_size = 0;
    cmdi->transfer_size_bytes = 0;
    cmdi->thread_id = 0;
    cmdi->buf = NULL;
    cmdi->status = 0;
    cmdi->nblocks = 0;
    cmdi->lba = 0;
    cmdi->user_status = NULL;

    
    cmdi->cmd_time = time(NULL);
    (*cmd)->cmdi = cmdi;


    /*
     * Remove command from free list
     */
    
    CBLK_DQ_NODE(chunk->head_free,chunk->tail_free,cmdi,free_prev,free_next);

    /*
     * place command on active list
     */
    
    CBLK_Q_NODE_TAIL(chunk->head_act,chunk->tail_act,cmdi,act_prev,act_next);
 		    
  
    cmdi->in_use = 1;
    (*cmd)->index = cmdi->index;

    CBLK_TRACE_LOG_FILE(9,"cmd = %p cmdi = %p",*cmd,cmdi);
    return 0;
}


/*
 * NAME:        cblk_get_buf_cmd
 *
 * FUNCTION:    Finds free command and allocates data buffer for command
 *              
 *
 *
 * INPUTS:
 *              chunk       - Chunk the read is associated.
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *
 *
 * RETURNS:
 *              None
 *              
 *              
 */

int cblk_get_buf_cmd(cflsh_chunk_t *chunk,void **buf, size_t buf_len, 
		     cflsh_cmd_mgm_t **cmd)
{
    int rc = 0;


    /*
     * AFU requires data buffer to have 16 byte alignment
     */

    if ( posix_memalign((void *)buf,64,buf_len)) {
		
	CBLK_TRACE_LOG_FILE(1,"posix_memalign failed for buffer size = %d,errno = %d",
			    chunk->cache_size,errno);
	

	return -1;

	
    }

    CFLASH_BLOCK_LOCK(chunk->lock);
    rc = cblk_find_free_cmd(chunk,cmd,CFLASH_WAIT_FREE_CMD);

    if (rc) {

        
        free(*buf);
	CBLK_TRACE_LOG_FILE(1,"could not find a free cmd, num_active_cmds = %d",chunk->num_active_cmds);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EBUSY;
	
	return -1;
    }

    chunk->cmd_info[(*cmd)->index].path_index = chunk->cur_path;
    CBLK_BUILD_ADAP_CMD(chunk,*cmd,*buf,buf_len,CFLASH_READ_DIR_OP);
    CFLASH_BLOCK_UNLOCK(chunk->lock);


    return rc;
}

#ifdef _COMMON_INTRPT_THREAD
/*
 * NAME:        cblk_start_common_intrpt_thread
 *
 * FUNCTION:    Starts common interrupt thread.
 *              When the block library is compiled
 *              in this mode, there is exactly one
 *              dedicated thread for processing all
 *              interrupts.  The alternate mode to compile
 *              the block library is cooperative interrupt
 *              processing, where multiple threads can
 *              coordinate the processing of interrupts.
 *
 * NOTE:        This routine assumes the caller
 *              is holding both the chunk lock and
 *              the global lock.
 *
 *
 * INPUTS:
 *              chunk - Chunk associated with a lun
 *                      
 *
 * RETURNS:
 *              0 - Success
 *             -1 - Error/failure
 *              
 */

int cblk_start_common_intrpt_thread(cflsh_chunk_t *chunk)
{
    int rc = 0;
    int pthread_rc;
    cflsh_async_thread_cmp_t *async_data;


    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	/* 
	 * Background threads are not allowed
	 */

	return rc;
    }

    chunk->thread_flags = 0;

    pthread_rc = pthread_cond_init(&(chunk->thread_event),NULL);
    
    if (pthread_rc) {
	
	CBLK_TRACE_LOG_FILE(1,"pthread_cond_init failed for thread_event rc = %d errno= %d",
			    pthread_rc,errno);

	    
	errno = EAGAIN;
	return -1;
	
    }

    pthread_rc = pthread_cond_init(&(chunk->cmd_cmplt_event),NULL);
    
    if (pthread_rc) {
	
	CBLK_TRACE_LOG_FILE(1,"pthread_cond_init failed for cmd_cmplt_event rc = %d errno= %d",
			    pthread_rc,errno);

	    
	errno = EAGAIN;
	return -1;
	
    }

    async_data = &(chunk->intrpt_data);
    async_data->chunk = chunk;
    async_data->cmd_index = 0;
	
    pthread_rc = pthread_create(&(chunk->thread_id),NULL,cblk_intrpt_thread,async_data);
	
    if (pthread_rc) {
	    
	chunk->stats.num_failed_threads++;
	    
	CBLK_TRACE_LOG_FILE(5,"pthread_create failed rc = %d,errno = %d num_active_cmds = 0x%x",
			    pthread_rc,errno, chunk->num_active_cmds);

	errno = EAGAIN;
	return -1;
    }

    /*
     * We successfully started the thread.
     * Update statistics reflecting this.
     */
	
    chunk->stats.num_success_threads++;

    chunk->stats.num_active_threads++;

    chunk->stats.max_num_act_threads = MAX(chunk->stats.max_num_act_threads,chunk->stats.num_active_threads);


    return rc;
}

#endif /* _COMMON_INTRPT_THREAD */



/*
 * NAME:        cblk_get_lun_id
 *
 * FUNCTION:    Gets the lun id of the physical
 *              lun associated with this chunk.
 *
 * NOTE:        This routine assumes the caller
 *              is not holding the chunk lock.
 *
 *
 * INPUTS:
 *              chunk - Chunk associated with a lun
 *                      
  *
 * RETURNS:
 *              NONE
 *              
 */

int cblk_get_lun_id(cflsh_chunk_t *chunk)
{
    int rc = 0;
    void *raw_lun_list = NULL;
    int list_size = 4096;
    uint64_t *lun_ids;
    int num_luns = 0;
    cflsh_cmd_mgm_t *cmd;
    size_t transfer_size = 0;
#ifdef BLOCK_FILEMODE_ENABLED
    struct lun_list_hdr *list_hdr;
#else 
    int cmd_index = 0;
#endif /* BLOCK_FILEMODE_ENABLED */



    
    if (cflsh_blk.flags & CFLSH_G_LUN_ID_VAL) {

        /*
	 * We have alread determined the lun id.
	 * So just set it for the chunk
	 * and return.
	 */

         chunk->path[chunk->cur_path]->lun_id = cflsh_blk.lun_id;

	 CBLK_TRACE_LOG_FILE(5,"rc = %d,lun_id = 0x%llx",
			rc,cflsh_blk.lun_id);
	 return rc;
    }


    if (cblk_get_buf_cmd(chunk,&raw_lun_list,list_size,
			 &cmd)) {

        
	return -1;

    }

    bzero(raw_lun_list,list_size);


    /*
     * This command will use transfer size in bytes
     */
    
    cmd->cmdi->transfer_size_bytes = 1;

    if (cflash_build_scsi_report_luns(CBLK_GET_CMD_CDB(chunk,cmd),
				      list_size)) {

	CBLK_TRACE_LOG_FILE(5,"build_scsi_report_luns failed rc = %d,",
			    rc);
	CBLK_FREE_CMD(chunk,cmd);
	free(raw_lun_list);
	return -1;

    }


    CFLASH_BLOCK_LOCK(chunk->lock);

    if (CBLK_ISSUE_CMD(chunk,cmd,raw_lun_list,0,0,0)) {

        
#ifdef BLOCK_FILEMODE_ENABLED
	

	/*
	 * Since we're emulating SCSI commands, here we need to do
	 * clean up from CBLK_ISSUE_CMD.
	 */

	if (chunk->path[chunk->cur_path]->afu->num_issued_cmds) {
	    fetch_and_add(&(chunk->path[chunk->cur_path]->afu->num_issued_cmds),-1);

	} else {
	    CBLK_TRACE_LOG_FILE(1,"bad num_issued_cmds field, cmd = 0x%llx,path_index = %d, ",
				(uint64_t)cmd,chunk->cur_path);
	}

#endif /* BLOCK_FILEMODE_ENABLED */

	CFLASH_BLOCK_UNLOCK(chunk->lock);
	CBLK_FREE_CMD(chunk,cmd);
        free(raw_lun_list);
        return -1;

    }

    CFLASH_BLOCK_UNLOCK(chunk->lock);

#ifdef BLOCK_FILEMODE_ENABLED
	

    /*
     * Since we're emulating SCSI commands, here we need to do
     * clean up from CBLK_ISSUE_CMD.
     */

    if (chunk->path[chunk->cur_path]->afu->num_issued_cmds) {
	fetch_and_add(&(chunk->path[chunk->cur_path]->afu->num_issued_cmds),-1);

    } else {
	CBLK_TRACE_LOG_FILE(1,"bad num_issued_cmds field, cmd = 0x%llx,path_index = %d, ",
			    (uint64_t)cmd,chunk->cur_path);
    }


    /*
     * For BLOCK_FILEMODE_ENABLED get the size of this file that was
     * just opened
     */


    list_hdr = raw_lun_list;

    list_hdr->lun_list_length = CFLASH_TO_ADAP32((sizeof(uint64_t) + sizeof(*list_hdr)));
    lun_ids = (uint64_t *) ++list_hdr;

    lun_ids[0] = cblk_lun_id;

    /*
     * This command completed,
     * clean it up.
     */

    chunk->num_active_cmds--;

    CBLK_FREE_CMD(chunk,cmd);


    transfer_size = sizeof (struct lun_list_hdr );

#else

    cmd_index = cmd->index;

    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&(cmd_index),&transfer_size,TRUE,0);

#ifdef _COMMON_INTRPT_THREAD

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&(cmd_index),&transfer_size,TRUE,0);
    } else {
	rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);
    }

#endif /* _COMMON_INTRPT_THREAD */

#endif /* BLOCK_FILEMODE_ENABLED */

    if (!rc) {

	/*
	 * For good completion, extract the first
	 * lun_id
	 */

	if (transfer_size < sizeof (struct lun_list_hdr )) {

	    CBLK_TRACE_LOG_FILE(1,"Report Luns returned data size is too small = 0x%x",transfer_size);
	    
	    errno = ENOMSG;
	    return -1;

	}

	rc = cflash_process_scsi_report_luns(raw_lun_list,list_size,
					     &lun_ids,&num_luns);

        if (rc) {

	    /*
	     * Failed to process returned lun list
	     */



	    CBLK_TRACE_LOG_FILE(1,"cflash_process_scsi_report_luns failed rc = %d",rc);

	    errno = ENOMSG;
	    rc = -1;

	} else {
	  

            /*
	     * We successfully processed the returned
	     * lun list. 
	     */

	  


	    if (num_luns) {

	        /*
		 * Report luns found some luns.
		 * Let's choose the first lun
		 * in the lun list.
		 */

		if ((lun_ids[0] == 0) &&
		    (num_luns > 1)) {

		    /*
		     * If more than 1 lun was returned and
		     * the first lun is 0, then choose
		     * the second lun.
		     */
		    cflsh_blk.lun_id = lun_ids[1];

		} else {
		    cflsh_blk.lun_id = lun_ids[0];

		}


		cflsh_blk.flags |= CFLSH_G_LUN_ID_VAL;

		chunk->path[chunk->cur_path]->lun_id = cflsh_blk.lun_id;


	    } else {
	        
	        /*
		 * No luns found fail this request.
		 */

	        rc = -1;

		errno = ENXIO;

#ifndef BLOCK_FILEMODE_ENABLED

		CBLK_TRACE_LOG_FILE(5,"no luns found. hardcode lun_id");

		chunk->path[chunk->cur_path]->lun_id = cblk_lun_id;

#endif /* BLOCK_FILEMODE_ENABLED */

	    }

	}

    }


    free(raw_lun_list);


    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d,lun_id = 0x%llx, num_luns = %d",
			rc,errno,cflsh_blk.lun_id, num_luns);
    return rc;
}

/*
 * NAME:        cblk_get_lun_capacity
 *
 * FUNCTION:    Gets the capacity (number of
 *              blocks) for a lun associated with
 *              a specific chunk.
 *
 * NOTE:        This routine assumes the caller
 *              is holding both the chunk lock and
 *              the global lock.
 *
 *
 * INPUTS:
 *              chunk - Chunk associated with a lun
 *                      
  *
 * RETURNS:
 *              0   - Success, otherwise error.
 *
 */

int cblk_get_lun_capacity(cflsh_chunk_t *chunk)
{
    int rc = 0;
#ifdef BLOCK_FILEMODE_ENABLED
    struct stat stats;
    uint64_t  st_size;
#else 
    int cmd_index = 0;
#endif /* !BLOCK_FILEMODE_ENABLED */
    struct readcap16_data *readcap16_data = NULL;
    cflsh_cmd_mgm_t *cmd;
    size_t transfer_size = 0;
    uint32_t block_size = 0;
    uint64_t last_lba = 0;




   
    if (cblk_get_buf_cmd(chunk,(void **)&readcap16_data,sizeof(struct readcap16_data),
			 &cmd)) {

        
	return -1;

    }

    bzero(readcap16_data,sizeof(*readcap16_data));


    /*
     * This command will use transfer size in bytes
     */
    
    cmd->cmdi->transfer_size_bytes = 1;

    if (cflash_build_scsi_read_cap16(CBLK_GET_CMD_CDB(chunk,cmd),
				     sizeof(struct readcap16_data))) {


	CBLK_TRACE_LOG_FILE(5,"build_scsi_read_cap16 failed rc = %d,",
			rc);
        free(readcap16_data);
	return -1;

    }
 


    CFLASH_BLOCK_LOCK(chunk->lock);

    if (CBLK_ISSUE_CMD(chunk,cmd,readcap16_data,0,0,0)) {

        
        
#ifdef BLOCK_FILEMODE_ENABLED
	

	/*
	 * Since we're emulating SCSI commands, here we need to do
	 * clean up from CBLK_ISSUE_CMD.
	 */

	if (chunk->path[chunk->cur_path]->afu->num_issued_cmds) {
	    fetch_and_add(&(chunk->path[chunk->cur_path]->afu->num_issued_cmds),-1);

	} else {
	    CBLK_TRACE_LOG_FILE(1,"bad num_issued_cmds field, cmd = 0x%llx,path_index = %d, ",
				(uint64_t)cmd,chunk->cur_path);
	}

#endif /* BLOCK_FILEMODE_ENABLED */


	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CBLK_FREE_CMD(chunk,cmd);

        free(readcap16_data);
        return -1;

    }

    CFLASH_BLOCK_UNLOCK(chunk->lock);

#ifdef BLOCK_FILEMODE_ENABLED
	
    /*
     * Since we're emulating SCSI commands, here we need to do
     * clean up from CBLK_ISSUE_CMD.
     */

    if (chunk->path[chunk->cur_path]->afu->num_issued_cmds) {
	fetch_and_add(&(chunk->path[chunk->cur_path]->afu->num_issued_cmds),-1);

    } else {
	CBLK_TRACE_LOG_FILE(1,"bad num_issued_cmds field, cmd = 0x%llx,path_index = %d, ",
			    (uint64_t)cmd,chunk->cur_path);
    }


    /*
     * For BLOCK_FILEMODE_ENABLED get the size of this file that was
     * just opened
     */

    /*
     * This command completed,
     * clean it up.
     */

    chunk->num_active_cmds--;

    CBLK_FREE_CMD(chunk,cmd);

    bzero((void *) &stats,sizeof(struct stat));

    rc = fstat(chunk->fd,&stats);

    if (rc) {


        CBLK_TRACE_LOG_FILE(1,"fstat failed with rc = %d, errno = %d",rc, errno);
        free(readcap16_data);
	return rc;
    }

    if (S_ISBLK(stats.st_mode) || S_ISCHR(stats.st_mode)) {

       /*
	* Do not allow special files for file mode
	*/

       errno = EINVAL;
       CBLK_TRACE_LOG_FILE(1,"fstat failed with rc = %d, errno = %d",rc, errno);
       free(readcap16_data);
       perror("cblk_open: Can not use device special files for file mode");
       return -1;


    }

    st_size = stats.st_size;
    CBLK_TRACE_LOG_FILE(5,"st_size = 0x%llx",st_size);
    st_size /= CAPI_FLASH_BLOCK_SIZE;
    CBLK_TRACE_LOG_FILE(5,"number of blocks from stat = 0x%llx",st_size);
    /*
     * LBA is the last valid sector on the disk, not the number
     * blocks on the disk. So decrement to get last LBA.
     */
    st_size--;


    CBLK_TRACE_LOG_FILE(5,"last blocks from stat = 0x%llx",st_size);

    readcap16_data->len = CFLASH_TO_ADAP32(CAPI_FLASH_BLOCK_SIZE);

    readcap16_data->lba = CFLASH_TO_ADAP64(st_size);

    if (readcap16_data->lba <= 1) {

      
      free(readcap16_data);
      CBLK_TRACE_LOG_FILE(1,"fstat returned size of 0 blocks");
      perror("cblk_open: file too small");

      return -1;
    }

    transfer_size = sizeof(*readcap16_data);
#else


    cmd_index = cmd->index;

    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&(cmd_index),&transfer_size,TRUE,0);

#ifdef _COMMON_INTRPT_THREAD

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&(cmd_index),&transfer_size,TRUE,0);
    } else {
	rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);
    }

#endif /* _COMMON_INTRPT_THREAD */

#endif /* BLOCK_FILEMODE_ENABLED */

    if (!rc) {

	/*
	 * For good completion, extract number of 
	 * 4K blocks..
	 */

	if (transfer_size < sizeof(*readcap16_data)) {

	    CBLK_TRACE_LOG_FILE(1,"Read capacity 16 returned data size is too small = 0x%x",transfer_size);

	    errno = ENOMSG;
	    return -1;
	     
	}


	if (cflash_process_scsi_read_cap16(readcap16_data,&block_size,&last_lba) == 0) {

	    CBLK_TRACE_LOG_FILE(5,"block_size = 0x%x,capacity = 0x%llx",
				block_size,last_lba);


	    if (block_size == CAPI_FLASH_BLOCK_SIZE) {
		/*
		 * If the device is reporting back 4K block size,
		 * then use the number of blocks specified as its
		 * capacity.
		 */
		chunk->num_blocks_lun = last_lba + 1;
		chunk->blk_size_mult = 1;
	    } else {
		/*
		 * If the device is reporting back an non-4K block size,
		 * then then convert it capacity to the number of 4K
		 * blocks.
		 */

		chunk->num_blocks_lun = 
		    ((last_lba + 1) * block_size)/CAPI_FLASH_BLOCK_SIZE;
		
		if (block_size) {
		    chunk->blk_size_mult = CAPI_FLASH_BLOCK_SIZE/block_size;
		} else {
		    chunk->blk_size_mult = 8;
		}
	    }
	}
    }


    free(readcap16_data);


    if (chunk->num_blocks_lun == 0) {

	errno = EIO;
	rc = -1;
    }


    if (!(chunk->flags & CFLSH_CHNK_VLUN)) {

      /*
       * If this is a physical lun
       * (not a virtual lun) then assign
       * the lun's capacity to this chunk.
       */

      chunk->num_blocks = chunk->num_blocks_lun;

    }


    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d,capacity = 0x%llx",
			rc,errno,chunk->num_blocks_lun);
    return rc;
}

/*
 * NAME:        cblk_open_cleanup_wait_thread
 *
 * FUNCTION:    If we are running with a single common interrupt thread
 *              per chunk, then this routine terminates that thread
 *              and waits for completion.
 *
 *
 * INPUTS:
 *              chunk - Chunk to be cleaned up.
 *
 * RETURNS:
 *              NONE
 *              
 */
void cblk_open_cleanup_wait_thread(cflsh_chunk_t *chunk) 
{
#ifdef _COMMON_INTRPT_THREAD
    int pthread_rc = 0;


    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	/* 
	 * Background threads are not allowed
	 */

	return;
    }


    chunk->thread_flags |= CFLSH_CHNK_EXIT_INTRPT;

    pthread_rc = pthread_cond_signal(&(chunk->thread_event));
	
    if (pthread_rc) {
	    
	CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d",
			    pthread_rc,errno);
    }

    /*
     * Since we are going to do pthread_join we need to unlock here.
     */

    CFLASH_BLOCK_UNLOCK(chunk->lock);

    pthread_join(chunk->thread_id,NULL);

    CFLASH_BLOCK_LOCK(chunk->lock);

    chunk->stats.num_active_threads--;

    chunk->thread_flags &= ~CFLSH_CHNK_EXIT_INTRPT;

#endif /* _COMMON_INTRPT_THREAD */

    return;
}


/*
 * NAME:        cblk_chunk_open_cleanup
 *
 * FUNCTION:    Cleans up a chunk and resets it
 *              for reuse. This routine assumes
 *              the caller has the chunk's lock.
 *
 *
 * INPUTS:
 *              chunk - Chunk to be cleaned up.
 *
 * RETURNS:
 *              NONE
 *              
 */

void cblk_chunk_open_cleanup(cflsh_chunk_t *chunk, int cleanup_depth)
{
    cflsh_chunk_hash_t *p_hash = NULL;
    int i;

    CBLK_TRACE_LOG_FILE(5,"cleanup = %d",cleanup_depth);

    switch (cleanup_depth) {

    case 50:
	
	cblk_chunk_free_mc_device_resources(chunk);
	/* Fall through */
    case 45:

	cblk_open_cleanup_wait_thread(chunk);
	/* Fall through */
    case 40:

	cblk_chunk_unmap(chunk,FALSE);

    case 35:

	cblk_chunk_detach(chunk,FALSE);


    case 30:

	close(chunk->fd);
	/* Fall through */
    case 20:


	free(chunk->cmd_start);

	free(chunk->cmd_info);

	chunk->cmd_start = NULL;
	chunk->cmd_curr = NULL;
	chunk->cmd_end = NULL;
	chunk->num_cmds = 0;
	/* Fall through */
    case 10:
	
	for (i=0;i < chunk->num_paths;i++) {
	    cblk_release_path(chunk,(chunk->path[i]));

	    chunk->path[i] = NULL;
	}

	/* Fall through */

    default:

    

    

	if (cflsh_blk.num_active_chunks > 0) {
	    cflsh_blk.num_active_chunks--;
	}

#ifndef _MASTER_CONTXT

	if (chunk->flags & CFLSH_CHNK_VLUN) {

	    if (cflsh_blk.num_active_chunks == 0) {

	        /*
		 * If this is the last chunk then
		 * the next cblk_open could use the physical
		 * lun.
		 */


	        cflsh_blk.next_chunk_starting_lba = 0;
	    } else if (cflsh_blk.next_chunk_starting_lba ==
		       (chunk->start_lba + chunk->num_blocks)) {

	        /*
		 * If chunk is the using physical LBAs
		 * at the end of the disk, then release them.
		 * Thus another chunk could use them.
		 */

	        cflsh_blk.next_chunk_starting_lba = chunk->start_lba;

	    }
	}
	
#endif

	chunk->eyec = 0;

	bzero(chunk->dev_name,PATH_MAX);

	for (i=0;i < chunk->num_paths;i++) {
	    if (chunk->path[i]) {
		chunk->path[i]->lun_id = 0;
	    }
	}
	
	if (chunk->udid) {
	    free(chunk->udid);
	}

	chunk->num_blocks = 0;
	chunk->flags = 0;
	chunk->in_use = FALSE;
	chunk->num_paths = 0;

	/*
	 * Remove chunk from hash list
	 */

	if (((ulong)chunk->next & CHUNK_BAD_ADDR_MASK ) ||
	    ((ulong)chunk->prev & CHUNK_BAD_ADDR_MASK )) {

	    /*
	     * Chunk addresses are allocated 
	     * on certain alignment. If these
	     * potential chunk addresses do not 
	     * have the correct alignment then 
	     * print an error to the trace log.
	     */

	    cflsh_blk.num_bad_chunk_ids++;
	    /*
	     * Try continue in this case.
	     */

	    cblk_notify_mc_err(chunk,0,0x208,0, CFLSH_BLK_NOTIFY_SFW_ERR,NULL);

	    CBLK_TRACE_LOG_FILE(1,"Corrupted chunk next address = 0x%p, prev address = 0x%p, hash[] = 0x%p", 
				chunk->next, chunk->prev, cflsh_blk.hash[chunk->index & CHUNK_HASH_MASK]);

	}

	p_hash = &cflsh_blk.hash[chunk->index & CHUNK_HASH_MASK];

	CFLASH_BLOCK_WR_RWLOCK(p_hash->lock);

	if (p_hash->head == chunk)
	{
	    p_hash->head = chunk->next;
	    if (chunk->next) {chunk->next->prev = NULL;}
	    if (p_hash->tail == chunk) {p_hash->tail = NULL;}
	}
	else if (p_hash->tail == chunk)
	{
	    chunk->prev->next = NULL;
	    p_hash->tail      = chunk->prev;
	}
	else
	{
	    chunk->prev->next = chunk->next;
	    chunk->next->prev = chunk->prev;
	}
	CFLASH_BLOCK_RWUNLOCK(p_hash->lock);

    }

    return;
}


/*
 * NAME:        cblk_listio_arg_verify
 *
 * FUNCTION:    Verifies arguments to cblk_listio API
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              flags       - Flags on this request.
 *
 * RETURNS:
 *              0   for good completion
 *              -1  for  error and errno is set.
 *              
 */

int cblk_listio_arg_verify(chunk_id_t chunk_id,
			   cblk_io_t *issue_io_list[],int issue_items,
			   cblk_io_t *pending_io_list[], int pending_items, 
			   cblk_io_t *wait_io_list[],int wait_items,
			   cblk_io_t *completion_io_list[],int *completion_items, 
			   uint64_t timeout,int flags)
{

    int rc = 0;
    cblk_io_t *io;
    int i;                                 /* General counter */



    if ((issue_items == 0) &&
	(pending_items == 0) &&
	(wait_items == 0)) {

	CBLK_TRACE_LOG_FILE(1,"No items specified for chunk_id = %d",
			    chunk_id);
	errno = EINVAL;
	return -1;
	
    }

    if ((wait_items) &&
	(wait_io_list == NULL)) {

	CBLK_TRACE_LOG_FILE(1,"No waiting list items specified for chunk_id = %d, with wait_items = %d",
			    chunk_id,wait_items);
	errno = EINVAL;
	return -1;

    }

    if (completion_items == NULL) {


	CBLK_TRACE_LOG_FILE(1,"No completion list items specified for chunk_id = %d",
			    chunk_id);
	errno = EINVAL;
	return -1;
    }


    if ((*completion_items) &&
	(completion_io_list == NULL)) {

	CBLK_TRACE_LOG_FILE(1,"No completion list items specified for chunk_id = %d with completion_items = %d",
			    chunk_id,*completion_items);
	errno = EINVAL;
	return -1;

    }

    if ((wait_items + *completion_items) < (issue_items + pending_items)) {

	/*
	 * Completion list needs to have enough space to place 
	 * all requests completing in this invocation.
	 */

	CBLK_TRACE_LOG_FILE(1,"completion list too small chunk_id = %d completion_items = %d, wait_items = %d",
			    chunk_id,*completion_items,wait_items);
	errno = EINVAL;
	return -1;

    }



    // TODO:?? This should be modularized into one or more subroutines.
    if (issue_items) {


	if (issue_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"Issue_io_list array is a null pointer for  chunk_id = %d and issue_items = %d",
				chunk_id,issue_items);
	    errno = EINVAL;

	    return -1;

	} 

	
	for (i=0; i< issue_items;i++) {

	    io = issue_io_list[i];


	    if (io == NULL) {


		CBLK_TRACE_LOG_FILE(1,"Issue_io_list[%d] is a null pointer for  chunk_id = %d and issue_items = %d",
				    i,chunk_id,issue_items);
		errno = EINVAL;

		return -1;

	    }

	    io->stat.blocks_transferred = 0;
	    io->stat.fail_errno = 0;
	    io->stat.status = CBLK_ARW_STATUS_PENDING;

	    if (io->buf == NULL) {



		CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
				chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Issue_io_list[%d] is invalid for  chunk_id = %d and issue_items = %d",
				    i,chunk_id,issue_items);
		errno = EINVAL;

		return -1;
		
	    }
	    
	    if ((io->request_type != CBLK_IO_TYPE_READ) &&
		(io->request_type != CBLK_IO_TYPE_WRITE)) {


		CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Issue_io_list[%d] is invalid for  chunk_id = %d and issue_items = %d",
				    i,chunk_id,issue_items);
		errno = EINVAL;

		return -1;
		
	    }

	} /* for */
	    
    }



    if (pending_items) {


	if (pending_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"pending_io_list array is a null pointer for  chunk_id = %d and pending_items = %d",
				chunk_id,pending_items);
	    errno = EINVAL;

	    return -1;

	} 
	
	for (i=0; i< pending_items;i++) {

	    io = pending_io_list[i];


	    if (io == NULL) {


		CBLK_TRACE_LOG_FILE(1,"pending_io_list[%d] is a null pointer for  chunk_id = %d and pending_items = %d",
				    i,chunk_id,pending_items);
		errno = EINVAL;

		return -1;

	    }

	    if (io->buf == NULL) {



		CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
				chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Pending_io_list[%d] is invalid for  chunk_id = %d and pending_items = %d",
				    i,chunk_id,pending_items);
		errno = EINVAL;

		return -1;
		
	    }
	    
	    if ((io->request_type != CBLK_IO_TYPE_READ) &&
		(io->request_type != CBLK_IO_TYPE_WRITE)) {


		CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Issue_io_list[%d] is invalid for  chunk_id = %d and pending_items = %d",
				    i,chunk_id,pending_items);
		errno = EINVAL;

		return -1;
		
	    }

	} /* for */
	    
    }




    if (wait_items) {


	if (wait_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"wait_io_list array is a null pointer for  chunk_id = %d and wait_items = %d",
				chunk_id,wait_items);
	    errno = EINVAL;

	    return -1;

	} 
	
	for (i=0; i< wait_items;i++) {

	    io = wait_io_list[i];


	    if (io == NULL) {


		CBLK_TRACE_LOG_FILE(1,"wait_io_list[%d] is a null pointer for  chunk_id = %d and wait_items = %d",
				    i,chunk_id,wait_items);
		errno = EINVAL;

		return -1;

	    }

	    if (io->buf == NULL) {



		CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
				chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Pending_io_list[%d] is invalid for  chunk_id = %d and pending_items = %d",
				    i,chunk_id,wait_items);
		errno = EINVAL;

		return -1;
		
	    }
	    
	    if ((io->request_type != CBLK_IO_TYPE_READ) &&
		(io->request_type != CBLK_IO_TYPE_WRITE)) {


		CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		CBLK_TRACE_LOG_FILE(1,"Issue_io_list[%d] is invalid for  chunk_id = %d and issue_items = %d",
				    i,chunk_id,wait_items);
		errno = EINVAL;

		return -1;
		
	    }

	    if (io->flags & CBLK_IO_USER_STATUS) {


		CBLK_TRACE_LOG_FILE(1,"Invalid to wait when user status supplied type e = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.status = CBLK_ARW_STATUS_INVALID;
		io->stat.fail_errno = EINVAL;

		errno = EINVAL;

		return -1;
	    }

	} /* for */
	    
    }

	
    if (*completion_items) {


	if (completion_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"completion_io_list array is a null pointer for  chunk_id = %d and completion_items = %d",
				chunk_id,*completion_items);
	    errno = EINVAL;

	    return -1;

	} 
	
	for (i=0; i< wait_items;i++) {

	    io = wait_io_list[i];


	    if (io == NULL) {


		CBLK_TRACE_LOG_FILE(1,"wait_io_list[%d] is a null pointer for  chunk_id = %d and wait_items = %d",
				    i,chunk_id,pending_items);
		errno = EINVAL;

		return -1;

	    }

	} /* for */
	    
    }

    return rc;
}

#ifdef _NOT_YET

/*
 * NAME:        cblk_listio_result
 *
 * FUNCTION:    Checks for results on the specified list supplied
 *              cblk_listio.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              flags       - Flags on this request.
 *
 * RETURNS:
 *              0   for good completion
 *              -1  for  error and errno is set.
 *              
 */

int cblk_listio_result(cflsh_chunk_t *chunk,chunk_id_t chunk_id,
			   cblk_io_t *io_list[],int io_items
		       cblk_io_t *wait_io_list[],int wait_items,
			   int waiting,int *completion_items, 
			   uint64_t timeout,int flags)
{
    int rc = 0;
    int i,j;                               /* General counters */
    cblk_io_t *io;
    struct timespec start_time;
    struct timespec last_time;
    uint64_t uelapsed_time = 0;            /* elapsed time in microseconds */
    int cmd_not_complete;
    cblk_io_t  *wait_io;
    int wait_item_found;


    if (io_items) {

	/*
	 * Caller is requesting I/Os to issued.
	 */


	if (io_list == NULL) {


	    
	    CBLK_TRACE_LOG_FILE(1,"io_list array is a null pointer for  chunk_id = %d and num_items = %d, waiting = %d",
				chunk_id,io_items,waiting);
	    errno = EINVAL;

	    return -1;

	}


	if (waiting) {

	    // TODO:?? Can this be moved to caller.
	    clock_gettime(CLOCK_MONOTONIC,&start_time);
	    clock_gettime(CLOCK_MONOTONIC,&last_time);


	    // TODO: ?? Add macros to replace this.

	    // TODO:?? Can this be moved to caller.
	    uelapsed_time = ((last_time.tv_sec - start_time.tv_sec) * 1000000) + ((last_time.tv_nsec - start_time.tv_nsec)/1000);

	}

	while ((timeout == 0) ||
	       (uelapsed_time < timeout)) {

	    /*
	     * If no time out is specified then only go thru this loop
	     * once. Otherwise continue thru this loop until
	     * our time has elapsed.
	     */

	    if (waiting) {
		cmd_not_complete = FALSE;
	    }


	    for (i=0; i< io_items;i++) {
		
		io = io_list[i];
		

		if (io == NULL) {

		    continue;

		}

		
		if ((io->buf == NULL) && (!waiting)) {
		    
		    
		    
		    CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
					chunk_id,i);
		    
		    
		    io->stat.status = CBLK_ARW_STATUS_INVALID;
		    io->stat.fail_errno = EINVAL;
		    
		    continue;
		    
		}

		if ((io->request_type != CBLK_IO_TYPE_READ) &&
		    (io->request_type != CBLK_IO_TYPE_WRITE)) {


		    CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
					io->request_type,chunk_id,i);


		    io->stat.status = CBLK_ARW_STATUS_INVALID;
		    io->stat.fail_errno = EINVAL;
		    continue;
		}

		if (io->stat.status != CBLK_ARW_STATUS_PENDING) {
		    
		    /*
		     * This I/O request has already completed.
		     * continue to the next wait I/O request.
		     */
		    
		    continue;
		}

		
		
		/*
		 * Process this I/O request
		 */
		
		
		// TODO:?? Need mechanism to specify time-out

		io_flags = 0;


		if ((timeout == 0) && (waiting)) {
		    
		    io_flags |= CBLK_ARESULT_BLOCKING;
		}
		
		if (io->flags & CBLK_IO_USER_TAG) {
		    
		    io_flags |= CBLK_ARESULT_USER_TAG;
		    
		} 
		
		rc = cblk_aresult(chunk_id,&(io->tag),&status,io_flags);
		
		if (rc < 0) {
		    
		    CBLK_TRACE_LOG_FILE(1,"Request failed for chunk_id = %d and index = %d with rc = %d, errno = %d",
					chunk_id,i,rc,errno);
		    
		    
		    // TODO:?? Should we filter on EINVAL and uses a different status?
		    io->stat.status = CBLK_ARW_STATUS_FAIL;
		    io->stat.fail_errno = errno;
		    io->stat.blocks_transferred = 0;
		    
		} else if (rc) {
		    
		    if (!waiting) {

			CBLK_TRACE_LOG_FILE(9,"Request chunk_id = %d and index = %d with rc = %d, errno = %d",
					    chunk_id,i,rc,errno);

			wait_item_found = FALSE;


			if (wait_items) {

			    /*
			     * If there are wait_items, then see
			     * if this item is one of them. If so
			     * update the associated wait_item.
			     */


			    for (j=0; j < wait_items; j++) {

				wait_io = wait_io_list[j];

				if ((wait_io->buf == io->buf) &&
				    (wait_io->lba == io->lba) &&
				    (wait_io->nblocks == io->nblocks) &&
				    (wait_io->tag == io->tag)) {
			    
				    wait_io->stat.status = CBLK_ARW_STATUS_SUCCESS;
				    wait_io->stat.fail_errno = errno;
				    wait_io->stat.blocks_transferred = rc;

				    wait_item_found = TRUE;

				    break;

				}

			    } /* inner for */

			}

			if (!wait_item_found) {

			    if ((complete_io) &&
				(*completion_items <avail_completions)) {
				complete_io->stat.status = CBLK_ARW_STATUS_SUCCESS;
				complete_io->stat.fail_errno = errno;
				complete_io->stat.blocks_transferred = rc;
				complete_io++;
				(*completion_items)++;
			    } else {


				CBLK_TRACE_LOG_FILE(1,"Request chunk_id = %d and index = %d no complete_io entry found",
						    chunk_id,i);
			    }
			}

		    } else {
			io->stat.status = CBLK_ARW_STATUS_SUCCESS;
			io->stat.fail_errno = errno;
			io->stat.blocks_transferred = rc;
		    }
		} else if (waiting) {

		    /*
		     * This command has not completed yet.
		     */

		    cmd_not_complete = TRUE;
		}
		
		
	    }  /* for */

	    if (timeout == 0) {

		/*
		 * Only go thru the while loop one time if
		 * no time out is specified, since we will block until
		 * command completion.
		 */

		break;
	    }

	    if ((cmd_not_complete) && (waiting)) {

		/*
		 * Sleep for one microsecond
		 */

		usleep(1);
	    } else {

		/*
		 * All I/O has completed. So exit this loop.
		 */

		break;
	    }

	    if (waiting) {

		clock_gettime(CLOCK_MONOTONIC,&last_time);


		// TODO: ?? Add macros to replace this.
		uelapsed_time = ((last_time.tv_sec - start_time.tv_sec) * 1000000) + ((last_time.tv_nsec - start_time.tv_nsec)/1000);

	    }


	} /* while */

    }

    return rc;

}

#endif /* _NOT_YET */

/*
 * NAME:        cblk_chk_cmd_bad_page
 *
 * FUNCTION:    This routine checks if the bad page fault address is
 *              associated with a specific command. If so then that 
 *              command is failed.
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *             None
 *              
 */
int cblk_chk_cmd_bad_page(cflsh_chunk_t *chunk, uint64_t bad_page_addr)
{
    int i;
    int found_cmd = FALSE;
    cflsh_cmd_mgm_t *cmd = NULL;
#ifdef _COMMON_INTRPT_THREAD	
    int pthread_rc = 0;
#endif /* _COMMON_INTRPT_THREAD	*/
    
    
    if (chunk->num_active_cmds) {
	
	for (i=0; i < chunk->num_cmds; i++) {
	    if ((chunk->cmd_info[i].in_use) &&
		((chunk->cmd_info[i].state == CFLSH_MGM_WAIT_CMP) ||
		 (chunk->cmd_info[i].state == CFLSH_MGM_HALTED))) {
		
		/*
		 * cmd_info and cmd_start array entries
		 * with the same index correspond to the
		 * same command.
		 */
		
		cmd = &chunk->cmd_start[i];
		
		if ((bad_page_addr >= (uint64_t)cmd) &&
		    (bad_page_addr <= ((uint64_t)(cmd) + sizeof(*cmd)))) {

		    /*
		     * Bad page fault is associated with this command
		     */
		    found_cmd = TRUE;

		    cblk_notify_mc_err(chunk,cmd->cmdi->path_index,0x209,bad_page_addr, CFLSH_BLK_NOTIFY_SFW_ERR,cmd);

		    CBLK_TRACE_LOG_FILE(5,"Bad page addr = 0x%llx is command",bad_page_addr);

		} else  if ((bad_page_addr >= (uint64_t) chunk->cmd_info[i].buf) &&
			    (bad_page_addr <= ((uint64_t) (chunk->cmd_info[i].buf) + (chunk->cmd_info[i].nblocks * CAPI_FLASH_BLOCK_SIZE)))) {

		    /*
		     * Bad page fault is associated with this command's data buffer: user/caller error
		     */
		    found_cmd = TRUE;

		    cblk_notify_mc_err(chunk,cmd->cmdi->path_index,0x20a,bad_page_addr, CFLSH_BLK_NOTIFY_SFW_ERR,cmd);
		    
		    CBLK_TRACE_LOG_FILE(5,"Bad page addr = 0x%llx is data buffer",bad_page_addr);

		}


		if (found_cmd) {
		    
		    /*
		     * Fail this command.
		     */
		    
		    
		    cmd->cmdi->status = EIO;
		    cmd->cmdi->transfer_size = 0;
		    
		    CBLK_TRACE_LOG_FILE(6,"cmd failed  lba = 0x%llx flags = 0x%x, chunk->index = %d",
					cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
		    
		    
		    /*
		     * Fail command back.
		     */
		    
		    cmd->cmdi->state = CFLSH_MGM_CMP;
		    
#ifdef _COMMON_INTRPT_THREAD
		    
		    if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
			
			/*
			 * If we are using a common interrupt thread
			 */
			
			pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
			
			if (pthread_rc) {
			    
			    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
						pthread_rc,errno,chunk->index);
			}
			
			
			/*
			 * Signal any one waiting for any command to complete.
			 */
			
			pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
			
			if (pthread_rc) {
			    
			    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
						pthread_rc,errno,chunk->index);
			}
		    }
		    
#endif /* _COMMON_INTRPT_THREAD	*/
		    
		    break;

		} /* if found_cmd */

		
	    }
	    
	} /* for */
	
    }

    
    return found_cmd;
}


/*
 * NAME:        cblk_fail_all_cmds
 *
 * FUNCTION:    This routine fails all commands
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *             None
 *              
 */
void cblk_fail_all_cmds(cflsh_chunk_t *chunk)
{
    int i;
    cflsh_cmd_mgm_t *cmd = NULL;
#ifdef _COMMON_INTRPT_THREAD	
    int pthread_rc = 0;
#endif /* _COMMON_INTRPT_THREAD	*/
    
    if (chunk->num_active_cmds) {
	

	
	CBLK_LIVE_DUMP_THRESHOLD(9,"0x201");


	for (i=0; i < chunk->num_cmds; i++) {
	    if ((chunk->cmd_info[i].in_use) &&
		((chunk->cmd_info[i].state == CFLSH_MGM_WAIT_CMP) ||
		 (chunk->cmd_info[i].state == CFLSH_MGM_HALTED))) {

		/*
		 * cmd_info and cmd_start array entries
		 * with the same index correspond to the
		 * same command.
		 */

		cmd = &chunk->cmd_start[i];
		
		/*
		 * Fail this command.
		 */
		
		
		cmd->cmdi->status = EIO;
		cmd->cmdi->transfer_size = 0;
		
		CBLK_TRACE_LOG_FILE(6,"cmd failed  lba = 0x%llx flags = 0x%x, chunk->index = %d",
				    cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
		
		
		/*
		 * Fail command back.
		 */
		
		cmd->cmdi->state = CFLSH_MGM_CMP;
	
#ifdef _COMMON_INTRPT_THREAD

		if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
		    /*
		     * If we are using a common interrupt thread
		     */
			
		    pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
		
		    if (pthread_rc) {
			
			CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
					    pthread_rc,errno,chunk->index);
		    }
		    
		    
		    /*
		     * Signal any one waiting for any command to complete.
		     */
		    
		    pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
		    
		    if (pthread_rc) {
			
			CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
					    pthread_rc,errno,chunk->index);
		    }
		}
		
#endif /* _COMMON_INTRPT_THREAD	*/
		
		
	    }
	    
	} /* for */
	
    }

    return;
}

/*
 * NAME:        cblk_halt_all_cmds
 *
 * FUNCTION:    This routine halts all commands. It assumes
 *              the AFU is being reset or about to reset.
 *              Thus it can mark all active commands in a halt
 *              state.
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *             None
 *              
 */
void cblk_halt_all_cmds(cflsh_chunk_t *chunk, int path_index, int all_paths)
{
    int i;
    

    chunk->flags |= CFLSH_CHNK_HALTED;


    /*
     * TODO:?? There maybe a race condition here. If we have an AFU shared
     *       by multiple chunks/paths, then the first one reset will correctly
     *       halt and resume. However other paths will not halt prior to context reset
     *       and thus when this routine is called for them, they may not get
     *       flushed again.
     */

    if (chunk->num_active_cmds) {
	
	for (i=0; i < chunk->num_cmds; i++) {
	    if ((chunk->cmd_info[i].in_use) &&
		 (chunk->cmd_info[i].state == CFLSH_MGM_WAIT_CMP) &&
		(all_paths || (chunk->cmd_info[i].path_index == path_index))) {
		
		/*
		 * Halt this command.
		 */
		
		chunk->cmd_info[i].state = CFLSH_MGM_HALTED;
		
		
	    }
	    
	} /* for */
	
    }

    return;
}

/*
 * NAME:        cblk_resume_all_halted_cmds
 *
 * FUNCTION:    This routine resumes all haltdd commands. It assumes
 *              the AFU reset is complete.
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *             None
 *              
 */
void cblk_resume_all_halted_cmds(cflsh_chunk_t *chunk, int increment_retries, 
				 int path_index, int all_paths)
{
    int i,j;
    int rc = 0;
    cflsh_cmd_mgm_t *cmd = NULL;
    cflsh_cmd_info_t *cmdi;
    int pthread_rc = 0;

    
    /*
     * TODO:?? There maybe a race condition here. If we have an AFU shared
     *       by multiple chunks/paths, then the first one reset will correctly
     *       halt and resume. However other paths will not halt prior to context reset
     *       and thus when this routine is called for them, they may not get
     *       flushed again.
     */


    chunk->flags &= ~CFLSH_CHNK_HALTED;

    if (chunk->num_active_cmds) {
	

	CBLK_TRACE_LOG_FILE(9,"resuming %d active commands",chunk->num_active_cmds);

	for (i=0; i < chunk->num_cmds; i++) {
	    if ((chunk->cmd_info[i].in_use) &&
		(chunk->cmd_info[i].state == CFLSH_MGM_HALTED) &&
		(all_paths || (chunk->cmd_info[i].path_index == path_index))) {

		/*
		 * cmd_info and cmd_start array entries
		 * with the same index correspond to the
		 * same command.
		 */

		cmd = &chunk->cmd_start[i];
		
		/*
		 * Resume this command.
		 */
		
		
		
		CBLK_INIT_ADAP_CMD_RESP(chunk,cmd);
    

		cmdi = &chunk->cmd_info[cmd->index];

		
		cmdi->cmd_time = time(NULL);

		if (increment_retries) {
		    cmdi->retry_count++;
		}

		if (cmdi->retry_count < CAPI_CMD_MAX_RETRIES) {


		    if (chunk->num_paths > 1) {

			/*
			 * If we have more than one path, then 
			 * allow retry down another path.
			 */

			CBLK_TRACE_LOG_FILE(9,"multiple paths, num_paths = %d path_index = %d",chunk->num_paths,path_index);
			for (j=0;j< chunk->num_paths; j++) {

			    if ((chunk->path[j]) &&
				(chunk->path[j]->flags & CFLSH_PATH_ACT) &&
				(j != path_index)) {

				/*
				 * This is a valid path.  So select it
				 * and specify a retry for this path.
				 */

				cmdi->path_index = j;

				CBLK_TRACE_LOG_FILE(9,"Retry path_index = %d",j);

				chunk->stats.num_path_fail_overs++;


				break;
		
			    }

			}

		    }


		    /*
		     * Update command possibly for new path or updated context id.
		     */

		    if (CBLK_UPDATE_PATH_ADAP_CMD(chunk,cmd,0)) {

			CBLK_TRACE_LOG_FILE(1,"CBLK_UPDATE_PATH_ADAP_CMD failed");

			rc = -1;
		    }
		  
		    if (!rc) {

			rc = CBLK_ISSUE_CMD(chunk,cmd,cmdi->buf,
					cmd->cmdi->lba,cmd->cmdi->nblocks,CFLASH_ISSUE_RETRY);
		    }

		    if (rc) {

			/*
			 * If we failed to issue this command, then fail it
			 */

			CBLK_TRACE_LOG_FILE(8,"resume issue failed with rc = 0x%x cmd->cmdi->lba = 0x%llx chunk->index = %d",
					    rc,cmd->cmdi->lba,chunk->index);
			cmd->cmdi->status = EIO;

			cmd->cmdi->transfer_size = 0;


		
			/*
			 * Fail command back.
			 */
		
			cmd->cmdi->state = CFLSH_MGM_CMP;
	
#ifdef _COMMON_INTRPT_THREAD

			if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
			    /*
			     * If we are using a common interrupt thread
			     */
			
			    pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
		
			    if (pthread_rc) {
		    
				CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
						    pthread_rc,errno,chunk->index);
			    }
			    
			    
			    /*
			     * Signal any one waiting for any command to complete.
			     */
			    
			    pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
			    
			    if (pthread_rc) {
				
				CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
						    pthread_rc,errno,chunk->index);
			    }
			}
		
#endif /* _COMMON_INTRPT_THREAD	*/
		    }

		} else {


		    /*
		     * If we exceeded retries then
		     * give up on it now.
		     */

		    if (!(cmd->cmdi->status)) {
			    
			cmd->cmdi->status = EIO;
		    }

		    cmd->cmdi->transfer_size = 0;


		
		    /*
		     * Fail command back.
		     */
		
		    cmd->cmdi->state = CFLSH_MGM_CMP;
	
#ifdef _COMMON_INTRPT_THREAD

		    if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
			/*
			 * If we are using a common interrupt thread
			 */
			
			pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
		
			if (pthread_rc) {
		    
			    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
						pthread_rc,errno,chunk->index);
			}
			
			
			/*
			 * Signal any one waiting for any command to complete.
			 */
			
			pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
			
			if (pthread_rc) {
			    
			    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
						pthread_rc,errno,chunk->index);
			}
		    }
		
#endif /* _COMMON_INTRPT_THREAD	*/
		}
	    }
		
	} /* for */
	
    } /* if num_active_cmds */


    /*
     * It is possible some commands were in the process of being issued, but
     * went to sleep because of this halted state. In that case, the command
     * will have a state of CFLSH_MGM_PENDFREE. Since the recovery operation,
     * may have taken considerable time, we need to set the initial cmd_time 
     * to now to avoid them being prematurely timed out. In addition, if
     * this halt/resume was due to an adapter reset, the context ID may
     * have also changed. So we need to update that as well.
     */

    
    for (i=0; i < chunk->num_cmds; i++) {
	if ((chunk->cmd_info[i].in_use) &&
	    (chunk->cmd_info[i].state == CFLSH_MGM_PENDFREE) &&
	    (all_paths || (chunk->cmd_info[i].path_index == path_index))) {
		
	    /*
	     * cmd_info and cmd_start array entries
	     * with the same index correspond to the
	     * same command.
	     */

	    cmd = &chunk->cmd_start[i];

	    cmdi = &chunk->cmd_info[i];

	    cmdi->cmd_time = time(NULL);
		    
	    if (CBLK_UPDATE_PATH_ADAP_CMD(chunk,cmd,0)) {

		CBLK_TRACE_LOG_FILE(1,"CBLK_UPDATE_PATH_ADAP_CMD failed");


	    }

		
	}
	    
    } /* for */    

    // TOD0:?? What about reseting one AFU and using the other?



    /*
     * NOTE: Even if we have no background thread, this is still valid.
     *       If we are being used by a single threaded process, then there
     *       will never be anything waiting to wake up. If we are being used
     *       by a multi-thread process, then there could be threads blocked
     *       waiting to resume.
     *   
     *       The assumption here is that who ever halts the commands will
     *       resume them before exiting this library.
     */

    pthread_rc = pthread_cond_broadcast(&(chunk->path[chunk->cur_path]->resume_event));
	
    if (pthread_rc) {
	    
	CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for resume_event rc = %d,errno = %d",
			    pthread_rc,errno);
    }



    CBLK_TRACE_LOG_FILE(9,"finished resuming %d active commands",chunk->num_active_cmds);
    return;
}

/*
 * NAME:        cblk_reset_context_shared_afu
 *
 * FUNCTION:    This routine will first determine which chunk's paths
 *              are associated with this shared AFU, then halt all commands
 *              that are active on this AFU. then reset the context and
 *              resume-retry the halted commands.
 *             
 *
 * Environment: This routine assumes no locks are taken.
 *
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *             None
 *              
 */
void cblk_reset_context_shared_afu(cflsh_afu_t *afu)
{
    cflsh_chunk_t *chunk = NULL;
    cflash_block_check_os_status_t status;
    cflsh_path_t *path;
    int reset_context_success =  TRUE;
    int detach = FALSE;
    int pthread_rc = 0;
    int path_index = 0;
    time_t timeout;

    if (afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"AFU is null");
    }
    
    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);
    
    timeout = time(NULL) - 1;

    CFLASH_BLOCK_LOCK(afu->lock);

    if (afu->flags & (CFLSH_AFU_HALTED|CFLSH_AFU_RECOV)) {

	CBLK_TRACE_LOG_FILE(5,"skipping context reset, since AFU is halted or recovering is pending");

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	CFLASH_BLOCK_UNLOCK(afu->lock);

	return;
    }
	
    if (afu->reset_time > timeout) {

	CBLK_TRACE_LOG_FILE(5,"afu reset done in the last second");
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	CFLASH_BLOCK_UNLOCK(afu->lock);
    
	return;
    }

    afu->flags |= CFLSH_AFU_HALTED;
    
    CFLASH_BLOCK_UNLOCK(afu->lock);
	
	
    CBLK_TRACE_LOG_FILE(9,"reseting context...");

    path = afu->head_path;

    while (path) {


	chunk = path->chunk;

	if (chunk == NULL) {
	    CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
	    continue;
	}

	CFLASH_BLOCK_LOCK(chunk->lock);

	path_index = path->path_index;
	cblk_halt_all_cmds(chunk,path_index, FALSE);
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	path = path->next;
    }
    
    
    if (chunk) {
	
	/*
	 * We found at least one chunk associated with this afu.
	 * Thus we'll reset the context by driving it from this
	 * chunk.  After this all commands
	 * need to be failed/resumed.
	 */
	
	
	
	if (CBLK_RESET_ADAP_CONTEXT(chunk,path_index)) {
	    
	    /* 
	     * Reset context failed
	     */
	    reset_context_success = FALSE;
	    
	    /*
	     * Check for adapter reset 
	     */

	    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

	    CFLASH_BLOCK_LOCK(chunk->lock);

	    status = cblk_check_os_adap_err(chunk,path_index);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    CBLK_TRACE_LOG_FILE(1,"reset context failed for path_index of %d, status = %d",path_index,status);

	    if (status != CFLSH_BLK_CHK_OS_NO_RESET) {

		/*
		 * If some type of reset was done, then
		 * exit here. If a reset was done then
		 * this supersedes context reset. If reset
		 * failed then it should have also done the
		 * clean up and failing of all I/O for this
		 * chunk.
		 */

		CBLK_TRACE_LOG_FILE(9,"AFU reset was done assuming context reset is unnecessary");
    
		return;

		
	    }

	    /*
	     * Drop thru and clean up
	     */

	    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);

	} 
	
	/*
	 * We need to explicitly take the afu
	 * lock here, since we are sending a broadcast
	 * to other threads. So we can not use
	 * the conditional taking of the afu lock
	 * via CFLASH_BLOCK_AFU_SHARE_LOCK. We'll
	 * explicitly release after the broadcast
	 */

	CFLASH_BLOCK_LOCK(afu->lock);
	
	
	afu->reset_time = time(NULL);

	afu->flags &= ~CFLSH_AFU_HALTED;

	afu->num_issued_cmds = 0;
	
	pthread_rc = pthread_cond_broadcast(&(afu->resume_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for AFU resume_event rc = %d,errno = %d",
				pthread_rc,errno);
	}	
	
	CFLASH_BLOCK_UNLOCK(afu->lock);

	path = afu->head_path;
	
	while (path) {
	    
	    
	    chunk = path->chunk;
	    
	    if (chunk == NULL) {
		CBLK_TRACE_LOG_FILE(1,"chunk is null for path_index of %d",path->path_index);
		continue;
	    }
	    
	    CFLASH_BLOCK_LOCK(chunk->lock);
	    
	    
	    if (reset_context_success) {
		
		cblk_resume_all_halted_cmds(chunk, TRUE,
					    path->path_index, FALSE);
	    } else {
		/*
		 * If any context reset failed, then fail all
		 * I/O.
		 */

		cblk_notify_mc_err(chunk,path_index,0x200,0, CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

		chunk->flags |= CFLSH_CHUNK_FAIL_IO;

		/*
		 * Clear halted state so that subsequent commands
		 * do not hang, but are failed immediately.
		 */

		chunk->flags &= ~CFLSH_CHNK_HALTED;
		
		cblk_chunk_free_mc_device_resources(chunk);
		
		if (!detach) {
		    
		    /*
		     * Only unmap and detach this AFU once
		     */
		    
		    detach = TRUE;
		    cblk_chunk_unmap(chunk,TRUE);
		    
		    cblk_chunk_detach(chunk,TRUE);
		    
		    
		}
		
		close(chunk->fd);
		
		
		/*
		 * Fail all other commands. We are allowing the commands
		 * that saw the time out to be failed with ETIMEDOUT.
		 * All other commands are failed here with EIO.
		 */
		
		cblk_fail_all_cmds(chunk);
		
		
	    }
	    
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    path = path->next;
	}

    
    }
    

    CBLK_TRACE_LOG_FILE(9,"reset context complete");
    
    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
    
    return;
}


/*
 * NAME:        cblk_retry_new_path
 *
 * FUNCTION:    Determine if there is another path this can
 *              be retried with. There are two scenarios here:	
 *
 *               - virtual lun using both paths from same AFU.
 *               - physical lun with multiple paths (AIX).
 *
 *
 * INPUTS:
 *              chunk - Chunk to which file I/O is being done.
 *              cmd   - Command for which we are doing I/O
 *
 * RETURNS:
 *              NONE
 *              
 */

int cblk_retry_new_path(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd, int delay_needed_same_afu)
{
    cflash_cmd_err_t rc = CFLASH_CMD_FATAL_ERR;
#ifdef _KERNEL_MASTER_CONTXT
    int cur_path;
#ifdef _AIX
    int i;
#endif /* AIX */
#endif /* !_KERNEL_MASTER_CONTXT */


    if ((chunk == NULL) || (cmd == NULL)) {

	CBLK_TRACE_LOG_FILE(5,"chunk or cmd is NULL");
	return rc;
    }

#ifdef _KERNEL_MASTER_CONTXT

    cur_path = chunk->cmd_info[cmd->index].path_index;


    if (chunk->path[cur_path] == NULL) {

	CBLK_TRACE_LOG_FILE(5,"path is NULL, cur_path = %d, cmd_index = 0x%x",cur_path,cmd->index);
	return rc;

    }

    if (chunk->path[cur_path]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(5,"afu is NULL");
	return rc;

    }

    CBLK_TRACE_LOG_FILE(9,"cur_path = %d, cmd_index = 0x%x",cur_path,cmd->index);

    if (chunk->path[cur_path]->num_ports > 1) {

	/*
	 * This AFU has multiple ports connected to this same
	 * path to this lun. So allow retry on the same host selected path
	 * (the AFU should try the other path FC port/path).
	 */
	
	CBLK_TRACE_LOG_FILE(9,"multiple ports delay_needed_same_afu = %d",delay_needed_same_afu);
	if (delay_needed_same_afu) {
	    rc = CFLASH_CMD_DLY_RETRY_ERR;
	} else {
	    rc = CFLASH_CMD_RETRY_ERR;
	}



#ifdef _AIX

    

    } else if (chunk->num_paths > 1) {

	/*
	 * If we have more than one path, then 
	 * attempt retry down alternate path.
	 */

	

	CBLK_TRACE_LOG_FILE(9,"multiple paths, num_paths = %d path_index = %d",chunk->num_paths,cur_path);
	for (i=0;i< chunk->num_paths; i++) {

	    if ((chunk->path[i]) &&
		(chunk->path[i]->flags & CFLSH_PATH_ACT) &&
		(i != cur_path)) {

		/*
		 * This is a valid path.  So select it
		 * and specify a retry for this path.
		 */


		CBLK_NOTIFY_LOG_THRESHOLD(3,chunk,(chunk->cmd_info[cmd->index].path_index),0x20c,
					  i,CFLSH_BLK_NOTIFY_SFW_ERR,cmd);


		chunk->cmd_info[cmd->index].path_index = i;

		CBLK_TRACE_LOG_FILE(9,"Retry path_index = %d",i);


		if ((chunk->path[cur_path]->afu == chunk->path[i]->afu) &&
		    (delay_needed_same_afu)) {

		    /*
		     * If the new path has the same AFU
		     * and the caller requested delay on
		     * on the same AFU, then do so.
		     */

		    rc = CFLASH_CMD_DLY_RETRY_ERR;
		} else {
		    rc = CFLASH_CMD_RETRY_ERR;
		}

		chunk->stats.num_path_fail_overs++;

		break;

		
	    }

	}

	if (chunk->cmd_info[cmd->index].path_index == cur_path) {

	    /*
	     * If no new paths are found, then
	     * give retry any way on same path.
	     */
	     
	    if (delay_needed_same_afu)	 {

		/*
		 * If no alternate path was found,
		 * then only do retry down current path
		 * if delay is needed for same AFU.
		 */

		rc = CFLASH_CMD_DLY_RETRY_ERR;
	    } else {
		rc = CFLASH_CMD_RETRY_ERR;


	    }

	}

#endif /* AIX */


    } else {


	/*
	 * This routine is only called for situations where a 
	 * retry might be valid. Thus at the very minimum do
	 * the retry, but for the same path. If the caller
	 * indicated a delay also, then do a delay retry.
	 */

	if (delay_needed_same_afu) {
	    rc = CFLASH_CMD_DLY_RETRY_ERR;
	} else {
	    rc = CFLASH_CMD_RETRY_ERR;
	}
    }


    if ((rc == CFLASH_CMD_DLY_RETRY_ERR) ||
	(rc == CFLASH_CMD_RETRY_ERR)) {

	/*
	 * Update command possibly for new path or context
	 */

	if (CBLK_UPDATE_PATH_ADAP_CMD(chunk,cmd,0)) {

	    CBLK_TRACE_LOG_FILE(1,"CBLK_UPDATE_PATH_ADAP_CMD failed");

	    rc = CFLASH_CMD_FATAL_ERR;
	}
		  


    } 

#else
    
    /*
     * For non-kernel MC assume the AFU has both
     * FC ports attached to the same lun and
     * thus do a retry.
     */

    rc = CFLASH_CMD_RETRY_ERR;

#endif /* !_KERNEL_MASTER_CONTXT */


    return rc;
}




#ifdef BLOCK_FILEMODE_ENABLED
/*
 * NAME:        cblk_filemde_io
 *
 * FUNCTION:    Issue I/O to file instead of a lun
 *
 *
 * INPUTS:
 *              chunk - Chunk to which file I/O is being done.
 *              cmd   - Command for which we are doing I/O
 *
 * RETURNS:
 *              NONE
 *              
 */

void cblk_filemode_io(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd)
{
    size_t lseek_rc;  
    int rc = 0;


#ifdef _AIX
    uint32_t  tmp_val;
#endif /* AIX */



    if (cmd->cmdi == NULL) {
	CBLK_TRACE_LOG_FILE(1,"cmdi is invalid for chunk->index = %d",chunk->index);

    }


    CBLK_TRACE_LOG_FILE(5,"llseek to lba = 0x%llx, chunk->index = %d",cmd->cmdi->lba,chunk->index);

#ifdef _AIX
    lseek_rc = llseek(chunk->fd,((cmd->cmdi->lba) * CAPI_FLASH_BLOCK_SIZE ),SEEK_SET);
#else
    lseek_rc = lseek(chunk->fd,((cmd->cmdi->lba) * CAPI_FLASH_BLOCK_SIZE ),SEEK_SET);
#endif 
    
    if (lseek_rc == ((cmd->cmdi->lba) * CAPI_FLASH_BLOCK_SIZE )) {
	

	if (cmd->cmdi->flags & CFLSH_MODE_READ) {
	    
	    rc = read(chunk->fd,cmd->cmdi->buf,CBLK_GET_CMD_DATA_LENGTH(chunk,cmd));
	    
	} else if (cmd->cmdi->flags & CFLSH_MODE_WRITE) {
	    
	    rc = write(chunk->fd,cmd->cmdi->buf,CBLK_GET_CMD_DATA_LENGTH(chunk,cmd));
	}
	
	if (rc) {
	    /*
	     * Convert file mode rc (number of bytes
	     * read/written) into cblk rc (number
	     * of blocks read/written)
	     */
	    rc = rc/CAPI_FLASH_BLOCK_SIZE;
	}
	
    } else {
	CBLK_TRACE_LOG_FILE(1,"llseek failed for lba = 0x%llx,,errno = %d, chunk->index = %d",
			    cmd->cmdi->lba,errno,chunk->index);
	rc = -1;
	/*
	 * If we failed  this I/O
	 * request. For now
	 * just an arbitrary error.
	 */
	
	
        CBLK_SET_ADAP_CMD_RSP_STATUS(chunk,cmd,FALSE);
    }
    
    
    if (rc == cmd->cmdi->nblocks) {
	
	/*
	 * Data was trasnferred, return good completion
	 */
	
        CBLK_SET_ADAP_CMD_RSP_STATUS(chunk,cmd,TRUE);
	rc = 0;
    } else {
	
	/*
	 * If we failed  this I/O
	 * request. For now
	 * just an arbitrary error.
	 */
	
	
        CBLK_SET_ADAP_CMD_RSP_STATUS(chunk,cmd,FALSE);

    }
   
    
#if !defined(__64BIT__) && defined(_AIX)
    /*
     * Compiler complains about 
     * recasting and assigning directly
     * from cmd into p_hrrq_curr. So
     * we use a two step process.
     */

    if (chunk->path[chunk->cur_path]->afu->flags & CFLSH_AFU_SQ) {
	tmp_val = (uint32_t)&(cmd->sisl_cmd.sa);
    } else {
	tmp_val = (uint32_t)cmd;
    }
    *(chunk->path[chunk->cur_path]->afu->p_hrrq_curr) = tmp_val | chunk->path[chunk->cur_path]->afu->toggle;
#else

   if (chunk->path[chunk->cur_path]->afu->flags & CFLSH_AFU_SQ) {
       *(chunk->path[chunk->cur_path]->afu->p_hrrq_curr) = (uint64_t)&(cmd->sisl_cmd.sa)| chunk->path[chunk->cur_path]->afu->toggle;
   } else {
       
       *(chunk->path[chunk->cur_path]->afu->p_hrrq_curr) = (uint64_t) cmd | chunk->path[chunk->cur_path]->afu->toggle;
   }
#endif
    
    
    CBLK_TRACE_LOG_FILE(7,"*(chunk->path[chunk->cur_path].p_hrrq_curr) = 0x%llx, chunk->path[chunk->cur_path].toggle = 0x%llx, chunk->index = %d",
			CBLK_READ_ADDR_CHK_UE(chunk->path[chunk->cur_path]->afu->p_hrrq_curr),chunk->path[chunk->cur_path]->afu->toggle,
			chunk->index);	

}

#endif /* BLOCK_FILEMODE_ENABLED */ 


/*
 * NAME:        cblk_process_sense_data
 *
 * FUNCTION:    This routine parses sense data
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
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
cflash_cmd_err_t cblk_process_sense_data(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd, struct request_sense_data *sense_data)
{
    cflash_cmd_err_t rc = CFLASH_CMD_IGNORE_ERR;



    if (sense_data == NULL) {

	cmd->cmdi->status = EIO;
	return CFLASH_CMD_FATAL_ERR;
    }

    CBLK_TRACE_LOG_FILE(5,"sense data: error code = 0x%x, sense_key = 0x%x, asc = 0x%x, ascq = 0x%x",
			sense_data->err_code, sense_data->sense_key, 
			sense_data->add_sense_key, 
			sense_data->add_sense_qualifier);



    switch (sense_data->sense_key) {


    case CFLSH_NO_SENSE:   
	/*
	 * Ignore error and treat as good completion
	 */
	rc = CFLASH_CMD_IGNORE_ERR;

	break;      
    case CFLSH_RECOVERED_ERROR: 

	/*
	 * Ignore error and treat as good completion
	 * However log it.
	 */
	rc = CFLASH_CMD_IGNORE_ERR;

	cblk_notify_mc_err(chunk,chunk->cur_path,0x201,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);
	break;
    case CFLSH_NOT_READY:  
           
	/*
	 * Retry command
	 */
	cmd->cmdi->status = EIO;
	rc = CFLASH_CMD_RETRY_ERR;

	break;
    case CFLSH_MEDIUM_ERROR:         
    case CFLSH_HARDWARE_ERROR:   
	/*
	 * Fatal error do not retry.
	 */
	cmd->cmdi->status = EIO;
	rc = CFLASH_CMD_FATAL_ERR;

	cblk_notify_mc_err(chunk,chunk->cur_path,0x202,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);

	break;      
    case CFLSH_ILLEGAL_REQUEST:   
	/*
	 * Fatal error do not retry.
	 */
	cmd->cmdi->status = EIO;
	rc = CFLASH_CMD_FATAL_ERR;

	cblk_notify_mc_err(chunk,chunk->cur_path,0x207,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);

	break;       
    case CFLSH_UNIT_ATTENTION: 
	

	switch (sense_data->add_sense_key) {

	case 0x29:
	    /*
	     * Power on Reset or Device Reset. 
	     * Retry command for now.
	     */
		
	    cmd->cmdi->status = EIO;

	    
	    if (cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd,sense_data)) {
		
		/*
		 * Verification failed
		 */

		rc = CFLASH_CMD_FATAL_ERR;

	    } else {

		rc = CFLASH_CMD_RETRY_ERR;
	    }
	    break;
	case 0x2A:
	    /*
	     * Device settings/capacity has changed
	     * Retry command for now.
	     */



	    cmd->cmdi->status = EIO;

	    
	    if (cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd,sense_data)) {
		
		/*
		 * Verification failed
		 */

		rc = CFLASH_CMD_FATAL_ERR;

	    } else {

		rc = CFLASH_CMD_RETRY_ERR;
	    }
	    break;
	case 0x3f:
	    

	    if (sense_data->add_sense_qualifier == 0x0e) {

		/*
		 * Report Luns data has changed
		 * Retry command for now.
		 */

		
		cmd->cmdi->status = EIO;
	    
		if (cblk_verify_mc_lun(chunk,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd,sense_data)) {
		
		    /*
		     * Verification failed
		     */

		    rc = CFLASH_CMD_FATAL_ERR;

		} else {

		    rc = CFLASH_CMD_RETRY_ERR;
		}
		break;

	    }
	    
	    /* Fall thru */
	default:
	    /*
	     * Fatal error
	     */

	    
	    cmd->cmdi->status = EIO;
	    
	    rc = CFLASH_CMD_FATAL_ERR;
	    
	    cblk_notify_mc_err(chunk,chunk->cur_path,0x203,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);
	    
	}
	    
	break;
    case CFLSH_DATA_PROTECT:         
    case CFLSH_BLANK_CHECK:          
    case CFLSH_VENDOR_UNIQUE:        
    case CFLSH_COPY_ABORTED:         
    case CFLSH_ABORTED_COMMAND:      
    case CFLSH_EQUAL_CMD:            
    case CFLSH_VOLUME_OVERFLOW:      
    case CFLSH_MISCOMPARE: 
    default:
	
  
	/*
	 * Fatal error do not retry.
	 */

	
	cblk_notify_mc_err(chunk,chunk->cur_path,0x204,0,CFLSH_BLK_NOTIFY_SCSI_CC_ERR,cmd);

	rc = CFLASH_CMD_FATAL_ERR;
	
	cmd->cmdi->status = EIO;

	
	CBLK_TRACE_LOG_FILE(1,"Fatal generic error sense data: sense_key = 0x%x, asc = 0x%x, ascq = 0x%x",
			    sense_data->sense_key,sense_data->add_sense_key, sense_data->add_sense_qualifier);
	break;   
	
    }

	
    return rc;
}


#ifdef _COMMON_INTRPT_THREAD
/*
 * NAME:        cblk_intrpt_thread
 *
 * FUNCTION:    This routine is invoked as a common 
 *              interrupt handler thread for all threads
 *              for this chunk.
 *
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              data - of type cflsh_async_thread_cmp_t 
 *
 * RETURNS:
 *              
 */
void *cblk_intrpt_thread(void *data)
{
    void *ret_code = NULL;
    int log_error;
    cflsh_async_thread_cmp_t *async_data = data;
    cflsh_chunk_t *chunk = NULL;
    int pthread_rc = 0;
    int tag;
    size_t transfer_size;
    cflsh_cmd_mgm_t *cmd = NULL;
    cflsh_cmd_info_t *cmdi = NULL;
    time_t timeout;
    int path_reset_index[CFLSH_BLK_MAX_NUM_PATHS];
    int reset_context = FALSE;
#ifdef BLOCK_FILEMODE_ENABLED
    int i;
    volatile uint64_t *p_hrrq_curr;
    uint64_t toggle;
#endif /* BLOCK_FILEMODE_ENABLED */
    cflash_block_check_os_status_t reset_status = CFLSH_BLK_CHK_OS_NO_RESET;


    
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

    chunk = async_data->chunk;

    if (chunk == NULL) {

	CBLK_TRACE_LOG_FILE(5,"chunk filename = %s cmd_index = %d, cmd is NULL",
			    async_data->chunk->dev_name);

	return (ret_code);
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"chunk filename = %s pthread_cond_wait failed rc = %d errno = %d",
			    async_data->chunk->dev_name,pthread_rc,errno);

	return (ret_code);
    }

    CBLK_TRACE_LOG_FILE(5,"start of thread chunk->index = %d",chunk->index);

    while (TRUE) {

	CFLASH_BLOCK_LOCK(chunk->lock);

	if (CFLSH_EYECATCH_CHUNK(chunk)) {
	    /*
	     * Invalid chunk. Exit now.
	     */

	    cflsh_blk.num_bad_chunk_ids++;
	    CBLK_TRACE_LOG_FILE(1,"chunk filename = %s invalid chunk eye catcher 0x%x",
				    async_data->chunk->dev_name,chunk->eyec);

	    CBLK_LIVE_DUMP_THRESHOLD(9,"0x20d");
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return (ret_code);
	}
	    

	if ((chunk->num_active_cmds == 0) &&
	    (chunk->flags & CFLSH_CHUNK_FAIL_IO)) {


	    /*
	     * If we have no active I/O
	     * and this chunk is in a failed
	     * state, then treat this as an
	     * exit for this thread.
	     */


	    CBLK_TRACE_LOG_FILE(5,"exiting thread: chunk->index = %d because CFLSH_CHUNK_FAIL_IO",
				chunk->index);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    break;
	}

	if (!(chunk->thread_flags) &&
	    (chunk->num_active_cmds == 0)) {
    
	    /*
	     * Only wait if the thread_flags
	     * has not been set and there are no active commands
	     */
	    pthread_rc = pthread_cond_wait(&(chunk->thread_event),&(chunk->lock.plock));
	
	    if (pthread_rc) {
	    
		CBLK_TRACE_LOG_FILE(5,"chunk filename = %s, chunk->index = %d, pthread_cond_wait failed rc = %d errno = %d",
				    chunk->dev_name,chunk->index,pthread_rc,errno);
		CFLASH_BLOCK_UNLOCK(chunk->lock);
		return (ret_code);
	    }
	
	}
     

	CBLK_TRACE_LOG_FILE(9,"chunk index = %d thread_flags = %d num_active_cmds = 0x%x, num_paths = %d",
			    chunk->index,chunk->thread_flags,chunk->num_active_cmds,chunk->num_paths);

	if (chunk->thread_flags & CFLSH_CHNK_EXIT_INTRPT) {

	    

	    CBLK_TRACE_LOG_FILE(5,"exiting thread: chunk->index = %d thread_flags = %d",
				chunk->index,chunk->thread_flags);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    break;
	} else if ((chunk->thread_flags & CFLSH_CHNK_POLL_INTRPT) ||
		   (chunk->num_active_cmds)) {
	    
#ifdef BLOCK_FILEMODE_ENABLED
	    p_hrrq_curr = (uint64_t*)chunk->path[chunk->cur_path]->afu->p_hrrq_curr;
	    toggle = chunk->path[chunk->cur_path]->afu->toggle;
	    /*
	     * TODO: ?? The following for loop should be replaced
	     *        with a loop only walking the active list
	     *        looking for commands to issue filemode_io.
	     */
	    for (i=0; i < chunk->num_cmds; i++) {
		if ((chunk->cmd_info[i].in_use) &&
		    (chunk->cmd_info[i].state == CFLSH_MGM_WAIT_CMP)) {

		    /*
		     * cmd_info and cmd_start array entries
		     * with the same index correspond to the
		     * same command.
		     */
		    cmd = &chunk->cmd_start[i];
		    
		    cblk_filemode_io(chunk,cmd);
		    CBLK_INC_RRQ_LOCK(chunk,chunk->cur_path);
		}
		
	    } /* for */

	    chunk->path[chunk->cur_path]->afu->p_hrrq_curr = p_hrrq_curr;
	    chunk->path[chunk->cur_path]->afu->toggle = toggle;
			   
#endif /* BLOCK_FILEMODE_ENABLED */

	    
	    chunk->thread_flags &= ~CFLSH_CHNK_POLL_INTRPT;

	    tag = -1;
	    
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    CBLK_WAIT_FOR_IO_COMPLETE(async_data->chunk,&tag,&transfer_size,TRUE,0);

	    CFLASH_BLOCK_LOCK(chunk->lock);

	    
	    

	    if ((chunk->num_active_cmds) &&
		(chunk->head_act) && 
		!(chunk->flags & (CFLSH_CHNK_HALTED|CFLSH_CHNK_RECOV_AFU))) {

		/*
		 * We need to check for dropped commands here if 
		 * we are not in a halted nor AFU recovery state.
		 * For common threads there is no effective mechanism in 
		 * CBLK_WAIT_FOR_IO_COMPLETE to detect commmands that time-out.
		 * So we will do that here. First find the oldest command,
		 * which will be at the head of the chunk's active queue.
		 */
		    

		/*
		 * Increase time-out detect to 10 times
		 * the value we are using in the IOARCB, because
		 * the recovery process will reset this context 
		 */

		if (cflsh_blk.timeout_units != CFLSH_G_TO_SEC) {

		    /*
		     * If the time-out units are not in seconds
		     * then only give the command only 1 second to complete
		     */
		    timeout = time(NULL) - 1;
		} else {
		    timeout = time(NULL) - (10 * cflsh_blk.timeout);
		}

		if (chunk->head_act->cmd_time < timeout) {
		    
		    /*
		     * At least one command timed out. Let's
		     * fail all commands that timed out. The longest
		     * active command will be the head of the active 
		     * queue. The shortest active command will 
		     * the tail of the active queue. So we will 
		     * walk from the oldest to the newest. When
		     * we find a commmand that has not been active
		     * long enough to have timed out, we will stop
		     * walking this list (since subsequent commands would
		     * have been active no longer than that command).
		     */

		    CBLK_LIVE_DUMP_THRESHOLD(9,"0x202");


		    bzero(&path_reset_index,sizeof(path_reset_index));
		    reset_context = FALSE;
		    cmdi = chunk->head_act;

		    log_error = TRUE;

		    while (cmdi) {


			if ((cmdi->in_use) &&
			    (cmdi->state == CFLSH_MGM_WAIT_CMP) &&
			    (cmdi->cmd_time < timeout)) {


			    cmd = &chunk->cmd_start[cmdi->index];
			    
			    /*
			     * This commmand has timed out
			     */
			    
			    

			    if (log_error) {

				/*
				 * Only log first command to time-out. 
				 * Don't log them all, since this could flood
				 * the error log.
				 */

				cblk_notify_mc_err(chunk,cmdi->path_index,0x20b,0,
						   CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);

				log_error = FALSE;
			    }


			    cmdi->status = ETIMEDOUT;
			    cmdi->transfer_size = 0;
			    
			    CBLK_TRACE_LOG_FILE(6,"cmd time-out  lba = 0x%llx flags = 0x%x, chunk->index = %d",
						cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
			    
			    
			    /*
			     * Fail command back.
			     */
#ifdef REMOVE			    
			    cmd->cmdi->state = CFLSH_MGM_CMP;
			    
			    pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
			    
			    if (pthread_rc) {
				
				CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
						    pthread_rc,errno,chunk->index);
			    }
			    
#endif /* REMOVE */
			    chunk->stats.num_fail_timeouts++;

			    path_reset_index[cmdi->path_index] = TRUE;
			    
			    reset_context = TRUE;

			} else if (cmdi->cmd_time > timeout) {

			    /*
			     * Since commands on the active queue are ordered,
			     * with the head being the oldest and the tail the newest,
			     * we do not need process the active queue further
			     * after we found the first command that is not considered
			     * timed out.
			     */

			    break;

			}


			cmdi = cmdi->act_next;

		    } /* while */

		    if (reset_context) {

			
#ifdef _KERNEL_MASTER_CONTXT

			int i;



			/*
			 * We found at least one valid time command.
			 * Thus we'll reset the context and then all commands
			 * will be retried.
			 */


			for (i = 0; i < chunk->num_paths;i++) {
			    
                            if (path_reset_index[i]) {
				

				CBLK_GET_INTRPT_STATUS(chunk,i,&reset_status);

				if (reset_status != CFLSH_BLK_CHK_OS_NO_RESET) {

				    /*
				     * We detected UE when reading interrupt status.
				     * So give up on this recovery, since AFU recovery
				     * would have handled recovery/retries.
				     */

				    CBLK_TRACE_LOG_FILE(5,"AFU Recovery in progress: abandon context reset chunk->index = %d, chunk->flags 0x%x",
						    chunk->index, chunk->flags);
				    break;
				}
				CFLASH_BLOCK_UNLOCK(chunk->lock);

				cblk_reset_context_shared_afu(chunk->path[i]->afu);
				
                                CFLASH_BLOCK_LOCK(chunk->lock);
			    }

                        }
			
			



#else

			
			CBLK_GET_INTRPT_STATUS(chunk,chunk->cur_path,&reset_status);


			/*
			 * Tear down the context and prevent it from being used.
			 * This will prevent AFU from DMAing into the user's
			 * data buffer.
			 */


			chunk->flags |= CFLSH_CHUNK_FAIL_IO;


			/*
			 * Clear halted state so that subsequent commands
			 * do not hang, but are failed immediately.
			 */

			chunk->flags &= ~CFLSH_CHNK_HALTED;

			cblk_chunk_free_mc_device_resources(chunk);

			cblk_chunk_unmap(chunk,TRUE);

			close(chunk->fd);
			
			
			/*
			 * Fail all other commands. We are allowing the commands
			 * that saw the time out to be failed with ETIMEDOUT.
			 * All other commands are failed here with EIO.
			 */
			
			cblk_fail_all_cmds(chunk);

#endif  /* !_KERNEL_MASTER_CONTXT  */

			
		    }
		    
		}
	    }


	}

	CFLASH_BLOCK_UNLOCK(chunk->lock);

    } /* while */

    return (ret_code);
}

#endif /* COMMON_INTRPT_THREAD */

/*
 * NAME:        cblk_async_recv_thread
 *
 * FUNCTION:    This routine is invoked as a thread
 *              to wait for async I/O completions.
 *
 * NOTE:        This thread under some error conditions
 *              can be canceled via pthread_cancel.
 *              By default it will be cancelable, but
 *              deferred type. Thus pthread_cancel on this
 *              thread will only cause the thread to be 
 *              canceled at cancelation points. The
 *              invocation of pthread_testcancel is a cancelation
 *              point. These need to be placed in situations where
 *              this thread is not holding any resources--especially
 *              mutex locks, because otherwise those resources will not
 *              be freed.  In the case of mutex locks, if the thread
 *              is canceled while it is holding a lock, that lock
 *              will remain locked until this process terminates.
 * 
 *              This routine is not changing the canceltype to 
 *              PTHREAD_CANCEL_ASYNCHRONOUS, because that appears to be 
 *              less safe. It can be problematic in cases where resources--especially
 *              mutex locks--are in use, thus those resources are never freed.
 *
 *              During certain portions of the code, it will change its
 *              cancelstate from the default (PTHREAD_CANCEL_ENABLE) to
 *              PTHREAD_CANCEL_DISABLE and vice versa. This is need primarily
 *              for the CBLK_TRACE_LOG_FILE macro. It acquiree log lock and then
 *              calls functions (such as fprintf) that are considered by the OS
 *              as valid cancelation points. If we allowed a cancel while
 *              these trace macros are running, we could cancel this thread
 *              and never unlock the log lock. 
 *
 * INPUTS:
 *              data - of type cflsh_async_thread_cmp_t 
 *
 * RETURNS:
 *             -1  - Fatal error
 *              0  - Ignore error (consider good completion)
 *              1  - Retry recommended
 *              
 */
void *cblk_async_recv_thread(void *data)
{
    void *ret_code = NULL;
    int rc = 0;
    cflsh_async_thread_cmp_t *async_data = data;
    size_t transfer_size;
    cflsh_cmd_mgm_t *cmd = NULL;
    cflsh_chunk_t *chunk = NULL;
    int pthread_rc = 0;


    
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

#ifdef _REMOVE 
    pthread_rc = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    if (pthread_rc) {

	CBLK_TRACE_LOG_FILE(5,"cmd_index = %d, pthread_setcanceltype failed rc = %d, errno = %d",
			    async_data->cmd_index,pthread_rc,errno);
	return (ret_code);
    }
#endif /* _REMOVE */


    chunk = async_data->chunk;

    if (chunk == NULL) {

	CBLK_TRACE_LOG_FILE(5,"chunk filename = %s cmd_index = %d, cmd is NULL",
			    async_data->chunk->dev_name,async_data->cmd_index);

	return (ret_code);
    }

    CBLK_TRACE_LOG_FILE(5,"chunk filename = %s chunk->index = %d cmd_index = %d",chunk->dev_name,
			chunk->index, async_data->cmd_index);

    /*
     * Create a cancellation point just before we
     * try to take the chunk->lock. Thus if we
     * are being canceled we would exit now
     * before blocking on the lock.
     */

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);


    CFLASH_BLOCK_LOCK(chunk->lock);

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"chunk filename = %s pthread_cond_wait failed rc = %d errno = %d",
			    async_data->chunk->dev_name,pthread_rc,errno);

	return (ret_code);
    }


    cmd = &(chunk->cmd_start[async_data->cmd_index]);

    if (cmd == NULL) {

	CBLK_TRACE_LOG_FILE(5,"chunk filename = %s cmd_index = %d, cmd is NULL",
			    async_data->chunk->dev_name,async_data->cmd_index);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	return (ret_code);
    }

    /*
     * Since we start this thread just before we attempt
     * to issue the corresponding command to the AFU,
     * we need to wait for a signal that it was successfully
     * issued before proceeding.
     *
     * It should also be noted that if we are being
     * canceled we would also be signaled too and
     * thus wake up. The pthread_test cancel
     * after we unlock after waiting here,
     * would be where this thread would exit.
     */

    if (!(cmd->cmdi->flags & CFLSH_ASYNC_IO_SNT)) {
    
	/*
	 * Only wait if the CFLSH_ASYNC_IO_SNT flag
	 * has not been set.
	 */
	pthread_rc = pthread_cond_wait(&(cmd->cmdi->thread_event),&(chunk->lock.plock));
	
	if (pthread_rc) {
	    
	    cmd->cmdi->flags |= CFLSH_ATHRD_EXIT;
	    CBLK_TRACE_LOG_FILE(5,"chunk filename = %s cmd_index = %d, pthread_cond_wait failed rc = %d errno = %d",
				async_data->chunk->dev_name,async_data->cmd_index,pthread_rc,errno);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return (ret_code);
	}
	
    }

    if (cmd->cmdi->state == CFLSH_MGM_ASY_CMP) {
	    
	/*
	 * The originator of this command
	 * has been notified that this command
	 * completed, but wase unable 
	 * to mark the command as free, since
	 * this thread is running. Mark 
	 * the command as free now.
	 */

	CBLK_FREE_CMD(chunk,cmd);

	CBLK_TRACE_LOG_FILE(5,"cmd_index = %d in_use = %d, cmd = 0x%llx",
			    async_data->cmd_index, cmd->cmdi->in_use,(uint64_t)cmd);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	return (ret_code);
    }

    if ((!cmd->cmdi->in_use) ||
	(cmd->cmdi->state == CFLSH_MGM_CMP)) {
	/*
	 * If the command is no longer in use,
	 * then exit now.
	 */

	cmd->cmdi->flags |= CFLSH_ATHRD_EXIT;
	CBLK_TRACE_LOG_FILE(5,"command not in use cmd_index = %d",
			    async_data->cmd_index);

	CFLASH_BLOCK_UNLOCK(chunk->lock);
	return (ret_code);
    }



    CFLASH_BLOCK_UNLOCK(chunk->lock);

    /*
     * Create a cancelation point just before
     * we start polling for completion, just in
     * case we are being canceled. This needs
     * to be after we unlocked to avoid never
     * releasing that lock.
     */

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&(async_data->cmd_index),&transfer_size,TRUE,0);



    CFLASH_BLOCK_LOCK(chunk->lock);  

    /*
     * TODO: ?? This is ugly that we are
     *       acquiring a lock to only decrement
     *       the number of active threads (i.e
     *       keep statistics. We may want to
     *       look at removing this in the future
     */

    chunk->stats.num_active_threads--;


    cmd->cmdi->flags |= CFLSH_ATHRD_EXIT;

    if (cmd->cmdi->state == CFLSH_MGM_ASY_CMP) {
	    
	/*
	 * The originator of this command
	 * has been notified that this command
	 * completed, but was unable 
	 * to mark the command as free, since
	 * this thread is running. Mark 
	 * the command as free now.
	 */
	
	CBLK_FREE_CMD(chunk,cmd);

	CBLK_TRACE_LOG_FILE(8,"cmd_index = %d in_use = %d, cmd = 0x%llx, chunk->index = %d",
			    async_data->cmd_index, cmd->cmdi->in_use,(uint64_t)cmd,chunk->index);
    }


    CFLASH_BLOCK_UNLOCK(chunk->lock);

    CBLK_TRACE_LOG_FILE(5,"CBLK_WAIT_FOR_IO_COMPLETE returned rc = %d, cmd_index = %d in_use = %d, chunk->index = %d",
			rc,async_data->cmd_index, cmd->cmdi->in_use,chunk->index);

    return (ret_code);
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cblk_trace_log_data_ext
 *                  
 * FUNCTION: This is a function call (as opposed to inlined) version
 *           of trace_log_data_ext from trace_log.h. It uses
 *           the same setup (setup_log_file in trace_log.h) and defines. 
 *
 *           Print a message to trace log. This
 *           function is the same as trace_log_data, except
 *           this function requires the caller to maintain
 *           the static variables via the extended argument. In addition
 *           it gives the caller additional control over logging.
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
void cblk_trace_log_data_ext(trace_log_ext_arg_t *ext_arg, FILE *logfp,char *filename, char *function, 
					   uint line_num,char *msg, ...)
{
    va_list ap;
    struct timeb cur_time, log_time, delta_time;
    uint print_log_number;
#ifndef TIMELEN
#define   TIMELEN  26           /* Linux does have a define for the minium size of the a timebuf */
				/* However linux man pages say it is 26                          */
#endif 
    char timebuf[TIMELEN+1];
    time_t curtime;

    if (ext_arg == NULL) {

	return;
    }

    if (logfp == NULL) {

	return;
    }


    if (ext_arg->flags & TRACE_LOG_NO_USE_LOG_NUM) {

	if (ext_arg->log_number > 0)  {
	    print_log_number = ext_arg->log_number - 1;
	} else {
	    print_log_number = 0;
	}

    } else {
	print_log_number = ext_arg->log_number;
    }

    ftime(&cur_time);
    
    if (!(ext_arg->flags & TRACE_LOG_START_VALID)) {

        /*
         * If start time is not set, then
         * set it now.
         */

        ext_arg->start_time = cur_time;

	
	ext_arg->flags |= TRACE_LOG_START_VALID;


        log_time.time = 0;
        log_time.millitm = 0;

        delta_time.time = 0;
        delta_time.millitm = 0;
        

        /*
         * Print header
         */
        fprintf(logfp,"---------------------------------------------------------------------------\n");
        fprintf(logfp,"Date for %s is %s at %s\n",__FILE__,__DATE__,__TIME__);

	curtime = time(NULL);
	fprintf(logfp,"Trace started at %s\n",ctime_r(&curtime,timebuf));
        fprintf(logfp,"Index   Sec   msec  delta dmsec      Filename            function, line ...\n");
        fprintf(logfp,"------- ----- ----- ----- ----- --------------------  ---------------------\n");

    } else {

        /*
         * Find time offset since starting time.
         */

        log_time.time = cur_time.time - ext_arg->start_time.time;
        log_time.millitm = cur_time.millitm - ext_arg->start_time.millitm;

        delta_time.time = log_time.time - ext_arg->last_time.time;
        delta_time.millitm = log_time.millitm - ext_arg->last_time.millitm;
    }

    fprintf(logfp,"%7d %5d.%05d %5d.%05d %-25s  %-35s line:%5d :",
            print_log_number,(int)log_time.time,log_time.millitm,(int)delta_time.time,delta_time.millitm,filename, function, line_num);
    /*
     * Initialize ap to store arguments after msg
     */

    va_start(ap,msg);
    vfprintf(logfp, msg, ap);
    va_end(ap);

    fprintf(logfp,"\n");

    fflush(logfp);

    if (!(ext_arg->flags & TRACE_LOG_NO_USE_LOG_NUM)) {
	ext_arg->log_number++;
    }

    ext_arg->last_time = log_time;

    return;
       
}



/*
 * NAME: cblk_display_stats
 *
 * FUNCTION: This routine is called whenever an RRQ has been processed.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */

void  cblk_display_stats(cflsh_chunk_t *chunk, int verbosity)
{
    CBLK_TRACE_LOG_FILE(verbosity,"\nCHUNK STATISTICS ...");
#ifdef BLOCK_FILEMODE_ENABLED
     CBLK_TRACE_LOG_FILE(verbosity,"FILEMODE");
#endif /* BLOCK_FILEMODE_ENABLED */ 
    
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    CBLK_TRACE_LOG_FILE(verbosity,"Little Endian");
#else
    CBLK_TRACE_LOG_FILE(verbosity,"Big Endian");
#endif     
#ifdef _MASTER_CONTXT
    CBLK_TRACE_LOG_FILE(verbosity,"Master Context");
#else
    CBLK_TRACE_LOG_FILE(verbosity,"No Master Context");
#endif 
    CBLK_TRACE_LOG_FILE(verbosity,"cblk_log_verbosity              0x%x",cblk_log_verbosity);
#if !defined(__64BIT__) && defined(_AIX)
    CBLK_TRACE_LOG_FILE(verbosity,"32-bit app support              ");
#else
    CBLK_TRACE_LOG_FILE(verbosity,"64-bit app support              ");

#endif
    CBLK_TRACE_LOG_FILE(verbosity,"flags                           0x%x",cflsh_blk.flags);
    CBLK_TRACE_LOG_FILE(verbosity,"host_type                       0x%x",cflsh_blk.host_type);
    CBLK_TRACE_LOG_FILE(verbosity,"lun_id                          0x%llx",cflsh_blk.lun_id);
    CBLK_TRACE_LOG_FILE(verbosity,"next_chunk_id                   0x%llx",cflsh_blk.next_chunk_id);
    CBLK_TRACE_LOG_FILE(verbosity,"num_active_chunks               0x%x",cflsh_blk.num_active_chunks);
    CBLK_TRACE_LOG_FILE(verbosity,"num_max_active_chunks           0x%x",cflsh_blk.num_max_active_chunks);
    CBLK_TRACE_LOG_FILE(verbosity,"num_bad_chunk_ids               0x%x",cflsh_blk.num_bad_chunk_ids);
    CBLK_TRACE_LOG_FILE(verbosity,"chunk_id                        0x%llx",chunk->index);
    if(chunk->udid) {
	CBLK_TRACE_LOG_FILE(verbosity,"udid                            %s",chunk->udid);
    }
    CBLK_TRACE_LOG_FILE(verbosity,"chunk_block size                0x%x",chunk->stats.block_size);
    CBLK_TRACE_LOG_FILE(verbosity,"num_paths                       0x%x",chunk->num_paths);
    CBLK_TRACE_LOG_FILE(verbosity,"primary_path_id                 0x%x",chunk->path[0]->path_id);
    CBLK_TRACE_LOG_FILE(verbosity,"chunk_type                      0x%x",chunk->path[chunk->cur_path]->type);
    CBLK_TRACE_LOG_FILE(verbosity,"num_blocks                      0x%x",chunk->num_blocks);
    CBLK_TRACE_LOG_FILE(verbosity,"num_blocks_lun                  0x%llx",chunk->num_blocks_lun);
    CBLK_TRACE_LOG_FILE(verbosity,"max_transfer_size               0x%llx",chunk->stats.max_transfer_size);
    CBLK_TRACE_LOG_FILE(verbosity,"num_cmds                        0x%x",chunk->num_cmds);
    CBLK_TRACE_LOG_FILE(verbosity,"num_active_cmds                 0x%x",chunk->num_active_cmds);
    CBLK_TRACE_LOG_FILE(verbosity,"num_reads                       0x%llx",chunk->stats.num_reads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_writes                      0x%llx",chunk->stats.num_writes);
    CBLK_TRACE_LOG_FILE(verbosity,"num_areads                      0x%llx",chunk->stats.num_areads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_awrites                     0x%llx",chunk->stats.num_awrites);
    CBLK_TRACE_LOG_FILE(verbosity,"num_act_reads                   0x%x",chunk->stats.num_act_reads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_act_writes                  0x%x",chunk->stats.num_act_writes);
    CBLK_TRACE_LOG_FILE(verbosity,"num_act_areads                  0x%x",chunk->stats.num_act_areads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_act_awrites                 0x%x",chunk->stats.num_act_awrites);
    CBLK_TRACE_LOG_FILE(verbosity,"max_num_act_reads               0x%x",chunk->stats.max_num_act_reads);
    CBLK_TRACE_LOG_FILE(verbosity,"max_num_act_writes              0x%x",chunk->stats.max_num_act_writes);
    CBLK_TRACE_LOG_FILE(verbosity,"max_num_act_areads              0x%x",chunk->stats.max_num_act_areads);
    CBLK_TRACE_LOG_FILE(verbosity,"max_num_act_awrites             0x%x",chunk->stats.max_num_act_awrites);
    CBLK_TRACE_LOG_FILE(verbosity,"num_blocks_read                 0x%llx",chunk->stats.num_blocks_read);
    CBLK_TRACE_LOG_FILE(verbosity,"num_blocks_written              0x%llx",chunk->stats.num_blocks_written);
    CBLK_TRACE_LOG_FILE(verbosity,"num_aresult_no_cmplt            0x%llx",chunk->stats.num_aresult_no_cmplt);
    CBLK_TRACE_LOG_FILE(verbosity,"num_errors                      0x%llx",chunk->stats.num_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_retries                     0x%llx",chunk->stats.num_retries);
    CBLK_TRACE_LOG_FILE(verbosity,"num_timeouts                    0x%llx",chunk->stats.num_timeouts);
    CBLK_TRACE_LOG_FILE(verbosity,"num_fail_timeouts               0x%llx",chunk->stats.num_fail_timeouts);
    CBLK_TRACE_LOG_FILE(verbosity,"num_reset_contexts              0x%llx",chunk->stats.num_reset_contexts);
    CBLK_TRACE_LOG_FILE(verbosity,"num_reset_context_fails         0x%llx",chunk->stats.num_reset_contxt_fails);
    CBLK_TRACE_LOG_FILE(verbosity,"num_no_cmds_free                0x%llx",chunk->stats.num_no_cmds_free);
    CBLK_TRACE_LOG_FILE(verbosity,"num_no_cmd_room                 0x%llx",chunk->stats.num_no_cmd_room);
    CBLK_TRACE_LOG_FILE(verbosity,"num_no_cmds_free_fail           0x%llx",chunk->stats.num_no_cmds_free_fail);
    CBLK_TRACE_LOG_FILE(verbosity,"num_cc_errors                   0x%llx",chunk->stats.num_cc_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_fc_errors                   0x%llx",chunk->stats.num_fc_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port0_linkdowns             0x%llx",chunk->stats.num_port0_linkdowns);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port1_linkdowns             0x%llx",chunk->stats.num_port1_linkdowns);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port0_no_logins             0x%llx",chunk->stats.num_port0_no_logins);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port1_no_logins             0x%llx",chunk->stats.num_port1_no_logins);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port0_fc_errors             0x%llx",chunk->stats.num_port0_fc_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_port1_fc_errors             0x%llx",chunk->stats.num_port1_fc_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_afu_errors                  0x%llx",chunk->stats.num_afu_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_false_reads            0x%llx",chunk->stats.num_capi_false_reads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_adap_resets            0x%llx",chunk->stats.num_capi_adap_resets);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_adap_chck_err          0x%llx",chunk->stats.num_capi_adap_chck_err);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_read_fails             0x%llx",chunk->stats.num_capi_read_fails);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_data_st_errs           0x%llx",chunk->stats.num_capi_data_st_errs);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_afu_errors             0x%llx",chunk->stats.num_capi_afu_errors);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_afu_intrpts            0x%llx",chunk->stats.num_capi_afu_intrpts);
    CBLK_TRACE_LOG_FILE(verbosity,"num_capi_unexp_afu_intrpts      0x%llx",chunk->stats.num_capi_unexp_afu_intrpts);
    CBLK_TRACE_LOG_FILE(verbosity,"num_cache_hits                  0x%llx",chunk->stats.num_cache_hits);
    CBLK_TRACE_LOG_FILE(verbosity,"num_success_threads             0x%llx",chunk->stats.num_success_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_failed_threads              0x%llx",chunk->stats.num_failed_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_canc_threads                0x%llx",chunk->stats.num_canc_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_fail_canc_threads           0x%llx",chunk->stats.num_fail_canc_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_fail_detach_threads         0x%llx",chunk->stats.num_fail_detach_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"num_active_threads              0x%llx",chunk->stats.num_active_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"max_num_act_threads             0x%llx",chunk->stats.max_num_act_threads);
    CBLK_TRACE_LOG_FILE(verbosity,"Avg latency                     %ld",(chunk->rcmd+chunk->wcmd) ?
                                                                        (chunk->rlat+chunk->wlat)/(chunk->rcmd+chunk->wcmd) : 0);
    CBLK_TRACE_LOG_FILE(verbosity,"Read latency                    %ld",(chunk->rcmd) ? chunk->rlat/chunk->rcmd : 0);
    CBLK_TRACE_LOG_FILE(verbosity,"Write latency                   %ld",(chunk->wcmd) ? chunk->wlat/chunk->wcmd : 0);

    return;
}


/*
 * NAME: cblk_setup_dump_file
 *
 * FUNCTION: This routine dump data structures for
 *           the block library.
 *  
 *
 *
 * RETURNS: 0 - Success, Otherwise error.
 *     
 *    
 */

int  cblk_setup_dump_file(void)
{
    int rc = 0;
    char *env_user = getenv("USER");
    char *dump_filename = getenv("CFLSH_BLK_DUMP");
    char *log_pid  = getenv("CFLSH_BLK_DUMP_PID");
    char filename[PATH_MAX];



    
    if (cblk_dumpfp) {

	/*
	 * If dump file pointer is setup,
	 * then do not set it up again.
	 */

	return rc;

    }


    if (dump_filename == NULL)
    {
        sprintf(filename, "/tmp/%s.cflash_block_dump", env_user);
        dump_filename = filename;
    }


    if (log_pid) {
	
	/*
	 * Use different filename for each process
	 */

	sprintf(filename,"%s.%d",dump_filename,getpid());


    }



    if ((cblk_dumpfp = fopen(dump_filename, "a")) == NULL) {


	CBLK_TRACE_LOG_FILE(1,"Failed to open dump_filename file %s",dump_filename);

	cblk_dumpfp = NULL;
	rc = -1;
    }


    return rc;
}

/*
 * NAME: cblk_dump_debug_data
 *
 * FUNCTION: This routine dump data structures for
 *           the block library.
 *  
 * NOTE:     This routine does not use any locking to serialize with the rest
 *           of the library, since it may be called to debug a range of 
 *           library issues--including deadlock. Thus there is risk,
 *           that it could hit a segmentation fault. As result, it needs
 *           to call fflush periodically to ensure any data it's able to get
 *           is written out the dump file. In addition this code has taken
 *           some steps to order and check things to minimize the likelihood
 *           of it hitting a segmentation fault, but there is still no
 *           guarantee this can be avoid.
 *
 * RETURNS: None
 *     
 *    
 */

void  cblk_dump_debug_data(const char *reason,const char *reason_filename,const char *reason_function,
			  int reason_line_num, const char *reason_date)
{
    int i,j;
    cflsh_chunk_t *chunk;
    cflsh_afu_t *afu = NULL;
    scsi_cdb_t *cdb = NULL;
    cflsh_cmd_info_t *cmd_info = NULL;
#ifndef TIMELEN
#define   TIMELEN  26           /* Linux does have a define for the minium size of the a timebuf */
				/* However linux man pages say it is 26                          */
#endif 
    char timebuf[TIMELEN+1];
    time_t cur_time;
    int num_cmds_processed;
    int num_cmds_mgm_completed;
    int num_cmds_asy_mgm_completed;
    int num_waiting_cmds;


    if (cblk_dumpfp == NULL) {

	return;
    }
    
    /*
     * Print header
     */
    fprintf(cblk_dumpfp,"---------------------------------------------------------------------------\n");
    fprintf(cblk_dumpfp,"Build date for %s is %s at %s\n",__FILE__,__DATE__,__TIME__);

    fflush(cblk_dumpfp);

    cur_time = time(NULL);

    fprintf(cblk_dumpfp,"Dump occurred at %s\n",ctime_r(&cur_time,timebuf));

    fflush(cblk_dumpfp);
    fprintf(cblk_dumpfp,"PID = 0x%"PRIx64", dump sequence number =  0x%x\n",
	    (uint64_t)cflsh_blk.caller_pid, dump_sequence_num);
    fprintf(cblk_dumpfp,"dump reason %s, filename = %s, function = %s, line_number = %d, date = %s\n",
	    reason,reason_filename,reason_function,reason_line_num,reason_date);
    fflush(cblk_dumpfp);
    fprintf(cblk_dumpfp,"---------------------------------------------------------------------------\n");

    fflush(cblk_dumpfp);

    fetch_and_add(&(dump_sequence_num),+1);


#ifdef BLOCK_FILEMODE_ENABLED
     fprintf(cblk_dumpfp,"FILEMODE\n");
#endif /* BLOCK_FILEMODE_ENABLED */ 
    
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    fprintf(cblk_dumpfp,"Little Endian\n");
#else
    fprintf(cblk_dumpfp,"Big Endian\n");
#endif    
#if !defined(__64BIT__) && defined(_AIX)
    fprintf(cblk_dumpfp,"32-bit app support\n");
#else
    fprintf(cblk_dumpfp,"64-bit app support\n");

#endif

    fprintf(cblk_dumpfp,"\n\n");

    fprintf(cblk_dumpfp,"cblk_log_verbosity         0x%x\n\n",cblk_log_verbosity);

    fprintf(cblk_dumpfp,"cblk_dump_level            0x%x\n\n",cblk_dump_level);

    fprintf(cblk_dumpfp,"cblk_notify_log_level      0x%x\n\n",cblk_notify_log_level);

    fprintf(cblk_dumpfp,"cblk_log_lock (addr)       %p\n\n",&cblk_log_lock);



    fprintf(cblk_dumpfp,"cflsh_blk     = %p\n\n",&cflsh_blk);

    fflush(cblk_dumpfp);

    fprintf(cblk_dumpfp,"    global_lock.file    = 0x%x\n",cflsh_blk.global_lock.file); 
    fprintf(cblk_dumpfp,"    global_lock.fname   = %s\n",cflsh_blk.global_lock.filename); 
    fprintf(cblk_dumpfp,"    global_lock.line    = %d\n",cflsh_blk.global_lock.line); 
    fprintf(cblk_dumpfp,"    global_lock.thread &= %p\n",&(cflsh_blk.global_lock.thread)); 
    fprintf(cblk_dumpfp,"    global_lock.thread  = 0x%x\n",(uint32_t)(cflsh_blk.global_lock.thread)); 
    fprintf(cblk_dumpfp,"    flags               = 0x%x\n",cflsh_blk.flags);
    fprintf(cblk_dumpfp,"    timeout             = 0x%x\n",cflsh_blk.timeout);
    fprintf(cblk_dumpfp,"    num_active_chunks   = 0x%x\n",cflsh_blk.num_active_chunks);
    fprintf(cblk_dumpfp,"    num_bad_chunk_ids   = 0x%x\n",cflsh_blk.num_bad_chunk_ids);
    fprintf(cblk_dumpfp,"    caller_pid          = 0x%"PRIx64"\n",(uint64_t)cflsh_blk.caller_pid);
    fprintf(cblk_dumpfp,"    process_name        = %s\n",cflsh_blk.process_name);
    fprintf(cblk_dumpfp,"    thread_log_mask     = 0x%x\n",cflsh_blk.thread_log_mask);
    fprintf(cblk_dumpfp,"    ext_arg.flags       = 0x%x\n",cflsh_blk.trace_ext.flags);
    fprintf(cblk_dumpfp,"    ext_arg.log_number  = 0x%x\n",cflsh_blk.trace_ext.log_number);
    fprintf(cblk_dumpfp,"    head_afu            = %p\n",cflsh_blk.head_afu);
    fprintf(cblk_dumpfp,"    tail_afu            = %p\n",cflsh_blk.tail_afu);
    
    fprintf(cblk_dumpfp,"    hash[]              = %p\n\n",&cflsh_blk.hash[0]);


    fprintf(cblk_dumpfp,"    eyec                = 0x%x",cflsh_blk.eyec);
    if (!CFLSH_EYECATCH_CBLK(&cflsh_blk)) {
	fprintf(cblk_dumpfp," (Valid)\n");
    } else {
	fprintf(cblk_dumpfp," (Invalid !!)\n");

	fflush(cblk_dumpfp);

	/*
	 * If global cflsh_blk is corrupted then return
	 * now. Don't attempt to traverse other
	 * data structures here, because this may
	 * result in a segmentation fault.
	 */
	return;
    }

    fflush(cblk_dumpfp);

    for (i=0;i<MAX_NUM_CHUNKS_HASH; i++) {

	chunk = cflsh_blk.hash[i].head;

	while (chunk) {

	    fprintf(cblk_dumpfp,"\n    chunk(%d)           = %p\n",i,chunk);

	    /*
	     * Do some basic tests to see if this chunk address appears
	     * to be valid. If not, then exit from this while loop,
	     * since we can not trust the next pointer and move
	     * to the next has bucket.
	     */

	    if ( (ulong)chunk & CHUNK_BAD_ADDR_MASK ) {

		fprintf(cblk_dumpfp,"        BAD CHUNK ADDR!!\n");

		break;
			
	    }

	    fprintf(cblk_dumpfp,"        eyec            = 0x%x",chunk->eyec);

	    if (!CFLSH_EYECATCH_CHUNK(chunk)) {
		fprintf(cblk_dumpfp," (Valid)\n");
	    } else {
		fprintf(cblk_dumpfp," (Invalid !!)\n");
		fflush(cblk_dumpfp);

		break;
	    }
	    
	    fprintf(cblk_dumpfp,"        in_use          = %d\n",chunk->in_use);
	    fprintf(cblk_dumpfp,"        flags           = 0x%x\n",chunk->flags);
	    fprintf(cblk_dumpfp,"        index           = 0x%x\n",chunk->index);
	    fprintf(cblk_dumpfp,"        dev_name        = %s\n",chunk->dev_name);
	    fprintf(cblk_dumpfp,"        num_blocks      = 0x%"PRIx64"\n",(uint64_t)chunk->num_blocks);
	    fprintf(cblk_dumpfp,"        num_blocks_lun  = 0x%"PRIx64"\n",(uint64_t)chunk->num_blocks_lun);
	    fprintf(cblk_dumpfp,"        lock.file       = 0x%x\n",chunk->lock.file); 
	    fprintf(cblk_dumpfp,"        lock.fname      = %s\n",chunk->lock.filename);
	    fprintf(cblk_dumpfp,"        lock.line       = %d\n",chunk->lock.line); 
	    fprintf(cblk_dumpfp,"        lock.thread &   = %p\n",&(chunk->lock.thread));
	    fprintf(cblk_dumpfp,"        lock.thread     = 0x%x\n",(uint32_t)(chunk->lock.thread));
 	    fprintf(cblk_dumpfp,"        cache_buffer    = %p\n",chunk->cache_buffer);
 	    fprintf(cblk_dumpfp,"        num_cmds        = 0x%x\n",chunk->num_cmds);
	    fprintf(cblk_dumpfp,"        num_active_cmds = 0x%x\n",chunk->num_active_cmds); 
	    fprintf(cblk_dumpfp,"        cmd_start       = %p\n",chunk->cmd_start);
	    fprintf(cblk_dumpfp,"        cmd_info (addr) = %p\n\n",chunk->cmd_info);


	    fflush(cblk_dumpfp);

	    if (chunk->cmd_info) {

		/*
		 * It is possible this might be called via signal when the library
		 * is closing. Since we are not locking, there is no guarantee
		 * the rug can not be pulled out from under us. However we
		 * will try to take some steps to minimize this. First will 
		 * save off the cmd_info address, since close will NULL it after
		 * it is free. Thus there is still a possibility that the data buffers
		 * have been reused by others, but we will try to minimize this some.
		 * We are not going to filter/check on CFLSH_CHNK_CLOSE, since it
		 * is possible we need to debug some issues at close time, and 
		 * there is no guarantee this close check would prevent all 
		 * potential issues here.
		 */
		 

		cmd_info = chunk->cmd_info;

		num_cmds_processed = 0;

		num_waiting_cmds = 0;

		num_cmds_mgm_completed = 0;

		num_cmds_asy_mgm_completed = 0;

		for (j=0; j < chunk->num_cmds; j++) {
		    
		    if ((chunk->cmd_info) &&
			(cmd_info[j].in_use)) {
			

			/*
			 * If chunk->cmd_info is NULL, then don't
			 * attempt to read these commands, since they
			 * have been freed and maybe allocated for others.
			 */

			fprintf(cblk_dumpfp,"\n            cmd_info[%d]           = %p\n",j,&(cmd_info[j]));
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                flags             = 0x%x\n",cmd_info[j].flags);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                user_tag          = 0x%x\n",cmd_info[j].user_tag);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                index             = 0x%x\n",cmd_info[j].index);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                path_index        = 0x%x\n",cmd_info[j].path_index);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                buf               = %p\n",cmd_info[j].buf);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                lba               = 0x%"PRIx64"\n",
				(uint64_t)cmd_info[j].lba);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                nblocks           = 0x%"PRIx64"\n",
				(uint64_t)cmd_info[j].nblocks);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                status            = 0x%"PRIx64"\n",
				(uint64_t)cmd_info[j].status);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                in_use            = 0x%x\n",cmd_info[j].in_use);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                state             = 0x%x\n",cmd_info[j].state);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                retry_count       = 0x%x\n",cmd_info[j].retry_count);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                transfer_size_byt = 0x%x\n",
				cmd_info[j].transfer_size_bytes);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                transfer_size     = 0x%"PRIx64"\n",
				cmd_info[j].transfer_size);
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                cmd               = %p\n",&(chunk->cmd_start[j]));
			fflush(cblk_dumpfp);
			fprintf(cblk_dumpfp,"                   cmd->cmd_info  = %p",chunk->cmd_start[j].cmdi);
			if (chunk->cmd_start[j].cmdi == &(cmd_info[j])) {
			    fprintf(cblk_dumpfp," (Valid)\n");
			} else {
			    fprintf(cblk_dumpfp," (Invalid !!)\n");
			    fflush(cblk_dumpfp);
			}
			
			fprintf(cblk_dumpfp,"                   cmd->index     = 0x%x\n",chunk->cmd_start[j].index);
			cdb = CBLK_GET_CMD_CDB(chunk,&chunk->cmd_start[j]);
			if (cdb) {
			    fprintf(cblk_dumpfp,"                   cmd: op_code   = 0x%x\n",cdb->scsi_op_code);
			}
			
			fprintf(cblk_dumpfp,"                chunk             = %p",cmd_info[j].chunk);
			if (cmd_info[j].chunk == chunk) {
			    fprintf(cblk_dumpfp," (Valid)\n");
			} else {
			    fprintf(cblk_dumpfp," (Invalid !!)\n");
			}
			
			fprintf(cblk_dumpfp,"                eyec              = 0x%x",cmd_info[j].eyec);
			
			if ( !CFLSH_EYECATCH_CMDI(&cmd_info[j])) {
			    fprintf(cblk_dumpfp," (Valid)\n");
			} else {
			    fprintf(cblk_dumpfp," (Invalid !!)\n");
			}
			
			
			fflush(cblk_dumpfp);

			if (cmd_info[j].flags & CFLSH_PROC_CMD) {
			    num_cmds_processed++;
			}

			if (cmd_info[j].state == CFLSH_MGM_CMP) {
			    num_cmds_mgm_completed++;
			} else if (cmd_info[j].state == CFLSH_MGM_WAIT_CMP) {
			    num_waiting_cmds++;
			} else if (cmd_info[j].state == CFLSH_MGM_ASY_CMP) {
			    num_cmds_asy_mgm_completed++;
			}

		    }
		} /* for */


		fflush(cblk_dumpfp);

		fprintf(cblk_dumpfp,"        number of active commands processed = %d\n",num_cmds_processed);

		fprintf(cblk_dumpfp,"        number of active commands waiting = %d\n",num_waiting_cmds);

		fprintf(cblk_dumpfp,"        number of active commands completed = %d\n",num_cmds_mgm_completed);

		fprintf(cblk_dumpfp,"        number of active async commands completed = %d\n",num_cmds_asy_mgm_completed);


		fprintf(cblk_dumpfp,"\n");
	    }

	    fprintf(cblk_dumpfp,"\n");
	    fflush(cblk_dumpfp);

	    fprintf(cblk_dumpfp,"        stats (addr)    = %p\n\n",&chunk->stats);
	    fflush(cblk_dumpfp);
	    fprintf(cblk_dumpfp,"            block_size      = 0x%x\n",chunk->stats.block_size);
	    fprintf(cblk_dumpfp,"            max_transfer    = 0x%"PRIx64"\n",chunk->stats.max_transfer_size);
	    fprintf(cblk_dumpfp,"            num_blocks_read = 0x%"PRIx64"\n",chunk->stats.num_blocks_read);
	    fprintf(cblk_dumpfp,"            num_blocks_writ = 0x%"PRIx64"\n",chunk->stats.num_blocks_written);
	    fprintf(cblk_dumpfp,"            no_cmd_room     = 0x%"PRIx64"\n",chunk->stats.num_no_cmd_room);
	    fprintf(cblk_dumpfp,"            no_cmds_free    = 0x%"PRIx64"\n",chunk->stats.num_no_cmds_free);
	    fprintf(cblk_dumpfp,"            no_cmds_free_fai= 0x%"PRIx64"\n",chunk->stats.num_no_cmds_free_fail);
	    fprintf(cblk_dumpfp,"            num_timeouts    = 0x%"PRIx64"\n",chunk->stats.num_timeouts);
	    fprintf(cblk_dumpfp,"            num_fail_timeout= 0x%"PRIx64"\n",chunk->stats.num_fail_timeouts);
	    fprintf(cblk_dumpfp,"            num_cc_errors   = 0x%"PRIx64"\n",chunk->stats.num_cc_errors);
	    fprintf(cblk_dumpfp,"            num_fc_errors   = 0x%"PRIx64"\n",chunk->stats.num_fc_errors);
	    fprintf(cblk_dumpfp,"            num_afu_errors  = 0x%"PRIx64"\n",chunk->stats.num_afu_errors);
	    fprintf(cblk_dumpfp,"            capi_data_st_err= 0x%"PRIx64"\n",chunk->stats.num_capi_data_st_errs);
	    fprintf(cblk_dumpfp,"            num_reset_contex= 0x%"PRIx64"\n",chunk->stats.num_reset_contexts);
	    fprintf(cblk_dumpfp,"            num_capi_adap_rs= 0x%"PRIx64"\n",chunk->stats.num_capi_adap_resets);
	    fprintf(cblk_dumpfp,"\n");

	    fflush(cblk_dumpfp);

	    fprintf(cblk_dumpfp,"        blk_size_mult   = 0x%x\n",chunk->blk_size_mult);
	    fprintf(cblk_dumpfp,"        thread_flags    = 0x%x\n",chunk->thread_flags);
	    fprintf(cblk_dumpfp,"        head_free       = %p\n",chunk->head_free);
	    fprintf(cblk_dumpfp,"        tail_free       = %p\n",chunk->tail_free);
	    fprintf(cblk_dumpfp,"        head_act        = %p\n",chunk->head_free);
	    fprintf(cblk_dumpfp,"        tail_act        = %p\n",chunk->tail_free);
	    fprintf(cblk_dumpfp,"        num_paths       = 0x%x\n",chunk->num_paths);
	    fprintf(cblk_dumpfp,"        cur_path        = 0x%x\n",chunk->cur_path);
	    fprintf(cblk_dumpfp,"        path[]          = %p\n\n",chunk->path);

	    
	    fflush(cblk_dumpfp);
	    for (j=0; j < chunk->num_paths; j++) {

		fprintf(cblk_dumpfp,"\n            path[%d]               = %p\n",j,chunk->path[j]);


		/*
		 * Do some basic tests to see if this path address appears
		 * to be valid. If not, then move to the next path.
		 */

		if (chunk->path[j] == NULL) {

		    fflush(cblk_dumpfp);
		    continue;
		}

		if ( (ulong)(chunk->path[j]) & PATH_BAD_ADDR_MASK ) {

		    fprintf(cblk_dumpfp,"        BAD PATH ADDR!!\n");
		    fflush(cblk_dumpfp);

		    continue;
			
		}

		fprintf(cblk_dumpfp,"                afu               = %p\n",chunk->path[j]->afu);
		fprintf(cblk_dumpfp,"                flags             = 0x%x\n",chunk->path[j]->flags);
		fprintf(cblk_dumpfp,"                path_index        = 0x%x\n",chunk->path[j]->path_index);
		fprintf(cblk_dumpfp,"                path_id           = 0x%x\n",chunk->path[j]->path_id);
		fprintf(cblk_dumpfp,"                path_id_mask      = 0x%x\n",chunk->path[j]->path_id_mask);
		fprintf(cblk_dumpfp,"                num_ports         = 0x%x\n",chunk->path[j]->num_ports);
#ifndef _AIX
		fprintf(cblk_dumpfp,"                dev_name          = %s\n",chunk->path[j]->dev_name);
#endif /* !_AIX */
		fprintf(cblk_dumpfp,"                fd                = %d\n",chunk->path[j]->fd);
		fprintf(cblk_dumpfp,"                chunk             = %p",chunk->path[j]->chunk);
		if (chunk->path[j]->chunk == chunk) {
		    fprintf(cblk_dumpfp," (Valid)\n");
		} else {
		    fprintf(cblk_dumpfp," (Invalid !!)\n");
		}
		fprintf(cblk_dumpfp,"                eyec              = 0x%x",chunk->path[j]->eyec);
		
		if (!CFLSH_EYECATCH_PATH(chunk->path[j])) {
		    fprintf(cblk_dumpfp," (Valid)\n");
		} else {
		    fprintf(cblk_dumpfp," (Invalid !!)\n");
		}
		
		
		
		fflush(cblk_dumpfp);
	    }

	    fprintf(cblk_dumpfp,"\n");

	    chunk = chunk->next;
	}

    }

    fflush(cblk_dumpfp);

    fprintf(cblk_dumpfp,"\n    afu list               \n\n");

 
    fflush(cblk_dumpfp);

    afu = cflsh_blk.head_afu;   


    while (afu) {
	

	fprintf(cblk_dumpfp,"\n        afu                     = %p\n",afu);

	/*
	 * Do some basic tests to see if this AFU address appears
	 * to be valid. If not, then exit from this while loop,
	 * since we can not trust the next pointer .
	 */

	if ( (ulong)afu & AFU_BAD_ADDR_MASK ) {

	    fprintf(cblk_dumpfp,"        BAD AFU ADDR!!\n");

	    break;
			
	}

	fprintf(cblk_dumpfp,"            eyec                = 0x%x",afu->eyec);
	
	if (!CFLSH_EYECATCH_AFU(afu)) {
	    fprintf(cblk_dumpfp," (Valid)\n");
	} else {
	    fprintf(cblk_dumpfp," (Invalid !!)\n");
	    fflush(cblk_dumpfp);

	    break;
	}
	

	fprintf(cblk_dumpfp,"            flags               = 0x%x\n",afu->flags);
	fprintf(cblk_dumpfp,"            ref_count           = 0x%x\n",afu->ref_count);
	fprintf(cblk_dumpfp,"            lock.file           = 0x%x\n",afu->lock.file); 
	fprintf(cblk_dumpfp,"            lock.fname          = %s\n",afu->lock.filename);
	fprintf(cblk_dumpfp,"            lock.line           = %d\n",afu->lock.line); 
	fprintf(cblk_dumpfp,"            lock.thread         = 0x%x\n",(uint32_t)(afu->lock.thread));
	fprintf(cblk_dumpfp,"            lock.thread &       = %p\n",&(afu->lock.thread));

	fflush(cblk_dumpfp);

	fprintf(cblk_dumpfp,"            contxt_id           = 0x%"PRIx64"\n",afu->contxt_id);
	fprintf(cblk_dumpfp,"            contxt_handle       = 0x%x\n",afu->contxt_handle);
	fprintf(cblk_dumpfp,"            type                = 0x%x\n",afu->type);
#ifdef _AIX
	fprintf(cblk_dumpfp,"            adap_devno          = 0x%"PRIx64"\n",(uint64_t)afu->adap_devno);
#endif 
	fprintf(cblk_dumpfp,"            toggle              = 0x%"PRIx64"\n",(uint64_t)afu->toggle);
	fprintf(cblk_dumpfp,"            hrrq_start          = %p\n",afu->p_hrrq_start);
	fprintf(cblk_dumpfp,"            hrrq_curr           = %p\n",afu->p_hrrq_curr);
	fprintf(cblk_dumpfp,"            hrrq_end            = %p\n",afu->p_hrrq_end);
	fprintf(cblk_dumpfp,"            num_rrqs            = 0x%x\n",afu->num_rrqs);
	fprintf(cblk_dumpfp,"            size_rrq            = 0x%x\n",afu->size_rrq);
	fprintf(cblk_dumpfp,"            sq_start            = %p\n",afu->p_sq_start);
	fprintf(cblk_dumpfp,"            sq_curr             = %p\n",afu->p_sq_curr);
	fprintf(cblk_dumpfp,"            sq_end              = %p\n",afu->p_sq_end);
	fprintf(cblk_dumpfp,"            size_sq             = 0x%x\n",afu->size_sq);
	fprintf(cblk_dumpfp,"            num_issued_cmds     = 0x%x\n",afu->num_issued_cmds);
	fprintf(cblk_dumpfp,"            cmd_room            = 0x%"PRIx64"\n",afu->cmd_room);

	fflush(cblk_dumpfp);

	fprintf(cblk_dumpfp,"            mmio                = %p\n",afu->mmio);
	fprintf(cblk_dumpfp,"            mmap_size           = 0x%"PRIx64"\n",afu->mmap_size);
	fprintf(cblk_dumpfp,"            master_name         = %s\n",afu->master_name);
	fprintf(cblk_dumpfp,"            head_path           = %p\n",afu->head_path);
	fprintf(cblk_dumpfp,"            tail_path           = %p\n",afu->tail_path);
	fprintf(cblk_dumpfp,"            head_complete       = %p\n",afu->head_complete);
	fprintf(cblk_dumpfp,"            tail_complete       = %p\n",afu->tail_complete);
	
        afu = afu->next;

	fflush(cblk_dumpfp);

    } /* while */


    fflush(cblk_dumpfp);

    return;
}


/*
 * NAME:        cblk_signal_dump_handler
 *
 * FUNCTION:    Sets up a signal handler to 
 *              For signal to dump our internal data
 *              structures.
 *
 * RETURNS:
 *              
 *             0 - Good completion, otherwise error 
 *              
 */

void cblk_signal_dump_handler(int signum, siginfo_t *siginfo, void *uctx)
{
    char reason[100];

    if (signum == SIGSEGV) {

	CBLK_TRACE_LOG_FILE(1,"si_code = %d, si_addr = 0x%p",
			    siginfo->si_code,siginfo->si_addr);
	
	sprintf(reason,"SIGSEGV: si_code = %d si_addr = %p",siginfo->si_code,siginfo->si_addr);


    } else {
	
	sprintf(reason,"Signal = %d",signum);

    }

    cblk_dump_debug_data(reason,__FILE__,__FUNCTION__,__LINE__,__DATE__);


    
    /*
     * If we get here then 
     * issue default signal.
     */

    if (signum != SIGUSR1) {

	signal(signum,SIG_DFL);
	kill(getpid(),signum);
    }

    return;
}

/*
 * NAME:        cblk_setup_sigsev_dump
 *
 * FUNCTION:    Sets up a signal handler to 
 *              For SIGSEGV to dump our internal data
 *              structures.
 *
 * RETURNS:
 *              
 *             0 - Good completion, otherwise error 
 *              
 */

int cblk_setup_sigsev_dump(void)
{
    int rc = 0;
    struct sigaction action, oaction;



    if (cblk_setup_dump_file()){

	return -1;

    }


    bzero((void *)&action,sizeof(action));

    bzero((void *)&oaction,sizeof(oaction));

    action.sa_sigaction = cblk_signal_dump_handler;
    action.sa_flags = SA_SIGINFO;


    if (sigaction(SIGSEGV, &action,&oaction)) {

        CBLK_TRACE_LOG_FILE(1,"Failed to set up SIGSEGV handler with errno = %d\n",
			    errno);

	return -1;
    }


    return rc;
}


/*
 * NAME:        cblk_setup_sigusr1_dump
 *
 * FUNCTION:    Sets up a signal handler to 
 *              For SIGSEGV to dump our internal data
 *              structures.
 *
 * RETURNS:
 *              
 *             0 - Good completion, otherwise error 
 *              
 */

int cblk_setup_sigusr1_dump(void)
{
    int rc = 0;
    struct sigaction action, oaction;



    if (cblk_setup_dump_file()){

	return -1;

    }


    bzero((void *)&action,sizeof(action));

    bzero((void *)&oaction,sizeof(oaction));

    action.sa_sigaction = cblk_signal_dump_handler;
    action.sa_flags = SA_SIGINFO;


    if (sigaction(SIGUSR1, &action,&oaction)) {

        CBLK_TRACE_LOG_FILE(1,"Failed to set up SIGUSR handler with errno = %d\n",
			    errno);

	return -1;
    }


    return rc;
}


/*
 * NAME:        cblk_dcbz_buffer
 *
 * FUNCTION:    Clear poison bits for potential 
 *              Uncorrectable Errors (UE) using the DCBZ
 *              (Data Cache Block set to Zero).
 *
 * RETURNS:
 *              
 *             None
 *              
 */
void cblk_dcbz_buffer(void *buf, size_t len)
{

    ulong start_addr;
    ulong end_addr;
    ulong addr;


    start_addr = (ulong)buf;
    end_addr = (ulong)buf+len;


    /*
     * Align start and end addresses on a cacheline boundary
     */

    if (start_addr & (CBLK_DCBZ_CACHE_LINE_SZ-1)) {

	start_addr = ((ulong)start_addr+CBLK_DCBZ_CACHE_LINE_SZ) & ~(CBLK_DCBZ_CACHE_LINE_SZ-1);

    }

    if (end_addr & (CBLK_DCBZ_CACHE_LINE_SZ-1)) {

	end_addr = ((ulong)end_addr+CBLK_DCBZ_CACHE_LINE_SZ) & ~(CBLK_DCBZ_CACHE_LINE_SZ-1);

    }

    addr = start_addr;

    /*
     * Issue DCBZ (Data Cache Block set to Zero) to clear 
     * all poison bits across the data buffer.
     */

    while (addr < end_addr) {
#ifndef TARGET_ARCH_x86_64

	asm volatile ("dcbz 0, %0" : : "r" (addr) : "memory");
#endif
	addr += CBLK_DCBZ_CACHE_LINE_SZ;
    }

    /* issue sync for cache lines */
#ifndef TARGET_ARCH_x86_64

    asm volatile ( "lwsync" : : ); 
#endif
    return;
}


/*
 * NAME:        cblk_clear_poison_bits
 *
 * FUNCTION:    Clear poison bits for potential 
 *              Uncorrectable Errors (UE).
 *
 * RETURNS:
 *              
 *             None
 *              
 */

void cblk_clear_poison_bits(void *buf, size_t len)
{


#ifdef _AIX
#ifdef _BSTERILE

    __bsterile(buf, len);
#else
    cblk_dcbz_buffer(buf, len);
    
#endif
#else

    cblk_dcbz_buffer(buf, len);
#endif

    return;
   
}


/*
 * NAME:        cblk_query_ue
 *
 * FUNCTION:    Checks if UE has occurred in
 *              the specified buffer.
 *
 * RETURNS:
 *              
 *             1 if UE found. O otherwise
 *              
 */

int cblk_query_ue(void *buf, size_t len)
{
    int ue_found = FALSE;

#if defined(_CBLK_UE_SAFE) && defined(_AIX)

    if (ue_query(buf,len)) {

	ue_found = TRUE;
	CBLK_TRACE_LOG_FILE(9,"buf = %p ue_found = %d",buf,ue_found);
    }

#endif

    return ue_found;
   
}

/*
 * NAME:        cblk_check_cmd_data_buf_for_ue
 *
 * FUNCTION:    Checks if UE has occurred in
 *              the specified data buffer of a command.
 *              If so then that command is failed with
 *              ENOTRECOVERABLE.
 *
 * RETURNS:
 *              
 *             None
 *              
 */

void cblk_check_cmd_data_buf_for_ue(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{

    cflsh_cmd_info_t *cmdi;
#ifdef _COMMON_INTRPT_THREAD
    int pthread_rc;
#endif /* _COMMON_INTRPT_THREAD	*/


    if (chunk == NULL) {

	return;
    }

    if (cmd == NULL) {

	return;
    }

    cmdi = cmd->cmdi;

    if (cmdi == NULL) {

	return;
    }


    if (cblk_query_ue(cmdi->buf,(cmdi->nblocks*CAPI_FLASH_BLOCK_SIZE))) {
	
	
	/*
	 * If this request's data buffer has a UE, then
	 * that request needs to be failed immediately
	 * with an errno of ENOTRECOVERABLE, since we
	 * can not clear the data buffer because it is owned
	 * by another command. The caller maybe able to retry 
	 * and recover if they can get the data from some other 
	 * place then this buffer.
	 */


	cmdi->status = ENOTRECOVERABLE;
	cmdi->transfer_size = 0;
			    

	cblk_notify_mc_err(chunk,cmdi->path_index,0x20e,0,
			   CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);

	CBLK_TRACE_LOG_FILE(1,"Command with UE  lba = 0x%llx flags = 0x%x, chunk->index = %d",
			    cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);

		
	/*
	 * Fail command back.
	 */
		
	cmdi->state = CFLSH_MGM_CMP;
#ifdef _COMMON_INTRPT_THREAD

	if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
	    /*
	     * If we are using a common interrupt thread
	     */
			
	    pthread_rc = pthread_cond_signal(&(cmdi->thread_event));
		
	    if (pthread_rc) {
			
		CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d, chunk->index = %d",
				    pthread_rc,errno,chunk->index);
	    }
		    
		    
	    /*
	     * Signal any one waiting for any command to complete.
	     */
		    
	    pthread_rc = pthread_cond_signal(&(chunk->cmd_cmplt_event));
		    
	    if (pthread_rc) {
			
		CBLK_TRACE_LOG_FILE(5,"pthread_cond_signal failed for cmd_cmplt_event rc = %d,errno = %d, chunk->index = %d",
				    pthread_rc,errno,chunk->index);
	    }
	}
		
#endif /* _COMMON_INTRPT_THREAD	*/

    }

    return;
   
}



/*
 * NAME:        cblk_clear_poison_bits_chunk
 *
 * FUNCTION:    Clear poison bits for potential 
 *              Uncorrectable Errors (UE). When a
 *              CAPI Flash adapter goes thru recovery
 *              (EEH, reset etc), there is the possibility
 *              that the event that caused this (such as
 *              a CAPP link going down), corrupted cachelines.
 *              To prevent data corruption
 *              the hardware will mark those cachelines as
 *              poison: any access of that cacheline will
 *              cause an Uncorrectable Error, which at a
 *              minimum would cause the application to segfault.
 *	        Since there is no way to determine prior to
 *              consuming the cacheline that a UE would occur,
 *              this code will error on the side of assuming
 *              this is always true for IOARCBs and data
 *              read buffers. As result it will use
 *              an OS specific interface (invoked from
 *              inside cblk_clear_poison_bits) to clear
 *              IOARCBs and read data buffers. 
 * 
 *              Write data buffers will not be cleared,
 *              since we have no interface to reconstruct
 *              the write data buffer.  Thus
 *              if write data cachelines were corrupted a UE
 *              could still happen. 
 *              
 *
 * NOTES:       This routine assumes the caller
 *              has the chunk lock and that
 *              all active commands are halted.
 *
 * RETURNS:
 *              
 *             None
 *              
 */

void cblk_clear_poison_bits_chunk(cflsh_chunk_t *chunk, int path_index, int all_paths)
{

    int i,j;
    scsi_cdb_t *cdb = NULL;
    cflsh_cmd_mgm_t *cmd = NULL;
    cflsh_cmd_info_t *cmdi, *cmdi2;
    int local_flags = 0;
    uint8_t op_code = 0;
    ulong cmdi_start_addr;
    ulong cmdi_end_addr;
    ulong cmdi2_start_addr;
    ulong cmdi2_end_addr;




    CBLK_TRACE_LOG_FILE(9,"chunk = %p,path_index = %d, all_paths = %d",
			chunk,path_index,all_paths);


    /*
     * Clear the RRQ here, since it could
     * have poison bits. 
     *
     * NOTE: When the RRQ was allocated, we ensured
     * it started and ended on cacheline boundaries.
     * Since cblk_clear_poison_bits only clears poison
     * bits in a cacheline this was needed.
     */
    
    cblk_clear_poison_bits(chunk->path[path_index]->afu->p_hrrq_start,chunk->path[path_index]->afu->size_rrq);


    /*
     * For each active command, clear all IOARCBs
     * and rebuild the IOARCB
     * (from the information in the associated
     * cmd_info) to allow it to be re-issued
     * safely later.
     *
     * Also for reads, we will also clear the data
     * buffers to prevent UEs when access the read
     * data buffer. .
     */
    
    for (i=0; i < chunk->num_cmds; i++) {
	if ((chunk->cmd_info[i].in_use) &&
	    (chunk->cmd_info[i].state == CFLSH_MGM_HALTED) &&
	    (all_paths || (chunk->cmd_info[i].path_index == path_index))) {

	    /*
	     * cmd_info and cmd_start array entries
	     * with the same index correspond to the
	     * same command.
	     */

	    cmdi = &chunk->cmd_info[i];

	    cmd = &chunk->cmd_start[i];
	    
	    if (cmdi->flags & CFLSH_MODE_READ) {

		/*
		 * Read request
		 */

		op_code = SCSI_READ_16;

		local_flags = CFLASH_READ_DIR_OP;
		
	    } else if (cmdi->flags & CFLSH_MODE_WRITE) {

		
		/*
		 * Write request
		 */

		op_code = SCSI_WRITE_16;

		local_flags = CFLASH_WRITE_DIR_OP;

	    } else {

		/*
		 * This is invalid request: skip to the next one
		 */
		CBLK_TRACE_LOG_FILE(1,"Invalid cmdi->flags: no read/write flags = 0x%x",cmdi->flags);

		continue;
	    }

	    /*
	     * Only the adapter specific area of a command
	     * could have poison bits, but for simplicity (to avoid
	     * using a an adapter specific routine to clear
	     * the poison bits), we will just clear the whole
	     * cmd.
	     */

	    cblk_clear_poison_bits(cmd,(sizeof(*cmd)));

	    cmd->cmdi = cmdi;

	    cmd->index = i;

	    CBLK_BUILD_ADAP_CMD(chunk,cmd,cmdi->buf,(CAPI_FLASH_BLOCK_SIZE * cmdi->nblocks),local_flags);
    
	    cdb = CBLK_GET_CMD_CDB(chunk,cmd);

	    cdb->scsi_op_code = op_code;

	    CFLASH_BUILD_RW_16(cdb,(chunk->start_lba + cmdi->lba)*(chunk->blk_size_mult),cmdi->nblocks*(chunk->blk_size_mult));
		

	    if (cmdi->flags & CFLSH_MODE_READ) {

	        /*
		 * Clear cache aligned portion of read data buffer of any potential poison bits
		 */

		cblk_clear_poison_bits(cmdi->buf,(cmdi->nblocks*CAPI_FLASH_BLOCK_SIZE));

                if ((ulong)cmdi->buf & (CBLK_DCBZ_CACHE_LINE_SZ-1)) {


                    /*
                     * This data buffer is not cache aligned and 
                     * the non-aligned portion could have UE that was not
                     * cleared when we cleared the cache aligned portion above.
                     * For the non-cache aligned part we need to see 
                     * if fortunately one of the other read commands has the remainder
                     * of this cache line and then clear the poison bits on that cache 
                     * line. Otherwise this will be a fatal error for this request.
                     *
                     * NOTE: This code assumes that a cache line size is smaller than
                     *       a disk block size. Thus at most there will be one other read
                     *       request that shares the same cache line with the 
                     *       beginning of this read/s buffers and one read request
                     *       that shares the same cache line with the end of this read's
                     *       buffer.
                     *       
                     */


                    
                    CBLK_TRACE_LOG_FILE(1,"non-aligned Read command with UE  lba = 0x%llx flags = 0x%x, chunk->index = %d",
                                        cmdi->lba,cmd->cmdi->flags,chunk->index);

                    cmdi_start_addr = (ulong)((ulong)cmdi->buf & ~(CBLK_DCBZ_CACHE_LINE_SZ-1));

                    cmdi_end_addr = (ulong)(((ulong)cmdi->buf +(cmdi->nblocks*CAPI_FLASH_BLOCK_SIZE)) & ~(CBLK_DCBZ_CACHE_LINE_SZ-1));

                    for (j=0; j < chunk->num_cmds; j++) {
                        if ((chunk->cmd_info[j].in_use) &&
                            (chunk->cmd_info[j].state == CFLSH_MGM_HALTED) &&
			    (j != i) &&
                            (all_paths || (chunk->cmd_info[j].path_index == path_index))) {
			    
                            cmdi2 = &chunk->cmd_info[j];

                            if (cmdi2->flags & CFLSH_MODE_READ) {
				
				/*
				 * This is also a read so that we can clear poison bits
				 * if necessary.
				 */

				cmdi2_start_addr = (ulong)((ulong)cmdi2->buf & ~(CBLK_DCBZ_CACHE_LINE_SZ-1));

				cmdi2_end_addr = (ulong)(((ulong)cmdi2->buf +(cmdi2->nblocks*CAPI_FLASH_BLOCK_SIZE)) & ~(CBLK_DCBZ_CACHE_LINE_SZ-1));

				if (cmdi2_start_addr == cmdi_end_addr) {

				    /*
				     * This cacheline is starts with cmdi2's data buffer
				     * and ends at the 1st cacheline boundary in cmdi's
				     * data buffer. Lets clear the poison bits in it.
				     */

				    cblk_clear_poison_bits((void *)cmdi2_start_addr,CBLK_DCBZ_CACHE_LINE_SZ);

				    CBLK_TRACE_LOG_FILE(1,"non-aligned Read command found other command with UE  lba = 0x%llx flags = 0x%x, chunk->index = %d",
                                        cmdi->lba,cmd->cmdi->flags,chunk->index);




				} else if (cmdi_start_addr == cmdi2_end_addr) {

				    /*
				     * This cacheline is starts with cmdi's data buffer
				     * and ends at the 1st cacheline boundary in cmdi2's
				     * data buffer. Lets clear the poison bits in it.
				     */

				    cblk_clear_poison_bits((void *)cmdi_start_addr,CBLK_DCBZ_CACHE_LINE_SZ);

				    CBLK_TRACE_LOG_FILE(1,"non-aligned Read command found other command with UE  lba = 0x%llx flags = 0x%x, chunk->index = %d",
                                        cmdi->lba,cmd->cmdi->flags,chunk->index);
				}
			    }
			}
			
		    } /* inner for loop */
		}

	    }
	    
	    
	    /*
	     * According to the CAPI architecture, we should never get a UE on a host buffer
	     * that is write only to storage and is cache aligned. UE's arise when the 
	     * AFU can checkout a cacheline to modify it (a Read from the host perspective) and the 
	     * CAPI link is dropped. Since a data buffer may not be cache aligned, either a read
	     * or write request could have a data buffer that contains a cache line shared
	     * by another command that is a read buffer. Thus that portion of the cacheline could
	     * have a UE.  We will call the following routine to determine if this is is the case.
	     * and fail it if so.
	     */
	    
	    cblk_check_cmd_data_buf_for_ue(chunk,cmd);
	    


	    /*
	     * Set new time
	     * in command
	     */

	    cmdi->cmd_time = time(NULL);


	    
	}

    } /* for */

    return;
}
