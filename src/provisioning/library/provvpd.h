/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/library/provvpd.h $                          */
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

#ifndef PROVVPD_H
#define PROVVPD_H

#include <stdint.h>
#include <stdbool.h>

//assume little-endian data
#define PROV_CONVERT_UINT8_ARRAY_UINT16(lo,hi) \
(((hi)<<8) | (lo))

#define KWDATA_SZ  256 //max size of a VPD buffer

//PCI Spec 2.1 section 6.4 specifies the structure of this data. However, the
//book "PCI System Architecture" explains it much more clearly.
//See tables 19-27, 19-28, 19-29, and 19-30 of the book for a good explanation.

//Note! The structres below can't / won't tell the total length of the combined
//read/write AND read-only data of the VPD!

//Note! The RO section may be followed by a RW section with its own length.
//Finding a null byte does not necessarily mean we've reached the end of the VPD
//for the adapter. The RO section may be zero-padded. In theory we must scan
//forward until we find either the PCI_RW_DATA_EYECATCHER or the PCI_DATA_ENDTAG.
#define PCI_FORMAT_EYECATCHER   0x82
#define PCI_RO_DATA_EYECATCHER  0x90
#define PCI_RW_DATA_EYECATCHER  0x91
#define PCI_DATA_ENDTAG         0x78
typedef struct  __attribute__((__packed__))prov_pci_vpd_header
{
    char    pci_eyecatcher; //must be 0x82
    uint8_t name_sz[2];     //length of the name field. byte 0 is lo, byte 1 is hi.
    char    name[1];
    
}prov_pci_vpd_header_t;

typedef struct  __attribute__((__packed__))prov_pci_vpd_segment
{
    char    segment_eyecatcher;  //must be 0x90 or 0x91
    uint8_t segment_sz[2];       //TOTAL length of the fields. byte 0 is lo, byte 1 is hi.
    uint8_t    keywords[1];    //variable length VPD data!
}prov_pci_vpd_segment_t;

#define KWNAME_SZ 3
typedef struct  __attribute__((__packed__))prov_pci_vpd_kwdata
{
    char    kwname[KWNAME_SZ];
    uint8_t kw_sz;
    uint8_t kwdata[1];      //variable length!
}prov_pci_vpd_kwdata_t;



bool provFindVPDKw(const char* i_kw, const uint8_t* i_vpd_buffer, size_t i_vpd_buffer_length, uint8_t* o_kwdata, int* io_kwdata_length);

#endif
