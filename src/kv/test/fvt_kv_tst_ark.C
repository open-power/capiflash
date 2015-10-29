/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_ark.C $                                */
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
#include <stdlib.h>

extern "C"
{
int tst_ark_entry(int argc, char **argv);
}

TEST(FVT_KV_GOOD_PATH, _tst_ark_default)
{
    char  dev_parm[]    = "-dev";
    char *env_FVT_DEV   = getenv("FVT_DEV");
    char *argv_parms[3] =
    {
            (char*)"tst_ark_entry",
    };

    EXPECT_EQ(0, tst_ark_entry(1,argv_parms));

    if (NULL != env_FVT_DEV)
    {
        argv_parms[1] = dev_parm;
        argv_parms[2] = env_FVT_DEV;
        EXPECT_EQ(0, tst_ark_entry(3,argv_parms));
    }
}

TEST(FVT_KV_GOOD_PATH, _tst_ark_1000_keys_100_iterations)
{
    char  dev_parm[]    = "-dev";
    char *env_FVT_DEV   = getenv("FVT_DEV");
    char *argv_parms[7] =
    {
            (char*)"tst_ark_entry",
            (char*)"-n",
            (char*)"100",
            (char*)"-k",
            (char*)"1000",
    };
    EXPECT_EQ(0, tst_ark_entry(5,argv_parms));

    if (NULL != env_FVT_DEV)
    {
        argv_parms[5] = dev_parm;
        argv_parms[6] = env_FVT_DEV;
        EXPECT_EQ(0, tst_ark_entry(7,argv_parms));
    }
}
