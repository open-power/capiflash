/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/mclient.h $                                       */
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
#ifndef _MCLIENT_H
#define _MCLIENT_H

#include <sislite.h>

/****************************************************************************
 * Each user process needs two interfaces to use the AFU: 
 *
 *   1. a host transport MMIO map to issue commands directly to the AFU.
 *      The client opens the regular AFU device, registers its process 
 *      context with the AFU driver (CXL_IOCTL_START_WORK ioctl) and 
 *      then mmaps a 64KB host transport that has registers to issue
 *      commands and setup responses.
 *
 *   2. an interface to the master context daemon via a mc_hndl_t.
 *      This is used to create & size resource handles which are equivalent
 *      to virtual disks. The resource handle is written into the command 
 *      block (IOARCB) sent to the AFU using interface #1.
 *
 * This header file describes the second interface. See sislite.h for the
 * first interface.
 *
 * User must create interface #1 first. Parameters returned by #1 are 
 * required to establish interface #2.
 *
 * All routines described here return 0 on success and -1 on failure.
 * errno is set on failure.                                                
 ***************************************************************************/

/* Max pathlen - e.g. for AFU device path */
#define MC_PATHLEN       64

/* Permission Flags */
#define MC_RDONLY        0x1u
#define MC_WRONLY        0x2u
#define MC_RDWR          0x3u

/* mc_hndl_t is the client end point for communication to the
 * master. After a fork(), the child must call mc_unregister
 * on any inherited mc handles. It is a programming error to pass
 * an inherited mc handle to any other API in the child.
 */
typedef void* mc_hndl_t;


/* mc_init & mc_term are called once per process */
int mc_init();
int mc_term();

/*
 * mc_register outputs a client handle (mc_hndl_t) when successful.
 * The handle is bound to the specified AFU and context handle.
 * Subsequent API calls using the handle apply to that context
 * and that AFU. The specified context handle must be owned by the
 * requesting user process.
 *
 * The mc_hndl_t is a client endpoint for communication with the master
 * context. It can be used to set up resources only for the single context 
 * on a single AFU bound to the mc handle.
 *
 * Users opening multiple AFU contexts must create multiple mc handles by
 * registering each context.
 *
 * Client is responsible for serialization when the same mc handle is 
 * shared by multiple threads.
 *
 * Different threads can work on different mc handles in parallel w/o 
 * any serialization.
 *
 * It is possible to mc_register the same (AFU + ctx_hndl) multiple times
 * creating multiple mc endpoints (mc_hndl) bound to the same context.
 * However, to accomplish this, the mc_hdup function must be used to 
 * duplicate an existing mc_hndl. Calling mc_register more than once with 
 * the same parameters will cancel all but the most recent registation.
 *
 * Inputs:
 *   master_dev_path - e.g. /dev/cxl/afu0.0m
 *                     The path must be to the master AFU device even if the
 *                     user opened the corresponding regular device.
 *   ctx_hndl        - User's context handle to register with master.
 *                     This is the AFU ctx_id or context handle returned by the
 *                     CXL_IOCTL_START_WORK ioctl. For surelock, this is
 *                     a number between 0 & 511. The ctx_hndl is allocated by
 *                     the kernel AFU driver without any involvement of the 
 *                     master context.
 *   p_mmio_map      - pointer to user's MMIO map
 *
 * Output:
 *   p_mc_hndl       - The output mc_hndl_t is written using this pointer.
 *                     The mc_hndl_t is passed in all subsequent calls.
 *
 */
int mc_register(char *master_dev_path, ctx_hndl_t ctx_hndl, 
		volatile __u64 *p_mmio_map, mc_hndl_t *p_mc_hndl);


