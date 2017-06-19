/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ark.h $                                                */
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
#ifndef __ARK_H__
#define __ARK_H__

#include <pthread.h>

#include "hash.h"
#include "bl.h"
#include "ea.h"
#include "bt.h"
#include "tag.h"
#include "queue.h"
#include "sq.h"
#include "ut.h"

/* if on, then KV_TRC_HEX traces up to 64 bytes of key/value */
#define DUMP_KV 0

#define ARK_VERBOSE_SIZE_DEF   (1024*1024)
#define ARK_VERBOSE_BSIZE_DEF         4096
#define ARK_VERBOSE_HASH_DEF    (200*1024)
#define ARK_VERBOSE_VLIMIT_DEF         256
#define ARK_VERBOSE_BLKBITS_DEF         34
#define ARK_VERBOSE_GROW_DEF          1024
#define ARK_VERBOSE_NTHRDS_DEF           1

#define ARK_MAX_TASK_OPS               100
#define ARK_MIN_NASYNCS   ARK_MAX_TASK_OPS
#define ARK_MAX_NASYNCS                512
#define ARK_MIN_BASYNCS               1024

#define ARK_MIN_BT                       1
#define ARK_MIN_VB                       1
#define ARK_MIN_AIOL                    10
#define ARK_MAX_HTC_BLKS                 1

#define PT_OFF  0
#define PT_IDLE 1
#define PT_RUN  2
#define PT_EXIT 3

#define ARK_VERB_FN "/tmp/kv_verbosity"

#define ARK_CLEANUP_DELAY 50

#define ARK_ALIGN 128
#define VDF_SZ    (sizeof(uint64_t))

typedef struct
{
    void *p;
    void *orig;
} htc_t;

#define HTC_GET(_htc, _bt, _sz) memcpy(_bt,    _htc.p, _sz)
#define HTC_PUT(_htc, _bt, _sz) memcpy(_htc.p, _bt,    _sz)
#define HTC_HIT(_htc, _blks)    (_htc.p && _blks<=ARK_MAX_HTC_BLKS)
#define HTC_INUSE(_htc)         (_htc.p)

#define HTC_NEW(_htc, _bt, _sz)                 \
  do                                            \
  {                                             \
      _htc.orig = am_malloc(_sz+ARK_ALIGN);     \
      if (_htc.orig)                            \
      {                                         \
          _htc.p = ptr_align(_htc.orig);        \
          memcpy(_htc.p, _bt, _bt->len);        \
      }                                         \
  } while (0)

#define HTC_FREE(_htc)      \
  do                        \
  {                         \
      am_free(_htc.orig);   \
      _htc.p    = NULL;     \
      _htc.orig = NULL;     \
  } while (0)

/* _s - string to prepend to hex dump
 * _p - ptr to bytes to dump
 * _l - length in bytes to dump
 */
#define DUMPX(_s, _p, _l)                                                      \
  do                                                                           \
  {                                                                            \
    uint32_t _ii;                                                              \
    uint8_t *_cc=(uint8_t*)_p;                                                 \
    if (_s) {printf("%s", _s);}                                                \
    for (_ii=0; _ii<(uint32_t)((_l>64)?64:_l); _ii++){printf("%02x",_cc[_ii]);}\
    printf("\n");                                                              \
  } while (0)

#ifdef _AIX
#define GEN_VAL_MOD for (_i=7; _i>=8-_mod; _i--) {*(_t++)=_s[_i];}
#else
#define GEN_VAL_MOD for (_i=0; _i<_mod; _i++) {*(_t++)=_s[_i];}
#endif

