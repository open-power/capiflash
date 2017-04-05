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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "am.h"
#include "ut.h"
#include "tag.h"
#include "vi.h"
#include "ea.h"
#include "bl.h"
#include "arkdb.h"
#include "ark.h"
#include "arp.h"
#include <ticks.h>

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include "capiblock.h"
#endif

#include <sys/stat.h>

#include <kv_trace.h>
#include <kv_inject.h>
#include <errno.h>

KV_Trace_t  arkdb_kv_trace;
KV_Trace_t *pAT             = &arkdb_kv_trace;
uint32_t    kv_inject_flags = 0;
uint32_t    do_mem_fastpath = 0;
uint64_t    ARK_MAX_LEN     = 0x1900000000; //100gb

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_persist_stats(_ARK * _arkp, ark_stats_t *pstats)
{
  int i = 0;
  
  pstats->kv_cnt = _arkp->pers_stats.kv_cnt;
  pstats->blk_cnt = _arkp->pers_stats.blk_cnt;
  pstats->byte_cnt = _arkp->pers_stats.byte_cnt;

  for (i = 0; i < _arkp->nthrds; i++)
  {
    pstats->kv_cnt += _arkp->poolthreads[i].poolstats.kv_cnt;
    pstats->blk_cnt += _arkp->poolthreads[i].poolstats.blk_cnt;
    pstats->byte_cnt += _arkp->poolthreads[i].poolstats.byte_cnt;
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
void ark_persistence_calc(_ARK *_arkp)
{
  uint64_t bytes = 0;

  // We need to determine the total size of the data
  // that needs to be persisted.  Items that we persist
  // are:
  //
  // - Configuration
  // - Hash Table (hash_t)
  // - Block List (BL)
  // - IV->data

  // Configuration
  _arkp->pers_cs_bytes = sizeof(p_cntr_t) + sizeof(P_ARK_t);

  // Hash Table
  _arkp->pers_cs_bytes += sizeof(hash_t) + (_arkp->ht->n * sizeof(uint64_t));

  // Block List
  _arkp->pers_cs_bytes += sizeof(BL);

  // calculate for a full ark
  bytes  = _arkp->pers_cs_bytes;
  bytes += _arkp->bl->list->words * sizeof(uint64_t);
  _arkp->pers_max_blocks = divup(bytes, _arkp->bsize);

  KV_TRC(pAT, "PERSIST_CALC pers_cs_bytes:%ld pers_max_blocks:%ld",
          _arkp->pers_cs_bytes, _arkp->pers_max_blocks);
  return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_persist(_ARK *_arkp)
{
  int32_t        rc          = 0;
  uint64_t       tot_bytes   = 0;
  uint64_t       wrblks      = 0;
  char          *p_data_orig = NULL;
  char          *p_data      = NULL;
  p_cntr_t      *pptr        = NULL;
  char          *dptr        = NULL;
  P_ARK_t       *pcfg        = NULL;
  ark_io_list_t *bl_array    = NULL;

  if ( (_arkp->ea->st_type == EA_STORE_TYPE_MEMORY) ||
      !(_arkp->flags & ARK_KV_PERSIST_STORE) )
  {
    return 0;
  }

  ark_persistence_calc(_arkp);

  // allocate write buffer
  tot_bytes   = _arkp->pers_max_blocks * _arkp->bsize;
  p_data_orig = am_malloc(tot_bytes + ARK_ALIGN);
  if (p_data_orig == NULL)
  {
    KV_TRC_FFDC(pAT, "Out of memory allocating %"PRIu64" bytes for "
                     "persistence data", tot_bytes);
    return ENOMEM;
  }
  p_data = ptr_align(p_data_orig);
  memset(p_data, 0, tot_bytes);

  // Record cntr data
  pptr   = (p_cntr_t *)p_data;
  memcpy(pptr->p_cntr_magic, ARK_P_MAGIC, sizeof(pptr->p_cntr_magic));
  pptr->p_cntr_version = ARK_P_VERSION_2;
  pptr->p_cntr_size    = sizeof(p_cntr_t);

  // Record configuration info
  pcfg = (P_ARK_t*)pptr->p_cntr_data;
  pcfg->flags   = _arkp->flags;
  pcfg->size    = _arkp->ea->size;
  pcfg->bsize   = _arkp->bsize;
  pcfg->bcount  = _arkp->bcount;
  pcfg->blkbits = _arkp->blkbits;
  pcfg->grow    = _arkp->blkbits;
  pcfg->hcount  = _arkp->hcount;
  pcfg->vlimit  = _arkp->vlimit;
  pcfg->blkused = _arkp->blkused;
  pcfg->nasyncs = _arkp->nasyncs;
  pcfg->basyncs = _arkp->basyncs;
  pcfg->ntasks  = _arkp->ntasks;
  pcfg->nthrds  = _arkp->nthrds;
  ark_persist_stats(_arkp, &(pcfg->pstats));
  pptr->p_cntr_cfg_offset = 0;
  pptr->p_cntr_cfg_size   = sizeof(P_ARK_t);

  dptr = pptr->p_cntr_data;

  // Record hash info
  dptr                  += pptr->p_cntr_cfg_size;
  pptr->p_cntr_ht_offset = dptr - pptr->p_cntr_data;
  pptr->p_cntr_ht_size   = sizeof(hash_t) + (_arkp->ht->n * sizeof(uint64_t));
  memcpy(dptr, _arkp->ht, pptr->p_cntr_ht_size);

  // Record block list info
  dptr                  += pptr->p_cntr_ht_size;
  pptr->p_cntr_bl_offset = dptr - pptr->p_cntr_data;
  pptr->p_cntr_bl_size   = sizeof(BL);
  memcpy(dptr, _arkp->bl, pptr->p_cntr_bl_size);

  // Record IV list info
  dptr                    += pptr->p_cntr_bl_size;
  pptr->p_cntr_bliv_offset = dptr - pptr->p_cntr_data;

  // bliv_size = bytes in bl->list->data[cs_blocks + kvdata_blocks]
  // add 2 to top because of how IV->data chaining works
  pptr->p_cntr_bliv_size = divup((_arkp->bl->top+2) * _arkp->bl->w, 8);
  memcpy(dptr, _arkp->bl->list->data, pptr->p_cntr_bliv_size);

  // Calculate wrblks: number of persist metadata blocks to write
  tot_bytes = _arkp->pers_cs_bytes + pptr->p_cntr_bliv_size;
  wrblks    = pcfg->pblocks = divup(tot_bytes, _arkp->bsize);

  KV_TRC(pAT, "PERSIST_WR dev:%s top:%ld wrblks:%ld vs pers_max_blocks:%ld",
         _arkp->ea->st_device, _arkp->bl->top, pcfg->pblocks,
         _arkp->pers_max_blocks);

  bl_array = bl_chain_blocks(_arkp->bl, 0, wrblks);
  if ( NULL == bl_array )
  {
    KV_TRC_FFDC(pAT, "Out of memory allocating %"PRIu64" blocks for block list",
                wrblks);
    rc = ENOMEM;
  }
  else
  {
    rc = ea_async_io(_arkp->ea, ARK_EA_WRITE, (void *)p_data, 
                     bl_array, wrblks, _arkp->nthrds);
    am_free(bl_array);
  }

  KV_TRC_PERF2(pAT, "PSTATS  kv:%8ld bytes:%8ld blks:%8ld",
               _arkp->pers_stats.kv_cnt,
               _arkp->pers_stats.byte_cnt,
               _arkp->pers_stats.blk_cnt);
  KV_TRC(pAT, "PERSIST_DATA_STORED rc:%d", rc);

  am_free(p_data_orig);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_check_persistence(_ARK *_arkp, uint64_t flags)
{
  int32_t  rc = -1;
  char    *p_data_orig = NULL;
  char    *p_data = NULL;
  ark_io_list_t *bl_array = NULL;
  p_cntr_t *pptr = NULL;
  P_ARK_t  *pcfg = NULL;
  hash_t   *htp = NULL;
  BL       *blp = NULL;
  uint64_t  rdblks = 0;

  // Ignore the persistence data and load from scratch
  if ( (!(flags & ARK_KV_PERSIST_LOAD)) || (flags & ARK_KV_VIRTUAL_LUN) )
  {
    return -1;
  }

  KV_TRC(pAT, "PERSIST_LOAD");

  p_data_orig = am_malloc(_arkp->bsize + ARK_ALIGN);
  if (p_data_orig == NULL)
  {
    KV_TRC_FFDC(pAT, "Out of memory allocating %"PRIu64" bytes for the first "
                     "persistence block", _arkp->bsize);
    rc = ENOMEM;
  }
  else
  {
    p_data = ptr_align(p_data_orig);
    bl_array = bl_chain_no_bl(0, 1);
    rc = ea_async_io(_arkp->ea, ARK_EA_READ, (void *)p_data, 
                      bl_array, 1, 1);
    am_free(bl_array);
  }

  if (rc == 0)
  {
    // We've read the first block.  We check to see if
    // persistence data is present and if so, then
    // read the rest of the data from the flash.
    pptr = (p_cntr_t *)p_data;
    if ( memcmp(pptr->p_cntr_magic, ARK_P_MAGIC, 
                 sizeof(pptr->p_cntr_magic) != 0))
    {
      KV_TRC_FFDC(pAT, "No magic number found in persistence data: %d", EINVAL);
      // The magic number does not match so data is either
      // not present or is corrupted.
      rc = -1;
    }
    else
    {
      // Now we check version and the first persistence data
      // needs to be the ARK_PERSIST_CONFIG block
      if (pptr->p_cntr_version != ARK_P_VERSION_1 &&
          pptr->p_cntr_version != ARK_P_VERSION_2)
      {
        KV_TRC_FFDC(pAT, "Invalid / unsupported version: %"PRIu64"",
                    pptr->p_cntr_version);
        rc = EINVAL;
      }
      else
      {
        // Read in the rest of the persistence data
        pcfg   = (P_ARK_t *)(pptr->p_cntr_data + pptr->p_cntr_cfg_offset);
        rdblks = pcfg->pblocks;
        if (rdblks > 1)
        {
          p_data_orig = am_realloc(p_data_orig, (rdblks * _arkp->bsize) +
                                   ARK_ALIGN);
          if (p_data_orig == NULL)
          {
            KV_TRC_FFDC(pAT, "Out of memory allocating %"PRIu64" bytes for "
                             "full persistence block",
                             (rdblks * _arkp->bsize));
            rc = ENOMEM;
          }
          else
          {
            p_data = ptr_align(p_data_orig);
            bl_array = bl_chain_no_bl(0, rdblks);
            if (bl_array == NULL)
            {
              KV_TRC_FFDC(pAT, "Out of memory allocating %"PRIu64" blocks for "
                               "full persistence data", rdblks);
              rc = ENOMEM;
            }
          }

          // We are still good to read the rest of the data
          // from the flash
          if (rc == 0)
          {
            KV_TRC(pAT, "PERSIST_RD rdblks:%ld", rdblks);
            rc = ea_async_io(_arkp->ea, ARK_EA_READ, (void *)p_data,
                           bl_array, rdblks, 1);
            am_free(bl_array);
            pptr = (p_cntr_t *)p_data;
            pcfg   = (P_ARK_t *)(pptr->p_cntr_data + pptr->p_cntr_cfg_offset);
          }
        }
      }
    }
  }

  // If rc == 0, that means we have persistence data
  if (rc == 0)
  {
      KV_TRC(pAT, "PERSIST_META size %ld bsize %ld hcount %ld bcount %ld "
                  "nthrds %d nasyncs %d basyncs %d blkbits %ld version:%ld",
                  pcfg->size, pcfg->bsize, pcfg->hcount, pcfg->bcount,
                  pcfg->nthrds, pcfg->nasyncs, pcfg->basyncs, pcfg->blkbits,
                  pptr->p_cntr_version);

    _arkp->persload            = 1;
    _arkp->size                = pcfg->size;
    _arkp->flags               = flags;
    _arkp->bsize               = pcfg->bsize;
    _arkp->bcount              = pcfg->bcount;
    _arkp->blkbits             = pcfg->blkbits;
    _arkp->grow                = pcfg->grow;
    _arkp->hcount              = pcfg->hcount;
    _arkp->vlimit              = pcfg->vlimit;
    _arkp->blkused             = pcfg->blkused;
    _arkp->pers_stats.kv_cnt   = pcfg->pstats.kv_cnt;
    _arkp->pers_stats.blk_cnt  = pcfg->pstats.blk_cnt;
    _arkp->pers_stats.byte_cnt = pcfg->pstats.byte_cnt;

    KV_TRC(pAT, "ARK_META size %ld bsize %ld hcount %ld bcount %ld "
                "nthrds %d nasyncs %ld basyncs %d blkbits %ld",
                _arkp->size, _arkp->bsize, _arkp->hcount, _arkp->bcount,
                _arkp->nthrds, _arkp->nasyncs, _arkp->basyncs, _arkp->blkbits);

    htp = (hash_t *)(pptr->p_cntr_data + pptr->p_cntr_ht_offset);
    _arkp->ht = hash_new(htp->n);
    if (_arkp->ht == NULL)
    {
        if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
        rc = errno;
        KV_TRC_FFDC(pAT, "ht_new failed: n:%ld rc:%d", htp->n, rc);
        goto error_exit;
    }
    memcpy(_arkp->ht, htp, pptr->p_cntr_ht_size);

    blp = (BL *)(pptr->p_cntr_data + pptr->p_cntr_bl_offset);
    _arkp->bl = bl_new(blp->n, blp->w);
    if (_arkp->bl == NULL)
    {
        if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
        rc = errno;
        KV_TRC_FFDC(pAT, "bl_new failed: n:%ld w:%ld rc:%d",
                    blp->n, blp->w, rc);
        goto error_exit;
    }
    _arkp->bl->count = blp->count;
    _arkp->bl->head  = blp->head;
    _arkp->bl->hold  = blp->hold;
    _arkp->bl->top   = blp->top;

    if (pptr->p_cntr_version == ARK_P_VERSION_1)
    {
        IV *piv = (IV *)(pptr->p_cntr_data + pptr->p_cntr_bliv_offset);

        KV_TRC(pAT, "PERSIST_VERSION_1 LOADED");
        _arkp->bl->top = _arkp->bl->n;

        // copy IV->data from piv->data
        memcpy(_arkp->bl->list->data,
               piv->data,
               pptr->p_cntr_bliv_size);
    }
    else if (pptr->p_cntr_version == ARK_P_VERSION_2)
    {
        KV_TRC(pAT, "PERSIST_VERSION_2 LOADED");
        // copy IV->data from bliv_offset
        memcpy(_arkp->bl->list->data,
               pptr->p_cntr_data + pptr->p_cntr_bliv_offset,
               pptr->p_cntr_bliv_size);
    }
    else
    {
        rc = EINVAL;
        KV_TRC_FFDC(pAT, "bad persistent version number: ver:%ld",
                    pptr->p_cntr_version);
        goto error_exit;
    }

    KV_TRC(pAT, "BL_META: n:%ld count:%ld head:%ld hold:%ld top:%ld",
            _arkp->bl->n, _arkp->bl->count, _arkp->bl->head, _arkp->bl->hold,
            _arkp->bl->top);
  }

error_exit:
  am_free(p_data_orig);
  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_create_verbose(char *path, ARK **arkret,
                       uint64_t size, uint64_t bsize, uint64_t hcount,
                       int nthrds, int nqueue, int basyncs, uint64_t flags)
{
  _ARK    *ark    = NULL;
  int      rc     = 0;
  int      p_rc   = 0;
  uint64_t bcount = 0;
  uint64_t x      = 0;
  int      i      = 0;
  int      tnum   = 0;
  int      rnum   = 0;
  int      am_sz  = 0;
  scb_t   *scbp   = NULL;

  hcount = hcount>ARK_VERBOSE_HASH_DEF ? ARK_VERBOSE_HASH_DEF : hcount;

  KV_TRC_OPEN(pAT, "arkdb");
  KV_TRC_PERF2(pAT, "%p path(%s) size %ld bsize %ld hcount %ld "
              "nthrds %d nqueue %d basyncs %d flags:%08lx",
              ark, path, size, bsize, hcount,
              nthrds, nqueue, basyncs, flags);

  if (NULL == arkret)
  {
    KV_TRC_FFDC(pAT, "Incorrect value for ARK control block: rc=EINVAL");
    rc = EINVAL;
    goto ark_create_ark_err;
  }

  if ( (flags & (ARK_KV_PERSIST_LOAD|ARK_KV_PERSIST_STORE)) && 
         (flags & ARK_KV_VIRTUAL_LUN) )
  {
    KV_TRC_FFDC(pAT, "Invalid persistence combination with ARK flags: %016lx",
                flags);
    rc = EINVAL;
    goto ark_create_ark_err;
  }

  if (nthrds <= 0)
  {
      KV_TRC_FFDC(pAT, "invalid nthrds:%d", nthrds);
      rc = EINVAL;
      goto ark_create_ark_err;
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

  ark->ns_per_tick = time_per_tick(1000, 100);

  KV_TRC_FFDC(pAT, "%p path(%s) size %ld bsize %ld hcount %ld "
              "nthrds %d nqueue %d basyncs %d flags:%08lx",
              ark, path, size, bsize, hcount, 
              nthrds, nqueue, basyncs, flags);

  nqueue = nqueue<ARK_MAX_NASYNCS ? nqueue : ARK_MAX_NASYNCS;

  ark->bsize    = bsize;
  ark->rthread  = 0;
  ark->persload = 0;
  ark->nasyncs  = nqueue<ARK_MIN_NASYNCS  ? ARK_MIN_NASYNCS : nqueue;;
  ark->basyncs  = basyncs<ARK_MIN_BASYNCS ? ARK_MIN_BASYNCS : basyncs;
  ark->ntasks   = ARK_MAX_TASK_OPS;
  ark->nthrds   = ARK_VERBOSE_NTHRDS_DEF; // hardcode, perf requirement

  // Create the KV storage, whether that will be memory based
  // or flash
  ark->ea = ea_new(path, ark->bsize, basyncs, &size, &bcount,
                    (flags & ARK_KV_VIRTUAL_LUN));
  if (ark->ea == NULL)
  {
    if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
    rc = errno;
    KV_TRC_FFDC(pAT, "KV storage initialization failed: rc/errno:%d", rc);
    goto ark_create_ea_err;
  }

  // Now that the "connection" to the store has been established
  // we need to check to see if data was persisted from a previous
  // instantiation of the KV store.
  p_rc = ark_check_persistence(ark, flags);
  if (p_rc > 0)
  {
    // We ran into an error while trying to read from
    // the store.
    rc = p_rc;
    KV_TRC_FFDC(pAT, "Persistence check failed: %d", rc);
    goto ark_create_persist_err;
  }
  else if (p_rc == -1)
  {
    KV_TRC(pAT, "NO PERSIST LOAD FLAG");
    // There was no persistence data, so we just build off
    // of what was passed into the API.

    ark->size = size;
    ark->bcount = bcount;
    ark->hcount = hcount;
    ark->vlimit = ARK_VERBOSE_VLIMIT_DEF;
    ark->blkbits = ARK_VERBOSE_BLKBITS_DEF;
    ark->grow = ARK_VERBOSE_GROW_DEF;
    ark->rthread = 0;
    ark->flags = flags;
    ark->astart = 0;
    ark->blkused = 1;
    ark->ark_exit = 0;
    ark->nactive = 0;
    ark->pers_stats.kv_cnt = 0;
    ark->pers_stats.blk_cnt = 0;
    ark->pers_stats.byte_cnt = 0;
    ark->pcmd = PT_IDLE;

    // Create the hash table
    ark->ht = hash_new(ark->hcount);
    if (ark->ht == NULL)
    {
      if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
      rc = errno;
      KV_TRC_FFDC(pAT, "Hash initialization failed: %d", rc);
      goto ark_create_ht_err;
    }

    // Create the block list
    ark->bl = bl_new(ark->bcount, ark->blkbits);
    if (ark->bl == NULL)
    {
      if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
      rc = errno;
      KV_TRC_FFDC(pAT, "Block list initialization failed: %d", rc);
      goto ark_create_bl_err;
    }
    if (flags & ARK_KV_PERSIST_STORE)
    {
      ark_persistence_calc(ark);
      if (bl_reserve(ark->bl, ark->pers_max_blocks))
          {goto ark_create_bl_err;}
    }
  }
  else
  {
      KV_TRC_PERF2(pAT, "PERSIST: %p path(%s) size %ld bsize %ld hcount %ld "
                  "nthrds %d nqueue %ld basyncs %d bcount %ld blkbits %ld "
                  "npart:%d",
                  ark, path, ark->size, ark->bsize, ark->hcount,
                  ark->nthrds, ark->nasyncs, ark->basyncs,
                  ark->bcount, ark->blkbits, ark->npart);
      KV_TRC_PERF2(pAT, "PSTATS  kv:%8ld bytes:%8ld blks:%8ld",
                   ark->pers_stats.kv_cnt,
                   ark->pers_stats.byte_cnt,
                   ark->pers_stats.blk_cnt);

  }

  // Create the requests and tag control blocks and queues.
  x = ark->hcount / ark->nthrds;
  ark->npart  = x + (ark->hcount % ark->nthrds ? 1 : 0);

  am_sz = ark->hcount*sizeof(htc_t);
  ark->htc_orig = am_malloc(am_sz + ARK_ALIGN);
  if (!ark->htc_orig)
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "HTC initialization failed: %d", rc);
    goto ark_create_htc_err;
  }
  ark->htc = ptr_align(ark->htc_orig);
  memset(ark->htc,0,am_sz);

  rc = pthread_mutex_init(&ark->mainmutex,NULL);
  if (rc != 0)
  {
    KV_TRC_FFDC(pAT, "pthread_mutex_init for main mutex failed: %d", rc);
    goto ark_create_pth_mutex_err;
  }

  ark->rtags = tag_new(ark->nasyncs);
  if ( NULL == ark->rtags )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Tag initialization for requests failed: %d", rc);
    goto ark_create_rtag_err;
  }

  ark->ttags = tag_new(ark->ntasks);
  if ( NULL == ark->ttags )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Tag initialization for tasks failed: %d", rc);
    goto ark_create_ttag_err;
  }

  am_sz     = ark->nasyncs * sizeof(rcb_t);
  ark->rcbs = am_malloc(am_sz);
  if ( NULL == ark->rcbs )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory allocation of %"PRIu64" bytes for request control blocks", (ark->nasyncs * sizeof(rcb_t)));
    goto ark_create_rcbs_err;
  }
  memset(ark->rcbs,0,am_sz);

  am_sz     = ark->ntasks * sizeof(tcb_t);
  ark->tcbs = am_malloc(am_sz);
  if ( NULL == ark->tcbs )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory allocation of %"PRIu64" bytes for task control blocks", (ark->ntasks * sizeof(rcb_t)));
    goto ark_create_tcbs_err;
  }
  memset(ark->tcbs,0,am_sz);

  am_sz      = ark->ntasks * sizeof(iocb_t);
  ark->iocbs = am_malloc(am_sz);
  if ( NULL == ark->iocbs )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory allocation of %"PRIu64" bytes for io control blocks", (ark->ntasks * sizeof(iocb_t)));
    goto ark_create_iocbs_err;
  }
  memset(ark->iocbs,0,am_sz);

  am_sz            = ark->nthrds * sizeof(scb_t);
  ark->poolthreads = am_malloc(am_sz);
  if ( NULL == ark->poolthreads )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory allocation of %"PRIu64" bytes for server thread control blocks", (ark->nthrds * sizeof(scb_t)));
    goto ark_create_poolthreads_err;
  }
  memset(ark->poolthreads,0,am_sz);

  for (rnum=0; rnum<ark->nasyncs; rnum++)
  {
    ark->rcbs[rnum].stat = A_NULL;
    pthread_cond_init(&(ark->rcbs[rnum].acond), NULL);
    pthread_mutex_init(&(ark->rcbs[rnum].alock), NULL);
  }

  ark->min_bt = ARK_MIN_BT*ark->bsize;

  for (tnum=0; tnum<ark->ntasks; tnum++)
  {
    ark->tcbs[tnum].inb = bt_new(ark->min_bt, ark->vlimit, VDF_SZ,
                                 &(ark->tcbs[tnum].inb_orig));
    if (ark->tcbs[tnum].inb == NULL)
    {
      if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
      rc = errno;
      KV_TRC_FFDC(pAT, "Bucket allocation for inbuffer failed: %d", rc);
      goto ark_create_taskloop_err;
    }
    ark->tcbs[tnum].inb_size = ark->min_bt;

    ark->tcbs[tnum].oub = bt_new(ark->min_bt, ark->vlimit, VDF_SZ,
                                 &(ark->tcbs[tnum].oub_orig));
    if (ark->tcbs[tnum].oub == NULL)
    {
      if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
      rc = errno;
      KV_TRC_FFDC(pAT, "Bucket allocation for outbuffer failed: %d", rc);
      goto ark_create_taskloop_err;
    }
    ark->tcbs[tnum].oub_size = ark->min_bt;

    ark->tcbs[tnum].vbsize  = bsize * ARK_MIN_VB;
    ark->tcbs[tnum].vb_orig = am_malloc(ark->tcbs[tnum].vbsize + ARK_ALIGN);
    if (ark->tcbs[tnum].vb_orig == NULL)
    {
      rc = ENOMEM;
      KV_TRC_FFDC(pAT, "Out of memory bytes:%ld for tcb->vb",
                  ark->tcbs[tnum].vbsize);
      goto ark_create_taskloop_err;
    }
    ark->tcbs[tnum].vb = ptr_align(ark->tcbs[tnum].vb_orig);
  }

  *arkret = (void *)ark;

  am_sz    = ark->nthrds * sizeof(PT);
  ark->pts = am_malloc(am_sz);
  if ( ark->pts == NULL )
  {
    rc = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory allocation for %"PRIu64" bytes for server thread data", (sizeof(PT) * ark->nthrds));
    goto ark_create_taskloop_err;
  }
  memset(ark->pts,0,am_sz);

  for (i = 0; i < ark->nthrds; i++) {
    PT *pt = &(ark->pts[i]);
    scbp = &(ark->poolthreads[i]);

    memset(scbp, 0, sizeof(scb_t));

    // Start off the random start point for this thread
    // at -1, to show that it has not been part of a
    // ark_random call.
    scbp->rlast = -1;
    scbp->poolstate = PT_RUN;

    scbp->poolstats.io_cnt = 0;
    scbp->poolstats.ops_cnt = 0;
    scbp->poolstats.kv_cnt = 0;
    scbp->poolstats.blk_cnt = 0;
    scbp->poolstats.byte_cnt = 0;

    pthread_mutex_init(&(scbp->poolmutex), NULL);
    pthread_cond_init(&(scbp->poolcond), NULL);

    scbp->reqQ      = queue_new(ark->nasyncs);
    scbp->cmpQ      = queue_new(ark->ntasks);
    scbp->taskQ     = queue_new(ark->ntasks);
    scbp->scheduleQ = queue_new(ark->ntasks);
    scbp->harvestQ  = queue_new(ark->ntasks);

    pt->id = i;
    pt->ark = ark;
    rc = pthread_create(&(scbp->pooltid), NULL, pool_function, pt);
    if (rc != 0)
    {
      KV_TRC_FFDC(pAT, "pthread_create of server thread failed: %d", rc);
      goto ark_create_poolloop_err;
    }
  }

  ark->pcmd = PT_RUN;

  goto ark_create_return;

