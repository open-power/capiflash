/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_errors.C $                             */
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
 *   Error test cases for kv FVT
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv.h>
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
#include <fvt_kv_utils_ark_io.h>
#include <kv_inject.h>
#include <ark.h>
#include <am.h>
#include <bl.h>
#include <errno.h>

ARK     *async_ark = NULL;
uint32_t async_io  = 0;
int32_t  async_err = 0;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_tst_io_errors_cb(int errcode, uint64_t dt, int64_t res)
{
    --async_io;
    if (async_err != errcode) printf("tag=%" PRIx64"\n", dt);
    ASSERT_EQ(async_err, errcode);
}

} //extern C

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_create)
{
    EXPECT_EQ(EINVAL, ark_create(NULL, NULL, ARK_KV_VIRTUAL_LUN));
    //ENOSPC
    //ENOTREADY
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_delete)
{
    EXPECT_EQ(EINVAL, ark_delete(NULL));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_set)
{
    ARK     *ark  = NULL;
    char     s[]  = {"12345"};
    int64_t  res  = 0;

    ARK_CREATE;
    EXPECT_EQ(EINVAL, ark_set(NULL, 5,    s,    5,    s, &res));
    EXPECT_EQ(EINVAL, ark_set(ark,  5,    NULL, 5,    s, &res));
    EXPECT_EQ(ENOMEM, ark_set(ark,  5,    s,   -1,    s, &res));
    EXPECT_EQ(EINVAL, ark_set(ark,  5,    s,    5, NULL, &res));
    EXPECT_EQ(EINVAL, ark_set(ark,  5,    s,    5,    s, NULL));
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_get)
{
    ARK     *ark           = NULL;
    kv_t     *db           = NULL;
    char     s[]           = {"12345"};
    int64_t  res           = 0;
    uint32_t LEN           = 100;

    ARK_CREATE;

    EXPECT_EQ(ENOENT, ark_get(ark,  5,    s,    5,    s, 0, &res));

    EXPECT_EQ(0,      ark_set(ark,  5,    s,    5,    s,    &res));

    EXPECT_EQ(EINVAL, ark_get(NULL, 5,    s,    5,    s, 0, &res));
    EXPECT_EQ(EINVAL, ark_get(ark,  5,    NULL, 5,    s, 0, &res));
    EXPECT_EQ(0,      ark_get(ark,  5,    s,   -1,    s, 0, &res));
    EXPECT_EQ(EINVAL, ark_get(ark,  5,    s,    5, NULL, 0, &res));
    EXPECT_EQ(EINVAL, ark_get(ark,  5,    s,    5,    s, 0, NULL));

    db = (kv_t*)kv_db_create_fixed(LEN, 2, 2);
    EXPECT_TRUE(db != NULL);
    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);
    EXPECT_EQ(ENOENT, ark_get(ark, 3, s, 5, s, 0, &res));

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_exists)
{
    ARK     *ark  = NULL;
    kv_t     *db  = NULL;
    char     s[]  = {"12345"};
    int64_t  res  = 0;
    uint32_t LEN  = 100;

    ARK_CREATE;

    EXPECT_EQ(EINVAL, ark_exists(NULL, 5,    s,    &res));
    EXPECT_EQ(EINVAL, ark_exists(ark,  5,    NULL, &res));
    EXPECT_EQ(EINVAL, ark_exists(ark,  5,    s,    NULL));

    EXPECT_EQ(ENOENT, ark_exists(ark, 5, s, &res));

    db = (kv_t*)kv_db_create_fixed(LEN, 2, 2);
    EXPECT_TRUE(db != NULL);
    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);
    EXPECT_EQ(ENOENT, ark_exists(ark, 5, s, &res));

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_del)
{
    ARK     *ark  = NULL;
    kv_t     *db  = NULL;
    char     s[]  = {"12345"};
    int64_t  res  = 0;
    uint32_t LEN  = 100;

    ARK_CREATE;

    EXPECT_EQ(EINVAL, ark_del(NULL, 5,    s,    &res));
    EXPECT_EQ(EINVAL, ark_del(ark,  5,    NULL, &res));
    EXPECT_EQ(EINVAL, ark_del(ark,  5,    s,    NULL));

    EXPECT_EQ(ENOENT, ark_del(ark, 5, s, &res));

    db = (kv_t*)kv_db_create_fixed(LEN, 2, 2);
    EXPECT_TRUE(db != NULL);
    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);
    EXPECT_EQ(ENOENT, ark_del(ark, 5, s, &res));

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_first)
{
    ARK     *ark           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    int64_t  klen          = 0;
    void    *_NULL         = NULL;

    ARK_CREATE;

    EXPECT_EQ(_NULL, ark_first(NULL, 5,    &klen, gvalue));
    EXPECT_EQ(errno, EINVAL);
    EXPECT_EQ(_NULL, ark_first(ark,  0,    &klen, gvalue));
    EXPECT_EQ(errno, EINVAL);
    EXPECT_EQ(_NULL, ark_first(ark,  5,    NULL, gvalue));
    EXPECT_EQ(errno, EINVAL);
    EXPECT_EQ(_NULL, ark_first(ark,  5,    &klen, NULL));
    EXPECT_EQ(errno, EINVAL);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_next)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    char     s[]           = {"12345"};
    int64_t  res           = 0;
    int64_t  klen          = 0;
    uint8_t  gvalue[KV_4K] = {0};

    ARK_CREATE;

    EXPECT_EQ(0, ark_set(ark, 5,    s,    5,    s, &res));

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    EXPECT_TRUE(ari != NULL);

    EXPECT_EQ(EINVAL, ark_next(NULL, KV_4K, &klen, gvalue));
    EXPECT_EQ(EINVAL, ark_next(ari,      0, &klen, gvalue));
    EXPECT_EQ(EINVAL, ark_next(ari,  KV_4K, NULL,  gvalue));
    EXPECT_EQ(EINVAL, ark_next(ari,  KV_4K, &klen, NULL));

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_count)
{
    ARK     *ark  = NULL;
    int64_t  cnt  = 0;

    EXPECT_EQ(EINVAL, ark_count(ark, &cnt));

    ARK_CREATE;

    EXPECT_EQ(EINVAL, ark_count(ark, NULL));

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_random)
{
    ARK     *ark           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    uint64_t klen          = 0;

    EXPECT_EQ(EINVAL, ark_random(ark, klen, &klen, gvalue));

    ARK_CREATE;

    EXPECT_EQ(EINVAL, ark_random(ark, klen, &klen, gvalue));
    EXPECT_EQ(EINVAL, ark_random(ark, 0,    &klen, gvalue));
    EXPECT_EQ(EINVAL, ark_random(ark, klen, NULL,  gvalue));
    EXPECT_EQ(EINVAL, ark_random(ark, klen, &klen, NULL));

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, BAD_PARMS_ark_fork)
{
    ARK *ark = NULL;

    EXPECT_EQ(-1, ark_fork(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, ALLOC_ERRORS)
{
    char     s[]  = {"12345678"};
    uint32_t klen = 8;
    int64_t  res  = 0;

    KV_SET_INJECT_ACTIVE;

    errno=0; KV_INJECT_ALLOC_ERROR;
    ASSERT_EQ(ENOMEM, ark_create(getenv("FVT_DEV"), 
                                 &async_ark, ARK_KV_VIRTUAL_LUN));

    errno=0;
    ASSERT_EQ(0, ark_create(getenv("FVT_DEV"), &async_ark, ARK_KV_VIRTUAL_LUN));
    EXPECT_TRUE(async_ark != NULL);

    /* sync */
    errno=0; KV_INJECT_ALLOC_ERROR;
    EXPECT_EQ(ENOMEM, ark_set(async_ark, klen, s, klen, s, &res));
    EXPECT_EQ(ENOENT, ark_get(async_ark, klen, s, klen, s, 0, &res));

    EXPECT_EQ(0, ark_set(async_ark, klen, s, klen, s, &res));
    EXPECT_EQ(0, ark_get(async_ark, klen, s, klen, s, 0, &res));

    errno=0; KV_INJECT_ALLOC_ERROR;
    EXPECT_EQ(ENOMEM, ark_del(async_ark, klen, s, &res));
    EXPECT_EQ(0,      ark_del(async_ark, klen, s, &res));

    errno=0; ++async_io; async_err = ENOMEM;
    KV_INJECT_ALLOC_ERROR;
    EXPECT_EQ(0, ark_set_async_cb(async_ark, klen, s, klen, s,
                                  kv_tst_io_errors_cb, 0xfee1));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = ENOENT;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee2));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_set_async_cb(async_ark, klen, s, klen, s,
                                  kv_tst_io_errors_cb, 0xfee3));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee4));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = ENOMEM;
    KV_INJECT_ALLOC_ERROR;
    EXPECT_EQ(0, ark_del_async_cb(async_ark, klen, s,
                                  kv_tst_io_errors_cb, 0xfee5));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_del_async_cb(async_ark, klen, s,
                                  kv_tst_io_errors_cb, 0xfee6));
    while (async_io) usleep(50000);

    errno=0; KV_SET_INJECT_INACTIVE;
    ASSERT_EQ(0, ark_delete(async_ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, IO_ERRORS)
{
    char           s[]      = {"12345678"};
    uint32_t       klen     = 8;
    int64_t        res      = 0;
    char          *path     = getenv("FVT_DEV");
    char           data[KV_4K];
    ark_io_list_t *bl_array = NULL;
    _ARK          *arkp     = NULL;

    KV_SET_INJECT_ACTIVE;

    ASSERT_EQ(0, ark_create(path, &async_ark, ARK_KV_VIRTUAL_LUN));
    EXPECT_TRUE(async_ark != NULL);

    arkp = (_ARK*)async_ark;

    bl_array = bl_chain_blocks(arkp->bl, 0, 1);
    ASSERT_TRUE(NULL != bl_array);

    errno=0; KV_INJECT_SCHD_READ_ERROR;
    EXPECT_EQ(5, ea_async_io(arkp->ea, ARK_EA_READ, &data, bl_array, 1, 1));

    errno=0; KV_INJECT_SCHD_WRITE_ERROR;
    EXPECT_EQ(5, ea_async_io(arkp->ea, ARK_EA_WRITE, &data, bl_array, 1, 1));

    errno=0; KV_INJECT_HARV_READ_ERROR;
    EXPECT_EQ(5, ea_async_io(arkp->ea, ARK_EA_READ,  &data, bl_array, 1, 1));

    errno=0; KV_INJECT_HARV_WRITE_ERROR;
    EXPECT_EQ(5, ea_async_io(arkp->ea, ARK_EA_WRITE, &data, bl_array, 1, 1));

    am_free(bl_array);

    /* sync */

    errno=0; KV_INJECT_SCHD_WRITE_ERROR;
    EXPECT_EQ(EIO,    ark_set(async_ark, klen, s, klen, s, &res));
    errno=0; KV_INJECT_HARV_WRITE_ERROR;
    EXPECT_EQ(EIO,    ark_set(async_ark, klen, s, klen, s, &res));
    EXPECT_EQ(ENOENT, ark_get(async_ark, klen, s, klen, s, 0, &res));

    EXPECT_EQ(0, ark_set(async_ark, klen, s, klen, s, &res));

    errno=0; KV_INJECT_SCHD_READ_ERROR;
    EXPECT_EQ(EIO, ark_get(async_ark, klen, s, klen, s, 0, &res));

    errno=0; KV_INJECT_SCHD_READ_ERROR;
    EXPECT_EQ(EIO, ark_exists(async_ark, klen, s, &res));

    errno=0; KV_INJECT_HARV_READ_ERROR;
    EXPECT_EQ(EIO, ark_get(async_ark, klen, s, klen, s, 0, &res));

    errno=0; KV_INJECT_HARV_READ_ERROR;
    EXPECT_EQ(EIO, ark_exists(async_ark, klen, s, &res));
#if 0
    errno=0; KV_INJECT_WRITE_ERROR;
    EXPECT_EQ(EIO, ark_del(async_ark, klen, s, &res));
#endif

    EXPECT_EQ(0, ark_del(async_ark, klen, s, &res));

    /* async */

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_SCHD_WRITE_ERROR;
    EXPECT_EQ(0, ark_set_async_cb(async_ark, klen, s, klen, s,
                                  kv_tst_io_errors_cb, 0xfee1));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_HARV_WRITE_ERROR;
    EXPECT_EQ(0, ark_set_async_cb(async_ark, klen, s, klen, s,
                                  kv_tst_io_errors_cb, 0xfee1));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = ENOENT;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee2));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_set_async_cb(async_ark, klen, s, klen, s,
                                  kv_tst_io_errors_cb, 0xfee3));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee4));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_SCHD_READ_ERROR;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee5));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_SCHD_READ_ERROR;
    EXPECT_EQ(0, ark_exists_async_cb(async_ark, klen, s,
                                     kv_tst_io_errors_cb, 0xfee6));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_HARV_READ_ERROR;
    EXPECT_EQ(0, ark_get_async_cb(async_ark, klen, s, klen, s, 0,
                                  kv_tst_io_errors_cb, 0xfee5));
    while (async_io) usleep(50000);

    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_HARV_READ_ERROR;
    EXPECT_EQ(0, ark_exists_async_cb(async_ark, klen, s,
                                     kv_tst_io_errors_cb, 0xfee6));
    while (async_io) usleep(50000);
