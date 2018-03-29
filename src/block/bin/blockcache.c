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
 * blockcache -d /dev/sg2:/dev/sg3 -e100 -q10                                 \n
 * r:100 q:50 s:4 p:1 n:2 S:2980 lat:367 mbps:2087 iops:267150 4k-iops:534300 \n
 *                                                                            \n
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
#define NBUFS      _8K
#define _1MSEC     1000
#define _1SEC      1000000
#define _10SEC     10000000
#define READ       1
#define WRITE      0
#define MAX_SGDEVS 8
#define PAGESIZE   _8K
#define PAGES      2048
#define INUSE      0x1111
#define GOOD       0x1001

int      id          = 0;
uint32_t seq         = 0;
size_t   nblks       = 0;
uint32_t nblocks     = (PAGESIZE/_4K);
uint32_t nRD         = 100;
uint64_t start_ticks = 0;
double   ns_per_tick = 0;
uint32_t nsecs       = 20;
char     devStrs[MAX_SGDEVS][64];
char    *devStr      = NULL;
char    *pstr        = NULL;
uint32_t devN        = 0;
uint32_t DEBUG       = 0;
uint32_t plun        = 1;
int      ids[_4K];

typedef struct
{
    uint64_t inuse;
    uint64_t state;
} state_t;

typedef struct
{
    union
    {
        uint64_t lba;
        uint8_t  pad1[128];
    };
} cmpdata_t;

typedef struct
{
    union
    {
        state_t s;
        uint8_t pad[128];
    };
    cmpdata_t cd;
} save_t;

typedef struct
{
    union
    {
        save_t  data;
        uint8_t pad1[PAGESIZE];
    };
    union
    {
        save_t  save;
        uint8_t pad2[PAGESIZE];
    };
} page_t;

typedef struct
{
    pthread_mutex_t lock;
    uint32_t        cid;
    uint64_t        next;
    uint32_t        rdN;
    uint32_t        wrN;
    uint64_t        rlat;
    uint64_t        wlat;
    uint32_t        idx;
} extdata_t;

typedef struct
{
    union
    {
        extdata_t ed;
        uint8_t   pad[128];
    };
    page_t        page[PAGES];
} extent_t;

extent_t *cache   = NULL;
uint32_t extN     = 300;
uint32_t MEMCMP   = 0;

