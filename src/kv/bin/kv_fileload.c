/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bin/kv_fileload.c $                                    */
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

#define MAX_LEN 1024

typedef struct
{
    uint64_t done;
    uint64_t key;
    uint32_t val[6];
    uint32_t iss;
} kv_t;

typedef struct
{
    ARK     *ark;
    char     fn[4096];
    uint32_t fnI;
} loadkeyfn_t;

uint64_t LEN             = 10000;
int      KLEN            = 8;
uint64_t VLEN            = 24;
int      fnN             = 3;
double   ns_per_tick     = 0;
uint64_t seed            = UINT64_C(0x7DCA000000000000);
uint64_t ark_create_flag = 0;

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
                                  ARK_MAX_NASYNCS,                             \
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

    e_secs = SDELTA(start,ns_per_tick);
    e_secs = e_secs==0?1:e_secs;

    ark_stats(ark, &ops, &ios);

    printf("GETN tops:%8ld tios:%8ld op/s:%8ld io/s:%8ld secs:%d cnt:%ld\n",
            ops-s_ops, ios-s_ios,
            (ops-s_ops)/e_secs, (ios-s_ios)/e_secs, e_secs, cnt);
exit:
    if (buf) {free(buf);}
    if (!rc) {printf("GETN failed, errno:%d\n", errno);}
    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void kv_genkeyfn(int num)
{
    FILE    *fp;
    char     fn[16];
    char     out[128];
    uint64_t key;
    uint32_t d[6];
    int      fnI = 0;
    uint64_t cur = 0;
    int      i   = 0;
    int      j   = 0;
    struct stat sbuf;

    srand(time(0));

    for (fnI=1; fnI<=fnN; fnI++)
    {
        sprintf(fn,"keys%d",fnI);

        if (stat(fn,&sbuf) != 0)
        {
            printf("GEN  %s %d\n",fn, num);
            fp = fopen(fn,"w");

            for (i=0; i<num; i++)
            {
                GEN_VAL(&key, seed+cur++, 8);
                for (j=0;j<=5;j++) {d[j]=rand();}

                sprintf(out,"%ld,%d,%d,%d,%d,%d,%d",key,d[0],d[1],d[2],d[3],d[4],d[5]);
                fprintf(fp,"%s\n",out);
            }
            fclose(fp);
        }
        else {printf("FOUND %s\n", fn);}
    }
    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void kv_fileload_cb(int errcode, uint64_t dt, int64_t res)
{
    if (dt)
    {
        uint64_t *done = (uint64_t*)dt;
        *done = 1;
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void* kv_fileload(void *p)
{
    loadkeyfn_t *parms         = p;
    FILE        *fp            = NULL;
    size_t       len           = MAX_LEN;          // max len of line from file
    char         line[MAX_LEN] = {0};
    char        *pstr          = NULL;
    int          cmdN          = 300;              // num cmds issued at a time
    int          kvN           = 60000;            // num k/v read into buffer
    kv_t        *kvs           = NULL;             // buffer to hold k/v's from file
    queue_t     *wq            = queue_new(kvN);   // work queue
    queue_t     *sq            = queue_new(cmdN);  // submission queue
    int          i             = 0;
    int          rc            = 0;
    int32_t      tag           = 0;
    uint32_t     num           = 0;

    fp  = fopen(parms->fn,"r");
    kvs = malloc(sizeof(kv_t) * kvN);

    while (fp && rc != -1)
    {
        // load a buffer of k/v from the file
        for (i=0; i<kvN; i++)
        {
            // get a csv line and parse it into key and value
            if ((rc=getline((char**)&line, &len, fp)) == -1) {break;}
            pstr=strsep((char**)&line,",");
            kvs[i].key = atoll(pstr);
            int valI = 0;
            int j    = 0;
            for (j=1; j<=6; j++)
            {
                pstr=strsep((char**)&line,",");
                kvs[i].val[valI++] = atoi(pstr);
            }
            // insert the k/v into the work queue
            kvs[i].done = 0;
            kvs[i].iss  = 0;
            queue_enq_unsafe(wq,i);
        }

        // loop until all the k/v entries in the work queue are in the Arkdb
        while (!Q_EMPTY(wq) || !Q_EMPTY(sq))
        {
            // loop while there are k/v in wq and sq
            while (!Q_EMPTY(wq) && !Q_FULL(sq))
            {
                // get a k/v from work queue and put into Arkdb
                queue_deq_unsafe(wq,&tag);
                rc = ark_set_async_cb(parms->ark,    KLEN,
                                      &kvs[tag].key, VLEN,
                                      &kvs[tag].val, kv_fileload_cb,
                                      (uint64_t)&kvs[tag].done);
                // if the Arkdb queue is full, put k/v back into wq
                if (rc == EAGAIN)
                {
                    queue_enq_unsafe(wq,tag);
                    break;
                }
                // put k/v into the submission queue
                queue_enq_unsafe(sq,tag);
            }

            // loop through submission queue and check if cmds are complete
            num = Q_CNT(sq);
            for (i=0; i<num; i++)
            {
                // EAGAIN, no more cmds in sq, go submit more
                if (EAGAIN==queue_deq_unsafe(sq,&tag)) {break;}
                // cmd not dome, put cmd back into sq
                if (!kvs[tag].done) {queue_enq_unsafe(sq,tag);}
            }
        }
    }

    free(kvs);
    queue_free(wq);
    queue_free(sq);
    return 0;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void kv_loadkeys(ARK* ark, char *fn)
{
    uint64_t     s_ops      = 0;
    uint64_t     s_ios      = 0;
    uint64_t     ops        = 0;
    uint64_t     ios        = 0;
    uint32_t     e_secs     = 0;
    uint64_t     start      = 0;
    int          i          = 0;
    char        *pstr       = NULL;
    loadkeyfn_t *parms      = malloc(fnN * sizeof(loadkeyfn_t));
    pthread_t    loadpths[fnN];

    bzero(parms,fnN * sizeof(loadkeyfn_t));

    if (!fn)
    {
        // create a file(s) with generated random k/v's
        kv_genkeyfn(LEN/fnN);
        for (i=0; i<fnN; i++) {sprintf(parms[i].fn,"keys%d",i+1);}
    }
    else
    {
        // copy colon separated filenames into parms struct
        fnN=0;
        while ((pstr=strsep((&fn),":")))
        {
            sprintf(parms[fnN++].fn,"%s",pstr);
        }
    }

    ark_stats(ark, &s_ops, &s_ios);
    start = getticks();

    // create threads to load the files in to the Arkdb
    void* (*fp)(void*) = kv_fileload;
    for (i=0; i<fnN; i++)
    {
        parms[i].ark = ark;
        parms[i].fnI = i+1;
        pthread_create(&loadpths[i], NULL, fp, (void*)&parms[i]);
    }
    // wait for all threads to finish
    for (i=0; i<fnN; i++) {pthread_join(loadpths[i], NULL);}

    // calc and print stats
    e_secs = SDELTA(start,ns_per_tick);
    e_secs = e_secs==0?1:e_secs;

    ark_stats(ark, &ops, &ios);

    printf("LOAD tops:%8ld tios:%8ld op/s:%8ld io/s:%8ld secs:%d\n",
            ops-s_ops, ios-s_ios,
            (ops-s_ops)/e_secs, (ios-s_ios)/e_secs, e_secs);

    free(parms);
    return;
}

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run KV IO                              \n
**   -q queuedepth *the number of outstanding ops to maintain to the Arkdb    \n
**   -l len        *number of k/v pairs to set/get (default is 10000)         \n
**   -t threads    *arkdb threads                                             \n
**   -h hte        *ht entries                                                \n
**   -c 0          *disable the metadata cache                                \n
**   -n            *write to a new persisted ark                              \n
**   -r            *read the existing persisted ark                           \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("   [-d device] [-f fn1:fn2] [-q qdepth] [-l len]"
           " [-t thds] [-h hte] [-n] [-r] [-c]\n");
    exit(0);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int main(int argc, char **argv)
{
    ARK     *ark         = NULL;
    char     FF          = 0xFF;
    char     c           = '\0';
    char    *dev         = NULL;
    char    *_fn         = NULL;
    char    *_len        = NULL;
    char    *_htc        = NULL;
    char    *_hte        = NULL;
    char    *_thds       = NULL;
    char    *_getN       = NULL;
    uint32_t readonly    = 0;
    uint32_t new         = 0;
    uint32_t getN        = 0;
    uint32_t htc         = 1;
    uint64_t hte         = ARK_VERBOSE_HASH_DEF;
    uint32_t thds        = ARK_VERBOSE_NTHRDS_DEF;
    uint32_t verbose     = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "f:d:l:c:t:h:G:Vrhn")))
    {
        switch (c)
        {
            case 'd': dev        = optarg;   break;
            case 'f': _fn        = optarg;   break;
            case 'l': _len       = optarg;   break;
            case 'c': _htc       = optarg;   break;
            case 'h': _hte       = optarg;   break;
            case 't': _thds      = optarg;   break;
            case 'G': _getN      = optarg;   break;
            case 'n': new        = 1;        break;
            case 'r': readonly   = 1; new=0; break;
            case 'V': verbose    = 1;        break;
            case '?': usage();               break;
        }
    }
    if (_len)  {LEN         = atoll(_len);}
    if (_htc)  {htc         = atoi (_htc);}
    if (_hte)  {hte         = atoll(_hte);}
    if (_thds) {thds        = atoi (_thds);}
    if (_getN) {getN        = atoi (_getN);}

    ns_per_tick = time_per_tick(1000, 100);

    if      (new)      {ark_create_flag = ARK_KV_PERSIST_STORE;}
    else if (readonly) {ark_create_flag = ARK_KV_PERSIST_LOAD;}

    if (htc) {ark_create_flag |= ARK_KV_HTC;}

    NEW_ARK(dev, &ark, thds, hte);

    if (!readonly) {kv_loadkeys(ark,_fn);}
    if (getN) {kv_getN(ark, KLEN, VLEN, LEN, getN);}

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
    return 0;
}
