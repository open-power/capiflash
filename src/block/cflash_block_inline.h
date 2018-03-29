/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_inline.h $                             */
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

//#define CFLSH_BLK_FILENUM 0x0300
#ifndef _H_CFLASH_BLOCK_INLINE
#define _H_CFLASH_BLOCK_INLINE


#include "cflash_block_internal.h"
#include "cflash_block_protos.h"

/*
 * NAME:        PTRC_PSAVE_TO_SLOT
 *
 * FUNCTION: find a slot and save if unused or higher priority
 *
 */
static inline void PTRC_PSAVE_TO_SLOT(PerfTrace_t *ptrc,
                                      uint32_t     verbosity,
                                      uint64_t     priority)
{
    int i=0;
    pthread_mutex_lock(&cblk_ptrc_slock);

    for (i=0; i<CBLK_PTRC_SLOTS; i++)
    {
        if (PTRC_PCOPY(ptrc,&cblk_ptrc_save[i],verbosity,priority)) {break;}
    }
    pthread_mutex_unlock(&cblk_ptrc_slock);
}

/*
 * NAME:        CBLK_PTRC_SAVE_CMDS
 *
 * FUNCTION: write all cmd traces to a file, if they have trace data
 *
 */
static inline void CBLK_PTRC_SAVE_CMDS(cflsh_chunk_t *chunk, uint32_t verbosity)
{
    int   i       = 0;
    char  fn[128] = {0};

    /* save all slots to a file */
    for (i=0; i<chunk->num_cmds; i++)
    {
        sprintf(fn,"/tmp/ptrc.cnk%d.cmd%d", chunk->index, i);
        PTRC_SAVE(&chunk->cmd_info[i].ptrc, verbosity, fn);
    }
}

/*
 * NAME:        CBLK_PTRC_SAVE_GLBS
 *
 * FUNCTION: write all save slots to a file, if they have trace data
 *
 */
static inline void CBLK_PTRC_SAVE_GLBS(uint32_t verbosity)
{
    int   i       = 0;
    char  fn[128] = {0};

    for (i=0; i<CBLK_PTRC_SLOTS; i++)
    {
        if (cblk_ptrc_verbosity >= 9)
        {
            /* add ticks to fn so previously saved traces are not clobbered */
            sprintf(fn, "/tmp/ptrc.glb%d.%ld", i,getticks());
        }
        else
            {sprintf(fn, "/tmp/ptrc.glb%d", i);}
        PTRC_SAVE(&cblk_ptrc_save[i], verbosity, fn);
    }
}

/*
 * NAME:        CBLK_CMD_TIMEOUT
 *
 * FUNCTION:    calcs if a command timed out
 *
 * INPUTS:
 *              cmd
 *
 * RETURNS:
 *             1 - cmd has timed out
 *             0 - no timeout
 */
static inline int CBLK_CMD_TIMEOUT(cflsh_cmd_info_t *pcmd)
{
    uint32_t to = SDELTA(pcmd->stime, cflsh_blk.nspt);

    if (to > cflsh_blk.timeout_secs)
    {
        PTRC(&pcmd->ptrc, 1, CBLK_CMD_TIMEDOUT, pcmd->chunk->index, pcmd->lba, to);
        return 1;
    }
    return 0;
}


/*
 * NAME:        CBLK_SETUP_BAD_MMIO_SIGNAL
 *
 * FUNCTION:    Sets up a signal handler to catch
 *              MMIO failure due to adapter reset
 *              or uncorrectable error.
 *
 * NOTES:       This routine assumes the caller is holding
 *              the chunk lock.
 *
 * INPUTS:
 *              chunk        - Chunk the cmd is associated.
 *
 *              upper_offset - Upper offset of MMIO.
 *
 * RETURNS:
 *              
 *             0 - Good completion, otherwise error 
 *              
 */

#ifdef DEBUG
static inline int CBLK_SETUP_BAD_MMIO_SIGNAL(cflsh_chunk_t *chunk, 
					     int path_index,
					     uint64_t upper_offset)
{
    int rc = 0;
    struct sigaction action;


    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");

	return -1;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");

	return -1;

    }

    bzero((void *)&action,sizeof(action));

    action.sa_sigaction = cblk_chunk_sigsev_handler;
    action.sa_flags = SA_SIGINFO;

    if (sigaction(SIGSEGV, &action,&(chunk->path[path_index]->old_action))) {

        CBLK_TRACE_LOG_FILE(1,"Failed to set up SIGSEGV handler with errno = %d\n",
			    errno);

	return rc;
    }


    chunk->path[path_index]->flags |= CFLSH_CHNK_SIGH;

    chunk->path[path_index]->upper_mmio_addr = chunk->path[path_index]->afu->mmio + upper_offset;



    if (setjmp(chunk->path[path_index]->jmp_mmio)) {

        /*
         * We only get here if a longjmp occurred,
         * which indicates we failed doing the MMIO
         * operation.
         */
	rc = TRUE;
	if (sigaction(SIGSEGV, &(chunk->path[path_index]->old_action),NULL)) {

	    CBLK_TRACE_LOG_FILE(1,"Failed to restore SIGSEGV handler with errno = %d\n",
				errno);
	}
	chunk->path[path_index]->flags &= ~CFLSH_CHNK_SIGH;
	CBLK_TRACE_LOG_FILE(1,"MMIO failure with upper_offset = 0x%llx",upper_offset);
    }


    return rc;
}
#else
static inline int CBLK_SETUP_BAD_MMIO_SIGNAL(cflsh_chunk_t *chunk, 
					     int path_index,
					     uint64_t upper_offset)
{

    return 0;
}
#endif /* !DEBUG */

/*
 * NAME:        CBLK_CLEANUP_BAD_MMIO_SIGNAL
 *
 * FUNCTION:    Removes the signal handler to catch
 *              MMIO failures ad restores the previous
 *              signal handler..
 *
 * NOTES:       This routine assumes the caller is holding
 *              the chunk lock.
 *
 *
 * INPUTS:
 *              chunk        - Chunk the cmd is associated.
 *
 *
 * RETURNS:
 *              0 - Good completion, otherwise error.
 *              
 *              
 */

#ifdef DEBUG
static inline void CBLK_CLEANUP_BAD_MMIO_SIGNAL(cflsh_chunk_t *chunk, 
						int path_index)
{



    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");

	return;

    }

    if (sigaction(SIGSEGV, &(chunk->path[path_index]->old_action),NULL)) {

        CBLK_TRACE_LOG_FILE(1,"Failed to restore SIGSEGV handler with errno = %d\n",
			    errno);
    }
    
    chunk->path[path_index]->flags &= ~CFLSH_CHNK_SIGH;
    return;
}
#else
static inline void CBLK_CLEANUP_BAD_MMIO_SIGNAL(cflsh_chunk_t *chunk, 
						int path_index)
{
    return;
}
#endif /* !DEBUG */



/*
 * NAME:        CBLK_INVALD_CHUNK_PATH_AFU
 *
 * FUNCTION:    Perform
 *              MMIO failures ad restores the previous
 *              signal handler..
 *
 * NOTES:       This routine assumes the caller is holding
 *              the chunk lock.
 *
 *
 * INPUTS:
 *              chunk        - Chunk the cmd is associated.
 *
 *
 * RETURNS:
 *              0 - Good completion, otherwise error.
 *              
 *              
 */


static inline int CBLK_INVALID_CHUNK_PATH_AFU(cflsh_chunk_t *chunk, 
					      int path_index,
					      const char *fcn_name)
{
    int rc = 0;

    if (chunk == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"chunk is null for fcn = %s",fcn_name);
	
	return -1;
    }

    if ( (ulong)chunk & CHUNK_BAD_ADDR_MASK ) {

        CBLK_TRACE_LOG_FILE(1,"chunk has invalid address for fcn = %s",fcn_name);
        
        return -1;
        
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk eyecatcher for fcn = %s",fcn_name);

	return -1;
    }


    if (path_index < 0) {

	CBLK_TRACE_LOG_FILE(1,"path_index is invald for fcn = %s",fcn_name);
	return -1;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null for fcn = %s",fcn_name);
	
        errno = EINVAL;
	return -1;

    }

    if ( (ulong)(chunk->path[path_index]) & PATH_BAD_ADDR_MASK ) {

        CBLK_TRACE_LOG_FILE(1,"path has invalid address for fcn = %s",fcn_name);
	
	return -1;
        
    }


    if (CFLSH_EYECATCH_PATH(chunk->path[path_index])) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid path eyecatcher for fcn = %s",fcn_name);

	return -1;
    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null for fcn = %s",fcn_name);
	
	return (-1);

    }

    if ( (ulong)(chunk->path[path_index]->afu) & AFU_BAD_ADDR_MASK ) {

        CBLK_TRACE_LOG_FILE(1,"AFU has invalid addressfor fcn = %s ",fcn_name);
	
	return -1;
        
    }

    if (CFLSH_EYECATCH_AFU(chunk->path[path_index]->afu)) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid AFU eyecatcher for %s",fcn_name);

	return -1;
    }
    return rc;
}

/*
 * NAME:        CBLK_INVALID_CMD_CMDI
 *
 * FUNCTION:    Perform
 *              MMIO failures ad restores the previous
 *              signal handler..
 *
 * NOTES:       This routine assumes the caller is holding
 *              the chunk lock.
 *
 *
 * INPUTS:
 *              chunk        - Chunk the cmd is associated.
 *
 *
 * RETURNS:
 *              0 - Good completion, otherwise error.
 *              
 *              
 */


