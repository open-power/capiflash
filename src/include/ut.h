/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ut.h $                                                 */
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
#ifndef __UT_H__
#define __UT_H__
#include <stdint.h>

uint64_t divup(uint64_t n, uint64_t m);

uint64_t divceil(uint64_t n, uint64_t m);

char *rndalpha(int n, int m);

double time_per_tick(int n, int del);


#if defined(__powerpc__) || defined(__ppc__)
typedef uint64_t ticks;
static __inline__ ticks getticks(void)
{
     unsigned int x, x0, x1;
     do {
          __asm__ __volatile__ ("mftbu %0" : "=r"(x0));
          __asm__ __volatile__ ("mftb %0" : "=r"(x));
          __asm__ __volatile__ ("mftbu %0" : "=r"(x1));
     } while (x0 != x1);

     return (((uint64_t)x0) << 32) | x;
}
#endif

#if defined(__x86_64__)
typedef uint64_t ticks;
static __inline__ ticks getticks(void)
{
     uint32_t x, y; 
     asm volatile("rdtsc" : "=a" (x), "=d" (y)); 
     return ((ticks)x) | (((ticks)y) << 32); 
}
#endif

static __inline__ double elapsed(ticks t1, ticks t0)
{
  return (double)t1 - (double)t0;
}

#endif //__UT_H__
