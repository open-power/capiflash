/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/run_kv_async_multi.c $                            */
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
 *   Key/Value ARK Database Aynchronous I/O Driver
 * \details
 *   This runs I/O to the Key/Value ARK Database using ASYNC IO.              \n
 *   Each ark does set/get/exists/delete in a loop for a list of              \n
 *   keys/values. This code essentially runs write/read/compare/delete.       \n
 *                                                                            \n
 *   Example:                                                                 \n
 *                                                                            \n
 *   Run to memory with 4 arks:                                               \n
 *   run_kv_async_multi                                                       \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:253200 io/s:2278800 secs:20                                  \n
 *                                                                            \n
 *   Run to a file with 1 ark:                                                \n
 *   run_kv_async_multi /kvstore                                              \n
 *   ctxt:1 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:27600 io/s:248400 secs:20                                    \n
 *                                                                            \n
 *   Run to a file as a physical lun with persisted data with 1 ark:          \n
 *   run_kv_async_multi -p /kvstore                                           \n
 *   Attempting to run with physical lun                                      \n
 *   ctxt:1 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:25200 io/s:226800 secs:20                                    \n
 *                                                                            \n
 *   Run to a capi dev with 4 arks:                                           \n
 *   run_kv_async_multi /dev/sg10                                             \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:21818 io/s:196363 secs:22                                    \n
 *                                                                            \n
 *   Run to a capi dev as a physical lun with persisted data with 1 ark:      \n
 *   run_kv_async_multi -p /dev/sg10                                          \n
 *   Attempting to run with physical lun                                      \n
 *   ctxt:1 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:21600 io/s:194400 secs:20                                    \n
 *                                                                            \n
 *   Run to 4 capi devs with 4 arks each:                                     \n
 *   run_kv_async_multi /dev/sg9 /dev/sg10 /dev/sg11 /dev/sg12                \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ctxt:4 async_ops:60 k/v:16x65536:                                        \n
 *   ASYNC: op/s:38090 io/s:342818 secs:22                                    \n
 *
 *******************************************************************************
 */
#include <arkdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>

#define KV_ASYNC_MAX_CONTEXTS      40
#define KV_ASYNC_MAX_CTXT_PER_DEV  4
#define KV_ASYNC_MAX_JOBS_PER_CTXT 60

#define KV_ASYNC_SECS              20

#define KV_ASYNC_KLEN              16
#define KV_ASYNC_VLEN              64*1024
#define KV_ASYNC_NUM_KV            100

#define KV_ASYNC_RUNNING           0x08000000
#define KV_ASYNC_SHUTDOWN          0x04000000
#define KV_ASYNC_SET               0x01000000
#define KV_ASYNC_GET               0x00800000
#define KV_ASYNC_EXISTS            0x00400000
#define KV_ASYNC_DEL               0x00200000

uint32_t KV_ASYNC_CONTEXTS      = 0;
uint32_t KV_ASYNC_CTXT_PER_DEV  = KV_ASYNC_MAX_CTXT_PER_DEV;
uint32_t KV_ASYNC_JOBS_PER_CTXT = KV_ASYNC_MAX_JOBS_PER_CTXT;

#define TRUE  1
#define FALSE 0

typedef struct
{
    uint8_t key  [KV_ASYNC_KLEN];
    uint8_t value[KV_ASYNC_VLEN];
} kv_t;

typedef struct
{
    kv_t db[KV_ASYNC_NUM_KV];
} db_t;

typedef struct
{
    ARC       *ark;
    kv_t      *db;
    uint32_t   len;
    uint32_t   len_i;
    uint32_t   flags;
    uint64_t   itag;
    uint64_t   tag;
    char       gvalue[KV_ASYNC_VLEN];
} async_CB_t;

typedef struct
{
    ARK       *ark;
    uint32_t   flags;
    async_CB_t pCBs[KV_ASYNC_MAX_JOBS_PER_CTXT];
} async_context_t;

async_context_t pCTs[KV_ASYNC_MAX_CONTEXTS];

db_t dbs[KV_ASYNC_MAX_JOBS_PER_CTXT];

uint32_t start           = 0;
uint32_t stop            = 0;
uint64_t ark_create_flag = ARK_KV_VIRTUAL_LUN;

void kv_async_SET_KEY   (async_CB_t *pCB);
void kv_async_GET_KEY   (async_CB_t *pCB);
void kv_async_EXISTS_KEY(async_CB_t *pCB);
void kv_async_DEL_KEY   (async_CB_t *pCB);
void kv_async_wait_jobs (void);

/*******************************************************************************
 ******************************************************************************/
#define SET_ITAG(ctxt_i, cb_i)      (UINT64_C(0xBEEF000000000000) | \
                                   ((uint64_t)ctxt_i)<<32         | \
                                    (uint64_t)cb_i   <<16)
#define GET_CTXT(_tag)   (uint32_t)((UINT64_C(0x0000ffff00000000) & _tag) >> 32)
#define GET_CB(_tag)     (uint32_t)((UINT64_C(0x00000000ffff0000) & _tag) >> 16)