/* generate pattern based upon _num into _val */
/* _num can be up to 64-bit value             */
#define GEN_VAL(_val,_num,_len)                                                \
  do                                                                           \
  {                                                                            \
      volatile int64_t   _i   = 0;                                             \
      volatile uint64_t  _d   = (uint64_t)(_num);                              \
      volatile uint8_t  *_s   = (uint8_t*)&_d;                                 \
      volatile uint8_t  *_t   = (uint8_t*)(_val);                              \
      volatile int64_t   _div = (_len)/8;                                      \
      volatile int64_t   _mod = (_len)%8;                                      \
      volatile uint64_t *_p   = (uint64_t*)(_t+_mod);                          \
      GEN_VAL_MOD                                                              \
      for (_i=0; _i<_div; _i++) {*(_p++)=_d;}                                  \
  } while (0)

#define UPDATE_LAT(park, prcb, _lat)                                           \
do                                                                             \
{                                                                              \
    if (prcb->cmd == K_SET)                                                    \
    {                                                                          \
        park->set_opsT += 1;                                                   \
        park->set_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_GET)                                               \
    {                                                                          \
        park->get_opsT += 1;                                                   \
        park->get_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_EXI)                                               \
    {                                                                          \
        park->exi_opsT += 1;                                                   \
        park->exi_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_DEL)                                               \
    {                                                                          \
        park->del_opsT += 1;                                                   \
        park->del_latT += _lat;                                                \
    }                                                                          \
} while (0);

extern uint32_t do_mem_fastpath;

#define MEM_FASTPATH \
    (do_mem_fastpath && _arkp->ea->st_type == EA_STORE_TYPE_MEMORY)

// Stats to collect on number of K/V ops
// and IOs
typedef struct ark_stats
{
  volatile uint64_t ops_cnt;
  volatile uint64_t io_cnt;
  volatile uint64_t kv_cnt;
  volatile uint64_t blk_cnt;
  volatile uint64_t byte_cnt;
} ark_stats_t;

#define ARK_P_VERSION_1       1
#define ARK_P_VERSION_2       2

#define ARK_PERSIST_CONFIG    1
#define ARK_PERSIST_END       2
#define ARK_PERSIST_HT        3
#define ARK_PERSIST_HT_BV     4
#define ARK_PERSIST_HT_IV     5
#define ARK_PERSIST_BL        6
#define ARK_PERSIST_BL_IV     7

typedef struct p_cntr
{
#define ARK_P_MAGIC           "ARKPERST"
  char     p_cntr_magic[8];
  uint64_t p_cntr_version;
  uint32_t p_cntr_type;
  uint64_t p_cntr_size;
  uint64_t p_cntr_cfg_offset;
  uint64_t p_cntr_cfg_size;
  uint64_t p_cntr_ht_offset;
  uint64_t p_cntr_ht_size;
  uint64_t p_cntr_bl_offset;
  uint64_t p_cntr_bl_size;
  uint64_t p_cntr_bliv_offset;
  uint64_t p_cntr_bliv_size;
  char     p_cntr_data[];
} p_cntr_t;

typedef struct p_ark
{
  uint64_t flags;
  uint64_t pblocks;
  uint64_t size;
  uint64_t bsize;
  uint64_t bcount;
  uint64_t blkbits;
  uint64_t grow;
  uint64_t hcount;
  uint64_t vlimit;
  uint64_t blkused;
  ark_stats_t  pstats;
  int      nasyncs;
  int      basyncs;
  int      nthrds;
  int      ntasks;
} P_ARK_t;

