/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/cflash_scsi_user.c $                               */
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cflash_tools_user.h>
#include <cflash_scsi_user.h>

/*
 * NAME:        cflash_build_scsi_inquiry
 *
 * FUNCTION:    Builds a SCSI Inquiry command for the specified
 *              page.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Inquiry command
 *
 *              code_page - Code page to use for Inquiry. -1 indicates
 *                          to use standard inquiry.
 *
 *              data_len  - Date length (in bytes) returned inquiry data
 *                          buffer. This can not exceed 255 bytes
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_inquiry(scsi_cdb_t *scsi_cdb, int page_code,uint8_t data_size)
{
    int rc = 0;

    if (scsi_cdb == NULL) {

	errno = EINVAL;
	return -1;
    }

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *                INQUIRY  Command
     *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *|Byte |       |       |       |       |       |       |       |       |
     *|=====+===============================================================|
     *| 0   |                       Operation Code (12h)                    |
     *|-----+---------------------------------------------------------------|
     *| 1   |               Reserved                        | CmdDT | EVPD  |
     *|-----+---------------------------------------------------------------|
     *| 2   |               Page or Operation Code                          |
     *|-----+---------------------------------------------------------------|
     *| 3   |               Reserved                                        |
     *|-----+---------------------------------------------------------------|
     *| 4   |               Allocation Length                               |
     *|-----+---------------------------------------------------------------|
     *| 5   |                           Control                             |
     *+=====================================================================+
     */
   
    scsi_cdb->scsi_op_code = SCSI_INQUIRY;
    if (page_code == -1) {

	/* Standard Inquiry */

        scsi_cdb->scsi_bytes[0] = 0x00;
        scsi_cdb->scsi_bytes[1] = 0x00;

    } else {
        scsi_cdb->scsi_bytes[0] = 0x01;
        scsi_cdb->scsi_bytes[1] = (uint8_t)(0xff & page_code);
    }

    scsi_cdb->scsi_bytes[2] = 0x00;
    scsi_cdb->scsi_bytes[3] = data_size;
    scsi_cdb->scsi_bytes[4] = 0x00;

    return rc;
}

/*
 * NAME:        cflash_process_scsi_inquiry_std_data
 *
 * FUNCTION:    Process the Standard Inquiry data
 *              
 *
 *
 * INPUTS:
 *              std_inq_data - Pointer to returned standard inquiry data.
 *
 *              inq_data_len - Length of inquiry data.
 *
 *              vendor_id    - returned vendor id found. Must be 8 bytes in 
 *                             in size.
 *
 *              product_id   - returned product id found. Must be 8 bytes in 
 *                             in size.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_process_scsi_inquiry(void *std_inq_data,int inq_data_len,
				void *vendor_id,void *product_id)
{
    int rc = 0;
    struct inqry_data *inq_data;
    uint8_t ansi_type;

    if ((vendor_id == NULL) ||
	(product_id == NULL)) {

	return EINVAL;
    }

    inq_data = std_inq_data;

    if ((inq_data->pdevtype & SCSI_PDEV_MASK) != SCSI_DISK) {

	/*
	 * This is not a disk device. Fail the request.
	 */

	return EINVAL;

    }
    ansi_type = inq_data->versions & 0x7;

    if  (!((ansi_type >= 0x3) && (inq_data->resdfmt & NORMACA_BIT) &&
	   (inq_data->flags2 & CMDQUE_BIT))) {
        /*
         * This device does not support both
	 * NACA and command tag queuing.
	 *
	 * TODO Maybe we should not impose this now, since
	 *      we are not using it.
         */
        
	return EINVAL;
    }
   

    bcopy(inq_data->vendor,vendor_id,8);

    bcopy(inq_data->product,product_id,8);

    return rc;
}

