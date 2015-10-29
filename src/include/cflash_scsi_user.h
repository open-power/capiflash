/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_scsi_user.h $                              */
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
#ifndef _H_CFLASH_SCSI_USER
#define _H_CFLASH_SCSI_USER

#include <strings.h>
#include <inttypes.h>
#include <cflash_tools_user.h>
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif /* !_AIX && !_MACOSX */


/*****************************************************************************/
/*                                                                           */
/* SCSI Op codes                                                             */
/*                                                                           */
/*****************************************************************************/

#define SCSI_INQUIRY                0x12
#define SCSI_MODE_SELECT            0x15
#define SCSI_MODE_SELECT_10         0x55
#define SCSI_MODE_SENSE             0x1A
#define SCSI_MODE_SENSE_10          0x5A
#define SCSI_PERSISTENT_RESERVE_IN  0x5E
#define SCSI_PERSISTENT_RESERVE_OUT 0x5F
#define SCSI_READ                   0x08
#define SCSI_READ_16                0x88
#define SCSI_READ_CAPACITY          0x25
#define SCSI_READ_EXTENDED          0x28
#define SCSI_REPORT_LUNS            0xA0
#define SCSI_REQUEST_SENSE          0x03
#define SCSI_SERVICE_ACTION_IN      0x9E
#define SCSI_SERVICE_ACTION_OUT     0x9F
#define SCSI_START_STOP_UNIT        0x1B
#define SCSI_TEST_UNIT_READY        0x00
#define SCSI_WRITE                  0x0A
#define SCSI_WRITE_16               0x8A
#define SCSI_WRITE_AND_VERIFY       0x2E
#define SCSI_WRITE_AND_VERIFY_16    0x8E
#define SCSI_WRITE_EXTENDED         0x2A
#define SCSI_WRITE_SAME             0x41
#define SCSI_WRITE_SAME_16          0x93




/*
  A simple subset of the ANSI SCSI SERVICE ACTION
  IN codes (in alphabetical order)
  */
#define SCSI_READ_CAP16_SRV_ACT_IN  0x10



#define SCSI_MODE_SNS_CHANGEABLE    0x1

/*
 * SCSI CDB structure
 */
typedef struct scsi_cdb {            /* structure of the SCSI cmd block */
        uint8_t  scsi_op_code;       /* first byte of SCSI cmd block    */
        uint8_t  scsi_bytes[15];     /* other bytes of SCSI cmd block   */
} scsi_cdb_t;



/*
 *
 *
 *                        READ(16) Command                                 
 *  +=====-======-======-======-======-======-======-======-======+        
 *  |  Bit|   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |        
 *  |Byte |      |      |      |      |      |      |      |      |        
 *  |=====+=======================================================|        
 *  | 0   |                Operation Code (88h)                   |        
 *  |-----+-------------------------------------------------------|        
 *  | 1   |                    | DPO  | FUA  | Reserved    |RelAdr|        
 *  |-----+-------------------------------------------------------|        
 *  | 2   | (MSB)                                                 |        
 *  |-----+---                                                 ---|        
 *  | 3   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 4   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 5   |             Logical Block Address                     |        
 *  |-----+---                                                 ---|        
 *  | 6   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 7   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 8   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 9   |                                                 (LSB) |        
 *  |-----+-------------------------------------------------------|        
 *  | 10  | (MSB)                                                 |        
 *  |-----+---                                                 ---|        
 *  | 11  |                                                       |        
 *  |-----+---              Transfer Length                    ---|        
 *  | 12  |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 13  |                                                 (LSB) |        
 *  |-----+-------------------------------------------------------|        
 *  | 14  |                    Reserved                           |        
 *  |-----+-------------------------------------------------------|        
 *  | 15  |                    Control                            |        
 *  +=============================================================+        
 *                                                                        
 *
 *                        WRITE(16) Command                                 
 *  +=====-======-======-======-======-======-======-======-======+        
 *  |  Bit|   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |        
 *  |Byte |      |      |      |      |      |      |      |      |        
 *  |=====+=======================================================|        
 *  | 0   |                Operation Code (8Ah)                   |        
 *  |-----+-------------------------------------------------------|        
 *  | 1   |                    | DPO  | FUA  | Reserved    |RelAdr|        
 *  |-----+-------------------------------------------------------|        
 *  | 2   | (MSB)                                                 |        
 *  |-----+---                                                 ---|        
 *  | 3   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 4   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 5   |             Logical Block Address                     |        
 *  |-----+---                                                 ---|        
 *  | 6   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 7   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 8   |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 9   |                                                 (LSB) |        
 *  |-----+-------------------------------------------------------|        
 *  | 10  | (MSB)                                                 |        
 *  |-----+---                                                 ---|        
 *  | 11  |                                                       |        
 *  |-----+---              Transfer Length                    ---|        
 *  | 12  |                                                       |        
 *  |-----+---                                                 ---|        
 *  | 13  |                                                 (LSB) |        
 *  |-----+-------------------------------------------------------|        
 *  | 14  |                    Reserved                           |        
 *  |-----+-------------------------------------------------------|        
 *  | 15  |                    Control                            |        
 *  +=============================================================+        
 *                                                                             
 *                                                                        
 */
