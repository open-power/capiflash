/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_ark_perf_tool.C $                             */
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
 *   kv ark perf tool with commandline args
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>
#include <fvt_kv_utils_sync_pth.h>

extern "C"
{
#include <fvt_kv_utils.h>
#include <fvt_kv_utils_async_cb.h>
}

void usage(void)
{
    printf("Usage:\n");
    printf("   [-k klen][-v vlen][-l LEN][-s secs][-c ctxts][-t threads]"
           "[-j jobs][-n npool][-S(sync)][-A(async)][-b(r/w)][-r(rdonly)]"
           "[-w(wronly)][-V(verbose)][-h]\n");
    exit(0);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int main(int argc, char **argv)
{
    char     c;
    char     FF    = 0xFF;
    char    *kparm = NULL;
    char    *vparm = NULL;
    char    *lparm = NULL;
    char    *sparm = NULL;
    char    *cparm = NULL;
    char    *tparm = NULL;
    char    *nparm = NULL;
    char    *jparm = NULL;
    uint32_t Sparm = FALSE;
    uint32_t Aparm = FALSE;
    uint32_t mem   = FALSE;
    uint32_t klen  = 16;
    uint32_t vlen  = 16;
    uint32_t secs  = 5;
    uint32_t pths  = 128;
    uint32_t jobs  = 128;
    uint32_t ctxts = 1;
    uint32_t npool = 1;
    uint32_t LEN   = 200;
    uint32_t ro    = FALSE;
    uint32_t wo    = FALSE;
    uint32_t rw    = FALSE;
    uint32_t async = FALSE;
    uint32_t sync  = FALSE;
    uint32_t verbo = FALSE;
    char    *penv  = getenv("FVT_DEV");

    opterr = 1;

    while (FF != (c=getopt(argc, argv, "k:v:l:s:c:t:n:j:rwmbVSAh")))
    {
        switch (c)
        {
            case 'k': kparm = optarg; break;
            case 'v': vparm = optarg; break;
            case 'l': lparm = optarg; break;
            case 's': sparm = optarg; break;
            case 'c': cparm = optarg; break;
            case 't': tparm = optarg; break;
            case 'n': nparm = optarg; break;
            case 'j': jparm = optarg; break;
            case 'r': ro    = TRUE;   break;
            case 'w': wo    = TRUE;   break;
            case 'b': rw    = TRUE;   break;
            case 'm': mem   = TRUE;   break;
            case 'V': verbo = TRUE;   break;
            case 'S': Sparm = TRUE;   break;
            case 'A': Aparm = TRUE;   break;
            case 'h':
            case '?': usage();        break;
        }
    }

    if (kparm) klen  = atoi(kparm);
    if (vparm) vlen  = atoi(vparm);
    if (lparm) LEN   = atoi(lparm);
    if (sparm) secs  = atoi(sparm);
    if (cparm) ctxts = atoi(cparm);
    if (tparm) pths  = atoi(tparm);
    if (jparm) jobs  = atoi(jparm);
    if (nparm) npool = atoi(nparm);
    if (Sparm) sync  = TRUE;
    if (Aparm) async = TRUE;

    if (!sync && !async) {sync=TRUE; async=TRUE;}

    if (mem) penv=NULL;

    if      (vlen < KV_4K)   LEN=200;
    else if (vlen < KV_64K)  LEN=50;
    else if (vlen < KV_250K) LEN=10;
    else                     LEN=1;

    KV_TRC_OPEN(pFT, "fvt");
    FVT_TRC_SIGINT_HANDLER;

    /*-------------------------------------*/
    /* read only perf stats                */
    /*-------------------------------------*/
    if (ro)
    {
        if (!verbo) printf("  ");

        if      (ctxts > 200 && vlen < KV_64K) LEN=50;
        else if (ctxts > 200 && vlen > KV_64K) LEN=2;

        if (ctxts > 50 && pths > 20) pths=20;

        if (verbo)
            printf("\n  RO: ctxts:%d pths:%d jobs:%d %dx%dx%d secs:%d npool:%d "
                    "rw:%d sync:%d async:%d path:%s\n  ",
        ctxts, pths, jobs, klen, vlen, LEN, secs, npool, rw, sync, async, penv);

        Sync_pth sync_pth;
        sync_pth.run_multi_ctxt_rd(ctxts, pths, npool, vlen, LEN, secs);

        printf("\n");
        exit(0);
    }

    /*-------------------------------------*/
    /* write only perf stats               */
    /*-------------------------------------*/
    if (wo)
    {
        if (!verbo) printf("  ");

        if      (ctxts > 200 && vlen < KV_64K) LEN=50;
        else if (ctxts > 200 && vlen > KV_64K) LEN=2;

        if (ctxts > 50 && pths > 20) pths=20;

        if (verbo)
            printf("\n  WO: ctxts:%d pths:%d jobs:%d %dx%dx%d secs:%d npool:%d "
                    "rw:%d sync:%d async:%d path:%s\n  ",
        ctxts, pths, jobs, klen, vlen, LEN, secs, npool, rw, sync, async, penv);

        Sync_pth sync_pth;
        sync_pth.run_multi_ctxt_wr(ctxts, pths, npool, vlen, LEN, secs);

        printf("\n");
        exit(0);
    }

    /*-------------------------------------*/
    /* use read/write separated perf stats */
    /*-------------------------------------*/
    if (rw)
    {
        if (!verbo) printf("\n");

        if (sync)
        {

            ARK *ark = NULL;
            int  mb;

            assert(0 == ark_create_verbose(penv, &ark,
                                           1048576,
                                           4096,
                                           1048576,
                                           npool,
                                           256,
                                           8*1024,
                                           ARK_KV_VIRTUAL_LUN));
            assert(NULL != ark);

            if      (vlen < 256)     mb=1;
            else if (vlen < 1024)    mb=4;
            else if (vlen < KV_4K)   mb=60;
            else if (vlen < KV_64K)  mb=200;
            else if (vlen < KV_250K) mb=600;
            else                     mb=1000;

            if (verbo)
                printf("\n  ctxts:1 pths:1 %dx%dx%d npool:%d rw:%d sync:%d\
  async:%d path:%s\n", klen, vlen, LEN, npool, rw, sync, async, penv);

            fvt_kv_utils_perf(ark, vlen, mb, 50);
            ark_delete(ark);
        }

        if (async)
        {
            if (verbo)
                printf("\n  ctxts:1 %dx%dx%d npool:%d rw:%d sync:%d async:%d\
 path:%s\n", klen, vlen, LEN, npool, rw, sync, async, penv);

            kv_async_job_perf(128, klen, vlen, LEN);
        }

        printf("\n");
        exit(0);
    }

    /*-------------------------------------*/
    /* use general ark stats, r/w combined */
    /*-------------------------------------*/
    if (!verbo) printf("  ");

    if      (ctxts > 200 && vlen < KV_64K) LEN=50;
    else if (ctxts > 200 && vlen > KV_64K) LEN=2;

    if (sync)
    {
        if (ctxts > 50 && pths > 20) pths=20;

        if (verbo)
            printf("\n  ctxts:%d pths:%d jobs:%d %dx%dx%d secs:%d npool:%d "
                   "rw:%d sync:%d async:%d path:%s\n  ",
        ctxts, pths, jobs, klen, vlen, LEN, secs, npool, rw, sync, async, penv);

        Sync_pth sync_pth;
        sync_pth.run_multi_ctxt(ctxts, pths, npool, vlen, LEN, secs);
    }

    if (async)
    {
        if (verbo)
            printf("\n  ctxts:%d pths:%d jobs:%d %dx%dx%d secs:%d npool:%d "
                   "rw:%d sync:%d async:%d path:%s\n  ",
        ctxts, pths, jobs, klen, vlen, LEN, secs, npool, rw, sync, async, penv);
        else
            printf("  ");

        kv_async_init_perf_io(ctxts, jobs, npool, klen, vlen, LEN, secs);
        printf("ctxts:%d jobs:%d npool=%d ", ctxts, jobs, npool); fflush(stdout);
        kv_async_run_jobs();
    }
    printf("\n");
    KV_TRC_CLOSE(pFT);
}
