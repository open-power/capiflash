/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/run_kv_async.c $                                  */
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
 *   Key/Value ARK Database Asynchronous I/O Driver
 * \details
 *   This runs I/O to the Key/Value ARK Database using ASYNC IO.              \n
 *   Each ark does set/get/exists/delete in a loop for a list of              \n
 *   keys/values. This code essentially runs write/read/compare/delete.       \n
 *   One or more completion threads are created to handle the                 \n
 *   async callback from the arkdb.                                           \n
 *                                                                            \n
 *   Example:                                                                 \n
 *                                                                            \n
 *   Run to memory with 4 arks:                                               \n
 *   run_kv_async                                                             \n
 *   ctxt:4 async_ops:100 k/v:16x65536: op/s:218666 io/s:1968000 secs:15      \n
 *                                                                            \n
 *   Run to a file with 1 ark:                                                \n
 *   run_kv_async                                                             \n
 *   ctxt:1 async_ops:100 k/v:16x65536: op/s:26666 io/s:240000 secs:15        \n
 *                                                                            \n
 *   Run to a capi dev with 4 arks:                                           \n
 *   run_kv_async                                                             \n
 *   ctxt:4 async_ops:100 k/v:16x65536: op/s:21176 io/s:190588 secs:17        \n
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

uint32_t KV_ASYNC_CONTEXTS =    2;
uint32_t KV_ASYNC_COMP_PTH =    2;
#define  KV_ASYNC_JOBS_PER_CTXT 40
#define  KV_ASYNC_MIN_SECS      15
#define  KV_ASYNC_KLEN          16
#define  KV_ASYNC_VLEN          64*1024
#define  KV_ASYNC_NUM_KV        100
#define  KV_ASYNC_RUNNING       0x08000000
#define  KV_ASYNC_SHUTDOWN      0x04000000
#define  TRUE                   1
#define  FALSE                  0

/**
********************************************************************************
** \brief
** struct to hold a generated key/value pair
*******************************************************************************/
typedef struct
{
    uint8_t key  [KV_ASYNC_KLEN];
    uint8_t value[KV_ASYNC_VLEN];
} kv_t;

/**
********************************************************************************
** \brief
** struct to hold a list ("database") of key/value pairs
*******************************************************************************/
typedef struct
{
    kv_t db[KV_ASYNC_NUM_KV];
} db_t;

/**
********************************************************************************
** \brief
** struct for each ASYNC op (job)
*******************************************************************************/
typedef struct
{
    ARK            *ark;
    kv_t           *db;
    uint64_t        itag;
    uint64_t        tag;
    int64_t         res;
    void          (*cb)(void*);
    uint32_t        len;
    uint32_t        len_i;
    uint32_t        flags;
    uint32_t        cb_rdy;
    int             errcode;
    char            gvalue[KV_ASYNC_VLEN];
} async_CB_t;

/**
********************************************************************************
** \brief
** struct to hold the async ops for a single context (ARK)
*******************************************************************************/
typedef struct
{
    ARK       *ark;
    async_CB_t pCBs[KV_ASYNC_JOBS_PER_CTXT];
} async_context_t;

/**
********************************************************************************
** structs for all contexts
*******************************************************************************/
async_context_t *pCTs;

/**
********************************************************************************
** \brief
** struct for all k/v databases, one for each job. These are reused in
** each context
*******************************************************************************/
db_t dbs[KV_ASYNC_JOBS_PER_CTXT];

/**
********************************************************************************
** \brief
** structs for each thread
*******************************************************************************/
typedef struct
{
    uint32_t  ctxt;
    uint32_t  num;
    pthread_t pth;
} pth_t;

uint32_t start = 0;
uint32_t stop  = 0;

void kv_async_SET_KEY       (async_CB_t *pCB);
void kv_async_GET_KEY       (async_CB_t *pCB);
void kv_async_EXISTS_KEY    (async_CB_t *pCB);
void kv_async_DEL_KEY       (async_CB_t *pCB);
void kv_async_wait_jobs     (void);
void kv_async_completion_pth(pth_t *p);

/**
********************************************************************************
** \brief
** setup unique values to help in debug
*******************************************************************************/
#define SET_ITAG(ctxt_i, cb_i)  (UINT64_C(0xBEEF000000000000) | \
                               ((uint64_t)ctxt_i)<<32         | \
                                (uint64_t)cb_i   <<16)
