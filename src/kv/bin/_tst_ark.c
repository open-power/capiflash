/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/_tst_ark.c $                                      */
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

#include <stdlib.h>
#include <stdio.h>

int tst_ark_entry(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
      printf("usage: %s <path-to-afu>\n",argv[0]);
      exit(0);
    }

    return tst_ark_entry(argc, argv);
}
