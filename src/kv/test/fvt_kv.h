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

#include <arkdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <fvt_trace.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ARK_CREATE \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV"), &ark, ARK_KV_VIRTUAL_LUN)); \
    ASSERT_TRUE(ark != NULL);

#define ARK_CREATE_PERSIST_READONLY \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark, \
                            ARK_KV_PERSIST_LOAD));           \
    ASSERT_TRUE(ark != NULL);

#define ARK_CREATE_PERSIST \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark,              \
                            ARK_KV_PERSIST_STORE | ARK_KV_PERSIST_LOAD)); \
    ASSERT_TRUE(ark != NULL);

#define ARK_CREATE_NEW_PERSIST \
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV_PERSIST"), &ark, \
                            ARK_KV_PERSIST_STORE));          \
    ASSERT_TRUE(ark != NULL);

#define ARK_DELETE \
    ASSERT_EQ(0, ark_delete(ark))

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
