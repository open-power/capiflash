/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/library/provafu.c $                          */
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
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>      //errno parsing on kernel calls
#include <libcxl.h>     //provides capi support
#include <inttypes.h>   //needed for PRIx64
#include <sys/mman.h>   //needed for mmap!

#include <cflash_mmio.h> //mmio's with byte swap
#include <prov.h>
#include <provafu.h>
#include <provutil.h>
#include <provvpd.h>
#include <afu_fc.h>


void * mmap_afu_wwpn_registers(int afu_master_fd)
{
    //Locals
	void *l_ret_ptr = NULL;
    
    //Code
    TRACEI("Attempting to mmap 0x%016X bytes of problem state.\n", FC_PORT_MMAP_SIZE);
    l_ret_ptr = mmap(NULL, FC_PORT_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
                     afu_master_fd, 0);
    TRACEV("MMAP AFU Master  complete\n");
    if (l_ret_ptr == MAP_FAILED) {
        TRACEE("Unable to mmap problem state regs. errno = %d (%s)\n", errno, strerror(errno));
        l_ret_ptr = NULL;
    }
    
	return l_ret_ptr;
}

void munmap_afu_wwpn_registers(void *ps_regs)
{
    if (munmap(ps_regs, FC_PORT_MMAP_SIZE))
        perror("munmap_afu_psa_registers");
}



#define MAX_PORT_STATUS_RETRY_COUNT 20 //20 500ms retries = 10 seconds
#define PORT_STATUS_RETRY_INTERVAL_US 500000 //microseconds
#define FC_MTIP_STATUS_MASK_OFFLINE 0x00000010    //bit 4 set, bit 5 cleared
#define FC_MTIP_STATUS_MASK_ONLINE  0x00000020    //bit 4 cleared, bit 5 set
bool wait_port_state(const uint8_t* i_afu_psa_addr, const int i_port_offset, const bool i_online)
{
    //Locals
    bool l_rc = false;
    uint8_t i = 0;
    uint32_t mmio_data32 = 0;
    uint32_t l_target_status = FC_MTIP_STATUS_MASK_OFFLINE;
    if(i_online)
    {
        l_target_status = FC_MTIP_STATUS_MASK_ONLINE;
    }
    TRACEV("Waiting for port status mask equal to 0x%08x\n", l_target_status);
    //Code
    for(i=0; i<MAX_PORT_STATUS_RETRY_COUNT; i++)
    {
        //FC_MTIP_STATUS is 32-bit as per MTIP programming reference guide, page 50
        //bit 4 of the register defines whether we are offline
        //bit 5 defines if we are online.
        mmio_data32 = in_mmio32((__u64*)&i_afu_psa_addr[FC_MTIP_STATUS + i_port_offset]);
        TRACEV("FC_MTIP_STATUS    (0x%08X): 0x%08X\n", FC_MTIP_STATUS + i_port_offset, mmio_data32);
        //TODO: define a bitwise union for this structure to ease manipulation
        if( (mmio_data32 & 0x30) == l_target_status)
        {
            //port is now in desired state
            l_rc = true;
            TRACEV("Port state bits are 0x%02x.\n", l_target_status);
            if(i_online)
            {
                TRACEI("Port is now online\n");
            }
            else
            {
                TRACEI("Port is now offline\n");
            }
            break; //break inner for loop...
        }
        TRACEV("Sleeping to retry again...\n");
        usleep(PORT_STATUS_RETRY_INTERVAL_US);
    }
    //rc is only set if we break out of the loop
    return l_rc;
}


