/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/bin/blockr0io.c $                                   */
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
 *     blockr0io -d RAID0                                                     \n
 *     r:100 q:300 s:4 p:0 n:1 i:o err:0 mbps:1401 iops:358676                \n
 *                                                                            \n
 *     blockr0io -d RAID0 -s 20 -q 1 -r 70 -p                                 \n
 *     r:70 q:1 s:20 p:1 n:1 i:0 err:0 mbps:26 iops:6905                      \n
 *                                                                            \n
 *     blockr0io -d RAID0  -q 10 -n 20 -p                                     \n
 *     r:100 q:10 s:4 p:1 n:20 i:0 mbps:784 iops:10050 4k-iops:201008         \n
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
#define MAX_WIDTH  8

/**
********************************************************************************
** \brief struct for each issued op
*******************************************************************************/
typedef struct
{
    int      id;
    cflsh_cg_tag_t tag;
    uint64_t start;
    uint64_t lba;
    uint32_t cnt;
    uint32_t miss;
    uint32_t min_to;
    uint32_t max_to;
    uint32_t iss;
    uint32_t rd;
} OP_t;

uint8_t *rbuf;
uint8_t *wbuf;
OP_t    *ops;
int      width=0;
int      id;
int      opI[MAX_WIDTH]={0};
uint32_t RD;
uint32_t WR;
size_t   lba;
size_t   last_lba;
uint32_t nblocks;
uint32_t issN[MAX_WIDTH]={0};

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
#define BMP_LBA()                                                              \
    if (seq)                                                                   \
    {                                                                          \
        lba += nblocks;                                                        \
        if (lba+nblocks >= last_lba) {lba=0;}                                  \
    }                                                                          \
    else {lba = lrand48() % last_lba;}

/**
********************************************************************************
** \brief call to begin an op, init data for the op
*******************************************************************************/
#define OP_BEG(_tag,_rd)                                                       \
  do                                                                           \
  {                                                                            \
     int   _i     = _tag.id % width;                                           \
     OP_t *_op    = ops + (_i*QD) + _tag.tag;                                  \
     _op->start   = getticks();                                                \
     _op->iss     = 1;                                                         \
     _op->id      = _tag.id;                                                   \
     _op->tag.id  = _tag.id;                                                   \
     _op->tag.tag = _tag.tag;                                                  \
     _op->lba     = lba;                                                       \
     _op->rd      = _rd;                                                       \
     _op->cnt     = cnt;                                                       \
     _op->miss    = 0;                                                         \
     _op->min_to  = 0;                                                         \
     _op->max_to  = 0;                                                         \
     N           += 1;                                                         \
     issN[_i]    += 1;                                                         \
     if (_rd==READ) {--RD;} else {--WR;}                                       \
     debug("OP_BEG: ops[%4ld]={%d:%3d} rd:%d lba:%9ld issN[%d]:%4d "           \
           "N:%3d cnt:%d RD:%d len:%d\n",                                      \
           (_op)-ops, _tag.id, _tag.tag, (_op->rd==READ),                      \
           _op->lba, _i, issN[_i], N, cnt, RD, nblocks);                       \
     BMP_LBA();                                                                \
  } while (0)

/**
********************************************************************************
** \brief call to end an op, reset issued flag, save tick delta
*******************************************************************************/
#define OP_END(_tag, _cticks)                                                  \
  do                                                                           \
  {                                                                            \
      int   _i     = _tag.id % width;                                          \
      OP_t *_op    = ops + (_i*QD) + _tag.tag;                                 \
      lat          = _cticks - _op->start;                                     \
      cnt         += 1;                                                        \
      _op->iss     = 0;                                                        \
      tlat        += lat;                                                      \
      N           -= 1;                                                        \
      issN[_i]    -= 1;                                                        \
      if (verbose)                                                             \
      {  if (_op->rd) {++cnt_rd; tlat_rd+=lat;}                                \
         else         {++cnt_wr; tlat_wr+=lat;}                                \
      }                                                                        \
    debug("OP_END: ops[%4ld]={%d:%3d} rd:%d lba:%9ld issN[%d]:%4d N:%3d\n",    \
               (_op)-ops, _op->tag.id, _op->tag.tag,                           \
               (_op->rd==READ), _op->lba, _i, issN[_i], N);                    \
  } while (0);

