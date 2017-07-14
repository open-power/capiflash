/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_sisl.c $                               */
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

#define CFLSH_BLK_FILENUM 0x0600
#include "cflash_block_internal.h"
#include "cflash_block_inline.h"

#define CFLSH_BLK_SISL_NUM_INTERRUPTS  4

/*
 * NAME: CBLK_IN_MMIO_REG
 *
 * FUNCTION: This routine reads from the AFU registers. 32-bits
 *           applications are limited to 4 byte reads of MMIO 
 *           space at one time. Where 64-bit apps read 8 bytes
 *           of MMIO space at one time. This routine will
 *           attempts to hide the difference between 32-bit and
 *           64-bit apps from its callers (thus avoiding  
 *           ifdefs throughout the code).
 * 
 * NOTE:     The get_msw parameter is only used for 32-bit applications.
 *           For 64-bit applications we always get 8 bytes of data.
 *
 *
 * RETURNS: None
 *     
 *    
 */

static inline __u64 CBLK_IN_MMIO_REG(volatile __u64 *addr, int get_msw) 
{

    __u64 rc;

#if !defined(__64BIT__)  && defined(_AIX)
    __u32 rc32;

    rc = in_mmio32 (addr);

    // Get upper word.


    if (get_msw) {
	rc32 = in_mmio32 (addr+4);

	rc |= (uint64_t)rc32 << 32;
    }
#else
    rc = in_mmio64 (addr);
#endif 
    return rc;
}

/*
 * NAME: CBLK_OUT_MMIO_REG
 *
 * FUNCTION: This routine writes to the AFU registers.32-bits
 *           applications are limited to 4 byte reads of MMIO 
 *           space at one time. Where 64-bit apps read 8 bytes
 *           of MMIO space at one time. This routine will
 *           attempts to hide the difference between 32-bit and
 *           64-bit apps from its callers (thus avoiding  
 *           ifdefs throughout the code). This routine assumes
 *           that there is never an upper word that needs to be
 *           written by a 32-bit app.
 *  
 *
 *
 * RETURNS: None
 *     
 *    
 */

static inline void CBLK_OUT_MMIO_REG(volatile __u64 *addr, __u64 val) 
{
#ifndef BLOCK_FILEMODE_ENABLED
#if !defined(__64BIT__)  && defined(_AIX)

    /*
     * NOTE: We are assuming for 32-bit apps that we
     *       never need to write more than 4 bytes.
     *       In the case of the endian control register,
     *       masking off the lsw is sufficient.
     */
   
    out_mmio32 (addr, (__u32)  (val & 0xffffffff));
#else
    out_mmio64 (addr, val);
#endif 
#endif /* !BLOCK_FILEMODE_ENABLED */
}

/*
 * NAME: cblk_get_sisl_num_interrupts
 *
 * FUNCTION: This routine returns the number of interrupts for this
 *           AFU architecture.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: The number of interrupts supported per context.
 *     
 *    
 */

int  cblk_get_sisl_num_interrupts(cflsh_chunk_t *chunk, int path_index)
{
#ifndef _AIX

    if (cflsh_blk.host_type == CFLASH_HOST_PHYP) {

	/*
	 * For linux pHyp the number of interrupts specified
	 * by user space should not include the page fault 
	 * (PSL) handler interrupt). Thus we must substract 1.
	 * For AIX or BML, the define CFLSH_BLK_SISL_NUM_INTERRUPTS
	 * is the correct number of interrupts to use.
	 */

	CBLK_TRACE_LOG_FILE(9,"This is a pHyp system, num interrupts = 0x%x",
			    (CFLSH_BLK_SISL_NUM_INTERRUPTS-1));

	return (CFLSH_BLK_SISL_NUM_INTERRUPTS-1);

	 
    }

#endif /* !_AIX */

    return CFLSH_BLK_SISL_NUM_INTERRUPTS;

}


/*
 * NAME: cblk_get_sisl_cmd_room
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

uint64_t  cblk_get_sisl_cmd_room(cflsh_chunk_t *chunk, int path_index)
{
    uint64_t cmd_room = 0;


#ifdef BLOCK_FILEMODE_ENABLED

    chunk->path[path_index]->afu->cmd_room = 1;

    cmd_room = chunk->path[path_index]->afu->cmd_room;
#else

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {

	CBLK_TRACE_LOG_FILE(1,"Attempt to read command room, but this AFU is using SQ");
	return 0;
    }

    if (chunk->path[path_index]->afu->cmd_room) {

	cmd_room = chunk->path[path_index]->afu->cmd_room;
	chunk->path[path_index]->afu->cmd_room--;
    } else {
	/*
	 * Read the command room from the adaptere
	 */

	chunk->path[path_index]->afu->cmd_room = CBLK_IN_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_CMD_ROOM_OFFSET,TRUE);

	CBLK_TRACE_LOG_FILE(9,"command room mmio is 0x%llx for path_index %d",
			    chunk->path[path_index]->afu->cmd_room,path_index);

	cmd_room = chunk->path[path_index]->afu->cmd_room;

	if (chunk->path[path_index]->afu->cmd_room == 0xffffffffffffffffLL) {


	    /*
	     * This is not valid. Set it to zero. The caller will
	     * handle UE recovery. So leave cmd_room set to this
	     * value.
	     */

	    chunk->path[path_index]->afu->cmd_room = 0;

	} else if (chunk->path[path_index]->afu->cmd_room) {

	    chunk->path[path_index]->afu->cmd_room--;
	}
    }

#endif
    if (cmd_room == 0) {
	CBLK_TRACE_LOG_FILE(6,"No command room");
	chunk->stats.num_no_cmd_room++;
    }

    return cmd_room;
}

/*
 * NAME: cblk_get_sisl_intrpt_status
 *
 * FUNCTION: This routine is called whenever one needs get 
 *           the interrupt status register.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: Contents of interrupt status registers
 *     
 *    
 */

uint64_t  cblk_get_sisl_intrpt_status(cflsh_chunk_t *chunk, int path_index)
{
    uint64_t intrpt_status = 0;

    /*
     * TODO: ?? Can we consolidate this routine with cblk_process_sisl_error_intrpt
     * used below for processing adapter interrupts.
     */

#ifndef BLOCK_FILEMODE_ENABLED


    /*
     * Read the command room from the adaptere
     */

    intrpt_status = CBLK_IN_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_INTR_STATUS_OFFSET,TRUE);

    CBLK_TRACE_LOG_FILE(9,"interrupt_status is 0x%llx",intrpt_status);


#endif

    return intrpt_status;
}

/*
 * NAME:        cblk_process_sisl_error_intrpt
 *
 * FUNCTION:    This routine processes SISlite adapter
 *              error/miscellaneous interrupts
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command.
 *
 * RETURNS:
 *             
 *              
 */
void cblk_process_sisl_error_intrpt(cflsh_chunk_t *chunk,int path_index,
				    cflsh_cmd_mgm_t **cmd)
{
    uint64_t reg;
    uint64_t reg_unmasked;


    reg = CBLK_IN_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_INTR_STATUS_OFFSET,TRUE);

    reg_unmasked = (reg & SISL_ISTATUS_UNMASK);

    chunk->stats.num_capi_afu_intrpts++;
 
    CBLK_TRACE_LOG_FILE(1,"Unexpected interrupt = 0x%llx, reg_mask = 0x%llx, chunk->index = %d",
			reg,reg_unmasked,chunk->index);

    if (reg_unmasked) {

	CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_INTR_CLEAR_OFFSET, reg_unmasked);

    }

    return;
}


/*
 * NAME: cblk_inc_sisl_rrq
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

void  cblk_inc_sisl_rrq(cflsh_chunk_t *chunk, int path_index)
{


    chunk->path[path_index]->afu->p_hrrq_curr++;



    if (chunk->path[path_index]->afu->p_hrrq_curr > chunk->path[path_index]->afu->p_hrrq_end)
    {

	chunk->path[path_index]->afu->p_hrrq_curr = chunk->path[path_index]->afu->p_hrrq_start;

	chunk->path[path_index]->afu->toggle ^= SISL_RESP_HANDLE_T_BIT;

    }
    

    
    return;
}

/*
 * NAME: cblk_inc_sisl_sq
 *
 * FUNCTION: This routine is called whenever an SQ entry has been issued
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */

void  cblk_inc_sisl_sq(cflsh_chunk_t *chunk, int path_index)
{


    chunk->path[path_index]->afu->p_sq_curr++;



    if (chunk->path[path_index]->afu->p_sq_curr > chunk->path[path_index]->afu->p_sq_end)
    {

	chunk->path[path_index]->afu->p_sq_curr = chunk->path[path_index]->afu->p_sq_start;


    }
    

    
    return;
}


/*
 * NAME: cblk_sisl_adap_sq_setup
 *
 * FUNCTION: This routine is called to set up the adapter to
 *           recognize an SQ
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: 0 - success, otherwise failure
 *          
 *     
 *    
 */

int  cblk_sisl_adap_sq_setup(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;


    if (chunk->path[path_index]->afu->mmio == NULL) {

	CBLK_TRACE_LOG_FILE(1,"mmio is not valid for chunk index = %d, and path_index = %d",
			    chunk->index,path_index);

	return -1;
    }

    if (!(chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ)) {

	CBLK_TRACE_LOG_FILE(1,"this AFU does not have SQ enabled afu->flags 0x%x, chunk index = %d, and path_index = %d",
			    chunk->path[path_index]->afu->flags,chunk->index,path_index);

	return -1;

    }

    CBLK_TRACE_LOG_FILE(9,"setup SQ register afu->p_sq_start = %p",
		       chunk->path[path_index]->afu->p_sq_start);

    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_SQ_START_EA_OFFSET, 
		      (uint64_t)chunk->path[path_index]->afu->p_sq_start);
    
    
    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_SQ_END_EA_OFFSET, 
		      (uint64_t)chunk->path[path_index]->afu->p_sq_end);

    return rc;
}


/*
 * NAME: cblk_sisl_adap_setup
 *
 * FUNCTION: This routine is called to set up the adapter to
 *           recognize our command pool.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: 0 - success, otherwise failure
 *          
 *     
 *    
 */

int  cblk_sisl_adap_setup(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;
#ifdef DEBUG
    uint64_t intrpt_status = 0;
#endif
    uint64_t ctx_control;

    if (CBLK_SETUP_BAD_MMIO_SIGNAL(chunk,path_index,MAX(CAPI_RRQ0_START_EA_OFFSET,CAPI_CTX_CTRL_OFFSET))) {
	
	/*
	 * If we get here then the MMIO below
	 * failed indicating the adapter either
	 * is being reset or encountered a UE.
	 */
	
	return -1;
    }

    if (chunk->path[path_index]->afu->mmio == NULL) {

	CBLK_TRACE_LOG_FILE(1,"mmio is not valid for chunk index = %d, and path_index = %d",
			    chunk->index,path_index);

	return -1;
    }

#ifdef DEBUG
    intrpt_status = cblk_get_sisl_intrpt_status(chunk,path_index);

    CBLK_TRACE_LOG_FILE(5,"Interrupt status before set up of adapter = 0x%llx",
			intrpt_status);
#endif /* DEBUG */

#ifdef _AIX

    /*
     * AIX uses Big Endian format. So set the
     * endian control register now.
     */

    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_ENDIAN_CTRL_OFFSET, (uint64_t)0x8000000000000080LL);

#endif /* AIX */

    CBLK_TRACE_LOG_FILE(9,"setup RRQ registers");

    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_RRQ0_START_EA_OFFSET, 
		      (uint64_t)chunk->path[path_index]->afu->p_hrrq_start);
    
    
    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_RRQ0_END_EA_OFFSET, 
		      (uint64_t)chunk->path[path_index]->afu->p_hrrq_end);


    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {



	rc = cblk_sisl_adap_sq_setup(chunk,path_index);

    }


    /*
     * Set up interrupts for when the interrupt status register
     * is updated to use the SISL_MSI_SYNC_ERROR IRQ.  
     */

    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_CTX_CTRL_OFFSET,(uint64_t)SISL_MSI_SYNC_ERROR);
    CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_INTR_MASK_OFFSET,(uint64_t)SISL_ISTATUS_MASK);

    ctx_control = CBLK_IN_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_CTX_CTRL_OFFSET,TRUE);

    CBLK_TRACE_LOG_FILE(9,"ctx_ctrl register = 0x%llx",
			ctx_control);

    if (ctx_control & CAPI_CTX_CTRL_UNMAP_FLAG) {
	
	CBLK_TRACE_LOG_FILE(9,"Unmap capability supported");
	chunk->path[path_index]->afu->flags |= CFLSH_AFU_UNNAP_SPT;

    }

#ifdef DEBUG
    intrpt_status = cblk_get_sisl_intrpt_status(chunk,path_index);

    CBLK_TRACE_LOG_FILE(5,"Interrupt status after set up of adapter = 0x%llx",
			intrpt_status);
#endif /* DEBUG */


    CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index); 


    return rc;
}


/*
 * NAME: cblk_get_sisl_cmd_data_length
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
uint32_t cblk_get_sisl_cmd_data_length(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd)
{
    sisl_ioarcb_t *ioarcb;


    ioarcb = &(cmd->sisl_cmd.rcb);

    return ioarcb->data_len;
}

/*
 * NAME: cblk_get_sisl_cmd_cdb
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
scsi_cdb_t * cblk_get_sisl_cmd_cdb(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd)
{
    sisl_ioarcb_t *ioarcb;


    ioarcb = &(cmd->sisl_cmd.rcb);

    return (scsi_cdb_t *)ioarcb->cdb;
}

/*
 * NAME: cblk_get_sisl_cmd_rsp
 *
 * FUNCTION: Returns the offset of the command this response is for.
 *  
 *
 * NOTE;    This routine assumes the caller is holding path->lock.
 *
 * RETURNS: 
 *     
 *    
 */
cflsh_cmd_mgm_t *cblk_get_sisl_cmd_rsp(cflsh_chunk_t *chunk,int path_index)
{
    uint64_t value;
#ifdef _AIX
    uint32_t  tmp_val;
#endif /* AIX */
   cflsh_cmd_mgm_t *cmd = NULL; 
   

    value = CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr);


    if (value == 0xffffffffffffffffLL) {

        CBLK_TRACE_LOG_FILE(1,"Potential UE encountered for RRQ\n");
        

        cblk_check_os_adap_err(chunk,path_index);

        /*
         * Tell caller there is no valid command
         */

        return NULL;
    }

    if (chunk->path[path_index]->afu->hrrq_to_ioarcb_off) {

	/*
	 * If the afu's hrrq_to_ioarcb_off field is set
	 * then it indicates the value returned in the RRQ
	 * is not the IOARCB (and thus not the command, since
	 * the IOARCB is at the beginning of the command). Thus
	 * this hrrq_to_ioarcb_off field must be subtracted from
	 * the value returned to get that offset.
	 *
	 * NOTE: This code assumes hrrq_to_ioarcb_off has the low order
	 *       bit (corresponding to the toggle bit) off.
	 */

	if (value <= chunk->path[path_index]->afu->hrrq_to_ioarcb_off) {
        
	    CBLK_TRACE_LOG_FILE(1,"hrrq_to_ioarcb_off 0x%llx is larger than rrq value 0x%llx\n",
				chunk->path[path_index]->afu->hrrq_to_ioarcb_off, value);
	    return NULL;
	}

	value -= chunk->path[path_index]->afu->hrrq_to_ioarcb_off;
    }



#if !defined(__64BIT__) && defined(_AIX)
   
   if ((value) & 0xffffffff00000000LL) {

       /*
	* This is not valid for 32-bit application.
	*/

       CBLK_TRACE_LOG_FILE(1,"Invalid cmd pointer received by AFU = 0x%llx p_hrrq_curr = 0x%llx, chunk->index = %d",
				    (uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);
       cblk_notify_mc_err(chunk,path_index,0x604,value,
			  CFLSH_BLK_NOTIFY_AFU_ERROR,NULL);

       CBLK_LIVE_DUMP_THRESHOLD(1,"0x604");

       /*
	* clear upper word only.
	*/

       //?? What about UE check on store??

       *(chunk->path[path_index]->afu->p_hrrq_curr) &= *(chunk->path[path_index]->afu->p_hrrq_curr) & 0x00000000ffffffffLL;


   }

   tmp_val = ((uint32_t)(value) & 0xffffffff)  & (~SISL_RESP_HANDLE_T_BIT);
   cmd = (cflsh_cmd_mgm_t *)tmp_val;
#else
   cmd = (cflsh_cmd_mgm_t *)((value) & (~SISL_RESP_HANDLE_T_BIT));
#endif 
   

    return cmd;
}



