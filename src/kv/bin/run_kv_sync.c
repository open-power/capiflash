/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/run_kv_sync.c $                                   */
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
 *   Key/Value ARK Database Synchronous I/O Driver
 * \details
 *   This runs I/O to the Key/Value ARK Database using SYNC IO.               \n
 *   Each ark does set/get/exists/delete in a loop for a list of              \n
 *   keys/values. This code essentially runs write/read/compare/delete.       \n
 *   Threads are created to run the SYNC IO to the ark databases.             \n
 *                                                                            \n
 *   Example:                                                                 \n
 *                                                                            \n
 *   Run to memory with 4 arks:                                               \n
 *   run_kv_sync                                                              \n
 *   SYNC: ctxt:4 threads:100 k/v:16x65536: op/s:186666 io/s:1680000 secs:15  \n
 *                                                                            \n
 *   Run to a file with 1 ark:                                                \n
 *   run_kv_sync /kvstore                                                     \n
 *   SYNC: ctxt:1 threads:100 k/v:16x65536: op/s:26666 io/s:240000 secs:15    \n
 *                                                                            \n
 *   Run to a capi dev with 4 arks:                                           \n
 *   run_kv_sync /dev/sg10                                                    \n
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
#include <inttypes.h>
#include <assert.h>
#include <errno.h>

uint32_t KV_CONTEXTS =       4;
#define  KV_WORKERS_PER_CTXT 100
#define  KV_WORKERS_PTH      (KV_CONTEXTS*KV_WORKERS_PER_CTXT)
#define  KV_WORKER_MIN_SECS  15
#define  KLEN                16
#define  VLEN                64*1024
#define  LEN                 100

/**
********************************************************************************
** \brief
** struct to hold a generated key/value pair
*******************************************************************************/
typedef struct
{
    uint8_t key  [KLEN];
    uint8_t value[VLEN];
} kv_t;

/**
********************************************************************************
** \brief
** struct to hold a list ("database") of key/value pairs
*******************************************************************************/
typedef struct
{
    kv_t db[LEN];
} db_t;

/**
********************************************************************************
** \brief
** struct for all k/v databases, one for each thread. These are reused in
** each context
*******************************************************************************/
db_t dbs[KV_WORKERS_PER_CTXT];

/**
********************************************************************************
** \brief
** ark for each context
*******************************************************************************/
ARK **arks;

/**
********************************************************************************
** \brief
** struct for each worker thread
*******************************************************************************/
typedef struct
{
    ARK      *ark;
    kv_t     *db;
    pthread_t pth;
} worker_t;

/**
********************************************************************************
** \brief
** stop on error
*******************************************************************************/
#define KV_ERR(_msg, _rc)             \
do                                    \
{                                     \
    printf("\n(%s:%d)\n", _msg,_rc);  \
    return _rc;                       \
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

    assert(8 <= KLEN && 0 == KLEN%8);
    assert(8 <= VLEN && 0 == VLEN%8);

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<KV_WORKERS_PER_CTXT; i++)
    {
        for (j=0; j<LEN; j++)
        {
            for (p=(uint64_t*)(dbs[i].db[j].key);
                 p<(uint64_t*)(dbs[i].db[j].key+KLEN);
                 p++)
            {
                tag = (i << 16) | j;
                *p=tag;
            }
            for (p=(uint64_t*)(dbs[i].db[j].value);
                 p<(uint64_t*)(dbs[i].db[j].value+VLEN);
                 p++)
            {
                tag = (i << 16) | j;
                *p=tag;
            }
        }
    }
}

