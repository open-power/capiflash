/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/flashgt_temp.c $                                     */
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

int main (int argc, char *argv[])
{
  int CFG;
  char device_num[1024];
  char bar2[1024];
  char cfg_file[1024];

  if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
    printf("Usage: ./flashgt_temp <pci_number> <card#>\nExample: ./flashgt_temp 0000:01:00.0 0\n");
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

  int temp,vendor,device;
  lseek(CFG, 0, SEEK_SET);
  read(CFG, &temp, 4);
  vendor = temp & 0xFFFF;
  device = (temp >> 16) & 0xFFFF;  
  // printf("Device ID: %04X\n", device);
  //printf("Vendor ID: %04X\n", vendor);

  if ( (vendor != 0x1014) || (( device != 0x0477) && (device != 0x04cf) && (device != 0x0601))) {
	printf("Unknown Vendor or Device ID\n");
	exit(-1);
  }
  
  uint64_t mmio_ptmon_addr = 0x0000000000000138;
  lseek(CFG, 0x404, SEEK_SET);
  read(CFG, &temp,4);
  //printf("  VSEC Length/VSEC Rev/VSEC ID: 0x%08X\n", temp);
 
  // Set stdout to autoflush
  setvbuf(stdout, NULL, _IONBF, 0);

// -------------------------------------------------------------------------------
// MMIOs to UCD
// -------------------------------------------------------------------------------
uint64_t mmio_write_data = 0xFFFFFFFFFFFFFFFF;
uint64_t mmio_read_data = 0x0;
uint8_t temperature = 0x00;
float temperature2 = 0;

  //printf("Enabling Temperature Monitor\n");
  mmio_write(bar2,mmio_ptmon_addr,mmio_write_data);

  mmio_read_data = mmio_read(bar2,mmio_ptmon_addr);
  //printf("PTMON data is %016llx \n", mmio_read_data);
  temperature = (mmio_read_data >> 40);//16 bit mmio field is 8.8 format. Get 8 bit integer portion here.
  temperature2 = temperature + ((mmio_read_data << 24) >> 56)/256.00;//Add 8 bit unsigned integer portion to 8 bit decimal portion.
  printf("FPGA Temperature is %8.8f degrees Celsius\n", temperature2);

  //printf("Disabling Temperature Monitor\n");
  mmio_write_data = 0x0000000000000000;
  mmio_write(bar2,mmio_ptmon_addr,mmio_write_data);

  close(CFG);
  return 0;
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


  adr = memblk + (offset & (sb.st_size - 1));

      val64 = *((uint64_t *)adr);

  munmap(memblk,sb.st_size);

  close(priv2);
  val64 = ((val64>>56)&0xff) | ((val64>>40)&0xff00) | ((val64>>24)&0xff0000) | ((val64>>8)&0xff000000)  | ((val64<<8)&0xff00000000) | ((val64<<24)&0xff0000000000) | ((val64<<40)&0xff000000000000) | ((val64<<56)&0xff00000000000000);
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

  val64 = ((val64>>56)&0xff) | ((val64>>40)&0xff00) | ((val64>>24)&0xff0000) | ((val64>>8)&0xff000000)  | ((val64<<8)&0xff00000000) | ((val64<<24)&0xff0000000000) | ((val64<<40)&0xff000000000000) | ((val64<<56)&0xff00000000000000);
      *((uint64_t *)adr) = val64;

  munmap(memblk,sb.st_size);
  close(priv2);

    return 0;
}

