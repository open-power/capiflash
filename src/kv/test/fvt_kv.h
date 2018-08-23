/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv.h $                                        */
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
/**
 *******************************************************************************
 * \file
 * \brief
 *   declarations to aid in KV testing
 * \details
 *   if env var "FVT_DEV" is unset, then tests run using memory file
 *   use export FVT_DEV=/dev/<filename> to run to hardware
 * \ingroup
 ******************************************************************************/
#ifndef FVT_KV_H
#define FVT_KV_H

#include <ark.h>
#include <arkdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <fvt_trace.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ARK_CREATE \
  do                                                            \
  {                                                             \
    uint64_t flags = 0;                                         \
    char    *dev   = getenv("FVT_DEV");                         \
    if (dev && strlen(dev)>0 &&                                 \
               (!(strncmp("/dev/", dev, 5)==0 ||                \
                  strncmp("RAID",  dev, 4)==0)))                \
    {                                                           \
        /* this is a file */                                    \
        flags = ARK_KV_HTC;                                     \
    }                                                           \
    else                                                        \
    {                                                           \
        flags = ARK_KV_VIRTUAL_LUN | ARK_KV_HTC;                \
    }                                                           \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV"), &ark, flags));   \
    ASSERT_TRUE(ark != NULL);                                   \
  } while (0)

#define ARK_CREATE_PERSIST_READONLY \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark,    \
                            ARK_KV_PERSIST_LOAD | ARK_KV_HTC)); \
    ASSERT_TRUE(ark != NULL);

#define ARK_CREATE_PERSIST \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark,              \
               ARK_KV_PERSIST_STORE | ARK_KV_PERSIST_LOAD | ARK_KV_HTC)); \
    ASSERT_TRUE(ark != NULL);

#define ARK_CREATE_NEW_PERSIST \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark,     \
                            ARK_KV_PERSIST_STORE | ARK_KV_HTC)); \
    ASSERT_TRUE(ark != NULL);

#define ARK_DELETE \
    ASSERT_EQ(0, ark_delete(ark))

#define TESTCASE_SKIP(_reason) \
    printf("[  SKIPPED ] %s\n", _reason);

#define TESTCASE_SKIP_IF_FILE                            \
do                                                       \
{                                                        \
    char *dev = getenv("FVT_DEV");                       \
    if (dev != NULL && (!(strncmp("/dev/", dev, 5)==0 || \
                          strncmp("RAID",  dev, 4)==0))) \
    {                                                    \
        TESTCASE_SKIP("FVT_DEV looks like a file");      \
        return;                                          \
    }                                                    \
} while (0);

#define TESTCASE_SKIP_IF_MEM                             \
do                                                       \
{                                                        \
    char *dev = getenv("FVT_DEV");                       \
    if (dev == NULL)                                     \
    {                                                    \
        TESTCASE_SKIP("FVT_DEV looks like MEM");         \
        return;                                          \
    }                                                    \
} while (0);

#define TESTCASE_SKIP_IF_RAID0                           \
do                                                       \
{                                                        \
    char *dev = getenv("FVT_DEV");                       \
    if (dev != NULL && (strncmp("RAID",  dev, 4)==0))    \
    {                                                    \
        TESTCASE_SKIP("FVT_DEV is RAID0");               \
        return;                                          \
    }                                                    \
} while (0);

#define TESTCASE_SKIP_IF_NO_EEH                             \
  do                                                        \
  {                                                         \
    char *env_filemode=getenv("BLOCK_FILEMODE_ENABLED");    \
    char *env_eeh=getenv("FVT_EEH");                        \
    if (env_filemode && atoi(env_filemode)==1)              \
    {                                                       \
        TESTCASE_SKIP("skip if BLOCK_FILEMODE_ENABLED==1"); \
        return;                                             \
    }                                                       \
    if (!env_eeh || (env_eeh && atoi(env_eeh)!=1))          \
    {                                                       \
      TESTCASE_SKIP("skip if FVT_EEH!=1");                  \
      return;                                               \
    }                                                       \
  } while (0)

#define KV_4K   4   *1024
#define KV_8K   8   *1024
#define KV_64K  64  *1024
#define KV_250K 250 *1024
#define KV_500K 500 *1024
#define KV_1M   1024*1024
#define KV_2M   1024*1024*2

#define UNUSED(x) (void)(x)

extern char *env_FVT_DEV;

#endif
