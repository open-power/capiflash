/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_ark_io.C $                           */
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
 * \ingroup
 ******************************************************************************/
extern "C"
{
#include <fvt_kv.h>
#include <kv_utils_db.h>
#include <fvt_kv_utils_ark_io.h>

}
void kv_async_get_arks(ARK *arks[], uint32_t num_ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_ark_io::run_SGD(ark_io_args_t *args)
{
    uint32_t  i          = 0;
    uint32_t  rc         = 0;
    void*   (*fp)(void*) = (void*(*)(void*))SGD;

    args->pth = 0;
    for (i=0; i<5000; i++)
    {
        if (0 == (rc=pthread_create(&args->pth,
                            NULL,
                            fp,
                            (void*)args))) break;
        usleep(10000);
    }
    if (rc)
    {
        args->pth = 0;
        printf("."); fflush(stdout);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_ark_io::wait(ark_io_args_t *args)
{
    if (args->pth) (void)pthread_join(args->pth, NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_ark_io::run_multi_arks(uint32_t  num_ctxt,
                                 uint32_t  num_pth,
                                 uint32_t  vlen,
                                 uint32_t  secs)
{
    uint32_t        i        = 0;
    uint32_t        ctxt_i   = 0;
    uint32_t        pth_i    = 0;
    uint32_t        klen     = KV_4K; //must be large, to not dup async db klens
    uint32_t        LEN      = 50;
    uint32_t        tot_pth  = num_ctxt * num_pth;
    ark_io_args_t   pth_args[tot_pth];
    ARK            *ark[num_ctxt];
    kv_t           *db[100];
    struct timeval  stop, start;
    uint64_t        ops      = 0;
    uint64_t        ios      = 0;
    uint32_t        t_ops    = 0;
    uint32_t        t_ios    = 0;

    memset(pth_args, 0, sizeof(ark_io_args_t) * tot_pth);

    /* alloc one set of db's, to be used for each context */
    for (i=0; i<num_pth; i++)
    {
        db[i] = (kv_t*)kv_db_create_fixed(LEN, klen+i, vlen+i);
    }

    kv_async_get_arks(ark, num_ctxt);

    /* start timer */
    gettimeofday(&start, NULL);

    /* init each pth per context and run the pth */
    ctxt_i = 0;
    i      = 0;
    while (i < tot_pth)
    {
        pth_args[i].ark  = ark[ctxt_i];
        pth_args[i].db   = db[pth_i];
        pth_args[i].vlen = vlen+pth_i;
        pth_args[i].LEN  = LEN;
        pth_args[i].secs = secs;
        run_SGD(pth_args+i);
        ++pth_i;
        ++i;
        if ((i%num_pth) == 0)
        {
            /* reset for next context */
            ++ctxt_i;
            pth_i = 0;
        }
    }
    for (i=0; i<tot_pth; i++)
    {
        wait(pth_args+i);
    }

    /* stop timer */
    gettimeofday(&stop, NULL);
    secs = stop.tv_sec - start.tv_sec;

    t_ops = 0;
    t_ios = 0;

    for (i=0; i<num_ctxt; i++)
    {
        (void)ark_stats(ark[i], &ops, &ios);
        t_ops += (uint32_t)ops;
        t_ios += (uint32_t)ios;
    }
    for (i=0; i<num_pth; i++)
    {
        kv_db_destroy(db[i], LEN);
    }
    t_ops = t_ops / secs;
    t_ios = t_ios / secs;
    printf("SYNC %dx%dx%d ctxt:%d pth:%d op/s:%d io/s:%d secs:%d\n",
            klen, vlen, LEN, num_ctxt, num_pth, t_ops, t_ios, secs);
}
