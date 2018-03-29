/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bin/run_kv_benchmark.c $                               */
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
********************************************************************************
** \file
** \brief
**   run iops benchmark for the Ark Key/Value Database Sync/Async IO
** \details
**   Usage:                                                                   \n
**     [-d device] [-r %rd] [-q qdepth] [-n npool] [-s secs] [-p]             \n
**                                                                            \n
**                                                                            \n
**    examples: run_kv_benchmark                                              \n
**              run_kv_benchmark -d /tmp/kvstore      *kvstore must be >= 1gb \n
**              run_kv_benchmark -d /dev/sg9                                  \n
**              run_kv_benchmark -d /dev/sg9 -q 1 -r 75                       \n
**              run_kv_benchmark -d /dev/sg9 -n 4                             \n
**              run_kv_benchmark -d /dev/sg9 -p                               \n
**              run_kv_benchmark -d /dev/sg9 -s 120                           \n
**
*******************************************************************************/
#include <ark.h>
#include <arkdb.h>
#include <arkdb_trace.h>
#include <ticks.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>
#include <assert.h>
#include <errno.h>

#define KV_PTHS             64
#define KV_JOBS             64
#define KV_NASYNC           64
#define KV_BASYNC           8*1024
int     KV_NPOOL    =       1;
int     KV_MIN_SECS =       12;

#define _4K                 (4*1024)
#define _64K                (64*1024)
#define KLEN                16
#define VLEN_4k             4000
#define VLEN_64k            64000
int     LEN =               200;        // make this 50 to view even % r/w
                                        // 200 per job/pth

#define MAX_IO_ARKS         10
#define MAX_IO_PTHS         40

#define KV_ASYNC_RUNNING           0x08000000
#define KV_ASYNC_SHUTDOWN          0x04000000
#define KV_ASYNC_SET               0x01000000
#define KV_ASYNC_GET               0x00800000
#define KV_ASYNC_EXISTS            0x00400000
#define KV_ASYNC_DEL               0x00200000

#define TRUE  1
#define FALSE 0

/**
********************************************************************************
** \brief
**   struct to hold a generated key/value pair
*******************************************************************************/
typedef struct
{
    uint8_t *key;
    uint8_t *value;
} kv_t;

/**
********************************************************************************
** \brief
**   struct to hold a list ("database") of key/value pairs
*******************************************************************************/
typedef struct
{
    kv_t    *kv;
    uint32_t klen;
    uint32_t vlen;
    uint32_t len;
} db_t;

/**
********************************************************************************
** \brief
**   struct to hold
*******************************************************************************/
typedef struct
{
    ARC       *ark;
    db_t      *db;
    uint32_t   len;
    uint32_t   len_i;
    uint32_t   getN;
    uint32_t   flags;
    uint64_t   itag;
    uint64_t   tag;
    char      *gvalue;
} async_CB_t;

/**
********************************************************************************
** \brief
**   struct to hold
*******************************************************************************/
typedef struct
{
    ARK       *ark;
    uint32_t   flags;
    async_CB_t pCBs[KV_NASYNC];
} async_context_t;

/**
********************************************************************************
** \brief
**   struct to hold
*******************************************************************************/
async_context_t *pCTs;

/**
********************************************************************************
** \brief
**   struct for each worker thread
*******************************************************************************/
typedef struct
{
    ARK      *ark;
    db_t     *db;
    pthread_t pth;
} worker_t;

/**
********************************************************************************
** \brief
** struct for all k/v databases, one for each thread. These are reused in
** each context
*******************************************************************************/
db_t *dbs;

uint32_t read100_count   = 0;
uint32_t read100         = 0;
uint32_t testcase_start  = 0;
uint32_t testcase_stop   = 0;
uint32_t error_stop      = 0;
uint64_t ark_create_flag = ARK_KV_VIRTUAL_LUN;

void kv_async_SET_KEY   (async_CB_t *pCB);
void kv_async_GET_KEY   (async_CB_t *pCB);
void kv_async_DEL_KEY   (async_CB_t *pCB);

/**
********************************************************************************
** \brief
**   init a 64-bit tag word with the context and control block indexes
*******************************************************************************/
#define SET_ITAG(ctxt_i, cb_i) (UINT64_C(0xBEEF000000000000) | \
                              ((uint64_t)ctxt_i)<<32         | \
                               (uint64_t)cb_i   <<16)

