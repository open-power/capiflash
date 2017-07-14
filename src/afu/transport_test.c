/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/transport_test.c $                                   */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _MACOSX 
#include <malloc.h>
#endif /* !_MACOS */
#include <memory.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif
#include <poll.h>
#include <cxl.h>
#ifdef _USE_LIB_AFU
#include <afu.h> 
#endif /* _USE_LIB_AFU */
#ifdef SIM
#include "sim_pthread.h"
#else
#include <pthread.h>
#endif

#include "transport_test.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
    

#define MIN(a,b) ((a)<(b) ? (a) : (b)

#ifdef _USE_LIB_AFU
struct afu *p_afu = NULL;
#endif /* _USE_LIB_AFU */

#define DATA_BUF_SIZE  4096

/*
 * Global variables declared in this module...
 */

static int bflag;               /* indicates the b flag was passed on command
                                 * line which indicates a block number was 
                                 * specified.
                                 */
static int cflag;               /* indicates the c flag was passed on command
                                 * line which indicates a cmd_type was 
                                 * specified.
                                 */
static int fflag;               /* indicates the f flag was passed on command
                                 * line which indicates a filename is
                                 * specified
                                 */

static int hflag;               /* help flag */
static int iflag;               /* indicates the i flag was passed on command
                                 * line which indicates the lun id
                                 * is specified
                                 */
static int lflag;               /* indicates the l flag was passed on command
                                 * line which indicates adapter/device name 
                                 * is specified
                                 */
static int nflag;               /* indicates the n flag was passed on command
                                 * line which indicates the number of loops 
                                 * or times to issue requests
                                 */

static int verbose_flag;         /* verbose mode flag */

static char *device_name = NULL; /* point to the arg for -l flag */
static char *filename = NULL;    /* point to the arg for -f flag */
static int num_loops = 1;
static int port_num = 0x3;
static uint64_t lun_id = 0x0;
static uint64_t block_number = 0;
FILE *file = NULL;



transp_scsi_cmd_t cmd_type = TRANSP_READ_CMD; // Default to SCSI Read 16

uint64_t *rrq = NULL;

uint64_t *rrq_current = NULL;

int rrq_len = 64;
int contxt_handle = 0;
uint64_t toggle = 1;            /* Toggle bit for RRQ          */

static inline void  INC_RRQ()
{

    rrq_current++;



    if (rrq_current > &rrq[rrq_len -1])
    {

        rrq_current = rrq;

        toggle ^= SISL_RESP_HANDLE_T_BIT;

    }
    

    
    return;
}

/* ----------------------------------------------------------------------------
 *
 * NAME: valid_endianess
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
int valid_endianess(void)
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

/*
 * NAME:        open_and_setup
 *
 * FUNCTION:    Opens a CAPI adapter, attaches to the AFU
 *              and mmaps the AFU MMIO space for it use.
 *
 *
 * INPUTS:
 *              name    - adapter name.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int open_and_setup(const char *name, int *fd, void **mmio) 
{
    int rc = 0;                 /* Return code                 */
    char adap_path[PATH_MAX];   /* Adapter special filename    */
#ifndef _USE_LIB_AFU 
    struct cxl_ioctl_start_work start_work;
#endif


    /*
     * Align RRQ on cacheline boundary.
     */

    if ( posix_memalign((void *)&rrq,128,(sizeof(*rrq) * rrq_len))) {

	perror("posix_memalign failed for rrq");

	return (errno);

    }

    bzero(adap_path,PATH_MAX);

    bzero((void *) rrq,(sizeof(*rrq) * rrq_len));

    rrq_current = rrq;

    sprintf(adap_path,"/dev/%s",name);
    
