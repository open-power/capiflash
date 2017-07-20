/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/fvt_utils_scripts_tst.C $                             */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2017                        */
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
 *   Simple test cases for utility scripts
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>
#include <stdlib.h>

#define SCRIPT_PATH "/usr/bin/utils_scripts_tst"
#define STDOUT_REDIRECTION "1>/dev/null "

#define CHECK_TESTCASE_SKIP                                                    \
 do                                                                            \
 {                                                                             \
     FILE *file_p = fopen(SCRIPT_PATH ,"r");                                   \
                                                                               \
     if (!file_p) {printf("[  SKIPPED ] %s not found\n", SCRIPT_PATH); return;}\
     else         {fclose(file_p);}                                            \
                                                                               \
     char *dev = getenv("FVT_DEV");                                            \
     if (!dev || (strncmp("/dev/", dev, 5)))                                   \
     {                                                                         \
         printf("[  SKIPPED ] FVT_DEV is invalid\n");                          \
         return;                                                               \
     }                                                                         \
 }                                                                             \
 while (0);

/**
 ****************************************************************
 * \brief
 *   Run script: cxlfstatus return code test (rc == 0 )
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE,CXLFSTATUS_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFSTATUS_RC_TEST "); /*test name in the script*/
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cxlfstatus expect no ERROR strings found
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLFSTATUS_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFSTATUS_ERRSTR_TEST "); /*test name in the script*/
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_version expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLVER_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLVER_RC_TEST "); /*test name in the script*/
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_version expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLVER_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLVER_ERRSTR_TEST "); /*test name in the script*/
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_reset expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_RESET_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_RESET_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_reset expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_RESET_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_RESET_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_perst expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_PERST_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_PERST_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_perst expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_PERST_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_PERST_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: capi_flash that listing devices rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_RC_TEST "); /*test name in the script*/
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: capi_flash find at least one CAPI device
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_FIND_CAPI_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_FIND_CAPI_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: capi_flash usage test no errors
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_USAGE_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_USAGE_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cxlffdc expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLFFDC_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFFDC_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cxlffdc expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLFFDC_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFFDC_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cxlfrefreshluns expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLREFRESH_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLREFRESH_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cxlfrefreshluns expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLREFRESH_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLREFRESH_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_perfcheck expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_PERFCHK_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_PERFCHK_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_perfcheck expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_PERFCHK_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_PERFCHK_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_capacity expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_CAP_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_CAP_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_capacity expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_CAP_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_CAP_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_devices expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_DEV_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_DEV_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_devices find some sg devices
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_DEV_FIND_SG_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_DEV_FIND_SG_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_temp expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_TEMP_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_TEMP_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}
/**
****************************************************************
 * \brief
 *  Run script: cflash_temp find a device
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_TEMP_FIND_DEV)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_TEMP_FIND_DEV ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: machine_info expect rc == 0
 ***************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, MACHINE_INFO_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t MACHINE_INFO_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: machine_info expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, MACHINE_INFO_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t MACHINE_INFO_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_wear expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_WEAR_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_WEAR_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_wear expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_WEAR_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_WEAR_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_wear expect at least one device
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_WEAR_FOUND_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_WEAR_FOUND_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cxlfsetlunmode expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLFSETLUNMODE_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFSETLUNMODE_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cxlfsetlunmode expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CXLFSETLUNMODE_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CXLFSETLUNMODE_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *  Run script: cflash_stick expect rc == 0
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_STICK_RC_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_STILST_RC_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}

/**
 ****************************************************************
 * \brief
 *   Run script: cflash_stick expect no ERROR string
 ****************************************************************/
TEST(UTIL_SCRIPTS_FVT_SUITE, CFLASH_STICK_ERRSTR_TEST)
{
   int rc = 0;
   std::string cmd= SCRIPT_PATH;

   CHECK_TESTCASE_SKIP;

   cmd.append(" -t CFLASH_STILST_ERRSTR_TEST ");
   cmd.append(STDOUT_REDIRECTION);

   rc = system (cmd.c_str());
   EXPECT_EQ(0,rc);
}
