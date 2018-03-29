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

#ifdef DBGTRC_ON

#define DBGTRC(fmt,...)                                         \
  do                                                            \
  {                                                             \
      printf("%-30s %-5d ", (char *)__FUNCTION__, __LINE__);    \
      printf(fmt, ##__VA_ARGS__);                               \
      printf("\n");                                             \
      fflush(stdout);                                           \
  } while (0);

#define DBGTRCV(V,v,fmt,...)                                    \
  do                                                            \
  {                                                             \
    if (v >= V)                                                 \
    {                                                           \
      printf("%-30s %-5d ", (char *)__FUNCTION__, __LINE__);    \
      printf(fmt, ##__VA_ARGS__);                               \
      printf("\n");                                             \
      fflush(stdout);                                           \
    }                                                           \
  } while (0);

#else

#define DBGTRC(fmt,...)
#define DBGTRCV(V,v,fmt,...)

#endif

#endif
