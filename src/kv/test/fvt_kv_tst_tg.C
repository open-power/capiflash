/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_tg.C $                                 */
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
int tst_tg_entry(int argc, char **argv);
}

TEST(FVT_KV_GOOD_PATH, kv_tst_tg_default)
{
    char *argv_parms[] =
    {
            (char*)"tst_tg_entry",
    };
    EXPECT_EQ(0, tst_tg_entry(1,argv_parms));
}

TEST(FVT_KV_GOOD_PATH, kv_tst_tg_big)
{
    char *argv_parms[] =
    {
            (char*)"tst_tg_entry",
            (char*)"-n",
            (char*)"100",
            (char*)"-c",
            (char*)"10",
            (char*)"-p",
            (char*)"15",
    };
    EXPECT_EQ(0, tst_tg_entry(7,argv_parms));
}