#define LOOPBACK_TEST_QUIET_PERIOD 2100000 //uS units - 2.1 seconds to account for 2S FC timeout + transmission time
bool loopbackDiag(char* i_afu_path, const AFU_PORT_ID i_port, uint64_t i_test_time_us)
{
    //Locals
    struct cxl_afu_h *afu_master_h = NULL;
    int afu_master_fd = 0;
    uint8_t *afu_psa_addr = NULL;
    uint64_t mmio_data = 0;
    int l_fc_port_offset = 0;
    bool l_rc = true;
    uint64_t l_init_txcount = 0;
    uint64_t l_init_errcount = 0;
    uint64_t l_init_passcount = 0;
    uint64_t l_txcount = 0;
    uint64_t l_errcount = 0;
    uint64_t l_passcount = 0;
    
    //Code
    do
    {
        if(i_port >= MAX_WWPNS_PER_ADAPTER)
        {
            TRACEE("Port number %d is not a valid port number. We only support up to %d numbers.\n", i_port, MAX_WWPNS_PER_ADAPTER);
            l_rc = false;
            break;
        }
        
        l_fc_port_offset = FC_PORT_BASE_OFFSET + (i_port * FC_PORT_REG_SIZE);
        TRACEV("Target reg bank for port %d = 0x%08x\n",i_port, l_fc_port_offset);
        
        TRACEI("Attempting to open device '%s'\n", i_afu_path);
        
        /* now that the AFU is started, lets set config options */
        if ((afu_master_h = cxl_afu_open_dev(i_afu_path)) < 0) {
            TRACEE("Unable to open AFU Master cxl device. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = false;
            break;
        }
        TRACEV("Opened %p, now attempting to get FD\n", afu_master_h);
        afu_master_fd = cxl_afu_fd(afu_master_h);
        TRACEV("Got FD! = %d\n", afu_master_fd);
        
        if (cxl_afu_attach(afu_master_h,
                           0)) //no WEQ needed
        {
            TRACEE("Call to cxl_afu_attach failed. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = false;
            break;
        }
        
        
        afu_psa_addr = mmap_afu_wwpn_registers(afu_master_fd);
        if (!afu_psa_addr) {
            TRACEE("Error attempting to map AFU problem state registers. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = false;
            break;
        }
        
        //Loopback enables a special echo mode and checks tx and rx counts to make sure
        //the phy works correctly. The test is relatively short.
        TRACEI("Enabling loopback mode for adapter %s, port %d\n", i_afu_path, i_port);
        //first read back all test register counts. The AFU does not zero them
        l_init_txcount =   in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_TXCNT + l_fc_port_offset]);
        l_init_errcount =  in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_ERRCNT + l_fc_port_offset]);
        l_init_passcount = in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_PASSCNT + l_fc_port_offset]);


        //enable the test
        mmio_data = in_mmio64((__u64*)&afu_psa_addr[FC_CONFIG2 + l_fc_port_offset]);
        mmio_data |= (uint64_t)0x01 << 40; //set ECHO generator for this port (bit 40)
        out_mmio64((__u64*)&afu_psa_addr[FC_CONFIG2 + l_fc_port_offset], mmio_data);

        TRACEI("Waiting %"PRIu64" microseconds for the test to complete...\n", i_test_time_us);
        usleep(i_test_time_us);
        TRACEI("Disabling loopback mode.\n");
        mmio_data = in_mmio64((__u64*)&afu_psa_addr[FC_CONFIG2 + l_fc_port_offset]);
        mmio_data &= ~((uint64_t)0x01 << 40); //clear ECHO generator for this port (bit 40)
        out_mmio64((__u64*)&afu_psa_addr[FC_CONFIG2 + l_fc_port_offset], mmio_data);

        TRACEI("Waiting for the link to quiesce...\n");
        usleep(LOOPBACK_TEST_QUIET_PERIOD);//wait for quiesce and any final timeouts
        
        //check test results from HW - subtract out initial readings
        l_txcount =   in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_TXCNT + l_fc_port_offset]) - l_init_txcount;
        l_errcount =  in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_ERRCNT + l_fc_port_offset]) - l_init_errcount;
        l_passcount = in_mmio64((__u64*)&afu_psa_addr[FC_LOOPBACK_PASSCNT + l_fc_port_offset]) - l_init_passcount;

        if((l_txcount == 0) || 
           (l_errcount != 0) || 
           (l_txcount != l_passcount))
        {
            TRACED("Loopback diagnostic failure detected. AFU: %s, port: %d, Tx Count: %"PRIu64", Pass Count: %"PRIu64", Error Count: %"PRIu64"\n", i_afu_path, i_port, l_txcount, l_passcount, l_errcount);
            l_rc = false;
        }
        else
        {
            TRACEV("Loopback test passed. AFU: %s, port: %d, Tx Count: %"PRIu64", Pass Count: %"PRIu64", Error Count: %"PRIu64"\n", i_afu_path, i_port, l_txcount, l_passcount, l_errcount);
            l_rc = true;
        }

        //done!
        munmap_afu_wwpn_registers((void *) afu_psa_addr);
        cxl_afu_free(afu_master_h);
        
        
    } while (0);
    
    return l_rc;
    
}



