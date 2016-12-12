/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/pvtestauto.c $                                       */
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


/*
 *                           AUTOMATED PVTEST
 *
 *   This test program is executed from the command line with:
 *
 *     "pvtestauto special_file,p|v,low_lba,high_lba,num_loops,s|r,d|n|r,
 *     num_blocks, block_size,  V"
 *
 *   The program performs a read/write/read test on the range of blocks
 *   passed in the paramater list.  The paramaters and their meanings are:
 *
 *        1)  spec_file:  the special file name
 *        2)  p:          physical lun
 *            v:          virtual lun
 *        3)  low_lba:    the lowest block number to be tested (in hex)
 *        4)  high_lba:   the highest block number to be tested (in hex)
 *        5)  num_loops:  the number of times to perform the sequential test,
 *                        if 0, test will run forever till kiled.
 *        6)  s:          perform the read/write/read's sequentially on
 *                        the given range of blocks.
 *            r:          perform the read/write/read's on random blocks
 *                        within the given range of blocks.
 *        7)  d:          (destructive)  the program writes , read and compares
 *                        data to the disk.  
 *            r:          (non-destructive) the data on the disk will read only
 *                        
 *            w:          (destructive) Write only test... 
 *
 *        8)  num_blocks  the number of blocks to be read/written per
 *                        read/write (in decimal). (current default 1)
 *
 *	  9)  block_size   This is only used when using blocks sizes other
 *                         Specify the block size in bytes.(current default 4096
 *
 *	  10) num_threads  This is the number of threads to run.
 *
 *        11) V            Verbose
 *            S            print I/O statistics.
 *
 *        12) F            Failover tests (for AIX only).
 *
 *
 */


#include "pvtestauto.h"

extern	int	errno;
int version_no = 1;
pthread_t pv_thread[MAX_NUM_THREADS];
pv_thread_data_t pv_thread_data;
int verbose = 0;
int statflg = 0;
int flags = 0;
int failover = 0;
pv_thread_data_t pv_data;
#define BUFSIZE 4096
pthread_mutex_t  completion_lock;

void dump_iostats(chunk_stats_t *stats, int *ii, int th);



/*-------------------------------------------------------------------------- 
 *
 * NAME: usage error
 *                  
 * FUNCTION: Displays usage
 *                                                                         
 *
 * (NOTES:)
 *
 *
 * (DATA STRUCTURES:)  
 *
 *                                                          
 * INPUTS:                                                  
 *
 * 
 * INTERNAL PROCEDURES CALLED:
 * 
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *   
 *
 * (RECOVERY OPERATION:)
 *
 * RETURNS:   
 *----------------------------------------------------------------------------
 */    

void usage(void)
{
	printf( "\n pvtestauto Version - %d \n\n",version_no);
	printf( "\nUsage:\n");
	printf( "\nFormat of command:\npvtestauto spec_file p|v low_lba high_lba #loops s|r d|r|w #blocks block_size threads [V|S] "); 
#ifdef _AIX
        printf("[F]\n");
#else
        printf("\n");
#endif
	printf( "\n1  spec_file:  the special file name");
	printf( "\n2  p:          Physical lun");
        printf( "\n   v:          Virtual  lun");
	printf( "\n3  low_lba:    the lowest block number to be tested (in hex)");
	printf( "\n4  high_lba:   the highest block number to be tested (in hex)");
	printf( "\n5  num_loops:  the number of times to perform the tests,");
	printf( "\n               if 0, test will run forever till killed.");
	printf( "\n6  s:          perform the read/write/read's sequentially on");
	printf( "\n               the given range of blocks.");
	printf( "\n   r:          perform the read/write/read's on random blocks");
	printf( "\n               within the given range of blocks.");
	printf( "\n7  d:          (destructive)  the program changes the data read");
	printf( "\n               before it is written back to the disk.");
	printf( "\n   r:          (non-destructive) the data on the disk will not");
	printf( "\n               be changed.Read Only test.");
	printf( "\n   w:          (destructive) the data on the disk will be");
	printf( "\n               changed. Write Only test.");
	printf( "\n8  num_blocks  the number of blocks to be read/written per");
	printf( "\n               read/write (in decimal).\n");
	printf( "\n9  block_size  Specify the block size for read/write in bytes(in decimal).\n");
	printf( "\n               for Surelock default is 4096.\n");
	printf( "\n10 num_threads number of threads\n");

	printf( "\n11 V:           Verbose \n");
	printf( "\n   S:           print io statistics\n");
	printf( "\n");
#ifdef _AIX
	printf( "\n12 F:           Failover \n");
	printf( "\n");
#endif
        printf( "\nFor Example:\n");
        printf( "\n To stress I/O on /dev/cxl/afu0.0m");
        printf( "\n virtual lun, lba 0x10 thru 0x1000, sequential lba, destructive I/O");
        printf( "\n num_blocks 1, blk size 4096, threads 10 and  loop 100 times.\n");
        printf( "\n Execute following command:");
        printf( "\n./pvtestauto /dev/cxl/afu0.0m v 10 1000 100 s d 1 4096 10\n");
	return;
}
/*-------------------------------------------------------------------------- 
 *
 * NAME: gethexnum
 *                  
 * FUNCTION: 
 *                                                                         
 *
 * (NOTES:)
 *
 *
 * (DATA STRUCTURES:)  
 *
 *                                                          
 * INPUTS:                                                  
 *
 * 
 * INTERNAL PROCEDURES CALLED:
 * 
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *   
 *
 * (RECOVERY OPERATION:)
 *
 * RETURNS:   
 *----------------------------------------------------------------------------
 */    
