/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_mmio.h $                                   */
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

#ifndef _H_CFLASH_MMIO
#define _H_CFLASH_MMIO


#include <sys/types.h>
#include <cflash_tools_user.h>

#if !defined(_AIX) && !defined(_MACOSX)

/*
 * TODO: ?? remove the out_be, and in_be functions, since 
 *       we should routines that can under the covers 
 *       handle the host endianness.
 */

// MMIO write, 64 bit
static inline void out_be64 (__u64 *addr, __u64 val)
{
    __u64 val2 = CFLASH_TO_ADAP64(val);
    __asm__ __volatile__ ("std%U0%X0 %1, %0"
			  : "=m" (*addr) : "r" (val2) : "memory");
}


// MMIO read, 64 bit
static inline __u64 in_be64 (__u64 *addr)
{
    __u64 ret;
    __asm__ __volatile__ ("ld%U1%X1 %0, %1; twi 0,%0,0; isync"
			  : "=r" (ret) : "m" (*addr) : "memory");
    return CFLASH_FROM_ADAP64(ret);
}



// MMIO write, 64 bit
static inline void out_mmio64 (volatile __u64 *addr, __u64 val)
{
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    __u64 zero = 0; 
    asm volatile ( "stdbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u64 *)(addr)) = val;
#endif /* _AIX */
}


// MMIO read, 64 bit
static inline __u64 in_mmio64 (volatile __u64 *addr)
{
    __u64 val;
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    __u64 zero = 0;
    asm volatile ( "ldbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
   val =  *((volatile __u64 *)(addr));
#endif /* _AIX */

    return val;
}

// MMIO write, 32 bit
static inline void out_mmio32 (volatile __u64 *addr, __u32 val)
{
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    __u32 zero = 0;
    asm volatile ( "stwbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u32 *)(addr)) = val;
#endif /* _AIX */
}

// MMIO read, 32 bit
static inline __u32 in_mmio32 (volatile __u64 *addr)
{
    __u32 val;
#ifdef CFLASH_LITTLE_ENDIAN_HOST
    __u32 zero = 0;
    asm volatile ( "lwbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
   val =  *((volatile __u64 *)(addr));
#endif /* _AIX */

    return val;
}

#else
static inline void out_be64 (__u64 *addr, __u64 val)
{
    __u64 val2 = CFLASH_TO_ADAP64(val);

    *((volatile __u64 *)addr) = val2;
}


// MMIO read, 64 bit
static inline __u64 in_be64 (__u64 *addr)
{
    __u64 ret;

    ret = *((volatile __u64 *)addr);

    return CFLASH_FROM_ADAP64(ret);
}

static inline void out_mmio64 (volatile __u64 *addr, __u64 val)
{
    __u64 val2 = CFLASH_TO_ADAP64(val);

    *((volatile __u64 *)addr) = val2;
}


// MMIO read, 64 bit
static inline __u64 in_mmio64 (volatile __u64 *addr)
{
    __u64 ret;

    ret = *((volatile __u64 *)addr);

    return CFLASH_FROM_ADAP64(ret);
}


static inline void out_mmio32 (volatile __u64 *addr, __u32 val)
{
    __u32 val2 = CFLASH_TO_ADAP32(val);

    *((volatile __u32 *)addr) = val2;
}


// MMIO read, 64 bit
static inline __u32 in_mmio32 (volatile __u64 *addr)
{
    __u64 ret;

    ret = *((volatile __u32 *)addr);

    return CFLASH_FROM_ADAP32(ret);
}
#endif /* AIX or MAC OSX */

#endif /* _H_CFLASH_MMIO */
