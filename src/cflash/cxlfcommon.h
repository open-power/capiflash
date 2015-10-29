/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlfcommon.h $                                     */
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

#ifndef _CXLFCOMMON_H
#define _CXLFCOMMON_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <scsi/cxlflash_ioctl.h>

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

#define TRACE_ENABLED
#define KWDATA_SZ 256
#define MAX_VPD_SIZE 0x58
#define DEV_STRING_SZ 128
#define MAX_NUM_SGDEVS 4096
#define MAX_NUM_LUNS 4096


/*@}*/  // Ending tag for external constants in doxygen

/*----------------------------------------------------------------------------*/
/* Enumerations                                                               */
/*----------------------------------------------------------------------------*/
typedef enum MODE
{
    MODE_LEGACY = 0,
    MODE_SIO = 1,
    MODE_INVALID = 2
} MODE_T;

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
extern int32_t                         g_traceE;       /* error traces */
extern int32_t                         g_traceI;       /* informative 'where we are in code' traces */
extern int32_t                         g_traceF;       /* function exit/enter */
extern int32_t                         g_traceV;       /* verbose trace...lots of information */


#pragma pack(1)
typedef struct pg83header
{
    uint8_t peripherial_type;
    uint8_t page_code;
    uint8_t reserved1;
    uint8_t page_length;
    uint8_t data[1];
}pg83header_t;

typedef struct pg83data
{
    uint8_t prot_info;
    uint8_t piv_assoc_id;
    uint8_t reserved2;
    uint8_t length;
    uint8_t data[1];
}pg83data_t;

typedef struct lun_table_entry
{
    char    sgdev[DEV_STRING_SZ];
    uint8_t lun[DK_CXLFLASH_MANAGE_LUN_WWID_LEN];
    MODE_T  mode;
} lun_table_entry_t;


/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

#define TRACEE(FMT, args...) if(g_traceE) \
do \
{  \
char __data__[256];                                             \
memset(__data__,0,256);                                         \
sprintf(__data__,"%s %s: " FMT, __FILE__, __func__, ## args);   \
perror(__data__);                                               \
} while(0)

#define TRACEF(FMT, args...) if(g_traceF) \
{ \
printf("%s %s: " FMT, __FILE__, __func__ ,## args); \
}

#define TRACEI(FMT, args...) if(g_traceI) \
{ \
printf("%s %s: " FMT, __FILE__, __func__ ,## args); \
}

#define TRACEV(FMT, args...) if(g_traceV) printf("%s %s: " FMT, __FILE__, __func__ ,## args)

#define TRACED(FMT, args...) printf(FMT,## args)


/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/
uint32_t convert_to_binary (uint8_t **output_buffer,
                            uint32_t *output_buffer_length,
                            char *input_buffer);



bool cxlf_get_mode(char* target_device);
bool cxlf_set_mode(char* target_device, uint8_t target_mode, uint8_t* wwid);
bool cxlf_parse_wwid(uint8_t* o_buffer, char* i_string, uint8_t i_buffer_sz);
int extract_lun_from_vpd(const char* i_sgdevpath, uint8_t* o_lun);
int compare_luns(const void* item1, const void* item2);
int update_siotable(lun_table_entry_t* o_lun_table, int* o_lun_table_sz);
int update_cxlflash_devs(lun_table_entry_t* o_cxldevices, int* o_cxldevices_sz, lun_table_entry_t* filter_lun);
int cxlf_refresh_luns(lun_table_entry_t* i_luntable, int i_luntable_sz, lun_table_entry_t* i_sgdevs, int i_sgdevs_sz);
void printentry(lun_table_entry_t* entry);
#endif //_CXLFLASHUTIL_H

