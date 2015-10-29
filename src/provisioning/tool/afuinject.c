/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/tool/afuinject.c $                           */
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

/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libcxl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <argp.h>

#include <provutil.h>
#include <afu_fc.h>
#include <cflash_mmio.h>
#include <sislite.h>
#include <afuinject.h>

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/
struct arguments g_args = {0};
int32_t g_traceE = 1;
int32_t g_traceI;
int32_t g_traceF;
int32_t g_traceV;

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
const char * argp_program_version = "afuinject 1.0\n";
const char * argp_program_bug_address = "tmpacyga@us.ibm.com";
char doc[] = "\nafuinject -- AFU register reader/writer\n\v";

/*----------------------------------------------------------------------------*/
/* Struct / Typedef                                                           */
/*----------------------------------------------------------------------------*/
enum argp_char_options {
    AFUINJ_DEVICE = 'd',
    AFUINJ_READ = 'r',
    AFUINJ_WRITE = 'w',
    AFUINJ_REGISTER = 'R',
    AFUINJ_DATA = 'D',
    AFUINJ_DEBUG = 'v',
};

static struct argp_option options[] = {
    {"device", AFUINJ_DEVICE, "device path", OPTION_ARG_OPTIONAL, 
        "Specify device to read/write from."},
    {"read", AFUINJ_READ, 0, 0, "Read from a register."},
    {"write", AFUINJ_WRITE, 0, 0, "Write to a register."},
    {"register", AFUINJ_REGISTER, "register offset", OPTION_ARG_OPTIONAL, 
        "Register offset to read/write from."},
    {"data", AFUINJ_DATA, "data to write", OPTION_ARG_OPTIONAL, 
        "Data to write to register."},
    {"debug", AFUINJ_DEBUG, "<LV>", 0, "Internal trace level for tool."},
    {0}
};

static struct argp argp = {options, parse_opt, 0, doc};

/*----------------------------------------------------------------------------*/
/* Functions                                                                  */
/*----------------------------------------------------------------------------*/
error_t parse_opt(int key, char * arg, struct argp_state * state) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/

    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    switch (key)
    {
        case AFUINJ_DEVICE:
            if((arg != NULL) && (strlen(arg) != 0)) {
                strncpy(g_args.target_device, arg, sizeof(g_args.target_device));
                TRACEI("Setting target device to: '%s'\n", g_args.target_device);
            }
            g_args.device = 1;
            break;
        
        case AFUINJ_READ:
            g_args.read = 1;
            break;
        
        case AFUINJ_WRITE:
            g_args.write = 1;
            break;
        
        case AFUINJ_REGISTER:
            if((arg != NULL) && (strlen(arg) != 0)) {
                strncpy(g_args.register_offset, arg, sizeof(g_args.register_offset));
                TRACEI("Using register offset: '%s'\n", g_args.register_offset);
            }
            g_args.reg = 1;
            break;
        
        case AFUINJ_DATA:
            if((arg != NULL) && (strlen(arg) != 0)) {
                strncpy(g_args.data, arg, sizeof(g_args.data));
                TRACEI("Using data: '%s'\n", g_args.data);
            }
            g_args.data_to_write = 1;
            break;

        case AFUINJ_DEBUG:
            g_args.verbose = atoi(arg);
            if (g_args.verbose >= 1)
                g_traceI = 1;
            if (g_args.verbose >= 2)
                g_traceF = 1;
            if (g_args.verbose >= 3)
                g_traceV = 1;
            TRACEI("Set verbose level to %d.\n", g_args.verbose);
            break;

        default:
            return (ARGP_ERR_UNKNOWN);
    }

    return (0);
}

