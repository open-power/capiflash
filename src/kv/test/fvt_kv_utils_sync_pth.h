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
#include <errno.h>
#include <ark.h>
#include <arkdb_trace.h>
}

#define KLEN 4

/**
 *******************************************************************************
 * \brief
 *    struct to contain args passed to child
 ******************************************************************************/
typedef struct
{
    ARK      *ark;
    char     *key;
    char     *val;
    char     *gval;
    uint32_t  n;
    int32_t   vlen;
    uint32_t  LEN;
    int32_t   secs;
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
        ARK            *ark     = sg_args->ark;
        int64_t         res     = 0;
        uint32_t        i       = 0;
        int             rc      = 0;

        for (i=0; i<sg_args->LEN; i++)
        {
            GEN_VAL(sg_args->key, sg_args->n+i, KLEN);
            GEN_VAL(sg_args->val, sg_args->n+i, sg_args->vlen);
            while (EAGAIN == (rc=ark_set(ark,
                                         KLEN,
                                         sg_args->key,
                                         sg_args->vlen,
                                         sg_args->val,
                                         &res))) {usleep(10000);}
            EXPECT_EQ(0, rc);
            EXPECT_EQ(sg_args->vlen, res);
        }
    }

    static void get(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        ARK            *ark     = sg_args->ark;
        int64_t         res     = 0;
        uint32_t        i       = 0;
        int             rc      = 0;

        for (i=0; i<sg_args->LEN; i++)
        {
            GEN_VAL(sg_args->key, sg_args->n+i, KLEN);
            GEN_VAL(sg_args->val, sg_args->n+i, sg_args->vlen);

            while (EAGAIN == (rc=ark_get(ark,
                                         KLEN,
                                         sg_args->key,
                                         sg_args->vlen,
                                         sg_args->gval,
                                         0,
                                         &res))) {usleep(10000);}
            EXPECT_EQ(0, rc);
            EXPECT_EQ(sg_args->vlen, res);
            EXPECT_EQ(0, memcmp(sg_args->val,sg_args->gval,sg_args->vlen));
            while (EAGAIN == (rc=ark_exists(ark,
                                            KLEN,
                                            sg_args->key,
                                            &res))) {usleep(10000);}
            EXPECT_EQ(0, rc);
            EXPECT_EQ(sg_args->vlen, res);
        }
    }

    static void del(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        ARK            *ark     = sg_args->ark;
        int64_t         res     = 0;
        uint32_t        i       = 0;
        int             rc      = 0;
        uint8_t         key[KLEN];

        for (i=0; i<sg_args->LEN; i++)
        {
            GEN_VAL(key, sg_args->n+i, KLEN);

            while (EAGAIN == (rc=ark_del(ark,
                                         KLEN,
                                         key,
                                         &res))) {usleep(10000);}
            EXPECT_EQ(0, rc);
            EXPECT_EQ(sg_args->vlen, res);
        }
    }

    static void SGD(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pAT, "RUN_PTH 0 minutes");

        do
        {
            if (cur > next)
            {
                KV_TRC(pAT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            set(args);
            get(args);
            del(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        KV_TRC(pAT, "RUN_PTH DONE");
    }

    static void read_loop(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pAT, "RUN_PTH 0 minutes");

        set(args);

        do
        {
            if (cur > next)
            {
                KV_TRC(pAT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            get(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        del(args);

        KV_TRC(pAT, "RUN_PTH DONE");
    }

    static void write_loop(void *args)
    {
        set_get_args_t *sg_args = (set_get_args_t*)args;
        int32_t         start   = time(0);
        int32_t         next    = start + 600;
        int32_t         cur     = 0;

        KV_TRC(pAT, "RUN_PTH 0 minutes");

        do
        {
            if (cur > next)
            {
                KV_TRC(pAT, "RUN_PTH %d minutes", (next-start)/60);
                next += 600;
            }

            set(args);
            del(args);

            cur = time(0);
        }
        while (cur-start < sg_args->secs);

        KV_TRC(pAT, "RUN_PTH DONE");
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
                        uint32_t vlen,
                        uint32_t nasync,
                        uint32_t basync,
                        uint32_t LEN,
                        uint32_t secs);

    void run_multi_ctxt(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t nasync,
                        uint32_t basync,
                        uint32_t npool,
                        uint32_t vlen,
                        uint32_t LEN,
                        uint32_t secs,
                        uint32_t *p_ops,
                        uint32_t *p_ios);

};

#endif
