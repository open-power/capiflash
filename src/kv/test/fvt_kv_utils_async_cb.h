/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/fvt_kv_utils_async_cb.h $                         */
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
 *   functions to aid in testing the async kv ark functions
 * \details
 * \ingroup
 ******************************************************************************/
#ifndef KV_ASYNC_CB_UTILS_H
#define KV_ASYNC_CB_UTILS_H

#define ASYNC_SINGLE_CONTEXT    0
#define ASYNC_SINGLE_JOB        0

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt(uint32_t ctxt, uint32_t secs);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt_starve(uint32_t ctxt,
                              uint32_t nasync,
                              uint32_t basync,
                              uint32_t secs);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_SGD(uint32_t ctxt,
                           uint32_t job,
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t len);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_REP(uint32_t ctxt,
                           uint32_t job,
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t len);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_easy(uint32_t ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_low_stress(uint32_t ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_high_stress(uint32_t ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_LARGE_BLOCKS(uint32_t ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_job_BIG_BLOCKS(uint32_t ctxt);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ctxt_io(uint32_t num_ctxt,
                           uint32_t jobs,
                           uint32_t klen,
                           uint32_t vlen,
                           uint32_t LEN,
                           uint32_t secs);
/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ark_io(uint32_t num_ctxt,
                          uint32_t jobs,
                          uint32_t vlen,
                          uint32_t secs);
/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_init_ark_io_inject(uint32_t num_ctxt,
                                 uint32_t jobs,
                                 uint32_t vlen,
                                 uint32_t secs);
/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
uint32_t kv_async_init_perf_io(uint32_t num_ctxt,
                               uint32_t jobs,
                               uint32_t npool,
                               uint32_t klen,
                               uint32_t vlen,
                               uint32_t LEN,
                               uint32_t secs);
/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_run_jobs(void);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_start_jobs(void);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_wait_jobs(void);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void kv_async_job_perf(uint32_t jobs, uint32_t klen,uint32_t vlen,uint32_t len);

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ARK* kv_async_get_ark(uint32_t ctxt);

#endif