ark_create_poolloop_err:

  for (; i >= 0; i--)
  {
    scbp = &(ark->poolthreads[i]);

    if (scbp->pooltid != 0)
    {
      queue_lock  (scbp->reqQ);
      queue_wakeup(scbp->reqQ);
      queue_unlock(scbp->reqQ);

      pthread_join(scbp->pooltid, NULL);

      pthread_mutex_destroy(&(scbp->poolmutex));
      pthread_cond_destroy(&(scbp->poolcond));

      if (scbp->reqQ)      {queue_free(scbp->reqQ);}
      if (scbp->cmpQ)      {queue_free(scbp->cmpQ);}
      if (scbp->taskQ)     {queue_free(scbp->taskQ);}
      if (scbp->scheduleQ) {queue_free(scbp->scheduleQ);}
      if (scbp->harvestQ)  {queue_free(scbp->harvestQ);}
    }
  }

  if ( ark->pts != NULL )
  {
    am_free(ark->pts);
  }

ark_create_taskloop_err:
  KV_TRC_DBG(pAT, "free inb/oub/vb");
  for ( tnum = 0; tnum < ark->ntasks; tnum++ )
  {
    bt_delete(ark->tcbs[tnum].inb_orig);
    bt_delete(ark->tcbs[tnum].oub_orig);
    am_free  (ark->tcbs[tnum].vb_orig);
  }

  KV_TRC_DBG(pAT, "destroy rcb cond/lock");
  for (rnum = 0; rnum < ark->nasyncs; rnum++)
  {
    pthread_cond_destroy(&(ark->rcbs[rnum].acond));
    pthread_mutex_destroy(&(ark->rcbs[rnum].alock));
  }

  KV_TRC_DBG(pAT, "free pool threads");
  if ( ark->poolthreads != NULL )
  {
    am_free(ark->poolthreads);
  }