static inline int CBLK_INVALID_CMD_CMDI(cflsh_chunk_t *chunk, 
					      cflsh_cmd_mgm_t *cmd,
					      cflsh_cmd_info_t *cmdi,
					      const char *fcn_name)
{
    int rc = 0;
    cflsh_cmd_info_t *tmp_cmdi;

    if (cmd == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"cmd is null for fcn = %s",fcn_name);
	
	return -1;
    }

    if ((uint64_t)cmd & CMD_BAD_ADDR_MASK) {
	
	CBLK_TRACE_LOG_FILE(1,"cmd is not aligned correctly = %p for fcn = %s",cmd,fcn_name);
	
	return -1;
    }

    if (cmdi == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"cmdi is null for fcn = %s",fcn_name);
	
	return -1;
    }

    if (chunk == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"chunk is null for fcn = %s",fcn_name);
	
	return -1;
    }

    if (cmdi->chunk == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"cmdi->chunk is null for fcn = %s",fcn_name);
	
	return -1;
    }

    if (cmdi->chunk != chunk) {
	
	CBLK_TRACE_LOG_FILE(1,"cmdi->chunk %p does not match passed in chunk %p fcn = %s",
			    cmdi->chunk,chunk,fcn_name);
	
	return -1;
    }
   
    if (CFLSH_EYECATCH_CHUNK(cmdi->chunk)) {

	CBLK_TRACE_LOG_FILE(1,"cmdi->chunk %p has invalid eyecather for fcn = %s",cmdi->chunk,fcn_name);
	
	return -1;
    }

    if ((cmdi < &(chunk->cmd_info[0])) ||
	(cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	CBLK_TRACE_LOG_FILE(1,"cmdi = %p with index %d is invalid, chunk->num_cmds = %d, chunk->flags =0x%x for fcn = %s",
			    cmdi,cmdi->index, chunk->num_cmds,chunk->flags,fcn_name);
	return -1;

    }

    if (CFLSH_EYECATCH_CMDI(cmdi)) {

	CBLK_TRACE_LOG_FILE(1,"Invalid eyecatcher cmdi = %p is invalid, chunk->num_cmds = %d, chunk->flags =0x%x for fcn = %s",
			    cmdi,cmdi->index, chunk->num_cmds,chunk->flags,fcn_name);
	return -1;

    }


    if (cmdi->index > chunk->num_cmds) {
	CBLK_TRACE_LOG_FILE(1,"cmdi = %p index is too large = %d, chunk->num_cmds = %d for fcn = %s",
			    cmdi,cmdi->index, chunk->num_cmds,fcn_name);


	CBLK_TRACE_LOG_FILE(1,"user_tag = 0x%x, *user_status = %p, path_index = %d for fcn = %s",
			    cmdi->user_tag,cmdi->user_status,cmdi->path_index,fcn_name);

	return -1;
    }

    if (cmdi->free_next) {

	tmp_cmdi = cmdi->free_next;

	if ((tmp_cmdi < &(chunk->cmd_info[0])) ||
	    (tmp_cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	    CBLK_TRACE_LOG_FILE(1,
				"cmdi = %p with index %d has invalid free_next pointer = %p, chunk->num_cmds = %d for fcn = %s",
				cmdi,cmdi->index, tmp_cmdi,chunk->num_cmds,fcn_name);
	    return -1;

	}
	
    }

    if (cmdi->free_prev) {

	tmp_cmdi = cmdi->free_prev;

	if ((tmp_cmdi < &(chunk->cmd_info[0])) ||
	    (tmp_cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	    CBLK_TRACE_LOG_FILE(1,
				"cmdi = %p with index %d has invalid free_next pointer = %p, chunk->num_cmds = %d for fcn = %s",
				cmdi,cmdi->index, tmp_cmdi,chunk->num_cmds,fcn_name);
	    return -1;

	}
	
    }


    if (cmdi->act_next) {

	tmp_cmdi = cmdi->act_next;

	if ((tmp_cmdi < &(chunk->cmd_info[0])) ||
	    (tmp_cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	    CBLK_TRACE_LOG_FILE(1,
				"cmdi = %p with index %d has invalid active_next pointer = %p, chunk->num_cmds = %d for fcn = %s",
				cmdi,cmdi->index, tmp_cmdi,chunk->num_cmds,fcn_name);
	    return -1;

	}
	
    }


    if (cmdi->act_prev) {

	tmp_cmdi = cmdi->act_prev;

	if ((tmp_cmdi < &(chunk->cmd_info[0])) ||
	    (tmp_cmdi > &(chunk->cmd_info[chunk->num_cmds]))) {



	    CBLK_TRACE_LOG_FILE(1,
				"cmdi = %p with index %d has invalid active_prev pointer = %p, chunk->num_cmds = %d for fcn = %s",
				cmdi,cmdi->index, tmp_cmdi,chunk->num_cmds,fcn_name);
	    return -1;

	}
	
    }

    if (cmd != &(chunk->cmd_start[cmdi->index])) {

	CBLK_TRACE_LOG_FILE(1,"cmd = %p != cmdi->index [%d] in cmd_start, chunk->num_cmds = %d for fcn = %s",
			    cmd,cmdi->index, chunk->num_cmds,fcn_name);
	return -1;

    }


    if ((cmd < chunk->cmd_start) ||
	(cmd > chunk->cmd_end)) {



	CBLK_TRACE_LOG_FILE(1,"cmd = %p with index = %d, is invalid, chunk->num_cmds = %d for fcn = %s",
			    cmd,cmdi->index, chunk->num_cmds,fcn_name);
	return -1;

    }


    return rc; 
}


/************************************************************************/
/* Adapter Specific Inline Functions                                    */
/************************************************************************/


/*
 * NAME: CBLK_GET_NUM_INTERRUPTS
 *
 * FUNCTION: This routine is called whenever one needs to issue
 *           an IOARCB to see if there is room for another command
 *           to be accepted by the AFU from this context.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: The number of commands that can currently be issued to the AFU
 *          for this context.
 *     
 *    
 */

static inline int  CBLK_GET_NUM_INTERRUPTS(cflsh_chunk_t *chunk, 
					  int path_index)
{
    int rc;

    if (chunk == NULL) {

	errno = EINVAL;

	return 0;
    }



    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->fcn_ptrs.get_num_interrupts == NULL) {

	errno = EINVAL;

	return 0;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then return 0 (no command room)
	 * from this routine
	 */

	
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return 0;

    }

    rc = chunk->path[path_index]->fcn_ptrs.get_num_interrupts(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);


    return rc;
}

/*
 * NAME: CBLK_GET_CMD_ROOM
 *
 * FUNCTION: This routine is called whenever one needs to issue
 *           an IOARCB to see if there is room for another command
 *           to be accepted by the AFU from this context.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: The number of commands that can currently be issued to the AFU
 *          for this context.
 *     
 *    
 */

static inline uint64_t  CBLK_GET_CMD_ROOM(cflsh_chunk_t *chunk, 
					  int path_index)
{
    uint64_t rc;

    if (chunk == NULL) {

	errno = EINVAL;

	return 0;
    }


    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->fcn_ptrs.get_cmd_room == NULL) {

	errno = EINVAL;

	return 0;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then return 0 (no command room)
	 * from this routine
	 */

	

	CBLK_TRACE_LOG_FILE(5,"afu is halted");
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return 0;

    }

    rc = chunk->path[path_index]->fcn_ptrs.get_cmd_room(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    if (rc == 0xffffffffffffffffLL) {

	CBLK_TRACE_LOG_FILE(1,"Potential UE encountered for command room\n");
	

	cblk_check_os_adap_err(chunk,path_index);

	/*
	 * Tell caller there is no command room
	 */

	rc = 0;
    }


    return rc;
}

/*
 * NAME: CBLK_GET_INTRPT_STATUS
 *
 * FUNCTION: This routine is called whenever one needs to read 
 *           the status of the adapter.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: The interrupt status register.
 *     
 *    
 */

static inline uint64_t  CBLK_GET_INTRPT_STATUS(cflsh_chunk_t *chunk, int path_index,
					       cflash_block_check_os_status_t *reset_status)
{
    uint64_t rc;

    if (chunk == NULL) {

	errno = EINVAL;

	return 0;
    }

    if (reset_status == NULL) {

	errno = EINVAL;

	return 0;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return 0;

    }

    if (chunk->path[path_index]->fcn_ptrs.get_intrpt_status == NULL) {

	errno = EINVAL;

	return 0;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then return 0 
	 * from this routine
	 */

	
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return 0;

    }

    rc = chunk->path[path_index]->fcn_ptrs.get_intrpt_status(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    if (rc == 0xffffffffffffffffLL) {

	CBLK_TRACE_LOG_FILE(1,"Potential UE encountered for interrupt status\n");

	*reset_status = cblk_check_os_adap_err(chunk,path_index);
    }


    return rc;
}

/*
 * NAME: CBLK_INC_RRQ
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

static inline void  CBLK_INC_RRQ(cflsh_chunk_t *chunk, int path_index)
{

 

    if (chunk == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }

    if (chunk->path[path_index]->fcn_ptrs.inc_rrq == NULL) {

	errno = EINVAL;

	return;
    }



    chunk->path[path_index]->fcn_ptrs.inc_rrq(chunk,path_index);



    return ;
}

/*
 * NAME: CBLK_INC_SQ
 *
 * FUNCTION: This routine is called whenever an SQ entry has been queued.
 *           
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */

static inline void  CBLK_INC_SQ(cflsh_chunk_t *chunk, int path_index)
{

 

    if (chunk == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }

    if (chunk->path[path_index]->fcn_ptrs.inc_sq == NULL) {

	errno = EINVAL;

	return;
    }



    chunk->path[path_index]->fcn_ptrs.inc_sq(chunk,path_index);



    return ;
}

/*
 * NAME: CBLK_INC_RRQ_LOCK
 *
 * FUNCTION: This routine is called whenever an RRQ has been processed and
 *           the path lock is needed.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */

static inline void  CBLK_INC_RRQ_LOCK(cflsh_chunk_t *chunk, int path_index)
{

 

    if (chunk == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return;

    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then return 
	 * from this routine
	 */

	
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return;

    }

    CBLK_INC_RRQ(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return;
}

/*
 * NAME: CBLK_GET_CMD_DATA_LENGTH
 *
 * FUNCTION: Returns the data length associated with a command
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline uint32_t CBLK_GET_CMD_DATA_LENGTH(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;

    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.get_cmd_data_length == NULL) {

	errno = EINVAL;

	return 0;
    }
    
    return (chunk->path[cmdi->path_index]->fcn_ptrs.get_cmd_data_length(chunk,cmd));
}

/*
 * NAME: CBLK_GET_CMD_CDB
 *
 * FUNCTION: Returns the offset of the CDB in the command.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: 
 *     
 *    
 */
static inline scsi_cdb_t * CBLK_GET_CMD_CDB(cflsh_chunk_t *chunk, 
					    cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;
    
    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return NULL;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return NULL;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return NULL;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.get_cmd_cdb == NULL) {

	errno = EINVAL;
	
	return NULL;
    }
    
    return (chunk->path[cmdi->path_index]->fcn_ptrs.get_cmd_cdb(chunk,cmd));
}

/*
 * NAME: CBLK_GET_CMD_RSP
 *
 * FUNCTION: Returns the offset of the command this response is for.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: 
 *     
 *    
 */
static inline cflsh_cmd_mgm_t *CBLK_GET_CMD_RSP(cflsh_chunk_t *chunk, int path_index)
{

    cflsh_cmd_mgm_t *cmd = NULL;

    if (chunk == NULL) {

	errno = EINVAL;

	return NULL;
    }


    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return NULL;

    }

    if (chunk->path[path_index]->fcn_ptrs.get_cmd_rsp == NULL) {

	errno = EINVAL;

	return NULL;
    }

    


    cmd = chunk->path[path_index]->fcn_ptrs.get_cmd_rsp(chunk,path_index);




    return cmd;

}

/*
 * NAME: CBLK_ADAP_SETUP
 *
 * FUNCTION: Builds and adapter specific command/request.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_ADAP_SETUP(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;

    if (chunk == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->fcn_ptrs.adap_setup == NULL) {

	errno = EINVAL;

	return -1;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then fail 
	 * from this routine
	 */

	
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return -1;

    }

    rc = chunk->path[path_index]->fcn_ptrs.adap_setup(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return rc;
}

/*
 * NAME: CBLK_ADAP_SQ_SETUP
 *
 * FUNCTION: Sets up an adapter's SQ
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_ADAP_SQ_SETUP(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;

    if (chunk == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->fcn_ptrs.adap_sq_setup == NULL) {

	errno = EINVAL;

	return -1;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then fail 
	 * from this routine
	 */

	
	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return -1;

    }

    rc = chunk->path[path_index]->fcn_ptrs.adap_sq_setup(chunk,path_index);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return rc;
}