/*
 * NAME:        cflash_process_scsi_inquiry_dev_id_page
 *
 * FUNCTION:    Process the Inquiry data from the device ID
 *              code page (0x83).
 *              
 *
 *
 * INPUTS:
 *              inq_data     - Pointer to returned inquiry data.
 *
 *              inq_data_len - Length of inquiry data.
 *
 *              wwid         - returned (lun) world wide id.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_process_scsi_inquiry_dev_id_page(void *inq_data,int inq_data_len,char *wwid)
{
    char  identifier_type;
    char  *ucp          = (char *) NULL;
    char  *end_of_page  = (char *) NULL;
    char  *iddesc_p     = (char *) NULL;
    char  tmp_buf[SCSI_INQSIZE+1];
    char  *inqdata = inq_data;
    int rc              = 0,
        page_length     = 0,
        id_len          = 0,
        id_desc_size    = 0,       
        id_codeset      = 0;

    unsigned long long serial_no_64; 
    
    /* Initialize wwid to a NULL string */
    *wwid = '\0';
    
    /*
     * Obtain the page_length -- actually the amount of space
     * beyond the device id header stuff allocated for holding
     * all the ID descriptors.
     */
    page_length = (int) *(inqdata +
        CFLASH_INQ_DEVICEID_PAGELEN_OFFSET);
    
    if (page_length > (inq_data_len - CFLASH_INQ_DEVICEID_PAGELEN_OFFSET)) {
        /*
         * If the specified page_length exceeds our inquiry data, then
         * trunc it to the max available.
         */
        page_length = inq_data_len - CFLASH_INQ_DEVICEID_PAGELEN_OFFSET;
    } 
    
    /*
     * Now form a ptr. to the last offset within the code page.
     * Note that owing to the possible adjustment of page_length
     * above, end_of_page is also bounded by the size of the
     * inquiry data.
     */
    end_of_page = inqdata + page_length + 
        CFLASH_INQ_DEVICEID_PAGELEN_OFFSET;
    
    /*
     * Set our pointer to the first ID descriptor.
     */
    ucp = inqdata + CFLASH_INQ_DEVICEID_IDDESC_LIST_OFFSET;
    
    /*
     * Walk through ID descriptors, searching for ID descriptor associated
     * with the LUN.
     */

    DEBUG_PRNT("%s:  at start of walk id desc, first at 0x%08X\n",
        "sas_get_wwid",ucp);
    DEBUG_PRNT("    and end_of_page is 0x%08X\n", end_of_page);


    while (!iddesc_p && (ucp < end_of_page)) {

        DEBUG_PRNT("%s:  Looking at id_descriptor at 0x%08X\n",
            "sas_get_wwid",
            ucp);

        /*
         * Check if this ID type is one of the ones we support.
         */
        identifier_type = (char) (*(ucp + 
                  CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_OFFSET) &
                 ((char) CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_MASK));

        /*
         * Check if the the ID Descriptor is for the LUN
         */
        if ( ( (char) *(ucp + CFLASH_INQ_DEVICEID_IDDESC_ASSOC_OFFSET) &
                   CFLASH_INQ_DEVICEID_IDDESC_ASSOC_MASK) ==
                   CFLASH_INQ_DEVICEID_IDDESC_ASSOC_LUN ) {
                      
            /* ID Descriptor is for addressed LUN */

            DEBUG_PRNT ("%s: Found ID Desc for address LUN\n",
	    "sas_get_wwid");

            if ((identifier_type == 
                CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NAA)   ||
                (identifier_type == 
                CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_EUI64)   ) {
                /* If ID Descriptor is NAA Type or EUI64.  Then 
                 * store LUN ID Descriptor
                 */

                DEBUG_PRNT("ID Desc is type NAA or EUI64. type %x\n",
			   identifier_type);
                iddesc_p = ucp;
            }
        }

        /*
         * Get the overall size of this ID descriptor so we can advance
         * to the next one.
         */
        id_desc_size = CFLASH_INQ_DEVICEID_IDESC_HEADER_SIZE +
            (int) *(ucp + CFLASH_INQ_DEVICEID_IDDESC_IDLEN_OFFSET);
        ucp += id_desc_size;
    } /* end-while !iddesc_p && (ucp < end_of_page) */
    
    if (iddesc_p) {
        /* Found a valid iddesc_p, update ww_id attribute with value */

        DEBUG_PRNT("%s: updating %llx,%llx \n",
                "sas_get_wwid", sid, lun);

        /*
         * Grab the id length.
         */
        id_len = (int)*(iddesc_p + 
                        CFLASH_INQ_DEVICEID_IDDESC_IDLEN_OFFSET);

        DEBUG_PRNT("%s:  id_len = %d = 0x%x\n", "sas_get_wwid", id_len, id_len);

        /*
         * The format (as indicated by id_codeset) of the identifier
         * will be either in binary or ascii.
         */
        id_codeset = (int) *(iddesc_p +
                        CFLASH_INQ_DEVICEID_IDDESC_CODESET_OFFSET) &
                        CFLASH_INQ_DEVICEID_IDDESC_CODESET_MASK;
        
        /*
         * Ensure that identifier isn't specified incorrectly such that
         * the length would put us past the end of the entire code page.
         */
        if ((iddesc_p + id_len +
            CFLASH_INQ_DEVICEID_IDDESC_IDENT_OFFSET - 1) > end_of_page) {

            DEBUG_PRNT("%s:  id_len %d puts us past the entire code page\n",
                "sas_get_wwid", id_len);

            return(EINVAL);
        }

        /*
         * Ensure that the id_len isn't too long for the target buf.
         */
        if (id_codeset == CFLASH_INQ_DEVICEID_IDDESC_CODESET_ASCII) {
            if ((id_len+1) > SCSI_INQSIZE) {

                 DEBUG_PRNT("%s: code page 0x83 ASCII id_len %d > INQ_SIZE\n",
                    "sas_get_wwid", id_len);

            return(EINVAL);
            }
        }
        else {
            /*
             * Code is binary, so representing in ASCII will require
             * two bytes per binary byte.
             */
            if (((id_len * 2)+1) > SCSI_INQSIZE) {

                DEBUG_PRNT("%s: code page 0x83 BINARY id_len %d * 2 > INQ_SIZE\n",
			   "sas_get_wwid", id_len);

                return(EINVAL);
            }
        } 

        DEBUG_PRNT("%s:  identifer_type = %d = 0x%x\n", "sas_get_wwid", 
                identifier_type, identifier_type);

        
        /*
         * The following ID types have a specified length.  Sanity check
         * that they conform to the spec.
         */
        if (identifier_type == CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_EUI64) {
            /*
             * Given the particular identifier types satisfied by the
             * condition above, we assume that the data herein is
             * ALWAYS binary and of a fixed-length called out in
             * the scsi spec.
             */


            if (sizeof(serial_no_64) != CFLASH_INQ_DEVICEID_IDDESC_EUI64_LEN) {
                return(EINVAL);
            }

        }
        else {
            if ((identifier_type !=
                   CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NOAUTH) &&
                 (identifier_type !=
                   CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_VENDOR_PLUS) &&
                 (identifier_type !=
                   CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NAA)) {

                DEBUG_PRNT("%s:For code page 0x83, dont support idtype 0x%02x\n",
			   "sas_get_wwid", identifier_type);

                return(EINVAL);
            }
        }
        
        switch(id_codeset) {
            case CFLASH_INQ_DEVICEID_IDDESC_CODESET_BIN:
                /*
                 * Data is binary.
                 */
                if ((identifier_type ==
                        CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NOAUTH) ||
                    (identifier_type ==
                        CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_VENDOR_PLUS) ||
                    (identifier_type ==
                        CFLASH_INQ_DEVICEID_IDDESC_IDTYPE_NAA) ) {

                    int        j;
                    char      *ucp2;


                    ucp = iddesc_p + CFLASH_INQ_DEVICEID_IDDESC_IDENT_OFFSET;
                    tmp_buf[0] = '\0';
                    ucp2 = tmp_buf;
                    for (j = 0; j < id_len; j++, ucp++) {
                        ucp2 += sprintf(ucp2,"%02x", *ucp);
                    }
                    *ucp2 = '\0';
                    strcpy(wwid, tmp_buf);
                }
                else {
                    memcpy((void *) &serial_no_64,
                        (void *) (iddesc_p +
                           CFLASH_INQ_DEVICEID_IDDESC_IDENT_OFFSET),
                        (size_t) id_len);

                    sprintf(wwid,"%016llx",serial_no_64);
                }
                break;

            case CFLASH_INQ_DEVICEID_IDDESC_CODESET_ASCII:
                strncpy(wwid, iddesc_p + 
                        CFLASH_INQ_DEVICEID_IDDESC_IDENT_OFFSET,
                        id_len);
                if (strlen(wwid) < 1) {
                    /*
                     * For some reason the src string was all blanks
                     * so just set it to empty.
                     */
                    *wwid = '\0';
                }
                break;

            default:
                /*
                 * Unknown id_codeset.
                 */
                return(EINVAL);
        }
    }
          
    return rc;
}


