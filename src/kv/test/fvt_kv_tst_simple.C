/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_tst_simple.C $                             */
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
 *   Simple test cases for kv FVT
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv_utils.h>
#include <errno.h>
#include <iv.h>
#include <bl.h>
#include <bt.h>
#include <am.h>
#include <ark.h>
#include <arkdb_trace.h>
uint64_t seed = UINT64_C(0xEDCA000000000000);
}

extern KV_Trace_t *pAT;

/**
 *******************************************************************************
 * \brief
 *   create kv databases and make sure lookups work, then destroy
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_validate_kv_utils)
{
    int32_t i    = 0;
    int32_t loop = 0;
    int32_t FLEN = 1000;
    int32_t MLEN = 1000;
    void   *null = NULL;

    for (loop=0; loop<100; loop++)
    {
        kv_t *db_f = (kv_t*)kv_db_create_fixed(FLEN, 256, 4);
        ASSERT_TRUE(db_f != NULL);

        kv_t *db_m = (kv_t*)kv_db_create_mixed(MLEN, 1024, 64);
        ASSERT_TRUE(db_m != NULL);

        for (i=0; i<FLEN; i++)
        {
            ASSERT_NE(null, kv_db_find(db_f, FLEN, db_f[i].key, db_f[i].klen));
        }

        for (i=0; i<MLEN; i++)
        {
            ASSERT_NE(null, kv_db_find(db_m, MLEN, db_m[i].key, db_m[i].klen));
        }
        kv_db_destroy(db_f, FLEN);
        kv_db_destroy(db_m, MLEN);
    }
}

/**
 *******************************************************************************
 * \brief
 *   test IV functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_iv)
{
    uint64_t n  = 233;
    uint64_t m  = 17;
    uint32_t i  = 0;
    IV      *iv = NULL;

    KV_TRC_OPEN(pAT, "arkdb");

    ASSERT_TRUE(iv_set(iv, 0, 0) < 0);
    ASSERT_TRUE(iv_get(iv, 0)    < 0);

    iv = iv_new(n, m);
    ASSERT_TRUE(NULL != iv);
    for (i=0;   i<n;  i++)        ASSERT_TRUE(0 == iv_set(iv, i, i));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i == iv_get(iv, i));

    n  = 433;
    iv = iv_resize(iv, n, m);
    ASSERT_TRUE(0 != iv);
    for (i=0;   i<n;  i++)        ASSERT_TRUE(0 == iv_set(iv, i, i));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i == iv_get(iv, i));

    for (i=800; i<800+n; i++)     ASSERT_TRUE(iv_set(iv, i, i) < 0);
    for (i=800; i<800+n; i++)     ASSERT_TRUE(iv_get(iv, i)    < 0);

    for (i=0;   i<n;  i++)        ASSERT_TRUE(0    == iv_set(iv, i, i+10));
    for (i=n-1; i>=0 && i<n; i--) ASSERT_TRUE(i+10 == iv_get(iv, i));

    iv_delete(iv);

    KV_TRC_CLOSE(pAT);
}

/**
 *******************************************************************************
 * \brief
 *   test BL functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_bl)
{

    uint64_t       n  = 20000;
    uint64_t       w  = ARK_VERBOSE_BLKBITS_DEF;
    BL            *bl = NULL;
    ark_io_list_t *aiol=NULL;

    KV_TRC_OPEN(pAT, "arkdb");

    ASSERT_TRUE(bl_init_chain_link(bl) == 1);
    ASSERT_TRUE(bl_reserve(bl,1) < 0);
    ASSERT_TRUE(bl_take(bl,1) < 0);
    ASSERT_TRUE(bl_drop(bl,1) < 0);
    ASSERT_TRUE(bl_left(bl) < 0);
    ASSERT_TRUE(bl_len(bl,1) == 0);
    ASSERT_TRUE(bl_next(bl,1) < 0);
    ASSERT_TRUE(bl_chain(bl,1,10,0) == 0);
    ASSERT_TRUE(bl_rechain(&aiol,bl,1,1,1,0) != 0);

    bl = bl_new(n, w);
    ASSERT_TRUE(NULL != bl);

    ASSERT_EQ(bl_init_chain_link(bl), 0);
    ASSERT_EQ(bl_left(bl), 10000);
    ASSERT_EQ(bl_reserve(bl,1), 0);
    ASSERT_EQ(bl_take(bl,1), 2);
    ASSERT_EQ(bl_drop(bl,2),1);
    ASSERT_EQ(bl_take(bl,2), 2);
    ASSERT_EQ(bl_len(bl,2), 2);
    ASSERT_EQ(bl_next(bl,2), 3);
    ASSERT_EQ(bl_drop(bl,2),2);
    ASSERT_EQ(bl_take(bl,10), 2);
    ASSERT_EQ(bl_len(bl,2), 10);
    aiol = bl_chain(bl,2,10,0);
    ASSERT_TRUE(aiol != NULL);
    ASSERT_EQ(bl_drop(bl,2),10);
    ASSERT_EQ(bl_take(bl,20), 2);
    ASSERT_EQ(bl_len(bl,2), 20);
    ASSERT_EQ(bl_rechain(&aiol,bl,2,20,10,0), 0);

    am_free(aiol);
    bl_free(bl);

    /* now do it backwards */
    bl = bl_new(n, w);
    ASSERT_TRUE(NULL != bl);
    ASSERT_EQ(bl_left(bl), 0);
    ASSERT_EQ(bl_drop(bl,1), 1);    ASSERT_EQ(bl_left(bl), 1);
    ASSERT_EQ(bl_drop(bl,5), 1);    ASSERT_EQ(bl_left(bl), 2);
    ASSERT_EQ(bl_drop(bl,3), 1);    ASSERT_EQ(bl_left(bl), 3);
    ASSERT_EQ(bl_drop(bl,7), 1);    ASSERT_EQ(bl_left(bl), 4);
    ASSERT_EQ(bl_take(bl,1), 7);    ASSERT_EQ(bl_left(bl), 3);
    ASSERT_EQ(bl_take(bl,1), 3);    ASSERT_EQ(bl_left(bl), 2);
    ASSERT_EQ(bl_take(bl,1), 5);    ASSERT_EQ(bl_left(bl), 1);
    ASSERT_EQ(bl_take(bl,1), 1);    ASSERT_EQ(bl_left(bl), 0);

    ASSERT_EQ(bl_drop(bl,1), 1);    ASSERT_EQ(bl_left(bl), 1);
    ASSERT_EQ(bl_drop(bl,5), 1);    ASSERT_EQ(bl_left(bl), 2);
    ASSERT_EQ(bl_take(bl,1), 5);    ASSERT_EQ(bl_left(bl), 1);
    ASSERT_EQ(bl_drop(bl,3), 1);    ASSERT_EQ(bl_left(bl), 2);
    ASSERT_EQ(bl_drop(bl,7), 1);    ASSERT_EQ(bl_left(bl), 3);
    ASSERT_EQ(bl_take(bl,1), 7);    ASSERT_EQ(bl_left(bl), 2);
    ASSERT_EQ(bl_take(bl,1), 3);    ASSERT_EQ(bl_left(bl), 1);
    ASSERT_EQ(bl_take(bl,1), 1);    ASSERT_EQ(bl_left(bl), 0);

    bl_free(bl);

    KV_TRC_CLOSE(pAT);

}