/*
 * NAME: CBLK_BUILD_ADAP_CMD
 *
 * FUNCTION: Builds and adapter specific command/request.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_BUILD_ADAP_CMD(cflsh_chunk_t *chunk,
				      cflsh_cmd_mgm_t *cmd, 
				      void *buf, size_t buf_len, int flags)
{
    cflsh_cmd_info_t *cmdi;

    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.build_adap_cmd == NULL) {

	errno = EINVAL;

	return -1;
    }

    return (chunk->path[cmdi->path_index]->fcn_ptrs.build_adap_cmd(chunk,cmdi->path_index,cmd,buf,buf_len,flags));
}

/*
 * NAME: CBLK_UPDATE_PATH_ADAP_CMD
 *
 * FUNCTION: Builds and adapter specific command/request.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_UPDATE_PATH_ADAP_CMD(cflsh_chunk_t *chunk,
					    cflsh_cmd_mgm_t *cmd,
					    int flags)
{
    cflsh_cmd_info_t *cmdi;

    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.update_adap_cmd == NULL) {

	errno = EINVAL;

	return -1;
    }

    return (chunk->path[cmdi->path_index]->fcn_ptrs.update_adap_cmd(chunk,cmdi->path_index,cmd,flags));
}

/*
 * NAME: CBLK_ISSUE_ADAP_CMD
 *
 * FUNCTION: Issues a command to the adapter specific command/request
 *           to the adapter. The first implementation will issue IOARCBs.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_ISSUE_ADAP_CMD(cflsh_chunk_t *chunk,
				      cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;


    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.issue_adap_cmd == NULL) {

	errno = EINVAL;

	return -1;
    }

    
    return (chunk->path[cmdi->path_index]->fcn_ptrs.issue_adap_cmd(chunk,cmdi->path_index,cmd));

}

/*
 * NAME: CBLK_COMPLETE_STATUS_ADAP_CMD
 *
 * FUNCTION: Indicates at high level of command completed with error or not.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_COMPLETE_STATUS_ADAP_CMD(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;


    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.complete_status_adap_cmd == NULL) {

	errno = EINVAL;

	return -1;
    }
    return (chunk->path[cmdi->path_index]->fcn_ptrs.complete_status_adap_cmd(chunk,cmd));

}

/*
 * NAME: CBLK_INIT_ADAP_CMD
 *
 * FUNCTION: Initialize command area so that it can be retried.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline void CBLK_INIT_ADAP_CMD(cflsh_chunk_t *chunk, 
					   cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;



    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }


    if (chunk->path[cmdi->path_index]->fcn_ptrs.init_adap_cmd == NULL) {

	errno = EINVAL;

	return;
    }
    chunk->path[cmdi->path_index]->fcn_ptrs.init_adap_cmd(chunk,cmd);

    return;

}

/*
 * NAME: CBLK_INIT_ADAP_CMD_RESP
 *
 * FUNCTION: Initialize command's response area so that it can be retried.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline void CBLK_INIT_ADAP_CMD_RESP(cflsh_chunk_t *chunk, 
					   cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;



    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }


    if (chunk->path[cmdi->path_index]->fcn_ptrs.init_adap_cmd_resp == NULL) {

	errno = EINVAL;

	return;
    }
    chunk->path[cmdi->path_index]->fcn_ptrs.init_adap_cmd_resp(chunk,cmd);

    return;

}

/*
 * NAME: CBLK_COPY_ADAP_CMD_RESP
 *
 * FUNCTION: Copy command's response area to specified buffer.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline void CBLK_COPY_ADAP_CMD_RESP(cflsh_chunk_t *chunk, 
					   cflsh_cmd_mgm_t *cmd,
					   void *buffer, int buffer_size)
{
    cflsh_cmd_info_t *cmdi;



    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }


    if (chunk->path[cmdi->path_index]->fcn_ptrs.copy_adap_cmd_resp == NULL) {

	errno = EINVAL;

	return;
    }
    chunk->path[cmdi->path_index]->fcn_ptrs.copy_adap_cmd_resp(chunk,cmd,buffer,buffer_size);

    return;

}

/*
 * NAME: CBLK_SET_ADAP_CMD_RSP_STATUS
 *
 * FUNCTION: Set command's response status for emulation.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline void CBLK_SET_ADAP_CMD_RSP_STATUS(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd,int success)
{
    cflsh_cmd_info_t *cmdi;


    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.set_adap_cmd_resp_status  == NULL) {

	errno = EINVAL;

	return;
    }

    chunk->path[cmdi->path_index]->fcn_ptrs.set_adap_cmd_resp_status(chunk,cmd,success);

    return;

}



/*
 * NAME: CBLK_PROCESS_ADAP_INTRPT
 *
 * FUNCTION: Process adapter interrupts
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_PROCESS_ADAP_INTRPT(cflsh_chunk_t *chunk, 
					   cflsh_cmd_mgm_t **cmd,
					   int intrpt_num,
					   int *cmd_complete,size_t *transfer_size)
{
    cflsh_cmd_info_t *cmdi;
    int path_index;
    int rc = 0;



    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }


    if ( (ulong)chunk & CHUNK_BAD_ADDR_MASK ) {

        errno = EINVAL;

        CBLK_TRACE_LOG_FILE(1,"Invalid chunk address");

        return -1;
        
    }

    if (CFLSH_EYECATCH_CHUNK(chunk)) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk eyecatcher");

	return -1;
    }

    if (*cmd) {
	cmdi = &chunk->cmd_info[(*cmd)->index];

	if (cmdi == NULL) {

	    errno = EINVAL;

	    return -1;
	}

	path_index = cmdi->path_index;

    } else {
	
	path_index = chunk->cur_path;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if ( (ulong)(chunk->path[path_index]) & PATH_BAD_ADDR_MASK ) {

        CBLK_TRACE_LOG_FILE(1,"path has invalid address");
	
	errno = EINVAL;
	return -1;
        
    }


    if (CFLSH_EYECATCH_PATH(chunk->path[path_index])) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk eyecatcher");

	errno = EINVAL;
	return -1;
    }


    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->fcn_ptrs.process_adap_intrpt == NULL) {

	errno = EINVAL;

	return -1;
    }

    if ( (ulong)(chunk->path[path_index]->afu) & AFU_BAD_ADDR_MASK ) {

        CBLK_TRACE_LOG_FILE(1,"AFU has invalid address");
	
	errno = EINVAL;
	return -1;
        
    }

    if (CFLSH_EYECATCH_AFU(chunk->path[path_index]->afu)) {
	
	CBLK_TRACE_LOG_FILE(1,"Invalid chunk eyecatcher");

	errno = EINVAL;
	return -1;
    }


    

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then fail 
	 * from this routine
	 */

	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	errno = EINVAL;

	return -1;

    }

    rc = chunk->path[path_index]->fcn_ptrs.process_adap_intrpt(chunk,path_index,cmd,intrpt_num,cmd_complete,transfer_size);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return rc;

}