/*
 * NAME:        cflash_build_scsi_tur
 *
 * FUNCTION:    Builds a SCSI Test Unit Ready command.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Test Unit Read
 *			    command.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_tur(scsi_cdb_t *scsi_cdb)
{
    int rc = 0;

    if (scsi_cdb == NULL) {

	errno = EINVAL;
	return -1;
    }

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*                  TEST UNIT READY Command
     *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *|Byte |       |       |       |       |       |       |       |       |
     *|=====+===============================================================|
     *| 0   |                           Operation Code (00h)                |
     *|-----+---------------------------------------------------------------|
     *| 1   | Logical Unit Number   |                  Reserved             |
     *|-----+---------------------------------------------------------------|
     *| 2   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 3   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 4   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 5   |                           Control                             |
     *+=====================================================================+
     */

    scsi_cdb->scsi_op_code = SCSI_TEST_UNIT_READY;

    return rc;
}

/*
 * NAME:        cflash_build_scsi_report_luns
 *
 * FUNCTION:    Builds a SCSI Report Luns command.
 *
 *
 * INPUTS:
 *              cdb          - Pointer to SCSI CDB to contain Report Luns command
 *
 *              length_list  - Length List (in bytes) of returned list of luns

 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_report_luns(scsi_cdb_t *scsi_cdb, uint32_t length_list)
{
    int rc = 0;

    if (scsi_cdb == NULL) {

	errno = EINVAL;
	return -1;
    }


    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     * 
     *                       REPORT LUNS command
     *  +====-======-======-======-======-======-======-======-======+
     *  | Bit|  7   |  6   |   5  |   4  |   3  |   2  |   1  |   0  |
     *  |Byte|      |      |      |      |      |      |      |      |
     *  |=====+======================================================|
     *  | 0   |               Operation code (A0h)                   |
     *  |-----+------------------------------------------------------|
     *  | 1   |               Reserved                               |
     *  |-----+--                                                 ---|
     *  | 5   |                                                      |
     *  |-----+------------------------------------------------------|
     *  | 6   |(MSB)                                                 |
     *  |- - -+- -            Allocation length                   - -|
     *  | 9   |                                                 (LSB)|
     *  |-----+------------------------------------------------------|
     *  | 10  |               Reserved                               |
     *  |-----+------------------------------------------------------|
     *  | 11  |               Control                                |
     *  +============================================================+

     */
   
    scsi_cdb->scsi_op_code = SCSI_REPORT_LUNS;

    scsi_cdb->scsi_bytes[5] = (length_list >>24) & 0xff;
    scsi_cdb->scsi_bytes[6] = (length_list >> 16) & 0xff;
    scsi_cdb->scsi_bytes[7] = (length_list >> 8) & 0xff;
    scsi_cdb->scsi_bytes[8] = length_list & 0xff;


    return rc;
}


