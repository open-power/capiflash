/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/run_kv_benchmark.c $                              */
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

#include <arkdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>
#include <assert.h>

#define KV_PTHS             128
#define KV_PTHS_BIG         80
#define KV_NASYNC           128
#define KV_BASYNC           8*1024
#define KV_NPOOL            20
#define KV_WORKER_MIN_SECS  15

#define KLEN                16
#define VLEN_4k             4000
#define VLEN_64k            64000
#ifdef _AIX
#define VLEN_500k           180*1024
#else
#define VLEN_500k           500*1024
#endif
#define LEN_4k              100    // make this 50 to view even % r/w
#define LEN_BIG             5

#define MAX_IO_ARKS         4
#define MAX_IO_PTHS         60

typedef struct
{
    uint8_t *key;
    uint8_t *value;
} kv_t;

typedef struct
{
    kv_t     *kv;
    uint32_t klen;
    uint32_t vlen;
    uint32_t LEN;
} db_t;

typedef struct
{
    ARK      *ark;
    db_t     *db;
    pthread_t pth;
} worker_t;

db_t    *dbs;
void*  (*fp)(void*);
uint32_t testcase_start  = 0;
uint32_t testcase_stop   = 0;
uint64_t ark_create_flag = ARK_KV_VIRTUAL_LUN;

/*******************************************************************************
 ******************************************************************************/
#define KV_ERR(_msg, _rc)             \
do                                    \
{                                     \
    printf("\n(%s:%d)\n", _msg,_rc);  \
    return _rc;                       \
} while (0)

/*******************************************************************************
 ******************************************************************************/
#define NEW_ARK(_dev, _ark) \
        assert(0 == ark_create_verbose(_dev, _ark,              \
                                       1048576,                 \
                                       4096,                    \
                                       1048576,                 \
                                       KV_NPOOL,                \
                                       KV_NASYNC,               \
                                       KV_BASYNC,               \
                                       ark_create_flag));       \
        assert(NULL != ark);                                    \

/*******************************************************************************
 ******************************************************************************/
void init_kv_db(uint32_t n_pth, uint32_t klen, uint32_t vlen, uint32_t LEN)
{
    uint32_t i,j;
    uint64_t *p,tag;

    assert(8 <= klen && 0 == klen%8);
    assert(8 <= vlen && 0 == vlen%8);

    dbs = (db_t*)malloc(n_pth*sizeof(db_t));
    assert(dbs);

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<n_pth; i++)
    {
        dbs[i].klen = klen;
        dbs[i].vlen = vlen;
        dbs[i].LEN  = LEN;
        dbs[i].kv = malloc(LEN*sizeof(kv_t));
        assert(dbs[i].kv);
        for (j=0; j<LEN; j++)
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

/*******************************************************************************
 ******************************************************************************/
void free_kv_db(uint32_t n_pth, uint32_t LEN)
{
    uint32_t i,j;

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<n_pth; i++)
    {
        for (j=0; j<LEN; j++)
        {
            free(dbs[i].kv[j].key);
            free(dbs[i].kv[j].value);
        }
        free(dbs[i].kv);
    }
    free(dbs);
}

/*******************************************************************************
 ******************************************************************************/
uint32_t kv_load(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<db->LEN; i++)
    {
        while (EAGAIN == (rc=ark_set(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     db->vlen,
                                     db->kv[i].value,
                                     &res))) usleep(10000);
        if (rc)              KV_ERR("set1",rc);
        if (db->vlen != res) KV_ERR("set2",0);
    }
    return 0;
}

/*******************************************************************************
 ******************************************************************************/
uint32_t kv_query(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;
    uint8_t  gvalue[db->vlen];

    for (i=0; i<db->LEN; i++)
    {
        while (EAGAIN == (rc=ark_get(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     db->vlen,
                                     gvalue,
                                     0,
                                     &res))) usleep(10000);
        if (rc)               KV_ERR("get1", rc);
        if (db->vlen != res)  KV_ERR("get2",0);

        while (EAGAIN == (rc=ark_exists(ark,
                                        db->klen,
                                        db->kv[i].key,
                                        &res))) usleep(10000);
        if (rc)              KV_ERR("exists1",rc);
        if (db->vlen != res) KV_ERR("exists2",0);
    }
    return 0;
}

/*******************************************************************************
 ******************************************************************************/
uint32_t kv_exists(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<db->LEN; i++)
    {
        while (EAGAIN == (rc=ark_exists(ark,
                                        db->klen,
                                        db->kv[i].key,
                                        &res))) usleep(10000);
        if (rc)              KV_ERR("exists1",rc);
        if (db->vlen != res) KV_ERR("exists2",0);
    }
    return 0;
}

/*******************************************************************************
 ******************************************************************************/
uint32_t kv_del(ARK *ark, db_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<db->LEN; i++)
    {
        while (EAGAIN == (rc=ark_del(ark,
                                     db->klen,
                                     db->kv[i].key,
                                     &res))) usleep(10000);
        if (rc)              KV_ERR("del1", rc);
        if (db->vlen != res) KV_ERR("del2",0);
    }
    return 0;
}

/*******************************************************************************
 ******************************************************************************/
void do_100_percent_read(worker_t *w)
{
    int start=0,cur=0,rc=0;

    kv_load(w->ark, w->db);

    start = time(0);
    do
    {
        if ((rc=kv_query(w->ark, w->db))) break;
        cur = time(0);
    }
    while (cur-start < KV_WORKER_MIN_SECS);

    kv_del(w->ark, w->db);
}

