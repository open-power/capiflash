/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/tool/provtool.h $                            */
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

#ifndef _PROVTOOL_H
#define _PROVTOOL_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <argp.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

#define TRACE_ENABLED
//Loopback test constants are defined in seconds for UI but internally we use microseconds
#define LOOPBACK_TEST_DEFAULT_S 1 //1 second
#define LOOPBACK_TEST_MIN_S 1 //1 second
#define LOOPBACK_TEST_MAX_S 86400 //24 hours
/*@}*/  // Ending tag for external constants in doxygen

/*----------------------------------------------------------------------------*/
/* Enumerations                                                               */
/*----------------------------------------------------------------------------*/
/**
 * \defgroup ExternalEnum External Enumerations
 */
/*@{*/   // Special tag to say everything between it and the ending
       // brace is a part of the external enum module in doxygen.


struct arguments
{
    int16_t                     list_afus;
    int16_t                     wwpn_read;
    int16_t                     wwpn_program;
    char                        target_adapter[128];
    bool                        read_wwpn_from_afu;
    int16_t                     verbose;
    int16_t                     loopback;
    int32_t                     loopback_time;
};

/*@}*/  // Ending tag for external structure module in doxygen


/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
extern struct arguments        g_args;

/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/

error_t parse_opt (int key,
                          char *arg,
                          struct argp_state *state);


uint32_t convert_to_binary (uint8_t **output_buffer,
                            uint32_t *output_buffer_length,
                            char *input_buffer);

void static inline toupper_string(char *text)
{
        size_t j;
        for(j=0;j < strlen(text);j++)
        {
                text[j] = toupper(text[j]);
        }
}

void prov_get_wwpns();

void prov_pgm_wwpns();

void prov_get_all_adapters();

bool prov_loopback();

#endif //_PROVTOOL_H

