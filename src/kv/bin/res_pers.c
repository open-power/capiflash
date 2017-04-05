/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/res_pers.c $                                      */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arkdb.h"

int main(int argc, char **argv) {
        int rc = 0;
        ARK *ark;
        char *key;
        char buf[1024];
        int64_t resultSize;

        printf("Starting\n");
        rc = ark_create_verbose("/tmp/peter.kv", &ark, 1048576, 4096, 4096, 20, 0, 256, ARK_KV_VIRTUAL_LUN | ARK_KV_PERSIST_STORE | ARK_KV_PERSIST_LOAD);
//      rc = ark_create("/tmp/peter/capi", &ark, ARK_KV_VIRTUAL_LUN);

        if (rc != 0) {
                printf("ark_create failed %d\n", rc);
                exit(1);
        }
 //       printf("ark_create %x\n", ark);
        key = "mykey";
        rc = ark_get(ark, (uint64_t)strlen(key), key, (uint64_t)sizeof(buf), &buf, 0, &resultSize);
        if (rc != 0 && rc != 2) {
                printf("ark_get 1 failed %d\n", rc);
                exit(1);
        }
        if (0 == rc) {
                printf("ark_get 1 resultSize=%ld\n", resultSize);
                if (resultSize != 0) {
//                        printf("result='%*s'\n", resultSize, buf);
                        printf("result='%s'\n", buf);
                }
                exit(0);
        }
        strcpy(buf, "myvalue");
        rc = ark_set(ark, (uint64_t)strlen(key), key, (uint64_t)strlen(buf), buf, &resultSize);
        if (rc != 0) {
                printf("ark_set 1 failed %d\n", rc);
                exit(1);
        }
        printf("ark_set written %ld\n", resultSize);
        rc = ark_get(ark, (uint64_t)strlen(key), key, (uint64_t)sizeof(buf), &buf, 0, &resultSize);
        printf("arg_get 2 returned %d\n", rc);
        if (rc != 0 && rc != 2) {
                printf("ark_get 2 failed %d\n", rc);
                exit(1);
        }
        if (0 == rc) {
                printf("ark_get 2 resultSize=%ld\n", resultSize);
                if (resultSize != 0) {
//                        printf("result='%*s'\n", resultSize, buf);
                        printf("result='%s'\n", buf);
                }
        }
        ark_delete(ark);

	return 0;
}
