/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/run_kv_persist.c $                                */
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
 *   Key/Value ARK Database Persistence Migration Driver
 *
 * \details
 *   -create and validate persisted ark data on capi flash or a file          \n
 *   -can be used to test migration from one release to another               \n
 *    ie, create a persisted ark with this binary from the build of an older  \n
 *        release, and validate the ark with this binary from a newer release \n
 *                                                                            \n
 *   Example:                                                                 \n
 *                                                                            \n
 *   create persisted ark:   run_kv_persist /dev/sgX -new                     \n
 *   validate persisted ark: run_kv_persist /dev/sgX
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

#define KV_WORKERS          100
#define KV_WORKER_MIN_SECS  10

#define MAX_KLEN            64
#define MAX_VLEN            64
#define MAX_LEN             10000
#define LEN                 1000

/**
********************************************************************************
** struct to hold a generated key/value pair
*******************************************************************************/
typedef struct
{
    uint8_t  key  [MAX_KLEN];
    uint8_t  value[MAX_VLEN];
    uint32_t klen;
    uint32_t vlen;
} kv_t;

/**
********************************************************************************
** struct to hold a list ("database") of key/value pairs
*******************************************************************************/
typedef struct
{
    kv_t db[MAX_LEN];
} db_t;

/**
********************************************************************************
** struct for all k/v databases, one for each thread. These are reused in
** each context
*******************************************************************************/
db_t dbs[KV_WORKERS];

/**
********************************************************************************
** persisted ark
*******************************************************************************/
ARK *ark;

/**
********************************************************************************
** struct for each worker thread
*******************************************************************************/
typedef struct
{
    ARK      *ark;
    kv_t     *db;
    uint32_t  db_len;
    uint32_t  loaded;
    pthread_t pth;
} worker_t;

/**
********************************************************************************
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
** setup all the databases with unique key/values, lengths a multiple of 8bytes
*******************************************************************************/
void init_kv_db(uint32_t klen, uint32_t vlen, uint32_t len)
{
    uint32_t i,j;
    uint64_t *p,tag;

    assert(8 <= klen && 0 == klen%8);
    assert(8 <= vlen && 0 == vlen%8);

    /* ensure unique keys for each k/v in all dbs */
    for (i=0; i<KV_WORKERS; i++)
    {
        for (j=0; j<len; j++)
        {
            dbs[i].db[j].klen = klen;
            dbs[i].db[j].vlen = vlen;
            for (p=(uint64_t*)(dbs[i].db[j].key);
                 p<(uint64_t*)(dbs[i].db[j].key+klen);
                 p++)
            {
                tag = UINT64_C(0x1111111100000000) | (i << 16) | j;
                *p=tag;
            }
            for (p=(uint64_t*)(dbs[i].db[j].value);
                 p<(uint64_t*)(dbs[i].db[j].value+vlen);
                 p++)
            {
                tag = UINT64_C(0x2222222200000000) | (i << 16) | j;
                *p=tag;
            }
        }
    }
}

/**
********************************************************************************
** issue a set for every key/value in the database to the ARK
*******************************************************************************/
uint32_t kv_load(ARK *ark, kv_t *db, uint32_t len)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<len; i++)
    {
        while (EAGAIN == (rc=ark_set(ark,
                                     db[i].klen,
                                     db[i].key,
                                     db[i].vlen,
                                     db[i].value,
                                     &res))) usleep(10000);
        if (rc)                KV_ERR("set1",rc);
        if (db[i].vlen != res) KV_ERR("set2",0);
    }
    return 0;
}

/**
********************************************************************************
** issue a get and exists for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_query(ARK *ark, kv_t *db, uint32_t len)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;
    uint8_t  gvalue[MAX_VLEN];

    for (i=0; i<len; i++)
    {
        while (EAGAIN == (rc=ark_get(ark,
                                     db[i].klen,
                                     db[i].key,
                                     MAX_VLEN,
                                     gvalue,
                                     0,
                                     &res))) usleep(10000);
        if (rc)                                     KV_ERR("get1", rc);
        if (db[i].vlen != res)                      KV_ERR("get2",0);
        if (0 != memcmp(db[i].value, gvalue,db[i].vlen)) KV_ERR("miscompare",0);

        while (EAGAIN == (rc=ark_exists(ark,
                                        db[i].klen,
                                        db[i].key,
                                        &res))) usleep(10000);
        if (rc)                KV_ERR("exists1",rc);
        if (db[i].vlen != res) KV_ERR("exists2",0);
    }
    return 0;
}

/**
********************************************************************************
** issue a delete for each k/v in the database to the ARK
*******************************************************************************/
uint32_t kv_del(ARK *ark, kv_t *db, uint32_t len)
{
    uint32_t i   = 0;
    uint32_t rc  = 0;
    int64_t  res = 0;

    for (i=0; i<len; i++)
    {
        while (EAGAIN == (rc=ark_del(ark,
                                     db[i].klen,
                                     db[i].key,
                                     &res))) usleep(10000);
        if (rc)                KV_ERR("del1", rc);
        if (db[i].vlen != res) KV_ERR("del2",0);
    }
    return 0;
}

/**
********************************************************************************
** loop until time elapses, sending set/get/exists/del for each k/v in database
*******************************************************************************/
void SGD(worker_t *w)
{
    int start=0,cur=0,rc=0;

    start = time(0);

    /* if the db is supposed to be loaded, then do a query to validate */
    if (w->loaded && kv_query(w->ark, w->db, w->db_len))
    {
        printf("Failed to verify the loaded kv database\n");
    }

    /* loop the SGD to run IO */
    do
    {
        if ((rc=kv_load (w->ark, w->db, w->db_len))) break;
        if ((rc=kv_query(w->ark, w->db, w->db_len))) break;
        if ((rc=kv_del  (w->ark, w->db, w->db_len))) break;

        cur = time(0);
    }
    while (cur-start < KV_WORKER_MIN_SECS);

    /* leave this db loaded */
    kv_load (w->ark, w->db, w->db_len);
    kv_query(w->ark, w->db, w->db_len);
}

