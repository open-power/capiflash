/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/blk_eras.c $                                         */
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
#include <cflash_block_eras_client.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif
    

/*
 * Global variables declared in this module...
 */
static int live_dump_flag;      /* indicates the D flag was passed on command
                                 * line which indicates to a do a live dump
                                 * with the specified reason
                                 */

static int trace_filename_flag; /* indicates the F flag was passed on command
                                 * line which indicates a trace filename is
                                 * specified
                                 */
static int get_statistics_flag; /* indicates the G flag was passed on command
                                 * line which indicates a disk name is
                                 * specified
                                 */

static int hflag;               /* help flag */

static int pid_flag;            /* indicates the P flag was passed on command
                                 * line which indicates that a Process ID (PID)
				 * was passed.
                                 */
static int verbose_flag;         /* verbose mode flag */

static int trace_verbosity_flag;/* indicates the V flag was passed on command
                                 * line which indicates that we have a user
				 * specified trace verbosity level.
				 */



static char *diskname = NULL;        /* point to the arg for -G flag */
static char *live_dump_reason = NULL;/* point to the arg for -D flag */
static char *pid_string = NULL;      /* point to the arg for -P flag */
static char *trace_filename = NULL;  /* point to the arg for -F flag */
static int  trace_verbosity = 0;     /* point to the arg for -V flag */

FILE *file = NULL;

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
    
    fprintf(stderr,"Usage: blk_eras -P pid [ -D live_dump_reason] [ -F trace_filename] [ -G disk_name] [-V trace_verbosity] \n\n");
    fprintf(stderr,"      where:\n");
    fprintf(stderr,"              -D  Live Dump reason string\n");
    fprintf(stderr,"              -F  trace filename\n");
    fprintf(stderr,"              -G  disk name for get statistics\n");
    fprintf(stderr,"              -h  help (this usage)\n");
    fprintf(stderr,"              -P  PID of process to change trace information\n");
    fprintf(stderr,"              -V  trace verbosity\n");


    
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
    live_dump_flag = FALSE;
    trace_filename_flag = FALSE;
    hflag     = FALSE;
    trace_verbosity_flag = FALSE;
    verbose_flag = FALSE;


    
    
    /* 
     * Get parameters
     */
    while ((c = getopt(argc,argv,"D:F:G:P:V:hv")) != EOF)
    {  
	switch (c)
	{ 
	case 'D' :
	    live_dump_reason = optarg;
	    if (live_dump_reason) {
		live_dump_flag = TRUE;
	    } else {
		
		fprintf(stderr,"-D flag requires a live dump reason \n");
		rc = EINVAL;
		
	    }
	    break;
	case 'F' :
	      trace_filename = optarg;
	    if (trace_filename) {
		trace_filename_flag = TRUE;
	    } else {
		
		fprintf(stderr,"-F flag requires a trace filename \n");
		rc = EINVAL;
		
	    }
	    break;
	case 'G' :
	    diskname = optarg;
	    if (diskname) {
		get_statistics_flag = TRUE;
	    } else {
		
		fprintf(stderr,"-G flag requires a disk name \n");
		rc = EINVAL;
		
	    }
	    break;
	case 'h' :
	    hflag = TRUE;
	    break;
	case 'P' :
	    pid_string = optarg;
	    if (pid_string) {
		pid_flag = TRUE;
	    } else {
		
		fprintf(stderr,"-P flag requires a process id (PID)\n");
		rc = EINVAL;
		
	    }
	    break; 
	case 'v' :
	    verbose_flag = TRUE;
	    break;  
	case 'V' :
	    if (optarg) {
		
		trace_verbosity = atoi(optarg);
		
		trace_verbosity_flag = TRUE;

	    } else {
		
		
		fprintf(stderr,"-V flag requires a trace verbosity value to be supplied\n");
		
	    }
	    break; 
	default: 
	    usage();
	    break;
	}/*switch*/
    }/*while*/


    if (!pid_flag) {
	fprintf(stderr,"The -P flag is required to specify a process id (PID)\n");
	usage();
	rc = EINVAL;
    }

    return (rc);


}/*parse_args*/

