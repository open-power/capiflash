/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ea.h $                                                 */
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
#ifndef __EA_H__
#define __EA_H__

#include <stdint.h>
#include "ark.h"
#include "bl.h"

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include "capiblock.h"
#endif

#define ARK_EA_READ  0
#define ARK_EA_WRITE 1

#define st_memory  store.memory
#define st_flash   store.flash.chkid
#define st_device  store.flash.device

typedef struct flash_cntrl
{
  chunk_id_t chkid;
  char      *device;
} flash_cntrl_t;

typedef union _store_id {
  uint8_t        *memory;
  flash_cntrl_t   flash;
} store_id_t;

#define ARK_EA_BLK_ASYNC_CMDS 4096

typedef struct _ea {
  pthread_rwlock_t ea_rwlock;
  uint64_t         bsize;
  uint64_t         bcount;
  uint64_t         size;
  store_id_t       store;

#define EA_STORE_TYPE_MEMORY  1
#define EA_STORE_TYPE_FILE    2
#define EA_STORE_TYPE_FLASH   3
  uint8_t    st_type;
} EA;

#define ARK_SYNC_EA_READ(_ea)                                  \
{                                                              \
  if ((_ea)->st_type == EA_STORE_TYPE_MEMORY )                 \
  {                                                            \
    pthread_rwlock_rdlock(&((_ea)->ea_rwlock));                \
  }                                                            \
}

#define ARK_SYNC_EA_WRITE(_ea)                                 \
{                                                              \
  if ((_ea)->st_type == EA_STORE_TYPE_MEMORY )                 \
  {                                                            \
    pthread_rwlock_wrlock(&((_ea)->ea_rwlock));                \
  }                                                            \
}

#define ARK_SYNC_EA_UNLOCK(_ea)                                \
{                                                              \
  if ((_ea)->st_type == EA_STORE_TYPE_MEMORY )                 \
  {                                                            \
    pthread_rwlock_unlock(&((_ea)->ea_rwlock));                \
  }                                                            \
}

EA *ea_new(const char *path, uint64_t bsize, int basyncs, uint64_t *size, 
           uint64_t *bcount, uint64_t vlun);
int ea_resize(EA *ea, uint64_t bsize, uint64_t bcount);
int ea_delete(EA *ea);

int ea_read(EA *ea, uint64_t lba, void *dst);

int ea_write(EA *ea, uint64_t lba, void *src);

int ea_async_io(EA *ea, int op, void *addr, ark_io_list_t *blist, int64_t len, int nthrs);


#endif
