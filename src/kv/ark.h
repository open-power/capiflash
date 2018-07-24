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
/**
 **************************************************************************
 * \file
 * \brief Definitions for an Ark
 **************************************************************************
 */
#ifndef __ARK_H__
#define __ARK_H__

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <ticks.h>
#include <kv_inject.h>
#include <arkdb_trace.h>
#include <am.h>
#include <hash.h>
#include <bv.h>
#include <bl.h>
#include <ea.h>
#include <vi.h>
#include <bt.h>
#include <tag.h>
#include <queue.h>
#include <ht.h>
#include <sq.h>
#include <ut.h>
#include <arkdb.h>

#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif

#include <errno.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* if on, then KV_TRC_HEX traces up to 64 bytes of key/value */
#define DUMP_KV 0

#define ARK_UQ_SIZE 255

#define ARK_VERBOSE_SIZE_DEF   (1024*1024)
#define ARK_VERBOSE_BSIZE_DEF         4096
#define ARK_VERBOSE_HASH_DEF   (1024*1024)
#define ARK_VERBOSE_HTC_DEF     (200*1024)
#define ARK_VERBOSE_VLIMIT_DEF         256
#define ARK_VERBOSE_BLKBITS_DEF         34
#define ARK_VERBOSE_GROW_DEF          8192
#define ARK_VERBOSE_NTHRDS_DEF           8

#define ARK_MAX_TASK_OPS                40
#define ARK_MIN_NASYNCS   ARK_MAX_TASK_OPS
#define ARK_MAX_NASYNCS                256
#define ARK_MAX_BASYNCS               8192
#define ARK_MIN_BASYNCS               1024

#define ARK_MIN_BT                       1
#define ARK_MIN_VB                       1
#define ARK_MIN_AIOL                    10

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

#define HTC_GET(_htc, _bt, _sz)    memcpy(_bt,    _htc.p, _sz)
#define HTC_PUT(_htc, _bt, _sz)    memcpy(_htc.p, _bt,    _sz)
#define HTC_HIT(_ark, _pos, _blks) (_ark->htc_blks && _ark->htc[_pos].p && _blks<=_ark->htc_blks)
#define HTC_INUSE(_ark, _htc)      (_ark->htc_blks && _htc.p)
#define HTC_AVAIL(_ark, _blks)     (_ark->htc_blks && _blks <= _ark->htc_blks)

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
      if (_htc.orig)        \
      {                     \
        am_free(_htc.orig); \
        _htc.p    = NULL;   \
        _htc.orig = NULL;   \
      }                     \
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

#define UPDATE_LAT(pscb, prcb, _lat)                                           \
do                                                                             \
{                                                                              \
    if (prcb->cmd == K_SET)                                                    \
    {                                                                          \
        pscb->set_opsT += 1;                                                   \
        pscb->set_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_GET)                                               \
    {                                                                          \
        pscb->get_opsT += 1;                                                   \
        pscb->get_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_EXI)                                               \
    {                                                                          \
        pscb->exi_opsT += 1;                                                   \
        pscb->exi_latT += _lat;                                                \
    }                                                                          \
    else if (prcb->cmd == K_DEL)                                               \
    {                                                                          \
        pscb->del_opsT += 1;                                                   \
        pscb->del_latT += _lat;                                                \
    }                                                                          \
} while (0);


// Stats to collect on number of K/V ops
// and IOs
typedef struct ark_stats
{
  volatile uint64_t ops_cnt;
  volatile uint64_t io_cnt;
  volatile uint64_t kv_cnt;
  volatile uint64_t blk_cnt;
  volatile uint64_t byte_cnt;
  volatile uint64_t um_cnt;
  volatile uint64_t hcl_cnt;
  volatile uint64_t hcl_tot;
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

/* persistent store data, do not modify */
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

  struct _scb     *poolthreads;
  struct _pt      *pts;
  struct _ea      *ea;          // in memory store space

  hash_t          *ht;          // hashtable
  htc_t           *htc;         // array of ptrs to hash cache elements
  void            *htc_orig;    // base addr of malloc before align
  uint32_t         htc_blks;    // blocks per htc element
  uint32_t         htc_max;     // max total blocks allowed for htc

  BL              *bl;          // block lists
  BL              *blu;         // block lists waiting for unmap

  uint32_t         min_bt;      // min size of inb/oub

  double           ns_per_tick;
  ark_stats_t      pers_stats;
} _ARK;

#define _ARC _ARK