/**
********************************************************************************
** \brief
**   extract the context index
*******************************************************************************/
#define GET_CTXT(_tag) (uint32_t)((UINT64_C(0x0000ffff00000000) & _tag) >> 32)

/**
********************************************************************************
** \brief
**   extract the control block index
*******************************************************************************/
#define GET_CB(_tag) (uint32_t)((UINT64_C(0x00000000ffff0000) & _tag) >> 16)

/**
********************************************************************************
** \brief
**   print error msg, clear RUNNING flag to shutdown job, and exit the function
*******************************************************************************/
#define KV_ERR_STOP(_pCB, _msg, _rc)                                           \
do                                                                             \
{                                                                              \
    printf("(%s:%d)", _msg,_rc);                                               \
    if (NULL == _pCB) {error_stop=1; return;}                                  \
    if (_pCB->flags & KV_ASYNC_RUNNING) {_pCB->flags &= ~KV_ASYNC_RUNNING;}    \
    else                                {error_stop=1;}                        \
    return;                                                                    \
} while (0)

/**
********************************************************************************
** \brief
**   print error msg and exit the function
*******************************************************************************/
#define KV_ERR(_msg, _rc)             \
do                                    \
{                                     \
    printf("\n(%s:%d)\n", _msg,_rc);  \
    error_stop=1;                     \
    return _rc;                       \
} while (0)

/**
********************************************************************************
** \brief
**   create a new ark with specific parms
*******************************************************************************/
#define NEW_ARK(_dev, _ark)                                                    \
        assert(0 == ark_create(_dev, _ark, ark_create_flag));                  \
        assert(NULL != ark);

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run KV IO                              \n
**   -r %rd        *the percentage of reads to issue (75 or 100)              \n
**   -q queuedepth *the number of outstanding ops to maintain                 \n
**   -n npool      *the number of threads in the Ark pool                     \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -p            *run in physical lun mode                                  \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("   [-d device] [-r %%rd] [-q qdepth] [-n npool] [-s secs] [-p]\n");
    exit(0);
}

/**
********************************************************************************
** \brief
**   initialize a set of key/value databases
*******************************************************************************/
void init_kv_db(uint32_t n_db, uint32_t klen, uint32_t vlen, uint32_t len)
{
    uint32_t i,j;
    uint64_t *p,tag;

    assert(8 <= klen && 0 == klen%8);
    assert(8 <= vlen && 0 == vlen%8);

    dbs = (db_t*)malloc(n_db*sizeof(db_t));
    assert(dbs);

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<n_db; i++)
    {
        dbs[i].klen = klen;
        dbs[i].vlen = vlen;
        dbs[i].len  = len;
        dbs[i].kv = malloc(len*sizeof(kv_t));
        assert(dbs[i].kv);
        for (j=0; j<len; j++)
        {
            dbs[i].kv[j].key   = malloc(klen);
            dbs[i].kv[j].value = malloc(vlen);
            assert(dbs[i].kv[j].key);
            assert(dbs[i].kv[j].value);

            for (p=(uint64_t*)(dbs[i].kv[j].key);
                 p<(uint64_t*)(dbs[i].kv[j].key+klen);
                 p++)
            {
                tag = UINT64_C(0xbeeffeed00000000) | (i << 16) | j;
                *p=tag;
            }
            for (p=(uint64_t*)(dbs[i].kv[j].value);
                 p<(uint64_t*)(dbs[i].kv[j].value+vlen);
                 p++)
            {
                tag = UINT64_C(0x1234567800000000) | (i << 16) | j;
                *p=tag;
            }
        }
    }
}

/**
********************************************************************************
** \brief
**   free the memory for the key/value databases
*******************************************************************************/
void free_kv_db(uint32_t n_db, uint32_t len)
{
    uint32_t i,j;

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<n_db; i++)
    {
        for (j=0; j<len; j++)
        {
            free(dbs[i].kv[j].key);
            free(dbs[i].kv[j].value);
        }
        free(dbs[i].kv);
    }
    free(dbs);
}