/* Duplicate a previously registered mc handle - e.g. for use in
 * another thread.
 *
 * Inputs:
 *   mc_hndl -         registered handle identifying (AFU + context)
 *
 * Output:
 *   p_mc_hndl_new -   The output mc_hndl_t is written using this pointer.
 *                     The new handle is bound to the same (AFU + context) 
 *                     and is interchangeable with the original handle.
 *
 * Note: mc_unregister must be called on the duplicated handle when no
 * longer needed. The duplicate mc handle and the original access the
 * same resource handles because they refer to the same (AFU + context).
 */
int mc_hdup(mc_hndl_t mc_hndl, mc_hndl_t *p_mc_hndl_new);


/* mc_unregister invalidates the mc handle. All resource
 * handles under the input mc handle are released if this mc_hndl is 
 * the only reference to those resources.
 *
 * Note that after a dup (see below), two contexts can be referencing 
 * the same resource handles. In that case, unregistering one context 
 * must not release resource handles since they are still referenced 
 * by the other context.
 *
 * Inputs:
 *   mc_hndl         - a mc_hndl that specifies a (context + AFU)
 *                     to unregister
 *
 * Output:
 *   none
 */
int mc_unregister(mc_hndl_t mc_hndl);


/* mc_open creates a zero sized virtual LBA space or a virtual disk
 * on the AFU and context bound to the mc handle.
 *
 * Inputs:
 *   mc_hndl         - mc handle that specifies a (context + AFU)
 *
 *   flags           - permission flags (see #defines)
 *
 * Output:
 *   p_res_hndl      - A resource handle allocated by master is written using
 *                     this pointer. The handle is valid only in the scope of
 *                     the AFU context bound to the input mc_hndl or any 
 *                     contexts dup'ed to it. User must not mix up resource 
 *                     handles from different contexts. Resource handle A for 
 *                     AFU context 1 and handle B for AFU context 2 can have the 
 *                     same internal representation, but they refer to two
 *                     completely different virtual LBA spaces.
 */
int mc_open(mc_hndl_t mc_hndl, __u64 flags, res_hndl_t *p_res_hndl);


/* mc_close deallocates any resources (LBAs) allocated to a virtual 
 * disk if this context is the last reference to those resources.
 *
 * Inputs:
 *   mc_hndl         - client handle that specifies a (context + AFU)
 *   res_hndl        - resource handle identifying the virtual disk 
 *                     to close
 *
 * Output:
 *   none
 */
int mc_close(mc_hndl_t mc_hndl, res_hndl_t res_hndl);


/* mc_size grows or shrinks a virtual disk to a new size. The size is
 * in units of a chunk which is the minimum allocation unit for a 
 * virtual disk.
 *
 * Inputs:
 *   mc_hndl         - client handle that specifies a (context + AFU)
 *   res_hndl        - resource handle identifying the virtual disk 
 *                     to resize
 *   new_size        - desired size in chunks
 *
 * Output:
 *   p_actual_new_size  - points to location where actual new size is
 *                    written. For a grow request, the actual size can
 *                    be less than the requested size.
 */
int mc_size(mc_hndl_t mc_hndl, res_hndl_t res_hndl,
	    __u64 new_size, __u64 *p_actual_new_size);


/* mc_xlate_lba returns a physical LBA for a given virtual LBA of
 * the input virtual disk.
 *
 * Inputs:
 *   mc_hndl         - client handle that specifies a (context + AFU)
 *   res_hndl        - resource handle identifying the virtual disk 
 *                     to translate
 *   v_lba           - virtual LBA in res_hndl
 *
 * Output:
 *   p_lba           - pointer to location that will contain the 
 *                     physical LBA.
 * Note:
 *   This function is of limited use to a user since all users must 
 *   run with LBA translation on. Normal user processes are not 
 *   permitted to issue requests directly to physical LBAs.
 *
 */
int mc_xlate_lba(mc_hndl_t mc_hndl, res_hndl_t res_hndl, 
		 __u64 v_lba, __u64 *p_lba);

