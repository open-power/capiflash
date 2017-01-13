/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_group.h $                              */
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

#ifndef _H_CFLASH_BLOCK_GROUP
#define _H_CFLASH_BLOCK_GROUP

#include "cflash_block_internal.h"
#include "cflash_block_protos.h"


#define MAX_CFLASH_GRP_FDS 64
#define MAX_CG_HASH (MAX_CFLASH_GRP_FDS-1)

/**
 *******************************************************************************
 * \brief
 *   data structure for chunk
 ******************************************************************************/
typedef struct cflash_block_chunk_entry_s
{
    uint64_t   flags;
    chunk_id_t id;                                 /* Chunk id                */
} cflash_block_chunk_entry_t;

/**
 *******************************************************************************
 * \brief
 *   data structure for a chunk group
 ******************************************************************************/
typedef struct cflash_block_grp_fd_s
{
    struct cflash_block_grp_fd_s *prev;            /* Previous group in list  */
    struct cflash_block_grp_fd_s *next;            /* Next group in list      */
    uint64_t                      flags;
    chunk_cg_id_t                 id;              /* file descriptor;        */
    int                           num_chunks;      /* Number of chunks used   */
    int                           cur_chunk_index; /* cur index in chunk_list */
    cflash_block_chunk_entry_t   *chunk_list;
    cflsh_blk_lock_t              lock;
} cflash_block_grp_fd_t;

/**
 *******************************************************************************
 * \brief
 *   root data structure pointing to a linked list of chunk groups
 ******************************************************************************/
typedef struct cflash_block_grp_fd_hash_s
{
    pthread_mutex_t        lock;
    cflash_block_grp_fd_t *head;
    cflash_block_grp_fd_t *tail;
} cflash_block_grp_fd_hash_t;

/**
 *******************************************************************************
 * \brief
 *  global anchor block for Chunk Groups
 ******************************************************************************/
typedef struct cflash_block_grp_global_s
{
    pthread_mutex_t            lock;
    cflash_block_grp_fd_hash_t ht[MAX_CFLASH_GRP_FDS];
    int                        flags;
    int                        next_group_id;
} cflash_block_grp_global_t;

extern cflash_block_grp_global_t cflash_block_grp_global;

#define CBLK_GET_CG_HASH_PTR(_p_hash, _id) \
  _p_hash=&cflash_block_grp_global.ht[_id & MAX_CG_HASH];

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        CBLK_GET_CG_HASH
 *
 * FUNCTION:    Find empty chunk group fd from file descriptor in the hash table
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              NULL = No chunk group found.
 *              pointer = chunk group found.
 ******************************************************************************/
static inline cflash_block_grp_fd_t* CBLK_GET_CG_HASH(int id)
{
    cflash_block_grp_fd_hash_t *p_hash = NULL;
    cflash_block_grp_fd_t      *p_grp  = NULL;

    CBLK_GET_CG_HASH_PTR(p_hash, id);

    pthread_mutex_lock(&p_hash->lock);
    p_grp = p_hash->head;
    while (p_grp && p_grp->id != id) {p_grp = p_grp->next;}
    pthread_mutex_unlock(&p_hash->lock);

    if (p_grp && p_grp->chunk_list == NULL) {p_grp = NULL;}

    return p_grp;
}

/* cflash_block_cgroup.c protos */
int cblk_cg_init(void);
int cblk_cg_alloc(int *id,int num_chunks,cflash_block_grp_fd_t **ret_chunk_grp);
int cblk_cg_free(cflash_block_grp_fd_t *chunk_grp);
chunk_id_t cblk_cg_get_next_chunk_id(chunk_cg_id_t cgid);

#endif /* _H_CFLASH_BLOCK_GROUP */
