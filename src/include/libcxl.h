/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/libcxl.h $                                        */
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
#ifndef _LIBCXL_H
#define _LIBCXL_H

#include <stdbool.h>
#include "cxl.h"

/*
 * This is a very early library to simplify userspace code accessing a CXL
 * device.
 *
 * Currently there are only a couple of functions here - more is on the way.
 *
 * Suggestions to improve the library, simplify it's usage, add additional
 * functionality, etc. are welcome
 */

#define CXL_SYSFS_CLASS "/sys/class/cxl"
#define CXL_DEV_DIR "/dev/cxl"

/*
 * Opaque types
 */
struct cxl_adapter_h;
struct cxl_afu_h;

/*
 * Adapter Enumeration
 *
 * Repeatedly call cxl_adapter_next() (or use the cxl_for_each_adapter macro)
 * to enumerate the available CXL adapters.
 *
 * cxl_adapter_next() will implicitly free used buffers if it is called on the
 * last adapter, or cxl_adapter_free() can be called explicitly.
 */
struct cxl_adapter_h * cxl_adapter_next(struct cxl_adapter_h *adapter);
char * cxl_adapter_devname(struct cxl_adapter_h *adapter);
void cxl_adapter_free(struct cxl_adapter_h *adapter);
#define cxl_for_each_adapter(adapter) \
	for (adapter = cxl_adapter_next(NULL); adapter; adapter = cxl_adapter_next(adapter))

/*
 * AFU Enumeration
 *
 * Repeatedly call cxl_adapter_afu_next() (or use the
 * cxl_for_each_adapter_afu macro) to enumerate AFUs on a specific CXL
 * adapter, or use cxl_afu_next() or cxl_for_each_afu to enumerate AFUs over
 * all CXL adapters in the system.
 *
 * For instance, if you just want to find any AFU attached to the system but
 * don't particularly care which one, just do:
 * struct cxl_afu_h *afu_h = cxl_afu_next(NULL);
 *
 * cxl_[adapter]_afu_next() will implicitly free used buffers if it is called
 * on the last AFU, or cxl_afu_free() can be called explicitly.
 */
struct cxl_afu_h * cxl_adapter_afu_next(struct cxl_adapter_h *adapter, struct cxl_afu_h *afu);
struct cxl_afu_h * cxl_afu_next(struct cxl_afu_h *afu);
char * cxl_afu_devname(struct cxl_afu_h *afu);
#define cxl_for_each_adapter_afu(adapter, afu) \
	for (afu = cxl_adapter_afu_next(adapter, NULL); afu; afu = cxl_adapter_afu_next(NULL, afu))
#define cxl_for_each_afu(afu) \
	for (afu = cxl_afu_next(NULL); afu; afu = cxl_afu_next(afu))


/*
 * Open AFU - either by path, by AFU being enumerated, or tie into an AFU file
 * descriptor that has already been opened. The AFU file descriptor will be
 * closed by cxl_afu_free() regardless of how it was opened.
 */
struct cxl_afu_h * cxl_afu_open_dev(char *path);
int                 cxl_afu_open_h(struct cxl_afu_h *afu, unsigned long master);
struct cxl_afu_h * cxl_afu_fd_to_h(int fd);
void cxl_afu_free(struct cxl_afu_h *afu);

/*
 * Attach AFU context to this process
 */
int cxl_afu_attach_full(struct cxl_afu_h *afu, __u64 wed, __u16 num_interrupts,
			__u64 amr);
int cxl_afu_attach(struct cxl_afu_h *afu, __u64 wed);

/*
 * Get AFU process element
 */
int cxl_afu_get_process_element(struct cxl_afu_h *afu);

/*
 * Returns the file descriptor for the open AFU to use with event loops.
 * Returns -1 if the AFU is not open.
 */
int cxl_afu_fd(struct cxl_afu_h *afu);

/*
 * TODO: All in one function - opens an AFU, verifies the operating mode and
 * attaches the context.
 * int cxl_afu_open_and_attach(struct cxl_afu_h *afu, mode)
 */

/*
 * sysfs helpers
 *
 * NOTE: On success, these functions automatically allocate the returned
 * buffers, which must be freed by the caller (much like asprintf).
 */
int cxl_afu_sysfs_pci(char **pathp, struct cxl_afu_h *afu);

/*
 * Events
 */
bool cxl_pending_event(struct cxl_afu_h *afu);
int cxl_read_event(struct cxl_afu_h *afu, struct cxl_event *event);
int cxl_read_expected_event(struct cxl_afu_h *afu, struct cxl_event *event,
			    __u32 type, __u16 irq);

/*
 * fprint wrappers to print out CXL events - useful for debugging.
 * fprint_cxl_event will select the appropriate implementation based on the
 * event type and fprint_cxl_unknown_event will print out a hex dump of the
 * raw event.
 */
int fprint_cxl_event(FILE *stream, struct cxl_event *event);
int fprint_cxl_unknown_event(FILE *stream, struct cxl_event *event);

#endif
