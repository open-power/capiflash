/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/unit_test-kv.C $                                     */
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
#include <tst_bl.h>
}

//it is the convention to write the test (case) names without quotes.
//Compilation error occurs if quotes are used around the strings 
//ie: TEST("System_Test_Kv", kv_tst_bv) or TEST(System_Test_Kv, "kv_tst_bv") //INVALID 
TEST(System_Test_Kv, kv_tst_bl)
{
/*
Function prototype for tst_bl: 
tst_bl(int iter, int bcnt,int ccnt,int cmax, int grow, int hold, int resz, int seed, int prnt, int dprt, int w);
*/
//Array used to store numerous lists of parameters
 int array[2][11]={{0,8,16,6,8,0,0,1234,0,0,34},
                   {0,8,16,6,8,0,0,100,0,0,25}};
//Usage of matrix to process all the information
  for(int i=0;i<2;i++)
  {
      EXPECT_EQ(0, tst_bl(array[i][0],array[i][1],array[i][2],array[i][3],array[i][4],array[i][5],array[i][6],array[i][7],array[i][8],array[i][9],array[i][10]));
      //If the return value of tst_bl is non valid for  **ANY** of the rows of the matrice then the test case fails.
  }

}
TEST(System_Test_Kv, kv_tst_bv)
{

  EXPECT_EQ(0, system("../../obj/tests/_tst_bv"));

}
TEST(System_Test_Kv, kv_tst_ht)
{

  EXPECT_EQ(0, system("../../obj/tests/_tst_ht"));

}
TEST(System_Test_Kv, kv_tst_vi)
{
  
  EXPECT_EQ(0, system("../../obj/tests/_tst_vi"));

}
TEST(System_Test_Kv, kv_tst_ark)
{

  EXPECT_EQ(0,system("../../obj/tests/_tst_ark"));

}
TEST(System_Test_Kv, kv_tst_bt)
{

  EXPECT_EQ(0, system("../../obj/tests/_tst_bt"));

}
TEST(System_Test_Kv, transport_test)
{

  EXPECT_EQ(0, system("../../obj/tests/transport_test"));

}
TEST(System_Test_Kv, kv_tst_iv)
{

  EXPECT_EQ(0, system("../../obj/tests/_tst_iv"));

}
    	 
