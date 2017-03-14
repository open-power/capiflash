/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_fixme1.C $                                 */
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
#include <gtest/gtest.h>

extern "C"
{
#include <kv_inject.h>
#include <fvt_kv.h>
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
#include <fvt_kv_utils_ark_io.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, RANDOM_ERRORS)
{
    uint32_t ctxts = 1;
    uint32_t vlen  = KV_1M;
    uint32_t secs  = 300;

    kv_async_init_ark_io_inject(ctxts, 128, vlen, secs);
    kv_async_start_jobs();

    printf("\n"); fflush(stdout);

    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(ctxts, 20, vlen, secs, 0);

    kv_async_wait_jobs();
}