/*
 * NAME: cblk_build_sisl_cmd
 *
 * FUNCTION: Builds a SIS Lite adapter specific command/request.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
int cblk_build_sisl_cmd(cflsh_chunk_t *chunk,int path_index,
			cflsh_cmd_mgm_t *cmd, 
			void *buf, size_t buf_len, 
			int flags)
{
    
    int rc = 0;
    sisl_ioarcb_t *ioarcb;





    ioarcb = &(cmd->sisl_cmd.rcb);


    // TODO: Add mask and maybe macro to get context id

    ioarcb->ctx_id = chunk->path[path_index]->afu->contxt_handle & 0xffff;

#ifdef  _MASTER_CONTXT   

    ioarcb->res_hndl = chunk->path[path_index]->sisl.resrc_handle;
#else
    ioarcb->lun_id = chunk->path[path_index]->lun_id;

    /*
     * Use port selection mask chosen and library
     * initialization time.
     */
    ioarcb->port_sel = cflsh_blk.port_select_mask;
#endif /* _MASTER_CONTXT */

#ifndef _SKIP_READ_CALL
#ifdef _ERROR_INTR_MODE

    /*
     * In error interrupt mode, we only get interrupts
     * for general errors: AFU, EEH etc. We do not get
     * interrupts for command completions.
     */
    ioarcb->msi = 0;
#else
    ioarcb->msi = SISL_MSI_RRQ_UPDATED;
#endif /* !_ERROR_INTR_MODE */

#else

    /*
     * Do not send interrupts to host on completion.
     */

    ioarcb->msi = 0;
#endif 

    if (flags & CFLASH_READ_DIR_OP) {


#ifdef  _MASTER_CONTXT   


	 ioarcb->req_flags = SISL_REQ_FLAGS_RES_HNDL | SISL_REQ_FLAGS_HOST_READ;


#else 

	/*
	 * TODO: This needs to change to resource handle
	 *       lun id when we have this information.
	 *       For now just use PORT and LUN ID. Since
	 *       these are both zero, 
	 *

	 ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;

	*/

#endif /* _MASTER_CONTXT */

    } else if (flags & CFLASH_WRITE_DIR_OP) {

#ifdef  _MASTER_CONTXT   

	ioarcb->req_flags = SISL_REQ_FLAGS_RES_HNDL | SISL_REQ_FLAGS_HOST_WRITE;

#else 
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_WRITE;
#endif /* _MASTER_CONTXT */

    }

    switch (cflsh_blk.timeout_units) {
      case CFLSH_G_TO_MSEC:
	ioarcb->req_flags |= SISL_REQ_FLAGS_TIMEOUT_MSECS;
	break;
      case CFLSH_G_TO_USEC:
	ioarcb->req_flags |= SISL_REQ_FLAGS_TIMEOUT_USECS;
	break;
      default:
	ioarcb->req_flags |= SISL_REQ_FLAGS_TIMEOUT_SECS;
    }

    ioarcb->timeout = cflsh_blk.timeout;




    ioarcb->data_ea = (ulong)buf;

    ioarcb->data_len = buf_len;

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {

	/*
	 * If we are using a submission queue (SQ), then
	 * we must fill in the address of the IOASA for
	 * this IOARCB.
	 */

	ioarcb->sq_ioasa_ea = (uint64_t)&(cmd->sisl_cmd.sa);
    }
    
    return rc;
}


/*
 * NAME: cblk_update_path_sisl_cmd
 *
 * FUNCTION: Updates an already built IOARCB for a new path.
 *  
 *
 * NOTE;    This routine assumes the caller is holding chunk->lock.
 *
 * RETURNS: None
 *     
 *    
 */
int cblk_update_path_sisl_cmd(cflsh_chunk_t *chunk,int path_index,
			      cflsh_cmd_mgm_t *cmd, int flags)
{
    
    int rc = 0;
    sisl_ioarcb_t *ioarcb;


    ioarcb = &(cmd->sisl_cmd.rcb);


    //TODO:?? Future: bzero(ioarcb,sizeof(*ioarcb));

    // TODO: Add mask and maybe macro to get context id

    ioarcb->ctx_id = chunk->path[path_index]->afu->contxt_handle & 0xffff;

#ifdef  _MASTER_CONTXT   

    ioarcb->res_hndl = chunk->path[path_index]->sisl.resrc_handle;
#else
    ioarcb->lun_id = chunk->path[path_index]->lun_id;

    /*
     * Use port selection mask chosen and library
     * initialization time.
     */
    ioarcb->port_sel = cflsh_blk.port_select_mask;
#endif /* _MASTER_CONTXT */


    // TODO:?? This code does not handle fail over from non-SISLITE AFUs

    return rc;
}
/*
 * NAME: cblk_issue_sisl_cmd
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
 
int cblk_issue_sisl_cmd(cflsh_chunk_t *chunk, int path_index,cflsh_cmd_mgm_t *cmd)
{	
    int rc = 0;
    int wait_room_retry = 0;
    int wait_rrq_retry = 0;
    sisl_ioarcb_t *ioarcb;
    int pthread_rc;


    ioarcb = &(cmd->sisl_cmd.rcb);
#ifdef _REMOVE
    if (cblk_log_verbosity >= 9) {
	fprintf(stderr,"Hex dump of ioarcb\n");
	hexdump(ioarcb,sizeof(*ioarcb),NULL);
    }

#endif /* _REMOVE */


#ifdef _FOR_DEBUG
    if (CBLK_SETUP_BAD_MMIO_SIGNAL(chunk,path_index,CAPI_IOARRIN_OFFSET+0x20)) {

	/*
	 * We must have failed the MMIO done below and long
	 * jump here.
	 */

	return -1;
    }

#endif /* _FOR_DEBUG */

#ifdef _USE_LIB_AFU
    afu_mmio_write_dw(p_afu, 8, (uint64_t)ioarcb);