static inline void CFLASH_BUILD_RW_16(scsi_cdb_t *scsi_cdb,uint64_t lba,
			       uint32_t blk_cnt)
{
    uint64_t *lba_offset;

    scsi_cdb->scsi_bytes[0] = 0x00;

    /* Set LBA */
    lba_offset = (uint64_t *)&scsi_cdb->scsi_bytes[1];
    *lba_offset = CFLASH_TO_ADAP64(lba);

    /* Set transfer size */
    scsi_cdb->scsi_bytes[9] = ((blk_cnt >> 24) & 0xff);
    scsi_cdb->scsi_bytes[10] = ((blk_cnt >> 16) & 0xff);
    scsi_cdb->scsi_bytes[11] = ((blk_cnt >> 8) & 0xff);
    scsi_cdb->scsi_bytes[12] = (blk_cnt & 0xff);

    scsi_cdb->scsi_bytes[13] = 0x00;

    return;
}

#define SCSI_INQSIZE 256

/* 
 *                    Standard SCSI-3 INQUIRY data format
 * +=====-=======-=======-=======-=======-=======-=======-=======-=======+
 * |  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 * |Byte |       |       |       |       |       |       |       |       |
 * |=====+=======================+=======================================|
 * | 0   | Peripheral qualifier  |           Peripheral device type      |
 * |-----+---------------------------------------------------------------|
 * | 1   |  RMB  |                  Device-type modifier                 |
 * |-----+---------------------------------------------------------------|
 * | 2   |   ISO version |       ECMA version    |  ANSI-approved version|
 * |-----+-----------------+---------------------------------------------|
 * | 3   |  AERC | TrmTsk|NormACA|Reserve|         Response data format  |
 * |-----+---------------------------------------------------------------|
 * | 4   |                           Additional length (n-4)             |
 * |-----+---------------------------------------------------------------|
 * | 5   |                           Reserved                            |
 * |-----+---------------------------------------------------------------|
 * | 6   |Reserve|EncServ|  Port | DualP | MChngr|ACKQREQ|Addr32 |Addr16 |
 * |-----+---------------------------------------------------------------|
 * | 7   | RelAdr| WBus32| WBus16|  Sync | Linked|TranDis| CmdQue| SftRe |
 * |-----+---------------------------------------------------------------|
 * | 8   | (MSB)                                                         |
 * |- - -+---                        Vendor identification            ---|
 * | 15  |                                                         (LSB) |
 * |-----+---------------------------------------------------------------|
 * | 16  | (MSB)                                                         |
 * |- - -+---                        Product identification           ---|
 * | 31  |                                                         (LSB) |
 * |-----+---------------------------------------------------------------|
 * | 32  | (MSB)                                                         |
 * |- - -+---                        Product revision level           ---|
 * | 35  |                                                         (LSB) |
 * |-----+---------------------------------------------------------------|
 * | 36  |                                                               |
 * |- - -+---                        Vendor-specific                  ---|
 * | 55  |                                                               |
 * |-----+---------------------------------------------------------------|
 * | 56  |                                                               |
 * |- - -+---                        Reserved                         ---|
 * | 95  |                                                               |
 * |=====+===============================================================|
 * |     |                       Vendor-specific parameters              |
 * |=====+===============================================================|
 * | 96  |                                                               |
 * |- - -+---                        Vendor-specific                  ---|
 * | n   |                                                               |
 * +=====================================================================+
 */