long long gethexnum(char *strg)
{
        long long r = 0;
        char ic;

        ic = *strg++;
        while (ic != '\0') {
            if ((ic < '0' || ic > '9') && (ic < 'a' || ic > 'f'))
                return(-1);
            else {
                r *= 16;
                r += (ic >= '0' && ic <= '9') ? (ic - '0') : (ic - 'a' + 10);
            }
            ic = *(strg++);
        }
        return(r);
}

/*-------------------------------------------------------------------------- 
 *
 * NAME: getdecnum
 *                  
 * FUNCTION: 
 *                                                                         
 *
 * (NOTES:)
 *
 *
 * (DATA STRUCTURES:)  
 *
 *                                                          
 * INPUTS:                                                  
 *
 * 
 * INTERNAL PROCEDURES CALLED:
 * 
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *   
 *
 * (RECOVERY OPERATION:)
 *
 * RETURNS:   
 *----------------------------------------------------------------------------
 */    
int getdecnum(char *strg)
{
        int r = 0;
        char ic;

        ic = *strg++;
        while (ic != '\0') {
            if (ic < '0' || ic > '9')
                return(-1);
            else {
                r *= 10;
                r += ic - '0';
            }
            ic = *(strg++);
        }
        return(r);
}





/*-------------------------------------------------------------------------- 
 *
 * NAME: parse_inputs
 *                  
 * FUNCTION: Initializes the dev
 *                                                                         
 *
 * (NOTES:)
 *
 *
 * (DATA STRUCTURES:)  
 *
 *                                                          
 * INPUTS:                                                  
 *
 * 
 * INTERNAL PROCEDURES CALLED:
 * 
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *   
 *
 * (RECOVERY OPERATION:)
 *
 * RETURNS:   
 *----------------------------------------------------------------------------
 */    