typedef struct _ark
{
  uint64_t flags;
  uint32_t persload;
  uint64_t pers_cs_bytes;    // amt required for only the Control Structures
  uint64_t pers_max_blocks;  // amt required to persist a full ark
  uint64_t size;
  uint64_t bsize;
  uint64_t bcount;
  uint64_t blkbits;
  uint64_t grow;
  uint64_t hcount;
  uint64_t vlimit;
  uint64_t blkused;
  uint64_t nasyncs;
  uint64_t ntasks;

  int nthrds;
  int basyncs;
  int npart;
  int nactive;
  int ark_exit;
  int pcmd;
  int rthread;
  int astart;

  pthread_mutex_t mainmutex;

  hash_t          *ht;      // hashtable
  htc_t           *htc;     // array of ptrs to hash cache elements
  void            *htc_orig;
  uint64_t         htc_hits;// # of hash cache hits
  uint64_t         htc_disc;// # of hash cache discards
  uint32_t         htcN;    // # of hash cache

  BL              *bl;      // block lists
  struct _ea      *ea;      // in memory store space
  struct _rcb     *rcbs;
  struct _tcb     *tcbs;
  struct _iocb    *iocbs;
  struct _scb     *poolthreads;
  struct _pt      *pts;
  struct _tags    *rtags;
  struct _tags    *ttags;

  uint32_t         min_bt;  // min size of inb/oub
  uint32_t         issT;
  uint64_t         QDT;
  uint64_t         QDN;
  uint64_t         set_latT;
  uint64_t         get_latT;
  uint64_t         exi_latT;
  uint64_t         del_latT;
  uint64_t         set_opsT;
  uint64_t         get_opsT;
  uint64_t         exi_opsT;
  uint64_t         del_opsT;
  double           ns_per_tick;
  ark_stats_t      pers_stats;
} _ARK;

#define _ARC _ARK

typedef struct _scb
{
  pthread_t        pooltid;
  queue_t         *reqQ;
  queue_t         *cmpQ;
  queue_t         *taskQ;
  queue_t         *scheduleQ;
  queue_t         *harvestQ;
  int32_t          poolstate;
  int32_t          rlast;
  int32_t          dogc;
  ark_stats_t      poolstats;
  pthread_mutex_t  poolmutex;
  pthread_cond_t   poolcond;
} scb_t;

// pool thread gets its id and the database struct
typedef struct _pt
{
  int id;
  _ARK *ark;
} PT;


#define K_NULL 0
#define K_GET  1
#define K_SET  2
#define K_DEL  3
#define K_EXI  4
#define K_RAND 5
#define K_FIRST 6
#define K_NEXT 7

#define A_NULL     0
#define A_INIT     1
#define A_COMPLETE 2
#define A_FINAL    3


// operation 
typedef struct _rcb
{
  _ARK       *ark;
  uint64_t    klen;
  void       *key;
  uint64_t    vlen;
  void       *val;
  uint64_t    voff;

  uint64_t    pos;
  uint64_t    hash;

  int64_t     res;
  uint64_t    dt;
  int32_t     rc;

  uint64_t    rnum;
  int32_t     rtag;
  int32_t     ttag;
  int32_t     sthrd;
  int32_t     cmd;
  int32_t     stat;
  uint64_t    stime;

  void        (*cb)(int errcode, uint64_t dt,int64_t res);
  pthread_cond_t  acond;
  pthread_mutex_t alock;
} rcb_t;

typedef struct _ari
{
  _ARK     *ark;
  int64_t  hpos;
  int64_t  key;
  int32_t  ithread;
  uint64_t btsize;
  BT      *bt;
  BT      *bt_orig;
  uint8_t *pos;
} _ARI;

#define ARK_CMD_INIT         1
#define ARK_CMD_DONE         2
#define ARK_IO_SCHEDULE      3
#define ARK_IO_HARVEST       4
#define ARK_IO_DONE          5
#define ARK_SET_START        6
#define ARK_SET_PROCESS      7
#define ARK_SET_WRITE        8
#define ARK_SET_FINISH       9
#define ARK_GET_START        10
#define ARK_GET_PROCESS      11
#define ARK_GET_FINISH       12
#define ARK_DEL_START        13
#define ARK_DEL_PROCESS      14
#define ARK_DEL_FINISH       15
#define ARK_EXIST_START      16
#define ARK_EXIST_FINISH     17
#define ARK_RAND_START       18
#define ARK_FIRST_START      19
#define ARK_NEXT_START       20

