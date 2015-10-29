/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/_tst_kv_utils.c $                                 */
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
 *   simple unit test driver for tst_kv_utils.c
 * \ingroup
 ******************************************************************************/
#include <fvt_trace.h>
#include <kv_utils_db.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int32_t test_db(uint32_t type, uint32_t klen, uint32_t vlen, uint32_t num)
{
    uint32_t i  = 0;
    uint32_t rc = 0;
    kv_t    *db = NULL;

    if (type)
    {
        db = (kv_t*)kv_db_create_fixed(num, klen, vlen);
    }
    else
    {
        db = (kv_t*)kv_db_create_mixed(num, klen, vlen);
    }

    assert(NULL != db);

    for (i=0; i<num; i++)
    {
        if (NULL == kv_db_find(db, num, db[i].key, db[i].klen))
        {
            printf(": not found in db: %d %d\n", i, num);
            rc = -1;
            break;
        }
    }

    kv_db_destroy(db, num);

    return rc;
}

#define NUMV 24

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int main(void)
{
    uint32_t i =0;
    uint32_t rc=0;

    KV_TRC_OPEN(pFT, "fvt");
    FVT_TRC_SIGINT_HANDLER;

    uint32_t variations[NUMV][4] = {
            {1, 1,    0,    100},
            {1, 1,    1,    100},
            {1, 1,    2,    100},
            {1, 2,    2,    100},
            {1, 2,    1,    100},
            {1, 7,    1,    400},
            {1, 10,   50,   1000},
            {1, 16,   128,  2000},
            {1, 64,   128,  4000},
            {1, 128,  4,    6000},
            {1, 256,  4,    10000},
            {1, 1024, 4,    10000},
            {0, 1,    0,    100},
            {0, 1,    1,    100},
            {0, 1,    2,    100},
            {0, 2,    2,    100},
            {0, 2,    1,    100},
            {0, 7,    1,    400},
            {0, 10,   50,   1000},
            {0, 16,   128,  2000},
            {0, 64,   128,  4000},
            {0, 128,  4,    6000},
            {0, 256,  4,    10000},
            {0, 1024, 4,    10000}
    };

    for (i=0; i<NUMV; i++)
    {
        printf("Running %d %d %d %d\n",
                variations[i][0],
                variations[i][1],
                variations[i][2],
                variations[i][3]);

        if (test_db(variations[i][0],
                    variations[i][1],
                    variations[i][2],
                    variations[i][3]))
        {
            printf(">>>>>FAILED\n");
            rc = -1;
            goto exception;
        }
    }

exception:
    KV_TRC_CLOSE(pFT);
    return rc;
}
