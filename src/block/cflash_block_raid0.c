/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_raid0.c $                              */
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

#include <cflash_block_cgroup.h>
#include <ticks.h>

#define CFLSH_BLK_FILENUM 0x0800
#define CFLASH_R0_MAX_DEV 8
#define CFLASH_R0_MAX_BUF 32
#define CFLASH_R0_CMD_TO  50

/**
********************************************************************************
** \brief
*******************************************************************************/
chunk_r0_id_t cblk_r0_open(const char     *pdevs,
                           int             max_num_requests,
                           int             mode,
                           chunk_ext_arg_t ext,
                           int             flags)
{
    char                   devstr[CFLASH_R0_MAX_DEV][CFLASH_R0_MAX_BUF];
    cflash_block_grp_fd_t *pgrp      = NULL;
    int                    devN      = 0;
    chunk_r0_id_t          rid       = 0;
    char                  *pstr      = NULL;
    char                  *dbuf      = NULL;
    char                   buf[2048] = {0};
    int                    i,j;

    memset(devstr,0,sizeof(devstr));

    if (pdevs)
    {
        CBLK_TRACE_LOG_FILE(2, "pdevs:%s QD:%d flags:%x",
                            pdevs, max_num_requests, flags);
    }

    flags &= ~CBLK_GROUP_RAID0;

    /*--------------------------------------------------------------------------
     * parse the colon separated /dev/sgX:...
     *------------------------------------------------------------------------*/
    if (pdevs && strncmp(pdevs,"/dev",4)==0)
    {

        dbuf = strdup(pdevs);
        sprintf(dbuf,"%s",pdevs);
        while ((pstr=strsep((&dbuf),":")))
        {
            strcat(buf, pstr); strcat(buf, " ");
            sprintf(devstr[devN],"%s",pstr);
            if (++devN == CFLASH_R0_MAX_DEV) {break;}
        }
        CBLK_TRACE_LOG_FILE(2, "parsed RAID0 (%s)", buf);
    }

#ifndef _AIX
    /*--------------------------------------------------------------------------
     * RAID0 is passed in, use a script to gather a list of devices
     *   -parse RAID0:N, RAID0:*
     *------------------------------------------------------------------------*/
    else if (pdevs && strncmp(pdevs,"RAID0",5)==0)
    {
        FILE *fp                       = NULL;
        char device[CFLASH_R0_MAX_BUF] = {0};
        char devNStr[3]                = {0};
        int  devN_in                   = 0;
        int  wildcard                  = 1;

        if (strlen(pdevs) > 6 && pdevs[5] == ':' && pdevs[6] != '*')
        {
            wildcard   = 0;
            devNStr[0] = pdevs[6];
            devNStr[1] = pdevs[7];
            devN_in    = atoi(devNStr);
        }
        fp = popen("/opt/ibm/capikv/afu/cflash_devices.pl", "r");
        if (!fp)
        {
            CBLK_TRACE_LOG_FILE(1, "cflash_devices.pl failed");
            errno = ENODEV;
            goto error_devs;
        }
        /* read in devices, store in devstr */
        while (fgets(device, CFLASH_R0_MAX_BUF, fp))
        {
            device[strlen(device)-1] = '\0';
            strcat(buf, device); strcat(buf, " ");
            memset(devstr[devN],0,CFLASH_R0_MAX_BUF);
            strcpy(devstr[devN], device);
            memset(device,0,CFLASH_R0_MAX_BUF);
            if (++devN == CFLASH_R0_MAX_DEV) {break;}
        }
        fclose(fp);
        if (devN>1) {CBLK_TRACE_LOG_FILE(1, "RAID0: (%s)", buf);}

        /* RAID0:N is passed, ensure the number of devices found matches */
        if (!wildcard && devN != devN_in)
        {
            CBLK_TRACE_LOG_FILE(1, "RAID0 devs found(%d) does not match "
                                   "the input(%s) num(%s<=>%d)",
                                   devN, pdevs, devNStr, devN_in);
            errno = ENODEV;
            goto error_devs;
        }
    }
#endif /* !_AIX */

    /*--------------------------------------------------------------------------
     * use whatever this is
     *------------------------------------------------------------------------*/
    else if (pdevs)
    {
        CBLK_TRACE_LOG_FILE(2, "using device(%s)", pdevs);
        strcpy(devstr[0], pdevs);
        devN = 1;
    }
    /*--------------------------------------------------------------------------
     * use NULL
     *------------------------------------------------------------------------*/
    else
    {
        CBLK_TRACE_LOG_FILE(2, "using devstr=NULL");
        devN = 1;
    }

    if (devN == 0)
    {
        errno = ENODEV;
        CBLK_TRACE_LOG_FILE(1,"ERROR: devN = 0, errno:%d", errno);
        goto error_devs;
    }

    if (cblk_cg_alloc(&rid, devN, &pgrp)) {goto error_devs;}
    if (pgrp == NULL)                     {goto error_devs;}

    pgrp->flags |= CBLK_GROUP_RAID0;

    /*--------------------------------------------------------------------------
     * open devices
     *------------------------------------------------------------------------*/
    for (i=0; i<pgrp->num_chunks; i++)
    {
        pgrp->chunk_list[i].id = cblk_open(devstr[i], max_num_requests, mode,
                                           ext, flags);
        if (pgrp->chunk_list[i].id == NULL_CHUNK_ID)
        {
            CBLK_TRACE_LOG_FILE(1, "error opening(%s)", devstr[i]);
            for (j=0; j < i; j++) {cblk_close(pgrp->chunk_list[j].id,0);}
            goto error_devs;
        }
        CBLK_TRACE_LOG_FILE(4, "new raid0 %d:%d", rid,pgrp->chunk_list[i].id);
    }

    errno=0;
    goto done;

error_devs:
    rid = NULL_CHUNK_CG_ID;

done:
    CBLK_TRACE_LOG_FILE(9,"rid:%d devN:%d", rid, devN);
    return rid;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_set_size(chunk_r0_id_t id, size_t blocksN)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    int                    rc   = 0;
    size_t                 blks = 0;
    int                    i    = 0;

    pgrp = CBLK_GET_CG_HASH(id);
    if (!pgrp || !blocksN) {errno=EINVAL; rc=-1; goto done;}

    blks = blocksN / pgrp->num_chunks;

    /* if blocksN is not an even stripe, alloc an extra block per chunk */
    if (blocksN % pgrp->num_chunks) {++blks;}

    for (i=0; i<pgrp->num_chunks;i++)
    {
        rc = cblk_set_size(pgrp->chunk_list[i].id, blks, 0);
        if (rc) {goto done;}
    }

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_read(chunk_r0_id_t   id,
                 void           *buf,
                 cflash_offset_t lba,
                 size_t          nblocks,
                 int             flags)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    chunk_id_t             cid  = 0;
    uint32_t               dev  = 0;
    cflash_offset_t        clba = 0;
    int                    rc   = 0;

    if (NULL==(pgrp=CBLK_GET_CG_HASH(id)))
        {errno=EINVAL; rc=-1; goto done;}

    flags &= ~CBLK_GROUP_RAID0;
    dev    = (uint32_t)lba % pgrp->num_chunks;
    cid    = pgrp->chunk_list[dev].id;
    clba   = lba/pgrp->num_chunks;

    rc = cblk_read(cid, buf, clba, nblocks, flags);

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_write(chunk_r0_id_t   id,
                  void           *pbuf,
                  cflash_offset_t lba,
                  size_t          nblocks,
                  int             flags)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    chunk_id_t             cid  = 0;
    uint32_t               dev  = 0;
    cflash_offset_t        clba = 0;
    int                    rc   = 0;

    if (NULL==(pgrp=CBLK_GET_CG_HASH(id)))
        {errno=EINVAL; rc=-1; goto done;}

    flags &= ~CBLK_GROUP_RAID0;
    dev    = (uint32_t)lba % pgrp->num_chunks;
    cid    = pgrp->chunk_list[dev].id;
    clba   = lba/pgrp->num_chunks;

    rc = cblk_write(cid, pbuf, clba, nblocks, flags);

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_aread(chunk_r0_id_t      id,
                  void              *buf,
                  cflash_offset_t    lba,
                  size_t             nblocks,
                  cflsh_cg_tag_t    *tag,
                  cblk_arw_status_t *status,
                  int                flags)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    chunk_id_t             cid  = 0;
    uint32_t               dev  = 0;
    cflash_offset_t        clba = 0;
    int                    ctag = 0;
    int                    rc   = 0;

    if (nblocks>1) {errno=EINVAL; rc=-1; goto done;}

    pgrp = CBLK_GET_CG_HASH(id);
    if (!pgrp)        {errno=EINVAL; rc=-1; goto done;}
    if (!buf || !tag) {errno=EINVAL; rc=-1; goto done;}

    flags  &= ~CBLK_GROUP_RAID0;
    dev     = (uint32_t)lba % pgrp->num_chunks;
    cid     = pgrp->chunk_list[dev].id;
    clba    = lba/pgrp->num_chunks;
    tag->id = cid;

    rc = cblk_aread(cid, buf, clba, nblocks, &ctag, status, flags);
    if (rc >= 0) {tag->tag = ctag;}
    else         {tag->tag = -1;}

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_awrite(chunk_r0_id_t      id,
                   void              *buf,
                   cflash_offset_t    lba,
                   size_t             nblocks,
                   cflsh_cg_tag_t    *tag,
                   cblk_arw_status_t *status,
                   int                flags)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    chunk_id_t             cid  = 0;
    uint32_t               dev  = 0;
    cflash_offset_t        clba = 0;
    int                    ctag = 0;
    int                    rc   = 0;

    if (nblocks>1) {errno=EINVAL; rc=-1; goto done;}

    pgrp = CBLK_GET_CG_HASH(id);
    if (!pgrp)        {errno=EINVAL; rc=-1; goto done;}
    if (!buf || !tag) {errno=EINVAL; rc=-1; goto done;}

    flags  &= ~CBLK_GROUP_RAID0;
    dev     = (uint32_t)lba % pgrp->num_chunks;
    cid     = pgrp->chunk_list[dev].id;
    clba    = lba/pgrp->num_chunks;
    tag->id = cid;

    rc = cblk_awrite(cid, buf, clba, nblocks, &ctag, status, flags);
    if (rc >= 0) {tag->tag = ctag;}
    else         {tag->tag = -1;}

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int cblk_r0_aresult(chunk_r0_id_t   id,
                    cflsh_cg_tag_t *ptag,
                    uint64_t       *status,
                    int             flags)
{
    cflash_block_grp_fd_t *pgrp = NULL;
    int                    rc   = 0;
    int                    i    = 0;

    flags &= ~CBLK_GROUP_RAID0;

    pgrp = CBLK_GET_CG_HASH(id);
    if (!pgrp) {errno=EINVAL; rc=-1; goto done;}
    if (!ptag) {errno=EINVAL; rc=-1; goto done;}

    if (flags & CBLK_ARESULT_NEXT_TAG)
    {
        int tflags = flags & ~CBLK_ARESULT_BLOCKING;

        /* if not blocking, check each device for a completed op */
        for (i=0; i<pgrp->num_chunks; i++)
        {
            ptag->id = cblk_cg_get_next_chunk_id(id);
            rc = cblk_aresult(ptag->id, &ptag->tag,status,tflags);
            if (rc>0)  {goto done;}
            if (rc<0 && errno==ENOENT) {rc=0;}
        }

        /* if blocking, then loop until timeout or completed op */
        uint64_t start = getticks();
        while (flags & CBLK_ARESULT_BLOCKING)
        {
            if (SDELTA(start,cflsh_blk.nspt) >= CFLASH_R0_CMD_TO)
            {
                CBLK_TRACE_LOG_FILE(1,"TIMEOUT: id:%d", id);
                rc=-1; errno=ETIME; goto done;
            }

            /* wait for a cmd to complete */
            for (i=0; i<pgrp->num_chunks; i++)
            {
                ptag->id = cblk_cg_get_next_chunk_id(id);
                rc = cblk_aresult(ptag->id, &ptag->tag, status, tflags);
                if (rc>0)  {goto done;}
                if (rc<0 && errno==ENOENT) {rc=0;}
            }
        }
    }
    else
    {
        rc = cblk_aresult(ptag->id, &ptag->tag, status, flags);
    }

done:
    if (rc<0) {CBLK_TRACE_LOG_FILE(2,"FFDC id:%d rc:%d", id, rc);}
    return rc;
}