/*
 * NAME:        cflash_process_scsi_report_luns
 *
 * FUNCTION:    Processes the returned lun list from the SCSI
 *              Report Luns command.
 *
 *
 * INPUTS:
 *              lun_list_rsp    - Pointer to lin list returned from Report 
 *                                Luns command.
 *
 *		length_list     - List (in bytes) of the lun_list_rsp buffer.
 *
 *              actual_lun_list - List of Lun ids returned (header removed)
 *                                It is assumed this buffer is large enough
 *                                hold the full number of lun_ids that
 *                                can fit in the returned lun_list_rsp.
 *
 *              num_actual_luns - Number of lun ids in the actual_lun_list.
 *                                
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_process_scsi_report_luns(void *lun_list_rsp, uint32_t length_list,
				    uint64_t **actual_lun_list, int *num_actual_luns)
{
    int rc = 0;
    char *lun_table;                        /* data returned from Report   */
                                            /* Luns command.               */
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    uint64_t *lun_ptr;
    int i;
#endif /* CFLASH_LITTLE_ENDIAN_HOST */

    struct lun_list_hdr *list_hdr;          /* Header of data returned from*/
                                            /* Report Luns command.        */



    if (lun_list_rsp == NULL) {

	errno = EINVAL;
	return -1;
    }

    /*
     * Report luns command was successful.
     * Now verify if the list contains all valid
     * luns for this device.
     */


    /*
     * SCSI-3 (SPC Rev 8) LUN reporting parameter list format
     * +=====-====-====-====-====-====-====-====-====+
     * |  Bit| 7  | 6  | 5  | 4  |  3 |  2 | 1  | 0  |
     * |Byte |    |    |    |    |    |    |    |    |
     * |=====+=======================================|
     * | 0   | (MSB)                                 |
     * |- - -+- -  LUN list length (n-7)          - -|
     * | 3   |                                 (LSB) |
     * |-----+---------------------------------------|
     * | 4   |                                       |
     * |- - -+- -       Reserved                  - -|
     * | 7   |                                       |
     * |=====+=======================================|
     * |     |          LUN list                     |
     * |=====+=======================================|
     * | 8   | (MSB)                                 |
     * |- - -+- -       LUN                       - -|
     * | 15  |                                 (LSB) |
     * |-----+---------------------------------------|
     * |     |               .                       |
     * |     |               .                       |
     * |     |               .                       |
     * |-----+---------------------------------------|
     * | n-7 | (MSB)                                 |
     * |- - -+- -        LUN                      - -|
     * | n   |                                 (LSB) |
     * +=============================================+
     */


    list_hdr = lun_list_rsp;
    lun_table = lun_list_rsp;



    if ((CFLASH_FROM_ADAP32(list_hdr->lun_list_length)) > length_list ) {
	/*
	 * Lun table does not contain all lun ids.
	 * So retry the Report Luns command again with
	 * a lun table of the correct length.
	 * NOTE: In the situation where the lun list allocated
	 * is not big enough for all luns, the returned
	 * lun_list_length will indicate how big it should be.
	 */

	return EAGAIN;
    } else {

	/*
	 * lun table contains all lun ids that we
	 * could extract for this SCSI id. So copy them
	 * out for the caller.
	 *
	 * NOTE: list_hdr->lun_list_length is the length
	 *       in bytes of the lun list after the 
	 *       header.
	 */

	if ((CFLASH_FROM_ADAP32(list_hdr->lun_list_length)) == 0) {


	    /*
	     * No luns found.
	     */
	    
	    *actual_lun_list = NULL;
	    rc = ENODEV;
	    

	} else if ((CFLASH_FROM_ADAP32(list_hdr->lun_list_length)) >  sizeof(*list_hdr)) {

	    /*
	     * Return the list of luns after the header to the
	     * the caller if it is a multiple of 8 in length
	     *
	     * NOTE: We return the number of valid lun ids found.
	     *       If lun_list_length is not a multiple of 8,
	     *       then the last fragment is not counted as lun id.
	     */


	    *num_actual_luns = (CFLASH_FROM_ADAP32(list_hdr->lun_list_length)) / sizeof(uint64_t);

	    if ((CFLASH_FROM_ADAP32(list_hdr->lun_list_length)) % sizeof(uint64_t)) {

		*actual_lun_list = NULL;
		rc = EINVAL;
	    } else {
		*actual_lun_list = (uint64_t *)&lun_table[START_LUNS];
#ifdef CFLASH_LITTLE_ENDIAN_HOST
		lun_ptr = (uint64_t *)&lun_table[START_LUNS];
		for (i=0; i < *num_actual_luns; i++) {
		    lun_ptr[i] = CFLASH_FROM_ADAP64(lun_ptr[i]);
		
		}
#endif /* CFLASH_LITTLE_ENDIAN_HOST */
	    }
	} else {

	    *actual_lun_list = NULL;
	}

    }

    return rc;
}

