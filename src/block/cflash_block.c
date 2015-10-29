/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block.c $                                    */
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


//ident on linux - testing if we can see this in the build(s)
//to find out if filemode is enabled, run "ident *.so"

#define CFLSH_BLK_FILENUM 0x0100
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"

#ifndef _AIX
#include <revtags.h>
REVISION_TAGS(block);
#ifdef BLOCK_FILEMODE_ENABLED
#ident "$Debug: BLOCK_FILEMODE_ENABLED $"
#endif
#ifdef CFLASH_LITTLE_ENDIAN_HOST
#ident "$Debug: LITTLE_ENDIAN $"
#else
#ident "$Debug: BIG_ENDIAN $"
#endif
#ifdef _MASTER_CONTXT
#ident "$Debug: MASTER_CONTEXT $"
#endif
#ifdef _KERNEL_MASTER_CONTXT
#ident "$Debug: KERNEL_MASTER_CONTEXT $"
#endif
#ifdef _COMMON_INTRPT_THREAD
#ident "$Debug: COMMON_INTRPT_THREAD $"
#endif
#else
#include <sys/scsi.h>
#endif
 
#define CFLSH_BLK_FILENUM 0x0100
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"
cflsh_block_t cflsh_blk;


char             *cblk_log_filename = NULL;    /* Trace log filename       */
                                               /* This traces internal     */
                                               /* activities               */
int              cblk_log_verbosity = 0;       /* Verbosity of traces in   */
                                               /* log.                     */
FILE             *cblk_logfp = NULL;           /* File pointer for         */
                                               /* trace log                */
FILE             *cblk_dumpfp = NULL;          /* File pointer  for        */
                                               /* dumpfile                 */
int              cblk_dump_level;              /* Run time level threshold */
					       /* required to initiate     */
					       /* live dump.               */
int              dump_sequence_num;            /* Dump sequence number     */


int 	         cblk_notify_log_level;        /* Run time level threshold */
					       /* notify/error logging     */


pthread_mutex_t  cblk_log_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t  cblk_init_lock = PTHREAD_MUTEX_INITIALIZER;

uint32_t num_thread_logs = 0;     /* Number of thread log files */


size_t           cblk_cache_size = CFLASH_CACHE_SIZE;
uint64_t         cblk_lun_id  = 0;
#ifndef _AIX
/* 
 * This __attribute constructor is not valid for the AIX xlc 
 * compiler. 
 */
static void _cflsh_blk_init(void) __attribute__((constructor));
static void _cflsh_blk_free(void) __attribute__((destructor));
#else 
static int cflsh_blk_initialize = 0;
static int cflsh_blk_init_finish = 0;
static int cflsh_blk_term = 0;

#endif


/*
 * NAME:        CBLK_GET_CHUNK_HASH
 *
 * FUNCTION:    Find chunk from chunk id in the hash table
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *
 * RETURNS:
 *              NULL = No chunk found.
 *              pointer = chunk found.
 *              
 */

inline cflsh_chunk_t *CBLK_GET_CHUNK_HASH(chunk_id_t chunk_id, int check_rdy)

{
    cflsh_chunk_t *chunk = NULL;


    chunk = cflsh_blk.hash[chunk_id & CHUNK_HASH_MASK];


    while (chunk) {

	if ( (ulong)chunk & CHUNK_BAD_ADDR_MASK ) {

	    CBLK_TRACE_LOG_FILE(1,"Corrupted chunk address = 0x%llx, index = 0x%x", 
				(uint64_t)chunk, (chunk_id & CHUNK_HASH_MASK));

	    cflsh_blk.num_bad_chunk_ids++;
	    chunk = NULL;

	    break;

	}


	if (chunk->index == chunk_id) {


	    /*
	     * Found the specified chunk. Let's
	     * validate it.
	     */

	    if (CFLSH_EYECATCH_CHUNK(chunk)) {
		/*
		 * Invalid chunk
		 */
		
		CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
				    chunk_id);
		chunk = NULL;
	    } else if ((!(chunk->flags & CFLSH_CHNK_RDY)) &&
		       (check_rdy)) {


		/*
		 * This chunk is not ready
		 */
		
		CBLK_TRACE_LOG_FILE(1,"chunk not ready, chunk_id = %d",
				    chunk_id);
		chunk = NULL;
	    } 
	    break;
	}

	chunk = chunk->next;

    } /* while */

    

    


    return (chunk);
}



/*
 * NAME:        CBLK_VALIDATE_RW
 *
 * FUNCTION:    Initial validation of read/write request.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer for read/write data
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *
 * RETURNS:
 *              0 for good completion
 *              -1 for error
 *              
 */

inline int CBLK_VALIDATE_RW(chunk_id_t chunk_id, void *buf, cflash_offset_t lba,size_t nblocks)

{

    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }


    if (buf == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null buf passed");

	errno = EINVAL;
	return -1;
    }

    if (lba < 0) {

        CBLK_TRACE_LOG_FILE(1,"Invalid LBA = 0x%llx",lba);

	errno = EINVAL;
	return -1;

    }

    return 0;
}


/*
 * NAME:        _cflsh_blk_init
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

void _cflsh_blk_init(void)
{
    char *cache_size = getenv("CFLSH_BLK_CACHE_SIZE");
    char *env_port_mask  = getenv("CFLSH_BLK_PORT_SEL_MASK");
    char *env_timeout = getenv("CFLSH_BLK_TIMEOUT");
    char *env_timeout_units = getenv("CFLSH_BLK_TIMEOUT_UNITS");
    char *env_sigsegv = getenv("CFLSH_BLK_SIGSEGV_DUMP");
    char *env_sigusr1 = getenv("CFLSH_BLK_SIGUSR1_DUMP");
    char *env_dump_level = getenv("CFLSH_BLK_DUMP_LEVEL");
    char *env_notify_log_level = getenv("CFLSH_BLK_NOTIFY_LOG_LEVEL");
#ifdef _SKIP_READ_CALL
    char *env_adap_poll_delay = getenv("CFLSH_BLK_ADAP_POLL_DLY");
#endif /* _SKIP_READ_CALL */
    int rc;

    /*
     * Require that the cflash_cmd_mgm_t structure be a multiple
     * of 64 bytes in size. Since we are looking at the sizeof 
     * of structure the preprocessor #error/#warning directives
     * can not be used.
     */

    CFLASH_COMPILE_ASSERT((sizeof(cflsh_cmd_mgm_t) % CFLASH_BLOCK_CMD_ALIGNMENT) == 0);

    
#ifdef _LINUX_MTRACE
    mtrace();
#endif


    CFLASH_BLOCK_RWLOCK_INIT(cflsh_blk.global_lock);

    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);



    rc = pthread_atfork(cblk_prepare_fork,cblk_parent_post_fork,cblk_child_post_fork);

    if (rc) {
#ifndef _AIX
	fprintf(stderr,"pthread_atfork failed rc = %d, errno = %d\n",rc,errno);
#endif
    }

    cblk_init_mc_interface();
    

    cflsh_blk.eyec = CFLSH_EYEC_CBLK;

    cflsh_blk.next_chunk_starting_lba = 0;

    cflsh_blk.timeout = CAPI_SCSI_IO_TIME_OUT;
    cflsh_blk.timeout_units = CFLSH_G_TO_SEC;
   
    if (env_timeout) {

	cflsh_blk.timeout = atoi(env_timeout);
    } 

    if (env_timeout_units) {

	if (!strcmp(env_timeout_units,"MILLI")) {

	    cflsh_blk.timeout_units = CFLSH_G_TO_MSEC;

	} else if (!strcmp(env_timeout_units,"MICRO")) {

	    cflsh_blk.timeout_units = CFLSH_G_TO_USEC;
	    
	}
    }


    if (env_port_mask) {
	
	/*
	 * If there is an environment variable specifying port
	 * selection mask then use it.
	 */
	cflsh_blk.port_select_mask = atoi(env_port_mask);

    } else {

	/*
	 * Allow both ports to be used
	 */

	cflsh_blk.port_select_mask = 0x3;
    }

    bzero((void *)cflsh_blk.hash,(sizeof(cflsh_chunk_t *) * MAX_NUM_CHUNKS_HASH));

    if (cache_size) {

	cblk_cache_size = atoi(cache_size);
    }
#ifdef _SKIP_READ_CALL


    cflsh_blk.adap_poll_delay = CFLASH_BLOCK_ADAP_POLL_DELAY;
    if (env_adap_poll_delay) {

	/*
	 * A poll delay has been specified.  So use it.
	 */

	cflsh_blk.adap_poll_delay = atoi(env_adap_poll_delay);
    }
#endif /* _SKIP_READ_CALL */

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);



    cblk_setup_trace_files(FALSE);


    if (!cblk_valid_endianess()) {
	
	 CBLK_TRACE_LOG_FILE(1,"This program is compiled for different endianess then the host is is running");
    }


    CBLK_TRACE_LOG_FILE(2,"cflsh_blk = %p",&cflsh_blk);

    if (env_sigsegv) {

	/*
	 * The caller has requested that on SIGSEGV
	 * signals (segmentation fault), that we dump
	 * data just prior the application crash (and
	 * potentially coredump). This will provide
	 * more data to aid in analyzing the core dump.
	 */

	if (cblk_setup_sigsev_dump()) {
	    CBLK_TRACE_LOG_FILE(1,"failed to set up sigsev dump handler");
	}

    }

    if (env_sigusr1) {

	/*
	 * The caller has requested that on SIGUSR1
	 * signals that we dump data.
	 */

	if (cblk_setup_sigusr1_dump()) {
	    CBLK_TRACE_LOG_FILE(1,"failed to set up sigusr1 dump handler");
	}

    }

    if (env_dump_level) {

	if (cblk_setup_dump_file()) {
	    CBLK_TRACE_LOG_FILE(1,"failed to set up dump file");

	} else {

	    cblk_dump_level = atoi(env_dump_level);
	}

    }


    if (env_notify_log_level) {

	cblk_notify_log_level = atoi(env_notify_log_level);

    }

    cflsh_blk.caller_pid = getpid();
    return;
}

/*
 * NAME:        _cflsh_blk_fee
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

void _cflsh_blk_free(void)
{


    if (cflsh_blk.flags & CFLSH_G_SYSLOG) {

	closelog();

    }


    if (num_thread_logs) {

	free(cflsh_blk.thread_logs);
    }

#ifdef _LINUX_MTRACE
    muntrace();
#endif

    cblk_cleanup_mc_interface();

    if (cflsh_blk.process_name) {

	free(cflsh_blk.process_name);
    }


    CBLK_TRACE_LOG_FILE(3,"\nLIBRARY STATISTICS ...");

#ifdef BLOCK_FILEMODE_ENABLED
     CBLK_TRACE_LOG_FILE(3,"FILEMODE");
#endif /* BLOCK_FILEMODE_ENABLED */ 
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    CBLK_TRACE_LOG_FILE(3,"Little Endian");
#else
    CBLK_TRACE_LOG_FILE(3,"Big Endian");
#endif 
        
#ifdef _MASTER_CONTXT
    CBLK_TRACE_LOG_FILE(3,"Master Context");
#else
    CBLK_TRACE_LOG_FILE(3,"No Master Context");
#endif 
    CBLK_TRACE_LOG_FILE(3,"cblk_log_verbosity              0x%x",cblk_log_verbosity);
    CBLK_TRACE_LOG_FILE(3,"flags                           0x%x",cflsh_blk.flags);
    CBLK_TRACE_LOG_FILE(3,"lun_id                          0x%llx",cflsh_blk.lun_id);
    CBLK_TRACE_LOG_FILE(3,"next_chunk_id                   0x%llx",cflsh_blk.next_chunk_id);
    CBLK_TRACE_LOG_FILE(3,"num_active_chunks               0x%x",cflsh_blk.num_active_chunks);
    CBLK_TRACE_LOG_FILE(3,"num_max_active_chunks           0x%x",cflsh_blk.num_max_active_chunks);
    CBLK_TRACE_LOG_FILE(3,"num_bad_chunk_ids               0x%x",cflsh_blk.num_bad_chunk_ids);

    return;

}