ark_create_poolthreads_err:
  KV_TRC_DBG(pAT, "free iocbs");
  if (ark->iocbs)
  {
    am_free(ark->iocbs);
  }

ark_create_iocbs_err:
  KV_TRC_DBG(pAT, "free tcbs");
  if (ark->tcbs)
  {
    am_free(ark->tcbs);
  }

ark_create_tcbs_err:
  KV_TRC_DBG(pAT, "free rcbs");
  if (ark->rcbs)
  {
    am_free(ark->rcbs);
  }

ark_create_rcbs_err:
  KV_TRC_DBG(pAT, "free ttags");
  if (ark->ttags)
  {
    tag_free(ark->ttags);
  }

ark_create_ttag_err:
  KV_TRC_DBG(pAT, "free rtags");
  if (ark->rtags)
  {
    tag_free(ark->rtags);
  }

ark_create_rtag_err:
  KV_TRC_DBG(pAT, "destroy mainmutex");
  pthread_mutex_destroy(&ark->mainmutex);

ark_create_pth_mutex_err:
  KV_TRC_DBG(pAT, "free htc");
  am_free(ark->htc_orig);

ark_create_htc_err:
  KV_TRC_DBG(pAT, "bl_delete");
  bl_delete(ark->bl);

ark_create_bl_err:
  KV_TRC_DBG(pAT, "free ht");
  hash_free(ark->ht);