#ifdef _USE_LIB_AFU

    if (verbose_flag) {
	fprintf(stderr,"Calling afu_map ...\n");
    }
    if ((p_afu = afu_map()) == NULL) {
	fprintf(stderr,"Cannot open AFU\n");
	exit(1);
    }


    if (verbose_flag) {
	fprintf(stderr,"Calling afu_start ...\n");
    }
    afu_start(p_afu);

    // set up RRQ
    // these funcs have syncs in them. offset is in 4-byte words.
    // assume problem space starts with the "Host transport MMIO regs"
    // in SISLite p 7.
    afu_mmio_write_dw(p_afu, 10, (uint64_t)rrq); // START_EA
    afu_mmio_write_dw(p_afu, 12, (sizeof(*rrq) * rrq_len));   // RRQ LEN

#else

    /*
     * Don't use libafu
     */

    if (verbose_flag) {
	fprintf(stderr,"Opening adapter ...\n");
    }

    *fd = open(adap_path,O_RDWR);  /* ??TODO Try without O_CLOEXEC */

    if (*fd < 0) {

	perror("open_and_setup: Unable to open device");

	return (errno);
    }

    if (verbose_flag) {
	fprintf(stderr,"*fd = 0x%x\n",*fd);
    }

    bzero(&start_work,sizeof(start_work));

    start_work.flags = CXL_START_WORK_NUM_IRQS;
    start_work.num_interrupts = 4;

    if (verbose_flag) {
	fprintf(stderr,"Issuing CXL_IOCTL_START_WORK ioctl ...\n");
    }

    rc = ioctl(*fd,CXL_IOCTL_START_WORK,&start_work);



    if (rc) {

	
	perror("open_and_setup: Unable to attach");

	close(*fd);

	return (errno);

    }

    rc = ioctl(*fd,CXL_IOCTL_GET_PROCESS_ELEMENT,&contxt_handle);



    if (rc) {

	
	perror("open_and_setup: Unable to get process element");

	close(*fd);

	return (errno);

    }

    if (verbose_flag) {
	
	fprintf(stderr,"context_handle = 0x%x\n",contxt_handle);
	fprintf(stderr,"mmap MMIO space ...\n");
    }

    *mmio = mmap(NULL,CAPI_FLASH_REG_SIZE,PROT_READ|PROT_WRITE, MAP_SHARED,*fd,0);

    if (*mmio == MAP_FAILED) {
	perror ("mmap of mmio space failed");

	close(*fd);
	return (errno);
    }


    if (verbose_flag) {
	
	fprintf(stderr,"mmio = 0x%lx\n",(uint64_t)(*mmio));

    }
    /*
     * Set up response queue
     */

    
    out_mmio64 (*mmio + CAPI_RRQ0_START_EA_OFFSET, (uint64_t)rrq);

    out_mmio64 (*mmio + CAPI_RRQ0_END_EA_OFFSET, (uint64_t)&rrq[(rrq_len-1)]);


#endif /* !_USE_LIB_AFU */


    return (rc);
}


/*
 * NAME:        close_and_cleanup
 *
 * FUNCTION:    Cleans up an adapter and closes
 *
 *
 * INPUTS:
 *              name    - adapter name.
 *
 * RETURNS:
 *              NONE
 *              
 */

void close_and_cleanup(int fd, void *mmio) 
{


    
    if (verbose_flag) {
	fprintf(stderr,"munmap MMIO space ...\n");
    }
#ifdef _USE_LIB_AFU

    afu_unmap(p_afu);
#else
    if (munmap(mmio,CAPI_FLASH_REG_SIZE)) {


	perror ("munmap of MMIO space failed");

	/*
	 * Don't return here on error. Continue
	 * to close
	 */
    }

    
    close (fd);

#endif /* !_USE_LIB_AFU */

    free(rrq);

    rrq = NULL;
    return;
}


/*
 * NAME:        build_ioarcb
 *
 * FUNCTION:    Build and queue one IOARCB
 *
 *
 * INPUTS:
 *              name    - adapter name.
 *
 * RETURNS:
 *              NONE
 *              
 */