// operation 
typedef struct _tcb
{
  int32_t         rtag;
  int32_t         ttag;
  int32_t         state;
  int32_t         sthrd;

  uint64_t        vbsize;
  uint8_t        *vb;       // value buffer space - aligned
  uint8_t        *vb_orig;  // value buffer space
  uint8_t        *vval;     // value buffer space
  uint64_t        vblkcnt;
  int64_t         vblk;
  uint64_t        hpos;
  uint64_t        hblk;
  int64_t         nblk;
  uint64_t        blen;
  uint64_t        old_btsize;
  int64_t         bytes;
  int64_t         vvlen;
  int32_t         new_key;

  uint64_t        pad;
  uint64_t        inb_size; // input  bucket space after alignment
  uint64_t        oub_size; // output bucket space after alignment
  BTP             inb_orig; // input  bucket space
  BTP             oub_orig; // output bucket space
  BTP             inb;      // input  bucket space - aligned
  BTP             oub;      // output bucket space - aligned

  ark_io_list_t  *aiol;
  uint64_t        aiolN;
} tcb_t;

typedef struct _iocb
{
  struct _ea     *ea; // in memory store space
  void           *addr;
  ark_io_list_t  *blist;
  uint64_t        nblks;
  uint64_t        start;
  uint64_t        new_start;
  uint64_t        cnt;
  int32_t         op;
  int32_t         tag;
  uint32_t        issT;
  uint32_t        cmpT;
  uint32_t        rd;
  uint32_t        lat;
  uint64_t        stime;
  uint32_t        hmissN;
  uint32_t        aflags;
  uint32_t        io_error;
  uint32_t        io_done;
} iocb_t;

int64_t ark_take_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t n);
void ark_drop_pool(_ARK *_arkp, ark_stats_t *stats, uint64_t blk);

int ark_set_async_tag(_ARK *ark,  uint64_t klen, void *key, uint64_t vlen, void *val);
int ark_get_async_tag(_ARK *ark, uint64_t klen,void *key,uint64_t vbuflen,void *vbuf,uint64_t voff);
int ark_del_async_tag(_ARK *ark,  uint64_t klen, void *key);
int ark_exists_async_tag(_ARK *ark,  uint64_t klen, void *key);
int ark_rand_async_tag(_ARK *ark, uint64_t klen, void *key, int32_t ptid);
int ark_first_async_tag(_ARK *ark, uint64_t klen, void *key, _ARI *_arip, int32_t ptid);
int ark_next_async_tag(_ARK *ark, uint64_t klen, void *key, _ARI *_arip, int32_t ptid);

int ark_anyreturn(_ARK *ark, int *tag, int64_t *res);

/**
 *******************************************************************************
 * \brief
 *  init iocb struct for IO
 ******************************************************************************/
static __inline__ void ea_async_io_init(_ARK          *_arkp,
                                        int            op,
                                        void          *addr,
                                        ark_io_list_t *blist,
                                        int64_t        nblks,
                                        int            start,
                                        int32_t        tag,
                                        int32_t        io_done)
{
  iocb_t *iocbp  = &(_arkp->iocbs[tag]);
  tcb_t  *iotcbp = &(_arkp->tcbs[tag]);

  iotcbp->state   = ARK_IO_SCHEDULE;
  iocbp->ea       = _arkp->ea;
  iocbp->op       = op;
  iocbp->addr     = addr;
  iocbp->blist    = blist;
  iocbp->nblks    = nblks;
  iocbp->start    = start;
  iocbp->issT     = 0;
  iocbp->cmpT     = 0;
  iocbp->rd       = 0;
  iocbp->lat      = 0;
  iocbp->hmissN   = 0;
  iocbp->aflags   = CBLK_GROUP_RAID0;
  iocbp->io_error = 0;
  iocbp->io_done  = io_done;
  iocbp->tag      = tag;
  return;
}

#endif
