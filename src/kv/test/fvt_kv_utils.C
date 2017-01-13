/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils.C $                                  */
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
 *   utility functions for KV FVT
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv.h>
#include <fvt_kv_utils.h>
#include <kv_utils_db.h>
#include <errno.h>
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_load(ARK *ark, kv_t *db, uint32_t LEN)
{
    int      rc  = 0;
    uint32_t i   = 0;
    int64_t  res = 0;

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);

    for (i=0; i<LEN; i++)
    {
        while (EAGAIN == (rc=ark_set(ark,
                                     db[i].klen,
                                     db[i].key,
                                     db[i].vlen,
                                     db[i].value,
                                     &res))) {usleep(10);}
        EXPECT_EQ(0, rc);
        EXPECT_EQ(db[i].vlen, res);
    }
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_query(ARK *ark, kv_t *db, uint32_t vbuflen, uint32_t LEN)
{
    int      rc     = 0;
    uint32_t i      = 0;
    int64_t  res    = 0;
    uint8_t *gvalue = NULL;

    if (vbuflen==0) {gvalue=(uint8_t*)malloc(1);}
    else            {gvalue=(uint8_t*)malloc(vbuflen);}
    ASSERT_TRUE(gvalue != NULL);

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);

    for (i=0; i<LEN; i++)
    {
        EXPECT_TRUE(db[i].vlen <= vbuflen);
        while (EAGAIN == (rc=ark_get(ark,
                                     db[i].klen,
                                     db[i].key,
                                     db[i].vlen,
                                     gvalue,
                                     0,
                                     &res))) {usleep(10);}
        EXPECT_EQ(0, rc);
        EXPECT_EQ(db[i].vlen, res);
        EXPECT_EQ(0, memcmp(db[i].value,gvalue,db[i].vlen));
        while (EAGAIN == (rc=ark_exists(ark,
                                        db[i].klen,
                                        db[i].key,
                                        &res))) {usleep(10);}
        EXPECT_EQ(0, rc);
    }
    free(gvalue);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_query_empty(ARK *ark, kv_t *db, uint32_t vbuflen,uint32_t LEN)
{
    uint32_t i      = 0;
    int64_t  res    = 0;
    uint8_t *gvalue = NULL;

    if (vbuflen==0) {gvalue=(uint8_t*)malloc(1);}
    else            {gvalue=(uint8_t*)malloc(vbuflen);}
    ASSERT_TRUE(gvalue != NULL);

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);

    for (i=0; i<LEN; i++)
    {
        EXPECT_TRUE(db[i].vlen <= vbuflen);
        EXPECT_EQ(ENOENT, ark_get(ark,
                             db[i].klen,
                             db[i].key,
                             db[i].vlen,
                             gvalue,
                             0,
                             &res));
        EXPECT_EQ(ENOENT, ark_exists(ark, db[i].klen, db[i].key, &res));
    }
    free(gvalue);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_query_off(ARK *ark, kv_t *db, uint32_t vbuflen, uint32_t LEN)
{
    uint32_t i      = 0;
    int64_t  res    = 0;
    uint32_t offset = 7;
    uint8_t *pval   = NULL;
    uint8_t *gvalue = NULL;

    if (vbuflen==0) {gvalue=(uint8_t*)malloc(1);}
    else            {gvalue=(uint8_t*)malloc(vbuflen);}
    ASSERT_TRUE(gvalue != NULL);

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);

    for (i=0; i<LEN; i++)
    {
        EXPECT_EQ(0, ark_exists(ark, db[i].klen, db[i].key, &res));
        EXPECT_TRUE(db[i].vlen <= vbuflen);
        EXPECT_EQ(0, ark_get(ark,
                             db[i].klen,
                             db[i].key,
                             db[i].vlen-offset,
                             gvalue,
                             offset,
                             &res));
        EXPECT_EQ(db[i].vlen, res);
        pval = (uint8_t*)db[i].value;
        EXPECT_EQ(0, memcmp(pval+offset, gvalue, db[i].vlen - offset));
    }
    free(gvalue);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_del(ARK *ark, kv_t *db, uint32_t LEN)
{
    int      rc  = 0;
    uint32_t i   = 0;
    int64_t  res = 0;

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);
    ASSERT_TRUE(0    != LEN);

    for (i=0; i<LEN; i++)
    {
        while (EAGAIN == (rc=ark_del(ark,
                                     db[i].klen,
                                     db[i].key,
                                     &res))) {usleep(10);}
        EXPECT_EQ(0, rc);
        EXPECT_EQ(db[i].vlen, res);
    }
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_SGD_LOOP(ARK     *ark,
                           void*  (*db_create)(uint32_t, uint32_t, uint32_t),
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t len,
                           uint32_t secs)
{
    uint32_t start = 0;
    uint32_t cur   = 0;
    int32_t  LEN   = (int32_t)len;
    kv_t    *db    = NULL;

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db_create);

    start = time(0);

    do
    {
        db = (kv_t*)db_create(LEN, klen, vlen);
        ASSERT_TRUE(db != NULL);

        /* load all key/value pairs from the db into the ark */
        fvt_kv_utils_load(ark, db, LEN);

        /* query all key/value pairs from the db */
        fvt_kv_utils_query(ark, db, vlen, LEN);

        /* delete all key/value pairs from the db */
        fvt_kv_utils_del(ark, db, LEN);

        /* query all key/value pairs from the db */
        fvt_kv_utils_query_empty(ark, db, vlen, LEN);

        kv_db_destroy(db, LEN);

        cur = time(0);
    }
    while (cur-start < secs);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_REP_LOOP(ARK       *ark,
                           void*    (*db_create)(uint32_t, uint32_t, uint32_t),
                           uint32_t (*db_regen) (kv_t*,    uint32_t, uint32_t),
                           uint32_t   klen,
                           uint32_t   vlen,
                           uint32_t   len,
                           uint32_t   secs)
{
    uint32_t i          = 0;
    uint32_t start      = 0;
    uint32_t cur        = 0;
    int32_t  LEN        = (int32_t)len;
    kv_t    *db         = NULL;
    uint32_t regen_vlen = vlen;

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db_create);
    ASSERT_TRUE(NULL != db_regen);

    db = (kv_t*)db_create(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    start = time(0);

    do
    {
        /* don't regen on first iteration */
        if (i > 0)
        {
            /* regenerate values to each key */
            if (i%2) regen_vlen = vlen+1;
            else     regen_vlen = vlen;
            ASSERT_TRUE(0 == db_regen(db, LEN, regen_vlen));
        }

        /* load/replace all key/value pairs from the db into the ark */
        fvt_kv_utils_load(ark, db, LEN);

        /* query all key/value pairs from the db */
        fvt_kv_utils_query(ark, db, regen_vlen, LEN);

        cur = time(0);
        ++i;
    }
    while (cur-start < secs);

    /* delete all key/value pairs from the db */
    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_read(ARK *ark, kv_t *db, uint32_t vbuflen, uint32_t LEN)
{
    uint32_t i      = 0;
    int64_t  res    = 0;
    uint8_t *gvalue = NULL;

    if (vbuflen==0) {gvalue=(uint8_t*)malloc(1);}
    else            {gvalue=(uint8_t*)malloc(vbuflen);}
    ASSERT_TRUE(gvalue != NULL);

    ASSERT_TRUE(NULL != ark);
    ASSERT_TRUE(NULL != db);

    for (i=0; i<LEN; i++)
    {
        EXPECT_TRUE(db[i].vlen <= vbuflen);
        EXPECT_EQ(0, ark_get(ark,
                             db[i].klen,
                             db[i].key,
                             db[i].vlen,
                             gvalue,
                             0,
                             &res));
        EXPECT_EQ(db[i].vlen, res);
    }
    free(gvalue);
}

/*******************************************************************************
 ******************************************************************************/
void fvt_kv_utils_perf(ARK *ark, uint32_t vlen, uint32_t mb, uint32_t LEN)
{
    kv_t    *db       = NULL;
    uint64_t mb64_1   = (uint64_t)KV_1M;
    uint64_t bytes    = mb64_1*mb;
    uint32_t i        = 0;
    uint32_t klen     = 16;
    uint64_t lchunk   = LEN*vlen;
    uint32_t loops    = mb;
    long int wr_us    = 0;
    long int rd_us    = 0;
    long int mil      = 1000000;
    float    wr_s     = 0;
    float    rd_s     = 0;
    float    wr_mb    = 0;
    uint64_t ops      = 0;
    uint64_t post_ops = 0;
    uint64_t io       = 0;
    uint64_t post_io  = 0;
    float    wr_ops   = 0;
    float    wr_ios   = 0;
    float    rd_ops   = 0;
    float    rd_ios   = 0;
    struct timeval stop, start;

    ASSERT_TRUE(NULL != ark);

    loops    = (uint32_t)(bytes / lchunk);

    db = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    printf("SYNC %dx%dx%d\n", klen, vlen, LEN);

    for (i=0; i<loops; i++)
    {
        /* write/load all key/value pairs from the db into the ark */
        (void)ark_stats(ark, &ops, &io);
        gettimeofday(&start, NULL);
        fvt_kv_utils_load(ark, db, LEN);
        gettimeofday(&stop, NULL);
        wr_us += (stop.tv_sec*mil  + stop.tv_usec) -
                 (start.tv_sec*mil + start.tv_usec);
        (void)ark_stats(ark, &post_ops, &post_io);
        wr_ops += post_ops - ops;
        wr_ios += post_io  - io;

        /* read/query all key/value pairs from the db */
        (void)ark_stats(ark, &ops, &io);
        gettimeofday(&start, NULL);
        fvt_kv_utils_read(ark, db, vlen, LEN);
        gettimeofday(&stop, NULL);
        rd_us += (stop.tv_sec*mil  + stop.tv_usec) -
                 (start.tv_sec*mil + start.tv_usec);
        (void)ark_stats(ark, &post_ops, &post_io);
        rd_ops += post_ops - ops;
        rd_ios += post_io  - io;

        /* delete all key/value pairs from the db */
        fvt_kv_utils_del(ark, db, LEN);
    }
    wr_s = (float)((float)wr_us/(float)mil);
    rd_s = (float)((float)rd_us/(float)mil);
    wr_mb = (float)((double)(bytes+(uint64_t)(klen*LEN*loops)) / (double)mb64_1);
    printf("   writes: %d mb in %.1f secs at %.3f mbps, %.0f op/s, %.0f io/s\n",
            mb,
            wr_s,
            wr_mb/wr_s,
            wr_ops/wr_s,
            wr_ios/wr_s);
    printf("   reads:  %d mb in %.1f secs at %.3f mbps, %.0f op/s, %.0f io/s\n",
            mb,
            rd_s,
            (float)mb/rd_s,
            rd_ops/rd_s,
            rd_ios/rd_s);
    kv_db_destroy(db, LEN);
}