int build_ioarcb(capi_ioarcb_t *ioarcb, int block_number, void *data_buf) 
{
    int rc = 0;
    int bytes_read = 0;

    bzero(ioarcb,sizeof(*ioarcb));


    ioarcb->timeout = CAPI_SCSI_IO_TIME_OUT;

    ioarcb->lun_id = lun_id;
    /*
     * Allow either FC port to be used.
     */
    ioarcb->port_sel = port_num;


    ioarcb->ctx_id = contxt_handle & 0xffff;



    ioarcb->data_ea = (uint64_t)data_buf;

    ioarcb->msi = 2;


    // TODO ?? Need to add data buffers

    switch (cmd_type) {
    case TRANSP_READ_CMD:
	/*
	 * Fill in CDB. Probably this should be
	 * a separate routine that just builds
	 * CDBs. For now it is inline.
	 */

	if (verbose_flag) {
		
	    fprintf(stderr,"Building Read 16\n");
	}
#ifdef _OLD_CODE
	ioarcb->cdb.scsi_op_code = SCSI_READ_16;
#else
	ioarcb->cdb[0] = SCSI_READ_16;
#endif 

	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	ioarcb->data_len = DATA_BUF_SIZE;

	CFLASH_BUILD_RW_16((scsi_cdb_t *)&(ioarcb->cdb),
			   block_number,8);
	break;

    case TRANSP_WRITE_CMD:
	/*
	 * Fill in CDB. Probably this should be
	 * a separate routine that just builds
	 * CDBs. For now it is inline.
	 */
    
	if (verbose_flag) {
	
	    fprintf(stderr,"Building Write 16\n");
	}

#ifdef _OLD_CODE
	ioarcb->cdb.scsi_op_code = SCSI_WRITE_16;
#else
	ioarcb->cdb[0] = SCSI_WRITE_16;
#endif 
	
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_WRITE;
	ioarcb->data_len = DATA_BUF_SIZE;

	if (file) {
	    /*
	     * If an input file was specified,
	     * then read the first DATA_BUF_SIZE bytes
	     * in to write out to the device.
	     */
       
	    bytes_read = fread(data_buf, 1, DATA_BUF_SIZE, file);
        
	    if (bytes_read != DATA_BUF_SIZE) {

		fprintf(stderr,"Unable able to read full size of %d, read instead %d\n",DATA_BUF_SIZE,bytes_read);

		/*
		 * Do not fail, just continue with questionable buffer contents
		 */
	    }

	    
	} else {
	    /*
	     * If no input file is specified then
	     * put a pattern in the buffer to
	     * be written
	     */
	    memset((uint8_t *)(data_buf), ((getpid())%256), 
		   ioarcb->data_len);
	}
	    
	CFLASH_BUILD_RW_16((scsi_cdb_t *)&(ioarcb->cdb),
			  block_number,8);
	break;

    case TRANSP_STD_INQ_CMD:
	if (verbose_flag) {
	    fprintf(stderr,"Building Standard Inquiry\n");
	}
	ioarcb->data_len = 255;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_inquiry((scsi_cdb_t *)&(ioarcb->cdb),-1,255);
	break;

    case TRANSP_PG83_INQ_CMD:
	
	if (verbose_flag) {
	    fprintf(stderr,"Building Inquiry for page 0x83\n");
	}
	ioarcb->data_len = 255;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_inquiry((scsi_cdb_t *)&(ioarcb->cdb),0x83,255);
	break;

    case TRANSP_TUR_CMD:
	if (verbose_flag) {
	    fprintf(stderr,"Building Test Unit Ready\n");
	}
	rc = cflash_build_scsi_tur((scsi_cdb_t *)&(ioarcb->cdb));
	break;
    case TRANSP_RD_CAP_CMD:
	
	if (verbose_flag) {
	    fprintf(stderr,"Building Read Capicity 16\n");
	}
	ioarcb->data_len = sizeof(struct readcap16_data);
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_read_cap16((scsi_cdb_t *)&(ioarcb->cdb),sizeof(struct readcap16_data));
	break;
    case TRANSP_RPT_LUNS_CMD:
	
	if (verbose_flag) {
	    fprintf(stderr,"Building Report luns\n");
	}

	ioarcb->data_len = 256;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_report_luns((scsi_cdb_t *)&(ioarcb->cdb),256);
	break;
    case TRANSP_MSENSE_CMD:
	
	if (verbose_flag) {
	    fprintf(stderr,"Building Mode Sense 10\n");
	}

	ioarcb->data_len = 256;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_mode_sense_10((scsi_cdb_t *)&(ioarcb->cdb),256,0);
	break;

    case TRANSP_MSELECT_CMD:
	
	if (verbose_flag) {
	    fprintf(stderr,"Building Mode Select 10\n");
	}

	ioarcb->data_len = 256;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_WRITE;
	rc = cflash_build_scsi_mode_select_10((scsi_cdb_t *)&(ioarcb->cdb),256,0);
	break;

    case TRANSP_REQSNS_CMD:
	if (verbose_flag) {
	    fprintf(stderr,"Building Request Sense\n");
	}
	ioarcb->data_len = 255;
	ioarcb->req_flags = SISL_REQ_FLAGS_PORT_LUN_ID | SISL_REQ_FLAGS_HOST_READ;
	rc = cflash_build_scsi_request_sense((scsi_cdb_t *)&(ioarcb->cdb),255);
	break;

    default:

	fprintf(stderr,"Invalid cmd_type specified cmd_type = %d\n",cmd_type);
	rc = EINVAL;

    }

	
    if (verbose_flag) {
	fprintf(stderr,"Hex dump of ioarcb\n");
	hexdump(ioarcb,sizeof(*ioarcb),NULL);
    }
    return rc;
}

