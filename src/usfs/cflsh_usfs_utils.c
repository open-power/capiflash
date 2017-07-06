//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/usfs/cflsh_usfs_utils.c $
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

#define CFLSH_USFS_FILENUM 0x0300


#include "cflsh_usfs_internal.h"
#include "cflsh_usfs_protos.h"


#ifdef _AIX
#include <procinfo.h>

#if !defined(__64BIT__)
#include <dlfcn.h>
#endif
#endif

char             cusfs_filename[PATH_MAX];

extern void cflsh_usfs_init(void);

#if !defined(__64BIT__) && defined(_AIX)
static time64_t (*real_time64)(time64_t *tp) = NULL;


/*
 * NAME:        time64
 *
 * FUNCTION:    For some levels of AIX, time64 is not
 *              available for 32-bit kernel. This
 *              function tries to determine if time64
 *              is available. If so it will use it. Otherwise
 *              it will use time.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     -1 failure, Otherwise success
 *              
 *              
 */

time64_t cflsh_usfs_time64(time64_t *tp)
{
    time64_t rc;
    time_t *tp32;

    /*
     * See if time64 is available on this system
     */

    real_time64 = (time64_t(*)(time64_t *))dlsym(RTLD_DEFAULT,"time64");

    if (real_time64) {
	
	/*
	 * This system has time64 for 32-bit applications.
	 */
	rc = real_time64(tp);
	
    } else {

	/*
	 * This system does not have time64 for 32-bit applications.
	 */

	if (tp) {

	    tp32 = (time_t *)tp;

	    /*
	     * Move to low order word of tp;
	     */

	    tp32++;

	    rc = time(tp32);

	    
	} else {
	    rc = time(NULL);
	}

    }

    return rc;

}

#endif /* 32-bit AIX */

/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_setup_trace_files
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
void cusfs_setup_trace_files(int new_process)
{
    int i;
    char *env_verbosity = getenv("CFLSH_USFS_TRC_VERBOSITY");
    char *env_use_syslog = getenv("CFLSH_USFS_TRC_SYSLOG");
    char *env_trace_append = getenv("CFLSH_USFS_TRC_APPEND");
    char *log_pid  = getenv("CFLSH_USFS_TRACE_PID");
    char *env_num_thread_logs  = getenv("CFLSH_USFS_TRACE_TID");
    char *env_user     = getenv("USER");
    uint32_t thread_logs = 0;
    char filename[PATH_MAX];
    char *filename_ptr = filename;
    int open_flags = 0;


#ifndef _AIX
    cusfs_global.flags |= CFLSH_G_SYSLOG;

    if (env_use_syslog) {

	if (strcmp(env_use_syslog,"ON")) {

	    /*
	     * Don't use syslog tracing. Instead 
	     * use tracing to a file.
	     */
	    cusfs_global.flags &= ~CFLSH_G_SYSLOG;


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
	    (cusfs_global.flags & CFLSH_G_SYSLOG)) {
	    
	    /*	
	     * If this is a new process (forked process)
	     * and we are not using traces per process,
	     * or we are logging via syslog
	     * then continue to the use the tracing 
	     * in place for the parent process.
	     */
	    
	    return;
	}

	strcpy(filename,cusfs_log_filename);
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
	cusfs_log_verbosity = atoi(env_verbosity);
	
    } else {
#ifdef _AIX
	cusfs_log_verbosity = 0;
#else
	cusfs_log_verbosity = 1;
#endif
    }

    cusfs_log_filename = getenv("CFLSH_USFS_TRACE");
    if (cusfs_log_filename == NULL)
    {
        sprintf(cusfs_filename, "/tmp/%s.cflsh_usfs_trc", env_user);
        cusfs_log_filename = cusfs_filename;
    }

    if ((log_pid) && !(cusfs_global.flags & CFLSH_G_SYSLOG)) {
	
	/*
	 * Use different filename for each process, when
	 * not using syslogging.
	 */

	sprintf(cusfs_filename,"%s.%d",cusfs_log_filename,getpid());

	if ((new_process) &&
	    !strcmp(cusfs_log_filename,filename)) {

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

	cusfs_log_filename = cusfs_filename;

    }

    bzero((void *)&(cusfs_global.trace_ext),sizeof(trace_log_ext_arg_t));

    /*
     * We need to serialize access to this log file
     * while we are setting it up.
     */

    pthread_mutex_lock(&cusfs_log_lock);

    if (cusfs_global.flags & CFLSH_G_SYSLOG) {

	openlog("CXLBLK",LOG_PID,LOG_USER);


    } else if (cusfs_log_verbosity) {

	if (setup_trace_log_file_ext(&cusfs_log_filename,&cusfs_logfp,cusfs_log_filename,open_flags)) {

	    //fprintf(stderr,"Failed to set up tracing for filename = %s\n",cusfs_log_filename);

	    /*
	     * Turn off tracing if this fails.
	     */
	    cusfs_log_verbosity = 0;
	}
    }



    if ((env_num_thread_logs) && !(cusfs_global.flags & CFLSH_G_SYSLOG)) {

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

	    free(cusfs_global.thread_logs);
	}

	num_thread_logs = atoi(env_num_thread_logs);

	num_thread_logs = MIN(num_thread_logs, MAX_NUM_THREAD_LOGS);

	if (num_thread_logs) {

	    /*
	     * Allocate there array of thread_log file pointers:
	     */

	    cusfs_global.thread_logs = (cflsh_thread_log_t *) malloc(num_thread_logs * sizeof(cflsh_thread_log_t));

	    if (cusfs_global.thread_logs) {

		bzero((void *)cusfs_global.thread_logs, num_thread_logs * sizeof(cflsh_thread_log_t));


		for (i=0; i< num_thread_logs;i++) {

		    sprintf(filename,"%s.%d",cusfs_log_filename,i);

		    
		    if (setup_trace_log_file(&filename_ptr,&cusfs_global.thread_logs[i].logfp,filename)) {

			fprintf(stderr,"Failed to set up tracing for filename = %s\n",filename);
			free(cusfs_global.thread_logs);

			num_thread_logs = 0;
			break;
		    }
		    
		    cusfs_global.thread_logs[i]. ext_arg.flags |= TRACE_LOG_NO_USE_LOG_NUM;

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
		cusfs_global.thread_log_mask = 0;

		while (thread_logs) {

		    cusfs_global.thread_log_mask = (cusfs_global.thread_log_mask << 1) | 1;
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

    pthread_mutex_unlock(&cusfs_log_lock);


    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_valid_endianess
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
 * RETURNS:  1  Host endianness matches compile flags
 *           0  Host endianness is invalid based on compile flags
 *     
 * ----------------------------------------------------------------------------
 */
int cusfs_valid_endianess(void)
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
 * NAME: cusfs_get_host_type
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


void cusfs_get_host_type(void)
{
#ifdef _AIX

    /*
     * For AIX always assume pHyp platform type
     */

    cusfs_global.host_type = CFLSH_USFS_HOST_PHYP;
    return;

#else
#define CFLSH_USFS_HOST_BUF_SIZE   256
    FILE *fp;
    char buf[CFLSH_USFS_HOST_BUF_SIZE];
    char *platform_line;

    /*
     * Set host type to unknown until we had a chance
     * to deterine otherwise.
     */

    cusfs_global.host_type = CFLSH_USFS_HOST_UNKNOWN;

    if ((fp = fopen(CFLSH_USFS_HOST_TYPE_FILE,"r")) == NULL) {

        CUSFS_TRACE_LOG_FILE(1,"Can not determine host type, failed to open = %s",
                            CFLSH_USFS_HOST_TYPE_FILE);
        return;
    }


    
    while (fgets(buf,CFLSH_USFS_HOST_BUF_SIZE,fp)) {


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

            CUSFS_TRACE_LOG_FILE(9,"Platform_line = %s",platform_line);


            /*
             * NOTE: The order below is import, because two of the 
             * platform strings start with pSeries.
             */

            if (strstr(platform_line,"PowerNV")) {

                CUSFS_TRACE_LOG_FILE(9,"BML host type");
                cusfs_global.host_type = CFLSH_USFS_HOST_NV;

                break;
            } else if (strstr(platform_line,"pSeries (emulated by qemu)")) {

                CUSFS_TRACE_LOG_FILE(9,"PowerKVM host type");
                cusfs_global.host_type = CFLSH_USFS_HOST_KVM;

            } else if (strstr(platform_line,"pSeries")) {

                CUSFS_TRACE_LOG_FILE(9,"pHyp host type");
                cusfs_global.host_type = CFLSH_USFS_HOST_PHYP;

            } else {
                CUSFS_TRACE_LOG_FILE(1,"Unknown platform string= %s",
                            platform_line);
            }

            break;


        }

    }

    if (cusfs_global.host_type == CFLSH_USFS_HOST_UNKNOWN) {

        CUSFS_TRACE_LOG_FILE(9,"could not determine host type");
    }


    fclose (fp);
    
    return;
#endif
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_chunk_sigsev_handler
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
void  cusfs_chunk_sigsev_handler (int signum, siginfo_t *siginfo, void *uctx)
{
    int i;
    cflsh_usfs_t *cufs;

    CUSFS_TRACE_LOG_FILE(1,"si_code = %d, si_addr = 0x%p",
			siginfo->si_code,siginfo->si_addr);

    switch (siginfo->si_code) {
#ifdef _AIX
      case SEGV_MAPERR: 
	  CUSFS_TRACE_LOG_FILE(1,"Address not mapped, address = 0x%p",
			      siginfo->si_addr);

	break;
#endif /* _AIX */
      case SEGV_ACCERR: 

	CUSFS_TRACE_LOG_FILE(1,"Invalid permissions, address = 0x%p",
			    siginfo->si_addr);

	break;
      default:

	CUSFS_TRACE_LOG_FILE(1,"Unknown si_code = %d, address = 0x%p",
		siginfo->si_code,siginfo->si_addr);
    }
    

    for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];
    

	while (cufs) {
#ifdef _REMOVE

	    for (j=0; j < chunk->num_paths;j++) {
		if ((chunk->flags & CFLSH_CHNK_SIGH) &&
		    (chunk->path[j]) &&) {

		    longjmp(chunk->path[j]->jmp_mmio,1);
		}

	    } /* for */
#endif /* _REMOVE */
	    cufs = cufs->next;

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


/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_prepare_fork
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
void cusfs_prepare_fork (void)
{
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file = NULL;
    int i;


    /*
     * ?? TODO: Is anything needed here for semop
     *          or socket locks?
     */

    pthread_mutex_lock(&cusfs_log_lock);

    CUSFS_WR_RWLOCK(cusfs_global.global_lock);


    for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];
    

	while (cufs) {

	    CUSFS_LOCK(cufs->lock);
	    CUSFS_LOCK(cufs->free_table_lock);
	    CUSFS_LOCK(cufs->inode_table_lock);
	    CUSFS_LOCK(cufs->journal_lock);
	    
	    
	    file = cufs->filelist_head;
	    
	    while (file) {
		
		CUSFS_LOCK(file->lock);
		
		file = file->next;
	    }


	    cufs = cufs->next;

	} /* while */


    } /* for */


    return;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_parent_post_fork
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
void  cusfs_parent_post_fork (void)
{
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file = NULL;
    int i;
    int rc;




    /*
     * ?? TODO: Is anything needed here for semop
     *          or socket locks?
     */

    for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];
    

	while (cufs) {

	    CUSFS_UNLOCK(cufs->lock);
	    CUSFS_UNLOCK(cufs->free_table_lock);
	    CUSFS_UNLOCK(cufs->inode_table_lock);
	    CUSFS_UNLOCK(cufs->journal_lock);
	    
	    
	    file = cufs->filelist_head;
	    
	    while (file) {
		
		CUSFS_UNLOCK(file->lock);
		
		file = file->next;
	    }

	    cufs = cufs->next;

	} /* while */


    } /* for */



    rc = pthread_mutex_unlock(&cusfs_log_lock);

    if (rc) {

	/*
	 * Find the first chunk do a notify
	 * against it.
	 */

	for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	    cufs = cusfs_global.hash[i];
    

	    if (cufs) {

		break;
	    }


	} /* for */
	
#ifdef _REMOVE
	if (cufs) {

//??	    cusfs_notify_mc_err(cufs,0,0x205,rc, CUSFS_GLOBAL_NOTIFY_SFW_ERR,NULL);

	}
#endif /* _REMOVE */
    }


    CUSFS_RWUNLOCK(cusfs_global.global_lock);
    return;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_aio_cleanup()
 *
 * FUNCTION:  If a process using this library does a fork, then
 *            this  routine will be run on the child after fork
 *            to clean up all aio related threads and resources
 *
 * RETURNS:  NONE
 *
 * ----------------------------------------------------------------------------
 */

void cusfs_aio_cleanup ( cflsh_usfs_t *cufs )
{

    cflsh_usfs_aio_entry_t *aio;
    cflsh_usfs_aio_entry_t *next_aio;
    cflsh_usfs_data_obj_t *file;

    aio = cufs->head_act;

    while (aio)
    {
        next_aio = aio->act_next;

        file = aio->file;

        /* caller of cusfs_free_aio() should have file lock */

        if (file)
           CUSFS_LOCK(file->lock);
        
        if( cusfs_free_aio(cufs,aio,0))
        {
          cufs->flags = ECHILD;

          if (file)
             CUSFS_UNLOCK(file->lock);
          break;
        }

        if (file)
           CUSFS_UNLOCK(file->lock);

        aio = next_aio;
    }

    CUSFS_TRACE_LOG_FILE(9,"aio cleanup called for disk %s, flags %d",cufs->device_name,cufs->flags);

    return ;

}
/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_fork_manager
 *
 * FUNCTION:  If a process using this library does a fork, then
 *            this routine will be run on the child after fork
 *            to manage the fork() semantics.
 *
 * RETURNS:  NONE
 *
 * ----------------------------------------------------------------------------
 */

void cusfs_fork_manager(void)
{
    int i;
#ifdef _REMOVE
    chunk_ext_arg_t ext = NULL;
#endif /* _REMOVE */
    cflsh_usfs_t *cufs = NULL;

    for (i=0; i < MAX_NUM_CUSFS_HASH; i++)
    {
         cufs = cusfs_global.hash[i];

         while (cufs)
         {
             CUSFS_LOCK(cufs->lock);

	     cufs->stats.num_success_threads = 0;
		
	     cufs->stats.num_active_threads = 0;
		
#ifdef _REMOVE
             cufs->chunk_id = cblk_open(cufs->device_name,cufs->max_requests,
                                           O_RDWR,ext,0);
             if (cufs->chunk_id == NULL_CHUNK_ID)
             {
                CUSFS_TRACE_LOG_FILE(1,"cblk_open failed for device= %s",cufs->device_name);
             }
	    
#endif /* REMOVE */
             if (cufs->async_pool_num_cmds)
             {
                cusfs_aio_cleanup(cufs);
             }

             if (cusfs_start_aio_thread_for_cufs(cufs,0))
             {
                 cufs->flags |= CFLSH_USFS_FLG_FAIL_FORK;
                 CUSFS_TRACE_LOG_FILE(1,"aio_thread fail to start for disk %s",cufs->device_name);

             }

             CUSFS_UNLOCK(cufs->lock);

             cufs = cufs->next;
         }
    }

   return ;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_child_post_fork
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
void  cusfs_child_post_fork (void)
{
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file = NULL;
    int i;
    int rc;



    /*
     * ?? TODO: Is anything needed here for semop
     *          or socket locks?
     */

    for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];
    

	while (cufs) {

	    CUSFS_UNLOCK(cufs->lock);
	    CUSFS_UNLOCK(cufs->free_table_lock);
	    CUSFS_UNLOCK(cufs->inode_table_lock);
	    CUSFS_UNLOCK(cufs->journal_lock);
	    
	    
	    file = cufs->filelist_head;
	    
	    while (file) {
		
		CUSFS_UNLOCK(file->lock);
		
		file = file->next;
	    }


	    cufs = cufs->next;

	} /* while */


    } /* for */



    rc = pthread_mutex_unlock(&cusfs_log_lock);

    if (rc) {


	/*
	 * Find the first chunk do a notify
	 * against it.
	 */

	for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	    cufs = cusfs_global.hash[i];
    
	    
	    if (cufs) {

		 break;
	    }


	} /* for */
#ifdef _REMOVE
	if (cufs) {

//??	    cusfs_notify_mc_err(cufs,0,0x206,rc, CUSFS_GLOBAL_NOTIFY_SFW_ERR,NULL);

	}
#endif /* _REMOVE */
    } else {

	cusfs_global.caller_pid = getpid();

	/*
	 * Since we forked the child, if we have tracing turned
	 * on for a trace file per process, then we need to
	 * open the new file for this child's PID.  The routine
	 * cblk_setup_trace_files will handle the situation
	 * where multiple chunks are cloned and using the same
	 * new trace file.
	 */

	cusfs_setup_trace_files(TRUE);

	CUSFS_TRACE_LOG_FILE(9,"PID = 0x%"PRIx64"",(uint64_t)cusfs_global.caller_pid);

        /*
         * We have forked the child and we need to re-open
         * device using cblk_open and get the chunk id again.
         * Child process does not inherit any pending or
         * on-going asynchronous IO openration fron its parent
         * process; aio active comamnd will be cleaned up and
         * aio handler thread will re-started again for child
         */
        cusfs_fork_manager();


    }


    CUSFS_RWUNLOCK(cusfs_global.global_lock);

    return;
}




/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_trace_log_data_ext
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
void cusfs_trace_log_data_ext(trace_log_ext_arg_t *ext_arg, FILE *logfp,char *filename, char *function, 
					   uint line_num,char *msg, ...)
{
    va_list ap;
    struct timeb cur_time, log_time, delta_time;
    uint print_log_number;
    char timebuf[TIMELEN+1];
#if !defined(__64BIT__) && defined(_AIX)
    time64_t curtime;
#else
    time_t curtime;
#endif /* not 32-bit AIX */

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

#if !defined(__64BIT__) && defined(_AIX)
	curtime = cflsh_usfs_time64(NULL);
	fprintf(logfp,"Trace started at %s\n",ctime64_r(&curtime,timebuf));
#else
	curtime = time(NULL);
	fprintf(logfp,"Trace started at %s\n",ctime_r(&curtime,timebuf));
#endif /* not 32-bit AIX */

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
 * NAME: cusfs_display_stats
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

void  cusfs_display_stats(cflsh_usfs_t *cufs, int verbosity)
{
    CUSFS_TRACE_LOG_FILE(verbosity,"\nCUFS STATISTICS ...");

    
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    CUSFS_TRACE_LOG_FILE(verbosity,"Little Endian");
#else
    CUSFS_TRACE_LOG_FILE(verbosity,"Big Endian");
#endif     
    CUSFS_TRACE_LOG_FILE(verbosity,"cusfs_log_verbosity              0x%x",cusfs_log_verbosity);
#if !defined(__64BIT__) && defined(_AIX)
    CUSFS_TRACE_LOG_FILE(verbosity,"32-bit app support              ");
#else
    CUSFS_TRACE_LOG_FILE(verbosity,"64-bit app support              ");

#endif
    CUSFS_TRACE_LOG_FILE(verbosity,"flags                           0x%x",cusfs_global.flags);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_areads                      0x%llx",cufs->stats.num_areads);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_awrites                     0x%llx",cufs->stats.num_awrites);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_error_areads                0x%llx",cufs->stats.num_error_areads);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_error_awrites               0x%llx",cufs->stats.num_error_awrites);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_act_areads                  0x%x",cufs->stats.num_act_areads);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_act_awrites                 0x%x",cufs->stats.num_act_awrites);
    CUSFS_TRACE_LOG_FILE(verbosity,"max_num_act_areads              0x%x",cufs->stats.max_num_act_areads);
    CUSFS_TRACE_LOG_FILE(verbosity,"max_num_act_awrites             0x%x",cufs->stats.max_num_act_awrites);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_success_threads             0x%llx",cufs->stats.num_success_threads);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_failed_threads              0x%llx",cufs->stats.num_failed_threads);
    CUSFS_TRACE_LOG_FILE(verbosity,"num_active_threads              0x%llx",cufs->stats.num_active_threads);

    return;
}

