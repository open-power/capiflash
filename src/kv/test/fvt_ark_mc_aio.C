/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_mc_aio.C $                                */
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
 *   Run multi-context async I/O
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>
#include <iostream>

extern "C"
{
#include <fvt_kv.h>
#include <fvt_kv_utils_async_cb.h>
}

using namespace std;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ARK_IO, ASYNC_CB_MULTI_CONTEXT)
{
    uint32_t ctxts   = 0;
    uint32_t jobs    = 0;
    uint32_t xmin    = 1;
    char    *env_min = getenv("FVT_ARK_IO_MIN");

    if (env_min) xmin = atoi(env_min);

    cout << "\nRunning for " << xmin << " minutes. How many contexts? ";
    cin >> ctxts;

    if (ctxts <= 0 || ctxts > 508)
    {
        printf("bad context num %d\n", ctxts);
        return;
    }

    if      (ctxts < 5)   jobs = 40;
    else if (ctxts < 100) jobs = 20;
    else                  jobs = 4;

    kv_async_init_ctxt_io(ctxts, jobs, 16, KV_500K, 1, xmin*60);
    printf("ctxts:%d jobs:%d ", ctxts, jobs); fflush(stdout);
    kv_async_run_jobs();
}