#ifdef _AIX

/*
 * NAME:        cflsh_blk_init
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

static void cflsh_blk_init(void)
{

    /*
     * The first thread will invoke this routine, but
     * but subsequent threads must not be allowed to 
     * proceed until the _cflsh_blk_init has completed.
     *
     * NOTE: the use of both a lock and a fetch_and_or
     *       is overkill here. A better solution would be
     *       to find a way to use fetch_and_or and only using
     *       the lock while the _cflsh_blk_init.
     */

    //pthread_mutex_lock(&cblk_init_lock);

    if (fetch_and_or(&cflsh_blk_initialize,1)) {



        /*
         * If cflsh_blk_initialize is set, then
         * we have done all the cflash block code
         * initialization. As result we can return
         * now provided we ensure any thread that is
	 * doing initializatoin for this library
	 * has completed.
         */

	while (!cflsh_blk_init_finish) {

	    usleep(1);

	}

        return;
    }


    /*
     * We get here if the cflash_block code has not been
     * initialized yet. 
     */

    _cflsh_blk_init();
	
    fetch_and_or(&cflsh_blk_init_finish,1);

    //pthread_mutex_unlock(&cblk_init_lock);

    return;
}


#endif /* AIX */






/*
 * NAME:        CBLK_IN_CACHE
 *
 * FUNCTION:    Check if data to be read starting
 *              at lba is in_cache.
 *              If so then copy data to user's buffer 
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
 *              FALSE - Data is not in cache
 *              TRUE  - Data is in cache.
 *              
 *              
 */

inline int CBLK_IN_CACHE(cflsh_chunk_t *chunk,void *buf, cflash_offset_t lba, size_t nblocks)
{
    int rc = FALSE;
    cflsh_cache_line_t *line;
    cflash_offset_t cur_lba, end_lba;
    void              *cur_buf;
    uint64_t           tag;
    uint32_t           inx;
    int                lru;
    int                mru;
    int                e;
    

    if (chunk == NULL) {

	return rc;
    }

    if ((chunk->cache == NULL) ||
	(chunk->cache_size == 0)) {

	return rc;
    }

    end_lba = lba + nblocks;

    for (cur_lba = lba, cur_buf = buf; cur_lba < end_lba; cur_lba++, cur_buf += CAPI_FLASH_BLOCK_SIZE) {
	
	inx = CFLSH_BLK_GETINX (cur_lba,chunk->l2setsz);
	tag = CFLSH_BLK_GETTAG (cur_lba,chunk->l2setsz);
	line = &chunk->cache [inx];

	rc = FALSE;

	for (e = 0; e < CFLSH_BLK_NSET; e++) {
	    if (line->entry[e].valid && line->entry[e].tag == tag) {
		lru = line->lrulist;
		if (lru == e)
		    line->lrulist = line->entry[e].next;
		else {
		    mru = line->entry[lru].prev;
		    if (e != mru) {
			line->entry[line->entry[e].prev].next =
			    line->entry[e].next;
			line->entry[line->entry[e].next].prev =
			    line->entry[e].prev;
			
			line->entry[e].next = lru;
			line->entry[e].prev = mru;
			line->entry[lru].prev = e;
			line->entry[mru].next = e;
		    }
		}
		
		
		bcopy(line->entry[e].data,cur_buf, nblocks * CAPI_FLASH_BLOCK_SIZE);
		chunk->stats.num_cache_hits++;
		rc = TRUE;
		
		break;
	    }
	} /* for */

	if (rc == FALSE) {
	    /*
	     * If any blocks are not in the cache the mark the entire.
	     * request as not being in the cache and force a full re-read of
	     * the data.
	     *
	     * In the future maybe we could improve this to just do the read for 
	     * blocks not in the cache.
	     */
	    break;
	}
    } /* outer for */



    return rc;
}




/*
 * NAME:        CBLK_BUILD_ISSUE_RW_CMD
 *
 * FUNCTION:    Builds and issues a READ16/WRITE16 command
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

inline int CBLK_BUILD_ISSUE_RW_CMD(cflsh_chunk_t *chunk, int *cmd_index, void *buf,cflash_offset_t lba, 
				   size_t nblocks, int flags, int lib_flags,uint8_t op_code,
				   cblk_arw_status_t *status)
{
    int rc = 0;
    int local_flags = 0;
    scsi_cdb_t *cdb = NULL;
    int pthread_rc; 
#ifndef _COMMON_INTRPT_THREAD
    cflsh_async_thread_cmp_t *async_data;
#endif 
    cflsh_cmd_mgm_t *cmd = NULL;

    CFLASH_BLOCK_LOCK(chunk->lock);

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	CFLASH_BLOCK_UNLOCK(chunk->lock);
	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk lba = 0xllx, chunk->index = %d",
			    lba,chunk->index);
	errno = EINVAL;
	return -1;
    }

    if (chunk->flags & CFLSH_CHNK_HALTED) {

	/*
	 * This chunk is in a halted state. Wait
	 * the chunk to be resumed.
	 */
	CBLK_TRACE_LOG_FILE(5,"chunk in halted state  lba = 0x%llx, chunk->index = %d",
			    lba,chunk->index);

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

	pthread_rc = pthread_cond_wait(&(chunk->path[chunk->cur_path]->resume_event),&(chunk->lock.plock));
	
	if (pthread_rc) {
	    



	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed for resume_event rc = %d errno = %d",
				pthread_rc,errno);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    errno = EIO;
	    return -1;
	}
    }

    if (chunk->flags & CFLSH_CHNK_CLOSE) {

	/*
	 * If this chunk is closing then
	 * return EINVAL
	 */
	CBLK_TRACE_LOG_FILE(1,"chunk is closing lba = 0x%llx, chunk->index = %d",
			    lba,chunk->index);
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EINVAL;
	return -1;
    }


    if (chunk->flags & CFLSH_CHUNK_FAIL_IO) {

	/*
	 * This chunk is in a failed state. All
	 * I/O needs to be failed.
	 */
	CBLK_TRACE_LOG_FILE(5,"chunk in failed state  lba = 0x%llx, chunk->index = %d",
			    lba,chunk->index);
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EIO;
	return -1;
    }





    if (chunk->num_blocks < (lba + nblocks)) {
	/*
	 * This request exceeds the capacity of
	 * this chunk.
	 */

	
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CBLK_TRACE_LOG_FILE(5,"Data request is beyond end of chunk: num_blocks = 0x%llx lba = 0x%llx, chunk->index = %d",
			    chunk->num_blocks, lba,chunk->index);
	errno = EINVAL;
	return -1;
    }


	
    if ((op_code == SCSI_READ_16) &&
	(CBLK_IN_CACHE(chunk,buf,lba,nblocks))) {

	/*
	 * Data is in cache.  So no need
	 * to issue requests to hardware.
	 */

	
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CBLK_TRACE_LOG_FILE(5,"Data read from cache rc = %d,errno = %d, chunk->index = %d",
			    rc,errno,chunk->index);

	rc = nblocks;

	if (flags & CBLK_ARW_USER_STATUS_FLAG) {
	    
	    /*
	     * Set user status
	     */

	    status->blocks_transferred = rc;
	    status->status = CBLK_ARW_STATUS_SUCCESS;
	}

	return rc;
    }

    if ((lib_flags & CFLASH_ASYNC_OP) &&
	!(flags & CBLK_ARW_WAIT_CMD_FLAGS)) {

	/*
	 * If this is an async read/write that
	 * did not indicate we should wait for a command
	 * then call cblk_find_free_cmd, indicating
	 * we do not want to wait for a free command.
	 */

        rc = cblk_find_free_cmd(chunk,&cmd,0);

	if (rc) {

	    errno = EWOULDBLOCK;

	    CBLK_TRACE_LOG_FILE(1,"could not find a free cmd, num_active_cmds = %d, errno = %d, chunk->index = %d",
				chunk->num_active_cmds,errno,chunk->index);

	}

    } else {
        rc = cblk_find_free_cmd(chunk,&cmd,CFLASH_WAIT_FREE_CMD);


	if (rc) {

	    errno = EBUSY;

	    CBLK_TRACE_LOG_FILE(1,"could not find a free cmd, num_active_cmds = %d, errno = %d, chunk->index = %d",
				chunk->num_active_cmds,errno,chunk->index);
	}
    }


    if (rc) {


	CBLK_TRACE_LOG_FILE(1,"could not find a free cmd, num_active_cmds = %d, errno = %d, chunk->index = %d",
			    chunk->num_active_cmds,errno,chunk->index);
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	return -1;
    }

    *cmd_index = cmd->index;

    if (flags & CBLK_ARW_USER_STATUS_FLAG) {
	    
	/*
	 * Set user status
	 */
	chunk->cmd_info[cmd->index].flags |= CFLSH_CMD_INFO_USTAT;
	chunk->cmd_info[cmd->index].user_status = status;
	chunk->cmd_info[cmd->index].path_index = chunk->cur_path;

	status->status = CBLK_ARW_STATUS_PENDING;
    }

    if (op_code == SCSI_READ_16) {

	local_flags = CFLASH_READ_DIR_OP;
    } else if (op_code == SCSI_WRITE_16) {

	local_flags = CFLASH_WRITE_DIR_OP;
	
    }

    CBLK_BUILD_ADAP_CMD(chunk,cmd,buf,(CAPI_FLASH_BLOCK_SIZE * nblocks),local_flags);


    cdb = CBLK_GET_CMD_CDB(chunk,cmd);
    cdb->scsi_op_code = op_code;

    
    /*
     * TODO: ?? Currently we are requiring callers of this
     *       interface to to use 4K alignment, but the SCSI
     *       device maybe is using 512 or 4K sectors. However
     *       we will only officially support 4K sectors when this
     *       code ships. So this current code should only be needed
     *       internally.
     */
    
    
    CFLASH_BUILD_RW_16(cdb,(chunk->start_lba + lba)*(chunk->blk_size_mult),nblocks*(chunk->blk_size_mult));

#ifndef _COMMON_INTRPT_THREAD
    if (lib_flags & CFLASH_ASYNC_OP) {

	/*
	 * If this is an async read/write then we need to
	 * start a thread to wait for its completion, since the
	 * caller of this routine expects to issue the request and
	 * later collect the status.
	 *
	 * NOTE:
	 *
	 * We need to start the async completion thread now, because if it
	 * fails we can inform the user (via an errno EAGAIN) that we
	 * could not issue the request now before we issue the request
	 * to the adapter. If we waited to start this thread after
	 * we issued the request to the adapter, and the thread create
	 * failed, we could not block and wait for the command to complete.
	 * Since there is no mechanism to abort AFU commands, we would have no
	 * mechanism to properly clean up. The caller of this routine might think
	 * the issuing of the command failed (due to error seen for the the pthread_create),
	 * and never check back for completion. Furthermore since the AFU is now
	 * handling the request it could DMA data into the users buffers, but the
	 * user may believe the buffers are available for reuse (thus causing 
	 * data corruption). Since we can cancel a thread, we can call
	 * pthread_cancel if we faile to issue the command to the adapter.
	 *
	 */

	async_data = &(cmd->cmdi->async_data);
	async_data->chunk = chunk;
	async_data->cmd_index = cmd->index;
	
	pthread_rc = pthread_create(&(cmd->cmdi->thread_id),NULL,cblk_async_recv_thread,async_data);
	
	if (pthread_rc) {
	    
	    chunk->stats.num_failed_threads++;
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_create failed rc = %d,errno = %d, cmd_index = %d,lba = 0x%llx, num_active_cmds = 0x%x, num_active_threads = 0x%x, chunk->index = %d",
				pthread_rc,errno, cmd->index,chunk->start_lba + lba,chunk->num_active_cmds,chunk->stats.num_active_threads,chunk->index);
	    
	    CBLK_FREE_CMD(chunk,cmd);

	    errno = EAGAIN;
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return -1;
	}

	/*
	 * We successfully started the thread.
	 * Update statistics reflecting this.
	 */
	
	chunk->stats.num_success_threads++;

	chunk->stats.num_active_threads++;

	chunk->stats.max_num_act_threads = MAX(chunk->stats.max_num_act_threads,chunk->stats.num_active_threads);
	
    }

