/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_sync_async.C $                         */
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

extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
#include <fvt_kv_utils_sync_pth.h>
#include <fvt_kv_utils_ark_io.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_CTXT_10_PTH_10_SMALL_BLOCKS)
{
    uint32_t num_ctxt = 10;
    uint32_t num_pth  = 10;
    uint32_t vlen     = 16;
    uint32_t LEN      = 100;
    uint32_t secs     = 5;

    TESTCASE_SKIP_IF_FILE;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_CTXT_10_PTH_32_BIG_BLOCKS)
{
    uint32_t num_ctxt = 10;
    uint32_t num_pth  = 32;
    uint32_t vlen     = KV_64K;
    uint32_t LEN      = 20;
    uint32_t secs     = 5;

    TESTCASE_SKIP_IF_FILE;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_ASYNC_EASY)
{
    uint32_t klen   = 256;
    uint32_t vlen   = KV_64K;
    uint32_t LEN    = 20;
    uint32_t secs   = 5;

    kv_async_init_ctxt(ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_easy(ASYNC_SINGLE_CONTEXT);

    printf("start async jobs\n");
    kv_async_start_jobs();

    printf("start sync job\n");
    fvt_kv_utils_SGD_LOOP(kv_async_get_ark(ASYNC_SINGLE_CONTEXT),
                          kv_db_create_fixed, klen, vlen, LEN, secs);

    printf("wait for async jobs\n");
    kv_async_wait_jobs();
}

void pr_perf_fail(const char *s, uint32_t x, uint32_t y)
{
    printf("*******************************\n");
    printf("* FAILURE: %s: %d < %d\n", s, x, y);
    printf("*******************************\n");
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_ASYNC_PERF)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 128;
    uint32_t npool    = 1;
    uint32_t vlen     = 16;
    uint32_t LEN      = 500;
    uint32_t secs     = 15;
    uint32_t ios      = 0;
    uint32_t ops      = 0;
    uint32_t e_ios    = 0;
    uint32_t e_ops    = 0;

    Sync_pth sync_pth;

    num_ctxt = 1; e_ops=e_ios=70000;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);

    num_ctxt=1; vlen=KV_64K; num_pth=40; e_ops=11500; e_ios=112000;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);

#ifndef _AIX
    num_ctxt = 1; vlen = KV_500K; num_pth = 20;  e_ops=2200; e_ios=150000;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);
#endif

    TESTCASE_SKIP_IF_FILE;

    num_ctxt = 4; vlen = 16; num_pth = 128; e_ops=e_ios=100000; secs=25;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);

    num_ctxt = 4; num_pth = 20; vlen = KV_64K; e_ops=19000;e_ios=180000;secs=25;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);

    num_ctxt = 20; vlen = 16; num_pth = 20; e_ops=e_ios=110000; secs=25;
    sync_pth.run_multi_ctxt(num_ctxt, num_pth, npool, vlen, LEN,secs,&ops,&ios);
    if (ops < e_ops) pr_perf_fail("ops", ops, e_ops);
    if (ios < e_ios) pr_perf_fail("ios", ios, e_ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_ASYNC_NPOOL_4)
{
    uint32_t num_ctxt = 1;
    uint32_t ops      = 20;
    uint32_t npool    = 4;
    uint32_t klen     = 16;
    uint32_t vlen     = 128;
    uint32_t LEN      = 500;
    uint32_t secs     = 15;

    kv_async_init_perf_io(num_ctxt, ops, npool, klen, vlen, LEN, secs);
    kv_async_start_jobs();

    printf("\n"); fflush(stdout);

    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(num_ctxt, ops, vlen, secs);

    printf("ASYNC: ");

    kv_async_wait_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_ASYNC_NPOOL_20)
{
    uint32_t num_ctxt = 1;
    uint32_t ops      = 20;
    uint32_t npool    = 20;
    uint32_t klen     = 16;
    uint32_t vlen     = 16;
    uint32_t LEN      = 500;
    uint32_t secs     = 15;

    TESTCASE_SKIP_IF_FILE;

    kv_async_init_perf_io(num_ctxt, ops, npool, klen, vlen, LEN, secs);
    kv_async_start_jobs();

    printf("\n"); fflush(stdout);

    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(num_ctxt, ops, vlen, secs);

    printf("ASYNC: ");

    kv_async_wait_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_ASYNC_MAGNUS_DIFFICULTUS)
{
    uint32_t num_ctxt = 20;
    uint32_t ops      = 50;
    uint32_t vlen     = KV_64K;
    uint32_t LEN      = 5;
    uint32_t secs     = 10;

    TESTCASE_SKIP_IF_FILE;

#ifdef _AIX
    if (dev == NULL)
    {
        printf("NOT_EXECUTED for memory on AIX\n");
        return;
    }
#endif

    printf("init async %dctxt/%djobs\n", num_ctxt, ops);
    kv_async_init_ctxt_io(num_ctxt, ops, 16, vlen, LEN, secs);

    printf("start async jobs\n");
    kv_async_start_jobs();

    printf("start %dctxt/%dpth sync jobs\n", num_ctxt, ops);
    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(num_ctxt, ops, vlen, secs);

    printf("wait for async jobs\n");
    kv_async_wait_jobs();
}
