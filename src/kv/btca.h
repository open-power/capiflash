/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/btca.h  $                                              */
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

#ifndef __BTCA_H__
#define __BTCA_H__

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>

#include <errno.h>

typedef struct
{
    uint64_t n;
    void    *e[];
} btca_t;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline btca_t *btca_new(uint64_t n)
{
    uint64_t size = sizeof(btca_t) + (sizeof(void*) * n);
    btca_t  *btca = (btca_t*)am_malloc(size);

    if (!btca)
    {
        KV_TRC_FFDC(pAT, "n:%ld size:%ld ENOMEM", n,size);
        errno = ENOMEM;
        return NULL;
    }

    bzero(btca, size);
    btca->n = n;
    KV_TRC_DBG(pAT, "%p n:%ld size:%ld", btca, n, size);
    return btca;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline void btca_free(btca_t *btca)
{
  KV_TRC_DBG(pAT, "%p", btca);
  if (btca) {am_free(btca);}
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline int btca_set(btca_t *btca, uint64_t pos, BT *bt)
{
    if (!btca || !bt)
    {
        KV_TRC_FFDC(pAT,"%p %p",btca,bt);
        return -1;
    }
    KV_TRC_DBG(pAT,"%p cpy(%p,%p,%ld",btca,btca->e[pos],bt,bt->len);
    btca->e[pos] = am_realloc(btca->e[pos], bt->len);
    memcpy(btca->e[pos],bt,bt->len);
    return 0;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline int btca_get(btca_t *btca, uint64_t pos, BT *bt)
{
    if (!btca || !btca->e[pos] || !bt)
    {
        KV_TRC_FFDC(pAT,"%p %p",btca,bt);
        return -1;
    }
    BT *e = (BT*)btca->e[pos];
    KV_TRC_DBG(pAT,"%p cpy(%p,%p,%ld",btca,bt,e,e->len);
    memcpy(bt,e,e->len);
    return 0;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline int btca_free_e(btca_t *btca, uint64_t pos)
{
    if (!btca)
    {
        KV_TRC_FFDC(pAT,"%p",btca);
        return -1;
    }
    BT **e = (BT**)&btca->e[pos];
    KV_TRC_DBG(pAT,"%p free:%p pos:%ld",btca,*e,pos);
    am_free(*e);
    *e=NULL;
    return 0;
}

#endif
