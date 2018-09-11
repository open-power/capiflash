/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/am.c $                                                 */
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

#ifndef _AIX
#include "zmalloc.h"
#endif

#include <ark.h>

int x = 1;

void *am_malloc(size_t size)
{
    unsigned char *p=NULL;

    if (check_alloc_error_injects()) {return NULL;}

    if ((int64_t)size > 0)
    {
#ifdef _AIX
      p = malloc(size);
#else
      p = zmalloc(size);
#endif
      if (!p)
      {
          KV_TRC_FFDC(pAT, "ALLOC_FAIL errno=%d", errno);
          if (!errno) {errno = ENOMEM;}
      }
    }
    KV_TRC_DBG(pAT, "ALLOC   %p size:%ld", p, size);
    return p;
}

void am_free(void *ptr)
{
  if (!ptr) {return;}

  if ((uint64_t)ptr < 0xFFFF) {KV_TRC_FFDC(pAT, "FREE  BAD:%p", ptr); return;}
  else                        {KV_TRC_DBG (pAT, "FREE  %p",     ptr);}

#ifdef _AIX
  if (ptr) {free(ptr); ptr=NULL;}
#else
  if (ptr) {zfree(ptr);ptr=NULL;}
#endif
}

void *am_realloc(void *ptr, size_t size)
{
    unsigned char *p = NULL;

    if (check_alloc_error_injects()) {return NULL;}

    if ((int64_t)size > 0)
    {
#ifdef _AIX
      p = realloc(ptr, size);
#else
      p = zrealloc(ptr, size);
#endif
    }
    else
    {
        if (!errno) {errno = ENOMEM;}
        KV_TRC_FFDC(pAT, "REALLOC_FAIL ptr:%p size:%"PRIu64"", ptr, size);
    }

    if (!p)
    {
        KV_TRC_FFDC(pAT, "REALLOC_FAIL size:%ld errno=%d", size, errno);
        if (!errno) {errno = ENOMEM;}
    }
    KV_TRC_DBG(pAT, "REALLOC old:%p new:%p size:%ld", ptr, p, size);

    return p;
}

void *ptr_align(void *ptr)
{
  void *new_ptr = NULL;

  if (ptr)
  {
    new_ptr = (void *)(((uintptr_t)(ptr) + ARK_ALIGN - 1) &
                              ~(uintptr_t)(ARK_ALIGN - 1));
  }
  else
  {
      KV_TRC_FFDC(pAT, "ptr_align NULL");
  }

  return new_ptr;
}

