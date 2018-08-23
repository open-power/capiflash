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
#include <cflash_test_utils.h>
#include <errno.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, PERSIST_100000_KV)
{
    ARK     *ark  = NULL;
    kv_t    *fdb  = NULL;
    kv_t    *mdb  = NULL;
    uint32_t klen = 512;
    uint32_t vlen = 128;
    uint32_t LEN  = 50000;
    uint32_t i    = 0;
    int64_t  res  = 0;
    char    *dev  = getenv("FVT_DEV_PERSIST");
    uint8_t  gvalue[vlen];
    struct stat sbuf;

    if (NULL == dev ||
        (!(strncmp(dev,"/dev",4)==0 ||
           strncmp(dev,"RAID",4)==0 ||
           stat(dev,&sbuf) == 0))
       )
    {
        TESTCASE_SKIP("FVT_DEV_PERSIST==NULL or file not found");
        return;
    }

    printf("create k/v databases\n"); fflush(stdout);
    fdb = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(fdb != NULL);
    mdb = (kv_t*)kv_db_create_mixed(LEN, klen-1, vlen);
    ASSERT_TRUE(mdb != NULL);

    printf("create ark\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("run 5 sec mixed REP_LOOP\n"); fflush(stdout);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen+1,
                          vlen,
                          1000,
                          5);

    printf("load ark with fixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, fdb, LEN);
    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("persist ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark and read persisted data\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("load ark with the mixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, mdb, LEN);
    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, vlen, LEN);

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);
    printf("delete fixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, fdb, LEN);

    printf("re-load ark with the fixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, fdb, LEN);
    printf("query db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("delete mixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, mdb, LEN);

    printf("persist ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark without LOAD\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("verify ark is empty\n"); fflush(stdout);

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
    kv_db_destroy(mdb, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, PERSIST_COMPLEX)
{
    ARK     *ark    = NULL;
    kv_t    *fdb    = NULL;
    kv_t    *mdb    = NULL;
    kv_t    *bdb    = NULL;
    uint32_t klen   = 377;
    uint32_t vlen   = 137;
    uint32_t LEN    = 10000;
    uint32_t BLEN   = 1000;
    uint32_t i      = 0;
    int64_t  res    = 0;
    char    *dev    = getenv("FVT_DEV_PERSIST");
    uint8_t  gvalue[vlen];
    struct stat sbuf;

    if (NULL == dev ||
        (!(strncmp(dev,"/dev",4)==0 ||
           strncmp(dev,"RAID",4)==0 ||
           stat(dev,&sbuf) == 0))
       )
    {
        TESTCASE_SKIP("FVT_DEV_PERSIST==NULL or file not found");
        return;
    }

    printf("create k/v databases\n"); fflush(stdout);
    fdb = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(fdb != NULL);
    bdb = (kv_t*)kv_db_create_fixed(BLEN, klen+1, KV_250K);
    ASSERT_TRUE(bdb != NULL);
    mdb = (kv_t*)kv_db_create_mixed(LEN, klen-1, vlen);
    ASSERT_TRUE(mdb != NULL);

    printf("create new ark\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open empty ark\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("load ark with fixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, fdb, LEN);
    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark containing fixed db\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("run SGD_LOOP on fixed db\n"); fflush(stdout);
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen+10, vlen+10,1000,5);

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark containing fixed db\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("load ark with BIG db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, bdb, BLEN);
    printf("query BIG db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, bdb, KV_250K, BLEN);

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);

    printf("run REP_LOOP on fixed db\n"); fflush(stdout);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_fixed_regen_values,
                          klen+100,
                          vlen,
                          5000,
                          5);

    printf("delete BIG db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, bdb, BLEN);

    printf("load ark with the mixed db\n"); fflush(stdout);
    fvt_kv_utils_load(ark, mdb, LEN);
    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, vlen, LEN);

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark with fixed and mixed db\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, vlen, LEN);
    printf("delete fixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, fdb, LEN);

    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, vlen, LEN);
    printf("delete mixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, mdb, LEN);

    printf("run REP_LOOP on mixed db\n"); fflush(stdout);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen+256,
                          vlen,
                          100,
                          5);

    printf("load ark with the mixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, mdb, LEN);
    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, vlen, LEN);

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("open new ark\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("verify empty\n"); fflush(stdout);
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

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark as readonly\n"); fflush(stdout);
    ARK_CREATE_PERSIST_READONLY;

    printf("load ark with the mixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, mdb, LEN);
    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, vlen, LEN);

    printf("run REP_LOOP on mixed db\n"); fflush(stdout);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          5);

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("open new ark\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("verify empty\n"); fflush(stdout);
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

    printf("delete ark\n"); fflush(stdout);
    ARK_DELETE;

    kv_db_destroy(fdb, LEN);
    kv_db_destroy(mdb, LEN);
    kv_db_destroy(bdb, BLEN);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, PERSIST_EEH)
{
    ARK     *ark   = NULL;
    kv_t    *fdb   = NULL;
    kv_t    *mdb   = NULL;
    uint32_t fklen = 5000;
    uint32_t fvlen = 257;
    uint32_t klen  = 512;
    uint32_t fLEN  = 5000;
    uint32_t vlen  = 128;
    uint32_t LEN   = 50000;
    uint32_t i     = 0;
    int64_t  res   = 0;
    char    *dev   = getenv("FVT_DEV_PERSIST");
    uint8_t  gvalue[fvlen];
    struct stat sbuf;

    if (NULL == dev ||
        (!(strncmp(dev,"/dev",4)==0 ||
           strncmp(dev,"RAID",4)==0 ||
           stat(dev,&sbuf) == 0))
       )
    {
        TESTCASE_SKIP("FVT_DEV_PERSIST==NULL or file not found");
        return;
    }

    TESTCASE_SKIP_IF_NO_EEH;

    printf("create k/v databases\n"); fflush(stdout);
    fdb = (kv_t*)kv_db_create_fixed(fLEN, fklen, fvlen);
    ASSERT_TRUE(fdb != NULL);
    mdb = (kv_t*)kv_db_create_mixed(LEN, klen-1, vlen);
    ASSERT_TRUE(mdb != NULL);

    printf("create ark\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("run 5 sec mixed REP_LOOP\n"); fflush(stdout);
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          fklen+1,
                          fvlen,
                          1000,
                          5);

    printf("load ark with fixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, fdb, fLEN);
    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, fvlen, fLEN);

    printf("load ark with the mixed db\n"); fflush(stdout);
    fvt_kv_utils_load (ark, mdb, LEN);
    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, fvlen, LEN);

    printf("persist ark\n"); fflush(stdout);
    ARK_DELETE;

    inject_EEH(dev); sleep(3);

    printf("re-open ark and read persisted data\n"); fflush(stdout);
    ARK_CREATE_PERSIST;

    printf("query fixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, fdb, fvlen, fLEN);

    printf("query mixed db\n"); fflush(stdout);
    fvt_kv_utils_query(ark, mdb, fvlen, LEN);

    printf("delete fixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, fdb, fLEN);

    printf("delete mixed db from ark\n"); fflush(stdout);
    fvt_kv_utils_del(ark, mdb, LEN);

    printf("persist ark\n"); fflush(stdout);
    ARK_DELETE;

    printf("re-open ark without LOAD\n"); fflush(stdout);
    ARK_CREATE_NEW_PERSIST;

    printf("verify ark is empty\n"); fflush(stdout);

    for (i=0; i<fLEN; i++)
    {
        ASSERT_EQ(ENOENT, ark_get(ark,
                                  fdb[i].klen,
                                  fdb[i].key,
                                  fdb[i].vlen,
                                  gvalue,
                                  0,
                                  &res));
    }

    kv_db_destroy(fdb, fLEN);
    kv_db_destroy(mdb, fLEN);
    ARK_DELETE;
}
