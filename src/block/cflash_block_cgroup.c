/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_cgroup.c $                             */
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

#define CFLSH_BLK_FILENUM 0x0700

#include <cflash_block_cgroup.h>
#include <ticks.h>

cflash_block_grp_global_t cflash_block_grp_global;
int                       init_done=0;

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_init
 *
 * FUNCTION:    init chunk_grp hash table structs
 *
 * INPUTS:
 *
 * RETURNS:
 *              0 success, otherwise error.
 ******************************************************************************/
int cblk_cg_init(void)
{
    int rc = 0;
    int i  = 0;

    if (init_done) {return 0;}

    memset(&cflash_block_grp_global,0,sizeof(cflash_block_grp_global));

    for (i=0; i<MAX_CFLASH_GRP_FDS; i++)
    {
        pthread_mutex_init(&cflash_block_grp_global.ht[i].lock,NULL);
    }

    CBLK_TRACE_LOG_FILE(9, "init_done exit rc:%d", rc);
    init_done=1;
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_alloc
 *
 * FUNCTION:    Allocate an chunk_grp structure and insert into a hash list
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              0 success, otherwise error.
 ******************************************************************************/
int cblk_cg_alloc(int                    *id,
                  int                     num_chunks,
                  cflash_block_grp_fd_t **p_ret_cg)
{
    cflash_block_grp_fd_hash_t *p_hash = NULL;
    cflash_block_grp_fd_t      *p_cg   = NULL;
    int                         rc     = 0;
    int                         i      = 0;

    if (id==NULL || p_ret_cg==NULL)
    {
        CBLK_TRACE_LOG_FILE(9, "bad parms");
        rc=-1; errno=EINVAL; goto done;
    }
    *p_ret_cg = NULL;

    if (!(p_cg=malloc(sizeof(cflash_block_grp_fd_t))))
    {
        CBLK_TRACE_LOG_FILE(9, "malloc failed, errno:%d", errno);
        rc=-2; goto error;
    }

    bzero(p_cg, sizeof(cflash_block_grp_fd_t));

    p_cg->chunk_list = malloc(sizeof(cflash_block_chunk_entry_t)*num_chunks);
    if (!p_cg->chunk_list)
    {
        CBLK_TRACE_LOG_FILE(9, "malloc failed, errno:%d", errno);
        rc=-1; goto error;
    }

    bzero(p_cg->chunk_list, sizeof(cflash_block_chunk_entry_t)*num_chunks);

    p_cg->num_chunks = num_chunks;

    for (i=0; i<num_chunks; i++) {p_cg->chunk_list[i].id = NULL_CHUNK_ID;}

    pthread_mutex_lock(&cflash_block_grp_global.lock);
    p_cg->id = cflash_block_grp_global.next_group_id++;
    pthread_mutex_unlock(&cflash_block_grp_global.lock);

    *id = p_cg->id;

    CBLK_GET_CG_HASH_PTR(p_hash, *id);
    if (!p_hash)
    {
        CBLK_TRACE_LOG_FILE(9, "no CG for id:%d", *id);
        rc=-3; goto error;
    }

    /* insert new CG at hash tail */
    pthread_mutex_lock(&p_hash->lock);
    if (!p_hash->head)
    {
        CBLK_TRACE_LOG_FILE(9, "new p_cg:%p at p_hash:%p, head", p_cg, p_hash);
        p_hash->head = p_cg;
    }
    else
    {
        CBLK_TRACE_LOG_FILE(9, "new p_cg:%p at p_hash:%p, tail", p_cg, p_hash);
        p_cg->prev         = p_hash->tail;
        p_hash->tail->next = p_cg;
    }
    p_hash->tail = p_cg;
    pthread_mutex_unlock(&p_hash->lock);

    *p_ret_cg = p_cg;
    goto done;

error:
    if (p_cg && p_cg->chunk_list) {free(p_cg->chunk_list);}
    if (p_cg)                     {free(p_cg);}

done:
    CBLK_TRACE_LOG_FILE(9, "id:%d num_chunks:%d rc:%d", *id, num_chunks, rc);
    return 0;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_get_next_chunk_id
 *
 * FUNCTION:    Get the next chunk id from the chunk group
 *              for I/O.
 *
 * INPUTS:
 *              file index    - file identifier
 *
 * RETURNS:
 *              NULL_CHUNK_ID for error. 
 *              Otherwise the chunk_id is returned.
 ******************************************************************************/
chunk_id_t cblk_cg_get_next_chunk_id(chunk_cg_id_t cgid)
{
    chunk_id_t             chunk_id = NULL_CHUNK_ID;
    cflash_block_grp_fd_t *p_cg     = NULL;
    int                    next     = 0;

    p_cg = CBLK_GET_CG_HASH(cgid);

    if (p_cg)
    {
        if (p_cg->chunk_list == NULL)
        {
            CBLK_TRACE_LOG_FILE(9, "not found, cgid:%d", cgid);
            return chunk_id;
        }
        next     = p_cg->cur_chunk_index;
        chunk_id = p_cg->chunk_list[next].id;
        p_cg->cur_chunk_index = (next+1) % p_cg->num_chunks;
    }

    CBLK_TRACE_LOG_FILE(9, "return id:%d", chunk_id);
    return chunk_id;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_open
 *
 * FUNCTION:    Opens a chunk group
 *
 * INPUTS:
 *              flags       - Flags for open
 *
 * RETURNS:
 *              ULL_CHUNK_CG_ID for error. 
 *              Otherwise the chunk_id is returned.
 ******************************************************************************/
chunk_cg_id_t cblk_cg_open(const char           *path,
                                 int             max_num_requests,
                                 int             mode,
                                 int             num_chunks,
                                 chunk_ext_arg_t ext,
                                 int             flags)
{
    chunk_cg_id_t          id   = NULL_CHUNK_CG_ID;
    cflash_block_grp_fd_t *p_cg = NULL;
    int                    i    = 0;

    if (!init_done) {cblk_cg_init();}

    errno = 0;
    CBLK_TRACE_LOG_FILE(4, "path:%s num_chunks:%d ext:%d flags:%x",
                            path, num_chunks, ext, flags);

    if (num_chunks<=0)
    {
        CBLK_TRACE_LOG_FILE(9, "bad parms, num_chunks=0");
        errno=EINVAL; goto error_open;
    }

    if (flags & CBLK_GROUP_RAID0)
    {
        id = cblk_r0_open(path, max_num_requests, mode, ext, flags);
    }
    else
    {
        if (flags & CBLK_OPN_VIRT_LUN)
        {
            CBLK_TRACE_LOG_FILE(4, "VLUN not supported for CG\n");
            errno = EINVAL;
            goto error_open;
        }
        flags &= ~CBLK_GROUP_ID;

        if (cblk_cg_alloc(&id, num_chunks, &p_cg))
        {
            CBLK_TRACE_LOG_FILE(9, "cg_alloc failed");
            goto error_open;
        }
        if (p_cg == NULL)
        {
            CBLK_TRACE_LOG_FILE(9, "NULL from cg_alloc");
            goto error_open;
        }

        for (i=0; i<p_cg->num_chunks; i++)
        {
            p_cg->chunk_list[i].id = cblk_open(path, max_num_requests,
                                               mode, ext, flags);
            if (p_cg->chunk_list[i].id == NULL_CHUNK_ID)
            {
                CBLK_TRACE_LOG_FILE(9, "open failed errno:%d", errno);
                goto error_open;
            }
            CBLK_TRACE_LOG_FILE(2, "new %d:%d", id, p_cg->chunk_list[i].id);
        }
    }
    goto done;

error_open:
    if (id != NULL_CHUNK_CG_ID) {cblk_cg_close(id, flags);}
    id = NULL_CHUNK_CG_ID;

done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d", id);
    return id;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_close
 *
 * FUNCTION:    Closes a chunk group. 
 *
 * INPUTS:
 *              flags       - Flags for close
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_close(chunk_cg_id_t cgid, int flags)
{
    cflash_block_grp_fd_hash_t *p_hash = NULL;
    cflash_block_grp_fd_t      *p_cg   = NULL;
    int                         rc     = 0;
    int                         i      = 0;

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (p_cg == NULL)
    {
        CBLK_TRACE_LOG_FILE(9, "cgid:%d not found", cgid);
        errno=EINVAL; rc=-1; goto done;
    }

    flags &= ~CBLK_GROUP_MASK;

    for (i=0; i<p_cg->num_chunks; i++)
    {
        if (p_cg->chunk_list[i].id != NULL_CHUNK_ID)
        {
            CBLK_TRACE_LOG_FILE(9, "closing %d:%d",cgid,p_cg->chunk_list[i].id);
            rc=cblk_close(p_cg->chunk_list[i].id, flags);
        }
        else
        {
            CBLK_TRACE_LOG_FILE(9, "cannot close NULL_CHUNK_ID, cgid:%d", cgid);
        }
    }

    CBLK_GET_CG_HASH_PTR(p_hash, cgid);

    pthread_mutex_lock(&p_hash->lock);
    if (p_hash->head == p_cg)
    {
        CBLK_TRACE_LOG_FILE(9, "delete p_cg:%p at p_hash:%p head",p_cg, p_hash);
        p_hash->head = p_cg->next;
        if (p_cg->next) {p_cg->next->prev = NULL;}
        if (p_hash->tail == p_cg) {p_hash->tail = NULL;}
    }
    else if (p_hash->tail == p_cg)
    {
        CBLK_TRACE_LOG_FILE(9, "delete p_cg:%p at p_hash:%p tail",p_cg, p_hash);
        p_cg->prev->next = NULL;
        p_hash->tail     = p_cg->prev;
    }
    else
    {
        CBLK_TRACE_LOG_FILE(9, "delete p_cg:%p at p_hash:%p mid", p_cg, p_hash);
        p_cg->prev->next = p_cg->next;
        p_cg->next->prev = p_cg->prev;
    }
    pthread_mutex_unlock(&p_hash->lock);

    if (p_cg->chunk_list) {free(p_cg->chunk_list);}
    if (p_cg)             {free(p_cg);}

done:
    CBLK_TRACE_LOG_FILE(9, "closed cgid:%d flags:%x rc:%d", cgid, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_get_stats
 *
 * FUNCTION:    Return statistics for this chunk group.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              tag         - Pointer to stats returned
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_get_stats(chunk_cg_id_t cgid, chunk_stats_t *stats, int flags)
{
    cflash_block_grp_fd_t *p_cg = NULL;
    chunk_stats_t          cs   = {0};
    int                    rc   = 0;
    int                    i    = 0;

    if (!stats) {errno=EINVAL; rc=-1; goto done;}
    memset(stats,0,sizeof(chunk_stats_t));

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (p_cg == NULL) {errno=EINVAL; rc=-1; goto done;}

    flags &= ~CBLK_GROUP_MASK;

    for (i=0; i<p_cg->num_chunks; i++)
    {
        if (p_cg->chunk_list[i].id == NULL_CHUNK_ID) {continue;}

        if (cblk_get_stats(p_cg->chunk_list[i].id, &cs, flags) == 0)
        {
            stats->max_transfer_size           += cs.max_transfer_size;
            stats->num_reads                   += cs.num_reads;
            stats->num_writes                  += cs.num_writes;
            stats->num_areads                  += cs.num_areads;
            stats->num_awrites                 += cs.num_awrites;
            stats->num_act_reads               += cs.num_act_reads;
            stats->num_act_writes              += cs.num_act_writes;
            stats->num_act_areads              += cs.num_act_areads;
            stats->num_act_awrites             += cs.num_act_awrites;
            stats->max_num_act_writes          += cs.max_num_act_writes;
            stats->max_num_act_reads           += cs.max_num_act_reads;
            stats->max_num_act_awrites         += cs.max_num_act_awrites;
            stats->max_num_act_areads          += cs.max_num_act_areads;
            stats->num_blocks_read             += cs.num_blocks_read;
            stats->num_blocks_written          += cs.num_blocks_written;
            stats->num_errors                  += cs.num_errors;
            stats->num_aresult_no_cmplt        += cs.num_aresult_no_cmplt;
            stats->num_retries                 += cs.num_retries;
            stats->num_timeouts                += cs.num_timeouts;
            stats->num_no_cmds_free            += cs.num_no_cmds_free;
            stats->num_no_cmd_room             += cs.num_no_cmd_room;
            stats->num_no_cmds_free_fail       += cs.num_no_cmds_free_fail;
            stats->num_fc_errors               += cs.num_fc_errors;
            stats->num_port0_linkdowns         += cs.num_port0_linkdowns;
            stats->num_port1_linkdowns         += cs.num_port1_linkdowns;
            stats->num_port0_no_logins         += cs.num_port0_no_logins;
            stats->num_port1_no_logins         += cs.num_port1_no_logins;
            stats->num_port0_fc_errors         += cs.num_port0_fc_errors;
            stats->num_port1_fc_errors         += cs.num_port1_fc_errors;
            stats->num_cc_errors               += cs.num_cc_errors;
            stats->num_afu_errors              += cs.num_afu_errors;
            stats->num_capi_false_reads        += cs.num_capi_false_reads;
            stats->num_capi_adap_resets        += cs.num_capi_adap_resets;
            stats->num_capi_afu_errors         += cs.num_capi_afu_errors;
            stats->num_capi_afu_intrpts        += cs.num_capi_afu_intrpts;
            stats->num_capi_unexp_afu_intrpts  += cs.num_capi_unexp_afu_intrpts;
            stats->num_active_threads          += cs.num_active_threads;
            stats->max_num_act_threads         += cs.max_num_act_threads;
            stats->num_cache_hits              += cs.num_cache_hits;
        }
    }

done:
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_get_attrs
 *
 * FUNCTION:    Return chunk attrs for this chunk group.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *
 * RETURNS:
 *              <0 fail, >0 good completion
 ******************************************************************************/
int cblk_cg_get_attrs(chunk_cg_id_t cgid, chunk_attrs_t *attr, int flags)
{
    cflash_block_grp_fd_t *p_cg  = NULL;
    chunk_attrs_t          ca    = {0};
    int                    rc    = 0;
    int                    i     = 0;
    int                    unmap = 1;
    uint32_t               np    = 0;
    uint32_t               bs    = 0;
    uint64_t               mt    = 0;

    if (!attr) {errno=EINVAL; rc=-1; goto done;}
    memset(attr,0,sizeof(chunk_attrs_t));

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (p_cg == NULL) {errno=EINVAL; rc=-1; goto done;}

    flags &= ~CBLK_GROUP_MASK;

    for (i=0; i<p_cg->num_chunks; i++)
    {
        if (p_cg->chunk_list[i].id == NULL_CHUNK_ID) {continue;}

        if ((rc=cblk_get_attrs(p_cg->chunk_list[i].id, &ca, flags)) != 0) {break;}
        else
        {
            /* if any do not support, then no unmap */
            if (!(ca.flags1 & CFLSH_ATTR_UNMAP)) {unmap=0;}

            /* use smallest num_paths */
            if (!np || np>ca.num_paths) {np=ca.num_paths;}

            /* use smallest block_size */
            if (!bs || bs>ca.block_size) {bs=ca.block_size;}

            /* use smallest max_transfer_size */
            if (!mt || mt>ca.max_transfer_size) {mt=ca.max_transfer_size;}
        }
    }

    if (rc==0)
    {
        if (unmap) {attr->flags1 |= CFLSH_ATTR_UNMAP;}
        attr->block_size          = bs;
        attr->max_transfer_size   = mt;
        attr->num_paths           = np;
    }

done:
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_get_num_chunks
 *
 * FUNCTION:    Return num_chunks for this chunk group.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *
 * RETURNS:
 *              <0 fail, >0 good completion
 ******************************************************************************/
int cblk_cg_get_num_chunks(chunk_cg_id_t cgid, int flags)
{
    cflash_block_grp_fd_t *p_cg = NULL;
    int                    num  = 0;

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (!p_cg)
    {
        CBLK_TRACE_LOG_FILE(9, "not found, cgid:%d", cgid);
        errno=EINVAL; num=-1; goto done;
    }

    num = p_cg->num_chunks;

done:
    return num;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_get_lun_size
 *
 * FUNCTION:    Returns the number of blocks
 *              associated with the physical lun(s)
 *              that contains this chunk group
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Address of returned number of 
 *                            blocks for this lun
 *              flags       - Flags for open
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_get_lun_size(chunk_cg_id_t cgid, size_t *nblocks, int flags)
{
    cflash_block_grp_fd_t *p_cg = NULL;
    int                    rc   = 0;
    int                    i    = 0;
    size_t                 blks = 0;

    if (!nblocks) {errno=EINVAL; rc=-1; goto done;}
    *nblocks=0;

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (!p_cg) {errno=ENODEV; rc=-1; goto done;}

    if (p_cg->flags & CBLK_GROUP_RAID0)
    {
        for (i=0; i<p_cg->num_chunks; i++)
        {
            rc = cblk_get_lun_size(p_cg->chunk_list[i].id, &blks, 0);
            if (rc) {goto done;}
            *nblocks += blks;
        }
    }
    else {rc = cblk_get_lun_size(p_cg->chunk_list[0].id, nblocks, 0);}

done:
    CBLK_TRACE_LOG_FILE(4, "cgid:%d *nblocks:%d flags:%x rc:%d",
                        cgid, *nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_get_size
 *
 * FUNCTION:    Returns the number of blocks
 *              associated with this chunk
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Address of returned number of 
 *                            blocks for this lun
 *              flags       - Flags for open
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_get_size(chunk_cg_id_t cgid, size_t *nblocks, int flags)
{
    cflash_block_grp_fd_t *p_cg = NULL;
    int                    rc   = 0;
    int                    i    = 0;
    size_t                 blks = 0;

    if (!nblocks) {errno=EINVAL; rc=-1; goto done;}
    *nblocks=0;

    p_cg = CBLK_GET_CG_HASH(cgid);
    if (!p_cg) {errno=EINVAL; rc=-1; goto done;}

    if (p_cg->flags & CBLK_GROUP_RAID0)
    {
        for (i=0; i<p_cg->num_chunks; i++)
        {
            rc = cblk_get_size(p_cg->chunk_list[i].id, &blks, 0);
            if (rc) {goto done;}
            *nblocks += blks;
        }
    }
    else {rc = cblk_get_size(p_cg->chunk_list[0].id, nblocks, 0);}

done:
    CBLK_TRACE_LOG_FILE(4, "cgid:%d *nblocks:%d flags:%x rc:%d",
                        cgid, *nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_set_size
 *
 * FUNCTION:    Sets the number of blocks
 *              associated with the physical lun that 
 *              contains this chunk
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              nblocks     - Address of returned number of 
 *                            blocks for this lun
 *              flags       - Flags for open
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_set_size(chunk_cg_id_t cgid, size_t nblocks, int flags)
{
    int rc = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_set_size(cgid, nblocks);
    }
    else
    {
        errno = EINVAL;
        rc    = -1;
    }
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_read
 *
 * FUNCTION:    Reads data from the specified offset in the chunk
 *              group and places that data in the specified buffer.
 *              This request is a blocking read request (i.e.
 *              it will not return until either data is read or
 *              an error is encountered).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_read(chunk_cg_id_t   cgid,
                 void           *pbuf,
                 cflash_offset_t lba,
                 size_t          nblocks,
                 int             flags)
{
    int        rc       = 0;
    chunk_id_t chunk_id = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_read(cgid, pbuf, lba, nblocks, flags);
    }
    else
    {
        errno = 0;
        chunk_id = cblk_cg_get_next_chunk_id(cgid);

        if (chunk_id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_read(chunk_id, pbuf, lba, nblocks, flags);
    }

done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%2d:%2d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, chunk_id, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_write
 *
 * FUNCTION:    Writes data to the specified offset in the chunk
 *              group from the specified buffer. This request is
 *              a blocking write request (i.e.it will not 
 *              return until either data is written or
 *              an error is encountered).
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_write(chunk_cg_id_t   cgid,
                  void           *pbuf,
                  cflash_offset_t lba,
                  size_t          nblocks,
                  int             flags)
{
    int        rc       = 0;
    chunk_id_t chunk_id = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_write(cgid, pbuf, lba, nblocks, flags);
    }
    else
    {
        errno = 0;
        chunk_id = cblk_cg_get_next_chunk_id(cgid);

        if (chunk_id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_write(chunk_id, pbuf, lba, nblocks, flags);
    }

done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_unmap
 *
 * FUNCTION:    Writes data to the specified offset in the chunk
 *              group from the specified buffer. This request is
 *              a blocking write request (i.e.it will not
 *              return until either data is written or
 *              an error is encountered).
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_unmap(chunk_cg_id_t   cgid,
                  void           *pbuf,
                  cflash_offset_t lba,
                  size_t          nblocks,
                  int             flags)
{
    int        rc       = 0;
    chunk_id_t chunk_id = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_write(cgid, pbuf, lba, nblocks, flags);
    }
    else
    {
        errno = 0;
        chunk_id = cblk_cg_get_next_chunk_id(cgid);

        if (chunk_id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_unmap(chunk_id, pbuf, lba, nblocks, flags);
    }

done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_aread
 *
 * FUNCTION:    Reads data from the specified offset in the chunk
 *              group and places that data in the specified buffer.
 *              This request is an asynchronous read request (i.e.
 *              it will return as soon as it has issued the request
 *              to the device. It will not wait for a response from 
 *              the device.).
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to read data into
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to read data from.
 *              nblocks     - Number of blocks to read.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0   for good completion,  
 *              >0  Read data was in cache and was read.
 *              -1 for error with ERRNO set
 ******************************************************************************/
int cblk_cg_aread(chunk_cg_id_t      cgid,
                  void              *pbuf,
                  cflash_offset_t    lba,
                  size_t             nblocks,
                  cflsh_cg_tag_t    *tag,
                  cblk_arw_status_t *status,
                  int                flags)
{
    int rc   = 0;
    int ctag = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_aread(cgid, pbuf, lba, nblocks, tag, status, flags);
    }
    else
    {
        errno   = 0;
        tag->id = cblk_cg_get_next_chunk_id(cgid);

        if (tag->id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_aread(tag->id, pbuf, lba, nblocks, &ctag, status, flags);
        if (rc >= 0) {tag->tag = ctag;}
        else         {tag->tag = -1;}
    }
done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d:%d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, tag->id, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_awrite
 *
 * FUNCTION:    Writes data to the specified offset in the chunk
 *              group from the specified buffer. This request is
 *              an asynchronous write request (i.e.it will
 *              return as soon as it has issued the request
 *              to the device. It will not wait for a response from 
 *              the device.).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_awrite(chunk_cg_id_t      cgid,
                   void              *pbuf,
                   cflash_offset_t    lba,
                   size_t             nblocks,
                   cflsh_cg_tag_t    *tag,
                   cblk_arw_status_t *status,
                   int                flags)
{
    int rc   = 0;
    int ctag = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_awrite(cgid, pbuf, lba, nblocks, tag, status, flags);
    }
    else
    {
        errno   = 0;
        tag->id = cblk_cg_get_next_chunk_id(cgid);

        if (tag->id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_awrite(tag->id, pbuf, lba, nblocks, &ctag, status, flags);
        if (rc >= 0) {tag->tag = ctag;}
        else         {tag->tag = -1;}
    }
done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d:%d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, tag->id, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_aunmap
 *
 * FUNCTION:    Unmap data blocks specified. The data buffer
 *              must be initialized to zeroes.This request is
 *              an asynchronous unmap request (i.e.it will
 *              return as soon as it has issued the request
 *              to the device. It will not wait for a response from
 *              the device.).
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              buf         - Buffer to write data from
 *              lba         - starting LBA (logical Block Address)
 *                            in chunk to write data to.
 *              nblocks     - Number of blocks to write.
 *              tag         - Tag associated with this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_aunmap(chunk_cg_id_t      cgid,
                   void              *pbuf,
                   cflash_offset_t    lba,
                   size_t             nblocks,
                   cflsh_cg_tag_t    *tag,
                   cblk_arw_status_t *status,
                   int                flags)
{
    int rc   = 0;
    int ctag = 0;

    if (flags & CBLK_GROUP_RAID0)
    {
        rc = cblk_r0_aunmap(cgid, pbuf, lba, nblocks, tag, status, flags);
    }
    else
    {
        errno   = 0;
        tag->id = cblk_cg_get_next_chunk_id(cgid);

        if (tag->id == NULL_CHUNK_ID)
        {
            errno = EINVAL;
            rc    = -1;
            goto done;
        }
        flags &= ~CBLK_GROUP_MASK;
        rc = cblk_aunmap(tag->id, pbuf, lba, nblocks, &ctag, status, flags);
        if (rc >= 0) {tag->tag = ctag;}
        else         {tag->tag = -1;}
    }
done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d:%d lba:%d nblocks:%d flags:%x rc:%d",
                        cgid, tag->id, lba, nblocks, flags, rc);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_aresult
 *
 * FUNCTION:    Waits for asynchronous read or writes to complete
 *              and returns the status of them.
 *
 *
 * INPUTS:
 *              chunk_id    - Chunk identifier
 *              tag         - Tag associated with this request that
 *                            completed.
 *              flags       - Flags on this request.
 *
 * RETURNS:
 *              0   for good completion, but requested tag has not yet completed.
 *              -1  for  error and errno is set.
 *              > 0 for good completion where the return value is the
 *                  number of blocks transferred.
 ******************************************************************************/
int cblk_cg_aresult(chunk_cg_id_t   cgid,
                    cflsh_cg_tag_t *ptag,
                    uint64_t       *status,
                    int             flags)
{
    cflash_block_grp_fd_t *pgrp   = NULL;
    int                    rc     = 0;
    int                    i      = 0;
    int                    tflags = flags & ~CBLK_GROUP_MASK;

    pgrp = CBLK_GET_CG_HASH(cgid);
    if (!pgrp) {errno=EINVAL; rc=-1; goto done;}
    if (!ptag) {errno=EINVAL; rc=-1; goto done;}

    /* we are looking for a specific tag */
    if (! (flags & CBLK_ARESULT_NEXT_TAG))
    {
        rc = cblk_aresult(ptag->id, &ptag->tag, status, tflags);
        goto done;
    }

    tflags &= ~CBLK_ARESULT_BLOCKING;

    /* if blocking, then loop until timeout or completed op */
    uint64_t start = getticks();
    do
    {
        if (SDELTA(start,cflsh_blk.nspt) >= 50)
        {
            CBLK_TRACE_LOG_FILE(1,"TIMEOUT: id:%d", cgid);
            rc=-1; errno=ETIME; goto done;
        }

        /* wait for a cmd to complete */
        for (i=0; i<pgrp->num_chunks; i++)
        {
            ptag->id = cblk_cg_get_next_chunk_id(cgid);
            rc = cblk_aresult(ptag->id, &ptag->tag, status, tflags);
            if (rc>0)  {goto done;}
            if (rc<0 && errno==ENOENT) {rc=0;}
        }
    } while (flags & CBLK_ARESULT_BLOCKING);

done:
    if (rc == -1)
    {
        /* ptag could be null, so don't dereference */
        CBLK_TRACE_LOG_FILE(9, "cgid:%d pgrp:%p ptag:%p flags:%x rc:%d",
                                cgid, pgrp, ptag, flags, rc);
    }
    else
    {
        CBLK_TRACE_LOG_FILE(9, "cgid:%d:%d tag:%d flags:%x rc:%d",
                                cgid, ptag->id, ptag->tag, flags, rc);
    }
    return rc;
}

/**
 *******************************************************************************
 * \brief
 * \details
 *
 * NAME:        cblk_cg_clone_after_fork
 *
 * FUNCTION:    clone a (virtual lun) chunk group with an existing (original)
 *              chunk group. This is useful if a process is forked and the
 *              child wants to read data from the parents chunk group.
 *
 * INPUTS:
 *              chunk_id          - Chunk id to be cloned.
 *              mode              - Access mode for the chunk 
 *                                  (O_RDONLY, O_WRONLY, O_RDWR)
 *              flags             - Flags on this request.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 ******************************************************************************/
int cblk_cg_clone_after_fork(chunk_cg_id_t cgid, int mode, int flags)
{
    cflash_block_grp_fd_t *p_cg = NULL;
    int                    rc   = 0;
    int                    i    = 0;

    p_cg = CBLK_GET_CG_HASH(cgid);

    if (p_cg == NULL) {errno=EINVAL; rc=-1; goto done;}

    if (flags & CBLK_GROUP_RAID0)
    {
        flags &= ~CBLK_GROUP_MASK;
        for (i=0; i<p_cg->num_chunks; i++)
        {
            rc = cblk_clone_after_fork(p_cg->chunk_list[i].id, mode, flags);
            if (rc) {break;}
        }
    }
    else
    {
        errno = EINVAL;
        rc    = -1;
    }

done:
    CBLK_TRACE_LOG_FILE(9, "cgid:%d mode:%d flags:%x rc:%d",
                        cgid, mode, flags, rc);
    return rc;
}