int main (int argc, char **argv)
{
    int rc = 0;       /* Return code                 */
    int flags = 0;
    cflsh_blk_eras_hndl_t cflsh_blk_eras_hndl;
    chunk_stats_t *stats;

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


    rc = cblk_init(NULL,0);
    if (rc) {

	fprintf(stderr,"cblk_init failed with errno = %d and rc = %d\n",
		errno,rc);
        return (-1);
    }

    rc = cblk_eras_init();
    if (rc) {

	fprintf(stderr,"cblk_eras_init failed with errno = %d and rc = %d\n",
		errno,rc);
        return (-1);
    }




    rc = cblk_eras_register(pid_string,&cflsh_blk_eras_hndl);
    if (rc) {

	fprintf(stderr,"cblk_eras_register failed with errno = %d and rc = %d\n",
		errno,rc);
        return (-1);
    }


    if (live_dump_flag) {

	rc = cblk_eras_live_dump(cflsh_blk_eras_hndl,live_dump_reason,0);

	if (rc) {

	    fprintf(stderr,"cblk_eras_live_dump failed with errno = %d and rc = %d\n",
		    errno,rc);
	} 


    } else if (trace_filename_flag ||
	       trace_verbosity_flag) {

	if (trace_filename_flag) {

	    flags |= CBLK_TRC_CLIENT_SET_FILENAME;
	}

	if (trace_verbosity_flag) {

	    flags |= CBLK_TRC_CLIENT_SET_VERBOSITY;
	}

	rc = cblk_eras_set_trace(cflsh_blk_eras_hndl,trace_verbosity,trace_filename,flags);

	if (rc) {

	    fprintf(stderr,"cblk_eras_set_trace failed with errno = %d and rc = %d\n",
		    errno,rc);
	}
    } else if (get_statistics_flag) {


	stats = malloc(sizeof(*stats));

	if (stats == NULL) {

	    fprintf(stderr,"malloc of stats failed with errno = %d and rc = %d\n",
		    errno,rc);
	    return (-1);
	}

	bzero((void *)stats,sizeof(*stats));

	rc = cblk_eras_get_stat(cflsh_blk_eras_hndl,diskname,stats,flags);

	if (rc) {

	    fprintf(stderr,"cblk_eras_get_stat failed with errno = %d and rc = %d\n",
		    errno,rc);
	} else {


	    fprintf(stdout,"chunk_block size                0x%x\n",stats->block_size);
	    fprintf(stdout,"num_reads                       0x%"PRIX64"\n",stats->num_reads);
	    fprintf(stdout,"num_writes                      0x%"PRIX64"\n",stats->num_writes);
	    fprintf(stdout,"num_areads                      0x%"PRIX64"\n",stats->num_areads);
	    fprintf(stdout,"num_awrites                     0x%"PRIX64"\n",stats->num_awrites);
	    fprintf(stdout,"num_act_reads                   0x%x\n",stats->num_act_reads);
	    fprintf(stdout,"num_act_writes                  0x%x\n",stats->num_act_writes);
	    fprintf(stdout,"num_act_areads                  0x%x\n",stats->num_act_areads);
	    fprintf(stdout,"num_act_awrites                 0x%x\n",stats->num_act_awrites);
	    fprintf(stdout,"max_num_act_reads               0x%x\n",stats->max_num_act_reads);
	    fprintf(stdout,"max_num_act_writes              0x%x\n",stats->max_num_act_writes);
	    fprintf(stdout,"max_num_act_areads              0x%x\n",stats->max_num_act_areads);
	    fprintf(stdout,"max_num_act_awrites             0x%x\n",stats->max_num_act_awrites);
	    fprintf(stdout,"num_blocks_read                 0x%"PRIX64"\n",stats->num_blocks_read);
	    fprintf(stdout,"num_blocks_written              0x%"PRIX64"\n",stats->num_blocks_written);
	    fprintf(stdout,"num_aresult_no_cmplt            0x%"PRIX64"\n",stats->num_aresult_no_cmplt);
	    fprintf(stdout,"num_errors                      0x%"PRIX64"\n",stats->num_errors);
	    fprintf(stdout,"num_retries                     0x%"PRIX64"\n",stats->num_retries);
	    fprintf(stdout,"num_timeouts                    0x%"PRIX64"\n",stats->num_timeouts);
	    fprintf(stdout,"num_fail_timeouts               0x%"PRIX64"\n",stats->num_fail_timeouts);
	    fprintf(stdout,"num_reset_contexts              0x%"PRIX64"\n",stats->num_reset_contexts);
	    fprintf(stdout,"num_reset_context_fails         0x%"PRIX64"\n",stats->num_reset_contxt_fails);
	    fprintf(stdout,"num_no_cmds_free                0x%"PRIX64"\n",stats->num_no_cmds_free);
	    fprintf(stdout,"num_no_cmd_room                 0x%"PRIX64"\n",stats->num_no_cmd_room);
	    fprintf(stdout,"num_no_cmds_free_fail           0x%"PRIX64"\n",stats->num_no_cmds_free_fail);
	    fprintf(stdout,"num_cc_errors                   0x%"PRIX64"\n",stats->num_cc_errors);
	    fprintf(stdout,"num_fc_errors                   0x%"PRIX64"\n",stats->num_fc_errors);
	    fprintf(stdout,"num_port0_linkdowns             0x%"PRIX64"\n",stats->num_port0_linkdowns);
	    fprintf(stdout,"num_port1_linkdowns             0x%"PRIX64"\n",stats->num_port1_linkdowns);
	    fprintf(stdout,"num_port0_no_logins             0x%"PRIX64"\n",stats->num_port0_no_logins);
	    fprintf(stdout,"num_port1_no_logins             0x%"PRIX64"\n",stats->num_port1_no_logins);
	    fprintf(stdout,"num_port0_fc_errors             0x%"PRIX64"\n",stats->num_port0_fc_errors);
	    fprintf(stdout,"num_port1_fc_errors             0x%"PRIX64"\n",stats->num_port1_fc_errors);
	    fprintf(stdout,"num_afu_errors                  0x%"PRIX64"\n",stats->num_afu_errors);
	    fprintf(stdout,"num_capi_false_reads            0x%"PRIX64"\n",stats->num_capi_false_reads);
	    fprintf(stdout,"num_capi_adap_resets            0x%"PRIX64"\n",stats->num_capi_adap_resets);
	    fprintf(stdout,"num_capi_adap_chck_err          0x%"PRIX64"\n",stats->num_capi_adap_chck_err);
	    fprintf(stdout,"num_capi_read_fails             0x%"PRIX64"\n",stats->num_capi_read_fails);
	    fprintf(stdout,"num_capi_data_st_errs           0x%"PRIX64"\n",stats->num_capi_data_st_errs);
	    fprintf(stdout,"num_capi_afu_errors             0x%"PRIX64"\n",stats->num_capi_afu_errors);
	    fprintf(stdout,"num_capi_afu_intrpts            0x%"PRIX64"\n",stats->num_capi_afu_intrpts);
	    fprintf(stdout,"num_capi_unexp_afu_intrpts      0x%"PRIX64"\n",stats->num_capi_unexp_afu_intrpts);
	    fprintf(stdout,"num_cache_hits                  0x%"PRIX64"\n",stats->num_cache_hits);
	    fprintf(stdout,"num_success_threads             0x%"PRIX64"\n",stats->num_success_threads);
	    fprintf(stdout,"num_failed_threads              0x%"PRIX64"\n",stats->num_failed_threads);
	    fprintf(stdout,"num_canc_threads                0x%"PRIX64"\n",stats->num_canc_threads);
	    fprintf(stdout,"num_fail_canc_threads           0x%"PRIX64"\n",stats->num_fail_canc_threads);
	    fprintf(stdout,"num_fail_detach_threads         0x%"PRIX64"\n",stats->num_fail_detach_threads);
	    fprintf(stdout,"num_active_threads              0x%"PRIX64"\n",stats->num_active_threads);
	    fprintf(stdout,"max_num_act_threads             0x%"PRIX64"\n",stats->max_num_act_threads);
	    fflush(stdout);
	}

	free(stats);
    }


    rc = cblk_eras_unregister(cflsh_blk_eras_hndl);
    if (rc) {

	fprintf(stderr,"cblk_eras_unregister failed with errno = %d and rc = %d\n",
		errno,rc);
    }


    cblk_eras_term();
    cblk_term(NULL,0);

    return 0;

}
