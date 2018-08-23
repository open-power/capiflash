/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bl.h $                                                 */
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
#ifndef __BL_H__
#define __BL_H__

#include <stdint.h>
#include <pthread.h>
#include <iv.h>

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif

#define BL_INITN 10000

typedef struct ark_io_list
{
  int64_t        blkno;
  cflsh_cg_tag_t a_tag;
} ark_io_list_t;

typedef struct _bl
{
  pthread_rwlock_t iv_rwlock;
  int64_t          n;           // number of block list elements
  int64_t          w;
  int64_t          head;
  int64_t          count;
  int64_t          hold;
  int64_t          top;         // index of the next block list element to initialize
  IV              *iv;
} BL;

// initializes/adds BL_INITN blocks to make them available for a take,
// called when an IO needs blocks and there aren't enough available
int bl_init_chain_link(BL *bl);

// create a chain 0..n-1
BL *bl_new(int64_t n, int w);

// reserve the first resN blocks,
// must be called before any take,
int bl_reserve(BL *bl, uint64_t resN);

BL *bl_resize(BL *bl, int64_t n, int64_t w);
void bl_free(BL *bl);

// take returns a root to a chain of n blocks, neagative is error
int64_t bl_take(BL *bl, int64_t n);
void bl_check_take(BL *bl, int64_t n);

// put a chain back, return the length of returned chain, negative is error
int64_t bl_drop(BL *bl, int64_t b);

// add single block
int64_t bls_add(BL *bl, int64_t b);

// remove single block
int64_t bls_rem(BL *bl);

// number of items left
int64_t bl_left(BL *bl);

// the length of a chain
int64_t bl_len(BL *bl, int64_t b);

// the next block in a chain
int64_t bl_next(BL *bl, int64_t);

// cause drops to be held until released to free pool
void bl_hold (BL *bl);

// release any held blocks to the free list
void bl_release(BL *bl);

// Return an array of linked blocks starting at b
ark_io_list_t *bl_chain(BL *bl, int64_t b, int64_t len, int64_t offset);

// realloc an ark_io_list if required, setup the list starting at b
int bl_rechain(ark_io_list_t **aiol,
               BL             *bl,
               int64_t         b,
               int64_t         n,
               int64_t         o,
               int64_t         offset);

ark_io_list_t *bl_chain_blocks(BL *bl, int64_t len, int64_t offset);

ark_io_list_t *bl_chain_no_bl(int64_t len, int64_t offset);

// generate a graph of the blocks
void bl_dot(BL *bl, int n, int *bcnt, int ccnt, int64_t *chain);


#endif
