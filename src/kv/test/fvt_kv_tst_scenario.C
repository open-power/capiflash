/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_scenario.C $                           */
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
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_CREATE_DELETE_LOOP)
{
    ARK     *ark   = NULL;
    int32_t  loops = 100;
    int32_t  i     = 0;

    for (i=0; i<loops; i++)
    {
        ARK_CREATE;
        ARK_DELETE;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_CREATE_DELETE_LOOP_FIXED_32x32)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 32;
    uint32_t vlen  = 32;
    uint32_t LEN   = 50;
    uint32_t secs  = 1;
    uint32_t i     = 0;

    for (i=0; i<5; i++)
    {
        ARK_CREATE;

        fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);

        ARK_DELETE;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_CREATE_DELETE_LOOP_MIXED_10x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 10;
    uint32_t vlen  = 10;
    uint32_t LEN   = 200;
    uint32_t secs  = 1;
    uint32_t i     = 0;

    for (i=0; i<5; i++)
    {
        ARK_CREATE;

        fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);

        ARK_DELETE;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_FIXED_1x0)
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
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_FIXED_4x4)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 4;
    uint32_t vlen  = 4;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_FIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 2000;
    uint32_t secs  = 3;

    ARK_CREATE;

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_MIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 2000;
    uint32_t secs  = 3;

    ARK_CREATE;

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_MIXED_BIG_BLOCKS_250K)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = KV_250K;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_SGD_LOOP_FIXED_BIG_BLOCKS_2M)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = KV_2M;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_FIXED_1x0)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 0;
    uint32_t LEN   = 200;
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
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_FIXED_2x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 2;
    uint32_t vlen  = 2;
    uint32_t LEN   = 200;
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
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_FIXED_256x4096)
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
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_MIXED_3x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 10;
    uint32_t LEN   = 300;
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
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_MIXED_256x4096)
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
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_MIXED_BIG_BLOCKS_250K)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_250K;
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
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_REP_LOOP_MIXED_BIG_BLOCKS_2M)
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
TEST(FVT_KV_GOOD_PATH, SCENARIO_COMBO)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 100;
    uint32_t vlen  = 256;
    int32_t  LEN   = 200;
    int32_t  secs  = 3;
    kv_t    *db_f  = NULL;
    uint32_t Fklen = 130;
    uint32_t Fvlen = 8;
    int64_t  count = 0;

    ARK_CREATE;

    /* create a fixed db_f to load and leave in the ark for the mixed loop */
    db_f = (kv_t*)kv_db_create_fixed(LEN, Fklen, Fvlen);
    ASSERT_TRUE(db_f != NULL);

    /* fill ark before loop */
    fvt_kv_utils_load (ark, db_f, LEN);
    fvt_kv_utils_query(ark, db_f, Fvlen, LEN);

    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);

    /* empty ark */
    fvt_kv_utils_del(ark, db_f, LEN);
    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    /* fill ark before loop */
    fvt_kv_utils_load (ark, db_f, LEN);
    fvt_kv_utils_query(ark, db_f, Fvlen, LEN);

    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);

    /* empty ark */
    fvt_kv_utils_del(ark, db_f, LEN);
    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    kv_db_destroy(db_f, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SCENARIO_fork_single)
 {
    ARK     *ark    = NULL;
    uint32_t fklen  = 30;
    uint32_t fvlen  = 111;
    kv_t    *db_f   = NULL;
    uint32_t klen   = 73;
    uint32_t vlen   = 179;
    int32_t  LEN    = 200;
    uint32_t secs   = 10;
    pid_t    pid    =-1;
    int      cdone  = 0;

    ARK_CREATE;

    /* create a fixed db_f to load and leave in the ark for the mixed loop */
    db_f = (kv_t*)kv_db_create_fixed(LEN, fklen, fvlen);
    ASSERT_TRUE(db_f != NULL);

    /* fill ark before loop */
    fvt_kv_utils_load (ark, db_f, LEN);
    fvt_kv_utils_query(ark, db_f, fvlen, LEN);

    pid = ark_fork(ark);
    ASSERT_TRUE(-1 != pid);

    if (0 == pid)
    {
        uint32_t start,cur;
        printf("child start\n");
        start = time(0);
        /* act like we are writing the ark contents to a backup */
        do
        {
            fvt_kv_utils_query(ark, db_f, fvlen, LEN);
            cur = time(0);
        }
        while (cur-start < secs);
        cdone = 1;
        printf("child done\n");
        _exit(0);
    }

    EXPECT_EQ(0, ark_fork_done(ark));

    printf("parent start\n");

    /* do set/get/del secs while child is doing backup */
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);

    printf("parent wait\n");

    wait(&cdone);

    printf("parent wait done\n");

    fvt_kv_utils_query(ark, db_f, fvlen, LEN);
    fvt_kv_utils_del(ark, db_f, LEN);
    printf("parent done\n");

    kv_db_destroy(db_f, LEN);
    ARK_DELETE;
}
