/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bl.c $                                                 */
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
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "am.h"
#include "ea.h"
#include "bl.h"
#include <arkdb_trace.h>
#include <errno.h>

/**
 *******************************************************************************
 * \brief
 *   make BL_INITN more blocks available for a take
 *
 * \retval >0 if the list init is complete
 * \retval  0 if there are more list elements which can be initialized
 ******************************************************************************/
int bl_init_chain_link(BL *bl)
{
    int      rc        = 1;
    uint64_t i         = 0;
    uint64_t availN    = 0;
    uint64_t chain_end = 0;
    uint64_t chainN    = 0;

    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        return rc;
    }

    availN = bl->n - bl->top -1;
    if (availN <= 0)
    {
        KV_TRC_FFDC(pAT, "no available blocks to chain n:%ld top:%ld",
                bl->n, bl->top);
        return rc;
    }

    pthread_rwlock_wrlock(&(bl->iv_rwlock));

    chainN    = (availN < BL_INITN) ? availN : BL_INITN;
    chain_end =  bl->top + chainN;

    // create a chain
    for (i=bl->top; i<chain_end; i++)
    {
        if (iv_set(bl->list, i, i+1) < 0)
        {
            KV_TRC_FFDC(pAT, "invalid index:%ld", i);
            goto exception;
        }
    }

    bl->count += chainN;
    bl->top    = i;

    rc = (bl->top == bl->n);

    KV_TRC(pAT, "CHAIN_NEW n:%ld top:%ld count:%ld hd:%ld availN:%ld "
                "chainN:%ld chain_exception:%ld rc:%d",
            bl->n, bl->top, bl->count, bl->head, availN, chainN, chain_end, rc);
exception:
    pthread_rwlock_unlock(&(bl->iv_rwlock));
    return rc;
}

/**
 *******************************************************************************
 * \brief
 *   create a chain 0..n-1
 ******************************************************************************/
BL *bl_new(int64_t n, int w)
{
    BL *bl = am_malloc(sizeof(BL));

    if (!bl)
    {
        errno = ENOMEM;
        KV_TRC_FFDC(pAT, "n %ld w %d, rc = %d", n, w, errno);
        goto exception;
    }

    bl->list = iv_new(n,w);
    if (NULL == bl->list)
    {
        KV_TRC_FFDC(pAT, "n %ld w %d, iv_new() failed", n, w);
        am_free(bl);
        bl = NULL;
        goto exception;
    }

    iv_set(bl->list,0,0); //reserve 0
    bl->n     = n;
    bl->top   = 1;
    bl->count = 0;
    bl->head  = 1;
    bl->hold  = -1;
    bl->w     = w;
    pthread_rwlock_init(&(bl->iv_rwlock), NULL);

    KV_TRC(pAT, "NEW bl:%p list:%p n:%ld count:%ld w:%ld",
           bl, bl->list, bl->n, bl->count, bl->w);

exception:
    return bl;
}

/**
 *******************************************************************************
 * \brief
 *   reserve the first n
 ******************************************************************************/
int bl_reserve(BL *bl, uint64_t resN)
{
    int i  = 0;
    int rc = -1;

    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        goto exception;
    }
    if (resN >= bl->n)
    {
        KV_TRC_FFDC(pAT, "not enough blocks available bl:%p resN:%ld", bl,resN);
        goto exception;
    }

    for (i=1; i<=resN; i++)
    {
        if (iv_set(bl->list,i,i) < 0)
        {
            KV_TRC_FFDC(pAT, "invalid index bl:%p resN:%ld i:%d", bl, resN, i);
            goto exception;
        }
    }

    bl->head  = resN + 1;
    bl->top   = resN + 1;
    bl->count = 0;
    rc        = 0;

    KV_TRC(pAT, "CHAIN_RESERVE bl:%p resN:%ld count:%ld hd:%ld",
               bl, resN, bl->count, bl->head);
