/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_sync_pth.h $                         */
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
 *   functions to aid in testing the async kv ark functions
 * \details
 * \ingroup
 ******************************************************************************/
#ifndef KV_SYNC_PTH_H
#define KV_SYNC_PTH_H

extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
}

#define MAX_PTH_PER_CONTEXT 256

/**
 *******************************************************************************
 * \brief
 *    struct to contain args passed to child
 ******************************************************************************/
typedef struct
{
    ARK      *ark;
    kv_t     *db;
    int32_t   vlen;
    int32_t   LEN;
    int32_t   secs;
    uint32_t  mb;
    uint32_t  ops;
    pthread_t pth;
} set_get_args_t;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
class Sync_pth
{
  protected:
    static void set(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        ARK     *ark = sg_args->ark;
        kv_t    *db  = sg_args->db;
        int64_t res  = 0;
        int32_t i    = 0;

        /* load all key/value pairs from the fixed db into the ark */
        for (i=0; i<sg_args->LEN; i++)
        {
            EXPECT_EQ(0, ark_set(ark, db[i].klen, db[i].key, db[i].vlen,
                                 db[i].value, &res));
            EXPECT_EQ(db[i].vlen, res);
        }
    }

    static void get(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        ARK      *ark   = sg_args->ark;
        kv_t     *db    = sg_args->db;
        int64_t  res    = 0;
        int32_t  i      = 0;
        uint8_t *gvalue = NULL;

        gvalue = (uint8_t*)malloc(sg_args->vlen);
        ASSERT_TRUE(gvalue != NULL);

        /* query all key/value pairs from the fixed db */
        for (i=sg_args->LEN-1; i>=0; i--)
        {
            EXPECT_EQ(0, ark_get(ark, db[i].klen, db[i].key, db[i].vlen,
                                 gvalue, 0, &res));
            EXPECT_EQ(db[i].vlen, res);
            EXPECT_EQ(0, memcmp(db[i].value,gvalue,db[i].vlen));
            EXPECT_EQ(0, ark_exists(ark, db[i].klen, db[i].key, &res));
            EXPECT_EQ(db[i].vlen, res);
        }
        free(gvalue);
    }

    static void del(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        ARK     *ark = sg_args->ark;
        kv_t    *db  = sg_args->db;
        int64_t res  = 0;
        int32_t i    = 0;

        /* delete all key/value pairs from the fixed db */
        for (i=0; i<sg_args->LEN; i++)
        {
            EXPECT_EQ(0, ark_del(ark, db[i].klen, db[i].key, &res));
            EXPECT_EQ(db[i].vlen, res);
        }
    }

    static void SGD(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pFT, "RUN_PTH 0 minutes");

        do
        {
            if (cur > next)
            {
                KV_TRC(pFT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            set(args);
            get(args);
            del(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        KV_TRC(pFT, "RUN_PTH DONE");
    }

    static void read_loop(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pFT, "RUN_PTH 0 minutes");

        set(args);

        do
        {
            if (cur > next)
            {
                KV_TRC(pFT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            get(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        del(args);

        KV_TRC(pFT, "RUN_PTH DONE");
    }

    static void write_loop(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pFT, "RUN_PTH 0 minutes");

        do
        {
            if (cur > next)
            {
                KV_TRC(pFT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            set(args);
            del(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        KV_TRC(pFT, "RUN_PTH DONE");
    }

  public:
    void run_set       (set_get_args_t *args);
    void run_get       (set_get_args_t *args);
    void run_del       (set_get_args_t *args);
    void run_SGD       (set_get_args_t *args);
    void run_read_loop (set_get_args_t *args);
    void run_write_loop(set_get_args_t *args);

    void wait(set_get_args_t *args);

    void run_multi_ctxt_rd(uint32_t num_ctxt,
                           uint32_t num_pth,
                           uint32_t npool,
                           uint32_t vlen,
                           uint32_t LEN,
                           uint32_t secs);

    void run_multi_ctxt_wr(uint32_t num_ctxt,
                           uint32_t num_pth,
                           uint32_t npool,
                           uint32_t vlen,
                           uint32_t LEN,
                           uint32_t secs);

    void run_multi_ctxt(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t vlen,
                        uint32_t LEN,
                        uint32_t secs);

    void run_multi_ctxt(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t npool,
                        uint32_t vlen,
                        uint32_t LEN,
                        uint32_t secs);

    void run_multi_ctxt(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t vlen,
                        uint32_t LEN,
                        uint32_t secs,
                        uint32_t *p_ops,
                        uint32_t *p_ios);

    void run_multi_ctxt(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t npool,
                        uint32_t vlen,
                        uint32_t LEN,
                        uint32_t secs,
                        uint32_t *p_ops,
                        uint32_t *p_ios);

};

#endif
