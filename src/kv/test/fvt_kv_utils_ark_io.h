/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_ark_io.h $                           */
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
 *   functions to aid in testing the sync kv ark functions
 * \details
 * \ingroup
 ******************************************************************************/
#ifndef KV_UTILS_ARK_IO_H
#define KV_UTILS_ARK_IO_H

extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
}

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
} ark_io_args_t;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
class Sync_ark_io
{
  protected:
    static void set(void *args)
    {
        ark_io_args_t *sg_args = (ark_io_args_t*)args;
        ARK     *ark = sg_args->ark;
        kv_t    *db  = sg_args->db;
        int64_t res  = 0;
        int32_t i    = 0;

        /* load all key/value pairs from the fixed db into the ark */
        for (i=0; i<sg_args->LEN; i++)
        {
            if (0 != ark_set(ark, db[i].klen, db[i].key, db[i].vlen,
                                  db[i].value, &res))
                printf("ark_set failed\n");

            if (db[i].vlen != res)
                printf("ark_set bad length\n");
        }
    }

    static void get(void *args)
    {
        ark_io_args_t *sg_args = (ark_io_args_t*)args;
        ARK      *ark   = sg_args->ark;
        kv_t     *db    = sg_args->db;
        int64_t  res    = 0;
        int32_t  i      = 0;
        uint8_t *gvalue = NULL;

        gvalue = (uint8_t*)malloc(sg_args->vlen);
        assert(gvalue);

        /* query all key/value pairs from the fixed db */
        for (i=sg_args->LEN-1; i>=0; i--)
        {
            if (0 != ark_get(ark, db[i].klen, db[i].key, db[i].vlen,
                                 gvalue, 0, &res))
                printf("ark_get failed\n");

            if (db[i].vlen != res)
                printf("ark_get bad length\n");

            if (0 != memcmp(db[i].value,gvalue,db[i].vlen))
                printf("ark_get miscompare\n");

            if (0 != ark_exists(ark, db[i].klen, db[i].key, &res))
                printf("ark_exists failed\n");

            if (db[i].vlen != res)
                printf("ark_exists bad length\n");
        }
        free(gvalue);
    }

    static void del(void *args)
    {
        ark_io_args_t *sg_args = (ark_io_args_t*)args;
        ARK     *ark = sg_args->ark;
        kv_t    *db  = sg_args->db;
        int64_t res  = 0;
        int32_t i    = 0;

        /* delete all key/value pairs from the fixed db */
        for (i=0; i<sg_args->LEN; i++)
        {
            if (0 != ark_del(ark, db[i].klen, db[i].key, &res))
                printf("ark_del failed\n");

            if (db[i].vlen != res)
                printf("ark_del bad length\n");
        }
    }

    static void SGD(void *args)
    {
        ark_io_args_t *sg_args = (ark_io_args_t*)args;
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

  public:
    void run_SGD(ark_io_args_t *args);

    void wait(ark_io_args_t *args);

    void run_multi_arks(uint32_t num_ctxt,
                        uint32_t num_pth,
                        uint32_t vlen,
                        uint32_t secs);
};

#endif
