/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/kv_trace.h $                                           */
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
 *   definitions for a kv trace facility
 * \details
 *   export KV_CFLAGS=-DKV_TRC_DISABLE before building to disable the trace
 *   export KV_TRC_VERBOSITY=9 for maximum tracing
 *
 *   OFF          KV_TRC_VERBOSITY == 0
 *   KV_TRC_FFDC  KV_TRC_VERBOSITY <= 1
 *   KV_TRC_PERF1 KV_TRC_VERBOSITY <= 2
 *   KV_TRC_PERF2 KV_TRC_VERBOSITY <= 3
 *   KV_TRC       KV_TRC_VERBOSITY <= 4
 *   KV_TRC_IO    KV_TRC_VERBOSITY <= 5
 *   KV_TRC_DBG   KV_TRC_VERBOSITY <= 6
 *   KV_TRC_EXT1  KV_TRC_VERBOSITY <= 7
 *   KV_TRC_EXT2  KV_TRC_VERBOSITY <= 8
 *   KV_TRC_EXT3  KV_TRC_VERBOSITY <= 9
 *   KV_TRC_HEX
 * \ingroup
 ******************************************************************************/
#ifndef _H_KV_TRACE_LOG
#define _H_KV_TRACE_LOG

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/timeb.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define B_EYE  0xABCDBEEF
#define E_EYE  0xDCBAFEED
#define STRLEN 1024

#define EYEC_INVALID(_p_KT) ((_p_KT->b_eye != B_EYE) || (_p_KT->e_eye != E_EYE))

typedef struct
{
    uint32_t        b_eye;
    pthread_mutex_t lock;
    uint32_t        init_done;
    uint32_t        verbosity;
    FILE           *logfp;
    uint32_t        log_number;
    uint32_t        start_time_valid;
    struct timeb    start_time;
    struct timeb    last_time;
    uint32_t        e_eye;
} KV_Trace_t;

#ifndef KV_TRC_DISABLE

/**
 *******************************************************************************
 * \brief
 *   setup to run kv log trace macro
 ******************************************************************************/
#define KV_TRC_OPEN( _pKT, _fn_in)                                             \
do                                                                             \
{                                                                              \
    char *env_verbosity = getenv("KV_TRC_VERBOSITY");                          \
    char *env_dir       = getenv("KV_TRC_DIR");                                \
    char *env_user      = getenv("USER");                                      \
    char  dir[STRLEN+1] = {0};                                                 \
    char  fn[STRLEN+1]  = {0};                                                 \
    int   pid           = getpid();                                            \
    struct timeb _cur_time;                                                    \
                                                                               \
    if (NULL == _pKT) break;                                                   \
                                                                               \
    /* if this is the first call or we were corrupted
     * begin the trace as an append to any previous traces for the filename
     */                                                                        \
    if (EYEC_INVALID(_pKT))                                                    \
    {                                                                          \
        memset(_pKT, 0, sizeof(KV_Trace_t));                                   \
        _pKT->b_eye     = B_EYE;                                               \
        _pKT->e_eye     = E_EYE;                                               \
        _pKT->verbosity = 1;                                                   \
        pthread_mutex_init(&_pKT->lock, NULL);                                 \
    }                                                                          \
                                                                               \
    pthread_mutex_lock(&_pKT->lock);                                           \
                                                                               \
    if (_pKT->init_done >= 1) {++_pKT->init_done; goto open_unlock;}           \
                                                                               \
    if (env_dir && strlen(env_dir)<256) {sprintf(dir, "%s", env_dir);}         \
    else                                {sprintf(dir, "/tmp");}                \
                                                                               \
    if (_fn_in) snprintf(fn, STRLEN, "%s/%s.%s.kv.log", dir, env_user, _fn_in);\
    else        snprintf(fn, STRLEN, "%s/%s.kv.log",    dir, env_user);        \
                                                                               \
    if ((_pKT->logfp = fopen(fn, "a")) == NULL)                                \
    {                                                                          \
        fprintf(stderr, "\nFailed to open log trace file %s\n", fn);           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        if (!_pKT->start_time_valid)                                           \
        {                                                                      \
            ftime(&_cur_time);                                                 \
            _pKT->start_time       = _cur_time;                                \
            _pKT->start_time_valid = 1;                                        \
            fprintf(_pKT->logfp,"----------------------------------------------\
---------------------------------\n");                                         \
            fprintf(_pKT->logfp,"START:%d Date is %s at %s\n",                 \
                    pid, __DATE__,__TIME__);                                   \
            fprintf(_pKT->logfp,"%-9s %-6s %10s %10s %-30s %-25s\n",           \
     "  PID", "Index", "sec.msec", "delta.dmsec", "Line:Filename", "Function");\
            fprintf(_pKT->logfp,"----------------------------------------------\
---------------------------------\n");                                         \
        }                                                                      \
        fprintf(_pKT->logfp,"OPEN:  PID:%d Date is %s at %s\n",                \
                            getpid(), __DATE__,__TIME__);                      \
        ++_pKT->init_done;                                                     \
        if (env_verbosity) {_pKT->verbosity = atoi(env_verbosity);}            \
    }                                                                          \
                                                                               \
open_unlock:                                                                   \
    pthread_mutex_unlock(&_pKT->lock);                                         \
} while (0)

/**
 *******************************************************************************
 * \brief
 *   cleanup and close the kv trace
 ******************************************************************************/
