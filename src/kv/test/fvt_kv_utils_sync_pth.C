/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_sync_pth.C $                         */
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
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>
#include <fvt_kv_utils_sync_pth.h>

extern "C"
{
#include <fvt_kv.h>
#include <arkdb_trace.h>
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_set(set_get_args_t *args)
{
    void*   (*fp)(void*) = (void*(*)(void*))set;
    EXPECT_EQ(0, pthread_create(&args->pth,
                                NULL,
                                fp,
                                (void*)args));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_get(set_get_args_t *args)
{
    void*   (*fp)(void*) = (void*(*)(void*))get;
    EXPECT_EQ(0, pthread_create(&args->pth,
                                NULL,
                                fp,
                                (void*)args));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_del(set_get_args_t *args)
{
    void*   (*fp)(void*) = (void*(*)(void*))del;
    EXPECT_EQ(0, pthread_create(&args->pth,
                                NULL,
                                fp,
                                (void*)args));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_SGD(set_get_args_t *args)
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
void Sync_pth::run_read_loop(set_get_args_t *args)
{
    uint32_t  i          = 0;
    uint32_t  rc         = 0;
    void*   (*fp)(void*) = (void*(*)(void*))read_loop;

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
void Sync_pth::run_write_loop(set_get_args_t *args)
{
    uint32_t  i          = 0;
    uint32_t  rc         = 0;
    void*   (*fp)(void*) = (void*(*)(void*))write_loop;

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
void Sync_pth::wait(set_get_args_t *args)
{
    if (args->pth)
        EXPECT_EQ(0, pthread_join(args->pth, NULL));
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt(uint32_t num_ctxt,
                              uint32_t num_pth,
                              uint32_t vlen,
                              uint32_t LEN,
                              uint32_t secs)
{
    uint32_t ops = 0;
    uint32_t ios = 0;

    run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs, &ops, &ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt(uint32_t num_ctxt,
                              uint32_t num_pth,
                              uint32_t npool,
                              uint32_t vlen,
                              uint32_t LEN,
                              uint32_t secs)
{
    uint32_t ops = 0;
    uint32_t ios = 0;

    run_multi_ctxt(num_ctxt, num_pth, vlen, LEN, secs, &ops, &ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt(uint32_t num_ctxt,
                              uint32_t num_pth,
                              uint32_t nasync,
                              uint32_t basync,
                              uint32_t vlen,
                              uint32_t LEN,
                              uint32_t secs)
{
    uint32_t ops = 0;
    uint32_t ios = 0;

    run_multi_ctxt(num_ctxt,num_pth,1,nasync,basync,vlen,LEN, secs, &ops, &ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt(uint32_t  num_ctxt,
                              uint32_t  num_pth,
                              uint32_t  vlen,
                              uint32_t  LEN,
                              uint32_t  secs,
                              uint32_t *p_ops,
                              uint32_t *p_ios)
{
    run_multi_ctxt(num_ctxt, num_pth, 1, 512, KV_8K, vlen,LEN,secs,p_ops,p_ios);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt(uint32_t  num_ctxt,
                              uint32_t  num_pth,
                              uint32_t  npool,
                              uint32_t  nasync,
                              uint32_t  basync,
                              uint32_t  vlen,
                              uint32_t  LEN,
                              uint32_t  secs,
                              uint32_t *p_ops,
                              uint32_t *p_ios)
{
    uint32_t        i        = 0;
    uint32_t        ctxt_i   = 0;
    uint32_t        pth_i    = 0;
    uint32_t        tot_pth  = num_ctxt * num_pth;
    set_get_args_t  pth_args[tot_pth];
    ARK            *ark[num_ctxt];
    struct timeval  stop, start;
    uint64_t        ops      = 0;
    uint64_t        ios      = 0;

    if (num_pth > MAX_PTH_PER_CONTEXT)
    {
        printf("cannot exceed %d pthreads for sync ops\n", MAX_PTH_PER_CONTEXT);
        return;
    }

    memset(pth_args, 0, sizeof(set_get_args_t) * tot_pth);

    /* alloc one ark for each context */
    for (ctxt_i=0; ctxt_i<num_ctxt; ctxt_i++)
    {
        ASSERT_EQ(0, ark_create_verbose(getenv("FVT_DEV"), &ark[ctxt_i],
                                        1048576,
                                        4096,
                                        1048576,
                                        npool,
                                        nasync,
                                        basync,
                                        ARK_KV_VIRTUAL_LUN));
        ASSERT_TRUE(NULL != ark[ctxt_i]);
    }

    /* start timer */
    gettimeofday(&start, NULL);

    /* init each pth per context and run the pth */
    ctxt_i = 0;
    i      = 0;
    while (i < tot_pth)
    {
        pth_args[i].key  = (char*)malloc(KLEN);
        pth_args[i].val  = (char*)malloc(vlen);
        pth_args[i].gval = (char*)malloc(vlen);
        pth_args[i].ark  = ark[ctxt_i];
        pth_args[i].n    = i*LEN;
        pth_args[i].vlen = vlen;
        pth_args[i].LEN  = LEN;
        pth_args[i].secs = secs;
        KV_TRC(pAT, "START   n:%d vl:%d LEN:%d",
                    pth_args[i].n, pth_args[i].vlen, pth_args[i].LEN);
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
    printf("SYNC %dx%dx%d ctxt:%d pth:%d npool:%d ",
            KLEN,vlen, LEN, num_ctxt, num_pth, npool); fflush(stdout);
    for (i=0; i<tot_pth; i++)
    {
        wait(pth_args+i);
        if (pth_args[i].key)  {free(pth_args[i].key);}
        if (pth_args[i].val)  {free(pth_args[i].val);}
        if (pth_args[i].gval) {free(pth_args[i].gval);}
    }

    /* stop timer */
    gettimeofday(&stop, NULL);
    secs = stop.tv_sec - start.tv_sec;

    *p_ops = 0;
    *p_ios = 0;

    for (i=0; i<num_ctxt; i++)
    {
        (void)ark_stats(ark[i], &ops, &ios);
        *p_ops += (uint32_t)ops;
        *p_ios += (uint32_t)ios;
        EXPECT_EQ(0, ark_delete(ark[i]));
    }
    *p_ops = *p_ops / secs;
    *p_ios = *p_ios / secs;
    printf("op/s:%d io/s:%d secs:%d\n", *p_ops, *p_ios, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt_rd(uint32_t  num_ctxt,
                                 uint32_t  num_pth,
                                 uint32_t  npool,
                                 uint32_t  vlen,
                                 uint32_t  LEN,
                                 uint32_t  secs)
{
    uint32_t        i        = 0;
    uint32_t        ctxt_i   = 0;
    uint32_t        pth_i    = 0;
    uint32_t        klen     = 16;
    uint32_t        tot_pth  = num_ctxt * num_pth;
    set_get_args_t  pth_args[tot_pth];
    ARK            *ark[num_ctxt];
    struct timeval  stop, start;
    uint64_t        ops      = 0;
    uint64_t        ios      = 0;
    uint32_t        t_ops    = 0;
    uint32_t        t_ios    = 0;

    if (num_pth > MAX_PTH_PER_CONTEXT)
    {
        printf("cannot exceed %d pthreads for sync ops\n", MAX_PTH_PER_CONTEXT);
        return;
    }

    memset(pth_args, 0, sizeof(set_get_args_t) * tot_pth);

    /* alloc one ark for each context */
    for (ctxt_i=0; ctxt_i<num_ctxt; ctxt_i++)
    {
        ASSERT_EQ(0, ark_create_verbose(getenv("FVT_DEV"), &ark[ctxt_i],
                                        1048576,
                                        4096,
                                        1048576,
                                        npool,
                                        256,
                                        8*1024,
                                        ARK_KV_VIRTUAL_LUN));
        ASSERT_TRUE(NULL != ark[ctxt_i]);
    }

    /* start timer */
    gettimeofday(&start, NULL);

    /* init each pth per context and run the pth */
    ctxt_i = 0;
    i      = 0;
    while (i < tot_pth)
    {
        pth_args[i].ark  = ark[ctxt_i];
        pth_args[i].n    = i*LEN;
        pth_args[i].vlen = vlen+pth_i;
        pth_args[i].LEN  = LEN;
        pth_args[i].secs = secs;
        run_read_loop(pth_args+i);
        ++pth_i;
        ++i;
        if ((i%num_pth) == 0)
        {
            /* reset for next context */
            ++ctxt_i;
            pth_i = 0;
        }
    }
    printf("SYNC %dx%dx%d ctxt:%d pth:%d npool:%d ",
                     klen, vlen, LEN, num_ctxt, num_pth, npool); fflush(stdout);
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
        EXPECT_EQ(0, ark_delete(ark[i]));
    }
    t_ops = t_ops / secs;
    t_ios = t_ios / secs;
    printf("op/s:%d io/s:%d secs:%d\n", t_ops, t_ios, secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void Sync_pth::run_multi_ctxt_wr(uint32_t  num_ctxt,
                                 uint32_t  num_pth,
                                 uint32_t  npool,
                                 uint32_t  vlen,
                                 uint32_t  LEN,
                                 uint32_t  secs)
{
    uint32_t        i        = 0;
    uint32_t        ctxt_i   = 0;
    uint32_t        pth_i    = 0;
    uint32_t        klen     = 16;
    uint32_t        tot_pth  = num_ctxt * num_pth;
    set_get_args_t  pth_args[tot_pth];
    ARK            *ark[num_ctxt];
    struct timeval  stop, start;
    uint64_t        ops      = 0;
    uint64_t        ios      = 0;
    uint32_t        t_ops    = 0;
    uint32_t        t_ios    = 0;

    if (num_pth > MAX_PTH_PER_CONTEXT)
    {
        printf("cannot exceed %d pthreads for sync ops\n", MAX_PTH_PER_CONTEXT);
        return;
    }

    memset(pth_args, 0, sizeof(set_get_args_t) * tot_pth);

    /* alloc one ark for each context */
    for (ctxt_i=0; ctxt_i<num_ctxt; ctxt_i++)
    {
        ASSERT_EQ(0, ark_create_verbose(getenv("FVT_DEV"), &ark[ctxt_i],
                                        1048576,
                                        4096,
                                        1048576,
                                        npool,
                                        256,
                                        8*1024,
                                        ARK_KV_VIRTUAL_LUN));
        ASSERT_TRUE(NULL != ark[ctxt_i]);
    }

    /* start timer */
    gettimeofday(&start, NULL);

    /* init each pth per context and run the pth */
    ctxt_i = 0;
    i      = 0;
    while (i < tot_pth)
    {
        pth_args[i].ark  = ark[ctxt_i];
        pth_args[i].n    = i*LEN;
        pth_args[i].vlen = vlen+pth_i;
        pth_args[i].LEN  = LEN;
        pth_args[i].secs = secs;
        run_write_loop(pth_args+i);
        ++pth_i;
        ++i;
        if ((i%num_pth) == 0)
        {
            /* reset for next context */
            ++ctxt_i;
            pth_i = 0;
        }
    }
    printf("SYNC %dx%dx%d ctxt:%d pth:%d npool:%d ",
                     klen, vlen, LEN, num_ctxt, num_pth, npool); fflush(stdout);
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
        EXPECT_EQ(0, ark_delete(ark[i]));
    }
    t_ops = t_ops / secs;
    t_ios = t_ios / secs;
    printf("op/s:%d io/s:%d secs:%d\n", t_ops, t_ios, secs);
}
