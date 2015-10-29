/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cxl.h $                                           */
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

#ifndef _MISC_CXL_H
#define _MISC_CXL_H


#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <capi_aix_types.h>
#endif /* !_AIX */


struct cxl_ioctl_start_work {
	__u64 flags;
	__u64 work_element_descriptor;
	__u64 amr;
	__s16 num_interrupts;
	__s16 reserved1;
	__s32 reserved2;
	__u64 reserved3;
	__u64 reserved4;
	__u64 reserved5;
	__u64 reserved6;
};

#define CXL_START_WORK_AMR		0x0000000000000001ULL
#define CXL_START_WORK_NUM_IRQS		0x0000000000000002ULL
#define CXL_START_WORK_ALL		(CXL_START_WORK_AMR |\
					 CXL_START_WORK_NUM_IRQS)

/* ioctl numbers */
#define CXL_MAGIC 0xCA
#define CXL_IOCTL_START_WORK		_IOW(CXL_MAGIC, 0x00, struct cxl_ioctl_start_work)
#define CXL_IOCTL_GET_PROCESS_ELEMENT	_IOR(CXL_MAGIC, 0x01, __u32)

#define CXL_READ_MIN_SIZE 0x1000 /* 4K */

/* Events from read() */
enum cxl_event_type {
	CXL_EVENT_RESERVED      = 0,
	CXL_EVENT_AFU_INTERRUPT = 1,
	CXL_EVENT_DATA_STORAGE  = 2,
	CXL_EVENT_AFU_ERROR     = 3,
};

struct cxl_event_header {
	__u16 type;
	__u16 size;
	__u16 process_element;
	__u16 reserved1;
};

struct cxl_event_afu_interrupt {
	__u16 flags;
	__u16 irq; /* Raised AFU interrupt number */
	__u32 reserved1;
};

struct cxl_event_data_storage {
	__u16 flags;
	__u16 reserved1;
	__u32 reserved2;
	__u64 addr;
	__u64 dsisr;
	__u64 reserved3;
};

struct cxl_event_afu_error {
	__u16 flags;
	__u16 reserved1;
	__u32 reserved2;
	__u64 error;
};

struct cxl_event {
	struct cxl_event_header header;
	union {
		struct cxl_event_afu_interrupt irq;
		struct cxl_event_data_storage fault;
		struct cxl_event_afu_error afu_error;
	};
};

#endif /* _MISC_CXL_H */
