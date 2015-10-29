/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/provextstructs.h $                                */
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
/**
 * @file provextstructs.h
 * @brief Contains all external structures and types required for provisioning
 *
 *
*/

#ifndef _PROVEXTSTRUCTS_H
#define _PROVEXTSTRUCTS_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <afu_fc.h>
/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

/*@{*/   // Special tag to say everything between it and the ending
         // brace is a part of the #defines in doxygen.

//def size of a WWPN in bytes, including a null terminator
#define WWPN_BUFFER_LENGTH 17
//def max size of device path buffer, including a null terminator
#define DEV_PATH_LENGTH 64

/*@}*/  // Ending tag for external constants in doxygen

/*----------------------------------------------------------------------------*/
/* Enumerations                                                               */
/*----------------------------------------------------------------------------*/
/**
 * \defgroup ExternalEnum External Enumerations
 */
/*@{*/   // Special tag to say everything between it and the ending
       // brace is a part of the external enum module in doxygen.

typedef struct prov_adapter_info
{
    char                    wwpn[MAX_WWPNS_PER_ADAPTER][WWPN_BUFFER_LENGTH];
    char                    pci_path[DEV_PATH_LENGTH];
    char                    afu_name[DEV_PATH_LENGTH];
}prov_adapter_info_t;

/**
 * @struct prov_wwpn_info_t
 * @brief typedef struct that provides a given WWPN for provisioning
 */
typedef struct prov_wwpn_info
{
    char                    wwpn[WWPN_BUFFER_LENGTH];       /** ASCII representation of the WWPN (16 char + a null) */
    AFU_PORT_ID             port_id;                        /** What port is this WWPN describing? */
    char                    afu_path[DEV_PATH_LENGTH];      /** CXL Device that owns this WWPN */
    char                    pci_path[DEV_PATH_LENGTH];      /** PCI device that owns this WWPN */
}prov_wwpn_info_t;


/*@}*/  // Ending tag for external structure module in doxygen




#endif //_PROVEXTSTRUCTS_H
