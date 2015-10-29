/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_persist.C $                            */
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
#include <fvt_kv.h>
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, PERSIST_FIXED_1kx1kx10000)
{
    ARK     *ark  = NULL;
    kv_t    *fdb  = NULL;
    uint32_t klen = 1024;
    uint32_t vlen = 1024;
    uint32_t LEN  = 10000;
    uint32_t i    = 0;
    int64_t  res  = 0;
    uint8_t  gvalue[vlen];

    if (NULL != getenv("FVT_DEV_PERSIST"))
    {
        printf("create ark\n");
        ARK_CREATE_NEW_PERSIST;

        fdb = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
        ASSERT_TRUE(fdb != NULL);

        printf("load ark\n");
        fvt_kv_utils_load (ark, fdb, LEN);
        printf("query db\n");
        fvt_kv_utils_query(ark, fdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_PERSIST;

        printf("re-opened ark, query db\n");
        fvt_kv_utils_query(ark, fdb, vlen, LEN);
        printf("delete db from ark\n");
        fvt_kv_utils_del(ark, fdb, LEN);

        printf("load ark with the same db\n");
        fvt_kv_utils_load (ark, fdb, LEN);
        printf("query db\n");
        fvt_kv_utils_query(ark, fdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_NEW_PERSIST;

        printf("re-open ark as new, verify empty\n");

        for (i=0; i<LEN; i++)
        {
            ASSERT_EQ(ENOENT, ark_get(ark,
                                      fdb[i].klen,
                                      fdb[i].key,
                                      fdb[i].vlen,
                                      gvalue,
                                      0,
                                      &res));
        }

        kv_db_destroy(fdb, LEN);
        ARK_DELETE;
    }
    else
    {
        printf("SKIPPING, FVT_DEV_PERSIST==NULL \n");
    }
}


/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, PERSIST)
{
    ARK     *ark    = NULL;
    kv_t    *fdb    = NULL;
    kv_t    *mdb    = NULL;
    uint32_t klen   = 100;
    uint32_t vlen   = 1024;
    uint32_t LEN    = 10;
    uint32_t i      = 0;
    int64_t  res    = 0;
    uint8_t  gvalue[vlen];

    if (NULL != getenv("FVT_DEV_PERSIST"))
    {
        printf("create ark, load fixed db\n");
        ARK_CREATE_NEW_PERSIST;

        fdb = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
        ASSERT_TRUE(fdb != NULL);
        mdb = (kv_t*)kv_db_create_mixed(LEN, klen, vlen);
        ASSERT_TRUE(mdb != NULL);

        printf("run SGD_LOOP on fixed db\n");
        fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen+10, vlen+10, LEN, 5);

        printf("load ark with fixed db, query fixed db\n");
        fvt_kv_utils_load (ark, fdb, LEN);
        fvt_kv_utils_query(ark, fdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_PERSIST;

        printf("re-opened ark, query fixed db\n");
        fvt_kv_utils_query(ark, fdb, vlen, LEN);
        printf("delete fixed db from ark\n");
        fvt_kv_utils_del(ark, fdb, LEN);

        printf("run SGD_LOOP on mixed db\n");
        fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen+20, vlen+20, LEN, 5);

        printf("load ark with mixed db\n");
        fvt_kv_utils_load (ark, mdb, LEN);
        printf("query mixed db\n");
        fvt_kv_utils_query(ark, mdb, vlen, LEN);
        printf("delete mixed db from ark\n");
        fvt_kv_utils_del(ark, mdb, LEN);

        printf("load ark with the mixed db\n");
        fvt_kv_utils_load (ark, mdb, LEN);
        printf("query mixed db\n");
        fvt_kv_utils_query(ark, mdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_PERSIST;

        printf("re-opened ark, query mixed db\n");
        fvt_kv_utils_query(ark, mdb, vlen, LEN);
        printf("delete mixed db from ark\n");
        fvt_kv_utils_del(ark, mdb, LEN);

        printf("run REP_LOOP on mixed db\n");
        fvt_kv_utils_REP_LOOP(ark,
                              kv_db_create_mixed,
                              kv_db_mixed_regen_values,
                              klen,
                              vlen,
                              LEN,
                              5);

        printf("load ark with the mixed db\n");
        fvt_kv_utils_load (ark, mdb, LEN);
        printf("query mixed db\n");
        fvt_kv_utils_query(ark, mdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_NEW_PERSIST;

        printf("re-open ark as new, verify empty\n");

        for (i=0; i<LEN; i++)
        {
            ASSERT_EQ(ENOENT, ark_get(ark,
                                      mdb[i].klen,
                                      mdb[i].key,
                                      mdb[i].vlen,
                                      gvalue,
                                      0,
                                      &res));
        }

        ARK_DELETE;

        printf("re-open ark as readonly\n");

        ARK_CREATE_PERSIST_READONLY;

        printf("load ark with the mixed db\n");
        fvt_kv_utils_load (ark, mdb, LEN);
        printf("query mixed db\n");
        fvt_kv_utils_query(ark, mdb, vlen, LEN);

        ARK_DELETE;

        ARK_CREATE_PERSIST;

        printf("re-open ark, verify empty\n");

        for (i=0; i<LEN; i++)
        {
            ASSERT_EQ(ENOENT, ark_get(ark,
                                      mdb[i].klen,
                                      mdb[i].key,
                                      mdb[i].vlen,
                                      gvalue,
                                      0,
                                      &res));
        }

        ARK_DELETE;

        kv_db_destroy(fdb, LEN);
        kv_db_destroy(mdb, LEN);
    }
    else
    {
        printf("SKIPPING, FVT_DEV_PERSIST==NULL \n");
    }
}
