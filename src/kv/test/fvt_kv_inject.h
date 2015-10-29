/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_inject.h $                                 */
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
#ifndef FVT_KV_INJECT_H
#define FVT_KV_INJECT_H

#include <inttypes.h>

#define FVT_INJECT_FLAGS_ACTIVE      0x8000
#define FVT_INJECT_FLAGS_READ_ERROR  0x4000
#define FVT_INJECT_FLAGS_WRITE_ERROR 0x2000
#define FVT_INJECT_FLAGS_ALLOC_ERROR 0x1000

extern uint32_t fvt_kv_inject;

#define FVT_KV_SET_INJECT_ACTIVE \
        fvt_kv_inject |= FVT_INJECT_FLAGS_ACTIVE

#define FVT_KV_SET_INJECT_INACTIVE \
     fvt_kv_inject = 0

#define FVT_KV_INJECT_READ_ERROR \
        fvt_kv_inject |= FVT_INJECT_FLAGS_READ_ERROR

#define FVT_KV_INJECT_WRITE_ERROR \
        fvt_kv_inject |= FVT_INJECT_FLAGS_WRITE_ERROR

#define FVT_KV_INJECT_ALLOC_ERROR \
        fvt_kv_inject |= FVT_INJECT_FLAGS_ALLOC_ERROR

#define FVT_KV_READ_ERROR_INJECT \
    (fvt_kv_inject & FVT_INJECT_FLAGS_ACTIVE && \
     fvt_kv_inject & FVT_INJECT_FLAGS_READ_ERROR)

#define FVT_KV_WRITE_ERROR_INJECT \
    (fvt_kv_inject & FVT_INJECT_FLAGS_ACTIVE && \
     fvt_kv_inject & FVT_INJECT_FLAGS_WRITE_ERROR)

#define FVT_KV_ALLOC_ERROR_INJECT \
    (fvt_kv_inject & FVT_INJECT_FLAGS_ACTIVE && \
     fvt_kv_inject & FVT_INJECT_FLAGS_ALLOC_ERROR)

#define FVT_KV_CLEAR_READ_ERROR \
     fvt_kv_inject &= ~FVT_INJECT_FLAGS_READ_ERROR

#define FVT_KV_CLEAR_WRITE_ERROR \
     fvt_kv_inject &= ~FVT_INJECT_FLAGS_WRITE_ERROR

#define FVT_KV_CLEAR_ALLOC_ERROR \
     fvt_kv_inject &= ~FVT_INJECT_FLAGS_ALLOC_ERROR

#endif