#else

    if (!(chunk->path[chunk->cur_path]->afu->flags & CFLSH_AFU_SQ)) {

	/*
	 * If we are not using a Submission Queue (SQ), then
	 * we need to check command room before attempting
	 * to issue IOARCBs.
	 */

	while ((CBLK_GET_CMD_ROOM(chunk,path_index) == 0)  && 
	       (wait_room_retry < CFLASH_BLOCK_MAX_WAIT_ROOM_RETRIES)) {

	    /*
	     * Wait a limited amount of time for the room on
	     * the AFU. Since we are waiting for the AFU
	     * to fetch some more commands, it is thought
	     * we can wait a little while here. 
	     */

	    CBLK_TRACE_LOG_FILE(5,"waiting for command room path_index = %d", 
				path_index);



	    if (chunk->flags & CFLSH_CHNK_HALTED) {

		/*
		 * If the chunk is halted, then
		 * wait until that situations is cleared.
		 */
		
		CBLK_TRACE_LOG_FILE(5,"waiting2 for command room path_index = %d", 
				    path_index);

		pthread_rc = pthread_cond_wait(&(chunk->path[path_index]->resume_event),&(chunk->lock.plock));
	
		if (pthread_rc) {
	    



		    CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed for resume_event rc = %d errno = %d",
					pthread_rc,errno);

		    /*
		     * So delay instead
		     */

		    CFLASH_BLOCK_UNLOCK(chunk->lock);
		    usleep(CFLASH_BLOCK_DELAY_ROOM);
		    CFLASH_BLOCK_LOCK(chunk->lock);
		}

	    } else {
		CFLASH_BLOCK_UNLOCK(chunk->lock);
		usleep(CFLASH_BLOCK_DELAY_ROOM);
		CFLASH_BLOCK_LOCK(chunk->lock);
	    }
        

	    if (chunk->flags & CFLSH_CHUNK_FAIL_IO) {

		errno = EIO;

		return -1;
	    }

	    wait_room_retry++;
	}

    }

    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
                    
	/*
	 * If path is in a halted state then wait for it to 
	 * resume. Since we are waiting for the AFU resume
	 * event, that afu-lock will be released, but our chunk
	 * lock will not be released. So do that now.
	 *
	 * Since we are going to wait for a
	 * signal from other threads for 
	 * this AFU recovery to complete, we need
	 * release our conditional access of the AFU
	 * lock and get an explicit access of the 
	 * afu lock.
	 */
	
	CBLK_TRACE_LOG_FILE(9,"AFU is halted , for chunk->index = %d, chunk->dev_name = %s, path_index = %d,chunk->flags = 0x%x,afu->flags = 0x%x",
			    chunk->index,chunk->dev_name,path_index,chunk->flags,chunk->path[path_index]->afu->flags);

	CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	CFLASH_BLOCK_UNLOCK(chunk->lock);

	CFLASH_BLOCK_LOCK(chunk->path[path_index]->afu->lock);

	if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {
                    

	    /*
	     * We are still halted after explicitly
	     * acquiring the AFU lock.
	     */

	    pthread_rc = pthread_cond_wait(&(chunk->path[path_index]->afu->resume_event),
					   &(chunk->path[path_index]->afu->lock.plock));

	    if (pthread_rc) {
                        
                        
		CBLK_TRACE_LOG_FILE(5,"pthread_cond_wait failed for resume_event rc = %d errno = %d",
				    pthread_rc,errno);
                        
		errno = EIO;

		CFLASH_BLOCK_UNLOCK(chunk->path[path_index]->afu->lock);         
		CFLASH_BLOCK_LOCK(chunk->lock);           
		return -1;
	    }

	    CBLK_TRACE_LOG_FILE(9,"AFU is resuming , for chunk->index = %d, chunk->dev_name = %s, path_index = %d,chunk->flags = 0x%x,afu->flags = 0x%x",
			    chunk->index,chunk->dev_name,path_index,chunk->flags,chunk->path[path_index]->afu->flags);
	}
   
	/*
	 * Chunk lock must be acquired first to prevent deadlock.
	 * Also get our conditional AFU lock back to match the state
	 * above.
	 */

	CFLASH_BLOCK_UNLOCK(chunk->path[path_index]->afu->lock);          
	CFLASH_BLOCK_LOCK(chunk->lock);
	CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);


	if (chunk->path[path_index]->afu->flags & CFLSH_AFU_HALTED) {

	    /*
	     * Give up if the AFU is still halted.
	     */
         
                        
	    CBLK_TRACE_LOG_FILE(5,"afu halted again afu->flag = 0x%x",
				chunk->path[path_index]->afu->flags);
                        
	    errno = EIO;

	    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);           
	    return -1;

	}
	
    }

    if (!(chunk->path[chunk->cur_path]->afu->flags & CFLSH_AFU_SQ)) {

	/*
	 * If we are not using a Submission Queue (SQ), then
	 * we need to check command room retries.
	 */
    
	if (wait_room_retry >= CFLASH_BLOCK_MAX_WAIT_ROOM_RETRIES) {



	    /*
	     * We do not have any room to send this
	     * command. Since it is possible that AFU
	     * may have been halted and we just woke up,
	     * check command room one more time, before failing
	     * this request.
	     */

	    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu); 

	    if (CBLK_GET_CMD_ROOM(chunk,path_index) == 0) {
#ifdef _FOR_DEBUG
		CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index);
#endif /* _FOR_DEBUG */
		errno = EBUSY;

		cblk_notify_mc_err(chunk,path_index,0x607,wait_room_retry,CFLSH_BLK_NOTIFY_ADAP_ERR,NULL);
		return -1;
	    }

	    CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);
	}
    }

    while ((chunk->path[path_index]->afu->num_issued_cmds >= chunk->path[path_index]->afu->num_rrqs) &&
           (wait_rrq_retry < CFLASH_BLOCK_MAX_WAIT_RRQ_RETRIES)) {

        /*
         * Do not issue more commands to this AFU then there are RRQ for command completions
         */

        
        CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

	/*
	 * Since we are waiting for commands to be processed,
	 * we need to unlock the chunk lock to allow this on
	 * other threads.
	 */


        CFLASH_BLOCK_UNLOCK(chunk->lock);

        usleep(CFLASH_BLOCK_DELAY_RRQ);

	if (chunk->flags & CFLSH_CHUNK_FAIL_IO) {

	    errno = EIO;

	    return -1;
	}

        CFLASH_BLOCK_LOCK(chunk->lock);

        CFLASH_BLOCK_AFU_SHARE_LOCK(chunk->path[path_index]->afu);

	wait_rrq_retry++;

    }

    /*
     * TODO:?? We are not rechecking command room here. Could
     *         we inadvertently attempt to issue a command with no
     *         command room?
     */


    if (wait_rrq_retry >= CFLASH_BLOCK_MAX_WAIT_RRQ_RETRIES) {



        /*
         * We do not have any room to send this
         * command. Fail this operation now.
         */

        CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);

#ifdef _FOR_DEBUG
        CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index);
#endif /* _FOR_DEBUG */
        errno = EBUSY;

        return -1;
    }

    chunk->path[path_index]->afu->num_issued_cmds++;


    /*
     * Update initial time of command, in case the
     * we had to wait above. Thus that wait could
     * cause a premature time-out detection error.
     */

    cmd->cmdi->cmd_time = time(NULL);

    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {
	
	/*
	 * Copy IOARCB to SQ and write.
	 * ?? TODO: Look at avoiding copy here and building 
	 *    directly in IOARCB.
	 */

	bcopy(ioarcb,(void *)chunk->path[path_index]->afu->p_sq_curr,
	      sizeof(*ioarcb));

	CBLK_INC_SQ(chunk,path_index);

	CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_SQ_TAIL_EA_OFFSET, 
			  (uint64_t)chunk->path[path_index]->afu->p_sq_curr);

	CBLK_TRACE_LOG_FILE(9,"SQ mmio = 0x%llx, issued command path_index = %d for ioarcb = %p, chunk->flags = 0x%x, afu = %p", 
			    (uint64_t)chunk->path[path_index]->afu->mmio_mmap,path_index,
			    ioarcb,chunk->flags, chunk->path[path_index]->afu);

    } else {
	CBLK_OUT_MMIO_REG(chunk->path[path_index]->afu->mmio + CAPI_IOARRIN_OFFSET, (uint64_t)ioarcb);

	CBLK_TRACE_LOG_FILE(9,"mmio = 0x%llx, issued command path_index = %d for ioarcb = %p, chunk->flags = 0x%x, afu = %p", 
			    (uint64_t)chunk->path[path_index]->afu->mmio_mmap,path_index,
			    ioarcb,chunk->flags, chunk->path[path_index]->afu);

    }
#endif /* !_USE_LIB_AFU */


    CFLASH_BLOCK_AFU_SHARE_UNLOCK(chunk->path[path_index]->afu);
#ifdef _FOR_DEBUG
    CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index);
#endif /* _FOR_DEBUG */


    return rc;
    
}

/*
 * NAME:        cblk_init_sisl_cmd
 *
 * FUNCTION:    This routine initializes the command
 *              area for a command retry.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command that completed
 *
 * RETURNS:
 *              0  - Good completoin
 *              Otherwise error.
 *              
 */
