/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/tool/provtool.c $                            */
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
 * @file provtool.c
 * @brief Provisioning utility tooling
 */


/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <argp.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdlib.h>
#include <prov.h>
#include <provutil.h>
#include <provtool.h>
/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
const char                 *argp_program_version = "provtool 1.0\n";
const char                 *argp_program_bug_address = "IBM Support";
char                        doc[] =
"\nprovtool -- Provisioning for CAPI KV Solution\n\v";

const char tpmd_session_entry[] = "provtool";



/*----------------------------------------------------------------------------*/
/* Struct / Typedef                                                           */
/*----------------------------------------------------------------------------*/

//
enum argp_char_options {
    
    // Note that we need to be careful to not re-use char's
    PROV_LIST_AFUS            = 'l',
    PROV_QUERY_AFU            = 'q',
    PROV_WWPNS_READ           = 'w',
    PROV_DEBUG                = 'D',
    PROV_WWPNS_PGM            = 'W',
    PROV_LOOPBACK             = 'L',
    PROV_HELP                 = 'h',


};

static struct argp_option   options[] = {
    //{"query_afu",           PROV_QUERY_AFU,  0, 0, "Query AFU for version information"},
    {"wwpn-read",           PROV_WWPNS_READ, "device path",  OPTION_ARG_OPTIONAL, "Show all WWPNs in VPD"},
    {"wwpn-pgm",            PROV_WWPNS_PGM,  "device path",  OPTION_ARG_OPTIONAL, "Program WWPNs"},
    {"debug",               PROV_DEBUG,  "<LV>", 0, "Internal trace level for tool"},
    {"loopback",            PROV_LOOPBACK,  "<seconds>", OPTION_ARG_OPTIONAL, "Run loopback diagnostics on all present adapters. Time defaults to 1 second"},
    {"help",                PROV_HELP,       0, 0, "print help"},
    {0}
};


static struct argp          argp = { options, parse_opt, 0, doc };



/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
struct arguments        g_args = {0};
extern int32_t                 g_traceE;   /* error traces */
extern int32_t                 g_traceI;       /* informative 'where we are in code' traces */
extern int32_t                 g_traceF;       /* function exit/enter */
extern int32_t                 g_traceV;       /* verbose trace...lots of information */
int32_t usage=0;

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
    static int32_t last_command = -1;
    //int32_t l_temp = 0;
    
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    switch (key)
    {
        case PROV_HELP: usage=1; break;
        case PROV_LIST_AFUS:
            g_args.list_afus = 1;
            break;
        
        case PROV_WWPNS_READ:
            if((arg != NULL) && (strlen(arg) != 0))
            {
                strncpy(g_args.target_adapter, arg, sizeof(g_args.target_adapter));
                TRACEV("Setting target adapter to: '%s'\n", g_args.target_adapter);
            }
            g_args.wwpn_read = 1;
            break;
            
        case PROV_WWPNS_PGM:
            if((arg != NULL) && (strlen(arg) != 0))
            {
                strncpy(g_args.target_adapter, arg, sizeof(g_args.target_adapter));
                TRACEV("Setting target adapter to: '%s'\n", g_args.target_adapter);
            }
            g_args.wwpn_program = 1;
            break;
        
        case PROV_LOOPBACK:
            g_args.loopback = 1;
            g_args.loopback_time = LOOPBACK_TEST_DEFAULT_S; //choose a default value and then check to see if we have an arg
            if((arg != NULL) && (strlen(arg) != 0))
            {
                g_args.loopback_time = atoi(arg);
                if((g_args.loopback_time < LOOPBACK_TEST_MIN_S) || (g_args.loopback_time > LOOPBACK_TEST_MAX_S))
                {
                    TRACED("Invalid loopback test time of %d. Defaulting to %d\n", g_args.loopback_time, LOOPBACK_TEST_DEFAULT_S);
                    g_args.loopback_time = LOOPBACK_TEST_DEFAULT_S;
                }
            }
            break;
            
        case PROV_DEBUG:
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
            
            if((last_command == PROV_QUERY_AFU) &&
               ((strncmp(arg,"/",1))) && // They may put input dir/file
               ((strncmp(arg,".",1))))   //  after comp so look for it
            {
                TRACEI("Also Look for component %s\n",arg);
                last_command = PROV_QUERY_AFU;
                goto exit_no_set;
                
            }
            
            break;
            
        default:
            return (ARGP_ERR_UNKNOWN);
    }
    
    last_command = -1;
    TRACEF ("Last cmd\n");
    return (0);
    
exit_no_set:
    return(0);
    
}


int main (int argc, char *argv[])
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    int32_t                     rc = 0;
    
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    memset(&g_args,0,sizeof(g_args));
    
    error_t err=argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &g_args);

    if (err || usage)
    {
        printf("Usage: provtool\n");
        exit(0);
    }

    if(g_args.loopback)
    {
        TRACEV("Calling prov_get_all_adapters\n");
        bool l_success = prov_loopback();
        if(!l_success)
        {
            rc = 1; //arbitrary non-zero RC
        }
    }
    
    if(g_args.list_afus)
    {
        TRACEV("Calling prov_get_all_adapters\n");
        prov_get_all_adapters();
    }

    if(g_args.wwpn_read)
    {
        TRACEV("Calling prov_get_wwpns\n");
        prov_get_wwpns();
    }
    if(g_args.wwpn_program)
    {
        TRACEV("Calling prov_pgm_wwpns\n");
        prov_pgm_wwpns();
    }
    
    return(rc);
}

