/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlfcommon.c $                                    */
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

#include <stdlib.h>
#include <inttypes.h>
#include <cxlfcommon.h>
#include <scsi/cxlflash_ioctl.h>
#include <fcntl.h>      // open/close
#include <sys/ioctl.h>  // ioctl cmds
#include <stddef.h>     //offsetof
#include <fcntl.h>      // open/close
#include <libgen.h> //basename
#include <cxlfini.h>
#include <glob.h>
#include <errno.h>

#define MAX_FOPEN_RETRIES 100
#define FOPEN_SLEEP_INTERVAL_US 100000 //100 ms sleep interval





int set_lun_mode(lun_table_entry_t* lun_entry_ptr, MODE_T mode)
{
    TRACEV("Setting %s to %02x\n",lun_entry_ptr->sgdev, mode);
    return cxlf_set_mode(lun_entry_ptr->sgdev, mode, lun_entry_ptr->lun);
}

/*
 need to pass down wwn (get from scsi inquiry data) - does this have a "3" on the front or not???
 need to save off wwns in a file
 need to call utility on udev startup / plugging... see ethernet code
 */


int cxlf_set_mode(char* target_device, uint8_t target_mode, uint8_t* wwid)
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    int fd = 0;
    int rc = false;
    struct dk_cxlflash_manage_lun manage_lun = {{0}};
    int retrycount=0;
    uint8_t empty_wwid[DK_CXLFLASH_MANAGE_LUN_WWID_LEN] = {0};
    int i = 0;
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    TRACEV("Setting target mode for '%s' to %d\n",target_device, target_mode);
    
    do
    {
        if((target_device == NULL) || (strlen(target_device)==0))
        {
            TRACED("Invalid target device.\n");
            rc = false;
            break;
        }
        if(wwid == NULL)
        {
            TRACED("NULL wwid is invalid\n");
            rc = false;
            break;
        }
        if(memcmp(empty_wwid, wwid, DK_CXLFLASH_MANAGE_LUN_WWID_LEN) == 0)
        {
            TRACED("Invalid empty WWID for device '%s'\n", target_device);
            rc = false;
            break;
        }
        if(g_traceV)
        {
            TRACEV("WWID: ");
            for(i=0; i < DK_CXLFLASH_MANAGE_LUN_WWID_LEN; i++)
            {
                TRACED("%02x", wwid[i]);
            }
            TRACED("\n");
        }

        
        //open the device exclusively so that others are no longer able to manipulate it
        do
        {
            if(retrycount != 0)
            {
                TRACEV("Failed to open device - retrying... %s %d\n", target_device, retrycount);
                usleep(FOPEN_SLEEP_INTERVAL_US);
            }
            fd = open(target_device, (O_EXCL|O_NONBLOCK|O_RDWR));
            retrycount++;
        } while((fd < 0) && (retrycount < MAX_FOPEN_RETRIES));
        if (fd < 0)
        {
            rc = EBUSY;
            break;
        }
        
        //we've successfully opened the device now.
        
        switch(target_mode)
        {
            case MODE_LEGACY:
                manage_lun.hdr.flags = DK_CXLFLASH_MANAGE_LUN_DISABLE_SUPERPIPE;
                break;
                
            case MODE_SIO:
                manage_lun.hdr.flags = DK_CXLFLASH_MANAGE_LUN_ENABLE_SUPERPIPE;
                break;
            default:
                TRACED("Unknown target mode: %d\n",target_mode);
                break;
        }
        
        
        memcpy(manage_lun.wwid, wwid, sizeof(manage_lun.wwid));
        errno=0;
        int rslt = ioctl(fd, DK_CXLFLASH_MANAGE_LUN, &manage_lun);
        if(rslt == 0)
        {
            //only set the "true" return code if we were able to successfully make the call to the DD
            rc=true;
        }
        else
        {
            TRACED("MANAGE_LUN ioctl (");
            for(i = 0; i<DK_CXLFLASH_MANAGE_LUN_WWID_LEN; i++)
            {
                TRACED("%02x", wwid[i]);
            }
            TRACED(") failed: %d errno:%d\n",rslt, errno);
            rc=false;
        }

    } while(0);
    
    TRACEV("Closing fd = %d\n",fd);
    //close on all paths
    close(fd);

    return rc;
}

