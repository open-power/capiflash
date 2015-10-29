/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/trace_log.h $                                     */
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

#ifndef _H_TRACE_LOG
#define _H_TRACE_LOG


#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif



typedef struct trace_log_ext_arg_s {
    int flags;
#define TRACE_LOG_START_VALID     0x0001 /* Start time valid.               */
#define TRACE_LOG_NO_USE_LOG_NUM  0x0002 /* Use log_number - 1 for trace    */
    struct timeb start_time;
    struct timeb last_time;
    uint log_number;
} trace_log_ext_arg_t;


#define TRACE_LOG_EXT_OPEN_APPEND_FLG  0x1



/*-------------------------------------------------------------------------
 *
 *
 * To use these convenience macros for these trace facilities, one 
 * needs to create  three globabl variables (static is optional) 
 * that are analogous to the the log_filename,
 * log_verbosity, and logfp below. Then the macros listed here would
 * be cloned (renamed) and modified to use these three variables
 */
#ifdef _SAMPLE_
static char             *log_filename;         /* Trace log filename       */
                                               /* This traces internal     */
                                               /* activities               */
static int              log_verbosity = 0;     /* Verbosity of traces in*/
                                               /* log.                     */
static FILE             *logfp;                /* File descriptor for      */
                                               /* trace log                */

static pthread_mutex_t  log_lock = PTHREAD_MUTEX_INITIALIZER;

#define TRACE_LOG_FILE_SETUP(filename)                                  \
do {                                                                    \
    setup_log_file(&log_filename, &logfp,char *filename);               \
} while (0)


#define TRACE_LOG_FILE_SET_VERBOSITY(new_verbosity)                     \
do {                                                                    \
    log_verbosity = new_verbosity;                                      \
} while (0)

#define TRACE_LOG_FILE_CLEANUP()                                        \
do {                                                                    \
    cleanup_log_file(log_filename,logfp);                               \
} while (0)


#define TRACE_LOG_FILE_TRC(verbosity,fmt, ...)                          \
do {                                                                    \
   if ((log_filename != NULL) &&                                        \
       (verbosity <= log_verbosity)) {                                  \
      pthread_mutex_lock(&log_lock);                                    \
      trace_log_data(logfp,__FILE__,__FUNCTION__,__LINE__,              \
                     fmt,## __VA_ARGS__);                               \
      pthread_mutex_unlock(&log_lock);                                  \
   }                                                                    \
} while (0)


#endif /* _SAMPLE_ */

/* ----------- End of sample macros ---------------------------*/



#define TRACE_LOG_DATA_TRC(tlog_lock,tlogfp,tlog_verbosity,tfilename,verbosity,fmt, ...) \
do {                                                                    \
   if ((tfilename != NULL) &&                                           \
       (verbosity <= tlog_verbosity)) {                                 \
      pthread_mutex_lock(&tlog_lock);                                   \
      trace_log_data(tlogfp,__FILE__,__FUNCTION__,                      \
                     __LINE__,fmt,## __VA_ARGS__);                      \
      pthread_mutex_unlock(&tlog_lock);                                 \
   }                                                                    \
} while (0)


/* ----------------------------------------------------------------------------
 *
 * NAME: trace_log_data
 *                  
 * FUNCTION: Print a message to trace log.
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
static inline void trace_log_data(FILE *logfp,char *filename, char *function, 
			   uint line_num,char *msg, ...)
{
    va_list ap;
    static uint log_number = 0;
    static struct timeb last_time;
    struct timeb cur_time, log_time, delta_time;
    static struct timeb start_time;
    static int start_time_valid = FALSE;


    ftime(&cur_time);
    
    if (!start_time_valid) {

        /*
         * If start time is not set, then
         * set it now.
         */

        start_time = cur_time;

        start_time_valid = TRUE;

        log_time.time = 0;
        log_time.millitm = 0;

        delta_time.time = 0;
        delta_time.millitm = 0;
        

        /*
         * Print header
         */
        fprintf(logfp,"---------------------------------------------------------------------------\n");
        fprintf(logfp,"Date for %s is %s at %s\n",__FILE__,__DATE__,__TIME__);
        fprintf(logfp,"Index   Sec   msec  delta dmsec      Filename            function, line ...\n");
        fprintf(logfp,"------- ----- ----- ----- ----- --------------------  ---------------------\n");

    } else {

        /*
         * Find time offset since starting time.
         */

        log_time.time = cur_time.time - start_time.time;
        log_time.millitm = cur_time.millitm - start_time.millitm;

        delta_time.time = log_time.time - last_time.time;
        delta_time.millitm = log_time.millitm - last_time.millitm;
    }

    fprintf(logfp,"%7d %5d.%05d %5d.%05d %20s  %s, line = %d :",
            log_number,(int)log_time.time,log_time.millitm,(int)delta_time.time,delta_time.millitm,filename, function, line_num);
    /*
     * Initialize ap to store arguments after msg
     */

    va_start(ap,msg);
    vfprintf(logfp, msg, ap);
    va_end(ap);

    fprintf(logfp,"\n");

    fflush(logfp);

    log_number++;

    last_time = log_time;

    return;
       
}