void cblk_init_sisl_cmd(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{
    sisl_ioarcb_t *ioarcb;

    ioarcb = &(cmd->sisl_cmd.rcb);


    bzero(ioarcb,sizeof(*ioarcb));

    return;
}

/*
 * NAME:        cblk_init_sisl_cmd_rsp
 *
 * FUNCTION:    This routine initializes the command
 *              response area for a command retry.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command that completed
 *
 * RETURNS:
 *              0  - Good completoin
 *              Otherwise error.
 *              
 */
void cblk_init_sisl_cmd_rsp(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{
    sisl_ioasa_t *ioasa;

    ioasa = &(cmd->sisl_cmd.sa);

    bzero(ioasa,sizeof(*ioasa));

    return;
}

/*
 * NAME:        cblk_copy_sisl_cmd_rsp
 *
 * FUNCTION:    This routine copies the response area
 *              for this command to specified buffer,
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command that completed
 *
 * RETURNS:
 *              0  - Good completoin
 *              Otherwise error.
 *              
 */
void cblk_copy_sisl_cmd_rsp(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd, void *buffer, int buffer_size)
{
    sisl_ioasa_t *ioasa;

    ioasa = &(cmd->sisl_cmd.sa);

    bcopy(ioasa,buffer,MIN(sizeof(*ioasa),buffer_size));

    return;
}

/*
 * NAME:        cblk_set_sisl_cmd_rsp_status
 *
 * FUNCTION:    This routine sets the 
 *              response area for a command to either success
 *              or failure based on the flag.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command that completed
 *
 * RETURNS:
 *              0  - Good completoin
 *              Otherwise error.
 *              
 */
void cblk_set_sisl_cmd_rsp_status(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd, int success)
{
    sisl_ioasa_t *ioasa;

    ioasa = &(cmd->sisl_cmd.sa);

    if (success) {
	/*
	 * caller wants to emulate good completion
	 */

	ioasa->ioasc = SISL_IOASC_GOOD_COMPLETION;
    } else {

	/*
	 * caller wants to emulate command failure
	 */
	ioasa->ioasc = 0xFF;
    }

    return;
}


/*
 * NAME:        cblk_complete_status_sisl_cmd
 *
 * FUNCTION:    This routine indicates if there is an error
 *              on the command that completed.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command that completed
 *
 * RETURNS:
 *              0  - Good completoin
 *              Otherwise error.
 *              
 */
int cblk_complete_status_sisl_cmd(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd)
{
    int rc = 0;
    sisl_ioarcb_t *ioarcb;
    sisl_ioasa_t *ioasa = NULL;

    ioarcb = &(cmd->sisl_cmd.rcb);
    ioasa = &(cmd->sisl_cmd.sa);


    if (cmd->cmdi == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmdi is null for cmd->index",cmd->index);
	
        errno = EINVAL;
	return -1;


    }

    if (ioasa->ioasc != SISL_IOASC_GOOD_COMPLETION) {
	
	/*
	 * Command completed with an error
	 */
	rc = -1;
    } else {
	
	
	/*
	 * For good completion set transfer_size 
	 * to full data transfer.
	 */
	
	if (cmd->cmdi->transfer_size_bytes) {
	    
	    /*
	     * The transfer size is in bytes
	     */
	    cmd->cmdi->transfer_size = ioarcb->data_len;
	} else {
	    
	    
	    /*
	     * The transfer size is in blocks
	     */
	    cmd->cmdi->transfer_size = cmd->cmdi->nblocks;
	}
	
    }

    return rc;
}



/*
 * NAME:        cblk_process_sisl_cmd_intrpt
 *
 * FUNCTION:    This routine processes SISlite completion
 *              interrupts
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command.
 *
 * RETURNS:
 *             
 *              
 */
int cblk_process_sisl_cmd_intrpt(cflsh_chunk_t *chunk,int path_index,cflsh_cmd_mgm_t **cmd,int *cmd_complete,size_t *transfer_size)
{
    int rc = 0;
    int cmd_rc = 0;
    cflsh_cmd_mgm_t *p_cmd = NULL;


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


    if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

	CBLK_LIVE_DUMP_THRESHOLD(5,"0x600");

	return -1;

    }

    if (chunk->path[path_index]->afu->p_hrrq_curr == NULL) {

	CBLK_TRACE_LOG_FILE(1,"p_hrrq_curr is null, chunk = %p",chunk);

	CBLK_LIVE_DUMP_THRESHOLD(5,"0x601");
	return -1;
    }



    CBLK_TRACE_LOG_FILE(7,"*(chunk->path[%d]->afu->p_hrrq_curr) = 0x%llx, chunk->path[path_index]->afu->toggle =  0x%llx, p_hrrq_curr = 0x%llx, chunk->index = %d",path_index,
			CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),(uint64_t)chunk->path[path_index]->afu->toggle,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);


    while (((CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr)) & (SISL_RESP_HANDLE_T_BIT)) == chunk->path[path_index]->afu->toggle) {

	/*
	 * Process all RRQs that have been posted via this interrupt
	 */

	p_cmd = CBLK_GET_CMD_RSP(chunk,path_index);

	CBLK_TRACE_LOG_FILE(8,"*(chunk->path[%d].p_hrrq_curr) = 0x%llx, chunk->path[path_index].toggle =  0x%llx, p_hrrq_curr = 0x%llx, chunk->index = %d",
			    path_index,
			    CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),(uint64_t)chunk->path[path_index]->afu->toggle,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);


		
	/*
	 * Increment the RRQ pointer
	 * and possibly adjust the toggle
	 * bit.
	 */

	CBLK_INC_RRQ(chunk,path_index);


	if (p_cmd) {


	    if ((uint64_t)p_cmd & CMD_BAD_ADDR_MASK) {
	
		CBLK_TRACE_LOG_FILE(1,"cmd is not aligned correctly = %p",cmd);
	
		continue;
	    }


	    if (p_cmd->cmdi == NULL) {


		CBLK_TRACE_LOG_FILE(1,"Invalid p_cmd pointer received by AFU = 0x%llx p_hrrq_curr = 0x%llx, chunk->index = %d",
				    (uint64_t)p_cmd,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);
		continue;
	    }

	    if (CBLK_INVALID_CMD_CMDI(p_cmd->cmdi->chunk,p_cmd,p_cmd->cmdi,__FUNCTION__)) {

		CBLK_TRACE_LOG_FILE(1,"Invalid p_cmd pointer received by AFU = 0x%llx p_hrrq_curr = 0x%llx, chunk->index = %d",
				    (uint64_t)p_cmd,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);
		continue;
	    }



	    if ((p_cmd < p_cmd->cmdi->chunk->cmd_start) ||
                (p_cmd > p_cmd->cmdi->chunk->cmd_end)) {



		CBLK_TRACE_LOG_FILE(1,"Invalid p_cmd pointer received by AFU = 0x%llx p_hrrq_curr = 0x%llx, chunk->index = %d",
				    (uint64_t)p_cmd,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);
		if (*cmd) {

		    CBLK_TRACE_LOG_FILE(1,"Invalid p_cmd occurred while waiting for cmd = 0x%llx flags = 0x%x lba = 0x%llx, chunk->index = %d",
					(uint64_t)*cmd,(*cmd)->cmdi->flags,(*cmd)->cmdi->lba,chunk->index);
		}


		CBLK_TRACE_LOG_FILE(7,"*(chunk->path[path_index].p_hrrq_curr) = 0x%llx, chunk->path[path_index].toggle = %d,  p_hrrq_curr = 0x%llx, chunk->index = %d",
				    CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),chunk->path[path_index]->afu->toggle,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);

		continue;
	    } 


	    

            if (p_cmd->cmdi->chunk == chunk) {


		    

		p_cmd->cmdi->state = CFLSH_MGM_CMP;


		cmd_rc = cblk_process_cmd(chunk,path_index,p_cmd);

		if ((*cmd == NULL) &&
		    (!(*cmd_complete))) {

		    /* 
		     * The caller is waiting for the next
		     * command. So set cmd to this
		     * command (p_cmd) that just completed.
		     */
		    *cmd = p_cmd;

		}

	    } else {

		/*
		 * Since this command info is for a different chunk than the one being
		 * processed (and the one for which were currently have a lock), put this
		 * command on that AFUs pending complete queue. The chunk associated with 
		 * this command will walk this list and finish completion on it at that
		 * time.
		 */


		CBLK_TRACE_LOG_FILE(9,"command for other chunk *(chunk->path[path_index].p_hrrq_curr) = 0x%llx, chunk->path[path_index].toggle =  %d, p_hrrq_curr = 0x%llx, chunk->path[path_index].index = %d",
			    CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),chunk->path[path_index]->afu->toggle,(uint64_t)chunk->path[path_index]->afu->p_hrrq_curr,chunk->index);


		if ((chunk->path[path_index] == NULL) || 
		    (chunk->path[path_index]->afu == NULL)) {


		    CBLK_TRACE_LOG_FILE(1,"Invalid path or afu pointer seen from chunk->index = %d, with path_index = %d",
					chunk->index,path_index);
		    continue;
		}


		CBLK_Q_NODE_TAIL(chunk->path[path_index]->afu->head_complete,chunk->path[path_index]->afu->tail_complete,
				 (p_cmd->cmdi),complete_prev,complete_next);


	    }

	}

	if ((p_cmd == *cmd) ||
	    ((*cmd) &&
	     ((*cmd)->cmdi->state == CFLSH_MGM_CMP) &&
	     (!(*cmd_complete)))) {

	    /*
	     * Either our command completed on this thread.
	     * or it completed on another thread. Let's process it.
	     */
		    

	    if (*cmd) {

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
		    
		    CBLK_COMPLETE_CMD(chunk,*cmd,transfer_size);
#else
		    
		    if (chunk->flags & CFLSH_CHNK_NO_BG_TD) {
			CBLK_COMPLETE_CMD(chunk,*cmd,transfer_size);
		    }
		    
#endif
		    *cmd_complete = TRUE;

		    if (cmd_rc == CFLASH_CMD_FATAL_ERR) {

			rc = -1;
		    }
		} 



	    }

	}

	if (CBLK_INVALID_CHUNK_PATH_AFU(chunk,path_index,__FUNCTION__)) {

	    CBLK_LIVE_DUMP_THRESHOLD(5,"0x620");

	    break;

	}

	if (chunk->path[path_index]->afu->p_hrrq_curr == NULL) {

	    CBLK_TRACE_LOG_FILE(1,"p_hrrq_curr is null, chunk = %p",chunk);

	    CBLK_LIVE_DUMP_THRESHOLD(5,"0x621");
	    break;
	}

	CBLK_TRACE_LOG_FILE(7,"*(chunk->path[path_index].p_hrrq_curr) = 0x%llx, chunk->path[path_index].toggle = 0x%llx, chunk->index = %d",
			    CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr),chunk->path[path_index]->afu->toggle,chunk->index);	
    } /* Inner while loop on RRQ */




    return (rc);
}






