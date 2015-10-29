/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_perf.C $                                  */
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
#include <fvt_kv_utils_async_cb.h>
#include <fvt_kv_utils_sync_pth.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, REDIS_PERF)
{
    ARK  *ark = NULL;

    ARK_CREATE;
    fvt_kv_utils_perf(ark, 16, 1, 1000);
    ARK_DELETE;

    kv_async_job_perf(100, 16, 16, 1000);

    kv_async_init_perf_io(100, 128, 4, 16, 16, 100, 10);
    printf("ctxts:100 jobs:128 npool=4 "); fflush(stdout);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_PERF)
{
    uint32_t ctxts = 1;
    uint32_t npool = 0;
    uint32_t secs  = 10;

    Sync_pth sync_pth;

    npool=4;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, 16,     50, secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, 16,     50, secs);
    printf("\n");

    npool=16;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, 16,     50, secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, 16,     50, secs);
    printf("\n");

    npool=24;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, 16,     50, secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, 16,     50, secs);
    printf("\n");

    npool=32;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, 16,     50, secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, 16,     50, secs);
    printf("\n");

    npool=64;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, 16,     50, secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, 16,     50, secs);
    printf("\n");

    npool=4;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_4K,  20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_4K,  20,  secs);
    printf("\n");

    npool=16;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_4K,  20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_4K,  20,  secs);
    printf("\n");

    npool=32;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_4K,  20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_4K,  20,  secs);
    printf("\n");

    npool=64;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_4K,  20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_4K,  20,  secs);
    printf("\n");

    npool=4;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_64K, 20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_64K, 20,  secs);
    printf("\n");

    npool=16;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_64K, 20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_64K, 20,  secs);
    printf("\n");

    npool=32;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_64K, 20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_64K, 20,  secs);
    printf("\n");

    npool=64;
    sync_pth.run_multi_ctxt(ctxts, 32, npool, KV_64K, 20,  secs);
    sync_pth.run_multi_ctxt(ctxts, 64, npool, KV_64K, 20,  secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_PERF)
{
    uint32_t ctxts = 1;
    uint32_t npool = 0;
    uint32_t jobs  = 100;
    uint32_t secs  = 10;

    npool=4;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=16;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=24;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=32;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=48;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=64;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
    printf("\n");

    npool=4;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_4K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=16;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_4K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=32;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_4K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=48;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_4K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=64;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_4K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
    printf("\n");

    npool=4;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_64K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=16;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_64K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=32;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_64K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=48;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_64K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();

    npool=64;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, KV_64K, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, MULTI_CTXT_PERF)
{
    uint32_t ctxts = 480;
    uint32_t npool = 0;
    uint32_t jobs  = 16;
    uint32_t pths  = 2;
    uint32_t secs  = 10;

    Sync_pth sync_pth;

    npool=4; pths=1;
    sync_pth.run_multi_ctxt(100,   pths, npool, 16,  100,  secs);
    pths=2;
    sync_pth.run_multi_ctxt(100,   pths, npool, 16,  100,  secs);
    sync_pth.run_multi_ctxt(300,   pths, npool, 16,  100,  secs);
    sync_pth.run_multi_ctxt(ctxts, pths, npool, 16,  100,  secs);
    npool=20;
    sync_pth.run_multi_ctxt(ctxts, pths, npool, 16,  100,  secs);

    npool=4; ctxts = 20;
    kv_async_init_perf_io(ctxts, 20, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:20 npool=%d ", ctxts, npool); fflush(stdout);
    kv_async_run_jobs();
    npool=4; ctxts = 100; jobs=4;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
    npool=4; ctxts = 480;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
    npool=20;
    kv_async_init_perf_io(ctxts, jobs, npool, 16, 16, 100, secs);
    printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, OLD_SYNC_PERF)
{
    ARK *ark = NULL;

    ARK_CREATE;
    fvt_kv_utils_perf(ark, 280,    4,    1000);
    fvt_kv_utils_perf(ark, KV_4K,  60,   1000);
    fvt_kv_utils_perf(ark, KV_64K, 200,  1000);
    fvt_kv_utils_perf(ark, KV_250K,600,  100);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, OLD_ASYNC_PERF)
{
    kv_async_job_perf(100, 16,    256,     1000);
    kv_async_job_perf(100, 16,    KV_4K,   1000);
    kv_async_job_perf(100, KV_4K, KV_4K,   1000);
    kv_async_job_perf(100, 16,    KV_64K,  1000);
}