#if 0
    errno=0; ++async_io; async_err = EIO;
    KV_INJECT_WRITE_ERROR;
    EXPECT_EQ(0, ark_del_async_cb(async_ark, klen, s,
                                  kv_tst_io_errors_cb, 0xfee7));
    while (async_io) usleep(50000);
#endif

    errno=0; ++async_io; async_err = 0;
    EXPECT_EQ(0, ark_del_async_cb(async_ark, klen, s,
                                  kv_tst_io_errors_cb, 0xfee8));
    while (async_io) usleep(50000);

    errno=0; KV_SET_INJECT_INACTIVE;
    ASSERT_EQ(0, ark_delete(async_ark));
}

/**
 *******************************************************************************
 * \brief
 *  -inject random errors to make sure no seg faults or worse happen
 *  -errors will print from failed ark get/set/exist/del and thats ok
 ******************************************************************************/
TEST(FVT_KV_ERROR_PATH, RANDOM_ERRORS)
{
    uint32_t ctxts = 10;
    uint32_t ops   = 10;
    uint32_t vlen  = KV_4K;
    uint32_t secs  = 20;
    char    *dev   = getenv("FVT_DEV");

    if (dev != NULL && strncmp("/dev/", dev, 5) != 0)
    {
        ctxts=1;
    }

    kv_async_init_ark_io_inject(ctxts, ops, vlen, secs);
    kv_async_start_jobs();

    printf("\n"); fflush(stdout);

    Sync_ark_io ark_io_job;
    ark_io_job.run_multi_arks(ctxts, ops, vlen, secs);

    kv_async_wait_jobs();
}