/*
 * NAME: CBLK_PROCESS_ADAP_CONVERT_INTRPT
 *
 * FUNCTION: Process adapter interrupts, but first converting from
 *           the generic library type to the AFU specify type.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_PROCESS_ADAP_CONVERT_INTRPT(cflsh_chunk_t *chunk, 
					   cflsh_cmd_mgm_t **cmd,
					   int intrpt_num,
					   int *cmd_complete,size_t *transfer_size)
{
    cflsh_cmd_info_t *cmdi;
    int path_index;
    int rc = 0;



    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    if (*cmd) {
	cmdi = &chunk->cmd_info[(*cmd)->index];

	if (cmdi == NULL) {

	    errno = EINVAL;

	    return -1;
	}

	path_index = cmdi->path_index;

    } else {
	
	path_index = chunk->cur_path;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }


    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->fcn_ptrs.process_adap_convert_intrpt == NULL) {

	errno = EINVAL;

	return -1;
    }


    

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	/*
	 * If path is in a halted state then fail 
	 * from this routine
	 */

	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	errno = EINVAL;

	return -1;

    }

    rc = chunk->path[path_index]->fcn_ptrs.process_adap_convert_intrpt(chunk,path_index,cmd,intrpt_num,cmd_complete,transfer_size);

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return rc;

}

/*
 * NAME: CBLK_PROCESS_ADAP_CMD_ERR
 *
 * FUNCTION: Process adapter error on this command
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_PROCESS_ADAP_CMD_ERR(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{
    cflsh_cmd_info_t *cmdi;


    if ((chunk == NULL) || (cmd == NULL)) {

	errno = EINVAL;

	return -1;
    }

    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[cmdi->path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[cmdi->path_index]->fcn_ptrs.process_adap_err == NULL) {

	errno = EINVAL;

	return -1;
    }

    return (chunk->path[cmdi->path_index]->fcn_ptrs.process_adap_err(chunk,cmdi->path_index,
							      cmd));

}

/*
 * NAME: CBLK_RESET_ADAP_CONTEXT
 *
 * FUNCTION: This will reset the adapter context so that
 *           any active commands will never be returned to the host.
 *           The AFU is not reset and new requests can be issued.
 *           This routine assumes the caller has the chunk lock.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_RESET_ADAP_CONTEXT(cflsh_chunk_t *chunk,int path_index)
{
    int rc = 0;
    cflsh_path_t *tmp_path;


    if (chunk == NULL) {

	errno = EINVAL;

	return -1;
    }

    if (chunk->path[path_index] == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->afu == NULL) {

	CBLK_TRACE_LOG_FILE(1,"afu is null");
	
	errno = EINVAL;

	return -1;

    }

    if (chunk->path[path_index]->fcn_ptrs.reset_adap_contxt == NULL) {

	errno = EINVAL;

	return -1;
    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);
    
    chunk->stats.num_reset_contexts++;
    
    rc = chunk->path[path_index]->fcn_ptrs.reset_adap_contxt(chunk,path_index);

    if (rc) {

	chunk->stats.num_reset_contxt_fails++;	
    }
    
    /*
     * Update other paths that a context reset was done
     */
    
    
    tmp_path = chunk->path[path_index]->afu->head_path;
    
    while (tmp_path) {
	
	
	if (tmp_path != chunk->path[path_index]) {
	    
	    fetch_and_or(&(tmp_path->flags),CFLSH_PATH_RST);
	    
	}
	
	tmp_path = tmp_path->next;
	
    }
    

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

    return rc;

}

/************************************************************************/
/* End of Adapter Specific Inline Functions                             */
/************************************************************************/



/* 
 * The code below is mostly (but not completely) adapter and 
 * OS agnostic.
 */

/*
 * NAME:        CBLK_SAVE_IN_CACHE
 *
 * FUNCTION:    Save data in cache tagged by lba.
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

static inline void CBLK_SAVE_IN_CACHE(cflsh_chunk_t *chunk,void *buf, cflash_offset_t lba, size_t nblocks)
{
    cflsh_cache_line_t *line;
    cflash_offset_t cur_lba, end_lba;
    void              *cur_buf;
    uint64_t           tag;
    unsigned           inx;
    int                lru;

    if (chunk == NULL) {

	return;
    }

    if ((chunk->cache == NULL) ||
	(chunk->cache_size == 0)) {

	return;
    }

    if (buf == NULL) {

	return;

    }

    end_lba = lba + nblocks;

    for (cur_lba = lba, cur_buf = buf; cur_lba < end_lba; cur_lba++, cur_buf += CAPI_FLASH_BLOCK_SIZE) {

	inx = CFLSH_BLK_GETINX (cur_lba,chunk->l2setsz);
	tag = CFLSH_BLK_GETTAG (cur_lba,chunk->l2setsz);
	line = &chunk->cache [inx];
	lru = line->lrulist;
	
	if ((line) && (line->entry[lru].data)) {
	    
	    /*
	     * Only update cache if data pointer is non-NULL
	     */
	    
	    line->entry[lru].valid = 1;
	    line->entry[lru].tag = tag;
	    
	    
	    bcopy(cur_buf, line->entry[lru].data,nblocks * CAPI_FLASH_BLOCK_SIZE);
	    
	    line->lrulist = line->entry[lru].next;
	}

    } /* for loop */


    return;
}



/*
 * NAME:        CBLK_FREE_CMD
 *
 * FUNCTION:    Marks command as free and ready for reuse
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

static inline void CBLK_FREE_CMD(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd)
{
    if ((chunk == NULL) ||
	(cmd == NULL)) {


	return;
    }


    if ((cmd < chunk->cmd_start) ||
	(cmd > chunk->cmd_end)) {



	CBLK_TRACE_LOG_FILE(1,"cmd = %p is invalid, chunk->num_cmds = %d",
			    cmd,chunk->num_cmds);
	return;

    }


    if (cmd->cmdi == NULL) {


	return;
    }

    if (CBLK_INVALID_CMD_CMDI(chunk,cmd,cmd->cmdi,"CBLK_FREE_CMD")) {

	return;
    }

    cmd->cmdi->in_use = 0;

    /*
     * Remove command from active list
     */

    CBLK_DQ_NODE(chunk->head_act,chunk->tail_act,&(chunk->cmd_info[cmd->index]),act_prev,act_next);

    /*
     * Place command on free list
     */

    CBLK_Q_NODE_TAIL(chunk->head_free,chunk->tail_free,&(chunk->cmd_info[cmd->index]),free_prev,free_next);

    return;
}


/*
 * NAME:        CBLK_FREE_CMDI
 *
 * FUNCTION:    Marks command info as free and ready for reuse
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

static inline void CBLK_FREE_CMDI(cflsh_chunk_t *chunk, cflsh_cmd_info_t *cmdi)
{
    if ((chunk == NULL) ||
	(cmdi == NULL)) {


	return;
    }

    cmdi->in_use = 0;

    /*
     * Remove command from active list
     */

    CBLK_DQ_NODE(chunk->head_act,chunk->tail_act,cmdi,act_prev,act_next);

    /*
     * Place command on free list
     */

    CBLK_Q_NODE_TAIL(chunk->head_free,chunk->tail_free,cmdi,free_prev,free_next);

    return;
}


/*
 * NAME: CBLK_ISSUE_CMD
 *
 * FUNCTION: Issues a commd to the adapter.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
static inline int CBLK_ISSUE_CMD(cflsh_chunk_t *chunk,
				 cflsh_cmd_mgm_t *cmd,void *buf,
				 cflash_offset_t lba,size_t nblocks, int flags)
{
    int rc = 0;
    cflsh_cmd_info_t *cmdi;
#ifdef _COMMON_INTRPT_THREAD
    int pthread_rc;


    if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {
		    
	/*
	 * Notify common async interrupt thread, that it
	 * needs to wait for this command's completion.
	 */

	chunk->thread_flags |= CFLSH_CHNK_POLL_INTRPT;

	pthread_rc = pthread_cond_signal(&(chunk->thread_event));
	
	if (pthread_rc) {
	    
	    CBLK_TRACE_LOG_FILE(1,"pthread_cond_signall failed rc = %d,errno = %d",
				pthread_rc,errno);

	    /*
	     * If we are unable to signal the interrupt thread,
	     * then fail this request now, since we have no guarantee
	     * its completion will be handled.
	     */
	    return -1;
	

	}
    }

#endif /* COMMON_INTRPT_THREAD */


    CBLK_LWSYNC();

    if (CBLK_ISSUE_ADAP_CMD(chunk,cmd)) {

	return -1;
    }



    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmd info is NULL for index = %d,chunk->index = %d",
			    cmd->index,chunk->index);

    }

    cmdi->state = CFLSH_MGM_WAIT_CMP;



    if (!(flags & CFLASH_ISSUE_RETRY)) {
	chunk->num_active_cmds++;

	/*
	 * Save off information
	 * about this request in the
	 * command management structure
	 */

	cmdi->buf = buf;

	cmdi->lba = lba;

	cmdi->nblocks = nblocks;

    }
    
    return rc;
}

/*
 * NAME:        CBLK_IO_LATENCY
 *
 * FUNCTION:    update latency vars for a rd/wr cmd,
 *              trace latency once per second per chunk
 *
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *              cmd   - Cmd which just completed
 *
 * RETURNS:
 */
