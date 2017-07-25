/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlfd.c $                                          */
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

/*!
 * @file cxlflashutil.c
 * @brief utility tooling
 */


/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <argp.h>
#include <unistd.h>
#include <sys/types.h>


#include <stdlib.h>

#include <cxlfcommon.h>
#include <cxlfd.h>

#include <time.h> //delete this!
/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/

error_t parse_opt (int key,
                   char *arg,
                   struct argp_state *state);



/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/


const char                 *argp_program_version = "cxlfd\n";
const char                 *argp_program_bug_address = "IBM Support";
char                        doc[] =
"\ncxlfd -- LUN management daemon for IBM Data Engine for NoSQL Software\n\v";


/*----------------------------------------------------------------------------*/
/* Struct / Typedef                                                           */
/*----------------------------------------------------------------------------*/

//
enum argp_char_options {
    
    // Note that we need to be careful to not re-use char's
    CXLF_DEBUG                = 'D',
    CXLF_TIMERFREQ            = 't',
    CXLF_HELP                 = 'h',

};

static struct argp_option   options[] = {
    {"timer",      CXLF_TIMERFREQ,  "<interval>",  OPTION_HIDDEN, "Update interval for commands to be sent to cxlflash driver (in seconds)."},
    {"debug",      CXLF_DEBUG,      "<LV>", 0, "Internal trace level for tool"},
    {"help",       CXLF_HELP,       0, 0, "print help"},
    {0}
};


static struct argp          argp = { options, parse_opt, 0, doc };



/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
struct arguments                g_args = {0};
int32_t                         g_traceE = 1;       /* error traces */
int32_t                         g_traceI = 0;       /* informative 'where we are in code' traces */
int32_t                         g_traceF = 0;       /* function exit/enter */
int32_t                         g_traceV = 0;       /* verbose trace...lots of information */
lun_table_entry_t               g_cxldevs[MAX_NUM_SGDEVS] = {{{0}}};
int32_t                         g_cxldevs_sz = 0;
lun_table_entry_t               g_luntable[MAX_NUM_LUNS] = {{{0}}};
int32_t                         g_luntable_sz = 0;
int32_t                         usage=0;

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

/*
 need to pass down wwn (get from scsi inquiry data) - does this have a "3" on the front or not???
 need to save off wwns in a file
 need to call utility on udev startup / plugging... see ethernet code
*/

error_t parse_opt (int key,
                   char *arg,
                   struct argp_state *state)
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    char* endptr = NULL;
    
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    switch (key)
    {
            //        case CXLF_GET_MODE:
            //g_args.get_mode = 1;
            //break;
            
        case CXLF_HELP: usage=1; break;
        case CXLF_TIMERFREQ:
            if(((uint16_t)strtol(arg,&endptr, 10)<= 0) || (endptr != (arg+strlen(arg))))
            {
                TRACED("Interval must be a positive integer in seconds. '%s' is invalid.\n", arg);
                exit(EINVAL);
            }
            else
            {
                g_args.timer_override = atoi(arg);
                TRACEV("Set timer override to %d\n", g_args.timer_override);
            }
            break;
            
        case CXLF_DEBUG:
            g_args.verbose = atoi (arg);
            TRACEI ("Set verbose level to %d\n", g_args.verbose);
            if (g_args.verbose >= 1)
                g_traceI = 1;
            if (g_args.verbose >= 2)
                g_traceF = 1;
            if (g_args.verbose >= 3)
                g_traceV = 1;
            break;
            
        case 0 :
            
            TRACEV("Got a naked argument: '%s'\n", arg);
            break;
            
        default:
            return (ARGP_ERR_UNKNOWN);
    }
    
    return (0);
    
}









#define BILLION 1000000000L
int main (int argc, char *argv[])
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    int32_t                     rc = 0;
    int16_t                     timer_interval = DEFAULT_TIMER_INTERVAL;
    //int i = 0;
    bool                        skip_trace = false; //prevent us from filling logs
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    memset(&g_args,0,sizeof(g_args));
    
    error_t err=argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &g_args);

    if (err || usage)
    {
        printf("Usage: cxlfd\n");
        exit(0);
    }

    if(g_args.timer_override != 0)
    {
        timer_interval = g_args.timer_override;
    }
    
    TRACED("Starting up with %d second interval\n", timer_interval);
    
    while(1)
    {
        update_cxlflash_devs(g_cxldevs, &g_cxldevs_sz, NULL);
        update_siotable(g_luntable, &g_luntable_sz);
        
        if(g_cxldevs_sz != 0)
        {
            skip_trace = false;
            cxlf_refresh_luns(g_luntable, g_luntable_sz, g_cxldevs, g_cxldevs_sz);
        }
        else
        {
            if(skip_trace != true)
            {
                TRACED("No CXL Devices were found; waiting...\n");
                skip_trace = true;
            }
        }
        
        sleep(timer_interval);
        continue;
        
        
    }
    
    
    return(rc);
}



