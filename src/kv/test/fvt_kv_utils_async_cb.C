/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_async_cb.C $                         */
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
 *   utility functions for async ark functions
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
#include <fvt_kv.h>
#include <fvt_kv_utils_async_cb.h>
#include <kv_utils_db.h>
#include <kv_inject.h>
#include <errno.h>
}

#define KV_ASYNC_EASY           16
#define KV_ASYNC_BIG_BLOCKS     64
#define KV_ASYNC_LOW_STRESS     128
#define KV_ASYNC_HIGH_STRESS    512
#define KV_ASYNC_JOB_Q          KV_ASYNC_HIGH_STRESS
#define KV_ASYNC_MAX_CONTEXTS   500

#define KV_ASYNC_CT_RUNNING        0x80000000
#define KV_ASYNC_CT_PERF           0x40000000
#define KV_ASYNC_CT_DONE           0x20000000
#define KV_ASYNC_CT_ERROR_INJECT   0x10000000

#define KV_ASYNC_CB_REPLACE        0x80000000
#define KV_ASYNC_CB_SGD            0x40000000
#define KV_ASYNC_CB_READ_PERF      0x20000000
#define KV_ASYNC_CB_WRITE_PERF     0x10000000
#define KV_ASYNC_CB_RUNNING        0x08000000
#define KV_ASYNC_CB_QUEUED         0x04000000
#define KV_ASYNC_CB_SHUTDOWN       0x02000000
#define KV_ASYNC_CB_MULTI_CTXT_IO  0x01000000
#define KV_ASYNC_CB_SET            0x00800000
#define KV_ASYNC_CB_GET            0x00400000
#define KV_ASYNC_CB_EXISTS         0x00200000
#define KV_ASYNC_CB_DEL            0x00100000
#define KV_ASYNC_CB_GTEST          0x00080000

#define B_MARK 0xBEEFED0B
#define E_MARK 0xBEEFED0E

#define IS_GTEST (pCB->flags & KV_ASYNC_CB_GTEST)

/**
 *******************************************************************************
 * \brief
 *   structs to represent a job in a context
 ******************************************************************************/
typedef struct async_CB_s
{
    uint64_t   b_mark;
    ARC       *ark;
    kv_t      *db;
    uint32_t   len;
    uint32_t   len_i;
    uint32_t   replace;
    uint32_t   flags;
    uint64_t   tag;
    uint32_t   perf_loops;
    void     (*cb)   (int, uint64_t, int64_t);
    uint32_t (*regen)(kv_t*, uint32_t, uint32_t);
    uint32_t   regen_len;
    char      *gvalue;
    uint64_t   e_mark;
} async_CB_t;

/**
 *******************************************************************************
 * \brief
 *   structs to represent a context with X jobs
 ******************************************************************************/
typedef struct async_context_s
{
    ARK       *ark;
    uint32_t   flags;
    uint32_t   secs;
    async_CB_t pCBs[KV_ASYNC_JOB_Q];
} async_context_t;

async_context_t pCTs[KV_ASYNC_MAX_CONTEXTS];
uint32_t start = 0;
uint32_t stop  = 0;

static void kv_async_SET_KEY   (async_CB_t *pCB);
static void kv_async_GET_KEY   (async_CB_t *pCB);
static void kv_async_EXISTS_KEY(async_CB_t *pCB);
static void kv_async_DEL_KEY   (async_CB_t *pCB);
static void kv_async_q_retry   (async_CB_t *pCB);
static void kv_async_dispatch  (async_CB_t *pCB);
static void kv_async_perf_done (async_CB_t *pCB);

/**
 *******************************************************************************
 * \brief
 *  callback function for set/get/exists/del
 ******************************************************************************/
