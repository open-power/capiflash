/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/block_alloc_internal.h $                           */
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
#ifndef _BLOCK_ALLOC_INTERNAL_H
#define _BLOCK_ALLOC_INTERNAL_H

#include <sys/types.h>
#ifndef _AIX
#include <linux/types.h>
#endif /* !_AIX */
#include <inttypes.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <syslog.h>

#define MAX_AUN_CLONE_CNT    0xFF

typedef struct lun_info {
    uint64_t      *lun_alloc_map;
    uint32_t       lun_bmap_size;
    uint32_t       total_aus;
    uint64_t       free_aun_cnt;

    /* indices to be used for elevator lookup of free map */
    uint32_t       free_low_idx;
    uint32_t       free_curr_idx;
    uint32_t       free_high_idx;

    unsigned char *aun_clone_map;
} lun_info_t;

// use only LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO & LOG_DEBUG
#define BA_TRACE_0(lvl, fmt) \
    if (trc_lvl > lvl) { syslog(lvl, fmt); }

#define BA_TRACE_1(lvl, fmt, A) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A); }

#define BA_TRACE_2(lvl, fmt, A, B) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B); }

#define BA_TRACE_3(lvl, fmt, A, B, C) \
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B, C); }

#define BA_TRACE_4(lvl, fmt, A, B, C, D)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B, C, D); }

#define BA_TRACE_5(lvl, fmt, A, B, C, D, E)\
    if (trc_lvl > lvl) { syslog(lvl, fmt, A, B, C, D, E); }

#endif /* ifndef _BLOCK_ALLOC_INTERNAL_H */