void * mmap_afu_registers(int afu_master_fd) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    void * l_ret_ptr = NULL;

    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    TRACEI("Attempting to mmap 0x%016zx bytes of problem state.\n", //%zx prints a size_t gracefully
        sizeof(struct surelock_afu_map));
    l_ret_ptr = mmap(NULL, sizeof(struct surelock_afu_map), PROT_READ|PROT_WRITE, MAP_SHARED,
        afu_master_fd, 0);
    TRACEI("MMAP AFU Master complete.\n");
    if (l_ret_ptr == MAP_FAILED) {
        TRACEE("Unable to mmap problem state regs. errno = %d (%s).\n", 
            errno, strerror(errno));
        l_ret_ptr = NULL;
    }

    return l_ret_ptr;
}

void munmap_afu_registers(void * ps_regs) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    if(munmap(ps_regs, sizeof(struct surelock_afu_map)))
        perror("munmap_afu_registers");
}

uint8_t * get_afu_psa_addr(char * i_afu_path) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    struct cxl_afu_h * afu_master_h = NULL;
    int afu_master_fd = 0;
    uint8_t * afu_psa_addr = NULL;    

    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    if ((afu_master_h = cxl_afu_open_dev(i_afu_path)) < 0) {
        TRACEE("Unable to open AFU Master cxl device. errno = %d (%s).\n",
             errno, strerror(errno));
         return NULL;
    }

    TRACEI("Opened %p, now attempting to get FD.\n", afu_master_h);
    afu_master_fd = cxl_afu_fd(afu_master_h);
    TRACEI("Got FD! = %d\n", afu_master_fd);

    if (cxl_afu_attach(afu_master_h, 0)) {
        TRACEE("Call to cxl_afu_attach failed. errno = %d (%s)\n", 
            errno, strerror(errno));
        return NULL;
    }

    afu_psa_addr = mmap_afu_registers(afu_master_fd);

    if (!afu_psa_addr) {
        TRACEE("Error attempting to map AFU problem state registers. \
            errno = %d (%s)\n", errno, strerror(errno));
        return NULL;
    }

    return afu_psa_addr;
}

int write_afu_register(uint8_t * afu_psa_addr, int reg, uint64_t mmio_data) { 
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    
    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    out_mmio64((__u64*)&afu_psa_addr[reg], mmio_data);
    TRACED("Wrote '0x%"PRIx64"' to regist '0x%08x'.\n", mmio_data, reg);

    return 0;
}

int read_afu_register(uint8_t * afu_psa_addr, int reg) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    uint64_t mmio_data = 0;

    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    mmio_data = in_mmio64((__u64*)&afu_psa_addr[reg]);
    TRACED("Register '0x%08x' contains '0x%"PRIx64"'.\n", reg, mmio_data);
    
    return 0;
}

int main(int argc, char * argv[]) {
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    uint8_t * afu_psa_addr = NULL;

    /*------------------------------------------------------------------------*/
    /*  Code                                                                  */
    /*------------------------------------------------------------------------*/
    memset(&g_args, 0, sizeof(g_args));
    argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, &g_args);
    
    if(g_args.device) {
        afu_psa_addr = get_afu_psa_addr(g_args.target_device);
    
        if(!afu_psa_addr) {
            TRACEE("Error getting afu_psa_addr.\n");
            return -1;
        }

        if(g_args.reg) {
            if(g_args.read) {
                read_afu_register(afu_psa_addr, 
                    (int)strtol(g_args.register_offset, NULL, 0));
            }
            
            else if(g_args.write) {
                if(g_args.data_to_write) {
                    write_afu_register(afu_psa_addr, 
                        (int)strtol(g_args.register_offset, NULL, 0),
                        (uint64_t)strtol(g_args.data, NULL, 0));
                }
                else
                    TRACED("Need to specify data '-D/--data', exiting.\n");
            }
            else {
                TRACED("Need to specify a read or write \
                    '-r/--read or -w/--write', exiting.\n");
            }
        }
        else
            TRACED("Need to specify register '-R/--register', exiting.\n");
        
        munmap_afu_registers((void *)afu_psa_addr);
    }
    else
        TRACED("Need to specify device '-d/--device', exiting.\n");

    return 0;
}
