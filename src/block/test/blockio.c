/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/blockio.c $                                    */
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
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
#include <capiblock.h>
#include <inttypes.h>
#include <ut.h>

#define _8K   8*1024
#define _4K   4*1024
#define NBUFS _8K

#define TIME_INTERVAL 1024
#define SET_NBLKS     260*1024

/**
********************************************************************************
** \brief struct for each issued op
*******************************************************************************/
typedef struct
{
    uint64_t start;
    uint8_t  iss;
} OP_t;

/**
********************************************************************************
** \brief call to begin an op, set issued flag, set start ticks
*******************************************************************************/
#define OP_BEG(_tag) op[_tag].iss=1; op[_tag].start=getticks();

/**
********************************************************************************
** \brief call to end an op, reset issued flag, save tick delta
*******************************************************************************/
#define OP_END(_tag) op[_tag].iss=0; tlat+=getticks() - op[_tag].start;

/**
********************************************************************************
** \brief bump lba, reset lba if it wraps
*******************************************************************************/
#define BMP_LBA() lba+=nblocks+1; \
                  if (lba+nblocks >= nblks) {lba=lrand48()%nblks;}

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -r %rd        *the percentage of reads to issue (0..100)                 \n
**   -q queuedepth *the number of outstanding ops to maintain                 \n
**   -n nblocks    *the number of 4k blocks in each I/O request               \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -p            *run in physical lun mode                                  \n
**   -i            *run using interrupts, not polling
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("   \
[-d device] [-r %%rd] [-q queuedepth] [-n nblocks] [-s secs] [-p] [-i]\n");
    exit(0);
}