static inline void CBLK_IO_LATENCY(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd, int lat)
{
    uint64_t          high = 0;
    uint32_t          avg  = 0;

    /* calc and save r/w latency stats */
    if (cmd->cmdi->flags & CFLSH_MODE_READ)
    {
        chunk->rcmd += 1;
        chunk->rlat += lat;
        if (chunk->rcmd > 10)
        {
            avg          = chunk->rlat/chunk->rcmd;
            avg          = avg<25 ? 25 : avg;
            if      (lat < 2*avg)          {++chunk->rlat_avg;}
            else if (lat < 6*avg)          {++chunk->rlat_mid;}
            else if (lat > chunk->rlat_hi) {chunk->rlat_hi=lat;}
        }
    }
    else if (cmd->cmdi->flags & CFLSH_MODE_WRITE)
    {
        chunk->wcmd += 1;
        chunk->wlat += lat;
        if (chunk->wcmd > 10)
        {
            avg          = chunk->wlat/chunk->wcmd;
            avg          = avg<25 ? 25 : avg;
            if      (lat < 2*avg)          {++chunk->wlat_avg;}
            else if (lat < 6*avg)          {++chunk->wlat_mid;}
            else if (lat > chunk->wlat_hi) {chunk->wlat_hi=lat;}
        }
    }

    /* save the cmd ptrc if this command had high latency */
    if (cblk_ptrc_verbosity >= 9)
    {
        if (cblk_ptrc_high_latency)
            {high = cblk_ptrc_high_latency;}
        else
            {high = (chunk->rcmd>1000) ? (chunk->rlat/chunk->rcmd)*10 : 50000;}

        if (lat > high) {PTRC_PSAVE_TO_SLOT(&cmd->cmdi->ptrc, 9, lat);}
    }

    if (cblk_log_verbosity >= 4)
    {
        if (MDELTA(chunk->ltime,cflsh_blk.nspt) > 1000)
        {
            if (chunk->rcmd+chunk->wcmd > 100)
            {
                /* trace latency averages once a second */
                CBLK_TRACE_LOG_FILE(4,
                            "id:%4ld iops:%7ld tlat:%6ld rlat:%6ld wlat:%6ld "
                            "rd(%8d:%6d:%7d) wr(%8d:%6d:%7d)",
                            chunk->index,
                            (chunk->rcmd+chunk->wcmd),
                            (chunk->rcmd+chunk->wcmd) ?
                            (chunk->rlat+chunk->wlat)/(chunk->rcmd+chunk->wcmd) : 0,
                            (chunk->rcmd) ? chunk->rlat/chunk->rcmd : 0,
                            (chunk->wcmd) ? chunk->wlat/chunk->wcmd : 0,
                            chunk->rlat_avg, chunk->rlat_mid, chunk->rlat_hi,
                            chunk->wlat_avg, chunk->wlat_mid, chunk->wlat_hi);
            }
            chunk->ltime    = getticks();
            chunk->rcmd     = 0;
            chunk->wcmd     = 0;
            chunk->rlat     = 0;
            chunk->wlat     = 0;
            chunk->rlat_avg = 0;
            chunk->rlat_mid = 0;
            chunk->rlat_hi  = 0;
            chunk->wlat_avg = 0;
            chunk->wlat_mid = 0;
            chunk->wlat_hi  = 0;
        }
        /* save glb ptrc slots, if any are inuse */
        if (cblk_ptrc_verbosity) {CBLK_PTRC_SAVE_GLBS(1);}
    }
}

/*
 * NAME:        CBLK_COMPLETE_CMD
 *
 * FUNCTION:    Cleans up and ootential frees a command,
 *              which has had its returned status processed
 *              
 *              
 * Environment: This routine assumes the chunk mutex
 *              lock is held by the caller.
 *
 * INPUTS:
 *              chunk - Chunk the cmd is associated.
 *
 *              cmd   - Cmd which just completed
 *
 * RETURNS:
 *             0  - Good completion
 *             -1 - Error
 *              
 *              
 */

static inline int CBLK_COMPLETE_CMD(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd, size_t *transfer_size)
{

    int rc = 0;
    cflsh_cmd_info_t *cmdi;


    if (transfer_size == NULL) {

	return (-1);
    }

    if (cmd == NULL) {

	return (-1);
    }


    cmdi = &chunk->cmd_info[cmd->index];

    if (cmdi == NULL) {

	return (-1);
    }

    if (cmdi->in_use == 0) {

	*transfer_size = 0;

	return (rc);

    }


    *transfer_size = cmdi->transfer_size;


    if (cmdi->status) {

	errno = cmdi->status;

	rc = -1;
    }

    int lat = UDELTA(cmdi->orig_stime, cflsh_blk.nspt);

    if (rc < 0)
    {
        PTRC(&cmdi->ptrc, 1, CBLK_CMP_CMD, chunk->index, rc, lat);
        PTRC_PSAVE_TO_SLOT(&cmdi->ptrc, 1, 10000005);
    }
    else {PTRC(&cmdi->ptrc, 5, CBLK_CMP_CMD, chunk->index, rc, lat);}

    /*
     * This command completed,
     * clean it up.
     */
    if ((!(cmdi->flags & CFLSH_ASYNC_IO)) ||
	(cmdi->flags & CFLSH_CMD_INFO_USTAT)) {
		
	/*
	 * For async I/O that are not associated
	 * with user specified status areas don't mark the
	 * command as available yet. Instead
	 * let the caller do this via cblk_aresult
	 */
	chunk->num_active_cmds--;
		    
	if (cmdi->flags & CFLSH_ASYNC_IO) {

	    if (cmdi->flags & CFLSH_MODE_READ) {
			

		chunk->stats.num_blocks_read += cmdi->transfer_size;
		if (chunk->stats.num_act_areads) {
		    chunk->stats.num_act_areads--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x, chunk->index = %d",
					cmdi->flags,chunk->index);
		}

			
	    } else if (cmdi->flags & CFLSH_MODE_WRITE) {
			
		chunk->stats.num_blocks_written += cmdi->transfer_size;
		if (chunk->stats.num_act_awrites) {
		    chunk->stats.num_act_awrites--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x, chunk->index = %d",
					cmdi->flags,chunk->index);
		}
	    } 

	    if (cmdi->flags & CFLSH_CMD_INFO_USTAT) {

		/*
		 * If this is command has a user defined status
		 * area, then update that now before freeing up
		 * the command.
		 */


		/*
		 * TODO: ?? Do we need to do anything like lwsync here?
		 */

		cmdi->user_status->blocks_transferred = cmdi->transfer_size;
		cmdi->user_status->fail_errno = cmdi->status;

		if (cmdi->status == 0) {
		    cmdi->user_status->status = CBLK_ARW_STATUS_SUCCESS;
		} else {
		    cmdi->user_status->status = CBLK_ARW_STATUS_FAIL;
		}
	    }
	    

	} else {

            PTRC(&chunk->ptrc, 6, CBLK_LAT, rc, cmd->cmdi->lba, lat);

            CBLK_IO_LATENCY(chunk, cmd, lat);

            if (cblk_log_verbosity >= cblk_latency_verbosity)
            {
                CBLK_TRACE_LOG_FILE(cblk_latency_verbosity,
                                      "in_use:0x%x lba:%8llx rc:%d lat:%6ld "
                                      "flags:0x%x id:%2d rd:%d",
                                      cmdi->in_use,cmdi->lba,rc,
                                      UDELTA(cmdi->orig_stime,cflsh_blk.nspt),
                                      cmdi->flags, chunk->index,
                                      ((cmdi->flags & CFLSH_MODE_READ)!=0));
            }

	    if (cmdi->flags & CFLSH_MODE_READ) {
			

		chunk->stats.num_blocks_read += cmdi->transfer_size;
		if (chunk->stats.num_act_reads) {
		    chunk->stats.num_act_reads--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x, chunk->index = %d",
					cmdi->flags,chunk->index);
		}

			
	    } else if (cmdi->flags & CFLSH_MODE_WRITE) {
			
		chunk->stats.num_blocks_written += cmdi->transfer_size;
		if (chunk->stats.num_act_writes) {
		    chunk->stats.num_act_writes--;
		} else {
		    CBLK_TRACE_LOG_FILE(1,"!! ----- ISSUE PROBLEM ----- !! flags = 0x%x, chunk->index = %d",
					cmdi->flags,chunk->index);
		}
	    } 

	}
	
	CBLK_FREE_CMD(chunk,cmd);
    }


    return (rc);
}