/*
 * NAME:        cflash_build_scsi_mode_sense_10
 *
 * FUNCTION:    Builds a SCSI Mode Sense 10 command.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Mode Sense 10 command
 *
 *              data_len  - Date length (in bytes) returned mode sense data
 *                          buffer. This can not exceed 64K bytes
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_mode_sense_10(scsi_cdb_t *scsi_cdb,
				    uint16_t data_len, int flags)
{
    int rc = 0;

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *                            MODE SENSE(10)  Command
     *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *|Byte |       |       |       |       |       |       |       |       |
     *|=====+===============================================================|
     *| 0   |                       Operation Code (5Ah)                    |
     *|-----+---------------------------------------------------------------|
     *| 1   |  Resvd                        | DBD   |       Reserved        |
     *|-----+---------------------------------------------------------------|
     *| 2   |      PC       |         Page Code                             |
     *|-----+---------------------------------------------------------------|
     *| 3   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 4   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 5   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 6   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 7   | (MSB)                                                         |
     *|-----+--                      Allocation Length                   ---|
     *| 8   |                                                         (LSB) |
     *|-----+---------------------------------------------------------------|
     *| 9   |                           Control                             |
     *+=====================================================================+
     *
     */
   
    scsi_cdb->scsi_op_code = SCSI_MODE_SENSE_10;

    /*
     * Set Page Code byte to request all supported pages....set Page
     * Control bits appropriately depending if we want changeable or
     * current mode data
     */


    scsi_cdb->scsi_bytes[1] = (0x3F |
                                    ((flags & SCSI_MODE_SNS_CHANGEABLE) ?
                                     0x40 : 0x0));
    scsi_cdb->scsi_bytes[6] =
        (uint8_t) ((data_len >> 8) & 0xff);
    scsi_cdb->scsi_bytes[7] =
        (uint8_t)(data_len & 0xff);



    return rc;
}