#endif /* !_COMMON_INTRPT_THREAD */

    if (CBLK_ISSUE_CMD(chunk,cmd,buf,chunk->start_lba + lba,nblocks,0)) {

        /*
	 * Failed to issue the request. Let's clean up
	 * and fail this request.
	 */

#ifndef _COMMON_INTRPT_THREAD
	if (lib_flags & CFLASH_ASYNC_OP) {

	    /*
	     * If this is an async I/O then we just
	     * created a thread to wait for its completion.
	     * Since we failed here, we need to cancel
	     * that thread now. This can be some what
	     * tricky. First our async receive completion,
	     * will be cancelable (default), but with the 
	     * a deferred type (default). Thus when we cancel
	     * it, the thread will only be canceled at 
	     * cancelation points. The code for that thread
	     * was written to create safe cancelation points 
	     * (i.e. calls to pthread_testcancel) where it
	     * should not be holding any resources-like locks.
	     * As a result these cancelation points alone have some
	     * limitations (for example if the thread is waiting
	     * on a signal from us, we would would first need
	     * to signal it so that it could wake up. In 
	     * addition since it would be locked on chunk->lock,
	     * it would be blocked from proceeding to a cancelation
	     * point until after we unlocked here.  
	     *
	     * To resolve these issues, we will signal here
	     * (the same as we would in the case where
	     * CBLK_ISSUE_CMD succeeded) to wake it up.
	     * and then unlock.  We will also set the
	     * CFLSH_ASYNC_IO_SNT prior to signaling and unlocking.
	     * Thus if it has not reached that point in the code
	     * yet will not sleep and proceed to unlocking
	     * and reaching a cancelation point.
	     */


	    cmd->cmdi->flags |= CFLSH_ASYNC_IO_SNT;

	    pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
	
	    if (pthread_rc) {
	    
		CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
				pthread_rc,errno,chunk->index);
	    }



	    pthread_rc = pthread_cancel(cmd->cmdi->thread_id);

	    if (pthread_rc) {
		
		chunk->stats.num_fail_canc_threads++;
		
		CBLK_TRACE_LOG_FILE(5,"pthread_cancel failed rc = %d,errno = %d, chunk->index = %d",
				    pthread_rc,errno,chunk->index);
	    } else {
		chunk->stats.num_canc_threads++;

		/*
		 * Since the pthread_ancel will only
		 * effect this thread at cancelation
		 * points, it is possible that it maybe
		 * waiting on the chunk->lock or the
		 * thread event signal above (actually
		 * because of the way this is currently
		 * implemented it should not be waiting on
		 * thread event, since we have not yet released
		 * the chunk->lock since creating this thread.
		 * Thus at most it could be waiting on the
		 * chunk->lock, However in the future 
		 * this may change. So we will assume 
		 * it is possible now and handle
		 * that case too). 
		 *
		 * So we need to unlock here so that
		 * it can wake up and proceed to
		 * a cancellation point. This should
		 * be ok, since we have not yet
		 * cleared cmd->cmdi->in_use. Thus the command
		 * can not be reused. If we did not wake up
		 * here we could hang on the pthread_join,
		 * if that thread was waiting on the
		 * chunk->lock.
		 */
		CFLASH_BLOCK_UNLOCK(chunk->lock);
		pthread_join(cmd->cmdi->thread_id,NULL);
		CFLASH_BLOCK_LOCK(chunk->lock);

		cmd->cmdi->thread_id = 0;

	    }


	}
	
#endif /* !_COMMON_INTRPT_THREAD */

	CBLK_FREE_CMD(chunk,cmd);

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CBLK_TRACE_LOG_FILE(5,"CBLK_ISSUE_CMD failed rc = %d,errno = %d, chunk->index = %d",
			    rc,errno,chunk->index);
        return -1;
    }

    if (lib_flags & CFLASH_ASYNC_OP) {
	/*
	 * Update async I/O statistics
	 */
      
        cmd->cmdi->flags |= CFLSH_ASYNC_IO;

	if (op_code == SCSI_READ_16) {
	    chunk->stats.num_areads++;
	    chunk->stats.num_act_areads++;

	    chunk->stats.max_num_act_areads = MAX(chunk->stats.num_act_areads,chunk->stats.max_num_act_areads);
	} else if (op_code == SCSI_WRITE_16) {
	    chunk->stats.num_awrites++;
	    chunk->stats.num_act_awrites++;

	    chunk->stats.max_num_act_awrites = MAX(chunk->stats.num_act_awrites,chunk->stats.max_num_act_awrites);

	}
	
	/*
	 * If this is an async I/O then we just
	 * created a thread to wait for its completion.
	 * Since we just successfully issue the command to the
	 * AFU, we need to wakeup the async thread waiting for 
	 * it to complete.
	 *
	 * We need to set the CFLSH_ASYNC_IO_SNT so that
	 * the receiver of this signal know we have just
	 * sent the signal and it should not do
	 * a pthread_cond_wait. We need to be locked
	 * here when we set this flag and send the signal
	 * to prevent the receiver from hanging.
	 */

#ifndef _COMMON_INTRPT_THREAD
	cmd->cmdi->flags |= CFLSH_ASYNC_IO_SNT;

	pthread_rc = pthread_cond_signal(&(cmd->cmdi->thread_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_signall failed rc = %d,errno = %d, chunk->index = %d",
				pthread_rc,errno,chunk->index);
	}
#endif /* COMMON_INTRPT_THREAD */

    } else {
	/*
	 * Update sync I/O statistics
	 */
	if (op_code == SCSI_READ_16) {
	    chunk->stats.num_reads++;
	    chunk->stats.num_act_reads++;

	    chunk->stats.max_num_act_reads = MAX(chunk->stats.num_act_reads,chunk->stats.max_num_act_reads);
	} else if (op_code == SCSI_WRITE_16) {
	    chunk->stats.num_writes++;
	    chunk->stats.num_act_writes++;

	    chunk->stats.max_num_act_writes = MAX(chunk->stats.num_act_writes,chunk->stats.max_num_act_writes);

	}
    }



    if (op_code == SCSI_READ_16) {
	    
        cmd->cmdi->flags |= CFLSH_MODE_READ;
	    
    } else if (op_code == SCSI_WRITE_16) {

        cmd->cmdi->flags |= CFLSH_MODE_WRITE;

    }


    /*
     * Unlock since we are waiting for 
     * a completions. Other threads may
     * also be doing doing I/O to this chunk
     * too.
     */


    CFLASH_BLOCK_UNLOCK(chunk->lock);


    return rc;
}



/*
 * NAME:        cblk_init
 *
 * FUNCTION:    Initializes library resources.
 *
 *
 * INPUTS:	NONE
 *              
 *
 * RETURNS:	NONE
 *              
 */

int cblk_init(void *arg,uint64_t flags)
{

    /*
     * Today, this routine is a no-op, but it provides
     * future expandability options (such as shared segments
     * per context, should that ever be needed.
     */


    return 0;
}
/*
 * NAME:        cblk_term
 *
 * FUNCTION:    Free library resources.
 *
 *
 * INPUTS:	NONE
 *              
 *
 * RETURNS:	NONE
 *              
 */

int cblk_term(void *arg,uint64_t flags)
{
#ifdef _AIX
    if (fetch_and_or(&cflsh_blk_term,1)) {



        /*
         * If cflsh_blk_term is set, then
         * we have done all the cflash block code
         * cleanup. As result we can return
         * now.
         */
        return 0;
    }

    /*
     * We get here if the cflash_block code has not been
     * terminated yet. 
     */

    _cflsh_blk_free();

#endif /* AIX */
    return 0;
}






/*
 * NAME:        cblk_open
 *
 * FUNCTION:    Opens a handle for a CAPI flash lun to
 *              a contiguous set of blocks (chunk).
 *
 *
 * INPUTS:
 *              path - Path of device to open
 *          
 *              flags - Flags for open
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 *              
 */

chunk_id_t cblk_open(const char *path, int max_num_requests, int mode, chunk_ext_arg_t ext, int flags)
{
    chunk_id_t ret_chunk_id = NULL_CHUNK_ID;
    int open_flags;
    int cleanup_depth;
    cflsh_chunk_t *chunk = NULL;

#ifdef _AIX
    int ext_flags = 0;
#endif /* _AIX */
    errno = 0;



#ifdef _AIX
    cflsh_blk_init();
#endif /* AIX */


    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);

    CBLK_TRACE_LOG_FILE(5,"opening %s with max_num_requests = %d, mode = 0x%x, flags = 0x%x for pid = 0x%llx",
			path,max_num_requests,mode,flags,(uint64_t)cflsh_blk.caller_pid);


    if (strlen(path) > PATH_MAX) {

        CBLK_TRACE_LOG_FILE(1,"opening failed because filename too long");

	errno = EINVAL;
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	return ret_chunk_id;
    }


    ret_chunk_id = cblk_get_chunk(CFLSH_BLK_CHUNK_SET_UP, max_num_requests);

    chunk = CBLK_GET_CHUNK_HASH(ret_chunk_id,FALSE);

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    if (chunk) {



	CFLASH_BLOCK_LOCK(chunk->lock);

	switch (mode & O_ACCMODE) {
	case O_RDONLY:
	    chunk->flags |= CFLSH_CHNK_RD_AC;
	    break;
	case O_WRONLY:
	    chunk->flags |= CFLSH_CHNK_WR_AC;
	    break;
	case O_RDWR:
	    chunk->flags |= CFLSH_CHNK_RD_AC | CFLSH_CHNK_WR_AC;
	    break;
	default:

	    errno = EINVAL;

	    CBLK_TRACE_LOG_FILE(1,"Invalid access mode %d",mode);

	    cblk_chunk_open_cleanup(chunk,20);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;
	    
	}

	/*
	 * For now set the starting LBA of
	 * this chunk to be the same as
	 * the physical LBA. For virtual
	 * luns, the caller is supposed to do 
	 * a set size operation, which at that
	 * time we will set the chunk's
	 * actual starting lba.
	 */

	chunk->start_lba = 0;


	if (flags & CBLK_OPN_VIRT_LUN) {

	    /*
	     * This is a virtual lun,
	     * not a physical lun.
	     */
	    chunk->flags |= CFLSH_CHNK_VLUN;
	    
	    if (flags & CBLK_OPN_SCRUB_DATA) {
		
		chunk->flags |= CFLSH_CHNK_VLUN_SCRUB;
	    }
	} 



	if (flags & CBLK_OPN_NO_INTRP_THREADS) {

	    /*
	     * This chunk is not allowed to use
	     * back ground threads.
	     */

	    chunk->flags |= CFLSH_CHNK_NO_BG_TD;
	}

	/*
	 * Don't use libafu
	 */

	strcpy(chunk->dev_name,path);

#ifdef BLOCK_FILEMODE_ENABLED
	open_flags = mode & O_ACCMODE;  

	if (!strncmp("/dev/",path,5)) {


	    errno = EINVAL;

	    CBLK_TRACE_LOG_FILE(1,"Can not use device special files for file mode");
	    perror("cblk_open: Can not use device special files for file mode");

	    cblk_chunk_open_cleanup(chunk,20);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;
	}
#else


	if (strncmp("/dev/",path,5)) {


	    errno = EINVAL;

	    CBLK_TRACE_LOG_FILE(1,"Can not use non device special files for real block mode");


	    cblk_chunk_open_cleanup(chunk,20);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;
	}


#endif 

	if (flags & CBLK_OPN_SHARE_CTXT) {

	    chunk->flags |= CFLSH_CHNK_SHARED;
	}


#ifdef _AIX

	open_flags =  (mode & O_ACCMODE) | O_NONBLOCK;   

	ext_flags |=  SC_CAPI_USER_IO;

	if ((flags & (CBLK_OPN_RESERVE|CBLK_OPN_FORCED_RESERVE)) &&
	    (flags & CBLK_OPN_MPIO_FO)) {


	    errno = EINVAL;

	    CBLK_TRACE_LOG_FILE(1,"Can not use MPIO with reservations");


	    cblk_chunk_open_cleanup(chunk,20);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;

	}


	if (!(flags & CBLK_OPN_RESERVE) &&
	    !(flags & CBLK_OPN_FORCED_RESERVE)) {


	    chunk->flags |= CFLSH_CHNK_NO_RESRV;
	    ext_flags |= SC_NO_RESERVE;
	}

	if (flags & CBLK_OPN_FORCED_RESERVE) {

	    ext_flags |= SC_FORCED_OPEN_LUN;
	}


	if (flags & CBLK_OPN_MPIO_FO) {


	    chunk->flags |= CFLSH_CHNK_MPIO_FO;
	}



	chunk->fd = openx(chunk->dev_name,open_flags,0,ext_flags);
#else
	
	open_flags = O_RDWR | O_NONBLOCK;   

	chunk->fd = open(chunk->dev_name,open_flags);
#endif /* !_AIX */
    
	if (chunk->fd < 0) {

	    CBLK_TRACE_LOG_FILE(1,"Unable to open device errno = %d",errno);
	    perror("cblk_open: Unable to open device");

	    cblk_chunk_open_cleanup(chunk,20);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;
	}

	cleanup_depth = 30;

	if (cblk_chunk_attach_process_map(chunk,mode,&cleanup_depth)) {

	    CBLK_TRACE_LOG_FILE(1,"Unable to attach errno = %d",errno);

	    cblk_chunk_open_cleanup(chunk,cleanup_depth);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;

	}
    


	cleanup_depth = 40;

#ifdef _COMMON_INTRPT_THREAD

	
	if (cblk_start_common_intrpt_thread(chunk)) {


	    CBLK_TRACE_LOG_FILE(1,"cblk_start_common_intrpt thread failed with errno= %d",
				errno);

	    
	    cblk_chunk_open_cleanup(chunk,cleanup_depth);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);


	    return NULL_CHUNK_ID;
	}

	cleanup_depth = 45;