/*
 * NAME:        CBLK_CHECK_COMPLETE_PATH
 *
 * FUNCTION:    Checks for commands received on this AFU, but
 *              via a different chunk that have not been processed.
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

static inline int  CBLK_CHECK_COMPLETE_PATH(cflsh_chunk_t *chunk, int path_index,
                                                 cflsh_cmd_mgm_t *cmd,
                                                 size_t *transfer_size, 
                                                 int *cmd_complete)
{

    cflsh_cmd_info_t *p_cmdi;
    cflsh_cmd_info_t *cmdi;
    cflsh_cmd_mgm_t *p_cmd;
    int rc = 0;
    int cmd_rc = 0;
    cflash_block_check_os_status_t reset_status = CFLSH_BLK_CHK_OS_NO_RESET;
    


    if (chunk == NULL) {

	CBLK_TRACE_LOG_FILE(8,"chunk is null");
	errno = EINVAL;
	return -1;
    }
       
    if ( (ulong)chunk & CHUNK_BAD_ADDR_MASK ) {

	CBLK_TRACE_LOG_FILE(1,"Corrupted chunk address = %p", 
                                chunk);

	errno = EINVAL;
	return -1;

    }
    
    if (cmd) {

	if (cmd->cmdi == NULL) {

	    CBLK_TRACE_LOG_FILE(8,"cmdi is nulll for cmd = 0x%llx chunk->index = %d",
				(uint64_t)cmd,chunk->index);
	    errno = EINVAL;
	    return -1;
	}

	if (cmd->cmdi->state == CFLSH_MGM_CMP) {

	    /*
	     * Our command completed.
	     */
	    CBLK_TRACE_LOG_FILE(8,"check cmd  lba = 0x%llx, cmd = 0x%llx cmd_index = %d, chunk->index = %d",
				cmd->cmdi->lba,(uint64_t)cmd,cmd->index,chunk->index);
	    rc = CBLK_COMPLETE_CMD(chunk,cmd,transfer_size);

	    *cmd_complete = TRUE;

	    return rc;
	} else if (cmd->cmdi->state == CFLSH_MGM_ASY_CMP) {

	    /*
	     * We have already return status
	     * back to caller for this
	     * command and are just waiting
	     * for the async interrupt thread
	     * (most likely this thread) to complete
	     * before the command can be freed.
	     */

	    *cmd_complete = TRUE;


	    return rc;
	}

	
	if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	    cmdi = cmd->cmdi;

	    if (cmdi) {

		if (CBLK_CMD_TIMEOUT(cmdi)) {

		    CBLK_TRACE_LOG_FILE(4,"Timeout for  cmd  lba = 0x%llx, cmd = 0x%llx cmd_index = %d, chunk->index = %d",
				cmdi->lba,(uint64_t)cmd,cmd->index,chunk->index);
		    PTRC(&cmdi->ptrc, 1, CBLK_CMD_TIMEDOUT, chunk->index, &(cmd->sisl_cmd.rcb),
		                    UDELTA(cmdi->orig_stime,cflsh_blk.nspt));
		    PTRC_PSAVE_TO_SLOT(&cmdi->ptrc, 1, 10000005);
		    cmdi->status = 0;
		    cmdi->transfer_size = 0;

		    
		    chunk->stats.num_fail_timeouts++;

		    
		    CBLK_GET_INTRPT_STATUS(chunk,path_index,&reset_status);
		    
		    if (reset_status == CFLSH_BLK_CHK_OS_NO_RESET) {
			
			/*
			 * If we did not detect UE, when reading the 
			 * interrupt status registers. So instead do
			 * context reset.
			 */
			
			cblk_notify_mc_err(chunk,cmdi->path_index,0x300,0,
					   CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
			
			CFLASH_BLOCK_UNLOCK(chunk->lock);
			
			PTRC(&cmdi->ptrc, 1, CBLK_CTXT_RESET, chunk->index, &(cmd->sisl_cmd.rcb),
			                UDELTA(cmdi->stime,cflsh_blk.nspt));
			PTRC_PSAVE_TO_SLOT(&cmdi->ptrc, 1, 10000010);

			cblk_reset_context_shared_afu(chunk->path[path_index]->afu);
			
			CFLASH_BLOCK_LOCK(chunk->lock);
			
			if ((cmdi->retry_count >= CAPI_CMD_MAX_RETRIES) ||
			    (cmdi->status)) {
			    
			    /*
			     * Don't fail yet, until all retries have
			     * been attempted. Also if the resume command
			     * logic in the block library fails to re-issue
			     * the command, then cmdi->status will be non-zero.
			     * NOTE: We cleared cmdi->status before unlocking
			     * and calling reset_context.  
			     */
			    
			    CBLK_TRACE_LOG_FILE(4,"Timeout for exceeded retries for  cmd  lba = 0x%llx, cmd = 0x%llx cmdi->retry_count = %d, cmdi->status = 0x%x",
						cmdi->lba,(uint64_t)cmd,cmdi->retry_count,cmdi->status);
			    
			    cmdi->status = ETIMEDOUT;
			    
			    *cmd_complete = TRUE;
			    
			    errno = ETIMEDOUT;
			    
			    rc = -1;
			}
		    }
		}

	    } else {

		CBLK_TRACE_LOG_FILE(1,"invalid cmdi for cmd  cmd = 0x%llx cmd_index = %d, chunk->index = %d",
				(uint64_t)cmd,cmd->index,chunk->index);	
	    }
	}

    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);
    
    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
	
	/*
	 * If path is in a halted state then return 
	 * from this routine
	 */
	
	    
	CBLK_TRACE_LOG_FILE(5,"afu halted exiting, afu->flags = 0x%x",
			    chunk->path[path_index]->afu->flags);
	    

	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	return rc;

	
	
    }

    p_cmdi = chunk->path[path_index]->afu->head_complete;
    while (p_cmdi) {

        cmdi = p_cmdi->complete_next;

	if (CFLSH_EYECATCH_CMDI(p_cmdi)) {

	    CBLK_TRACE_LOG_FILE(1,"invalid eyecatcher for cmdi = 0x%x",
				cmdi->eyec);	

	    break;
	}


        if (p_cmdi->chunk == chunk) {

            /* 
             * This is a command that has completed
             * while waiting for commands on another chunk.
             * Let's handle it now.
             */

            p_cmd = &chunk->cmd_start[p_cmdi->index];


            CBLK_DQ_NODE(chunk->path[path_index]->afu->head_complete,chunk->path[path_index]->afu->tail_complete,p_cmdi,
                             complete_prev,complete_next);

            p_cmdi->state = CFLSH_MGM_CMP;
            
                
            cmd_rc = cblk_process_cmd(chunk,path_index,p_cmd);
            
        
            if (p_cmd == cmd) {
                
                /*
                 * Our command completed, Let's process it.
                 */
                
        
                if ((cmd_rc != CFLASH_CMD_RETRY_ERR) &&
                    (cmd_rc != CFLASH_CMD_DLY_RETRY_ERR)) {
                    
                    /*
                     * Since we found our command completed and
                     * we are not retrying it, lets
                     * set the flag so we can avoid polling for any
                     * more interrupts. However we need to process
                     * all responses posted to the RRQ for this
                     * interrupt before exiting.
                     */
#ifndef _COMMON_INTRPT_THREAD
                    
                    CBLK_COMPLETE_CMD(chunk,cmd,transfer_size);
#else
                    
                    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {
                        CBLK_COMPLETE_CMD(chunk,cmd,transfer_size);
                    }
                    
#endif

                    *cmd_complete = TRUE;

		    if (cmd_rc == CFLASH_CMD_FATAL_ERR) {

			rc = -1;
		    }
                    
                }
                
            }    


        }

        p_cmdi = cmdi;
    }
  

    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);


    return rc;
}



/*
 * NAME:        CBLK_WAIT_FOR_IO_COMPLETE_PATH
 *
 * FUNCTION:    Waits for the specified cmd to receive
 *              a completion or time-out.
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

static inline int CBLK_WAIT_FOR_IO_COMPLETE_PATH(cflsh_chunk_t *chunk, int path_index,
						 cflsh_cmd_mgm_t *cmd,
						 int *cmd_index, size_t *transfer_size, 
						 int wait,int *cmd_complete,
						 int *poll_retry, int *poll_fail_retries)
{
    int rc = 0;
#ifndef BLOCK_FILEMODE_ENABLED
    int pthread_rc;
#endif
 
#ifndef _SKIP_POLL_CALL
    cflsh_cmd_info_t *cmdi = NULL;
    int poll_ret=0;
    CFLASH_POLL_LIST_INIT(chunk,(chunk->path[path_index]),poll_list);
#endif /* _SKIP_POLL_CALL */
    cflash_block_check_os_status_t reset_status = CFLSH_BLK_CHK_OS_NO_RESET;


    ticks b4polls=getticks();


    if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,"CBLK_WAIT_FOR_IO_COMPLETE_PATH")) {

        errno = EINVAL;
	return -1;

    }


    CBLK_TRACE_LOG_FILE(9,"waiting for cmd with cmd_index = 0x%x on chunk->index = %d, path_index = %d",
			*cmd_index,chunk->index,path_index);
    CBLK_TRACE_LOG_FILE(9,"chunk->fd = %d, poll_fd= %d, afu = %p",
			chunk->fd,chunk->path[path_index]->afu->poll_fd,chunk->path[path_index]->afu);

#ifndef BLOCK_FILEMODE_ENABLED

    CFLASH_BLOCK_LOCK(chunk->lock);

    if (cmd) {
	CBLK_TRACE_LOG_FILE(8,"check cmd  lba = 0x%llx, cmd = 0x%llx cmd_index = %d, chunk->index = %d",
			    cmd->cmdi->lba,(uint64_t)cmd,cmd->index,chunk->index);
    } else if (chunk->num_active_cmds == 0) {

	/*
	 * If we do not have a specific command and there
	 * are no commands active, then let's give up.
	 */

	CFLASH_BLOCK_UNLOCK(chunk->lock);


	return (-1);

    }

    /*
     * Check if our command has already been
     * completed.
     */

    rc = CBLK_CHECK_COMPLETE_PATH(chunk,path_index,cmd,transfer_size,cmd_complete);
    if (rc) {

	
        CFLASH_BLOCK_UNLOCK(chunk->lock);
        
        return rc;
    }


    if ((rc) || (*cmd_complete)) {
        
        CFLASH_BLOCK_UNLOCK(chunk->lock);
        
        return rc;
    }


    if (chunk->flags & CFLSH_CHNK_HALTED) {

	/*
	 * This chunk is in a halted state. Wait
	 * the chunk to be resumed.
	 */
	CBLK_TRACE_LOG_FILE(5,"chunk in halted state  chunk->index = %d",
			    chunk->index);

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


	pthread_rc = pthread_cond_wait(&(chunk->path[path_index]->resume_event),&(chunk->lock.plock));
	
	if (pthread_rc) {
	    



	    CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed for resume_event rc = %d errno = %d",
				pthread_rc,errno);
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    errno = EIO;
	    return -1;
	}


    }