#define GET_CTXT(_tag)   (uint32_t)((UINT64_C(0x0000ffff00000000) & _tag) >> 32)
#define GET_CB(_tag)     (uint32_t)((UINT64_C(0x00000000ffff0000) & _tag) >> 16)

/**
********************************************************************************
** \brief
** stop on error
*******************************************************************************/
#define KV_ERR_STOP(_pCB, _msg, _rc)           \
do                                             \
{                                              \
    printf("\n(%s:%p:%d)\n", _msg, _pCB, _rc); \
    if (NULL == _pCB) return;                  \
    _pCB->flags &= ~KV_ASYNC_RUNNING;          \
    return;                                    \
} while (0)

/**
********************************************************************************
** \brief
** setup all the databases with unique key/values, lengths a multiple of 8bytes
*******************************************************************************/
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

/**
********************************************************************************
** \brief
** callback fcn passed to ARK for an async op
*******************************************************************************/
void kv_async_cb(int errcode, uint64_t dt, int64_t res)
{
    async_context_t *pCT  = pCTs+GET_CTXT(dt);
    async_CB_t      *pCB  = NULL;

    if (NULL == pCT) KV_ERR_STOP(pCB, "bad dt: ctxt", 0);
    pCB  = pCT->pCBs+GET_CB(dt);

    if (NULL == pCB)      KV_ERR_STOP(pCB, "bad dt: cb",  0);
    if (dt   != pCB->tag) KV_ERR_STOP(pCB, "bad dt: tag", 0);

    pCB->errcode = errcode;
    pCB->res     = res;
    pCB->cb_rdy  = TRUE;
}

/**
********************************************************************************
** \brief
** process the call back for a ARK set
*******************************************************************************/
void kv_set_cb(void *p)
{
    async_CB_t *pCB = (async_CB_t*)p;

    ++pCB->len_i;

    if (0        != pCB->errcode)  KV_ERR_STOP(pCB, "setcb1", pCB->errcode);
    if (pCB->res != KV_ASYNC_VLEN) KV_ERR_STOP(pCB, "setcb2", 0);

    /* if end of db len sequence, move to get */
    if (pCB->len_i == pCB->len)
    {
        pCB->len_i = 0;
        kv_async_GET_KEY(pCB);
        goto done;
    }
    /* else, do another set */
    kv_async_SET_KEY(pCB);

done:
    return;
}

/**
********************************************************************************
** \brief
** process the call back for a ARK get
*******************************************************************************/
void kv_get_cb(void *p)
{
    async_CB_t *pCB  = (async_CB_t*)p;
    kv_t       *p_kv = pCB->db + pCB->len_i;

    ++pCB->len_i;

    if (0        != pCB->errcode)  KV_ERR_STOP(pCB, "getcb1", pCB->errcode);
    if (pCB->res != KV_ASYNC_VLEN) KV_ERR_STOP(pCB, "getcb2", 0);

    if (memcmp(p_kv->value, pCB->gvalue, KV_ASYNC_VLEN))
    {
        KV_ERR_STOP(pCB,"get miscompare", 0);
    }
    /* end of db len sequence, move to exists */
    if (pCB->len_i == pCB->len)
    {
        pCB->len_i = 0;
        kv_async_EXISTS_KEY(pCB);
        goto done;
    }
    /* else, do another get */
    kv_async_GET_KEY(pCB);

done:
    return;
}

/**
********************************************************************************
** \brief
** process the call back for a ARK exists
*******************************************************************************/
void kv_exists_cb(void *p)
{
    async_CB_t *pCB = (async_CB_t*)p;

    ++pCB->len_i;

    if (0        != pCB->errcode)  KV_ERR_STOP(pCB,"existcb1",pCB->errcode);
    if (pCB->res != KV_ASYNC_VLEN) KV_ERR_STOP(pCB,"existcb2",0);

    /* if end of db len sequence, move to del */
    if (pCB->len_i == pCB->len)
    {
        pCB->len_i = 0;
        kv_async_DEL_KEY(pCB);
        goto done;
    }
    /* else, do another exists */
    kv_async_EXISTS_KEY(pCB);

done:
    return;
}