ark_create_ht_err:
ark_create_persist_err:
  KV_TRC_DBG(pAT, "ea_delete");
  ea_delete(ark->ea);

ark_create_ea_err:
  KV_TRC_DBG(pAT, "free ark");
  am_free(ark);
  *arkret = NULL;

ark_create_ark_err:
  KV_TRC_CLOSE(pAT);

ark_create_return:
  KV_TRC_PERF2(pAT, "DONE: %p path(%s) id:%d size %ld bsize %ld hcount %ld "
              "ntasks:%ld nthrds %d nqueue %ld basyncs %d flags:%08lx",
              ark, path, ark->ea->st_flash, size, bsize, hcount, ark->ntasks,
              nthrds, ark->nasyncs, ark->basyncs, flags);
  return rc;
}

int ark_create(char *path, ARK **arkret, uint64_t flags)
{
  return ark_create_verbose(path, arkret, 
                            ARK_VERBOSE_SIZE_DEF, 
                            ARK_VERBOSE_BSIZE_DEF,
                            ARK_VERBOSE_HASH_DEF,
                            ARK_VERBOSE_NTHRDS_DEF, 
                            ARK_MAX_NASYNCS,
                            ARK_EA_BLK_ASYNC_CMDS,
                            flags);
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_delete(ARK *ark)
{
  int rc = 0;
  int i = 0;
  _ARK *_arkp = (_ARK *)ark;
  scb_t *scbp = NULL;

  if (NULL == ark)
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "Invalid ARK control block parameter: %d", rc);
    goto ark_delete_ark_err;
  }

  KV_TRC_DBG(pAT, "free thread resources");

  // Wait for all active threads to exit
  for (i = 0; i < _arkp->nthrds; i++)
  {
      scbp = &(_arkp->poolthreads[i]);
      scbp->poolstate = PT_EXIT;

      queue_lock  (scbp->reqQ);
      queue_wakeup(scbp->reqQ);
      queue_unlock(scbp->reqQ);

      pthread_join(scbp->pooltid, NULL);

      queue_free(scbp->reqQ);
      queue_free(scbp->cmpQ);
      queue_free(scbp->taskQ);
      queue_free(scbp->scheduleQ);
      queue_free(scbp->harvestQ);

      pthread_mutex_destroy(&(scbp->poolmutex));
      pthread_cond_destroy(&(scbp->poolcond));
      KV_TRC(pAT, "thread %d joined", i);
  }

  KV_TRC_DBG(pAT, "free arkp->poolthreads");
  if (_arkp->poolthreads) {am_free(_arkp->poolthreads);}

  KV_TRC_DBG(pAT, "free arkp->pts");
  if (_arkp->pts) {am_free(_arkp->pts);}

  KV_TRC_DBG(pAT, "destroy rcb cond/lock");
  for ( i = 0; i < _arkp->nasyncs ; i++ )
  {
    pthread_cond_destroy(&(_arkp->rcbs[i].acond));
    pthread_mutex_destroy(&(_arkp->rcbs[i].alock));
  }

  KV_TRC_DBG(pAT, "free inb/oub/vb");
  for (i=0; i<_arkp->ntasks; i++)
  {
    bt_delete(_arkp->tcbs[i].inb_orig);
    bt_delete(_arkp->tcbs[i].oub_orig);
    am_free  (_arkp->tcbs[i].vb_orig);
    am_free  (_arkp->tcbs[i].aiol);
  }

  KV_TRC_DBG(pAT, "free iocbs");
  am_free(_arkp->iocbs);

  KV_TRC_DBG(pAT, "free tcbs");
  am_free(_arkp->tcbs);

  KV_TRC_DBG(pAT, "free rcbs");
  am_free(_arkp->rcbs);

  KV_TRC_DBG(pAT, "free ttags");
  if (_arkp->ttags) {tag_free(_arkp->ttags);}

  KV_TRC_DBG(pAT, "free rtags");
  if (_arkp->rtags) {tag_free(_arkp->rtags);}

  if (!(_arkp->flags & ARK_KV_VIRTUAL_LUN))
  {
    rc = ark_persist(_arkp);
    if ( rc != 0 )
    {
      KV_TRC_FFDC(pAT, "FFDC: ark_persist failed: %d", rc);
    }
  }

  KV_TRC_DBG(pAT, "destroy mainmutex");
  pthread_mutex_destroy(&_arkp->mainmutex);

  KV_TRC_DBG(pAT, "ea_delete");
  (void)ea_delete(_arkp->ea);

  KV_TRC_DBG(pAT, "free ht");
  hash_free(_arkp->ht);

  KV_TRC_DBG(pAT, "free htc");
  for (i=0; i<_arkp->hcount; i++) {am_free(_arkp->htc[i].orig);}
  am_free(_arkp->htc_orig);

  KV_TRC_DBG(pAT, "bl_delete");
  bl_delete(_arkp->bl);

  KV_TRC(pAT, "done, free arkp %p", _arkp);
  am_free(_arkp);

