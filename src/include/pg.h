/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/pg.h $                                            */
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

#ifndef __PG_H__
#define __PG_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <idx.h>
#include <string.h>

#define DBGTRC_OFF
#include <dbgtrc.h>

/*******************************************************************************
 * Pointer Group
 *  add a sizeof(void*) value to the store and receive an index
 *  get the value using the index
 *  short get time, no locks for get, small mem footprint for very large lists
 ******************************************************************************/
typedef struct pg_s
{
  pthread_mutex_t l;
  int     x;
  int     y;
  int     max;
  idx_t **idx;
  void   *p[];
} pg_t;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void pg_dump(pg_t *pg)
{
    void **p   = NULL;
    int    x   = 0;
    int    i   = 0;

    for (i=0; i<pg->max; i++)
    {
        x = i / pg->y;
        p = (void**)pg->p[x];
        if (p) {DBGTRC("pg[%d[%d]=%p", x, i%pg->y, p[i%pg->y]);}
        else   {DBGTRC("p[%d]=NULL",i); i+=pg->y;}
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ pg_t* pg_new(int x, int y)
{
  int   size = sizeof(pg_t) + (x * sizeof(void*));
  pg_t *pg   = (pg_t*)malloc(size);

  if (!pg) {return pg;}
  memset(pg,0,size);

  pthread_mutex_init(&pg->l, NULL);
  pg->x   = x;
  pg->y   = y;
  pg->max = pg->x*pg->y;

  size    = x*sizeof(void*);
  pg->idx = (idx_t**)malloc(size);
  if (!pg->idx) {free(pg); pg=NULL;}
  else          {memset(pg->idx,0,size);}

  DBGTRC("pg:%p x:%d y:%d max:%d", pg,x,y,pg->max);
  return pg;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void pg_free(pg_t *pg)
{
  int i = 0;

  if (!pg) {goto done;}

  pthread_mutex_lock(&pg->l);

  for (i=0; i<pg->x; i++)
  {
      if (pg->p[i])
      {
          DBGTRC("pg->p[%d]:%p pg->idx[%d]:%p", i, pg->p[i], i, pg->idx[i]);
          free(pg->p[i]);
          if (pg->idx[i])
          {
              idx_free(pg->idx[i]);
          }
      }
  }

  pthread_mutex_unlock(&pg->l);
  pthread_mutex_destroy(&pg->l);
  free(pg);

done:
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int pg_add(pg_t *pg, void *v)
{
  int     x   = 0;
  int     y   = 0;
  int     rc  = EAGAIN;
  int     ret = -1;

  if (!pg) {return ret;}

  pthread_mutex_lock(&pg->l);

  /* search for an existing idx with a slot */
  for (x=0; x<pg->x && pg->idx[x] && pg->p[x]; x++)
  {
      if (EAGAIN != (rc=idx_get(pg->idx[x], &y)))
      {
          void **p = (void**)pg->p[x];
          /* found available idx, add v */
          p[y] = v;
          ret  = (x*pg->y)+y;
          DBGTRC("ret:%3d exist pg:%p pg->p[%d][%d]=%p",
                 ret,pg,x,y,v);
          break;
      }
  }

  /* all existing idx are full, but there might be room to malloc another */
  if (rc == EAGAIN)
  {
      /* find first available slot, and add another group */
      for (x=0; x<pg->x; x++)
      {
          if (!pg->p[x])
          {
              int size = pg->y*sizeof(void*);
              pg->p[x] = malloc(size);
              memset(pg->p[x],0,size);
              if (!pg->p[x]) {goto error;}
              pg->idx[x] = idx_new(pg->y);
              if (!pg->idx[x]) {free(pg->p[x]); pg->p[x]=NULL; goto error;}
              idx_get(pg->idx[x], &y);
              void **p = (void**)pg->p[x];
              p[y] = v;
              ret  = (x*pg->y)+y;
              DBGTRC("ret:%3d new   pg:%p pg->p[%d][%d]=%p",
                     ret,pg,x,y,v);
              break;
          }
      }
  }

error:
  pthread_mutex_unlock(&pg->l);

  return ret;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int pg_del(pg_t *pg, int n)
{
  void **p  = NULL;
  int    x  = 0;
  int    y  = 0;
  int    rc = -1;

  if (!pg || n<0 || n>=pg->max) {goto done;}

  x = n / pg->y;
  y = n % pg->y;
  if (x>=pg->x || y>=pg->y) {goto done;}

  pthread_mutex_lock(&pg->l);

  p    = (void**)pg->p[x];
  p[y] = NULL;
  if (pg->idx[x])
  {
      idx_put(pg->idx[x],y);
      if (IDX_FULL(pg->idx[x]))
      {
          DBGTRC("free          pg:%p pg->p[%d]:%p",pg,x,pg->p[x]);
          idx_free(pg->idx[x]);
          if (pg->p[x]) {free(pg->p[x]);}
          pg->idx[x] = NULL;
          pg->p[x]   = NULL;
      }
  }
  pthread_mutex_unlock(&pg->l);
  rc=0;

done:
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void* pg_get(pg_t *pg, int n)
{
  int    x   = 0;
  int    y   = 0;
  void  *ret = NULL;
  void **p   = NULL;

  if (!pg || n <0 || n>=pg->max) {goto done;}

  x = n / pg->y;
  y = n % pg->y;
  if (x>=pg->x || y>=pg->y || !pg->p[x]) {goto done;}
  p   = (void**)pg->p[x];
  ret = p[y];

done:
  return ret;
}

/**
 *******************************************************************************
 * \brief
 * find the next non-null ptr, starting with *n, return *n = found index + 1
 ******************************************************************************/
static __inline__ void* pg_getnext(pg_t *pg, int *n)
{
  pg_t  *ret = NULL;
  void **p   = NULL;
  int    x   = 0;
  int    y   = 0;

  if (!pg || !n || *n <0 || *n>=pg->max) {goto done;}

  do
  {
      x = *n / pg->y;
      if (x >= pg->x) {break;}
      p = (void**)pg->p[x];
      if (!p) {*n=(x+1)*pg->y; continue;} /* skip to next pg */
      y = *n % pg->y;
      if (y >= pg->y) {continue;}
      if ((ret=(pg_t*)p[y]))
      {
          *n=(x*pg->y)+y+1;
          DBGTRC("found pg:%p x[%d]y[%d]=%p *n:%d ", pg,x,y,ret,*n);
          break;
      }
      *n += 1;
  } while (*n<pg->max);

done:
  return ret;
}

#endif
