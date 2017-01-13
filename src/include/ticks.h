/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/ticks.h $                                         */
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
#ifndef _TICKS_H__
#define _TICKS_H__
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

typedef uint64_t ticks;

static __inline__ ticks getticks(void);

/**
 *******************************************************************************
 * \brief
 * seconds delta of current ticks and sticks
 ******************************************************************************/
static __inline__ uint32_t SDELTA(ticks sticks, double ns)
{
    return (uint32_t)((((getticks())-sticks) * ns) / 1000000000);
}

/**
 *******************************************************************************
 * \brief
 * milliseconds delta of current ticks and sticks
 ******************************************************************************/
static __inline__ uint64_t MDELTA(ticks sticks, double ns)
{
    return (((getticks())-sticks) * ns) / 1000000;
}

/**
 *******************************************************************************
 * \brief
 * microseconds delta of current ticks and sticks
 ******************************************************************************/
static __inline__ uint64_t UDELTA(ticks sticks, double ns)
{
    return (((getticks())-sticks) * ns) / 1000;
}

/**
 *******************************************************************************
 * \brief
 * nanoseconds delta of current ticks and sticks
 ******************************************************************************/
static __inline__ uint64_t NDELTA(ticks sticks, double ns)
{
    return (((getticks())-sticks) * ns);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
#if defined(__powerpc__) || defined(__ppc__)
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

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
#if defined(__x86_64__)
static __inline__ ticks getticks(void)
{
     uint32_t x, y; 
     asm volatile("rdtsc" : "=a" (x), "=d" (y)); 
     return ((ticks)x) | (((ticks)y) << 32);
}
#endif

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ double elapsed(ticks t1, ticks t0)
{
  return (double)t1 - (double)t0;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ double time_diff(struct timeval x , struct timeval y)
{
    double x_ms , y_ms , diff;

    x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;

    diff = (double)y_ms - (double)x_ms;

    return diff;
}

/**
 *******************************************************************************
 * \brief
 *   return nanoseconds per tick
 ******************************************************************************/
static __inline__ double time_per_tick(int n, int del)
{
  double        *td = malloc(n * sizeof(double));
  double        *tv = malloc(n * sizeof(double));
  struct timeval tvs;
  struct timeval tve;
  ticks          ts;
  ticks          te;
  int            i;

  for (i=0; i<n; i++)
  {
    gettimeofday(&tvs, NULL);
    ts = getticks();

    usleep(del);

    te = getticks();
    gettimeofday(&tve, NULL);

    td[i] = elapsed(te,ts);
    tv[i] = time_diff(tvs,tve);
  }

  double sum = 0.0;

  for(i=0; i<n; i++) {sum +=  1000.0 * tv[i] / td[i];}

  free(td);
  free(tv);
  return sum / n ;
}

#endif //_TICKS_H__
