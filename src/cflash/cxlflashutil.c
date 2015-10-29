/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlflashutil.c $                                   */
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
#include <libgen.h> //basename
#include <cxlfcommon.h>
#include <cxlflashutil.h>

/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/
error_t parse_opt (int key,
                   char *arg,
                   struct argp_state *state);



/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
const char                 *argp_program_version = "cxlflashutil 2.0\n";
const char                 *argp_program_bug_address = "IBM Support";
char                        doc[] =
"\ncxlflashutil -- Control utility for IBM Data Engine for NoSQL Software\n\v";


/*----------------------------------------------------------------------------*/
/* Struct / Typedef                                                           */
/*----------------------------------------------------------------------------*/

//
enum argp_char_options {
    
    // Note that we need to be careful to not re-use char's
    //CXLF_GET_MODE             = 'm',
    CXLF_CONFIG               = 'c',
    CXLF_DEV                  = 'd',
    CXLF_DEBUG                = 'D',
    CXLF_LUN_ID               = 'l',
    CXLF_SET_MODE             = 'm',
    
};

static struct argp_option   options[] = {
    {"set-mode",    CXLF_SET_MODE,  "<0=legacy/1=sio>", 0, "Set the IO mode for ALL instances and paths to a target LUN's disk(s)."},
    {"lun",         CXLF_LUN_ID,    "<LUN ID>", 0, "Target LUN to operate on (16 byte hex value)"},
    //{"get-mode",    CXLF_GET_MODE,  0, 0, "Get the IO mode for ALL instances and paths to a target LUN's disk(s)."},
    {"device",      CXLF_DEV,       "<device path>",  0, "Target device (e.g. /dev/sg123)"},
    {"debug",       CXLF_DEBUG,     "<LV>", 0, "Internal trace level for tool"},
    {"config",      CXLF_CONFIG,    0, 0, "Configure device according to the LUN table"},
    {0}
};


static struct argp          argp = { options, parse_opt, 0, doc };



/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
struct arguments                g_args = {{0}};
int32_t                         g_traceE = 1;       /* error traces */
int32_t                         g_traceI = 0;       /* informative 'where we are in code' traces */
int32_t                         g_traceF = 0;       /* function exit/enter */
int32_t                         g_traceV = 0;       /* verbose trace...lots of information */
/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/


error_t parse_opt (int key,
                   char *arg,
                   struct argp_state *state)
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    //bool rc = false;
    char* endptr = NULL;
    int rc = 0;
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    switch (key)
    {
            //        case CXLF_GET_MODE:
            //g_args.get_mode = 1;
            //break;
        case CXLF_CONFIG:
            if(g_args.set_mode != 0)
            {
                //disallow these args simultaneously
                TRACED("Cannot set mode and configure-default option at the same time.\n");
                exit(EINVAL);
            }
            g_args.configure_default = 1;
            break;
            
        case CXLF_LUN_ID:
            
            if(strlen(g_args.target_device) != 0)
            {
                //disallow these args simultaneously
                TRACED("Cannot specify LUN config for multiple devices and a specific device simultaneously.\n");
                exit(EINVAL);
            }
            rc = cxlf_parse_wwid(g_args.wwid, arg, sizeof(g_args.wwid));
            if(rc == false)
            {
                TRACED("LUN ID is invalid.\n");
                //zero out the wwid just in case
                memset(g_args.wwid,0,sizeof(g_args.wwid));
                g_args.wwid_valid = false;
                exit(EINVAL);
            }
            else
            {
                TRACEV("Got a valid WWID\n");
                g_args.wwid_valid = true;
            }
            break;
            
        case CXLF_DEV:
            if(g_args.wwid_valid != 0)
            {
                //disallow these args simultaneously
                TRACED("Cannot specify LUN config for multiple devices and a specific device simultaneously.\n");
                exit(EINVAL);
            }
            memset(g_args.target_device, 0, DEV_STRING_SZ);
            //leave a null byte on the end to avoid buffer overruns
            strncpy(g_args.target_device, arg, DEV_STRING_SZ-1);
            TRACEV("Set target device to '%s'\n",g_args.target_device);
            break;
            
        case CXLF_SET_MODE:
            if(g_args.configure_default != 0)
            {
                //disallow configure and set_mode at the same time.
                TRACED("Cannot set mode and configure-default option at the same time.\n");
                exit(EINVAL);
            }
            if((uint8_t)strtol(arg,&endptr, 10)>= MODE_INVALID || (endptr != (arg+strlen(arg))))
            {
                TRACED("Mode argument must be '0' (LEGACY) or '1' (SIO). '%s' is invalid.\n", arg);
                exit(EINVAL);
            }
            else
            {
                g_args.set_mode = 1;
                g_args.target_mode = atoi(arg);
                TRACEV("Set mode to %d\n", g_args.target_mode);
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
            
            TRACEI("Got a naked argument: '%s'\n", arg);
            break;
            
        default:
            return (ARGP_ERR_UNKNOWN);
    }
    
    return (0);
    
}


