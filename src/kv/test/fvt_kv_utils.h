/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils.h $                                  */
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
 *   functions to aid in KV testing
 * \ingroup
 ******************************************************************************/
#ifndef FVT_KV_UTILS_H
#define FVT_KV_UTILS_H

#include <fvt_kv.h>
#include <kv_utils_db.h>

/**
 *******************************************************************************
 * \brief
 *   load seed generated key/values into the ark using ark_set
 ******************************************************************************/
void fvt_kv_utils_sload(ARK     *ark,
                        uint64_t seed,
                        uint32_t klen,
                        uint32_t vlen,
                        uint32_t LEN);
/**
 *******************************************************************************
 * \brief
 *   query seed generated key/values in the ark using ark_get/ark_exists
 ******************************************************************************/
void fvt_kv_utils_squery(ARK     *ark,
                         uint64_t seed,
                         uint32_t klen,
                         uint32_t vlen,
                         uint32_t LEN);
/**
 *******************************************************************************
 * \brief
 *   query seed generated key/values in the ark using ark_get/ark_exists
 ******************************************************************************/
void fvt_kv_utils_squery_empty(ARK     *ark,
                               uint64_t seed,
                               uint32_t klen,
                               uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   delete seed generated key/values from the ark using ark_del
 ******************************************************************************/
void fvt_kv_utils_sdel(ARK     *ark,
                       uint64_t seed,
                       uint32_t klen,
                       uint32_t vlen,
                       uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   create db, set, get, exists, del, destroy seed generated key/values
 ******************************************************************************/
void fvt_kv_utils_SGD_SLOOP(ARK     *ark,
                           uint64_t seed,
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t len,
                           uint32_t secs);

/**
 *******************************************************************************
 * \brief
 *   load all the db key/values into the ark using ark_set
 ******************************************************************************/
void fvt_kv_utils_load(ARK *ark, kv_t *db, uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   query all the db key/values in the ark using ark_get/ark_exists
 ******************************************************************************/
void fvt_kv_utils_query(ARK *ark, kv_t *db, uint32_t vbuflen, uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   query all the db key/values in the ark using ark_get/ark_exists and offset
 ******************************************************************************/
void fvt_kv_utils_query_off(ARK *ark, kv_t *db, uint32_t vbuflen, uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   delete all the db key/values from the ark using ark_del
 ******************************************************************************/
void fvt_kv_utils_del(ARK *ark, kv_t *db, uint32_t LEN);

/**
 *******************************************************************************
 * \brief
 *   create db, set, get, exists, del, destroy db
 ******************************************************************************/
void fvt_kv_utils_SGD_LOOP(ARK     *ark,
                           void*  (*db_create)(uint32_t, uint32_t, uint32_t),
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t len,
                           uint32_t secs);

/**
 *******************************************************************************
 * \brief
 *   create db, set, get, exists, regen db, set, get, exists, del, destroy db
 ******************************************************************************/
void fvt_kv_utils_REP_LOOP(ARK       *ark,
                           void*    (*db_create)(uint32_t, uint32_t, uint32_t),
                           uint32_t (*db_regen) (kv_t*,    uint32_t, uint32_t),
                           uint32_t   klen,
                           uint32_t   vlen,
                           uint32_t   len,
                           uint32_t   secs);

/**
 *******************************************************************************
 * \brief
 *   calc sync data rates for read and write
 ******************************************************************************/
void fvt_kv_utils_perf(ARK *ark, uint32_t vlen, uint32_t mb, uint32_t LEN);

#endif