/* mc_clone copies all virtual disks (resource handles) in a source 
 * context into a destination context. The destination context must
 * not have any virtual disks open or the clone will fail.
 *
 * The source context must be owned by the requesting user process.
 *
 * Clone is a snapshot facility. After the clone, changes to the 
 * source context does not affect the destination and vice versa. 
 * If the source context opened a new virtual disk after the clone 
 * and the destination did the same, the two new disks are completely 
 * different and are seen only by the sole context that created it.
 * Similarly, if a virtual disk was resized after the clone in the
 * source context, the destination still has the original size.
 * However, until copy-on-write is supported, cloned and original
 * resource handles point to the same LBAs and their access must
 * be coordinated by the user.
 *
 * Clone is supported only on the same AFU and NOT across AFUs.
 *
 * Inputs:
 *   mc_hndl         - a mc_hndl that specifies a (context + AFU)
 *                     as the destination of the clone
 *                     This context must have no resources open.
 *   mc_hndl_src     - source mc handle to clone from
 *   flags           - permission flags (see #defines)
 *
 * Output:
 *   none
 *
 * Note:
 *
 *   Clone copies all open resource handles in the source context to 
 *   the destination. There is no interface to select which resouce
 *   handles to clone.
 *
 *   After clone, the state of resource handles in the destination
 *   are as if they have been opened and sized.
 *
 *   There is no interface to query the resource handles that get
 *   cloned into and are valid in the destination. These handles are 
 *   the same handles valid in the source context that the user 
 *   already knows about.
 *   Example:
 *   1. user created res_hndls 0, 1, 2 & 3 in context=5.
 *   2. closed handles 1 & 2 in context 5.
 *   3. then cloned context 5 into a new context 8.
 *   4. The resource handles valid in context 8 now are 0 & 3.
 *
 *   The flags can only restrict the clone to a subset of the 
 *   permissions in the original. It cannot be used to gain new
 *   permissions not in the original. For example, if the original
 *   was Readonly, and flags=RW, the clone is Readonly.
 *
 */
int mc_clone(mc_hndl_t mc_hndl, mc_hndl_t mc_hndl_src, 
	     __u64 flags);

/*
 * mc_dup allows any number of contexts to be linked together so that
 * they are interchangeable and share all resources (virtual disks).
 * Two contexts are linked per call: a destination context to a 
 * candidate context. The call can be repeated to link  more than 2 
 * contexts. The destination context must not have any resources open 
 * or be in a dup'ed group already, else dup will fail. 
 *
 * The candidate context must be owned by the requesting user process.
 *
 * A resource handle created using one context is valid in any other
 * context dup'ed to it. By default (w/o dup), resource handles are
 * local and private to the context that created it. Further, a resource
 * handle can be opened in one context and closed in another dup'ed 
 * context. Resizing a resource changes the virtual disk for all dup'ed 
 * contexts.
 *
 * dup allows 2 or more contexts to do IO to the same virtual disks w/o
 * any restrictions.
 *
 * Note that each context has an implementation defined limit (N) on how 
 * many virtual disks it can open. mc_dup reduces the total number of
 * virtual disks for the process as a whole. If a process opens 2 contexts, 
 * without dup, each context can open N virtual disks for a total of 2*N.
 * If instead the 2 contexts are duped to each other, the same process can 
 * now open only N disks.
 *
 * Inputs:
 *   mc_hndl         - a mc_hndl that specifies a (context + AFU)
 *                     as the context to dup to (destination)
 *                     This context must have no resources open.
 *   mc_hndl_cand    - candidate context to dup from
 *
 * Output:
 *   none
 *
 * Dup is supported only on the same AFU and NOT across AFUs.
 *
 */
int mc_dup(mc_hndl_t mc_hndl, mc_hndl_t mc_hndl_cand);


/* mc_stat is analogous to fstat in POSIX. It returns information on
 * a virtual disk.
 *
 * Inputs:
 *   mc_hndl         - client handle that specifies a (context + AFU)
 *   res_hndl        - resource handle identifying the virtual disk 
 *                     to query
 *
 * Output:
 *   p_mc_stat       - pointer to location that will contain the 
 *                     output data
 */