/**
********************************************************************************
** \brief print errno and exit
** \details
**   An IO has failed, print the errno and exit
*******************************************************************************/
void io_error(int id, int err)
{
    fprintf(stderr, "io_error: errno:%d\n", err);
    cblk_close(id,0);
    cblk_term(NULL,0);
    exit(err);
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
    struct timeval  start, delta;
    long int        mil         = 1000000;
    float           esecs       = 0;
    uint8_t        *rbuf        = NULL;
    uint8_t        *wbuf        = NULL;
    OP_t           *op          = NULL;
    char           *dev         = NULL;
    char            FF          = 0xFF;
    char            c           = '\0';
    chunk_ext_arg_t ext         = 0;
    int             flags       = 0;
    int             i, rc       = 0;
    int             id          = 0;
    char           *_secs       = NULL;
    char           *_QD         = NULL;
    char           *_RD         = NULL;
    char           *_nblocks    = NULL;
    uint32_t        plun        = 0;
    uint32_t        nsecs       = 4;
    uint32_t        QD          = 256;
    uint32_t        nRD         = 100;
    uint32_t        RD          = 0;
    uint32_t        WR          = 0;
    uint32_t        intrp_thds  = 0;
    int             rtag        = 0;
    int             htag        = 0;
    uint32_t        lba         = 0;
    size_t          nblks       = 0;
    uint32_t        nblocks     = 1;
    uint32_t        cnt         = 0;
    uint32_t        tmiss       = 0;
    uint64_t        status      = 0;
    uint32_t        TI          = TIME_INTERVAL;
    uint32_t        N           = 0;
    uint32_t        TIME        = 1;
    uint32_t        COMP        = 0;
    uint32_t        miss        = 0;
    uint64_t        tlat        = 0;
    double          ns_per_tick = 0;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:r:q:n:s:phi")))
    {
        switch (c)
        {
            case 'd': dev        = optarg; break;
            case 'r': _RD        = optarg; break;
            case 'q': _QD        = optarg; break;
            case 'n': _nblocks   = optarg; break;
            case 's': _secs      = optarg; break;
            case 'p': plun       = 1;      break;
            case 'i': intrp_thds = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs)      nsecs   = atoi(_secs);
    if (_QD)        QD      = atoi(_QD);
    if (_nblocks)   nblocks = atoi(_nblocks);
    if (_RD)        nRD     = atoi(_RD);

    if (QD > _8K)   QD      = _8K;
    if (nRD > 100)  nRD     = 100;

    if (!plun && nblocks > 1)
    {
        printf("error: <-n %d> can only be used with a physical lun\n",nblocks);
        usage();
    }
    if (dev == NULL) usage();

    srand48(time(0));
    ns_per_tick = time_per_tick(1000, 100);

    N    = QD;
    COMP = QD < 8 ? 1 : QD/8;

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
    id = cblk_open(dev, QD, O_RDWR, ext, flags);
    if (id == NULL_CHUNK_ID)
    {
        if      (ENOSPC == errno) fprintf(stderr,"cblk_open: ENOSPC\n");
        else if (ENODEV == errno) fprintf(stderr,"cblk_open: ENODEV\n");
        else                      fprintf(stderr,"cblk_open: errno:%d\n",errno);
        cblk_term(NULL,0);
        exit(errno);
    }

    rc = cblk_get_lun_size(id, &nblks, 0);
    if (rc)
    {
        fprintf(stderr, "cblk_get_lun_size failed: errno: %d\n", errno);
        exit(errno);
    }
    if (!plun)
    {
        nblks = nblks > SET_NBLKS ? SET_NBLKS : nblks;
        rc = cblk_set_size(id, nblks, 0);
        if (rc)
        {
            fprintf(stderr, "cblk_set_size failed, errno: %d\n", errno);
            exit(errno);
        }
    }
    lba = lrand48() % nblks;

    /*--------------------------------------------------------------------------
     * alloc data for IO
     *------------------------------------------------------------------------*/
    op = malloc(QD*sizeof(OP_t));
    if ((rc=posix_memalign((void**)&rbuf, _4K, _4K*nblocks)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        cblk_close(id,0);
        cblk_term(NULL,0);
        exit(0);
    }
    if ((rc=posix_memalign((void**)&wbuf, _4K, _4K*nblocks)))
    {
        fprintf(stderr,"posix_memalign failed, size=%d, rc=%d\n",
                _4K*nblocks, rc);
        cblk_close(id,0);
        cblk_term(NULL,0);
        exit(0);
    }
    memset(wbuf,0x79,_4K*nblocks);
    memset(op,  0,   QD*sizeof(OP_t));

    /*--------------------------------------------------------------------------
     * loop running IO until secs expire
     *------------------------------------------------------------------------*/
    gettimeofday(&start, NULL);

    do
    {
        /* setup #read ops and #write ops to send before completing ops */
        if (!RD && !WR) {RD=nRD; WR=100-RD;}

        /*----------------------------------------------------------------------
         * send up to RD reads, as long as the queuedepth N is not max
         *--------------------------------------------------------------------*/
        while (TIME && RD && N)
        {
            rc = cblk_aread(id, rbuf, lba, nblocks, &rtag, NULL,
                            CBLK_ARW_WAIT_CMD_FLAGS);
            if (0 == rc) {OP_BEG(rtag); --RD; --N; BMP_LBA();}
            else if (EBUSY == errno) {break;}
            else                     {io_error(id,errno);}
        }
        /*----------------------------------------------------------------------
         * send up to WR writes, as long as the queuedepth N is not max
         *--------------------------------------------------------------------*/
        while (TIME && WR && N)
        {
            rc = cblk_awrite(id, wbuf, lba, nblocks, &rtag, NULL,
                            CBLK_ARW_WAIT_CMD_FLAGS);
            if (0 == rc) {OP_BEG(rtag); --WR; --N; BMP_LBA();}
            else if (EBUSY == errno) {break;}
            else                     {io_error(id,errno);}
        }

        /*----------------------------------------------------------------------
         * complete cmds
         *--------------------------------------------------------------------*/
        for (i=0; i<QD && N<COMP; i++, htag++)
        {
            if (intrp_thds)
            {
                rc = cblk_aresult(id, &htag, &status, CBLK_ARESULT_BLOCKING |
                                                      CBLK_ARESULT_NEXT_TAG);
                if (rc != nblocks) {io_error(id,errno);}
                OP_END(htag); ++cnt; ++N;
                continue;
            }

            if (htag>=QD) htag=0;
            if (!op[htag].iss) {continue;}
            rc = cblk_aresult(id, &htag, &status, 0);
            if (rc == 0)
            {
                if (QD==1 && ++miss==1) {usleep(80);}
                ++tmiss; continue;
            }
            else if (rc < 0)
                {io_error(id,errno);}

            OP_END(htag); ++cnt; ++N; miss=0;
        }

        /*----------------------------------------------------------------------
         * at an interval which does not impact performance, check if secs
         * have expired, and randomize lba
         *--------------------------------------------------------------------*/
        if (cnt > TI)
        {
            TI += TIME_INTERVAL;
            gettimeofday(&delta, NULL);
            if (delta.tv_sec - start.tv_sec >= nsecs) {TIME=0; COMP=QD;}
            lba = lrand48() % nblks;
        }
    }
    while (TIME || QD-N);

    /*--------------------------------------------------------------------------
     * print IO stats
     *------------------------------------------------------------------------*/
    gettimeofday(&delta, NULL);
    esecs = ((float)((delta.tv_sec*mil + delta.tv_usec) -
                     (start.tv_sec*mil + start.tv_usec))) / (float)mil;
    printf("d:%s r:%d q:%d s:%d p:%d n:%d i:%d miss:%d lat:%d mbps:%d iops:%d",
            dev, nRD, QD, nsecs, plun, nblocks, intrp_thds, tmiss,
            (uint32_t)((tlat*ns_per_tick)/cnt/1000),
            (uint32_t)((float)((cnt*nblocks*4)/1024)/esecs),
            (uint32_t)((float)(cnt/esecs)));
    if (plun && nblocks > 1)
        printf(" 4k-iops:%d", (uint32_t)((float)(cnt*nblocks)/esecs));
    printf("\n");

    /*--------------------------------------------------------------------------
     * cleanup
     *------------------------------------------------------------------------*/
    free(op);
    free(rbuf);
    free(wbuf);
    cblk_close(id,0);
    cblk_term(NULL,0);
    return 0;
}