void rdy_unmap(BL *bl, BL *blu, uint64_t blk)
{
    uint32_t i=0,len=0;

    len = bl_drop(bl,blk);

    for (i=0; i<len; i++)
    {
        ASSERT_TRUE((blk=bl_take(bl,1)) > 0);
        ASSERT_EQ(bls_add(blu,blk),1);
    }
}

/**
 *******************************************************************************
 * \brief
 *   test BL functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_unmap_bl)
{
    uint64_t       n    = 10;
    uint64_t       w    = ARK_VERBOSE_BLKBITS_DEF;
    BL            *bl   = NULL;
    BL            *blu  = NULL;
    int64_t        a    = 0;
    int64_t        b    = 0;
    int64_t        c    = 0;
    int64_t        d    = 0;
    uint32_t       alen = 3;
    uint32_t       blen = 1;
    uint32_t       clen = 2;
    uint32_t       dlen = 7;
    uint32_t       i    = 0;

    KV_TRC_OPEN(pAT, "arkdb");

    bl  = bl_new(n, w);
    blu = bl_new(n, w);
    ASSERT_TRUE(NULL != bl);
    ASSERT_TRUE(NULL != blu);

    /* init */
    ASSERT_EQ(bl_init_chain_link(bl), 0);
    ASSERT_EQ(bl_left(bl),  8);
    ASSERT_EQ(bl_left(blu), 0);

    /* take chains */
    ASSERT_EQ((a=bl_take(bl,alen)), 1); ASSERT_EQ(bl_left(bl), 5);
    ASSERT_EQ((b=bl_take(bl,blen)), 4); ASSERT_EQ(bl_left(bl), 4);
    ASSERT_EQ((c=bl_take(bl,clen)), 5); ASSERT_EQ(bl_left(bl), 2);

    /* update bl and blu with chains for unmap */
    rdy_unmap(bl, blu, b); ASSERT_EQ(bl_left(blu), 1); ASSERT_EQ(bl_left(bl), 2);
    rdy_unmap(bl, blu, c); ASSERT_EQ(bl_left(blu), 3); ASSERT_EQ(bl_left(bl), 2);
    rdy_unmap(bl, blu, a); ASSERT_EQ(bl_left(blu), 6); ASSERT_EQ(bl_left(bl), 2);

    uint32_t left=bl_left(blu);

    /* mark unmaps complete */
    for (i=0; i<left; i++)
    {
        ASSERT_TRUE((a=bls_rem(blu)) > 0);
        ASSERT_EQ(bl_drop(bl,a),1);
    }

    /* verify */
    ASSERT_EQ(bl_left(bl),  8);
    ASSERT_EQ(bl_left(blu), 0);

    /* do it again, with a resize */

    /* take chains */
    ASSERT_EQ((b=bl_take(bl,blen)), 4); ASSERT_EQ(bl_left(bl), 7);
    ASSERT_EQ((a=bl_take(bl,alen)), 5); ASSERT_EQ(bl_left(bl), 4);
    ASSERT_EQ((c=bl_take(bl,clen)), 2); ASSERT_EQ(bl_left(bl), 2);

    ASSERT_EQ(bl_resize(bl,  bl->n*2,  bl->w),  bl);
    ASSERT_EQ(bl_resize(blu, blu->n*2, blu->w), blu);

    ASSERT_EQ(bl_left(bl),  2);
    ASSERT_EQ(bl_left(blu), 0);

    ASSERT_EQ((d=bl_take(bl,dlen)), 7); ASSERT_EQ(bl_left(bl), 5);

    /* update bl and blu with chains for unmap */
    rdy_unmap(bl, blu, c); ASSERT_EQ(bl_left(blu), 2);  ASSERT_EQ(bl_left(bl), 5);
    rdy_unmap(bl, blu, d); ASSERT_EQ(bl_left(blu), 9);  ASSERT_EQ(bl_left(bl), 5);
    rdy_unmap(bl, blu, b); ASSERT_EQ(bl_left(blu), 10); ASSERT_EQ(bl_left(bl), 5);
    rdy_unmap(bl, blu, a); ASSERT_EQ(bl_left(blu), 13); ASSERT_EQ(bl_left(bl), 5);

    left=bl_left(blu);

    /* mark unmaps complete */
    for (i=0; i<left; i++)
    {
        ASSERT_TRUE((a=bl_take(blu,1)) > 0);
        ASSERT_EQ(bl_drop(bl,a),1);
    }

    /* verify */
    ASSERT_EQ(bl_left(bl),  18);
    ASSERT_EQ(bl_left(blu), 0);

    /* cleanup */
    bl_free(bl);
    bl_free(blu);
}

