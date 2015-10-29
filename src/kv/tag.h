/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/tag.h $                                                */
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

#ifndef __TAG_H__
#define __TAG_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef struct _tags {
  pthread_mutex_t l;
  uint32_t n;
  uint32_t c;
  int32_t  s[];
} tags_t;

tags_t *tag_new(uint32_t n);
void tag_free(tags_t *tags);

int tag_unbury(tags_t *tags, int32_t *tag);
int tag_bury(tags_t *tags, int32_t tag);

#define tag_empty(tag) ((tag)->c==0)

#endif