/*******************************************************************************
 ******************************************************************************/
#define KV_ERR_STOP(_pCB, _msg, _rc)     \
do                                       \
{                                        \
    printf("(%s:%d)", _msg,_rc);         \
    if (NULL == _pCB) return;            \
    _pCB->flags &= ~KV_ASYNC_RUNNING;    \
    return;                              \
} while (0)

/*******************************************************************************
 ******************************************************************************/
void init_kv_db(void)
{
    uint32_t i,j;
    uint64_t *p,tag;

    assert(8 <= KV_ASYNC_KLEN && 0 == KV_ASYNC_KLEN%8);
    assert(8 <= KV_ASYNC_VLEN && 0 == KV_ASYNC_VLEN%8);

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<KV_ASYNC_JOBS_PER_CTXT; i++)
    {
        for (j=0; j<KV_ASYNC_NUM_KV; j++)
        {
            for (p=(uint64_t*)(dbs[i].db[j].key);
                 p<(uint64_t*)(dbs[i].db[j].key+KV_ASYNC_KLEN);
                 p++)
            {
                tag = SET_ITAG(0xfeed,i);
                *p=tag+j;
            }
            for (p=(uint64_t*)(dbs[i].db[j].value);
                 p<(uint64_t*)(dbs[i].db[j].value+KV_ASYNC_VLEN);
                 p++)
            {
                tag = SET_ITAG(0xfeed,i);
                *p=tag+j;
            }
        }
    }
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_cb(int errcode, uint64_t dt, int64_t res)
{
    async_context_t *pCT  = pCTs+GET_CTXT(dt);
    async_CB_t      *pCB  = NULL;
    kv_t            *p_kv = NULL;

    if (NULL == pCT) KV_ERR_STOP(pCB, "bad dt: ctxt", 0);
    pCB  = pCT->pCBs+GET_CB(dt);

    if (NULL == pCB)          KV_ERR_STOP(pCB, "bad dt: cb", 0);
    if (0   != errcode)       KV_ERR_STOP(pCB, "bad errcode", errcode);
    if (dt  != pCB->tag)      KV_ERR_STOP(pCB, "bad tag", 0);
    if (res != KV_ASYNC_VLEN) KV_ERR_STOP(pCB, "bad res", 0);

    p_kv = pCB->db + pCB->len_i;
    ++pCB->len_i;

    if (pCB->flags & KV_ASYNC_SET)
    {
        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_SET;
            pCB->flags |= KV_ASYNC_GET;
            kv_async_GET_KEY(pCB);
            goto done;
        }
        kv_async_SET_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_GET)
    {
        if (0 != memcmp(p_kv->value, pCB->gvalue, KV_ASYNC_VLEN))
        {
            KV_ERR_STOP(pCB,"get miscompare",0);
        }

        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_GET;
            pCB->flags |= KV_ASYNC_EXISTS;
            kv_async_EXISTS_KEY(pCB);
            goto done;
        }
        kv_async_GET_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_EXISTS)
    {
        /* if end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_EXISTS;
            pCB->flags |= KV_ASYNC_DEL;
            kv_async_DEL_KEY(pCB);
            goto done;
        }
        kv_async_EXISTS_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_DEL)
    {
        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            if (pCB->flags & KV_ASYNC_SHUTDOWN)
            {
                pCB->flags &= ~KV_ASYNC_RUNNING;
                goto done;
            }
            pCB->flags &= ~KV_ASYNC_DEL;
            pCB->flags |= KV_ASYNC_SET;
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
        assert(0);
    }

done:
    return;
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_SET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_set_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          KV_ASYNC_VLEN,
                                          pCB->db[pCB->len_i].value,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"SET_KEY",rc);
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_GET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_get_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          KV_ASYNC_VLEN,
                                          pCB->gvalue,
                                          0,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"GET_KEY",rc);
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_EXISTS_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_exists_async_cb(pCB->ark,
                                             KV_ASYNC_KLEN,
                                             pCB->db[pCB->len_i].key,
                                             kv_async_cb,
                                             pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"EXIST_KEY",rc);
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_DEL_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_del_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"DEL_KEY",rc);
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_init_io(char    *dev,
                      uint32_t jobs,
                      uint32_t klen,
                      uint32_t vlen,
                      uint32_t LEN)
{
    async_context_t *pCT     = NULL;
    async_CB_t      *pCB     = NULL;
    uint32_t         job     = 0;
    uint32_t         ctxt    = 0;

    printf("ctxt:%d async_ops:%d k/v:%dx%d: \n",
            KV_ASYNC_CTXT_PER_DEV, jobs, klen, vlen);
    fflush(stdout);

    init_kv_db();

    for (ctxt=KV_ASYNC_CONTEXTS;
         ctxt<KV_ASYNC_CONTEXTS+KV_ASYNC_CTXT_PER_DEV; ctxt++)
    {
        pCT = pCTs+ctxt;
        assert(0 == ark_create(dev, &pCT->ark, ark_create_flag));
        assert(NULL != pCT->ark);

        for (job=0; job<jobs; job++)
        {
            pCB            = pCT->pCBs+job;
            pCB->itag      = SET_ITAG(ctxt,job);
            pCB->ark       = pCT->ark;
            pCB->flags     = KV_ASYNC_SET;
            pCB->db        = dbs[job].db;
            pCB->len       = LEN;
        }
    }
}

/*******************************************************************************
 ******************************************************************************/
void kv_async_wait_jobs(void)
{
    async_CB_t *pCB          = NULL;
    uint32_t    ctxt_running = 0;
    uint32_t    i            = 0;
    uint32_t    e_secs       = 0;
    uint64_t    ops          = 0;
    uint64_t    ios          = 0;
    uint32_t    tops         = 0;
    uint32_t    tios         = 0;

    printf("ASYNC: "); fflush(stdout);

    /* loop until all jobs are done or until time elapses */
    do
    {
        ctxt_running = FALSE;

        /* loop through contexts(arks) and check if all jobs are done or
         * time has elapsed
         */
        for (i=0; i<KV_ASYNC_CONTEXTS; i++)
        {
            /* check if all jobs are done */
            for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+KV_ASYNC_JOBS_PER_CTXT;pCB++)
            {
                if (pCB->flags & KV_ASYNC_RUNNING)
                {
                    ctxt_running = TRUE;
                }
            }
            if (!ctxt_running) continue;

            /* check if time has elapsed */
            if (time(0) - start < KV_ASYNC_SECS) continue;

            for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+KV_ASYNC_JOBS_PER_CTXT;pCB++)
            {
                if (pCB->flags & KV_ASYNC_RUNNING &&
                 (!(pCB->flags & KV_ASYNC_SHUTDOWN)) )
                {
                    pCB->flags |=  KV_ASYNC_SHUTDOWN;
                }
            }
        }
    }
    while (ctxt_running);

    stop = time(0);
    e_secs = stop - start;

    /* sum perf ops for all contexts/jobs and delete arks */
    for (i=0; i<KV_ASYNC_CONTEXTS; i++)
    {
        (void)ark_stats(pCTs[i].ark, &ops, &ios);
        tops += (uint32_t)ops;
        tios += (uint32_t)ios;
    }

    tops = tops / e_secs;
    tios = tios / e_secs;
    printf("op/s:%d io/s:%d secs:%d\n", tops, tios, e_secs);

    /* sum perf ops for all contexts/jobs and delete arks */
    for (i=0; i<KV_ASYNC_CONTEXTS; i++)
    {
        assert(0 == ark_delete(pCTs[i].ark));
    }
}

