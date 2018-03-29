/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/pbuf.h $                                          */
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

#ifndef __PBUF_H__
#define __PBUF_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include <dbgtrc.h>

/*******************************************************************************
 * Pointer Buffers
 *  provides a method to gain exclusive access to buffers in a pool of buffers
 *    that are the same size
 *  pbuf_new - mallocs space for n buffers aligned at 4k
 *  pbuf_get - gives a tag and a buffer addr; this buffer is now reserved
 *  pbuf_put - frees the buffer for the corresponding tag
 ******************************************************************************/
typedef struct pbuf_s
{
  pthread_mutex_t l;
  int    n;
  int    c;
  void  *o;
  void **p;
} pbuf_t;

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ pbuf_t* pbuf_new(int n, int buf_size)
{
  char   *p     = NULL;
  int     align = 4096;
  int     i     = 0;
  int     rc    = 0;
  int     size  = sizeof(pbuf_t);
  pbuf_t *pbuf  = (pbuf_t*)malloc(size);

  if (!pbuf) {goto done;}
  memset(pbuf,0,size);

  size = n * buf_size;
  if ((rc=posix_memalign((void**)&pbuf->o, align, size)))
  {
      DBGTRC("posix_memalign failed, size=%d, rc=%d\n", size, rc);
      free(pbuf);
      pbuf = NULL;
      goto done;
  }
  memset(pbuf->o,0,size);

  size    = n * sizeof(void*);
  pbuf->p = malloc(size);
  if (!pbuf->p)
  {
      DBGTRC("malloc failed, size=%d, errno=%d\n", size, errno);
      free(pbuf);
      free(pbuf->o);
      pbuf = NULL;
      goto done;
  }

  /* fill the buffer list */
  p = (char*)pbuf->o;
  for (i=0; i<n; i++) {pbuf->p[i] = p+(i*buf_size);}

  pthread_mutex_init(&pbuf->l, NULL);
  pbuf->c = pbuf->n = n;

done:
  DBGTRC("pbuf:%p pbuf->o:%p n:%d bs:%d", pbuf,pbuf->o,n,buf_size);
  return pbuf;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void pbuf_free(pbuf_t *pbuf)
{
  if (!pbuf) {goto done;}

  pthread_mutex_lock(&pbuf->l);

  DBGTRC("pbuf:%p", pbuf);
  if (pbuf->o) {free(pbuf->o);}
  if (pbuf->p) {free(pbuf->p);}

  pthread_mutex_unlock(&pbuf->l);
  pthread_mutex_destroy(&pbuf->l);
  free(pbuf);

done:
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void* pbuf_get(pbuf_t *pbuf)
{
    void *buf = NULL;

    if (!pbuf || !pbuf->c) {goto done;}

    buf = pbuf->p[--(pbuf->c)];

done:
    DBGTRC("pbuf:%p pbuf->o:%p buf:%p", pbuf,pbuf->o,buf);
    return buf;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int pbuf_put(pbuf_t *pbuf, void *p)
{
    int rc = 0;

    if (!pbuf || pbuf->c==pbuf->n) {rc=-1; goto done;}

    pbuf->p[pbuf->c++] = p;

done:
    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ void* pbuf_getl(pbuf_t *pbuf)
{
    void *buf = NULL;

    if (!pbuf) {goto done;}

    pthread_mutex_lock(&pbuf->l);
    buf = pbuf_get(pbuf);
    pthread_mutex_unlock(&pbuf->l);

done:
    return buf;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static __inline__ int pbuf_putl(pbuf_t *pbuf, void *p)
{
    int rc = -1;

    if (!pbuf) {goto done;}

    pthread_mutex_lock(&pbuf->l);
    rc = pbuf_put(pbuf,p);
    pthread_mutex_unlock(&pbuf->l);

done:
    return rc;
}

#endif