/*
 * NAME: cusfs_setup_dump_file
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

int  cusfs_setup_dump_file(void)
{
    int rc = 0;
    char *env_user = getenv("USER");
    char *dump_filename = getenv("CFLSH_USFS_DUMP");
    char *log_pid  = getenv("CFLSH_USFS_DUMP_PID");
    char filename[PATH_MAX];



    
    if (cusfs_dumpfp) {

	/*
	 * If dump file pointer is setup,
	 * then do not set it up again.
	 */

	return rc;

    }


    if (dump_filename == NULL)
    {
        sprintf(filename, "/tmp/%s.cflsh_usfs_dump", env_user);
        dump_filename = filename;
    }


    if (log_pid) {
	
	/*
	 * Use different filename for each process
	 */

	sprintf(filename,"%s.%d",dump_filename,getpid());


    }



    if ((cusfs_dumpfp = fopen(dump_filename, "a")) == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Failed to open dump_filename file %s",dump_filename);

	cusfs_dumpfp = NULL;
	rc = -1;
    }


    return rc;
}

/*
 * NAME: cusfs_dump_debug_data
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

void  cusfs_dump_debug_data(const char *reason,const char *reason_filename,const char *reason_function,
			  int reason_line_num, const char *reason_date)
{
    int i;
    cflsh_usfs_t *cufs;


    char timebuf[TIMELEN+1];
#if !defined(__64BIT__) && defined(_AIX)
    time64_t cur_time;
#else
    time_t cur_time;
#endif /* not 32-bit AIX */




    if (cusfs_dumpfp == NULL) {

	return;
    }
    
    /*
     * Print header
     */
    fprintf(cusfs_dumpfp,"---------------------------------------------------------------------------\n");
    fprintf(cusfs_dumpfp,"Build date for %s is %s at %s\n",__FILE__,__DATE__,__TIME__);

    fflush(cusfs_dumpfp);

#if !defined(__64BIT__) && defined(_AIX)
    cur_time = cflsh_usfs_time64(NULL);
    fprintf(cusfs_dumpfp,"Dump occurred at %s\n",ctime64_r(&cur_time,timebuf));
#else
    cur_time = time(NULL);
    fprintf(cusfs_dumpfp,"Dump occurred at %s\n",ctime_r(&cur_time,timebuf));
#endif /* not 32-bit AIX */

    fflush(cusfs_dumpfp);
    fprintf(cusfs_dumpfp,"PID = 0x%"PRIx64", dump sequence number =  0x%x\n",
	    (uint64_t)cusfs_global.caller_pid, dump_sequence_num);
    fprintf(cusfs_dumpfp,"dump reason %s, filename = %s, function = %s, line_number = %d, date = %s\n",
	    reason,reason_filename,reason_function,reason_line_num,reason_date);
    fflush(cusfs_dumpfp);
    fprintf(cusfs_dumpfp,"---------------------------------------------------------------------------\n");

    fflush(cusfs_dumpfp);

    fetch_and_add(&(dump_sequence_num),+1);


#ifdef BLOCK_FILEMODE_ENABLED
     fprintf(cusfs_dumpfp,"FILEMODE\n");
#endif /* BLOCK_FILEMODE_ENABLED */ 
    
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    fprintf(cusfs_dumpfp,"Little Endian\n");
#else
    fprintf(cusfs_dumpfp,"Big Endian\n");
#endif    
#if !defined(__64BIT__) && defined(_AIX)
    fprintf(cusfs_dumpfp,"32-bit app support\n");
#else
    fprintf(cusfs_dumpfp,"64-bit app support\n");

#endif

    fprintf(cusfs_dumpfp,"\n\n");

    fprintf(cusfs_dumpfp,"cusfs_log_verbosity         0x%x\n\n",cusfs_log_verbosity);

    fprintf(cusfs_dumpfp,"cusfs_dump_level            0x%x\n\n",cusfs_dump_level);

    fprintf(cusfs_dumpfp,"cusfs_notify_log_level      0x%x\n\n",cusfs_notify_log_level);

    fprintf(cusfs_dumpfp,"cusfs_log_lock (addr)       %p\n\n",&cusfs_log_lock);



    fprintf(cusfs_dumpfp,"cufs     = %p\n\n",&cusfs_global);

    fflush(cusfs_dumpfp);

    fprintf(cusfs_dumpfp,"    global_lock.file    = 0x%x\n",cusfs_global.global_lock.file); 
    fprintf(cusfs_dumpfp,"    global_lock.fname   = %s\n",cusfs_global.global_lock.filename); 
    fprintf(cusfs_dumpfp,"    global_lock.line    = %d\n",cusfs_global.global_lock.line); 
    fprintf(cusfs_dumpfp,"    global_lock.thread &= %p\n",&(cusfs_global.global_lock.thread)); 
    fprintf(cusfs_dumpfp,"    global_lock.thread  = 0x%x\n",(uint32_t)(cusfs_global.global_lock.thread)); 
    fprintf(cusfs_dumpfp,"    flags               = 0x%x\n",cusfs_global.flags);
    fprintf(cusfs_dumpfp,"    hash[]              = %p\n\n",&cusfs_global.hash[0]);


    fprintf(cusfs_dumpfp,"    eyec                = 0x%x",cusfs_global.eyec);
    if (!CFLSH_EYECATCH_GLOBAL(&cusfs_global)) {
	fprintf(cusfs_dumpfp," (Valid)\n");
    } else {
	fprintf(cusfs_dumpfp," (Invalid !!)\n");

	fflush(cusfs_dumpfp);

	/*
	 * If global cusfs_global is corrupted then return
	 * now. Don't attempt to traverse other
	 * data structures here, because this may
	 * result in a segmentation fault.
	 */
	return;
    }

    fflush(cusfs_dumpfp);

    for (i=0;i<MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];

	while (cufs) {

	    fprintf(cusfs_dumpfp,"\n    cufs(%d)           = %p\n",i,cufs);

	    /*
	     * Do some basic tests to see if this cufs address appears
	     * to be valid. If not, then exit from this while loop,
	     * since we can not trust the next pointer and move
	     * to the next has bucket.
	     */

	    if ( (ulong)cufs & CUSFS_BAD_ADDR_MASK ) {

		fprintf(cusfs_dumpfp,"        BAD CUFS ADDR!!\n");

		break;
			
	    }

	    fprintf(cusfs_dumpfp,"        eyec            = 0x%x",cufs->eyec);

	    fflush(cusfs_dumpfp);

	    fprintf(cusfs_dumpfp,"\n");

	    cufs = cufs->next;
	}

    }

    fflush(cusfs_dumpfp);



 
    fflush(cusfs_dumpfp);

    //?? loop thru all files


    fflush(cusfs_dumpfp);

    return;
}


/*
 * NAME:        cusfs_signal_dump_handler
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

void cusfs_signal_dump_handler(int signum, siginfo_t *siginfo, void *uctx)
{
    char reason[100];

    if (signum == SIGSEGV) {

	CUSFS_TRACE_LOG_FILE(1,"si_code = %d, si_addr = 0x%p",
			    siginfo->si_code,siginfo->si_addr);
	
	sprintf(reason,"SIGSEGV: si_code = %d si_addr = %p",siginfo->si_code,siginfo->si_addr);


    } else {
	
	sprintf(reason,"Signal = %d",signum);

    }

    cusfs_dump_debug_data(reason,__FILE__,__FUNCTION__,__LINE__,__DATE__);


    
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
 * NAME:        cusfs_setup_sigsev_dump
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

int cusfs_setup_sigsev_dump(void)
{
    int rc = 0;
    struct sigaction action, oaction;



    if (cusfs_setup_dump_file()){

	return -1;

    }


    bzero((void *)&action,sizeof(action));

    bzero((void *)&oaction,sizeof(oaction));

    action.sa_sigaction = cusfs_signal_dump_handler;
    action.sa_flags = SA_SIGINFO;


    if (sigaction(SIGSEGV, &action,&oaction)) {

        CUSFS_TRACE_LOG_FILE(1,"Failed to set up SIGSEGV handler with errno = %d\n",
			    errno);

	return -1;
    }


    return rc;
}


/*
 * NAME:        cusfs_setup_sigusr1_dump
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

int cusfs_setup_sigusr1_dump(void)
{
    int rc = 0;
    struct sigaction action, oaction;



    if (cusfs_setup_dump_file()){

	return -1;

    }


    bzero((void *)&action,sizeof(action));

    bzero((void *)&oaction,sizeof(oaction));

    action.sa_sigaction = cusfs_signal_dump_handler;
    action.sa_flags = SA_SIGINFO;


    if (sigaction(SIGUSR1, &action,&oaction)) {

        CUSFS_TRACE_LOG_FILE(1,"Failed to set up SIGUSR handler with errno = %d\n",
			    errno);

	return -1;
    }


    return rc;
}



/*
 * NAME:        cusfs_get_os_type
 *
 * FUNCTION:    Get Operating System type
 *             
 *             
 *
 * RETURNS:
 *              
 *             os_type
 *              
 */

cflsh_usfs_os_type_t cusfs_get_os_type(void)
{
 
#ifdef _AIX

    return CFLSH_OS_TYPE_AIX;

#else

    return CFLSH_OS_TYPE_LINUX;

#endif    

}

#ifdef _AIX
/* ----------------------------------------------------------------------------
 *
 * NAME: cusfs_get_program_name
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

char *cusfs_get_program_name(pid_t pid) 
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

	CUSFS_TRACE_LOG_FILE(1,"Failed to allocate process list of size = %d,with errno = %d",
			    process_list_size,errno);

	return NULL;
    }


    do {


	bzero(process_list,process_list_size);

	num_process = getprocs64(process_list,sizeof(*process_list),
			       NULL,0,&process_index,MAX_FETCH_PROCS);


	if (num_process == 0) {

	    
	    CUSFS_TRACE_LOG_FILE(5,"No processes returned from getprocs64. last index = %d",
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

	    CUSFS_TRACE_LOG_FILE(5,"No more processes left to fetch. last index = %d",
				process_index);
	    break;
	}


    } while (num_process);


    
#endif /* 64-bit */

    CUSFS_TRACE_LOG_FILE(5,"Our process name = %s",process_name);


    return process_name;
}

#endif /* _AIX */


/*
 * NAME:        cusfs_get_fs_block_size
 *
 * FUNCTION:    This routine determines the data block
 *              size used by the filesystem. This is the
 *              smallest data granularity allowed.
 *
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     -1 - error, otherwise success.
 *              
 *              
 */