ark_delete_ark_err:
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
 ******************************************************************************/
// if successful then returns vlen else returns negative number error code
int ark_set(ARK *ark, uint64_t klen, 
            void *key, uint64_t vlen, void *val, int64_t *rval) {
  int rc = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((_arkp == NULL) || klen < 0 || vlen < 0 ||
      ((klen > 0) && (key == NULL)) || (klen>ARK_MAX_LEN) ||
      ((vlen > 0) && (val == NULL)) || (vlen>ARK_MAX_LEN) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "FFDC: EINVAL: ark %p klen %"PRIu64" "
                     "key %p vlen %"PRIu64" val %p rval %p",
                     ark, klen, key, vlen, val, rval);
    rc = EINVAL;
  }
  else
  {
    *rval = -1;
    tag = ark_set_async_tag(_arkp, klen, key, vlen, val);
    if (tag < 0)
    {
      rc = EAGAIN;
      KV_TRC_IO(pAT, "EAGAIN: ark_set_async_tag");
    }
    else
    {
      // Will wait here for command to complete
      rc = ark_wait_tag(_arkp, tag, &errcode, &res);
      if (rc == 0)
      {
        if (errcode != 0)
        {
          KV_TRC(pAT, "SET_ERR rc:%d", errcode);
          rc = errcode;
        }

        *rval = res;
      }
    }
  }

  return rc;
}


