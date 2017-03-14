/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/kv_utils_db.c $                                   */
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
 *   utility functions to build key/value tables with random patterns for k/v
 * \ingroup
 ******************************************************************************/
#include <fvt_kv.h>
#include <kv_utils_db.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <arkdb_trace.h>

#define TRUE 1
#define FALSE 0

#define MAX_1BYTE_KEYS 255
#define MAX_2BYTE_KEYS 30000

/**
 *******************************************************************************
 * \brief
 *   return a random number >0
 ******************************************************************************/
static uint32_t
get_rand8(void)
{
    uint32_t r = 0;

    while (0 == r)
    {
        r = lrand48();
    }
    return r;
}

/**
 *******************************************************************************
 * \brief
 *   return a random number >0
 ******************************************************************************/
static uint8_t
get_rand1(void)
{
    uint32_t r = 0;

    while (0 == r)
    {
        r = rand() & 0xff;
    }
    return r;
}

/**
 *******************************************************************************
 * \brief
 *   return a random number between 0 and max
 ******************************************************************************/
uint32_t
get_rand_max(uint32_t max)
{
    uint32_t r = 0;

    while (0 == r)
        r = get_rand8() % (max+1);

    return r;
}

/**
 *******************************************************************************
 * \brief
 *   fill the array p of len bytes with random numbers
 ******************************************************************************/
static void
fill_rand(void *p, uint32_t len)
{
    uint32_t  i     = 0;
    uint32_t *p_u32 = (uint32_t*)p;
    uint8_t  *p_u8  = (uint8_t*)p;
    uint8_t   rbyt  = 4;
    uint32_t  div   = (len/rbyt);
    uint8_t   l32   = (div < rbyt) ? div : rbyt;

    assert(NULL != p);
    assert(0     < len);

    /* set the entire buf to a random number for general fill */
    memset(p_u8, get_rand1(), len);

    /* set up to 3 8-bit random numbers in any odd space */
    for (i=0; i<len-(rbyt*div); i++) *(p_u8++) = get_rand1();

    /* set up to 4 32-bit random numbers to ensure uniqueness */
    p_u32 = (uint32_t*)p_u8;
    for (i=0; i<l32; i++) *(p_u32++) = get_rand8();
}

/*******************************************************************************
 * print len bytes in p in hex
 ******************************************************************************/
void
disp(void *p, uint32_t len, uint32_t newline)
{
    uint32_t i  = 0;
    uint8_t *p8 = (uint8_t*)p;

    assert(NULL != p);
    assert(0    <  len);

    for (i=0; i<len; i++)
        printf("%.2X", p8[i]);

    if (newline) printf("\n");
}

/*******************************************************************************
 * print all the key/value pairs in the db
 ******************************************************************************/
void
disp_db(kv_t *db, uint32_t num_keys)
{
    uint32_t i = 0;

    assert(NULL != db);

    for (i=0; i<num_keys; i++)
    {
        printf("%.3d: ", i);
        disp(db[i].key,   db[i].klen, 0);
        printf(" : ");
        disp(db[i].value, db[i].vlen, 1);
    }
}

/**
 *******************************************************************************
 * \brief
 *   if e exists in db 2 times, return 1, else return 0
 ******************************************************************************/