int parse_inputs(int argc, char **argv) 
{
    int error = FALSE;
    show_errors = TRUE;
    bzero (&pv_data, sizeof(pv_thread_data_t));

    if (strlen(argv[1]) > (PATH_MAX -1) ) {
        fprintf(stderr, "Path too long %s\n",argv[1]);    
        return (TRUE);
    }
    strcpy(dev_name,argv[1]);

    if (strcmp("v",argv[2]) == 0)
        virtual_flag = TRUE;
    else if (strcmp("p",argv[2]) == 0)
        virtual_flag = FALSE;
    else {
        fprintf(stderr,"lun must be v|p, It is %s\n",argv[2]);    
        return (TRUE);
    }

    low_lba = gethexnum(argv[3]);
    pv_data.low_lba = low_lba;
    if (low_lba == -1){
        fprintf(stderr,"Invalid low lba 0x%llx\n",low_lba);    
        return (TRUE);
    }
    high_lba = gethexnum(argv[4]);
    pv_data.high_lba = high_lba;
    if (high_lba == -1){
        fprintf(stderr,"Invalid high lba 0x%llx\n",high_lba);    
        return (TRUE);
    }
    if (high_lba < low_lba) {
        fprintf(stderr,"Invalid high lba 0x%llx, must be greater than low\n",high_lba);    
        return (TRUE);
    }

    num_loops = getdecnum(argv[5]);
    if (num_loops == -1){
        fprintf(stderr,"Invalid num_loops %d\n",num_loops);    
        return (TRUE);
    } else if (!num_loops) {
        loop_forever = 1;
    }

    if (strcmp("s",argv[6]) == 0)
        seq_or_rand = 1;
    else if (strcmp("r",argv[6]) == 0)
        seq_or_rand = 0;
    else {
        fprintf(stderr,"Invalid seq_or_rand arg must be s | r \n");    
        return (TRUE);
    }

    if (strcmp("d",argv[7]) == 0)
        destr = 1;
    else if (strcmp("w",argv[7]) == 0)
        destr = 2;
    else if (strcmp("r",argv[7]) == 0)
        destr = 3;
    else  {
        fprintf(stderr,"Invalid destr, read or write arg must be d | r | w \n");    
        return (TRUE);
    }
    /* Number of blocks per read/write is default 1 for Surelock */
    num_blocks = getdecnum(argv[8]);
    if (virtual_flag && (num_blocks != 1)) { 
        fprintf(stderr,"\nInvalid Number of blocks = %d, should be 1\n",
                num_blocks);
        fprintf(stderr,"Setting to Default num_blocks = 1\n");
        num_blocks = 1;
    } else {
        /* physical lun can support upto 16M (4096*4096) */
        if (num_blocks > 4096) {
            fprintf(stderr,"\nInvalid Number of blocks = %d, should _not_ be greater than 512\n",num_blocks);
            fprintf(stderr,"Setting to xfersize num_blocks = 512 blocks\n");
            num_blocks = 4096;
        }
    }

    blk_size = getdecnum(argv[9]);
    /* TODO for now donot accept blocksize other than 4096 */
    if (blk_size != BLK_SIZE) {
        printf("\nInvalid block size = %d, should be 4096\n",blk_size);
        fprintf(stderr,"Setting to Default blocks size to 4096\n");
        blk_size = BLK_SIZE;
    }
    num_threads = getdecnum(argv[10]);
    if (num_threads < 0) {
        fprintf(stderr, "Thread count must be specify, Setting to default 0\n");
        thread_flag = 0;
    }

    if (num_threads > 0) {
        if ((pv_data.high_lba - pv_data.low_lba) < num_threads) {
            fprintf(stderr, "Invalid lba range, It needs to be larger than number of threads\n");
            fprintf(stderr, "Either increase the lba range or decrease the number of threads\n");
            return(TRUE);
        }

        if (num_threads > MAX_NUM_THREADS) {
            num_threads = MAX_NUM_THREADS;
            printf("Number of threads exceed maximum allowed, setting to default max %d\n",num_threads);
        }
        thread_flag = 1;
    }

    verbose = 0;
    if (argc > 11) {
        if (strcmp("V",argv[11]) == 0)
            verbose = 1;
        else if (strcmp("S",argv[11]) == 0)
            statflg = 1;
    }
#ifdef _AIX
    if (argc > 12) {
        if ((strcmp("F",argv[12]) == 0) &&
                !virtual_flag) {
            failover = 1;
        } else {
            failover = 0;
            printf("\nfailover disabled , supported only in physical mode\n");
        }
    }
#endif

    return (error);


}

/*----------------------------------------------------------------------- 
 *
 * NAME: Main
 *                  
 * FUNCTION: 
 *                                                                         
 *
 * (NOTES:)
 *
 *
 * (DATA STRUCTURES:)  
 *
 *                                                          
 * INPUTS:                                                  
 *
 * 
 * INTERNAL PROCEDURES CALLED:
 * 
 *                              
 * EXTERNAL PROCEDURES CALLED:
 *   
 *
 * (RECOVERY OPERATION:)
 *
 * RETURNS:   
 *------------------------------------------------------------
 */   
 