/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if success returns the size of value 
int ark_exists(ARK *ark, uint64_t klen, void *key, int64_t *rval) {
  int rc = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((_arkp == NULL) || klen < 0 ||
      ((klen > 0) && (key == NULL)) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "FFDC: rc:EINVAL ark %p, klen %"PRIu64" "
                     "key %p, rval %p", ark, klen, key, rval);
    rc = EINVAL;
  }
  else
  {
    *rval = -1;
    tag = ark_exists_async_tag(_arkp, klen, key);
    if (tag < 0)
    {
      rc = EAGAIN;
      KV_TRC_IO(pAT, "EAGAIN: ark_exists_async_tag");
    }
    else
    {
      // Will wait here for command to complete
      rc = ark_wait_tag(_arkp, tag, &errcode, &res);
      if (rc == 0)
      {
        if (errcode != 0)
        {
          KV_TRC(pAT, "EXI_ERR rc:%d", errcode);
          rc = errcode;
        }

        *rval = res;
      }
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
// if successful returns length of value
// vbuf is filled with value starting at voff bytes in,
// at most vbuflen bytes returned
int ark_get(ARK *ark, uint64_t klen, void *key,
        uint64_t vbuflen, void *vbuf, uint64_t voff, int64_t *rval)
{
  int rc = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((_arkp == NULL) ||
      ((klen > 0) && (key == NULL)) ||
      ((vbuflen > 0) && (vbuf == NULL)) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "FFDC: EINVAL: ark %p klen %"PRIu64" key %p "
                     "vbuflen %"PRIu64" vbuf %p rval %p",
                     _arkp, klen, key, vbuflen, vbuf, rval);
    rc = EINVAL;
  }
  else
  {
    *rval = -1;
    tag = ark_get_async_tag(_arkp, klen, key, vbuflen, vbuf, voff);
    if (tag < 0)
    {
      KV_TRC_IO(pAT, "EAGAIN: ark_get_async_tag failed");
      rc = EAGAIN;
    }
    else
    {
      // Will wait here for command to complete
      rc = ark_wait_tag(_arkp, tag, &errcode, &res);
      if (rc == 0)
      {
        if (errcode != 0)
        {
          KV_TRC(pAT, "GET_ERR rc:%d", errcode);
          rc = errcode;
        }

        *rval = res;
      }
    }
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
  int rc = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((_arkp == NULL) ||
      ((klen > 0) && (key == NULL)) ||
      (rval == NULL))
  {
    KV_TRC_FFDC(pAT, "FFDC: EINVAL: ark %p klen %"PRIu64" key %p rval %p",
                _arkp, klen, key, rval);
    rc = EINVAL;
  }
  else
  {
    *rval = -1;
    tag = ark_del_async_tag(_arkp, klen, key);
    if (tag < 0)
    {
      KV_TRC_IO(pAT, "EAGAIN: ark_del_async_tag");
      rc = EAGAIN;
    }
    else
    {
      // Will wait here for command to complete
      rc = ark_wait_tag(_arkp, tag, &errcode, &res);
      if (rc == 0)
      {
        if (errcode != 0)
        {
          KV_TRC(pAT, "DEL_ERR rc:%d", errcode);
          rc = errcode;
        }

        *rval = res;
      }
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_random(ARK *ark, uint64_t kbuflen, uint64_t *klen, void *kbuf)
{
  uint64_t i;
  int32_t  rc = 0;
  int32_t  done = 0;
  int32_t  ptid = -1;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  if (_arkp == NULL || 0 >= kbuflen || NULL == klen || NULL == kbuf)
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL ark %p,  kbuflen %"PRIu64", klen %p, kbuf %p", _arkp, kbuflen, klen, kbuf);
    rc = EINVAL;
  }
  else
  {
    *klen = -1;
    
    ptid = _arkp->rthread;

    // Because the keys are stored in a hash table, their order
    // by nature is random.  Therefore, let's just pick a pool
    // thread and see if it has any k/v pairs that it is monitoring
    // and return those.  THen move on to the next pool thread
    // once done with the first thread...and so on.
    for (i = 0; (i <= _arkp->nthrds) && (!done); i++)
    {
      tag = ark_rand_async_tag(_arkp, kbuflen, kbuf, ptid);
      if (tag < 0)
      {
        KV_TRC_IO(pAT, "ark_rand_async_tag failed rc = %d", rc);
        rc = EAGAIN;
        done = 1;
      }
      else
      {
        // Will wait here for command to complete
        rc = ark_wait_tag(_arkp, tag, &errcode, &res);
        if (rc == 0)
        {

          // If EAGAIN is returned, we need to try the next
          // pool thread and see if it has any keys.  All
          // other errors (or success) we stop looking.
          if (errcode != EAGAIN)
          {
            *klen = res;
            done = 1;

            // Remember what thread we left off on
            _arkp->rthread = ptid;
          }
        }
      }

      ptid++;
      if (ptid == _arkp->nthrds)
      {
        // Loop back around to pool thread 0 and continue looking.
        ptid = 0;
      }
    }

    // If done is not set, that means we didn't find a single
    // key/value pair.
    if (!done)
    {
      rc = ENOENT;
      KV_TRC(pAT, "No more key/value pairs rc = %d", ENOENT);
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
ARI *ark_first(ARK *ark, uint64_t kbuflen, int64_t *klen, void *kbuf) {
  _ARI *_arip = NULL;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  done = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = (_ARK *)ark;

  errno = 0;

  if ((_arkp == NULL) || 
      (klen == NULL) || 
      (kbuf == NULL) ||
      (kbuflen == 0))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: ark %p kbuflen %ld klen %p kbuf %p",
              _arkp, kbuflen, klen, kbuf);
    errno = EINVAL;
  }
  else
  {
    _arip = am_malloc(sizeof(_ARI));
    if (NULL == _arip)
    {
      KV_TRC_FFDC(pAT, "Out of memory on allocation of %"PRIu64" bytes for random index control block", sizeof(_ARI));
      *klen = -1;
      errno = ENOMEM;
    }
    else
    {
      *klen = -1;
    
      // Start with the first thread.  We start with the first pool
      // thread and get it's first key.  If by chance the first thread
      // does not have any key's in it's control, we move on to the next
      // pool thread.
      for (i = 0; (i < _arkp->nthrds) && (!done); i++)
      {
        tag = ark_first_async_tag(_arkp, kbuflen, kbuf, _arip, i);
        if (tag < 0)
        {
          am_free(_arip);
          _arip = NULL;
          rc = EAGAIN;
          done = 1;
          KV_TRC_IO(pAT, "ark_first_async_tag failed rc = %d", rc);
        }
        else
        {
          // Will wait here for command to complete
          rc = ark_wait_tag(_arkp, tag, &errcode, &res);
          if (rc == 0)
          {

            // If EAGAIN is returned, we need to try the next
            // pool thread and see if it has any keys.  All
            // other errors (or success) we stop looking.
            if (errcode != EAGAIN)
            {
              *klen = res;
              done = 1;

              // Remember what thread we left off on
              _arip->ithread = i;
            }
          }
        }
      }

      // If done is not set, that means we didn't find a single
      // key/value pair.
      if (!done)
      {
        am_free(_arip);
        _arip = NULL;
        rc = ENOENT;
        KV_TRC(pAT, "No more key/value pairs in the store: rc = %d", rc);
      }
      else
      {
        if (rc)
        {
          am_free(_arip);
          _arip = NULL;
        }
      }
    }
  }

  return (ARI *)_arip;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_next(ARI *iter, uint64_t kbuflen, int64_t *klen, void *kbuf)
{
  _ARI *_arip = (_ARI *)iter;
  uint64_t i;
  int32_t  rc = 0;
  int32_t  done = 0;
  int errcode = 0;
  int tag = 0;
  int64_t res = 0;
  _ARK *_arkp = NULL;

  if ((_arip == NULL) || 
      (_arip->ark == NULL) || 
      (klen == NULL) || 
      (kbuf == NULL) ||
      (kbuflen == 0))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: ari %p kbuflen %ld klen %p kbuf %p",
              _arip, kbuflen, klen, kbuf);
    rc = EINVAL;
  }
  else
  {
    _arkp = _arip->ark;
    *klen = -1;
    
    // Start with the thread we left off on last time and then loop
    // to the end
    for (i = _arip->ithread; (i < _arkp->nthrds) && (!done); i++)
    {
      tag = ark_next_async_tag(_arkp, kbuflen, kbuf, _arip, i);
      if (tag < 0)
      {
        rc = EAGAIN;
        KV_TRC_IO(pAT, "ark_next_async_tag failed rc = %d", rc);
        done = 1;
      }
      else
      {
        // Will wait here for command to complete
        rc = ark_wait_tag(_arkp, tag, &errcode, &res);
        if (rc == 0)
        {

          // If EAGAIN is returned, we need to try the next
          // pool thread and see if it has any keys.  All
          // other errors (or success) we stop looking.
          if (errcode != EAGAIN)
          {
            *klen = res;
            done = 1;

            // Remember what thread we left off on
            _arip->ithread = i;
          }
        }
      }
    }

    // If done is not set, that means we didn't find a single
    // key/value pair.
    if (!done)
    {
      am_free(_arip);
      rc = ENOENT;
      KV_TRC(pAT, "No more key/value pairs in the store: rc = %d", rc);
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_null_async_cb(ARK *ark,  uint64_t klen, void *key,
                      void (*cb)(int errcode, uint64_t dt, int64_t res), uint64_t dt) {
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "FFDC EINVAL, ark %p cb %p", ark, cb);
    return EINVAL;
  }
  else
  {
    _ARK *_arkp = (_ARK *)ark;
    return ark_enq_cmd(K_NULL,_arkp,klen,key,0,0,0,cb,dt, -1, NULL);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_set_async_cb(ARK *ark, uint64_t klen, void *key, uint64_t vlen, void *val,
                     void (*cb)(int errcode, uint64_t dt, int64_t res), uint64_t dt) {
  if (NULL == ark || ((vlen > 0) && (val == NULL)) ||
      (cb == NULL))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: vlen %"PRIu64", val %p, cb %p",
            vlen, val, cb);
    return EINVAL;
  }
  else
  {
    _ARK *_arkp = (_ARK *)ark;
    return ark_enq_cmd(K_SET, _arkp, klen,key,vlen,val,0,cb,dt, -1, NULL);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_get_async_cb(ARK *ark, uint64_t klen,void *key,uint64_t vbuflen,void *vbuf,uint64_t voff,
                     void (*cb)(int errcode, uint64_t dt, int64_t res), uint64_t dt) {
  if (NULL == ark || ((vbuflen > 0) && (vbuf == NULL))||
      (cb == NULL))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: vbuflen %"PRIu64", vbuf %p, cb %p",
            vbuflen, vbuf, cb);
    return EINVAL;
  }
  else
  {
    _ARK *_arkp = (_ARK *)ark;
    return ark_enq_cmd(K_GET, _arkp, klen,key,vbuflen,vbuf,voff,cb,dt, -1, NULL);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_del_async_cb(ARK *ark,  uint64_t klen, void *key,
                     void (*cb)(int errcode, uint64_t dt, int64_t res), uint64_t dt) {
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: cb %p", cb);
    return EINVAL;
  }
  else
  {
    _ARK *_arkp = (_ARK *)ark;
    return ark_enq_cmd(K_DEL, _arkp, klen,key,0,0,0,cb,dt, -1, NULL);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_exists_async_cb(ARK *ark, uint64_t klen, void *key,
                        void (*cb)(int errcode, uint64_t dt, int64_t res), uint64_t dt) {
  if (NULL == ark || cb == NULL)
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL: cb %p", cb);
    return EINVAL;
  }
  else
  {
    _ARK *_arkp = (_ARK *)ark;
    return ark_enq_cmd(K_EXI, _arkp, klen,key,0,0,0,cb,dt, -1, NULL);
  }
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_null_async_tag(_ARK *_arkp,  uint64_t klen, void *key) {
  int tag = -1;
  (void)ark_enq_cmd(K_NULL, _arkp, klen,key,0,0,0,NULL,0, -1, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_set_async_tag(_ARK *_arkp, uint64_t klen, void *key, uint64_t vlen, void *val) {
  int tag = -1;
  (void)ark_enq_cmd(K_SET, _arkp, klen,key,vlen,val,0,NULL,0, -1, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_get_async_tag(_ARK *_arkp, uint64_t klen,void *key,uint64_t vbuflen,void *vbuf,uint64_t voff) {
  int tag = -1;
  (void)ark_enq_cmd(K_GET, _arkp, klen,key,vbuflen,vbuf,voff,NULL,0, -1, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_del_async_tag(_ARK *_arkp,  uint64_t klen, void *key) {
  int tag = -1;
  ark_enq_cmd(K_DEL, _arkp, klen,key,0,0,0,NULL,0, -1, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_exists_async_tag(_ARK *_arkp, uint64_t klen, void *key) {
  int tag = -1;
  (void)ark_enq_cmd(K_EXI, _arkp, klen,key,0,0,0,NULL,0, -1, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_rand_async_tag(_ARK *_arkp, uint64_t klen, void *key, int32_t ptid) {
  int tag = -1;
  (void)ark_enq_cmd(K_RAND, _arkp, klen,key,0,NULL,0,NULL,0, ptid, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_first_async_tag(_ARK *_arkp, uint64_t klen, void *key, _ARI *_arip, int32_t ptid) {
  int tag;
  (void)ark_enq_cmd(K_FIRST, _arkp, klen, key, 0, (void *)_arip, 0, NULL,0, ptid, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_next_async_tag(_ARK *_arkp, uint64_t klen, void *key, _ARI *_arip, int32_t ptid) {
  int tag;
  (void)ark_enq_cmd(K_NEXT, _arkp, klen, key, 0, (void *)_arip, 0, NULL,0, ptid, &tag);
  return tag;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_count(ARK *ark, int64_t *count)
{
  uint32_t i;
  int32_t  rc = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((_arkp == NULL) || (count == NULL))
  {
    KV_TRC_FFDC(pAT, "rc = EINVAL ark %p, count %p", _arkp, count);
    rc = EINVAL;
  }
  else
  {
    *count = _arkp->pers_stats.kv_cnt;

    for (i = 0; i < _arkp->nthrds; i++)
    {
      *count += _arkp->poolthreads[i].poolstats.kv_cnt;
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_flush(ARK *ark)
{
  int rc = 0;
  int i = 0;
  _ARK *_arkp = (_ARK *)ark;
  hash_t *ht = NULL;
  BL *bl = NULL;

  if (ark == NULL)
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
    goto ark_flush_err;
  }

  // recreate the hash and block lists first
  ht = hash_new(_arkp->hcount);
  if (ht == NULL)
  {
    if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
    rc = errno;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
    goto ark_flush_err;
  }

  bl = bl_new(_arkp->bcount, _arkp->blkbits);
  if (bl == NULL)
  {
    if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOMEM;}
    rc = errno;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
    goto ark_flush_err;
  }

  // If we've made it here then we delete the old
  // hash and block list structures and set the new ones
  hash_free(_arkp->ht);
  bl_delete(_arkp->bl);

  _arkp->ht = ht;
  _arkp->bl = bl;

  // We need to reset the counts for each pool thread
  for (i = 0; i < _arkp->nthrds; i++)
  {
    _arkp->poolthreads[i].poolstats.blk_cnt = 0;
    _arkp->poolthreads[i].poolstats.kv_cnt = 0;
    _arkp->poolthreads[i].poolstats.byte_cnt = 0;
  }

ark_flush_err:
  if (rc)
  {
    // If we ran into an error, delete the newly
    // created hash and block lists structures
    if (ht)
    {
      hash_free(ht);
    }

    if (bl)
    {
      bl_delete(bl);
    }
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
pid_t ark_fork(ARK *ark)
{
  _ARK *_arkp = (_ARK *)ark;

  // Let's start with an error just incase we don't make it
  // to the fork() system call and the -1 will tell the
  // caller an error was encountered and a child process
  // was not created.
  pid_t cpid = -1;

  if (ark == NULL)
  {
    KV_TRC_FFDC(pAT, "rc = %s", "EINVAL");
    return cpid;
  }

  // Tell the store to hold all "to be freed" blocks.  This
  // will be undid in ark_fork_done() or in the error
  // case of fork below.
  bl_hold(_arkp->bl);

  cpid = fork();

  switch (cpid)
  {
    // Ran into an error, perform any cleanup before returning
    case -1 :
      bl_release(_arkp->bl);
      break;

    // This is the child process.
    case 0  :
    {
      // Need to do error checking for this block
      int i;

      _arkp->pcmd = PT_RUN;
      _arkp->ark_exit = 0;

      for (i = 0; i < _arkp->nthrds; i++) {
        PT *pt = am_malloc(sizeof(PT)); //TODO: handle malloc fail
        pt->id = i;
        pt->ark = _arkp;

        _arkp->poolthreads[i].poolstate = PT_OFF;
        pthread_mutex_init(&(_arkp->poolthreads[i].poolmutex),NULL);
        pt->id = i;
        pt->ark = _arkp;
        pthread_create(&(_arkp->poolthreads[i].pooltid), NULL, pool_function, pt);
      }

      if (_arkp->ea->st_type != EA_STORE_TYPE_MEMORY)
      {
        int c_rc = 0;

        c_rc = cblk_cg_clone_after_fork(_arkp->ea->st_flash,
                                        O_RDONLY, CBLK_GROUP_RAID0);

        // If we encountered an error, force the child to
        // exit with a non-zero status code
        if (c_rc != 0)
        {
            KV_TRC_FFDC(pAT, "FFDC, rc = %d", c_rc);
            _exit(c_rc);
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
  _ARK *_arkp = (_ARK *)ark;

  if (ark == NULL)
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "FFDC, rc = %s", "EINVAL");
    return rc;
  }

  bl_release(_arkp->bl);

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_inuse(ARK *ark, uint64_t *size) {

  _ARK *_arkp = (_ARK *)ark;
  int rc = 0;
  uint64_t i;

  if ((ark == NULL) || (size == NULL))
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
  }
  else
  {
    *size = _arkp->pers_stats.blk_cnt;
    for (i = 0; i < _arkp->nthrds; i++)
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
  int rc = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ((ark == NULL) || (size == NULL))
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
  }
  else
  {
    *size = _arkp->ea->size;
  }

  return rc;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
int ark_actual(ARK *ark, uint64_t *size)
{
  int32_t  rc = 0;
  _ARK *_arkp = (_ARK *)ark;
  uint64_t i;

  if ((ark == NULL) || (size == NULL))
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
  }
  else
  {
    *size = _arkp->pers_stats.byte_cnt;
    for (i = 0; i < _arkp->nthrds; i++)
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
  int rc = 0;
  int i = 0;
  _ARK *_arkp = (_ARK *)ark;

  if ( (ark == NULL) || (ops == NULL) || (ios == NULL) )
  {
    rc = EINVAL;
    KV_TRC_FFDC(pAT, "rc = %d", rc);
  }
  else
  {
    *ops = 0;
    *ios = 0;

    // Go through the pool threads and collect each
    // threads counts
    for (i = 0; i < _arkp->nthrds; i++)
    {
      *ops += _arkp->poolthreads[i].poolstats.ops_cnt;
      *ios += _arkp->poolthreads[i].poolstats.io_cnt;
    }
  }

  return rc;
}
