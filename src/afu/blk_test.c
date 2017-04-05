/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/blk_test.c $                                         */
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
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _MACOSX 
#include <malloc.h>
#endif /* !_MACOS */
#include <unistd.h>
#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif
#include <cflash_tools_user.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
    
#define MAX_NUM_THREADS 4096

#define MAX_OPENS 520


typedef struct blk_thread_s {
    pthread_t ptid;
    chunk_id_t chunk_id;
} blk_thread_t;


blk_thread_t blk_thread[MAX_NUM_THREADS];

blk_thread_t alt_blk_thread[MAX_NUM_THREADS];

static pthread_mutex_t  completion_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t thread_count = 0;

/* TODO: ??This data buf size may need to change */

#define DATA_BUF_SIZE  4096

/*
 * Global variables declared in this module...
 */
static int aresult_wait_flag;   /* indicates the a flag was passed on command
                                 * line which indicates to tell cblk_aresult
                                 * to wait.
                                 */
static int bflag;               /* indicates the b flag was passed on command
                                 * line which indicates a block number was 
                                 * specified.
                                 */
static int no_bg_thread_flag;   /* indicates the B flag was passed on command
                                 * line which indicates to not have the block
                                 * library used background threads.
                                 */
static int shared_contxt_flag;  /* indicates the C flag was passed on command
                                 * line which indicates to share contexts.
                                 */
static int cflag;               /* indicates the c flag was passed on command
                                 * line which indicates a cmd_type was 
                                 * specified.
                                 */
static int dif_blk_flag;        /* indicates the d flag was passed on command
                                 * line which indicates each thread should
                                 * use a different range of blocks.
                                 */
static int each_thread_vlun_flag;/* indicates the e flag was passed on command
                                 * line which indicates each thread should
                                 * use its own virtual lun.
                                 */
static int fflag;               /* indicates the f flag was passed on command
                                 * line which indicates a filename is
                                 * specified
                                 */

static int fork_rerun_flag;     /* indicates the z flag was passed on command
                                 * line which indicates to run the operation
				 * then fork a child to rerun the same operation.
                                 */
static int hflag;               /* help flag */

static int inc_flag;            /* indicates the i flag was passed on command
                                 * line which indicates adapter/device name 
                                 * is specified
                                 */
static int lflag;               /* indicates the l flag was passed on command
                                 * line which indicates adapter/device name 
                                 * is specified
                                 */
static int mpio_flag;           /* indicates the m flag was passed on command
                                 * line which indicates enable MPIO fail over
                                 */
static int nflag;               /* indicates the n flag was passed on command
                                 * line which indicates the number of loops 
                                 * or times to issue requests
                                 */
static int open_flag;           /* indicates the o flag was passed on command
                                 * line which indicates the number of opens
				 * to do for this chunk before attempting an I/O.
                                 */
static int pool_flag;           /* indicates the p flag was passed on command
                                 * line which indicates what command pool
				 * size should be for this chunk.
                                 */
static int reserve_flag;        /* indicates the r flag was passed on command
                                 * line which indicates to allow the disk
				 * driver to use normal reserve policy..
                                 */
static int break_reserve_flag;  /* indicates the r flag was passed on command
                                 * line which indicates to allow the disk
				 * driver to break resources and then 
				 * use normal reserve policy..
                                 */
static int size_flag;           /* indicates the s flag was passed on command
                                 * line which indicates what size to usses for
				 * a chunk.
                                 */
static int poll_status_flag;    /* indicates the S flag was passed on command
                                 * line which indicates poll on user status 
				 * area.
                                 */

static int thread_flag;         /* indicates the t flag was passed on command
                                 * line which indicates that we are to run
                                 * multiple threads.
				 */

static int timeout_flag;        /* indicates the T flag was passed on command
                                 * line which indicates that we have a user
				 * specified time out value.
				 */

static int verbose_flag;         /* verbose mode flag */


static int aresult_next_flag;   /* indicates the x flag was passed on command
                                 * line which indicates  to tell cblk_aresult
                                 * to return on the next command that completes.
                                 */

static int wait_async_issue_flag;/* indicates the w flag was passed on command
                                 * line which indicates that we are to wait
                                 * to issue the async I/O
				 */

static int virtual_lun_flag;     /* virtual lun flag */

static char *device_name = NULL; /* point to the arg for -l flag */
static char *alt_device_name = NULL; /* point to the arg for -C flag */
static char *filename = NULL;    /* point to the arg for -f flag */
static int num_loops = 1;
static uint64_t block_number = 0;
static size_t size  = 0;
static uint64_t timeout  = 0;
static int num_opens = 1;
static int num_threads = 0;
static int pool_size = 0;

FILE *file = NULL;

typedef
enum {
    TRANSP_READ_CMD     = 0x1,  /* Read Operation                     */
    TRANSP_WRITE_CMD    = 0x2,  /* Write Operation                    */
    TRANSP_GET_SIZE     = 0x3,  /* Get Size                           */
    TRANSP_SET_SIZE     = 0x4,  /* Set Size                           */
    TRANSP_GET_LUN_SIZE = 0x5,  /* Get Lun size                       */
    TRANSP_AREAD_CMD    = 0x6,  /* Async Read Operation               */
    TRANSP_AWRITE_CMD   = 0x7,  /* Async Write Operation              */
    TRANSP_WRITE_READ   = 0x8,  /* Performan write/read compare       */
    TRANSP_AWRITE_AREAD = 0x9,  /* Performan async write/read compare */
    TRANSP_MWRITE_MREAD = 0xa,  /* Performan mix of write/read        */
				/* with async write/read compare      */
    TRANSP_GET_STATS    = 0xb,  /* Get Statistics                     */
    TRANSP_WRT_FRK_RD   = 0xc,  /* Write fork read                    */
    TRANSP_AREAD_NEXT   = 0xd,  /* Issue a collection of async reads  */
				/* then wait for next commands to     */
				/* complete.                          */
    TRANSP_LWRITE_LREAD = 0xe,  /* Perform listio write/read compare  */
    TRANSP_LAST_CMD     = 0xf,  /* Not valid command                  */
} transp_scsi_cmd_t;

