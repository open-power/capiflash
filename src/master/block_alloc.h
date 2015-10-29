/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/block_alloc.h $                                    */
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
#ifndef _BLOCK_ALLOC_H
#define _BLOCK_ALLOC_H

#include <inttypes.h>

typedef size_t aun_t;

typedef struct ba_lun {
    uint64_t    lun_id;
    uint64_t    wwpn;
    size_t      lsize;     /* Lun size in number of LBAs             */
    size_t      lba_size;  /* LBA size in number of bytes            */
    size_t      au_size;   /* Allocation Unit size in number of LBAs */
    void       *ba_lun_handle;
} ba_lun_t;

int ba_init(ba_lun_t *ba_lun);
aun_t ba_alloc(ba_lun_t *ba_lun);
int ba_free(ba_lun_t *ba_lun, aun_t to_free);
int ba_clone(ba_lun_t *ba_lun, aun_t to_clone);
uint64_t ba_space(ba_lun_t *ba_lun);

#ifdef BA_DEBUG
void dump_ba_map(ba_lun_t *ba_lun);
void dump_ba_clone_map(ba_lun_t *ba_lun);
#endif

#endif /* _BLOCK_ALLOC_H */