#endif 

	if (cblk_chunk_get_mc_device_resources(chunk,&cleanup_depth)) {
	    

	    CBLK_TRACE_LOG_FILE(5,"cblk_get_device_info failed errno = %d",
				errno);
	    
	    cblk_chunk_open_cleanup(chunk,cleanup_depth);
	    
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
	    free(chunk);
	    

	    return NULL_CHUNK_ID;

	}

	if (!(chunk->flags & CFLSH_CHNK_VLUN)) {

	    cblk_chunk_init_cache(chunk,chunk->num_blocks);


	}

	/*
	 * This chunk is ready for use.
	 */
	chunk->flags |= CFLSH_CHNK_RDY;

    	CFLASH_BLOCK_UNLOCK(chunk->lock);
    }


    CBLK_TRACE_LOG_FILE(5,"opening for %s, returned rc = %d",path,ret_chunk_id);

    return ret_chunk_id;
}

/*
 * NAME:        cblk_close
 *
 * FUNCTION:    Closes a chunk. 
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *
 *              flags       - Flags for close
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_close(chunk_id_t chunk_id, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;
    int loop_cnt = 0;

    errno = 0;

    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }


    CBLK_TRACE_LOG_FILE(5,"closing  chunk_id = %d",chunk_id);


    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);

    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"closing failed because chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    if (chunk->in_use == FALSE) {

	CBLK_TRACE_LOG_FILE(1,"closing failed because chunk not in use, rchunk_id = %d, path = %s",
			    chunk_id,chunk->dev_name);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_LOCK(chunk->lock);

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }

    CBLK_TRACE_LOG_FILE(9,"closing  chunk->dev_name = %s",chunk->dev_name);

    if (chunk->flags & CFLSH_CHNK_CLOSE) {

	CBLK_TRACE_LOG_FILE(1,"Trying to close chunk that someone else is also closing, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }


    chunk->flags |= CFLSH_CHNK_CLOSE;

    while ((chunk->num_active_cmds > 0) &&
	   (loop_cnt < CFLSH_BLK_WAIT_CLOSE_RETRIES)) {

	/*
	 * Wait for a limited time for 
	 * active commands to complete. 
	 * Unlock when we are sleeping.
	 */
	
	CFLASH_BLOCK_UNLOCK(chunk->lock);
        CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

	usleep(CFLSH_BLK_WAIT_CLOSE_DELAY);

        CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);
	CFLASH_BLOCK_LOCK(chunk->lock);
        loop_cnt++;
	

    }


    if (chunk->num_active_cmds > 0) {


	/*
	 * If this chunk still has active
	 * commands then fails this
	 * close.
	 */

	CBLK_TRACE_LOG_FILE(1,"closing failed because chunk in use, rchunk_id = %d, path = %s, num_active_cmds %d",
			    chunk_id,chunk->dev_name,chunk->num_active_cmds);
	

	chunk->flags &= ~CFLSH_CHNK_CLOSE;

	cblk_display_stats(chunk,3);
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EBUSY;
	return -1;
    }


    if (chunk->cache) {

	cblk_chunk_free_cache(chunk);

    }

    /*
     * Indicate chunk is not ready for use
     */

    chunk->flags &= ~CFLSH_CHNK_RDY;

    cblk_display_stats(chunk,3);

    cblk_chunk_open_cleanup(chunk,50);

    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    
    free(chunk);
    

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);


    CBLK_TRACE_LOG_FILE(5,"closing  chunk_id = %d returned rc = %d",chunk_id, rc);
    return rc;
}

/*
 * NAME:        cblk_get_lun_size
 *
 * FUNCTION:    Returns the number of blocks
 *              associated with the physical lun that 
 *              contains this chunk
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Address of returned number of 
 *                            blocks for this lun
 *              flags       - Flags for open
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_get_lun_size(chunk_id_t chunk_id, size_t *nblocks, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;


    errno = 0;
    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }

    if (nblocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);

    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    *nblocks = chunk->num_blocks_lun;

    CBLK_TRACE_LOG_FILE(5,"get_lun_size returned = 0x%llx, rc = %d",(uint64_t)(*nblocks), rc);
    return rc;
}


/*
 * NAME:        cblk_get_size
 *
 * FUNCTION:    Returns the number of blocks
 *              associated with this chunk. 
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Address of returned number of 
 *                            blocks for this chunk.
 *              flags       - Flags for open
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_get_size(chunk_id_t chunk_id, size_t *nblocks, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;


    errno = 0;
    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }

    if (nblocks == NULL) {

	errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);

    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    *nblocks = chunk->num_blocks;

    CBLK_TRACE_LOG_FILE(5,"get_size returned = 0x%llx, rc = %d",(uint64_t)(*nblocks), rc);


    return rc;
}


/*
 * NAME:        cblk_set_size
 *
 * FUNCTION:    Specifies the number of blocks
 *              associated with this chunk. If blocks are already
 *              associated then this request can decrease/increase
 *              them.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Number of blocks for this chunk.
 *              flags       - Flags for open
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_set_size(chunk_id_t chunk_id, size_t nblocks, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;



    errno = 0;
    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }


    CBLK_TRACE_LOG_FILE(5,"set_size size = 0x%llx",(uint64_t)nblocks);

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);

    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

   
    CFLASH_BLOCK_LOCK(chunk->lock);

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    if (!(chunk->flags & CFLSH_CHNK_VLUN)) {


	CBLK_TRACE_LOG_FILE(1,"set_size failed with EINVAL no VLUN");
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EINVAL;
	return -1;
    }
    

#if defined(_KERNEL_MASTER_CONTXT) || defined(BLOCK_FILEMODE_ENABLED)
    if (nblocks > chunk->num_blocks_lun) {


	CBLK_TRACE_LOG_FILE(1,"set_size failed with EINVAL, nblocks = 0x%llx, num_blocks_lun = 0x%llx",
			    (uint64_t)nblocks,(uint64_t)chunk->num_blocks_lun);
	CBLK_TRACE_LOG_FILE(1,"set_size failed with EINVAL, nblocks = 0x%llx",nblocks);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EINVAL;
	return -1;
    }
#endif	/* !_MASTER_CONTXT */

    
    CBLK_TRACE_LOG_FILE(9,"set size for chunk->dev_name = %s",chunk->dev_name);


    rc = cblk_chunk_set_mc_size(chunk,nblocks);

    if (rc) {


	CFLASH_BLOCK_UNLOCK(chunk->lock);

	return -1;
    }


    /*
     * If we got this far, then the we succeeded in
     * getting the space requested.
     */



    if (chunk->cache) {

	cblk_chunk_free_cache(chunk);

    }


    cblk_chunk_init_cache(chunk,nblocks);

    CFLASH_BLOCK_UNLOCK(chunk->lock);

    return rc;
}


/*
 * NAME:        cblk_read
 *
 * FUNCTION:    Reads data from the specified offset in the chunk
 *              and places that data in the specified buffer.
 *              This request is a blocking read request (i.e.
 *              it will not return until either data is read or
 *              an error is encountered).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_read(chunk_id_t chunk_id,void *buf,cflash_offset_t lba, size_t nblocks, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;
    int cmd_index = 0;
    size_t transfer_size = 0;
#ifdef _COMMON_INTRPT_THREAD
    cflsh_cmd_mgm_t *cmd = NULL;
    int pthread_rc;
#endif



    errno = 0;
    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, lba = 0x%llx, nblocks = 0x%llx, buf = %p",chunk_id,lba,(uint64_t)nblocks,buf);


    if (CBLK_VALIDATE_RW(chunk_id,buf,lba,nblocks)) {

	return -1;
    }


    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);


    if (nblocks > chunk->stats.max_transfer_size) {


	CBLK_TRACE_LOG_FILE(1,"nblocks too large = 0x%llx",nblocks);

	errno = EINVAL;
	return -1;
    }

    if (!(chunk->flags & CFLSH_CHNK_RD_AC)) {


	CBLK_TRACE_LOG_FILE(1,"chunk does not have read access",nblocks);

	errno = EINVAL;
	return -1;

    }


    rc = CBLK_BUILD_ISSUE_RW_CMD(chunk,&cmd_index,buf,lba,nblocks,flags,0,SCSI_READ_16,NULL);


    if (rc) {
	
	return rc;
    }

#ifndef _COMMON_INTRPT_THREAD

    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&cmd_index,&transfer_size,TRUE,0);


#else     

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&cmd_index,&transfer_size,TRUE,0);

    } else {

	if ((cmd_index >= chunk->num_cmds) ||
	    (cmd_index < 0)) {
	
	    errno = EINVAL;

	    rc = -1;
	    CBLK_TRACE_LOG_FILE(1,"Invalid cmd_index = 0x%x",cmd_index);

	    return rc;
	}


	CFLASH_BLOCK_LOCK(chunk->lock);

	if (CFLSH_EYECATCH_CHUNK(chunk)) {
	    /*
	     * Invalid chunk. Exit now.
	     */

	    cflsh_blk.num_bad_chunk_ids++;
	    CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
				chunk_id);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    errno = EINVAL;
	    return -1;
	}

	cmd = &(chunk->cmd_start[cmd_index]);

	if (cmd->cmdi == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"null cmdi for cmd_index = 0x%x",cmd_index);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    errno = EINVAL;

	    rc = -1;

	    return rc;

	}


	if (cmd->cmdi->state != CFLSH_MGM_CMP) {
    
	    /*
	     * Only wait if the cmd is not completed.
	     */
	    pthread_rc = pthread_cond_wait(&(cmd->cmdi->thread_event),&(chunk->lock.plock));
	
	    if (pthread_rc) {
	    



		CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed cmd_index = 0x%x rc = %d errno = %d",
				    cmd->index,pthread_rc,errno);
	    } else {


		rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);

	    
	    }
	
	} else {

	    rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);
	}
    
	CFLASH_BLOCK_UNLOCK(chunk->lock);
    }
    
