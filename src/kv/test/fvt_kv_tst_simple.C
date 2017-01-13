/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_simple.C $                             */
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
 *   Simple test cases for kv FVT
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv_utils.h>
#include <errno.h>
#include <iv.h>
#include <bl.h>
#include <ark.h>
}

/**
 *******************************************************************************
 * \brief
 *   create kv databases and make sure lookups work, then destroy
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_validate_kv_utils)
{
    int32_t i    = 0;
    int32_t FLEN = 1000;
    int32_t MLEN = 1000;
    void   *null = NULL;

    kv_t *db_f = (kv_t*)kv_db_create_fixed(FLEN, 64, 4);
    ASSERT_TRUE(db_f != NULL);

    kv_t *db_m = (kv_t*)kv_db_create_mixed(MLEN, 128, 64);
    ASSERT_TRUE(db_m != NULL);

    for (i=0; i<FLEN; i++)
    {
        ASSERT_NE(null, kv_db_find(db_f, FLEN, db_f[i].key, db_f[i].klen));
    }

    for (i=0; i<MLEN; i++)
    {
        ASSERT_NE(null, kv_db_find(db_m, MLEN, db_m[i].key, db_m[i].klen));
    }
    kv_db_destroy(db_f, FLEN);
    kv_db_destroy(db_m, MLEN);
}

/**
 *******************************************************************************
 * \brief
 *   test IV functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_iv)
{
    uint64_t n  = 233;
    uint64_t m  = 17;
    uint32_t i  = 0;
    IV      *iv = NULL;

    ASSERT_TRUE(iv_set(iv, 0, 0) < 0);
    ASSERT_TRUE(iv_get(iv, 0)    < 0);

    iv = iv_new(n, m);
    ASSERT_TRUE(NULL != iv);
    for (i=0;   i<n;  i++)        ASSERT_TRUE(0 == iv_set(iv, i, i));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i == iv_get(iv, i));

    n  = 433;
    iv = iv_resize(iv, n, m);
    ASSERT_TRUE(0 != iv);
    for (i=0;   i<n;  i++)        ASSERT_TRUE(0 == iv_set(iv, i, i));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i == iv_get(iv, i));

    for (i=800; i<800+n; i++)     ASSERT_TRUE(iv_set(iv, i, i) < 0);
    for (i=800; i<800+n; i++)     ASSERT_TRUE(iv_get(iv, i)    < 0);

    for (i=0;   i<n;  i++)        ASSERT_TRUE(0    == iv_set(iv, i, i+10));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i+10 == iv_get(iv, i));

    iv_delete(iv);
}

/**
 *******************************************************************************
 * \brief
 *   test BL functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_bl)
{
    uint64_t n  = 233;
    uint64_t w  = 17;
    BL      *bl = NULL;

    ASSERT_TRUE(bl_init_chain_link(bl) == 1);
    ASSERT_TRUE(bl_reserve(bl,1) < 0);
    ASSERT_TRUE(bl_take(bl,1) < 0);
    ASSERT_TRUE(bl_drop(bl,1) < 0);
    ASSERT_TRUE(bl_left(bl) < 0);
    ASSERT_TRUE(bl_len(bl,1) == 0);
    ASSERT_TRUE(bl_next(bl,1) < 0);
    ASSERT_TRUE(bl_chain(bl,1,1) == 0);
    ASSERT_TRUE(bl_chain_blocks(bl,1,1) == 0);
    bl_check_take(bl, 1);

    bl = bl_new(n, w);
    ASSERT_TRUE(NULL != bl);

    bl_delete(bl);
}

/**
 *******************************************************************************
 * \brief
 *   create/delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_create_delete)
{
    ARK  *ark = NULL;

    ARK_CREATE;
    usleep(100000);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SET_GET_EXISTS_DEL)
{
    ARK     *ark  = NULL;
    char     k[]  = {"1234"};
    uint32_t klen = 4;
    char     v[]  = {"12345678"};
    char     g[8] = {0};
    uint32_t vlen = 8;
    int64_t  res  = 0;

    ARK_CREATE;

    EXPECT_EQ(ENOENT, ark_get   (ark, klen, k, vlen, g, 0, &res));
    EXPECT_EQ(ENOENT, ark_exists(ark, klen, k, &res));
    EXPECT_EQ(ENOENT, ark_del   (ark, klen, k, &res));
    EXPECT_EQ(-1, res);
    EXPECT_EQ(0,      ark_set   (ark, klen, k, vlen, v, &res));
    EXPECT_EQ(vlen, res);
    EXPECT_EQ(0,      ark_get   (ark, klen, k, vlen, g, 0, &res));
    EXPECT_EQ(vlen, res);
    EXPECT_EQ(0, memcmp(v,g,vlen));
    EXPECT_EQ(0,      ark_exists(ark, klen, k, &res));
    EXPECT_EQ(0,      ark_del   (ark, klen, k, &res));
    EXPECT_EQ(vlen, res);
    EXPECT_EQ(ENOENT, ark_get   (ark, klen, k, vlen, g, 0, &res));
    EXPECT_EQ(ENOENT, ark_exists(ark, klen, k, &res));

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_1x0x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 0;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_1x1x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 1;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_2x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 2;
    uint32_t vlen  = 2;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_4x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 4;
    uint32_t vlen  = 3;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_32x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 32;
    uint32_t vlen  = 128;
    uint32_t LEN   = 500;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_64x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_128x64K)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_64K;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_WITH_OFFSET_FIXED_3x11)
{
    ARK     *ark   = NULL;
    kv_t    *db    = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 11;
    uint32_t LEN   = 200;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query_off(ark, db, vlen, LEN);

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_3x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 2;
    uint32_t LEN   = 300;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_10x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 10;
    uint32_t vlen  = 10;
    uint32_t LEN   = 500;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_100x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 100;
    uint32_t vlen  = 100;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_BIG_BLOCKS)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_2M;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_1x0)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 0;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_64x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 128;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_3x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 2;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_64x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 128;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_BIG_BLOCKS)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_2M;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_ark_get_PARTIAL_VALUE)
{
    ARK     *ark  = NULL;
    char     s[]  = {"12345"};
    char     g[5] = {0};
    int64_t  res  = 0;

    ARK_CREATE;

    EXPECT_EQ(0, ark_set(ark,  5,    s,    5,    s,    &res));
    EXPECT_EQ(0, ark_get(ark,  5,    s,    3,    g, 0, &res));

    EXPECT_EQ(0, memcmp(s,g,3));
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   create, query non-existing key, delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_count)
{
    ARK     *ark   = NULL;
    kv_t    *db    = NULL;
    int32_t  LEN   = 394;
    int64_t  count = 0;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, 32, 1);
    ASSERT_TRUE(db != NULL);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, LEN);

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   load keys, ark_random, verify key is valid and different from last key,
 *    delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_random)
{
    ARK     *ark           = NULL;
    kv_t    *db            = NULL;
    int32_t  LEN           = 1000;
    uint64_t klen          = 0;
    uint64_t sklen         = 0;
    uint32_t i             = 0;
    char     svalue[KV_4K] = {0};
    char     kvalue[KV_4K] = {0};
    void    *null          = NULL;

    ARK_CREATE;

    EXPECT_EQ(ENOENT, ark_random(ark, KV_4K, &klen, kvalue));

    db = (kv_t*)kv_db_create_fixed(LEN, 64, 1);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    for (i=0; i<100; i++)
    {
        ASSERT_EQ(0, ark_random(ark, KV_4K, &klen, kvalue));

        /* make sure the key exists in our db */
        ASSERT_NE(null, kv_db_find(db, LEN, kvalue, klen));

        /* make sure the new key is not the same as the last key */
        if (i && klen == sklen)
        {
            ASSERT_NE(0, memcmp(kvalue, svalue, klen));
        }

        /* save this key to compare to the next key */
        memcpy(svalue, kvalue, klen);
        sklen = klen;
    }

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_next_FIXED_256x1x10000)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    int32_t  i             = 0;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 10000;
    int64_t  res           = 0;
    int64_t  klen          = 0;
    int32_t  num           = 1;
    int32_t  rc            = 0;
    void    *null          = NULL;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, 256, 1);
    ASSERT_TRUE(db != NULL);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari != NULL);
    ASSERT_TRUE(klen != 0);

    /* use ark_next to query all key/value pairs from the ark */
    for (i=1; i<LEN; i++)
    {
        ASSERT_NE(null, kv_db_find(db, LEN, gvalue, klen));
        ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
        ASSERT_NE(0, res);
        rc = ark_next(ari, KV_4K, &klen, gvalue);
        ASSERT_TRUE(klen == 256);
        ASSERT_TRUE(rc == 0);
        ++num;
    }
    // query LEN+1
    ASSERT_EQ(ENOENT, ark_next(ari, KV_4K, &klen, gvalue));

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_next_MIXED_256x4)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    int32_t  i             = 0;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 500;
    int64_t  res           = 0;
    int64_t  klen          = 0;
    int32_t  num           = 0;
    int32_t  rc            = 0;
    void    *null          = NULL;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_mixed(LEN, 256, 4);
    ASSERT_TRUE(db != NULL);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari != NULL);
    ASSERT_TRUE(klen != 0);

    /* use ark_next to query all key/value pairs from the ark */
    for (i=0; i<LEN; i++)
    {
        ASSERT_NE(null, kv_db_find(db, LEN, gvalue, klen));
        ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
        ASSERT_NE(0, res);
        rc = ark_next(ari, KV_4K, &klen, gvalue);
        ASSERT_TRUE(rc == 0 || rc == ENOENT);
        ++num;
    }
    ASSERT_EQ(LEN, num);

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_allocated_inuse_actual)
{
    ARK     *ark  = NULL;
    kv_t    *db   = NULL;
    uint64_t ret  = 0;
    uint64_t num  = 0;
    int32_t  LEN  = 1000;
    int      rc   = 0;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, 64, 1);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    /* alloc:5246976 */
    rc = ark_allocated(ark, &ret);
    EXPECT_EQ(0, rc);
    num = 5100000; EXPECT_LT(num, ret);
    num = 5300000; EXPECT_GT(num, ret);
    /* inuse:4091904 */
    rc = ark_inuse(ark, &ret);
    EXPECT_EQ(0, rc);
    num = 3800000; EXPECT_LT(num, ret);
    num = 4200000; EXPECT_GT(num, ret);
    /* actual:67000 */
    rc = ark_actual(ark, &ret);
    EXPECT_EQ(0, rc);
    num = 66000; EXPECT_LT(num,  ret);
    num = 68000; EXPECT_GT(num, ret);

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_long_hash_list_smallkv)
{
    ARK     *ark  = NULL;
    kv_t    *db   = NULL;
    int32_t  LEN  = 4000;
    int      vlen = 32;
    char    *dev  = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                        ARK_VERBOSE_SIZE_DEF,
                                        ARK_VERBOSE_BSIZE_DEF,
                                        14,
                                        ARK_VERBOSE_NTHRDS_DEF,
                                        ARK_MAX_NASYNCS,
                                        ARK_EA_BLK_ASYNC_CMDS,
                                        ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_fixed(LEN, 16, vlen);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_long_hash_list)
{
    ARK     *ark  = NULL;
    kv_t    *db   = NULL;
    int32_t  LEN  = 500;
    int      vlen = 5000;
    char    *dev  = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                        ARK_VERBOSE_SIZE_DEF,
                                        ARK_VERBOSE_BSIZE_DEF,
                                        4,
                                        ARK_VERBOSE_NTHRDS_DEF,
                                        ARK_MAX_NASYNCS,
                                        ARK_EA_BLK_ASYNC_CMDS,
                                        ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_fixed(LEN, 4096, 5000);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    kv_db_destroy(db, LEN);
    ARK_DELETE;
}