static uint32_t
kv_db_exists_twice(kv_t *db, uint32_t num_keys, kv_t *e)
{
    uint32_t i     = 0;
    uint32_t found = 0;

    assert(NULL != db);

    for (i=0; i<num_keys; i++)
    {
        assert(NULL != db[i].key);
        assert(NULL != e->key);

        if (db[i].klen == e->klen &&
            memcmp(e->key, db[i].key, db[i].klen) == 0)
        {
            ++found;
        }
        if (2 == found)
        {
            assert(0 < db[i].klen);
            KV_TRC(pAT, "DUP(klen:%d key:%p):",db[i].klen, e->key);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 *******************************************************************************
 * \brief
 *   fill the key for element i in db with random bit patterns
 ******************************************************************************/
static void
create_key(kv_t *db, uint32_t i, uint32_t klen)
{
    kv_t *e = db + i;

    assert(NULL != db);
    assert(0    <  klen);
    assert(0    <= i);

    fill_rand(e->key, klen);

    /* if klen > 512, its not likely that two keys match */
    if (klen >= 512)
    {
        fill_rand(e->key, klen);
    }
    else
    {
        while (kv_db_exists_twice(db, i+1, e))
        {
            fill_rand(e->key, klen);
        }
    }
    //disp(e->key, klen, TRUE);
}

/*******************************************************************************
 * create a fixed db of size num with keys the len of key_size
 * and values the len of value_size
 ******************************************************************************/
void*
kv_db_create_fixed(uint32_t num, uint32_t key_size, uint32_t value_size)
{
    void    *rc   = NULL;
    uint32_t i    = 0;
    kv_t    *db   = (kv_t*)malloc(num*sizeof(kv_t));

    assert(NULL != db && 0 < key_size && 0 <= value_size);
    assert(!(key_size == 1 && num > MAX_1BYTE_KEYS));
    assert(!(key_size == 2 && num > MAX_2BYTE_KEYS));

    srand(time(0));
    srand48(time(0));
    bzero(db, num*sizeof(kv_t));

    for (i=0; i<num; i++)
    {
        assert(0 < key_size);
        db[i].key = malloc(key_size);
        assert(NULL != db[i].key);
        db[i].klen = key_size;
        db[i].tag  = key_size;
        create_key(db, i, key_size);

        if (0 == value_size)
        {
            db[i].value = NULL;
            db[i].vlen  = 0;
            continue;
        }

        db[i].value = malloc(value_size);
        assert(NULL != db[i].value);
        fill_rand(db[i].value, value_size);
        db[i].vlen = value_size;
    }
    rc = db;

    KV_TRC(pAT, "CREATE_FIXED: %p %dx%dx%d, rc=%p",
            db, key_size, value_size, num, rc);
    return rc;
}

/*******************************************************************************
 * create a db of size num with variable length keys/values
 ******************************************************************************/
void*
kv_db_create_mixed(uint32_t num, uint32_t key_max, uint32_t value_max)
{
    void    *rc    = NULL;
    uint32_t i     = 0;
    kv_t    *db    = (kv_t*)malloc(num*sizeof(kv_t));
    uint32_t size  = 0;
    uint32_t byte1 = 0;
    uint32_t byte2 = 0;

    assert(NULL != db && 0 < key_max && 0 <= value_max);
    assert(!(key_max == 1 && num > MAX_1BYTE_KEYS));
    assert(!(key_max == 2 && num > MAX_2BYTE_KEYS));

    srand(time(0));
    srand48(time(0));
    bzero(db, num*sizeof(kv_t));

    for (i=0; i<num; i++)
    {
        if (key_max > 1)
        {
            size = get_rand_max(key_max);
            /* if we already have 256 1byte keys, can't have anymore */
            if (1 == size && MAX_1BYTE_KEYS == byte1)
            {
                size = 2;
            }
            /* if we already have 64k 2byte keys, can't have anymore */
            if (2 == size && MAX_2BYTE_KEYS == byte2)
            {
                size = 3;
            }
            if      (1 == size) ++byte1;
            else if (2 == size) ++byte2;
        }
        else
        {
            size = 1;
        }
        assert(0 < size);
        db[i].key = malloc(size);
        assert(NULL != db[i].key);
        db[i].klen = size;
        db[i].tag  = size;
        create_key(db, i, size);

        if (0 == value_max)
        {
            db[i].value = NULL;
            db[i].vlen  = 0;
            continue;
        }

        if (value_max > 1)
        {
            size = get_rand_max(value_max);
        }
        else
        {
            size = 1;
        }
        assert(0 < size);
        db[i].value = malloc(size);
        assert(NULL != db[i].value);
        fill_rand(db[i].value, size);
        db[i].vlen = size;
    }
    rc = db;

    KV_TRC(pAT, "CREATE_MIXED: %p %dx%dx%d, i:%d rc=%p",
            db,key_max,value_max,num,i,rc);
    return rc;
}

/*******************************************************************************
 *   replace the value for each key
 ******************************************************************************/
uint32_t kv_db_fixed_regen_values(kv_t *db, uint32_t num, uint32_t vlen)
{
    uint32_t rc = 0;
    uint32_t i  = 0;

    for (i=0; i<num; i++)
    {
        if (NULL != db[i].value) free(db[i].value);

        db[i].value = NULL;
        db[i].vlen  = vlen;

        if (vlen == 0) continue;

        db[i].value = malloc(vlen);
        assert(NULL != db[i].value);
        fill_rand(db[i].value, vlen);
    }

    KV_TRC_IO(pAT, "REGEN_FIXED: %p %dx%d, rc=%d", db, vlen, num, rc);
    return rc;
}

/*******************************************************************************
 *   replace the value for each key
 ******************************************************************************/
uint32_t kv_db_mixed_regen_values(kv_t *db, uint32_t num, uint32_t value_max)
{
    uint32_t rc   = 0;
    uint32_t i    = 0;
    uint32_t vlen = 0;

    for (i=0; i<num; i++)
    {
        if (NULL != db[i].value) free(db[i].value);

        db[i].value = NULL;

        if (value_max == 0)
        {
            db[i].vlen = 0;
            continue;
        }

        if (value_max > 1)
        {
            vlen = get_rand_max(value_max);
        }
        else
        {
            vlen = value_max;
        }
        db[i].value = malloc(vlen);
        db[i].vlen  = vlen;
        assert(NULL != db[i].value);
        fill_rand(db[i].value, vlen);
    }

    KV_TRC_IO(pAT, "REGEN_MIXED: %p %dx%d, rc=%d", db, value_max, num, rc);
    return rc;
}

/*******************************************************************************
 * free all the storage malloc'ed for the db
 ******************************************************************************/
void
kv_db_destroy(kv_t *db, uint32_t num_keys)
{
    uint32_t i = 0;

    assert(NULL != db);
    assert(0     < num_keys);

    for (i=0; i<num_keys; i++)
    {
        if (NULL != db[i].key)   free(db[i].key);
        if (NULL != db[i].value) free(db[i].value);
    }

    free(db);
    KV_TRC(pAT, "DB_DESTROY: %p #keys=%d", db, num_keys);
}

/*******************************************************************************
 * search for key in db
 ******************************************************************************/
void*
kv_db_find(kv_t *db, uint32_t num_keys, void *key, uint32_t klen)
{
    uint32_t i = 0;

    if (NULL == db) return NULL;

    for (i=0; i<num_keys; i++)
    {
        if (db[i].klen == klen &&
            memcmp(key, db[i].key, klen) == 0)
        {
            if (db[i].value == NULL) return "";
            else                     return db[i].value;
        }
    }
    return NULL;
}
