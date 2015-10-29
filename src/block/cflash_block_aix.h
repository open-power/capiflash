/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_internal.h $                           */
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

#ifndef _H_CFLASH_BLOCK_AIX
#define _H_CFLASH_BLOCK_AIX

#define CFLASH_BLOCK_EXCEP_QDEPTH  32768 /* Number of exceptions--including  */
				         /* page faults--for this context,   */
				         /* before we hold off all interrupts*/
				         /* for this context                 */

/*****************************************************************************/
/*  Error logging information                                                */
/*****************************************************************************/


#define CFLASH_BLOCK_LOG_DATA_LEN   224

struct cflash_block_log {  

 /*---------------start of first line of detail data----------------*/  
     
    uint   app_indicator;           /* Indicates the application/library    */
                                    /* associated with this.                */
#define CFLSH_BLK_LIB      0x10     /* Indicates this library will be used  */
                                    /* for this error log.                  */
    uint   errnum;                  /* 4 bytes                              */
    uint64_t rc;                    /* Return code from the failed calls    */
    uchar  type;                    /* Type of data in detail data          */
    uchar  afu_type;                /* Type of AFU.                         */
    ushort version;                 /* Version of detail data               */
#define CFLSH_BLK_LIB_VER_0     0x0 /* Version 0                            */

    int    cflsh_blk_flags;         /* Global library flags                 */
    int    num_active_chunks;       /* Number of active chunks              */
    int    num_max_active_chunks;   /* Maximum number of active             */
                                    /* chunks seen at a time.               */

/*---------------start of second line of detail data----------------*/

    int    chunk_flags;             /* Flags for this chunk                 */
    uint32_t num_active_cmds;       /* Number of active commands            */
    int     num_cmds;               /* Number of commands in                */
                                    /* in command queue                     */
    int    num_paths;               /* Number of paths                      */
    int    path_flags;              /* Flags for this path                  */
    int    path_index;              /* Path to issue command                */
    int    afu_flags;               /* Flags for this AFU                   */
    int    num_rrqs;                /* Number of RRQ elements               */


/*---------------start of third line of detail data----------------*/

    uint32_t num_act_reads;         /* Current number of reads active       */
                                    /* via cblk_read interface              */
    uint32_t num_act_writes;        /* Current number of writes active      */
                                    /* via cblk_write interface             */
    uint32_t num_act_areads;        /* Current number of async reads        */
                                    /* active via cblk_aread interface      */
    uint32_t num_act_awrites;       /* Current number of async writes       */
                                    /* active via cblk_awrite interface     */
    uint32_t max_num_act_writes;    /* High water mark on the maximum       */
                                    /* number of writes active at once      */
    uint32_t max_num_act_reads;     /* High water mark on the maximum       */
                                    /* number of reads active at once       */
    uint32_t max_num_act_awrites;   /* High water mark on the maximum       */
                                    /* number of asyync writes active       */
                                    /* at once.                             */
    uint32_t max_num_act_areads;    /* High water mark on the maximum       */
                                    /* number of asyync reads active        */
                                    /* at once.                             */
/*---------------start of fourth line of detail data----------------*/

    uint64_t num_cc_errors;         /* Total number of all check            */
                                    /* condition responses seen             */
    uint64_t num_afu_errors;        /* Total number of all AFU error        */
                                    /* responses seen                       */
    uint64_t num_fc_errors;         /* Total number of all FC               */
                                    /* error responses seen                 */
    uint64_t num_errors;            /* Total number of all error            */
                                    /* responses seen                       */
 


/*---------------start of fifth line of detail data----------------*/

    uint64_t num_reset_contexts;    /* Total number of reset contexts       */
                                    /* done                                 */

    uint64_t num_reset_contxt_fails;/* Total number of reset context        */
                                    /* failures                             */
    uint64_t num_path_fail_overs;   /* Total number of times a request      */
                                    /* has failed over to another path.     */

    uint32_t block_size;            /* Block size of this chunk.            */
    uint32_t primary_path_id;       /* Primary path id                      */


/*---------------start of sixth line of detail data----------------*/

    uint64_t num_no_cmd_room;       /* Total number of times we didm't      */
                                    /* have room to issue a command to      */
                                    /* the AFU.                             */
    uint64_t num_no_cmds_free;      /* Total number of times we didm't      */
                                    /* have free command available          */
    uint64_t num_no_cmds_free_fail; /* Total number of times we didn't      */
                                    /* have free command available and      */
                                    /* failed a request because of this     */
    uint64_t num_fail_timeouts;     /* Total number of all commmand         */
                                    /* time-outs that led to a command      */
                                    /* failure.                             */

/*---------------start of seventh line of detail data----------------*/

    uint64_t num_capi_adap_chck_err;/* Total number of all check            */
                                    /* adapter errors.                      */


    uint64_t num_capi_adap_resets;  /* Total number of all adapter          */
                                    /* reset errors.                        */

    uint64_t num_capi_data_st_errs; /* Total number of all                  */

                                    /* CAPI data storage event              */
                                    /* responses seen.                      */

    uint64_t num_capi_afu_errors;   /* Total number of all                  */
                                    /* CAPI error responses seen            */

/*---------------start of eighth line of detail data----------------*/

    scsi_cdb_t	failed_cdb;         /* CDB that failed                      */
    uint64_t    data_ea;            /* Effective address of data buffer     */
    uint64_t    data_len;           /* Length of data for this CDB          */




/*---------------start of nineth  line of detail data----------------*/


    uint64_t    lba;                /* Starting LBA                         */
    uint64_t    mmio;               /* Start address of MMIO space          */
    uint64_t    hrrq_start;         /* Start address of HRRQ                */
    uint64_t    cmd_start;          /* Start address of commands            */

    

/*---------------start of tentth  line of detail data until end ---------*/

    uchar        data[CFLASH_BLOCK_LOG_DATA_LEN];


};

#endif /* _H_CFLASH_BLOCK_AIX */
    