int cusfs_get_fs_block_size(cflsh_usfs_t *cufs, int flags)
{


    /*
     * If each bit in a 4K page represents a block,
     * then such a page can represent 32K data
     * blocks (8 * 4K = 32K) of the associated 
     * filesystem block size.
     */
      

    /*
     * ?? TODO: Maybe this routine should also take into account
     *          max transfer size too in the future, since that
     *          reflects how easy it is to read the whole free block
     *          table into host memory..
     */


    if (cufs->num_blocks < 0x50) {

	/*
	 * This disk is too small to
	 * fit our metadata an have any
	 * limited notion of data blocks.
	 */

	CUSFS_TRACE_LOG_FILE(1,"This disk %s is too small for an FS, only 0x%x blocks total",
			    cufs->device_name,cufs->num_blocks);
	errno = EINVAL;
	return -1;

    } else if (cufs->num_blocks <= 0x20000) {


	/*
	 * The largest disk size for this range
	 * will require 4 4K disk blocks  
	 * for the free block table with a
	 * fs block size of CFLSH_USFS_BLOCK_SIZE_MIN
	 * (4K). This supports up to 128K 4K disk blocks
	 * (or 512 MB).
	 *
	 * NOTE: A disk max transfer size of 16K would
	 *       allow the whole table to be read
	 *       with one read.
	 */

	cufs->fs_block_size = CFLSH_USFS_BLOCK_SIZE_MIN;

	
    } else if (cufs->num_blocks <= 0x200000) {



	/*
	 * The largest disk size for this range
	 * will require 16 16K disk blocks  
	 * for the free block table. This
	 * supportes up 512K 16K data blocks,
	 * (or 8GB).
	 *
	 * NOTE: A disk max transfer size of 64K would
	 *       allow the whole table to be read
	 *       with one read.
	 */

	cufs->fs_block_size = 0x4000;

    } else if (cufs->num_blocks <= 0x2000000) {



	/*
	 * The largest disk size for this range
	 * will require 64 64K disk blocks to 
	 * for the free block table.
	 * (or 128GB).
	 *
	 * NOTE: A disk max transfer size of 256K would
	 *       allow the whole table to be read
	 *       with one read.
	 */

	cufs->fs_block_size = 0x10000;

	
    } else if (cufs->num_blocks <= 0x20000000LL) {


	/*
	 * The largest disk size for this range
	 * will require 256 256K disk blocks to 
	 * for the free block table.
	 * (or 2TB).
	 *
	 * NOTE: A disk max transfer size of 1 MB would
	 *       allow the whole table to be read
	 *       with one read.
	 */

	cufs->fs_block_size = 0x40000;

	
    } else if (cufs->num_blocks <= 0x2000000000LL) {



	/*
	 * The largest disk size for this range
	 * will require 1024 1MB disk blocks to 
	 * for the free block table.
	 * (or 32TB).
	 *
	 * NOTE: A disk max transfer size of 4 MB would
	 *       allow the whole table to be read
	 *       with one read.
	 */

	cufs->fs_block_size = 0x100000LL;

    } else {

	/*
	 * For disks larger than 32TB, we will use
	 * 16 MB block sizes.
	 */

	cufs->fs_block_size = 0x1000000LL;

    }

    if (cufs->fs_block_size % cufs->disk_block_size) {


	CUSFS_TRACE_LOG_FILE(1,"The fs_block_size 0x%x is not a multiple of disk_block_size 0x%x for %",
			    cufs->fs_block_size,cufs->disk_block_size,cufs->device_name);
	return -1;
    }


    return cufs->fs_block_size;
}


    

/*
 * NAME:        cusfs_get_inode_table_size
 *
 * FUNCTION:    This routine determines the size of the
 *              inode table used by the filesystem.
 *
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     0 - success, otherwise error.
 *              
 *              
 */
int cusfs_get_inode_table_size(cflsh_usfs_t *cufs, int flags)
{
    int rc = 0;


 
    /*
     * ?? TODO: Maybe this routine should also take into account
     *          the size of the disk and the 
     *          max transfer size too in the future, since that
     *          reflects how easy it is to read the whole inode 
     *          table into host memory..
     */
     
    /*
     * NOTES:
     *
     * 8 inode entries fit into a 4K block. Thus if the max transfer
     * size is 16MB (the maximum supported, this means that 
     * 32K inodes could be read into host memory at time.
     *
     * 
     */
       
    
    if (cufs->num_blocks < 0x50) {

	/*
	 * This disk is too small to
	 * fit our metadata an have any
	 * limited notion of inodes
	 */

	CUSFS_TRACE_LOG_FILE(1,"This disk %s is too small for an FS, only 0x%x blocks total",
			    cufs->device_name,cufs->num_blocks);
	errno = EINVAL;
	return -1;

    } else if (cufs->num_blocks <= 0x20000) {

	/*
	 * If the disk soze is no
	 * greater than 512 MB, then limit
	 * the size of the inode table to
	 * 512K, which means only 1K inodes
	 * supported.
	 */
       
	cufs->superblock.num_inodes = 0x400;

    } else if (cufs->num_blocks <= 0x2000000) {



	/*
	 * If the disk size is no greater 
	 * 128 GB, then limit the size of the
	 * inode table to 16 MB, which means
	 * only 32K inodes supported.
	 */


	cufs->superblock.num_inodes = 0x8000;

    } else {

	/*
	 * For disks larger than 128 GB,
	 * limit the size of the inode table
	 * to 64MB, which means only
	 * 128K inodes supported.
	 */

	cufs->superblock.num_inodes = 0x20000;
	
    }
	
    cufs->num_inode_data_blocks  = (cufs->superblock.num_inodes * sizeof(cflsh_usfs_inode_t))/cufs->disk_block_size;


    /*
     * If we are accessing an existing filesystem, we need to ensure our calculations on
     * on the size of inode table, free table etc, match those in the superblock.
     */

    return rc;
}

/*
 * NAME:        cusfs_get_cufs
 *
 * FUNCTION:    This routine gets a CAPI flash Userspace 
 *              filesystem object.
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              Pointer to cufs
 *              
 */
cflsh_usfs_t *cusfs_get_cufs(char *device_name, int flags)
{
    cflsh_usfs_t *cufs= NULL;
    cflsh_usfs_t *tmp_cufs;
    size_t num_blocks;
    chunk_ext_arg_t ext = (chunk_ext_arg_t)NULL;
    int i,j;                                           /* General counters */

#define CFLSH_NUM_FREE_FAIL_CMDS 10

    if (device_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Null device_name");
	return NULL;
    }
#ifdef _AIX
    cflsh_usfs_init();
#endif /* AIX */

    /*
     * Align on 4K boundary, so that we can use
     * the low order bits for eyecatcher ir hashing
     * if we decide to pass back a modified pointer
     * to the user. Currently we are not doing this,
     * but depending on the efficiency of the hash table
     * we may need to in the future.
     */

    if ( posix_memalign((void *)&cufs,4096,
                        (sizeof(*cufs)))) {

                    
        CUSFS_TRACE_LOG_FILE(1,"Failed posix_memalign for cufs of %s, errno= %d",
			    device_name,errno);
        

        return NULL;

    }
    
    bzero(cufs,sizeof(*cufs));


    cufs->max_requests = CFLSH_USFS_DISK_MAX_CMDS;

    cufs->chunk_id = cblk_open(device_name,cufs->max_requests,O_RDWR,ext,0);

    if (cufs->chunk_id == NULL_CHUNK_ID) {

	      
        CUSFS_TRACE_LOG_FILE(1,"cblk_open failed errno= %d, num_requests = %d, os_type = %d",
			    errno,cufs->max_requests,cusfs_global.os_type);

	free(cufs);

	return NULL;
    }


    strcpy(cufs->device_name,device_name);
    
    if (cblk_get_lun_size(cufs->chunk_id,&num_blocks,0)) {


	CUSFS_TRACE_LOG_FILE(1,"failed to get lun size with errno = %d",
                            errno);

	cblk_close(cufs->chunk_id,0);
	free(cufs);

	return NULL;

    }

    cufs->num_blocks = num_blocks;

    CUSFS_TRACE_LOG_FILE(5,"capacity = 0x%llx",cufs->num_blocks);

    if (cblk_get_stats(cufs->chunk_id,&(cufs->disk_stats),0)) {


	CUSFS_TRACE_LOG_FILE(1,"failed to get stats with errno = %d",
                            errno);

	cblk_close(cufs->chunk_id,0);
	free(cufs);

	return NULL;

    }

    cufs->max_xfer_size = cufs->disk_stats.max_transfer_size;


    CUSFS_TRACE_LOG_FILE(5,"disk block size reported = 0x%x, max_xfer_size = 0x%x,num_paths = %d ",
			cufs->disk_stats.block_size,
			cufs->max_xfer_size,
			cufs->disk_stats.num_paths);


    cufs->disk_block_size = CFLSH_USFS_DEVICE_BLOCK_SIZE;

		   
    /*
     * Set buf size to the initial max_xfer_size
     */

    cufs->buf_size = cufs->max_xfer_size * cufs->disk_block_size;

    if (cufs->buf_size < cufs->disk_block_size) {
      
        CUSFS_TRACE_LOG_FILE(1,"max transfer size = 0x%x is less than disk block size = 0x%x",
			    cufs->buf_size,cufs->disk_block_size);
        
	cblk_close(cufs->chunk_id,0);
	free(cufs);

        return NULL;

    }


    if ( posix_memalign((void *)&(cufs->buf),4096,
                        cufs->buf_size)) {

                    
        CUSFS_TRACE_LOG_FILE(1,"Failed posix_memalign for buf failed, errno= %d, size = 0x%x",
			    errno,cufs->buf_size);
        
	cblk_close(cufs->chunk_id,0);
	free(cufs);

        return NULL;
    }

 
    
    cufs->free_blk_tbl_buf_size = cufs->buf_size;

    if ( posix_memalign((void *)&(cufs->free_blk_tbl_buf),4096,
                        cufs->free_blk_tbl_buf_size)) {

                    
        CUSFS_TRACE_LOG_FILE(1,"Failed posix_memalign for free_blk_tbl_buf failed, errno= %d, size = 0x%x",
			    errno,cufs->free_blk_tbl_buf_size);
        

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs);

        return NULL;
    }

    cufs->inode_tbl_buf_size = cufs->buf_size;

    if ( posix_memalign((void *)&(cufs->inode_tbl_buf),4096,
                        cufs->inode_tbl_buf_size)) {

                    
        CUSFS_TRACE_LOG_FILE(1,"Failed posix_memalign for inode_tbl_buf failed, errno= %d, size = 0x%x",
			    errno,cufs->inode_tbl_buf_size);
        

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs);

        return NULL;
    }
    

    /*
     * ?? TODO maybe we need to only get_fs_block_size and
     *    inode table size, if we are explicilty creating
     *    a filesystem. Otherwise we should determine this
     *    information (or maybe validate) it from the superblock.
     */

    if (cusfs_get_fs_block_size(cufs,0) == -1) {


	CUSFS_TRACE_LOG_FILE(1,"failed to get fs block size for disk %s",
                            device_name);

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);
	free(cufs);

	return NULL;
	
    }

    if (cusfs_get_inode_table_size(cufs,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to get inode table size for disk %s",
                            device_name);

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);
	free(cufs);

	return NULL;

    }
   
    CUSFS_LOCK_INIT((cufs->lock));

    CUSFS_LOCK_INIT((cufs->free_table_lock));

    CUSFS_LOCK_INIT((cufs->inode_table_lock));

    CUSFS_LOCK_INIT((cufs->journal_lock));

    cusfs_global.num_active_cufs++;
    cusfs_global.num_max_active_cufs = MAX(cusfs_global.num_active_cufs,cusfs_global.num_max_active_cufs);


    cufs->index = cusfs_global.next_cusfs_id++;

    cufs->eyec = CFLSH_USFS_EYEC_FS;


    cufs->async_pool_num_cmds = cufs->max_requests;
    cufs->async_pool_size = sizeof(cflsh_usfs_aio_entry_t) * cufs->async_pool_num_cmds;
    cufs->async_pool = malloc(cufs->async_pool_size);

    if (cufs->async_pool == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to malloc async_pool disk %s",
			     device_name);

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);
	free(cufs);

	return NULL;
    }

    for (i= 0; i < cufs->async_pool_num_cmds; i++) {


	cufs->async_pool[i].eyec = CFLSH_USFS_EYEC_AIO;
	cufs->async_pool[i].index = i;
	cufs->async_pool[i].local_buffer_len = cufs->fs_block_size;
	cufs->async_pool[i].local_buffer = malloc(cufs->async_pool[i].local_buffer_len);
	if (cufs->async_pool[i].local_buffer == NULL) {
	    

	    /*
	     * Just try to continue with no command pool.
	     */

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc an async_pool command %d async_pool_num_cmds %d disk %s",
                                 i,cufs->async_pool_num_cmds,device_name);

				 
	    if (i > CFLSH_NUM_FREE_FAIL_CMDS) {

		/*
		 * Only free up the previous async command and try to
		 * continue to run.
		 */

		
		for (j=i-1; j >= i-CFLSH_NUM_FREE_FAIL_CMDS; j--) {

		    CUSFS_DQ_NODE(cufs->head_free,cufs->tail_free, &(cufs->async_pool[j]),free_prev,free_next);

		    cufs->async_pool[j].eyec = 0;

		    free(cufs->async_pool[j].local_buffer);
		}

		cufs->async_pool_num_cmds = i-CFLSH_NUM_FREE_FAIL_CMDS;
		
	    } else {

		for (j=0; j < i; j++) {

		    CUSFS_DQ_NODE(cufs->head_free,cufs->tail_free, &(cufs->async_pool[j]),free_prev,free_next);

		    cufs->async_pool[j].eyec = 0;

		    free(cufs->async_pool[j].local_buffer);
		}

		cufs->async_pool_num_cmds = 0;
		
	    }

	    break;
				 
	} else {
	    CUSFS_Q_NODE_TAIL(cufs->head_free,cufs->tail_free, &(cufs->async_pool[i]),free_prev,free_next);
	}
    }

    CUSFS_TRACE_LOG_FILE(9,"cufs->async_pool_num_cmds = 0x%x, buffer_size = 0x%llx",
			 cufs->async_pool_num_cmds,cufs->fs_block_size);

    if (cusfs_start_aio_thread_for_cufs(cufs,0)) {

	CUSFS_TRACE_LOG_FILE(1,"failed to start background thread for disk %s",
			     device_name);

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);
	free(cufs);

	return NULL;
    }


    if (cflsh_usfs_master_register(device_name,&(cufs->master_handle))) {

	CUSFS_TRACE_LOG_FILE(1,"failed to register with masterfor disk %s",
			     device_name);

	cblk_close(cufs->chunk_id,0);
	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);
	free(cufs);

	return NULL;
	
    }

    if (cusfs_global.hash[cufs->index & CUSFS_HASH_MASK] == NULL) {

	cusfs_global.hash[cufs->index & CUSFS_HASH_MASK] = cufs;
    } else {

	tmp_cufs = cusfs_global.hash[cufs->index & CUSFS_HASH_MASK];

	while (tmp_cufs) {

	    if ((ulong)tmp_cufs & CUSFS_BAD_ADDR_MASK ) {

		/*
		 * Cufs addresses are allocated 
		 * on certain alignment. If this 
		 * potential cufs address does not 
		 * have the correct alignment then fail
		 * this request.
		 */

		cusfs_global.num_bad_cusfs_ids++;

		CUSFS_TRACE_LOG_FILE(1,"Corrupted cufs address = 0x%p, hash[] = 0x%p index = 0x%x", 
				    tmp_cufs, cusfs_global.hash[cufs->index & CUSFS_HASH_MASK],
				    (cufs->index & CUSFS_HASH_MASK));

		CUSFS_LIVE_DUMP_THRESHOLD(5,"0x200");

		cufs->eyec = 0;
		free(cufs->buf);
		free(cufs->free_blk_tbl_buf);
		free(cufs->inode_tbl_buf);
		
		if (cflsh_usfs_master_unregister(cufs->master_handle)) {
		    
		    CUSFS_TRACE_LOG_FILE(1,"failed to unregister with master for disk %s with errno %d",
					 cufs->device_name,errno);
		}

		free(cufs);

		errno = EFAULT;
		return NULL;
	    }


	    if (tmp_cufs->next == NULL) {

		tmp_cufs->next = cufs;

		cufs->prev = tmp_cufs;
		break;
	    }

	    tmp_cufs = tmp_cufs->next;

	} /* while */

    }
                



    return cufs;


}

/*
 * NAME:        cusfs_release_cufs
 *
 * FUNCTION:    This routine frees up all resources
 *              for a filesystem object.
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     None
 *              
 */
