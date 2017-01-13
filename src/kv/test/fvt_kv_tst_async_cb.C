/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_async_cb.C $                           */
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
 *   utility functions to support async KV ops for the FVT
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_SINGLE_SGD_JOB)
{
    uint32_t klen  = 32;
    uint32_t vlen  = 128;
    uint32_t LEN   = 300;
    uint32_t secs  = 3;

    kv_async_init_ctxt   (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_SGD(ASYNC_SINGLE_CONTEXT,
                          ASYNC_SINGLE_JOB, klen, vlen, LEN);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_SINGLE_REP_JOB)
{
    uint32_t klen  = 22;
    uint32_t vlen  = 243;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    kv_async_init_ctxt   (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_REP(ASYNC_SINGLE_CONTEXT,
                          ASYNC_SINGLE_JOB, klen, vlen, LEN);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_EASY)
{
    uint32_t secs = 5;

    kv_async_init_ctxt    (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_easy(ASYNC_SINGLE_CONTEXT);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_BIG_BLOCKS)
{
    uint32_t secs = 5;

    kv_async_init_ctxt          (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_BIG_BLOCKS(ASYNC_SINGLE_CONTEXT);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_BIG_BLOCKS_STARVE)
{
    uint32_t secs = 5;

    kv_async_init_ctxt_starve   (ASYNC_SINGLE_CONTEXT, 20, 256, secs);
    kv_async_init_job_BIG_BLOCKS(ASYNC_SINGLE_CONTEXT);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_STRESS_LOW)
{
    uint32_t secs = 5;

    kv_async_init_ctxt          (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_low_stress(ASYNC_SINGLE_CONTEXT);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_STRESS_HIGH)
{
    uint32_t secs = 5;

    kv_async_init_ctxt           (ASYNC_SINGLE_CONTEXT, secs);
    kv_async_init_job_high_stress(ASYNC_SINGLE_CONTEXT);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_2_CONTEXT)
{
    uint32_t ctxt = 0;
    uint32_t secs = 5;

    TESTCASE_SKIP_IF_FILE;

    kv_async_init_ctxt          (ctxt, secs);
    kv_async_init_job_low_stress(ctxt++);
    kv_async_init_ctxt          (ctxt, secs);
    kv_async_init_job_low_stress(ctxt);
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, ASYNC_CB_10_CONTEXT)
{
    uint32_t ctxt = 0;
    uint32_t secs = 5;

    TESTCASE_SKIP_IF_FILE;

    for (ctxt=0; ctxt<10; ctxt++)
    {
        kv_async_init_ctxt    (ctxt, secs);
        kv_async_init_job_easy(ctxt);
    }
    kv_async_run_jobs();
}