int main (int argc, char* argv[]) 
{
    int error = FALSE;
    int rc = 0;
    loop_forever = 0;

    rc = cblk_init(NULL,0);

    if (rc) {

        fprintf(stderr,"cblk_init failed with rc = %d, and errno = %d\n",
                rc,errno);
        return (1);
    }

    if (verbose)
        printf( "\nWelcome to pvtestauto \n");
    if ((argc < 11) || (argc > 13)) {
        fprintf(stderr,"num args less than req\n");
        error = TRUE;
    } else {
        rc = parse_inputs(argc,argv);
        if (!rc) {
            if (verbose)
                fprintf(stderr,"Initializing  %s, lba 0x%llx - 0x%llx, threads = %d, loops %d\n", dev_name, low_lba, high_lba, num_threads, num_loops);
            if (dev_init() == 0) {
                rc = run_pv_test(&rc,&error);
                if (rc) {
                    fprintf(stderr, "\n\nTest Failed on Loop = %d\n\n",
                            num_loops);
                } else {
                    if (verbose)
                        fprintf(stderr, "\n\nTest Completed. Loop Count = %d\n\n",num_loops);
                }		
                if (verbose)
                    fprintf(stderr,"Calling cblk_close ...\n");
                rc = cblk_close(pv_data.chunk_id,0);
                if (rc)
                    fprintf(stderr,"Close of %s failed with errno = %d\n",dev_name,errno);
            }
        } else {
            error = TRUE;
        }
    }
    if (error ) {
        usage();
        cblk_term(NULL,0);
        return (1);
    }

    cblk_term(NULL,0);
    return (rc);
}


int dev_init()
{
    int rc = 0;
    int ret_code = 0;
    size_t lba_range = 0;
    size_t chunk_sz = 0;
    int    flags = 0;
    int  open_mode = O_RDWR;
    chunk_ext_arg_t ext = 0;
    size_t lun_sz = 0;

    lba_range = high_lba+1;
    if (virtual_flag)
        flags = CBLK_OPN_VIRT_LUN;
    else
        flags =0;
#ifdef _AIX
    if (failover)
        flags |= CBLK_OPN_MPIO_FO ;
#endif

    if ((pv_data.chunk_id = cblk_open(dev_name,0,open_mode,ext,flags))
            == NULL_CHUNK_ID) {
        fprintf(stderr,"Open of %s failed with errno = %d\n",dev_name,errno);
        errno_process (errno);	
        /*
         * Save off error code
         */ 
        ret_code = pv_data.chunk_id;
        pv_data.chunk_id = 0;
    } else {
        /* get size of the lun associated with this chunk */
        rc = cblk_get_lun_size(pv_data.chunk_id, &lun_sz, 0);
        if (!rc) {
            if (lun_sz < lba_range) {
                rc = -1;
                fprintf(stderr,"requested block size is more than lun size %zu\n",lun_sz);
                return(-1);
            }
        }
        if (flags & CBLK_OPN_VIRT_LUN) {
            rc =cblk_set_size(pv_data.chunk_id,(size_t)lba_range, 0);
            if (verbose)
                fprintf(stderr,"\nSet chunk size rc = %d, errno= %d \n",rc, errno);
            if (!rc) {
                /* Verify it was set correctly */
                rc = cblk_get_size (pv_data.chunk_id,&chunk_sz, 0);
                if (verbose)
                    fprintf(stderr,"\nGet chunk size  rc = %d, errno = %d, size (blocks)  = %zu\n",rc, errno, chunk_sz);
                if (!rc) {
                    if (chunk_sz != lba_range){
                        fprintf(stderr," failed to set correct size expected %zu, received %zu\n", lba_range,chunk_sz);
                    } 
                } 
            }
        }
    }
    if (ret_code || rc) {
        fprintf(stderr,"Open %s... Failed\n",dev_name);
        errno_process();

        /*
         * Save off error code
         */ 
        ret_code = pv_data.chunk_id;

    } else {
        /*
         * If open was successful then
         * increment open count.
         */ 
        open_cnt++;
        if (verbose)
            fprintf(stderr,"Open of %s successful\n",dev_name);
    }
    return(ret_code || rc);
}