void cusfs_release_cufs(cflsh_usfs_t *cufs, int flags)
{
    int rc;
    int i;


    if (cufs != NULL) {

	cusfs_display_stats(cufs,3);

	for (i=0; i < cufs->async_pool_num_cmds; i++) {
	    
	    free(cufs->async_pool[i].local_buffer);
	}

	cusfs_stop_aio_thread_for_cufs(cufs,0);

	if (cflsh_usfs_master_unregister(cufs->master_handle)) {
		    
	    CUSFS_TRACE_LOG_FILE(1,"failed to unregister with master for disk %s with errno %d",
				 cufs->device_name, errno);
	}

	free(cufs->buf);
	free(cufs->free_blk_tbl_buf);
	free(cufs->inode_tbl_buf);

	rc = cblk_close(cufs->chunk_id,0);

	if (rc) {

	    CUSFS_TRACE_LOG_FILE(1,"cblk_close failed errno= %d, device_name = %s, os_type = %d",
				errno,cufs->device_name,cusfs_global.os_type);
	}
	
        if (cusfs_global.num_active_cufs > 0) {
            cusfs_global.num_active_cufs--;
        }
        cufs->eyec = 0;

        bzero(cufs->device_name,PATH_MAX);
	/*
         * Remove cufs from hash list
         */

        if (((ulong)cufs->next & CUSFS_BAD_ADDR_MASK ) ||
            ((ulong)cufs->prev & CUSFS_BAD_ADDR_MASK )) {

            /*
             * Cufs addresses are allocated 
             * on certain alignment. If these
             * potential cufs addresses do not 
             * have the correct alignment then 
             * print an error to the trace log.
             */

            cusfs_global.num_bad_cusfs_ids++;
            /*
             * Try continue in this case.
             */


            CUSFS_TRACE_LOG_FILE(1,"Corrupted cufs next address = 0x%p, prev address = 0x%p, hash[] = 0x%p", 
                                cufs->next, cufs->prev, cusfs_global.hash[cufs->index & CUSFS_HASH_MASK]);

        }

        if (cufs->prev) {
            cufs->prev->next = cufs->next;

        } else {

            cusfs_global.hash[cufs->index & CUSFS_HASH_MASK] = cufs->next;
        }

        if (cufs->next) {
            cufs->next->prev = cufs->prev;
        }

    }


    return;
}


/*
 * NAME:        cusfs_alloc_file
 *
 * FUNCTION:    This routine allocates a file (data object)
 *              filesystem object.
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              Pointer to file
 *              
 */
cflsh_usfs_data_obj_t *cusfs_alloc_file(cflsh_usfs_t *cufs, char *filename, int flags)
{
    cflsh_usfs_data_obj_t *file = NULL;



    if (cufs == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Null cufs");
	errno = EINVAL;
	return NULL;
    }
	
    if (filename == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Null device_name");
	errno = EINVAL;
	return NULL;
    }

    /*
     * Align on 4K boundary, so that we can use
     * the low order bits for eyecatcher ir hashing
     * if we decide to pass back a modified pointer
     * to the user. Currently we are not doing this,
     * but depending on the efficiency of the hash table
     * we may need to in the future.
     */

    if ( posix_memalign((void *)&file,4096,
                        (sizeof(*file)))) {

                    
        CUSFS_TRACE_LOG_FILE(1,"Failed posix_memalign for file of %s, errno= %d",
			    filename,errno);
        

        return NULL;

    }
    
    bzero(file,sizeof(*file));

    if (flags & CFLSH_USFS_ALLOC_READ_AHEAD) {
	file->seek_ahead.local_buffer_len = cufs->fs_block_size;
	file->seek_ahead.local_buffer = malloc(file->seek_ahead.local_buffer_len);

	if (file->seek_ahead.local_buffer == NULL) {

	    CUSFS_TRACE_LOG_FILE(1,"Failed to malloc read ahead buffer for filename %s",
			     filename);
	    /*
	     * Continue, but provide no read ahead
	     */
	    file->seek_ahead.local_buffer_len = 0;
	    
	}
    }


    strcpy(file->filename,filename);

    file->eyec = CFLSH_USFS_EYEC_FL;
    
    file->fs = cufs;

    file->index = cusfs_global.next_file_id++;

    CUSFS_LOCK_INIT((file->lock));

    return file;

}



/*
 * NAME:        cusfs_free_file
 *
 * FUNCTION:    This routine frees a file (data object)
 *              filesystem object from memory.
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:
 *              Pointer to file
 *              
 */
void cusfs_free_file(cflsh_usfs_data_obj_t *file)
{
    if (file == NULL) {

	return;
    }

    if (file->data_buf) {
	free(file->data_buf);
    }

    if (file->seek_ahead.local_buffer) {
	free(file->seek_ahead.local_buffer);
    }
    free(file);
    return;
}


/*
 * NAME:        cusfs_get_aio
 *
 * FUNCTION:    Find a free async I/O entry for the specified filesystem
 *              
 *
 * NOTE:        This routine assumes the caller
 *              has the filesystem lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_get_aio(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, cflsh_usfs_aio_entry_t **aio,int flags)
{
    cflsh_usfs_aio_entry_t *aiop;
    int pthread_rc = 0;


    if (cufs == NULL) {
	CUSFS_TRACE_LOG_FILE(1,"No valid cufs");

	errno = EINVAL;
        return -1;
    }

    if (file == NULL) {
	CUSFS_TRACE_LOG_FILE(1,"No valid file for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;
    }

    if (CFLSH_EYECATCH_FS(cufs)) {
	CUSFS_TRACE_LOG_FILE(1,"Invalid file for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;
    }

    aiop = cufs->head_free;

    if (aiop == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"No free aio for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
			     cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);


	/*
	 * We're out of aio commands, signal back ground to process them in case
	 * it stopped.
	 */

	pthread_rc = pthread_cond_signal(&(cufs->thread_event));
	
	if (pthread_rc) {
	    
	     CUSFS_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d",
				pthread_rc,errno);
	}

	errno = EBUSY;
        return -1;
    }

    if (CFLSH_EYECATCH_AIO(aiop)) {
	CUSFS_TRACE_LOG_FILE(1,"Invalid aiop for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;

    }

    /*
     * Remove command from free list
     */

    CUSFS_DQ_NODE(cufs->head_free,cufs->tail_free, aiop,free_prev,free_next);

    aiop->flags = 0;

    aiop->file = file;

    /*
     * place command on active list for both filesystem and file.
     */

    CUSFS_Q_NODE_TAIL(cufs->head_act,cufs->tail_act, aiop,act_prev,act_next);

    CUSFS_Q_NODE_TAIL(file->head_act,file->tail_act, aiop,file_prev,file_next);

    CUSFS_TRACE_LOG_FILE(9,"Got aiop = %p, for disk %s",aiop,cufs->device_name);


    *aio = aiop;

    return 0;
}

/*
 * NAME:        cusfs_free_aio
 *
 * FUNCTION:    Free async I/O entry for the specified filesystem
 *              
 *
 * NOTE:        This routine assumes the caller
 *              has the file lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_free_aio(cflsh_usfs_t *cufs, cflsh_usfs_aio_entry_t *aio,int flags)
{
    cflsh_usfs_data_obj_t *file; 

    if (cufs == NULL) {
	CUSFS_TRACE_LOG_FILE(1,"No valid cufs");

	errno = EINVAL;
        return -1;
    }

    if (CFLSH_EYECATCH_FS(cufs)) {
	CUSFS_TRACE_LOG_FILE(1,"Invalid file for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;
    }

    if (CFLSH_EYECATCH_AIO(aio)) {
	CUSFS_TRACE_LOG_FILE(1,"Invalid aio for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;

    }

    if (aio->op == CFLSH_USFS_AIO_E_INVAL) {

	CUSFS_TRACE_LOG_FILE(1,"This aio has already been freed for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;

    }

    /*
     * Remove command from active list
     */
 
    
    CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);

  
    file = aio->file;

    if (file) {
    
	CUSFS_DQ_NODE(file->head_act,file->tail_act, aio,file_prev,file_next);
    
    } else {

	CUSFS_TRACE_LOG_FILE(1,"Freeing aio = %p, but file not set for disk %s,aio->op = 0x%x, aio->flags = 0x%x",
			     file->head_act, cufs->device_name,aio->op,aio->flags);	
    }

    CUSFS_TRACE_LOG_FILE(9,"Freeing aio = %p, for disk %s, for start_lba 0x%llx aio_offset = 0x%llx aio_nbytes = 0x%llx num_act_areads = 0x%x, num_act_awrites = 0x%x",
			     file->head_act, cufs->device_name,
			 aio->aio_offset,aio->aio_nbytes,
			 cufs->stats.num_act_areads,cufs->stats.num_act_awrites);

    if (aio->op == CFLSH_USFS_AIO_E_WRITE) {
	cufs->stats.num_act_awrites--;
    } else {

	cufs->stats.num_act_areads--;
    }
    aio->op = CFLSH_USFS_AIO_E_INVAL;
    aio->caller_buffer = NULL;
    aio->caller_buffer_len = 0;
    aio->local_buffer_len_used = 0;
    aio->tag = 0;
    aio->offset = 0;
    aio->start_lba = 0;
    aio->aiocbp = NULL;
    
    bzero(&(aio->cblk_status),sizeof(aio->cblk_status));

    bzero(&(aio->aio_sigevent),sizeof(aio->aio_sigevent));
	  
    /*
     * Place command on free list
     */

    CUSFS_Q_NODE_TAIL(cufs->head_free,cufs->tail_free, aio,free_prev,free_next);

    return 0;
}


/*
 * NAME:        cusfs_issue_aio
 *
 * FUNCTION:    async I/O entry for this file
 *              
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_issue_aio(cflsh_usfs_t *cufs,  cflsh_usfs_data_obj_t *file, uint64_t start_lba, struct aiocb64 *aiocbp, 
		    void *buffer, size_t nbytes, int flags)
{

    cflsh_usfs_aio_entry_t *aio = NULL;
#ifdef _CFLSH_USFS_ARESULT
    int arw_flags = (CBLK_ARW_USER_TAG_FLAG|CBLK_ARW_WAIT_CMD_FLAGS);
#else
    int arw_flags = (CBLK_ARW_USER_STATUS_FLAG|CBLK_ARW_WAIT_CMD_FLAGS);
#endif
    void *buf;
    size_t nblocks;
    int pthread_rc = 0;
    int rc;
    int retry;
    
    if (cufs == NULL) {
	CUSFS_TRACE_LOG_FILE(1,"No valid cufs");

	errno = EINVAL;
        return -1;
    }

    if (file == NULL) {
	CUSFS_TRACE_LOG_FILE(1,"No valid file for disk %s",cufs->device_name);

	errno = EINVAL;
        return -1;
    }

    if (cufs->async_pool_num_cmds == 0) {

	CUSFS_TRACE_LOG_FILE(1,"No async commands were allocated for disk %s",cufs->device_name);

	errno = ENOMEM;
        return -1;

    }
    
    
    
    if (aiocbp) {
	
	retry = 1;
    } else {
	retry = 1000;
    }

    while (retry--) {
	
	if (cusfs_get_aio(cufs,file,&aio,0)) {
	    
	    if (retry == 0) {
		errno = EAGAIN;
	    
		return -1;
	    }
	} else {

	    break;
	}
	cusfs_check_active_aio_complete(cufs,0);
	CUSFS_UNLOCK(file->lock);
	CUSFS_UNLOCK(cufs->lock);
	usleep(1000);
	CUSFS_LOCK(cufs->lock);
	CUSFS_LOCK(file->lock);

    }


    nblocks = nbytes/cufs->disk_block_size;
    
    
    
    /*
     * If both buffer and aiocb are set,
     * then assume buffer is the intended use.
     * This can happen if an aiocb needs
     * to be broken into multiple requests.
     */
    
    if (aiocbp) {

	/*
	 * Since one aiocb can be broken
	 * into multiple requests, save off
	 * original aiocbp offset and size.
	 */
#ifdef _AIX
	aio->aiocbp = (struct aiocb64 *)aiocbp->aio_handle;
	if (aiocbp->aio_word1 & CUSFS_AIOCB32_FLG) {
	    aio->flags |= CFLSH_USFS_AIO_E_AIOCB;
	}  else {
	    aio->flags |= CFLSH_USFS_AIO_E_AIOCB64;  
	}
#else
	aio->aiocbp = (struct aiocb64 *)aiocbp->__next_prio;
	if (aiocbp->__glibc_reserved[0] & CUSFS_AIOCB32_FLG) {
	    aio->flags |= CFLSH_USFS_AIO_E_AIOCB;
	}  else {
	    aio->flags |= CFLSH_USFS_AIO_E_AIOCB64;  
	}
#endif  /* !_AIX */

	aio->aio_offset = aiocbp->aio_offset;
	aio->aio_nbytes = aiocbp->aio_nbytes;
	aio->aio_sigevent = aiocbp->aio_sigevent;

    }
    
    if (flags & CFLSH_USFS_RDWR_DIRECT) {
	
	/*
	 * The caller is requesting that we use
	 * the caller's buffer and allow direct
	 * DMA to/from it.
	 */

	aio->caller_buffer = (void *)aiocbp->aio_buf;
	aio->caller_buffer_len = aiocbp->aio_nbytes;
	
	buf = aio->caller_buffer;
	
	if (aiocbp->aio_offset % cufs->disk_block_size) {
	    CUSFS_TRACE_LOG_FILE(1,"Request is not block aligned offset = 0x%llx for file %s for disk %s",
				 aiocbp->aio_offset,file->filename,cufs->device_name);
	    
	    cusfs_free_aio(cufs,aio,0);
	
	    errno = EINVAL;
	    return -1;
	    
	}
    } else {
	aio->flags |= CFLSH_USFS_AIO_E_LBUF;
	buf = aio->local_buffer; 

	if (nbytes > aio->local_buffer_len) {
	    CUSFS_TRACE_LOG_FILE(1,"Request transfer size of 0x%llx is larger than local_buffer_len of 0x%llx, for file %s for disk %s",
				 nbytes,aio->local_buffer_len,file->filename,cufs->device_name);
	    
	    cusfs_free_aio(cufs,aio,0);
	
	    errno = EINVAL;
	    return -1;
	}
	aio->local_buffer_len_used = nbytes;
	bcopy(buffer,aio->local_buffer,aio->local_buffer_len_used);
    }
    aio->start_lba = start_lba;
    
    

    aio->flags |= CFLSH_USFS_AIO_E_ISSUED;

    /*
     * Signal background thread to look for completion even
     * though we have not yet sent it. This is because it
     * will take a while to wake up and we currently have the
     * same lock. Thus it can not start to process until this 
     * thread releases the lock.
     */

    pthread_rc = pthread_cond_signal(&(cufs->thread_event));
	
    if (pthread_rc) {
	    
	CUSFS_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d",
			     pthread_rc,errno);
    }

    

    CUSFS_TRACE_LOG_FILE(9,"Issuing aio %p aio_offset = 0x%llx, start_lba 0x%llx with status 0x%x of on aio->aiocbp = %p",
			 aio,aio->aio_offset,aio->start_lba,aio->cblk_status.status,aio->aiocbp);

#ifdef _CFLSH_USFS_ARESULT

    aio->tag = aio->index;
#endif /* _CFLSH_USFS_ARESULT */

    if (flags & CFLSH_USFS_RDWR_WRITE) {
		
	aio->op = CFLSH_USFS_AIO_E_WRITE;
	rc = cblk_awrite(cufs->chunk_id,buf,aio->start_lba,nblocks,&(aio->tag),&(aio->cblk_status),arw_flags);
    } else {

	aio->op = CFLSH_USFS_AIO_E_READ;
	rc = cblk_aread(cufs->chunk_id,buf,aio->start_lba,nblocks,&(aio->tag),&(aio->cblk_status),arw_flags);

    }

    if (rc < 0) {

	CUSFS_TRACE_LOG_FILE(1,"Failed to issue aio %p with ,errno = %d",
			     aio,errno);


	if (flags & CFLSH_USFS_RDWR_WRITE) {
	
	    cufs->stats.num_error_awrites++;

	} else {

	    cufs->stats.num_error_areads++;

	}

	cusfs_free_aio(cufs,aio,0);
	
	

    } else {
	
	if (flags & CFLSH_USFS_RDWR_WRITE) {
	
	    cufs->stats.num_awrites++;
	    cufs->stats.num_act_awrites++;

	    cufs->stats.max_num_act_awrites = MAX(cufs->stats.max_num_act_awrites,cufs->stats.num_act_awrites);

	} else {

	    cufs->stats.num_areads++;
	    cufs->stats.num_act_areads++;

	    cufs->stats.max_num_act_areads = MAX(cufs->stats.max_num_act_areads,cufs->stats.num_act_areads);

	}

	rc = nblocks;
    }
    return rc;

}