#ifndef _SKIP_POLL_CALL

    if (wait)
    {
        /* if not 1st time here, and elapse time < 250us, skip poll */
        if (chunk->ktime && UDELTA(chunk->ktime, cflsh_blk.nspt) > 250)
        {
            chunk->ktime = getticks();
            CFLASH_CLR_POLL_REVENTS(chunk,(chunk->path[path_index]),poll_list);

            CFLASH_BLOCK_UNLOCK(chunk->lock);

            if (cmd) {
                CBLK_TRACE_LOG_FILE(8,"poll for cmd  lba = 0x%llx",cmd->cmdi->lba);
            }

            poll_ret = CFLASH_POLL(poll_list,CAPI_POLL_IO_TIME_OUT);

            CFLASH_BLOCK_LOCK(chunk->lock);

            if (cmd)
            {
                PTRC(&cmd->cmdi->ptrc, 5, CBLK_CFLASH_POLL, chunk->index,
                      cmd->cmdi->lba, UDELTA(chunk->ktime, cflsh_blk.nspt));
            }
            chunk->ktime = getticks();

            CBLK_TRACE_LOG_FILE(8,"poll_ret = 0x%x, chunk->index = %d, path_index = %d",poll_ret,chunk->index,path_index);
        }
        else
        {
            /*
             * Simulate POLLIN event
             */
            CFLASH_SET_POLLIN(chunk,(chunk->path[path_index]),poll_list);
            poll_ret = 1;
        }

	/*
	 * Check if our command has already been
	 * completed.
	 */

	rc = CBLK_CHECK_COMPLETE_PATH(chunk,path_index,cmd,transfer_size,cmd_complete);
	

	if ((rc) || (*cmd_complete)) {
        
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
        
	    return rc;
	}


	if (chunk->num_active_cmds == 0) {

	    /*
	     * If we do not have a specific command and there
	     * are no commands active, then let's give up.
	     */

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return rc;


	}

    } else {

        // TODO: ?? Should we still continue here if we received a poll?

        /*
	 * Simulate POLLIN event
	 */

	CFLASH_SET_POLLIN(chunk,(chunk->path[path_index]),poll_list);
	poll_ret = 1;
    } 

#endif /* !_SKIP_POLL_CALL */



#else
    /*
     * This is BLOCK_FILEMODE_ENABLED simulation
     */

    poll_ret = 1;

    CFLASH_BLOCK_LOCK(chunk->lock);

#ifndef _COMMON_INTRPT_THREAD

    if ((*cmd_index == -1) ||
	(cmd->cmdi == NULL)) {

	/*
	 * FILE_MODE can not work if no tag is specified
	 * Thus fail now.
	 */

	rc = -1;

	errno = EINVAL;

	CBLK_TRACE_LOG_FILE(5,"Invalid cmd_index");
	CFLASH_BLOCK_UNLOCK(chunk->lock);
	return rc;

    }


    if ((cmd->cmdi->in_use == 0) || (cmd->cmdi->state == CFLSH_MGM_ASY_CMP)) {

	CBLK_TRACE_LOG_FILE(1,"cmd->cmdi->in_use = 0 flags = 0x%x lba = 0x%llx, chunk->index = %d",
			    cmd->cmdi->flags,cmd->cmdi->lba,chunk->index);
	rc = -1;

	errno = EINVAL;

	CFLASH_BLOCK_UNLOCK(chunk->lock);
	return rc;

    }

    cblk_filemode_io(chunk,cmd);
#else

    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {

	if ((*cmd_index == -1) ||
	    (cmd->cmdi == NULL)) {

	    /*
	     * FILE_MODE can not work if no tag is specified
	     * Thus fail now.
	     */

	    rc = -1;

	    errno = EINVAL;

	    CBLK_TRACE_LOG_FILE(5,"Invalid cmd_index");
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return rc;

	}


	if ((cmd->cmdi->in_use == 0) || (cmd->cmdi->state == CFLSH_MGM_ASY_CMP)) {

	    CBLK_TRACE_LOG_FILE(1,"cmd->cmdi->in_use = 0 flags = 0x%x lba = 0x%llx, chunk->index = %d",
				cmd->cmdi->flags,cmd->cmdi->lba,chunk->index);
	    rc = -1;

	    errno = EINVAL;

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	    return rc;

	}

	cblk_filemode_io(chunk,cmd);

    }

#endif /* _COMMON_INTRPT_THREAD */

#endif /* BLOCK_FILEMODE_ENABLED */

#ifndef _SKIP_POLL_CALL

    if ((poll_ret == 0) && (wait)) {

	/* 
	 * We timed-out waiting for a command
	 * to complete. First let's check to see if
	 * perhaps our command has already completed (possibly
	 * via another thread). If so then we can process it
	 * now. Otherwise this is is an error.
	 */


	if ((cmd) && ((!cmd->cmdi->in_use) || (cmd->cmdi->state == CFLSH_MGM_ASY_CMP))) {

		
	    CBLK_TRACE_LOG_FILE(5,"cmd time-out unnecessary since cmd not in use cmd = 0x%llx, chunk->index = %d",
				(uint64_t)cmd,chunk->index);

	    CFLASH_BLOCK_UNLOCK(chunk->lock);

	    return rc;
	}

       
        
	rc = CBLK_CHECK_COMPLETE_PATH(chunk,path_index,cmd,transfer_size,cmd_complete);
	if (rc) {

	
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
        
	    return rc;
	}




	if (chunk->num_active_cmds) {


	   

	    rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,&cmd,
						  CFLSH_BLK_INTRPT_CMD_CMPLT,
						  cmd_complete, transfer_size);
#ifndef _ERROR_INTR_MODE

	    /*
	     * Only use these traces and error logs if
	     * we are not in error interrupt mode (i.e
	     * if we are relying on command completion 
	     * interrupts
	     */

	    if ((cmd) && (*cmd_complete)) {


		CBLK_NOTIFY_LOG_THRESHOLD(9,chunk,path_index,0x302,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);


		CBLK_TRACE_LOG_FILE(5,"Poll time-out, but cmd complete cmd with lba = 0x%llx, with rc = %d, errno = %d in_use = %d, chunk->index = %d",
			    cmd->cmdi->lba, rc, errno,cmd->cmdi->in_use,chunk->index);

		
		CBLK_TRACE_LOG_FILE(5,"Poll time-out, but cmd = %p",
			    cmd);
	    }
#endif /* ! _ERROR_INTR_MODE */
	}


	if ((rc) || (*cmd_complete)) {
        
	    CFLASH_BLOCK_UNLOCK(chunk->lock);
        
	    return rc;
	} else {

	    /*
	     * This appears to be a a potential time-out
	     */

#ifdef _COMMON_INTRPT_THREAD
	    if ((*poll_retry) &&
		(!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) &&
		(cmd == NULL) &&
		(chunk->num_active_cmds == 0)) {

		/*
		 * When using a single common interrupt
		 * thread for all interrupts, we do not
		 * detect time-outs from poll time outs 
		 * Instead they are detected in the common
		 * interrupt thread that looks at command time
		 * stamps.  So if there have no active commands
		 * then we want to exit this loop. We are allowing
		 * at least on poll time-out iteraction in case
		 * something gets issued when we unlock and
		 * num_active_cmds is about to increase.
		 *
		 * NOTE: Currently for common interrupt
		 *       thread one other caller (cblk_aresult)
		 *       can also call this routine if one
		 *       asked to wait for the next tag.
		 *       In that case it will also specify 
		 *       no initial tag, but it needs to wait
		 *       for commands to complete.  
		 */

		    
		CFLASH_BLOCK_UNLOCK(chunk->lock);
		return rc;


	    }

#else

	    CBLK_GET_INTRPT_STATUS(chunk,path_index,&reset_status);

		
#endif /* _COMMON_INTRPT_THREAD */

	    (*poll_retry)++;


	    rc = -1;

	    errno = ETIMEDOUT;

	    if (chunk->path[path_index]->afu->p_hrrq_curr) {

		CBLK_TRACE_LOG_FILE(7,"*(chunk->path[%d]->afu->p_hrrq_curr) = 0x%llx, chunk->path[path_index]->toggle = 0x%llx , chunk->index = %d",
				    path_index,
				    CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),chunk->path[path_index]->afu->toggle,
				    chunk->index);	

	    }

	    if (cmd) {
	    

		cmd->cmdi->status = errno;

		cmd->cmdi->transfer_size = 0;

		CBLK_TRACE_LOG_FILE(6,"potential cmd time-out   lba = 0x%llx flags = 0x%x, chunk->index = %d",
				    cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
	    } else {

		CBLK_TRACE_LOG_FILE(6,"potential cmd time-out no command specified, chunk->index = %d",chunk->index);
	    }

	}

	chunk->stats.num_timeouts++;

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	if (*poll_retry < CFLASH_MAX_POLL_RETRIES) {


	    return rc;
	} 
	
	
	if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {
	    /*
	     * If we are not using a back ground thread
	     * for polling, then get interrupt status
	     */
	    
	    CBLK_GET_INTRPT_STATUS(chunk,path_index,&reset_status);
	}
	if (cmd) {


	    CFLASH_BLOCK_LOCK(chunk->lock);

	    chunk->stats.num_fail_timeouts++;

	    if (chunk->flags & CFLSH_CHNK_NO_BG_TD){
		
		/*
		 * Mark command as complete (failed).
		 */


		cmdi = cmd->cmdi;

		if (cmdi) {

		    if (CBLK_CMD_TIMEOUT(cmdi)) {

			/*
			 * Don't fail yet, until all retries have
			 * been attempted. Also if the resume command
			 * logic in the block library fails to re-issue
			 * the command, then cmdi->status will be non-zero.
			 * NOTE: We cleared cmdi->status before unlocking
			 * and calling reset_context.  
			 */

			CBLK_TRACE_LOG_FILE(1,"Timeout for for cmd  lba = 0x%llx, cmd = 0x%llx cmd_index = %d, chunk->index = %d",
					    cmdi->lba,(uint64_t)cmd,cmd->index,chunk->index);

		    
			cmdi->status = 0;
			cmdi->transfer_size = 0;

		    
		    
			CBLK_GET_INTRPT_STATUS(chunk,path_index,&reset_status);
			
			
			if (reset_status == CFLSH_BLK_CHK_OS_NO_RESET) {
			    
			    /*
			     * If we did not detect UE, when reading the 
			     * interrupt status registers. So instead do
			     * context reset.
			     */
			    
			    cblk_notify_mc_err(chunk,cmdi->path_index,0x301,0,
					       CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
			    
			    CFLASH_BLOCK_UNLOCK(chunk->lock);
			    
			    cblk_reset_context_shared_afu(chunk->path[path_index]->afu);
			    
			    CFLASH_BLOCK_LOCK(chunk->lock);
			    
			    if ((cmdi->retry_count >= CAPI_CMD_MAX_RETRIES) ||
				(cmdi->status)) {
				
				CBLK_TRACE_LOG_FILE(1,"Timeout for exceeded retries for  cmd  lba = 0x%llx, cmd = 0x%llx cmdi->retry_count = %d, cmdi->status = 0x%x",
						    cmdi->lba,(uint64_t)cmd,cmdi->retry_count,cmdi->status);
				
				*cmd_complete = TRUE;
				
				cmdi->status = ETIMEDOUT;
				errno = ETIMEDOUT;
				
				rc = -1;
			    }

			}
		    }

		    /*
		     * The caller should retry this until either time-out is 
		     * exceeded or the command completes. So even for N_BG_TD,
		     * we just return here (a few lines below).
		     */

		}

	    
		
	    }

	    CFLASH_BLOCK_UNLOCK(chunk->lock);
	}

	
	return rc;
	    
    } else if (poll_ret < 0) {

	    
	/* 
	 * Poll failed, Give up
	 */


	if (errno == EIO) {


	    CBLK_TRACE_LOG_FILE(1,"Potential UE encountered for command room\n");

	    cblk_check_os_adap_err(chunk,path_index);
	}
	    

	CBLK_TRACE_LOG_FILE(1,"poll failed, with errno = %d, chunk->index = %d",errno,chunk->index);

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	(*poll_fail_retries)++;

	if (*poll_fail_retries < CFLASH_MAX_POLL_FAIL_RETRIES) {

	    /*
	     * Retry
	     */

	    return rc;
	}

	if (cmd) {
		
		
	    cmd->cmdi->status = errno;
		
	    cmd->cmdi->transfer_size = 0;
		
	    CBLK_TRACE_LOG_FILE(6,"Poll failure  lba = 0x%llx flags = 0x%x, chunk->index = %d",
				cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
	}
	return rc;
	    
    } else {
#endif /* !_SKIP_POLL_CALL */	    
	/*
	 * We may have received events for this file descriptor. Let's
	 * first read the events and then process them accordingly.
	 */
#ifndef _SKIP_READ_CALL

	rc = cblk_read_os_specific_intrpt_event(chunk,path_index,&cmd,cmd_complete,transfer_size,poll_list);

#else
	/*
	 * Process command completion interrupt.
	 */
	rc = CBLK_PROCESS_ADAP_CONVERT_INTRPT(chunk,&cmd,CFLSH_BLK_INTRPT_CMD_CMPLT,cmd_complete, transfer_size);

#endif


#ifndef _SKIP_POLL_CALL	    
    }

#endif /* !_SKIP_POLL_CALL */	   

    CFLASH_BLOCK_UNLOCK(chunk->lock);


    if (cmd && cmd->cmdi && cmd->cmdi->in_use==1)
    {
        CBLK_TRACE_LOG_FILE(5,"waiting returned for cmd with lba = 0x%llx, with rc = %d, errno = %d in_use = %d, chunk->index = %d",
                            cmd->cmdi->lba, rc, errno,cmd->cmdi->in_use,chunk->index);

        PTRC(&cmd->cmdi->ptrc, 5, CBLK_W4CMP_PATH,
             chunk->index, cmd->cmdi->lba, UDELTA(b4polls,cflsh_blk.nspt));
    }
    else
    {
        CBLK_TRACE_LOG_FILE(5,"waiting returned cmd:0x%llx rc:%d errno:%d chunk:%d",
                            cmd, rc, errno, chunk->index);
    }


    return rc;
}


