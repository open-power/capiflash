/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/tag.c $                                                */
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

#include <ark.h>

tags_t* tag_new(uint32_t n)
{
  int i;
  tags_t *tags = am_malloc(sizeof(tags_t) + n * sizeof(int32_t));

  tags->n = n;
  tags->c = n;
  pthread_mutex_init(&(tags->l), NULL);
  for(i=0; i<n; i++) {tags->s[i] = i;}

  KV_TRC_EXT3(pAT, "NEWTAG  tags:%p n:%d", tags, tags->c);
  return tags;
}

void tag_free(tags_t *tags)
{
  pthread_mutex_destroy(&(tags->l));
  am_free(tags);
  return;
}

int tag_unbury(tags_t *tags, int32_t *tag)
{
  int ret = 0;

  pthread_mutex_lock(&(tags->l));
  ret = tag_get(tags,tag);
  pthread_mutex_unlock(&(tags->l));

  return ret;
}

int tag_bury(tags_t *tags, int32_t tag)
{
  int ret = 0;

  pthread_mutex_lock(&(tags->l));
  ret = tag_put(tags,tag);
  pthread_mutex_unlock(&(tags->l));

  return ret;
}
