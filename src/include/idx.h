/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/idx.h $                                           */
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

#ifndef __IDX_H__
#define __IDX_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>

#include <dbgtrc.h>

typedef int IDX;

/*******************************************************************************
 * create a list of 0..n-1 indexes, pull off list, put back into list
 * ensures a fixed number of unique indexes
 ******************************************************************************/
typedef struct _idx
{
  pthread_mutex_t l;
  int n;
  int c;
  IDX d[];
} idx_t;

#define IDX_VALID(_p_idx, _idx) (_idx>=0 && _idx<_p_idx->n)
#define IDX_FULL(_p_idx) (_p_idx->c==_p_idx->n)

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ idx_t* idx_new(uint32_t n)
{
  idx_t *p_idx = (idx_t*)malloc(sizeof(idx_t) + (n * sizeof(IDX)));
  IDX    i     = 0;

  p_idx->c = p_idx->n = n;
  pthread_mutex_init(&p_idx->l, NULL);
  for (i=0; i<p_idx->n; i++) {p_idx->d[i] = n-i-1;}

  DBGTRC("p_idx:%p n:%d", p_idx,n);
  return p_idx;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void idx_free(idx_t *p_idx)
{
  pthread_mutex_destroy(&p_idx->l);
  if (p_idx) {free(p_idx);}
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int idx_get(idx_t *p_idx, IDX *idx)
{
  int ret = 0;

  if (p_idx->c==0) {ret = EAGAIN;}
  else             {*idx = p_idx->d[--(p_idx->c)];}

  DBGTRC("p_idx:%p idx:%p ret:%d", p_idx,idx,ret);
  return ret;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int idx_put(idx_t *p_idx, IDX idx)
{
  int ret = 0;

  if      (p_idx->c == p_idx->n)   {ret = ENOSPC;}
  else if (idx<0 || idx>=p_idx->n) {ret = EINVAL;}
  else                             {p_idx->d[p_idx->c++] = idx;}

  return ret;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int idx_getl(idx_t *p_idx, IDX *idx)
{
  int ret = 0;

  pthread_mutex_lock(&p_idx->l);
  ret = idx_get(p_idx,idx);
  pthread_mutex_unlock(&p_idx->l);

  return ret;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int idx_putl(idx_t *p_idx, IDX idx)
{
  int ret = 0;

  pthread_mutex_lock(&p_idx->l);
  ret = idx_put(p_idx,idx);
  pthread_mutex_unlock(&p_idx->l);

  return ret;
}

#endif
