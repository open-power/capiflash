/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/blockplistio.c $                               */
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
 * \brief Block Interface Pending List I/O Driver
 * \details
 *   This runs I/O to the capi Block Interface using List IO and pending
 *   lists. The expected iops are 300k-400k per capi card.                    \n
 *   Example:                                                                 \n
 *                                                                            \n
 *     blockplistio -d /dev/sg5                                               \n
 *     d:/dev/sg34 l:1 c:120 p:0 s:5 i:0 tmiss:9 mbps:1498 iops:383657        \n
 *                                                                            \n
 *     blockplistio -d /dev/sg10 -s 20 -l 1 -c 1 -p -i                        \n
 *     d:/dev/sg34 l:1 c:1 p:1 s:20 i:1 tmiss:18620996 mbps:18 iops:4760
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

#define _8K   8*1024
#define _4K   4*1024

#define TIME_INTERVAL 1024
#define SET_NBLKS     260*1024

#ifdef _AIX
#define USLEEP 100
#else
#define USLEEP 50
#endif

typedef struct
{
    cblk_io_t **ioI;
    cblk_io_t **ioP;
    cblk_io_t **ioC;
    cblk_io_t  *p_ioI;
    cblk_io_t  *p_ioP;
    cblk_io_t  *p_ioC;
    uint8_t   **rbuf;
    uint32_t    do_send;
    uint32_t    do_cmp;
    uint32_t    ncmp;
    uint32_t    nI;
    uint32_t    nP;
} listio_t;

/**
********************************************************************************
** \brief bump lba, reset lba if it wraps
*******************************************************************************/
#define BMP_LBA() lba+=2; if (lba >= nblks) {lba=lrand48()%nblks;}

