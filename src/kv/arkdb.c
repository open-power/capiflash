/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arkdb.c $                                              */
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
#include <revtags.h>
REVISION_TAGS(arkdb);
#endif
#include <ark.h>

KV_Trace_t  arkdb_kv_trace;
KV_Trace_t *pAT             = &arkdb_kv_trace;
uint32_t    kv_inject_flags = 0;
uint64_t    ARK_MAX_LEN     = UINT64_C(0x1900000000); //100gb

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_persistence_calc(_ARK *_arkp)
{
  uint64_t pers_cs_bytes  = 0;
  uint64_t size           = 0;
  uint64_t size_per_thd   = 0;
  uint64_t bcount         = 0;
  uint64_t bcount_per_thd = 0;
  EA      *eap            = NULL;
  hash_t  *htp            = NULL;
  BL      *blp            = NULL;

  // We need to determine the total size of the data
  // that needs to be persisted.  Items that we persist
  // are:
  //
  // - Ark Config
  // - Hash Tables
  // - Block Lists

  // Ark Config
  pers_cs_bytes = sizeof(p_cntr_t) +
                  sizeof(P_ARK_t)  +
                 (sizeof(ark_stats_t)*_arkp->nthrds);

  // Hash Tables
  htp           = hash_new(_arkp->hcount, _arkp->blkbits);
  size_per_thd  = sizeof(hash_t);
  size_per_thd += htp->iv->words * sizeof(uint64_t);
  hash_free(htp);
  pers_cs_bytes += size_per_thd * _arkp->nthrds;

  // Block Lists - calculate for a full ark
  eap = ea_new(_arkp->devs[0], _arkp->bsize, ARK_MAX_BASYNCS, &size, &bcount,0);
  ea_free(eap);
  bcount_per_thd = bcount / _arkp->nthrds;
  size_per_thd   = bcount_per_thd;
  size_per_thd  += sizeof(BL);
  blp            = bl_new(bcount_per_thd, _arkp->blkbits);
  size_per_thd  += blp->iv->words * sizeof(uint64_t);
  bl_free(blp);
  pers_cs_bytes += size_per_thd * _arkp->nthrds;

  // round up to 4k
  _arkp->pers_max_blocks = divup(pers_cs_bytes, _arkp->bsize);

  KV_TRC_PERF1(pAT, "PERSIST_CALC pers_cs_bytes:%ld pers_max_blocks:%ld",
                  pers_cs_bytes, _arkp->pers_max_blocks);
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_persist(_ARK *_arkp)
{
  int32_t        rc          = 0;
  int32_t        i           = 0;
  uint64_t       tot_bytes   = 0;
  uint64_t       wrblks      = 0;
  char          *p_data_orig = NULL;
  char          *p_data      = NULL;
  p_cntr_t      *pptr        = NULL;
  char          *dptr        = NULL;
  P_ARK_t       *pcfg        = NULL;
  ark_io_list_t *bl_array    = NULL;
  uint64_t       size        = 0;
  scb_t         *scbp        = NULL;
  vbe_t         *vbep        = NULL;

  ark_persistence_calc(_arkp);

  // allocate write buffer
  tot_bytes   = _arkp->pers_max_blocks * _arkp->bsize;
  p_data_orig = am_malloc(tot_bytes + ARK_ALIGN);
  if (!p_data_orig)
  {
      KV_TRC_FFDC(pAT, "ENOMEM bytes:%ld for persist write", tot_bytes);
      rc = ENOMEM;
      goto done;
  }
  p_data = ptr_align(p_data_orig);
  memset(p_data, 0, tot_bytes);
  KV_TRC_PERF1(pAT, "PERSIST p_data tot_bytes:%ld",tot_bytes);

  // Record cntr data
  pptr   = (p_cntr_t *)p_data;
  memcpy(pptr->p_cntr_magic, ARK_P_MAGIC, sizeof(pptr->p_cntr_magic));
  pptr->p_cntr_version = ARK_P_VERSION_4;
  pptr->p_cntr_size    = sizeof(p_cntr_t);

  // Record configuration info
  pptr->p_cntr_cfg_offset = 0;
  pptr->p_cntr_cfg_size   = sizeof(P_ARK_t);
  pcfg                    = (P_ARK_t*)pptr->p_cntr_data;
  pcfg->flags             = _arkp->flags;
  pcfg->max_blocks        = _arkp->pers_max_blocks;
  pcfg->bsize             = _arkp->bsize;
  pcfg->blkbits           = _arkp->blkbits;
  pcfg->grow              = _arkp->grow;
  pcfg->hcount            = _arkp->hcount;
  pcfg->vlimit            = _arkp->vlimit;
  pcfg->nasyncs           = _arkp->nasyncs;
  pcfg->basyncs           = _arkp->basyncs;
  pcfg->ntasks            = _arkp->ntasks;
  pcfg->nthrds            = _arkp->nthrds;

  // Record stats info
  pptr->p_cntr_st_offset = pptr->p_cntr_cfg_size;
  dptr                   = pptr->p_cntr_data + pptr->p_cntr_st_offset;
  pptr->p_cntr_st_size   = 0;
  for (i=0; i<_arkp->nthrds; i++)
  {
      scbp = &_arkp->poolthreads[i];
      size = sizeof(ark_stats_t);
      memcpy(dptr, &scbp->poolstats, size);
      KV_TRC(pAT, "PERSIST_WR stats[%3d] offset:%12ld len:%ld",
                      i, dptr-(char*)pptr, size);
      pptr->p_cntr_st_size += size;
      dptr                 += size;
  }

  // Record hash info
  pptr->p_cntr_ht_offset = pptr->p_cntr_cfg_size +
                           pptr->p_cntr_st_size;
  dptr                   = pptr->p_cntr_data + pptr->p_cntr_ht_offset;
  pptr->p_cntr_ht_size   = 0;
  for (i=0; i<_arkp->nthrds; i++)
  {
      scbp = &_arkp->poolthreads[i];
      size = scbp->ht->iv->words * sizeof(uint64_t);
      memcpy(dptr, scbp->ht->iv->data, size);
      KV_TRC(pAT, "PERSIST_WR ht[%3d] offset:%12ld len:%ld",
                      i, dptr-(char*)pptr, size);
      pptr->p_cntr_ht_size += size;
      dptr                 += size;
  }

  // Record block list info
  pptr->p_cntr_bl_offset = pptr->p_cntr_cfg_size +
                           pptr->p_cntr_st_size  +
                           pptr->p_cntr_ht_size;
  dptr                   = pptr->p_cntr_data + pptr->p_cntr_bl_offset;
  pptr->p_cntr_bl_size   = 0;
  for (i=0; i<_arkp->nthrds; i++)
  {
      scbp = &_arkp->poolthreads[i];
      size = sizeof(BL)-sizeof(void*);
      memcpy(dptr, scbp->bl, size);
      pptr->p_cntr_bl_size += size;
      KV_TRC(pAT, "PERSIST_WR bl[%3d] offset:%12ld len:%ld",
                      i, dptr-(char*)pptr, size);
      dptr += size;

      size      = divup((scbp->bl->top+5) * scbp->bl->w, 8);
      vbep      = (vbe_t*)dptr;
      vbep->len = size;
      memcpy(vbep->p, scbp->bl->iv->data, vbep->len);
      pptr->p_cntr_bl_size += vbep->len;
      KV_TRC(pAT, "PERSIST_WR iv[%3d] offset:%12ld len:%ld",
                      i, dptr-(char*)pptr, vbep->len);
      dptr += sizeof(vbep->len) + vbep->len;
  }

  // Calculate wrblks: number of persist metadata blocks to write
  tot_bytes = pptr->p_cntr_cfg_size +
              pptr->p_cntr_st_size  +
              pptr->p_cntr_ht_size  +
              pptr->p_cntr_bl_size;
  wrblks    = pcfg->pblocks = divup(tot_bytes, _arkp->bsize);

  KV_TRC_PERF1(pAT, "PERSIST_WR wrblks:%ld vs pers_max_blocks:%ld",
                  wrblks, _arkp->pers_max_blocks);

  bl_array = bl_chain_no_bl(wrblks,0);
  if (!bl_array)
  {
      KV_TRC_FFDC(pAT, "ENOMEM blocks:%ld for write", wrblks);
      rc = ENOMEM;
      goto done;
  }
  else
  {
      rc = ea_async_io(_arkp->poolthreads->ea, ARK_EA_WRITE, p_data,
                       bl_array, wrblks);
      am_free(bl_array);
      if (rc != 0)
      {
          KV_TRC_FFDC(pAT, "PERSIST VERSION_4 STORE FAILED rc:%d", rc);
          goto done;
      }
  }

  KV_TRC_PERF1(pAT, "PERSIST ARK_META size %ld bsize %ld hcount %ld "
              "nthrds %d nasyncs %ld basyncs %d blkbits %ld",
              _arkp->size, _arkp->bsize, _arkp->hcount,
              _arkp->nthrds, _arkp->nasyncs, _arkp->basyncs, _arkp->blkbits);

  KV_TRC_PERF1(pAT, "PERSIST pcfg     size %ld bsize %ld hcount %ld "
              "nthrds %d nasyncs %d basyncs %d blkbits %ld version:%ld",
              pcfg->size, pcfg->bsize, pcfg->hcount,
              pcfg->nthrds, pcfg->nasyncs, pcfg->basyncs, pcfg->blkbits,
              pptr->p_cntr_version);

  KV_TRC_FFDC(pAT, "PERSIST VERSION_4 STORED");

done:
  am_free(p_data_orig);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_persist_load(_ARK *_arkp)
{
    int32_t        rc          = -1;
    char          *p_data_orig = NULL;
    char          *p_data      = NULL;
    ark_io_list_t *bl_array    = NULL;
    p_cntr_t      *pptr        = NULL;
    P_ARK_t       *pcfg        = NULL;
    EA            *eap         = NULL;
    uint64_t      rdblks       = 0;
    uint64_t      size         = 0;
    uint64_t      bcount       = 0;

    KV_TRC_PERF1(pAT, "PERSIST LOAD %s", _arkp->devs[0]);

    // Create the KV storage, whether that will be memory based or flash
    eap = ea_new(_arkp->devs[0], _arkp->bsize, ARK_MAX_BASYNCS, &size, &bcount,0);
    if (!eap)
    {
        rc = errno;
        goto error_exit;
    }

    // malloc a single block for read
    p_data_orig = am_malloc(_arkp->bsize + ARK_ALIGN);
    if (!p_data_orig)
    {
        KV_TRC_FFDC(pAT, "Out of memory allocating %ld bytes for the first "
                         "persistence block", _arkp->bsize);
        rc = ENOMEM;
        goto error_exit;
    }

    // read first block
    p_data   = ptr_align(p_data_orig);
    bl_array = bl_chain_no_bl(1, 0);
    rc       = ea_async_io(eap, ARK_EA_READ, p_data, bl_array, 1);
    am_free(bl_array);
    if (rc != 0) {goto error_exit;}

    // check if persist data is valid
    pptr = (p_cntr_t *)p_data;
    if (memcmp(pptr->p_cntr_magic,ARK_P_MAGIC,sizeof(pptr->p_cntr_magic) != 0))
    {
        KV_TRC_FFDC(pAT, "No magic number found in persistence data: %d", EINVAL);
        // The magic number does not match so data is either
        // not present or is corrupted.
        rc = EINVAL;
        goto error_exit;
    }

    // check version
    if (pptr->p_cntr_version != ARK_P_VERSION_4)
    {
        KV_TRC_FFDC(pAT, "The persist data has a version:%ld, which is not supported "
                         "by this level of Arkdb software. This software supports "
                         "version:%d",
                         pptr->p_cntr_version, ARK_P_VERSION_4);
        rc = EINVAL;
        goto error_exit;
    }

    // Read in the rest of the persistence data
    pcfg   = (P_ARK_t *)(pptr->p_cntr_data + pptr->p_cntr_cfg_offset);
    rdblks = pcfg->pblocks;
    if (rdblks > 1)
    {
        p_data_orig = am_realloc(p_data_orig, (rdblks*_arkp->bsize)+ARK_ALIGN);
        if (!p_data_orig)
        {
            KV_TRC_FFDC(pAT, "Out of memory allocating %ld bytes for "
                            "full persistence block", rdblks * _arkp->bsize);
            rc = ENOMEM;
            goto error_exit;
        }

        p_data   = ptr_align(p_data_orig);
        bl_array = bl_chain_no_bl(rdblks,0);
        if (!bl_array)
        {
            KV_TRC_FFDC(pAT, "Out of memory allocating %ld blocks for "
                             "full persistence data", rdblks);
            rc = ENOMEM;
            goto error_exit;
        }
    }
    KV_TRC_PERF1(pAT, "PERSIST_RD rdblks:%ld", rdblks);
    rc = ea_async_io(eap, ARK_EA_READ, (void *)p_data, bl_array, rdblks);
    am_free(bl_array);
    if (rc != 0) {goto error_exit;}

    // read was successful, parse the data
    pptr = (p_cntr_t *)p_data;
    pcfg = (P_ARK_t *)(pptr->p_cntr_data + pptr->p_cntr_cfg_offset);

    KV_TRC_FFDC(pAT, "PERSIST_VERSION_4 LOADED");
    KV_TRC_PERF1(pAT, "PERSIST pcfg     size %ld bsize %ld hcount %ld "
                "nthrds %d nasyncs %d basyncs %d blkbits %ld version:%ld",
                pcfg->size, pcfg->bsize, pcfg->hcount,
                pcfg->nthrds, pcfg->nasyncs, pcfg->basyncs, pcfg->blkbits,
                pptr->p_cntr_version);

    _arkp->persload        = 1;
    _arkp->size            = pcfg->size;
    _arkp->bsize           = pcfg->bsize;
    _arkp->blkbits         = pcfg->blkbits;
    _arkp->grow            = pcfg->grow;
    _arkp->hcount          = pcfg->hcount;
    _arkp->vlimit          = pcfg->vlimit;
    _arkp->nasyncs         = pcfg->nasyncs;
    _arkp->basyncs         = pcfg->basyncs;
    _arkp->ntasks          = pcfg->ntasks;
    _arkp->nthrds          = pcfg->nthrds;
    _arkp->pers_max_blocks = pcfg->max_blocks;

    KV_TRC_PERF1(pAT, "PERSIST ARK_META size %ld bsize %ld hcount %ld "
                "nthrds %d nasyncs %ld basyncs %d blkbits %ld",
                _arkp->size, _arkp->bsize, _arkp->hcount,
                _arkp->nthrds, _arkp->nasyncs, _arkp->basyncs, _arkp->blkbits);

    _arkp->persist_orig = p_data_orig;
    _arkp->persist      = pptr;
    goto success;

error_exit:
  ea_free(eap);
  am_free(p_data_orig);

success:
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_persist_restore(_ARK *_arkp)
{
    p_cntr_t *pptr = _arkp->persist;
    scb_t    *scbp = NULL;
    char     *dptr = NULL;
    vbe_t    *vbep = NULL;
    int       i    = 0;
    uint64_t  size = 0;

    dptr = pptr->p_cntr_data + pptr->p_cntr_st_offset;
    size = pptr->p_cntr_st_size / _arkp->nthrds;
    for (i=0; i<_arkp->nthrds; i++)
    {
        scbp = &_arkp->poolthreads[i];
        memcpy(&scbp->poolstats, dptr, size);
        KV_TRC(pAT, "PERSIST_RES stats[%3d] offset:%12ld len:%ld",
                        i, dptr-(char*)pptr, size);
        dptr += size;
    }

    dptr = pptr->p_cntr_data + pptr->p_cntr_ht_offset;
    size = pptr->p_cntr_ht_size / _arkp->nthrds;
    for (i=0; i<_arkp->nthrds; i++)
    {
        scbp = &_arkp->poolthreads[i];
        memcpy(scbp->ht->iv->data, dptr, size);
        KV_TRC(pAT, "PERSIST_RES ht[%3d] offset:%12ld len:%ld",
                        i, dptr-(char*)pptr, size);
        dptr += size;
    }

    dptr = pptr->p_cntr_data + pptr->p_cntr_bl_offset;
    for (i=0; i<_arkp->nthrds; i++)
    {
        scbp = &_arkp->poolthreads[i];
        size = sizeof(BL) - sizeof(void*);
        memcpy(scbp->bl, dptr, size);
        KV_TRC(pAT, "PERSIST_RES bl[%3d] offset:%12ld len:%ld",
                        i, dptr-(char*)pptr, size);
        dptr += size;

        vbep = (vbe_t*)dptr;
        memcpy(scbp->bl->iv->data, vbep->p, vbep->len);
        KV_TRC(pAT, "PERSIST_RES iv[%3d] offset:%12ld len:%ld",
                        i, dptr-(char*)pptr, vbep->len);
        dptr += sizeof(vbep->len) + vbep->len;
    }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_thd_free_res(_ARK *_arkp, int tid)
{
    scb_t *scbp = &_arkp->poolthreads[tid];
    int    j    = 0;
    int    rnum = 0;

    if (!_arkp) {return;}

    KV_TRC_DBG(pAT, "free inb/oub/vb");
    for (j=0; j<_arkp->ntasks && scbp->tcbs; j++)
    {
        bt_delete(scbp->tcbs[j].inb_orig);
        bt_delete(scbp->tcbs[j].oub_orig);
        am_free  (scbp->tcbs[j].vb_orig);
        am_free  (scbp->tcbs[j].aiol);
    }
    KV_TRC_DBG(pAT, "free tcbs");
    am_free(scbp->tcbs);

    KV_TRC_DBG(pAT, "free iocbs");
    am_free(scbp->iocbs);

    KV_TRC_DBG(pAT, "destroy rcb cond/lock");
    for (rnum=0; rnum<_arkp->nasyncs && scbp->rcbs; rnum++)
    {
        pthread_cond_destroy(&(scbp->rcbs[rnum].acond));
        pthread_mutex_destroy(&(scbp->rcbs[rnum].alock));
    }

    KV_TRC_DBG(pAT, "free rcbs");
    if (scbp->rcbs) {am_free(scbp->rcbs);}

    KV_TRC_DBG(pAT, "free rtags");
    if (scbp->rtags) {tag_free(scbp->rtags);}

    KV_TRC_DBG(pAT, "free ttags");
    if (scbp->ttags) {tag_free(scbp->ttags);}

    KV_TRC_DBG(pAT, "free ht");
    hash_free(scbp->ht);

    KV_TRC_DBG(pAT, "free bl");
    if (scbp->bl) {bl_free(scbp->bl);}

    queue_free(scbp->reqQ);
    queue_free(scbp->cmpQ);
    queue_free(scbp->taskQ);
    queue_free(scbp->scheduleQ);
    queue_free(scbp->harvestQ);

    pthread_mutex_destroy(&(scbp->lock));

    KV_TRC_DBG(pAT, "ea_delete");
    ea_free(scbp->ea);

    if (scbp->htc_orig)
    {
        KV_TRC_DBG(pAT, "free htc");
        for (j=0; j<scbp->hcount; j++) {am_free(scbp->htc[j].orig);}
        am_free(scbp->htc_orig);
    }
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_free_resources(_ARK  *_arkp)
{
  int i = 0;

  if (!_arkp) {return;}

  for (i=0; i<_arkp->nthrds && _arkp->poolthreads; i++)
  {
      ark_thd_free_res(_arkp, i);
  }

  KV_TRC_DBG(pAT, "free arkp->poolthreads");
  if (_arkp->poolthreads) {am_free(_arkp->poolthreads);}

  KV_TRC_DBG(pAT, "free arkp->pts");
  if (_arkp->pts) {am_free(_arkp->pts);}

  KV_TRC(pAT, "done, free arkp %p", _arkp);
  am_free(_arkp);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_thd_init_res(_ARK *ark, int tid)
{
    char    *dev    = NULL;
    int      rc     = -1;
    scb_t   *scbp   = &ark->poolthreads[tid];
    uint64_t sz     = 0;
    uint64_t bcount = 0;

    memset(scbp, 0, sizeof(scb_t));

    dev          = ark->devs[tid % ark->devN];
    scbp->size   = ark->size / ark->nthrds;
    scbp->hcount = ark->hcount_per_thd;

    scbp->ea = ea_new(dev, ark->bsize, ark->basyncs, &scbp->size, &bcount,
                      (ark->flags & ARK_KV_VIRTUAL_LUN));
    if (!scbp->ea)
    {
        if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
        rc = errno;
        KV_TRC_FFDC(pAT, "KV storage initialization failed: rc/errno:%d", rc);
        goto error;
    }

    if (ark->flags & ARK_KV_VIRTUAL_LUN)
    {
        scbp->bcount = bcount;
        KV_TRC_PERF2(pAT,"VLUN dev:%s tid:%d size:%ld bcount:%ld",
               dev, tid, scbp->size, scbp->bcount);
    }
    else
    {
        if (scbp->ea->st_type == EA_STORE_TYPE_MEMORY)
        {
            scbp->bcount = bcount;
        }
        else
        {
            // plun to file or device
            scbp->bcount = bcount / ark->nthrds;
            scbp->size  /= ark->nthrds;
            scbp->offset = ark->pers_max_blocks + (scbp->bcount * tid);
        }

        KV_TRC_PERF2(pAT,"PLUN dev:%s tid:%3d size:%ld bcount:%ld offset:%ld",
               dev, tid, scbp->size, scbp->bcount, scbp->offset);
    }

    if (ark->flags & ARK_KV_HTC)
    {
        sz = scbp->hcount*sizeof(htc_t);
        KV_TRC_PERF2(pAT,"htc sz:%ld hcount:%ld", sz, scbp->hcount);
        scbp->htc_orig = am_malloc(sz + ARK_ALIGN);
        if (!scbp->htc_orig)
        {
            rc = ENOMEM;
            KV_TRC_FFDC(pAT, "HTC initialization failed: %d", rc);
            goto error;
        }
        scbp->htc_blks = 1;
        scbp->htc      = ptr_align(scbp->htc_orig);
        memset(scbp->htc,0,sz);
    }
    else {scbp->htc_blks = 0;}

    // Create the hash table
    scbp->ht = hash_new(scbp->hcount, ark->blkbits);
    if (scbp->ht == NULL)
    {
        if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
        rc = errno;
        KV_TRC_FFDC(pAT, "Hash initialization failed: %d", rc);
        goto error;
    }

    // Create the block list
    scbp->bl = bl_new(scbp->bcount, ark->blkbits);
    if (scbp->bl == NULL)
    {
        if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
        rc = errno;
        KV_TRC_FFDC(pAT, "Block list initialization failed: %d", rc);
        goto error;
    }

    scbp->ttags = tag_new(ark->ntasks);
    if ( NULL == scbp->ttags )
    {
        rc = ENOMEM;
        KV_TRC_FFDC(pAT, "Tag initialization for tasks failed: %d", rc);
        goto error;
    }

    sz         = ark->ntasks * sizeof(tcb_t);
    scbp->tcbs = am_malloc(sz);
    if (!scbp->tcbs)
    {
        rc = ENOMEM;
        KV_TRC_FFDC(pAT, "Out of memory allocation of %ld bytes for tcbs", sz);
        goto error;
    }
    memset(scbp->tcbs,0,sz);

    /*-------------------------------------------------------------------------
     * loop for ntasks resources
     *-----------------------------------------------------------------------*/
    int num=0;
    for (num=0; num<ark->ntasks; num++)
    {
        tcb_t *tcbp = &scbp->tcbs[num];

        tcbp->inb = bt_new(ark->min_bt, ark->vlimit, VDF_SZ, &tcbp->inb_orig);
        if (tcbp->inb == NULL)
        {
          if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
          rc = errno;
          KV_TRC_FFDC(pAT, "Bucket allocation for inbuffer failed: %d", rc);
          goto error;
        }
        tcbp->inb_size = ark->min_bt;

        tcbp->oub = bt_new(ark->min_bt, ark->vlimit, VDF_SZ,
                                     &(tcbp->oub_orig));
        if (tcbp->oub == NULL)
        {
          if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
          rc = errno;
          KV_TRC_FFDC(pAT, "Bucket allocation for outbuffer failed: %d", rc);
          goto error;
        }
        tcbp->oub_size = ark->min_bt;

        tcbp->vbsize  = ark->bsize * ARK_MIN_VB;
        tcbp->vb_orig = am_malloc(tcbp->vbsize +ARK_ALIGN);
        if (tcbp->vb_orig == NULL)
        {
          rc = ENOMEM;
          KV_TRC_FFDC(pAT, "Out of memory bytes:%ld for tcb->vb",
                      tcbp->vbsize);
          goto error;
        }
        tcbp->vb = ptr_align(tcbp->vb_orig);
        KV_TRC_DBG(pAT, "num:%d VAB:%p %ld", num, tcbp->vb_orig, tcbp->vbsize);
    }

    sz          = ark->ntasks * sizeof(iocb_t);
    scbp->iocbs = am_malloc(sz);
    if ( NULL == scbp->iocbs )
    {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory allocation of %ld bytes for iocbs", sz);
      goto error;
    }
    memset(scbp->iocbs,0,sz);

    sz         = ark->nasyncs * sizeof(rcb_t);
    scbp->rcbs = am_malloc(sz);
    if (!scbp->rcbs)
    {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory bytes:%ld for rcbs",sz);
      goto error;
    }
    memset(scbp->rcbs,0,sz);

    for (num=0; num<ark->nasyncs; num++)
    {
        scbp->rcbs[num].stat = A_NULL;
        pthread_cond_init (&(scbp->rcbs[num].acond), NULL);
        pthread_mutex_init(&(scbp->rcbs[num].alock), NULL);
    }

    scbp->rtags = tag_new(ark->nasyncs);
    if (!scbp->rtags)
    {
        rc = ENOMEM;
        KV_TRC_FFDC(pAT, "Tag initialization for requests failed: %d", rc);
        goto error;
    }

    scbp->poolstate          = PT_RUN;
    scbp->poolstats.io_cnt   = 0;
    scbp->poolstats.ops_cnt  = 0;
    scbp->poolstats.kv_cnt   = 0;
    scbp->poolstats.blk_cnt  = 0;
    scbp->poolstats.byte_cnt = 0;

    pthread_mutex_init(&(scbp->lock), NULL);

    scbp->reqQ       = queue_new(ark->nasyncs);
    scbp->cmpQ       = queue_new(ark->ntasks);
    scbp->taskQ      = queue_new(ark->ntasks);
    scbp->scheduleQ  = queue_new(ark->ntasks);
    scbp->harvestQ   = queue_new(ark->ntasks);

    KV_TRC_PERF2(pAT, "THD:%d size:%ld bcount:%ld hcount:%ld avail:%ld errno:%d",
                    tid, scbp->size, scbp->bcount, scbp->hcount,
                    scbp->bl->n-scbp->bl->head, errno);

    rc=0;

error:
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_thd_start_all(_ARK *_arkp)
{
    int    rc    = 0;
    int    i     = 0;
    scb_t *scbp  = NULL;

    KV_TRC_DBG(pAT, "create threads");

    for (i=0; i<_arkp->nthrds; i++)
    {
        PT *pt  = &_arkp->pts[i];
        scbp    = &_arkp->poolthreads[i];
        pt->id  = i;
        pt->ark = _arkp;
        rc      = pthread_create(&(scbp->pooltid), NULL, pool_function, pt);
        if (rc != 0)
        {
          KV_TRC_FFDC(pAT, "pthread_create of server thread failed: %d", rc);
          goto ark_create_poolloop_err;
        }
        KV_TRC_DBG(pAT, "create thread%d success",i);
    }

    goto success;

ark_create_poolloop_err:

      for (; --i >= 0; i--)
      {
        scbp = &_arkp->poolthreads[i];

        KV_TRC_DBG(pAT, "abort/join pt:%d", i);

        if (scbp->pooltid)
        {
            queue_lock  (scbp->reqQ);
            queue_wakeup(scbp->reqQ);
            queue_unlock(scbp->reqQ);
            pthread_join(scbp->pooltid, NULL);
        }
      }

success:
    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_log_stats(ARK *ark)
{
    _ARK    *_arkp  = (_ARK*)ark;
    uint64_t size   = 0;
    uint64_t inuse  = 0;
    uint64_t actual = 0;
    int64_t  count  = 0;

    if (!ark) {return;}

    ark_allocated(ark, &size);
    ark_inuse    (ark, &inuse);
    ark_actual   (ark, &actual);
    ark_count    (ark, &count);

    KV_TRC_FFDC(pAT, "PERSIST max_blocks:%ld size:%ld inuse:%ld actual:%ld "
                     "count:%ld",
                _arkp->pers_max_blocks*_arkp->bsize,size,inuse,actual,count);
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_create_verbose(char *path, ARK **arkret,
                       uint64_t size, uint64_t bsize, uint64_t hcount,
                       int nthrds, int nqueue, int basyncs, uint64_t flags)
{
  _ARK    *ark           = NULL;
  int      rc            = 0;
  int      i             = 0;
  uint64_t am_sz         = 0;
  int      vlun          = flags & ARK_KV_VIRTUAL_LUN;
  int      persist_load  = flags & ARK_KV_PERSIST_LOAD;
  int      persist_store = flags & ARK_KV_PERSIST_STORE;
  int      mem_store     = path == NULL;

  KV_TRC_OPEN(pAT, "arkdb");
  KV_TRC_PERF2(pAT, "path(%s) size %ld bsize %ld hcount %ld "
              "nthrds %d nqueue %d basyncs %d flags:%08lx",
              path, size, bsize, hcount, nthrds, nqueue, basyncs, flags);

  if (!arkret)
  {
      KV_TRC_FFDC(pAT, "Incorrect value for ARK control block: rc=EINVAL");
      rc = EINVAL;
      goto ark_create_ark_err;
  }

  if ((persist_load || persist_store) && (mem_store || vlun))
  {
      KV_TRC_FFDC(pAT, "Invalid persistence combination with ARK flags:%016lx "
                       "or store type path:%s",flags,path);
      rc = EINVAL;
      goto ark_create_ark_err;
  }
  if (nthrds <= 0)
  {
      KV_TRC_FFDC(pAT, "invalid nthrds:%d", nthrds);
      rc = EINVAL;
      goto ark_create_ark_err;
  }
  // running to memory as plun, force minimum size to 1g
  if ((!path || strlen(path)==0) && !vlun && size < ARK_VERBOSE_PLUN_SIZE_DEF)
  {
      KV_TRC_FFDC(pAT, "INFO:   using memory in plun mode, forcing size=1g");
      size = size<ARK_VERBOSE_PLUN_SIZE_DEF ? ARK_VERBOSE_PLUN_SIZE_DEF : size;
  }

  am_sz = sizeof(_ARK);
  ark   = am_malloc(am_sz);
  if (ark == NULL)
  {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory allocating ARK control structure for %ld",
                sizeof(_ARK));
      goto ark_create_ark_err;
  }
  memset(ark,0,am_sz);

  KV_TRC_FFDC(pAT, "%p path(%s) size %ld bsize %ld hcount %ld "
              "nthrds %d nqueue %d basyncs %d flags:%08lx",
              ark, path, size, bsize, hcount, nthrds, nqueue, basyncs, flags);

  /*---------------------------------------------------------------------------
   * check for '=', which is a corsa multi-port to the same lun
   *-------------------------------------------------------------------------*/
  char *pstr = NULL;
  char *dbuf = NULL;

  memset(ark->devs,0,sizeof(ark->devs));
  if (path && strlen(path) != 0)
  {
      if (strchr(path,'='))
      {
          if (flags & ARK_KV_VIRTUAL_LUN)
          {
              rc = EINVAL;
              KV_TRC_FFDC(pAT, "Bad path with VLUN %s", path);
              goto ark_create_ark_err;
          }
          dbuf = strdup(path);
          sprintf(dbuf,"%s",path);
          while ((pstr=strsep((&dbuf),"=")))
          {
              sprintf(ark->devs[ark->devN++],"%s",pstr);
              KV_TRC(pAT,"%d -> %s", ark->devN-1, ark->devs[ark->devN-1]);
          }
      }
      else
      {
          sprintf(ark->devs[ark->devN++],"%s",path);
      }
  }
  else
  {
      ark->devN=1;
  }

  nqueue = nqueue<ARK_MAX_NASYNCS ? nqueue : ARK_MAX_NASYNCS;

  ark->ns_per_tick         = time_per_tick(1000, 100);
  ark->flags               = flags;
  ark->bsize               = bsize;
  ark->rand_thdI           = 0;
  ark->persload            = 0;
  ark->astart              = 0;
  ark->ark_exit            = 0;
  ark->nactive             = 0;
  ark->pcmd                = PT_IDLE;

  /*------------------------------------------------------------------------
   * check if data was persisted for a previous arkdb
   *----------------------------------------------------------------------*/
  if (persist_load)
  {
      if ((rc=ark_persist_load(ark)) == 0)
      {
          KV_TRC_PERF1(pAT, "PERSIST: %p path(%s) size %ld bsize %ld hcount %ld "
                      "nthrds %d nqueue %ld basyncs %d blkbits %ld ",
                      ark, path, ark->size, ark->bsize, ark->hcount,
                      ark->nthrds, ark->nasyncs, ark->basyncs, ark->blkbits);
      }
      else
      {
          KV_TRC_FFDC(pAT, "Persistence check failed: %d", rc);
          goto ark_create_ark_err;
      }
  }
  else
  {
      KV_TRC(pAT, "NO PERSIST LOAD FLAG");
      ark->nthrds  = ark->devN   * divup(nthrds,ark->devN);
      ark->size    = ark->nthrds * divup(size,ark->nthrds);
      ark->nasyncs = nqueue <ARK_MIN_NASYNCS?ARK_MIN_NASYNCS:nqueue;
      ark->basyncs = basyncs<ARK_MIN_BASYNCS?ARK_MIN_BASYNCS:basyncs;
      ark->ntasks  = ARK_MAX_TASK_OPS;
      ark->hcount  = ark->nthrds * divup(hcount,ark->nthrds);
      ark->vlimit  = ARK_VERBOSE_VLIMIT_DEF;
      ark->blkbits = ARK_VERBOSE_BLKBITS_DEF;
      ark->grow    = ARK_VERBOSE_GROW_DEF;
  }

  ark->hcount_per_thd = ark->hcount / ark->nthrds;

  // if we are persisting for the first time, calc the max size for persist data
  if (persist_store && ark->pers_max_blocks==0) {ark_persistence_calc(ark);}

  KV_TRC_PERF1(pAT,"path:%s nthrds:%d hcount:%ld:%ld",
                  path, ark->nthrds, ark->hcount, ark->hcount_per_thd);

  ark->min_bt = ARK_MIN_BT*ark->bsize;

  am_sz            = ark->nthrds * sizeof(scb_t);
  ark->poolthreads = am_malloc(am_sz);
  if (!ark->poolthreads)
  {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory allocation of %ld bytes for scbs", am_sz);
      goto ark_create_ark_err;
  }
  memset(ark->poolthreads,0,am_sz);

  am_sz    = ark->nthrds * sizeof(PT);
  ark->pts = am_malloc(am_sz);
  if (!ark->pts)
  {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory bytes:%ld for pts", am_sz);
      goto ark_create_ark_err;
  }
  memset(ark->pts,0,am_sz);

  /*------------------------------------------------------------------------
   * init ark threads
   *----------------------------------------------------------------------*/
  for (i=0; i<ark->nthrds; i++)
  {
      if ((rc=ark_thd_init_res(ark,i)) != 0) {goto ark_create_ark_err;}
  }

  /*------------------------------------------------------------------------
   * restore the persisted data into the ark data structures
   *----------------------------------------------------------------------*/
  if (persist_load) {ark_persist_restore(ark);}

  /*------------------------------------------------------------------------
   * create ark threads
   *----------------------------------------------------------------------*/
  if ((rc=ark_thd_start_all(ark)) != 0) {goto ark_create_ark_err;}

  *arkret   = (void *)ark;
  ark->pcmd = PT_RUN;

  /* success, goto end */
  KV_TRC_PERF2(pAT,"DONE: ark:%p path(%s) type:%d size:%ld bsize:%ld hcount:%ld "
              "ntasks:%ld nthrds:%d nqueue:%ld basyncs:%d flags:%08lx rc:%d errno:%d",
              ark, path, ark->st_type, ark->size, ark->bsize, ark->hcount,
              ark->ntasks, ark->nthrds, ark->nasyncs,ark->basyncs,ark->flags,rc,errno);

  ark_log_stats(*arkret);

  goto done;

ark_create_ark_err:
  KV_TRC_FFDC(pAT, "ark_create_ark_err ark:%p", ark);
  ark_free_resources(ark);
  KV_TRC_CLOSE(pAT);

done:
  KV_TRC_DBG(pAT, "free arkp->persist_orig");
  if (ark) {am_free(ark->persist_orig);}

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_create(char *path, ARK **arkret, uint64_t flags)
{
  return ark_create_verbose(path, arkret,
                            ARK_VERBOSE_SIZE_DEF,
                            ARK_VERBOSE_BSIZE_DEF,
                            ARK_VERBOSE_HASH_DEF,
                            ARK_VERBOSE_NTHRDS_DEF,
                            ARK_MAX_NASYNCS,
                            ARK_MAX_BASYNCS,
                            flags);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_thd_stop_all(_ARK *_arkp)
{
  int    rc    = 0;
  int    i     = 0;
  scb_t *scbp  = NULL;

  KV_TRC_DBG(pAT, "stop all threads");

  // Wait for all active threads to exit
  for (i=0; i<_arkp->nthrds; i++)
  {
      scbp = &(_arkp->poolthreads[i]);
      scbp->poolstate = PT_EXIT;

      queue_lock  (scbp->reqQ);
      queue_wakeup(scbp->reqQ);
      queue_unlock(scbp->reqQ);

      pthread_join(scbp->pooltid, NULL);
      KV_TRC(pAT, "thread %d joined", i);
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_delete(ARK *ark)
{
  _ARK *_arkp = (_ARK *)ark;
  int   rc    = 0;

  if (!ark)
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "ERROR   Invalid ARK control block parameter: %d", rc);
    goto ark_delete_ark_err;
  }

  ark_thd_stop_all(_arkp);
  ark_log_stats(ark);

  if (_arkp->flags & ARK_KV_PERSIST_STORE) {rc=ark_persist(_arkp);}

  KV_TRC_DBG(pAT, "free thread resources");
  ark_free_resources(_arkp);

ark_delete_ark_err:
  KV_TRC_FFDC(pAT, "ERROR   %p",ark);
  KV_TRC_CLOSE(pAT);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_connect_verbose(ARC **arc, ARK *ark, int nasync)
{
  int rc = 0;
  if ((arc == NULL) || (ark == NULL))
  {
    rc = EINVAL;
  }
  else
  {
    *arc = ark;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_connect(ARC **arc, ARK *ark)
{
  return ark_connect_verbose(arc, ark, 128);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_disconnect(ARC *arc)
{
  int rc = 0;

  if (arc == NULL)
  {
    rc = EINVAL;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 * if successful then returns vlen else returns negative number error code
 ******************************************************************************/
int ark_set(ARK *ark, uint64_t klen, 
            void *key, uint64_t vlen, void *val, int64_t *rval)
{
  int     rc      = 0;
  int     errcode = 0;
  rcb_t  *rcb     = NULL;
  int64_t res     = 0;
  _ARK   *_arkp   = (_ARK *)ark;

  if ((_arkp == NULL) || klen < 0 || vlen < 0 ||
      ((klen > 0) && (key == NULL)) || (klen>ARK_MAX_LEN) ||
      ((vlen > 0) && (val == NULL)) || (vlen>ARK_MAX_LEN) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL: ark %p klen %ld "
                     "key %p vlen %ld val %p rval %p",
                     ark, klen, key, vlen, val, rval);
    return EINVAL;
  }

  ark_enq_cmd(K_SET, _arkp, klen,key,vlen,val,0,NULL,0, -1, NULL,&rcb);
  if (!rcb)
  {
      KV_TRC_IO(pAT, "EAGAIN");
      return EAGAIN;
  }

  // Will wait here for command to complete
  rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);
  if (rc == 0)
  {
      if (errcode != 0)
      {
          KV_TRC(pAT, "ERROR   rc:%d", errcode);
          rc = errcode;
      }

      *rval = res;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if success returns the size of value 
int ark_exists(ARK *ark, uint64_t klen, void *key, int64_t *rval)
{
  int     rc      = 0;
  int     errcode = 0;
  rcb_t  *rcb     = NULL;
  int64_t res     = 0;
  _ARK   *_arkp   = (_ARK *)ark;

  if ((_arkp == NULL) || klen < 0 ||
      ((klen > 0) && (key == NULL)) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL ark %p, klen %ld "
                     "key %p, rval %p", ark, klen, key, rval);
    return EINVAL;
  }

  ark_enq_cmd(K_EXI, _arkp, klen,key,0,0,0,NULL,0, -1,NULL,&rcb);
  if (!rcb)
  {
      KV_TRC_IO(pAT, "EAGAIN");
      return EAGAIN;
  }

  // Will wait here for command to complete
  rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);
  if (rc == 0)
  {
      if (errcode != 0)
      {
          KV_TRC(pAT, "ERROR   rc:%d", errcode);
          rc = errcode;
      }

      *rval = res;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 * if successful returns length of value
 * vbuf is filled with value starting at voff bytes in,
 * at most vbuflen bytes returned
 ******************************************************************************/
int ark_get(ARK *ark, uint64_t klen, void *key,
        uint64_t vbuflen, void *vbuf, uint64_t voff, int64_t *rval)
{
  int     rc      = 0;
  int     errcode = 0;
  rcb_t  *rcb     = NULL;
  int64_t res     = 0;
  _ARK   *_arkp   = (_ARK *)ark;

  if ((_arkp == NULL) ||
      ((klen > 0) && (key == NULL)) ||
      ((vbuflen > 0) && (vbuf == NULL)) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL: ark %p klen %ld key %p "
                     "vbuflen %ld vbuf %p rval %p",
                     _arkp, klen, key, vbuflen, vbuf, rval);
    return EINVAL;
  }

  ark_enq_cmd(K_GET,_arkp,klen,key,vbuflen,vbuf,voff,NULL,0,-1,NULL,&rcb);
  if (!rcb)
  {
      KV_TRC_IO(pAT, "EAGAIN");
      return EAGAIN;
  }

  // Will wait here for command to complete
  rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);
  if (rc == 0)
  {
      if (errcode != 0)
      {
          KV_TRC(pAT, "ERROR   rc:%d", errcode);
          rc = errcode;
      }

      *rval = res;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if success returns size of value deleted
int ark_del(ARK *ark, uint64_t klen, void *key, int64_t *rval)
{
  int     rc      = 0;
  int     errcode = 0;
  rcb_t  *rcb     = NULL;
  int64_t res     = 0;
  _ARK   *_arkp   = (_ARK *)ark;

  if (!_arkp || ((klen > 0) && !key) || !rval)
  {
      KV_TRC_FFDC(pAT, "ERROR   EINVAL: ark %p klen %ld key %p rval %p",
                      _arkp, klen, key, rval);
      return EINVAL;
  }

  ark_enq_cmd(K_DEL, _arkp, klen,key,0,0,0,NULL,0, -1,NULL,&rcb);
  if (!rcb)
  {
      KV_TRC_IO(pAT, "EAGAIN");
      return EAGAIN;
  }

  // Will wait here for command to complete
  rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);
  if (rc == 0)
  {
      if (errcode != 0)
      {
          KV_TRC(pAT, "ERROR   rc:%d", errcode);
          rc = errcode;
      }

      *rval = res;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_random(ARK *ark, uint64_t kbuflen, uint64_t *klen, void *kbuf)
{
  uint64_t i       = 0;
  int32_t  rc      = 0;
  int      errcode = 0;
  rcb_t   *rcb     = NULL;
  int64_t  res     = 0;
  _ARK    *_arkp   = (_ARK *)ark;
  int      start   = 1;

  if (!_arkp || 0 >= kbuflen || !klen || !kbuf)
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL ark %p,  kbuflen %ld, klen %p, kbuf %p",
                _arkp, kbuflen, klen, kbuf);
    rc = EINVAL;
    goto done;
  }

  *klen = -1;

  KV_TRC_DBG(pAT, "start   tid:%3d htI:%ld btI:%d",
                  _arkp->rand_thdI,
                  _arkp->poolthreads[_arkp->rand_thdI].rand_htI,
                  _arkp->poolthreads[_arkp->rand_thdI].rand_btI);

  // Because the keys are stored in a hash table, their order
  // by nature is random.  Therefore, let's just pick a pool
  // thread and see if it has any k/v pairs that it is monitoring
  // and return those.  THen move on to the next pool thread
  // once done with the first thread...and so on.
  for (i=0; i<_arkp->nthrds; i++)
  {
      ark_enq_cmd(K_RAND, _arkp, kbuflen, kbuf,
                      0,NULL,0,NULL,0, _arkp->rand_thdI, &start,&rcb);
      start = 0;
      if (!rcb)
      {
          KV_TRC_IO(pAT, "EAGAIN");
          rc = EAGAIN;
          goto done;
      }
      rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);
      if (rc == 0 && errcode == 0)
      {
          // found a key
          *klen = res;
          break;
      }
      else if (rc == 0 && errcode == EAGAIN)
      {
          // try the next pool thread and see if it has any keys
          KV_TRC_DBG(pAT, "errcode:%d thdI:%3d i:%ld", errcode,_arkp->rand_thdI,i);
          continue;
      }
      else
      {
          // error
          goto done;
      }
  }

  // If done is not set, that means we didn't find a single key/value pair.
  if (i == _arkp->nthrds)
  {
      rc = ENOENT;
      KV_TRC_DBG(pAT, "No more key/value pairs rc = %d", ENOENT);
  }

done:
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ARI *ark_first(ARK *ark, uint64_t kbuflen, int64_t *klen, void *kbuf)
{
  _ARK   *_arkp = (_ARK *)ark;
  _ARI   *_arip = NULL;
  int32_t rc    = 0;
  int     tid   = 0;
  int64_t kl    = -1;

  KV_TRC_DBG(pAT, "start   tid:%3d", 0);

  if (!_arkp || !klen || !kbuf || kbuflen == 0)
  {
      KV_TRC_FFDC(pAT, "ERROR   EINVAL: ark %p kbuflen %ld klen %p kbuf %p",
                      _arkp, kbuflen, klen, kbuf);
      errno = EINVAL;
      goto done;
  }

  _arip = am_malloc(sizeof(_ARI));
  if (!_arip)
  {
      KV_TRC_FFDC(pAT, "Out of memory bytes:%ld for ARI control block", sizeof(_ARI));
      *klen = -1;
      errno = ENOMEM;
      goto done;
  }
  memset(_arip,0,sizeof(_ARI));

  _arip->ark = _arkp;
  *klen      = -1;

  if ((rc=ark_next((ARI*)_arip, kbuflen, klen, kbuf)) != 0)
  {
      _arip = NULL;
  }
  else {tid=_arip->thdI;}

  kl = *klen;

done:
  KV_TRC_DBG(pAT, "DONE    tid:%3d klen:%ld arip:%p rc:%d", tid, kl, _arip, rc);
  return (ARI *)_arip;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_next(ARI *iter, uint64_t kbuflen, int64_t *klen, void *kbuf)
{
  _ARI    *_arip   = (_ARI *)iter;
  int32_t  rc      = 0;
  int      errcode = 0;
  rcb_t   *rcb     = 0;
  int64_t  res     = 0;
  _ARK    *_arkp   = NULL;
  int      found   = 0;
  int      start   = 1;

  if (!_arip || !_arip->ark || klen == 0 || !kbuf || kbuflen == 0)
  {
      KV_TRC_FFDC(pAT, "ERROR   EINVAL: ari %p kbuflen %ld klen %p kbuf %p",
                      _arip, kbuflen, klen, kbuf);
      rc = EINVAL;
      goto done;
  }

  _arkp = _arip->ark;
  *klen = -1;

  // Start with the thread we left off on last time and then loop to the end
  while (_arip->thdI < _arkp->nthrds)
  {
      KV_TRC_DBG(pAT, "ENQ_CMD tid:%3d", _arip->thdI);
      rc = ark_enq_cmd(K_NEXT, _arkp, kbuflen, kbuf,
                       0, _arip, 0, NULL, 0, _arip->thdI, &start, &rcb);
      start = 0;
      if (!rcb)
      {
          am_free(_arip);
          _arip = NULL;
          rc    = EAGAIN;
          KV_TRC_IO(pAT, "ark_next_async_tag failed rc = %d", rc);
          goto done;
      }

      rc = ark_wait_cmd(_arkp, rcb, &errcode, &res);

      _arip->vbep = rcb->vbep;
      _arip->cnt  = rcb->cnt;

      if (rc == 0 && errcode == 0)
      {
          found = 1;
          if (_arip->getN && rcb->full) {break;}
          else
          {
              // found a key
              *klen = res;
              break;
          }
      }
      else if (rc == 0 && errcode == EAGAIN)
      {
          if (_arip->getN && rcb->full) {break;}
          // try the next pool thread and see if it has any keys
          KV_TRC_DBG(pAT, "errcode:%d tid:%3d", errcode,_arip->thdI);
          continue;
      }
      else
      {
          // error
          am_free(_arip);
          _arip = NULL;
          goto done;
      }
  }

  if (!found)
  {
      am_free(_arip);
      _arip = NULL;
      rc    = ENOENT;
      KV_TRC_DBG(pAT, "No key/value pairs in the store: rc = %d", rc);
  }
  else
  {
      KV_TRC(pAT, "FOUND   tid:%3d full:%d", _arip->thdI, _arip->full);
  }

done:
  return rc;
}

/**
 *******************************************************************************
 * \brief
 * Get as many keys from the Arkdb as will fit into kbuf (kbuflen)
 * -num_keys is set to the number of keys that fit in kbuf
 ******************************************************************************/
int ark_nextN(ARI *iter, uint64_t kbuflen, void *kbuf, int64_t *num_keys)
{
    _ARI    *_arip = (_ARI *)iter;
    int32_t  rc    = 0;

    KV_TRC_DBG(pAT, "START");

    if (!iter || !_arip->ark || !num_keys || !kbuf || !kbuflen)
    {
        KV_TRC_FFDC(pAT, "ERROR   EINVAL: ari:%p ark:%p kbuflen:%ld "
                         "num_keys:%p kbuf:%p",
                        iter, _arip?_arip->ark:NULL, kbuflen, num_keys, kbuf);
        rc = EINVAL;
        goto done;
    }

    _arip->getN = 1;
    _arip->cnt  = 0;
    _arip->vbep = kbuf;
    _arip->full = 0;
    *num_keys   = 0;

    rc = ark_next(iter, kbuflen, num_keys, kbuf);
    *num_keys = _arip->cnt;

done:
    KV_TRC_DBG(pAT, "DONE    rc:%d num_keys:%ld", rc,num_keys?*num_keys:-1);
    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_null_async_cb(ARK *ark,
                      uint64_t klen, void *key,
                      void (*cb)(int errcode, uint64_t dt, int64_t res),
                      uint64_t dt)
{
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "ERROR   EINVAL, ark %p cb %p", ark, cb);
    return EINVAL;
  }

  _ARK *_arkp = (_ARK *)ark;
  return ark_enq_cmd(K_NULL,_arkp,klen,key,0,0,0,cb,dt,-1,NULL,NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_set_async_cb(ARK *ark,
                     uint64_t klen, void *key,
                     uint64_t vlen, void *val,
                     void (*cb)(int, uint64_t, int64_t),
                     uint64_t dt)
{
  if (NULL == ark || ((vlen > 0) && (val == NULL)) || (cb == NULL))
  {
    KV_TRC_FFDC(pAT, "ERROR   rc = EINVAL: vlen %ld, val %p, cb %p",
            vlen, val, cb);
    return EINVAL;
  }

  _ARK  *_arkp = (_ARK *)ark;
  return ark_enq_cmd(K_SET,_arkp,klen,key,vlen,val,0,cb,dt,-1,NULL,NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_get_async_cb(ARK *ark,
                     uint64_t klen,   void *key,
                     uint64_t vbuflen,void *vbuf, uint64_t voff,
                     void (*cb)(int errcode, uint64_t dt, int64_t res),
                     uint64_t dt)
{
  if (NULL == ark || ((vbuflen > 0) && (vbuf == NULL)) || (cb == NULL))
  {
    KV_TRC_FFDC(pAT, "ERROR   rc = EINVAL: vbuflen %ld, vbuf %p, cb %p",
            vbuflen, vbuf, cb);
    return EINVAL;
  }

  _ARK *_arkp = (_ARK*)ark;
  return ark_enq_cmd(K_GET,_arkp,klen,key,vbuflen,vbuf,voff,cb,dt,-1,NULL,NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_del_async_cb(ARK *ark,
                     uint64_t klen, void *key,
                     void (*cb)(int errcode, uint64_t dt, int64_t res),
                     uint64_t dt)
{
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "ERROR   rc = EINVAL: cb %p", cb);
    return EINVAL;
  }

  _ARK *_arkp = (_ARK *)ark;
  return ark_enq_cmd(K_DEL,_arkp,klen,key,0,0,0,cb,dt,-1,NULL,NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_exists_async_cb(ARK *ark,
                        uint64_t klen, void *key,
                        void (*cb)(int errcode, uint64_t dt, int64_t res),
                        uint64_t dt)
{
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "ERROR   rc = EINVAL: cb %p", cb);
    return EINVAL;
  }

  _ARK *_arkp = (_ARK *)ark;
  return ark_enq_cmd(K_EXI,_arkp,klen,key,0,0,0,cb,dt,-1,NULL,NULL);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_flush(ARK *ark)
{
  _ARK   *_arkp = (_ARK *)ark;
  int     i     = 0;
  int     rc    = 0;

  if (ark == NULL)
  {
      rc = EINVAL;
      KV_TRC_FFDC(pAT, "ERROR   rc = %d", rc);
      goto ark_flush_err;
  }

  KV_TRC(pAT, "ark:%p",ark);

  ark_thd_stop_all(_arkp);

  for (i=0; i<_arkp->nthrds; i++)
  {
      ark_thd_free_res(_arkp, i);
      if ((rc=ark_thd_init_res(_arkp, i))) {break;}
  }

  rc = ark_thd_start_all(_arkp);

ark_flush_err:
  KV_TRC(pAT, "ark:%p rc:%d errno:%d",ark,rc,errno);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
pid_t ark_fork(ARK *ark)
{
  scb_t *scbp = NULL;
  pid_t  cpid = -1;
  int    i    = 0;

  _ARK *_arkp = (_ARK *)ark;

  // Let's start with an error just incase we don't make it
  // to the fork() system call and the -1 will tell the
  // caller an error was encountered and a child process
  // was not created.

  if (ark == NULL)
  {
    KV_TRC_FFDC(pAT, "rERROR   c = %s", "EINVAL");
    return cpid;
  }

  for (i=0; i<_arkp->nthrds; i++)
  {
      // Tell the store to hold all "to be freed" blocks.  This
      // will be undid in ark_fork_done() or in the error
      // case of fork below.
      bl_hold(_arkp->poolthreads[i].bl);
  }

  cpid = fork();

  switch (cpid)
  {
    // Ran into an error, perform any cleanup before returning
    case -1 :
      for (i=0; i<_arkp->nthrds; i++)
      {
          // Tell the store to hold all "to be freed" blocks.  This
          // will be undid in ark_fork_done() or in the error
          // case of fork below.
          bl_release(_arkp->poolthreads[i].bl);
      }
      break;

    // This is the child process.
    case 0  :
    {
      // Need to do error checking for this block
      int i;

      _arkp->pcmd = PT_RUN;
      _arkp->ark_exit = 0;

      /* clear any waiters, they are the parent ark threads */
      for (i=0; i<_arkp->nthrds; i++)
      {
          scbp = &_arkp->poolthreads[i];
          scbp->reqQ->waiters=0;
          pthread_cond_init (&scbp->reqQ->cond,NULL);
          pthread_mutex_init(&scbp->reqQ->m,   NULL);
      }

      /* create new ark threads for the child */
      for (i=0; i<_arkp->nthrds; i++)
      {
          PT *pt          = am_malloc(sizeof(PT)); //TODO: handle malloc fail
          pt->id          = i;
          pt->ark         = _arkp;
          scbp            = &_arkp->poolthreads[i];
          scbp->poolstate = PT_OFF;
          pt->id          = i;
          pt->ark         = _arkp;
          pthread_mutex_init(&scbp->lock,NULL);
          pthread_create(&(scbp->pooltid), NULL, pool_function, pt);

          if (scbp->ea->st_type != EA_STORE_TYPE_MEMORY)
          {
              int c_rc = 0;

              c_rc = cblk_cg_clone_after_fork(scbp->ea->st_flash,
                                              O_RDONLY, CBLK_GROUP_RAID0);

              // If we encountered an error, force the child to
              // exit with a non-zero status code
              if (c_rc != 0)
              {
                  KV_TRC_FFDC(pAT, "ERROR   rc = %d", c_rc);
                  _exit(c_rc);
              }
          }
      }

      break;
    }

    // Parent process will go here, with cpid being the pid
    // of the child process.
    default :
      break;
  }

  return cpid;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_fork_done(ARK *ark)
{
  int rc = 0;
  int i  = 0;

  _ARK *_arkp = (_ARK *)ark;

  if (ark == NULL)
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "ERROR   rc = %s", "EINVAL");
    return rc;
  }

  for (i=0; i<_arkp->nthrds; i++)
  {
      bl_release(_arkp->poolthreads[i].bl);
  }
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_count(ARK *ark, int64_t *count)
{
    _ARK    *_arkp = (_ARK *)ark;
    uint32_t i     = 0;
    int32_t  rc    = 0;

    if (!_arkp || !count)
    {
        KV_TRC_FFDC(pAT, "ERROR   rc = EINVAL ark %p, count %p", _arkp, count);
        rc = EINVAL;
    }
    else
    {
        *count = 0;
        for (i=0; i<_arkp->nthrds; i++)
        {
            *count += _arkp->poolthreads[i].poolstats.kv_cnt;
        }
    }

    KV_TRC_DBG(pAT, "count:%ld rc:%d ark:%p", *count, rc, _arkp);

    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_inuse(ARK *ark, uint64_t *size)
{
  _ARK    *_arkp = (_ARK *)ark;
  int      rc    = 0;
  uint64_t i     = 0;

  if (!ark || !size)
  {
      rc = EINVAL;
      KV_TRC_FFDC(pAT, "ERROR   rc:%d ark:%p size:%p", rc,ark,size);
  }
  else
  {
      *size = _arkp->pers_max_blocks;
      for (i=0; i<_arkp->nthrds; i++)
      {
          *size += _arkp->poolthreads[i].poolstats.blk_cnt;
      }

      *size = *size * _arkp->bsize;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_allocated(ARK *ark, uint64_t *size)
{
    int      rc    = 0;
    uint64_t i     = 0;
    _ARK    *_arkp = (_ARK *)ark;

    if (!ark || !size)
    {
        rc = EINVAL;
        KV_TRC_FFDC(pAT, "ERROR   rc:%d ark:%p size:%p", rc,ark,size);
    }
    else
    {
        *size = _arkp->pers_max_blocks * _arkp->bsize;
        for (i=0; i<_arkp->nthrds; i++)
        {
            *size += _arkp->poolthreads[i].size;
        }
    }
    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_actual(ARK *ark, uint64_t *size)
{
    int32_t  rc    = 0;
    _ARK    *_arkp = (_ARK *)ark;
    uint64_t i;

    if (!ark || !size)
    {
        rc = EINVAL;
        KV_TRC_FFDC(pAT, "ERROR   rc:%d ark:%p size:%p", rc,ark,size);
    }
    else
    {
        *size = _arkp->pers_max_blocks * _arkp->bsize;
        for (i=0; i<_arkp->nthrds; i++)
        {
          *size += _arkp->poolthreads[i].poolstats.byte_cnt;
        }
    }

    return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_stats(ARK *ark, uint64_t *ops, uint64_t *ios)
{
    int   rc    = 0;
    int   i     = 0;
    _ARK *_arkp = (_ARK *)ark;

    if (!ark || !ops || !ios)
    {
        rc = EINVAL;
        KV_TRC_FFDC(pAT, "ERROR   rc = %d", rc);
    }
    else
    {
        *ops = 0;
        *ios = 0;

        // Go through the pool threads and collect each
        // threads counts
        for (i=0; i<_arkp->nthrds; i++)
        {
            *ops += _arkp->poolthreads[i].poolstats.ops_cnt;
            *ios += _arkp->poolthreads[i].poolstats.io_cnt;
        }
    }

    return rc;
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
void open_vlun_(char *path, ARK **ark)
{
    if (path && ark)
    {
        ark_create_verbose(path, ark,
                           ARK_VERBOSE_SIZE_DEF,
                           ARK_VERBOSE_BSIZE_DEF,
                           100*1024,
                           ARK_VERBOSE_NTHRDS_DEF,
                           ARK_MAX_NASYNCS,
                           ARK_MAX_BASYNCS,
                           ARK_KV_VIRTUAL_LUN | ARK_KV_HTC);
    }
    return;
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
void close_vlun_(ARK **ark)
{
    if (ark && *ark) {ark_delete(*ark);}
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
int write_vlun_(ARK **ark, uint64_t klen, void *key, uint64_t vlen, void *val)
{
    int     rc   = 0;
    int64_t rval = 0;

    if (!ark || !*ark || !key || !val) {return -1;}

    rc=ark_set(*ark,klen,key,vlen,val,&rval);

    return rc;
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
int read_vlun_(ARK **ark, uint64_t klen, void *key, uint64_t vbuflen, void *vbuf)
{
    int      rc   = 0;
    int64_t  rval = 0;
    uint64_t voff = 0;

    if (!ark || !*ark || !key || !vbuf) {return -1;}

    rc=ark_get(*ark,klen,key,vbuflen,vbuf,voff,&rval);

    return rc;
}

void dummy_cb(int errcode, uint64_t dt, int64_t res)
{
    if (dt)
    {
        uint64_t *done = (uint64_t*)dt;
        *done = 1;
    }
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
int awrite_vlun_(ARK **ark, uint64_t klen, void *key, uint64_t vlen, void *val,
                 uint64_t *done)
{
    int rc = 0;

    if (!ark || !*ark || !key || !val || !done) {return -1;}

    rc=ark_set_async_cb(*ark,klen,key,vlen,val,dummy_cb,(uint64_t)done);

    return rc;
}

/**
 *******************************************************************************
 * \brief
 * fortran interface
 ******************************************************************************/
int aread_vlun_(ARK **ark, uint64_t klen, void *key, uint64_t vbuflen, void *vbuf,
                 uint64_t *done)
{
    int rc = 0;

    if (!ark || !*ark || !key || !vbuf || !done) {return -1;}

    rc=ark_get_async_cb(*ark,klen,key,vbuflen,vbuf,0,dummy_cb,(uint64_t)done);

    return rc;
}