/*
 * NAME:        cusfs_check_active_aio_complete
 *
 * FUNCTION:    Check if active async I/O is complete
 *              then process them accordingly.
 *              
 * NOTES:       This routine assumes the caller has the cufs->lock
 *              
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_check_active_aio_complete(cflsh_usfs_t *cufs,  int flags)
{
    int rc = 0;
    cflsh_usfs_aio_entry_t *aio;
    int pthread_rc = 0;
    pthread_t thread_id;
    pthread_attr_t *thread_attr = NULL;
    struct aiocb *aiocbp;
    struct aiocb64 *aiocbp64;
    int error_code;
    ssize_t ret_code;
    cflsh_usfs_data_obj_t *file;
#ifndef _CFLSH_USFS_ARESULT
    cflsh_usfs_aio_entry_t *next_aio;
#else
    int tag;
    uint64_t status;
#endif /* _CFLSH_USFS_ARESULT */


    aio = cufs->head_act;

    CUSFS_TRACE_LOG_FILE(9,"aio = %p, for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
			 aio, cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);
    if ((aio) &&
	(flags & CFLSH_USFS_PTHREAD_SIG)) {
	
	pthread_rc = pthread_cond_signal(&(cufs->thread_event));
        
	if (pthread_rc) {
            
	    CUSFS_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d",
				pthread_rc,errno);
	}
	
    }


    while (aio) {


	CUSFS_TRACE_LOG_FILE(6,"Found aio %p for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx",
			     aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset);


#ifndef _CFLSH_USFS_ARESULT



	next_aio = aio->act_next;

	if ((aio->flags & CFLSH_USFS_AIO_E_ISSUED) &&
	    ((aio->cblk_status.status != CBLK_ARW_STATUS_PENDING))) {

	    /*
	     * AIO completed with success/error. Lets process it
	     * now. First dequue it from active list.
	     */
	    
	    aio->flags &= ~CFLSH_USFS_AIO_E_ISSUED;




	    CUSFS_TRACE_LOG_FILE(6,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->flags = 0x%x",
				 aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->flags);


	    CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);
	    
	    if ((aio->flags & CFLSH_USFS_AIO_E_AIOCB) ||
		(aio->flags & CFLSH_USFS_AIO_E_AIOCB64)) {

		/*
		 * This aio is associated with a POSIX AIO
		 * request. So queue it to the completion
		 * queue.
		 */

		error_code = 0;
		ret_code = -1;

		switch(aio->cblk_status.status) {
		case CBLK_ARW_STATUS_PENDING: 
		    error_code =  EINPROGRESS;
		    ret_code = -1;
		    break;
		case CBLK_ARW_STATUS_SUCCESS:
		    
		    //?? This does not handle situation where one aiocb is split across multiple AIOs.
		    error_code = 0;
		    ret_code  = aio->aio_nbytes;

		    break;
		case CBLK_ARW_STATUS_INVALID:
		    
		    error_code = EINVAL;
		    ret_code = -1;
		    break;
		case CBLK_ARW_STATUS_FAIL:

		    error_code = aio->cblk_status.fail_errno;
		    ret_code = -1;
		    break;
		
		}


		if (aio->flags & CFLSH_USFS_AIO_E_AIOCB64) {
		    
		    aiocbp64 = aio->aiocbp;

		    
		    /*
		     * Set errno and return code
		     * in aiocb here.
		     */


		    CUSFS_TRACE_LOG_FILE(9,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->aiocbp = %p",
				 aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->aiocbp);


		    CUSFS_RET_AIOCB(aiocbp64,ret_code,error_code);

		} else {


		    aiocbp = (struct aiocb *)aio->aiocbp;

		    
		    /*
		     * Set errno and return code
		     * in aiocb here.
		     */


		    CUSFS_TRACE_LOG_FILE(9,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->aiocbp = %p",
				 aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->aiocbp);


		    CUSFS_RET_AIOCB(aiocbp,ret_code,error_code);

		}

		if (aio->aio_sigevent.sigev_notify == SIGEV_NONE) {
		
		    /*
		     * Do not send any signal.
		     */

		    CUSFS_TRACE_LOG_FILE(6,"d_queue aio %p for start_lba 0x%llx with status 0x%x of on file = %s",
					 aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);

		} else if (aio->aio_sigevent.sigev_notify == SIGEV_SIGNAL) {
		
		    /*
		     * Send signal to process, which is using this
		     * library, which is this process.
		     */


		    if (aio->aio_sigevent.sigev_signo) {
			rc = sigqueue(getpid(),aio->aio_sigevent.sigev_signo,
				      aio->aio_sigevent.sigev_value);

			if (rc) {

			    CUSFS_TRACE_LOG_FILE(1,"signal 0x%x failed sig_value = 0x%x, rc %d, errno = %d",
						 aio->aio_sigevent.sigev_signo,
						 aio->aio_sigevent.sigev_value,
						 rc,errno);

			    CUSFS_TRACE_LOG_FILE(1,"signal failed for aio %p completed for start_lba 0x%llx with status 0x%x of on file = %s",
						 aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);

			
			}

			rc = 0;

		    } else {

			    CUSFS_TRACE_LOG_FILE(1,"signal is 0 for filename %s",
						 aio->file->filename);
		    }


		} else if (aio->aio_sigevent.sigev_notify == SIGEV_THREAD) {
		
		    /*
		     * Send signal to thread, which is using this
		     * library, which is this process.
		     */

		    if (aio->aio_sigevent.sigev_notify_attributes) {

			thread_attr = (pthread_attr_t *)aio->aio_sigevent.sigev_notify_attributes;
		    } 

		    if (aio->aio_sigevent.sigev_notify_function) {
			pthread_rc = pthread_create(&(thread_id),thread_attr,
						    (void *(*)(void *))(aio->aio_sigevent.sigev_notify_function),
						    (void *)&(aio->aio_sigevent.sigev_value));

			
			if (pthread_rc) {
            
			    cufs->stats.num_failed_threads++;
            
			    CUSFS_TRACE_LOG_FILE(5,"pthread_create failed rc = %d,errno = %d, thread_attr = %p",
						 pthread_rc,errno, thread_attr);
			}

		    } else {

			
			CUSFS_TRACE_LOG_FILE(1,"Null sig_notify_function for filename %s",
					     aio->file->filename);
		    }

        
		}

	    } 
		
	    file = aio->file;

	    if (file) {
		CUSFS_LOCK(file->lock);
	    }

	    cusfs_free_aio(cufs,aio,0);

	    if (file) {
		CUSFS_UNLOCK(file->lock);
	    }

	}

	aio = next_aio;

    

#else

	CUSFS_UNLOCK(cufs->lock);

	ret_code = cblk_aresult(cufs->chunk_id,&tag,&status,
			  (CBLK_ARESULT_BLOCKING|CBLK_ARESULT_NEXT_TAG|CBLK_ARESULT_USER_TAG));

	error_code = errno;

	CUSFS_LOCK(cufs->lock);


	if (ret_code < 1) {
	    /*
	     * This should not happen
	     */

	    CUSFS_TRACE_LOG_FILE(1,"cblk_aresult failed rc = %d,errno = %d",
				 ret_code,errno);

	    return -1;

	}

	aio = &cufs->async_pool[tag];

	if ((aio == NULL) ||
	    !(aio->flags & CFLSH_USFS_AIO_E_ISSUED)) {

	    /*
	     * This should not happen
	     */

	    CUSFS_TRACE_LOG_FILE(1,"invalid aio command returned tag = %d, aio = %p",
				 tag,aio);

	    return -1;

	}

	/*
	 * AIO completed with success/error. Lets process it
	 * now. First dequue it from active list.
	 */
	
	aio->flags &= ~CFLSH_USFS_AIO_E_ISSUED;
	
	
	
	
	CUSFS_TRACE_LOG_FILE(6,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->flags = 0x%x",
			     aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->flags);
	
	
	CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);
	
	if ((aio->flags & CFLSH_USFS_AIO_E_AIOCB) ||
	    (aio->flags & CFLSH_USFS_AIO_E_AIOCB64)) {
	    
	    /*
	     * This aio is associated with a POSIX AIO
	     * request. So queue it to the completion
	     * queue.
	     */

	    if (aio->flags & CFLSH_USFS_AIO_E_AIOCB64) {
		
		aiocbp64 = aio->aiocbp;
		
		
		/*
		 * Set errno and return code
		 * in aiocb here.
		 */
		
		
		CUSFS_TRACE_LOG_FILE(9,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->aiocbp = %p",
				     aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->aiocbp);
		
		
		CUSFS_RET_AIOCB(aiocbp64,ret_code,error_code);
		
	    } else {
		
		
		aiocbp = (struct aiocb *)aio->aiocbp;
		
		
		/*
		 * Set errno and return code
		 * in aiocb here.
		 */
		
		
		CUSFS_TRACE_LOG_FILE(9,"aio %p completed for start_lba 0x%llx with status 0x%x for aio_offset = 0x%llx, aio->aiocbp = %p",
				     aio,aio->start_lba,aio->cblk_status.status,aio->aio_offset, aio->aiocbp);
		
		
		CUSFS_RET_AIOCB(aiocbp,ret_code,error_code);
		
	    }
	    
	    if (aio->aio_sigevent.sigev_notify == SIGEV_NONE) {
		
		/*
		 * Do not send any signal.
		 */
		
		CUSFS_TRACE_LOG_FILE(6,"d_queue aio %p for start_lba 0x%llx with status 0x%x of on file = %s",
				     aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);
		
	    } else if (aio->aio_sigevent.sigev_notify == SIGEV_SIGNAL) {
		
		/*
		 * Send signal to process, which is using this
		 * library, which is this process.
		 */
		
		
		if (aio->aio_sigevent.sigev_signo) {
		    rc = sigqueue(getpid(),aio->aio_sigevent.sigev_signo,
				  aio->aio_sigevent.sigev_value);
		    
		    if (rc) {
			
			CUSFS_TRACE_LOG_FILE(1,"signal 0x%x failed sig_value = 0x%x, rc %d, errno = %d",
					     aio->aio_sigevent.sigev_signo,
					     aio->aio_sigevent.sigev_value,
					     rc,errno);
			
			CUSFS_TRACE_LOG_FILE(1,"signal failed for aio %p completed for start_lba 0x%llx with status 0x%x of on file = %s",
					     aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);
			
			
		    }
		    
		    rc = 0;
		    
		} else {
		    
		    CUSFS_TRACE_LOG_FILE(1,"signal is 0 for filename %s",
					 aio->file->filename);
		}
		
		
	    } else if (aio->aio_sigevent.sigev_notify == SIGEV_THREAD) {
		
		/*
		 * Send signal to thread, which is using this
		 * library, which is this process.
		 */
		
		if (aio->aio_sigevent.sigev_notify_attributes) {
		    
		    thread_attr = (pthread_attr_t *)aio->aio_sigevent.sigev_notify_attributes;
		} 
		
		if (aio->aio_sigevent.sigev_notify_function) {
		    pthread_rc = pthread_create(&(thread_id),thread_attr,
						(void *(*)(void *))(aio->aio_sigevent.sigev_notify_function),
						(void *)&(aio->aio_sigevent.sigev_value));
		    
		    
		    if (pthread_rc) {
			
			cufs->stats.num_failed_threads++;
			
			CUSFS_TRACE_LOG_FILE(5,"pthread_create failed rc = %d,errno = %d, thread_attr = %p",
					     pthread_rc,errno, thread_attr);
		    }
		    
		} else {
		    
		    
		    CUSFS_TRACE_LOG_FILE(1,"Null sig_notify_function for filename %s",
					 aio->file->filename);
		}
		
		
	    }
	    
	} 
	
	file = aio->file;
	
	if (file) {
	    CUSFS_LOCK(file->lock);
	}
	
	cusfs_free_aio(cufs,aio,0);
	
	if (file) {
	    CUSFS_UNLOCK(file->lock);
	}
	
    
	aio = cufs->head_act;

#endif /* _CFLSH_USFS_ARESULT */

    } /* while */


    return rc;
}

/*
 * NAME:        cusfs_cleanup_all_completed_aios_for_file.
 *
 * FUNCTION:    Clean up all completed aios for file on
 *              assumption no one will call aio_return for them
 *              (such as close time).
 *              
 * NOTES:       This routine assumes the caller has the file->lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  None
 *              
 *              
 */
void cusfs_cleanup_all_completed_aios_for_file(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, int flags)
{
    cflsh_usfs_aio_entry_t *aio;
    cflsh_usfs_aio_entry_t *next_aio;



    aio = file->head_act;

    while (aio) {

	    
	next_aio = aio->file_next;

	CUSFS_TRACE_LOG_FILE(6,"aio %p for start_lba 0x%llx with status 0x%x of on file = %s",
			     aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);





	if (aio->cblk_status.status != CBLK_ARW_STATUS_PENDING) {

	    /*
	     * AIO completed with success/error. Lets process it
	     * now. First dequue it from active list.
	     */
	    
	    aio->flags &= ~CFLSH_USFS_AIO_E_ISSUED;

	    CUSFS_TRACE_LOG_FILE(6,"aio %p completed for start_lba 0x%llx with status 0x%x of on file = %s",
				 aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);


	    CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);


	    /*
	     * free_aio removes it from file->head_act list and cufs->head_complt list.
	     */

	    cusfs_free_aio(cufs,aio,0);
	}
	

	aio = next_aio;

    } /* while */

    return;
}


/*
 * NAME:        cusfs_wait_for_aio_for_file
 *
 * FUNCTION:    Wait for all Async I/O for this file.
 *              
 * NOTES:       This routine assumes the caller has the file->lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_wait_for_aio_for_file(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, int flags)
{
    int rc = 0;
    int retry = 0;
    cflsh_usfs_aio_entry_t *aio;
    int found_issued_aio;

    if (flags & CFLSH_USFS_ONLY_ASYNC_CMPLT) {

	CUSFS_TRACE_LOG_FILE(9,"aio = %p, for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
			     file->head_act, cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);

	while ((file->head_act) &&
	       (retry < CFLSH_USFS_WAIT_AIO_RETRY)) {
	    
	    aio = file->head_act;
	    found_issued_aio = FALSE;
	    while (aio) {
		
		if (aio->flags & CFLSH_USFS_AIO_E_ISSUED) {
		    CUSFS_TRACE_LOG_FILE(6,"Found aio %p for completionfor start_lba 0x%llx with status 0x%x of on file = %s",
					 aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);
		    found_issued_aio = TRUE;
		    break;
		}

		aio = aio->file_next;
		
	    } /* while */
	    
	    
	    if (!found_issued_aio) {
		
		break;
	    } else {
		
		CUSFS_UNLOCK(file->lock);
		usleep(CFLSH_USFS_WAIT_AIO_DELAY);
		CUSFS_LOCK(file->lock);
		retry++;
	    }
	} /* outer while */

	if (retry == CFLSH_USFS_WAIT_AIO_RETRY) {

	    CUSFS_TRACE_LOG_FILE(1,"exceeded wait time for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
				 cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);
	    errno = EBUSY;
	    rc = -1;
	}

    } else {
	
	CUSFS_TRACE_LOG_FILE(9,"aio = %p, for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
			     file->head_act, cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);

	while ((file->head_act) &&
	       (retry < CFLSH_USFS_WAIT_AIO_RETRY)) {
	    
	    
	    CUSFS_UNLOCK(file->lock);
	    usleep(CFLSH_USFS_WAIT_AIO_DELAY);
	    CUSFS_LOCK(file->lock);
	    cusfs_cleanup_all_completed_aios_for_file(cufs,file,0);

	    retry++;
	}
    
	if (file->head_act) {
	    
	    CUSFS_TRACE_LOG_FILE(1,"exceeded wait time for disk %s, num_act_areads = 0x%x, num_act_awrites = 0x%x",
				 cufs->device_name,cufs->stats.num_act_areads,cufs->stats.num_act_awrites);

	    errno = EBUSY;
	    rc = -1;
	}
    }

    return rc;
}