/**
********************************************************************************
** \brief print the usage
** \details
**   -d device     *the device name to run Block IO                           \n
**   -l #lists     *the number of lists                                       \n
**   -c #cmds      *the number of cmds per list                               \n
**   -s secs       *the number of seconds to run the I/O                      \n
**   -p            *run in physical lun mode                                  \n
**   -i            *run using interrupts, not polling                         \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("   [-d device] [-l lists] [-c cmdsperlist] [-p] [-s secs] [-i]\n");
    exit(0);
}

/**
********************************************************************************
** \brief print errno and exit
** \details
**   an IO has failed, print the errno and exit
*******************************************************************************/
void io_error(int id, char *str, int err)
{
    fprintf(stderr, "io_error: %s errno:%d\n", str, err);
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
    long int        mil        = 1000000;
    float           esecs      = 0;
    char           *dev        = NULL;
    char            FF         = 0xFF;
    char            c          = '\0';
    chunk_ext_arg_t ext        = 0;
    int             flags      = 0;
    int             rc         = 0;
    int             i,j        = 0;
    int             id         = 0;
    char           *_secs      = NULL;
    char           *_NL        = NULL;
    char           *_LD        = NULL;
    uint32_t        nsecs      = 5;
    uint32_t        NL         = 1;
    uint32_t        LD         = 120;
    uint32_t        intrp_thds = 0;
    uint32_t        plun       = 0;
    int             tag        = 0;
    int             nC         = LD;
    uint32_t        cnt        = 0;
    size_t          nblks      = 0;
    uint32_t        TI         = TIME_INTERVAL;
    uint32_t        N          = 0;
    uint32_t        TIME       = 1;
    uint32_t        miss       = 0;
    uint32_t        tmiss      = 0;
    uint32_t        lba        = 0;
    uint32_t        p_i        = 0;
    listio_t       *lio        = NULL;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "d:l:c:s:phi")))
    {
        switch (c)
        {
            case 'd': dev        = optarg; break;
            case 'l': _NL        = optarg; break;
            case 'c': _LD        = optarg; break;
            case 's': _secs      = optarg; break;
            case 'p': plun       = 1;      break;
            case 'i': intrp_thds = 1;      break;
            case 'h':
            case '?': usage();             break;
        }
    }
    if (_secs) nsecs = atoi(_secs);
    if (_NL)   NL    = atoi(_NL);
    if (_LD)   LD    = atoi(_LD);

    if (dev == NULL) usage();

    srand48(time(0));

    if (NL*LD > _8K)
    {
        fprintf(stderr, "-l %d * -c %d cannot be greater than 8k\n", NL, LD);
        usage();
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
    id = cblk_open(dev, NL*LD, O_RDWR, ext, flags);
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
        fprintf(stderr, "cblk_get_lun_size failed, errno:%d\n", errno);
        exit(errno);
    }
    if (!plun)
    {
        nblks = nblks > SET_NBLKS ? SET_NBLKS : nblks;
        rc = cblk_set_size(id, nblks, 0);
        if (rc)
        {
            fprintf(stderr, "cblk_set_size failed, errno:%d\n", errno);
            exit(errno);
        }
    }

    /*--------------------------------------------------------------------------
     * alloc data for IO
     *------------------------------------------------------------------------*/
    lio = malloc(NL*sizeof(listio_t));
    assert(lio != NULL);

    for (i=0; i<NL; i++)
    {
        memset(lio+i, 0, sizeof(listio_t));
        lio[i].do_send = 1;
        lio[i].ioI     = malloc(LD*sizeof(cblk_io_t**));
        lio[i].ioP     = malloc(LD*sizeof(cblk_io_t**));
        lio[i].ioC     = malloc(LD*sizeof(cblk_io_t**));
        lio[i].p_ioI   = malloc(LD*sizeof(cblk_io_t));
        lio[i].p_ioP   = malloc(LD*sizeof(cblk_io_t));
        lio[i].p_ioC   = malloc(LD*sizeof(cblk_io_t));
        lio[i].rbuf    = malloc(LD*sizeof(uint8_t*));
        assert(lio[i].ioI   != NULL);
        assert(lio[i].ioP   != NULL);
        assert(lio[i].ioC   != NULL);
        assert(lio[i].p_ioI != NULL);
        assert(lio[i].p_ioP != NULL);
        assert(lio[i].p_ioC != NULL);
        assert(lio[i].rbuf  != NULL);
        memset(lio[i].rbuf,  0, LD*sizeof(uint8_t*));
        memset(lio[i].p_ioI, 0, LD*sizeof(cblk_io_t));
        memset(lio[i].p_ioP, 0, LD*sizeof(cblk_io_t));
        memset(lio[i].p_ioC, 0, LD*sizeof(cblk_io_t));

        for (j=0; j<LD; j++)
        {
            if (posix_memalign((void**)&lio[i].rbuf[j], _4K, _4K))
            {
                fprintf(stderr,"posix_memalign failed, exiting\n");
                cblk_close(id,0);
                cblk_term(NULL,0);
                exit(0);
            }
            lio[i].ncmp   = LD;
            lio[i].ioI[j] = lio[i].p_ioI+j;
            lio[i].ioP[j] = lio[i].p_ioP+j;
            lio[i].ioC[j] = lio[i].p_ioC+j;

            lba+=2;
            lio[i].ioI[j]->request_type = CBLK_IO_TYPE_READ;
            lio[i].ioI[j]->buf          = lio[i].rbuf[j];
            lio[i].ioI[j]->lba          = lba;
            lio[i].ioI[j]->nblocks      = 1;
        }
    }

    /*--------------------------------------------------------------------------
     * loop running IO until secs expire
     *------------------------------------------------------------------------*/
    gettimeofday(&start, NULL);

    do
    {
        lio[tag].nI   = 0;
        lio[tag].ncmp = 0;
        nC            = LD;

        /* if time is up and there are no pending ops for this tag, skip */
        if (!TIME && lio[tag].nP == 0) {if (++tag >= NL) tag = 0; continue;}

        if (TIME)
        {
            /* set lba to issue LD-nP more reads */
            for (lio[tag].nI=0; lio[tag].nI<LD-lio[tag].nP; lio[tag].nI++)
            {
                lio[tag].ioI[lio[tag].nI]->lba = lba;
                BMP_LBA();
            }
        }

        rc = cblk_listio(id, lio[tag].ioI, lio[tag].nI,
                             lio[tag].ioP, lio[tag].nP,
                             NULL,0,
                             lio[tag].ioC, &nC,
                             0,0);
        if (rc == -1) {io_error(id, "COMP", errno);}

        N += lio[tag].nI;

        /*--------------------------------------------------------------
         * collapse the pending list to only the pending cmds
         *------------------------------------------------------------*/
        p_i = 0;
        for (i=0; i<lio[tag].nP; i++)
        {
            if (lio[tag].ioP[i]->stat.status == -1)
                {io_error(id, "status==-1", -1);}
            else if (lio[tag].ioP[i]->stat.status ==CBLK_ARW_STATUS_SUCCESS)
                {++cnt; ++lio[tag].ncmp; miss=0;}
            else if (lio[tag].ioP[i]->stat.status == CBLK_ARW_STATUS_FAIL)
                {io_error(id, "CMD FAIL",lio[tag].ioP[i]->stat.fail_errno);}
            else if (lio[tag].ioP[i]->stat.status ==CBLK_ARW_STATUS_PENDING)
            {
                if (i==p_i) {++p_i; continue;}
                else
                {
                    memcpy(lio[tag].ioP[p_i++],
                           lio[tag].ioP[i], sizeof(cblk_io_t));
                }
            }
            else
                {io_error(id, "PEN CMD ELSE",lio[tag].ioP[i]->stat.status);}
        }
        /*--------------------------------------------------------------
         * copy the newly issued pending cmds to the pending list
         *------------------------------------------------------------*/
        for (i=0; i<lio[tag].nI; i++)
        {
            if (lio[tag].ioI[i]->stat.status == -1)
                {io_error(id, "status==-1", -1);}
            else if (lio[tag].ioI[i]->stat.status ==CBLK_ARW_STATUS_SUCCESS)
                {++cnt; ++lio[tag].ncmp; miss=0;}
            else if (lio[tag].ioI[i]->stat.status == CBLK_ARW_STATUS_FAIL)
                {io_error(id, "CMD FAIL",lio[tag].ioI[i]->stat.fail_errno);}
            else if (lio[tag].ioI[i]->stat.status ==CBLK_ARW_STATUS_PENDING)
                {memcpy(lio[tag].ioP[p_i++],lio[tag].ioI[i],sizeof(cblk_io_t));}
            else
                {io_error(id, "ISS CMD ELSE",lio[tag].ioI[i]->stat.status);}
        }
        lio[tag].nP = p_i;

        if      (lio[tag].ncmp == 0)  {++miss; ++tmiss;}
        else if (lio[tag].ncmp == LD) {miss=0;}

        if (NL==1 && LD==1 && !lio[tag].ncmp && miss==1) {usleep(80);}

        N -= lio[tag].ncmp;
        if (++tag >= NL) tag = 0;

        /*----------------------------------------------------------------------
         * at an interval which does not impact performance, check if secs
         * have expired, and randomize lba
         *--------------------------------------------------------------------*/
        if (cnt >= TI)
        {
            TI += TIME_INTERVAL;
            gettimeofday(&delta, NULL);
            if (delta.tv_sec - start.tv_sec >= nsecs) {TIME=0;}
            lba = lrand48() % nblks;
        }
    }
    while (TIME || N);

    /*--------------------------------------------------------------------------
     * print IO stats
     *------------------------------------------------------------------------*/
    gettimeofday(&delta, NULL);
    esecs = ((float)((delta.tv_sec*mil + delta.tv_usec) -
                     (start.tv_sec*mil + start.tv_usec))) / (float)mil;
    printf("d:%s l:%d c:%d p:%d s:%d i:%d tmiss:%d mbps:%d iops:%d\n",
            dev, NL, LD, plun, nsecs, intrp_thds, tmiss,
            (uint32_t)((float)(cnt*4)/1024/esecs),
            (uint32_t)((float)cnt/esecs));

    /*--------------------------------------------------------------------------
     * cleanup
     *------------------------------------------------------------------------*/
    for (i=0; i<NL; i++)
    {
        free(lio[i].ioI);
        free(lio[i].ioP);
        free(lio[i].ioC);
        free(lio[i].p_ioI);
        free(lio[i].p_ioP);
        free(lio[i].p_ioC);
        for (j=0; j<LD; j++) free(lio[i].rbuf[j]);
        free(lio[i].rbuf);
    }
    free(lio);
    cblk_close(id,0);
    cblk_term(NULL,0);
    return 0;
}