#define LOOPBACK_TEST_TIME_MIN 1000000l //uS units - 1 second test time
#define LOOPBACK_TEST_TIME_MAX 86400000000l //uS units - 24 hrs test time
bool provLoopbackTest(const prov_adapter_info_t* i_adapter, uint64_t i_test_time_us)
{
    //Locals
    bool l_rc = true; //rely on l_rc defaulting to true. only write it to false on error
    bool l_portrc = true;
    uint8_t l_curr_port = 0;
    char l_afu_path[DEV_PATH_LENGTH*2] = {0};
    //Code
    do
    {
        if(i_adapter == NULL)
        {
            TRACEE("Error! invalid args detected.\n");
            l_rc = false;
            break;
        }
        if((i_test_time_us < LOOPBACK_TEST_TIME_MIN) || (i_test_time_us > LOOPBACK_TEST_TIME_MAX))
        {
            TRACEE("Error! Test time of %"PRIu64" is invalid.\n", i_test_time_us);
            l_rc = false;
            break;
        }
        snprintf(l_afu_path, sizeof(l_afu_path), "/dev/cxl/%sm", i_adapter->afu_name);
        
        TRACEI("Running loopback for %s\n", l_afu_path);
        for(l_curr_port=0; l_curr_port < MAX_WWPNS_PER_ADAPTER; l_curr_port++)
        {
            l_portrc = loopbackDiag(l_afu_path, l_curr_port, i_test_time_us);
            if(l_portrc == false)
            {
                TRACEI("Loopback failed.\n");
                l_rc = false; //any single port failure makes the entire test fail
            }
        }
        
    } while (0);
    

    return l_rc;
}

//Current goal: clean up set_afu_master_psa_registers() - this code
//needs to handle (correctly) finding the AFU device path,
//opening it, etc.
//once initialize_wwpn can work for a single port, we will iterate across the wwpn
//structures sequentially programming each port for all AFUs.

