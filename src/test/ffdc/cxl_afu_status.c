/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/cxl_afu_status.c $                              */
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libcxl.h>
#include <afu_fc.h>
#include <unistd.h>

#define DEBUG 0
#define debug(fmt, ...) do { if (DEBUG) fprintf(stderr,fmt,__VA_ARGS__); fflush(stderr);} while (0)
#define debug0(fmt)     do { if (DEBUG) fprintf(stderr,fmt); fflush(stderr);} while (0)

struct cxl_afu_h *afu;
uint64_t r,addr;
uint64_t FC_STATUS2           = 0x348;
uint64_t FC_ERRINJ_DATA       = 0x3a8;
uint64_t gen1                 = 0b001000000;
uint64_t gen2                 = 0b010000000;
uint64_t gen3                 = 0b100000000;
uint64_t compq_empty          = 0b10000000;
uint64_t subq_empty           = 0b01000000;
uint64_t pcie_init_cmplt      = 0b00001000;
uint64_t ioq_rdy              = 0b00000100;
uint64_t cntrl_rdy            = 0b00000010;
uint64_t link_training_cmplt  = 0b00000001;
uint64_t nvme_port_online     = 0b00001111;

/**
********************************************************************************
** \brief usage
*******************************************************************************/
void usage(void)
{
    printf("Usage: [-d afuX]\n");
    exit(0);
}

/**
********************************************************************************
** \brief show_port
** \details
**
*******************************************************************************/
void show_port(uint32_t port)
{
    uint64_t base;
    uint64_t status2;
    uint32_t num;

    if (port)  base=FC_PORT1_OFFSET;
    else       base=FC_PORT0_OFFSET;

    printf(" NVMe%d: ", port);

    addr=base+FC_STATUS2;
    cxl_mmio_read64(afu, addr,(__u64 *)&status2);
    if (status2&gen1)                    printf("GEN1 ");
    if (status2&gen2)                    printf("GEN2 ");
    if (status2&gen3)                    printf("GEN3 ");
    uint64_t width=((status2>>9) & 0b1111);
    printf("width=%ld ", width);
    if (status2&0b110000000000000)       printf("link up");

    printf("\n");

    if      (status2&0b10000000000000000)printf("   user reset indication\n");
    if      (status2&0b1000000000000000) printf("   core phy link down\n");
    //if      (status2&0b110000000000000)  printf("");
    else if (status2&0b100000000000000)  printf("   link up, init in process\n");
    else if (status2&0b0100000000000000) printf("   link train in progress\n");
    else                                 printf("   no receivers\n");

    addr=base+FC_MTIP_STATUS;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    if (r&0b10000)                       printf("   offline\n");

    addr=base+FC_CONFIG;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    if (r&0b10000000)                    printf("   timeouts disabled\n");
    if (r&0b10000)                       printf("   fc_reset asserted\n");
    if (!r&0b1)                          printf("   autologin not enabled\n");

    addr=base+FC_CONFIG2;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    if (r&0b100000)                      printf("   shutdown abrupt started\n");
    if (r&0b10000)                       printf("   shutdown normal started\n");

    addr=base+FC_STATUS;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    if      (!r&link_training_cmplt)     printf("   link train failed\n");
    else if (!r&cntrl_rdy)               printf("   NVMe controller not rdy\n");
    else if (!r&ioq_rdy)                 printf("   ioq not ready\n");
    else if (!r&pcie_init_cmplt)         printf("   pcie_init failed\n");
    if      (r&0b100000)                 printf("   shutdown complete\n");
    if      (r&0b10000)                  printf("   shutdown active\n");
    if      (!r&compq_empty)             printf("   compq not empty\n");
    if      (!r&subq_empty)              printf("   subq not empty\n");

    addr=base+FC_CNT_LINKERR;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    num=r&0xffff;
    if (num)                             printf("   pcie_resets=%d\n", num);
    num=(r&0xffff0000)>>16;
    if (num)                             printf("   cntrlr_resets=%d\n",num);

    addr=base+FC_CNT_CRCTOT;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    num=r&0xffffffff;
    if (num)                             printf("   crc_errs=%d\n", num);

    addr=base+FC_CNT_TIMEOUT;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    num=r&0xffffffff;
    if (num)                             printf("   timeouts=%d\n", num);

    addr=base+FC_CNT_AFUABORT;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    num=r&0xffffffff;
    if (num)                             printf("   afuaborts=%d\n", num);

    addr=base+FC_ERROR;
    cxl_mmio_read64(afu, addr,(__u64 *)&r);
    num=r&0xffffffff;
    if (num & 0x100000)                  printf("   PORT_LOGIN_FAILURE\n");
}

/**
********************************************************************************
** \brief get_str
** \details
**
*******************************************************************************/
void get_str(char *out, __u64 in)
{
  int i;
  for (i=0; i<8; i++) {
    out[i] = (in & 0xFFl);
    in = in >> 8;
  }
  out[8] = 0;
}

/**
********************************************************************************
** \brief main
** \details
**
*******************************************************************************/
int main(int argc, char **argv)
{
  struct cxl_ioctl_start_work *work;
  long  reported  = 0;
  char  FF        = 0xFF;
  char  c         = '\0';
  char  dev[128];
  char *_dev      = NULL;

  /*--------------------------------------------------------------------------
   * process and verify input parms
   *------------------------------------------------------------------------*/
  while (FF != (c=getopt(argc, argv, "d:h")))
  {
      switch (c)
      {
          case 'd': _dev = optarg; break;
          case 'h':
          case '?': usage();
      }
  }
  if (!_dev) {usage();}
  if (strncmp(_dev,"afu",3) == 0) {sprintf(dev, "/dev/cxl/%s",_dev);}
  else                            {sprintf(dev, "%s",_dev);}

  afu = cxl_afu_open_dev(dev);
  if (!afu) {printf("unable to open %s", dev); exit(-1);}

  debug("Allocated memory at %p for AFU\n", afu);
  debug("Creating and opening AFU file descriptor %s\n", dev);

  work = (struct cxl_ioctl_start_work*) cxl_work_alloc();
  if (!work) {perror("cxl_work_alloc"); exit(-2);}
  if (cxl_afu_attach_work(afu, work))
  {
      perror("cxl_afu_attach_work failed\n");
      exit(-3);
  }

  debug0("Start command succeeded on AFU\n");

  if (cxl_mmio_map(afu, CXL_MMIO_BIG_ENDIAN) != 0)
  {
    perror("cxl_mmio_map");
    exit(-4);
  }

  cxl_get_mmio_size(afu, &reported);
  debug ("mmio_size 0x%lx for AFU\n", reported);

  debug0 ("Installing libcxl SIGBUS handler\n");
  cxl_mmio_install_sigbus_handler();

  show_port(0);
  show_port(1);

  exit(0);
}
