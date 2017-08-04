/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/cxl_afu_inject.c $                              */
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <libcxl.h>
#include <afu_fc.h>

int      rc=0;
uint32_t DEBUG=0;
#define debug(fmt, ...) do { if (DEBUG) fprintf(stderr,fmt,__VA_ARGS__); fflush(stderr);} while (0)
#define debug0(fmt)     do { if (DEBUG) fprintf(stderr,fmt); fflush(stderr);} while (0)

#define RESET        0
#define DOWN         1
#define READ_LBA     2
#define WRITE_LBA    3
#define READ_DMA     4
#define WRITE_DMA    5
#define READ_BAD     6
#define WRITE_BAD    7
#define CLEAR_PERM   8
#define PARITY_ON_WR 9

struct cxl_afu_h *afu;
uint64_t r,addr;
uint64_t FC_STATUS2           = 0x348;
uint64_t FC_ERRINJ_DATA       = 0x3a8;
uint64_t FC_PE_ERRINJ         = 0x3B0;
uint64_t FC_PE_ERRINJ_DATA    = 0x3B8;
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
    printf("Usage:\n");
    printf("  [-d afuX] [-p port] [-a action] [-l lba] [-P] [-i interval]\n");
    printf("  interval: #cmds before an inject\n");
    printf("  action: 0-PORT_RESET    1-PORT_OFFLINE\n");
    printf("          2-READ_LBA_ERR  3-WRITE_LBA_ERR\n");
    printf("          4-READ_DMA_ERR  5-WRITE_DMA_ERR\n");
    printf("          6-READ_BAD_DATA 7-WRITE_BAD_DATA\n");
    printf("          8-CLEAR_PERM_RD_WR_ERR\n");
    printf("          9-PARITY_ERR_ON_WRITE\n");
    exit(0);
}

/**
********************************************************************************
** \brief nvme_link
** \details
**   reset an NVMe port, or set it offline
*******************************************************************************/
void nvme_link(uint32_t port, uint32_t state)
{
    uint64_t base;
    int      i=0;

    SET_BASE(port,base);

    addr = base+FC_CONFIG;

    /* set offline */
    if (!state)
    {
        cxl_mmio_read64(afu, addr, &r);
        debug("rd FC_CONFIG: 0x%016lx\n", r);
        r |= 0x10;   //set reset
        r &= ~0x1;   //clear auto-login
        debug("wr FC_CONFIG: 0x%016lx\n", r);
        cxl_mmio_write64(afu, addr, r);
        for (i=0; i<10; i++)
        {
            usleep(100000);
            addr=base+FC_STATUS;
            cxl_mmio_read64(afu, addr, &r);
            debug("rd FC_STATUS: 0x%016lx\n", r);
            if ((r&nvme_port_online) != nvme_port_online) {break;}
        }
        if ((r&nvme_port_online) == nvme_port_online)
        {
            printf("NVMe port offline failed\n");
            rc=-5;
        }
    }
    /* reset or set online  */
    else
    {
        cxl_mmio_read64(afu, addr, &r);
        debug("rd FC_CONFIG: 0x%016lx\n", r);
        r |= 0x11;   //set reset and auto-login
        debug("wr FC_CONFIG: 0x%016lx\n", r);
        cxl_mmio_write64(afu, addr, r);
        for (i=0; i<10; i++)
        {
            usleep(100000);
            addr=base+FC_STATUS;
            cxl_mmio_read64(afu, addr, &r);
            debug("rd FC_STATUS: 0x%016lx\n", r);
            if ((r&nvme_port_online) == nvme_port_online) {break;}
        }
        if ((r&nvme_port_online) != nvme_port_online)
        {
            printf("NVMe port online failed\n");
            rc=-6;
        }
    }
}

/**
********************************************************************************
** \brief io_error
** \details
**   mmio write two regs for the rd/wr inject
*******************************************************************************/
void io_error(uint32_t port,
              uint32_t type,
              uint32_t lba,
              uint32_t perm,
              uint64_t status,
              uint32_t interval)
{
    uint64_t base;
    uint64_t errinj = 0x80000000 | interval;
    uint64_t data   = (status<<32)|lba;

    SET_BASE(port,base);

    if (type == CLEAR_PERM) {errinj=0; data=0;}
    else                    {errinj|=(type<<20);}

    if (perm) {errinj|=0b110000000000000000;}
    else      {errinj|=0b010000000000000000;}

    debug("port:%d type:%d lba:%.8d perm:%d status:%.2lx interval:%d\n",
            port,type,lba,perm,status,interval);
    debug("ERRINJ:%016lx : ERRINJ_DATA:%016lx\n", errinj, data);
    addr=base+FC_ERRINJ;
    cxl_mmio_write64(afu, addr, errinj);

    addr=base+FC_ERRINJ_DATA;
    cxl_mmio_write64(afu, addr, data);
}