/**
********************************************************************************
** \brief call to calc delta us
*******************************************************************************/
#define USTAMP(_usecs, _sticks, _cticks)                                       \
        _usecs=((_cticks-_sticks)*ns_per_tick) / 1000

/**
********************************************************************************
** \brief call to calc delta secs
*******************************************************************************/
#define TSTAMP(_secs, _sticks, _cticks)                                        \
        _secs=((_cticks-_sticks)*ns_per_tick) / 1000000000

/**
********************************************************************************
** \brief call to check timeout for an op(cmd), print msg for warning TO & TO
*******************************************************************************/
#define OP_CHK_WARN_TO(_op, _cticks, _min, _max)                               \
        if (verbose)                                                           \
        {                                                                      \
          lat = ((_cticks - _op->start) * ns_per_tick) / 1000;                 \
          if (lat > _min && !_op->min_to++)                                    \
              {printf("ETO: pid:%d cnt:%7d lba:%16ld rd:%d miss:%d TIME:%d\n", \
                       getpid(), cnt, _op->lba, _op->rd,                       \
                       _op->miss, lat);}                                       \
          else if (lat > _max && !_op->max_to++)                               \
              {printf("TO:  pid:%d cnt:%7d lba:%16ld rd:%d miss:%d TIME:%d\n", \
                       getpid(), cnt, _op->lba, _op->rd,                       \
                       _op->miss, lat);}                                       \
        }

/**
********************************************************************************
** \brief call to check latency for each op, print msg for LONG cmds
*******************************************************************************/
#define OP_CHK_LATENCY(_op, _lto, _cticks)                                     \
        if (verbose)                                                           \
        {                                                                      \
          lat = ((_cticks - _op->start) * ns_per_tick) / 1000;                 \
          if (lat > _lto)                                                      \
              {printf("LTO: pid:%d cnt:%7d lba:%16ld rd:%d miss:%d TIME:%d\n", \
                       getpid(), cnt, _op->lba, _op->rd,                       \
                       _op->miss, lat);}                                       \
        }

/**
********************************************************************************
** \brief print errno and exit
** \details
**   An IO has failed, print the errno and exit
*******************************************************************************/
#define IO_ISS_ERR(_type)                                                      \
    fprintf(stderr, "io_iss_error: errno:%d pid:%d cnt:%7d rd:%d\n",           \
            errno, getpid(), cnt, (READ==_type));                              \
    goto exit;