typedef struct _scb
{
  pthread_mutex_t  lock;
  pthread_t        pooltid;

  struct _rcb     *rcbs;    // request cbs
  struct _tcb     *tcbs;    // task cbs
  struct _iocb    *iocbs;   // io cbs
  struct _tcb     *utcbs;   // unmap tcbs
  struct _iocb    *ucbs;    // unmap iocbs

  struct _tags    *rtags;   // request tags
  struct _tags    *ttags;   // task/io tags
  struct _tags    *utags;   // unmap tags

  queue_t         *reqQ;
  queue_t         *taskQ;
  queue_t         *scheduleQ;
  queue_t         *harvestQ;
  queue_t         *cmpQ;
  queue_t         *uscheduleQ;
  queue_t         *uharvestQ;

  ticks            utime;        // time since last unmaps processes

  /* perf stats */
  uint32_t         issT;
  uint64_t         QDT;
  uint64_t         QDN;
  uint64_t         set_latT;
  uint64_t         get_latT;
  uint64_t         exi_latT;
  uint64_t         del_latT;
  uint64_t         um_latT;
  uint64_t         set_opsT;
  uint64_t         get_opsT;
  uint64_t         exi_opsT;
  uint64_t         del_opsT;
  uint64_t         um_opsT;
  uint64_t         htc_hits;    // ht cache hits
  uint64_t         htc_disc;    // ht cache discards
  uint32_t         htcN;        // current allocated ht cache elements

  struct _ea      *ea;          // in memory store space

  ticks            perflogtime;  // time since last perf log
  int32_t          rlast;
  int32_t          poolstate;
  ark_stats_t      poolstats;
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
#define ARK_UNMAP_PROCESS    21

// operation 
typedef struct _tcb
{
  int32_t         rtag;
  int32_t         ttag;
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
  ticks           stime;    // timestamp of command start
  ticks           ltime;    // timestamp of last log of an error
  int32_t         state;
  int32_t         op;
  int32_t         tag;
  uint32_t        issT;
  uint32_t        cmpT;
  uint32_t        lat;
  uint32_t        hmissN;
  uint32_t        aflags;
  uint32_t        io_error;
  uint32_t        io_done;
  uint32_t        eagain;
} iocb_t;

int64_t ark_take_pool(_ARK *_arkp, int id, uint64_t n);
void    ark_drop_pool(_ARK *_arkp, int id, uint64_t blk);

rcb_t* ark_set_async_tag   (_ARK *ark, uint64_t klen, void *key, uint64_t vlen, void *val);
rcb_t* ark_get_async_tag   (_ARK *ark, uint64_t klen, void *key, uint64_t vbuflen,void *vbuf,uint64_t voff);
rcb_t* ark_del_async_tag   (_ARK *ark, uint64_t klen, void *key);
rcb_t* ark_exists_async_tag(_ARK *ark, uint64_t klen, void *key);
rcb_t* ark_rand_async_tag  (_ARK *ark, uint64_t klen, void *key, int32_t ptid);
rcb_t* ark_first_async_tag (_ARK *ark, uint64_t klen, void *key, _ARI *_arip, int32_t ptid);
rcb_t* ark_next_async_tag  (_ARK *ark, uint64_t klen, void *key, _ARI *_arip, int32_t ptid);

void *pool_function(void *arg);

int ark_enq_cmd(int cmd, _ARK *_arkp,
                uint64_t klen,    void *key,
                uint64_t vbuflen, void *vbuf, uint64_t voff,
                void (*cb)(int errcode, uint64_t dt, int64_t res),
                uint64_t dt, int32_t pthr, int *ptag, rcb_t **rcb);

int ark_wait_tag(_ARK *_arkp, rcb_t *rcbp, int *errcode, int64_t *res);

void ark_set_finish     (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_write      (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_process    (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_process_inb(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_start      (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_get_start      (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_get_finish     (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_get_process    (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_del_start      (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_del_process    (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_del_finish     (_ARK *_arkp, int32_t tid, tcb_t *tcbp);
void ark_exist_start    (_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_exist_finish   (_ARK *_arkp, int tid, tcb_t *tcbp);

void ea_async_io_schedule(_ARK *_arkp, int32_t tid, iocb_t *iocbp);
void ea_async_io_harvest (_ARK *_arkp, int32_t tid, iocb_t *iocbp);

/**
 *******************************************************************************
 * \brief
 *  init iocb struct for IO
 ******************************************************************************/
static __inline__ void ea_async_io_init(_ARK          *_arkp,
                                        struct _ea    *eap,
                                        iocb_t        *iocbp,
                                        int            op,
                                        void          *addr,
                                        ark_io_list_t *blist,
                                        int64_t        nblks,
                                        int            start,
                                        int32_t        tag,
                                        int32_t        io_done)
{
  memset(iocbp,0,sizeof(iocb_t));
  iocbp->state    = ARK_IO_SCHEDULE;
  iocbp->ea       = eap;
  iocbp->op       = op;
  iocbp->addr     = addr;
  iocbp->blist    = blist;
  iocbp->nblks    = nblks;
  iocbp->start    = start;
  iocbp->aflags   = CBLK_GROUP_RAID0;
  iocbp->io_done  = io_done;
  iocbp->tag      = tag;
  return;
}

#endif
