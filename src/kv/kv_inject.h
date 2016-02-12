/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/kv_inject.h $                                          */
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
 * \ingroup
 ******************************************************************************/
#ifndef KV_INJECT_H
#define KV_INJECT_H

#include <inttypes.h>

int check_alloc_error_injects(void);
int check_sched_error_injects(int op);
int check_harv_error_injects (int op);

#define KV_INJECT_FLAGS_ACTIVE           0x8000
#define KV_INJECT_FLAGS_SCHD_READ_ERROR  0x4000
#define KV_INJECT_FLAGS_SCHD_WRITE_ERROR 0x2000
#define KV_INJECT_FLAGS_HARV_READ_ERROR  0x1000
#define KV_INJECT_FLAGS_HARV_WRITE_ERROR 0x0800
#define KV_INJECT_FLAGS_ALLOC_ERROR      0x0400

extern uint32_t kv_inject_flags;

#define KV_SET_INJECT_ACTIVE \
        kv_inject_flags |= KV_INJECT_FLAGS_ACTIVE

#define KV_SET_INJECT_INACTIVE \
        kv_inject_flags = 0

#define KV_INJECT_SCHD_READ_ERROR \
        kv_inject_flags |= KV_INJECT_FLAGS_SCHD_READ_ERROR

#define KV_INJECT_SCHD_WRITE_ERROR \
        kv_inject_flags |= KV_INJECT_FLAGS_SCHD_WRITE_ERROR

#define KV_INJECT_HARV_READ_ERROR \
        kv_inject_flags |= KV_INJECT_FLAGS_HARV_READ_ERROR

#define KV_INJECT_HARV_WRITE_ERROR \
        kv_inject_flags |= KV_INJECT_FLAGS_HARV_WRITE_ERROR

#define KV_INJECT_ALLOC_ERROR \
        kv_inject_flags |= KV_INJECT_FLAGS_ALLOC_ERROR

#define KV_SCHD_READ_ERROR_INJECT \
    (kv_inject_flags & KV_INJECT_FLAGS_ACTIVE && \
     kv_inject_flags & KV_INJECT_FLAGS_SCHD_READ_ERROR)

#define KV_SCHD_WRITE_ERROR_INJECT \
    (kv_inject_flags & KV_INJECT_FLAGS_ACTIVE && \
     kv_inject_flags & KV_INJECT_FLAGS_SCHD_WRITE_ERROR)

#define KV_HARV_READ_ERROR_INJECT \
    (kv_inject_flags & KV_INJECT_FLAGS_ACTIVE && \
     kv_inject_flags & KV_INJECT_FLAGS_HARV_READ_ERROR)

#define KV_HARV_WRITE_ERROR_INJECT \
    (kv_inject_flags & KV_INJECT_FLAGS_ACTIVE && \
     kv_inject_flags & KV_INJECT_FLAGS_HARV_WRITE_ERROR)

#define KV_ALLOC_ERROR_INJECT \
    (kv_inject_flags & KV_INJECT_FLAGS_ACTIVE && \
     kv_inject_flags & KV_INJECT_FLAGS_ALLOC_ERROR)

#define KV_CLEAR_SCHD_READ_ERROR \
     kv_inject_flags &= ~KV_INJECT_FLAGS_SCHD_READ_ERROR

#define KV_CLEAR_SCHD_WRITE_ERROR \
     kv_inject_flags &= ~KV_INJECT_FLAGS_SCHD_WRITE_ERROR

#define KV_CLEAR_HARV_READ_ERROR \
     kv_inject_flags &= ~KV_INJECT_FLAGS_HARV_READ_ERROR

#define KV_CLEAR_HARV_WRITE_ERROR \
     kv_inject_flags &= ~KV_INJECT_FLAGS_HARV_WRITE_ERROR

#define KV_CLEAR_ALLOC_ERROR \
     kv_inject_flags &= ~KV_INJECT_FLAGS_ALLOC_ERROR

#endif
