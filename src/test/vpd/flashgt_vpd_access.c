/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/vpd/flashgt_vpd_access.c $                           */
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

uint64_t mmio_read(char resource_file[],uint64_t offset);
uint64_t mmio_write(char resource_file[],uint64_t offset,uint64_t val64);
uint64_t query_i2c_ready(char resource_file[]);
uint64_t query_i2c_error(char resource_file[]);
uint64_t query_i2c_readdata_val(char resource_file[]);
void     poll_vpd_read_done(int basecfg, int cfgaddr);
void poll_vpd_write_done(int basecfg, int cfgaddr);

int main (int argc, char *argv[])
{
  int CFG;
  int VPDRBF;
  char device_num[1024];
  char bar2[1024];

  char cfg_file[1024];
  char vpdrbf_file[1024];

  if (argc < 3) {
    printf("Usage: ./flashgt_vpd_access <pci_number> <card#> <output vpd bin name>\nExample: ./flashgt_vpd_access 0000:01:00.0 0 vpd_FlashGT_050916.vpdrbf\n");
    return -1;
  }
  strcpy (device_num, argv[1]);
  strcpy(bar2, "/sys/bus/pci/devices/");
  strcat(bar2, device_num);
  strcat(bar2, "/resource2");

  strcpy(cfg_file, "/sys/class/cxl/card");
  strcat(cfg_file, argv[2]);
  strcat(cfg_file, "/device/config");

  if ((CFG = open(cfg_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",cfg_file);
    exit(-1);
  }


  strcpy(vpdrbf_file, argv[3]);

  //printf("Opening VPD FILE, %s\n",vpdrbf_file);
  if ((VPDRBF = open(vpdrbf_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",vpdrbf_file);
    exit(-1);
  }

  int temp,vendor,device;
  lseek(CFG, 0, SEEK_SET);
  read(CFG, &temp, 4);
  vendor = temp & 0xFFFF;
  device = (temp >> 16) & 0xFFFF;  
  printf("Device ID: %04X\n", device);
  printf("Vendor ID: %04X\n", vendor);

  if ( (vendor != 0x1014) || (( device != 0x0477) && (device != 0x04cf) && (device != 0x0601))) {
	printf("Unknown Vendor or Device ID\n");
	exit(-1);
  }
  
  int addr_reg, data_reg;
  uint64_t mmio_i2c_addr = 0x0000000000000130;
  lseek(CFG, 0x404, SEEK_SET);
  read(CFG, &temp,4);
  printf("  VSEC Length/VSEC Rev/VSEC ID: 0x%08X\n", temp);
    addr_reg = 0x40;
    data_reg = 0x44;
 
  // Set stdout to autoflush
  setvbuf(stdout, NULL, _IONBF, 0);

// -------------------------------------------------------------------------------
// MMIOs to UCD
// -------------------------------------------------------------------------------
uint64_t mmio_write_data = 0x00000000000000AB;
uint64_t mmio_read_data = 0x0;
uint8_t i2c_read_data = 0x00;
uint64_t i = 0;
int ready_status = 1;

  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(bar2);
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

  mmio_write_data = 0x00030001FAD00006;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FAD00006;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(bar2);
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

  query_i2c_error(bar2);

  mmio_write_data = 0x00010001FAD10006;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FAD10006;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(bar2);
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

  query_i2c_error(bar2);

  //Final Check to make sure GPIO select took effect
  if(i2c_read_data == 0x06) {
    printf("Successfully targetting proper GPIO\n");
  }
  else {
    printf("ERROR: I2C to target EEPROM wp GPIO did not work\n");
  }

  mmio_write_data = 0x00030001FBD00003;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FBD00003;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(bar2);
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

  query_i2c_error(bar2);

  mmio_write_data = 0x00010001FBD10003;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FBD10003;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(bar2);
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

  query_i2c_error(bar2);

  //Final Check to make sure GPIO took down the write protect
  if(i2c_read_data == 0x03) {
    printf("Successfully changed write protect GPIO value\n");
  }
  else {
    printf("ERROR: I2C to alter EEPROM wp value did not work\n");
  }

  mmio_write_data = 0x00030001FBD00002;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FBD00002;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    ready_status = query_i2c_ready(bar2);
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

  query_i2c_error(bar2);
  //lseek(CFG, cntl_reg, SEEK_SET);
  mmio_write_data = 0x00010001FBD10002;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);
  mmio_write_data = 0x00000001FBD10002;
  mmio_write(bar2,mmio_i2c_addr,mmio_write_data);

  ready_status = 1;
  for(i = 0; i < 100; i++) {
    mmio_read_data = query_i2c_readdata_val(bar2);
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

  query_i2c_error(bar2);

  //Final Check to make sure GPIO change is made temporary
  if(i2c_read_data == 0x02) {
    printf("Successfully made write protect change temporary\n");
  }
  else {
    printf("ERROR: I2C to make wp change temporary did not work. Please do not do further i2c operations to UCD\n");
  }

  //lseek(CFG, cntl_reg, SEEK_SET);
  //write(CFG,&temp,4);
 

// -------------------------------------------------------------------------------
// Read and Write VPD Segments
// -------------------------------------------------------------------------------

  uint32_t vpd_data = 0x0;
  uint32_t vpd_f_addr_struct = 0x00000000;
  int rawbinaddr = 0;
  int readvalid = 1;
//  uint32_t vpd[32];

  //Loop to read original vpd contents
  printf("Current VPD contents are: \n");
  for(int j = 0; j < 64; j++) {
    lseek(CFG, addr_reg, SEEK_SET);
    write(CFG, &vpd_f_addr_struct,4);

    poll_vpd_read_done(CFG, addr_reg);
    
    lseek(CFG, data_reg, SEEK_SET);
    read(CFG, &vpd_data,4);
//    vpd[j] = vpd_data;
    //lseek(VPDRBF, rawbinaddr, SEEK_SET);
    //write(VPDRBF, &vpd[j], 4);
    printf("0x%08x\n",vpd_data);
    vpd_f_addr_struct = vpd_f_addr_struct + 0x00040000;
  }

  //Loop to write new vpd contents
  printf("Writing in new VPD contents:\n");
  vpd_f_addr_struct = 0x80000000;
  for(int j = 0; j < 64; j++) {
    if (readvalid)
    {
      lseek(VPDRBF, rawbinaddr, SEEK_SET);
      readvalid = read(VPDRBF, &vpd_data, 4);
    }
    else {vpd_data = 0xFFFFFFFF;}

    printf("0x%08x\n",vpd_data);
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
  exit(0);
}



uint64_t mmio_read (char resource_file[],uint64_t offset)
{
  int priv2;
  struct stat sb;
  void *memblk, *adr;
  uint64_t val64;

  if ((priv2 = open(resource_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",resource_file);
    exit(-1);
  }

  fstat(priv2, &sb);

  memblk = mmap(NULL,sb.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, priv2, 0);
  if (memblk == MAP_FAILED) {
    printf("Can not mmap %s\n",resource_file);
    exit(-1);
  }

  //offset = strtoul(argv[2], 0, 0);

  adr = memblk + (offset & (sb.st_size - 1));

  //printf("\n");
      val64 = *((uint64_t *)adr);
      //printf("Reading double : [%jx]= 0x%016jx\n",adr, val64);

  munmap(memblk,sb.st_size);

  //printf("\n");
  close(priv2);
  val64 = ((val64>>56)&0xff) | ((val64>>40)&0xff00) | ((val64>>24)&0xff0000) | ((val64>>8)&0xff000000)  | ((val64<<8)&0xff00000000) | ((val64<<24)&0xff0000000000) | ((val64<<40)&0xff000000000000) | ((val64<<56)&0xff00000000000000);
  //printf("Reading double : [%jx]= 0x%016jx\n",adr, val64);
  return val64;
}

uint64_t mmio_write (char resource_file[],uint64_t offset, uint64_t val64)
{
  int priv2;
  struct stat sb;
  void *memblk, *adr;

  if ((priv2 = open(resource_file, O_RDWR)) < 0) {
    printf("Can not open %s\n",resource_file);
    exit(-1);
  }

  fstat(priv2, &sb);

  memblk = mmap(NULL,sb.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, priv2, 0);
  if (memblk == MAP_FAILED) {
    printf("Can not mmap %s\n",resource_file);
    exit(-1);
  }


  adr = memblk + (offset & (sb.st_size - 1));

  //printf("\n");
//printf("Writing double : [%jx]= 0x%016jx\n",adr, val64);
  val64 = ((val64>>56)&0xff) | ((val64>>40)&0xff00) | ((val64>>24)&0xff0000) | ((val64>>8)&0xff000000)  | ((val64<<8)&0xff00000000) | ((val64<<24)&0xff0000000000) | ((val64<<40)&0xff000000000000) | ((val64<<56)&0xff00000000000000);
      *((uint64_t *)adr) = val64;
      //printf("Writing double : [%jx]= 0x%016jx\n",adr, val64);

  munmap(memblk,sb.st_size);
  close(priv2);

  //printf("\n");
    return 0;
}

uint64_t query_i2c_ready(char resource_file[])
{
  uint64_t offset = 0x0000000000000130;
  uint64_t read_data;

  read_data = mmio_read(resource_file, offset);
  if((read_data & 0x8000000000000000) == 0x8000000000000000) {
    printf("I2C controller is ready\n");
    return 0;
  }
  else return 1;
}

uint64_t query_i2c_error(char resource_file[])
{
  uint64_t offset = 0x0000000000000130;
  uint64_t read_data;

  read_data = mmio_read(resource_file, offset);
  if((read_data & 0x2000000000000000) == 0x2000000000000000) {
    printf("I2C controller is in error\n");
    exit(-1);
  }
  else return 1;
}

uint64_t query_i2c_readdata_val(char resource_file[])
{
  uint64_t offset = 0x0000000000000130;
  uint64_t read_data;

  read_data = mmio_read(resource_file, offset);
  if((read_data & 0x4000000000000000) == 0x4000000000000000) {
    printf("I2C controller read data is valid\n");
    return read_data;
  }
  else return 1;
}

void poll_vpd_read_done(int basecfg, int cfgaddr) {
  int i = 0;
  uint32_t temp;

  for(i = 0; i < 10000; i++) {
    lseek(basecfg, cfgaddr, SEEK_SET);
    read(basecfg,&temp,4);
    if((temp & 0x80000000) == 0x80000000) {
      return;
    }
    else if(i >= 9999) {
      printf("ERROR: VPD Read is not finishing\n");
      printf("VPD reg value is %08x\n",temp);
      exit(-1);
    }
  }
}

void poll_vpd_write_done(int basecfg, int cfgaddr) {
  int i = 0;
  uint32_t temp;

  for(i = 0; i < 1000000; i++) {
    lseek(basecfg, cfgaddr, SEEK_SET);
    read(basecfg,&temp,4);
    if((temp & 0x80000000) == 0x00000000) {
      return;
    }
    else if(i >= 999999) {
      printf("ERROR: VPD Write is not finishing\n");
      exit(-1);
    }
  }
}