/*
 * ----------------------------------------------------------------------------
 * NAME: errno_process
 *                  
 * FUNCTION:  Parse error and print out its define from errno.h
 *                                                 
 *                                                                         
 *
 * CALLED BY:
 *
 * NOTES:  This routine outputs to stdout instead of stderr, because
 *         when using stderr it causes a crash (memory leak)
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
void errno_process(int err)
{
    switch (err) {
        case EACCES:
            fprintf(stderr, "EACCES");
            break;
        case EBUSY:
            fprintf(stderr, "EBUSY");
            break;
        case EFAULT:
            fprintf(stderr, "EFAULT");
            break;
        case EINVAL:
            fprintf(stderr, "EINVAL");
            break;
        case EIO:
            fprintf(stderr, "EIO");
            break;
        case ENOMEM:
            fprintf(stderr, "ENOMEM");
            break;
        case ENODEV:
            fprintf(stderr, "ENODEV");
            break;
        case ENXIO:
            fprintf(stderr, "ENXIO");
            break;
        case EPERM:
            fprintf(stderr, "EPERM");
            break;
        case ETIMEDOUT:
            fprintf(stderr, "ETIMEDOUT");
            break;
        case EBADF:
            fprintf(stderr, "EBADF");
            break;
        case EAGAIN:
            fprintf(stderr, "EAGAIN");
            break;
        case EDEADLK:
            fprintf(stderr, "EDEADLK");
            break;
        case EINTR:
            fprintf(stderr, "EINTR");
            break;
        case EMFILE:
            fprintf(stderr, "EMFILE");
            break;
        case ECHILD:
            fprintf(stderr, "ECHILD");
            break;
        case ESTALE:
            fprintf(stderr, "ESTALE");
            break;
        case EINPROGRESS:
            fprintf(stderr, "EINPROGRESS");
            break;
        default:
            fprintf(stderr, "Unknown 0x%x",err);
    }
    fprintf(stderr, "\n");

    return;
}



void *run_pvthread_loop(void *data)
{
    int rc = 0;
    int rtag,wtag;
    int i;
    pv_thread_data_t *pvt_data = data;
    uint32_t t = 0;
    void *data_buf = NULL;
    void *comp_data_buf = NULL;
    uint64_t status;
    int cmd_type;
    int fd;
    int arflag = 0;
    uint64_t lba;
    uint64_t t_low_lba;
    uint64_t t_high_lba;
    uint64_t t_lbasz;
    int t_loops = 0;
    void *ret_code = NULL;
    chunk_stats_t stats;
    int stat_title=0;

    errno = 0;

    pthread_mutex_lock(&completion_lock);

    t = thread_count++;

    /*
     * Each thread is using a different
     * block number range.
     */

    t_low_lba = pvt_data->t_data[t].t_low_lba;
    t_lbasz = pvt_data->t_lbasz;
    t_high_lba = t_low_lba + t_lbasz;
    lba = t_low_lba;
    t_loops = num_loops;


    pthread_mutex_unlock(&completion_lock);

    /*
     * Align data buffer on page boundary.
     */	
    if ( posix_memalign((void *)&data_buf,4096,BUFSIZE*num_blocks)) {

        perror("posix_memalign failed for data buffer");

        pvt_data->t_data[t].ret = 0;
        pvt_data->t_data[t].errcode = errno;
        return(ret_code);
    }

    if ( posix_memalign((void *)&comp_data_buf,4096,BUFSIZE*num_blocks)) {

        perror("posix_memalign failed for data buffer");
        pvt_data->t_data[t].ret= 0;
        pvt_data->t_data[t].errcode = errno;
        free(data_buf);
        return(ret_code);

    }

    for (i =0; (loop_forever || (i<t_loops)) ; i++ ) {

        if (destr == 1) {
            if (i % 2) {
                cmd_type = PV_RW_AWAR;
            } else {
                cmd_type = PV_RW_COMP;
            }
        } else if (destr == 2) {
            cmd_type = PV_AWRITE_ONLY;
        } else 
            cmd_type = PV_AREAD_ONLY;

        for (lba = t_low_lba; lba < t_high_lba ; ) {

            switch (cmd_type) {

                case PV_AWRITE_ONLY:

                    rc = cblk_awrite(pvt_data->chunk_id,data_buf,lba,num_blocks,&rtag,NULL,0);

                    if (rc < 0) {
                        pvt_data->t_data[t].ret= rc;
                        pvt_data->t_data[t].errcode = errno;
                        printf("Async write Failed, errno = %d\n", errno);
                        free(comp_data_buf);
                        free(data_buf);
                        return(ret_code);
                    } 

                    arflag = 0;
                    while (TRUE) {

                        rc = cblk_aresult(pvt_data->chunk_id,&rtag, &status,arflag);
                        if (rc < 0) {
                            pvt_data->t_data[t].ret= rc;
                            pvt_data->t_data[t].errcode = errno;
                            fprintf(stderr,"Async result write Failed, errno = %d\n",errno);
                            free(comp_data_buf);
                            free(data_buf);
                            return(ret_code);
                        }
                        if (rc > 0) {
                            if (verbose)
                                fprintf(stderr,"Async result write completed tag =%d\n",rtag);
                            break;
                        }

                    } /* while */

                    break;

                case PV_AREAD_ONLY:

                    rc = cblk_aread(pvt_data->chunk_id,data_buf,lba,num_blocks,&rtag,NULL,0);

                    if (rc < 0) {
                        pvt_data->t_data[t].ret= rc;
                        pvt_data->t_data[t].errcode = errno;
                        printf("Async read Failed, errno = %d\n", errno);
                        free(comp_data_buf);
                        free(data_buf);
                        return(ret_code);
                    } 

                    arflag = 0;
                    while (TRUE) {

                        rc = cblk_aresult(pvt_data->chunk_id,&rtag, &status,arflag);
                        if (rc < 0) {
                            pvt_data->t_data[t].ret= rc;
                            pvt_data->t_data[t].errcode = errno;
                            fprintf(stderr,"Async result read Failed, errno = %d\n",errno);
                            free(comp_data_buf);
                            free(data_buf);
                            return(ret_code);
                        }
                        if (rc > 0) {
                            if (verbose)
                                fprintf(stderr,"Async result read completed tag =%d\n",rtag);
                            break;
                        }

                    } /* while */


                    break;

                case PV_RW_COMP:

                    /* 
                     * Perform write then read comparision test
                     */

                    fd = open ("/dev/urandom", O_RDONLY);
                    rc = read (fd, comp_data_buf, BUFSIZE);
                    close (fd);

                    rc = cblk_write(pvt_data->chunk_id,comp_data_buf,lba,num_blocks,0);

                    if (rc != num_blocks) {
                        pvt_data->t_data[t].ret = rc;
                        pvt_data->t_data[t].errcode = errno;
                        free(comp_data_buf);
                        free(data_buf);
                        fprintf(stderr,"Write failed rc = %d, errno = %d\n",rc, errno);
                        return(ret_code);
                    }
                    if (verbose) {
                        if (thread_flag)
                            fprintf(stderr,"Thread %d ",t);
                        fprintf(stderr,"Write complete at lba 0x%lx\n",lba);
                    }
                    rc = cblk_read(pvt_data->chunk_id,data_buf,lba,num_blocks,0);

                    if (rc != num_blocks) {

                        pvt_data->t_data[t].ret = rc;
                        pvt_data->t_data[t].errcode = errno;
                        free(comp_data_buf);
                        free(data_buf);
                        fprintf(stderr,"Read failed rc = %d, errno = %d\n",rc,errno);
                        return(ret_code);
                    }
                    if (verbose) {
                        if (thread_flag)
                            fprintf(stderr,"Thread %d ",t);
                        fprintf(stderr,"Read complete at lba 0x%lx\n",lba);
                    }

                    rc = memcmp(data_buf,comp_data_buf,BUFSIZE*num_blocks);

                    if (rc) {
                        pvt_data->t_data[t].ret = rc;
                        pvt_data->t_data[t].errcode = errno;
                        pthread_mutex_lock(&completion_lock);
                        if (thread_flag)
                            fprintf(stderr,"Thread %d ",t);
                        fprintf(stderr,"Miscompare at lba 0x%lx\n",lba);
                        fprintf(stderr,"Written data:\n");
                        dumppage(data_buf,BUFSIZE);
                        fprintf(stderr,"**************************************************\n\n");
                        fprintf(stderr,"read data:\n");
                        dumppage(comp_data_buf,BUFSIZE);
                        fprintf(stderr,"**************************************************\n\n");
                        pthread_mutex_unlock(&completion_lock);

                        rc = cblk_read(pvt_data->chunk_id,data_buf,lba,1,0);
                        if (rc == num_blocks) {
                            pthread_mutex_lock(&completion_lock);
                            fprintf(stderr,"Dump of re-read\n");
                            dumppage(data_buf,BUFSIZE);
                            pthread_mutex_unlock(&completion_lock);
                        }

                    } else {
                        if (verbose) {
                            if(thread_flag)
                                fprintf(stderr,"Thread %d",t);
                            fprintf(stderr,"Compare ok at lba 0x%lx\n",lba);
                        }
                    }
                    break;
                case PV_RW_AWAR:

                    /* 
                     * Perform write then read comparision test
                     */

                    fd = open ("/dev/urandom", O_RDONLY);
                    rc = read (fd, comp_data_buf, BUFSIZE);
                    close (fd);

                    rc = cblk_awrite(pvt_data->chunk_id,comp_data_buf,lba,num_blocks,&wtag,NULL,0);

                    if (rc < 0) {
                        pvt_data->t_data[t].ret= rc;
                        pvt_data->t_data[t].errcode = errno;
                        fprintf(stderr,"Async write Failed, errno = %d\n",errno);
                        free(comp_data_buf);
                        free(data_buf);
                        return(ret_code);
                    } 
                    arflag = 0;
                    while (TRUE) {
                        rc = cblk_aresult(pvt_data->chunk_id,&wtag, &status,arflag);
                        if (rc < 0) {
                            pvt_data->t_data[t].ret= rc;
                            pvt_data->t_data[t].errcode = errno;
                            fprintf(stderr,"Async result write Failed, errno = %d\n",errno);
                            free(comp_data_buf);
                            free(data_buf);
                            return(ret_code);
                        }
                        if (rc > 0) {
                            if (verbose)
                                fprintf(stderr,"Async result write completed tag=%d \n",wtag);
                            break;
                        }

                    } /* while */


                    rc = cblk_aread(pvt_data->chunk_id,data_buf,lba,num_blocks,&rtag,NULL,0);

                    if (rc < 0) {
                        pvt_data->t_data[t].ret= rc;
                        pvt_data->t_data[t].errcode = errno;
                        printf("Async read Failed, errno = %d\n", errno);
                        free(comp_data_buf);
                        free(data_buf);
                        return(ret_code);
                    } 

                    arflag = 0;

                    while (TRUE) {

                        rc = cblk_aresult(pvt_data->chunk_id,&rtag, &status,arflag);
                        if (rc < 0) {
                            pvt_data->t_data[t].ret= rc;
                            pvt_data->t_data[t].errcode = errno;
                            fprintf(stderr,"Async result read Failed, errno = %d\n",errno);
                            free(comp_data_buf);
                            free(data_buf);
                            return(ret_code);
                        }
                        if (rc > 0) {
                            if (verbose)
                                fprintf(stderr,"Async result read completed tag =%d\n",rtag);
                            break;
                        }

                    } /* while */



                    rc = memcmp(data_buf,comp_data_buf,BUFSIZE*num_blocks);

                    if (rc) {
                        pvt_data->t_data[t].ret = rc;
                        pvt_data->t_data[t].errcode = errno;
                        pthread_mutex_lock(&completion_lock);
                        if (thread_flag)
                            fprintf(stderr,"Thread %d ",t);
                        fprintf(stderr,"Miscompare at lba 0x%lx\n",lba);
                        fprintf(stderr,"**************************************************\n\n");
                        fprintf(stderr,"async write data:\n");
                        dumppage(data_buf,BUFSIZE);
                        fprintf(stderr,"**************************************************\n\n");
                        fprintf(stderr,"async read data:\n");
                        dumppage(comp_data_buf,BUFSIZE);
                        fprintf(stderr,"**************************************************\n\n");
                        pthread_mutex_unlock(&completion_lock);

                        rc = cblk_read(pvt_data->chunk_id,data_buf,lba,num_blocks,0);

                        if (rc == num_blocks) {
                            fprintf(stderr,"Dump of re-read\n");
                            dumppage(data_buf,BUFSIZE);
                        }

                    } else {
                        if(verbose) {
                            if (thread_flag)
                                fprintf(stderr,"Thread %d ",t);
                            fprintf(stderr,"Async compare ok at lba 0x%lx\n",lba);
                        }
                    }
                    break;
                default:
                    fprintf(stderr,"Invalid cmd_type = %d\n",cmd_type);
                    i = t_loops;
            } /* switch */

            if (seq_or_rand) {
                lba += 1; 
            } else {
                lba = rand() % (t_high_lba - t_low_lba + 1) + t_low_lba;
            }
            if (statflg) {
                pthread_mutex_lock(&completion_lock);
                rc = cblk_get_stats (pvt_data->chunk_id, &stats, 0);
                dump_iostats( &stats,&stat_title, t);
                pthread_mutex_unlock(&completion_lock);
            } 

            if(verbose)
                fprintf(stderr,"\nTesting block no 0x%lx\n",lba);

        }
        if(thread_flag && verbose) 
            fprintf(stderr,"Thread %d ",t);
        if (verbose)
            fprintf(stderr,"Loop Completed %d\n",i);

    } 


    free(data_buf);
    free(comp_data_buf);
    return(ret_code);
}

