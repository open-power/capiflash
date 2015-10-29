/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlflashutil.h $                                   */
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

#ifndef _CXLFLASHUTIL_H
#define _CXLFLASHUTIL_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <argp.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <scsi/cxlflash_ioctl.h>

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/



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
    char                        target_device[DEV_STRING_SZ];
    uint8_t                     set_mode;
    uint8_t                     configure_default;
    uint8_t                     target_mode;
    uint8_t                     get_mode;
    uint8_t                     verbose;
    bool                        wwid_valid;
    uint8_t                     wwid[DK_CXLFLASH_MANAGE_LUN_WWID_LEN];
};

/*@}*/  // Ending tag for external structure module in doxygen


/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/




#endif //_CXLFLASHUTIL_H

