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
#include <errno.h>
#include "am.h"
#include "ea.h"
#include "bl.h"
#include <arkdb_trace.h>

// create a chain 0..n-1
BL *bl_new(int64_t n, int w) {

  BL *bl = am_malloc(sizeof(BL));
  if (bl == NULL)
  {
      errno = ENOMEM;
      KV_TRC_FFDC(pAT, "n %ld w %d, rc = %d", n, w, errno);
      goto exception;
  }

    IV *iv = bl->list = iv_new(n,w);
    if (NULL == bl->list)
    {
        KV_TRC_FFDC(pAT, "n %ld w %d, iv_new() failed", n, w);
        am_free(bl);
        bl = NULL;
        goto exception;
    }

    uint64_t i;
    iv_set(iv,0,0);
    for (i=1; i<n-1; i++) iv_set(iv,i,i+1);
    iv_set(iv,n-1,0);
    bl->n = n;
    bl->count = n-1;
    bl->head = 1;
    bl->hold = -1;
    bl->w = w;
    pthread_rwlock_init(&(bl->iv_rwlock), NULL);

    KV_TRC(pAT, "bl %p iv %p n %ld w %d", bl, iv, n, w);

exception:
  return bl;
}

void bl_adjust(BL *bl, uint64_t blks)
{
#if 0
  uint64_t i;
  for (i = 1; i <= blks; i++)
  {
    iv_set(bl->list, i, 0);
  }
#endif
  bl->head = blks + 1;
  bl->count -= blks;
}

void bl_delete(BL *bl) {
    KV_TRC(pAT, "bl %p", bl);
  iv_delete(bl->list);
  pthread_rwlock_destroy(&(bl->iv_rwlock));
  am_free(bl);
}

BL *bl_resize(BL *bl, int64_t n, int w) {
  int64_t i;
  if (w!=bl->w) {
      KV_TRC_FFDC(pAT, "Sizes do not match bl %p n %ld w %d", bl, n, w);
      return NULL;
  }

  int64_t delta = n - bl->n;
  if (delta == 0)
  {
      KV_TRC_FFDC(pAT, "No size difference bl %p n %ld w %d", bl, n, w);
      return bl;
  }

  pthread_rwlock_wrlock(&(bl->iv_rwlock));

  bl->list = iv_resize(bl->list, n, w);
  if (bl->list) {
    for(i=bl->n; i<n-1; i++) {
      iv_set(bl->list,i,i+1);
    }
    iv_set(bl->list,n-1, bl->head);
    bl->head = bl->n;
    bl->n = n;
    bl->count += delta;
  } else {
    KV_TRC_FFDC(pAT, "bl %p n %ld w %d", bl, n, w);
    bl = NULL;
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));

  KV_TRC(pAT, "bl %p", bl);
  return bl;
}

int64_t bl_left(BL *bl) {
  return bl->count;
}

// take returns a root to a chain of n blocks
int64_t bl_take(BL *bl, int64_t n) {
  if (n > bl->count) {
      KV_TRC_FFDC(pAT, "Not enough free blocks bl %p n %ld bl->count %ld", bl, n, bl->count);
      return -1;
  }
  if (n==0) {
      KV_TRC_FFDC(pAT, "Zero block request bl %p n %ld", bl, n);
      return 0;
  }

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  int64_t hd = bl->head;
  int64_t tl = bl->head;
  int64_t m = n - 1;
  while (m>0) {
    tl = iv_get(bl->list,tl);
    m--;
  }
  bl->head= iv_get(bl->list,tl);
  iv_set(bl->list,tl,0);
  bl->count -= n;

  pthread_rwlock_unlock(&(bl->iv_rwlock));

  KV_TRC_DBG(pAT, "bl %p n %ld bl->count %ld bl->head %ld tl %ld m %ld",
          bl, n, bl->count, bl->head, tl, m);
  return hd;
}

