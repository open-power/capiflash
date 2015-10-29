/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_perf2.C $                                 */
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
TEST(FVT_KV_GOOD_PATH, PERF_16x16)
{
    uint32_t ctxts = 1;
    uint32_t mb    = 1;
    uint32_t klen  = 16;
    uint32_t vlen  = 16;
    uint32_t LEN   = 500;
    uint32_t secs  = 10;
    uint32_t jobs  = 1;
    uint32_t ops   = 0;
    uint32_t ios   = 0;
    ARK     *ark   = NULL;

    /* single context single job 16x16 */
    Sync_pth sync_pth;
    sync_pth.run_multi_ctxt(ctxts, jobs, vlen, LEN, secs, &ops, &ios);

    ARK_CREATE;
    fvt_kv_utils_perf(ark, vlen, mb, LEN);

    kv_async_job_perf(jobs, klen, vlen, LEN);

    kv_async_init_ctxt_io(ctxts, jobs, klen, vlen, LEN, 10);
    printf("ctxt:%d jobs:%d ", ctxts, jobs);
    kv_async_run_jobs();

    printf("---------\n");

    /* single context 20 job 16x16 */
    jobs = 20;

    sync_pth.run_multi_ctxt(ctxts, jobs, vlen, LEN, secs, &ops, &ios);

    kv_async_job_perf(jobs, klen, vlen, LEN);

    kv_async_init_ctxt_io(ctxts, jobs, klen, vlen, LEN, 10);
    printf("ctxt:%d jobs:%d ", ctxts, jobs);
    kv_async_run_jobs();

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SYNC_PERF)
{
    uint32_t LEN = 500;
    uint32_t ops = 0;
    uint32_t ios = 0;
    ARK     *ark = NULL;

    ARK_CREATE;
    fvt_kv_utils_perf(ark, 280,    4,   LEN);
    fvt_kv_utils_perf(ark, KV_64K, 200, 50);
    ARK_DELETE;

    printf("---------\n");

    /* single context single job */

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(1, 1, 280,    LEN, 10, &ops, &ios);
    sync_pth.run_multi_ctxt(1, 1, KV_64K, 10,  10, &ops, &ios);

    /* single context 20 job */

    sync_pth.run_multi_ctxt(1, 20, 280,    LEN, 10, &ops, &ios);
    sync_pth.run_multi_ctxt(1, 20, KV_64K, 10,  10, &ops, &ios);

    printf("---------\n");

    /* 50 context 20 job */

    sync_pth.run_multi_ctxt(50, 20, 280,    LEN, 10, &ops, &ios);
    sync_pth.run_multi_ctxt(50, 20, KV_64K, 10,  10, &ops, &ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_PERF)
{
    uint32_t ctxts = 1;
    uint32_t jobs  = 20;
    uint32_t klen  = 16;
    uint32_t vlen  = 16;
    uint32_t LEN   = 500;

    kv_async_job_perf(jobs, klen,  280,     LEN);
    kv_async_job_perf(jobs, KV_4K, KV_4K,   100);
    kv_async_job_perf(jobs, klen,  KV_64K,  50);

    kv_async_init_ctxt_io(ctxts, jobs, klen, vlen, LEN, 10);
    printf("ctxt:%d jobs:%d ", ctxts, jobs);
    kv_async_run_jobs();

    kv_async_init_ctxt_io(ctxts, jobs, klen, KV_64K, 50, 10);
    printf("ctxt:%d jobs:%d ", ctxts, jobs);
    kv_async_run_jobs();

    printf("---------\n");

    /* 50 context 20 job */
    ctxts = 50;
    kv_async_init_ctxt_io(ctxts, jobs, klen, vlen, LEN, 10);
    printf("ctxt:%d jobs:%d ", ctxts, jobs);
    kv_async_run_jobs();
}