/**
********************************************************************************
** \brief
** process the call back for a ARK delete
*******************************************************************************/
void kv_del_cb(void *p)
{
    async_CB_t *pCB = (async_CB_t*)p;

    ++pCB->len_i;

    if (0        != pCB->errcode)  KV_ERR_STOP(pCB, "delcb1", pCB->errcode);
    if (pCB->res != KV_ASYNC_VLEN) KV_ERR_STOP(pCB, "delcb2", 0);

    /* end of db len sequence, move to next step */
    if (pCB->len_i == pCB->len)
    {
        if (pCB->flags & KV_ASYNC_SHUTDOWN)
        {
            pCB->flags &= ~KV_ASYNC_RUNNING;
            goto done;
        }
        pCB->len_i = 0;
        kv_async_SET_KEY(pCB);
        goto done;
    }
    /* else, do another del */
    kv_async_DEL_KEY(pCB);

done:
    return;
}

/**
********************************************************************************
** \brief
** issue a set to the ARK
*******************************************************************************/
void kv_async_SET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;

    pCB->tag = pCB->itag + pCB->len_i;
    pCB->cb  = (void (*)(void*))kv_set_cb;

    while (EAGAIN == (rc=ark_set_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          KV_ASYNC_VLEN,
                                          pCB->db[pCB->len_i].value,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10);
    if (rc) KV_ERR_STOP(pCB,"SET_KEY",rc);
}

/**
********************************************************************************
** \brief
** issue a get to the ARK
*******************************************************************************/
void kv_async_GET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;

    pCB->tag = pCB->itag + pCB->len_i;
    pCB->cb = (void (*)(void*))kv_get_cb;

    while (EAGAIN == (rc=ark_get_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          KV_ASYNC_VLEN,
                                          pCB->gvalue,
                                          0,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10);
    if (rc) KV_ERR_STOP(pCB,"GET_KEY",rc);
}

/**
********************************************************************************
** \brief
** issue an exists to the ARK
*******************************************************************************/
void kv_async_EXISTS_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;

    pCB->tag = pCB->itag + pCB->len_i;
    pCB->cb = (void (*)(void*))kv_exists_cb;

    while (EAGAIN == (rc=ark_exists_async_cb(pCB->ark,
                                             KV_ASYNC_KLEN,
                                             pCB->db[pCB->len_i].key,
                                             kv_async_cb,
                                             pCB->tag))) usleep(10);
    if (rc) KV_ERR_STOP(pCB,"EXIST_KEY",rc);
}

