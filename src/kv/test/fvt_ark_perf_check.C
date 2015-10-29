/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_perf_check.C $                            */
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
TEST(FVT_KV_GOOD_PATH, COMMIT_PERF_CHECK)
{
    uint32_t ctxts = 1;
    uint32_t jobs  = 128;
    uint32_t pths  = 100;
    uint32_t secs  = 15;

    Sync_pth sync_pth;

    printf("\nctxts=1, 16x16\n");
    sync_pth.run_multi_ctxt(ctxts, pths, 16, 500, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, 16, 500, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();

    printf("\nctxts=1, 16x4k\n");
    sync_pth.run_multi_ctxt(ctxts, pths, KV_4K, 500, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, KV_4K, 500, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();

    printf("\nctxts=1, 16x64k\n");

    sync_pth.run_multi_ctxt(ctxts, pths, KV_64K, 1, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, KV_64K, 1, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();

    ctxts=8; secs=25;

    printf("\nctxts=%d, 16x16\n", ctxts);

    sync_pth.run_multi_ctxt(ctxts, pths, 16, 500, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, 16, 500, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();

    printf("\nctxts=%d, 16x64k\n", ctxts);

    sync_pth.run_multi_ctxt(ctxts, pths, KV_64K, 1, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, KV_64K, 1, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();

    ctxts=40; jobs=20; pths=20;

    printf("\nctxts=%d, 16x16\n", ctxts);

    sync_pth.run_multi_ctxt(ctxts, pths, 16, 500, secs);

    kv_async_init_ctxt_io(ctxts, jobs, 16, 16, 500, secs);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();
}