int64_t bl_end(BL *bl, int64_t b) {
  int64_t i = b;
  int64_t ret = 0;

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (i>=0) {
    while ((ret = iv_get(bl->list,i)) > 0) i = ret;
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return i;
}

// return a chain from a root
int64_t bl_drop(BL *bl, int64_t b) {
  int64_t i = bl_end(bl,b);
  int64_t n = bl_len(bl,b);

  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  if (bl->hold==-1) {
    iv_set(bl->list,i,bl->head);
    //bl->chain[i] = bl->head;
    bl->head = b;
    bl->count += n;
  } else if (bl->hold == 0) {
    bl->hold = b;
  } else {
    iv_set(bl->list, i, bl->hold);
    //bl->chain[i] = bl->hold;
    bl->hold = b;
  }
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  KV_TRC_DBG(pAT, "bl %p b %ld bl->count %ld bl->hold %ld bl->head %ld",
          bl, b, bl->count, bl->hold, bl->head);
  return n; 
}

// cause drops to be held until released to free pool
void bl_hold (BL *bl) {
  if (bl->hold == -1) bl->hold = 0;
}

// release any held blocks to the free list
void bl_release(BL *bl) {
  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  if (0<bl->hold) {
    int64_t i = bl_end(bl,bl->hold);
    int64_t n = bl_len(bl,bl->hold);
    
    if (n>0) {
      iv_set(bl->list, i, bl->head);
      bl->head = bl->hold;
      bl->count += n;
    }
  }
  bl->hold = -1;
  pthread_rwlock_unlock(&(bl->iv_rwlock));
}


// the length of a chain
int64_t bl_len(BL *bl, int64_t b) {
  int64_t n = 0;
  int64_t i = b;
  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  while (0 < i) {
    n++;
    i = iv_get(bl->list,i);
  }
  pthread_rwlock_unlock(&(bl->iv_rwlock));
  return n;
}
// the next block in a chain
int64_t bl_next(BL *bl, int64_t b) { 
  uint64_t blk;

  pthread_rwlock_rdlock(&(bl->iv_rwlock));
  blk = iv_get(bl->list, b);  
  pthread_rwlock_unlock(&(bl->iv_rwlock));

  return blk;
}

ark_io_list_t *bl_chain(BL *bl, int64_t b, int64_t len)
{
  ark_io_list_t *bl_array = NULL;
  int             i = 0;

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (bl != NULL)
  {
    bl_array = (ark_io_list_t *)am_malloc(sizeof(ark_io_list_t) * len);
    if (bl_array != NULL)
    {
      while (0 < b) {
        bl_array[i].blkno = b;
        bl_array[i].a_tag = -1;
        b = iv_get(bl->list, b);
	i++;
      }
    }
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));

  return bl_array;
}

ark_io_list_t *bl_chain_blocks(BL *bl, int64_t start, int64_t len)
{
  ark_io_list_t *bl_array = NULL;
  int             i = 0;

  pthread_rwlock_rdlock(&(bl->iv_rwlock));

  if (bl != NULL)
  {
    bl_array = (ark_io_list_t *)am_malloc(sizeof(ark_io_list_t) * len);
    if (bl_array != NULL)
    {
      for (i = 0; i < len; i++)
      {
        bl_array[i].blkno = start + i;
        bl_array[i].a_tag = -1;
      }
    }
  }

  pthread_rwlock_unlock(&(bl->iv_rwlock));

  return bl_array;
}

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
      bl_array[i].a_tag = -1;
    }
  }

  return bl_array;
}

/* int64_t bl_cnt(BL *bl, int64_t b, int64_t i) { */
/*   int64_t cnt = 0; */
/*   while (b >= 0) { */
/*     if (b==i) cnt++; */
/*     b = iv_get(bl->list, b); */
/*     //    b = bl->chain[b]; */
/*   } */
/*   return cnt; */
/* } */
  
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
    if (0<iv_get(bl->list,b)) fprintf(F,"   b%"PRIi64"_%d -> b%"PRIi64"_%d;\n",
				      b, bcnt[b], iv_get(bl->list,b), bcnt[iv_get(bl->list,b)]);
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
