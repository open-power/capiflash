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
    uint32_t ctxt     = 1;
    uint32_t pth      = 32;
    uint32_t vlen     = 300;
    uint32_t len      = 500;
    uint32_t secs     = 10;
    char    *env_ctxt = getenv("FVT_MCIO_CTXT");
    char    *env_pth  = getenv("FVT_MCIO_PTH");
    char    *env_vlen = getenv("FVT_MCIO_VLEN");
    char    *env_len  = getenv("FVT_MCIO_LEN");
    char    *env_secs = getenv("FVT_MCIO_SEC");

    if (env_ctxt) ctxt = atoi(env_ctxt);
    if (env_pth)  pth  = atoi(env_pth);
    if (env_vlen) vlen = atoi(env_vlen);
    if (env_len)  len  = atoi(env_len);
    if (env_secs) secs = atoi(env_secs);

    if (ctxt <= 0 || ctxt > 508)
    {
        printf("bad context num %d\n", ctxt);
        return;
    }

    Sync_pth sync_pth;

    sync_pth.run_multi_ctxt(ctxt, pth, vlen, len, secs);
}