/**
********************************************************************************
** \brief
**   callback for ark functions: set,get,del
*******************************************************************************/
void kv_async_cb(int errcode, uint64_t dt, int64_t res)
{
    async_context_t *pCT  = pCTs+GET_CTXT(dt);
    async_CB_t      *pCB  = NULL;

    KV_TRC_DBG(pAT, "ASYNCCB errcode:%d dt:%lx res:%ld", errcode, dt, res);

    if (errcode) KV_ERR_STOP(pCB, "bad errcode", errcode);
    if (!pCT)    KV_ERR_STOP(pCB, "bad dt: ctxt", 0);

    pCB = pCT->pCBs+GET_CB(dt);

    if (!pCB)                 KV_ERR_STOP(pCB, "bad dt: cb", 0);
    if (res != pCB->db->vlen) KV_ERR_STOP(pCB, "bad res", 0);
    if (dt  != pCB->tag)      KV_ERR_STOP(pCB, "bad tag", 0);

    ++pCB->len_i;

    if (pCB->flags & KV_ASYNC_SET)
    {
        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            pCB->len_i  = 0;
            pCB->flags &= ~KV_ASYNC_SET;
            pCB->flags |= KV_ASYNC_GET;
            if (read100_count)
            {
                pCB->flags &= ~KV_ASYNC_RUNNING;
                --read100_count;
                return;
            }
            else
            {
                kv_async_GET_KEY(pCB);
            }
            goto done;
        }
        kv_async_SET_KEY(pCB);
        goto done;
    }
    else if (pCB->flags & KV_ASYNC_GET)
    {
        /* end of db len sequence, move to next step */
        if (pCB->len_i == pCB->len)
        {
            if (pCB->getN < 5)
            {
                pCB->len_i = 0;
                pCB->getN += 1;
                kv_async_GET_KEY(pCB);
                goto done;
            }
            else
            {
                pCB->len_i = 0;
                pCB->getN  = 0;
                if (read100 && !(pCB->flags & KV_ASYNC_SHUTDOWN))
                {
                    kv_async_GET_KEY(pCB);
                }
                else
                {
                    pCB->flags &= ~KV_ASYNC_GET;
                    pCB->flags |= KV_ASYNC_DEL;
                    kv_async_DEL_KEY(pCB);
                }
            }
            goto done;
        }
        kv_async_GET_KEY(pCB);
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

/**
********************************************************************************
** \brief
**   save a key/value into the ark db
*******************************************************************************/
void kv_async_SET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_set_async_cb(pCB->ark,
                                          pCB->db->klen,
                                          pCB->db->kv[pCB->len_i].key,
                                          pCB->db->vlen,
                                          pCB->db->kv[pCB->len_i].value,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"SET_KEY",rc);
}

/**
********************************************************************************
** \brief
**   save a key/value into the ark db
*******************************************************************************/
void kv_async_GET_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_get_async_cb(pCB->ark,
                                          pCB->db->klen,
                                          pCB->db->kv[pCB->len_i].key,
                                          pCB->db->vlen,
                                          pCB->gvalue,
                                          0,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"GET_KEY",rc);
}

/**
********************************************************************************
** \brief
**   save a key/value into the ark db
*******************************************************************************/
void kv_async_DEL_KEY(async_CB_t *pCB)
{
    uint32_t rc=0;
    pCB->tag = pCB->itag + pCB->len_i;

    while (EAGAIN == (rc=ark_del_async_cb(pCB->ark,
                                          pCB->db->klen,
                                          pCB->db->kv[pCB->len_i].key,
                                          kv_async_cb,
                                          pCB->tag))) usleep(10000);
    if (rc) KV_ERR_STOP(pCB,"DEL_KEY",rc);
}

