/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/bin/blockio.c $                                     */
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
 * \brief Block Interface I/O Driver
 * \details
 *   This runs I/O to the capi Block Interface using aread/awrite/aresult. The
 *   expected iops are 300k-400k per capi card.                               \n
 *   Using the queuedepth (-q) option affects iops, as there are less cmds.   \n
 *   Using the physical lun (-p) option affects iops, as it uses only 1 port  \n
 *   Using the -p with blocksize (-n) option also affects iops.               \n
 *                                                                            \n
 *   Examples:                                                                \n
 *                                                                            \n
 *     blockio -d /dev/sg10                                                   \n
 *     d:/dev/sg10 r:100 q:300 s:4 p:0 n:1 i:o err:0 mbps:1401 iops:358676    \n
 *                                                                            \n
 *     blockio -d /dev/sg10 -s 20 -q 1 -r 70 -p                               \n
 *     d:/dev/sg10 r:70 q:1 s:20 p:1 n:1 i:0 err:0 mbps:26 iops:6905          \n
 *                                                                            \n
 *     blockio -d /dev/sg34  -q 10 -n 20 -p                                   \n
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
#include <pbuf.h>

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

/**
********************************************************************************
** \brief struct for each issued op
*******************************************************************************/
typedef struct
{
    int      id;
    uint64_t start;
    uint64_t lba;
    uint32_t cnt;
    uint32_t miss;
    uint32_t min_to;
    uint32_t max_to;
    uint32_t iss;
    uint32_t rd;
    void    *pbuf;
} OP_t;

/**
********************************************************************************
** \brief struct for each dev
*******************************************************************************/
typedef struct
{
    int      id;
    int      htag;
    OP_t    *op;
    uint32_t RD;
    uint32_t WR;
    uint32_t cnt;
    uint32_t cnt_rd;
    uint32_t cnt_wr;
    uint64_t tlat;
    uint64_t tlat_rd;
    uint64_t tlat_wr;
    uint32_t tmiss;
    uint32_t unmap;
    size_t   lba;
    size_t   last_lba;
    uint32_t nblocks;
    uint32_t issN;
    pbuf_t  *pbuf;
} SGDEV_t;

uint32_t nsecs       = 4;
uint32_t QD          = 128;
uint32_t nRD         = 100;
uint32_t intrp_thds  = 0;
uint32_t nblocks     = 1;
uint32_t DEBUG       = 0;
uint32_t MAX_POLL    = 100;
uint32_t seq         = 0;
uint32_t verbose     = 0;
uint32_t COMP        = 0;
uint32_t eto         = 1000000;
uint32_t lto         = 10000;
uint32_t hbar        = 0;
uint32_t UNMAP       = 0;
double   ns_per_tick = 0;

