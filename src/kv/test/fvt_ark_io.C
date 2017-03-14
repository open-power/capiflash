/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_io.C $                                    */
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
#include <fvt_kv.h>
#include <fvt_kv_utils_async_cb.h>
#include <fvt_kv_utils_ark_io.h>
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_ARK_IO, ASYNC_CB_ARK_IO)
{
    uint32_t ctxts = 4;
    uint32_t jobs  = 20;
    uint32_t pths  = 20;
    uint32_t vlen  = KV_64K;
    uint32_t xmin  = 1;
    char    *env_min = getenv("FVT_ARK_IO_MIN");

    if (env_min) xmin = atoi(env_min);
    printf("running for mins:%d\n", xmin); fflush(stdout);

    kv_async_init_ark_io(ctxts, jobs, vlen, xmin*60);
    kv_async_start_jobs();

    printf("\n"); fflush(stdout);

    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(ctxts, pths, vlen, xmin*60, 0);

    printf("ASYNC: ");

    kv_async_wait_jobs();
}