/*
 * NAME:        cblk_process_sisl_adap_intrpt
 *
 * FUNCTION:    This routine processes SISlite adapter
 *              interrupts
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command.
 *
 * RETURNS:
 *             
 *              
 */
int cblk_process_sisl_adap_intrpt(cflsh_chunk_t *chunk,
				  int path_index,
				  cflsh_cmd_mgm_t **cmd,
				  int intrpt_num,int *cmd_complete,
				  size_t *transfer_size)
{
    int rc = 0;

    switch (intrpt_num) {
    case SISL_MSI_RRQ_UPDATED:
	/*
	 * Command completion interrupt
	 */

	rc = cblk_process_sisl_cmd_intrpt(chunk,path_index,cmd,cmd_complete,
					  transfer_size);
	break;
    case SISL_MSI_SYNC_ERROR:

	/*
	 * Error interrupt
	 */
	cblk_process_sisl_error_intrpt(chunk,path_index,cmd);
	break;
    default:

	rc = -1;
	CBLK_TRACE_LOG_FILE(1,"Unknown interrupt number = %d",intrpt_num);
		

    }
    

    return rc;
}



/*
 * NAME:        cblk_process_sisl_adap_convert_intrpt
 *
 * FUNCTION:    This routine processes SISlite adapter
 *              interrupts by first converting from the 
 *              generic library interrupt number to the AFU
 *              specific number.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              cmd      - command.
 *
 * RETURNS:
 *             
 *              
 */
int cblk_process_sisl_adap_convert_intrpt(cflsh_chunk_t *chunk,
				  int path_index,
				  cflsh_cmd_mgm_t **cmd,
				  int intrpt_num,int *cmd_complete,
				  size_t *transfer_size)
{
    int rc = 0;

    switch (intrpt_num) {
    case CFLSH_BLK_INTRPT_CMD_CMPLT:
	/*
	 * Command completion interrupt
	 */

	rc = cblk_process_sisl_cmd_intrpt(chunk,path_index,cmd,cmd_complete,
					  transfer_size);
	break;
    case CFLSH_BLK_INTRPT_STATUS:

	/*
	 * Error interrupt
	 */
	cblk_process_sisl_error_intrpt(chunk,path_index,cmd);
	break;
    default:

	rc = -1;
	CBLK_TRACE_LOG_FILE(1,"Unknown interrupt number = %d",intrpt_num);
		

    }
    

    return rc;
}
/*
 * NAME:        cblk_process_sisl_cmd_err
 *
 * FUNCTION:    This routine parses the iosa errors
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *              ioasa    - I/O Adapter status response
 *
 * RETURNS:
 *             -1  - Fatal error
 *              0  - Ignore error (consider good completion)
 *              1  - Retry recommended
 *              2  - Retry with delay recommended.
 *              
 */