#endif

    if (!rc) {

	/*
	 * For good completion, indicate we read
	 * all the data requested.
	 */
	rc = transfer_size;
    }

    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx",rc,errno,lba);


    return rc;
}
  
/*
 * NAME:        cblk_write
 *
 * FUNCTION:    Writes data to the specified offset in the chunk
 *              from the specified buffer. This request is
 *              a blocking write request (i.e.it will not 
 *              return until either data is written or
 *              an error is encountered).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_write(chunk_id_t chunk_id,void *buf,cflash_offset_t lba, size_t nblocks, int flags)
{
    int rc = 0;
    cflsh_chunk_t *chunk;
    int cmd_index = 0;
    size_t transfer_size = 0;
#ifdef _COMMON_INTRPT_THREAD
    cflsh_cmd_mgm_t *cmd = NULL;
    int pthread_rc;
#endif


    errno = 0;
    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, lba = 0x%llx, nblocks = 0x%llx, buf = %p",chunk_id,lba,(uint64_t)nblocks,buf);

    if (CBLK_VALIDATE_RW(chunk_id,buf,lba,nblocks)) {

	return -1;
    }


    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);


    if (nblocks > chunk->stats.max_transfer_size) {

	CBLK_TRACE_LOG_FILE(1,"nblocks too large = 0x%llx",nblocks);

	errno = EINVAL;
	return -1;
    }

    if (!(chunk->flags & CFLSH_CHNK_WR_AC)) {


	CBLK_TRACE_LOG_FILE(1,"chunk does not have write access",nblocks);

#if defined(_AIX)
	errno = EWRPROTECT;
#else
	errno = EINVAL;
#endif /* !defined(_AIX) && !defined(_MACOSX) */

	return -1;

    }

    rc = CBLK_BUILD_ISSUE_RW_CMD(chunk,&cmd_index,buf,lba,nblocks,flags,0,SCSI_WRITE_16,NULL);


    if (rc) {

	return rc;
    }


#ifndef _COMMON_INTRPT_THREAD

    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&cmd_index,&transfer_size,TRUE,0);

#else     

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,&cmd_index,&transfer_size,TRUE,0);

    } else {

	if ((cmd_index >= chunk->num_cmds) ||
	    (cmd_index < 0)) {
	
	    errno = EINVAL;

	    rc = -1;
	    CBLK_TRACE_LOG_FILE(1,"Invalid cmd_index = 0x%x",cmd_index);

	    return rc;
	}


	CFLASH_BLOCK_LOCK(chunk->lock);

	if (CFLSH_EYECATCH_CHUNK(chunk)) {
	    /*
	     * Invalid chunk. Exit now.
	     */
	
	    cflsh_blk.num_bad_chunk_ids++;
	    CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
				chunk_id);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    errno = EINVAL;
	    return -1;
	}

	cmd = &(chunk->cmd_start[cmd_index]);

	if (cmd->cmdi == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"null cmdi for cmd_index = 0x%x",cmd_index);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    errno = EINVAL;

	    rc = -1;

	    return rc;

	}

    
	if (cmd->cmdi->state != CFLSH_MGM_CMP) {
    
	    /*
	     * Only wait if the command is not completed.
	     */
	    pthread_rc = pthread_cond_wait(&(cmd->cmdi->thread_event),&(chunk->lock.plock));
	
	    if (pthread_rc) {
	    

		CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed cmd_index = 0x%x rc = %d errno = %d",
				    cmd->index,pthread_rc,errno);
	    } else {


		rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);

	    
	    }
	
	} else {

	    rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);
	}
    
	CFLASH_BLOCK_UNLOCK(chunk->lock);
    }
    
#endif

    if (!rc) {

	/*
	 * For good completion, indicate we read
	 * all the data requested.
	 */
	rc = transfer_size;
    }
     
    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx",rc,errno,lba);


    return rc;
}
  

/*
 * NAME:        _cblk_aread
 *
 * FUNCTION:    Internal implementation of async read
 *
 *
 * INPUTS:
 *              chunk       - Chunk associated with operation
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0   for good completion,  
 *              >0  Read data was in cache and was read.
 *              -1 for error with ERRNO set
 *              
 */

static inline int _cblk_aread(cflsh_chunk_t *chunk,void *buf,cflash_offset_t lba, size_t nblocks, int *tag, 
	       cblk_arw_status_t *status, int flags)
{
    int rc = 0;
    int cmd_index = 0;


    
    if (nblocks > chunk->stats.max_transfer_size) {

	CBLK_TRACE_LOG_FILE(1,"nblocks too large = 0x%llx",nblocks);

	errno = EINVAL;
	return -1;
    }

    if (!(chunk->flags & CFLSH_CHNK_RD_AC)) {


	CBLK_TRACE_LOG_FILE(1,"chunk does not have read access",nblocks);

	errno = EINVAL;
	return -1;

    }

    if ((flags & CBLK_ARW_USER_STATUS_FLAG) &&
	(status == NULL)) {


	CBLK_TRACE_LOG_FILE(1,"status field is NULL");

	errno = EINVAL;
	return -1;
    }


    if ((flags & CBLK_ARW_USER_STATUS_FLAG) &&
	(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {

	/*
	 * If this chunk was opened with no background
	 * thread, then the caller can not expect
	 * to use a user specified status field to
	 * be updated/notified when a command is complete.
	 */

	errno = EINVAL;
	    
	return -1;
    }
    /* 
     * NOTE: If data was read from the cache, then the rc 
     *       from CBLK_BUILD_ISSUE_RW_CMD will be greater
     *       than 0. If CBLK_BUILD_ISSUE_RW_CMD fails, then
     *       rc = -1.
     */
    rc = CBLK_BUILD_ISSUE_RW_CMD(chunk,&cmd_index,buf,lba,nblocks,flags,CFLASH_ASYNC_OP,SCSI_READ_16,status);

    if (rc) {

	
	CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx",rc,errno,lba);

	return rc;
    } else {

	/*
	 * TODO:?? Is this the best solution to set
	 *         user defined tag?
	 */

	if (!(flags & CBLK_ARW_USER_TAG_FLAG)) {
	
	    *tag = cmd_index;
	} else {

	    /*
	     * Set user defined tag
	     */

	    chunk->cmd_info[cmd_index].flags |= CFLSH_CMD_INFO_UTAG;
	    chunk->cmd_info[cmd_index].user_tag = *tag;
	}


    }

    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx, tag = 0x%x",rc,errno,lba,*tag);

    return rc;

}


/*
 * NAME:        cblk_aread
 *
 * FUNCTION:    Reads data from the specified offset in the chunk
 *              and places that data in the specified buffer.
 *              This request is an asynchronous read request (i.e.
 *              it will return as soon as it has issued the request
 *              to the device. It will not wait for a response from 
 *              the device.).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0   for good completion,  
 *              >0  Read data was in cache and was read.
 *              -1 for error with ERRNO set
 *              
 */

int cblk_aread(chunk_id_t chunk_id,void *buf,cflash_offset_t lba, size_t nblocks, int *tag, 
	       cblk_arw_status_t *status, int flags)
{
    cflsh_chunk_t *chunk;

    errno = 0;

    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, lba = 0x%llx, nblocks = 0x%llx, buf = %p",chunk_id,lba,(uint64_t)nblocks,buf);

    if (CBLK_VALIDATE_RW(chunk_id,buf,lba,nblocks)) {

	return -1;
    }
    
    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    return (_cblk_aread(chunk,buf,lba,nblocks,tag,status,flags));
}


/*
 * NAME:        _cblk_awrite
 *
 * FUNCTION:    Internal implementation of async write
 *
 *
 * INPUTS:
 *              chunk       - Chunk associated with operation
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

static inline int _cblk_awrite(cflsh_chunk_t *chunk,void *buf,cflash_offset_t lba, size_t nblocks, int *tag, 
		cblk_arw_status_t *status, int flags)
{
    int rc = 0;
    int cmd_index = 0;

    if (nblocks > chunk->stats.max_transfer_size) {

	CBLK_TRACE_LOG_FILE(1,"nblocks too large = 0x%llx",nblocks);

	errno = EINVAL;
	return -1;
    }

    if (!(chunk->flags & CFLSH_CHNK_WR_AC)) {


	CBLK_TRACE_LOG_FILE(1,"chunk does not have write access",nblocks);

#if defined(_AIX)
	errno = EWRPROTECT;
#else
	errno = EINVAL;
#endif /* !defined(_AIX) && !defined(_MACOSX) */
	return -1;

    }

    if ((flags & CBLK_ARW_USER_STATUS_FLAG) &&
	(status == NULL)) {


	CBLK_TRACE_LOG_FILE(1,"status field is NULL");

	errno = EINVAL;
	return -1;
    }

    if ((flags & CBLK_ARW_USER_STATUS_FLAG) &&
	(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {

	/*
	 * If this chunk was opened with no background
	 * thread, then the caller can not expect
	 * to use a user specified status field to
	 * be updated/notified when a command is complete.
	 */

	errno = EINVAL;
	    
	return -1;
    }

    rc = CBLK_BUILD_ISSUE_RW_CMD(chunk,&cmd_index,buf,lba,nblocks,flags,CFLASH_ASYNC_OP,SCSI_WRITE_16,status);

    if (rc) {

	CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx",rc,errno,lba);

	return rc;
    } else {

	/*
	 * TODO:?? Is this the best solution to set
	 *         user defined tag?
	 */

	if (!(flags & CBLK_ARW_USER_TAG_FLAG)) {
	
	    *tag = cmd_index;
	} else {

	    /*
	     * Set user defined tag
	     */

	    chunk->cmd_info[cmd_index].flags |= CFLSH_CMD_INFO_UTAG;
	    chunk->cmd_info[cmd_index].user_tag = *tag;
	}

    }


    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d lba = 0x%llx, tag = 0x%x",rc,errno,lba,*tag);
    return rc;
}



/*
 * NAME:        cblk_awrite
 *
 * FUNCTION:    Writes data to the specified offset in the chunk
 *              from the specified buffer. This request is
 *              an asynchronous write request (i.e.it will
 *              return as soon as it has issued the request
 *              to the device. It will not wait for a response from 
 *              the device.).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_awrite(chunk_id_t chunk_id,void *buf,cflash_offset_t lba, size_t nblocks, int *tag, 
		cblk_arw_status_t *status, int flags)
{

    cflsh_chunk_t *chunk;


    errno = 0;

    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, lba = 0x%llx, nblocks = 0x%llx, buf = %p",chunk_id,lba,(uint64_t)nblocks,buf);


    if (CBLK_VALIDATE_RW(chunk_id,buf,lba,nblocks)) {

	return -1;
    }


    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }


    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    return (_cblk_awrite(chunk,buf,lba,nblocks,tag,status,flags));
}
  

/*
 * NAME:        _cblk_aresult
 *
 * FUNCTION:    Internal implementation of async result
 *
 *
 * INPUTS:
 *              chunk       - Chunk associated with operation
 *              tag         - Tag associated with this request that
 *                            completed.
 *              flags       - Flags on this request.
 *              harvest     - 0 to skip harvest call, else do harvest
 *
 * RETURNS:
 *              0   for good completion, but requested tag has not yet completed.
 *              -1  for  error and errno is set.
 *              > 0 for good completion where the return value is the
 *                  number of blocks transferred.
 *              
 */

static inline int _cblk_aresult(cflsh_chunk_t *chunk,int *tag, uint64_t *status,
                                int flags, int harvest)
{

    int rc = 0;
    cflsh_cmd_mgm_t *cmd = NULL;
    size_t transfer_size = 0;
    int pthread_rc;
    cflsh_cmd_info_t *cmdi;
    int i;




    if (tag == NULL) {

        errno = EINVAL;
	return -1;
    
    }

    if (status == NULL) {

        errno = EINVAL;
	return -1;
    
    }


    *status = 0;

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD && harvest) {

	/*
	 * Check if any commands completed since the last time we
	 * checked, but do not wait for any to complete.
	 */

	rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,tag,&transfer_size,FALSE,CFLASH_ASYNC_OP);

    }

    CFLASH_BLOCK_LOCK(chunk->lock);


    if (flags & CBLK_ARESULT_NEXT_TAG) {

        /* 
	 * The caller has asked to be
	 * notified of the first tag
	 * (commmand) to complete.
	 */

	

	if (chunk->num_active_cmds == 0) {

	    /*
	     * No commands are active to wait on.
	     */
 
	    errno = EINVAL;
	    
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return -1;
	}

        *tag = -1;

	/*
	 * In the case where someone has all commands active,
	 * and they are issue new requests to us as soone as one
	 * completes, we need to ensure fairness in how command responses
	 * are returned. We can not just return the same command tag
	 * and allow another to issued using that uses same command, because
	 * the other commands would be starved out. So we will start with the
	 * head of the active queue, which will be the oldest command outstanding.
	 * In most cases it should be more likely to have completed and be ready
	 * for completion that newer commands. The tail of the active queue is the newest
	 * command. Thus we'll traverse from the oldest to the newest looking for completions.
	 */



	cmdi = chunk->head_act;

	while (cmdi) {


	    if ((cmdi->in_use) &&
		(cmdi->flags & CFLSH_ASYNC_IO) &&
		(cmdi->state == CFLSH_MGM_CMP)) {
	        
	        /*
		 * This is an async command that completed
		 * but has not yet been seen by a caller.
		 */

	        *tag = cmdi->index;
		break;
	    }
	    

	    cmdi = cmdi->act_next;
	}


	if (*tag == -1) {
	    if (!(flags & CBLK_ARESULT_BLOCKING)) {

	        /*
		 * No commands have completed that 
		 * are waiting for processing and
		 * the caller does not want to block
		 * to wait for any. So return
		 * with an indication we did not find
		 * any commands.
		 */
		CFLASH_BLOCK_UNLOCK(chunk->lock);
	        return 0;
	    } else {

	        *tag = -1;

#ifndef _COMMON_INTRPT_THREAD

		CFLASH_BLOCK_UNLOCK(chunk->lock);

	        rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,tag,&transfer_size,TRUE,CFLASH_ASYNC_OP);

		
		CFLASH_BLOCK_LOCK(chunk->lock);
#else 

		if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {


		    CFLASH_BLOCK_UNLOCK(chunk->lock);
		    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,tag,&transfer_size,TRUE,CFLASH_ASYNC_OP);

		    CFLASH_BLOCK_LOCK(chunk->lock);

		} else {

		
		    /*
		     * Wait for any command completion.
		     */
		    pthread_rc = pthread_cond_wait(&(chunk->cmd_cmplt_event),&(chunk->lock.plock));
	
		    if (pthread_rc) {
	    

			CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed rc = %d errno = %d",
					    pthread_rc,errno);


		    } 
		}

#endif 
		if (*tag == -1) {
		    
		    /*
		     * No command has been found above. Check
		     * to see if a command was processed by the back ground
		     * threads.
		     */
		    
		    cmdi = chunk->head_act;

		    while (cmdi) {


			if ((cmdi->in_use) &&
			    (cmdi->flags & CFLSH_ASYNC_IO) &&
			    (cmdi->state == CFLSH_MGM_CMP)) {
	        
			    /*
			     * This is an async command that completed
			     * but has not yet been seen by a caller.
			     */

			    *tag = cmdi->index;
			    break;
			}
	    

			cmdi = cmdi->act_next;
		    }
		    
		    if (*tag == -1) {
			
			/*
			 * No command was still found that completed. 
			 */
			
			
			if (!rc) {
			    
			    /*
			     * If no error was reported for no
			     * command, then return an error
			     * here.
			     */
			    
			    errno = ETIMEDOUT;
			    
			    CFLASH_BLOCK_UNLOCK(chunk->lock);
			    return(-1);
			    
			}
			
			CFLASH_BLOCK_UNLOCK(chunk->lock);
			return 0;

		    }

		}
	
	    }
	}



    }



    /*
     * Since we unlocked above it is possible
     * another thread tried to close this thread
     * As a result we'll double check the chunk]
     * again. There is one issue with this approach.
     * If the chunk is invalid then is locking
     * chunk->locking chunk->lock valid.
     */

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
			    chunk->index);
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	errno = EINVAL;
	return -1;
    }

    if (flags & CBLK_ARESULT_USER_TAG) {
	
	cmd = NULL;

	for (i = 0; i < chunk->num_cmds;i++) {

	    if (chunk->cmd_info[i].user_tag == *tag) {

		/*
		 * We found the user tag provided
		 */

		cmd = &(chunk->cmd_start[i]);

		break;
	    }
	}

	if (cmd == NULL) {

	    errno = EINVAL;

	    rc = -1;

	    /*
	     * TODO: ?? add stats for user defined tag not found
	     */

	    CBLK_TRACE_LOG_FILE(3," user defined tag not found. rc = %d,errno = %d, tag = %d",
			    rc, errno,*tag);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    return rc;
	}


    } else {

	if ((*tag >= chunk->num_cmds) ||
	    (*tag < 0)) {
	
	    errno = EINVAL;

	    rc = -1;
	    CBLK_TRACE_LOG_FILE(1,"Invalid cmd_index = 0x%x",*tag);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    return rc;
	}
	cmd = &(chunk->cmd_start[*tag]);
    }



    if (cmd->cmdi == NULL) {

	errno = EINVAL;

	rc = -1;
	CBLK_TRACE_LOG_FILE(1,"null cmdi for cmd_index = 0x%x",cmd->index);

	return rc;

    }

    if (!cmd->cmdi->in_use) {

	errno = ENOMSG;

	rc = -1;
	CBLK_TRACE_LOG_FILE(3," tag not in use. rc = %d,errno = %d, tag = %d, cmd = 0x%llx",
			    rc, errno,*tag,(uint64_t)cmd);
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	return rc;
    }