#define MAX_NUM_ADAPTERS 4

void prov_get_all_adapters()
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    prov_adapter_info_t l_adapters[MAX_NUM_ADAPTERS];
    int l_num_adapters = MAX_NUM_ADAPTERS;
    bool l_rc = false;
    int l_curr_adapter = 0;
    int l_curr_wwpn = 0;
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    
    bzero(l_adapters, sizeof(l_adapters));
    
    l_rc = provGetAllAdapters(l_adapters, &l_num_adapters);
    if(l_rc == false)
    {
        TRACEE("Error occurred in call to provGetAllAdapters\n");
    }
    else
    {
        TRACEV("Got back %d adapters\n", l_num_adapters);
        for(l_curr_adapter = 0; l_curr_adapter < l_num_adapters; l_curr_adapter++)
        {
            TRACED("Adapter  '%s'\n", l_adapters[l_curr_adapter].afu_name);
            TRACED("PCI Path '%s'\n", l_adapters[l_curr_adapter].pci_path);
            for(l_curr_wwpn = 0; l_curr_wwpn < MAX_WWPNS_PER_ADAPTER; l_curr_wwpn++)
            {
                TRACEI("    Port %d WWPN: 0x%s\n", l_curr_wwpn, l_adapters[l_curr_adapter].wwpn[l_curr_wwpn]);
            }
        }
    }
}


void prov_pgm_wwpns()
{

    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    prov_adapter_info_t l_adapters[MAX_NUM_ADAPTERS];
    int l_num_adapters = MAX_NUM_ADAPTERS;
    bool l_rc = false;
    int l_curr_adapter = 0;
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    
    bzero(l_adapters, sizeof(l_adapters));
    
    l_rc = provGetAllAdapters(l_adapters, &l_num_adapters);
    if(l_rc == false)
    {
        TRACEE("Error occurred in call to provGetAllAdapters. This is likely due to missing or invalid adapter VPD.\n");
    }
    else
    {
        TRACEV("Got back %d adapters\n", l_num_adapters);
        for(l_curr_adapter = 0; l_curr_adapter < l_num_adapters; l_curr_adapter++)
        {
            l_rc = provInitAdapter(&l_adapters[l_curr_adapter]);
            if(l_rc == false)
            {
                TRACEE("Error init'ing adapter '%s'\n",l_adapters[l_curr_adapter].afu_name);
            }
        }
    }
    
}


#define TIME_1S_US 1000000 //1million uS in a second
bool prov_loopback()
{

    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    prov_adapter_info_t l_adapters[MAX_NUM_ADAPTERS];
    int l_num_adapters = MAX_NUM_ADAPTERS;
    bool l_rc = true;
    bool l_adapter_rc = false;
    int l_curr_adapter = 0;
    uint64_t l_loopback_time_us = 0;
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/

    bzero(l_adapters, sizeof(l_adapters));

    l_rc = provGetAllAdapters(l_adapters, &l_num_adapters);
    if(l_rc == false)
    {
        printf("ERROR: Error occurred in call to provGetAllAdapters. This is likely due to missing or invalid adapter VPD.\n");
    }
    else
    {
        printf("Running Loopback Diagnostic Tests...\n");
        l_loopback_time_us = (uint64_t)g_args.loopback_time * TIME_1S_US;
        TRACEV("Running time for test in microseconds: %"PRIu64"\n",l_loopback_time_us);
        TRACEV("Got back %d adapters\n", l_num_adapters);
        for(l_curr_adapter = 0; l_curr_adapter < l_num_adapters; l_curr_adapter++)
        {
            l_adapter_rc = provLoopbackTest(&l_adapters[l_curr_adapter], l_loopback_time_us);
            const char *l_result = (l_adapter_rc == true) ? "SUCCESS" : "FAIL"; 
            printf("Adapter '%s' result: %s\n",l_adapters[l_curr_adapter].afu_name, l_result);
            if(l_adapter_rc == false)
            {
                //if any test fails, all tests fail
                l_rc = false;
            }
        }
    }
    return l_rc;
}

void prov_get_wwpns()
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    prov_wwpn_info_t wwpn_array[32];
    uint16_t num_wwpns = 32;
    int i = 0;
    
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    provGetAllWWPNs(wwpn_array, &num_wwpns);
    TRACED("Number of WWPNs read back: %d\n", num_wwpns);
    for(i=0; i<num_wwpns; i++)
    {
        TRACED("WWPN %i: Adapter: '%s', port %d: 0x%s\n", i, wwpn_array[i].afu_path, wwpn_array[i].port_id, wwpn_array[i].wwpn);
    }
    return;
}

