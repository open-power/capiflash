/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/ffdc/afu.c $                                         */
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
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <afu.h>

//#define AFU_PS_REGS_SIZE (64*1024*1024)

#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#ifndef htobe16
#define htobe16(x) (x)
#endif
#ifndef htobe32
#define htobe32(x) (x)
#endif
#ifndef htobe64
#define htobe64(x) (x)
#endif
#else
#ifndef htobe16
#define htobe16(x) __bswap_16(x)
#endif
#ifndef htobe32
#define htobe32(x) __bswap_32(x)
#endif
#ifndef htobe64
#define htobe64(x) __bswap_64(x)
#endif
#endif


// MMIO write, 32 bit
static inline void out_be32 (__u64 *addr, __u32 val)
{
	__asm__ __volatile__ ("sync; stw%U0%X0 %1, %0"
		: "=m" (*addr) : "r" (val) : "memory");
}

// MMIO write, 64 bit
static inline void out_be64 (__u64 *addr, __u64 val)
{
	__asm__ __volatile__ ("sync; std%U0%X0 %1, %0"
		: "=m" (*addr) : "r" (val) : "memory");
}

// MMIO read, 32 bit
static inline __u32 in_be32 (__u64 *addr)
{
	__u32 ret;
	__asm__ __volatile__ ("sync; lwz%U1%X1 %0, %1; twi 0,%0,0; isync"
		: "=r" (ret) : "m" (*addr) : "memory");
	return ret;
}

// MMIO read, 64 bit
static inline __u64 in_be64 (__u64 *addr)
{
	__u64 ret;
	__asm__ __volatile__ ("sync; ld%U1%X1 %0, %1; twi 0,%0,0; isync"
		: "=r" (ret) : "m" (*addr) : "memory");
	return ret;
}

// mmap AFU MMIO registers
static void mmap_problem_state_registers (struct afu *afu)
{
	void *ret;
	debug0 ("Mapping AFU problem state registers...\n");
	ret = mmap (NULL, AFU_PS_REGS_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
		   afu->fd, 0);
	if (ret == MAP_FAILED) {
		perror ("mmap_problem_state_registers");
		afu->ps_addr = NULL;
		return;
	}
	afu->ps_addr = ret;
}

// munmap AFU MMIO registers
static void munmap_problem_state_registers (struct afu *afu)
{
	if (afu->ps_addr) {
		debug0 ("Unmapping AFU problem state registers...\n");
		if (munmap (afu->ps_addr, AFU_PS_REGS_SIZE))
			perror ("munmap_problem_state_registers");
		debug0 ("Done unmapping AFU problem state registers\n");
		afu->ps_addr = NULL;
	}
}

// Create and open AFU device then map MMIO registers
struct afu *afu_map (char *afu_path)
{
	struct afu *afu = (struct afu *) malloc (sizeof (struct afu));
	if (!afu) {
		perror ("malloc");
		return NULL;
	}
	debug ("Allocated memory at 0x%016lx for AFU\n", (__u64) afu);

	memset(afu, 0, sizeof(*afu));
	afu->work.num_interrupts = -1;

	fprintf(stderr,"Creating and opening AFU file descriptor %s\n",afu_path);
	// afu->fd = create_and_open_dev ("/dev/cxl/afu0.0m", "capi", 1);
	//        afu->fd = open("/dev/cxl/afu0.0m", O_RDWR);
        afu->fd = open(afu_path, O_RDWR);

	if (afu->fd < 0) {
		perror ("create_and_open_dev");
		afu_unmap (afu);
		return NULL;
	}

	// attach the process before mmap
	afu_start(afu);
	if (afu->started == 0) {
		perror ("afu_start failed");
		afu_unmap (afu);
		return NULL;
	}
        printf("afu_start: context=%d\n",afu->process_element); fflush(stdout);

	// map this context's problem space MMIO (SIS-Lite regs)
	mmap_problem_state_registers (afu);
	if (!afu->ps_addr) {
		perror ("mmap_problem_state_registers");
		afu_unmap (afu);
		return NULL;
	}

