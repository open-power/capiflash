/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/vpd/flashgt_vpd_dump.c $                             */
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

// dump VPD data from FlashGT+ card via I2C

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <endian.h>

uint64_t poll_vpd_read_done(int basecfg, int cfgaddr);

int main (int argc, char *argv[])
{
  int CFG;
  char device_num[1024];
  char cfg_file[1024];

  if (argc != 2) {
    printf("Usage: flashgtp_vpd_dump <pci_number> \nExample: flashgtp_vpd_dump 0000:01:00.0\n");
    return -1;
  }

  strcpy (device_num, argv[1]);
  strcpy(cfg_file, "/sys/bus/pci/devices/");
  strcat(cfg_file, device_num);
  strcat(cfg_file, "/config");


  if ((CFG = open(cfg_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",cfg_file);
    exit(-1);
  }

  int temp,vendor,device;
  lseek(CFG, 0, SEEK_SET);
  read(CFG, &temp, 4);
  vendor = temp & 0xFFFF;
  device = (temp >> 16) & 0xFFFF;  

  if ( (vendor != 0x1014) || (( device != 0x0477) && (device != 0x04cf) && (device != 0x0628) && (device != 0x9038))) {
	printf("Unknown Vendor or Device ID\n");
	exit(-1);
  }
  
  int addr_reg, data_reg;
  lseek(CFG, 0x404, SEEK_SET);
  read(CFG, &temp,4);
  addr_reg = 0xB0;  // PCIe config space address of the VPD capability
  data_reg = 0xB4;
 
  // Set stdout to autoflush
  setvbuf(stdout, NULL, _IONBF, 0);

// -------------------------------------------------------------------------------
// Read VPD Segments only
// -------------------------------------------------------------------------------

  uint32_t vpd_data = 0x0;
  uint32_t vpd_f_addr_struct = 0x00000000;

  // set bit in i2c register to make sure we're not reading hardcoded vpd
  uint32_t use_realvpd = 0x10000000;
  lseek(CFG,0x41c,SEEK_SET);
  write(CFG,&use_realvpd,4);

  //Loop to read original vpd contents
  int j=0;
  for(j=0; j<64; j++) {
    lseek(CFG, addr_reg, SEEK_SET);
    write(CFG, &vpd_f_addr_struct,4);

    poll_vpd_read_done(CFG, addr_reg);
    
    lseek(CFG, data_reg, SEEK_SET);
    read(CFG, &vpd_data,4);
    printf("%08x: %08x\n",j*4, htobe32(vpd_data));
    vpd_f_addr_struct = vpd_f_addr_struct + 0x00040000;
  }

  use_realvpd = 0;
  lseek(CFG,0x41c,SEEK_SET);
  write(CFG,&use_realvpd,4);

  close(CFG);
  return 0;
}

//-------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------


uint64_t poll_vpd_read_done(int basecfg, int cfgaddr) {
  int i = 0;
  uint32_t temp;

  for(i = 0; i < 10000; i++) {
    lseek(basecfg, cfgaddr, SEEK_SET);
    read(basecfg,&temp,4);
    if((temp & 0x80000000) == 0x80000000) {
      return 0;
    }
    else if(i >= 9999) {
      printf("ERROR: VPD Read is not finishing\n");
      printf("VPD reg value is %08x\n",temp);
      exit(-1);
    }
  }
  return 0;
}
