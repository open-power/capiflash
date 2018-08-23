/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/kv_utils_db.h $                                   */
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
 *   functions to aid in testing kv software
 * \details
 *   the array database created is an array of kv_t structs, with a key/value
 *   malloc'd for each kv_t. The fixed db malloc's keys with the same len, and
 *   mallocs values with the same length. The mixed db malloc's all keys/values
 *   with random lengths.
 * \ingroup
 ******************************************************************************/
#ifndef TST_KV_UTILS_H
#define TST_KV_UTILS_H

#include <inttypes.h>

typedef struct
{
    void    *key;
    void    *value;
    uint32_t klen;
    uint32_t vlen;
    uint32_t tag;
} kv_t;

/**
 *******************************************************************************
 * \brief
 *   create an array database with fixed key/value sizes, & init random patterns
 ******************************************************************************/
void* kv_db_create_fixed(uint32_t num, uint32_t key_size, uint32_t value_size);

/**
 *******************************************************************************
 * \brief
 *   create an array database with max key/value sizes, and init random patterns
 * \details
 *   each key and value are random lengths from 1 byte to the max bytes
 ******************************************************************************/
void* kv_db_create_mixed(uint32_t num, uint32_t key_max, uint32_t value_max);

/**
 *******************************************************************************
 * \brief
 *   replace the value for each key
 ******************************************************************************/
uint32_t kv_db_fixed_regen_values(kv_t *db, uint32_t num, uint32_t value_max);

/**
 *******************************************************************************
 * \brief
 *   replace the value for each key
 ******************************************************************************/
uint32_t kv_db_mixed_regen_values(kv_t *db, uint32_t num, uint32_t value_max);

/**
 *******************************************************************************
 * \brief
 *   destroy an array database
 ******************************************************************************/
void kv_db_destroy(kv_t *db, uint32_t num_keys);

/**
 *******************************************************************************
 * \brief
 *   lookup key and return value if found, else return NULL
 ******************************************************************************/
void* kv_db_find(kv_t *db, uint32_t num_keys, void *key, uint32_t klen);

/**
 *******************************************************************************
 * \brief
 *   lookup key and delete value if found, else return 0
 ******************************************************************************/
int kv_db_delete(kv_t *db, uint32_t num_keys, void *key, uint32_t klen);

/**
 *******************************************************************************
 * \brief
 *   print an array of bytes in hex, with newline=1, or without=0
 ******************************************************************************/
void disp(void *p, uint32_t len, uint32_t newline);

/**
 *******************************************************************************
 * \brief
 *   print each key/value in the db
 ******************************************************************************/
void disp_db(kv_t *db, uint32_t num_keys);

/**
 *******************************************************************************
 * \brief
 *   gen a random number X, where 0 < X < max
 ******************************************************************************/
uint32_t
get_rand_max(uint32_t max);

#endif