/**
********************************************************************************
** \brief
** issue a delete to the ARK
*******************************************************************************/
void kv_async_DEL_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;

    pCB->tag = pCB->itag + pCB->len_i;
    pCB->cb = (void (*)(void*))kv_del_cb;

    while (EAGAIN == (rc=ark_del_async_cb(pCB->ark,
                                          KV_ASYNC_KLEN,
                                          pCB->db[pCB->len_i].key,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10);
    if (rc) KV_ERR_STOP(pCB,"DEL_KEY",rc);
}

/**
********************************************************************************
** \brief
** create completion thread(s), wait for jobs to complete, cleanup
*******************************************************************************/
void kv_async_wait_jobs(void)
{
    uint32_t    i            = 0;
    uint32_t    e_secs       = 0;
    uint64_t    ops          = 0;
    uint64_t    ios          = 0;
    uint32_t    tops         = 0;
    uint32_t    tios         = 0;
    pth_t       p[KV_ASYNC_COMP_PTH];
    void* (*fp)(void*) = (void*(*)(void*))kv_async_completion_pth;

    /* create completion threads */
    for (i=0; i<KV_ASYNC_COMP_PTH; i++)
    {
        p[i].ctxt = i*(KV_ASYNC_CONTEXTS/KV_ASYNC_COMP_PTH);
        p[i].num  = KV_ASYNC_CONTEXTS/KV_ASYNC_COMP_PTH;
        pthread_create(&p[i].pth, NULL, fp, (void*)(p+i));
    }
    /* wait for all work to complete */
    for (i=0; i<KV_ASYNC_COMP_PTH; i++)
    {
        pthread_join(p[i].pth, NULL);
    }

    /* all work is done, cleanup and print stats */
    stop = time(0);
    e_secs = stop - start;

    /* sum perf ops for all contexts/jobs and delete arks */
    for (i=0; i<KV_ASYNC_CONTEXTS; i++)
    {
        (void)ark_stats(pCTs[i].ark, &ops, &ios);
        tops += (uint32_t)ops;
        tios += (uint32_t)ios;

        assert(0 == ark_delete(pCTs[i].ark));
    }

    tops = tops / e_secs;
    tios = tios / e_secs;
    printf("op/s:%d io/s:%d secs:%d\n", tops, tios, e_secs);
}

/**
********************************************************************************
** \brief
** create arks, start all jobs, wait for jobs to complete
*******************************************************************************/
void kv_async_run_io(char *dev)
{
    async_context_t *pCT  = NULL;
    async_CB_t      *pCB  = NULL;
    uint32_t         job  = 0;
    uint32_t         ctxt = 0;

    init_kv_db();

    for (ctxt=0; ctxt<KV_ASYNC_CONTEXTS; ctxt++)
    {
        pCT = pCTs+ctxt;
        assert(0 == ark_create(dev, &pCT->ark, ARK_KV_VIRTUAL_LUN));
        assert(NULL != pCT->ark);

        for (job=0; job<KV_ASYNC_JOBS_PER_CTXT; job++)
        {
            pCB       = pCT->pCBs+job;
            pCB->itag = SET_ITAG(ctxt,job);
            pCB->ark  = pCT->ark;
            pCB->db   = dbs[job].db;
            pCB->len  = KV_ASYNC_NUM_KV;
        }
    }

    start = time(0);

    /* start all jobs for all contexts(arks) */
    for (ctxt=0; ctxt<KV_ASYNC_CONTEXTS; ctxt++)
    {
        for (pCB=pCTs[ctxt].pCBs;
             pCB<pCTs[ctxt].pCBs+KV_ASYNC_JOBS_PER_CTXT;
             pCB++)
        {
            pCB->flags |= KV_ASYNC_RUNNING;
            kv_async_SET_KEY(pCB);
        }
    }
    kv_async_wait_jobs();
}

/**
********************************************************************************
** \brief
**   function called at completion thread create
** \details
**   loop while jobs are running                                              \n
**      if the ARK has called back for an async op, run its callback function \n
**      if time is elapsed, shutdown jobs                                     \n
*******************************************************************************/
void kv_async_completion_pth(pth_t *p)
{
    async_CB_t *pCB          = NULL;
    uint32_t    ctxt_running = 0;
    uint32_t    i            = 0;

    /* loop until all jobs are done or until time elapses */
    do
    {
        ctxt_running = FALSE;

        /* loop through contexts(arks) and check if all jobs are done or
         * time has elapsed
         */
        for (i=p->ctxt; i<p->ctxt+p->num; i++)
        {
            /* check if all jobs are done */
            for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+KV_ASYNC_JOBS_PER_CTXT;pCB++)
            {
                if (pCB->cb_rdy)
                {
                    pCB->cb_rdy = FALSE;
                    pCB->cb(pCB);
                }
                if (pCB->flags & KV_ASYNC_RUNNING)
                {
                    ctxt_running = TRUE;
                }
            }
            if (!ctxt_running) continue;

            /* check if time has elapsed */
            if (time(0) - start < KV_ASYNC_MIN_SECS) continue;

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
}

/**
********************************************************************************
** \brief
** check input parm, start all jobs
*******************************************************************************/
int main(int argc, char **argv)
{
    if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
      printf("usage: %s </dev/sgN>\n",argv[0]);
      exit(0);
    }

    /* if running to a file, cannot do multi-context */
    if (argv[1] != NULL && strncmp(argv[1], "/dev/", 5) != 0)
    {
        KV_ASYNC_CONTEXTS = 1;
        KV_ASYNC_COMP_PTH = 1;
    }

    pCTs = malloc(sizeof(async_context_t) * KV_ASYNC_CONTEXTS);
    bzero(pCTs,   sizeof(async_context_t) * KV_ASYNC_CONTEXTS);

    printf("ctxt:%d async_ops:%d k/v:%dx%d: ",
            KV_ASYNC_CONTEXTS,
            KV_ASYNC_JOBS_PER_CTXT,
            KV_ASYNC_KLEN,
            KV_ASYNC_VLEN);
    fflush(stdout);

    kv_async_run_io(argv[1]);

    free(pCTs);
    return 0;
}
