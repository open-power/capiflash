/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/tool/afuinject.h $                           */
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

#ifndef AFUINJECT_H
#define AFUINJECT_H

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <afu_fc.h>

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
    int16_t device;
    char target_device[128];
    int16_t read;
    int16_t write;
    int16_t reg;
    char register_offset[128];
    int16_t data_to_write;
    char data[128];
    int16_t verbose;
};

/*@}*/   // Ending tag for external structure module in doxygen

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
extern struct arguments g_args;

/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/
error_t parse_opt(int key, char * arg, struct argp_state * state);
void * mmap_afu_registers(int afu_master_fd);
void munmap_afu_registers(void * ps_regs); 
uint8_t * get_afu_psa_addr(char * i_afu_path); 
int write_afu_register(uint8_t * afu_psa_addr, int reg, uint64_t mmio_data); 
int read_afu_register(uint8_t * afu_psa_addr, int reg);

#endif //AFUINJECT_H