/**
********************************************************************************
** \brief
**   do an ark asynchronous IO performance run
*******************************************************************************/
void kv_async_io(char    *dev,
                 uint32_t ctxts,
                 uint32_t jobs,
                 uint32_t vlen,
                 uint32_t len,
                 uint32_t read)
{
    async_context_t *pCT          = NULL;
    async_CB_t      *pCB          = NULL;
    uint32_t         job          = 0;
    uint32_t         ctxt         = 0;
    uint32_t         ctxt_running = TRUE;
    uint32_t         i            = 0;
    uint32_t         e_secs       = 0;
    uint64_t         ops          = 0;
    uint64_t         ios          = 0;
    uint32_t         tops         = 0;
    uint32_t         tios         = 0;
    uint32_t         wr_ops       = 0;
    uint32_t         wr_ios       = 0;
    uint32_t         time_started = 0;

    if (read == 75) {read100 = FALSE;}
    else            {read100 = TRUE; read100_count=ctxts*jobs;}
    printf("ASYNC: ctxt:%-2d QD:%-3d size:%2dk read:%3d%% ",
            ctxts, jobs, vlen/1000, read);
    fflush(stdout);

    init_kv_db(jobs, KLEN, vlen, len);
    pCTs = (async_context_t*) malloc(ctxts*sizeof(async_context_t));
    assert(0 != pCTs);

    for (ctxt=0; ctxt<ctxts; ctxt++)
    {
        ARK *ark = NULL;
        pCT = pCTs+ctxt;
        memset(pCT, 0, sizeof(async_context_t));

        NEW_ARK(dev, &ark);
        pCT->ark = ark;

        for (job=0; job<jobs; job++)
        {
            pCB            = pCT->pCBs+job;
            pCB->itag      = SET_ITAG(ctxt,job);
            pCB->ark       = pCT->ark;
            pCB->flags     = KV_ASYNC_SET;
            pCB->db        = dbs+job;
            pCB->len       = len;
            if (0==posix_memalign((void**)&pCB->gvalue, 128, vlen))
                {assert(pCB->gvalue);}
        }
    }

    testcase_start = time(0);
    if (!read100) {time_started=TRUE;}

    /* start all jobs for all contexts */
    for (ctxt=0; ctxt<ctxts; ctxt++)
    {
        for (pCB=pCTs[ctxt].pCBs;
             pCB<pCTs[ctxt].pCBs+jobs;
             pCB++)
        {
            pCB->flags |=  KV_ASYNC_RUNNING;
            kv_async_SET_KEY(pCB);
        }
    }

    /* loop until all jobs are done or until time elapses */
    do
    {
        /* restart jobs if 100% reads, and all writes are complete */
        if (read100 && read100_count==0 && !time_started)
        {

            testcase_start = time(0);
            time_started   = TRUE;
            /* continue all jobs for all contexts */
            for (ctxt=0; ctxt<ctxts; ctxt++)
            {
                (void)ark_stats(pCTs[ctxt].ark, &ops, &ios);
                wr_ops += (uint32_t)ops;
                wr_ios += (uint32_t)ios;
                for (pCB=pCTs[ctxt].pCBs;
                     pCB<pCTs[ctxt].pCBs+jobs;
                     pCB++)
                {
                    pCB->flags |=  KV_ASYNC_RUNNING;
                    kv_async_GET_KEY(pCB);
                }
            }
        }
        else if (read100 && !time_started)
        {
            usleep(50000);
            continue;
        }

        ctxt_running = FALSE;

        /* loop through contexts(arks) and check if all jobs are done or
         * time has elapsed
         */
        for (i=0; i<ctxts; i++)
        {
            /* check if all jobs are done */
            for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+jobs;pCB++)
            {
                if (pCB->flags & KV_ASYNC_RUNNING)
                {
                    ctxt_running = TRUE;
                }
            }
            if (!ctxt_running) continue;

            /* check if time has elapsed */
            if (time(0) - testcase_start < KV_MIN_SECS) continue;

            for (pCB=pCTs[i].pCBs;pCB<pCTs[i].pCBs+jobs;pCB++)
            {
                if (pCB->flags & KV_ASYNC_RUNNING &&
                 (!(pCB->flags & KV_ASYNC_SHUTDOWN)) )
                {
                    pCB->flags |=  KV_ASYNC_SHUTDOWN;
                }
            }
        }
        usleep(50000);
    }
    while (!error_stop && ctxt_running);

    testcase_stop = time(0);
    e_secs = testcase_stop - testcase_start;

    /* sum perf ops for all contexts/jobs and delete arks */
    for (i=0; i<ctxts; i++)
    {
        (void)ark_stats(pCTs[i].ark, &ops, &ios);
        tops += (uint32_t)ops;
        tios += (uint32_t)ios;
    }

    if (read100) {tops-=wr_ops; tios-=wr_ios;}
    tops = tops / e_secs;
    tios = tios / e_secs;
    printf("op/s:%-7d io/s:%-7d secs:%d\n", tops, tios, e_secs);

    /* sum perf ops for all contexts/jobs and delete arks */
    for (i=0; i<ctxts; i++)
    {
        for (job=0; job<jobs; job++) {free(pCTs[i].pCBs[job].gvalue);}
        assert(0 == ark_delete(pCTs[i].ark));
    }

    free_kv_db(jobs, len);
}