/*
 * NAME:        cflash_build_scsi_mode_select_10
 *
 * FUNCTION:    Builds a SCSI Mode Select 10 command.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Mode Select 10 command
 *
 *              data_len  - Date length (in bytes) returned mode sense data
 *                          buffer. This can not exceed 64K bytes
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_mode_select_10(scsi_cdb_t *scsi_cdb,
				    uint16_t data_len, int flags)
{
    int rc = 0;

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *                            MODE SELECT(10)  Command
     *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *|Byte |       |       |       |       |       |       |       |       |
     *|=====+===============================================================|
     *| 0   |                       Operation Code (55h)                    |
     *|-----+---------------------------------------------------------------|
     *| 1   | Reserved              |  PF   |    Reserved            |  SP  |
     *|-----+---------------------------------------------------------------|
     *| 2   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 3   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 4   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 5   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 6   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 7   | (MSB)                                                         |
     *|-----+--                      Parameter List Length               ---|
     *| 8   |                                                         (LSB) |
     *|-----+---------------------------------------------------------------|
     *| 9   |                           Control                             |
     *+=====================================================================+
     *
     */
   
    scsi_cdb->scsi_op_code = SCSI_MODE_SELECT_10;


    /* 
     * Indicates mode pages comply to Page Format, by setting 
     * PF bit
     */
    scsi_cdb->scsi_bytes[0] = 0x10;
    scsi_cdb->scsi_bytes[6] =
        (uint8_t) ((data_len >> 8) & 0xff);
    scsi_cdb->scsi_bytes[7] =
        (uint8_t)(data_len & 0xff);



    return rc;
}

/*
 * NAME:        cflash_build_scsi_read_cap
 *
 * FUNCTION:    Builds a SCSI Read Capacity (10 command.
 *
 * NOTE:        There is no data length for Read Capacity since it is assumed
 *              to always be 8 bytes.
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Read Capacity 10 command
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_read_cap(scsi_cdb_t *scsi_cdb)
{
    int rc = 0;

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *                READ CAPACITY  Command
     *   +=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *   |  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *   |Byte |       |       |       |       |       |       |       |       |
     *   |=====+===============================================================|
     *   | 0   |                       Operation Code (25h)                    |
     *   |-----+---------------------------------------------------------------|
     *   | 1   | Logical Unit Number   |   Reserved                    |RelAdr |
     *   |-----+---------------------------------------------------------------|
     *   | 2   | (MSB)                                                         |
     *   |-----+-----                                                   -------|
     *   | 3   |                 Logical Block Address                         |
     *   |-----+-----                                                   -------|
     *   | 4   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 5   |                                                        (LSB)  |
     *   |-----+---------------------------------------------------------------|
     *   | 6   |                           Reserved                            |
     *   |-----+---------------------------------------------------------------|
     *   | 7   |                           Reserved                            |
     *   |-----+---------------------------------------------------------------|
     *   | 8   |                           Reserved                     | PMI  |
     *   |-----+---------------------------------------------------------------|
     *   | 9   |                           Control                             |
     *   +=====================================================================+
     */
   
    scsi_cdb->scsi_op_code = SCSI_READ_CAPACITY;

    return rc;
}