static void kv_async_cb(int errcode, uint64_t dt, int64_t res)
{
    async_CB_t *pCB  = (async_CB_t*)dt;
    kv_t       *p_kv = NULL;
    uint64_t    tag  = (uint64_t)pCB;

    if (pCB == NULL)
    {
        KV_TRC_FFDC(pFT, "FFDC: pCB NULL");
        return;
    }
    if (pCB->b_mark != B_MARK)
    {
        KV_TRC_FFDC(pFT, "FFDC: B_MARK FAILURE %p: %lx", pCB, pCB->b_mark);
        return;
    }
    if (pCB->e_mark != E_MARK)
    {
        KV_TRC_FFDC(pFT, "FFDC: E_MARK FAILURE %p: %lx", pCB, pCB->e_mark);
        return;
    }
    if (EBUSY == errcode) {kv_async_q_retry(pCB); goto done;}

    if (IS_GTEST)
    {
        EXPECT_EQ(0,   errcode);
        EXPECT_EQ(tag, pCB->tag);
    }
    p_kv = pCB->db + pCB->len_i;
    ++pCB->len_i;

    if (pCB->flags & KV_ASYNC_CB_SET)
    {
        KV_TRC_IO(pFT, "KV_ASYNC_CB_SET, %p %d %d", pCB, pCB->len_i, pCB->len);
        if (0   != errcode)    printf("ark_set failed, errcode=%d\n", errcode);
        if (tag != pCB->tag)   printf("ark_set bad tag\n");
        if (res != p_kv->vlen) printf("ark_set bad vlen\n");
        if (IS_GTEST) {        EXPECT_EQ(res, p_kv->vlen);}

        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            if (pCB->flags & KV_ASYNC_CB_WRITE_PERF)
            {
                pCB->len_i = 0;
                kv_async_perf_done(pCB);
                goto done;
            }
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_CB_SET;
            pCB->flags |= KV_ASYNC_CB_GET;
            kv_async_GET_KEY(pCB);
            goto done;
        }
        kv_async_SET_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_CB_GET)
    {
        uint32_t miscompare = memcmp(p_kv->value, pCB->gvalue, p_kv->vlen);

        KV_TRC_IO(pFT, "KV_ASYNC_CB_GET, %p %d %d", pCB, pCB->len_i, pCB->len);
        if (0   != errcode)    printf("ark_get failed, errcode=%d\n", errcode);
        if (tag != pCB->tag)   printf("ark_get bad tag\n");
        if (res != p_kv->vlen) printf("ark_get bad vlen\n");
        if (IS_GTEST) {        EXPECT_TRUE(0 == miscompare);}

        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            if (pCB->flags & KV_ASYNC_CB_READ_PERF)
            {
                pCB->len_i = 0;
                kv_async_perf_done(pCB);
                goto done;
            }
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_CB_GET;
            pCB->flags |= KV_ASYNC_CB_EXISTS;
            kv_async_EXISTS_KEY(pCB);
            goto done;
        }
        kv_async_GET_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_CB_EXISTS)
    {
        KV_TRC_IO(pFT, "KV_ASYNC_CB_EXISTS, %p %d %d", pCB, pCB->len_i, pCB->len);
        if (0   != errcode)    printf("ark_exists failed,errcode=%d\n",errcode);
        if (tag != pCB->tag)   printf("ark_exists bad tag\n");
        if (res != p_kv->vlen) printf("ark_exists bad vlen\n");
        if (IS_GTEST) {        EXPECT_EQ(res, p_kv->vlen);}

        /* if end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_CB_EXISTS;

            if (pCB->flags & KV_ASYNC_CB_SGD)
            {
                pCB->flags |= KV_ASYNC_CB_DEL;
                kv_async_DEL_KEY(pCB);
                goto done;
            }
            else if (pCB->flags & KV_ASYNC_CB_REPLACE)
            {
                /* make sure we don't shutdown before we have replaced once */
                if (pCB->replace &&
                    pCB->flags & KV_ASYNC_CB_SHUTDOWN)
                {
                    pCB->flags |= KV_ASYNC_CB_DEL;
                    kv_async_DEL_KEY(pCB);
                    goto done;
                }
                pCB->replace = TRUE;
                if (0 != pCB->regen(pCB->db, pCB->len, pCB->regen_len))
                {
                    printf("regen failure, fatal\n");
                    KV_TRC_FFDC(pFT, "FFDC: regen failure");
                    memset(pCB, 0, sizeof(async_CB_t));
                    goto done;
                }
                pCB->flags |= KV_ASYNC_CB_SET;
                kv_async_SET_KEY(pCB);
                goto done;
            }
            else
            {
                /* should not be here */
                EXPECT_TRUE(0);
            }
        }
        kv_async_EXISTS_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_CB_DEL)
    {
        KV_TRC_IO(pFT, "KV_ASYNC_CB_DEL, %p i:%d len:%d", pCB, pCB->len_i,pCB->len);
        if (0   != errcode)    printf("ark_del failed, errcode=%d\n",errcode);
        if (tag != pCB->tag)   printf("ark_del bad tag\n");
        if (res != p_kv->vlen) printf("ark_del bad vlen\n");
        if (IS_GTEST) {        EXPECT_EQ(res, p_kv->vlen);}

        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            if (pCB->flags & KV_ASYNC_CB_SHUTDOWN)
            {
                if (!(pCB->flags & KV_ASYNC_CB_MULTI_CTXT_IO))
                {
                    kv_db_destroy(pCB->db, pCB->len);
                }
                if (pCB->gvalue) free(pCB->gvalue);
                memset(pCB, 0, sizeof(async_CB_t));
                KV_TRC_IO(pFT, "LOOP_DONE: %p", pCB);
                goto done;
            }
            KV_TRC_IO(pFT, "NEXT_LOOP, %p", pCB);
            pCB->flags &= ~KV_ASYNC_CB_DEL;
            pCB->flags |= KV_ASYNC_CB_SET;
            pCB->len_i  = 0;
            kv_async_SET_KEY(pCB);
            goto done;
        }
        kv_async_DEL_KEY(pCB);
        goto done;
    }
    else
    {
        /* should not be here */
        EXPECT_TRUE(0);
    }

