/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/perf_trace.h $                                    */
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
 *   definitions for a performance trace facility
 * \details
 *   export PTRC_VERBOSITY=9 for maximum tracing
 *
 *   OFF          PTRC_VERBOSITY == 0
 *   ON           PTRC_VERBOSITY >= 1
 *   THREAD_ID    PTRC_VERBOSITY >= 2
 *   CPU_ID       PTRC_VERBOSITY >= 3
 * \ingroup
 ******************************************************************************/
#ifndef _PERF_TRACE_H
#define _PERF_TRACE_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/timeb.h>
#include <string.h>
#include <unistd.h>
#include <ticks.h>
#include <math.h>

#ifdef _AIX
#define GETTID pthread_getthreadid_np()
#define GETCPU(cpu) cpu=0
#else
#include <sys/syscall.h>
#define GETTID (syscall(SYS_gettid))
#define GETCPU(_cpu)                                                           \
do                                                                             \
{                                                                              \
    int cpu;                                                                   \
    syscall(SYS_getcpu, &cpu, NULL, NULL);                                     \
    _cpu = cpu;                                                                \
} while (0)
#ifndef fetch_and_add
#define fetch_and_add(ptr,val) (__sync_fetch_and_add(ptr,val))
#endif
#endif

#define B_EYE  0xFEADDEAF
#define E_EYE  0xDEAFFEAD

#define EYEC_ERR(_ptrc)              \
        ((!(_ptrc))               || \
        ((_ptrc)->b_eye != B_EYE) || \
        ((_ptrc)->e_eye != E_EYE))

typedef struct
{
    uint64_t ntime;
    uint32_t tid;
    uint16_t cpu;
    uint16_t id;
    uint64_t d1;
    uint64_t d2;
    uint64_t d3;
} PE_t;

typedef struct
{
    uint32_t        b_eye;
    uint32_t        verbosity;
    pthread_mutex_t lock;
    uint64_t        init_done;
    uint64_t        inuse;
    uint64_t        priority;
    PE_t           *pLog;
    uint32_t        logN;
    uint32_t        logI;
    double          nspt;
    uint32_t        e_eye;
} PerfTrace_t;

/**
 *******************************************************************************
 * \brief
 *   setup to run perf log trace macro
 ******************************************************************************/
static __inline__ void PTRC_INIT(PerfTrace_t *ptrc, uint32_t num_logs, double ns)
{
    char *env_verbosity = getenv("PTRC_VERBOSITY");

    memset(ptrc, 0, sizeof(PerfTrace_t));
    if (env_verbosity) {ptrc->verbosity = atoi(env_verbosity);}
    if (ptrc->verbosity && EYEC_ERR(ptrc))
    {
        pthread_mutex_init(&ptrc->lock, NULL);
        pthread_mutex_lock(&ptrc->lock);
        ptrc->b_eye     = B_EYE;
        ptrc->e_eye     = E_EYE;
        ptrc->logN      = num_logs;
        if (ns) {ptrc->nspt = ns;}
        else    {ptrc->nspt = time_per_tick(1000,100);}
        ptrc->init_done = 1;
        int bytes       = num_logs * sizeof(PE_t);
        ptrc->pLog      = malloc(bytes);
        memset(ptrc->pLog,0,bytes);
        pthread_mutex_unlock(&ptrc->lock);
    }
}

/**
 *******************************************************************************
 * \brief
 *   save trace entries to a file, and clear inuse
 ******************************************************************************/
static __inline__ void PTRC_WRITE_TO_FILE(PerfTrace_t *ptrc, char *fn)
{
    if (ptrc->verbosity && ptrc->inuse)
    {
        uint16_t N  = ptrc->logI;
        int      i  = 0;
        FILE     *fp = fopen(fn,"wb");
        for (i=0; fp && i<ptrc->logN; i++)
        {
            if (ptrc->pLog[N].ntime)
                {fwrite(ptrc->pLog+N,sizeof(PE_t),1,fp);}
            N = (N+1) % ptrc->logN;
        }
        if (fp) {fclose(fp);}
        ptrc->inuse = 0;
    }
}

/**
 *******************************************************************************
 * \brief
 *   save file and free mem
 ******************************************************************************/
static __inline__ void PTRC_DONE(PerfTrace_t *ptrc, char *fn)
{
    if (EYEC_ERR(ptrc)) return;

    pthread_mutex_lock(&ptrc->lock);

    if (ptrc->init_done)
    {
        PTRC_WRITE_TO_FILE(ptrc,fn);
        free(ptrc->pLog);
        ptrc->init_done=0;
    }

    pthread_mutex_unlock(&ptrc->lock);
}