#ifndef _COMMON_INTRPT_THREAD
    if (cmd->cmdi->thread_id &&
	cmd->cmdi->in_use &&
	(flags & CBLK_ARESULT_BLOCKING)) {

        /*
	 * The caller is indicating they
	 * want to wait for completion of
	 * this tag. So lets do that now.
	 */

	CFLASH_BLOCK_UNLOCK(chunk->lock);

        pthread_join(cmd->cmdi->thread_id,NULL);
	CFLASH_BLOCK_LOCK(chunk->lock);

	cmd->cmdi->thread_id = 0;
    }
#else 


    if ((cmd->cmdi->state != CFLSH_MGM_CMP) &&
	cmd->cmdi->in_use &&
	(flags & CBLK_ARESULT_BLOCKING)) {
    

        /*
	 * The caller is indicating they
	 * want to wait for completion of
	 * this tag. So lets do that now.
	 */
	    
	if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {


	    rc = CBLK_WAIT_FOR_IO_COMPLETE(chunk,tag,&transfer_size,TRUE,CFLASH_ASYNC_OP); 

	} else {
	    /*
	     * Only wait if the command is not completed
	     */
	    pthread_rc = pthread_cond_wait(&(cmd->cmdi->thread_event),&(chunk->lock.plock));
	
	    if (pthread_rc) {
	    

		CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed cmd_index = 0x%x rc = %d errno = %d",
				    cmd->index,pthread_rc,errno);
	    } else {


		rc = CBLK_COMPLETE_CMD(chunk,cmd,&transfer_size);

		if ((rc == 0) &&
		    (transfer_size == 0)) {


		    /*
		     * If this routine returns a 0 return code, then
		     * the caller will believe the command has not yet
		     * completed. So we need to map this to an error.
		     */

		    rc = -1;

		    errno = EIO;

		}
	    }
	
	}
    }
    

#endif


    if (cmd->cmdi->state == CFLSH_MGM_CMP) {

        /*
	 * This command completed,
	 * clean it up.
	 */
      

         if (!(cmd->cmdi->status)) {

	     /*
	      * Good completion
	      */

	     rc = cmd->cmdi->transfer_size;

	     errno = 0;

	     if (rc == 0) {

		 /*
		  * If this routine returns a 0 return code, then
		  * the caller will believe the command has not yet
		  * completed. So we need to map this to an error.
		  */

		 rc = -1;

		 errno = EIO;

	     }
	     
	 } else {
	    
	    /*
	     * We encountered an error. 
	     */

	    rc = -1;

	    errno = cmd->cmdi->status & 0xffffffff;
	    
	} 
    
	if (cmd->cmdi->flags & CFLSH_ASYNC_IO) {
	    
	    if (cmd->cmdi->flags & CFLSH_MODE_READ) {
		
		chunk->stats.num_blocks_read += cmd->cmdi->transfer_size;
		if (chunk->stats.num_act_areads) {
		    chunk->stats.num_act_areads--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
		}
		
	    } else if (cmd->cmdi->flags & CFLSH_MODE_WRITE) {
		
		
		chunk->stats.num_blocks_written += cmd->cmdi->transfer_size;
		if (chunk->stats.num_act_awrites) {
		    chunk->stats.num_act_awrites--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
		}
	    } else {
		CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
	    }
	} else {
	    


	    // This can happen in mixed mode (both async and sync) and if the caller waits for next tag.
	    if (cmd->cmdi->flags & CFLSH_MODE_READ) {
		
		chunk->stats.num_blocks_read += cmd->cmdi->transfer_size;
		if (chunk->stats.num_act_reads) {
		    chunk->stats.num_act_reads--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
		}
		
	    } else if (cmd->cmdi->flags & CFLSH_MODE_WRITE) {
		
		chunk->stats.num_blocks_written += cmd->cmdi->transfer_size;
		if (chunk->stats.num_act_writes) {
		    chunk->stats.num_act_writes--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
		}
	    } else {
		CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x",
					cmd->cmdi->flags);
	    }
	}

        chunk->num_active_cmds--;
	

#ifndef _COMMON_INTRPT_THREAD
	/*
	 * The caller may have told us to not wait
	 * so we do not want to wait for the cancel to complete.
	 * Instead will check flag in the command to 
	 * to see if the thread was exiting the last
	 * time it held the lock. If so then no cancel is
	 * needed here, since that thread is done accessing
	 * this command. Thus we can mark it free. Otherwise
	 * we need to set a flag so that the async thread
	 * marks this command as available (clears in_use) when it 
	 * exits.
	 */

	if (cmd->cmdi->flags & CFLSH_ATHRD_EXIT) {

	    CBLK_TRACE_LOG_FILE(7,"cmd = 0x%llx, tag = 0x%x",cmd,*tag);

	    
	    CBLK_FREE_CMD(chunk,cmd);

	} else {

	    CBLK_TRACE_LOG_FILE(7,"cmd = 0x%llx, tag = 0x%x",cmd,*tag);
	    cmd->cmdi->state = CFLSH_MGM_ASY_CMP;
	}


	/*
	 * Inform system to reclaim space for this thread when it
	 * terminates.
	 */

	 if (cmd->cmdi->thread_id) {

	     pthread_rc = pthread_detach(cmd->cmdi->thread_id);


	     cmd->cmdi->thread_id = 0;

	     if (pthread_rc) {

		 chunk->stats.num_fail_detach_threads++;
		 CBLK_TRACE_LOG_FILE(5,"pthread_detach failed rc = %d errno = %d, cmd_index = %d,lba = 0x%llx, num_active_cmds = 0x%x, num_active_threads = 0x%x",
				pthread_rc,errno, cmd->index,cmd->cmdi->lba,chunk->num_active_cmds,chunk->stats.num_active_threads);
	    
	     }
	 }