#define debug(fmt, ...) \
   do {if (DEBUG) {fprintf(stdout,fmt,##__VA_ARGS__); fflush(stdout);}} while(0)

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -r %rd        *the percentage of reads to issue (0..100)                 \n
**   -q queuedepth *the number of outstanding ops to maintain per device      \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -i            *run with interrupt threads enabled                        \n
**   -M            *do a memcmp of the entire read buf                        \n
**   -e            *number of cache extents                                   \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("  [-d device] [-r %%rd] [-q queuedepth] [-e extents] [-s secs] "
           "[-i] [-M]\n");
    exit(0);
}

/**
********************************************************************************
** \brief run sync IO in a thread
*******************************************************************************/
void run_sync(void *ptr)
{
    int       tid     = (uint32_t)(uint64_t)ptr;
    int       rc      = 0;
    int       flags   = 0;
    int       i       = 0;
    int       rd      = 0;
    uint32_t  lba     = 0;
    uint64_t  stime   = 0;
    uint64_t  sticks  = 0;
    uint64_t  cticks  = 0;
    uint32_t  extI    = tid%extN;
    uint64_t *p       = NULL;
    uint64_t *p2      = NULL;
    extent_t *extent  = NULL;
    page_t   *page    = NULL;
    uint32_t  CMPSIZE = (uint8_t*)(&cache->page[0].save) -
                        (uint8_t*)(&cache->page[0].data.cd);

    if (plun) {flags = CBLK_GROUP_ID;}

    debug("start tid:%d\n", tid);

    /*--------------------------------------------------------------------------
     * loop running IO for nsecs
     *------------------------------------------------------------------------*/
    stime = getticks();
    do
    {
        extent = &cache[extI];

        /* get a cache page and mark it INUSE */
        pthread_mutex_lock(&extent->ed.lock);
        page = &extent->page[extent->ed.next];
        while (page->save.s.inuse == INUSE)
        {
            extent->ed.next = (extent->ed.next+1) % PAGES;
            page            = &extent->page[extent->ed.next];
        };
        page->save.s.inuse = INUSE;
        pthread_mutex_unlock(&extent->ed.lock);

        /* read and compare */
        if (page->save.s.state == GOOD)
        {
            rd=1;
            debug("read  tid:%4d cid:%d ext:%d page:%ld:%p lba:%ld\n",
                            tid, extent->ed.cid, extI, page-extent->page,
                            &page->data, page->save.cd.lba);
            /* clobber read buf before reading data */
            memset(&page->data.cd,0x94,CMPSIZE);
            sticks = getticks();
            rc     = cblk_read(extent->ed.cid, &page->data, page->save.cd.lba,
                            nblocks, flags);
            cticks = getticks()-sticks;
            if (nblocks == rc)
            {
                /* check if lbas match */
                if (page->data.cd.lba != page->save.cd.lba)
                {
                    printf("miscomp: lba:   data:%ld save:%ld\n",
                                    page->data.cd.lba, page->save.cd.lba);
                }
                /* check if all cmpdata matches */
                if (MEMCMP && memcmp(&page->data.cd, &page->save.cd, CMPSIZE) != 0)
                {
                    printf("memcmp   tid:%d  lba:%ld\n", tid, page->save.cd.lba);
                    if (0)
                    {
                        /* show which 64-bit word(s) miscompare */
                        p  = (uint64_t*)(&page->data.cd);
                        p2 = (uint64_t*)(&page->save.cd);
                        for (i=0; i<CMPSIZE/8; i++,p++,p2++)
                        {
                            if (*p != *p2) {printf("u64cmp: tid:%d i:%d\n",tid,i);}
                        }
                    }
                }
            }
            else if (EBUSY != errno) {printf("read err rc:%d errno:%d\n", rc, errno);}
        }
        else
        {
            rd=0;
            sticks = getticks();
            p      = (uint64_t*)(&page->data.cd);
            /* assign a unique value to each uint64_t in the write buf */
            for (i=0; i<CMPSIZE/8; i++,p++) {*p=sticks+i;}
            lba = ((page-extent->page)*nblocks) + (PAGES*extent->ed.idx*nblocks);
            page->save.cd.lba = lba;
            debug("write tid:%4d cid:%d n:%d ext:%d page:%ld:%p lba:%ld\n",
                            tid, extent->ed.cid, nblocks, extI,
                            page-extent->page, &page->save, page->save.cd.lba);
            sticks = getticks();
            rc     = cblk_write(extent->ed.cid, &page->save, lba, nblocks, flags);
            cticks = getticks()-sticks;
            if (nblocks == rc)
            {
                page->save.s.state = GOOD;
            }
            else if (EBUSY != errno)
            {
                page->save.s.state = 0;
                printf("write err rc:%d errno:%d\n", rc, errno);
            }
        }

        /* free page */
        pthread_mutex_lock(&extent->ed.lock);
        page->save.s.inuse = 0;
        extent->ed.next    = (extent->ed.next+1) % PAGES;
        if (nblocks == rc)
        {
            if (rd)
            {
                extent->ed.rdN  += 1;
                extent->ed.rlat += cticks;
            }
            else
            {
                extent->ed.wrN  += 1;
                extent->ed.wlat += cticks;
            }
        }
        pthread_mutex_unlock(&extent->ed.lock);

        /* move to next extent */
        extI = (extI+1)%extN;

    } while (SDELTA(stime,ns_per_tick) < nsecs);

    debug("exiting tid:%d\n", tid);
    return;
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
    char           *_e          = NULL;
    uint32_t        intrp_thds  = 0;
    uint32_t        pths        = 0;
    uint32_t        QD          = 100;
    uint32_t        esecs       = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:s:e:hiMD")))
    {
        switch (c)
        {
            case 'd': devStr     = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 's': _secs      = optarg; break;
            case 'e': _e         = optarg; break;
            case 'i': intrp_thds = 1;      break;
            case 'M': MEMCMP     = 1;      break;
            case 'D': DEBUG      = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)      nsecs    = atoi(_secs);
    if (_QD)        QD       = atoi(_QD);
    if (_RD)        nRD      = atoi(_RD);
    if (_e)         extN     = atoi(_e);

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
    flags = CBLK_OPN_GROUP; ext=9;
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
    if ((rc=posix_memalign((void**)&cache, 128, sizeof(extent_t)*extN)))
    {
        fprintf(stderr,"posix_memalign size:%ld rc:%d\n",
                        sizeof(extent_t)*extN, rc);
        exit(0);
    }

    extent_t *e = cache;

    debug("page_t:%ld save_t:%ld\n", sizeof(page_t), sizeof(save_t));
    start_ticks = getticks();

    for (i=0; i<extN; i++)
    {
        pthread_mutex_init(&e->ed.lock, NULL);
        e->ed.rdN  = 0;
        e->ed.wrN  = 0;
        e->ed.rlat = 0;
        e->ed.wlat = 0;
        e->ed.next = 0;
        e->ed.cid  = ids[i%devN];
        e->ed.idx  = i/devN;
        debug("extent %d cid:%d idx:%d\n", i, e->ed.cid, e->ed.idx);
        ++e;
    }

    pthread_t pth[_8K];
    void* (*fp)(void*) = (void*(*)(void*))run_sync;

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

    e = cache;
    for (i=0; i<extN; i++)
    {
        rdT   += e->ed.rdN;
        wrT   += e->ed.wrN;
        rlatT += e->ed.rlat;
        wlatT += e->ed.wlat;
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
    cblk_term(NULL,0);
    return 0;
}