exception:
    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bl_delete(BL *bl)
{
  KV_TRC(pAT, "DELETE bl:%p", bl);
  if (!bl) return;
  iv_delete(bl->list);
  pthread_rwlock_destroy(&(bl->iv_rwlock));
  am_free(bl);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
BL *bl_resize(BL *bl, int64_t n, int w)
{
  int64_t b4 = bl->n;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return NULL;
  }
  if (w != bl->w)
  {
      KV_TRC_FFDC(pAT, "blkbits do not match bl:%p bl->w:%ld w:%d",
                  bl, bl->w, w);
      return NULL;
  }
  if (n - bl->n == 0)
  {
      KV_TRC_FFDC(pAT, "No size difference bl %p n %ld w %d", bl, n, w);
      return bl;
  }

  pthread_rwlock_wrlock(&(bl->iv_rwlock));

  bl->list = iv_resize(bl->list, n, w);
  if (bl->list == NULL)
  {
    KV_TRC_FFDC(pAT, "bl %p n %ld w %d", bl, n, w);
    bl = NULL;
    goto exception;
  }
  bl->n = n;

  KV_TRC(pAT, "RESIZE bl:%p b4:%ld n:%ld count:%ld",
         bl, b4, bl->n, bl->count);

exception:
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return bl;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bl_left(BL *bl)
{
    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        return -1;
    }
    return bl->count;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int64_t bl_end(BL *bl, int64_t b)
{
    int64_t i = b;
    int64_t r = i;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return -1;
  }
  if (b <= 0 || b >= bl->n)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return -1;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (i >= 0)
  {
    while ((r=iv_get(bl->list,i)) > 0) {i = r;}
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));
  if (r == -1)
  {
      KV_TRC_FFDC(pAT, "invalid index bl:%p b:%ld i:%ld", bl, b, i);
  }
  return i;
}

/**
 *******************************************************************************
 * \brief
 *   if we need more blocks than are initialized AND blocks are available
 *     call function to initialize more blocks
 ******************************************************************************/
void bl_check_take(BL *bl, int64_t n)
{
    int64_t avail = 0;

    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        return;
    }

    avail = bl->n - bl->top -1;

    // if we need blocks and we have uninitialized blocks, init them
    if (n > bl->count && avail)
    {
        KV_TRC(pAT, "CHAIN_NEEDED bl:%p n:%ld bl->n:%ld top:%ld count:%ld "
                    "hd:%ld ",
                    bl, n, bl->n, bl->top, bl->count, bl->head);

        while (!bl_init_chain_link(bl))
        {
            if (bl->count >= n) {break;}
        }
    }
}

/**
 *******************************************************************************
 * \brief
 *   allocates a chain of n blocks and returns the index of the chain head
 ******************************************************************************/
int64_t bl_take(BL *bl, int64_t n)
{
    int64_t hd = 0;
    int64_t tl = 0;
    int64_t m  = 0;

    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        return -1;
    }

    bl_check_take(bl, n);

    if (n > bl->count)
    {
        KV_TRC_FFDC(pAT, "Not enough free blocks bl %p n %ld bl->count %ld",
                    bl, n, bl->count);
        return -1;
    }
    if (n == 0)
    {
        KV_TRC_FFDC(pAT, "Zero block request bl %p n %ld", bl, n);
        return 0;
    }

    pthread_rwlock_rdlock(&(bl->iv_rwlock));

    hd = bl->head;
    tl = bl->head;
    m  = n - 1;
    while (m > 0)
    {
      if ((tl=iv_get(bl->list,tl)) < 0)
      {
          KV_TRC_FFDC(pAT, "invalid index bl:%p n:%ld tl:%ld", bl, n, tl);
          hd = -1;
          goto exception;
      }
      m--;
    }
    if ((bl->head=iv_get(bl->list,tl)) < 0)
    {
        KV_TRC_FFDC(pAT, "invalid index bl:%p n:%ld tl:%ld", bl, n, tl);
        hd = -1;
        goto exception;
    }
    if (iv_set(bl->list,tl,0) < 0)
    {
        KV_TRC_FFDC(pAT, "invalid index bl:%p n:%ld tl:%ld", bl, n, tl);
        hd = -1;
        goto exception;
    }
    bl->count -= n;


    KV_TRC_DBG(pAT, "CHAIN_TAKE bl:%p n:%ld ret_hd:%ld count:%ld top:%ld "
                     "hd:%ld",
                     bl, n, hd, bl->count, bl->top, bl->head);