int main (int argc, char *argv[])
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    int32_t                     rc = 0;
    char                        devname[DEV_STRING_SZ] = {0};
    char*                       devbasename = NULL;
    lun_table_entry_t           device_entry = {{0}};
    lun_table_entry_t           luntable[MAX_NUM_LUNS] = {{{0}}};
    lun_table_entry_t           devtable[MAX_NUM_SGDEVS] = {{{0}}};
    int                         luntable_sz = 0;
    int                         devtable_sz = 0;
    int                         i = 0;
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    memset(&g_args,0,sizeof(g_args));
    
    argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &g_args);
    
    
    do
    {
        //if a LUN was specified, we need to make up a single-item LUN table
        //and then take some action. To do this, we get all SG devs, and
        //filter based on the matching LUN entry.
        //we then either keep or delete that entry so that we can set the
        //LUN to SIO mode or LEGACY.
        if(g_args.wwid_valid)
        {
            TRACEV("Setting LUN %s to mode %d\n",g_args.wwid, g_args.target_mode);
            //make a single LUN table entry
            luntable_sz=1;
            memcpy(luntable[0].lun, g_args.wwid, DK_CXLFLASH_MANAGE_LUN_WWID_LEN);
            
            //get all possible wwids
            rc = update_cxlflash_devs(devtable, &devtable_sz, &luntable[0]);
            if(rc!= 0)
            {
                TRACED("Error processing device tables.\n");
                break;
            }
            
            if(g_args.target_mode == MODE_LEGACY)
            {
                //"delete" the entry in the LUN table so that we can
                //trigger a refresh, and effectively set all LUNs to legacy
                //otherwise allow the single entry to continue to exist
                luntable_sz = 0;
            }
            //refresh the device(s) that matched
            rc = cxlf_refresh_luns(luntable, luntable_sz, devtable, devtable_sz);
            if(rc!= 0)
            {
                TRACED("Error refreshing LUNs.\n");
                break;
            }
        }
        else
        {
            //set a single device's values...
            if(strlen(g_args.target_device) == 0)
            {
                TRACED("Error: device special file name required.\n");
                break;
            }
            
            //copy this since basename() may corrupt the original string depending
            //on the implementation
            strncpy(devname, g_args.target_device, DEV_STRING_SZ);
            devbasename = basename(devname);
            if(strcmp(devname, ".")==0)
            {
                TRACED("Error: device name '%s' appears to be invalid.",g_args.target_device);
                break;
            }
            rc = extract_lun_from_vpd(devbasename, device_entry.lun);
            if(rc != 0)
            {
                TRACED("Error: Unable to find a matching LUN ID for device '%s' - is this a cxlflash device, and are you able to manage it?\n", g_args.target_device);
                break;
            }
            
            //if the default config option is set, then read the LUN table,
            //and set a mode based on its contents
            if(g_args.configure_default != 0)
            {
                
                g_args.set_mode = 1;
                rc = update_siotable(luntable, &luntable_sz);
                if(rc != 0)
                {
                    TRACED("Error: unable to read LUN table successfully.\n");
                    break;
                }
                //scan the sio table for this disk's LUN, and if found,
                //set the mode to SIO
                g_args.target_mode = MODE_LEGACY;
                for(i = 0; i < luntable_sz; i++)
                {
                    if(compare_luns(&luntable[i], &device_entry) == 0)
                    {
                        
                        g_args.target_mode = MODE_SIO;
                        break;
                    }
                }
            }
            
            
            
            bool l_success = cxlf_set_mode(g_args.target_device, g_args.target_mode, device_entry.lun);
            if(!l_success)
            {
                TRACED("ERROR: Device driver call returned an error.\n");
                rc = EIO; //arbitrary non-zero RC
                break;
            }
            
        }
        
        TRACED("SUCCESS\n");
        rc = 0;
    } while(0);
    
    
    return(rc);
}