/*
 * NAME:        CBLK_WAIT_FOR_IO_COMPLETE
 *
 * FUNCTION:    Waits for the specified cmd to receive
 *              a completion or time-out.
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

static inline int CBLK_WAIT_FOR_IO_COMPLETE(cflsh_chunk_t *chunk,
					    int *cmd_index, size_t *transfer_size, 
					    int wait, int lib_flags)
{
    int rc = 0;
    int cmd_complete = FALSE;
    cflsh_cmd_mgm_t *cmd = NULL;
    int path_index = -1;
    int poll_retry = 0;
    int poll_fail_retries = 0;


    

    if (chunk == NULL) {
	
	CBLK_TRACE_LOG_FILE(1,"chunk is null");
	
        errno = EINVAL;
	return (-1);
    }

    if (cmd_index == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmd_index is null");
        errno = EINVAL;
	return (-1);
    }

    CBLK_TRACE_LOG_FILE(5,"waiting for cmd with cmd_index = 0x%x on chunk->index = %d",
			*cmd_index,chunk->index);




    if (*cmd_index != -1) {

        /* 
	 * A tag of -1, indicates the caller wants
	 * this routine to return when any command completes.
	 */

        if ((*cmd_index >= chunk->num_cmds) ||
	    (*cmd_index < 0)) {
	
	    CBLK_TRACE_LOG_FILE(1,"Invalid cmd_index = 0x%x, chunk->index = %d",*cmd_index,chunk->index);

	    errno = EINVAL;
	    return (-1);
	}


	cmd = &(chunk->cmd_start[*cmd_index]);

        if (cmd->cmdi == NULL) {
	
	    CBLK_TRACE_LOG_FILE(1,"Invalid cmd_index = 0x%x, chunk->index = %d, cmdi is null",*cmd_index,chunk->index);

	    errno = EINVAL;
	    return (-1);
	}
    
	if ( (cmd->cmdi->in_use == 0) || (cmd->cmdi->state == CFLSH_MGM_ASY_CMP)) {

	    CBLK_TRACE_LOG_FILE(1,"cmd->cmdi->in_use = 0 flags = 0x%x lba = 0x%llx, chunk->index = %d chunk->flags = %d",
				cmd->cmdi->flags,cmd->cmdi->lba,chunk->index,chunk->flags);

	    errno = EINVAL;

	    return -1;
	}

	path_index = chunk->cmd_info[*cmd_index].path_index;


	CBLK_TRACE_LOG_FILE(7,"waiting for cmd with lba = 0x%llx flags = 0x%x, chunk->index = %d",
			    cmd->cmdi->lba,cmd->cmdi->flags,chunk->index);
    }

    if (cmd)
    {
        if (chunk->cmd_info[cmd->index].spolltime)
        {
            PTRC(&cmd->cmdi->ptrc, 5, CBLK_POLLN,
                            UDELTA(cmd->cmdi->lpolltime,cflsh_blk.nspt),
                            cmd->cmdi->lba,
                            UDELTA(cmd->cmdi->spolltime,cflsh_blk.nspt));
        }
        else
        {
            chunk->cmd_info[cmd->index].spolltime = getticks();
            PTRC(&cmd->cmdi->ptrc, 5, CBLK_POLL1, chunk->index, cmd->cmdi->lba,
                 UDELTA(cmd->cmdi->orig_stime,cflsh_blk.nspt));
        }
        chunk->cmd_info[cmd->index].lpolltime = getticks();
    }

    if (path_index >= 0)
    {
        cflsh_cmd_info_t *pcmd = NULL;

        poll_retry        = 0;
        poll_fail_retries = 0;

        /*
         * If there are commands for this path then check for completion.
         */
        while (!cmd_complete && chunk->num_active_cmds)
        {
            /* set pcmd for timeout check */
            if (cmd) {pcmd = cmd->cmdi;}
            else     {pcmd = chunk->head_act;}

            rc = CBLK_WAIT_FOR_IO_COMPLETE_PATH(chunk, path_index,cmd,cmd_index, transfer_size,wait,
                            &cmd_complete,&poll_retry,&poll_fail_retries);

            if (rc != 0 && CBLK_CMD_TIMEOUT(pcmd)) {break;}

            /* if caller is requesting we don't retry */
            if (lib_flags & CFLASH_SHORT_POLL) {break;}

            /* if this is the intrpt thd, dont wait until cmd timeout */
            if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {break;}

#ifdef _AIX
            usleep(5);
#endif
        }
    }
    else
    {
        CBLK_TRACE_LOG_FILE(9,"chunk->index = %d, num_paths = %d",
                        chunk->index,chunk->num_paths);

        for (path_index=0;path_index < chunk->num_paths;path_index++)
        {
            if (chunk->path[path_index]->afu->num_issued_cmds)
            {
                cflsh_cmd_info_t *pcmd = NULL;

                poll_retry        = 0;
                poll_fail_retries = 0;

                /*
                 * If there are commands for this path then check for completion.
                 */
                while (!cmd_complete && chunk->num_active_cmds)
                {
                    /* set pcmd for timeout check */
                    if (cmd) {pcmd = cmd->cmdi;}
                    else     {pcmd = chunk->head_act;}

                    rc = CBLK_WAIT_FOR_IO_COMPLETE_PATH(chunk, path_index,cmd,
                                    cmd_index, transfer_size,wait,
                                    &cmd_complete,&poll_retry,&poll_fail_retries);

                    if (rc != 0 && CBLK_CMD_TIMEOUT(pcmd)) {break;}

                    /* if caller is requesting we don't retry */
                    if (lib_flags & CFLASH_SHORT_POLL) {break;}

                    /* if this is the intrpt thd, dont wait until cmd timeout */
                    if (!(chunk->flags & CFLSH_CHNK_NO_BG_TD)) {break;}

#ifdef _AIX
                    usleep(5);
#endif
                }
            }
        }
    }

    if (cmd && cmd->cmdi)
    {
        if (rc==0) {errno=0;}

	CBLK_TRACE_LOG_FILE(5,"waiting returned for cmd with lba = 0x%llx, with rc = %d, errno = %d in_use = %d, chunk->index = %d",
			    cmd->cmdi->lba, rc, errno,cmd->cmdi->in_use,chunk->index);
    }



    return rc;

}



#endif /* _H_CFLASH_BLOCK_INLINE */
