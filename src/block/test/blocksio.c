/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/blocksio.c $                                   */
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
 * \brief Block Interface Sync I/O Driver
 * \details
 *   This runs I/O to the capi Block Interface using read/write. The          \n
 *   expected iops are 300k-400k per card.                                    \n
 *   Using the queuedepth (-q) option affects iops, as there are less cmds.   \n
 *   Using the -p with blocksize (-n) option also affects iops.               \n
 *                                                                            \n
 *   Examples:                                                                \n
 *                                                                            \n
 *     blocksio -d /dev/sg10                                                  \n
 *     d:/dev/sg10 r:100 q:300 s:4 p:0 n:1 i:o err:0 mbps:1401 iops:358676    \n
 *                                                                            \n
 *     blocksio -d /dev/sg10 -s 20 -q 1 -r 70 -p                              \n
 *     d:/dev/sg10 r:70 q:1 s:20 p:1 n:1 i:0 err:0 mbps:26 iops:6905          \n
 *                                                                            \n
 *     blocksio -d /dev/sg34  -q 10 -n 20 -p                                  \n
 *     d:/dev/sg34 r:100 q:10 s:4 p:1 n:20 i:0                                \n
 *      mbps:784 iops:10050 4k-iops:201008                                    \n
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

#define _8K        8*1024
#define _4K        4*1024
#define NBUFS      _8K
#define _1MSEC     1000
#define _1SEC      1000000
#define _10SEC     10000000
#define READ       1
#define WRITE      0
#define MAX_SGDEVS 8

int      id          = 0;
uint64_t tlat        = 0;
uint32_t seq         = 0;
size_t   nblks       = 0;
uint32_t nblocks     = 1;
uint32_t tcnt        = 0;
uint32_t nRD         = 100;
uint64_t start_ticks = 0;
double   ns_per_tick = 0;
uint32_t nsecs       = 4;
char     devStrs[MAX_SGDEVS][64];
char    *devStr      = NULL;
char    *pstr        = NULL;
uint32_t devN        = 0;
uint32_t DEBUG       = 0;
uint32_t plun        = 0;
int      ids[_4K];
uint8_t  *p_rbuf   = NULL;
uint8_t  *p_wbuf   = NULL;

pthread_mutex_t lock;

/**
********************************************************************************
** \brief print errno and exit
** \details
**   An IO has failed, print the errno and exit
*******************************************************************************/
#define IO_ERR(_rd, _lba, _errno)                                              \
    fprintf(stderr, "io_error: errno:%d pid:%d lba:%8d rd:%d\n",               \
                    _errno, getpid(), _lba, _rd);                              \
    return;

/**
********************************************************************************
** \brief bump lba, reset lba if it wraps
*******************************************************************************/
#define BMP_LBA() if (seq)                                                     \
                  {                                                            \
                     lba+=nblocks;                                             \
                     if (lba+nblocks >= nblks) {lba=0;}                        \
                  }                                                            \
                  else {lba=lrand48()%nblks;}

