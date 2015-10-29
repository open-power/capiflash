/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_tools_user.h $                             */
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
#ifndef _H_CFLASH_TOOLS_USER
#define _H_CFLASH_TOOLS_USER

//the if / elif / else syntax below is intentionally explicit
//to help find cases where the TARGET_ARCH is set to an unknown
//value (e.g. someone mis-spells PPC64EL and PPC64LE).
#if defined TARGET_ARCH_PPC64EL
    #define CFLASH_LITTLE_ENDIAN_HOST 1
#elif defined TARGET_ARCH_PPC64BE
    #define CFLASH_BIG_ENDIAN_HOST 1
#else
    #error "Unknown Architecture. This typically indicates a makefile or environment variable error. Check the TARGET_ARCH env var."
#endif

/*
 * Macros to convert to and from bus byte order.  Note that these macros
 * assume (and require) that shorts are 16 bits and uint32_ts are 32 bits.
 *
 *
 *      CFLSH_REV16()     Byte reverse 16-bit value.
 *      CFLSH_REV24()     Byte reverse 24-bit value.
 *      CFLSH_REV32()     Byte reverse 32-bit value.
 *      CFLSH_REV64()     Byte reverse 64-bit value.
 */
#define CFLASH_REV16(h)    ((ushort) (                    \
                        (((ushort)(h)<<8) & 0xff00) | \
                        (((ushort)(h)>>8) & 0x00ff)   \
                        ))
#define CFLASH_REV24(w)    ((uint32_t) (                          \
                        (((uint32_t)(w)>>16) & 0x000000ff)  | \
                        ((uint32_t)(w) & 0x0000ff00)        | \
                        (((uint32_t)(w)<<16) & 0x00ff0000)    \
                        ))

#define CFLASH_REV32(w)    ((uint32_t) (                          \
                        (((uint32_t)(w)>>24) & 0x000000ff)  | \
                        (((uint32_t)(w)>> 8) & 0x0000ff00)  | \
                        (((uint32_t)(w)<< 8) & 0x00ff0000)  | \
                        (((uint32_t)(w)<<24) & 0xff000000)    \
                        ))

#define CFLASH_MASKSHIFT(x) ((uint64_t)0xff << (x))

#define CFLASH_REV64(w)     ((uint64_t) (                           \
                        (((uint64_t)(w)>>56) & CFLASH_MASKSHIFT( 0)) | \
                        (((uint64_t)(w)>>40) & CFLASH_MASKSHIFT( 8)) | \
                        (((uint64_t)(w)>>24) & CFLASH_MASKSHIFT(16)) | \
                        (((uint64_t)(w)>> 8) & CFLASH_MASKSHIFT(24)) | \
                        (((uint64_t)(w)<< 8) & CFLASH_MASKSHIFT(32)) | \
                        (((uint64_t)(w)<<24) & CFLASH_MASKSHIFT(40)) | \
                        (((uint64_t)(w)<<40) & CFLASH_MASKSHIFT(48)) | \
                        (((uint64_t)(w)<<56) & CFLASH_MASKSHIFT(56)) \
                        ))

#ifdef CFLASH_LITTLE_ENDIAN_HOST

/*
 * Little Endian Host 
 */
// TODO:?? Maybe use byteswap.h and __bswap_ routines here for Linux.
#define CFLASH_TO_ADAP16(h)   CFLASH_REV16(h)

#define CFLASH_TO_ADAP32(w)   CFLASH_REV32(w)

#define CFLASH_TO_ADAP64(w)   CFLASH_REV64(w)

#define CFLASH_FROM_ADAP16(h)   CFLASH_REV16(h)

#define CFLASH_FROM_ADAP32(w)   CFLASH_REV32(w)

#define CFLASH_FROM_ADAP64(w)   CFLASH_REV64(w)

#else

/*
 * BIG Endian Host 
 */
#define CFLASH_TO_ADAP16(h)   (h)

#define CFLASH_TO_ADAP32(w)   (w)

#define CFLASH_TO_ADAP64(w)   (w)

#define CFLASH_FROM_ADAP16(h) (h)

#define CFLASH_FROM_ADAP32(w) (w)

#define CFLASH_FROM_ADAP64(w) (w)


#endif


#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRNT(fmt, ...) fprintf(stderr,fmt,## __VA_ARGS__) 
#else
#define DEBUG_PRNT(...)
#endif


/************************************************************************/
/* Function prototypes                                                  */
/************************************************************************/
void dumppage(void *buffer, int size);
void hexdump(void *data, long len, const char *hdr);


#endif /* _H_CFLASH_TOOLS_USER */