struct  inqry_data {
        uint8_t   pdevtype;     /* Peripherial device/qualifier     */

#define SCSI_PDEV_MASK  0x1F   /* Mask to extract Peripheral device*/
                               /* type from byte 0 of Inquiry data */
#define SCSI_DISK       0x00   /* Peripheral Device Type of SCSI   */
			       /* Disk.                            */

        uint8_t   rmbdevtq;
        uint8_t   versions;    /* Versions field                   */
        uint8_t   resdfmt;

#define NORMACA_BIT     0x20   /* Mask to extract NormACA bit      */
                               /* from standard inquiry data       */
        uint8_t   add_length;  /* Additional length                */
        uint8_t   reserved;
        uint8_t   flags1;
#define SES_MSK     0x40       /* Mask to extract the EncServ bit  */ 
        uint8_t   flags2;
#define CMDQUE_BIT      0x02   /* Mask to extract cmdque bit       */
                               /* from standard inquiry data       */
        char    vendor[8];     /* Vendor ID                        */
        char    product[8];    /* Product ID                       */
        char    res[12];
        char    sno[8];
        char    misc[212];
};



/************************************************************************/
/* Device ID Code Page defines used for extracting WWID                 */
/************************************************************************/
#define CFLASH_INQ_DEVICEID_CODE_PAGE            0x83
#define CFLASH_INQ_DEVICEID_PAGELEN_OFFSET          3
#define CFLASH_INQ_DEVICEID_IDESC_HEADER_SIZE       4
#define CFLASH_INQ_DEVICEID_IDDESC_LIST_OFFSET      4
#define CFLASH_INQ_DEVICEID_IDDESC_CODESET_OFFSET   0
#define CFLASH_INQ_DEVICEID_IDDESC_CODESET_MASK   0xF
#define CFLASH_INQ_DEVICEID_IDDESC_CODESET_BIN      1
#define CFLASH_INQ_DEVICEID_IDDESC_CODESET_ASCII    2
#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_OFFSET    1
#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_MASK    0xF
#define CFLASH_INQ_DEVICEID_IDDESC_ASSOC_OFFSET     1
#define CFLASH_INQ_DEVICEID_IDDESC_ASSOC_MASK     0x30

#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NOAUTH    0

#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_VENDOR_PLUS    1
#define CFLASH_INQ_DEVICEID_IDDESC_VENDOR_PORTION_LEN    8

#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_EUI64     2
#define CFLASH_INQ_DEVICEID_IDDESC_EUI64_LEN        8

#define CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NAA  3
#define CFLASH_INQ_DEVICEID_IDDESC_NAA_LEN     8

#define CFLASH_INQ_DEVICEID_IDDESC_ASSOC_LUN    0x00
#define CFLASH_INQ_DEVICEID_IDDESC_ASSOC_PORT   0x10
#define CFLASH_INQ_DEVICEID_IDDESC_ASSOC_TARGET 0x20

#define CFLASH_INQ_DEVICEID_IDDESC_IDLEN_OFFSET     3
#define CFLASH_INQ_DEVICEID_IDDESC_IDENT_OFFSET     4

