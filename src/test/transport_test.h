/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/transport_test.h $                                   */
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
//#define _OLD_CODE 1
#include <cflash_scsi_user.h>
#include <cflash_tools_user.h>
#include <cflash_mmio.h>
#include <cxl.h>
#ifndef _OLD_CODE
#include <sislite.h>
#endif /* _OLD_CODE */

#define CAPI_FLASH_REG_SIZE 0x2000000

#define CAPI_SCSI_IO_TIME_OUT  5


typedef
enum {
    TRANSP_READ_CMD     = 0x1,  /* SCSI Read 16 Command               */
    TRANSP_WRITE_CMD    = 0x2,  /* SCSI Write 16 Command              */
    TRANSP_STD_INQ_CMD  = 0x3,  /* SCSI Inquiry standard data         */
    TRANSP_PG83_INQ_CMD = 0x4,  /* SCSI Inquiry  page 0x83 data       */
    TRANSP_RD_CAP_CMD   = 0x5,  /* SCSI Read Capacity 16 command      */
    TRANSP_RPT_LUNS_CMD = 0x6,  /* SCSI Report Luns command           */
    TRANSP_TUR_CMD      = 0x7,  /* SCSI Test Unit Ready command       */
    TRANSP_MSENSE_CMD   = 0x8,  /* SCSI Mode Sense 10 command         */
    TRANSP_MSELECT_CMD  = 0x9,  /* SCSI Mode Select 10 command        */
    TRANSP_REQSNS_CMD   = 0xa,  /* SCSI Request Sense command         */
    TRANSP_LAST_CMD     = 0xb,  /* Not valid command                  */
} transp_scsi_cmd_t;

/*----------------------- SIS lite header  stuff ----------------------------*/


#ifdef _OLD_CODE
/************************************************************************/
/* CAPI Flash SIS LITE Register offsets                                 */
/************************************************************************/

#define CAPI_IOARRIN_OFFSET       0x20 /* Offset of IOARRIN register          */
#define CAPI_RRQ0_START_EA_OFFSET 0x28 /* Offset of RRQ # 0 start EA register */
#define CAPI_RRQ0_END_EA_OFFSET   0x30 /* Offset of RRQ # 0 Last EA register  */

#define SISL_RESP_HANDLE_T_BIT        0x1ull /* Toggle bit */

#ifdef _MACOSX
typedef unsigned short ushort;
#endif /* MACOSX */

typedef struct capi_ioarcb_s {
    ushort   ctx_id;           /* Context ID from context handle.  */
    ushort   req_flags;        /* Request flags                    */
#define SISL_REQ_FLAGS_RES_HNDL       0x8000u
#define SISL_REQ_FLAGS_PORT_LUN_ID    0x0000u
#define SISL_REQ_FLAGS_SUPRESS_ULEN   0x0002u
#define SISL_REQ_FLAGS_HOST_WRITE     0x0001u
#define SISL_REQ_FLAGS_HOST_READ      0x0000u

    uint32_t hndl_fc;          /* Resouorce handle for fc_port     */
    uint64_t lun_id;           /* Destination Lun ID if valid.     */
    uint32_t data_len;         /* Data length                      */
    uint32_t ioadl_len;        /* IODL Length for scatter/gather   */
			       /* list                             */
    uint64_t data_ea;          /* Effective address of data buffer */
			       /* or IOADL                         */
    uint8_t msi;               /* MSI number to interrupt when this*/
			       /* IOARCB completes                 */

    uint8_t rrq_num;           /* response queue number            */
    ushort reserved1;          /* Reserved for future use          */
    uint32_t timeout;          /* Time-out in seconds              */
    struct scsi_cdb cdb;       /* SCSI Command Descriptor Block    */
    uint64_t reserved2;        /* Reserved for future use          */
    uint32_t ioasc;            /* SIS IOA Status Code              */
    uint32_t residual_len;     /* Residual length                  */
    ushort ioasa_flags;        /* Status flags                     */
#define CAPI_IOASA_FLG_SNS_VAL 1 /* Sense Data Valid               */
    ushort fc_port;;           /* FC Port on which the request was */
                               /* issued.                          */
    char sense_data[20];       /* First 20 bytes of sense data     */
} capi_ioarcb_t;

/*----------------------- SIS lite header  stuff ----------------------------*/
#else
typedef sisl_ioarcb_t capi_ioarcb_t;
#endif /* _OLD_CODE */