bool cxlf_parse_wwid(uint8_t* o_buffer, char* i_string, uint8_t i_buffer_sz)
{
    //this is simplistic, and not really apt to check for errors.
    if(strlen(i_string) != (DK_CXLFLASH_MANAGE_LUN_WWID_LEN*2))
    {
        TRACEV("WWIDs must be %d characters long. '%s' is invalid.\n",(DK_CXLFLASH_MANAGE_LUN_WWID_LEN*2),i_string);
        return false;
    }
    char* nextchar = i_string;
    int i=0;
    for(i = 0; i < DK_CXLFLASH_MANAGE_LUN_WWID_LEN; i++)
    {
        int upper = 0;
        int lower = 0;
        //read in two characters (representing 4 bits each)
        int validchars = sscanf(nextchar, "%1x%1x", &upper, &lower); //consume two characters
                                                                     //assemble a valid 8-bit byte
        o_buffer[i] = (char)upper << 4 | lower;
        //TRACED("parsed %c%c, valid chars: %d, resulting in %02x\n", *nextchar, *(nextchar+1), validchars, o_buffer[i]);
        //check to see if we read 2 characters successfully
        if(validchars != 2)
        {
            TRACED("Error parsing '%s'\n", i_string);
            return false;
        }
        nextchar+=2;//advance by two characters
    }
    TRACEV("wwid valid '%s'\n", i_string);
    //if we got this far, the wwid is valid
    TRACEV("Controlling WWID: 0x");
    for(i = 0; i<DK_CXLFLASH_MANAGE_LUN_WWID_LEN; i++)
    {
        TRACEV("%02x", o_buffer[i]);
    }
    TRACEV("\n");
    return true;
}


//TODO; check how this handles a "virtual LUN0" page 83's data. can we tell them apart?
int extract_lun_from_vpd(const char* i_sgdevname, uint8_t* o_lun)
{
    
    //Locals
    //bool l_rc = false;
    pg83header_t* l_vpd_header = NULL;
    char l_pci_path[DEV_STRING_SZ];
    uint8_t l_vpd_buffer[MAX_VPD_SIZE];
    //int l_section_length = 0;
    FILE* l_vpd_file = NULL;
    //uint8_t* l_buffer_ptr = NULL;
    //char l_curr_kw_data[KWDATA_SZ] = {0};
    pg83data_t* l_pg83data = NULL;
    //uint8_t l_curr_kw_sz = 0;
    int n = 0;
    int i =0;
    
    bzero(l_pci_path, DEV_STRING_SZ);
    bzero(l_vpd_buffer, MAX_VPD_SIZE);
    
    //generate the proper VPD path to get VPD
    snprintf(l_pci_path, DEV_STRING_SZ, "/sys/class/scsi_generic/%s/device/vpd_pg83", i_sgdevname);
    
    //TRACEV("Opening '%s'\n", l_pci_path);
    
    do
    {
        l_vpd_file = fopen(l_pci_path, "rb");
        if (l_vpd_file)
        {
            //read page 83 data from Flash 900
            n = fread(l_vpd_buffer, 1, MAX_VPD_SIZE, l_vpd_file);
        }
        else
        {
            TRACEI("Unable to read file. Do you have permission to open '%s' ?", l_pci_path);
            //l_rc = false;
            return -1;
            // error opening file
        }
        
        if(n < MIN_VPD_SIZE)
        {
            //l_rc = false;
            TRACEI("Warning: Buffer underrun. This indicates a potential VPD format problem.\n");
            break;
        }
        l_vpd_header =  (pg83header_t*)l_vpd_buffer;
        if(l_vpd_header->page_code != 0x83)
        {
            TRACED("Didn't get valid page 83 data for %s", l_pci_path);
        }
        if(l_vpd_header->page_length == 0x00)
        {
            TRACEI("Didn't get valid page data length for %s", l_pci_path);
        }
        i=offsetof(pg83header_t, data);
        l_pg83data = (void*)(l_vpd_header->data);
        
        //scan and look for LUN ID data...
        while(i<n)
        {
            if((l_pg83data->length == 16) &&
               (l_pg83data->prot_info == 0x01) &&
               (l_pg83data->piv_assoc_id == 0x03))
            {
                //TRACEV("Found valid wwid for this LUN\n");
                break;
            }
            i+=l_pg83data->length + offsetof(pg83data_t, data);
            l_pg83data = (void*)l_pg83data + l_pg83data->length + offsetof(pg83data_t, data);
        }
        memcpy(o_lun, l_pg83data->data, 16);
        
        
        
    }while(0);
    if(l_vpd_file)
    {
        fclose(l_vpd_file);
        l_vpd_file = NULL;
    }
    return 0;
}