/*
 * NAME:        issue_ioarcb
 *
 * FUNCTION:    Issues one IOARCB and waits for completion
 *
 *
 * INPUTS:
 *              name    - adapter name.
 *
 * RETURNS:
 *              NONE
 *              
 */

int issue_ioarcb(int fd, void *mmio,int block_number) 
{
#define LUN_LIST_SIZE  512
    int rc = 0;
    int poll_ret;
    void *data_buf = NULL;
    capi_ioarcb_t *ioarcb = NULL;  
    struct pollfd poll_list = { fd, POLLIN, 0};
    char vendor_id[8];
    char product_id[8];
    char wwid[256];
    int read_rc = 0;
    struct cxl_event cxl_event;
    int num_actual_luns = 0;
    uint64_t *actual_lun_list;
    int i;
    uint32_t block_size;
    uint64_t last_lba;
    struct readcap16_data *readcap16_data = NULL;
    
#ifndef _OLD_CODE
    sisl_ioasa_t *ioasa;
    struct request_sense_data *sense_data;
    sisl_iocmd_t *cmd;
#endif

    /*
     * Align IOARCB on cacheline boundary.
     */	

#ifdef _OLD_CODE
    if ( posix_memalign((void *)&ioarcb,128,sizeof(*ioarcb))) {
#else
    if ( posix_memalign((void *)&cmd,128,sizeof(sisl_iocmd_t))) {
#endif

	perror("posix_memalign failed for ioarcb");

	return (errno);

    }
#ifndef _OLD_CODE
    ioarcb = &(cmd->rcb);
#endif /* !_OLD_CODE */
 
    if (verbose_flag) {
	fprintf(stderr,"sizeof *ioarcb = 0x%x\n",(int)sizeof(*ioarcb));
	fprintf(stderr,"ioarcb = 0x%p\n", ioarcb);
#ifndef _MACOSX

	fprintf(stderr,"offset of ctx_id = 0x%x\n",(int)offsetof(capi_ioarcb_t,ctx_id));
	fprintf(stderr,"offset of req_flags = 0x%x\n",(int)offsetof(capi_ioarcb_t,req_flags));
	fprintf(stderr,"offset of lun_id = 0x%x\n",(int)offsetof(capi_ioarcb_t,lun_id));
	fprintf(stderr,"offset of data_len = 0x%x\n",(int)offsetof(capi_ioarcb_t,data_len));
	fprintf(stderr,"offset of data_ea = 0x%x\n",(int)offsetof(capi_ioarcb_t,data_ea));
	fprintf(stderr,"offset of msi = 0x%x\n",(int)offsetof(capi_ioarcb_t,msi));
#ifdef _OLD_CODE
	fprintf(stderr,"offset of rrq_num = 0x%x\n",(int)offsetof(capi_ioarcb_t,rrq_num));
	fprintf(stderr,"offset of cdb = 0x%x\n",(int)offsetof(capi_ioarcb_t,cdb));
	fprintf(stderr,"offset of cdb[15] = 0x%x\n",(int)offsetof(capi_ioarcb_t,cdb.scsi_bytes[14]));
#else
	fprintf(stderr,"offset of rrq_num = 0x%x\n",(int)offsetof(capi_ioarcb_t,rrq));
	fprintf(stderr,"offset of cdb = 0x%x\n",(int)offsetof(capi_ioarcb_t,cdb));
	fprintf(stderr,"offset of cdb[15] = 0x%x\n",(int)offsetof(capi_ioarcb_t,cdb[15]));
#endif
	fprintf(stderr,"offset of timeout = 0x%x\n",(int)offsetof(capi_ioarcb_t,timeout));
#endif /* !_MACOSX */
    }
   

    /*
     * Align data buffer on page boundary.
     */	
    if ( posix_memalign((void *)&data_buf,4096,DATA_BUF_SIZE)) {

	perror("posix_memalign failed for data buffer");

	free(ioarcb);
	return (errno);

    }

    rc = build_ioarcb(ioarcb,block_number,data_buf);

    if (rc) {

	fprintf(stderr,"Failed to build IOARCB with rc = %d\n",rc);
	free(ioarcb);
	free(data_buf);
	return (rc);
    }


    // Issue MMIO to IOARRIN
    
    if (verbose_flag) {
	fprintf(stderr,"MMIO to IOARRIN\n");
    }
#ifdef _USE_LIB_AFU
    afu_mmio_write_dw(p_afu, 8, (uint64_t)ioarcb);
#else
    out_mmio64 (mmio + CAPI_IOARRIN_OFFSET, (uint64_t)ioarcb);

    fprintf(stderr,"mmio = 0x%lx\n", (uint64_t)(mmio));
    fprintf(stderr,"mmio + CAPI_IOARRIN_OFFSET = 0x%lx\n", (uint64_t)(mmio + CAPI_IOARRIN_OFFSET));

#endif /* !_USE_LIB_AFU */


    /*
     * Wait for completion.  
     *
     * NOTE: The last argument to poll is the time-out value in milliseconds.
     * So convert our time-out value to milliseconds and then double it
     */

    if (verbose_flag) {
	fprintf(stderr,"polling on fd = 0x%x...\n",fd);
    }

    poll_ret = poll(&poll_list,1, (2 *(CAPI_SCSI_IO_TIME_OUT * 1000)));

    if (poll_ret == 0) {

	/* 
	 * We timed-out
	 */
	fprintf(stderr,"Poll timed out waiting on interrupt.\n");

	rc = -1;

#ifdef _REMOVE
	fprintf(stderr,"Reading anyway ..\n");
	read_rc = read(fd,&cxl_event,sizeof(struct cxl_event));
	
	if (read_rc != sizeof(struct cxl_event)) {
	    
	    fprintf(stderr,"read event failed, with rc = %d errno = %d\n",rc, errno);
	    free(ioarcb);
	    free(data_buf);
	    return (read_rc);
	}
#endif

    } else  if (poll_ret < 0) {
	
	
	/* 
	 * Poll failed, Give up
	 */
	fprintf(stderr,"Poll failed.\n");

	rc = -1;
	


    } else {
	
	/* 
	 * We received interrupt 
	 */


	fprintf(stderr,"Poll received interrupt.\n");
	
	read_rc = read(fd,&cxl_event,sizeof(struct cxl_event));
	
	if (read_rc != sizeof(struct cxl_event)) {
	    
	    fprintf(stderr,"read event failed, with rc = %d errno = %d\n",rc, errno);
	    free(ioarcb);
	    free(data_buf);
	    return (read_rc);
	}
	
	fprintf(stderr,"capi event type = %d\n",cxl_event.header.type);

	if (cxl_event.header.type != CXL_EVENT_AFU_INTERRUPT) {
	    
	    
	    fprintf(stderr,"capi event != CXL_EVENT_AFU_INTERRUPT type = %d, proess_element = 0x%x\n",cxl_event.header.type,cxl_event.header.process_element);


	    if (cxl_event.header.type == CXL_EVENT_DATA_STORAGE) {
		struct cxl_event_data_storage *capi_ds =
		                             (struct cxl_event_data_storage*)&cxl_event;

		fprintf(stderr,"CXL_EVENT_DATA_STORAGE: addr = 0x%"PRIx64"\n",
			capi_ds->addr);
		fprintf(stderr,"ioarcb = 0x%"PRIx64", data_buf = 0x%"PRIx64"\n",
			(uint64_t) ioarcb, (uint64_t)data_buf);

		
	    }
	    free(ioarcb);
	    free(data_buf);
	    return (read_rc);

	} 

#ifndef _OLD_CODE
	cmd = (sisl_iocmd_t *) ioarcb;
	ioasa = &(cmd->sa);
	sense_data = (struct request_sense_data*)ioasa->sense_data;
#endif	

	if (((*rrq_current) & (SISL_RESP_HANDLE_T_BIT)) == toggle) {


#ifdef _OLD_CODE
	    fprintf(stderr,"\n IOARCB completed with iosa = 0x%x,  flags = 0x%x\n",ioarcb->ioasc,ioarcb->ioasa_flags);
#else
	    fprintf(stderr,"\n IOARCB completed with iosa = 0x%x,  resid = 0x%x\n",ioasa->ioasc,ioasa->resid);
	    fprintf(stderr,"\n        sense data: sense_key = 0x%x, asc = 0x%x, ascq = 0x%x",
			    sense_data->sense_key,sense_data->add_sense_key, sense_data->add_sense_qualifier);
#endif

    

	} else {

	     fprintf(stderr,"\n toggle bit does not match on rrq\n");
	    
	}


    
	
	if (verbose_flag) {
	    fprintf(stderr,"Returned data hex dump\n");
	    hexdump(data_buf,DATA_BUF_SIZE,NULL);
	}
	
	switch (cmd_type) {
	  case TRANSP_STD_INQ_CMD:
	    
	    rc = cflash_process_scsi_inquiry(data_buf,255,vendor_id,product_id);
	    
	    printf("inq_data: vendor_id = %s, product_id = %s\n",vendor_id,product_id);
	    break;
	    
	  case TRANSP_PG83_INQ_CMD:
	    cflash_process_scsi_inquiry_dev_id_page(data_buf,255,wwid);
	    printf("wwid = %s\n",wwid);
	    break;
	    
	  case TRANSP_RD_CAP_CMD:
	    
	    readcap16_data = (struct readcap16_data *)data_buf;
	    if (cflash_process_scsi_read_cap16(readcap16_data,&block_size,&last_lba) == 0) {
		printf("last lba = 0x%" PRIx64 ", block length = 0x%x\n",
		       last_lba, block_size);
	    }
	    break;
	    
	  case TRANSP_RPT_LUNS_CMD:

	    rc = cflash_process_scsi_report_luns(data_buf,DATA_BUF_SIZE,(uint64_t **)&actual_lun_list,&num_actual_luns);
	    
	    
	    printf("Report Luns data: number of luns returned = %d\n",num_actual_luns);
	    
	    if (num_actual_luns > LUN_LIST_SIZE) {
		num_actual_luns = LUN_LIST_SIZE;
	    }
	    
	    for (i=0; i < num_actual_luns; i++) {
		printf("%d: lun_id = %lx\n",i,actual_lun_list[i]);
		
	    }
	    
	    break;
	  default:
	    break;
	} /* switch */
	INC_RRQ();
    }

    free(ioarcb);
    free(data_buf);
    return rc;
}


/*
 * NAME:        Usage
 *
 * FUNCTION:    print usage message and returns
 *
 *
 * INPUTS:
 *    argc  -- INPUT; the argc parm passed into main().
 *    argv  -- INPUT; the argv parm passed into main().
 *
 * RETURNS:
 *              0: Success
 *              -1: Error
 *              
 */
static void
usage(void)
{

    fprintf(stderr,"\n");
    
    fprintf(stderr,"Usage: transport_test -l device [-n num_loops] [-c cmd_type ] [-f filename ] [-i lun_id] [-b block_number] [-p port_num] [-h]\n\n");
    fprintf(stderr,"      where:\n");
    fprintf(stderr,"              -b  block number (default is 0 if b flag is not specified) \n");
    fprintf(stderr,"              -c  cmd_type which is a number\n");
    fprintf(stderr,"                  defined as following:\n");
    fprintf(stderr,"                  1 - SCSI Read 16 (default if c flag not specified)\n");
    fprintf(stderr,"                  2 - SCSI Write 16 \n");
    fprintf(stderr,"                  3 - SCSI Inquiry (std) \n");
    fprintf(stderr,"                  4 - SCSI Inquiry (0x83)\n");
    fprintf(stderr,"                  5 - SCSI Read Capacity 16 \n");
    fprintf(stderr,"                  6 - SCSI Report Luns\n");
    fprintf(stderr,"                  7 - SCSI Test Unit Readu \n");
    fprintf(stderr,"                  8 - SCSI Mode Sense 10 \n");
    fprintf(stderr,"                  9 - SCSI Mode Select 10 \n");
    fprintf(stderr,"                 10 - SCSI Request Sense\n");
    fprintf(stderr,"              -f  filename\n");
    fprintf(stderr,"              -h  help (this usage)\n");
    fprintf(stderr,"              -i  lun_id in hex (default is 0 if i flag is not specified) \n");
    fprintf(stderr,"              -l  logical device name\n");
    fprintf(stderr,"              -n  Number of loops to run ioctl\n");
    fprintf(stderr,"              -p  Port number mask (0x3 both ports)\n");
    fprintf(stderr,"              -v  verbose mode\n");

    
    return;
}

/*
 * NAME:        parse_args
 *
 * FUNCTION:    The parse_args() routine parses the command line arguments.
 *    		The arguments are read, validated, and stored in global
 *    		variables.
 *
 *
 * INPUTS:
 *    argc  -- INPUT; the argc parm passed into main().
 *    argv  -- INPUT; the argv parm passed into main().
 *
 * RETURNS:
 *              0: Success
 *              -1: Error
 *              
 */

static int
parse_args(int argc, char **argv)
{
    extern int   optind;
    extern char *optarg;
    int rc,c;
    int len,number;


    rc = len = c  = number =0;

    /* Init flags... */
    bflag     = FALSE;
    cflag     = FALSE;
    fflag     = FALSE;
    hflag     = FALSE;
    iflag     = FALSE;
    lflag     = FALSE;
    nflag     = FALSE;
    verbose_flag = FALSE;
    num_loops = 1;
    port_num = 0x3;

    
    
    /* 
     * Get parameters
     */
    while ((c = getopt(argc,argv,"b:c:f:i:l:n:p:hv")) != EOF)
    {  
	switch (c)
	{ 
	case 'b' :
	    if (optarg) {

		block_number = strtoul(optarg,NULL,16);
		iflag = TRUE;
	    } else {


		fprintf(stderr,"-b flag requires a block number be supplied\n");

	    }
	    break;
	case 'c' :
	    if (optarg) {
		
		cmd_type = atoi(optarg);
		
		if ((cmd_type < TRANSP_READ_CMD) ||
		    (cmd_type > TRANSP_LAST_CMD)) {
		    fprintf(stderr,"Invalid cmd_tyupe for -c flag\n");
		    
		    usage();
		    rc = -1;
		} else {
		    cflag = TRUE;
		}
	    } else {
		
		
		fprintf(stderr,"-c flag requires a value to be supplied\n");
		
	    }
	    break;
	  case 'f' :
	      filename = optarg;
	    if (filename) {
		fflag = TRUE;
	    } else {
		
		fprintf(stderr,"-f flag requires a filename \n");
		
	    }
	    break; 

	case 'h' :
	    hflag = TRUE;
	    break; 
	case 'i' :
	    if (optarg) {

		lun_id = strtoul(optarg,NULL,16);
		iflag = TRUE;
	    } else {


		fprintf(stderr,"-i flag requires a lun id be supplied\n");

	    }
	    break;
	case 'l' :
	    device_name = optarg;
	    if (device_name) {
		lflag = TRUE;
	    } else {

		fprintf(stderr,"-l flag requires a logical name \n");

	    }
	    break; 
	case 'n' :
	    if (optarg) {

		num_loops = atoi(optarg);
		nflag = TRUE;
	    } else {


		fprintf(stderr,"-n flag requires a number of loops value to be supplied\n");

	    }
	    break;
	case 'p' :
	    if (optarg) {

		port_num = atoi(optarg);
		nflag = TRUE;
	    } else {


		fprintf(stderr,"-p flag requires port mask value to be supplied\n");

	    }
	    break;
	case 'v' :
	    verbose_flag = TRUE;
	    break; 
	default: 
	    usage();
	    break;
	}/*switch*/
    }/*while*/


    if (!lflag) {
	fprintf(stderr,"The -l flag is required to specify a device name\n");
	usage();
	rc = EINVAL;
    }


    return (rc);


}/*parse_args*/


/*
 * NAME:        main
 *
 * FUNCTION:    Simple test program to validate transport
 *              interfaces to a CAPI Flash AFU.
 *
 *
 * INPUTS:
 *              TBD
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int main (int argc, char **argv)
{
    int rc;           /* Return code                 */
    int fd;           /* File descriptor for adapter */
    void *mmio;       /* MMIO space that is mmapped  */
    int i;
    uint64_t file_len = 0;


    if (!valid_endianess()) {
	fprintf(stderr,"This program is compiled for different endianness then the host is is running\n");
	 
    }

   /* parse the input args & handle syntax request quickly */
    rc = parse_args( argc, argv );
    if (rc)
    {   
        return (0);
    }

    if ( hflag)
    {   
        usage();
        return (0);
    }

    if (fflag) {

	if (cmd_type == TRANSP_WRITE_CMD) {
	    /*
	     * If we are doing a write and
	     * filename is specified then this
	     * file will be input file for data
	     * we will write to the device.
	     */
	    
	    /* Open file */
	    file = fopen(filename, "rb");
	    
	    if (!file) {
		fprintf(stderr,"Failed to open filename = %s\n",filename);
		
		return errno;
	    }
	    
	    /* Get file length */
	    
	    fseek(file, 0, SEEK_END);
	    file_len=ftell(file);
	    fseek(file, 0, SEEK_SET);

	    if (file_len < DATA_BUF_SIZE) {

		fprintf(stderr,"Input file must be at least 4096 bytes in size\n");
		
		fclose(file);
		return 0;
	    }
	    
	    
	    
	} else {

	    fflag = FALSE;
	}
    }
	
    rc = open_and_setup(device_name,&fd,&mmio);

    if (rc) {

	fprintf(stderr,"Open and setup failed rc = %d\n",rc);
	if (fflag) {
	    fclose(file);
	}
	return rc;
    }

    

    for (i=0;i< num_loops;i++) {
	rc = issue_ioarcb(fd,mmio,block_number);

	if (rc) {
	    break;
	}
    }

    close_and_cleanup(fd,mmio);
    if (fflag) {
	fclose(file);
    }
    return 0;
}
