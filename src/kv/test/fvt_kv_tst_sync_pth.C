/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_sync_pth.C $                           */
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
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>
#include <fvt_kv_utils_sync_pth.h>

extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_PTH_FIXED_8x2)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 1;
    uint32_t vlen     = 2;
    uint32_t LEN      = 200;
    uint32_t secs     = 3;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_PTH_FIXED_8x4k)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 1;
    uint32_t vlen     = KV_4K;
    uint32_t LEN      = 200;
    uint32_t secs     = 3;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_PTH_FIXED_BIG_BLOCKS)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 1;
    uint32_t vlen     = KV_500K;
    uint32_t LEN      = 50;
    uint32_t secs     = 3;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_2_PTH_1_CONTEXT)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 2;
    uint32_t vlen     = KV_4K;
    uint32_t LEN      = 200;
    uint32_t secs     = 3;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_64_PTH_BIG_BLOCKS)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 64;
    uint32_t vlen     = KV_500K;
    uint32_t LEN      = 2;
    uint32_t secs     = 3;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_100_PTH_STARVE)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 100;
    uint32_t vlen     = KV_64K;
    uint32_t LEN      = 100;
    uint32_t secs     = 3;

    TESTCASE_SKIP_IF_FILE;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, 50, 256, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_1_PTH_2_CONTEXT)
{
    uint32_t num_ctxt = 2;
    uint32_t num_pth  = 1;
    uint32_t vlen     = KV_4K;
    uint32_t LEN      = 200;
    uint32_t secs     = 3;

    TESTCASE_SKIP_IF_FILE;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_4_PTH_40_CONTEXT)
{
    uint32_t num_ctxt = 40;
    uint32_t num_pth  = 4;
    uint32_t vlen     = 128;
    uint32_t LEN      = 200;
    uint32_t secs     = 3;

    TESTCASE_SKIP_IF_FILE;
    TESTCASE_SKIP_IF_RAID0;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}
