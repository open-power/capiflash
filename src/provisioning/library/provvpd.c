/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/library/provvpd.c $                          */
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


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <provvpd.h>
#include <provutil.h>
/*
 Hmmph. this is getting overly-complicated. what's the flow really need to be?
 Parse a segment of a buffer. Return the KW found, the data, and the length of the data.
 Tell me where to start for the next segment of the buffer.
 
 In a separate loop, iterate until we exceed the size of the buffer, or reach
 a stop condition.
 */

bool provFindVPDKw(const char* i_kw, const uint8_t* i_vpd_buffer, size_t i_vpd_buffer_length, uint8_t* o_kwdata, int* io_kwdata_length)
{
    //Locals
    bool l_rc = false;
    bool l_found_kw = false;
    prov_pci_vpd_header_t* l_vpd_header = NULL;
    int l_section_length = 0;
    uint8_t* l_buffer_ptr = NULL;
    char l_curr_kw_name[KWNAME_SZ + 1] = {0}; //+1 b/c we want a terminating null
    char l_curr_kw_data[KWDATA_SZ] = {0};
    char l_vpd_name[KWDATA_SZ] = {0};
    int  l_vpd_name_sz = 0;
    prov_pci_vpd_segment_t* l_vpd_section = NULL;
    const uint8_t* l_end_of_buffer = &i_vpd_buffer[i_vpd_buffer_length]; //get the address of the end of the buffer. note this is the 1st byte PAST the end of the array
    uint8_t l_curr_kw_sz = 0;
    //Code
    TRACEV("Entry\n");
    do
    {
        if((i_kw == NULL) || (i_vpd_buffer == NULL) ||
           (i_vpd_buffer_length == 0) ||
           (o_kwdata == NULL) || (io_kwdata_length == NULL))
        {
            TRACEE("Invalid or null Args. Unable to parse VPD structures.\n");
            l_rc = false;
            break;
        }
        
        //hope for the best
        l_vpd_header = (prov_pci_vpd_header_t*) i_vpd_buffer;
        
        //validate if we have a real PCI VPD or not
        //we expect read-only data to come first
        if(l_vpd_header->pci_eyecatcher != PCI_FORMAT_EYECATCHER)
        {
            TRACEE("This doesn't appear to be valid VPD. "
                   "PCI eyecatcher = 0x%02x, expected 0x%02x.\n",
                   l_vpd_header->pci_eyecatcher, PCI_FORMAT_EYECATCHER);
            l_rc = false;
            break;
        }
        l_vpd_name_sz = PROV_CONVERT_UINT8_ARRAY_UINT16(l_vpd_header->name_sz[0],
                                          l_vpd_header->name_sz[1]);
        TRACEV("Got apparently-valid eyecatcher data. Name size is %d.\n", l_vpd_name_sz);

        if(l_vpd_name_sz > KWDATA_SZ) {
            TRACEI("Warning: Trimming KW Name down to %d bytes. Original was %d\n", KWDATA_SZ, l_vpd_name_sz);
            l_vpd_name_sz = KWDATA_SZ;
        }
        bzero(l_vpd_name, sizeof(l_vpd_name));
        strncpy(l_vpd_name, l_vpd_header->name, l_vpd_name_sz);
        TRACEV("Parsing VPD for '%s'\n", l_vpd_name);
        
        //get the address of the VPD section that follows the name by relying on
        //the fact that the name section is an "array" in the struct, and that we can
        //index into the array for the length of the KW. For example - a 0-length
        //name would technically mean that the "name" byte of the struct represents
        //the next segment of data. A 1-byte name would get the 2nd byte after, etc.
        l_vpd_section = (prov_pci_vpd_segment_t*)&l_vpd_header->name[l_vpd_name_sz];
        
        l_section_length = PROV_CONVERT_UINT8_ARRAY_UINT16(l_vpd_section->segment_sz[0],
                                                           l_vpd_section->segment_sz[1]);
        TRACEV("Got %d bytes of RO section data.\n", l_section_length);
        //set up the pointer to the beginning of the keyword data
        l_buffer_ptr = l_vpd_section->keywords;
        //l_buffer_pt
        while((l_buffer_ptr < l_end_of_buffer) &&
              (*l_buffer_ptr != PCI_DATA_ENDTAG))
        {
            bzero(l_curr_kw_name, sizeof(l_curr_kw_name));
            bzero(l_curr_kw_data, sizeof(l_curr_kw_data));
            l_curr_kw_sz = 0;
            
            if(*l_buffer_ptr == PCI_RW_DATA_EYECATCHER)
            {
                uint8_t lo = *l_buffer_ptr++;
                uint8_t hi = *l_buffer_ptr++;

                l_section_length = PROV_CONVERT_UINT8_ARRAY_UINT16(lo,
                                                                   hi);
                TRACEI("RW Data section found of length %d bytes, starting a new section.\n", l_section_length);
                continue; //new section found, so continue processing

            }
            
            //get the name of the KW + its size
            l_curr_kw_name[0] = *l_buffer_ptr++;
            l_curr_kw_name[1] = *l_buffer_ptr++;
            l_curr_kw_sz = *l_buffer_ptr++;
            TRACEV("Current KW: '%s' size = %d\n",l_curr_kw_name, l_curr_kw_sz);
            
            //copy the data out. note this may copy zero bytes if the KW is zero
            //length (which seems to be allowed by the spec).
            memcpy(l_curr_kw_data, l_buffer_ptr, l_curr_kw_sz);
            
            
            //check to see if we found the desired KW!
            if(0 == strcmp(i_kw, l_curr_kw_name))
            {
                l_found_kw = true;
                break;
            }
            
            //advance the pointer by the size of the KW and loop again...
            l_buffer_ptr+=l_curr_kw_sz;
            
        } //end inner while that is searching the buffer for KW data
        
        if(l_found_kw)
        {
            TRACEV("Found VPD for keyword '%s' length %d\n",l_curr_kw_name, l_curr_kw_sz);
            if(*io_kwdata_length < l_curr_kw_sz)
            {
                TRACED("Output buffer %d is too small for keyword '%s' data. We need at least %d bytes.\n", *io_kwdata_length, l_curr_kw_name, l_curr_kw_sz);
                l_rc = false;
                break;
            }
            else
            {
                TRACEV("Copying data to output buffer...\n");
                *io_kwdata_length = l_curr_kw_sz;
                memcpy(o_kwdata, l_curr_kw_data, l_curr_kw_sz);
                l_rc = true;
            }
        }

    } while (0);
    //all paths exit via the same return path
    if(l_rc == false)
    {
        //set the output size to 0 for consistency
        *io_kwdata_length = 0;
    }
    return l_rc;
}
