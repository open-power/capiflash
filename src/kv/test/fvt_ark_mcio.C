/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_mcio.C $                                  */
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
#include <iostream>

extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
#include <fvt_kv_utils_sync_pth.h>
#include <fvt_kv_utils_async_cb.h>
}

using namespace std;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ARK_IO, SYNC_PTH_MULTI_CONTEXT)
{
    uint32_t num_ctxt = 1;
    uint32_t num_pth  = 32;
    uint32_t vlen     = KV_500K;
    uint32_t LEN      = 1;
    uint32_t xmin     = 1;
    char    *env_min  = getenv("FVT_ARK_IO_MIN");

    if (env_min) xmin = atoi(env_min);

    cout << "\nRunning for " << xmin;
    cout << " minutes. How many contexts? ";
    cin >> num_ctxt;

    if (num_ctxt <= 0 || num_ctxt > 508)
    {
        printf("bad context num %d\n", num_ctxt);
        return;
    }
    if      (num_ctxt < 10) num_pth = 32;
    else if (num_ctxt < 32) num_pth = 8;
    else                    num_pth = 2;

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, xmin*60);
}