/* 
 *                    Standard SCSI-3 Report Luns data header
 * +=====-=======-=======-=======-=======-=======-=======-=======-=======+
 * |  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 * |Byte |       |       |       |       |       |       |       |       |
 * |=====+=======================+=======================================|
 * | 0   | (MSB)                                                         |
 * |-----+---                      Lun List Length                    ---|
 * | 1   |                                                               |
 * |-----+---                                                         ---|
 * | 2   |                                                               | 
 * |-----+---                                                         ---|
 * | 3   |                                                         (LSB) | 
 * |-----+---------------------------------------------------------------|
 * | 4   |                                                               |
 * |-----+----                       Reserved                     -------|
 * | 7   |                                                               |
 * +---------------------------------------------------------------------+
 */

struct lun_list_hdr {
        uint32_t   lun_list_length;
        uint32_t   resvd;
};
#define START_LUNS 8              /* The offset in the report luns    */
                                  /* where the actual lun list starts.*/


/************************************************************************/
/* Read Capacity 10 returned data                                       */
/************************************************************************/
struct readcap_data {
    uint32_t   lba;             /* last logical block address */
    int        len;             /* block length in bytes      */
};


/************************************************************************/
/* Read Capacity 16 returned data                                       */
/************************************************************************/
struct readcap16_data {
    uint64_t     lba;          /* last logical block address */
    int          len;          /* block length in bytes      */
    uint32_t reserved:4;       /* reserved field             */
    int     p_type:3;          /* The T10 protection type to */
                               /* which this lun is formatted*/
    
    int    prot_en:1;          /* Indicates the protection   */
                               /* is enabled or not          */
    uint8_t reserved1[19];     /* Reserved                   */
};



/************************************************************************/
/* SCSI Status                                                          */
/************************************************************************/
#define SCSI_GOOD_STATUS          0x00 /* target completed successfully */
#define SCSI_CHECK_CONDITION      0x02 /* target is reporting an error, 
                                         exception, or abnormal condition */
#define SCSI_BUSY_STATUS          0x08 /* target is busy and cannot accept
                                        a command from initiator */
#define SCSI_INTMD_GOOD           0x10 /* intermediate status good when using
                                         linked commands */

#define SCSI_RESERVATION_CONFLICT 0x18 /* attempted to access a LUN which is
                                        reserved by another initiator */
#define SCSI_COMMAND_TERMINATED   0x22 /* Command has been terminated by
                                        the device. */
#define SCSI_QUEUE_FULL           0x28 /* Device's command queue is full */
#define SCSI_ACA_ACTIVE           0x30 /* The device has an ACA condition
                                        that requires a Clear ACA task 
                                        management request to clear. */
#define SCSI_TASK_ABORTED         0x40 /* The device has aborted a command */


/************************************************************************/
/* Request (Auto) Sense Data Block                                      */
/************************************************************************/
/*
 *          Error Codes 70h and 71h Fixed Sense Data Format
 * +=====-======-======-======-======-======-======-======-======+
 * |  Bit|   7  |   6  |   5  |   4  |   3  |   2  |   1  |   0  |
 * |Byte |      |      |      |      |      |      |      |      |
 * |=====+======+================================================|
 * | 0   | Valid|          Error Code (70h or 71h)               |
 * |-----+-------------------------------------------------------|
 * | 1   |                 Segment Number                        |
 * |-----+-------------------------------------------------------|
 * | 2   |Filema|  EOM |  ILI |Reserv|     Sense Key             |
 * |-----+-------------------------------------------------------|
 * | 3   | (MSB)                                                 |
 * |- - -+---              Information                        ---|
 * | 6   |                                                 (LSB) |
 * |-----+-------------------------------------------------------|
 * | 7   |                 Additional Sense Length (n-7)         |
 * |-----+-------------------------------------------------------|
 * | 8   | (MSB)                                                 |
 * |- - -+---              Command-Specific Information       ---|
 * | 11  |                                                 (LSB) |
 * |-----+-------------------------------------------------------|
 * | 12  |                 Additional Sense Code                 |
 * |-----+-------------------------------------------------------|
 * | 13  |                 Additional Sense Code Qualifier       |
 * |-----+-------------------------------------------------------|
 * | 14  |                 Field Replaceable Unit Code           |
 * |-----+-------------------------------------------------------|
 * | 15  |  SKSV|                                                |
 * |- - -+------------     Sense-Key Specific                 ---|
 * | 17  |                                                       |
 * |-----+-------------------------------------------------------|
 * | 18  |                                                       |
 * |- - -+---              Additional Sense Bytes             ---|
 * | n   |                                                       |
 * +=============================================================+
 *
 *  Structure for Fixed Sense Data Format
 */