exception:
    pthread_rwlock_unlock(&(bl->iv_rwlock));
    return hd;
}

/**
 *******************************************************************************
 * \brief
 *   return a chain from a root
 ******************************************************************************/
int64_t bl_drop(BL *bl, int64_t b)
{
  int64_t i = bl_end(bl,b);
  int64_t n = bl_len(bl,b);

  if (!bl || i == -1 || n == -1)
  {
      KV_TRC_FFDC(pAT, "invalid parm bl:%p b:%ld i:%ld n:%ld", bl, b, i, n);
      return -1;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (bl->hold == -1)
  {
    if (iv_set(bl->list,i,bl->head) < 0)
    {
        n = -1;
        KV_TRC_FFDC(pAT, "invalid index bl:%p b:%ld i:%ld n:%ld hd:%ld",
                    bl, b, i, n, bl->head);
        goto exception;
    }
    bl->head = b;
    bl->count += n;
  }
  else if (bl->hold == 0)
  {
    bl->hold = b;
  }
  else
  {
    if (iv_set(bl->list, i, bl->hold) < 0)
    {
        n = -1;
        KV_TRC_FFDC(pAT, "invalid index bl:%p b:%ld i:%ld n:%ld hd:%ld",
                    bl, b, i, n, bl->hold);
        goto exception;
    }
    bl->hold = b;
  }

  KV_TRC_DBG(pAT, "CHAIN_DROP bl:%p b:%ld tl_b:%ld len_b:%ld "
                  "cnt:%ld hold:%ld top:%ld hd:%ld",
                  bl, b, i, n, bl->count, bl->hold, bl->top, bl->head);
exception:
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return n;
}

/**
 *******************************************************************************
 * \brief
 *   cause drops to be held until released to free pool
 ******************************************************************************/
void bl_hold (BL *bl)
{
    if (!bl)
    {
        KV_TRC_FFDC(pAT, "NULL bl");
        return;
    }
    if (bl->hold == -1) bl->hold = 0;
}

/**
 *******************************************************************************
 * \brief
 *   release any held blocks to the free list
 ******************************************************************************/
void bl_release(BL *bl)
{
  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return;
  }

  pthread_rwlock_wrlock(&(bl->iv_rwlock));
  if (bl->hold > 0)
  {
    int64_t i = bl_end(bl,bl->hold);
    int64_t n = bl_len(bl,bl->hold);

    if (i == -1 || n == -1)
    {
        KV_TRC_FFDC(pAT, "invalid data bl:%p i:%ld n:%ld", bl, i, n);
        goto exception;
    }

    if (n > 0)
    {
      if (iv_set(bl->list, i, bl->head) < 0)
      {
          KV_TRC_FFDC(pAT, "invalid idx bl:%p i:%ld n:%ld", bl, i, n);
          goto exception;
      }
      bl->head = bl->hold;
      bl->count += n;
    }
  }
  bl->hold = -1;

  KV_TRC_DBG(pAT, "CHAIN_RELEASE bl:%p cnt:%ld hold:%ld hd:%ld tl:%ld",
             bl, bl->count, bl->hold, bl->head, bl_len(bl,bl->head));

exception:
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return;
}

/**
 *******************************************************************************
 * \brief
 *   length of a chain
 ******************************************************************************/
