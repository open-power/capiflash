/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/bin/blockcache.c $                                  */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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
 * \brief Block Interface I/O Driver using cached data
 * \details
 *   This runs I/O to the capi Block Interface using Sync read/write.         \n
 *                                                                            \n
 *   Example:                                                                 \n
 *                                                                            \n
 * blockcache -d /dev/sg2:/dev/sg3 -P10000 -q400 -i                           \n
 *
 *******************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
#include <inttypes.h>
#include <ticks.h>
#include <math.h>
#include <pthread.h>

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif

#include <errno.h>

#define _8K        (8*1024)
#define _4K        (4*1024)
#define _1MSEC     1000
#define _1SEC      1000000
#define _10SEC     10000000
#define READ       1
#define WRITE      0
#define MAX_SGDEVS 8
#define PAGESIZE   (_4K*16)
#define INUSE      0x1111
#define GOOD       0x1001
#define PRE_CHK    0x0010
#define ALIGN      16

int      id          = 0;
size_t   nblks       = 0;
uint32_t nblocks     = (PAGESIZE/_4K);
uint32_t nRD         = 100;
uint64_t start_ticks = 0;
double   ns_per_tick = 0;
uint32_t nsecs       = 10;
char     devStrs[MAX_SGDEVS][64];
char    *devStr      = NULL;
char    *pstr        = NULL;
uint32_t devN        = 0;
uint32_t DEBUG       = 0;
uint32_t plun        = 1;
int      ids[_4K];

typedef struct
{
    pthread_mutex_t lock;
    uint64_t        inuse;
    uint64_t        state;
    uint64_t        lba;
    uint32_t        cid;           // chunk id
    uint64_t        next;          // next page to try
    uint32_t        rdN;
    uint32_t        wrN;
    uint64_t        rlat;
    uint64_t        wlat;
} page_state_t;

typedef struct
{
    uint8_t d[PAGESIZE];
} page_t;

int           PAGES  = 10000;
page_t       *page   = NULL;       // r/w  page for this extent
page_t       *data   = NULL;       // data page for this extent
page_state_t *pstate = NULL;       // state for each page