#define KV_TRC_CLOSE(_pKT)                                                     \
do                                                                             \
{                                                                              \
    if (NULL == _pKT)         break;                                           \
    if (EYEC_INVALID(_pKT))   break;                                           \
    if (_pKT->init_done == 0) break;                                           \
                                                                               \
    pthread_mutex_lock(&_pKT->lock);                                           \
                                                                               \
    if (_pKT->init_done > 1)  {--_pKT->init_done; goto close_unlock;}          \
                                                                               \
    if (_pKT->logfp)                                                           \
    {                                                                          \
        if (_pKT->start_time_valid)                                            \
        {                                                                      \
            fprintf(_pKT->logfp,"CLOSE: PID:%d Date is %s at %s\n",            \
                                getpid(), __DATE__,__TIME__);                  \
        }                                                                      \
        fflush(_pKT->logfp);                                                   \
        fclose(_pKT->logfp);                                                   \
    }                                                                          \
    --_pKT->init_done;                                                         \
                                                                               \
close_unlock:                                                                  \
    pthread_mutex_unlock(&_pKT->lock);                                         \
} while (0)

/**
 *******************************************************************************
 * \brief
 *   save the msg to the log file
 ******************************************************************************/
#define KV_TRACE_LOG_DATA(_pKT, _fmt, ...)                                     \
do                                                                             \
{                                                                              \
    char         _str[STRLEN] = {0};                                           \
    int          pid          = getpid();                                      \
    struct timeb _cur_time, _log_time, _delta_time;                            \
                                                                               \
    if (EYEC_INVALID(_pKT))  break;                                            \
    if (!_pKT->init_done)    break;                                            \
    if (NULL == _pKT->logfp) break;                                            \
                                                                               \
    pthread_mutex_lock(&_pKT->lock);                                           \
                                                                               \
    ftime(&_cur_time);                                                         \
                                                                               \
    /* Find time offset since starting time */                                 \
    _log_time.time      = _cur_time.time    - _pKT->start_time.time;           \
    _log_time.millitm   = _cur_time.millitm - _pKT->start_time.millitm;        \
    _delta_time.time    = _log_time.time    - _pKT->last_time.time;            \
    _delta_time.millitm = _log_time.millitm - _pKT->last_time.millitm;         \
                                                                               \
    fprintf(_pKT->logfp,"  %-6d %-6d %3d.%03d %3d.%05d ",                      \
            pid,                                                               \
            _pKT->log_number,                                                  \
            (int)_log_time.time,                                               \
            _log_time.millitm,                                                 \
            (int)_delta_time.time,                                             \
            _delta_time.millitm);                                              \
                                                                               \
    snprintf(_str, STRLEN, _fmt, ##__VA_ARGS__);                               \
    fprintf(_pKT->logfp," %4d:%-25s %-25s %s\n",                               \
            __LINE__,__FILE__,__FUNCTION__,_str);                              \
    fflush(_pKT->logfp);                                                       \
                                                                               \
    _pKT->log_number++;                                                        \
    _pKT->last_time = _log_time;                                               \
                                                                               \
    pthread_mutex_unlock(&_pKT->lock);                                         \
} while (0)

/* _v  - verbosity
 * _s  - string to prepend
 * _hx - array of uint8_t to dump as hex
 * _l  - length in bytes to dump
 */
#define KV_TRC_HEX(_pKT, _v, _s, _hx, _l)                                      \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= _v)                                 \
            {                                                                  \
                char     buf[512]={0};                                         \
                uint8_t *hxd=(uint8_t*)_hx;                                    \
                char    *pb=buf+strlen(_s);                                    \
                uint32_t i;                                                    \
                sprintf(buf, "%s", (char*)_s);                                 \
                for (i=0; i<(uint32_t)(_l>64?64:_l); i++)                      \
                    {sprintf(pb+(i*2), "%02x",hxd[i]);}                        \
                KV_TRACE_LOG_DATA(_pKT, "%s", buf);                            \
            }                                                                  \
        } while (0)

#define KV_TRC_FFDC(_pKT, msg, ...)                                            \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 1)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_PERF1(_pKT, msg, ...)                                           \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 2)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_PERF2(_pKT, msg, ...)                                           \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 3)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC(_pKT, msg, ...)                                                 \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 4)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_IO(_pKT, msg, ...)                                              \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 5)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_DBG(_pKT, msg, ...)                                             \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 6)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_EXT1(_pKT, msg, ...)                                            \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 7)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_EXT2(_pKT, msg, ...)                                            \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 8)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)

#define KV_TRC_EXT3(_pKT, msg, ...)                                            \
        do                                                                     \
        {                                                                      \
            if (_pKT && _pKT->verbosity >= 9)                                  \
            {                                                                  \
                KV_TRACE_LOG_DATA(_pKT, msg, ## __VA_ARGS__);                  \
            }                                                                  \
        } while (0)
#else
#define KV_TRC_OPEN(_pKT, _fn_in)
#define KV_TRC_CLOSE(_pKT)
#define KV_TRC_FFDC(_pKT, msg, ...)
#define KV_TRC_PERF1(_pKT, msg, ...)
#define KV_TRC_PERF2(_pKT, msg, ...)
#define KV_TRC(_pKT, msg, ...)
#define KV_TRC_IO(_pKT, msg, ...)
#define KV_TRC_DBG(_pKT, msg, ...)
#define KV_TRC_EXT1(_pKT, msg, ...)
#define KV_TRC_EXT2(_pKT, msg, ...)
#define KV_TRC_EXT3(_pKT, msg, ...)
#endif

#endif