/*******************************************************************************
 ******************************************************************************/
void do_75_percent_read(worker_t *w)
{
    int start=0,cur=0,rc=0;

    start = time(0);
    do
    {
        if ((rc=kv_load (w->ark, w->db))) break;
        if ((rc=kv_query(w->ark, w->db))) break;
        if ((rc=kv_del  (w->ark, w->db))) break;
        cur = time(0);
    }
    while (cur-start < KV_WORKER_MIN_SECS);
}

/*******************************************************************************
 ******************************************************************************/
void do_50_percent_read(worker_t *w)
{
    int start=0,cur=0,rc=0;

    start = time(0);
    do
    {
        if ((rc=kv_load  (w->ark, w->db))) break;
        if ((rc=kv_exists(w->ark, w->db))) break;
        if ((rc=kv_del   (w->ark, w->db))) break;
        cur = time(0);
    }
    while (cur-start < KV_WORKER_MIN_SECS);
}

/*******************************************************************************
 ******************************************************************************/
void get_stats(ARK *ark, uint32_t *ops, uint32_t *ios)
{
    uint64_t ops64    = 0;
    uint64_t ios64    = 0;

    (void)ark_stats(ark, &ops64, &ios64);
    *ops = (uint32_t)ops64;
    *ios = (uint32_t)ios64;
}

/*******************************************************************************
 ******************************************************************************/
void run_io(ARK *ark, uint32_t pths)
{
    worker_t w[pths];
    uint32_t i      = 0;
    uint32_t ops    = 0;
    uint32_t ios    = 0;
    uint32_t e_secs = 0;

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
    printf("op/s:%7d io/s:%7d secs:%d\n", ops, ios, e_secs);
}

/*******************************************************************************
 ******************************************************************************/
void run_max_io(char *dev, uint32_t vlen, uint32_t LEN)
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

    init_kv_db(MAX_IO_PTHS, KLEN, vlen, LEN);

    testcase_start = time(0);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        NEW_ARK(dev, &ark[i]);

        /* create all threads */
        for (j=0; j<MAX_IO_PTHS; j++)
        {
            w[i][j].ark = ark[i];
            w[i][j].db  = dbs+(j % KV_PTHS);
            pthread_create(&(w[i][j].pth), NULL, fp, (void*)(&w[i][j]));
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
    printf("op/s:%7d io/s:%7d secs:%d\n", ops, ios, e_secs);

    free_kv_db(MAX_IO_PTHS, LEN);

    for (i=0; i<MAX_IO_ARKS; i++)
    {
        assert(0 == ark_delete(ark[i]));
    }
}

/*******************************************************************************
 ******************************************************************************/
int main(int argc, char **argv)
{
    ARK *ark;
    int  i=1;

    if (argv[1] == NULL)
    {
        printf("dev name required as parameter\n");
        exit(-1);
    }

    if (0 == strncmp(argv[1], "-p", 7))
    {
        printf("Attempting to run with physical lun\n");
        ark_create_flag = ARK_KV_PERSIST_STORE;
        i               = 2;
    }

    printf("%s: SYNC: npool:%d nasync:%d basync:%d\n",
                 argv[i], KV_NPOOL, KV_NASYNC, KV_BASYNC);
    fflush(stdout);

    /* 1 ctxt, vlen 4k */
    init_kv_db(KV_PTHS, KLEN, VLEN_4k, LEN_4k);
    NEW_ARK(argv[i], &ark);
    printf("ctxt:1   4k-k/v: 75%%  read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_75_percent_read;
    run_io(ark, KV_PTHS);
    free_kv_db(KV_PTHS, LEN_4k);
    assert(0 == ark_delete(ark));

    /* 1 ctxt, vlen 4k */
    init_kv_db(KV_PTHS, KLEN, VLEN_4k, LEN_4k);
    NEW_ARK(argv[i], &ark);
    printf("ctxt:1   4k-k/v: 100%% read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_100_percent_read;
    run_io(ark, KV_PTHS);
    free_kv_db(KV_PTHS, LEN_4k);
    assert(0 == ark_delete(ark));

    /* 1 ctxt, vlen 64k */
    init_kv_db(KV_PTHS_BIG, KLEN, VLEN_64k, LEN_BIG);
    NEW_ARK(argv[i], &ark);
    printf("ctxt:1  64k-k/v: 100%% read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_100_percent_read;
    run_io(ark, KV_PTHS_BIG);
    free_kv_db(KV_PTHS_BIG, LEN_BIG);
    assert(0 == ark_delete(ark));

    /* 1 ctxt, vlen 1mb */
    init_kv_db(KV_PTHS_BIG, KLEN, VLEN_500k, LEN_BIG);
    NEW_ARK(argv[i], &ark);

    printf("ctxt:1 500k-k/v: 100%% read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_100_percent_read;
    run_io(ark, KV_PTHS_BIG);

    free_kv_db(KV_PTHS_BIG, LEN_BIG);
    assert(0 == ark_delete(ark));

    /* if physical lun, cannot run multi-context, so exit */
    if (ARK_KV_PERSIST_STORE == ark_create_flag) return 0;

    printf("ctxt:4   4k-k/v: 100%% read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_100_percent_read;
    run_max_io(argv[i], VLEN_4k, LEN_4k);

    printf("ctxt:4  64k-k/v: 100%% read:     "); fflush(stdout);
    fp = (void*(*)(void*))do_100_percent_read;
    run_max_io(argv[i], VLEN_64k, LEN_BIG);

    return 0;
}
