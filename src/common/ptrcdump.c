/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/ptrcdump.c $                                       */
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
 * \brief dump perf trace file
 *******************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <malloc.h>
#include <inttypes.h>
#include <ticks.h>
#include <math.h>
#include <pthread.h>
#include <perf_trace.h>
#include <cflash_ptrc_ids.h>

/* setup an array of strings for the trace id enums */
char *ptrc_fmt[] =
{
#define X(_e, _s) (_s)
    PTRC_ENUMS
#undef X
};

/**
********************************************************************************
** \brief compare function for qsort
*******************************************************************************/
int cmpfunc (const void *a, const void *b)
{
    PE_t *A = (PE_t*)a;
    PE_t *B = (PE_t*)b;
    if      (A->ntime > B->ntime) return 1;
    else if (A->ntime < B->ntime) return -1;
    else                          return 0;
}

/**
********************************************************************************
** \brief print the usage
** \details
**   -f file       *the ptrc binary input file                                \n
**   -t tid        *the thread id to filter traces                            \n
**   -l lba        *the lba to filter traces                                  \n
**   -csv          *print traces as csv                                       \n
**   -v            *print traces to the console rather than a file            \n
*******************************************************************************/
void usage(void)
{
    printf("Usage:\n");
    printf("\
  [-f input_file] [-t tid] [-l lba] [-csv] [-v]\n\
  *default output is in input_file.txt");
    exit(0);
}

/**
********************************************************************************
** \brief main
** \details
**   process input parms                                                      \n
**   read input                                                               \n
**   print output                                                             \n
*******************************************************************************/
int main(int argc, char **argv)
{
    PE_t     *pLog    = NULL;
    char      fn[128] = {0};
    char     *opt_fn  = NULL;
    char     *opt_tid = NULL;
    char     *opt_lba = NULL;
    FILE     *fp      = NULL;
    int       first   = 1;
    uint64_t  prev    = 0;
    uint64_t  r       = 0;
    int       s       = 0;
    int       ms      = 0;
    int       us      = 0;
    int       ns      = 0;
    int       i       = 0;
    int       verbose = 0;
    int       csv     = 0;
    uint64_t  tid     = 0;
    uint64_t  lba     = 0;
    char      FF      = 0xFF;
    char      c       = '\0';
    struct stat st;

    /*--------------------------------------------------------------------------
     * process and verify input parms
     *------------------------------------------------------------------------*/
    while (FF != (c=getopt(argc, argv, "f:t:l:c:vh")))
    {
        switch (c)
        {
            case 'f': opt_fn  = optarg; break;
            case 't': opt_tid = optarg; break;
            case 'l': opt_lba = optarg; break;
            case 'v': verbose = 1;      break;
            case 'c': csv     = 1;      break;
            case 'h': usage();
        }
    }

    /*--------------------------------------------------------------------------
     * setup and read input
     *------------------------------------------------------------------------*/
    if (opt_fn) {sprintf(fn, "%s", opt_fn);}
    else        {sprintf(fn, "/tmp/ptrc");}
    if (opt_tid) {tid = strtol(opt_tid, NULL, 16); printf("tid:0x%16lx\n", tid);}
    if (opt_lba) {lba = strtol(opt_lba, NULL, 16);}

    if (stat(fn, &st)) {printf("cannot open %s\n", fn); goto exit;}

    fp = fopen(fn, "rb");

    pLog = malloc(st.st_size);
    fread(pLog,st.st_size,1,fp);
    fclose(fp);

    /*--------------------------------------------------------------------------
     * setup for output
     *------------------------------------------------------------------------*/
    int logN = st.st_size / sizeof(PE_t);
    qsort(pLog, logN, sizeof(PE_t), cmpfunc);

    if      (csv)    {sprintf(fn, "%s.csv", opt_fn);}
    else if (opt_fn) {sprintf(fn, "%s.txt", opt_fn);}
    else             {sprintf(fn, "/tmp/ptrc.txt");}

    if (verbose) {fp=stdout;}
    else         {fp = fopen(fn,"w");}
    if (!csv)
    {
            fprintf(fp, "SEC  MS  US  NS ID                         "
                        "CPU    TID         d1            d2                 d3\n");
    }

    /*--------------------------------------------------------------------------
     * loop through each trace entry and output
     *------------------------------------------------------------------------*/
    for (i=0; i<logN; i++)
    {
        if (pLog[i].ntime == 0)        {goto done;}
        if (tid && pLog[i].tid != tid) {continue;}
        if (lba && pLog[i].d2  != lba) {continue;}

        if (csv)
        {
            fprintf(fp, "%ld,%d,%d,%d,%lX,%lX,%ld\n",
                    pLog[i].ntime, pLog[i].id, pLog[i].cpu, pLog[i].tid,
                    pLog[i].d1, pLog[i].d2, pLog[i].d3);
            continue;
        }

        if (first) {first=0;}
        else
        {
            r    = pLog[i].ntime - prev; if (!r) {continue;}
            s    = r/1000000000UL; r -= s*1000000000UL;
            ms   = r/1000000;    r -= ms*1000000;
            us   = r/1000;       r -= us*1000;
            ns   = r;
        }
        prev = pLog[i].ntime;

        if (pLog[i].id == CBLK_CMP_CMD)
        {
            fprintf(fp, "%3d %3d %3d %3d %-25s %3d %8X %16lX %16ld %16ld\n",
                 s, ms, us, ns, ptrc_fmt[pLog[i].id], pLog[i].cpu, pLog[i].tid,
                 pLog[i].d1, pLog[i].d2, pLog[i].d3);
        }
        else if (pLog[i].id > 0 && pLog[i].id < PTRC_LAST)
        {
            fprintf(fp, "%3d %3d %3d %3d %-25s %3d %8X %16lX %16lX %16ld\n",
              s, ms, us, ns, ptrc_fmt[pLog[i].id], pLog[i].cpu, pLog[i].tid,
              pLog[i].d1, pLog[i].d2, pLog[i].d3);
        }
        else
        {
            fprintf(fp, "%3d %3d %3d %3d %3d %3d %8X %16lX %16lX %16lX\n",
                            s, ms, us, ns, pLog[i].id, pLog[i].cpu, pLog[i].tid,
                            pLog[i].d1, pLog[i].d2, pLog[i].d3);
        }
    }
done:
    fclose(fp);
    free(pLog);
exit:
    return 0;
}