/*******************************************************************************
 ******************************************************************************/
int main(int argc, char **argv)
{
    async_CB_t *pCB  = NULL;
    uint32_t    i    = 1;
    uint32_t    ctxt = 0;

    if (argv[1] != NULL && argv[1][0] == '?')
    {
        printf("usage: run_kv_async_multi [-p] <dev1> <dev2> ...\n");
        printf("   ex: run_kv_async_multi /dev/sd2 /dev/sd3 /dev/sd4\n");
        printf("   ex: run_kv_async_multi -p /dev/sg4 /dev/sg8\n");
        exit(0);
    }
    bzero(pCTs, sizeof(pCTs));

    /* if running to a file, cannot do multi-context */
    if (argv[1] != NULL && strncmp(argv[1], "/dev/", 5) != 0)
    {
        KV_ASYNC_CTXT_PER_DEV = 1;
    }

    if (argv[1] != NULL && 0 == strncmp(argv[1], "-p", 2))
    {
        printf("Attempting to run with physical lun\n");
        ark_create_flag       = ARK_KV_PERSIST_STORE;
        KV_ASYNC_CTXT_PER_DEV = 1;
        i                     = 2;
    }

    if (argv[1] == NULL)
    {
        kv_async_init_io(NULL,
                         KV_ASYNC_JOBS_PER_CTXT,
                         KV_ASYNC_KLEN,
                         KV_ASYNC_VLEN,
                         KV_ASYNC_NUM_KV);
        KV_ASYNC_CONTEXTS += KV_ASYNC_CTXT_PER_DEV;
    }
    else
    {
        while (argv[i] != NULL && KV_ASYNC_CONTEXTS <= KV_ASYNC_MAX_CONTEXTS)
        {
            kv_async_init_io(argv[i++],
                             KV_ASYNC_JOBS_PER_CTXT,
                             KV_ASYNC_KLEN,
                             KV_ASYNC_VLEN,
                             KV_ASYNC_NUM_KV);
            KV_ASYNC_CONTEXTS += KV_ASYNC_CTXT_PER_DEV;
        }
    }
    start = time(0);

    /* start all jobs for all contexts */
    for (ctxt=0; ctxt<KV_ASYNC_CONTEXTS; ctxt++)
    {
        for (pCB=pCTs[ctxt].pCBs;
             pCB<pCTs[ctxt].pCBs+KV_ASYNC_JOBS_PER_CTXT;
             pCB++)
        {
            pCB->flags |=  KV_ASYNC_RUNNING;
            kv_async_SET_KEY(pCB);
        }
    }
    kv_async_wait_jobs();

    return 0;
}
