/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/prov.h $                                          */
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


#ifndef _PROV_H
#define _PROV_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <provextstructs.h>
#include <stdbool.h>
#include <sislite.h>  /* for NUM_FC_PORTS */

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

#define SL_INI_SINI_MARKER      0x53494e49
#define SL_INI_ELMD_MARKER      0x454c4d44
/*----------------------------------------------------------------------------*/
/* Types                                                                      */
/*----------------------------------------------------------------------------*/
/**
 * @brief mserv packed binary config element
 * There may be any number of these. Each element describes all necessary info
 * for MServ to initialize the resources described by the element. This
 * typically includes the AFU, port names, and other info.
 */
typedef struct capikv_ini_elm {
  __u64 elmd_marker;       // element data marker: set to 0x454c4d44
  __u64 lun_id;            // lun_id to use (only 1)
  __u64 wwpn[SURELOCK_NUM_FC_PORTS]; // wwpn of AFU ports
  char  afu_dev_path[64];  // non master path to /dev
  char  afu_pci_path[64];  // non master path to /sys
  char  afu_dev_pathm[64]; // master path to /dev
  char  afu_pci_pathm[64]; // master path to /sys
  /* future expansions go here */
}capikv_ini_elm_t;

/**
 * @brief mserv config header
 * This header structure provides a description of how many init elements are
 * contained in the variable-length struct. Note that we may have anywhere from
 * 0 to N 
 */
typedef struct capikv_ini {
  __u32 sini_marker; // set to 0x53494e49
  __u32 flags;       // to version or for other purposes, presently 0
  __u32 nelm;        // number of elements
  __u32 size;        // size of each element
  __u32 rsvd[8];     // must be zeroed

  /*
   * NOTE: 
   *
   * 1. to maintain beckward compatibility, the header of the ini file
   *    (i.e. the fields above) cannot change nor can the header be 
   *    expanded.
   *
   * 2. Each element can be expanded by adding fields to the end but they 
   *    cannot be shrunk.
   *
   * 3. any reserved field must be zeroed
   *
   */

  struct capikv_ini_elm elm[1];  // variable length elements, 1 per AFU
                                   // minimum 1 AFU required
}capikv_ini_t;



/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/


/**
 * @brief Initialize an adapter
 * @param i_adapter device to be init'd
 */
bool provInitAdapter(const prov_adapter_info_t* i_adapter);

/**
 * @brief Get All WWPNs for ALL adapters in the system
 * Note - caller is responsible for allocating and freeing o_wwpn_info!
 * Code will return up to io_num_wwpns.
 * @param io_wwpn_info empty buffer for wwpn_info. will be filled in on success
 * @param io_num_wwpns input - max # of wwpn structs caller would like; output - actual # of wwpn structs filled in by fn. may be zero on failure.
 * @returns TRUE if an error occurs, or if the buffer is not large enough according to io_num_wwpns
 */

uint8_t provGetAllWWPNs(prov_wwpn_info_t* io_wwpn_info, uint16_t *io_num_wwpns);

/**
 * @brief List all Adapters found
 * @param o_info output parm in which to place found adapter(s). If no adapters
 *               are found, this pointer will remain valid. Check io_num_adapters
 *               to know how many entries are returned.
 * @param io_num_adapters input - max # of prov_adapter_info_t the caller will accept
 *                         output - actual number of prov_adapter_info_t returned by the function
 *
 */
bool provGetAllAdapters(prov_adapter_info_t* o_info, int* io_num_adapters);

/** 
 * @brief Write config data for Master Context
 * Write configuration file for capikv Master Context to the file system.
 * @param i_inielem initialization element array starting member
 * @param i_numelems number of i_inielem members
 * @param i_flags any configuration flags (opaque)
 * @param i_cfgfilename destination file to be written
 * @returns true on sucess, false on failure
 */
bool provWriteMasterCfg(capikv_ini_elm_t* i_inielem, __u32 i_numelems, __u32 i_flags, char* i_cfgfilename);

/**
 * @brief Run a loopback test on a targeted adapter for a configurable amount of time.
 * @param i_adapter Target card
 * @param i_test_time_us Test time in microseconds for this run. Time must be at least 1 second, and can be up to 24 hours.
 * @returns false on failure and true on pass of the test
 */
bool provLoopbackTest(const prov_adapter_info_t* i_adapter, uint64_t i_test_time_us);

#endif //_PROV_H