	printf ("Problem state registers mapped to %p\n", afu->ps_addr);

	return afu;
}

// Unmap AFU device
void afu_unmap (struct afu *afu)
{
	if (afu) {
		munmap_problem_state_registers (afu);
		if (afu->fd >= 0) {
			debug0 ("Closing AFU file descriptor...\n");
			close (afu->fd);
		}
	}
}

// Set WED address and have PSL send reset and start to AFU
void afu_start (struct afu *afu)
{
	/* Set WED in PSL and send start command to AFU */
	debug ("Sending WED address 0x%016lx to PSL...\n", (__u64) afu->work.work_element_descriptor);

	if (ioctl (afu->fd, CXL_IOCTL_START_WORK, &afu->work) == 0) {
	  debug0 ("Start command succeeded on AFU\n");
	  afu->started = 1;
	}
	else {
	  debug0 ("Start command to AFU failed\n");
	}

	if (ioctl (afu->fd, CXL_IOCTL_GET_PROCESS_ELEMENT,&(afu->process_element)) == 0) {
	  debug0 ("Get process element succeeded on AFU\n");
	} else {
	  debug0 ("Get process element succeeded on AFU\n");
	}
}

// MMIO write based on AFU offset, 32-bit
void afu_mmio_write_sw (struct afu *afu, unsigned offset, __u32 value)
{
#ifdef SLAVE_CONTEXT
  if (offset < (512*0x4000)) offset = offset * (0x4000 * afu->process_element);
#endif
  __u64 addr = 4 * (__u64) offset;
  out_be32 (afu->ps_addr + addr, htobe32(value));
  debug ("Wrote 0x%08x to AFU register offset %x\n", value, offset);
}

// MMIO write based on AFU offset, 64-bit
void afu_mmio_write_dw (struct afu *afu, unsigned offset, __u64 value)
{
#ifdef SLAVE_CONTEXT
  if (offset < (512*0x4000)) offset = offset + (0x4000 * afu->process_element);
#endif
  __u64 addr = 4 * (__u64) (offset & ~0x1);	// Force 8byte align
  out_be64 (afu->ps_addr + addr, htobe64(value));
  debug ("Wrote 0x%016lx to AFU register offset %x\n", value, offset);
}

// MMIO read based on AFU offset, 32-bit
void afu_mmio_read_sw (struct afu *afu, unsigned offset, __u32 *value)
{
#ifdef SLAVE_CONTEXT
  if (offset < (512*0x4000)) offset = offset + (0x4000 * afu->process_element);
#endif
  __u64 addr = 4 * (__u64) offset;
  *value = htobe32(in_be32 (afu->ps_addr + addr));
  debug ("Read 0x%08x from AFU register offset %x\n", *value, offset);
}

// MMIO read based on AFU offset, 64-bit
void afu_mmio_read_dw (struct afu *afu, unsigned offset, __u64 *value)
{
#ifdef SLAVE_CONTEXT
  if (offset < (512*0x4000)) offset = offset + (0x4000 * afu->process_element);
#endif
  __u64 addr = 4 * (__u64) (offset & ~0x1);	// Force 8byte align
  *value = htobe64(in_be64 (afu->ps_addr + addr));
  debug ("Read 0x%016lx from AFU register offset %x\n", *value, offset);
}

// Wait for AFU to complete job
int afu_wait (struct afu *afu)
{
	if (afu->started) {
		debug0 ("Waiting for AFU to finish...\n");
		struct pollfd poll_list = { afu->fd, POLLIN, 0};
		int ret;

		ret = poll (&poll_list, 1, 5000);
		if (ret == 0)
		printf ("Poll timed out waiting on interrupt.\n");

		/* For now, assume a non-zero response is a real interrupt
		 * later, maybe check regs / fd, loop for events, etc.
		 */
		debug0 ("AFU finished\n");
	}
}