typedef struct mc_stat_s {
  __u32       blk_len;   /* length of 1 block in bytes as reported by device */
  __u8        nmask;     /* chunk_size = (1 << nmask) in device blocks */
  __u8        rsvd[3];
  __u64       size;      /* current size of the res_hndl in chunks */
  __u64       flags;     /* permission flags */
} mc_stat_t;

int mc_stat(mc_hndl_t mc_hndl, res_hndl_t res_hndl,
	    mc_stat_t *p_mc_stat);


/* In the course of doing IOs, the user may be the first to notice certain
 * critical events on the AFU or the backend storage. mc_notify allows a 
 * user to pass such information to the master. The master will verify the
 * events and can take appropriate action.
 *
 * Inputs:
 *   mc_hndl         - client handle that specifies a (context + AFU)
 *                     The event pertains to this AFU.
 *
 *   p_mc_notify     - pointer to location that contains the event
 *
 * Output:
 */
typedef struct mc_notify_s {
  __u8 event;  /* MC_NOTIF_xxx */
#define MC_NOTIFY_CMD_TIMEOUT    0x01 /* user command timeout */
#define MC_NOTIFY_SCSI_SENSE     0x02 /* interesting sense data */
#define MC_NOTIFY_AFU_EEH        0x03 /* user detected AFU is frozen */
#define MC_NOTIFY_AFU_RST        0x04 /* user detected AFU has been reset */
#define MC_NOTIFY_AFU_ERR        0x05 /* other AFU error, unexpected response */
  /*
   * Note: the event must be sent on a mc_hndl_t that pertains to the
   * affected AFU. This is important when the user interacts with multiple
   * AFUs.
   */

  union {
    struct {
      res_hndl_t       res_hndl;
    } cmd_timeout;

    struct {
      res_hndl_t       res_hndl;
      char data[20]; /* 20 bytes of sense data */
    } scsi_sense;

    struct {
    } afu_eeh;

    struct {
    } afu_rst;

    struct {
    } afu_err;
  };
} mc_notify_t;

int mc_notify(mc_hndl_t mc_hndl, mc_notify_t *p_mc_notify);


// can we force r0 in asm ? so we do not need the "zero" to alloc a register

/* The write_nn or read_nn routines can be used to do byte reversed MMIO
   or byte reversed SCSI CDB/data.
*/
static inline void write_64(volatile __u64 *addr, __u64 val)
{
    __u64 zero = 0; 
#ifndef _AIX
    asm volatile ( "stdbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u64 *)(addr)) = val;
#endif /* _AIX */
}

static inline void write_32(volatile __u32 *addr, __u32 val)
{
    __u32 zero = 0;
#ifndef _AIX
    asm volatile ( "stwbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u32 *)(addr)) = val;
#endif /* _AIX */
}

static inline void write_16(volatile __u16 *addr, __u16 val)
{
    __u16 zero = 0;
#ifndef _AIX
    asm volatile ( "sthbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u16 *)(addr)) = val;
#endif /* _AIX */
}

static inline __u64 read_64(volatile __u64 *addr)
{
    __u64 val;
    __u64 zero = 0;
#ifndef _AIX
    asm volatile ( "ldbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) ); 
#else
   val =  *((volatile __u64 *)(addr));
#endif /* _AIX */

    return val;
}

static inline __u32 read_32(volatile __u32 *addr)
{
    __u32 val;
    __u32 zero = 0;
#ifndef _AIX
    asm volatile ( "lwbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
     val =  *((volatile __u32 *)(addr));
#endif /* _AIX */    
    return val;
}

static inline __u16 read_16(volatile __u16 *addr)
{
    __u16 val;
    __u16 zero = 0;
#ifndef _AIX
    asm volatile ( "lhbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) ); 
#else
     val =  *((volatile __u16 *)(addr));
#endif /* _AIX */   
    return val;
}

#endif  /* ifndef _MCLIENT_H */