/* ----------------------------------------------------------------------------
 *
 * NAME: trace_log_data_ext
 *                  
 * FUNCTION: Print a message to trace log. This
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
static inline void trace_log_data_ext(trace_log_ext_arg_t *ext_arg, FILE *logfp,char *filename, char *function, 
					   uint line_num,char *msg, ...)
{
    va_list ap;
    struct timeb cur_time, log_time, delta_time;
    uint print_log_number;

    if (ext_arg == NULL) {

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

    fprintf(logfp,"%7d %5d.%05d %5d.%05d %20s  %s, line = %d :",
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




/* ----------------------------------------------------------------------------
 *
 * NAME: setup_log_file
 *                  
 * FUNCTION:  Set up trace_log file
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
static inline int setup_trace_log_file(char **log_filename, FILE **logfp,char *filename)
{

    if ((*log_filename) &&
        (*logfp)) {

        fflush(*logfp);
        fclose(*logfp);
    }

    *log_filename = filename;

    if ((*logfp = fopen(*log_filename, "a")) == NULL) {

        fprintf (stderr,
                 "\nFailed to open log trace file %s\n",*log_filename);

        *log_filename = NULL;

        return 1;
    }

    return 0;
}


/* ----------------------------------------------------------------------------
 *
 * NAME: setup_log_file_ext
 *                  
 * FUNCTION:  Set up trace_log file
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
static inline int setup_trace_log_file_ext(char **log_filename, FILE **logfp,char *filename,int flags)
{
    char *open_mode_str = "w";


    if ((*log_filename) &&
        (*logfp)) {

        fflush(*logfp);
        fclose(*logfp);
    }

    *log_filename = filename;

    if (flags & TRACE_LOG_EXT_OPEN_APPEND_FLG) {

	open_mode_str = "a";
    }

    if ((*logfp = fopen(*log_filename, open_mode_str)) == NULL) {

        fprintf (stderr,
                 "\nFailed to open log trace file %s\n",*log_filename);

        *log_filename = NULL;

        return 1;
    }

    return 0;
}



/* ----------------------------------------------------------------------------
 *
 * NAME: cleanup_trace_log_file
 *                  
 * FUNCTION:  clean up and close trace log file.
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
static inline void cleanup_trace_log_file(char *log_filename,FILE *logfp)
{


    if ((log_filename) &&
        (logfp)) {

        fflush(logfp);
        fclose(logfp);
    }

    log_filename = NULL;

    return;
}
#ifdef _REMOVE
// This function is too specific to a an implementation so remove it.
/* ----------------------------------------------------------------------------
 *
 * NAME: trace_log_init
 *                  
 * FUNCTION:  Get environment variables.
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
static inline void log_init(char **log_filename, FILE **logfp,int *trace_log_verbosity)
{

    char *log = getenv("ZLIB_LOG");
    char *env_verbosity = getenv("ZLIB_LOG_VERBOSITY");
    int verbosity = 0;



        
    if (log && env_verbosity) {

        /*
         * Turn on logging
         */

        verbosity = atoi(env_verbosity);

	*trace_log_verbosity = verbosity;

        setup_trace_log_file(log_filename,logfp,log);

    }
    return;
}

#endif /* _REMOVE */
#endif /* H_TRACE_LOG */