/*
 * NAME:        cflash_build_scsi_read_cap16
 *
 * FUNCTION:    Builds a SCSI Read Capacity 16 command.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Read Capacity 16 command
 *
 *              data_len  - Date length (in bytes) of returned Read Capacity 16
 *                          data buffer. This can not exceed 255 bytes. Ideally
 *                          this should sizeof(struct readcap16_data).
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_read_cap16(scsi_cdb_t *scsi_cdb, uint8_t data_len)
{
    int rc = 0;

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *           SERVICE ACTION IN  Command for issuing
     *           READ CAPACITY 16
     *   +=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *   |  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *   |Byte |       |       |       |       |       |       |       |       |
     *   |=====+===============================================================|
     *   | 0   |                       Operation Code (9Eh)                    |
     *   |-----+---------------------------------------------------------------|
     *   | 1   | Reserved              |   SERVICE ACTION (10h)                |
     *   |-----+---------------------------------------------------------------|
     *   | 2   | (MSB)                                                         |
     *   |-----+-----                                                   -------|
     *   | 3   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 4   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 5   |                  Logical Block Address                        |
     *   |-----+-----                                                   -------|
     *   | 6   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 7   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 8   |                                                               |
     *   |-----+-----                                                   -------|
     *   | 9   |                                                         (LSB) |
     *   |-----+---------------------------------------------------------------|
     *   | 10  | (MSB)                                                         |
     *   |-----+-----                                                  --------|
     *   | 11  |                                                               |
     *   |-----+-----              Allocation Length                   --------|
     *   | 12  |                                                               |
     *   |-----+-----                                                  --------|
     *   | 13  |                                                         (LSB) |
     *   |-----+---------------------------------------------------------------|
     *   | 14  |                           Reserved                     | PMI  |
     *   |-----+---------------------------------------------------------------|
     *   | 15  |                           Control                             |
     *   +=====================================================================+
     */
   
    scsi_cdb->scsi_op_code = SCSI_SERVICE_ACTION_IN;
    scsi_cdb->scsi_bytes[0] = SCSI_READ_CAP16_SRV_ACT_IN;
    scsi_cdb->scsi_bytes[12] = data_len;

    return rc;
}


/*
 * NAME:        cflash_process_scsi_read_cap16
 *
 * FUNCTION:    Processes the returned lun list from the SCSI
 *              Report Luns command.
 *
 *
 * INPUTS:
 *              lun_list_rsp    - Pointer to lin list returned from Report 
 *                                Luns command.
 *
 *		length_list     - List (in bytes) of the lun_list_rsp buffer.
 *
 *              actual_lun_list - List of Lun ids returned (header removed)
 *                                It is assumed this buffer is large enough
 *                                hold the full number of lun_ids that
 *                                can fit in the returned lun_list_rsp.
 *
 *              num_actual_luns - Number of lun ids in the actual_lun_list.
 *                                
 *
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_process_scsi_read_cap16(struct readcap16_data *readcap16_data, uint32_t *block_size,
				    uint64_t *last_lba)
{
    int rc = 0;


    if (readcap16_data == NULL) {

	return -1;
    }

    *block_size = CFLASH_FROM_ADAP32(readcap16_data->len);
    *last_lba = CFLASH_FROM_ADAP64(readcap16_data->lba);

    return rc;
}

/*
 * NAME:        cflash_build_scsi_request_sense
 *
 * FUNCTION:    Builds a SCSI Request Sense command.
 *
 *
 * INPUTS:
 *              cdb       - Pointer to SCSI CDB to contain Request Sense command
 *
 *              data_len  - Date length (in bytes) of returned sense data.
 *                          This can not exceed 255 bytes
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */

int cflash_build_scsi_request_sense(scsi_cdb_t *scsi_cdb, uint8_t data_len)
{
    int rc = 0;

    bzero(scsi_cdb,sizeof(*scsi_cdb));

    /*
     *                        REQUEST SENSE  Command
     *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
     *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
     *|Byte |       |       |       |       |       |       |       |       |
     *|=====+===============================================================|
     *| 0   |                       Operation Code (03h)                    |
     *|-----+---------------------------------------------------------------|
     *| 1   | Logical Unit Number   |         Reserved                      |
     *|-----+---------------------------------------------------------------|
     *| 2   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 3   |                           Reserved                            |
     *|-----+---------------------------------------------------------------|
     *| 4   |                           Allocation Length                   |
     *|-----+---------------------------------------------------------------|
     *| 5   |                           Control                             |
     *+=====================================================================+
     */
   
    scsi_cdb->scsi_op_code = SCSI_REQUEST_SENSE;
    scsi_cdb->scsi_bytes[3] = data_len;

    return rc;
}