/*
 * NAME:        cusfs_get_status_all_aio_for_aiocb
 *
 * FUNCTION:    For the specified aiocb64, find all asociated 
 *              aio I/O requests's status.
 *              
 * NOTES:       This routine assumes the caller has the file->lock and
 *              the cufs->lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  Return status of aiocb
 *              
 *              
 */
int cusfs_get_status_all_aio_for_aiocb(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, struct aiocb64 *aiocbp, int flags)
{
    int rc = 0;
    cflsh_usfs_aio_entry_t *aio;
    int num_found_aio = 0;
    struct aiocb64 *aio_handle;




    if (aiocbp == NULL) {


	return EINVAL;
    }

#ifdef _AIX
    aio_handle = (struct aiocb64 *)aiocbp->aio_handle;
#else
    aio_handle = (struct aiocb64 *)aiocbp->__next_prio;
#endif /* !_AIX */

    aio = file->head_act;

    while (aio) {


	if ((aio_handle == aio->aiocbp) &&
	    (aiocbp->aio_offset == aio->aio_offset) &&
	    (aiocbp->aio_nbytes == aio->aio_nbytes)) {
	    
	    CUSFS_TRACE_LOG_FILE(6,"Found aio %p for aio_handle %p, start_lba 0x%llx with status 0x%x of on file = %s",
				 aio,aio_handle,aio->start_lba,aio->cblk_status.status,aio->file->filename);

	    num_found_aio++;

	    
	    if ((aio->flags & CFLSH_USFS_AIO_E_ISSUED) &&
		((aio->cblk_status.status != CBLK_ARW_STATUS_PENDING))) {

		/*
		 * AIO completed with success/error. Lets process it
		 * now. First dequue it from active list.
		 */
	    
		aio->flags &= ~CFLSH_USFS_AIO_E_ISSUED;

		CUSFS_TRACE_LOG_FILE(6,"aio %p completed for start_lba 0x%llx with status 0x%x of on file = %s",
				     aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);


		CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);

		/*
		 * Since this aio is associated with a POSIX AIO
		 * request, queue it to the completion
		 * queue.
		 */


		CUSFS_Q_NODE_TAIL(cufs->head_cmplt,cufs->tail_cmplt, 
				  aio,cmplt_prev,cmplt_next);

	    }

	    /*
	     * Give EINPROGRESS priority over all
	     * errors here, since we do not want
	     * caller to call aio_return until
	     * everything is done.
	     */
	    
	    switch(aio->cblk_status.status) {
	      case CBLK_ARW_STATUS_PENDING: 
		rc =  EINPROGRESS;
		break;
	      case CBLK_ARW_STATUS_SUCCESS:
		if (!rc) {
		    rc = 0;
		}
		break;
	      case CBLK_ARW_STATUS_INVALID:
		CUSFS_TRACE_LOG_FILE(1,"Invalid status aio %p for start_lba 0x%llx with status 0x%x fail_errno = %d of on file = %s",
				 aio,aio->start_lba,aio->cblk_status.status, aio->cblk_status.fail_errno,aio->file->filename);
		if (rc != EINPROGRESS) {
		    rc = EINVAL;
		}
		break;
	      case CBLK_ARW_STATUS_FAIL:
		if (rc != EINPROGRESS) {
		    rc = aio->cblk_status.fail_errno;
		}
		break;
		
	    }
	}

	aio = aio->file_next;
    }


    

    if (num_found_aio == 0) {

	CUSFS_TRACE_LOG_FILE(1,"no file->head_act found %p aio_handle %p, aiocbp->aio_offset = 0x%llx aiocbp->aio_nbytes = 0x%llx",
			     file->head_act,aio_handle,aiocbp->aio_offset,aiocbp->aio_nbytes);

	aio = file->head_act;

	while (aio) {

	    CUSFS_TRACE_LOG_FILE(1,"file act aio %p for start_lba 0x%llx aio_offset = 0x%llx aio_nbytes = 0x%llx with status 0x%x, aiocbp = %p",
				 aio,aio->start_lba,
				 aio->aio_offset,
				 aio->aio_nbytes,
				 aio->cblk_status.status,
				 aio->aiocbp);
	    aio = aio->file_next;
	}


	aio = cufs->head_act;

	while (aio) {

	    CUSFS_TRACE_LOG_FILE(1,"cufs act aio %p for start_lba 0x%llx aio_offset = 0x%llx aio_nbytes = 0x%llx with status 0x%x, aiocbp = %p",
				 aio,aio->start_lba,
				 aio->aio_offset,
				 aio->aio_nbytes,
				 aio->cblk_status.status,
				 aio->aiocbp);
	    aio = aio->act_next;
	}


	aio = cufs->head_cmplt;


	while (aio) {

	    CUSFS_TRACE_LOG_FILE(1,"complete act aio %p for start_lba 0x%llx aio_offset = 0x%llx aio_nbytes = 0x%llx with status 0x%x, aiocbp = %p",
				 aio,aio->start_lba,
				 aio->aio_offset,
				 aio->aio_nbytes,
				 aio->cblk_status.status,
				 aio->aiocbp);
	    aio = aio->cmplt_next;
	}


	rc = EINVAL;
    }

    CUSFS_TRACE_LOG_FILE(6,"num_found_aio = %d, rc = %d",
			 num_found_aio,rc);
    return rc;
}


/*
 * NAME:        cusfs_return_status_all_aio_for_aiocb
 *
 * FUNCTION:    For the specified aiocb64, find all asociated 
 *              aio I/O requests's completed status.
 *              
 * NOTES:       This routine assumes the caller has the file->lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  Return status of aiocb
 *              
 *              
 */
int cusfs_return_status_all_aio_for_aiocb(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, struct aiocb64 *aiocbp, int flags)
{
    int rc = 0;
    cflsh_usfs_aio_entry_t *aio;
    cflsh_usfs_aio_entry_t *next_aio;
    int num_found_aio = 0;
    struct aiocb64 *aio_handle;


#ifdef _AIX
    aio_handle = (struct aiocb64 *)aiocbp->aio_handle;
#else
    aio_handle = (struct aiocb64 *)aiocbp->__next_prio;
#endif /* !_AIX */

    aio = file->head_act;

    while (aio) {

	if ((aio_handle == aio->aiocbp) &&
	    (aiocbp->aio_offset == aio->aio_offset) &&
	    (aiocbp->aio_nbytes == aio->aio_nbytes)) {
	    
	    CUSFS_TRACE_LOG_FILE(6,"Found aio %p for aio_handle %p start_lba 0x%llx with status 0x%x of on file = %s",
				 aio,aio_handle,aio->start_lba,aio->cblk_status.status,aio->file->filename);

	    num_found_aio++;
	    
	    

	    if ((aio->flags & CFLSH_USFS_AIO_E_ISSUED) &&
		((aio->cblk_status.status != CBLK_ARW_STATUS_PENDING))) {

		/*
		 * AIO completed with success/error. Lets process it
		 * now. First dequue it from active list.
		 */
	    
		aio->flags &= ~CFLSH_USFS_AIO_E_ISSUED;

		CUSFS_TRACE_LOG_FILE(6,"aio %p completed for start_lba 0x%llx with status 0x%x of on file = %s",
				     aio,aio->start_lba,aio->cblk_status.status,aio->file->filename);


		CUSFS_DQ_NODE(cufs->head_act,cufs->tail_act, aio,act_prev,act_next);

		/*
		 * Since this aio is associated with a POSIX AIO
		 * request, queue it to the completion
		 * queue.
		 */


		CUSFS_Q_NODE_TAIL(cufs->head_cmplt,cufs->tail_cmplt, 
				  aio,cmplt_prev,cmplt_next);
	    }

	    /*
	     * Give EINPROGRESS priority over all
	     * errors here, since we do not want
	     * caller to call aio_return until
	     * everything is done.
	     */
	    
	    switch(aio->cblk_status.status) {
	      case CBLK_ARW_STATUS_PENDING: 
		rc =  -EINPROGRESS;
		break;
	      case CBLK_ARW_STATUS_SUCCESS:
		if (!rc) {
		    rc = aiocbp->aio_nbytes;
		}
		break;
	      case CBLK_ARW_STATUS_INVALID:
		if (rc != -EINPROGRESS) {
		    rc = -EINVAL;
		}
		break;
	      case CBLK_ARW_STATUS_FAIL:
		if (rc != -EINPROGRESS) {
		    rc = -aio->cblk_status.fail_errno;
		}
		break;
		
	    }

	    
	    /*
	     * Set errno and return code
	     * in aiocb here.
	     */

	    if (rc < 0) {

		CUSFS_RET_AIOCB(aiocbp,-1,-rc);
	    } else {

		CUSFS_RET_AIOCB(aiocbp,rc,0);

	    }

	}

	aio = aio->file_next;
    } /* while */


    if (num_found_aio == 0) {

	rc = EINVAL;
    } else if (rc != EINPROGRESS) {
	/*
	 * if there are one or more
	 * aio's associated with this aiocb
	 * and they are all completed.
	 * Then free each of them up now.
	 */

	aio = file->head_act;
	
	while (aio) {
	    
	    next_aio = aio->file_next;

	    if ((aio_handle == aio->aiocbp) &&
		(aiocbp->aio_offset == aio->aio_offset) &&
		(aiocbp->aio_nbytes == aio->aio_nbytes)) {
		
		CUSFS_TRACE_LOG_FILE(6,"Found aio %p aio_handle %p for completionfor start_lba 0x%llx with status 0x%x aio->aiocbp %p",
				     aio,aio_handle,aio->start_lba,aio->cblk_status.status,aio->aiocbp);
		

		
		
		if (!(aio->flags & CFLSH_USFS_AIO_E_ISSUED) &&
		    ((aio->cblk_status.status != CBLK_ARW_STATUS_PENDING))) {
		    
		    /*
		     * Command completed, process it.
		     */
		    CUSFS_DQ_NODE(cufs->head_cmplt,cufs->tail_cmplt, aio,cmplt_prev,cmplt_next);

		    cusfs_free_aio(cufs,aio,0);
		}

	    }

	    aio = next_aio;

	} /* while */
    }

    CUSFS_TRACE_LOG_FILE(6,"num_found_aio = %d, rc = %d",
			 num_found_aio,rc);
    return rc;
}


/*
 * NAME:        cusfs_start_aio_thread_for_cufs
 *
 * FUNCTION:    Start a background thread for this filesystem
 *              to process async I/O completions.
 *              
 * NOTES:       This routine assumes the caller
 *              is holding both the cufs lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  0 - success, Otherwise error.
 *              
 *              
 */
int cusfs_start_aio_thread_for_cufs(cflsh_usfs_t *cufs,  int flags)
{
    int rc = 0;
    int pthread_rc;
    cflsh_usfs_async_thread_t *async_data;


    pthread_rc = pthread_cond_init(&(cufs->thread_event),NULL);
    
    if (pthread_rc) {
        
        CUSFS_TRACE_LOG_FILE(1,"pthread_cond_init failed for thread_event rc = %d errno= %d",
                            pthread_rc,errno);

            
        errno = EAGAIN;
        return -1;
        
    }

    async_data = &(cufs->async_data);
    async_data->cufs = cufs;

    pthread_rc = pthread_create(&(cufs->thread_id),NULL,cusfs_aio_complete_thread,async_data);
        
    if (pthread_rc) {
            
        cufs->stats.num_failed_threads++;
            
        CUSFS_TRACE_LOG_FILE(5,"pthread_create failed rc = %d,errno = %d",
			     pthread_rc,errno);

        errno = EAGAIN;
        return -1;
    } else {
	cufs->stats.num_success_threads++;

	cufs->stats.num_active_threads++;
    }


    return rc;
}

/*
 * NAME:        cusfs_stop_aio_thread_for_cufs
 *
 * FUNCTION:    Stop the background thread for this file
 *              system.
 *              
 * NOTES:       This routine assumes the caller
 *              is holding both the cufs lock.
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:  None
 *              
 *              
 */
void cusfs_stop_aio_thread_for_cufs(cflsh_usfs_t *cufs,  int flags)
{
    int pthread_rc;



    cufs->thread_flags |= CFLSH_CUFS_EXIT_ASYNC;

    pthread_rc = pthread_cond_signal(&(cufs->thread_event));
        
    if (pthread_rc) {
            
        CUSFS_TRACE_LOG_FILE(5,"pthread_cond_signal failed rc = %d,errno = %d",
                            pthread_rc,errno);
    }

    /*
     * Since we are going to do pthread_join we need to unlock here.
     */

    CUSFS_UNLOCK(cufs->lock);

    pthread_join(cufs->thread_id,NULL);

    CUSFS_LOCK(cufs->lock);

    cufs->stats.num_active_threads--;

    cufs->thread_flags &= ~CFLSH_CUFS_EXIT_ASYNC;


    return;
}

/*
 * NAME:        cusfs_aio_complete_thread
 *
 * FUNCTION:    This routine is invoked as background
 *              thread to process async I/O events
 *              for this cufs.
 *
 *
 * Environment: This routine assumes the cufs mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              data - of type cflsh_usfs_async_thread_t
 *
 * RETURNS:
 *              
 */
void *cusfs_aio_complete_thread(void *data)
{
    void *ret_code = NULL;
    cflsh_usfs_async_thread_t *async_data = data;
    int pthread_rc = 0;
    cflsh_usfs_t *cufs;


    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);

    cufs = async_data->cufs;

    if (cufs == NULL) {

        CUSFS_TRACE_LOG_FILE(5,"cufs filename is NULL");

        return (ret_code);
    }

    if (CFLSH_EYECATCH_FS(cufs)) {
        /*
         * Invalid cufs. Exit now.
         */

        //??cflsh_blk.num_bad_cufs_ids++;
        CUSFS_TRACE_LOG_FILE(1,"cufs filename = %s pthread_cond_wait failed rc = %d errno = %d",
                            async_data->cufs->device_name,pthread_rc,errno);

        return (ret_code);
    }

    CUSFS_TRACE_LOG_FILE(5,"start of thread cufs->device_name = %d",cufs->device_name);

    while (TRUE) {

        CUSFS_LOCK(cufs->lock);

        if (CFLSH_EYECATCH_FS(cufs)) {
            /*
             * Invalid cufs. Exit now.
             */

            //??cflsh_blk.num_bad_cufs_ids++;
            CUSFS_TRACE_LOG_FILE(1,"cufs filename = %s invalid cufs eye catcher 0x%x",
                                    async_data->cufs->device_name,cufs->eyec);

            CUSFS_LIVE_DUMP_THRESHOLD(9,"0x20d");
            CUSFS_UNLOCK(cufs->lock);
            return (ret_code);
        }
 
        if (!(cufs->thread_flags) &&
            (cufs->head_act == NULL)) {
    
            /*
             * Only wait if the thread_flags
             * has not been set and there are no active commands
             */
            pthread_rc = pthread_cond_wait(&(cufs->thread_event),&(cufs->lock.plock));
        
            if (pthread_rc) {
            
                CUSFS_TRACE_LOG_FILE(5,"cufs filename = %s, pthread_cond_wait failed rc = %d errno = %d",
                                    cufs->device_name,pthread_rc,errno);
                CUSFS_UNLOCK(cufs->lock);
                return (ret_code);
            }
        
        }
     

        CUSFS_TRACE_LOG_FILE(9,"cufs filename = %s thread_flags = %d ",
                            cufs->device_name,cufs->thread_flags);

        if (cufs->thread_flags & CFLSH_CUFS_EXIT_ASYNC) {

            

            CUSFS_TRACE_LOG_FILE(5,"exiting thread: cufs->device_name = %s thread_flags = %d",
                                cufs->device_name,cufs->thread_flags);

            CUSFS_UNLOCK(cufs->lock);
            break;
        }           


	cusfs_check_active_aio_complete(cufs,0);

	//?? Should this sleep and process peridically if commands found, but not complete.

        CUSFS_UNLOCK(cufs->lock);

#ifdef _REMOVE
	if (cufs->head_act) {
	    usleep(CFLSH_USFS_WAIT_AIO_BG_DLY);
	}
#endif
    } /* while */

    return ret_code;

}

    

