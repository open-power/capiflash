/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_ptrc_ids.h $                               */
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
 *   definitions for performance trace ids
 * \ingroup
 ******************************************************************************/
#ifndef _CFLASH_PTRC_IDS_H
#define _CFLASH_PTRC_IDS_H

/* these are defined this way in order to create strings for printing */
#define PTRC_ENUMS \
\
    X(PTRC_FIRST, "PTRC_FIRST"), \
\
    /* debug */ \
    X(CHECKPOINT1, "CHECKPOINT1"), \
    X(CHECKPOINT2, "CHECKPOINT2"), \
    X(CHECKPOINT3, "CHECKPOINT3"), \
    X(CHECKPOINT4, "CHECKPOINT4"), \
    X(CHECKPOINT5, "CHECKPOINT5"), \
    X(CHECKPOINT6, "CHECKPOINT6"), \
    X(CHECKPOINT7, "CHECKPOINT7"), \
    X(CHECKPOINT8, "CHECKPOINT8"), \
    X(CHECKPOINT9, "CHECKPOINT9"), \
\
    /* cblk */ \
    X(CBLK_BUILD,               "CBLK_BUILD"), \
    X(CBLK_ISSUE,               "CBLK_ISSUE"), \
    X(CBLK_ISSUE_W4ROOM,        "CBLK_ISSUE_W4ROOM"), \
    X(CBLK_ISSUE_SISL_END,      "CBLK_ISSUE_SISL_END"), \
    X(CBLK_CFLASH_SKIP_POLL,    "CBLK_CFLASH_SKIP_POLL"), \
    X(CBLK_CFLASH_POLL,         "CBLK_CFLASH_POLL"), \
    X(CBLK_W4CMP_PATH,          "CBLK_W4CMP_PATH"), \
    X(CBLK_POLL1,               "CBLK_POLL1"), \
    X(CBLK_POLLN,               "CBLK_POLLN"), \
    X(CBLK_CMD_TIMEDOUT,        "CBLK_CMD_TIMEDOUT"), \
    X(CBLK_AFU_FAILED_CMD,      "CBLK_AFU_FAILED_CMD"), \
    X(CBLK_CTXT_RESET,          "CBLK_CTXT_RESET"), \
    X(CBLK_CMP_CMD,             "CBLK_CMP_CMD"), \
    X(CBLK_ARESULT_CMP,         "CBLK_ARESULT_CMP"), \
    X(CBLK_LAT,                 "CBLK_LAT"), \
\
    /* app */ \
    X(APP_READ,         "APP_READ"), \
    X(APP_WRITE,        "APP_WRITE"), \
    X(APP_AREAD,        "APP_AREAD"), \
    X(APP_AWRITE,       "APP_AWRITE"), \
    X(APP_POLL,         "APP_POLL"), \
    X(APP_CMD_CMP,      "APP_CMD_CMP"), \
\
    X(PTRC_LAST, "PTRC_LAST")

enum ptrc_enum
{
#define X(_e, _s) _e
    PTRC_ENUMS
#undef X
};

#endif
