/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/vpd/flashgt_vpd_write.c $                            */
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

void i2cacc_write(int cfgfh, uint64_t wrdata);
uint64_t i2cacc_read(int cfgfh);
uint64_t query_i2c_ready(int cfgfh);
uint64_t query_i2c_error(int cfgfh);
uint64_t query_i2c_readdata_val(int cfgfh);
uint64_t poll_vpd_read_done(int basecfg, int cfgaddr);
uint64_t poll_vpd_write_done(int basecfg, int cfgaddr);

int main (int argc, char *argv[])
{
  int CFG;
  int VPDRBF;
  char device_num[1024];

  char cfg_file[1024];
  char vpdrbf_file[1024];

//  uint32_t vpd[512];

  if (argc != 3) {
    printf("Write VPD data to flashGT+ card\nUsage: ./flashgtp_vpd_access <pci_number> <binary vpd filename>\nExample: ./flashgtp_vpd_access 0000:01:00.0 vpd_FlashGT_050916.vpdrbf\n");
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

  strcpy(vpdrbf_file, argv[2]);

  if ((VPDRBF = open(vpdrbf_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",vpdrbf_file);
    exit(-1);
  }

  int temp,vendor,device;
  lseek(CFG, 0, SEEK_SET);
  read(CFG, &temp, 4);
  vendor = temp & 0xFFFF;
  device = (temp >> 16) & 0xFFFF;  

  if ( (vendor != 0x1014) || (device != 0x0628)) {
	printf("Unknown Vendor or Device ID\n");
	exit(-1);
  }
  
  int addr_reg, data_reg;
  lseek(CFG, 0x404, SEEK_SET);
  read(CFG, &temp,4);
  addr_reg = 0xB0;
  data_reg = 0xB4;
 
  // Set stdout to autoflush
  setvbuf(stdout, NULL, _IONBF, 0);

// -------------------------------------------------------------------------------
// MMIOs to UCD
// -------------------------------------------------------------------------------
uint8_t i2c_read_data = 0x00;
uint64_t i = 0, j=0;
int ready_status = 1;
uint64_t mmio_read_data;

int ucd_error = 0;

  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(CFG);
    if(ready_status == 0) {
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  i2cacc_write(CFG,0x00030001FAD00006);
  i2cacc_write(CFG,0x00000001FAD00006);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(CFG);
    if(ready_status == 0) {
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);

  i2cacc_write(CFG,0x00010001FAD10006);
  i2cacc_write(CFG,0x00000001FAD10006);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(CFG);
    if((mmio_read_data & 0x8000000000000000) == 0x8000000000000000) {
      ready_status = 0;
    }
    if(ready_status == 0) {
      i2c_read_data = ((mmio_read_data & 0x000000000000FF00) >> 8); 
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);

  //Final Check to make sure GPIO select took effect
  if(i2c_read_data != 0x06) {
    printf("ERROR: I2C to target EEPROM wp GPIO did not work\n");
    ucd_error++;
  }
  i2cacc_write(CFG,0x00030001FBD00003);
  i2cacc_write(CFG,0x00000001FBD00003);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(CFG);
    if(ready_status == 0) {
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);

  i2cacc_write(CFG,0x00010001FBD10003);
  i2cacc_write(CFG,0x00000001FBD10003);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(CFG);
    if((mmio_read_data & 0x8000000000000000) == 0x8000000000000000) {
      ready_status = 0;
    }
    if(ready_status == 0) {
      i2c_read_data = ((mmio_read_data & 0x000000000000FF00) >> 8);
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);

  //Final Check to make sure GPIO took down the write protect
  if(i2c_read_data != 0x03)
  {
    printf("ERROR: I2C to alter EEPROM wp value did not work\n");
    ucd_error++;
  }

  i2cacc_write(CFG,0x00030001FBD00002);
  i2cacc_write(CFG,0x00000001FBD00002);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(CFG);
    if(ready_status == 0) {
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);
  //lseek(CFG, cntl_reg, SEEK_SET);
  i2cacc_write(CFG,0x00010001FBD10002);
  i2cacc_write(CFG,0x00000001FBD10002);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(CFG);
    if((mmio_read_data & 0x8000000000000000) == 0x8000000000000000) {
      ready_status = 0;
    }
    if(ready_status == 0) {
      i2c_read_data = ((mmio_read_data & 0x000000000000FF00) >> 8);
      break;
    }
    else if((i == 99) && (ready_status != 0)) {
      printf("I2C Failed to be ready\n");
      exit(-1);
    }
    else {
      printf("I2C Not ready yet...\r");
    }
  }

  ucd_error += query_i2c_error(CFG);

  //Final Check to make sure GPIO change is made temporary
  if(i2c_read_data == 0x02) {
  }
  else {
    printf("ERROR: I2C to make wp change temporary did not work. Please do not do further i2c operations to UCD\n");
  }

  //lseek(CFG, cntl_reg, SEEK_SET);
  //write(CFG,&temp,4);
 
  if( ucd_error > 0 ) {
    printf("Error during write protect disable.  Confirm that the jtag breakout card is disconnected from FlashGT+ card and try again.\n");
    exit(-1);
  }

// -------------------------------------------------------------------------------
// Read and Write VPD Segments
// -------------------------------------------------------------------------------

  uint32_t vpd_data = 0x0;
  uint32_t vpd_f_addr_struct = 0x00000000;
  int rawbinaddr = 0;
  int readvalid = 1;

  // select non-hardcoded VPD
  i2cacc_write(CFG,0x1000000000000000);

  //Loop to read original vpd contents
  for(j = 0; j < 64; j++) {
    lseek(CFG, addr_reg, SEEK_SET);
    write(CFG, &vpd_f_addr_struct,4);

    poll_vpd_read_done(CFG, addr_reg);
    
    lseek(CFG, data_reg, SEEK_SET);
    read(CFG, &vpd_data,4);
//    vpd[j] = vpd_data;
    //lseek(VPDRBF, rawbinaddr, SEEK_SET);
    //write(VPDRBF, &vpd[j], 4);
    vpd_f_addr_struct = vpd_f_addr_struct + 0x00040000;
  }

  //Loop to write new vpd contents
  vpd_f_addr_struct = 0x80000000;
  for(j = 0; j < 64; j++) {
    lseek(VPDRBF, rawbinaddr, SEEK_SET);
    if (readvalid) {readvalid = read(VPDRBF, &vpd_data, 4);}
    if(!readvalid) {
      vpd_data = 0xFFFFFFFF;
    }
    lseek(CFG, data_reg, SEEK_SET);
    write(CFG, &vpd_data, 4);
    lseek(CFG, addr_reg, SEEK_SET);
    write(CFG, &vpd_f_addr_struct,4);

    poll_vpd_write_done(CFG, addr_reg);

    //lseek(CFG, data_reg, SEEK_SET);
    //read(CFG, &vpd_data,4);
    //vpd[j] = vpd_data;
    //lseek(VPDRBF, rawbinaddr, SEEK_SET);
    //write(VPDRBF, &vpd[j], 4);
    rawbinaddr = rawbinaddr + 4;
    vpd_f_addr_struct = vpd_f_addr_struct + 0x00040000;
  }

  close(VPDRBF);
  close(CFG);
  return 0;
}

uint64_t i2cacc_read(int cfgfh)
{
  uint32_t  temp;
  uint64_t rddata;

  lseek(cfgfh, 0x418, SEEK_SET);
  read(cfgfh, &temp, 4);
  rddata = temp;
  lseek(cfgfh, 0x41C, SEEK_SET);
  read(cfgfh, &temp, 4);
  rddata |= ((uint64_t)temp)<<32;
  return rddata;
}

void i2cacc_write(int cfgfh, uint64_t wrdata)
{
  int temp;
  temp = wrdata & 0xFFFFFFFF;
  lseek(cfgfh, 0x418, SEEK_SET);
  write(cfgfh, &temp, 4);
  temp = (wrdata >> 32);
  lseek(cfgfh, 0x41C, SEEK_SET);
  write(cfgfh, &temp, 4);
}


uint64_t query_i2c_ready(int cfgfh)
{
  uint64_t read_data;

  read_data = i2cacc_read(cfgfh);
  if((read_data & 0x8000000000000000) == 0x8000000000000000) {
    return 0;
  }
  else return 1;
}

uint64_t query_i2c_error(int cfgfh)
{
  uint64_t read_data;

  read_data = i2cacc_read(cfgfh);
  if((read_data & 0x2000000000000000) == 0x2000000000000000) {
    printf("I2C controller is in error\n");
    return 1;
  }
  else return 0;
}

uint64_t query_i2c_readdata_val(int cfgfh)
{
  uint64_t read_data;

  read_data = i2cacc_read(cfgfh);
  if((read_data & 0x4000000000000000) == 0x4000000000000000) {
    return read_data;
  }
  else return 1;
}

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

uint64_t poll_vpd_write_done(int basecfg, int cfgaddr) {
  int i = 0;
  uint32_t temp;

  for(i = 0; i < 1000000; i++) {
    lseek(basecfg, cfgaddr, SEEK_SET);
    read(basecfg,&temp,4);
    if((temp & 0x80000000) == 0x00000000) {
      return 0;
    }
    else if(i >= 999999) {
      printf("ERROR: VPD Write is not finishing\n");
      exit(-1);
    }
  }
  return 0;
}