/*
 * NAME:        cusfs_access_fs
 *
 * FUNCTION:    Access an existing filesystem
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.

 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, otherwise success
 *              
 *              
 */

cflsh_usfs_t *cusfs_access_fs(char *device_name, int flags)
{
    cflsh_usfs_t *cufs;


    if (device_name == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"Null device_name");

	errno = EINVAL;
	return NULL;
    }

    cufs = cusfs_get_cufs(device_name,0);

    if (cufs == NULL) {

	return NULL;
    }

    if (cusfs_validate_superblock(cufs,0)) {


	cusfs_release_cufs(cufs,0);
	return NULL;
    }
    


    /*
     * Get root directory
     *
     * NOTE: cusfs_lookup_data_obj, queues
     *       root directory file into cufs'
     *       filelist and set the root_directory
     *       pointer to it.
     */

    if (cusfs_lookup_data_obj(cufs,"/",0) == NULL) {

	cusfs_release_cufs(cufs,0);
	return NULL;
    }



    return cufs;
}


/*
 * NAME:        cusfs_add_file_to_hash
 *
 * FUNCTION:    Add file to global hash table
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
void cusfs_add_file_to_hash(cflsh_usfs_data_obj_t *file, int flags)
{
    cflsh_usfs_data_obj_t *tmp_file = NULL;

    if (file == NULL) {

	return;
    }


    if (file->flags & CFLSH_USFS_IN_HASH) {

	/*
	 * Already in hash table
	 */

	return;
    }


    if (cusfs_global.file_hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)] == NULL) {

	cusfs_global.file_hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)] = file;

	file->flags |= CFLSH_USFS_IN_HASH;

    } else {

	tmp_file = cusfs_global.file_hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)];

	while (tmp_file) {

	    CUSFS_TRACE_LOG_FILE(9,"tmp_file address = 0x%p, file address = 0x%p, hash[] = 0x%p index = 0x%x", 
				 tmp_file, file,cusfs_global.hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)],
				 (file->index & (MAX_NUM_CUSFS_FILE_HASH-1)));
	

	    if ((ulong)tmp_file & CUSFS_BAD_ADDR_MASK) {


		cusfs_global.num_bad_file_ids++;

		

		CUSFS_TRACE_LOG_FILE(1,"Corrupted file address = 0x%p, hash[] = 0x%p index = 0x%x", 
				     tmp_file, cusfs_global.hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)],
                                        (file->index & MAX_NUM_CUSFS_FILE_HASH));
	    }


	    if (tmp_file == tmp_file->fnext) {
		CUSFS_TRACE_LOG_FILE(1,"tmp_file  = 0x%p points to itself, hash[] = 0x%p index = 0x%x", 
				     tmp_file, cusfs_global.hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)],
                                        (file->index & MAX_NUM_CUSFS_FILE_HASH));

		break;
	    }



	    if (tmp_file->fnext == NULL) {

		tmp_file->fnext = file;

	        file->fprev = tmp_file;

		file->flags |= CFLSH_USFS_IN_HASH;

		break;

	    }

	    tmp_file = tmp_file->fnext;
	}
    }
    return;

}

/*
 * NAME:        cusfs_remove_file_from_hash
 *
 * FUNCTION:    Remove file to global hash table
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 success, otherwise failure
 *              
 *              
 */
void cusfs_remove_file_from_hash(cflsh_usfs_data_obj_t *file, int flags)
{
    if (file == NULL) {

	return;
    }
    
    if (file->fprev) {
	file->fprev->fnext = file->fnext;
	
    } else {
	
	cusfs_global.file_hash[file->index & (MAX_NUM_CUSFS_FILE_HASH-1)] = file->fnext;
    }
    
    if (file->fnext) {
	file->fnext->fprev = file->fprev;
    }
    
    
    file->flags &= ~CFLSH_USFS_IN_HASH;

    return;

}

/*
 * NAME:        cusfs_find_data_obj_from_inode
 *
 * FUNCTION:    This routine finds existing
 *              data objects for the specified filesystem
 *              from inode number.
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     None
 *              
 */
cflsh_usfs_data_obj_t *cusfs_find_data_obj_from_inode(cflsh_usfs_t *cufs, uint64_t inode_num, int flags)
{
    cflsh_usfs_data_obj_t *file = NULL;


    file = cufs->filelist_head;

    while (file) {

	if (file->inode->index == inode_num) {

	    break;
	}
	
	file = file->next;
    }


    return file;
}

/*
 * NAME:        cusfs_find_data_obj_from_filename
 *
 * FUNCTION:    This routine finds existing
 *              data objects for the specified filesystem
 *              from filename.
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     None
 *              
 */
cflsh_usfs_data_obj_t *cusfs_find_data_obj_from_filename(cflsh_usfs_t *cufs, char *filename, int flags)
{
    cflsh_usfs_data_obj_t *file = NULL;



    CUSFS_TRACE_LOG_FILE(9,"filename = %s",filename);

    file = cufs->filelist_head;

    while (file) {

	
	CUSFS_TRACE_LOG_FILE(9,"file->filename = %s",file->filename);
	if (!(strcmp(file->filename,filename))) {

	    break;
	}
	
	file = file->next;
    }

    CUSFS_TRACE_LOG_FILE(9,"file->filename = %s",
			 ((file != NULL) ? file->filename:NULL));
    return file;
}

/*
 * NAME:        cusfs_rdwr_data_object
 *
 * FUNCTION:    This routine will read/write the data blocks
 *              for the specified inode into the
 *              supplied buffer
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     0 success,
 *              
 */
int cusfs_rdwr_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags)
{
    int rc = 0;
    cflsh_usfs_disk_lba_e_t *disk_lbas;
    uint64_t num_disk_lbas;
    uint64_t offset = 0;
    size_t nblocks;
    size_t bytes_transferred = 0;
    size_t ret_code;
    char *bptr;
    int i;
    int exit_loop = FALSE;


    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }

    if (inode == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Inode pointer is null for device %s",
			    cufs->device_name);
	errno = EINVAL;
	return -1;
    }

#ifdef _REMOVE

    //?? Allow one to specify a buffer smaller than the file size?
    if (inode->file_size > *buf_size) {

	CUSFS_TRACE_LOG_FILE(1,"Inode file_size 0x%llx is larger then buf_size 0x%llxfor device %s",
			    inode->file_size,buf_size,cufs->device_name);
	errno = EINVAL;
	return -1;
    }
    
#endif /* _REMOVE */


    if ((*buf_size) % cufs->disk_block_size) {
		
	/*
	 * ?? TODO: Don't support internally allocating a buffer
	 * to the end of the sector and returning data back to the
	 * caller.
	 */

	CUSFS_TRACE_LOG_FILE(1,"Data buffer of size 0x%llx does not end on sector boundary for device %s",
			    *buf_size,cufs->device_name);

	return -1;

    }


    if (cusfs_get_disk_lbas_from_inode_ptr_for_transfer(cufs, inode, offset,
						       *buf_size, &disk_lbas,&num_disk_lbas,0)) {
	
	CUSFS_TRACE_LOG_FILE(1,"For inode of type %d on disk %s failed to get inode pointers offset = 0x%llx",
			    inode->type,cufs->device_name,offset);
	
	
	return -1;

    }

    bptr = buf;

    bytes_transferred = 0;

    for (i = 0; i < num_disk_lbas; i++) {

	nblocks = MIN(disk_lbas[i].num_lbas,cufs->max_xfer_size);

	nblocks = MIN(nblocks,((*buf_size - bytes_transferred) / cufs->disk_block_size));



	while (nblocks) {

	    if (flags & CFLSH_USFS_RDWR_WRITE) {
		ret_code = cblk_write(cufs->chunk_id,bptr,disk_lbas[i].start_lba,nblocks,0);
	    } else {
		ret_code = cblk_read(cufs->chunk_id,bptr,disk_lbas[i].start_lba,nblocks,0);
	    }

	    if (ret_code < 0) {

		exit_loop = TRUE;
		rc = -1;
		break;
	    }

	    bptr += ret_code * cufs->disk_block_size;

	    nblocks = MIN((disk_lbas[i].num_lbas - ret_code),cufs->max_xfer_size);

	    bytes_transferred += ret_code * cufs->disk_block_size;
	    if (ret_code < nblocks) {

		/*
		 * Give up on future reads,
		 * but return good status with
		 * the number of bytes read.
		 */

		exit_loop = TRUE;
		break;
	    }

	} /* while */


	if (bytes_transferred == *buf_size) {

	    break;
	}
	
	if (exit_loop) {

	    break;
	}

    } /* for */

    *buf_size = bytes_transferred;
    
    return rc;
}

/*
 * NAME:        cusfs_read_data_object
 *
 * FUNCTION:    This routine will read the data blocks
 *              for the specified inode into the
 *              supplied buffer
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     0 success,
 *              
 */
int cusfs_read_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags)
{

    return (cusfs_rdwr_data_object(cufs,inode,buf,buf_size,flags));
}

/*
 * NAME:        cusfs_read_data_obj_for_inode_no
 *
 * FUNCTION:    This routine will read the data blocks
 *              for the specified inode number into the
 *              supplied buffer
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     0 success,
 *              
 */
int cusfs_read_data_object_for_inode_no(cflsh_usfs_t *cufs, uint64_t inode_num, 
				      void **buf, int *buf_size, int flags)
{
    int rc = 0;
    cflsh_usfs_inode_t *inode;
    void *lbuf;
    int lbuf_size;

    if (cufs == NULL) {


	errno = EINVAL;
	return -1;
    }
    

    inode = cusfs_get_inode_from_index(cufs,inode_num,0);


    if (inode == NULL) {

	return -1;
    }
    

    lbuf_size = inode->file_size;

    lbuf = malloc(lbuf_size);

    if (lbuf == NULL) {

	CUSFS_TRACE_LOG_FILE(1,"malloc of buf of size 0x%x for failed for disk %s",
			    lbuf_size,cufs->device_name);
		
	return -1;
    }

    rc = cusfs_read_data_object(cufs, inode, lbuf,&lbuf_size, flags);

    free(inode);

    *buf = lbuf;

    *buf_size = lbuf_size;

    return rc;
}

/*
 * NAME:        cusfs_write_data_obj
 *
 * FUNCTION:    This routine will write the data blocks
 *              for the specified inode into the
 *              supplied buffer
 *
 *
 * NOTE:        
 *
 *
 * INPUTS:
 *              
 *
 * RETURNS:     0 success,
 *              
 */
int cusfs_write_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags)
{
    
    return (cusfs_rdwr_data_object(cufs,inode,buf,buf_size,(flags | CFLSH_USFS_RDWR_WRITE)));
}


/*
 * NAME:        cusfs_lookup_data_obj
 *
 * FUNCTION:    Look up a filesystem data object..
 *
 *
 * NOTES:       This code assumes the pathname specified
 *              starts with the disk special filename
 *              followed a ":" and then the
 *              filename.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, Otherwise success
 *              
 *              
 */

cflsh_usfs_data_obj_t *cusfs_lookup_data_obj(cflsh_usfs_t *cufs, 
					   char *pathname, int flags)
{
    cflsh_usfs_data_obj_t *file = NULL;
    char filename2[PATH_MAX];
    char *local_pathname;
    char *filename;
    char *name;
    uint64_t directory_inode_no;
    void *buf;
    int buf_size;


    if (cufs == NULL) {


	errno = EINVAL;
	return NULL;
    }
		   
    if (pathname == NULL) {


	errno = EINVAL;
	return NULL;
    }

    local_pathname = cusfs_get_only_filename_from_diskpath(pathname,0);



    CUSFS_TRACE_LOG_FILE(9,"local_pathname = %s, pathname = %s",
			local_pathname,pathname);

    /*
     * See if we already have a data object for this
     * filename.
     */

    file = cusfs_find_data_obj_from_filename(cufs,local_pathname,0);

    if (file) {

	return file;
    }

    /*
     * If no data object was found, then search
     * through the directories on the disk
     * to find one that matches this pathname.
     */

    file = cusfs_alloc_file(cufs,local_pathname,CFLSH_USFS_ALLOC_READ_AHEAD);

    if (file == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"malloc of file %s for failed for disk %s",
			    local_pathname,cufs->device_name);
	return NULL;
    }

    if (!strcmp(local_pathname,"/")) {

	/*
	 * This is the root directory
	 */


	file->flags |= CFLSH_USFS_ROOT_DIR;

	file->inode = cusfs_find_root_inode(cufs,0);

	if (file->inode == NULL) {


	    CUSFS_TRACE_LOG_FILE(1,"Root directory inode is NULL for %s on disk %s%s",
				local_pathname,cufs->device_name);
	    cusfs_free_file(file);
	    return NULL;
	}

	cufs->root_directory = file;

    } else {


	strcpy(filename2,local_pathname);

	/*
	 * Skip past the first slash
	 */
	filename = filename2;
	filename++;



	name = strtok(filename,"/");

	directory_inode_no = cufs->root_directory->inode->index;

	while (name) {


	    buf = NULL;

	    if (cusfs_read_data_object_for_inode_no(cufs, directory_inode_no, 
						   &buf, &buf_size,0)) {

		
		CUSFS_TRACE_LOG_FILE(5,"Failed to read directory inode = 0x%llx of type %d on disk %s, ",
				    directory_inode_no, cufs->device_name);

		if (buf) {
		    free(buf);
		}

		free(file->inode);

		cusfs_free_file(file);
		
		return NULL;
	    }


	    file->inode = cusfs_find_inode_num_from_directory_by_name(cufs,name,buf,buf_size,0);


	    if (file->inode == NULL)  {

		CUSFS_TRACE_LOG_FILE(5,"Failed to find %s in directory inode = 0x%llx on disk %s",
				    name,directory_inode_no, cufs->device_name);
		
		free(buf);
		cusfs_free_file(file);
		
		return NULL;
	    }

	    if (file->inode->type == CFLSH_USFS_INODE_FILE_DIRECTORY) {


		directory_inode_no = file->inode->index;

		name = strtok(NULL,"/");

		if (name) {
		    free(file->inode);
		} else {
		    file->file_type = file->inode->type;

		}
	    } else {


		file->file_type = file->inode->type;
		name = NULL;
	    }



	    free(buf);

	} /* while */

    }



    file->data_buf_size = cufs->fs_block_size;

    file->data_buf = malloc(file->data_buf_size);


    if (file->data_buf == NULL) {


	if (file->flags & CFLSH_USFS_ROOT_DIR) {

	    cufs->root_directory = NULL;
	}



	cusfs_free_file(file);

	return NULL;
    }

    //?? what about cufs->locking here?

    if (file) {

	CUSFS_Q_NODE_TAIL(cufs->filelist_head,cufs->filelist_tail,file,prev,next);

    }

    CUSFS_TRACE_LOG_FILE(5,"Found file = %s of type %d on disk %s, ",
			pathname,file->inode->type, cufs->device_name);



    return file;
}


/*
 * NAME:        cusfs_find_fs_for_disk
 *
 * FUNCTION:    This routine looks for a cufs entry
 *              for this process that has the specified
 *              device_name. If that is not found, then
 *              it attempts to read the filesystem from the specified
 *              disk
 *
 *
 * ?? TODO: 	This code does not handle the linux case
 *   	       	where different disk special names can 
 *   	       	be paths to the same disk. Thus this code
 *          	could incorrectly have multiple cufs entries
 *          	for a process pointing to the same disk.
 *
 *          	For linux maybe this is solved via the 
 *          	WWID/udid
 *
 *
 * NOTE:        This routine assumes the caller
 *              has the cusfs_global.global_lock.
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, otherwise success
 *              
 *              
 */