void printentry(lun_table_entry_t* entry)
{
    int i = 0;
    uint64_t* lhs_ptr = (uint64_t*)&entry->lun[0];
    uint64_t* rhs_ptr = (uint64_t*)&entry->lun[8];
    uint64_t lhs_val = *lhs_ptr;
    uint64_t rhs_val = *rhs_ptr;
    TRACED("%-16s",entry->sgdev);
    for(i=0; i < DK_CXLFLASH_MANAGE_LUN_WWID_LEN; i++)
    {
        TRACED("%02x",entry->lun[i]);
    }
    TRACED(" aka %"PRIx64"%"PRIx64"\n",lhs_val,rhs_val);
    
}

int compare_luns(const void* item1, const void* item2)
{
    //quickly compare 128-bit entries
    //memcmp exists, but is slower
    lun_table_entry_t* lhs = (lun_table_entry_t*) item1;
    lun_table_entry_t* rhs = (lun_table_entry_t*) item2;
    
    if((lhs == NULL) || (rhs == NULL))
    {
        TRACEI("Invalid lhs = %p or rhs = %p", lhs, rhs);
        return 0;
    }
    //memcmp is 30% slower in a synthetic benchmark
    //return memcmp(lhs->lun, rhs->lun, DK_CXLFLASH_MANAGE_LUN_WWID_LEN);
    uint64_t* lhs_ptr = (uint64_t*)&lhs->lun[0];
    uint64_t* rhs_ptr = (uint64_t*)&rhs->lun[0];
    uint64_t lhs_val = *lhs_ptr;
    uint64_t rhs_val = *rhs_ptr;
    
    if(lhs_val != rhs_val)
    {
        if(lhs_val < rhs_val)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        //check the low order bytes
        lhs_ptr++;
        rhs_ptr++;
        lhs_val = *lhs_ptr;
        rhs_val = *rhs_ptr;
        
        if(lhs_val != rhs_val)
        {
            if(lhs_val < rhs_val)
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
        else
        {
            return 0;
        }
    }
}



int update_siotable(lun_table_entry_t* o_lun_table, int* o_lun_table_sz)
{
    //todo - static to select whether we do the update or not, based on stat data
    ini_dict_t*                 dict = NULL;
    ini_dict_t*                 curr_entry = NULL;
    uint32_t                    linefail = 0;
    uint16_t                    luncount = 0;
    int                         curr_mode;
    dict = cxlfIniParse("/etc/cxlflash/sioluntable.ini", &linefail );
    curr_entry = dict;
    bool rslt = false;
    *o_lun_table_sz=0;
    while((curr_entry != NULL) && (luncount < MAX_NUM_LUNS))
    {
        //atoi will return zero for any non-successful conversion. this is
        //ideal, since we will treat anything that doesn't convert to "1" integer
        //as legacy mode.
        curr_mode = atoi(curr_entry->value);
        if(curr_mode == 1)
        {
            //attempt to parse any key we find that's the right size, and put it
            //into the appropriate place in the LUN table, assuming LUNs are stored
            //in hex with no leading 0x
            if(strlen(curr_entry->key) == (DK_CXLFLASH_MANAGE_LUN_WWID_LEN*2))
            {
                rslt = cxlf_parse_wwid(o_lun_table[luncount].lun, curr_entry->key, strlen(curr_entry->key));
                if(rslt == true)
                {
                    luncount++;
                }
            }
            
        }
        curr_entry = curr_entry->next;
    }
    *o_lun_table_sz = luncount;
    TRACEI("Read %d LUNs from the table\n", *o_lun_table_sz);
    qsort(o_lun_table, luncount, sizeof(lun_table_entry_t), compare_luns);
    if(dict)
    {
        cxlfIniFree(dict);
    }
    
    return 0;
}


int update_cxlflash_devs(lun_table_entry_t* o_cxldevices, int* o_cxldevices_sz, lun_table_entry_t* filter_lun)
{
    //todo- do something to determine if we make a change... perhaps based on stats?
    int i = 0;
    char* devname = NULL;
    char sgdevpath[DEV_STRING_SZ];
    glob_t globbuf;
    int rc = 0;
    int num_devs = 0;
    
    TRACEV("Starting update...\n");
    
    do
    {
        *o_cxldevices_sz=0;
        memset(sgdevpath,0,DEV_STRING_SZ);
        
        rc = glob( "/sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/host*/target*:*:*/*:*:*:*/scsi_generic/sg*", GLOB_ONLYDIR, NULL, &globbuf);
        if(rc != 0)
        {
            TRACEI("Error on glob() call\n");
            break;
        }
        
        if(globbuf.gl_pathc > MAX_NUM_SGDEVS)
        {
            TRACED("This application supports up to %d devices, but %d were found.\n", MAX_NUM_SGDEVS, (int)globbuf.gl_pathc);
            break;
        }
        for(i = 0; i < globbuf.gl_pathc; i++ )
        {
            devname = basename(globbuf.gl_pathv[i]);
            if(strcmp(devname, ".")==0)
            {
                TRACEI("Found invalid SG device name for '%s'\n", globbuf.gl_pathv[i]);
                break;
            }
            else
            {
                snprintf(sgdevpath, DEV_STRING_SZ, "/dev/%s",devname);
                strncpy(o_cxldevices[num_devs].sgdev, sgdevpath, DEV_STRING_SZ);
                if(extract_lun_from_vpd(devname, o_cxldevices[num_devs].lun) != 0)
                {
                    TRACED("Unable to extract VPD for device %s\n", devname);
                    rc = -2;
                    break;
                }
                if(filter_lun != NULL)
                {
                    if(compare_luns(filter_lun, &o_cxldevices[num_devs]) != 0)
                    {
                        continue;
                    }
                } //if filter lun
                num_devs++;
            } //else
        } //for(i...)
        *o_cxldevices_sz = num_devs;
        qsort(o_cxldevices, *o_cxldevices_sz, sizeof(lun_table_entry_t), compare_luns);
        TRACEI("Read %d Device's properties from sysfs\n", *o_cxldevices_sz);
    }while(0);
    
    globfree(&globbuf); //cleanup from glob() must always occur
    
    
    return rc;
}


int cxlf_refresh_luns(lun_table_entry_t* i_luntable, int i_luntable_sz, lun_table_entry_t* i_sgdevs, int i_sgdevs_sz)
{
    
    int32_t                     curr_sg=0;
    int32_t                     curr_lun=0;
    int                         rslt = 0;
    int                         sl_rc= true;
    int                         rc   = 0;
    
    //there are likely more SG devices than LUN table entries
    //assume the list is sorted, and start  comparing the
    //sorted lists
    TRACEI("Max SG Devs: %d, MAX Luns: %d\n", i_sgdevs_sz, i_luntable_sz);
    while((curr_sg < i_sgdevs_sz) && (curr_lun < i_luntable_sz))
    {
        sl_rc=true;
        TRACEV("curr_sg = %d, curr_lun= %d\n", curr_sg, curr_lun);
        //TRACED("SG:    ");
        //printentry(&i_sgdevs[curr_sg]);
        //TRACED("Table: ");
        //printentry(&i_sgdevs[curr_lun]);
        rslt = compare_luns(&i_sgdevs[curr_sg], &i_luntable[curr_lun]);
        //negative indicates we are NOT in the LUN table and
        //should try and advance the device table pointer
        if(rslt < 0)
        {
            sl_rc=set_lun_mode(&i_sgdevs[curr_sg], MODE_LEGACY);
            curr_sg++;
        }
        //positive indicates the current LUN wasn't found, so we should
        //advance until we are less than or equal to a table entry
        else if(rslt > 0)
        {
            curr_lun++;
        }
        //indicates we found a match!
        else
        {
            sl_rc=set_lun_mode(&i_sgdevs[curr_sg], MODE_SIO);
            curr_sg++;
        }
        if (sl_rc==false) rc=-1;
    }
    TRACEV("Cleaning up any LUN NOT in the LUN table...\n");
    while(curr_sg<i_sgdevs_sz)
    {
        sl_rc=true;
        //clean up stragglers - anyone not in the SIO table will be legacy
        TRACEV("curr_sg = %d, curr_lun= %d\n", curr_sg, curr_lun);
        //TRACEV("SG:    ");
        //printentry(&i_sgdevs[curr_sg]);
        sl_rc=set_lun_mode(&i_sgdevs[curr_sg], MODE_LEGACY);
        curr_sg++;
        if (sl_rc==false) rc=-2;
    }
    TRACEV("Done, rc=%d\n", rc);
    return rc;

}