/**
********************************************************************************
** \brief print debug msg if DEBUG (-D)
*******************************************************************************/
#define debug(fmt, ...) \
   do {if (DEBUG) {fprintf(stdout,fmt,##__VA_ARGS__); fflush(stdout);}} while(0)

/**
********************************************************************************
** \brief bump lba, reset lba if it wraps
*******************************************************************************/
#define BMP_LBA(_dev)                                                          \
    if (seq)                                                                   \
    {                                                                          \
        _dev->lba += _dev->nblocks;                                            \
        if (_dev->lba+_dev->nblocks >= _dev->last_lba) {_dev->lba=0;}          \
    }                                                                          \
    else {_dev->lba = lrand48() % _dev->last_lba;}

/**
********************************************************************************
** \brief call to begin an op, init data for the op
*******************************************************************************/
#define OP_BEG(_dev,_rtag,_pbuf,_rd)                                           \
        op = &_dev->op[rtag];                                                  \
        op->start  = getticks();                                               \
        op->iss    = 1;                                                        \
        op->id     = _dev->id;                                                 \
        op->lba    = _dev->lba;                                                \
        op->rd     = _rd;                                                      \
        op->pbuf   = _pbuf;                                                    \
        op->cnt    = _dev->cnt;                                                \
        op->miss   = 0;                                                        \
        op->min_to = 0;                                                        \
        op->max_to = 0;                                                        \
        dev->issN += 1;                                                        \
        if (_rd==READ) {--_dev->RD;} else {--_dev->WR;}                        \
        debug("OP_BEG: id:%d rd:%d lba:%9ld len:%5d, issN:%4d RD:%d cnt:%d\n", \
            op->id, (op->rd==READ), op->lba, _dev->nblocks, _dev->issN,        \
            _dev->RD,_dev->cnt);                                               \
            BMP_LBA(_dev);

/**
********************************************************************************
** \brief call to end an op, reset issued flag, save tick delta
*******************************************************************************/
#define OP_END(_dev, _op, _cticks)                                             \
                        lat         = _cticks - _op->start;                    \
                        _dev->cnt  += 1;                                       \
                        _dev->issN -= 1;                                       \
                        _op->iss    = 0;                                       \
                        _dev->tlat += lat;                                     \
                        if (verbose)                                           \
                        {  if (_op->rd) {++_dev->cnt_rd; _dev->tlat_rd+=lat;}  \
                           else         {++_dev->cnt_wr; _dev->tlat_wr+=lat;}  \
                        }                                                      \
                        pbuf_put(_dev->pbuf,_op->pbuf);                        \
    debug("OP_END: id:%d rd:%d lba:%9ld lat:%5d, issN:%4d\n",                  \
          _op->id, (_op->rd==READ), _op->lba,                                  \
          (uint32_t)((lat*ns_per_tick)/1000),                                  \
          _dev->issN);

/**
********************************************************************************
** \brief call to check timeout for an op(cmd), print msg for warning TO & TO
*******************************************************************************/
#define OP_CHK_WARN_TO(_op, _cticks, _min, _max)                               \
        if (verbose && _op->miss)                                              \
        {                                                                      \
          lat = ((_cticks - _op->start) * ns_per_tick) / 1000;                 \
          if (lat > _min && !_op->min_to++)                                    \
              {printf("ETO: id:%d pid:%d lba:%16ld rd:%d miss:%d TIME:%d\n",   \
                       _op->id, getpid(), _op->lba, _op->rd,                   \
                       _op->miss, lat);}                                       \
          else if (lat > _max && !_op->max_to++)                               \
              {printf("TO:  id:%d pid:%d lba:%16ld rd:%d miss:%d TIME:%d\n",   \
                       _op->id, getpid(), _op->lba, _op->rd,                   \
                       _op->miss, lat);}                                       \
        }

/**
********************************************************************************
** \brief call to check latency for each op, print msg for LONG cmds
*******************************************************************************/
#define OP_CHK_LATENCY(_op, _lto, _cticks)                                     \
        if (verbose && _op->miss)                                              \
        {                                                                      \
          lat = ((_cticks - _op->start) * ns_per_tick) / 1000;                 \
          if (lat > _lto)                                                      \
              {printf("LTO: id:%d pid:%d lba:%16ld rd:%d miss:%d TIME:%d\n",   \
                       _op->id, getpid(), _op->lba, _op->rd,                   \
                       _op->miss, lat);}                                       \
        }

/**
********************************************************************************
** \brief print errno and exit
** \details
**   An IO has failed, print the errno and exit
*******************************************************************************/
#define IO_ISS_ERR(_type)                                                      \
    fprintf(stderr, "io_iss_error: errno:%d pid:%d rd:%d\n",                   \
            errno, getpid(), (READ==_type));                                   \
    goto exit;

#define IO_CMP_ERR(_op)                                                        \
    fprintf(stderr, "io_cmp_error: errno:%d pid:%d cnt:%7d lba:%16ld rd:%d\n", \
            errno, getpid(), _op->cnt, _op->lba, (READ==_op->rd));             \
    goto exit;

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -r %rd        *the percentage of reads to issue (0..100)                 \n
**   -q queuedepth *the number of outstanding ops to maintain                 \n
**   -n nblocks    *the number of 4k blocks in each I/O request               \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -e eto        *early timeout: microseconds, print warning for each op    \n
**   -l lto        *long timeout, microseconds, print elapse time for each op \n
**   -S vlunsize   *size in gb for the vlun                                   \n
**   -p            *run in physical lun mode                                  \n
**   -i            *run using interrupts, not polling                         \n
**   -R            *randomize lbas, default is on, use -R0 to run sequential  \n
**   -U            *run 100% unmaps                                           \n
**   -v            *print cmd timeout warnings                                \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("  \
[-d device:device] [-r %%rd] [-q queuedepth] [-n nblocks] [-s secs] [-e eto] \
[-l lto] [-S vlunsize] [-p] [-i] [-R0] [-U] [-v]\n");
    exit(0);
}

/**
********************************************************************************
** \brief
**  run async IO to a device
*******************************************************************************/
void run_async(SGDEV_t *dev)
{
    uint64_t  sticks = 0;
    uint64_t  cticks = 0;
    uint32_t  aresN  = 0;
    uint32_t  TIME   = 1;
    uint32_t  hits   = 1;
    int       pflag  = 0;
    int       rtag   = 0;
    int       i      = 0;
    int       rc     = 0;
    uint64_t  status = 0;
    OP_t     *op     = NULL;
    uint32_t  lat    = 0;
    uint8_t  *pbuf   = NULL;
    int       bs     = _4K * nblocks;

    debug("start dev:%p id:%d\n", dev, dev->id);

    sticks = cticks = getticks();

    dev->pbuf = pbuf_new(QD, bs);

    /*--------------------------------------------------------------------------
     * loop running IO for nsecs
     *------------------------------------------------------------------------*/
    do
    {
        /* setup #read ops and #write ops to send */
        if (!dev->RD && !dev->WR) {dev->RD=nRD; dev->WR=100-nRD;}

        /*------------------------------------------------------------------
         * send up to RD reads, as long as the queuedepth N is not max
         *----------------------------------------------------------------*/
        for (i=0; i<QD && TIME && dev->RD && dev->issN<QD; i++)
        {
            if (NULL == (pbuf=pbuf_get(dev->pbuf))) {break;}
            *pbuf=0;
            rc = cblk_aread(dev->id, pbuf, dev->lba, dev->nblocks, &rtag, 0, 0);
            if      (0      == rc)    {OP_BEG(dev,rtag,pbuf,READ);}
            else if (EAGAIN == errno) {debug("EAGAIN\n"); break;}
            else if (EBUSY  == errno) {debug("EBUSY\n");  break;}
            else                      {IO_ISS_ERR(READ);}
        }
        /*------------------------------------------------------------------
         * send up to WR writes, as long as the queuedepth N is not max
         *----------------------------------------------------------------*/
        for (i=0; i<QD && TIME && dev->WR && dev->issN<QD; i++)
        {
            if (NULL == (pbuf=pbuf_get(dev->pbuf))) {break;}
            if (dev->unmap)
            {
                memset(pbuf,0,bs);
                rc = cblk_aunmap(dev->id, pbuf,dev->lba,dev->nblocks,&rtag,0,0);
            }
            else
            {
                memset(pbuf,0x79,bs);
                rc = cblk_awrite(dev->id, pbuf,dev->lba,dev->nblocks,&rtag,0,0);
            }
            if      (0      == rc)    {OP_BEG(dev,rtag,pbuf,WRITE);}
            else if (EAGAIN == errno) {debug("EAGAIN\n"); break;}
            else if (EBUSY  == errno) {debug("EBUSY\n");  break;}
            else                      {IO_ISS_ERR(WRITE);}
        }

        /*----------------------------------------------------------------------
         * complete cmds
         *--------------------------------------------------------------------*/
        pflag=0; hits=0; aresN=0;

        for (i=0; i<QD && dev->issN; i++, dev->htag++)
        {
            if (intrp_thds)
            {
                rc = cblk_aresult(dev->id, &dev->htag, &status,
                                  CBLK_ARESULT_BLOCKING |
                                  CBLK_ARESULT_NEXT_TAG);
                cticks = getticks();

                op=dev->op+dev->htag;
                if (rc != dev->nblocks) {IO_CMP_ERR(op);}
                OP_END(dev,op,cticks);
                if (++aresN >= COMP) {break;}
            }
            else
            {
                dev->htag %= QD;
                op         = dev->op + dev->htag;
                if (!op->iss) {continue;}
                if (++aresN > MAX_POLL) {break;}

                if (pflag==0) {debug("HARVEST id:%d\n", dev->id);}
                rc = cblk_aresult(dev->id, &dev->htag, &status, pflag);
                cticks = getticks();

                OP_CHK_WARN_TO(op, cticks, eto, _10SEC);
                if      (!pflag)  {pflag=CBLK_ARESULT_NO_HARVEST;}
                if      (rc == 0) {++op->miss; ++dev->tmiss; continue;}
                else if (rc < 0)  {IO_CMP_ERR(op);}

                OP_CHK_LATENCY(op,lto,cticks);
                OP_END(dev,op,cticks); ++hits;
            }
        }

        if (TIME && SDELTA(sticks,ns_per_tick) >= nsecs) {TIME=0;}
    }
    while (TIME || dev->issN);

exit:
    if (dev->pbuf) {pbuf_free(dev->pbuf);}
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
    char            devStrs[MAX_SGDEVS][64];
    char           *devStr      = NULL;
    char           *pstr        = NULL;
    SGDEV_t        *devs        = NULL;
    SGDEV_t        *dev         = NULL;
    uint32_t        devN        = 0;
    char            FF          = 0xFF;
    char            c           = '\0';
    chunk_ext_arg_t ext         = 0;
    int             flags       = 0;
    int             i=0, rc     = 0;
    char           *_secs       = NULL;
    char           *_QD         = NULL;
    char           *_RD         = NULL;
    char           *_nblocks    = NULL;
    char           *_vlunsize   = NULL;
    char           *_eto        = NULL;
    char           *_lto        = NULL;
    char           *_R          = NULL;
    uint32_t        plun        = 0;
    size_t          lun_size    = 0;
    uint32_t        vlunsize    = 1;
    uint64_t        tlat        = 0;
    uint64_t        tlat_rd     = 0;
    uint64_t        tlat_wr     = 0;
    uint32_t        cnt         = 0;
    uint32_t        cnt_rd      = 0;
    uint32_t        cnt_wr      = 0;
    uint32_t        tmiss       = 0;
    uint32_t        esecs       = 0;
    uint64_t        sticks      = 0;
    chunk_attrs_t   attrs;
    pthread_t       pth[32];

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:n:s:e:S:R:l:phivUD")))
    {
        switch (c)
        {
            case 'd': devStr     = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 'n': _nblocks   = optarg; break;
            case 'S': _vlunsize  = optarg; break;
            case 's': _secs      = optarg; break;
            case 'e': _eto       = optarg; break;
            case 'l': _lto       = optarg; break;
            case 'R': _R         = optarg; break;
            case 'p': plun       = 1;      break;
            case 'i': intrp_thds = 1;      break;
            case 'D': DEBUG      = 1;      break;
            case 'U': UNMAP      = 1;      break;
            case 'v': verbose    = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)      nsecs    = atoi(_secs);
    if (_QD)        QD       = atoi(_QD);
    if (_nblocks)   nblocks  = atoi(_nblocks);
    if (_vlunsize)  vlunsize = atoi(_vlunsize);
    if (_RD)        nRD      = atoi(_RD);
    if (_eto)       eto      = atoi(_eto);
    if (_lto)       lto      = atoi(_lto);
    else if (QD>1)  lto      = 20000;
    if (_R && _R[0]=='0') seq= 1;

    QD  = (QD < _8K)  ? QD  : _8K;
    nRD = (nRD < 100) ? nRD : 100;

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

    while ((pstr=strsep(&devStr,":")))
    {
        if (devN == MAX_SGDEVS) {debug("too many devs\n"); break;};
        debug("%s\n",pstr);
        sprintf(devStrs[devN++],"%s",pstr);
    }

    vlunsize = vlunsize*256*1024;

    srand48(time(0));
    ns_per_tick = time_per_tick(1000, 100);

    hbar     = QD<20 ? 1 : (QD>64 ? 0 : QD/20);
    COMP     = QD<6  ? 1 : QD/6;
    MAX_POLL = QD<4  ? 1 : QD/4;

    /*--------------------------------------------------------------------------
     * alloc data
     *------------------------------------------------------------------------*/
    debug("malloc devs: %d\n", devN);
    if ((rc=posix_memalign((void**)&devs, 128, devN*sizeof(SGDEV_t))))
    {
        fprintf(stderr,"posix_memalign failed, size=%ld, rc=%d\n",
                devN*sizeof(SGDEV_t), rc);
        exit(0);
    }
    memset(devs,0,devN*sizeof(SGDEV_t));
    for (i=0; i<devN; i++)
    {
        debug("malloc dev[%d]->op QD:%d\n", i, QD);
        if ((rc=posix_memalign((void**)&devs[i].op, 128, QD*sizeof(OP_t))))
        {
            fprintf(stderr,"posix_memalign failed, size=%ld, rc=%d\n",
                    QD*sizeof(OP_t), rc);
            exit(0);
        }
        memset(devs[i].op,0,QD*sizeof(OP_t));
    }

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
    if (!plun)       flags  = CBLK_OPN_VIRT_LUN;
    if (!intrp_thds) flags |= CBLK_OPN_NO_INTRP_THREADS;

    for (i=0; i<devN; i++)
    {
        dev=devs+i;
        dev->id = cblk_open(devStrs[i], QD, O_RDWR, ext, flags);

        if (dev->id == NULL_CHUNK_ID)
        {
            if      (ENOSPC == errno) fprintf(stderr,"cblk_open: ENOSPC\n");
            else if (ENODEV == errno) fprintf(stderr,"cblk_open: ENODEV\n");
            else                      fprintf(stderr,"cblk_open: errno:%d\n",errno);
            cblk_term(NULL,0);
            exit(errno);
        }

        rc = cblk_get_lun_size(dev->id, &lun_size, 0);
        if (rc)
        {
            fprintf(stderr, "cblk_get_lun_size failed: errno: %d\n", errno);
            exit(errno);
        }
        if (plun) {dev->last_lba = lun_size-1;}
        else
        {
            dev->last_lba = vlunsize > lun_size ? lun_size-1 : vlunsize-1;
            rc = cblk_set_size(dev->id, dev->last_lba+1, 0);
            if (rc)
            {
                fprintf(stderr, "cblk_set_size failed, errno: %d\n", errno);
                exit(errno);
            }
        }

        if (UNMAP)
        {
            dev->unmap = 1;
            bzero(&attrs, sizeof(attrs));
            cblk_get_attrs(dev->id, &attrs, 0);
            if (!(attrs.flags1 & CFLSH_ATTR_UNMAP))
            {
                printf("%s: unmap is not supported\n", devStrs[i]);
                dev->unmap=0;
            }
        }

        debug("open: %s dev:%p id:%d nblks:%ld\n",
                devStrs[i],dev, dev->id, dev->last_lba+1);

        dev->nblocks = nblocks;
        BMP_LBA(dev);
    }

    /*--------------------------------------------------------------------------
     * use threads to run IO
     *------------------------------------------------------------------------*/
    sticks = getticks();

    void* (*fp)(void*) = (void*(*)(void*))run_async;

    for (i=0; i<devN; i++)
    {
        debug("pthread_create %d for dev:%p id:%d\n", i, devs+i, (devs+i)->id);
        pthread_create(pth+i, NULL, fp, (void*)(devs+i));
    }
    for (i=0; i<devN; i++)
    {
        pthread_join(pth[i], NULL);
        debug("pthread_done %d\n", i);
    }

    esecs = SDELTA(sticks,ns_per_tick);

    for (i=0; i<devN; i++)
    {
        dev=devs+i;
        if (verbose)
        {
            printf("%-10s id:%d lat:%d cnt:%d\n",
                    devStrs[i],
                    dev->id,
                    (uint32_t)((dev->tlat*ns_per_tick)/dev->cnt/1000),
                    dev->cnt);
        }
        tmiss   += dev->tmiss;
        cnt     += dev->cnt;
        cnt_rd  += dev->cnt_rd;
        cnt_wr  += dev->cnt_wr;
        tlat    += dev->tlat;
        tlat_rd += dev->tlat_rd;
        tlat_wr += dev->tlat_wr;
    }

    /*--------------------------------------------------------------------------
     * print IO stats
     *------------------------------------------------------------------------*/
    if (cnt==0) {goto cleanup;}
    printf("r:%d q:%d s:%d p:%d n:%d i:%d v:%d eto:%d miss:%d/%d \
lat:%d mbps:%d iops:%d",
            nRD, QD, nsecs, plun, nblocks,
            intrp_thds, verbose, eto, tmiss/cnt, tmiss,
            (uint32_t)((tlat*ns_per_tick)/cnt/1000),
            ((cnt*nblocks*4)/1024)/esecs,
            cnt/esecs);
    if (plun && nblocks > 1) {printf(" 4k-iops:%d", (cnt*nblocks)/esecs);}
    if (verbose) {printf(" rlat:%d wlat:%d",
                         (uint32_t)((tlat_rd*ns_per_tick)/cnt_rd/1000),
                         (uint32_t)((tlat_wr*ns_per_tick)/cnt_wr/1000));}
    printf("\n"); fflush(stdout);

cleanup:
    /*--------------------------------------------------------------------------
     * cleanup
     *------------------------------------------------------------------------*/
    for (i=0; i<devN; i++)
    {
        free(devs[i].op);
        debug("cblk_close id:%d\n", devs[i].id);
        cblk_close(devs[i].id,0);
    }
    free(devs);
    cblk_term(NULL,0);
    return rc;
}