cflsh_usfs_t *cusfs_find_fs_for_disk(char *device_name, int flags)
{
    cflsh_usfs_t *cufs;
    int i;
    int found = FALSE;


    for (i=0; i < MAX_NUM_CUSFS_HASH; i++) {

	cufs = cusfs_global.hash[i];

	while (cufs) {
	    
	    if (!strcmp(cufs->device_name,device_name)) {

		found = TRUE;

		break;
	    }

	    cufs = cufs->next;
	}

	if (found) {
	    break;
	}
    }

    if (cufs == NULL) {


	cufs = cusfs_access_fs(device_name,0);

    }

    return cufs;
}
/*
 * NAME:        cusfs_get_only_filename_from_diskpath
 *
 * FUNCTION:    This routine from a concatenated
 *              disk filename will extract only the
 *              file path inside filesystem without the disk
 *              name. If there is no disk path then the full
 *              disk pathname is returned.
 *
 *
 * NOTES:       This code assumes the pathname specified
 *              starts with the disk special filename
 *              followed a ":" and then the
 *              filename.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, Otherwise success.
 *              
 *              
 */
char *cusfs_get_only_filename_from_diskpath(char *disk_pathname, int flags)
{
    char *filename;


	   
    if (disk_pathname == NULL) {


	errno = EINVAL;
	return NULL;
    }


 
 
    filename = strchr(disk_pathname,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename == NULL) {

	return disk_pathname;
    }

    filename++;

    

    return filename;

}


/*
 * NAME:        cusfs_find_fs_from_path
 *
 * FUNCTION:    This routine from a concatenated
 *              disk filename will get the associated
 *              filesystem data structure.
 *
 *
 * NOTES:       This code assumes the pathname specified
 *              starts with the disk special filename
 *              followed a ":" and then the
 *              filename.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, Otherwise success.
 *              
 *              
 */
cflsh_usfs_t *cusfs_find_fs_from_path(char *disk_pathname, int flags)
{
    char device_name[PATH_MAX];
    char disk_pathname_copy[PATH_MAX];
    char *filename;
    cflsh_usfs_t *cufs = NULL;


	   
    if (disk_pathname == NULL) {


	errno = EINVAL;
	return NULL;
    }


    /*
     * Copy disk_pathname, since we're
     * going to modify it.
     */

    strcpy(disk_pathname_copy,disk_pathname);
    
    filename = strchr(disk_pathname_copy,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
			    disk_pathname);
	return NULL;
    }

    filename[0] = '\0';

    

    /*
     * Extract disk special filename
     */

    strcpy(device_name,disk_pathname_copy);

    cufs = cusfs_find_fs_for_disk(device_name,0);

    return cufs;

}

/*
 * NAME:        cusfs_open_disk_filename
 *
 * FUNCTION:    This routine from a concatenated
 *              disk filename will open file on
 *              the specified disk.
 *
 *
 * NOTES:       This code assumes the pathname specified
 *              starts with the disk special filename
 *              followed a ":" and then the
 *              filename.
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     Null failure, Otherwise success.
 *              
 *              
 */
cflsh_usfs_data_obj_t *cusfs_open_disk_filename(char *disk_pathname, int flags)
{
    char device_name[PATH_MAX];
    char *filename;
    char pathname[PATH_MAX];
    char disk_pathname_copy[PATH_MAX];
    cflsh_usfs_t *cufs = NULL;
    cflsh_usfs_data_obj_t *file = NULL;

	   
    if (disk_pathname == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"disk_pathname is null");

	errno = EINVAL;
	return NULL;
    }

    CUSFS_TRACE_LOG_FILE(9,"disk_pathname = %s",
			disk_pathname);


    /*
     * Copy disk_pathname, since we're
     * going to modify it.
     */

    strcpy(disk_pathname_copy,disk_pathname);

    filename = strchr(disk_pathname_copy,CFLSH_USFS_DISK_DELIMINATOR);

    if (filename == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Invalid disk_pathname %s",
			    disk_pathname);
	return NULL;
    }

    filename[0] = '\0';

    filename++;

    

    /*
     * Extract disk special filename
     */

    strcpy(device_name,disk_pathname_copy);

    /*
     * Extract file pathname for this filesystem
     * contained on this disk.
     */

    strcpy(pathname,filename);

    /*
     * ?? TODO: This code does not handle the linux case
     *          where different disk special names can 
     *          be paths to the same disk. Thus this code
     *          could incorrectly have multiple cufs entries
     *          for a process pointing to the same disk.
     *
     *          For linux maybe this is solved via the 
     *          WWID/udid
     */


    CUSFS_TRACE_LOG_FILE(9,"device_name = %s, filename = %s",
			device_name,filename);

    cufs = cusfs_find_fs_for_disk(device_name,0);

    if (cufs == NULL) {

	/*
	 * If we did not yet access that filesystem,
	 * then do that now.
	 */

	cufs = cusfs_access_fs(device_name,0);

	if (cufs == NULL) {
	    errno = ENOENT;

	}

    }


    file = cusfs_lookup_data_obj(cufs,filename,0);

    return file;
    
}


/*
 * NAME:        cusfs_flush_inode_timestamps
 *
 * FUNCTION:    Flush any pending inode time stamp
 *              updates to the disk.
 *
 * NOTE:        This routine assumes one is called under a lock
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:     0 sucess, Otherwise failure
 *              
 *              
 */
int cusfs_flush_inode_timestamps(cflsh_usfs_t *cusfs, cflsh_usfs_data_obj_t *file)
{

    if (file->inode_updates.flags & (CFLSH_USFS_D_INODE_MTIME|CFLSH_USFS_D_INODE_ATIME)) {
    
	if ((file->inode_updates.flags & CFLSH_USFS_D_INODE_MTIME) &&
	    (file->inode_updates.mtime > file->inode->mtime)) {

	    file->inode->mtime = file->inode_updates.mtime;
	}

	if ((file->inode_updates.flags & CFLSH_USFS_D_INODE_ATIME) &&
	    (file->inode_updates.atime > file->inode->atime)) {

	    file->inode->atime = file->inode_updates.atime;
	}

	file->inode_updates.flags &= ~(CFLSH_USFS_D_INODE_MTIME|CFLSH_USFS_D_INODE_ATIME);
	cusfs_update_inode(cusfs,file->inode,0);
    }

    return 0;

}

/*
 * NAME:        cusfs_get_disk_lbas_for_transfer
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

int cusfs_get_disk_lbas_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file,
				     uint64_t offset, size_t num_bytes, 
				     cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
				     int flags)
{
    int rc = 0;
    int i;
    uint64_t start_disk_lba_index;
    uint64_t start_lba = 0;
    uint64_t local_num_disk_lbas;
    uint64_t offset_in_this_fs_lba;
    cflsh_usfs_disk_lba_e_t *lba_list;
    uint64_t num_lbas=0, num_lbas2=0,orig_num_lbas;


    if (cufs == NULL) {

	errno = EINVAL;
	return -1;

    } 

    if (file == NULL) {

	errno = EINVAL;
	return -1;
    }



    start_disk_lba_index = offset/cufs->fs_block_size;

    

    local_num_disk_lbas = num_bytes/cufs->fs_block_size;

    if (num_bytes % cufs->fs_block_size) {

	local_num_disk_lbas++;
    }

    if ((offset % cufs->fs_block_size) &&
	(offset/cufs->fs_block_size != (offset+num_bytes-1)/cufs->fs_block_size)) {

	/*
	 * If the offset is not aligned on a filesystem block and
	 * the start and end of the transfer are not on the same
	 * filesystem block, then we need to span an additional
	 * filesystem block.
	 */

	local_num_disk_lbas++;
    }

    if (local_num_disk_lbas > file->num_disk_lbas) {

	errno = EINVAL;
	return -1;
    }

    lba_list = malloc(sizeof(cflsh_usfs_disk_lba_e_t) * local_num_disk_lbas);

    if (lba_list == NULL) {


	CUSFS_TRACE_LOG_FILE(1,"Malloc of disk_lbas on disk %s failed with errno %d ",
			    cufs->device_name,errno);

	return -1;
    }    


    CUSFS_TRACE_LOG_FILE(9,"offset = 0x%llx, num_bytes = 0x%llx",
			 offset,num_bytes);

    /*
     * Find start lba entry
     */

    for (i=0; i < local_num_disk_lbas;i++) {

	lba_list[i].num_lbas = file->disk_lbas[start_disk_lba_index + i].num_lbas;
	lba_list[i].start_lba = file->disk_lbas[start_disk_lba_index + i].start_lba;

	orig_num_lbas = lba_list[i].num_lbas;

	CUSFS_TRACE_LOG_FILE(9,"lba_list[%d].num_lbas = 0x%llx, start_lba = 0x%llx",
			     i,lba_list[i].num_lbas,lba_list[i].start_lba);
	if (i == 0) {

	    offset_in_this_fs_lba = offset % cufs->fs_block_size;

	    /*
	     * Adjust the transfer's start lba to the closest disk lba in this
	     * fs lba.
	     */

	    if (offset_in_this_fs_lba) {

		start_lba = lba_list[i].start_lba;

		start_lba += offset_in_this_fs_lba/cufs->disk_block_size;

		lba_list[i].num_lbas -= (start_lba - lba_list[i].start_lba);

		lba_list[i].start_lba = start_lba;
	    }

	    CUSFS_TRACE_LOG_FILE(9,"offset = 0x%llx, offset_in_this_fs_lba = 0x%llx, start_lba = 0x%llx",
			    offset,offset_in_this_fs_lba,start_lba);
	}

	if (i == (local_num_disk_lbas - 1)) {
	    
	    offset_in_this_fs_lba = (offset + num_bytes) % cufs->fs_block_size;


	    /*
	     * Adjust the transfer's end lba to the closest disk lba in this
	     * fs lba
	     */

	    if (offset_in_this_fs_lba) {

	    	num_lbas =  orig_num_lbas;

		num_lbas2 = offset_in_this_fs_lba/cufs->disk_block_size;

		if (offset_in_this_fs_lba % cufs->disk_block_size) {
		    num_lbas2++;
		}

		if (num_lbas2 < num_lbas) {

		    lba_list[i].num_lbas -= (num_lbas - num_lbas2);
		}
	    }

	    CUSFS_TRACE_LOG_FILE(9,"offset_in_this_fs_lba = 0x%llx, num_lbas = 0x%llx, num_lbas2 = 0x%llx",
				 offset_in_this_fs_lba,num_lbas,num_lbas2);


	}

	CUSFS_TRACE_LOG_FILE(9,"lba_list[%d].num_lbas = 0x%llx, start_lba = 0x%llx",
			     i,lba_list[i].num_lbas,lba_list[i].start_lba);
    }

    *num_disk_lbas = local_num_disk_lbas;

    *disk_lbas = lba_list;

    CUSFS_TRACE_LOG_FILE(9,"offset = 0x%llx, num_bytes = 0x%llx, *num_disk_lbas = 0x%llx, rc = %d",
			    offset,num_bytes,*num_disk_lbas,rc);

    return rc;
}

/*
 * NAME:        cusfs_seek_read_ahead
 *
 * FUNCTION:    When a seek request is done to the filesystem
 *              This routine will do a read starting at that sector
 *              in case the next request is a read.
 *
 * NOTE:        This routine assumes the caller has the file->lock.
 *
 * INPUTS:
 *             
 *
 * RETURNS:     None
 *              
 *              
 */

void cusfs_seek_read_ahead(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file,
			   uint64_t offset, int flags)
{
#ifdef EXPERIMENTAL
    int rc = 0;
    int arw_flags = (CBLK_ARW_USER_STATUS_FLAG|CBLK_ARW_WAIT_CMD_FLAGS);
    size_t size_transfer;
    cflsh_usfs_disk_lba_e_t *disk_lbas;
    uint64_t num_disk_lbas;
    uint64_t start_offset = offset;

    if ((file->seek_ahead.flags & CFLSH_USFS_D_RH_ACTIVE) &&
	(file->seek_ahead.cblk_status.status == CBLK_ARW_STATUS_PENDING)) {

	/*
	 * If the seek read head buffer is in use, then
	 * give up.
	 */

	CUSFS_TRACE_LOG_FILE(1,"Read ahead in use for lba = 0x%llx and nblocks 0x%llx",
			     file->seek_ahead.start_lba,file->seek_ahead.nblocks);
	return;
    }

    if ((file->seek_ahead.local_buffer == NULL) ||
	(file->seek_ahead.local_buffer_len == 0x0LL)) {
	
	CUSFS_TRACE_LOG_FILE(1,"Read ahead buffer %p is either null or size 0 (0x%llx)",
			     file->seek_ahead.local_buffer,file->seek_ahead.local_buffer_len);
	return;
    }

    size_transfer = MIN(cufs->disk_block_size,file->seek_ahead.local_buffer_len);


    if (offset + size_transfer > file->inode->file_size) {


	/*
	 * If doing a read ahead here goes beyond the end of the current
	 * file, then don't attempt a read ahead.
	 */

	CUSFS_TRACE_LOG_FILE(1,"Read ahead is beyond end of disk offset = 0x%llx  transfer size = 0x%llx file size = 0x%llx",
			     offset,size_transfer,file->inode->file_size);
	return;
    }

    /*
     * If we got here, then the seek read ahead is not in use
     * so use it now. Get start_offset that is fs block aligned.
     */


    start_offset &= ~(size_transfer - 1);


    if (cusfs_get_disk_lbas_for_transfer(cufs, file, start_offset,
					 size_transfer, &disk_lbas,&num_disk_lbas,0)) {
	
	CUSFS_TRACE_LOG_FILE(1,"For file = %s on disk %s failed to get inode pointers offset = 0x%llx",
			    file->filename,cufs->device_name,start_offset);
	
	
	

	return;

    }

    if (num_disk_lbas > 1) {

	/*
	 * If read ahead spans multiple block,
	 * then do not do it.
	 */

	CUSFS_TRACE_LOG_FILE(6,"Read ahead not attempted file = %s on disk %s num_disk_lbas = 0x%llx",
			    file->filename,cufs->device_name,num_disk_lbas);
	return;
	 
    }

    if (disk_lbas[0].num_lbas < 1) {

	/*
	 * If read ahead is not at least one sector
	 */

	CUSFS_TRACE_LOG_FILE(6,"Read ahead not attempted file = %s on disk %s disk_lbas[0].num_lbas = 0x%llx",
			    file->filename,cufs->device_name,disk_lbas[0].num_lbas);
	return;
	 
    }


    file->seek_ahead.flags &= ~CFLSH_USFS_D_RH_ACTIVE;

    file->seek_ahead.start_lba = disk_lbas[0].start_lba;
    file->seek_ahead.nblocks = MIN(disk_lbas[0].num_lbas,cufs->max_xfer_size);;
    bzero(file->seek_ahead.local_buffer,file->seek_ahead.local_buffer_len);
    bzero(&(file->seek_ahead.cblk_status),sizeof(file->seek_ahead.cblk_status));

    /*
     * Issue async read operation
     */

    rc = cblk_aread(cufs->chunk_id,file->seek_ahead.local_buffer,
		    file->seek_ahead.start_lba,file->seek_ahead.nblocks,
		    &(file->seek_ahead.tag),
		    &(file->seek_ahead.cblk_status),
		    arw_flags);

    if (rc >= 0) {
	file->seek_ahead.flags |= CFLSH_USFS_D_RH_ACTIVE;

	file->seek_ahead.start_offset = start_offset;
	
	CUSFS_TRACE_LOG_FILE(9,"Successful issue aread for lba = 0x%llx and nblocks 0x%llx, with ,rc = %d",
			     file->seek_ahead.start_lba,file->seek_ahead.nblocks,rc);
    } else {


	CUSFS_TRACE_LOG_FILE(1,"Failed to issue aread for lba = 0x%llx and nblocks 0x%llx, with ,errno = %d",
			     file->seek_ahead.start_lba,file->seek_ahead.nblocks,errno);
    }
#endif /* EXPERIMENTAL */
    return;
}



    
