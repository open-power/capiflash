/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/afu.h $                                         */
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
#ifndef _AFU_MEMCOPY_H_
#define _AFU_MEMCOPY_H_

#include <cxl.h>
#include <stdio.h>
#define AFU_PS_REGS_SIZE 0x4000000


#define DEBUG 0
#define debug(fmt, ...) do { if (DEBUG) fprintf(stderr,fmt,__VA_ARGS__); fflush(stderr);} while (0)
#define debug0(fmt)     do { if (DEBUG) fprintf(stderr,fmt); fflush(stderr);} while (0)

struct afu {
  int fd;			/* file descriptor */
  void *ps_addr;		/* problem state registers */
  struct cxl_ioctl_start_work work;
  __u32 process_element;
  int started;		/* AFU state */
};

// Create and open AFU device then map MMIO registers
struct afu *afu_map (char *path);

// Unmap AFU device
void afu_unmap (struct afu *afu);

// Set WED address and have PSL send reset and start to AFU
void afu_start (struct afu *afu);

// MMIO write based on AFU offset, 32-bit
void afu_mmio_write_sw (struct afu *afu, unsigned offset, __u32 value);

// MMIO write based on AFU offset, 64-bit
void afu_mmio_write_dw (struct afu *afu, unsigned offset, __u64 value);

// MMIO read based on AFU offset, 32-bit
void afu_mmio_read_sw (struct afu *afu, unsigned offset, __u32 *value);

// MMIO read based on AFU offset, 64-bit
void afu_mmio_read_dw (struct afu *afu, unsigned offset, __u64 *value);

// Wait for AFU to complete job
int afu_wait (struct afu *afu);

#endif /* #define _AFU_MEMCOPY_H_ */