/**
********************************************************************************
** \brief
**   load a k/v database into an ark
*******************************************************************************/
uint32_t kv_load(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<db->len; i++)
    {
        while (EAGAIN == (rc=ark_set(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     db->vlen,
                                     db->kv[i].value,
                                     &res))) usleep(10);
        if (rc)              KV_ERR("set1",rc);
        if (db->vlen != res) KV_ERR("set2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
**   validate that a k/v database exists in the ark
*******************************************************************************/
uint32_t kv_query(ARK *ark, db_t *db, char *buf)
{
    uint32_t i      = 0;
    uint32_t rc     = 0;
    int64_t  res    = 0;

    assert(buf);

    for (i=0; i<db->len; i++)
    {
        while (EAGAIN == (rc=ark_get(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     db->vlen,
                                     buf,
                                     0,
                                     &res))) usleep(10);
        if (rc)               KV_ERR("get1", rc);
        if (db->vlen != res)  KV_ERR("get2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
**   delete a k/v database from an ark
*******************************************************************************/
uint32_t kv_del(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<db->len; i++)
    {
        while (EAGAIN == (rc=ark_del(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     &res))) usleep(10);
        if (rc)              KV_ERR("del1", rc);
        if (db->vlen != res) KV_ERR("del2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
**   read values from the ark using the keys in a database
*******************************************************************************/
void read_100_percent(worker_t *w)
{
    int      i=0,rc=0;
    char    *buf         = NULL;
    ticks    start       = 0;
    double   ns_per_tick = 0;
    uint32_t num         = 0;
    int64_t  res         = 0;

    if (0==posix_memalign((void**)&buf, _4K, _64K)) {assert(buf);}

    ns_per_tick = time_per_tick(1000, 100);
    start = getticks();
    do
    {
        for (i=0; i<w->db->len; i++)
        {
            while (EAGAIN == (rc=ark_get(w->ark,
                                         w->db->klen,
                                         w->db->kv[i].key,
                                         w->db->vlen,
                                         buf,
                                         0,
                                         &res))) {continue;}
            if (rc)                 {printf("get1 rc:%d",rc); goto error;}
            if (w->db->vlen != res) {printf("get2");          goto error;}
        }
        num += w->db->len;
    }
    while (SDELTA(start,ns_per_tick) < KV_MIN_SECS);

error:
    //KV_TRC_PERF2(pAT, "LAT:%5ld", UDELTA(start,ns_per_tick)/num);

    free(buf);
}

/**
********************************************************************************
** \brief
**   write and read k/v in a database from the ark to equal 75% reads 25% writes
*******************************************************************************/
void read_75_percent(worker_t *w)
{
    int start=0,cur=0,rc=0;
    char *buf=NULL;

    if (0==posix_memalign((void**)&buf, _4K, _64K)) {assert(buf);}

    start = time(0);
    do
    {
        if ((rc=kv_load (w->ark, w->db)))      break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_query(w->ark, w->db, buf))) break;
        if ((rc=kv_del  (w->ark, w->db)))      break;
        cur = time(0);
    }
    while (cur-start < KV_MIN_SECS);
    free(buf);
}

/**
********************************************************************************
** \brief
**   query the io stats from the ark
*******************************************************************************/
void get_stats(ARK *ark, uint32_t *ops, uint32_t *ios)
{
    uint64_t ops64    = 0;
    uint64_t ios64    = 0;

    (void)ark_stats(ark, &ops64, &ios64);
    *ops = (uint32_t)ops64;
    *ios = (uint32_t)ios64;
}

/**
********************************************************************************
** \brief
**   create threads to run one of the IO functions, wait for the threads to
**   complete, and print the iops
*******************************************************************************/
void kv_sync_io(char    *dev,
                uint32_t vlen,
                uint32_t len,
                uint32_t pths,
                uint32_t read)
{
    worker_t w[512];
    ARK     *ark    = NULL;
    uint32_t i      = 0;
    uint32_t ops    = 0;
    uint32_t ios    = 0;
    uint32_t e_secs = 0;
    char    *fp_txt = NULL;
    void*  (*fp)(void*);

    init_kv_db(pths, KLEN, vlen, len);
    NEW_ARK(dev, &ark);

    if (read == 75)
    {
        fp = (void*(*)(void*))read_75_percent;
        fp_txt="read: 75%";
    }
    else
    {
        fp = (void*(*)(void*))read_100_percent;
        fp_txt="read:100%";
        for (i=0; i<pths; i++) {kv_load(ark, dbs+i);}
    }

    printf("SYNC:  ctxt:%-2d QD:%-3d size:%2dk %s ", 1, pths, vlen/1000,fp_txt);
    fflush(stdout);

    testcase_start = time(0);

    /* start all threads */
    for (i=0; i<pths; i++)
    {
        w[i].ark = ark;
        w[i].db  = dbs+(i % pths);
        pthread_create(&(w[i].pth), NULL, fp, (void*)(w+i));
    }

    /* wait for all threads to complete */
    for (i=0; i<pths; i++)
    {
        pthread_join(w[i].pth, NULL);
    }

    testcase_stop = time(0);
    e_secs = testcase_stop - testcase_start;

    get_stats(ark, &ops, &ios);
    ops = ops / e_secs;
    ios = ios / e_secs;
    printf("op/s:%-7d io/s:%-7d secs:%d\n", ops, ios, e_secs);

    free_kv_db(pths, len);
    assert(0 == ark_delete(ark));
}

/**
********************************************************************************
** \brief
**   create max arks, create max threads to run one of the IO functions, wait
**   for the threads to complete, prints the iops, delete the arks
*******************************************************************************/
void kv_max_sync_io(char    *dev,
                    uint32_t vlen,
                    uint32_t len)
{
    ARK     *ark[MAX_IO_ARKS];
    worker_t w[MAX_IO_ARKS][MAX_IO_PTHS];
    uint32_t i      = 0;
    uint32_t j      = 0;
    uint32_t ops    = 0;
    uint32_t ios    = 0;
    uint32_t tops   = 0;
    uint32_t tios   = 0;
    uint32_t e_secs = 0;

    printf("SYNC:  ctxt:%-2d QD:%-3d size: 4k read:100%% ",
            MAX_IO_ARKS, MAX_IO_PTHS); fflush(stdout);

    init_kv_db(MAX_IO_PTHS, KLEN, vlen, len);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        NEW_ARK(dev, ark+i);
    }

    testcase_start = time(0);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        for (j=0; j<MAX_IO_PTHS; j++) {kv_load(ark[i], dbs+j);}

        /* create all threads */
        for (j=0; j<MAX_IO_PTHS; j++)
        {
            w[i][j].ark = ark[i];
            w[i][j].db  = dbs+(j % KV_PTHS);
            pthread_create(&(w[i][j].pth),
                           NULL,
                           (void*(*)(void*))read_100_percent,
                           (void*)(&w[i][j]));
        }
    }

    /* wait for all threads to complete */
    for (i=0; i<MAX_IO_ARKS; i++)
    {
        for (j=0; j<MAX_IO_PTHS; j++)
        {
            pthread_join(w[i][j].pth, NULL);
        }
    }

    testcase_stop = time(0);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        get_stats(ark[i], &ops, &ios);
        tops += ops;
        tios += ios;
    }
    e_secs = testcase_stop - testcase_start;
    ops = tops / e_secs;
    ios = tios / e_secs;
    printf("op/s:%-7d io/s:%-7d secs:%d\n", ops, ios, e_secs);

    free_kv_db(MAX_IO_PTHS, len);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        assert(0 == ark_delete(ark[i]));
    }
}

/**
********************************************************************************
** \brief
**   run the benchmark IO
** \details
**   -parse input parms                                                       \n
**   -run async IO single context benchmarks                                  \n
**   -if the input device is not physical lun and not a file                  \n
**      run async multi-context benchmarks                                    \n
**   -run sync IO single context benchmarks                                   \n
**   -if the input device is not physical lun and not a file                  \n
**      run sync multi-context benchmarks
*******************************************************************************/
int main(int argc, char **argv)
{
    char     FF          = 0xFF;
    char     c           = '\0';
    char    *dev         = NULL;
    char    *_secs       = NULL;
    char    *_QD         = NULL;
    char    *_RD         = NULL;
    char    *_np         = NULL;
    char    *_len        = NULL;
    char    *_htc        = NULL;
    uint32_t plun        = 0;
    uint32_t sync        = 0;
    uint32_t QD          = 0;
    uint32_t nRD         = 100;
    uint32_t all         = 0;
    uint32_t htc         = 1;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:s:n:l:c:hpiSa")))
    {
        switch (c)
        {
            case 'd': dev        = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 's': _secs      = optarg; break;
            case 'n': _np        = optarg; break;
            case 'l': _len       = optarg; break;
            case 'c': _htc       = optarg; break;
            case 'p': plun       = 1;      break;
            case 'a': all        = 1;      break;
            case 'S': sync       = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)     {KV_MIN_SECS = atoi(_secs);}
    if (_QD)       {QD          = atoi(_QD);}
    if (_RD)       {nRD         = atoi(_RD);}
    if (_np)       {KV_NPOOL    = atoi(_np);}
    if (_len)      {LEN         = atoi(_len);}
    if (_htc)      {htc         = atoi(_htc);}

    if (QD  >  KV_NASYNC) QD          = KV_NASYNC;
    if (nRD >= 100)       nRD         = 100;
    else                  nRD         = 75;

    if (htc) {ark_create_flag |= ARK_KV_HTC;}

    if (dev == NULL || (dev && strlen(dev)==0))
    {
        printf("MEMORY: npool:%d nasync:%d basync:%d\n",
                KV_NPOOL, KV_NASYNC, KV_BASYNC);
    }
    else if (plun)
    {
        printf("PHYSICAL: %s: npool:%d nasync:%d basync:%d\n",
                 dev, KV_NPOOL, KV_NASYNC, KV_BASYNC);
        ark_create_flag &= ~ARK_KV_VIRTUAL_LUN;
    }
    else if (QD)
    {
        if (sync)
        {
            printf("VIRTUAL SYNC: %s: QD:%d npool:%d nasync:%d basync:%d ",
                   dev, QD, KV_NPOOL, KV_NASYNC, KV_BASYNC);
        }
        else
        {
            printf("VIRTUAL ASYNC: %s: QD:%d npool:%d nasync:%d basync:%d ",
                   dev, QD, KV_NPOOL, KV_NASYNC, KV_BASYNC);
        }
    }
    else
    {
        printf("VIRTUAL: %s: npool:%d nasync:%d basync:%d\n",
                 dev, KV_NPOOL, KV_NASYNC, KV_BASYNC);
    }
    fflush(stdout);

    if (QD)
    {
        if (sync) {kv_sync_io(dev, VLEN_4k, LEN, QD, nRD);}
        else      {kv_async_io(dev, 1, QD, VLEN_4k, LEN, nRD);}
        exit(0);
    }

    if (all) {kv_async_io(dev, 1,  1,       16,       LEN*10, 75);}
              kv_async_io(dev, 1,  1,       16,       LEN*10, 100);
    if (all) {kv_async_io(dev, 1,  KV_JOBS, 16,       LEN*10, 75);}
             {kv_async_io(dev, 1,  KV_JOBS, 16,       LEN*10, 100);}
    if (all) {kv_async_io(dev, 1,  1,       VLEN_4k,  LEN,    75);}
              kv_async_io(dev, 1,  1,       VLEN_4k,  LEN,    100);
              kv_async_io(dev, 1,  KV_JOBS, VLEN_4k,  LEN,    75);
              kv_async_io(dev, 1,  KV_JOBS, VLEN_4k,  LEN,    100);
    if (all) {kv_async_io(dev, 1,  1,       VLEN_64k, LEN,    75);}
    if (all) {kv_async_io(dev, 1,  1,       VLEN_64k, LEN,    100);}
    if (all) {kv_async_io(dev, 1,  KV_JOBS, VLEN_64k,  50,    75);}
              kv_async_io(dev, 1,  KV_JOBS, VLEN_64k,  50,    100);

    if (all) {kv_sync_io (dev, 16,       LEN, 1,       75);}
              kv_sync_io (dev, 16,       LEN, 1,       100);
    if (all) {kv_sync_io (dev, 16,       LEN, KV_PTHS, 75);}
             {kv_sync_io (dev, 16,       LEN, KV_PTHS, 100);}
    if (all) {kv_sync_io (dev, VLEN_4k,  LEN, 1,       75);}
              kv_sync_io (dev, VLEN_4k,  LEN, 1,       100);
              kv_sync_io (dev, VLEN_4k,  LEN, KV_PTHS, 75);
              kv_sync_io (dev, VLEN_4k,  LEN, KV_PTHS, 100);
    if (all) {kv_sync_io (dev, VLEN_64k, LEN, 1,       75);}
    if (all) {kv_sync_io (dev, VLEN_64k, LEN, 1,       100);}
    if (all) {kv_sync_io (dev, VLEN_64k, LEN, KV_PTHS, 75);}
              kv_sync_io (dev, VLEN_64k, LEN, KV_PTHS, 100);
    exit(0);
}