done:
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_SET_KEY(async_CB_t *pCB)
{
    uint64_t tag = (uint64_t)pCB;
    int32_t  rc  = 0;

    KV_TRC_IO(pFT, "SET_KEY: %p, %p %lx %d", pCB, pCB->db, tag, pCB->len_i);

    pCB->tag = tag;

    rc = ark_set_async_cb(pCB->ark,
                          pCB->db[pCB->len_i].klen,
                          pCB->db[pCB->len_i].key,
                          pCB->db[pCB->len_i].vlen,
                          pCB->db[pCB->len_i].value,
                          pCB->cb,
                          tag);
    if (EAGAIN == rc)
    {
        kv_async_q_retry(pCB);
    }
    else
    {
        EXPECT_EQ(0, rc);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_GET_KEY(async_CB_t *pCB)
{
    uint64_t tag = (uint64_t)pCB;
    int32_t  rc  = 0;

    KV_TRC_IO(pFT, "GET_KEY: %p, %" PRIx64 " %d", pCB, tag, pCB->len_i);

    pCB->tag = tag;

    rc = ark_get_async_cb(pCB->ark,
                          pCB->db[pCB->len_i].klen,
                          pCB->db[pCB->len_i].key,
                          pCB->db[pCB->len_i].vlen,
                          pCB->gvalue,
                          0,
                          pCB->cb,
                          tag);
    if (EAGAIN == rc)
    {
        kv_async_q_retry(pCB);
    }
    else
    {
        EXPECT_EQ(0, rc);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_EXISTS_KEY(async_CB_t *pCB)
{
    uint64_t tag = (uint64_t)pCB;
    int32_t  rc  = 0;

    KV_TRC_DBG(pFT, "EXI_KEY: %p, %" PRIx64 "", pCB, tag);

    pCB->tag = tag;

    rc = ark_exists_async_cb(pCB->ark,
                             pCB->db[pCB->len_i].klen,
                             pCB->db[pCB->len_i].key,
                             pCB->cb,
                             tag);
    if (EAGAIN == rc)
    {
        kv_async_q_retry(pCB);
    }
    else
    {
        EXPECT_EQ(0, rc);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_DEL_KEY(async_CB_t *pCB)
{
    uint64_t tag = (uint64_t)pCB;
    int32_t  rc  = 0;

    KV_TRC_DBG(pFT, "DEL_KEY: %p, %" PRIx64 "", pCB, tag);

    pCB->tag = tag;

    rc = ark_del_async_cb(pCB->ark,
                          pCB->db[pCB->len_i].klen,
                          pCB->db[pCB->len_i].key,
                          pCB->cb,
                          tag);
    if (EAGAIN == rc)
    {
        kv_async_q_retry(pCB);
    }
    else
    {
        EXPECT_EQ(0, rc);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_q_retry(async_CB_t *pCB)
{
    uint32_t new_flags = pCB->flags;

    KV_TRC_DBG(pFT, "Q_RETRY %p", pCB);
    new_flags &= ~KV_ASYNC_CB_RUNNING;
    new_flags |=  KV_ASYNC_CB_QUEUED;
    pCB->flags = new_flags;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_perf_done(async_CB_t *pCB)
{
    ++pCB->perf_loops;

    if (pCB->flags & KV_ASYNC_CB_SHUTDOWN)
    {
        KV_TRC(pFT, "shutdown %p %d", pCB, pCB->perf_loops);

        pCB->flags &= ~(KV_ASYNC_CB_SET        |
                        KV_ASYNC_CB_GET        |
                        KV_ASYNC_CB_WRITE_PERF |
                        KV_ASYNC_CB_READ_PERF  |
                        KV_ASYNC_CB_RUNNING);
        return;
    }

    kv_async_dispatch(pCB);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt_perf(uint32_t ctxt, uint32_t npool, uint32_t secs)
{
    char            *env_FVT = getenv("FVT_DEV");
    async_context_t *pCT     = pCTs+ctxt;

    ASSERT_TRUE(ctxt >= 0 && ctxt <= KV_ASYNC_MAX_CONTEXTS);
    memset(pCT,       0, sizeof(async_context_t));
    memset(pCT->pCBs, 0, sizeof(async_CB_t)*KV_ASYNC_JOB_Q);

    ASSERT_EQ(0, ark_create_verbose(env_FVT, &pCT->ark,
                                        1048576,
                                        4096,
                                        1048576,
                                        npool,
                                        256,
                                        8*1024,
                                        ARK_KV_VIRTUAL_LUN));
    ASSERT_TRUE(NULL != pCT->ark);

    pCT->flags |= KV_ASYNC_CT_RUNNING;
    pCT->secs   = secs;
    KV_TRC(pFT, "init_ctxt ctxt:%d ark:%p secs:%d", ctxt, pCT->ark, pCT->secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt(uint32_t ctxt, uint32_t secs)
{
    char            *env_FVT = getenv("FVT_DEV");
    async_context_t *pCT     = pCTs+ctxt;

    ASSERT_TRUE(ctxt >= 0 && ctxt <= KV_ASYNC_MAX_CONTEXTS);
    memset(pCT,       0, sizeof(async_context_t));
    memset(pCT->pCBs, 0, sizeof(async_CB_t)*KV_ASYNC_JOB_Q);

    ASSERT_EQ(0, ark_create_verbose(env_FVT, &pCT->ark,
                                    1048576,
                                    4096,
                                    1048576,
                                    1,
                                    256,
                                    8*1024,
                                    ARK_KV_VIRTUAL_LUN));
    ASSERT_TRUE(NULL != pCT->ark);

    pCT->flags |= KV_ASYNC_CT_RUNNING;
    pCT->secs   = secs;
    KV_TRC(pFT, "init_ctxt ctxt:%d ark:%p secs:%d", ctxt, pCT->ark, pCT->secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt_starve(uint32_t ctxt,
                               uint32_t nasync,
                               uint32_t basync,
                               uint32_t secs)
{
    char            *env_FVT = getenv("FVT_DEV");
    async_context_t *pCT     = pCTs+ctxt;

    ASSERT_TRUE(ctxt >= 0 && ctxt <= KV_ASYNC_MAX_CONTEXTS);
    memset(pCT,       0, sizeof(async_context_t));
    memset(pCT->pCBs, 0, sizeof(async_CB_t)*KV_ASYNC_JOB_Q);

    ASSERT_EQ(0, ark_create_verbose(env_FVT, &pCT->ark,
                                    1048576,
                                    4096,
                                    1048576,
                                    1,
                                    nasync,
                                    basync,
                                    ARK_KV_VIRTUAL_LUN));
    ASSERT_TRUE(NULL != pCT->ark);

    pCT->flags |= KV_ASYNC_CT_RUNNING;
    pCT->secs   = secs;
    KV_TRC(pFT, "init_ctxt_starve ctxt:%d ark:%p secs:%d",
           ctxt, pCT->ark, pCT->secs);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_set_job(uint32_t  flags,
                      uint32_t  ctxt,
                      uint32_t  job,
                      kv_t     *db,
                      uint32_t  vlen,
                      uint32_t  len)
{
    async_context_t *pCT     = pCTs+ctxt;
    async_CB_t      *pCB     = NULL;
    char             type[4] = {0};

    ASSERT_TRUE(NULL != pCT);
    ASSERT_TRUE(ctxt >= 0);
    ASSERT_TRUE(ctxt < KV_ASYNC_MAX_CONTEXTS);
    ASSERT_TRUE(0 != len);

    if (flags & KV_ASYNC_CB_SGD) sprintf(type, "SGD");
    else                         sprintf(type, "REP");

    pCB            = pCT->pCBs+job; memset(pCB, 0, sizeof(async_CB_t));
    pCB->ark       = pCT->ark;
    pCB->flags     = flags | KV_ASYNC_CB_SET | KV_ASYNC_CB_QUEUED;
    pCB->db        = db;
    pCB->regen     = kv_db_fixed_regen_values;
    pCB->len       = len;
    pCB->cb        = kv_async_cb;
    pCB->regen_len = vlen;
    pCB->gvalue    = (char*)malloc(pCB->regen_len);
    pCB->b_mark    = B_MARK;
    pCB->e_mark    = E_MARK;

    KV_TRC(pFT, "CREATE_JOB: ctxt:%d ark:%p %s:   pCB:%p flags:%X",
       ctxt, pCT->ark, type, pCB, pCB->flags);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job(uint32_t  flags,
                       uint32_t  ctxt,
                       uint32_t  job,
                       uint32_t  klen,
                       uint32_t  vlen,
                       uint32_t  len)
{
    kv_t *db = (kv_t*)kv_db_create_fixed(len, klen, vlen);
    ASSERT_TRUE(NULL != db);

    KV_TRC(pFT, "CREATE_JOB FIXED %dx%dx%d", klen, vlen, len);

    kv_async_set_job(flags|KV_ASYNC_CB_GTEST, ctxt, job, db, vlen, len);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_job_perf(uint32_t jobs, uint32_t klen, uint32_t vlen,uint32_t len)
{
    long int wr_us    = 0;
    long int rd_us    = 0;
    long int mil      = 1000000;
    float    wr_s     = 0;
    float    rd_s     = 0;
    float    wr_mb    = 0;
    float    rd_mb    = 0;
    uint64_t mb64_1   = (uint64_t)KV_1M;
    uint64_t wr_bytes = 0;
    uint64_t rd_bytes = 0;
    uint64_t ops      = 0;
    uint64_t post_ops = 0;
    uint64_t ios      = 0;
    uint64_t post_ios = 0;
    float    wr_ops   = 0;
    float    wr_ios   = 0;
    float    rd_ops   = 0;
    float    rd_ios   = 0;
    uint32_t secs     = 5;
    uint32_t job      = 0;
    struct timeval stop, start;

    kv_async_init_ctxt(0, secs);

    for (job=0; job<jobs; job++)
    {
        kv_async_init_job(KV_ASYNC_CB_SGD|KV_ASYNC_CB_WRITE_PERF,
                          ASYNC_SINGLE_CONTEXT,
                          job, klen+job, vlen, len);
    }
    pCTs->flags |= KV_ASYNC_CT_PERF;

    /* do writes */
    (void)ark_stats(kv_async_get_ark(ASYNC_SINGLE_CONTEXT), &ops, &ios);
    KV_TRC(pFT, "PERF wr: ops:%ld ios:%ld", ops, ios);
    gettimeofday(&start, NULL);
    kv_async_run_jobs();        /* run write jobs */
    KV_TRC(pFT, "writes done");
    gettimeofday(&stop, NULL);
    wr_us += (stop.tv_sec*mil  + stop.tv_usec) -
             (start.tv_sec*mil + start.tv_usec);
    (void)ark_stats(kv_async_get_ark(ASYNC_SINGLE_CONTEXT),&post_ops,&post_ios);
    KV_TRC(pFT, "PERF wr: ops:%ld ios:%ld", post_ops, post_ios);
    wr_ops += post_ops - ops;
    wr_ios += post_ios - ios;

    /* calc bytes written */
    for (job=0; job<jobs; job++)
    {
        wr_bytes += (klen+vlen+job)*len*(pCTs->pCBs+job)->perf_loops;
    }

    /* do reads */
    for (job=0; job<jobs; job++)
    {
        if ((pCTs->pCBs+job)->perf_loops)
        {
            (pCTs->pCBs+job)->perf_loops = 0;
            (pCTs->pCBs+job)->flags = KV_ASYNC_CB_GET    |
                                      KV_ASYNC_CB_QUEUED |
                                      KV_ASYNC_CB_READ_PERF;
        }
    }
    pCTs->flags |= KV_ASYNC_CT_RUNNING;

    (void)ark_stats(kv_async_get_ark(0), &ops, &ios);
    KV_TRC(pFT, "PERF rd: ops:%ld ios:%ld", ops, ios);
    gettimeofday(&start, NULL);
    kv_async_run_jobs();        /* run read jobs */
    gettimeofday(&stop, NULL);
    KV_TRC(pFT, "reads done");
    rd_us += (stop.tv_sec*mil  + stop.tv_usec) -
             (start.tv_sec*mil + start.tv_usec);
    (void)ark_stats(kv_async_get_ark(0), &post_ops, &post_ios);
    KV_TRC(pFT, "PERF rd: ops:%ld ios:%ld", post_ops, post_ios);
    rd_ops += post_ops - ops;
    rd_ios += post_ios - ios;

    ASSERT_EQ(0, ark_delete(pCTs->ark));

    /* calc bytes read */
    for (job=0; job<jobs; job++)
    {
        rd_bytes += vlen*len*(pCTs->pCBs+job)->perf_loops;
        kv_db_destroy((pCTs->pCBs+job)->db, (pCTs->pCBs+job)->len);
        if ((pCTs->pCBs+job)->gvalue)
            free((pCTs->pCBs+job)->gvalue);
    }

    /* calc and print results */
    wr_s  = (float)((float)wr_us/(float)mil);
    wr_mb = (float)((double)wr_bytes / (double)mb64_1);
    rd_s  = (float)((float)rd_us/(float)mil);
    rd_mb = (float)((double)rd_bytes / (double)mb64_1);

    printf("ASYNC %dx%dx%d writes: %.3d jobs %2.3f mb in %.1f secs at ",
            klen, vlen, len, jobs, wr_mb, wr_s);
    printf("%2.3f mbps, %6.0f op/s, %.0f io/s\n",
            wr_mb/wr_s,
            wr_ops/wr_s,
            wr_ios/wr_s);
    printf("ASYNC %dx%dx%d reads:  %.3d jobs %2.3f mb in %.1f secs at ",
            klen, vlen, len, jobs, rd_mb, rd_s);
    printf("%2.3f mbps, %6.0f op/s, %.0f io/s\n",
            rd_mb/rd_s,
            rd_ops/rd_s,
            rd_ios/rd_s);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_SGD(uint32_t  ctxt,
                           uint32_t  job,
                           uint32_t  klen,
                           uint32_t  vlen,
                           uint32_t  len)
{
    kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, job, klen, vlen, len);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_REP(uint32_t  ctxt,
                           uint32_t  job,
                           uint32_t  klen,
                           uint32_t  vlen,
                           uint32_t  len)
{
    kv_async_init_job(KV_ASYNC_CB_REPLACE, ctxt, job, klen, vlen, len);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_easy(uint32_t ctxt)
{
    uint32_t i=0;

    for (i=0; i<KV_ASYNC_EASY; i++)
    {
        if (i%2 == 0)
        {
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, i+2, 200);
        }
        else
        {
            kv_async_init_job(KV_ASYNC_CB_REPLACE, ctxt, i, i+2, i+2, 200);
        }
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_low_stress(uint32_t ctxt)
{
    uint32_t i    = 0;
    uint32_t vlen = 0;
    uint32_t LEN  = 0;

    for (i=0; i<KV_ASYNC_LOW_STRESS; i++)
    {
        if (i%2 == 0)
        {
            if (0==i%4)
            {
                vlen = KV_4K+i;
                LEN  = 50;
            }
            else
            {
                vlen = 257+i;
                LEN  = 100;
            }
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, vlen, LEN);
        }
        else
        {
            kv_async_init_job(KV_ASYNC_CB_REPLACE, ctxt, i, i+2, i+2, 50);
        }
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_high_stress(uint32_t ctxt)
{
    uint32_t i    = 0;
    uint32_t vlen = 0;
    uint32_t LEN  = 0;

    for (i=0; i<KV_ASYNC_HIGH_STRESS; i++)
    {
        if (i%2 == 0)
        {
            if (0==i%64)
            {
                vlen = KV_64K;
                LEN  = 10;
            }
            else if (0==i%8)
            {
                vlen = KV_4K;
                LEN  = 25;
            }
            else
            {
                vlen = 256+2;
                LEN  = 50;
            }
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, vlen, LEN);
        }
        else
        {
            kv_async_init_job(KV_ASYNC_CB_REPLACE, ctxt, i, i+2, i+2, 50);
        }
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_BIG_BLOCKS(uint32_t ctxt)
{
    uint32_t i=0;

    for (i=0; i<KV_ASYNC_BIG_BLOCKS; i++)
    {
        if (i%32 == 0)
        {
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, KV_2M, 2);
        }
        else if (i%16 == 0)
        {
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, KV_1M, 4);
        }
        else if (i%2 == 0)
        {
            kv_async_init_job(KV_ASYNC_CB_SGD, ctxt, i, i+2, KV_500K, 16);
        }
        else
        {
            kv_async_init_job(KV_ASYNC_CB_REPLACE, ctxt, i, i+2, KV_250K, 32);
        }
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ark_io(uint32_t num_ctxt,
                          uint32_t jobs,
                          uint32_t vlen,
                          uint32_t secs)
{
    uint32_t klen     = 16;
    uint32_t LEN      = 50;
    uint32_t job      = 0;
    uint32_t ctxt     = 0;
    kv_t    *db[jobs];

    printf("ctxt:%d jobs:%d ASYNC %dx%dx%d",
            num_ctxt, jobs, klen, vlen, LEN); fflush(stdout);
    for (job=0; job<jobs; job++)
    {
        db[job] = (kv_t*)kv_db_create_fixed(LEN, klen+job, vlen);
    }
    printf("."); fflush(stdout);
    for (ctxt=0; ctxt<num_ctxt; ctxt++)
    {
        kv_async_init_ctxt(ctxt, secs);

        for (job=0; job<jobs; job++)
        {
            kv_async_set_job(KV_ASYNC_CB_SGD | KV_ASYNC_CB_MULTI_CTXT_IO,
                             ctxt, job, db[job], vlen, LEN);
            KV_TRC(pFT, "CREATE_JOB FIXED %dx%dx%d", klen+job, vlen, LEN);
        }
    }
    printf("> "); fflush(stdout);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ark_io_inject(uint32_t num_ctxt,
                                 uint32_t jobs,
                                 uint32_t vlen,
                                 uint32_t secs)
{
    uint32_t i=0;

    KV_SET_INJECT_ACTIVE;

    kv_async_init_ark_io(num_ctxt, jobs, vlen, secs);

    for (i=0; i<num_ctxt; i++)
    {
        pCTs[i].flags |= KV_ASYNC_CT_ERROR_INJECT;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt_io(uint32_t num_ctxt,
                           uint32_t jobs,
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t LEN,
                           uint32_t secs)
{
    uint32_t job  = 0;
    uint32_t ctxt = 0;
    kv_t    *db[jobs];

    printf("ASYNC %dx%dx%d", klen, vlen, LEN); fflush(stdout);
    for (job=0; job<jobs; job++)
    {
        db[job] = (kv_t*)kv_db_create_fixed(LEN, klen+job, vlen);
    }
    printf("."); fflush(stdout);
    for (ctxt=0; ctxt<num_ctxt; ctxt++)
    {
        kv_async_init_ctxt(ctxt, secs);

        for (job=0; job<jobs; job++)
        {
            kv_async_set_job(KV_ASYNC_CB_SGD           |
                             KV_ASYNC_CB_MULTI_CTXT_IO |
                             KV_ASYNC_CB_GTEST,
                             ctxt, job, db[job], vlen, LEN);
            KV_TRC(pFT, "CREATE_JOB FIXED %dx%dx%d", klen+job, vlen, LEN);
        }
    }
    printf("> "); fflush(stdout);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
uint32_t kv_async_init_perf_io(uint32_t num_ctxt,
                               uint32_t jobs,
                               uint32_t npool,
                               uint32_t klen,
                               uint32_t vlen,
                               uint32_t LEN,
                               uint32_t secs)
{
    uint32_t job  = 0;
    uint32_t ctxt = 0;
    kv_t    *db[jobs];

    printf("ASYNC %dx%dx%d", klen, vlen, LEN); fflush(stdout);
    for (job=0; job<jobs; job++)
    {
        db[job] = (kv_t*)kv_db_create_fixed(LEN, klen+job, vlen);
    }
    printf("."); fflush(stdout);
    for (ctxt=0; ctxt<num_ctxt; ctxt++)
    {
        kv_async_init_ctxt_perf(ctxt, npool, secs);

        for (job=0; job<jobs; job++)
        {
            kv_async_set_job(KV_ASYNC_CB_SGD           |
                             KV_ASYNC_CB_MULTI_CTXT_IO |
                             KV_ASYNC_CB_GTEST,
                             ctxt, job, db[job], vlen, LEN);
            KV_TRC(pFT, "CREATE_JOB FIXED %dx%dx%d", klen+job, vlen, LEN);
        }
    }
    printf("> "); fflush(stdout);
    return 0;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static void kv_async_dispatch(async_CB_t *pCB)
{
    uint32_t new_flags = pCB->flags;

    new_flags &= ~KV_ASYNC_CB_QUEUED;
    new_flags |=  KV_ASYNC_CB_RUNNING;
    pCB->flags = new_flags;

    if (pCB->flags & KV_ASYNC_CB_SET)
    {
        KV_TRC_IO(pFT, "DISPATCH: SET: %p", pCB);
        kv_async_SET_KEY(pCB);
    }
    else if (pCB->flags & KV_ASYNC_CB_GET)
    {
        KV_TRC_IO(pFT, "DISPATCH: GET: %p", pCB);
        kv_async_GET_KEY(pCB);
    }
    else if (pCB->flags & KV_ASYNC_CB_EXISTS)
    {
        KV_TRC_IO(pFT, "DISPATCH: EXI: %p", pCB);
        kv_async_EXISTS_KEY(pCB);
    }
    else if (pCB->flags & KV_ASYNC_CB_DEL)
    {
        KV_TRC_IO(pFT, "DISPATCH: DEL: %p", pCB);
        kv_async_DEL_KEY(pCB);
    }
    else
    {
        EXPECT_TRUE(0);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
uint32_t kv_async_dispatch_jobs(uint32_t ctxt)
{
    async_context_t *pCT          = pCTs+ctxt;
    async_CB_t      *pCB          = NULL;
    uint32_t         jobs_running = 0;

    if (ctxt < 0 || ctxt > KV_ASYNC_MAX_CONTEXTS)
    {
        KV_TRC_FFDC(pFT, "FFDC %x", ctxt);
        return FALSE;
    }

    for (pCB=pCT->pCBs; pCB<pCT->pCBs+KV_ASYNC_JOB_Q; pCB++)
    {
        if (pCB->flags & KV_ASYNC_CB_QUEUED)
        {
            kv_async_dispatch(pCB);
            jobs_running = 1;
            usleep(1000);
        }
        else if (pCB->flags & KV_ASYNC_CB_RUNNING)
        {
            jobs_running = 1;
        }
    }
    return jobs_running;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_run_jobs(void)
{
    async_CB_t *pCB          = NULL;
    uint32_t    ctxt_running = 0;
    uint32_t    jobs_running = 0;
    uint32_t    i            = 0;
    uint32_t    next         = 0;
    uint32_t    elapse       = 0;
    uint32_t    inject       = 0;
    uint32_t    secs         = 0;
    uint32_t    log_interval = 600;
    uint64_t    ops          = 0;
    uint64_t    ios          = 0;
    uint32_t    tops         = 0;
    uint32_t    tios         = 0;
    uint32_t    perf         = 0;

    KV_TRC(pFT, "ASYNC START: 0 minutes");

    if (!(pCTs->pCBs->flags & KV_ASYNC_CB_RUNNING)) start = time(0);
    next = log_interval;

    do
    {
        ctxt_running = FALSE;

        if (elapse > next)
        {
            KV_TRC(pFT, "ASYNC RUNNING: %d elapsed minutes", elapse/60);
            next += log_interval;
        }

        for (i=0; i<KV_ASYNC_MAX_CONTEXTS; i++)
        {
            if (! (pCTs[i].flags & KV_ASYNC_CT_RUNNING)) continue;

            jobs_running = kv_async_dispatch_jobs(i);

            if (!jobs_running)
            {
                pCTs[i].flags &= ~KV_ASYNC_CT_RUNNING;
                pCTs[i].flags |=  KV_ASYNC_CT_DONE;
                KV_TRC(pFT, "ASYNC DONE ctxt %d %x", i, pCTs[i].flags);
                continue;
            }
            else
            {
                ctxt_running = TRUE;
            }

            elapse = time(0) - start;

            if (elapse >= inject &&
                pCTs[i].flags & KV_ASYNC_CT_ERROR_INJECT)
            {
                KV_TRC_FFDC(pFT, "FFDC: INJECT ERRORS %d", inject);
                if (inject)
                {
                    KV_INJECT_SCHD_READ_ERROR;
                    KV_INJECT_SCHD_WRITE_ERROR;
                }
                else
                {
                    KV_INJECT_HARV_READ_ERROR;
                    KV_INJECT_HARV_WRITE_ERROR;
                }
                KV_INJECT_ALLOC_ERROR;
                ++inject;
            }

            if (elapse >= pCTs[i].secs)
            {
                for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+KV_ASYNC_JOB_Q;pCB++)
                {
                    if ((pCB->flags & KV_ASYNC_CB_RUNNING ||
                         pCB->flags & KV_ASYNC_CB_QUEUED)
                        &&
                        (!(pCB->flags & KV_ASYNC_CB_SHUTDOWN)) )
                    {
                        pCB->flags |=  KV_ASYNC_CB_SHUTDOWN;
                        KV_TRC_IO(pFT, "SHUTDOWN pCB %p (%d >= %d)", pCB, elapse, pCTs[i].secs);
                    }
                }
            }
            usleep(100);
        }
    }
    while (ctxt_running);

    stop = time(0);
    secs = stop - start;

    KV_TRC(pFT, "ASYNC RUNNING DONE: %d minutes", elapse/60);

    /* log cleanup, since the first ark_delete closes the log file */
    for (i=0; i<KV_ASYNC_MAX_CONTEXTS; i++)
    {
        if (pCTs[i].flags & KV_ASYNC_CT_DONE)
            KV_TRC(pFT, "ASYNC CLEANUP: ctxt:%d ark:%p", i, pCTs[i].ark);
    }

    /* check for MULTI_CTXT_IO, destroy common kv dbs */
    for (pCB=pCTs->pCBs;pCB<pCTs->pCBs+KV_ASYNC_JOB_Q;pCB++)
    {
        if (pCB->flags & KV_ASYNC_CB_MULTI_CTXT_IO)
        {
            kv_db_destroy(pCB->db, pCB->len);
        }
    }

    for (i=0; i<KV_ASYNC_MAX_CONTEXTS; i++)
    {
        /* if this context didn't run any I/O */
        if (! (pCTs[i].flags & KV_ASYNC_CT_DONE)) continue;

        pCTs[i].flags &= ~KV_ASYNC_CT_DONE;

        /* if perf then don't delete the ark here */
        if (pCTs[i].flags & KV_ASYNC_CT_PERF)
        {
            perf = TRUE;
            continue;
        }

        (void)ark_stats(pCTs[i].ark, &ops, &ios);
        tops += (uint32_t)ops;
        tios += (uint32_t)ios;
        KV_TRC(pFT, "PERF ark%p ops:%ld ios:%ld",
               pCTs[i].ark, ops, ios);

        EXPECT_EQ(0, ark_delete(pCTs[i].ark));
    }

    if (!perf)
    {
        tops = tops / secs;
        tios = tios / secs;
        printf("op/s:%d io/s:%d secs:%d\n", tops, tios, secs);
        KV_TRC(pFT, "PERF op/s:%d io/s:%d secs:%d",
                tops, tios, secs);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_start_jobs(void)
{
    uint32_t i=0;

    start = time(0);

    for (i=0; i<KV_ASYNC_MAX_CONTEXTS; i++)
    {
        kv_async_dispatch_jobs(i);
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_wait_jobs(void)
{
    kv_async_run_jobs();
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_get_arks(ARK *arks[], uint32_t num_ctxt)
{
    uint32_t i=0;

    for (i=0; i<num_ctxt; i++)
    {
        arks[i] = pCTs[i].ark;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ARK* kv_async_get_ark(uint32_t ctxt)
{
    async_context_t *pCT = pCTs+ctxt;

    if (ctxt < 0 || ctxt > KV_ASYNC_MAX_CONTEXTS)
    {
        KV_TRC_FFDC(pFT, "FFDC %x", ctxt);
        return FALSE;
    }

    return pCT->ark;
}