/**
 *******************************************************************************
 * \brief
 *   save trace entries to a file
 ******************************************************************************/
static __inline__ void PTRC_SAVE(PerfTrace_t *ptrc, uint32_t v, char *fn)
{
    if (EYEC_ERR(ptrc) || !(ptrc->verbosity>v)) return;

    pthread_mutex_lock(&ptrc->lock);

    if (ptrc->init_done) {PTRC_WRITE_TO_FILE(ptrc,fn);}

    pthread_mutex_unlock(&ptrc->lock);
}

/**
 *******************************************************************************
 * \brief
 *   copy the logs from _s to _t
 ******************************************************************************/
static __inline__ int PTRC_COPY(PerfTrace_t *s, PerfTrace_t *t, uint32_t v)
{
    int cp_rc=0;

    if (t->verbosity>=v && !((EYEC_ERR(s)) || (EYEC_ERR(t))) && t->init_done)
    {
        pthread_mutex_lock(&t->lock);
        memcpy(t->pLog, s->pLog, s->logN*sizeof(PE_t));
        t->inuse = 1;
        pthread_mutex_unlock(&t->lock);
        cp_rc=1;
    }
    return cp_rc;
}

/**
 *******************************************************************************
 * \brief
 *   copy the logs from _s to _t, if _p > _t->priority
 ******************************************************************************/
static __inline__ int PTRC_PCOPY(PerfTrace_t *s,
                                 PerfTrace_t *t,
                                 uint32_t     v,
                                 uint64_t     p)
{
    int pcp_rc=0;

    if (s && t && !((EYEC_ERR(t))) &&
        t->init_done && p > t->priority)
    {
        if ((pcp_rc=PTRC_COPY(s, t, v))) {t->priority=p;}
    }
    return pcp_rc;
}

/**
 *******************************************************************************
 * \brief
 *   save the log to the buf
 ******************************************************************************/
#define _PTRC_E(ptrc, _id, _d1, _d2, _d3)                                      \
do                                                                             \
{                                                                              \
        PE_t *_pe=&(ptrc)->pLog[(ptrc)->logI++];                               \
        if ((ptrc)->logI==(ptrc)->logN) {(ptrc)->logI=0;}                      \
        (ptrc)->inuse = 1;                                                     \
        if ((ptrc)->verbosity >= 2)                                            \
        {                                                                      \
            _pe->tid = (uint64_t)GETTID;                                       \
            if ((ptrc)->verbosity >= 3) {GETCPU(_pe->cpu);}                    \
        }                                                                      \
        else {_pe->tid=0; _pe->cpu = 0;}                                       \
        uint64_t tks=getticks();                                               \
        _pe->id    = (uint64_t)_id;                                            \
        _pe->d1    = (uint64_t)_d1;                                            \
        _pe->d2    = (uint64_t)_d2;                                            \
        _pe->d3    = (uint64_t)_d3;                                            \
        _pe->ntime = (tks * ((ptrc)->nspt));                                   \
} while (0)

/**
 *******************************************************************************
 * \brief
 *   save the log to the buf
 ******************************************************************************/
#define PTRC_E(ptrc, _v, _e) _PTRC_E(ptrc, _e);

/**
 *******************************************************************************
 * \brief
 *   save the log to the buf, if UDELTA(_t) > _l
 ******************************************************************************/
#define PTRC_U(ptrc, _v, _t, _l, _id, _d1, _d2)                                \
do                                                                             \
{                                                                              \
    if ((ptrc)->verbosity >= _v && ! (EYEC_ERR(ptrc)) && (ptrc)->init_done)    \
    {                                                                          \
        ticks _d = UDELTA(_t,(ptrc)->nspt);                                    \
        if (_d > _l)                                                           \
        {                                                                      \
            _PTRC_E(ptrc, _id, _d1, _d2, _d);                                  \
        }                                                                      \
    }                                                                          \
} while (0)

/**
 *******************************************************************************
 * \brief
 *   save the log to the buf
 ******************************************************************************/
#define PTRC(ptrc, _v, _id, _d1, _d2, _d3)                                     \
do                                                                             \
{                                                                              \
    if ((ptrc)->verbosity >= _v && ! (EYEC_ERR(ptrc)) && (ptrc)->init_done)    \
    {                                                                          \
        _PTRC_E(ptrc, _id, _d1, _d2, _d3);                                     \
    }                                                                          \
} while (0)

#endif