int64_t bl_len(BL *bl, int64_t b)
{
  int64_t n = 0;
  int64_t i = b;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return 0;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  while (i > 0)
  {
    n++;
    i = iv_get(bl->list,i);
  }
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  if (i<0)
  {
      KV_TRC_FFDC(pAT, "invalid index:%ld", b);
      n=0;
  }
  return n;
}

/**
 *******************************************************************************
 * \brief
 *   return the next block in a chain
 ******************************************************************************/
int64_t bl_next(BL *bl, int64_t b)
{
  uint64_t blk;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return -1;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  blk = iv_get(bl->list, b);
  pthread_rwlock_unlock(&(bl->iv_rwlock));

  return blk;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ark_io_list_t *bl_chain(BL *bl, int64_t b, int64_t len)
{
  ark_io_list_t *bl_array = NULL;
  int            i        = 0;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      return NULL;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (bl != NULL)
  {
    bl_array = (ark_io_list_t *)am_malloc(sizeof(ark_io_list_t) * len);
    if (bl_array != NULL)
    {
      while (0 < b) {
        bl_array[i].blkno = b;
        bl_array[i].a_tag.tag = -1;
        if ((b=iv_get(bl->list, b)) < 0)
        {
            KV_TRC_FFDC(pAT, "invalid chain index:%ld", b);
            bl_array = NULL;
            goto exception;
        }
        i++;
      }
    }
  }

exception:
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return bl_array;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ark_io_list_t *bl_chain_blocks(BL *bl, int64_t start, int64_t len)
{
  ark_io_list_t *bl_array = NULL;
  int            i        = 0;

  if (!bl)
  {
      KV_TRC_FFDC(pAT, "NULL bl");
      goto exception;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (bl != NULL)
  {
    bl_array = (ark_io_list_t *)am_malloc(sizeof(ark_io_list_t) * len);
    if (bl_array != NULL)
    {
      for (i = 0; i < len; i++)
      {
        bl_array[i].blkno = start + i;
        bl_array[i].a_tag.tag = -1;
      }
    }
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));

exception:
  return bl_array;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ark_io_list_t *bl_chain_no_bl(int64_t start, int64_t len)
{
  ark_io_list_t *bl_array = NULL;
  int             i = 0;

  bl_array = (ark_io_list_t *)am_malloc(sizeof(ark_io_list_t) * len);
  if (bl_array != NULL)
  {
    for (i = 0; i < len; i++)
    {
      bl_array[i].blkno = start + i;
      bl_array[i].a_tag.tag = -1;
    }
  }

  return bl_array;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void bl_dot(BL *bl, int i, int *bcnt, int ccnt, int64_t *chains) {
  char f[256];
  sprintf(f,"bl%03d.dot", i);
  FILE *F = fopen(f,"w");
  
  fprintf(F,"digraph G {\n");
  fprintf(F,"  head [shape=Mdiamond];\n");
  fprintf(F,"  chain [shape=Mdiamond];\n");

  int64_t b = bl->head;
  if (0<=b) fprintf(F,"   head -> b%"PRIi64"_%d;\n", b, bcnt[b]);
  while (0<=b) {
    if (0<iv_get(bl->list,b))
    {
        fprintf(F,"   b%"PRIi64"_%d -> b%"PRIi64"_%d;\n",
                b, bcnt[b], iv_get(bl->list,b), bcnt[iv_get(bl->list,b)]);
    }
    b = iv_get(bl->list,b);
  }

  for(i=i; i<ccnt; i++) {
    int64_t b = chains[i];
    if (0<=b) fprintf(F,"  chain -> b%"PRIi64"_%d;\n", b, bcnt[b]);
    while (0<=b) {
      int64_t bn = iv_get(bl->list,b);
      if (0<=bn) fprintf(F,"  b%"PRIi64"_%d -> b%"PRIi64"_%d;\n", b, bcnt[b], bn, bcnt[bn]);
      b = bn;
    }
      
  }
  
  fprintf(F,"}\n");
  fclose(F);
}