/**
********************************************************************************
** loop until time elapses, sending set/get/exists/del for each k/v in database
*******************************************************************************/
void SGD_IO(worker_t *w)
{
    int rc=0;

    if ((rc=kv_load (w->ark, w->db, w->db_len))) return;
    if ((rc=kv_query(w->ark, w->db, w->db_len))) return;
    if ((rc=kv_del  (w->ark, w->db, w->db_len))) return;
}

/**
********************************************************************************
** \brief
**   process/validate a new or existing persistent ark database
** \details
**   -parse input parms                                                       \n
**   -if newark                                                               \n
**     create a new ark and load a filler k/v database                        \n
**   -else                                                                    \n
**     load the persistent data from the existing ark and validate it         \n
**   -create threads to run set/get/delete loops for a time period,           \n
**    and leave the k/v db loaded                                             \n
**   -wait for threads to complete                                            \n
**   -calculate iops for the SGD loops                                        \n
**   -if newark                                                               \n
**      add another filler k/v database                                       \n
**   -create threads to run SGD loops for a time period                       \n
**   -wait for threads to complete
*******************************************************************************/
int main(int argc, char **argv)
{
    void*  (*fp)   (void*) = (void*(*)(void*))SGD;
    void*  (*fp_io)(void*) = (void*(*)(void*))SGD_IO;
    ARK     *ark;
    worker_t w[KV_WORKERS];
    uint64_t ops    = 0;
    uint64_t ios    = 0;
    uint32_t tops   = 0;
    uint32_t tios   = 0;
    uint32_t start  = 0;
    uint32_t stop   = 0;
    uint32_t e_secs = 0;
    uint32_t i      = 0;
    int64_t  count  = 0;
    int64_t  newark = 0;
    uint64_t flags  = 0;

    if (argc < 2 || (argc==2 && argv[1][0]=='-')) {
      printf("usage: %s </dev/sgN>\n",argv[0]);
      exit(0);
    }

    if (argv[1] == NULL)
    {
        printf("dev name required as parameter\n");
        exit(-1);
    }
    if (argv[2] != NULL && strncmp(argv[2],"-n",2) == 0)
    {
        newark=1;
    }

    if (newark)
    {
        printf("create new ark\n");
        flags = ARK_KV_PERSIST_STORE;
    }
    else
    {
        printf("load the existing ark data\n");
        flags = ARK_KV_PERSIST_STORE | ARK_KV_PERSIST_LOAD;
    }

    assert(0 == ark_create(argv[1], &ark, flags));
    assert(NULL != ark);
    assert(0 == ark_count(ark, &count));
    printf("ark contains %ld keys\n", count);

    if (newark)
    {
        printf("load %d pre-filler k/v\n", MAX_LEN);
        init_kv_db(MAX_KLEN-8, MAX_VLEN, MAX_LEN);
        kv_load (ark, dbs[0].db, MAX_LEN);
        kv_query(ark, dbs[0].db, MAX_LEN);
    }

    if (newark) {printf("load %d k/v\n", KV_WORKERS*LEN);}
    else if (count == 0)
    {
        printf("no ark data found, create an ark with \'-new' option\n");
        goto exit;
    }
    else {printf("validate existing %d k/v\n", KV_WORKERS*LEN);}

    init_kv_db(MAX_KLEN, MAX_VLEN, LEN);

    /* create the ark threads to run IO */
    start = time(0);
    for (i=0; i<KV_WORKERS; i++)
    {
        w[i].loaded = (count>0);
        w[i].ark    = ark;
        w[i].db     = dbs[i].db;
        w[i].db_len = LEN;
        pthread_create(&(w[i].pth), NULL, fp, (void*)(w+i));
    }
    /* wait for all threads to complete */
    for (i=0; i<KV_WORKERS; i++)
    {
        pthread_join(w[i].pth, NULL);
    }
    /* calc iops */
    stop   = time(0);
    e_secs = stop - start;
    (void)ark_stats(ark, &ops, &ios);
    tops += (uint32_t)ops;
    tios += (uint32_t)ios;
    tops  = tops / e_secs;
    tios  = tios / e_secs;
    printf("op/s:%d io/s:%d secs:%d\n", tops, tios, e_secs);

    if (newark)
    {
        printf("load %d post-filler k/v\n", MAX_LEN);
        init_kv_db(MAX_KLEN-16, MAX_VLEN, MAX_LEN);
        kv_load (ark, dbs[0].db, MAX_LEN);
        kv_query(ark, dbs[0].db, MAX_LEN);
    }

    /* validate IO still runs */
    printf("set/get/del an additional %d k/v\n", KV_WORKERS*LEN);
    init_kv_db(MAX_KLEN-24, MAX_VLEN, LEN);
    /* create the ark threads to run IO */
    for (i=0; i<KV_WORKERS; i++)
    {
        w[i].loaded = 0;
        w[i].ark    = ark;
        w[i].db     = dbs[i].db;
        w[i].db_len = LEN;
        pthread_create(&(w[i].pth), NULL, fp_io, (void*)(w+i));
    }
    /* wait for all threads to complete */
    for (i=0; i<KV_WORKERS; i++)
    {
        pthread_join(w[i].pth, NULL);
    }
    assert(0 == ark_count(ark, &count));
    printf("persist ark with %ld keys\n", count);

exit:
    assert(0 == ark_delete(ark));
    return 0;
}