bool initialize_wwpn(char* i_afu_path, const AFU_PORT_ID i_port, const uint64_t i_wwpn)
{
    //Locals
	struct cxl_afu_h *afu_master_h = NULL;
	int afu_master_fd = 0;
	uint8_t *afu_psa_addr = NULL;
    uint32_t mmio_data32 = 0;
    uint64_t mmio_data = 0;
    int l_fc_port_offset = 0; //todo: replace all references with "l_fc_port_offset"
    bool l_rc = 0;
    
    //Code
    do
    {
        if(i_port >= MAX_WWPNS_PER_ADAPTER)
        {
            TRACEE("Port number %d is not a valid port number. We only support up to %d numbers.\n", i_port, MAX_WWPNS_PER_ADAPTER);
            l_rc = false;
            break;
        }
        if(i_wwpn == 0)
        {
            TRACEE("WWPN for %s port %d is zero and therefore invalid.\n", i_afu_path, i_port);
            l_rc = false;
            break;
        }
        
        l_fc_port_offset = FC_PORT_BASE_OFFSET + (i_port * FC_PORT_REG_SIZE);
        TRACEV("Target reg bank for port %d = 0x%08x\n",i_port, l_fc_port_offset);
        
        TRACEI("Attempting to open device '%s'\n", i_afu_path);
        
        /* now that the AFU is started, lets set config options */
        if ((afu_master_h = cxl_afu_open_dev(i_afu_path)) < 0) {
            TRACEE("Unable to open AFU Master cxl device. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = false;
            break;
        }
        TRACEV("Opened %p, now attempting to get FD\n", afu_master_h);
        afu_master_fd = cxl_afu_fd(afu_master_h);
        TRACEV("Got FD! = %d\n", afu_master_fd);
        
        if (cxl_afu_attach(afu_master_h,
                           0)) //no WEQ needed
        {
            TRACEE("Call to cxl_afu_attach failed. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = false;
            break;
        }
        
        
        afu_psa_addr = mmap_afu_wwpn_registers(afu_master_fd);
        if (!afu_psa_addr) {
            TRACEE("Error attempting to map AFU problem state registers. errno = %d (%s)\n", errno, strerror(errno));
            l_rc = -1;
            break;
        }
        
        //Take port offline
        
        //TRACED("Bringing port offline\n");

        
        //get status bits for debug
        //TODO: maybe wrap up this print into a nice macro...
        mmio_data32 = in_mmio32((__u64*)&afu_psa_addr[FC_MTIP_STATUS + l_fc_port_offset]);
        TRACEV("FC_MTIP_STATUS    (0x%08X): 0x%08X\n", FC_MTIP_STATUS + l_fc_port_offset, mmio_data32);
        mmio_data32 = in_mmio32((__u64*)&afu_psa_addr[FC_MTIP_CMDCONFIG + l_fc_port_offset]);
        TRACEV("FC_MTIP_CMDCONFIG (0x%08X): 0x%08X\n", FC_MTIP_CMDCONFIG + l_fc_port_offset, mmio_data32);
        mmio_data32 &= ~0x20; // clear ON_LINE
        mmio_data32 |= 0x40;  // set OFF_LINE
        TRACEV("FC_MTIP_CMDCONFIG: Proposed:    0x%08X\n", mmio_data32);
        out_mmio32((__u64*)&afu_psa_addr[FC_MTIP_CMDCONFIG + l_fc_port_offset], mmio_data32);
        
        //wait for the port to be offline
        l_rc = wait_port_state(afu_psa_addr, l_fc_port_offset, false);
        if(l_rc == false)
        {
            TRACEE("Port not offline in time. Aborting.\n");
            l_rc = -1;
            break;
        }
        
        //now we know we are offline, so write the PN...
        
        
        //read out the current PN
        //PN register is 64-bit as per spec
        mmio_data = in_mmio64((__u64*)&afu_psa_addr[FC_PNAME + l_fc_port_offset]);
        TRACEV("FC_PNAME:         (0x%08X): 0x%"PRIx64"\n",FC_PNAME + l_fc_port_offset, mmio_data);
        TRACEI("Current Port Name is  0x%"PRIx64"\n", mmio_data);
        
        mmio_data = i_wwpn;
        TRACEI("New Port Name will be 0x%"PRIx64"\n", mmio_data);
        
        out_mmio64((__u64*)&afu_psa_addr[FC_PNAME + l_fc_port_offset], mmio_data);
        
        //bring the port back online
        //read control bits...
        mmio_data32 = in_mmio32((__u64*)&afu_psa_addr[FC_MTIP_CMDCONFIG + l_fc_port_offset]);
        TRACEV("FC_MTIP_CMDCONFIG (0x%08X): 0x%08X\n", FC_MTIP_CMDCONFIG + l_fc_port_offset, mmio_data32);
        mmio_data32 |= 0x20; // set ON_LINE
        mmio_data32 &= ~0x40;  // clear OFF_LINE
                               //TODO: ask todd - should we explicitly-set other bits? I needed to force the port into FC mode (set bit 1)
        //net result is we write 0x23 to the lowest byte
        TRACEV("FC_MTIP_CMDCONFIG: Proposed:    0x%08X\n", mmio_data32);
        out_mmio32((__u64*)&afu_psa_addr[FC_MTIP_CMDCONFIG + l_fc_port_offset], mmio_data32);
        
        //wait for the port to be online
        l_rc = wait_port_state(afu_psa_addr, l_fc_port_offset, true);
        if(l_rc == false)
        {
            TRACEE("Port not online in time.\n");
            l_rc = -1;
            break;
        }
        
        //done!
        munmap_afu_wwpn_registers((void *) afu_psa_addr);
        cxl_afu_free(afu_master_h);
        
        
        
        TRACEI("Successfully programmed WWPN!\n");
        l_rc = true;
        
    } while (0);
    
	return l_rc;
    
}



bool provInitAdapter(const prov_adapter_info_t* i_adapter)
{
    //Locals
    bool l_rc = false;
    uint64_t l_curr_wwpn = 0;
    uint8_t l_curr_port = 0;
    char l_afu_path[DEV_PATH_LENGTH*2] = {0};
    //Code
    do
    {
        if(i_adapter == NULL)
        {
            TRACEE("Error! invalid args detected.\n");
            l_rc = false;
            break;
        }
        snprintf(l_afu_path, sizeof(l_afu_path), "/dev/cxl/%sm", i_adapter->afu_name);
        TRACED("Setting up adapter '%s'...\n", l_afu_path);
        
        
        TRACEI("Initializing WWPN Data\n");
        for(l_curr_port=0; l_curr_port < MAX_WWPNS_PER_ADAPTER; l_curr_port++)
        {
            l_curr_wwpn = strtoul(i_adapter->wwpn[l_curr_port],NULL,16);
            TRACEV("Initing AFU '%s' port %d\n", l_afu_path, l_curr_port);
            l_rc = initialize_wwpn(l_afu_path, l_curr_port, l_curr_wwpn);
            if(l_rc == false)
            {
                TRACEE("Error occurred while initializing WWPN Data.\n");
                break;
            }
        }
        
        //finished!
        TRACED("SUCCESS: Initialization Complete!\n");
    } while (0);
    

    return l_rc;
}

void provGetAFUs()
{
    //Locals
    struct cxl_afu_h *l_afu;
    char *l_pci_path = NULL;
    int l_rc = 0;
    
    //Code
    cxl_for_each_afu(l_afu)
    {
        TRACEI("AFU found: '%s'\n", cxl_afu_dev_name(l_afu));
        l_rc = cxl_afu_sysfs_pci(l_afu, &l_pci_path);
        TRACEI("sysfs rc: %d\n", l_rc);
        TRACEI("sysfs path: '%s'\n", l_pci_path);
        free(l_pci_path);
    }
}



#define MAX_VPD_SIZE 512
#define MIN_VPD_SIZE 80

bool provGetAllAdapters(prov_adapter_info_t* o_info, int* io_num_adapters)
{
    //Locals
    struct cxl_afu_h *l_afu;
    char *l_pci_path = NULL;
    bool l_rc = 0;
    //prov_adapter_info_t* l_curr_adapter_info = NULL;
    int l_num_adapters = 0;
    
    //Code
    cxl_for_each_afu(l_afu)
    {
        if(l_num_adapters >= *io_num_adapters)
        {
            TRACEE("Warning: io_num_adapters = %d, and we have at least one more adapter. Flagging this as an error.\n", *io_num_adapters);
            l_rc = false;
            break;
        }
        TRACEI("AFU found: '%s'\n", cxl_afu_dev_name(l_afu));
        l_rc = cxl_afu_sysfs_pci(l_afu, &l_pci_path);
        TRACEI("sysfs rc: %d\n", l_rc);
        TRACEI("sysfs path: '%s'\n", l_pci_path);
        l_rc = provGetAdapterInfo(l_pci_path, &o_info[l_num_adapters]);
        if(l_rc == true)
        {
            strncpy(o_info[l_num_adapters].afu_name, cxl_afu_dev_name(l_afu), sizeof(o_info[l_num_adapters].afu_name));
            l_num_adapters++;
        }
        else
        {
            TRACEE("Failure occurred parsing adapter info for '%s'\n",l_pci_path);
            l_rc = false;
            break;
        }

    }
    
    //all exit paths return the same way
    if(l_rc == false)
    {
        *io_num_adapters = 0;
    }
    else
    {
        *io_num_adapters = l_num_adapters;
    }
    return l_rc;
}


bool provGetAdapterInfo(const char* i_pci_path, prov_adapter_info_t* o_info)
{
    //Locals
    bool l_rc = true;
    FILE *l_vpd_file = NULL;
    char l_pci_path[DEV_PATH_LENGTH];
    uint8_t l_vpd_buffer[MAX_VPD_SIZE];
    int l_kw_length = 0;
    size_t n=0;
    
    //Code
    do
    {
        TRACEI("Reading WWPN data...\n");
        if((i_pci_path == NULL) ||
           (strlen(i_pci_path) == 0) ||
           (strlen(i_pci_path) > DEV_PATH_LENGTH) ||
           (o_info == NULL))
        {
            TRACEI("Found null or invalid parm!\n");
            l_rc = false;
            break;
        }
        
        bzero(l_pci_path, DEV_PATH_LENGTH);
        bzero(l_vpd_buffer, MAX_VPD_SIZE);
        bzero(o_info, sizeof(prov_adapter_info_t));
        
        //generate the proper VPD path to get VPD
        snprintf(l_pci_path, DEV_PATH_LENGTH, "%s/%s", i_pci_path, "vpd");
        
        TRACEV("Opening up '%s'\n", l_pci_path);
        
        l_vpd_file = fopen(l_pci_path, "rb");
        if (l_vpd_file)
        {
            n = fread(l_vpd_buffer, 1, MAX_VPD_SIZE, l_vpd_file);
        }
        else
        {
            TRACEI("Unable to read file. Do you have permission to open '%s' ?", l_pci_path);
            l_rc = false;
            break;
            // error opening file
        }

        if(n < MIN_VPD_SIZE)
        {
            l_rc = false;
            TRACEV("Warning: (%ld<%d) Buffer underrun. This indicates a "
                    "potential VPD format problem.\n", n, MAX_VPD_SIZE);
            break;
        }

        TRACEV("Searching for V5 and V6 KW data...\n");
        l_kw_length = 16;
        l_rc = provFindVPDKw("V5", l_vpd_buffer, n, (uint8_t*)o_info->wwpn[0],&l_kw_length);
        if(l_rc == false)
        {
            TRACEE("Error: Unable to find Port name VPD for Port 1 (VPD KW V5)");
            break;
        }
        l_kw_length = 16;
        l_rc = provFindVPDKw("V6", l_vpd_buffer, n, (uint8_t*)o_info->wwpn[1],&l_kw_length);
        if(l_rc == false)
        {
            TRACEE("Error: Unable to find Port name VPD for Port 1 (VPD KW V5)");
            break;
        }
        //set up the output var
        strcpy(o_info->afu_name, "unknown");
        strncpy(o_info->pci_path, i_pci_path, DEV_PATH_LENGTH);
        //TODO - need fns to get VPD data here and fill it in.
        
        //put data in successfully!
        l_rc = true;
        
    } while (0);
    //close the file on all paths, if it was opened
    if(l_vpd_file)
    {
        fclose(l_vpd_file);
        l_vpd_file = NULL;
    }
    return l_rc;
}

uint8_t provGetAllWWPNs(prov_wwpn_info_t* io_wwpn_info, uint16_t *io_num_wwpns)
{
    //Locals
    struct cxl_afu_h *l_afu;
    char *l_pci_path = NULL;
    char l_afu_path[DEV_PATH_LENGTH];
    int l_rc = 0;
    uint16_t l_total_wwpns = 0;
    uint16_t l_adapter_wwpn_count = 0;
    prov_adapter_info_t l_adapter_info;
    //Code
    do
    {
        TRACEI("Getting WWPNs\n");
        if((io_num_wwpns == 0) || io_wwpn_info == NULL)
        {
            TRACEI("Buffer is too small, or null!");
            break;
        }
        cxl_for_each_afu(l_afu)
        {
            bzero(l_afu_path, sizeof(l_afu_path));
            l_pci_path = NULL;
            TRACEI("AFU found: '%s'\n", cxl_afu_dev_name(l_afu));
            //TODO: why do i have to build this myself? should we be
            //always appending the "master" context on the end here?
            snprintf(l_afu_path, sizeof(l_afu_path), "/dev/cxl/%sm", cxl_afu_dev_name(l_afu));
            l_rc = cxl_afu_sysfs_pci(l_afu, &l_pci_path);
            TRACEI("sysfs rc: %d\n", l_rc);
            TRACEI("sysfs path: '%s'\n", l_pci_path);
            TRACEI("afu path: '%s'\n", l_afu_path);
            l_rc = provGetAdapterInfo(l_pci_path, &l_adapter_info);
            //free the buffer so we can reallocate it next time
            if(l_rc == false)
            {
                //skip - invalid data came back
                TRACEI("Invalid VPD found for this adapter. skipping it.\n");
                continue;
            }
            for(l_adapter_wwpn_count = 0; l_adapter_wwpn_count < MAX_WWPNS_PER_ADAPTER; l_adapter_wwpn_count++)
            {
                TRACEI("Got a wwpn: '%s'\n", l_adapter_info.wwpn[l_adapter_wwpn_count]);
                //io_wwpn_info[l_total_wwpns].afu_path
                
                //zero out the next available element
                bzero(&io_wwpn_info[l_total_wwpns], sizeof(prov_wwpn_info_t));
                strncpy(io_wwpn_info[l_total_wwpns].pci_path, l_pci_path, DEV_PATH_LENGTH);
                strncpy(io_wwpn_info[l_total_wwpns].afu_path, l_afu_path, DEV_PATH_LENGTH);
                io_wwpn_info[l_total_wwpns].port_id = l_adapter_wwpn_count;
                strncpy(io_wwpn_info[l_total_wwpns].wwpn, l_adapter_info.wwpn[l_adapter_wwpn_count], WWPN_BUFFER_LENGTH);
                l_total_wwpns++;
            }
            free(l_pci_path);
            l_pci_path = NULL;
            
        }
        if(l_total_wwpns == 0)
        {
            TRACEI("No wwpns found!\n");
        }
        
        
    } while (0);
    
    *io_num_wwpns = l_total_wwpns;
    
    return 0;
    
}
