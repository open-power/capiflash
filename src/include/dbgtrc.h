/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/dbgtrc.h $                                        */
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

#ifndef __DBGTRC_H__
#define __DBGTRC_H__

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#ifndef _AIX
#include <sys/syscall.h>
#endif
#include <pthread.h>

#ifdef DBGTRC_VERBOSITY

#ifdef _AIX

#define DBGTRC(fmt,...)                                                               \
  do                                                                                  \
  {                                                                                   \
      int  tid    =0;                                                                 \
      char b1[200]={0};                                                               \
      char b2[200]={0};                                                               \
      pthread_t my_pthread = pthread_self();                                          \
      pthread_getunique_np(&my_pthread,&tid);                                         \
      sprintf(b1,"%-40s %-30s %-5d ", (char *)__FILE__,(char *)__FUNCTION__,__LINE__);\
      sprintf(b2,fmt, ##__VA_ARGS__);                                                 \
      printf("%-8ld %s%s\n",tid,b1,b2);                                               \
      fflush(stdout);                                                                 \
  } while (0);

#define DBGTRCV(V,fmt,...)                                      \
  do                                                            \
  {                                                             \
    if (DBGTRC_VERBOSITY >= V)                                  \
    {                                                           \
        DBGTRC(fmt,## __VA_ARGS__);                             \
    }                                                           \
  } while (0);

#else

#define DBGTRC(fmt,...)                                                               \
  do                                                                                  \
  {                                                                                   \
      int  tid    =0;                                                                 \
      char b1[200]={0};                                                               \
      char b2[200]={0};                                                               \
      tid=syscall(SYS_gettid);                                                        \
      sprintf(b1,"%-40s %-30s %-5d ", (char *)__FILE__,(char *)__FUNCTION__,__LINE__);\
      sprintf(b2,fmt, ##__VA_ARGS__);                                                 \
      printf("%-8d %s%s\n",tid,b1,b2);                                                \
      fflush(stdout);                                                                 \
  } while (0);

#define DBGTRCV(V,fmt,...)                                      \
  do                                                            \
  {                                                             \
    if (DBGTRC_VERBOSITY >= V)                                  \
    {                                                           \
        DBGTRC(fmt,## __VA_ARGS__);                             \
    }                                                           \
  } while (0);

#endif

#else

#define DBGTRC(fmt,...)
#define DBGTRCV(V,fmt,...)

#endif

#endif