cflash_cmd_err_t cblk_process_sisl_cmd_err(cflsh_chunk_t *chunk,int path_index,cflsh_cmd_mgm_t *cmd)
{
    cflash_cmd_err_t rc = CFLASH_CMD_IGNORE_ERR;
    cflash_cmd_err_t rc2;
    sisl_ioarcb_t *ioarcb;
    sisl_ioasa_t *ioasa;
    
    

    if (cmd == NULL) {

	return CFLASH_CMD_FATAL_ERR;
    }
    
    if (cmd->cmdi == NULL) {

	CBLK_TRACE_LOG_FILE(1,"cmdi is null for cmd->index",cmd->index);
	
        errno = EINVAL;
	return CFLASH_CMD_FATAL_ERR;


    }

    ioarcb = &(cmd->sisl_cmd.rcb);
    ioasa = &(cmd->sisl_cmd.sa);

#ifdef _REMOVE
    if (cblk_log_verbosity >= 9) {
	fprintf(stderr,"Hex dump of ioasa\n");
	hexdump(ioasa,sizeof(*ioasa),NULL);
    }

#endif /* _REMOVE */


    CBLK_TRACE_LOG_FILE(5,"cmd error ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, port = 0x%x, path_index = %d",
			cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags,ioasa->port,path_index);

 
    if (ioasa->rc.flags & SISL_RC_FLAGS_UNDERRUN) {

	CBLK_TRACE_LOG_FILE(5,"cmd underrun ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, port = 0x%x",
			cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags,ioasa->port);
	/*
	 * We encountered a data underrun. Set
	 * transfer_size accordingly.
	 */


	if (ioarcb->data_len >= ioasa->resid) {

	    if (cmd->cmdi->transfer_size_bytes) {

		/*
		 * The transfer size is in bytes
		 */
		cmd->cmdi->transfer_size = ioarcb->data_len - ioasa->resid;
	    } else {


		/*
		 * The transfer size is in blocks
		 */
		cmd->cmdi->transfer_size = (ioarcb->data_len - ioasa->resid)/CAPI_FLASH_BLOCK_SIZE;
	    }
	} else {
	    cmd->cmdi->transfer_size = 0;
	}

    } 

   if (ioasa->rc.flags & SISL_RC_FLAGS_OVERRUN) {

	CBLK_TRACE_LOG_FILE(5,"cmd overrun ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, port = 0x%x",
			cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags,ioasa->port);
	

	cmd->cmdi->transfer_size = 0;
   }


    CBLK_TRACE_LOG_FILE(7,"cmd failed ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, scsi_status = 0x%x",
			cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags, ioasa->rc.scsi_rc);

    CBLK_TRACE_LOG_FILE(7,"cmd failed port = 0x%x, afu_extra = 0x%x, scsi_entra = 0x%x, fc_extra = 0x%x",
			ioasa->port,ioasa->afu_extra,ioasa->scsi_extra,ioasa->fc_extra);



    if (ioasa->rc.scsi_rc) {



	/*
	 * We have a SCSI status
	 */

	if (ioasa->rc.flags & SISL_RC_FLAGS_SENSE_VALID) {

	    CBLK_TRACE_LOG_FILE(5,"sense data: error code = 0x%x, sense_key = 0x%x, asc = 0x%x, ascq = 0x%x",
				ioasa->sense_data[0],ioasa->sense_data[2],ioasa->sense_data[12],ioasa->sense_data[13]);

	    chunk->stats.num_cc_errors++;
	
	    rc2 = cblk_process_sense_data(chunk,cmd,(struct request_sense_data *)ioasa->sense_data);
		    

	    if (rc == CFLASH_CMD_IGNORE_ERR) {

		/*
		 * If we have not indicated an error, then use the 
		 * return code from the sense data processing.
		 */

		rc = rc2;
	    }


	} else if (ioasa->rc.scsi_rc) {


	    /*
	     * We have a SCSI status, but no sense data
	     */


	    CBLK_TRACE_LOG_FILE(1,"cmd failed ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, scsi_status = 0x%x",
				cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags, ioasa->rc.scsi_rc);

	    cmd->cmdi->transfer_size = 0;
	    chunk->stats.num_errors++;

	    switch (ioasa->rc.scsi_rc) {
	    case SCSI_CHECK_CONDITION:

		/*
		 * This mostly likely indicates a misbehaving device, that is
		 * reporting a check condition, but is returning no sense data
		 */

	    
		rc = CFLASH_CMD_RETRY_ERR;
		cmd->cmdi->status = EIO;

		break;
	    case SCSI_BUSY_STATUS:
	    case SCSI_QUEUE_FULL:

		/*
		 * Retry with delay
		 */
		
		cmd->cmdi->status = EBUSY;
		rc = CFLASH_CMD_DLY_RETRY_ERR;

		break;
	    case SCSI_RESERVATION_CONFLICT:
		cmd->cmdi->status = EBUSY;
		rc = CFLASH_CMD_FATAL_ERR;
		
		cblk_notify_mc_err(chunk,path_index,0x605,0,CFLSH_BLK_NOTIFY_DISK_ERR,cmd);
		break;

	    default:
		rc = CFLASH_CMD_FATAL_ERR;
		cmd->cmdi->status = EIO;
		
		cblk_notify_mc_err(chunk,path_index,0x606,0,CFLSH_BLK_NOTIFY_DISK_ERR,cmd);
	    }

	} 


    }


    
    /*
     * We encountered an error. For now return
     * EIO for all errors.
     */


    if (ioasa->rc.fc_rc) {

	/*
	 * We have an FC status
	 */

       

       CBLK_TRACE_LOG_FILE(1,"cmd failed ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, fc_extra = 0x%x",
			   cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags, ioasa->fc_extra);


	switch (ioasa->rc.fc_rc) {

	case SISL_FC_RC_LINKDOWN: 
	    chunk->stats.num_fc_errors++;
	    if (ioasa->port == 0) {
		chunk->stats.num_port0_linkdowns++;
	    } else {
		chunk->stats.num_port1_linkdowns++;
	    }
	    chunk->stats.num_errors++;

	    
	    CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x608,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    rc = cblk_retry_new_path(chunk,cmd,FALSE);
	    cmd->cmdi->status = ENETDOWN;
	    cmd->cmdi->transfer_size = 0;
	    break;
	case SISL_FC_RC_NOLOGI: 
	    chunk->stats.num_fc_errors++;
	    if (ioasa->port == 0) {
		chunk->stats.num_port0_no_logins++;
	    } else {
		chunk->stats.num_port1_no_logins++;
	    }
	    chunk->stats.num_errors++;
	    
	    CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x609,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    rc = cblk_retry_new_path(chunk,cmd,FALSE);
	    cmd->cmdi->status = ENETDOWN;
	    cmd->cmdi->transfer_size = 0;

	    break;

	case SISL_FC_RC_ABORTPEND:

	    chunk->stats.num_errors++;
	    rc = CFLASH_CMD_RETRY_ERR;
	    cmd->cmdi->status = ETIMEDOUT;
	    cmd->cmdi->transfer_size = 0;
	    
	    CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x60a,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    if (ioasa->port == 0) {
		chunk->stats.num_port0_fc_errors++;
	    } else {
		chunk->stats.num_port1_fc_errors++;
	    }
	    break;
	case SISL_FC_RC_RESID:
	    /*
	     * This indicates an FCP resid underrun
	     */

	    if (!(ioasa->rc.flags & SISL_RC_FLAGS_OVERRUN)) {
		/*
		 * If the SISL_RC_FLAGS_OVERRUN flag was set,
		 * then we will handle this error else where.
		 * If not then we must handle it here.
		 * This is probably an AFU bug. We will 
		 * attempt a retry to see if that resolves it.
		 */

		chunk->stats.num_errors++;
		rc = CFLASH_CMD_RETRY_ERR;
		cmd->cmdi->status = EIO;
		cmd->cmdi->transfer_size = 0;
		if (ioasa->port == 0) {
		    chunk->stats.num_port0_fc_errors++;
		} else {
		    chunk->stats.num_port1_fc_errors++;
		}

	    }
	    break;
	case SISL_FC_RC_RESIDERR: // Resid mismatch between adapter and device
	case SISL_FC_RC_TGTABORT:
	case SISL_FC_RC_ABORTOK:
	case SISL_FC_RC_ABORTFAIL:

	    chunk->stats.num_errors++;
	    rc = CFLASH_CMD_RETRY_ERR;
	    cmd->cmdi->status = EIO;
	    cmd->cmdi->transfer_size = 0;
	    if (ioasa->port == 0) {
		chunk->stats.num_port0_fc_errors++;
	    } else {
		chunk->stats.num_port1_fc_errors++;
	    }
	    break;

	case SISL_FC_RC_WRABORTPEND:
	case SISL_FC_RC_NOEXP:
	case SISL_FC_RC_INUSE:


	    chunk->stats.num_fc_errors++;
	    chunk->stats.num_errors++;
	    if (ioasa->port == 0) {
		chunk->stats.num_port0_fc_errors++;
	    } else {
		chunk->stats.num_port1_fc_errors++;
	    }
	    rc = CFLASH_CMD_FATAL_ERR;
	    cmd->cmdi->status = EIO;
	    cmd->cmdi->transfer_size = 0;
	    break;
		

	}
    }

    if (ioasa->rc.afu_rc) {

	
	/*
	 * We have a AFU error
	 */

	CBLK_TRACE_LOG_FILE(6,"afu error ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, afu error = 0x%x",
			    cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags, ioasa->rc.afu_rc);

	CBLK_TRACE_LOG_FILE(6,"contxt_handle = 0x%x",chunk->path[path_index]->afu->contxt_handle);
	CBLK_TRACE_LOG_FILE(6,"mmio_map = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio_mmap);
	CBLK_TRACE_LOG_FILE(6,"mmio = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmio);
	CBLK_TRACE_LOG_FILE(6,"mmap_size = 0x%llx",(uint64_t)chunk->path[path_index]->afu->mmap_size);
	CBLK_TRACE_LOG_FILE(6,"hrrq_start = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_start);
	CBLK_TRACE_LOG_FILE(6,"hrrq_end = 0x%llx",(uint64_t)chunk->path[path_index]->afu->p_hrrq_end);
	CBLK_TRACE_LOG_FILE(6,"chunk->flags = 0x%x, afu_flags = 0x%x",chunk->flags,chunk->path[path_index]->afu->flags);
	CBLK_TRACE_LOG_FILE(6,"cmd_start = 0x%llx",(uint64_t)chunk->cmd_start);
	CBLK_TRACE_LOG_FILE(6,"cmd_end = 0x%llx",(uint64_t)chunk->cmd_end);

	CBLK_TRACE_LOG_FILE(6," cmd = 0x%llx lba = 0x%llx flags = 0x%x, cmd->cmdi->buf = 0x%llx",
			    cmd,cmd->cmdi->lba,cmd->cmdi->flags,cmd->cmdi->buf);


	chunk->stats.num_afu_errors++;

	cmd->cmdi->transfer_size = 0;


	switch (ioasa->rc.afu_rc) {
	case SISL_AFU_RC_RHT_INVALID: 
	case SISL_AFU_RC_RHT_OUT_OF_BOUNDS:
	case SISL_AFU_RC_LXT_OUT_OF_BOUNDS:
	    /*
	     * This most likely indicates a code bug
	     * in this code.
	     */

	    CBLK_TRACE_LOG_FILE(1,"afu error ctx_id = 0x%x, ioasc = 0x%x, resid = 0x%x, flags = 0x%x, afu error = 0x%x",
				cmd->sisl_cmd.rcb.ctx_id,ioasa->ioasc,ioasa->resid,ioasa->rc.flags, ioasa->rc.afu_rc);
	    rc = CFLASH_CMD_FATAL_ERR;
	    cmd->cmdi->status = EIO;
	    break;
	case SISL_AFU_RC_RHT_UNALIGNED:
	case SISL_AFU_RC_LXT_UNALIGNED:
	    /*
	     * These should never happen
	     */

	    cblk_notify_mc_err(chunk,path_index,0x600,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    rc = CFLASH_CMD_FATAL_ERR;
	    cmd->cmdi->status = EIO;
	    break;

	case SISL_AFU_RC_NO_CHANNELS:

		/*
		 * Retry with delay
		 */
	        
	        CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x60c,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
		cmd->cmdi->status = ENETDOWN;
		rc = cblk_retry_new_path(chunk,cmd, TRUE);

		break;

	case SISL_AFU_RC_RHT_DMA_ERR: 
	case SISL_AFU_RC_LXT_DMA_ERR:
	case SISL_AFU_RC_DATA_DMA_ERR:
	    switch (ioasa->afu_extra) {
	    case SISL_AFU_DMA_ERR_PAGE_IN:

		/*
		 * Retry 
		 */
		
		CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x60d,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
		cmd->cmdi->status = EIO;
		rc = CFLASH_CMD_RETRY_ERR;
		break;

	    case SISL_AFU_DMA_ERR_INVALID_EA:
	    default:

		cblk_notify_mc_err(chunk,path_index,0x601,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
		rc = CFLASH_CMD_FATAL_ERR;
		cmd->cmdi->status = EIO;
	    }
	    break;
	case SISL_AFU_RC_OUT_OF_DATA_BUFS:
	    /*
	     * Retry
	     */

	    CBLK_NOTIFY_LOG_THRESHOLD(5,chunk,path_index,0x60e,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    cmd->cmdi->status = EIO;
	    rc = CFLASH_CMD_RETRY_ERR;
	    break;
	case SISL_AFU_RC_CAP_VIOLATION:
	    /*
	     * Retry, assume EEH recovery completes before retry.
	     */

	    cmd->cmdi->status = EIO;
	    rc = CFLASH_CMD_RETRY_ERR;
	    cblk_notify_mc_err(chunk,path_index,0x602,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    break;
	case SISL_AFU_RC_TIMED_OUT_PRE_FC:
	case SISL_AFU_RC_TIMED_OUT:
	    cmd->cmdi->status = ETIMEDOUT;
	    CBLK_NOTIFY_LOG_THRESHOLD(3,chunk,path_index,0x60b,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    rc = CFLASH_CMD_RETRY_ERR;
	    break;
	default:

	    cmd->cmdi->status = EIO;
	    cblk_notify_mc_err(chunk,path_index,0x603,0,CFLSH_BLK_NOTIFY_AFU_ERROR,cmd);
	    rc = cblk_retry_new_path(chunk,cmd,FALSE);
	}

    }


    if (cmd->cmdi->status) {

	errno = cmd->cmdi->status;
    }


    return rc;
}

/*
 * NAME:        cblk_reset_context_sisl
 *
 * FUNCTION:    This will reset the adapter context so that
 *              any active commands will never be returned to the host.
 *              The AFU is not reset and new requests can be issued.
 *              This routine assumes the caller has the afu->lock.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *              0  - Good completion
 *
 *              
 */
int cblk_reset_context_sisl(cflsh_chunk_t *chunk, int path_index)
{
    int rc = 0;
    int wait_reset_context_retry = 0;
    volatile __u64 *reg_offset = 0;


    
    if (chunk->path[path_index]->afu->flags & CFLSH_AFU_SQ) {

	/*
	 * This AFU is using SQ, so it does context reset
	 * via the SQ context reset register.
	 */

	reg_offset = chunk->path[path_index]->afu->mmio + CAPI_SQ_CTXTRST_EA_OFFSET;
    } else {
	/*
	 * This AFU is not using SQ, so it does context reset
	 * via the IOARRIN register.
	 */
	reg_offset = chunk->path[path_index]->afu->mmio + CAPI_IOARRIN_OFFSET;
    }

#ifdef _FOR_DEBUG
    if (CBLK_SETUP_BAD_MMIO_SIGNAL(chunk,path_index,CAPI_IOARRIN_OFFSET+0x20)) {

	/*
	 * We must have failed the MMIO done below and long
	 * jump here.
	 */

	return -1;
    }

#endif /* _FOR_DEBUG */

    /*
     * Writing 1 to the IOARRIN/SQ_CONTEXT_RESET, will cause all active commands
     * to ultimately be dropped by the AFU. Then the AFU can 
     * be issued commands again.
     */

    
    CBLK_OUT_MMIO_REG(reg_offset, (uint64_t)1);

    while ((CBLK_IN_MMIO_REG(reg_offset,TRUE) & CFLASH_BLOCK_RST_CTX_COMPLETE_MASK) &&
	    (wait_reset_context_retry < CFLASH_BLOCK_MAX_WAIT_RST_CTX_RETRIES)) {

	/*
	 * Wait a limited amount of time for the reset
	 * context to complete. We are notified of this when
	 * a read of IOARRIN returns 0.
	 *
	 * We also need to detect possible EEH here.
	 *
	 */

	CBLK_TRACE_LOG_FILE(5,"waiting for context reset to complete");

	if ((CBLK_IN_MMIO_REG(reg_offset,TRUE)) == 0xffffffffffffffffLL) {

	    CBLK_TRACE_LOG_FILE(5,"Possible UE detected on context reset");

	    /*
	     * Fail this here and let the caller check for EEH and possibly
	     * recover.
	     */

	    return -1;
	}
	usleep(CFLASH_BLOCK_DELAY_RST_CTX);
	wait_reset_context_retry++;

    }

#ifdef _FOR_DEBUG
    CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index);
#endif /* _FOR_DEBUG */


    if (wait_reset_context_retry >= CFLASH_BLOCK_MAX_WAIT_RST_CTX_RETRIES) {



	/*
	 * Reset context failed. Fail this operation now.
	 */

	CBLK_TRACE_LOG_FILE(1,"Reset context timed out, chunk->index = %d, num_active_cmds = %d",
			    chunk->index,chunk->num_active_cmds);
#ifdef _FOR_DEBUG
	CBLK_CLEANUP_BAD_MMIO_SIGNAL(chunk,path_index);
#endif /* _FOR_DEBUG */
	errno = ETIMEDOUT;

	return -1;
    }


    /*
     * Now we need to clean up the RRQ for all commands that
     * may have posted completion. In this case we will discard
     * all of their completion status, with the
     * caller of this routine doing the retries.
     */

    while (((CBLK_READ_ADDR_CHK_UE(chunk->path[path_index]->afu->p_hrrq_curr)) & (SISL_RESP_HANDLE_T_BIT)) == chunk->path[path_index]->afu->toggle) {

		
	/*
	 * Increment the RRQ pointer
	 * and possibly adjust the toggle
	 * bit.
	 */

	CBLK_INC_RRQ(chunk,path_index);



    } /* while */


    // TODO: ?? Need to handle clean up of commmands on this AFU but other  chunks

    return rc;
}



/*
 * NAME:        cblk_init_sisl_fcn_ptrs
 *
 * FUNCTION:    This routine initializes the function
 *              pointers for a SIS Lite chunk.
 *
 * INPUTS:
 *              chunk    - Chunk associated with this error
 *
 * RETURNS:
 *              0  - Good completion
 *
 *              
 */
int cblk_init_sisl_fcn_ptrs(cflsh_path_t *path)
{
    
    if (path == NULL) {

	CBLK_TRACE_LOG_FILE(1,"path = NULL");
	return -1;
    }

    path->fcn_ptrs.get_num_interrupts = cblk_get_sisl_num_interrupts;
    path->fcn_ptrs.get_cmd_room = cblk_get_sisl_cmd_room;
    path->fcn_ptrs.adap_setup = cblk_sisl_adap_setup;
    path->fcn_ptrs.adap_sq_setup = cblk_sisl_adap_sq_setup;
    path->fcn_ptrs.get_intrpt_status = cblk_get_sisl_intrpt_status;
    path->fcn_ptrs.inc_rrq = cblk_inc_sisl_rrq;
    path->fcn_ptrs.inc_sq = cblk_inc_sisl_sq;
    path->fcn_ptrs.get_cmd_data_length = cblk_get_sisl_cmd_data_length;
    path->fcn_ptrs.get_cmd_cdb = cblk_get_sisl_cmd_cdb;
    path->fcn_ptrs.get_cmd_rsp = cblk_get_sisl_cmd_rsp;
    path->fcn_ptrs.build_adap_cmd = cblk_build_sisl_cmd;
    path->fcn_ptrs.update_adap_cmd = cblk_update_path_sisl_cmd;
    path->fcn_ptrs.issue_adap_cmd = cblk_issue_sisl_cmd;
    path->fcn_ptrs.process_adap_err = cblk_process_sisl_cmd_err;
    path->fcn_ptrs.process_adap_intrpt = cblk_process_sisl_adap_intrpt;
    path->fcn_ptrs.process_adap_convert_intrpt = cblk_process_sisl_adap_convert_intrpt;
    path->fcn_ptrs.complete_status_adap_cmd = cblk_complete_status_sisl_cmd;
    path->fcn_ptrs.init_adap_cmd = cblk_init_sisl_cmd;
    path->fcn_ptrs.init_adap_cmd_resp = cblk_init_sisl_cmd_rsp;
    path->fcn_ptrs.copy_adap_cmd_resp = cblk_copy_sisl_cmd_rsp;
    path->fcn_ptrs.set_adap_cmd_resp_status = cblk_set_sisl_cmd_rsp_status;
    path->fcn_ptrs.reset_adap_contxt = cblk_reset_context_sisl;

    return 0;
}