#define debug(fmt, ...) \
   do {if (DEBUG) {fprintf(stdout,fmt,##__VA_ARGS__); fflush(stdout);}} while(0)

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -r %rd        *the percentage of reads to issue (0..100)                 \n
**   -q queuedepth *the number of outstanding ops to maintain                 \n
**   -n nblocks    *the number of 4k blocks in each I/O request               \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -S vlunsize   *size in gb for the vlun                                   \n
**   -p            *run in physical lun mode                                  \n
**   -i            *run using interrupts, not polling                         \n
**   -R            *randomize lbas, default is on, use -R0 to run sequential  \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("  [-d device] [-r %%rd] [-q queuedepth] [-n nblocks] "
           "[-s secs] [-S vlunsize] [-p] [-i] [-R0]\n");
    exit(0);
}

/**
********************************************************************************
** \brief run sync IO in a thread
*******************************************************************************/
void run_sync(void *p)
{
    int       tid    = (uint32_t)(uint64_t)p;
    int       cid    = ids[tid % devN];
    int       rc     = 0;
    int       flags  = 0;
    uint32_t  lba    = 0;
    uint64_t  stime  = 0;
    uint64_t  sticks = 0;
    uint64_t  cticks = 0;
    uint32_t  RD     = 0;
    uint32_t  WR     = 0;
    uint32_t  cnt    = 0;
    uint64_t  lat    = 0;
    uint8_t  *rbuf   = p_rbuf+(tid*_4K);
    uint8_t  *wbuf   = p_wbuf+(tid*_4K);

    if (!seq) {lba   = lrand48() % nblks;}
    if (plun) {flags = CBLK_GROUP_ID;}

    debug("start tid:%d cid:%d\n", tid, cid);

    /*--------------------------------------------------------------------------
     * loop running IO for nsecs
     *------------------------------------------------------------------------*/
    stime = getticks();
    do
    {
        /* setup #read ops and #write ops to send */
        if (!RD && !WR) {RD=nRD; WR=100-RD;}

        if (RD)
        {
            debug("read  cid:%d lba:%d\n", cid, lba);
            sticks=getticks();
            rc = cblk_read(cid, rbuf, lba, nblocks, flags);
            if (nblocks == rc)
            {
                cticks=getticks()-sticks;
                --RD; BMP_LBA();
                lat +=cticks;
                ++cnt;
            }
            else if (EBUSY != errno) {IO_ERR(READ,lba,errno);}
        }
        if (WR)
        {
            debug("write cid:%d lba:%d\n", cid, lba);
            sticks=getticks();
            rc = cblk_write(cid, wbuf, lba, nblocks,flags);
            if (nblocks == rc)
            {
                cticks=getticks()-sticks;
                --WR; BMP_LBA();
                lat +=cticks;
                ++cnt;
            }
            else if (EBUSY != errno) {IO_ERR(WRITE,lba,errno);}
        }
    } while (SDELTA(stime,ns_per_tick) < nsecs);

    pthread_mutex_lock(&lock);
    tlat    += lat;
    tcnt    += cnt;
    pthread_mutex_unlock(&lock);

    debug("exiting cid:%d\n", cid);
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
    char           *_nblocks    = NULL;
    char           *_vlunsize   = NULL;
    char           *_R          = NULL;
    uint32_t        intrp_thds  = 0;
    uint32_t        pths        = 0;
    uint32_t        QD          = 128;
    uint32_t        vlunsize    = 1;
    uint32_t        esecs       = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:n:s:S:R:phiD")))
    {
        switch (c)
        {
            case 'd': devStr     = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 'n': _nblocks   = optarg; break;
            case 'S': _vlunsize  = optarg; break;
            case 's': _secs      = optarg; break;
            case 'R': _R         = optarg; break;
            case 'i': intrp_thds = 1;      break;
            case 'p': plun       = 1;      break;
            case 'D': DEBUG      = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)      nsecs    = atoi(_secs);
    if (_QD)        QD       = atoi(_QD);
    if (_nblocks)   nblocks  = atoi(_nblocks);
    if (_vlunsize)  vlunsize = atoi(_vlunsize);
    if (_RD)        nRD      = atoi(_RD);
    if (_R && _R[0]=='0') seq= 1;

    if (QD > _8K)   QD  = _8K;
    if (nRD > 100)  nRD = 100;

    if (plun && vlunsize > 1)
    {
        printf("error: <-S %d> can only be used with a virtual lun\n",vlunsize);
        usage();
    }
    if (!plun && nblocks > 1)
    {
        printf("error: <-n %d> can only be used with a physical lun\n",nblocks);
        usage();
    }
    if (devStr == NULL) usage();

    vlunsize = vlunsize*256*1024;

    srand48(time(0));
    ns_per_tick = time_per_tick(1000, 100);

    while ((pstr=strsep(&devStr,":")))
    {
        if (devN == MAX_SGDEVS) {break;};
        debug("%s\n",pstr);
        sprintf(devStrs[devN++],"%s",pstr);
    }

    pths = (devN*QD > _8K) ? _8K : devN*QD;

    /*--------------------------------------------------------------------------
     * alloc data for IO
     *------------------------------------------------------------------------*/
    if ((rc=posix_memalign((void**)&p_rbuf, 128, _4K*nblocks*pths)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        cblk_term(NULL,0);
        exit(0);
    }
    if ((rc=posix_memalign((void**)&p_wbuf, 128, _4K*nblocks*pths)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        cblk_term(NULL,0);
        exit(0);
    }
    memset(p_wbuf,0x79,_4K*nblocks*QD);

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
    if (plun)        {flags  = CBLK_OPN_GROUP; ext=9;}
    else             {flags  = CBLK_OPN_VIRT_LUN;}
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
        if (!plun)
        {
            nblks = vlunsize > nblks ? nblks : vlunsize;
            rc = cblk_set_size(ids[i], nblks, flags);
            if (rc)
            {
                fprintf(stderr, "cblk_set_size failed, errno: %d\n", errno);
                exit(errno);
            }
        }
        debug("open: %s id:%d nblks:%ld\n", devStrs[i%devN], ids[i], nblks);
    }
    pthread_mutex_init(&lock,NULL);

    /*--------------------------------------------------------------------------
     * create threads to run sync IO
     *------------------------------------------------------------------------*/
    start_ticks = getticks();

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

    esecs=SDELTA(start_ticks,ns_per_tick);

    /*--------------------------------------------------------------------------
     * print IO stats
     *------------------------------------------------------------------------*/
    printf("r:%d q:%d s:%d p:%d n:%d S:%ld lat:%d mbps:%d iops:%d",
            nRD, QD, nsecs, plun, nblocks, (nblks*4096)/(1024*1024*1024),
            (uint32_t)((tlat*ns_per_tick)/tcnt/1000),
            ((tcnt*nblocks*4)/1024)/esecs,
            tcnt/esecs);
    if (plun && nblocks > 1) {printf(" 4k-iops:%d", (tcnt*nblocks)/esecs);}
    printf("\n"); fflush(stdout);

    if (plun) {flags=CBLK_GROUP_ID;}
    for (i=0; i<devN; i++)
        {debug("close %d\n", ids[i]); cblk_close(ids[i], flags);}
    cblk_term(NULL,0);
    return 0;
}