#else

	 CBLK_FREE_CMD(chunk,cmd);

#endif /* _COMMON_INTRPT_THREAD */
    }

    if (rc == 0) {

	chunk->stats.num_aresult_no_cmplt++;
    }

    CFLASH_BLOCK_UNLOCK(chunk->lock);

    


    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d, tag = %d",rc,errno,*tag);

    return rc;
}


/*
 * NAME:        cblk_aresult
 *
 * FUNCTION:    Waits for asynchronous read or writes to complete
 *              and returns the status of them.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              tag         - Tag associated with this request that
 *                            completed.
 *              flags       - Flags on this request.
 *
 * RETURNS:
 *              0   for good completion, but requested tag has not yet completed.
 *              -1  for  error and errno is set.
 *              > 0 for good completion where the return value is the
 *                  number of blocks transferred.
 *              
 */

int cblk_aresult(chunk_id_t chunk_id,int *tag, uint64_t *status, int flags)
{
    cflsh_chunk_t *chunk;


    errno = 0;

    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, flags = 0x%x, tag = 0x%x",chunk_id,flags,*tag);

    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {


        errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);

    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    return (_cblk_aresult(chunk,tag,status,flags,1));
}
  

/*
 * NAME:        cblk_listio
 *
 * FUNCTION:    Issues and waits for multiple I/O requests.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              flags       - Flags on this request.
 *
 * RETURNS:
 *              0   for good completion, but requested tag has not yet completed.
 *              -1  for  error and errno is set.
 *              > 0 for good completion where the return value is the
 *                  number of blocks transferred.
 *              
 */

int cblk_listio(chunk_id_t chunk_id,
		cblk_io_t *issue_io_list[],int issue_items,
		cblk_io_t *pending_io_list[], int pending_items, 
		cblk_io_t *wait_io_list[],int wait_items,
		cblk_io_t *completion_io_list[],int *completion_items, 
		uint64_t timeout,int flags)
{

    int rc = 0;
    int wait_item_found;
    int item_found;
    cflsh_chunk_t *chunk;
    cblk_io_t *io;
    cblk_io_t *tmp_io;
    cblk_io_t  *wait_io;
    cblk_io_t  *complete_io;
    int io_flags;
    int i,j;                               /* General counters */
    uint64_t status;
    struct timespec start_time;
    struct timespec last_time;
    uint64_t uelapsed_time = 0;            /* elapsed time in microseconds */
    int cmd_not_complete;
    int avail_completions;
    int harvest=1;/* send TRUE to aresult for the first cmd in the list only */

    errno = 0;

    CBLK_TRACE_LOG_FILE(5,"chunk_id = %d, issue_items = 0x%x, pending_items = 0x%x, wait_items = 0x%x, timeout = 0x%llx,flags = 0x%x",
			chunk_id,issue_items,pending_items,wait_items,timeout,flags);

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }


    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);



    /*
     * Do some validation on lists past before we do any processing.
     * This allows us to completely fail the request before some I/O
     * requests may have been issued.
     */

    if (cblk_listio_arg_verify(chunk_id,issue_io_list,issue_items,pending_io_list,pending_items,
				wait_io_list,wait_items,completion_io_list,completion_items,timeout,flags)) {


	return -1;
    }	

    complete_io = completion_io_list[0];

    avail_completions = *completion_items;


    /*
     * Reset complete_items to 0 completed.
     */

    *completion_items = 0;


    /*
     * TODO: ?? This API is ugly in that a command may be in both the wait_io_list
     *       and one of the other list (pending, or issue).  Since commands in the wait_io_list
     *       are not copied int the complete_io_list. Each pending or issue completion needs to first be checked
     *       via the waiting list and ensure it is not there already. Otherwise the entry in the wait_io_list should be 
     *       updated.
     */


    if (issue_items) {

	/*
	 * Caller is requesting I/Os to issued.
	 */


	if (issue_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"Issue_io_list array is a null pointer for  chunk_id = %d and issue_items = %d",
				chunk_id,issue_items);
	    errno = EINVAL;

	    return -1;

	} else if (issue_io_list[0] == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"Issue_io_list[0] is a null pointer for  chunk_id = %d and issue_items = %d",
				chunk_id,issue_items);
	    errno = EINVAL;

	    return -1;
	}
    

	for (i=0; i< issue_items;i++) {

	    io = issue_io_list[i];


	    if (io == NULL) {


		continue;

	    }

	    io->stat.blocks_transferred = 0;
	    io->stat.fail_errno = 0;
	    io->stat.status = CBLK_ARW_STATUS_PENDING;

	    if (io->buf == NULL) {



		CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
				chunk_id,i);


		io->stat.fail_errno = EINVAL;
		io->stat.status = CBLK_ARW_STATUS_INVALID;

		continue;
		
	    }

	    if ((io->request_type != CBLK_IO_TYPE_READ) &&
		(io->request_type != CBLK_IO_TYPE_WRITE)) {


		CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.fail_errno = EINVAL;
		io->stat.status = CBLK_ARW_STATUS_INVALID;
		continue;
	    }



	    /*
	     * Process this I/O request 
	     */

	    io_flags = 0;

	    if (io->flags & CBLK_IO_USER_TAG) {

		io_flags |= CBLK_ARW_USER_TAG_FLAG;

	    } else if (io->flags & CBLK_IO_USER_STATUS) {

		io_flags |= CBLK_ARW_USER_STATUS_FLAG;

	    }

	    if (flags & CBLK_LISTIO_WAIT_ISSUE_CMD) {

		io_flags |= CBLK_ARW_WAIT_CMD_FLAGS;
	    }


	    // TODO:?? Is it correct to give address of io->tag, since it is not in wait_list?

	    // TODO:?? Should we specify io->stat address here.  Does the caller expect this to be updated?

	    
	    if (io->request_type == CBLK_IO_TYPE_READ) {

		rc = _cblk_aread(chunk,io->buf,io->lba,io->nblocks,&(io->tag),&(io->stat),io_flags);
	    } else {

		rc = _cblk_awrite(chunk,io->buf,io->lba,io->nblocks,&(io->tag),&(io->stat),io_flags);
	    }

	    if (rc < 0) {

		CBLK_TRACE_LOG_FILE(1,"Request failed for chunk_id = %d and index = %d with rc = %d, errno = %d",
				    chunk_id,i,rc,errno);

		// TODO:?? Should we filter on EINVAL and uses a different status?
		io->stat.fail_errno = errno;
		io->stat.blocks_transferred = 0;
		io->stat.status = CBLK_ARW_STATUS_FAIL;

	    } else if (rc) {

		CBLK_TRACE_LOG_FILE(9,"Request chunk_id = %d and index = %d with rc = %d, errno = %d",
				    chunk_id,i,rc,errno);

		io->stat.fail_errno = 0;
		io->stat.blocks_transferred = rc;
		io->stat.status = CBLK_ARW_STATUS_SUCCESS;
	    }

	    if (rc) {


		/*
		 * For a non-zero status update wait list
		 * if this request is on that list.
		 */

		CBLK_TRACE_LOG_FILE(9,"Request chunk_id = %d and index = %d with rc = %d, errno = %d",
				    chunk_id,i,rc,errno);

		wait_item_found = FALSE;


		if (wait_items) {

		    for (j=0; j < wait_items; j++) {

			wait_io = wait_io_list[j];

			if ((wait_io->buf == io->buf) &&
			    (wait_io->lba == io->lba) &&
			    (wait_io->nblocks == io->nblocks)) {
			    
			    wait_io->stat.fail_errno = io->stat.fail_errno;
			    wait_io->stat.blocks_transferred = rc;
			    wait_io->stat.status = io->stat.status;
			    wait_io->tag = io->tag;

			    wait_item_found = TRUE;

			    break;

			}

		    } /* inner for */

		}

		if (!wait_item_found) {

		    if ((complete_io) &&
			(*completion_items <avail_completions)) {
			complete_io->stat.fail_errno = io->stat.fail_errno;
			complete_io->stat.blocks_transferred = rc;
			complete_io->stat.status = io->stat.status;
			complete_io->tag = io->tag;
			complete_io++;
			(*completion_items)++;
		    } else {


			CBLK_TRACE_LOG_FILE(1,"Request chunk_id = %d and index = %d no complete_io entry found",
					    chunk_id,i);
		    }
		}

	    } else {


		/*
		 * Command did not complete yet. Check to see if this
		 * item is in the wait list and if necessary update
		 * its io_tag, since it may be a copy of this issue io item.
		 */


		if (wait_items) {

		    for (j=0; j < wait_items; j++) {

			wait_io = wait_io_list[j];

			if ((wait_io->buf == io->buf) &&
			    (wait_io->lba == io->lba) &&
			    (wait_io->nblocks == io->nblocks)) {

			    wait_io->tag = io->tag;

			    break;

			}

		    } /* inner for */

		}

	    }

	}  /* for */


    }


    /* 
     * TODO: ?? Look for a common routine that could do logic required of both both pending and
     *       waiting list. Thus this routine could be invoked twice here. Once for pending and then again
     *       for waiting list.
     */


    if (pending_items) {

	/*
	 * Caller is requesting I/Os to issued.
	 */


	if (pending_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"pending_io_list array is a null pointer for  chunk_id = %d and pending_items = %d",
				chunk_id,pending_items);
	    errno = EINVAL;

	    return -1;

	}


	for (i=0; i< pending_items;i++) {

	    io = pending_io_list[i];


	    if (io == NULL) {

		continue;

	    }

	    if (io->buf == NULL) {



		CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
				chunk_id,i);


		io->stat.fail_errno = EINVAL;
		io->stat.status = CBLK_ARW_STATUS_INVALID;

		continue;
		
	    }

	    if ((io->request_type != CBLK_IO_TYPE_READ) &&
		(io->request_type != CBLK_IO_TYPE_WRITE)) {


		CBLK_TRACE_LOG_FILE(1,"Invalid request_type = %d  chunk_id = %d and index = %d",
				    io->request_type,chunk_id,i);


		io->stat.fail_errno = EINVAL;
		io->stat.status = CBLK_ARW_STATUS_INVALID;
		continue;
	    }

	    if (io->stat.status != CBLK_ARW_STATUS_PENDING) {


		/*
		 * Is not an error, since a caller may pass the same 
		 * list over and over again. In that case we should
		 * only process commands that are pending.
		 */

		continue;
		
	    }

	    /*
	     * Process this I/O request 
	     */



	    io_flags = 0;

	    if (io->flags & CBLK_IO_USER_TAG) {

		io_flags |= CBLK_ARESULT_USER_TAG;

	    } 

	    rc = _cblk_aresult(chunk,&(io->tag),&status,io_flags,harvest);

	    if (harvest) {
		harvest=0;
	    }

	    if (rc < 0) {

		CBLK_TRACE_LOG_FILE(1,"Request failed for chunk_id = %d and index = %d with rc = %d, errno = %d",
				    chunk_id,i,rc,errno);


		// TODO:?? Should we filter on EINVAL and uses a different status?

		if (errno == EAGAIN) {

		    io->stat.fail_errno = 0;
		    io->stat.blocks_transferred = rc;
		    io->stat.status = CBLK_ARW_STATUS_PENDING;

		} else {
		    io->stat.fail_errno = errno;
		    io->stat.blocks_transferred = 0;
		    io->stat.status = CBLK_ARW_STATUS_FAIL;
		}

	    }  else if (rc) {

		CBLK_TRACE_LOG_FILE(9,"Request chunk_id = %d and index = %d with rc = %d, errno = %d",
				    chunk_id,i,rc,errno);

		io->stat.fail_errno = 0;
		io->stat.blocks_transferred = rc;
		io->stat.status = CBLK_ARW_STATUS_SUCCESS;
	    }



	    if (rc) {

		/*
		 * For a non-zero status update wait list
		 * if this request is on that list.
		 */

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
			    
			    wait_io->stat.fail_errno = io->stat.fail_errno;
			    wait_io->stat.blocks_transferred = rc;
			    wait_io->stat.status = io->stat.status;

			    wait_item_found = TRUE;

			    break;

			}

		    } /* inner for */

		}

		if (!wait_item_found) {

		    if ((complete_io) &&
			(*completion_items <avail_completions)) {  
			complete_io->stat.fail_errno = io->stat.fail_errno;
			complete_io->stat.blocks_transferred = rc;
			complete_io->stat.status = io->stat.status;
			complete_io->tag = io->tag;
			complete_io++;
			(*completion_items)++;
		    } else {


			CBLK_TRACE_LOG_FILE(1,"Request chunk_id = %d and index = %d no complete_io entry found",
					    chunk_id,i);
		    }
		}

	    }
	    

	}  /* for */

    }
    
    if (wait_items) {

	/*
	 * Caller is requesting to wait for these I/O
	 */


	if (wait_io_list == NULL) {


	    CBLK_TRACE_LOG_FILE(1,"wait_io_list array is a null pointer for  chunk_id = %d and pending_items = %d",
				chunk_id,wait_items);
	    errno = EINVAL;

	    return -1;

	}

	clock_gettime(CLOCK_MONOTONIC,&start_time);
	clock_gettime(CLOCK_MONOTONIC,&last_time);


	// TODO: ?? Add macros to replace this.

	uelapsed_time = ((last_time.tv_sec - start_time.tv_sec) * 1000000) + ((last_time.tv_nsec - start_time.tv_nsec)/1000);



	while ((timeout == 0) ||
	       (uelapsed_time < timeout)) {

	    /*
	     * If no time out is specified then only go thru this loop
	     * once. Otherwise continue thru this loop until
	     * our time has elapsed.
	     */

	    cmd_not_complete = FALSE;


	    for (i=0; i< wait_items;i++) {
		
		io = wait_io_list[i];
		

		if (io == NULL) {

		    continue;

		}

		if (io->stat.status != CBLK_ARW_STATUS_PENDING) {
		    
		    /*
		     * This I/O request has already completed.
		     * continue to the next wait I/O request.
		     */
		    
		    continue;
		}
		
		
		
		if (io->buf == NULL) {
		    
		    
		    
		    CBLK_TRACE_LOG_FILE(1,"data buffer is a null pointer for  chunk_id = %d and index = %d",
					chunk_id,i);
		    
		    
		    io->stat.fail_errno = EINVAL;
		    io->stat.status = CBLK_ARW_STATUS_INVALID;
		    
		    continue;
		    
		}
		
		
		/*
		 * Process this I/O request, always wait for request.
		 */
		
		
		// TODO:?? Need mechanism to specify time-out

		io_flags = 0;


		if (timeout == 0) {
		    
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
		    io->stat.fail_errno = errno;
		    io->stat.blocks_transferred = 0;
		    io->stat.status = CBLK_ARW_STATUS_FAIL;
		    
		} else if (rc) {
		    
		    
		    io->stat.fail_errno = 0;
		    io->stat.blocks_transferred = rc;
		    io->stat.status = CBLK_ARW_STATUS_SUCCESS;
		} else {

		    /*
		     * This command has not completed yet.
		     */

		    cmd_not_complete = TRUE;
		}
		
		if (rc) {

		    /*
		     * For a non-zero status update 
		     * associated issue/pending list.
		     */

		    item_found = FALSE;


		    if (issue_items) {

			/*
			 * If there are wait_items, then see
			 * if this item is one of them. If so
			 * update the associated wait_item.
			 */

			for (j=0; j < issue_items; j++) {

			    tmp_io = issue_io_list[j];

			    if ((tmp_io->buf == io->buf) &&
				(tmp_io->lba == io->lba) &&
				(tmp_io->nblocks == io->nblocks) &&
				(tmp_io->tag == io->tag)) {
			    
				tmp_io->stat.fail_errno = io->stat.fail_errno;
				tmp_io->stat.blocks_transferred = rc;
				tmp_io->stat.status = io->stat.status;

				item_found = TRUE;

				break;

			    }

			} /* inner for */

		    }

		    if ((pending_items) && (!item_found)) {

			/*
			 * If there are wait_items, then see
			 * if this item is one of them. If so
			 * update the associated wait_item.
			 */

			for (j=0; j < pending_items; j++) {

			    tmp_io = pending_io_list[j];

			    if ((tmp_io->buf == io->buf) &&
				(tmp_io->lba == io->lba) &&
				(tmp_io->nblocks == io->nblocks) &&
				(tmp_io->tag == io->tag)) {
			    
				tmp_io->stat.fail_errno = io->stat.fail_errno;
				tmp_io->stat.blocks_transferred = rc;
				tmp_io->stat.status = io->stat.status;

				item_found = TRUE;

				break;

			    }

			} /* inner for */

		    }

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

	    if (cmd_not_complete) {

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

	    clock_gettime(CLOCK_MONOTONIC,&last_time);


	    // TODO: ?? Add macros to replace this.
	    uelapsed_time = ((last_time.tv_sec - start_time.tv_sec) * 1000000) + ((last_time.tv_nsec - start_time.tv_nsec)/1000);



	} /* while */

    }

    rc = 0;

    
    if ((timeout) && (uelapsed_time >= timeout)) {

	errno = ETIMEDOUT;
	rc = -1;
    }
    
    CBLK_TRACE_LOG_FILE(5,"rc = %d, errno = %d",rc,errno);
    return rc;
}







/*
 * NAME:        cblk_clone_after_fork
 *
 * FUNCTION:    clone a (virtual lun) chunk with an existing (original)
 *              chunk. This is useful if a process is forked and the
 *              child wants to read data from the parents chunk.
 *
 *
 * INPUTS:
 *              chunk_id          - Chunk id to be cloned.
 *              mode              - Access mode for the chunk 
 *                                  (O_RDONLY, O_WRONLY, O_RDWR)
 *              flags             - Flags on this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */
int cblk_clone_after_fork(chunk_id_t chunk_id, int mode, int flags)
{

    int rc = 0;
    cflsh_chunk_t *chunk;

    errno = 0;

    

    CBLK_TRACE_LOG_FILE(5,"orig_chunk_id = %d mode = 0x%x, flags = 0x%x",
			chunk_id,mode,flags);

    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_WR_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }


    CFLASH_BLOCK_LOCK(chunk->lock);

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


    if (chunk->num_active_cmds > 0) {


	/*
	 * If this chunk still has active
	 * commands then fails this
	 * close.
	 */

	CBLK_TRACE_LOG_FILE(1,"cloning failed because chunk in use, rchunk_id = %d, path = %s, num_active_cmds %d",
			    chunk_id,chunk->dev_name,chunk->num_active_cmds);
	

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EBUSY;
	return -1;
    }

    /*
     * Ensure child does not have more read/write access
     * than the parent.
     */

    switch (mode & O_ACCMODE) {
    case O_RDONLY:

	if (chunk->flags & CFLSH_CHNK_RD_AC) {

	    /*
	     * Clear write access if it was previously
	     * enabled.
	     */

	    chunk->flags &= ~CFLSH_CHNK_WR_AC;
	} else {

	    CBLK_TRACE_LOG_FILE(1,"cloning failed because parent does not have read access, rchunk_id = %d, path = %s",
				chunk_id,chunk->dev_name);
	

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	    errno = EINVAL;
	    return -1;
	}

	break;
    case O_WRONLY:
	if (chunk->flags & CFLSH_CHNK_WR_AC) {
	    /*
	     * Clear read access if it was previously
	     * enabled.
	     */
	    chunk->flags &= ~CFLSH_CHNK_RD_AC;
	} else {

	    CBLK_TRACE_LOG_FILE(1,"cloning failed because parent does not have write access, rchunk_id = %d, path = %s",
				chunk_id,chunk->dev_name);
	

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	    errno = EINVAL;
	    return -1;

	}

	break;
    case O_RDWR:
	if (!((chunk->flags & CFLSH_CHNK_WR_AC) &&
	      (chunk->flags & CFLSH_CHNK_RD_AC))) {
	    /*
	     * If caller is specifying read/write
	     * access, but this chunk did not have that
	     * permission then fail this
	     * request.
	     */
	    CBLK_TRACE_LOG_FILE(1,"cloning failed because parent does not have both read and write access, rchunk_id = %d, path = %s",
				chunk_id,chunk->dev_name);
	

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	    errno = EINVAL;
	    return -1;
	}
	break;
    default:


	CBLK_TRACE_LOG_FILE(1,"Invalid access mode %d",mode);
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;



	    
    }


    if (chunk->path[chunk->cur_path] == NULL) {


	CBLK_TRACE_LOG_FILE(1,"Null path");
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EIO;
	return -1;



    }

    if (chunk->path[chunk->cur_path]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null afu");
	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EIO;
	return -1;
    }


    rc = cblk_mc_clone(chunk,mode,flags);

    if (!rc) {

	/*
	 * If this worked, then reinitialize RRQ and toggle.
	 */


	// TODO: ?? this assumes rrq size matches this chunk's num_cmds
	bzero((void *)chunk->path[chunk->cur_path]->afu->p_hrrq_start ,
	      (sizeof(*(chunk->path[chunk->cur_path]->afu->p_hrrq_start)) * chunk->path[chunk->cur_path]->afu->num_rrqs));

	chunk->path[chunk->cur_path]->afu->p_hrrq_curr = chunk->path[chunk->cur_path]->afu->p_hrrq_start;

	/*
	 * Since the host RRQ is
	 * bzeroed. The toggle bit in the host
	 * RRQ that initially indicates we
	 * have a new RRQ will need to be 1.
	 */


	chunk->path[chunk->cur_path]->afu->toggle = 1;
        
	chunk->path[chunk->cur_path]->afu->num_issued_cmds = 0;

	chunk->cmd_curr = chunk->cmd_start;

    }



    CFLASH_BLOCK_UNLOCK(chunk->lock);
    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);

    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d",rc,errno);

    return rc;
}
  