transp_scsi_cmd_t cmd_type = TRANSP_READ_CMD; // Default to READ

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
    
    fprintf(stderr,"Usage: blk_test -l device [-n num_loops] [-c cmd_type ] [-f filename ] [-b block_number] [-o num_opens ]  [-p pool_size ]  [-s size ] [-t num_threads ] [-T time_out ] [-h] [-a ] [-B ] [-C shared_device] [-i] [-e] [-m] [-S] [-u] [-v][-z]\n\n");
    fprintf(stderr,"      where:\n");
    fprintf(stderr,"              -a  indicates to cblk_aresult should be called to wait for completion\n");
    fprintf(stderr,"              -b  block number (default is 0 if b flag is not specified) \n");
    fprintf(stderr,"              -B  Do not allow back ground threads in block library\n");
    fprintf(stderr,"              -c  cmd_type which is a number\n");
    fprintf(stderr,"                  defined as following:\n");
    fprintf(stderr,"                  1 - read\n");
    fprintf(stderr,"                  2 - write\n");
    fprintf(stderr,"                  3 - get_size\n");
    fprintf(stderr,"                  4 - set_size \n");
    fprintf(stderr,"                  5 - get_lun_size\n");
    fprintf(stderr,"                  6 - async read\n");
    fprintf(stderr,"                  7 - async write\n");
    fprintf(stderr,"                  8 - write read compare\n");
    fprintf(stderr,"                  9 - async write async read compare\n");
    fprintf(stderr,"                  10 - mix write read compare with async write async read compare\n");
    fprintf(stderr,"                  11 - get_statistics\n");
    fprintf(stderr,"                  12 - write fork read (single threaded only)\n");
    fprintf(stderr,"                  13 - burst async reads then wait for all to complete(single threaded only)\n");
    fprintf(stderr,"                  14 - listio write listio read compare\n");
    fprintf(stderr,"              -C  enable shared context\n");
    fprintf(stderr,"              -d  indicates to use different block ranges for each thread\n");
    fprintf(stderr,"              -e  each thread uses its own virtual lun\n");
    fprintf(stderr,"              -f  filename (for input data to be used on writes)\n");
    fprintf(stderr,"              -h  help (this usage)\n");
    fprintf(stderr,"              -i  increment flag: indicates block numbers should be incrementd on each loop\n");
    fprintf(stderr,"              -l  logical device name\n");
    fprintf(stderr,"              -m  Enable MPIO fail over\n");
    fprintf(stderr,"              -n  Number of loops to run\n");
    fprintf(stderr,"              -o  Number of opens before doing I/O\n");
    fprintf(stderr,"              -p  Pool Size (number of commands in command pool)\n");
    fprintf(stderr,"              -s  size (number of blocks for chunk in hex)\n");
    fprintf(stderr,"              -S  For async I/O poll on user specified status field\n");
    fprintf(stderr,"              -t  Number of threads\n");
    fprintf(stderr,"              -T  Time out in microseconds (hex) for cblk_listio\n");
    fprintf(stderr,"              -u  Virtual Lun for entire process. NOTE -e is a virtual lun for each thread\n");
    fprintf(stderr,"              -w  wait to issue async I/O\n");
    fprintf(stderr,"              -x  indicates to cblk_aresult should be called to return on the next command\n");
    fprintf(stderr,"              -z  One operation to completion, then fork and have child do same operation again.\n");
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
    aresult_wait_flag = FALSE;
    aresult_next_flag = FALSE;
    bflag     = FALSE;
    cflag     = FALSE;
    fflag     = FALSE;
    hflag     = FALSE;
    lflag     = FALSE;
    nflag     = FALSE;
    fork_rerun_flag = FALSE;
    size_flag = FALSE;
    thread_flag = FALSE;
    timeout_flag = FALSE;
    verbose_flag = FALSE;
    each_thread_vlun_flag = FALSE;
    virtual_lun_flag = FALSE;
    poll_status_flag = FALSE;
    shared_contxt_flag = FALSE;
    mpio_flag = FALSE;
    reserve_flag = FALSE;
    break_reserve_flag = FALSE;
    num_loops = 1;

    
    
    /* 
     * Get parameters
     */
    while ((c = getopt(argc,argv,"b:c:C:f:l:n:p:o:s:t:T:aBdehimrRSuvwxz")) != EOF)
    {  
	switch (c)
	{ 
	case 'a' :
	    aresult_wait_flag = TRUE;
	    break; 
	case 'b' :
	    if (optarg) {

		block_number = strtoul(optarg,NULL,16);
		bflag = TRUE;
	    } else {


		fprintf(stderr,"-b flag requires a block number be supplied\n");
		
		rc = EINVAL;
	    }
	    break;
	case 'B' :
	    no_bg_thread_flag = TRUE;
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
	case 'C' :
	    alt_device_name = optarg;
	    if (alt_device_name) {
	        shared_contxt_flag = TRUE;
	    } else {

		fprintf(stderr,"-C flag requires a logical name \n");
		rc = EINVAL;

	    }
	    break; 
	case 'd' :
	    dif_blk_flag = TRUE;
	    break; 
	case 'e' :
	    each_thread_vlun_flag = TRUE;
	    break;
	case 'f' :
	      filename = optarg;
	    if (filename) {
		fflag = TRUE;
	    } else {
		
		fprintf(stderr,"-f flag requires a filename \n");
		rc = EINVAL;
		
	    }
	    break; 

	case 'h' :
	    hflag = TRUE;
	    break; 
	case 'i' :
	    inc_flag = TRUE;
	    break; 
	case 'l' :
	    device_name = optarg;
	    if (device_name) {
		lflag = TRUE;
	    } else {

		fprintf(stderr,"-l flag requires a logical name \n");
		rc = EINVAL;

	    }
	    break;
	case 'm' :
	    mpio_flag = TRUE;

	    
	    break;  
	case 'n' :
	    if (optarg) {

		num_loops = atoi(optarg);
		nflag = TRUE;
	    } else {


		fprintf(stderr,"-n flag requires a number of loops value to be supplied\n");
		rc = EINVAL;

	    }
	    break;
	case 'o' :
	    if (optarg) {

		num_opens = atoi(optarg);

		if (num_opens > MAX_OPENS) {
		    fprintf(stderr,"Number of opens %d exceedds the largest value supported %d\n",MAX_OPENS, num_opens);
		    
		    rc = EINVAL;
		} 
		open_flag = TRUE;
	    } else {


		fprintf(stderr,"-o flag requires a number of op value to be supplied\n");

	    }
	    break;
	case 'p' :
	    if (optarg) {

		pool_size = atoi(optarg);
		pool_flag = TRUE;
	    } else {


		fprintf(stderr,"-p flag requires a pool size value to be supplied\n");

	    }
	    break;
	case 'r' :
#ifndef _AIX
	    fprintf(stderr,"-r flag is not supported on this operating system\n");
	    rc = EINVAL;
#else
	    reserve_flag = TRUE;
#endif 
	    
	    break; 
	case 'R' :
#ifndef _AIX
	    fprintf(stderr,"-R flag is not supported on this operating system\n");
	    rc = EINVAL;
#else
	    break_reserve_flag = TRUE;
#endif 
	    break; 
        case 's' :
            if (optarg) {

                size = strtoul(optarg,NULL,16);
                size_flag = TRUE;
            } else {


                fprintf(stderr,"-s flag requires a size be supplied\n");

            }
            break;
	case 'S' :
	    poll_status_flag = TRUE;
	    break;
	case 't' :
	    if (optarg) {

		num_threads = atoi(optarg);

		if (num_threads > MAX_NUM_THREADS) {

		    num_threads = MAX_NUM_THREADS;

		    fprintf(stderr,"-number of threads exceeds programs upper limit, reducing number to %d\n",num_threads);
		}
		thread_flag = TRUE;
	    } else {


		fprintf(stderr,"-t flag requires a number of threads value to be supplied\n");
	    }
	    break;
	case 'T' :
            if (optarg) {

                timeout = strtoul(optarg,NULL,16);
                timeout_flag = TRUE;
            } else {


                fprintf(stderr,"-T flag requires a value be supplied\n");

            }
	    break;
	case 'u' :
	    virtual_lun_flag = TRUE;
	    break;
	case 'v' :
	    verbose_flag = TRUE;
	    break; 
	case 'w' :
	    wait_async_issue_flag = TRUE;
	    break; 
	case 'x' :
	    aresult_next_flag = TRUE;
	    break; 
	case 'z' :
	    fork_rerun_flag = TRUE;
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

    if (thread_flag && (cmd_type == TRANSP_WRT_FRK_RD)) {

	fprintf(stderr,"The -c cmd_type of %d does not support the -t flag \n",TRANSP_WRT_FRK_RD);
	usage();
	rc = EINVAL;
    }

    if ((pool_size == 0) && (cmd_type == TRANSP_AREAD_NEXT)) {

	fprintf(stderr,"The -c cmd_type of %d pool size be specified (-p flag) \n",TRANSP_AREAD_NEXT);
	usage();
	rc = EINVAL;
    }
    if (thread_flag && (cmd_type == TRANSP_AREAD_NEXT)) {

	fprintf(stderr,"The -c cmd_type of %d does not support the -t flag \n",TRANSP_AREAD_NEXT);
	usage();
	rc = EINVAL;
    }

    if (thread_flag && (aresult_next_flag)) {

	fprintf(stderr,"The -x does not support the -t flag \n");
	usage();
	rc = EINVAL;
    }

    if (poll_status_flag &&
	(cmd_type != TRANSP_AREAD_CMD) &&
	(cmd_type != TRANSP_AWRITE_CMD) &&
	(cmd_type != TRANSP_AWRITE_AREAD) &&
	(cmd_type != TRANSP_MWRITE_MREAD)) {


	fprintf(stderr,"The -S flag is not supported with this cmd_type. Instead it requires async I/O \n");
	usage();
	rc = EINVAL;
    }

    if (timeout_flag &&
	(cmd_type != TRANSP_LWRITE_LREAD)) {


	fprintf(stderr,"The -T flag is not supported with this cmd_type. Instead it requires listio I/O \n");
	usage();
	rc = EINVAL;
    }

    if ((cmd_type == TRANSP_SET_SIZE) &&
	(!size_flag)) {

	fprintf(stderr,"The -c cmd_type of 4 requires the -s flag \n");
	usage();
	rc = EINVAL;
    }
    return (rc);


}/*parse_args*/

/*
 *******************************************************************
 * NAME       : run_loop
 * DESCRIPTION:
 *    Run commands in a loop num_loops times.
 *
 * PARAMETERS:
 *    
 * GLOBALS ACCESSED:
 *    
 * RETURNS:
 *    nothing.
 *******************************************************************
 */
void *run_loop(void *data)
{
    void *ret_code = NULL;
    int rc = 0;
    int tag;
    int i;
    int exit_loop = FALSE;
    blk_thread_t *blk_data = data;
    uint32_t local_thread_count = 0;
    void *data_buf = NULL;
    void *comp_data_buf = NULL;
    size_t num_blocks;
    uint64_t blk_number;
    uint64_t status;
    int bytes_read = 0;
    transp_scsi_cmd_t local_cmd_type;
    chunk_stats_t stats;
    int async_flags = 0;
    int aresult_flags = 0;
    int open_flags = 0;
    int active_cmds;
    pid_t pid;
    cblk_arw_status_t arw_status;
    chunk_ext_arg_t ext = 0;
    cblk_io_t cblk_issue_io;
    cblk_io_t *cblk_issue_ary[1];
    cblk_io_t cblk_complete_io;
    cblk_io_t *cblk_complete_ary[1];
    int  complete_items;


    bzero(&arw_status,sizeof(arw_status));

    pthread_mutex_lock(&completion_lock);
    local_thread_count = thread_count++;

    if (dif_blk_flag) {
	/*
	 * Each thread is using a different
	 * block number range.
	 */
	blk_number = block_number + (num_loops * thread_count);

    } else {
	blk_number = block_number;
    }

    if (aresult_wait_flag) {

	aresult_flags = CBLK_ARESULT_BLOCKING;
    }
    if ((aresult_next_flag) && (!thread_flag)) {

	/*
	 * For multiple threads all reading/writing
	 * we can not allow one thread's completion
	 * to be seen by another thread, since it 
	 * did not issue the request.
	 */

	aresult_flags |= CBLK_ARESULT_NEXT_TAG;
    }

    pthread_mutex_unlock(&completion_lock);

    if (wait_async_issue_flag) {

	async_flags = CBLK_ARW_WAIT_CMD_FLAGS;

    } 

    if (poll_status_flag) {

	async_flags |= CBLK_ARW_USER_STATUS_FLAG;
    }

    /*
     * Align data buffer on page boundary.
     */	
    if ( posix_memalign((void *)&data_buf,4096,DATA_BUF_SIZE)) {

	perror("posix_memalign failed for data buffer");

	return (ret_code);

    }

    errno = 0;

    if (cmd_type == TRANSP_MWRITE_MREAD) {

	/*
	 * Use both synchronous and asynchronouse
	 * I/O. To facilitate this have even threads
	 * use synchronouse and odd ones to use asynchronous
	 * I/O.
	 */

	if (local_thread_count % 2) {
	    local_cmd_type = TRANSP_AWRITE_AREAD;
	} else {
	    local_cmd_type = TRANSP_WRITE_READ;
	}
	
    } else {

	local_cmd_type = cmd_type;
    }


    if (each_thread_vlun_flag) {

	/*
	 * If we are using a virtual lun for each
	 * thread then open the virtual lun now and
	 * set its size.
	 */

	if (verbose_flag && !thread_flag) {
	    fprintf(stderr,"Calling cblk_open ...\n");
	}

	open_flags = CBLK_OPN_VIRT_LUN;

	if (no_bg_thread_flag) {

	    open_flags |= CBLK_OPN_NO_INTRP_THREADS;
	}

	if (shared_contxt_flag) {

	    open_flags |= CBLK_OPN_SHARE_CTXT;
	}


	if (mpio_flag) {

	    open_flags |= CBLK_OPN_MPIO_FO;
	}

#ifdef _AIX
	if (reserve_flag) {

	    open_flags |= CBLK_OPN_RESERVE;
	}

	if (break_reserve_flag) {

	    open_flags |= CBLK_OPN_FORCED_RESERVE;
	}



#endif


	blk_data->chunk_id = cblk_open(device_name,pool_size,O_RDWR,ext,open_flags);

	if (blk_data->chunk_id == NULL_CHUNK_ID) {

	    fprintf(stderr,"Open of (virtual lun) %s failed with errno = %d\n",device_name,errno);

	    free(comp_data_buf);
		
	    free(data_buf);
	    return (ret_code);
	}



	if (verbose_flag && !thread_flag) {

	    fprintf(stderr,"Calling cblk_set_size for size = 0x%"PRIX64" ...\n",size);

	}


	rc = cblk_set_size(blk_data->chunk_id,size,0);

	if (rc) {
	    fprintf(stderr,"cblk_set_size failed with rc = %d, and errno = %d \n",rc,errno);

	    free(comp_data_buf);
		
	    free(data_buf);
	    return (ret_code);

	}

    }

    
    for (i =0; i<num_loops; i++) {

	switch (local_cmd_type) {
	case TRANSP_READ_CMD:
	
	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_read for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

	    
	    if (!thread_flag) {
		printf("Read completed with rc = %d\n",rc);
	    }

	    
	    if (verbose_flag) {
		printf("Returned data is ...\n");
		hexdump(data_buf,DATA_BUF_SIZE,NULL);
	    }
	    break;
	case TRANSP_AREAD_CMD:
	
	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_aread for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_aread(blk_data->chunk_id,data_buf,blk_number,1,&tag,&arw_status,async_flags);

	    if (rc > 0) {
		fprintf(stderr,"cblk_aread succeeded for  lba = 0x%lx, rc = %d, errno = %d\n",blk_number,rc,errno);


		printf("Async read data completed ...\n");
		printf("Returned data is ...\n");
		hexdump(data_buf,DATA_BUF_SIZE,NULL);

	    } else if (rc < 0) {
		fprintf(stderr,"cblk_aread failed for  lba = 0x%lx, rc = %d, errno = %d\n",blk_number,rc,errno);

	    } else {


		if (verbose_flag && !thread_flag) {

		    if (poll_status_flag) {
			fprintf(stderr,"Polling user status for tag = 0x%x ...\n",tag);
		    } else {
			fprintf(stderr,"Calling cblk_aresult for tag = 0x%x ...\n",tag);
		    }
		}
		
		while (TRUE) {

		    if (poll_status_flag) {
			
			switch (arw_status.status) {
			  case CBLK_ARW_STATUS_SUCCESS:
			    rc = arw_status.blocks_transferred;
			    break;
			  case CBLK_ARW_STATUS_PENDING:
			    rc = 0;
			    break;
			  default:
			    rc = -1;
			    errno = arw_status.fail_errno;
			}

		    } else {
			rc = cblk_aresult(blk_data->chunk_id,&tag, &status,aresult_flags);
		    }
		    if (rc > 0) {
		
			if (verbose_flag) {
			    printf("Async read data completed ...\n");
			    printf("Returned data is ...\n");
			    hexdump(data_buf,DATA_BUF_SIZE,NULL);
			}
		    } else if (rc == 0) {
			fprintf(stderr,"cblk_aresult completed: command still active  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			usleep(300);
			continue;
		    } else {
			fprintf(stderr,"cblk_aresult completed for  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
		    }

		    break;

		} /* while */

	    }
	    break;
	case TRANSP_WRITE_CMD:


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
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_write for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_write(blk_data->chunk_id,data_buf,blk_number,1,0);

	    if (!thread_flag) {

		printf("write completed with rc = %d\n",rc);
	    }
	    break;
	case TRANSP_AWRITE_CMD:
	

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
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_awrite for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_awrite(blk_data->chunk_id,data_buf,blk_number,1,&tag,&arw_status,async_flags);

	    if (rc) {
		fprintf(stderr,"cblk_awrite failed for  lba = 0x%lx, rc = %d, errno = %d\n",blk_number,rc,errno);

	    } else {


		if (verbose_flag && !thread_flag) {

		    if (poll_status_flag) {
			fprintf(stderr,"Polling user status for tag = 0x%x ...\n",tag);
		    } else {
			fprintf(stderr,"Calling cblk_aresult for tag = 0x%x ...\n",tag);
		    }
		}
		
		while (TRUE) {

		    if (poll_status_flag) {
			
			switch (arw_status.status) {
			  case CBLK_ARW_STATUS_SUCCESS:
			    rc = arw_status.blocks_transferred;
			    break;
			  case CBLK_ARW_STATUS_PENDING:
			    rc = 0;
			    break;
			  default:
			    rc = -1;
			    errno = arw_status.fail_errno;
			}

		    } else {
			rc = cblk_aresult(blk_data->chunk_id,&tag, &status,aresult_flags);
		    }

		    if (rc > 0) {
		
			if (verbose_flag && !thread_flag) {
			    printf("Async write data completed ...\n");
			}
		    } else if (rc == 0) {
			fprintf(stderr,"cblk_aresult completed: command still active  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			usleep(300);
			continue;
		    } else {
			fprintf(stderr,"cblk_aresult completed for  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);

		    }

		    break;

		} /* while */

	    }
	    break;
	case TRANSP_GET_SIZE:
	    if (verbose_flag) {
		fprintf(stderr,"Calling cblk_get_size ...\n");
	    }
	    rc = cblk_get_size(blk_data->chunk_id,&num_blocks,0);
	    if (!thread_flag) {

		printf("Get_size returned rc = %d, with num_blocks = 0x%lx",
		       rc,num_blocks);
	    }
	    break;
	case TRANSP_GET_LUN_SIZE:
	    if (verbose_flag) {
		fprintf(stderr,"Calling cblk_get_lun_size ...\n");
	    }
	    rc = cblk_get_lun_size(blk_data->chunk_id,&num_blocks,0);
	    if (!thread_flag) {

		printf("Get_lun_size returned rc = %d, with num_blocks = 0x%lx",
		       rc,num_blocks);
	    }
	    break;
	case TRANSP_SET_SIZE:
	    if (verbose_flag) {
		fprintf(stderr,"Calling cblk_set_size ...\n");
	    }
	    rc = cblk_set_size(blk_data->chunk_id,size,0);
	    
	    if (!thread_flag) {

		printf("set_size returned rc = %d, with num_blocks = 0x%lx",
		       rc,size);
	    }
	    break;
	case TRANSP_WRITE_READ:

	    /* 
	     * Perform write then read comparision test
	     */


	    /*
	     * Align data buffer on page boundary.
	     */	
	    if ( posix_memalign((void *)&comp_data_buf,4096,DATA_BUF_SIZE)) {

		perror("posix_memalign failed for data buffer");

		cblk_close(blk_data->chunk_id,0);
		return (ret_code);

	    }

	    if (file) {
		/*
		 * If an input file was specified,
		 * then read the first DATA_BUF_SIZE bytes
		 * in to write out to the device.
		 */
       
		bytes_read = fread(comp_data_buf, 1, DATA_BUF_SIZE, file);
        
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
		memset((uint8_t *)(comp_data_buf), ((blk_number)%256), 
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_write for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_write(blk_data->chunk_id,comp_data_buf,blk_number,1,0);

	    if (!thread_flag) {

		printf("write completed with rc = %d\n",rc);
	    }

	    if (rc != 1) {


		fprintf(stderr,"cblk_write failed for lba = 0x%"PRIX64" with rc = 0x%x errno = %d\n",blk_number,rc,errno);

		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);
	    }
	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_read for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

	    
	    if (!thread_flag) {
		printf("Read completed with rc = %d\n",rc);
	    }

	    if (rc != 1) {


		fprintf(stderr,"cblk_read failed for lba = 0x%"PRIX64" with rc = 0x%x errno = %d\n",blk_number,rc, errno);

		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);
	    }

	    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

	    if (rc) {

		pthread_mutex_lock(&completion_lock);

		fprintf(stderr,"Memcmp failed with rc = 0x%x, for blk_number = 0x%"PRIX64"\n",rc,blk_number);

		printf("Written data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(data_buf,DATA_BUF_SIZE);
		printf("**********************************************************\n\n");
		printf("read data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(comp_data_buf,DATA_BUF_SIZE);

		
		printf("**********************************************************\n\n");

		pthread_mutex_unlock(&completion_lock);
		
		rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

		if (rc == 1) {
		    
		    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

		    pthread_mutex_lock(&completion_lock);
		    if (rc) {


			fprintf(stderr,"Memcmp for re-read failed\nDump of re-read data for blk_number = 0x%"PRIX64"\n",blk_number);
		    
			dumppage(data_buf,DATA_BUF_SIZE);
		    } else {
			
			fprintf(stderr,"Memcmp for re-read successful for blk_number = 0x%"PRIX64"\n",blk_number);
		    }
		    
		    pthread_mutex_unlock(&completion_lock);

		}
		
	    } else if ((verbose_flag) && (!thread_flag)) {
		printf("Memcmp succeeded for blk_number = 0x%"PRIX64"\n",blk_number);
	    }

	    

	    free(comp_data_buf);
	    break;
	case TRANSP_AWRITE_AREAD:

	    /* 
	     * Perform async write then async read comparision test
	     */


	    /*
	     * Align data buffer on page boundary.
	     */	
	    if ( posix_memalign((void *)&comp_data_buf,4096,DATA_BUF_SIZE)) {

		perror("posix_memalign failed for data buffer");

		cblk_close(blk_data->chunk_id,0);
		return (ret_code);

	    }

	    if (file) {
		/*
		 * If an input file was specified,
		 * then read the first DATA_BUF_SIZE bytes
		 * in to write out to the device.
		 */
       
		bytes_read = fread(comp_data_buf, 1, DATA_BUF_SIZE, file);
        
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
		memset((uint8_t *)(comp_data_buf), ((blk_number)%256), 
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_awrite for lba = 0x%"PRIX64" ...\n",blk_number);

	    }


	    rc = cblk_awrite(blk_data->chunk_id,comp_data_buf,blk_number,1,&tag,&arw_status,async_flags);

	    if (rc) {
		fprintf(stderr,"cblk_awrite failed for  lba = 0x%" PRIx64 ", rc = %d, errno = %d\n",blk_number,rc,errno);
		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);

	    } else {


		if (verbose_flag && !thread_flag) {

		    if (poll_status_flag) {
			fprintf(stderr,"Polling user status for tag = 0x%x ...\n",tag);
		    } else {
			fprintf(stderr,"Calling cblk_aresult for tag = 0x%x ...\n",tag);
		    }
		}
		
		while (TRUE) {

		    if (poll_status_flag) {
			
			switch (arw_status.status) {
			  case CBLK_ARW_STATUS_SUCCESS:
			    rc = arw_status.blocks_transferred;
			    break;
			  case CBLK_ARW_STATUS_PENDING:
			    rc = 0;
			    break;
			  default:
			    rc = -1;
			    errno = arw_status.fail_errno;
			}

		    } else {
			rc = cblk_aresult(blk_data->chunk_id,&tag, &status,aresult_flags);
		    }

		    if (rc > 0) {
		
			if (verbose_flag && !thread_flag) {
			    printf("Async write data completed ...\n");
			}
		    } else if (rc == 0) {
			if (!thread_flag) {
			    fprintf(stderr,"cblk_aresult completed: command still active  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			}
			usleep(300);
			continue;
		    } else {
			fprintf(stderr,"cblk_aresult completed (failed write) for  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			
			exit_loop = TRUE;
		    }

		    break;

		} /* while */

	    }

	    if (exit_loop) {

		break;
	    }


	    if (!thread_flag) {

		printf("write completed with rc = %d\n",rc);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_aread for lba = 0x%"PRIX64" ...\n",blk_number);

	    }



	    rc = cblk_aread(blk_data->chunk_id,data_buf,blk_number,1,&tag,&arw_status,async_flags);

	    if (rc > 0) {
		if (!thread_flag)  {
		    fprintf(stderr,"cblk_aread succeeded for  lba = 0x%" PRIx64 " , rc = %d, errno = %d\n",blk_number,rc,errno);


		    printf("Async read data completed ...\n");
		}

	    } else if (rc < 0) {
		fprintf(stderr,"cblk_aread failed for  lba = 0x%" PRIx64 ", rc = %d, errno = %d\n",blk_number,rc,errno);
		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);

	    } else {


		if (verbose_flag && !thread_flag) {

		    if (poll_status_flag) {
			fprintf(stderr,"Polling user status for tag = 0x%x ...\n",tag);
		    } else {
			fprintf(stderr,"Calling cblk_aresult for tag = 0x%x ...\n",tag);
		    }
		}
		
		while (TRUE) {

		    if (poll_status_flag) {
			
			switch (arw_status.status) {
			  case CBLK_ARW_STATUS_SUCCESS:
			    rc = arw_status.blocks_transferred;
			    break;
			  case CBLK_ARW_STATUS_PENDING:
			    rc = 0;
			    break;
			  default:
			    rc = -1;
			    errno = arw_status.fail_errno;
			}

		    } else {
			rc = cblk_aresult(blk_data->chunk_id,&tag, &status,aresult_flags);
		    }

		    if (rc > 0) {
		
			if (verbose_flag && !thread_flag) {
			    printf("Async read data completed ...\n");
			}
		    } else if (rc == 0) {
			if (!thread_flag) {
			    fprintf(stderr,"cblk_aresult completed: command still active  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			}
			usleep(300);
			continue;
		    } else {
			fprintf(stderr,"cblk_aresult completed (failed read) for  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
			exit_loop = TRUE;
		    }

		    break;

		} /* while */

	    }

	    if (exit_loop) {

		break;
	    }
	    if (!thread_flag) {
		printf("Read completed with rc = %d\n",rc);
	    }

	    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

	    if (rc) {

		pthread_mutex_lock(&completion_lock);

		fprintf(stderr,"Memcmp failed with rc = 0x%x, for blk_number = 0x%"PRIX64"\n",rc,blk_number);

		printf("Written data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(data_buf,DATA_BUF_SIZE);
		printf("**********************************************************\n\n");
		printf("read data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(comp_data_buf,DATA_BUF_SIZE);

		printf("**********************************************************\n\n");
		pthread_mutex_unlock(&completion_lock);

		rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

		if (rc == 1) {
		    
		    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

		    pthread_mutex_lock(&completion_lock);
		    if (rc) {



			fprintf(stderr,"Memcmp for re-read failed\nDump of re-read data for blk_number = 0x%"PRIX64"\n",blk_number);
		    
			dumppage(data_buf,DATA_BUF_SIZE);
		    } else {
			
			fprintf(stderr,"Memcmp for re-read successful for blk_number = 0x%"PRIX64"\n",blk_number);
		    }
		    
		    pthread_mutex_unlock(&completion_lock);
		}
		
	    } else if ((verbose_flag) && (!thread_flag)) {
		printf("Memcmp succeeded for blk_number = 0x%"PRIX64"\n",blk_number);
	    }

	    
	    free(comp_data_buf);
	    break;
	case TRANSP_GET_STATS:
	    
	    bzero (&stats, sizeof(stats));
	    if (verbose_flag) {
		fprintf(stderr,"Calling cblk_get_stats ...\n");
	    }
	    rc = cblk_get_stats(blk_data->chunk_id,&stats,0);
	    if (!thread_flag) {
		
		printf("Get_stats returned rc = %d",rc);

		
		hexdump(&stats,sizeof(stats),NULL);
	    }
	    break;
	case TRANSP_WRT_FRK_RD:

	    /* 
	     * Perform write then fork and child read. comparision test
	     */


	    /*
	     * Align data buffer on page boundary.
	     */	
	    if ( posix_memalign((void *)&comp_data_buf,4096,DATA_BUF_SIZE)) {

		perror("posix_memalign failed for data buffer");

		cblk_close(blk_data->chunk_id,0);
		return (ret_code);

	    }

	    if (file) {
		/*
		 * If an input file was specified,
		 * then read the first DATA_BUF_SIZE bytes
		 * in to write out to the device.
		 */
       
		bytes_read = fread(comp_data_buf, 1, DATA_BUF_SIZE, file);
        
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
		memset((uint8_t *)(comp_data_buf), ((blk_number)%256), 
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_write for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_write(blk_data->chunk_id,comp_data_buf,blk_number,1,0);

	    if (!thread_flag) {

		printf("write completed with rc = %d\n",rc);
	    }

	    if (rc != 1) {


		fprintf(stderr,"cblk_write failed for lba = 0x%"PRIX64" with rc = 0x%x errno = %d\n",blk_number,rc,errno);

		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);
	    }
	    
	    if ((pid = fork()) < 0) {  /* fork failed */
                perror("Fork failed \n");
		free(comp_data_buf);
		
		break;       
	    }                 
	    else if (pid > 0)  {               /* parents fork call */

		if (verbose_flag && !thread_flag) {
		    fprintf(stderr,"Fork succeeded \n");
		}

		/*
		 * Sleep for a while to allow child process to complete
		 */

		sleep(10);
		free(comp_data_buf);
		

		break;
	    }
	    
	    /*
	     * Only the child process will reach this point
	     * ie fork = 0.
	     */
	    

	    if (virtual_lun_flag) {

		/*
		 * Clone after fork is only supported for
		 * virtual luns. Physical luns do not need this.
		 */


		if (verbose_flag && !thread_flag) {

		    fprintf(stderr,"Calling cblk_clone_after_fork...\n");

		}
		rc = cblk_clone_after_fork(blk_data->chunk_id,O_RDONLY,0);

	    
		if (verbose_flag && !thread_flag) {
		    printf("clone completed with rc = %d, errno = %d\n",rc,errno);
		}

		if (rc) {


		    fprintf(stderr,"cblk_chunk_clone failed with rc = 0x%x errno = %d\n",rc, errno);
		    free(comp_data_buf);
		
		    free(data_buf);
		    return (ret_code);
		}

	    }


	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_read for lba = 0x%"PRIX64" ...\n",blk_number);

	    }
	    rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

	    
	    if (!thread_flag) {
		printf("Read completed with rc = %d\n",rc);
	    }

	    if (rc != 1) {


		fprintf(stderr,"cblk_read failed for lba = 0x%"PRIX64" with rc = 0x%x errno = %d\n",blk_number,rc, errno);

		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);
	    }

	    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

	    if (rc) {

		pthread_mutex_lock(&completion_lock);

		fprintf(stderr,"Memcmp failed with rc = 0x%x, for blk_number = 0x%"PRIX64"\n",rc,blk_number);

		printf("Written data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(data_buf,DATA_BUF_SIZE);
		printf("**********************************************************\n\n");
		printf("read data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(comp_data_buf,DATA_BUF_SIZE);
		
		printf("**********************************************************\n\n");

		pthread_mutex_unlock(&completion_lock);


		rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

		if (rc == 1) {
		    
		    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

		    pthread_mutex_lock(&completion_lock);
		    if (rc) {


			fprintf(stderr,"Memcmp for re-read failed\nDump of re-read data for blk_number = 0x%"PRIX64"\n",blk_number);
		    
			dumppage(data_buf,DATA_BUF_SIZE);
		    } else {
			
			fprintf(stderr,"Memcmp for re-read successful for blk_number = 0x%"PRIX64"\n",blk_number);
		    }
		    
		    pthread_mutex_unlock(&completion_lock);

		}
		
	    } else if ((verbose_flag) && (!thread_flag)) {
		printf("Memcmp succeeded for blk_number = 0x%"PRIX64"\n",blk_number);

	    }

	    free(comp_data_buf);

	    break;
	case TRANSP_AREAD_NEXT:
	
	    
	    if (!pool_size) {

		fprintf(stderr,"Pool size not specified\n");
	    }

	    active_cmds = 0;
	    while (active_cmds < pool_size) {

		if (verbose_flag && !thread_flag) {

		    fprintf(stderr,"Calling cblk_aread for lba = 0x%"PRIX64" ...\n",blk_number);

		}
		rc = cblk_aread(blk_data->chunk_id,data_buf,blk_number,1,&tag,&arw_status,async_flags);

		if (rc > 0) {
		    fprintf(stderr,"cblk_aread succeeded for  lba = 0x%lx, rc = %d, errno = %d\n",blk_number,rc,errno);


		    printf("Async read data completed ...\n");
		    printf("Returned data is ...\n");
		    hexdump(data_buf,DATA_BUF_SIZE,NULL);

		} else if (rc < 0) {
		    fprintf(stderr,"cblk_aread failed for  lba = 0x%lx, rc = %d, errno = %d\n",blk_number,rc,errno);


		} else {

		    active_cmds++;
		    
		    if (inc_flag) {
			
			blk_number++;
			i++;

			if (i == num_loops) {

			    break;
			}
		    }
		    

		}

	    } /* while loop */


	    /*
	     * Now wait for all queued requests to complete.
	     */

	    aresult_flags |= CBLK_ARESULT_NEXT_TAG;

	    while (active_cmds) {
		
		
		if (verbose_flag && !thread_flag) {
		    fprintf(stderr,"Calling cblk_aresult...\n");
		}
		    
		rc = cblk_aresult(blk_data->chunk_id,&tag, &status,aresult_flags);

		if (rc > 0) {
		    
		    active_cmds--;
		    if (verbose_flag) {
			printf("Async read data completed for tag = 0x%x...\n",tag);
			printf("Returned data is ...\n");
			hexdump(data_buf,DATA_BUF_SIZE,NULL);
		    }
		} else if (rc == 0) {
		    fprintf(stderr,"cblk_aresult completed: command still active  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);
		    usleep(300);
		    continue;
		} else {
		    fprintf(stderr,"cblk_aresult completed for  for tag = 0x%x, rc = %d, errno = %d\n",tag,rc,errno);


		    active_cmds--;

		}

	    } /* while loop */


	    break;

	case TRANSP_LWRITE_LREAD:

	    /* 
	     * Perform async write then async read comparision test
	     */

	    


	    /*
	     * Align data buffer on page boundary.
	     */	
	    if ( posix_memalign((void *)&comp_data_buf,4096,DATA_BUF_SIZE)) {

		perror("posix_memalign failed for data buffer");

		cblk_close(blk_data->chunk_id,0);
		return (ret_code);

	    }

	    if (file) {
		/*
		 * If an input file was specified,
		 * then read the first DATA_BUF_SIZE bytes
		 * in to write out to the device.
		 */
       
		bytes_read = fread(comp_data_buf, 1, DATA_BUF_SIZE, file);
        
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
		memset((uint8_t *)(comp_data_buf), ((blk_number)%256), 
		       DATA_BUF_SIZE);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_listio write for lba = 0x%"PRIX64" ...\n",blk_number);

	    }

	    bzero(&cblk_issue_io,sizeof(cblk_issue_io));

	    cblk_issue_io.request_type = CBLK_IO_TYPE_WRITE;
	    cblk_issue_io.buf = comp_data_buf;
	    cblk_issue_io.lba = blk_number,
	    cblk_issue_io.nblocks = 1;

	    
	    bzero(&cblk_complete_io,sizeof(cblk_complete_io));

	    cblk_issue_ary[0] = &cblk_issue_io;
	    cblk_complete_ary[0] = &cblk_complete_io;
	    complete_items = 1;

	    rc = cblk_listio(blk_data->chunk_id,cblk_issue_ary,1,NULL,0,NULL,0,cblk_complete_ary,&complete_items,timeout,0);


	    if (rc) {
		fprintf(stderr,"cblk_listio write failed for  lba = 0x%" PRIx64 ", rc = %d, errno = %d\n",blk_number,rc,errno);
		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);

	    } else {


		if (verbose_flag && !thread_flag) {

		    fprintf(stderr,"Calling cblk_listio poll for write ...\n");

		}
		
		while (TRUE) {

		    complete_items = 1;
		    rc = cblk_listio(blk_data->chunk_id,NULL,0,NULL,0,cblk_issue_ary,1,cblk_complete_ary,&complete_items,timeout,0);


		    

		    if (rc) {
		
			fprintf(stderr,"cblk_listio poll for write completed (failed write) for  rc = %d, errno = %d\n",rc,errno);
			
			exit_loop = TRUE;

		    } else {

			if (cblk_issue_io.stat.status == CBLK_ARW_STATUS_PENDING) {

			    fprintf(stderr,"cblk_listio poll for write completed: command still active  rc = %d, errno = %d\n",rc,errno);
			    usleep(300);
			    continue;
			} else {
			    
			    if (verbose_flag && !thread_flag) {

				fprintf(stderr,"write listio completed with status = %d, errno = %d\n",
					cblk_issue_io.stat.status,cblk_complete_io.stat.fail_errno);

			    }
			}
		    }

		    break;

		} /* while */

	    }


	    if (exit_loop) {

		break;
	    }


	    if (!thread_flag) {

		printf("write completed with rc = %d\n",rc);
	    }

	    if (verbose_flag && !thread_flag) {

		fprintf(stderr,"Calling cblk_listio read for lba = 0x%"PRIX64" ...\n",blk_number);

	    }


	    bzero(&cblk_issue_io,sizeof(cblk_issue_io));

	    cblk_issue_io.request_type = CBLK_IO_TYPE_READ;
	    cblk_issue_io.buf = data_buf;
	    cblk_issue_io.lba = blk_number,
	    cblk_issue_io.nblocks = 1;

	    
	    bzero(&cblk_complete_io,sizeof(cblk_complete_io));

	    cblk_issue_ary[0] = &cblk_issue_io;
	    cblk_complete_ary[0] = &cblk_complete_io;
	    complete_items = 1;

	    
	    
	    rc = cblk_listio(blk_data->chunk_id,cblk_issue_ary,1,NULL,0,NULL,0,cblk_complete_ary,&complete_items,timeout,0);


	    if (rc > 0) {
		if (!thread_flag)  {
		    fprintf(stderr,"cblk_listio read succeeded for  lba = 0x%" PRIx64 " , rc = %d, errno = %d\n",blk_number,rc,errno);


		    printf("Async read data completed ...\n");
		}

	    } else if (rc < 0) {
		fprintf(stderr,"cblk_listio read failed for  lba = 0x%" PRIx64 ", rc = %d, errno = %d\n",blk_number,rc,errno);
		free(comp_data_buf);
		
		free(data_buf);
		return (ret_code);

	    } else {


		if (verbose_flag && !thread_flag) {

		    fprintf(stderr,"Calling cblk_listio poll ...\n");

		}
		
		while (TRUE) {

		    complete_items = 1;
		    rc = cblk_listio(blk_data->chunk_id,NULL,0,NULL,0,cblk_issue_ary,1,cblk_complete_ary,&complete_items,timeout,0);



		    if (rc) {
		
			fprintf(stderr,"cblk_listio poll for read completed (failed write) for  rc = %d, errno = %d\n",rc,errno);
			
			exit_loop = TRUE;

		    } else {

			if (cblk_issue_io.stat.status == CBLK_ARW_STATUS_PENDING) {

			    fprintf(stderr,"cblk_listio poll for read completed: command still active  rc = %d, errno = %d\n",rc,errno);
			    usleep(300);
			    continue;
			} else {
			    
			    if (verbose_flag && !thread_flag) {

				fprintf(stderr,"read listio completed with status = %d, errno = %d\n",
					cblk_issue_io.stat.status,cblk_complete_io.stat.fail_errno);

			    }
			}
		    }


		    break;

		} /* while */

	    }

	    if (exit_loop) {

		break;
	    }
	    if (!thread_flag) {
		printf("Read completed with rc = %d\n",rc);
	    }

	    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

	    if (rc) {

		pthread_mutex_lock(&completion_lock);

		fprintf(stderr,"Memcmp failed with rc = 0x%x, for blk_number = 0x%"PRIX64"\n",rc,blk_number);

		printf("Written data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(data_buf,DATA_BUF_SIZE);
		printf("**********************************************************\n\n");
		printf("read data for blk_number = 0x%"PRIX64":\n",blk_number);
		dumppage(comp_data_buf,DATA_BUF_SIZE);

		printf("**********************************************************\n\n");
		pthread_mutex_unlock(&completion_lock);

		rc = cblk_read(blk_data->chunk_id,data_buf,blk_number,1,0);

		if (rc == 1) {
		    
		    rc = memcmp(data_buf,comp_data_buf,DATA_BUF_SIZE);

		    pthread_mutex_lock(&completion_lock);
		    if (rc) {



			fprintf(stderr,"Memcmp for re-read failed\nDump of re-read data for blk_number = 0x%"PRIX64"\n",blk_number);
		    
			dumppage(data_buf,DATA_BUF_SIZE);
		    } else {
			
			fprintf(stderr,"Memcmp for re-read successful for blk_number = 0x%"PRIX64"\n",blk_number);
		    }
		    
		    pthread_mutex_unlock(&completion_lock);
		}
		
	    } else if ((verbose_flag) && (!thread_flag)) {
		printf("Memcmp succeeded for blk_number = 0x%"PRIX64"\n",blk_number);
	    }

	    
	    free(comp_data_buf);
	    break;

	default:
			   
	    fprintf(stderr,"Invalid local_cmd_type = %d\n",local_cmd_type);
			   i = num_loops;
	} /* switch */


	if (inc_flag) {

	    blk_number++;
	}	


/* ??
        if (rc) {

            break;
        }
*/
    }

    if (each_thread_vlun_flag) {


	/*
	 * If we are using a virtual lun per thread
	 * then close it now.
	 */

		
	if (verbose_flag && !thread_flag) {
	    fprintf(stderr,"Calling cblk_close ...\n");
	}
    
	rc = cblk_close(blk_data->chunk_id,0);

	if (rc) {

	    fprintf(stderr,"Close of chunk_id %d failed with errno = %d\n",blk_data->chunk_id,errno);
	}

    }

    free(data_buf);

    return (ret_code);
}

int main (int argc, char **argv)
{
    int rc = 0;       /* Return code                 */
    int rc2 = 0;      /* Return code                 */
    uint64_t file_len = 0;
    chunk_id_t chunk_id = 0;
    chunk_id_t alt_chunk_id = 0;
    int flags = 0;
    int i;
    pid_t pid;
    void            *status;
    chunk_ext_arg_t ext = 0;


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
	

    rc = cblk_init(NULL,0);
    if (rc) {

	fprintf(stderr,"cblk_init failed with errno = %d and rc = %d\n",
		errno,rc);
    }


    if (!each_thread_vlun_flag) {

	if (virtual_lun_flag) {

	    flags = CBLK_OPN_VIRT_LUN;
	}

	if (no_bg_thread_flag) {

	    flags |= CBLK_OPN_NO_INTRP_THREADS;
	}

	if (shared_contxt_flag) {

	    flags |= CBLK_OPN_SHARE_CTXT;
	}


	if (mpio_flag) {

	    flags |= CBLK_OPN_MPIO_FO;
	}
#ifdef _AIX

	if (reserve_flag) {

	    flags |= CBLK_OPN_RESERVE;
	}

	if (break_reserve_flag) {

	    flags |= CBLK_OPN_FORCED_RESERVE;
	}


#endif

	for (i = 0; i < num_opens; i++) {
	    if (verbose_flag) {
		fprintf(stderr,"Calling cblk_open ...\n");
	    }
	    chunk_id = cblk_open(device_name,pool_size,O_RDWR,ext,flags);
	}

	if (chunk_id == NULL_CHUNK_ID) {

	    fprintf(stderr,"Open of %s failed with errno = %d\n",device_name,errno);
	    cblk_term(NULL,0);
	    return -1;
	}


	blk_thread[0].chunk_id = chunk_id;

	if (shared_contxt_flag) {
	    alt_chunk_id = cblk_open(alt_device_name,pool_size,O_RDWR,ext,flags);
	    if (alt_chunk_id == NULL_CHUNK_ID) {
		
		fprintf(stderr,"Open of %s failed with errno = %d\n",alt_device_name,errno);
		cblk_close(chunk_id,0);
		cblk_term(NULL,0);
		return -1;
	    }
	    alt_blk_thread[0].chunk_id = alt_chunk_id;
	}


	if (virtual_lun_flag) {

	    rc = cblk_set_size(chunk_id,size,0);

	    if (rc) {
		fprintf(stderr,"cblk_set_size failed with rc = %d, and errno = %d \n",rc,errno);

		rc = cblk_close(chunk_id,0);

		if (rc) {

		    fprintf(stderr,"Close of %s failed with errno = %d\n",device_name,errno);
		}

		if (shared_contxt_flag) {
		    rc = cblk_close(alt_chunk_id,0);

		    if (rc) {

			fprintf(stderr,"Close of %s failed with errno = %d\n",alt_device_name,errno);
		    }
		}


		cblk_term(NULL,0);
		return -1;

	    }

	    if (shared_contxt_flag) {
		
		rc = cblk_set_size(alt_chunk_id,size,0);
		
		if (rc) {
		    fprintf(stderr,"cblk_set_size failed with rc = %d, and errno = %d \n",rc,errno);
		    
		    rc = cblk_close(chunk_id,0);
		    
		    if (rc) {
			
			fprintf(stderr,"Close of %s failed with errno = %d\n",device_name,errno);
		    }
		    
		    rc = cblk_close(alt_chunk_id,0);
		    
		    if (rc) {
			
			fprintf(stderr,"Close of %s failed with errno = %d\n",alt_device_name,errno);
		    }
		    
		    
		    cblk_term(NULL,0);
		    return -1;
		    
		}
		
	    }

	}


    } /* !each_thread_vlun_flag */

    if ((thread_flag) &&
        (num_threads > 1)) {

        /*
         * Create all threads here
         */


        for (i=0; i< num_threads; i++) {

	    blk_thread[i].chunk_id = chunk_id;

            rc = pthread_create(&blk_thread[i].ptid,NULL,run_loop,(void *)&blk_thread[i]);

            if (rc) {

                fprintf(stderr, "pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
                        i, rc,errno);

		/*
		 * If we fail to create this thread and we are sharing contexts,
		 * then do not create the shared context associated with this.
		 */
		continue;

            }

	    if (shared_contxt_flag) {
		
		
		alt_blk_thread[i].chunk_id = alt_chunk_id;
		
		rc = pthread_create(&alt_blk_thread[i].ptid,NULL,run_loop,(void *)&alt_blk_thread[i]);
		
		if (rc) {
		    
		    fprintf(stderr, "pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
			    i, rc,errno);
		    
		}
		
	    }


        }


        /* 
         * Wait for all threads to complete
         */


        errno = 0;

        for (i=0; i< num_threads; i++) {

            rc = pthread_join(blk_thread[i].ptid,&status);

            if (rc) {

                fprintf(stderr, "pthread_join failed for %d rc 0x%x, errno = 0x%x\n",
                        i, rc,errno);

            }

	    if (shared_contxt_flag) {
		
		rc = pthread_join(alt_blk_thread[i].ptid,&status);
		
		if (rc) {
		    
		    fprintf(stderr, "pthread_join failed for %d rc 0x%x, errno = 0x%x\n",
			    i, rc,errno);
		    
		}
		
		
	    }
        }

        
        fprintf(stderr, "All threads exited\n");

    } else {
        run_loop(&blk_thread[0]);

	
	if (shared_contxt_flag) {
	    run_loop(&alt_blk_thread[0]);
	}
    }
    
    if (fork_rerun_flag) {

	/*
	 * We need to rerun the same operations again on the
	 * child after a fork.
	 */

	if ((pid = fork()) < 0) {  /* fork failed */
	    perror("Fork failed \n");

	}                 
	else if (pid > 0)  {               /* parents fork call */

	    if (verbose_flag && !thread_flag) {
		fprintf(stderr,"Fork succeeded \n");
	    }

	    /*
	     * Let parent sleep long enough for child to start up.
	     */

	    sleep(2);


	} else {
	    
	    /*
	     * Only the child process will reach this point
	     * ie fork = 0.
	     */
	    

	    
	    if (virtual_lun_flag) {

		/*
		 * Clone after fork is only supported for
		 * virtual luns. Physical luns do not need this.
		 */

		if (verbose_flag && !thread_flag) {

		    fprintf(stderr,"Calling cblk_clone_after_fork...\n");

		}
		rc = cblk_clone_after_fork(chunk_id,O_RDWR,0);

	    
		if (verbose_flag && !thread_flag) {
		    printf("clone completed with rc = %d, errno = %d\n",rc,errno);
		}

		if (rc) {


		    fprintf(stderr,"cblk_chunk_clone failed with rc = 0x%x errno = %d\n",rc, errno);


		}


		if ((!rc) && (shared_contxt_flag))  {
		
		
		    rc2 = cblk_clone_after_fork(alt_chunk_id,O_RDWR,0);
		
		
		    if (verbose_flag && !thread_flag) {
			printf("clone completed with rc = %d, errno = %d\n",rc,errno);
		    }
		
		    if (rc2) {
		    
		    
			fprintf(stderr,"cblk_chunk_clone failed with rc = 0x%x errno = %d\n",rc, errno);
		    
		    
		    }
		}

	    } /* if (virtual_lun_flag) */


	    if ((!rc) && (!rc2) &&
		(thread_flag) &&
		(num_threads > 1)) {

		/*
		 * Create all threads here
		 */


		for (i=0; i< num_threads; i++) {

		    blk_thread[i].chunk_id = chunk_id;

		    rc = pthread_create(&blk_thread[i].ptid,NULL,run_loop,(void *)&blk_thread[i]);

		    if (rc) {

			fprintf(stderr, "pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
				i, rc,errno);


			/*
			 * If we fail to create this thread and we are sharing contexts,
			 * then do not create the shared context associated with this.
			 */
			continue;

		    }

		    if (shared_contxt_flag) {
			
			
			alt_blk_thread[i].chunk_id = alt_chunk_id;
			
			rc = pthread_create(&alt_blk_thread[i].ptid,NULL,run_loop,(void *)&alt_blk_thread[i]);
			
			if (rc) {
			    
			    fprintf(stderr, "pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
				    i, rc,errno);
			    
			    
			}
			
		    }
		}


		/* 
		 * Wait for all threads to complete
		 */


		errno = 0;

		for (i=0; i< num_threads; i++) {

		    rc = pthread_join(blk_thread[i].ptid,&status);

		    if (rc) {

			fprintf(stderr, "pthread_join failed for %d rc 0x%x, errno = 0x%x\n",
				i, rc,errno);
			
			
		    }

		    if (shared_contxt_flag) {
			
			rc = pthread_join(alt_blk_thread[i].ptid,&status);
			
			if (rc) {
			    
			    fprintf(stderr, "pthread_join failed for %d rc 0x%x, errno = 0x%x\n",
				    i, rc,errno);
			    
			    
			}
			
		    }
		}

        
		fprintf(stderr, "All threads exited after fork\n");

	    } else if (!rc)  {
		run_loop(&blk_thread[0]);
		
		if (shared_contxt_flag) {
		    run_loop(&alt_blk_thread[0]);
		}
			
		
	    }
	}
    
    }

    if (!each_thread_vlun_flag) {
	if (verbose_flag) {
	    fprintf(stderr,"Calling cblk_close ...\n");
	}
	rc = cblk_close(chunk_id,0);

	if (rc) {

	    fprintf(stderr,"Close of %s failed with errno = %d\n",device_name,errno);
	}

	if (shared_contxt_flag) {
	    rc = cblk_close(alt_chunk_id,0);

	    if (rc) {

		fprintf(stderr,"Close of %s failed with errno = %d\n",alt_device_name,errno);
	    }
	}

    }


    cblk_term(NULL,0);

    return 0;

}
