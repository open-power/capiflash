/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_bins.C $                               */
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
#include <stdlib.h>
char *env_root_path = getenv("SURELOCKROOT");
char cmd[1024]      = {0};
}

// disabling this test because it is broken and we may not fix it
TEST(FVT_KV_GOOD_PATH, BIN_tst_ark)
{
    if (env_root_path)
        sprintf(cmd, "%s/obj/tests/fvt_kv_tst_ark 2>&1 >/dev/null", env_root_path);
    else
        sprintf(cmd, "fvt_kv_tst_ark 2>&1 >/dev/null");
    EXPECT_EQ(0, system(cmd));
}