struct request_sense_data  {
    uint8_t     err_code;        /* error class and code   */
    uint8_t     rsvd0;
    uint8_t     sense_key;
#define CFLSH_NO_SENSE              0x00
#define CFLSH_RECOVERED_ERROR       0x01
#define CFLSH_NOT_READY             0x02
#define CFLSH_MEDIUM_ERROR          0x03
#define CFLSH_HARDWARE_ERROR        0x04
#define CFLSH_ILLEGAL_REQUEST       0x05
#define CFLSH_UNIT_ATTENTION        0x06
#define CFLSH_DATA_PROTECT          0x07
#define CFLSH_BLANK_CHECK           0x08
#define CFLSH_VENDOR_UNIQUE         0x09
#define CFLSH_COPY_ABORTED          0x0A
#define CFLSH_ABORTED_COMMAND       0x0B
#define CFLSH_EQUAL_CMD             0x0C
#define CFLSH_VOLUME_OVERFLOW       0x0D
#define CFLSH_MISCOMPARE            0x0E

    uint8_t     sense_byte0;
    uint8_t     sense_byte1;
    uint8_t     sense_byte2;
    uint8_t     sense_byte3;
    uint8_t     add_sense_length;
    uint8_t     add_sense_byte0;
    uint8_t     add_sense_byte1;
    uint8_t     add_sense_byte2;
    uint8_t     add_sense_byte3;
    uint8_t     add_sense_key;
    uint8_t     add_sense_qualifier;
    uint8_t     fru;
    uint8_t     flag_byte;
    uint8_t     field_ptrM;
    uint8_t     field_ptrL;
};


/************************************************************************/
/* Function prototypes                                                  */
/************************************************************************/
int cflash_build_scsi_inquiry(scsi_cdb_t *scsi_cdb, int page_code,uint8_t data_size);
int cflash_process_scsi_inquiry(void *std_inq_data,int inq_data_len,
				void *vendor_id,void *product_id);
int cflash_process_scsi_inquiry_dev_id_page(void *inqdata,int inq_data_len,char *wwid);
int cflash_build_scsi_tur(scsi_cdb_t *scsi_cdb);
int cflash_build_scsi_report_luns(scsi_cdb_t *scsi_cdb, uint32_t length_list);
int cflash_process_scsi_report_luns(void *lun_list_rsp, uint32_t length_list,
				    uint64_t **actual_lun_list, int *num_actual_luns);
int cflash_build_scsi_mode_sense_10(scsi_cdb_t *scsi_cdb,
				    uint16_t data_len, int flags);
int cflash_build_scsi_mode_select_10(scsi_cdb_t *scsi_cdb,
				     uint16_t data_len, int flags);
int cflash_build_scsi_read_cap(scsi_cdb_t *scsi_cdb);
int cflash_build_scsi_read_cap16(scsi_cdb_t *scsi_cdb, uint8_t data_len);
int cflash_process_scsi_read_cap16(struct readcap16_data *readcap16_data, uint32_t *block_size,
				   uint64_t *last_lba);
int cflash_build_scsi_request_sense(scsi_cdb_t *scsi_cdb, uint8_t data_len);

#endif /* _H_CFLASH_SCSI_USER */
