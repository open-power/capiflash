/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bin/kv_perf.c $                                        */
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
**   performance benchmark for the Ark Key/Value Database
*******************************************************************************/
#include <ark.h>
#include <arkdb.h>
#include <arkdb_trace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <errno.h>
#include <ticks.h>

#define KV_MAX_QD           1024
#define KV_NASYNC           1024
#define KV_BASYNC           8*1024
int     KV_MIN_SECS =       1;

#define  _4K                (4*1024)
#define  _64K               (64*1024)
#define  VLEN_4k            4000
#define  VLEN_64k           64000
uint64_t LEN  =             10000;
int      KLEN =             8;
uint64_t VLEN =             24;

#define KV_ASYNC_RUNNING           0x08000000
#define KV_ASYNC_SHUTDOWN          0x04000000
#define KV_ASYNC_SET               0x01000000
#define KV_ASYNC_GET               0x00800000
#define KV_ASYNC_EXISTS            0x00400000
#define KV_ASYNC_DEL               0x00200000
#define KV_ASYNC_EAGAIN            0x00100000

#define TRUE  1
#define FALSE 0

/**
********************************************************************************
** \brief
**   struct to hold
*******************************************************************************/
typedef struct
{
    ARC       *ark;
    uint32_t   cur;
    uint32_t   beg;
    uint32_t   end;
    uint32_t   klen;
    uint64_t   vlen;
    uint32_t   flags;
    uint64_t   itag;
    uint64_t   tag;
    char      *keyval;
    char      *genval;
    char      *getval;
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
async_context_t  CT;
async_context_t *pCTs=&CT;

/**
********************************************************************************
** \brief
**   struct for each worker thread
*******************************************************************************/
typedef struct
{
    ARK      *ark;
    pthread_t pth;
    uint32_t  beg;
    uint32_t  end;
    uint32_t  klen;
    uint64_t  vlen;
    char     *keyval;
    char     *genval;
    char     *getval;
} worker_t;

/**
********************************************************************************
** \brief
** struct for all k/v databases, one for each thread. These are reused in
** each context
*******************************************************************************/
uint32_t error_stop      = 0;
uint64_t ark_create_flag = 0;
double   ns_per_tick     = 0;
uint64_t seed            = UINT64_C(0x7DCA000000000000);
uint32_t MEMCMP          = 0;

void kv_async_SET_KEY   (async_CB_t *pCB);
void kv_async_GET_KEY   (async_CB_t *pCB);
void kv_async_DEL_KEY   (async_CB_t *pCB);

/**
********************************************************************************
** \brief
**   init a 64-bit tag word with the context and control block indexes
*******************************************************************************/
#define SET_ITAG(cb_i) (UINT64_C(0xBEEF000000000000) | (uint64_t)cb_i <<32)

/**
********************************************************************************
** \brief
**   extract the control block index
*******************************************************************************/
#define GET_CB(_tag) (uint32_t)((UINT64_C(0x0000ffff00000000) & _tag) >> 32)

#define P_VAL(_v,_n)                                \
  do                                                \
  {                                                 \
    int _i=0;                                       \
    uint8_t *_p=(uint8_t*)_v;                       \
    for (_i=0;_i<_n;_i++) {printf("%02x",*(_p+_i));}\
    printf("\n");                                   \
  } while (0)

/**
********************************************************************************
** \brief
**   print error msg, clear RUNNING flag to shutdown job, and exit the function
*******************************************************************************/
#define KV_ERR_STOP(_pCB, _msg, _rc)                                           \
do                                                                             \
{                                                                              \
    int64_t _rc64=(int64_t)_rc;                                                \
    error_stop=1;                                                              \
    KV_TRC_FFDC(pAT, "ERR_STOP pCB:%p msg:(%s) rc:%ld", _pCB, _msg,_rc64);     \
    printf("(%s:%ld)", _msg,_rc64);                                            \
    if (_pCB->flags & KV_ASYNC_RUNNING) {_pCB->flags &= ~KV_ASYNC_RUNNING;}    \
    return;                                                                    \
} while (0)

/**
********************************************************************************
** \brief
**   print error msg and exit the function
*******************************************************************************/
#define KV_ERR(_msg, _rc)                                                      \
do                                                                             \
{                                                                              \
    KV_TRC(pAT, "ERR     msg:(%s) rc:%d", _msg,_rc);                           \
    printf("\n(%s:%d)\n", _msg,_rc);                                           \
    error_stop=1;                                                              \
    return _rc;                                                                \
} while (0)

/**
********************************************************************************
** \brief
**   create a new ark with specific parms
*******************************************************************************/
#define NEW_ARK(_dev, _ark, _thds, _ht)                                        \
  do                                                                           \
  {                                                                            \
      int rc = ark_create_verbose(_dev, _ark,                                  \
                                  ARK_VERBOSE_SIZE_DEF,                        \
                                  ARK_VERBOSE_BSIZE_DEF,                       \
                                  _ht,                                         \
                                  _thds,                                       \
                                  KV_NASYNC,                                   \
                                  ARK_MAX_BASYNCS,                             \
                                  ark_create_flag);                            \
      if (rc != 0 || NULL == *_ark)                                            \
      {                                                                        \
          printf(" ark_create failed rc:%d errno:%d\n", rc, errno);            \
          exit(-2);                                                            \
      }                                                                        \
  } while (0)

/**
********************************************************************************
** \brief
**   callback for ark functions: set,get,del
*******************************************************************************/
void kv_async_cb(int errcode, uint64_t dt, int64_t res)
{
    async_CB_t *pCB = NULL;

    pCB = pCTs->pCBs+GET_CB(dt);

    KV_TRC(pAT, "ASYNCCB %p dt:%lx cur:%d beg:%d end:%d",
                pCB, dt, pCB->cur, pCB->beg, pCB->end);

    if (!pCB)             KV_ERR_STOP(pCB, "bad dt: cb", 0);
    if (0   != errcode)   KV_ERR_STOP(pCB, "bad errcode", errcode);
    if (dt  != pCB->tag)  KV_ERR_STOP(pCB, "bad tag", 0);
    if (res != pCB->vlen) KV_ERR_STOP(pCB, "bad res", 0);

    if (MEMCMP && pCB->flags & KV_ASYNC_GET)
    {
        GEN_VAL(pCB->genval, seed+pCB->cur, pCB->vlen);
        if (0 != memcmp(pCB->genval, pCB->getval, pCB->vlen))
        {
            KV_ERR_STOP(pCB,"get miscompare",pCB->vlen);
            goto done;
        }
    }

    if (error_stop) {KV_ERR_STOP(pCB, "ALLSTOP", 99); goto done;}

    pCB->cur += 1;
    if (pCB->cur > pCB->end)
    {
        if (pCB->flags & KV_ASYNC_SHUTDOWN || pCB->flags & KV_ASYNC_DEL)
        {
            KV_TRC(pAT, "Stop RUNNING:%p", pCB);
            pCB->flags &= ~KV_ASYNC_RUNNING;
            goto done;
        }

        KV_TRC(pAT, "RESTART loop:%p", pCB);
        pCB->cur = pCB->beg;
    }

    if      (pCB->flags & KV_ASYNC_SET) {kv_async_SET_KEY(pCB);}
    else if (pCB->flags & KV_ASYNC_DEL) {kv_async_DEL_KEY(pCB);}
    else if (pCB->flags & KV_ASYNC_GET) {kv_async_GET_KEY(pCB);}
    else                                {assert(0);}

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
    uint32_t rc = 0;

    GEN_VAL(pCB->keyval, seed+pCB->cur, pCB->klen);

    if (!MEMCMP && pCB->vlen>_64K)
    {
        GEN_VAL(pCB->genval, seed+pCB->cur, _64K);
    }
    else
    {
        GEN_VAL(pCB->genval, seed+pCB->cur, pCB->vlen);
    }
    pCB->tag = pCB->itag + pCB->cur;

    if (EAGAIN == (rc=ark_set_async_cb(pCB->ark,
                                          pCB->klen,
                                          pCB->keyval,
                                          pCB->vlen,
                                          pCB->genval,
                                          kv_async_cb,
                                          pCB->tag)))
    {
        pCB->flags |= KV_ASYNC_EAGAIN;
        return;
    }

    if (rc) KV_ERR_STOP(pCB,"SET_KEY",rc);
}

/**
********************************************************************************
** \brief
**   save a key/value into the ark db
*******************************************************************************/
void kv_async_GET_KEY(async_CB_t *pCB)
{
    uint32_t rc  = 0;

    GEN_VAL(pCB->keyval, seed+pCB->cur, pCB->klen);
    pCB->tag = pCB->itag + pCB->cur;

    if (EAGAIN == (rc=ark_get_async_cb(pCB->ark,
                                       pCB->klen,
                                       pCB->keyval,
                                       pCB->vlen,
                                       pCB->getval,
                                       0,
                                       kv_async_cb,
                                       pCB->tag)))
    {
        pCB->flags |= KV_ASYNC_EAGAIN;
        return;
    }
    if (rc) KV_ERR_STOP(pCB,"GET_KEY",rc);
}

/**
********************************************************************************
** \brief
**   save a key/value into the ark db
*******************************************************************************/
void kv_async_DEL_KEY(async_CB_t *pCB)
{
    uint32_t rc  = 0;

    GEN_VAL(pCB->keyval, seed+pCB->cur, pCB->klen);
    pCB->tag = pCB->itag + pCB->cur;

    if (EAGAIN == (rc=ark_del_async_cb(pCB->ark,
                                       pCB->klen,
                                       pCB->keyval,
                                       kv_async_cb,
                                       pCB->tag)))
    {
        pCB->flags |= KV_ASYNC_EAGAIN;
        return;
    }
    if (rc) KV_ERR_STOP(pCB,"DEL_KEY",rc);
}

/**
********************************************************************************
** \brief
**   do an ark asynchronous IO performance run
*******************************************************************************/
void kv_async_io(ARK     *ark,
                 uint32_t flags,
                 uint32_t jobs,
                 uint32_t klen,
                 uint64_t vlen,
                 uint64_t len)
{
    async_CB_t      *pCB          = NULL;
    uint32_t         job          = 0;
    uint32_t         ctxt_running = TRUE;
    uint32_t         e_secs       = 0;
    uint64_t         s_ops        = 0;
    uint64_t         s_ios        = 0;
    uint64_t         ops          = 0;
    uint64_t         ios          = 0;
    uint32_t         shutdown     = 0;
    ticks            start        = 0;

    memset(pCTs, 0, sizeof(async_context_t));

    pCTs->ark = ark;
    ark_stats(pCTs->ark, &s_ops, &s_ios);

    if (len<jobs) {len=jobs;}

    for (job=0; job<jobs; job++)
    {
        pCB            = pCTs->pCBs+job;
        pCB->itag      = SET_ITAG(job);
        pCB->ark       = pCTs->ark;
        pCB->flags     = flags;
        pCB->klen      = klen;
        pCB->vlen      = vlen;
        pCB->beg       = job==0 ? 0 : (pCTs->pCBs+(job-1))->end+1;
        pCB->end       = pCB->beg + (len/jobs)-1;
        pCB->cur       = pCB->beg;
        KV_TRC(pAT, "JOB:    i:%3d klen:%5d vlen:%5ld len:%ld beg:%3d end:%3d",
                    job, klen, vlen, len, pCB->beg, pCB->end);
        if (0==posix_memalign((void**)&pCB->keyval, 128, klen))
            {assert(pCB->keyval);}
        if (0==posix_memalign((void**)&pCB->genval, 128, vlen))
            {assert(pCB->genval);}
        if (0==posix_memalign((void**)&pCB->getval, 128, vlen))
            {assert(pCB->getval);}
    }

    start = getticks();

    /* start all jobs for all contexts */
    for (pCB=pCTs->pCBs; pCB<pCTs->pCBs+jobs; pCB++)
    {
        pCB->flags |=  KV_ASYNC_RUNNING;
        if (KV_MIN_SECS==0) {pCB->flags |= KV_ASYNC_SHUTDOWN;}
        if      (pCB->flags & KV_ASYNC_SET) {kv_async_SET_KEY(pCB);}
        else if (pCB->flags & KV_ASYNC_GET) {kv_async_GET_KEY(pCB);}
        else                                {kv_async_DEL_KEY(pCB);}
    }

    /* loop until all jobs are done or until time elapses */
    do
    {
        usleep(50000);
        ctxt_running = FALSE;

        /* loop through contexts(arks) and check if all jobs are done or
         * time has elapsed
         */
        for (pCB=pCTs->pCBs; pCB<pCTs->pCBs+jobs; pCB++)
        {
            if (pCB->flags & KV_ASYNC_RUNNING) {ctxt_running=TRUE;}
            if (pCB->flags & KV_ASYNC_EAGAIN)
            {
                pCB->flags &= ~KV_ASYNC_EAGAIN;
                if      (pCB->flags & KV_ASYNC_SET) {kv_async_SET_KEY(pCB);}
                else if (pCB->flags & KV_ASYNC_GET) {kv_async_GET_KEY(pCB);}
                else                                {kv_async_DEL_KEY(pCB);}
            }
        }

        if (!shutdown && SDELTA(start,ns_per_tick) > KV_MIN_SECS)
        {
            shutdown=1;
            for (pCB=pCTs->pCBs; pCB<pCTs->pCBs+jobs; pCB++)
            {
                KV_TRC(pAT, "Shutdown job:%p", pCB);
                pCB->flags |=  KV_ASYNC_SHUTDOWN;
            }
        }
    }
    while (ctxt_running);

    KV_TRC(pAT, "Shutdown complete:%p", pCB);

    e_secs = SDELTA(start,ns_per_tick);
    e_secs = (e_secs <= 0) ? 1 : e_secs;

    /* sum perf ops for all contexts/jobs and delete arks */
    ark_stats(pCTs->ark, &ops, &ios);
    char *op=NULL;
    if      (flags & KV_ASYNC_SET) {op="SET";}
    else if (flags & KV_ASYNC_GET) {op="GET";}
    if (op)
    {
        printf("%s   tops:%8ld tios:%8ld op/s:%8ld io/s:%8ld secs:%d\n",
               op, ops-s_ops, ios-s_ios,
               (ops-s_ops)/e_secs, (ios-s_ios)/e_secs, e_secs);
    }
    /* sum perf ops for all contexts/jobs and delete arks */
    for (job=0; job<jobs; job++)
    {
        free(pCTs->pCBs[job].keyval);
        free(pCTs->pCBs[job].genval);
        free(pCTs->pCBs[job].getval);
    }
}

/**
********************************************************************************
** \brief
**   load a k/v database into an ark
*******************************************************************************/
uint32_t kv_set(worker_t *w)
{
    uint32_t i      = 0;
    uint32_t rc     = 0;
    int64_t  res    = 0;
    ticks    tstart = getticks();
    KV_TRC(pAT, "START w:%p", &w);

    do
    {
        for (i=w->beg; i<=w->end; i++)
        {
            GEN_VAL(w->keyval, seed+i, w->klen);
            if (!MEMCMP && w->vlen>_64K) {GEN_VAL(w->genval, seed+i, _64K);}
            else                         {GEN_VAL(w->genval, seed+i, w->vlen);}

            while (EAGAIN == (rc=ark_set(w->ark,
                                         w->klen,
                                         w->keyval,
                                         w->vlen,
                                         w->genval,
                                         &res))) usleep(10000);
            if (rc)             KV_ERR("set1",rc);
            if (w->vlen != res) KV_ERR("set2",0);
        }
    } while (SDELTA(tstart,ns_per_tick) < KV_MIN_SECS);

    KV_TRC(pAT, "DONE w:%p", &w);
    return 0;
}

/**
********************************************************************************
** issue a get and exists for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_get(worker_t *w)
{
    uint32_t i     = 0;
    uint32_t rc    = 0;
    int64_t  res   = 0;
    ticks    start = getticks();
    KV_TRC(pAT, "START w:%p", &w);

    do
    {
        for (i=w->beg; i<=w->end; i++)
        {
            GEN_VAL(w->keyval, seed+i, w->klen);
            if (!MEMCMP && w->vlen>_64K) {GEN_VAL(w->genval, seed+i, _64K);}
            else                         {GEN_VAL(w->genval, seed+i, w->vlen);}

            while (EAGAIN == (rc=ark_get(w->ark,
                                         w->klen,
                                         w->keyval,
                                         w->vlen,
                                         w->getval,
                                         0,
                                         &res))) usleep(10000);
            if (rc)                                    {KV_ERR("get1", rc);}
            if (w->vlen != res)                        {KV_ERR("get2",0);}
            if (MEMCMP &&
                memcmp(w->genval,w->getval,w->vlen))   {KV_ERR("miscompare",0);}
        }
    } while (SDELTA(start,ns_per_tick) < KV_MIN_SECS);

    KV_TRC(pAT, "DONE w:%p", &w);

    return 0;
}

/**
********************************************************************************
** issue a delete for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_del(worker_t *w)
{
    uint32_t i     = 0;
    uint32_t rc    = 0;
    int64_t  res   = 0;

    KV_TRC(pAT, "DEL   w:%p", &w);

    for (i=w->beg; i<=w->end; i++)
    {
        GEN_VAL(w->keyval, seed+i, w->klen);

        while (EAGAIN == (rc=ark_del(w->ark,
                                     w->klen,
                                     w->keyval,
                                     &res))) usleep(10000);
        if (rc)             KV_ERR("del1", rc);
        if (w->vlen != res) KV_ERR("del2",0);
    }
    KV_TRC(pAT, "DONE w:%p", &w);

    return 0;
}

/**
********************************************************************************
** \brief
**   create max arks, create max threads to run one of the IO functions, wait
**   for the threads to complete, prints the iops, delete the arks
*******************************************************************************/
void kv_sync_io(ARK     *ark,
                uint32_t QD,
                uint32_t klen,
                uint64_t vlen,
                uint64_t len,
                void*  (*fp)(void*))
{
    worker_t w[KV_MAX_QD];
    uint32_t i      = 0;
    uint64_t s_ops  = 0;
    uint64_t s_ios  = 0;
    uint64_t ops    = 0;
    uint64_t ios    = 0;
    uint32_t e_secs = 0;
    uint64_t start  = 0;

    ark_stats(ark, &s_ops, &s_ios);
    start = getticks();

    /* create all threads */
    for (i=0; i<QD; i++)
    {
        w[i].ark  = ark;
        w[i].klen = klen;
        w[i].vlen = vlen;
        w[i].beg  = i==0 ? 0 : w[i-1].end+1;
        w[i].end  = w[i].beg + (len/QD)-1;
        KV_TRC_DBG(pAT, "START   w[%d]:%p beg:%d end:%d", i,&w,w[i].beg,w[i].end);
        if (0==posix_memalign((void**)&w[i].keyval, 128, klen))
            {assert(w[i].keyval);}
        if (0==posix_memalign((void**)&w[i].genval, 128, vlen))
            {assert(w[i].genval);}
        if (0==posix_memalign((void**)&w[i].getval, 128, vlen))
            {assert(w[i].getval);}
        pthread_create(&(w[i].pth), NULL, fp, (void*)(&w[i]));
    }

    /* wait for all threads to complete */
    for (i=0; i<QD; i++) {pthread_join(w[i].pth, NULL);}

    e_secs = SDELTA(start,ns_per_tick);
    e_secs = e_secs==0?1:e_secs;

    /* sum perf ops for all contexts/jobs and delete arks */
    ark_stats(ark, &ops, &ios);
    char *op=NULL;
    if ((void*)fp == (void*)kv_set) {op="SET";} else {op="GET";}
    printf("%s   tops:%8ld tios:%8ld op/s:%8ld io/s:%8ld secs:%d\n",
            op, ops-s_ops, ios-s_ios,
            (ops-s_ops)/e_secs, (ios-s_ios)/e_secs, e_secs);

    for (i=0; i<QD; i++)
    {
        free(w[i].keyval);
        free(w[i].genval);
        free(w[i].getval);
    }
    return;
}

/**
********************************************************************************
** \brief
**   get all keys in an Arkdb, getN at a time in a buf
*******************************************************************************/
void kv_getN(ARK *ark, int64_t klen, uint64_t vlen, uint64_t len, uint32_t getN)
{
    ARI      *ari    = NULL;
    uint32_t  rc     = 0;
    uint64_t  s_ops  = 0;
    uint64_t  s_ios  = 0;
    uint64_t  ops    = 0;
    uint64_t  ios    = 0;
    uint32_t  e_secs = 0;
    uint64_t  cnt    = 0;
    uint64_t  start  = 0;
    uint64_t  buflen = 0;
    uint8_t  *buf    = NULL;
    int64_t   keyN   = 0;
    int       i      = 0;

    buflen = getN ? getN*(klen+8) : klen;
    buf    = malloc(buflen);

    ark_stats(ark, &s_ops, &s_ios);
    start = getticks();

    if (!(ari=ark_first(ark, buflen, &klen, buf)))
    {
        rc = errno;
        goto exit;
    }
    ++cnt;

    KV_TRC(pAT,"     getN:%d keyN:%ld cnt:%2ld ",getN,keyN,cnt);

    while (rc==0)
    {
        if (getN==1)
        {
            rc = ark_next(ari, buflen, &keyN, buf);
            if (rc==0)
            {
                //key = buf;
                cnt += 1;
            }
        }
        else
        {
            rc = ark_nextN(ari, buflen, buf, &keyN);
            if (rc==0)
            {
                cnt += keyN;
                keye_t *keye = (keye_t*)buf;
                for (i=0; i<keyN; i++)
                {
                    //key  = keye->p;
                    //klen = keye->len;
                    keye = BMP_KEYE(keye);
                }
            }
        }
    }

    if (cnt != len) {printf("cnt:%ld != len:%ld\n",cnt,len); rc=-1; goto exit;}

    e_secs = SDELTA(start,ns_per_tick);
    e_secs = e_secs==0?1:e_secs;

    ark_stats(ark, &ops, &ios);

    printf("GETN  tops:%8ld tios:%8ld op/s:%8ld io/s:%8ld secs:%d cnt:%ld\n",
            ops-s_ops, ios-s_ios,
            (ops-s_ops)/e_secs, (ios-s_ios)/e_secs, e_secs, cnt);
exit:
    if (buf) {free(buf);}
    if (!rc) {printf("GETN failed, errno:%d\n", errno);}
    return;
}

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run KV IO                              \n
**   -q queuedepth *the number of outstanding ops to maintain to the Arkdb    \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -l len        *number of k/v pairs to set/get (default is 10000)         \n
**   -k klen       *len in bytes of each key                                  \n
**   -v vlen       *len in bytes of each value                                \n
**   -t vlen       *arkdb threads                                             \n
**   -h hte        *ht entries                                                \n
**   -p            *use plun rather than vlun                                 \n
**   -c 0          *disable the metadata cache                                \n
**   -n            *write to a new persisted ark                              \n
**   -r            *read the existing persisted ark                           \n
**   -S            *run sync Arkdb ops                                        \n
**   -M            *memcmp every read to validate data integrity              \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("   [-d device] [-q qdepth] [-s secs] [-l len]"
           " [-k klen] [-v vlen] [-t thds] [-h hte] [-p] [-n] [-r] [-S] [-M] [-c]\n");
    exit(0);
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
    ARK     *ark         = NULL;
    char     FF          = 0xFF;
    char     c           = '\0';
    char    *dev         = NULL;
    char    *_secs       = NULL;
    char    *_QD         = NULL;
    char    *_len        = NULL;
    char    *_klen       = NULL;
    char    *_vlen       = NULL;
    char    *_htc        = NULL;
    char    *_hte        = NULL;
    char    *_thds       = NULL;
    char    *_getN       = NULL;
    uint32_t readonly    = 0;
    uint32_t new         = 0;
    uint32_t sync        = 0;
    uint32_t getN        = 0;
    uint32_t htc         = 1;
    uint64_t hte         = ARK_VERBOSE_HASH_DEF;
    uint32_t thds        = ARK_VERBOSE_NTHRDS_DEF;
    uint32_t plun        = 0;
    uint32_t QD          = 400;
    uint32_t verbose     = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:q:s:l:v:k:c:t:h:G:VprhSMn")))
    {
        switch (c)
        {
            case 'd': dev        = optarg;   break;
            case 'q': _QD        = optarg;   break;
            case 's': _secs      = optarg;   break;
            case 'l': _len       = optarg;   break;
            case 'v': _vlen      = optarg;   break;
            case 'k': _klen      = optarg;   break;
            case 'c': _htc       = optarg;   break;
            case 'h': _hte       = optarg;   break;
            case 't': _thds      = optarg;   break;
            case 'G': _getN      = optarg;   break;
            case 'p': plun       = 1;        break;
            case 'n': new        = 1;        break;
            case 'r': readonly   = 1; new=0; break;
            case 'S': sync       = 1;        break;
            case 'M': MEMCMP     = 1;        break;
            case 'V': verbose    = 1;        break;
            case '?': usage();               break;
        }
    }
    if (_secs) {KV_MIN_SECS = atoi (_secs);}
    if (_QD)   {QD          = atoi (_QD);}
    if (_len)  {LEN         = atoll(_len);}
    if (_klen) {KLEN        = atoi (_klen);}
    if (_vlen) {VLEN        = atoll(_vlen);}
    if (_htc)  {htc         = atoi (_htc);}
    if (_hte)  {hte         = atoll(_hte);}
    if (_thds) {thds        = atoi (_thds);}
    if (_getN) {getN        = atoi (_getN);}

    QD          = QD>KV_NASYNC ? KV_NASYNC : QD;
    hte         = hte<thds     ? thds      : hte;
    LEN         = LEN<QD       ? QD        : LEN;
    LEN         = LEN - (LEN%QD);
    ns_per_tick = time_per_tick(1000, 100);

    if      (new)      {ark_create_flag = ARK_KV_PERSIST_STORE;}
    else if (readonly) {ark_create_flag = ARK_KV_PERSIST_LOAD;}
    else if (!plun)    {ark_create_flag = ARK_KV_VIRTUAL_LUN;}

    if (htc) {ark_create_flag |= ARK_KV_HTC;}

    NEW_ARK(dev, &ark, thds, hte);

    if (getN)
    {
        KV_MIN_SECS=0;
        printf("GETN: %s: QD:%d LEN:%ld getN:%d\n", dev,QD,LEN,getN); fflush(stdout);
        if (!readonly) {kv_async_io(ark, KV_ASYNC_SET, QD, KLEN,VLEN,LEN);}
        kv_getN(ark, KLEN, VLEN, LEN, getN);
    }
    else if (sync)
    {
        printf("SYNC: %s: QD:%d LEN:%ld\n", dev, QD, LEN); fflush(stdout);
        if (!readonly) {kv_sync_io(ark, QD, KLEN, VLEN, LEN,(void*(*)(void*))kv_set);}
        if (!new)      {kv_sync_io(ark, QD, KLEN, VLEN, LEN,(void*(*)(void*))kv_get);}
    }
    else
    {
        printf("ASYNC: %s: QD:%d LEN:%ld\n", dev, QD, LEN); fflush(stdout);
        if (!readonly) {kv_async_io(ark, KV_ASYNC_SET, QD, KLEN,VLEN,LEN);}
        if (!new)      {kv_async_io(ark, KV_ASYNC_GET, QD, KLEN,VLEN,LEN);}
    }

//    if (!new && !readonly) {kv_async_io(ark, dev, KV_ASYNC_DEL, QD, KLEN,VLEN,LEN);}

    if (verbose)
    {
        uint64_t size   = 0;
        uint64_t inuse  = 0;
        uint64_t actual = 0;
        int64_t  count  = 0;

        ark_allocated(ark, &size);
        ark_inuse    (ark, &inuse);
        ark_actual   (ark, &actual);
        ark_count    (ark, &count);

        printf("\nsize:%ld inuse:%ld actual:%ld count:%ld\n",size,inuse,actual,count);
    }

    assert(0 == ark_delete(ark));

    if (error_stop) {exit(-1);}
    else            {exit(0);}
}