/**
********************************************************************************
** \brief
** issue a set for every key/value in the database to the ARK
*******************************************************************************/
uint32_t kv_load(ARK *ark, kv_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<LEN; i++)
    {
        while (EAGAIN == (rc=ark_set(ark,
                                     KLEN,
                                     db[i].key,
                                     VLEN,
                                     db[i].value,
                                     &res))) usleep(10000);
        if (rc)          KV_ERR("set1",rc);
        if (VLEN != res) KV_ERR("set2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
** issue a get and exists for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_query(ARK *ark, kv_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;
    uint8_t  gvalue[VLEN];

    for (i=0; i<LEN; i++)
    {
        while (EAGAIN == (rc=ark_get(ark,
                                     KLEN,
                                     db[i].key,
                                     VLEN,
                                     gvalue,
                                     0,
                                     &res))) usleep(10000);
        if (rc)                                     KV_ERR("get1", rc);
        if (VLEN != res)                            KV_ERR("get2",0);
        if (0 != memcmp(db[i].value, gvalue, VLEN)) KV_ERR("miscompare",0);

        while (EAGAIN == (rc=ark_exists(ark,
                                        KLEN,
                                        db[i].key,
                                        &res))) usleep(10000);
        if (rc)          KV_ERR("exists1",rc);
        if (VLEN != res) KV_ERR("exists2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
** issue a delete for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_del(ARK *ark, kv_t *db)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<LEN; i++)
    {
        while (EAGAIN == (rc=ark_del(ark,
                                     KLEN,
                                     db[i].key,
                                     &res))) usleep(10000);
        if (rc)          KV_ERR("del1", rc);
        if (VLEN != res) KV_ERR("del2",0);
    }
    return 0;
}

/**
********************************************************************************
** \brief
** loop until time elapses, sending set/get/exists/del for each k/v in database
*******************************************************************************/
void SGD(worker_t *w)
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


/**
********************************************************************************
** \brief
**   run the synchronous IO to the ARK
** \detail
**   create and init a key/value database for each ark                        \n
**   create arks and threads                                                  \n
**   wait for all threads to complete                                         \n
**   print iops and cleanup                                                   \n
*******************************************************************************/
int main(int argc, char **argv)
{
    void*  (*fp)(void*) = (void*(*)(void*))SGD;
    ARK     *ark;
    worker_t w[KV_WORKERS_PTH];
    uint64_t ops    = 0;
    uint64_t ios    = 0;
    uint32_t tops   = 0;
    uint32_t tios   = 0;
    uint32_t start  = 0;
    uint32_t stop   = 0;
    uint32_t e_secs = 0;
    uint32_t i      = 0;

    if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
      printf("usage: %s </dev/sgN>\n",argv[0]);
      exit(0);
    }

    /* if running to a file, cannot do multi-context */
    if (argv[1] != NULL && strncmp(argv[1], "/dev/", 5) != 0)
    {
        KV_CONTEXTS = 1;
    }

    arks = (ARK**)malloc(sizeof(ARK*) * KV_CONTEXTS);

    init_kv_db();

    printf("SYNC: ctxt:%d threads:%d k/v:%dx%d: ",
            KV_CONTEXTS, KV_WORKERS_PER_CTXT, KLEN, VLEN); fflush(stdout);

    start = time(0);

    /* create an ark and threads for each context */
    for (i=0; i<KV_WORKERS_PTH; i++)
    {
        if (0 == i % KV_WORKERS_PER_CTXT)
        {
            assert(0 == ark_create(argv[1],
                                   &ark,
                                   ARK_KV_VIRTUAL_LUN));
            assert(NULL != ark);
            arks[i / KV_WORKERS_PER_CTXT] = ark;
        }
        w[i].ark = ark;
        w[i].db  = dbs [i % KV_WORKERS_PER_CTXT].db;
        pthread_create(&(w[i].pth), NULL, fp, (void*)(w+i));
    }

    /* wait for all threads to complete */
    for (i=0; i<KV_WORKERS_PTH; i++)
    {
        pthread_join(w[i].pth, NULL);
    }

    stop = time(0);

    /* calc iops and cleanup */
    e_secs = stop - start;
    for (i=0; i<KV_CONTEXTS; i++)
    {
        (void)ark_stats(arks[i], &ops, &ios);
        tops += (uint32_t)ops;
        tios += (uint32_t)ios;

        assert(0 == ark_delete(arks[i]));
    }
    tops = tops / e_secs;
    tios = tios / e_secs;
    printf("op/s:%d io/s:%d secs:%d\n", tops, tios, e_secs);

    free(arks);
    return 0;
}
