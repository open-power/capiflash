/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_iv.C $                                 */
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
int tst_iv_entry(int argc, char **argv);
}

TEST(FVT_KV_GOOD_PATH, tst_iv_default1)
{
    char *argv_parms[] =
    {
            (char*)"tst_iv_entry",
    };
    EXPECT_EQ(0, tst_iv_entry(1,argv_parms));
}

TEST(FVT_KV_GOOD_PATH, tst_iv_default0)
{
    char *argv_parms[] =
    {
            (char*)"tst_iv_entry",
            (char*)"-t",
            (char*)"0",
    };
    EXPECT_EQ(0, tst_iv_entry(3,argv_parms));
}

TEST(FVT_KV_GOOD_PATH, _tst_iv_big)
{
    char *argv_parms[] =
    {
            (char*)"tst_iv_entry",
            (char*)"-n",
            (char*)"128",
            (char*)"-m",
            (char*)"512",
            (char*)"-i",
            (char*)"50000",
            (char*)"-s",
            (char*)"675432",
    };
    EXPECT_EQ(0, tst_iv_entry(9,argv_parms));
}