/**
********************************************************************************
** \brief parity_error
** \details
**   mmio write two regs for the parity inject
*******************************************************************************/
void parity_err(uint32_t port,
                uint32_t type,
                uint32_t lba,
                uint32_t perm,
                uint64_t status,
                uint32_t interval)
{
    uint64_t base;
    uint64_t errinj = 0;
    uint64_t data   = 0;

    SET_BASE(port,base);

    debug("port:%d type:%d lba:%.8d perm:%d status:%.2lx interval:%d\n",
            port,type,lba,perm,status,interval);

    addr=base+FC_PE_ERRINJ_DATA;
    debug("addr:%016lx : ERRINJ_DATA:%016lx\n", addr, data);
    cxl_mmio_write64(afu, addr, data);

    addr=base+FC_PE_ERRINJ;
    debug("addr:%016lx : ERRINJ:%016lx\n", addr, errinj);
    cxl_mmio_write64(afu, addr, errinj);

    errinj = 1;
    data   = 0x100400;

    addr=base+FC_PE_ERRINJ_DATA;
    debug("addr:%016lx : ERRINJ_DATA:%016lx\n", addr, data);
    cxl_mmio_write64(afu, addr, data);

    addr=base+FC_PE_ERRINJ;
    debug("addr:%016lx : ERRINJ:%016lx\n", addr, errinj);
    cxl_mmio_write64(afu, addr, errinj);
}

/**
********************************************************************************
** \brief main
*******************************************************************************/
int main(int argc, char **argv)
{
    struct cxl_ioctl_start_work *work;
    long     mmio_size = 0;
    char     FF        = 0xFF;
    char     c         = '\0';
    char     dev[128];
    char    *_dev      = NULL;
    char    *_port     = NULL;
    char    *_action   = NULL;
    char    *_lba      = NULL;
    char    *_interval = NULL;
    char    *_status   = NULL;
    uint32_t port      = 0;
    uint32_t action    = 0;
    uint32_t lba       = 0;
    uint32_t interval  = 0;
    uint32_t status    = 0;
    uint32_t perm      = 0;
    uint32_t verbose   = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:p:a:l:i:s:Pvh")))
    {
        switch (c)
        {
            case 'd': _dev       = optarg; break;
            case 'p': _port      = optarg; break;
            case 'a': _action    = optarg; break;
            case 'l': _lba       = optarg; break;
            case 'i': _interval  = optarg; break;
            case 's': _status    = optarg; break;
            case 'P': perm       = 1;      break;
            case 'v': verbose    = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (verbose)   {DEBUG=1;}
    if (!_dev)     {usage();}
    if (_port)     port     = atoi(_port);
    if (_action)   action   = atoi(_action);
    if (_lba)      lba      = atoi(_lba);
    if (_interval) interval = atoi(_interval);
    if (_status)   status   = atoi(_status);

    if (strncmp(_dev,"afu",3) == 0) {sprintf(dev, "/dev/cxl/%s",_dev);}
    else                            {sprintf(dev, "%s",_dev);}
    afu = cxl_afu_open_dev(dev);
    if (!afu)
    {
      printf("open of %s failed\n", dev);
      exit(-1);
    }
    debug("Allocated memory at 0x%016lx for AFU\n", (__u64) afu);

    debug("Creating and opening %s\n", dev);
    work = (struct cxl_ioctl_start_work*) cxl_work_alloc();
    if (work == NULL)
    {
        perror("cxl_work_alloc");
        exit(-2);
    }
    if (cxl_afu_attach_work(afu, work))
    {
        debug0 ("Start command to AFU failed\n");
        perror("cxl_afu_attach_work(master)");
        exit(-3);
    }

    debug0("Start command succeeded on AFU\n");

    if (cxl_mmio_map(afu, CXL_MMIO_BIG_ENDIAN) != 0)
    {
        perror ("mmap_problem_state_registers");
        exit(-4);
    }

    cxl_get_mmio_size(afu, &mmio_size);
    debug("mmio_size 0x%lx for AFU\n", mmio_size);

    debug0("Installing libcxl SIGBUS handler\n");
    cxl_mmio_install_sigbus_handler();

    if      (action==RESET)       {nvme_link(port,1);}
    else if (action==DOWN)        {nvme_link(port,0);}
    else if (action==READ_LBA)    {io_error(port,2,lba,perm,status,interval);}
    else if (action==WRITE_LBA)   {io_error(port,1,lba,perm,status,interval);}
    else if (action==READ_DMA)    {io_error(port,7,lba,perm,status,interval);}
    else if (action==WRITE_DMA)   {io_error(port,8,lba,perm,status,interval);}
    else if (action==READ_BAD)    {io_error(port,4,lba,perm,status,interval);}
    else if (action==WRITE_BAD)   {io_error(port,3,lba,perm,status,interval);}
    else if (action==CLEAR_PERM)  {io_error(port,9,lba,perm,status,interval);}
    else if (action==PARITY_ON_WR){parity_err(port,9,lba,perm,status,interval);}
    else                          {printf("invalid action: %d\n", action);}

    exit(rc);
}