void dump_iostats(chunk_stats_t *stats, int *stat_title, int th)
{
    if ((*stat_title == 0) && (th == 0)){
        fprintf(stderr,"\nchunk_statistics:\n");
        fprintf(stderr,"*****************\n");
        fprintf(stderr,"num_reads     num_writes     num_areads     num_awrites     num_threads\n");
        *stat_title = 1;
    }
    fprintf(stderr,"%8lx     %8lx     %8lx     %8lx     %8x\r",
            stats->num_reads,stats->num_writes,stats->num_areads,
            stats->num_awrites, th);
}

int run_pv_test(int *ret, int *err)
{


    int rc = 0;           /* Return code                 */
    int i= 0;
    void            *status;

    pv_data.size = 64;

    if (virtual_flag)
        flags = CBLK_OPN_VIRT_LUN;

    num_opens = 1;    // FIXME JAY


    pv_data.t_lbasz = (pv_data.high_lba - pv_data.low_lba)/num_threads;

    if (num_threads >= 1) {

        /*
         * Create all threads here
         */

        for (i=0; i< num_threads; i++) {

            pv_data.t_data[i].t_low_lba = pv_data.low_lba + (pv_data.t_lbasz)*i ;

            if (verbose)
                fprintf(stderr,"Setting low_lba for thread %d, 0x%llx\n",i,pv_data.t_data[i].t_low_lba);
            rc = pthread_create(&pv_thread[i],NULL,run_pvthread_loop,(void *)&pv_data);
            if (rc) {

                fprintf(stderr, "pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
                        i, rc,errno);
                *ret = -1;
                *err = errno;
                return (rc);
            }
            if (verbose)
                fprintf(stderr, "pthread %d, started\n", i);
        }


        /* 
         * Wait for all threads to complete
         */


        errno = 0;

        for (i=0; i< num_threads; i++) {

            rc = pthread_join(pv_thread[i],&status);

            if (rc) {
                fprintf(stderr,"Thread %d returned fail ret %d, errno = %d\n",i,
                        rc,errno);
            }
            if (verbose)
                fprintf(stderr, "pthread %d, exited\n", i);
        }
        fprintf(stderr,"\n\n");
        return (rc);
    } else {

        if(verbose)
            fprintf (stderr,"starting w/o thread \n");
        pv_data.t_data[i].t_low_lba = pv_data.low_lba ;
        pv_data.t_lbasz = (pv_data.high_lba - pv_data.low_lba);
        run_pvthread_loop((void *)&pv_data);
        if (pv_data.t_data[i].ret)
            fprintf(stderr,"Test failed ret %d, errno = %d\n",pv_data.t_data[i].ret, pv_data.t_data[i].errcode);

        fprintf(stderr,"\n\n");
        return (pv_data.t_data[i].ret);
    }
}