/**
 *******************************************************************************
 * \brief
 *   test IV functions
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_bt)
{
    BT      *bt1;
    BT      *bt2;
    BT      *bt1_orig;
    BT      *bt2_orig;
    uint8_t  key4[5] = {"key4"};
    uint8_t  key5[6] = {"key55"};
    uint8_t  key6[7] = {"key666"};
    uint8_t  key7[8] = {"key7777"};
    uint8_t  val4[5] = {"vala"};
    uint8_t  val5[6] = {"valbb"};
    uint8_t  val6[7] = {"valccc"};
    uint8_t  val7[8] = {"valdddd"};
    uint8_t  val9[10]= {"blknumber"};
    uint8_t  buf[11];
    uint64_t oldvdf;
    uint64_t oldvlen;

    KV_TRC_OPEN(pAT, "arkdb");

    bt1 = bt_new(128, 8, sizeof(uint64_t), &bt1_orig);
    ASSERT_TRUE(NULL != bt1);
    bt2 = bt_new(128, 8, sizeof(uint64_t), &bt2_orig);
    ASSERT_TRUE(NULL != bt2);

    /* insert head */
    ASSERT_EQ(1, bt_set      (bt2, bt1, 4, key4, 4, val4, &oldvdf, &oldvlen));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    /* insert head with 1 existing */
    ASSERT_EQ(1, bt_set      (bt1, bt2, 5, key5, 5, val5, &oldvdf, &oldvlen));
    ASSERT_EQ(5, bt_exists   (bt1,      5, key5));
    ASSERT_EQ(5, bt_get      (bt1,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));
    ASSERT_EQ(4, bt_exists   (bt1,      4, key4));
    ASSERT_EQ(4, bt_get      (bt1,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    /* insert head with 2 existing */
    ASSERT_EQ(1, bt_set      (bt2, bt1, 6, key6, 6, val6, &oldvdf, &oldvlen));
    ASSERT_EQ(6, bt_exists   (bt2,      6, key6));
    ASSERT_EQ(6, bt_get      (bt2,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));
    ASSERT_EQ(5, bt_exists   (bt2,      5, key5));
    ASSERT_EQ(5, bt_get      (bt2,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));

    /* replace value of 1st key with larger size > vdf */
    ASSERT_EQ(0, bt_set      (bt1, bt2, 6, key6, 9, val9, &oldvdf, &oldvlen));
    ASSERT_EQ(9, bt_exists   (bt1,      6, key6));
    ASSERT_EQ(9, bt_get      (bt1,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val9,buf,8));
    ASSERT_EQ(4, bt_exists   (bt1,      4, key4));
    ASSERT_EQ(4, bt_get      (bt1,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));
    ASSERT_EQ(5, bt_exists   (bt1,      5, key5));
    ASSERT_EQ(5, bt_get      (bt1,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));

    /* replace value of 1st key with smaller size < vdf */
    ASSERT_EQ(0, bt_set      (bt2, bt1, 6, key6, 6, val6, &oldvdf, &oldvlen));
    ASSERT_EQ(6, bt_exists   (bt2,      6, key6));
    ASSERT_EQ(6, bt_get      (bt2,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));
    ASSERT_EQ(5, bt_exists   (bt2,      5, key5));
    ASSERT_EQ(5, bt_get      (bt2,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));

    /* delete middle */
    ASSERT_EQ(5, bt_del      (bt1, bt2, 5, key5));
    ASSERT_EQ(-1,bt_exists   (bt1,      5, key5));
    ASSERT_EQ(-1,bt_get      (bt1,      5, key5, buf));
    ASSERT_EQ(6, bt_exists   (bt1,      6, key6));
    ASSERT_EQ(6, bt_get      (bt1,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));
    ASSERT_EQ(4, bt_exists   (bt1,      4, key4));
    ASSERT_EQ(4, bt_get      (bt1,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    /* delete tail */
    ASSERT_EQ(4, bt_del      (bt2, bt1, 4, key4));
    ASSERT_EQ(-1,bt_exists   (bt2,      4, key4));
    ASSERT_EQ(-1,bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(6, bt_exists   (bt2,      6, key6));
    ASSERT_EQ(6, bt_get      (bt2,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));

    /* delete head */
    ASSERT_EQ(6, bt_del      (bt1, bt2, 6, key6));
    ASSERT_EQ(-1,bt_exists   (bt1,      6, key6));
    ASSERT_EQ(-1,bt_get      (bt1,      6, key6, buf));

    /* insert head */
    ASSERT_EQ(1, bt_set      (bt2, bt1, 4, key4, 4, val4, &oldvdf, &oldvlen));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    /* insert head with 1 existing */
    ASSERT_EQ(1, bt_set      (bt1, bt2, 5, key5, 5, val5, &oldvdf, &oldvlen));
    ASSERT_EQ(5, bt_exists   (bt1,      5, key5));
    ASSERT_EQ(5, bt_get      (bt1,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));
    ASSERT_EQ(4, bt_exists   (bt1,      4, key4));
    ASSERT_EQ(4, bt_get      (bt1,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    /* insert head with 2 existing */
    ASSERT_EQ(1, bt_set      (bt2, bt1, 6, key6, 6, val6, &oldvdf, &oldvlen));
    ASSERT_EQ(6, bt_exists   (bt2,      6, key6));
    ASSERT_EQ(6, bt_get      (bt2,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));
    ASSERT_EQ(5, bt_exists   (bt2,      5, key5));
    ASSERT_EQ(5, bt_get      (bt2,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val5,buf,5));

    /* replace value of middle key with > vdf */
    ASSERT_EQ(0, bt_set      (bt1, bt2, 5, key5, 9, val9, &oldvdf, &oldvlen));
    ASSERT_EQ(9, bt_exists   (bt1,      5, key5));
    ASSERT_EQ(9, bt_get      (bt1,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val9,buf,8));
    ASSERT_EQ(6, bt_exists   (bt1,      6, key6));
    ASSERT_EQ(6, bt_get      (bt1,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));
    ASSERT_EQ(4, bt_exists   (bt1,      4, key4));
    ASSERT_EQ(4, bt_get      (bt1,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));

    ASSERT_EQ(1, bt_set      (bt2, bt1, 7, key7, 7, val7, &oldvdf, &oldvlen));
    ASSERT_EQ(7, bt_exists   (bt2,      7, key7));
    ASSERT_EQ(7, bt_get      (bt2,      7, key7, buf));
    ASSERT_EQ(0, memcmp(val7,buf,7));
    ASSERT_EQ(4, bt_exists   (bt2,      4, key4));
    ASSERT_EQ(4, bt_get      (bt2,      4, key4, buf));
    ASSERT_EQ(0, memcmp(val4,buf,4));
    ASSERT_EQ(9, bt_exists   (bt2,      5, key5));
    ASSERT_EQ(9, bt_get      (bt2,      5, key5, buf));
    ASSERT_EQ(0, memcmp(val9,buf,8));
    ASSERT_EQ(6, bt_exists   (bt2,      6, key6));
    ASSERT_EQ(6, bt_get      (bt2,      6, key6, buf));
    ASSERT_EQ(0, memcmp(val6,buf,6));

    bt_delete(bt1_orig);
    bt_delete(bt2_orig);

    KV_TRC_CLOSE(pAT);
}

/**
 *******************************************************************************
 * \brief
 *   create/delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_create_delete)
{
    ARK  *ark = NULL;

    EXPECT_EQ(0, ark_create(getenv("FVT_DEV"), &ark, 0));
    usleep(100000);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SET_GET_EXISTS_DEL)
{
    ARK     *ark  = NULL;
    char     k0[]  = {"1234"};
    char     k1[]  = {"5678"};
    char     k2[]  = {"9101"};
    char     k3[]  = {"1121"};
    uint32_t klen = 4;
    char     v0[]  = {"12345678"};
    char     v1[]  = {"9101112 "};
    char     v2[]  = {"13141516"};
    char     v3[300];
    char     g[300];
    uint32_t vlen = 8;
    int64_t  res  = 0;

    ARK_CREATE;

    ASSERT_EQ(ENOENT, ark_get   (ark, klen, k0, vlen, g, 0, &res));
    ASSERT_EQ(ENOENT, ark_exists(ark, klen, k0, &res));
    ASSERT_EQ(ENOENT, ark_del   (ark, klen, k0, &res));
    ASSERT_EQ(-1, res);
    ASSERT_EQ(0,      ark_set   (ark, klen, k0, vlen, v0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0,      ark_get   (ark, klen, k0, vlen, g, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(v0,g,vlen));
    ASSERT_EQ(0,      ark_exists(ark, klen, k0, &res));
    ASSERT_EQ(0,      ark_del   (ark, klen, k0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(ENOENT, ark_get   (ark, klen, k0, vlen, g, 0, &res));
    ASSERT_EQ(ENOENT, ark_exists(ark, klen, k0, &res));
    ASSERT_EQ(ENOENT, ark_del   (ark, klen, k0, &res));
    ASSERT_EQ(-1, res);

    ASSERT_EQ(0,      ark_set   (ark, klen, k0, vlen, v0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0,      ark_get   (ark, klen, k0, vlen, g, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(v0,g,vlen));
    ASSERT_EQ(0,      ark_exists(ark, klen, k0, &res));

    ASSERT_EQ(0,      ark_set   (ark, klen, k1, vlen, v1, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0,      ark_get   (ark, klen, k1, vlen, g, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(v1,g,vlen));
    ASSERT_EQ(0,      ark_exists(ark, klen, k1, &res));

    ASSERT_EQ(0,      ark_set   (ark, klen, k2, vlen, v2, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0,      ark_get   (ark, klen, k2, vlen, g, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(v2,g,vlen));
    ASSERT_EQ(0,      ark_exists(ark, klen, k2, &res));

    ASSERT_EQ(0,      ark_set   (ark, klen, k3, 300, v3, &res));
    ASSERT_EQ(300, res);
    ASSERT_EQ(0,      ark_get   (ark, klen, k3, 300, g, 0, &res));
    ASSERT_EQ(300, res);
    ASSERT_EQ(0, memcmp(v3,g,300));
    ASSERT_EQ(0,      ark_exists(ark, klen, k3, &res));

    ASSERT_EQ(0,      ark_del   (ark, klen, k3, &res));
    ASSERT_EQ(300, res);
    ASSERT_EQ(ENOENT, ark_get   (ark, klen, k3, vlen, g, 0, &res));
    ASSERT_EQ(ENOENT, ark_exists(ark, klen, k3, &res));
    ASSERT_EQ(ENOENT, ark_del   (ark, klen, k3, &res));
    ASSERT_EQ(-1, res);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_1x0x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 0;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_1x1x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 1;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_2x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 2;
    uint32_t vlen  = 2;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_4x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 4;
    uint32_t vlen  = 3;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_32x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 32;
    uint32_t vlen  = 128;
    uint32_t LEN   = 500;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_64x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_FIXED_128x64K)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_64K;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_fixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_WITH_OFFSET_FIXED_3x11)
{
    ARK     *ark   = NULL;
    kv_t    *db    = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 11;
    uint32_t LEN   = 200;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query_off(ark, db, vlen, LEN);

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_3x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 2;
    uint32_t LEN   = 300;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_10x10)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 10;
    uint32_t vlen  = 10;
    uint32_t LEN   = 500;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_100x100)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 100;
    uint32_t vlen  = 100;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_SGD_MIXED_BIG_BLOCKS)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_2M;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_SGD_LOOP(ark, kv_db_create_mixed, klen, vlen, LEN, secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_1x0)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 1;
    uint32_t vlen  = 0;
    uint32_t LEN   = 100;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_64x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 128;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_FIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_fixed,
                          kv_db_fixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED)
{
    ARK     *ark      = NULL;
    uint32_t klen     = 3;
    uint32_t vlen     = 10;
    int      idx      = 10;
    int64_t  res      = 0;
    char     key[4]   = "abc";
    char     val[4096] = {0};
    char     buf[4096] = {0};

    ARK_CREATE;

    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with a different value */
    idx = 15;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with a bigger different value */
    idx  = 20;
    vlen = 20;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with >vdf bigger different value */
    idx  = 4096;
    vlen = 4096;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with a smaller different value */
    idx  = 256;
    vlen = 256;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with a smaller different value */
    idx  = 4;
    vlen = 4;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    /* replace with a bigger different value */
    idx  = 256;
    klen = 256;
    vlen = 4096;
    GEN_VAL(val, idx, vlen);
    ASSERT_EQ(0, ark_set(ark, klen, key, vlen, val, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, ark_get(ark, klen, key, vlen, buf, 0, &res));
    ASSERT_EQ(vlen, res);
    ASSERT_EQ(0, memcmp(val,buf,vlen));

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_3x2)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 3;
    uint32_t vlen  = 2;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_64x128)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 64;
    uint32_t vlen  = 128;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add variable length key/value to ark, query them, replace them, verify
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_256x4096)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 256;
    uint32_t vlen  = 4096;
    uint32_t LEN   = 1000;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   add fixed length key/value to ark, then query them
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_REP_MIXED_BIG_BLOCKS)
{
    ARK     *ark   = NULL;
    uint32_t klen  = 128;
    uint32_t vlen  = KV_2M;
    uint32_t LEN   = 200;
    uint32_t secs  = 3;

    ARK_CREATE;
    fvt_kv_utils_REP_LOOP(ark,
                          kv_db_create_mixed,
                          kv_db_mixed_regen_values,
                          klen,
                          vlen,
                          LEN,
                          secs);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_ark_get_PARTIAL_VALUE)
{
    ARK     *ark  = NULL;
    char     s[]  = {"12345"};
    char     g[5] = {0};
    int64_t  res  = 0;

    ARK_CREATE;

    ASSERT_EQ(0, ark_set(ark,  5,    s,    5,    s,    &res));
    ASSERT_EQ(0, ark_get(ark,  5,    s,    3,    g, 0, &res));

    ASSERT_EQ(0, memcmp(s,g,3));
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   create, query non-existing key, delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_count)
{
    ARK     *ark   = NULL;
    kv_t    *db    = NULL;
    int32_t  LEN   = 394;
    int64_t  count = 0;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_fixed(LEN, 32, 1);
    ASSERT_TRUE(db != NULL);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, LEN);

    fvt_kv_utils_del(ark, db, LEN);

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_count(ark, &count));
    ASSERT_EQ(count, 0);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   load keys, ark_random, verify key is valid and different from last key,
 *    delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_random)
{
    ARK     *ark           = NULL;
    kv_t    *db            = NULL;
    uint32_t LEN           = 4931;
    uint64_t klen          = 0;
    uint64_t sklen         = 0;
    uint32_t i             = 0;
    char     svalue[KV_4K] = {0};
    char     kvalue[KV_4K] = {0};
    void    *null          = NULL;
    char    *dev           = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    1973,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN|ARK_KV_HTC));

    ASSERT_EQ(ENOENT, ark_random(ark, KV_4K, &klen, kvalue));

    db   = (kv_t*)kv_db_create_fixed(LEN, 32, 250);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ASSERT_EQ(0, ark_random(ark, KV_4K, &klen, kvalue));

    for (i=0; i<LEN*2; i++)
    {
        ASSERT_EQ(0, ark_random(ark, KV_4K, &klen, kvalue));

        /* make sure the key exists in our db */
        ASSERT_NE(null, kv_db_find(db, LEN, kvalue, klen));

        /* make sure the new key is not the same as the last key */
        if (i && klen == sklen)
        {
            ASSERT_NE(0, memcmp(kvalue, svalue, klen));
        }

        /* save this key to compare to the next key */
        memcpy(svalue, kvalue, klen);
        sklen = klen;
    }
    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   load keys, ark_random, verify key is valid and different from last key,
 *    delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_random_no_HTC)
{
    ARK     *ark           = NULL;
    kv_t    *db            = NULL;
    uint32_t LEN           = 4931;
    uint64_t klen          = 0;
    uint64_t sklen         = 0;
    uint32_t i             = 0;
    char     svalue[KV_4K] = {0};
    char     kvalue[KV_4K] = {0};
    void    *null          = NULL;
    char    *dev           = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    1973,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    ASSERT_EQ(ENOENT, ark_random(ark, KV_4K, &klen, kvalue));

    db   = (kv_t*)kv_db_create_fixed(LEN, 32, 250);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ASSERT_EQ(0, ark_random(ark, KV_4K, &klen, kvalue));

    for (i=0; i<LEN*2; i++)
    {
        ASSERT_EQ(0, ark_random(ark, KV_4K, &klen, kvalue));

        /* make sure the key exists in our db */
        ASSERT_NE(null, kv_db_find(db, LEN, kvalue, klen));

        /* make sure the new key is not the same as the last key */
        if (i && klen == sklen)
        {
            ASSERT_NE(0, memcmp(kvalue, svalue, klen));
        }

        /* save this key to compare to the next key */
        memcpy(svalue, kvalue, klen);
        sklen = klen;
    }
    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_next_FIXED_134x1)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 1733;
    int64_t  res           = 0;
    int64_t  klen          = 134;
    int32_t  num           = 0;
    int32_t  rc            = 0;
    char    *dev           = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    579,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_fixed(LEN, klen, 1);
    ASSERT_TRUE(db != NULL);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_NE(ari,  (ARI*)NULL);
    ASSERT_NE(klen, 0);
    ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
    ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
    ASSERT_NE(0, res);

    /* use ark_next to query all key/value pairs from the ark */
    while (++num < LEN)
    {
        rc = ark_next(ari, KV_4K, &klen, gvalue);
        ASSERT_EQ(rc,   0);
        ASSERT_EQ(klen, klen);
        ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
        ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
        ASSERT_NE(0, res);
    }
    ASSERT_EQ(num,LEN);

    // query LEN+1
    ASSERT_EQ(ENOENT, ark_next(ari, KV_4K, &klen, gvalue));

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_next_FIXED_23x277)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 1917;
    int64_t  res           = 0;
    int64_t  klen          = 23;
    int32_t  num           = 0;
    int32_t  rc            = 0;
    char    *dev           = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    ARK_VERBOSE_HASH_DEF,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_fixed(LEN, klen, 1377);
    ASSERT_TRUE(db != NULL);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_NE(ari,  (ARI*)NULL);
    ASSERT_NE(klen, 0);
    ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
    ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
    ASSERT_NE(0, res);

    /* use ark_next to query all key/value pairs from the ark */
    while (++num < LEN)
    {
        rc = ark_next(ari, KV_4K, &klen, gvalue);
        ASSERT_EQ(rc,   0);
        ASSERT_EQ(klen, klen);
        ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
        ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
        ASSERT_NE(0, res);
    }
    ASSERT_EQ(num,LEN);

    // query LEN+1
    ASSERT_EQ(ENOENT, ark_next(ari, KV_4K, &klen, gvalue));

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_next_MIXED_256x4)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 500;
    int64_t  res           = 0;
    int64_t  klen          = 256;
    int32_t  num           = 0;
    int32_t  rc            = 0;

    ARK_CREATE;

    db = (kv_t*)kv_db_create_mixed(LEN, 256, 4);
    ASSERT_TRUE(db != NULL);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_NE(ari,  (ARI*)NULL);
    ASSERT_NE(klen, 0);
    ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
    ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
    ASSERT_NE(0, res);

    /* use ark_next to query all key/value pairs from the ark */
    while (++num < LEN)
    {
        rc = ark_next(ari, KV_4K, &klen, gvalue);
        ASSERT_EQ(rc,0);
        ASSERT_TRUE(klen >= 0);
        ASSERT_TRUE(klen < 257);
        ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
        ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
        ASSERT_NE(0, res);
    }
    ASSERT_EQ(LEN, num);

    // query LEN+1
    ASSERT_EQ(ENOENT, ark_next(ari, KV_4K, &klen, gvalue));

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_first_nextN)
{
    ARK     *ark           = NULL;
    ARI     *ari           = NULL;
    uint8_t  gvalue[KV_4K] = {0};
    kv_t    *db            = NULL;
    int32_t  LEN           = 5333;
    int64_t  res           = 0;
    int64_t  klen          = 237;
    int32_t  num           = 0;
    int64_t  keyN          = -1;
    uint8_t *buf[KV_64K]   = {0};
    keye_t  *keye          = NULL;
    void    *null          = NULL;
    char    *dev           = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    1000,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_mixed(LEN, 256, 4);
    ASSERT_TRUE(db != NULL);

    ASSERT_EQ(EINVAL, ark_nextN(ari, KV_64K, buf, &keyN));
    ASSERT_NE(keyN, 0);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_TRUE(ari == NULL);

    ASSERT_EQ(EINVAL, ark_nextN(ari,  0, buf, &keyN));
    ASSERT_NE(keyN, 0);
    ASSERT_EQ(EINVAL, ark_nextN(ari,  KV_64K, null, &keyN));
    ASSERT_NE(keyN, 0);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ari = ark_first(ark, KV_4K, &klen, gvalue);
    ASSERT_NE(ari,  (ARI*)NULL);
    ASSERT_NE(klen, 0);
    ++num;
    ASSERT_EQ(0, kv_db_delete(db, LEN, gvalue, klen));
    ASSERT_EQ(0, ark_exists(ark, klen, gvalue, &res));
    ASSERT_NE(0, res);

    /* use ark_next to query all key/value pairs from the ark */
    while (num < LEN)
    {
        ASSERT_EQ(0, ark_nextN(ari, KV_64K, buf, &keyN));
        int i=0;
        num += keyN;
        keye = (keye_t*)buf;
        for (i=0; i<keyN; i++)
        {
            ASSERT_TRUE(keye->len >= 0);
            ASSERT_TRUE(keye->len < 257);
            ASSERT_EQ(0, kv_db_delete(db, LEN, keye->p, keye->len));
            ASSERT_EQ(0, ark_exists(ark, keye->len, keye->p, &res));
            ASSERT_NE(0, res);
            keye = BMP_KEYE(keye);
        }
    }
    ASSERT_EQ(LEN, num);

    // query LEN+1
    ASSERT_EQ(ENOENT, ark_nextN(ari, KV_64K, buf, &keyN));
    ASSERT_EQ(0,keyN);

    kv_db_destroy(db, LEN);

    ASSERT_EQ(0, ark_delete(ark));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_allocated_inuse_actual)
{
    ARK     *ark        = NULL;
    _ARK    *park       = NULL;
    uint64_t zero       = 0;
    uint64_t ret        = 0;
    uint64_t expect     = 0;
    int64_t  res        = 0;
    uint64_t bs         = 0;
    uint64_t kl         = 0;
    uint64_t vl         = 0;
    uint32_t LEN        = 1000;
    uint8_t  key[1000]  = {0};
    uint8_t  val[10000] = {0};
    char    *dev        = getenv("FVT_DEV");

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    100,
                                    6,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));
    park = (_ARK *)ark;

    /*-------------------------------------------------------------------------------*/
    kl=10; vl=10; LEN=15000;
    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes, since the data pattern is the same each run */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); expect=524288004;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); expect=417792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); expect=330816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes */
    EXPECT_EQ(0, ark_allocated(ark, &ret));  expect=524288004;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_inuse    (ark, &ret));  expect=417792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret));  expect=330816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sdel        (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery_empty(ark, seed, kl, LEN);

    /* empty */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > 0);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /*-------------------------------------------------------------------------------*/
    kl=10; vl=3000; LEN=15000;
    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes, since the data pattern is the same each run */
    EXPECT_EQ(0, ark_allocated(ark, &ret));    expect=524288004;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_inuse    (ark, &ret));    expect=61857792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret));    expect=45315816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes */
    EXPECT_EQ(0, ark_allocated(ark, &ret));    expect=524288004;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_inuse    (ark, &ret));    expect=61857792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret));    expect=45315816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sdel        (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery_empty(ark, seed, kl, LEN);

    /* empty */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > zero);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /*-------------------------------------------------------------------------------*/
    kl=300; vl=4000; LEN=1000;
    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes, since the data pattern is the same each run */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > 4464640);
    EXPECT_EQ(0, ark_inuse    (ark, &ret));
    expect=4513792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret));
    expect=4312816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > 4464640);
    EXPECT_EQ(0, ark_inuse    (ark, &ret));
    expect=4513792;
    EXPECT_EQ(ret, expect);
    EXPECT_EQ(0, ark_actual   (ark, &ret));
    expect=4312816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sdel        (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery_empty(ark, seed, kl, LEN);

    /* empty */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > zero);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /*-------------------------------------------------------------------------------*/
    kl=4000; vl=4000; LEN=1000;
    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > 8192000);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_TRUE(ret == 8192000);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); expect=8012816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);

    /* check blocks and bytes */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > 8192000);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_TRUE(ret == 8192000);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); expect=8012816;
    EXPECT_EQ(ret, expect);

    fvt_kv_utils_sdel        (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery_empty(ark, seed, kl, LEN);

    /* empty */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > zero);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /*-------------------------------------------------------------------------------*/
    kl=3745; vl=251;
    fvt_kv_utils_sload       (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery      (ark, seed, kl, vl, LEN);
    fvt_kv_utils_sdel        (ark, seed, kl, vl, LEN);
    fvt_kv_utils_squery_empty(ark, seed, kl, LEN);

    /* empty */
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > zero);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /* add k/v */
    kl=10; vl=20;
    GEN_VAL(key, seed, kl);
    GEN_VAL(val, seed, vl);
    bs   = park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 48);

    /* del k/v */
    EXPECT_EQ(0, ark_del      (ark, kl, key, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > zero);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    /* add k/v */
    kl=100; vl=200;
    GEN_VAL(key, seed+1, kl);
    GEN_VAL(val, seed+1, vl);
    bs   = park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 319);

    /* add 2nd k/v */
    kl=150; vl=2000;
    GEN_VAL(key, seed+2, kl);
    GEN_VAL(val, seed+2, vl);
    bs  += 2*park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 2497);

    /* add 3rd k/v */
    kl=200; vl=250;
    GEN_VAL(key, seed+3, kl);
    GEN_VAL(val, seed+3, vl);
    bs  += park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 2967);

    /* rep 2nd k/v, VDF->INLINE */
    kl=150; vl=10;
    GEN_VAL(key, seed+2, kl);
    GEN_VAL(val, seed+2, vl);
    bs  -= park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 968);

    /* rep 2nd k/v, INLINE->VDF */
    kl=150; vl=9000;
    GEN_VAL(key, seed+2, kl);
    GEN_VAL(val, seed+2, vl);
    bs  += 3*park->bsize;
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 9967);

    /* rep 3rd k/v INLINE->INLINE*/
    kl=200; vl=20;
    GEN_VAL(key, seed+3, kl);
    GEN_VAL(val, seed+3, vl);
    EXPECT_EQ(0, ark_set(ark, kl, key, vl, val, &res));
    EXPECT_EQ(res, (int64_t)vl);
    EXPECT_EQ(0, ark_allocated(ark, &ret)); EXPECT_TRUE(ret > bs);
    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, bs);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_TRUE(ret == 9736);

    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_long_hash_list_smallkv)
{
    ARK     *ark  = NULL;
    kv_t    *db   = NULL;
    int32_t  LEN  = 4000;
    int      klen = 16;
    int      vlen = 32;
    char    *dev  = getenv("FVT_DEV");

#ifdef _AIX
    LEN=2000;
#endif

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    64,
                                    ARK_VERBOSE_NTHRDS_DEF,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN|ARK_KV_HTC));

    db = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_long_hash_list_bigkv)
{
    ARK     *ark  = NULL;
    kv_t    *db   = NULL;
    int32_t  LEN  = 5000;
    int      klen = 512;
    int      vlen = 512;
    char    *dev  = getenv("FVT_DEV");

#ifdef _AIX
    LEN=2000;
#endif

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    16,
                                    6,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    db = (kv_t*)kv_db_create_fixed(LEN, klen, vlen);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, vlen, LEN);
    fvt_kv_utils_del  (ark, db, LEN);

    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 *   load keys, ark_random, verify key is valid and different from last key,
 *    delete ark
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_flush)
{
    ARK     *ark           = NULL;
    kv_t    *db            = NULL;
    uint32_t LEN           = 10000;
    uint64_t klen          = 0;
    uint64_t zero          = 0;
    uint64_t ret           = 0;
    char     kvalue[KV_4K] = {0};

    ARK_CREATE;

    ASSERT_EQ(ENOENT, ark_random(ark, KV_4K, &klen, kvalue));

    db   = (kv_t*)kv_db_create_fixed(LEN, 8, 24);
    ASSERT_TRUE(db != NULL);

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    ASSERT_EQ(0, ark_flush(ark));

    EXPECT_EQ(0, ark_inuse    (ark, &ret)); EXPECT_EQ(ret, zero);
    EXPECT_EQ(0, ark_actual   (ark, &ret)); EXPECT_EQ(ret, zero);

    ASSERT_EQ(ENOENT, ark_random(ark, KV_4K, &klen, kvalue));

    fvt_kv_utils_load (ark, db, LEN);
    fvt_kv_utils_query(ark, db, KV_4K, LEN);

    fvt_kv_utils_del(ark, db, LEN);
    kv_db_destroy(db, LEN);
    ARK_DELETE;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
TEST(FVT_KV_GOOD_PATH, SIMPLE_cleanup_task_memory)
{
    ARK     *ark   = NULL;
    int32_t  LEN   = 3000;
    int      klen1 = 17;
    int      vlen1 = 31;
    int      klen2 = 23;
    int      vlen2 = 9382;
    int      i     = 0;
    int      loops = 6;
    char    *dev   = getenv("FVT_DEV");

#ifdef _AIX
    LEN=400;
#endif

    ASSERT_EQ(0, ark_create_verbose(dev, &ark,
                                    ARK_VERBOSE_SIZE_DEF*100,
                                    ARK_VERBOSE_BSIZE_DEF,
                                    4,
                                    6,
                                    ARK_MAX_NASYNCS,
                                    ARK_MAX_BASYNCS,
                                    ARK_KV_VIRTUAL_LUN));

    if (dev != NULL) {loops=3;}

    for (i=0; i<loops; i++)
    {
        fvt_kv_utils_sload       (ark, seed, klen1, vlen1, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_squery      (ark, seed, klen1, vlen1, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_sload       (ark, seed, klen2, vlen2, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_squery      (ark, seed, klen2, vlen2, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_sdel        (ark, seed, klen2, vlen2, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_squery_empty(ark, seed, klen2, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_sdel        (ark, seed, klen1, vlen1, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
        fvt_kv_utils_squery_empty(ark, seed, klen1, LEN);
        usleep(ARK_CLEANUP_DELAY*2000); fflush(stdout);
    }
    ARK_DELETE;
}
