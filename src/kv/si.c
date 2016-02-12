/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/si.c $                                                 */
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
#include <string.h>
#include <inttypes.h>

#include "am.h"
#include "ut.h"
#include "si.h"

#include <arkdb_trace.h>
#include <errno.h>

// there is a better variable size implementation but for now
typedef struct _sie {
  uint64_t nxt; // next sie
  uint64_t gid; // global id 
  uint64_t len; // string length
  uint64_t dat; // position in main buffer
} SIE;

typedef struct _si {
  uint64_t nh; // # of hash table entries
  uint64_t ne; // # of si entries
  uint64_t nb; // # bytes for the table
  uint64_t ent_next; // next free entry
  uint64_t dat_next; // next free bytes
  uint64_t gid_next; // next free global id

  int64_t *tbl; // hash table
  SIE     *ent; // SI entries
  char    *dat; // the data
} SI;

uint64_t si_hash(char *buf, int n) {
  unsigned char *b = (unsigned char *)buf;
  uint64_t sum = 0;
  int i;
  for (i=0; i<n; i++) sum = sum * 65559 + b[i];
  return sum;
}

void *si_new(uint64_t nh, uint64_t ne, uint64_t nb) {

  SI *si = am_malloc(sizeof(SI)); 
  if (si == NULL)
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "FFDC1: nh %ld ne %ld nb %ld, rc = %d",
            nh, ne, nb, errno);
    return NULL;
  }
  
  si->nh = nh;
  si->ne = ne;
  si->nb = nb;
  
  si->ent_next = 0;
  si->gid_next = 0;
  si->dat_next = 0;

  si->tbl = am_malloc(nh * sizeof(uint64_t));
  if ( si->tbl == NULL )
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "FFDC2: nh %ld ne %ld nb %ld, rc = %d",
            nh, ne, nb, errno);
  }
  else
  {
    memset(si->tbl, 0xFF, nh * sizeof(uint64_t));
    si->dat = am_malloc(nb);
    if (si->dat == NULL)
    {
      errno = ENOMEM;
      KV_TRC_FFDC(pAT, "FFDC3: nh %ld ne %ld nb %ld, rc = %d",
              nh, ne, nb, errno);
      am_free(si->tbl);
      am_free(si);
      si = NULL;
    }
    else
    {
      si->ent = am_malloc(ne * sizeof(SIE));
      if (si->ent == NULL)
      {
        errno = ENOMEM;
        KV_TRC_FFDC(pAT, "FFDC4: nh %ld ne %ld nb %ld, rc = %d",
                nh, ne, nb, errno);
        am_free(si->tbl);
        am_free(si->dat);
        am_free(si);
        si = NULL;
      }
    }
  }

  memset(si->tbl, 0xFF, nh * sizeof(uint64_t));
  return si;
}

uint64_t si_intern(void *siv, char *buf, int n) {
  SI *si = (SI*)siv;
  uint64_t id = 0;
  uint64_t hsh =  si_hash(buf,n);
  uint64_t pos = hsh % si->nh;
  int64_t  ent = si->tbl[pos];
  int found = 0;
  while (!found && ent>=0 ) {
    if (memcmp(si->dat + si->ent[ent].dat, buf, n)==0) {
      found = 1;
    } else {
      ent = si->ent[ent].nxt;
    }
  }
  if (found) {
    id = si->ent[ent].gid;
  } else if ((si->ent_next < si->ne-1) && (si->dat_next+n < si->nb)) {
    id  = si->gid_next++;
    ent = si->ent_next++;
    
    si->ent[ent].nxt = si->tbl[pos];
    si->ent[ent].gid = id;
    si->ent[ent].len = n;
    si->ent[ent].dat = si->dat_next;
    
    memcpy(si->dat + si->dat_next, buf, n);
    si->dat_next += n;
    
    si->tbl[pos] = ent;
  } else {
    id = -1;
  }
  return id;  
}

void si_summary(void *siv) {
  SI *si = (SI*)siv;
  uint64_t ecnt = 0;
  uint64_t i;
  for(i=0; i<si->nh; i++) if (si->tbl[i]>=0) ecnt++;
    
  printf("table %"PRIu64"/%"PRIu64" entries %"PRIu64"/%"PRIu64" data %"PRIu64"/%"PRIu64"\n", 
         ecnt, si->nh,
         si->ent_next, si->ne,
         si->dat_next, si->nb);
}