#define IO_CMP_ERR(_tag)                                                       \
  do                                                                           \
  {                                                                            \
      int   _i     = _tag.id % width;                                          \
      OP_t *_op    = ops + (_i*QD) + _tag.tag;                                 \
      fprintf(stderr, "io_cmp_error: errno:%d pid:%d cnt:%7d lba:%16ld "       \
                      "rd:%d\n",                                               \
              errno, getpid(), _op->cnt, _op->lba, (READ==_op->rd));           \
    goto exit;                                                                 \
  } while (0);

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
**   -v            *print cmd timeout warnings                                \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("  \
[-d device:device] [-r %%rd] [-q queuedepth] [-n nblocks] [-s secs] [-e eto] \
[-l lto] [-S vlunsize] [-p] [-i] [-R0] [-v]\n");
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
    OP_t           *op          = NULL;
    char            FF          = 0xFF;
    char            c           = '\0';
    int             flags       = CBLK_GROUP_RAID0;
    int             i=0,j=0, rc= 0;
    char           *_secs       = NULL;
    char           *_QD         = NULL;
    char           *_RD         = NULL;
    char           *_nblocks    = NULL;
    char           *_vlunsize   = NULL;
    char           *_eto        = NULL;
    char           *_lto        = NULL;
    char           *_R          = NULL;
    uint32_t        plun        = 0;
    uint32_t        nsecs       = 4;
    uint32_t        QD          = 240;
    uint32_t        QDT         = 0;
    uint32_t        nRD         = 100;
    uint32_t        eto         = 1000000;
    uint32_t        lto         = 10000;
    uint32_t        intrp_thds  = 0;
    cflsh_cg_tag_t  rtag        = {-1,-1};
    int             pflag       = 0;
    size_t          lun_size    = 0;
    uint32_t        nblocks     = 1;
    uint32_t        vlunsize    = 1;
    uint32_t        cnt         = 0;
    uint32_t        cnt_rd      = 0;
    uint32_t        cnt_wr      = 0;
    uint32_t        tmiss       = 0;
    uint32_t        hits        = 1;
    uint32_t        hbar        = 0;
    uint64_t        status      = 0;
    uint32_t        N           = 0;
    uint32_t        TIME        = 1;
    uint32_t        COMP        = 0;
    uint32_t        lat         = 0;
    uint32_t        usecs       = 0;
    uint32_t        esecs       = 0;
    uint32_t        isecs       = 0;
    uint32_t        seq         = 0;
    uint32_t        aresN       = 0;
    uint32_t        verbose     = 0;
    uint32_t        DEBUG       = 0;
    uint32_t        MAX_POLL    = 100;
    uint64_t        tlat        = 0;
    uint64_t        tlat_rd     = 0;
    uint64_t        tlat_wr     = 0;
    uint64_t        sticks      = 0;
    uint64_t        cticks      = 0;
    double          ns_per_tick = 0;
    uint32_t        cur         = 0;
    uint32_t        wI          = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:n:s:e:S:R:l:phivD")))
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

    if (nblocks > 1)
    {
        printf("nblocks > 1 is not supported yet\n");
        usage();
    }
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

    /*--------------------------------------------------------------------------
     * open device and set lun size
     *------------------------------------------------------------------------*/
    rc = cblk_init(NULL,0);
    if (rc)
    {
        fprintf(stderr,"cblk_init failed rc = %d and errno = %d\n", rc,errno);
        exit(1);
    }
    if (!plun)       flags |= CBLK_OPN_VIRT_LUN;
    if (!intrp_thds) flags |= CBLK_OPN_NO_INTRP_THREADS;

    debug("start open: %s flags:0x%08x\n", devStr, flags);
    id = cblk_cg_open(devStr, QD, O_RDWR, 1, 0, flags);

    if (id == NULL_CHUNK_ID)
    {
        if      (ENOSPC == errno) fprintf(stderr,"cblk_open: ENOSPC\n");
        else if (ENODEV == errno) fprintf(stderr,"cblk_open: ENODEV\n");
        else                      fprintf(stderr,"cblk_open: errno:%d\n",errno);
        cblk_term(NULL,0);
        exit(errno);
    }

    rc = cblk_cg_get_lun_size(id, &lun_size, CBLK_GROUP_RAID0);
    if (rc)
    {
        fprintf(stderr, "cblk_get_lun_size failed: errno: %d\n", errno);
        exit(errno);
    }
    if (plun) {last_lba = lun_size-1;}
    else
    {
        last_lba = vlunsize > lun_size ? lun_size-1 : vlunsize-1;
        rc = cblk_cg_set_size(id, last_lba+1, CBLK_GROUP_RAID0);
        if (rc)
        {
            fprintf(stderr, "cblk_set_size failed, errno: %d\n", errno);
            exit(errno);
        }
    }

    /*--------------------------------------------------------------------------
     * alloc data
     *------------------------------------------------------------------------*/
    width = cblk_cg_get_num_chunks(id, 0);
    QDT   = QD*width;
    if ((rc=posix_memalign((void**)&ops, 128, QDT*sizeof(OP_t))))
    {
        fprintf(stderr,"posix_memalign failed, size=%ld, rc=%d\n",
                QDT*sizeof(OP_t), rc);
        exit(0);
    }
    memset(ops, 0, QDT*sizeof(OP_t));

    if ((rc=posix_memalign((void**)&rbuf, _4K, _4K*nblocks)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        exit(0);
    }
    if ((rc=posix_memalign((void**)&wbuf, _4K, _4K*nblocks)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        exit(0);
    }
    memset(wbuf,0x79,_4K*nblocks);
    if (!seq) {BMP_LBA();}

    hbar     = QD<20 ? 1 : 2;
    COMP     = QD<10 ? 1 : QD/10;
    MAX_POLL = QD<48 ? 4 : (QD/width)*3/4;

    sticks = cticks = getticks();

    debug("open: %s id:%d width:%d nblks:%ld\n", devStr, id, width, last_lba+1);

    /*--------------------------------------------------------------------------
     * loop running IO for nsecs
     *------------------------------------------------------------------------*/
    do
    {
        /* setup #read ops and #write ops to send */
        if (!RD && !WR) {RD=nRD; WR=100-nRD;}

        /*------------------------------------------------------------------
         * send up to RD reads, as long as the queuedepth N is not max
         *----------------------------------------------------------------*/
        for (i=0; i<QD && TIME && RD && N<QD; i++)
        {
            rc = cblk_cg_aread(id, rbuf, lba, nblocks,&rtag,0,CBLK_GROUP_RAID0);
            if      (0 == rc)                       {OP_BEG(rtag, READ);}
            else if (EBUSY==errno || EAGAIN==errno) {break;}
            else                                    {IO_ISS_ERR(READ);}

            if (QD==1) {debug("USLEEP_RD\n");usleep(10);}
        }

        /*------------------------------------------------------------------
         * send up to WR writes, as long as the queuedepth N is not max
         *----------------------------------------------------------------*/
        for (i=0; i<QD && TIME && WR && N<QD; i++)
        {
            rc = cblk_cg_awrite(id, wbuf, lba,nblocks,&rtag,0,CBLK_GROUP_RAID0);
            if      (0 == rc)                       {OP_BEG(rtag, WRITE);}
            else if (EBUSY==errno || EAGAIN==errno) {break;}
            else                                    {IO_ISS_ERR(WRITE);}
        }

        /* if polling, usleep if #hits are below bar */
        if (QD>1 && !intrp_thds && hits<hbar)
            {debug("USLEEP N:%d\n",N); usleep(10);}

        /*----------------------------------------------------------------------
         * complete cmds
         *--------------------------------------------------------------------*/
        hits=0; aresN=0;

        if (intrp_thds)
        {
            int j=0;
            for (j=0; N && j<COMP; j++)
            {
                rc = cblk_cg_aresult(id, &rtag, &status,
                                     CBLK_ARESULT_BLOCKING |
                                     CBLK_ARESULT_NEXT_TAG |
                                     CBLK_GROUP_RAID0);
                cticks = getticks();

                if (rc != nblocks) {IO_CMP_ERR(rtag);}
                OP_END(rtag,cticks);
            }
        }
        else
        {
            if (issN[wI])
            {
                pflag=CBLK_GROUP_RAID0;

                for (j=0; j<QD; j++)
                {
                    cur = ((QD*wI) + opI[wI]);
                    op = ops + cur;
                    opI[wI] = (opI[wI]+1) % QD;
                    if (!op->iss) {continue;}

                    if (++aresN > MAX_POLL) {break;}

                    if (!(pflag&CBLK_ARESULT_NO_HARVEST))
                        {debug("HARVEST %d\n", op->id);}
                    rc = cblk_cg_aresult(id, &op->tag, &status, pflag);
                    cticks = getticks();

                    OP_CHK_WARN_TO(op, cticks, eto, _10SEC);
                    if (!(pflag&CBLK_ARESULT_NO_HARVEST))
                        {pflag|=CBLK_ARESULT_NO_HARVEST;}
                    if      (rc == 0) {++op->miss; ++tmiss; continue;}
                    else if (rc < 0)  {IO_CMP_ERR(op->tag);}

                    OP_CHK_LATENCY(op,lto,cticks);
                    OP_END(op->tag,cticks); ++hits;
                }
            }
            wI=(wI+1)%width;
        }

        USTAMP(usecs, sticks, cticks);

        /* check time expiration every second */
        if (usecs > isecs)
        {
            isecs += _1SEC;
            TSTAMP(esecs, sticks, cticks);
            if (esecs >= nsecs) {TIME=0; debug("TIME'S UP: N:%d\n", N);}
        }
    }
    while (TIME || N);

    cticks = getticks();
    TSTAMP(esecs, sticks, cticks);

    /*--------------------------------------------------------------------------
     * print IO stats
     *------------------------------------------------------------------------*/
    printf("r:%d q:%d s:%d p:%d n:%d i:%d v:%d eto:%d amiss:%d tmiss:%d \
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

exit:
    /*--------------------------------------------------------------------------
     * cleanup
     *------------------------------------------------------------------------*/
    free(ops);
    free(rbuf);
    free(wbuf);
    debug("cblk_close id:%d\n", id);
    cblk_cg_close(id,CBLK_GROUP_RAID0);
    cblk_term(NULL,0);
    return rc;
}