#define debug(fmt, ...) \
   do {if (DEBUG) {fprintf(stdout,fmt,##__VA_ARGS__); fflush(stdout);}} while(0)

#define NEXT_PAGE(_pI,_pN) _pI=(_pI+1)%_pN;

int global_stop=0;

#define PTR_ALIGN(_p, _a) ((void*)(((uintptr_t)(_p) + _a - 1) & ~(uintptr_t)(_a - 1)))

/**
********************************************************************************
** \brief run sync IO in a thread
*******************************************************************************/
void run_sync(void *d)
{
    int           tid     = (uint32_t)(uint64_t)d;
    int           rc      = 0;
    int           flags   = 0;
    int           i       = 0;
    int           rd      = 0;
    uint64_t      stime   = 0;
    uint64_t      sticks  = 0;
    uint64_t      cticks  = 0;
    uint32_t      pI      = 0;
    uint64_t     *p       = NULL;
    uint64_t     *p2      = NULL;
    page_t       *P       = NULL;
    page_t       *D       = NULL;
    page_state_t *S       = NULL;
    uint64_t      miss    = 0;

    if (plun) {flags = CBLK_GROUP_ID;}

    debug("start tid:%d\n", tid);

    /*--------------------------------------------------------------------------
     * loop running IO for nsecs
     *------------------------------------------------------------------------*/
    stime = getticks();
    do
    {
        if (global_stop) {return;}

        P = &page[pI];
        D = &data[pI];
        S = &pstate[pI];

        if (S->inuse || pthread_mutex_trylock(&S->lock) != 0)
        {
            ++miss;
            NEXT_PAGE(pI,PAGES);
            continue;
        }
        S->inuse = INUSE;

        /* read and compare */
        if (S->state & GOOD)
        {
            rd=1;
            /* pre-check if all cmpdata matches */
            if ((S->state & PRE_CHK) && memcmp(P, D, PAGESIZE) != 0)
            {
                debug("PRE-MISCOMPARE tid:%d lba:%ld\n", tid, S->lba);
            }
            debug("RD_BEG tid:%4d cid:%d page:%ld:%p lba:%ld\n",
                            tid, S->cid, P-page, P, S->lba);
            /* clobber read buf before reading data */
            memset(P,0x94,PAGESIZE);
            sticks = getticks();
            rc     = cblk_read(S->cid, P, S->lba, nblocks, flags);
            cticks = getticks()-sticks;
            if (errno == 0 && nblocks == rc)
            {
                /* check if all cmpdata matches */
                if (memcmp(P, D, PAGESIZE) != 0)
                {
                    printf("MISCOMPARE tid:%d lba:%ld\n", tid, S->lba);
                    global_stop=1;

                    if (1)
                    {
                        /* show which 64-bit word(s) miscompare */
                        p  = (uint64_t*)P;
                        p2 = (uint64_t*)D;
                        for (i=0; i<PAGESIZE/8; i++,p++,p2++)
                        {
                            if (*p != *p2) {printf("u64cmp: tid:%d addr:%p i:%d P:%016lx D:%016lx\n",tid,p,i,*p,*p2);}
                        }
                    }
                    memset(P,0x94,PAGESIZE);
                    rc = cblk_read(S->cid, P, S->lba, nblocks, flags);
                    if (errno == 0 && nblocks == rc)
                    {
                        /* check if all cmpdata matches */
                        if (memcmp(P, D, PAGESIZE) != 0)
                        {
                            printf("MISCOMPARE on 2nd read tid:%d lba:%ld\n", tid, S->lba);
                        }
                        else
                        {
                            printf("NO MISCOMPARE on 2nd read tid:%d lba:%ld\n", tid, S->lba);
                        }
                    }
                    memset(P,0x94,PAGESIZE);
                    rc = cblk_read(S->cid, P, S->lba, nblocks, flags);
                    if (errno == 0 && nblocks == rc)
                    {
                        /* check if all cmpdata matches */
                        if (memcmp(P, D, PAGESIZE) != 0)
                        {
                            printf("MISCOMPARE on 3rd read tid:%d lba:%ld\n", tid, S->lba);
                            exit(0);
                        }
                        else
                        {
                            printf("NO MISCOMPARE on 3rd read tid:%d lba:%ld\n", tid, S->lba);
                        }
                    }
                }
                else
                {
                    S->state |= PRE_CHK;
                }
            }
            else if (EBUSY != errno)
            {
                S->state &= ~PRE_CHK;
                printf("read err rc:%d errno:%d\n", rc, errno);
            }
        }
        else
        {
            rd=0;
            sticks = getticks();
            p      = (uint64_t*)D;
            /* assign a unique value to each uint64_t in the write buf */
            for (i=0; i<PAGESIZE/8; i++,p++) {*p=sticks+i;}
            debug("WR_BEG tid:%4d cid:%d page:%ld:%p lba:%ld\n",
                            tid, S->cid, P-page, D, S->lba);
            sticks = getticks();
            rc     = cblk_write(S->cid, D, S->lba, nblocks, flags);
            cticks = getticks()-sticks;
            if (nblocks == rc)
            {
                S->state = GOOD;
            }
            else if (EBUSY != errno)
            {
                S->state = 0;
                printf("write err rc:%d errno:%d\n", rc, errno);
            }
        }

        /* free page */
        S->inuse = 0;
        if (nblocks == rc)
        {
            if (rd)
            {
                S->rdN  += 1;
                S->rlat += cticks;
            }
            else
            {
                S->wrN  += 1;
                S->wlat += cticks;
            }
        }
        pthread_mutex_unlock(&S->lock);

        NEXT_PAGE(pI,PAGES);

    } while (SDELTA(stime,ns_per_tick) < nsecs);

    debug("exiting tid:%d miss:%ld\n", tid, miss);
    return;
}

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -r %rd        *the percentage of reads to issue (0..100)                 \n
**   -q queuedepth *the number of outstanding ops to maintain per device      \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -i            *run with interrupt threads enabled                        \n
**   -P            *number of cache pages                                     \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("  [-d device] [-q queuedepth] [-P pages] [-s secs] [-i]\n");
    exit(0);
}

/**
********************************************************************************
** \brief main
** \details
**   process input parms                                                      \n
**   open device                                                              \n
**   alloc memory                                                             \n
**   loop running IO until secs expire                                        \n
**   print IO stats                                                           \n
**   cleanup
*******************************************************************************/
int main(int argc, char **argv)
{
    char           *devStr      = NULL;
    char            FF          = 0xFF;
    char            c           = '\0';
    chunk_ext_arg_t ext         = 1;
    int             flags       = 0;
    int             i,rc        = 0;
    char           *_secs       = NULL;
    char           *_QD         = NULL;
    char           *_RD         = NULL;
    char           *_P          = NULL;
    uint32_t        intrp_thds  = 0;
    uint32_t        pths        = 0;
    uint32_t        QD          = 200;
    uint32_t        esecs       = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:s:P:hiD")))
    {
        switch (c)
        {
            case 'd': devStr     = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 's': _secs      = optarg; break;
            case 'P': _P         = optarg; break;
            case 'i': intrp_thds = 1;      break;
            case 'D': DEBUG      = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)      nsecs    = atoi(_secs);
    if (_QD)        QD       = atoi(_QD);
    if (_RD)        nRD      = atoi(_RD);
    if (_P)         PAGES    = atoi(_P);

    if (QD > _8K)   QD  = _8K;
    if (nRD > 100)  nRD = 100;

    if (devStr == NULL) usage();

    ns_per_tick = time_per_tick(1000, 100);

    while ((pstr=strsep(&devStr,":")))
    {
        if (devN == MAX_SGDEVS) {break;};
        debug("%s\n",pstr);
        sprintf(devStrs[devN++],"%s",pstr);
    }

    PAGES   = devN * ((PAGES / devN)+1);
    nblocks = PAGESIZE/_4K;
    pths    = (devN*QD > _8K) ? _8K : devN*QD;

    /*--------------------------------------------------------------------------
     * open device and set lun size
     *------------------------------------------------------------------------*/
    rc = cblk_init(NULL,0);
    if (rc)
    {
        fprintf(stderr,"cblk_init failed with rc = %d and errno = %d\n",
                rc,errno);
        exit(1);
    }
    flags = CBLK_OPN_GROUP; ext=16;
    if (!intrp_thds) {flags |= CBLK_OPN_NO_INTRP_THREADS;}

    for (i=0; i<devN; i++)
    {
        debug("cblk_open: %s QD:%d ext:%ld flags:0x%08x\n",
                devStrs[i%devN], QD, ext, flags);
        ids[i] = cblk_open(devStrs[i%devN], QD, O_RDWR, ext, flags);

        if (ids[i] == NULL_CHUNK_ID)
        {
            if      (ENOSPC == errno) fprintf(stderr,"cblk_open: ENOSPC\n");
            else if (ENODEV == errno) fprintf(stderr,"cblk_open: ENODEV\n");
            else                      fprintf(stderr,"cblk_open: errno:%d\n",errno);
            cblk_term(NULL,0);
            exit(errno);
        }
        rc = cblk_get_lun_size(ids[i], &nblks, flags);
        if (rc)
        {
            fprintf(stderr, "cblk_get_lun_size failed: errno: %d\n", errno);
            exit(errno);
        }
        debug("open: %s id:%d nblks:%ld\n", devStrs[i%devN], ids[i], nblks);
    }

    /*--------------------------------------------------------------------------
     * create threads to run sync IO
     *------------------------------------------------------------------------*/
    page_t *cache=NULL;
    if (!(cache=malloc(sizeof(page_t)*(PAGES+1))))
    {
        fprintf(stderr,"posix_memalign size:%ld rc:%d\n",
                        sizeof(page_t)*PAGES, rc);
        exit(0);
    }
    page = PTR_ALIGN(cache,ALIGN);

    if ((rc=posix_memalign((void**)&data, 128, sizeof(page_t)*PAGES)))
    {
        fprintf(stderr,"posix_memalign size:%ld rc:%d\n",
                        sizeof(page_t)*PAGES, rc);
        exit(0);
    }
    if (!(pstate=malloc(sizeof(page_state_t)*PAGES)))
    {
        fprintf(stderr,"posix_memalign size:%ld rc:%d\n",
                        sizeof(page_state_t)*PAGES, rc);
        exit(0);
    }

    for (i=0; i<PAGES; i++)
    {
        memset(pstate+i, 0, sizeof(page_state_t));
        pthread_mutex_init(&pstate[i].lock, NULL);
        pstate[i].rdN  = 0;
        pstate[i].wrN  = 0;
        pstate[i].rlat = 0;
        pstate[i].wlat = 0;
        pstate[i].next = 0;
        pstate[i].cid  = ids[i%devN];
        pstate[i].lba  = (i/devN) * nblocks;
        debug("page:%d cid:%d lba:%ld page:%p\n",
              i, pstate[i].cid, pstate[i].lba, page+i);
    }

    pthread_t pth[_8K];
    void* (*fp)(void*) = (void*(*)(void*))run_sync;

    start_ticks = getticks();
    for (i=0; i<pths; i++)
    {
        debug("pthread_create %d\n", i);
        pthread_create(pth+i, NULL, fp, (void*)(uint64_t)i);
    }
    for (i=0; i<pths; i++)
    {
        pthread_join(pth[i], NULL);
    }

    /*--------------------------------------------------------------------------
     * calc and print IO stats
     *------------------------------------------------------------------------*/
    esecs=SDELTA(start_ticks,ns_per_tick);

    uint64_t rlatT = 0;
    uint64_t wlatT = 0;
    uint32_t rdT   = 0;
    uint32_t wrT   = 0;
    uint32_t tcnt  = 0;

    for (i=0; i<PAGES; i++)
    {
        rdT   += pstate[i].rdN;
        wrT   += pstate[i].wrN;
        rlatT += pstate[i].rlat;
        wlatT += pstate[i].wlat;
    }
    tcnt = rdT + wrT;

    if (tcnt==0) {goto cleanup;}
    printf("r:%d q:%d s:%d p:%d n:%d S:%ld wlat:%d rlat:%d mbps:%d iops:%d",
            nRD, QD, nsecs, plun, nblocks, (nblks*4096)/(1024*1024*1024),
            wrT==0?0:(uint32_t)((wlatT*ns_per_tick)/wrT/1000),
            rdT==0?0:(uint32_t)((rlatT*ns_per_tick)/rdT/1000),
            ((tcnt*nblocks*4)/1024)/esecs,
            tcnt/esecs);
    if (plun && nblocks > 1) {printf(" 4k-iops:%d", (tcnt*nblocks)/esecs);}
    printf("\n"); fflush(stdout);

    if (plun) {flags=CBLK_GROUP_ID;}
    for (i=0; i<devN; i++)
        {debug("close %d\n", ids[i]); cblk_close(ids[i], flags);}

cleanup:
    free(cache);
    free(data);
    free(pstate);
    cblk_term(NULL,0);
    return 0;
}