/*
 * NAME:        cblk_get_stats
 *
 * FUNCTION:    Return statistics for this chunk.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              tag         - Pointer to stats returned
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cblk_get_stats(chunk_id_t chunk_id, chunk_stats_t *stats, int flags)
{

    int rc = 0;
    cflsh_chunk_t *chunk;


    
    errno = 0;
    
    CBLK_TRACE_LOG_FILE(6,"flags = 0x%x",flags);
			

    if ((chunk_id <= NULL_CHUNK_ID) ||
	(chunk_id >= cflsh_blk.next_chunk_id)) {

	errno = EINVAL;
	return -1;
    }
			

    if (stats == NULL) {

	CBLK_TRACE_LOG_FILE(1,"Null stats passed");

	errno = EINVAL;
	return -1;
    }

    CFLASH_BLOCK_RD_RWLOCK(cflsh_blk.global_lock);
    chunk = CBLK_GET_CHUNK_HASH(chunk_id,TRUE);

    if (chunk == NULL) { 


	CBLK_TRACE_LOG_FILE(1,"chunk not found, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;	
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	/*
	 * Invalid chunk. Exit now.
	 */

	cflsh_blk.num_bad_chunk_ids++;
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk, chunk_id = %d",
			    chunk_id);
	CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);
	errno = EINVAL;
	return -1;
    }
			
    CFLASH_BLOCK_LOCK(chunk->lock);

    CFLASH_BLOCK_RWUNLOCK(cflsh_blk.global_lock);


    /* 
     * Copy stats back to caller
     */

    bcopy((void *)&(chunk->stats),(void *)stats, sizeof(chunk->stats));


    CFLASH_BLOCK_UNLOCK(chunk->lock);


    CBLK_TRACE_LOG_FILE(5,"rc = %d,errno = %d ",rc,errno);
    return rc;
}
  
