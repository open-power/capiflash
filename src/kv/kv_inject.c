/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/kv_inject.c $                                          */
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
 *   code to aid in KV testing
 * \ingroup
 ******************************************************************************/

#include <kv_inject.h>
#include <arkdb_trace.h>
#include <errno.h>

/**
 *******************************************************************************
 * \brief
 *  inject IO error if global inject variable is set
 ******************************************************************************/
int check_alloc_error_injects(void)
{
    int rc=0;

    if (KV_ALLOC_ERROR_INJECT)
    {
        KV_CLEAR_ALLOC_ERROR;
        KV_TRC_FFDC(pAT, "ALLOC_ERROR_INJECT");
        rc    = 1;
        errno = ENOMEM;
    }
    return rc;
}

/**
 *******************************************************************************
 * \brief
 *  inject IO error if global inject variable is set
 ******************************************************************************/
int check_sched_error_injects(int op)
{
    int rc=0;

    if (op == 0 && KV_SCHD_READ_ERROR_INJECT)
    {
        KV_CLEAR_SCHD_READ_ERROR;
        KV_TRC_FFDC(pAT, "IO_INJ: SCHD_READ_ERROR_INJECT");
        errno = EIO;
        rc    = 1;
    }
    else if (op == 1 && KV_SCHD_WRITE_ERROR_INJECT)
    {
        KV_CLEAR_SCHD_WRITE_ERROR;
        KV_TRC_FFDC(pAT, "IO_INJ: SCHD_WRITE_ERROR_INJECT");
        errno = EIO;
        rc    = 1;
    }
    return rc;
}

/**
 *******************************************************************************
 * \brief
 *  inject IO error if global inject variable is set
 ******************************************************************************/
int check_harv_error_injects(int op)
{
    int rc=0;

    if (op == 0 && KV_HARV_READ_ERROR_INJECT)
    {
        KV_CLEAR_HARV_READ_ERROR;
        KV_TRC_FFDC(pAT, "IO_INJ: HARV_READ_ERROR_INJECT");
        errno = EIO;
        rc    = 1;
    }
    else if (op == 1 && KV_HARV_WRITE_ERROR_INJECT)
    {
        KV_CLEAR_HARV_WRITE_ERROR;
        KV_TRC_FFDC(pAT, "IO_INJ: HARV_WRITE_ERROR_INJECT");
        errno = EIO;
        rc    = 1;
    }
    return rc;
}

