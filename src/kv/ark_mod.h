/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ark_mod.h $                                            */
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

#ifndef __ARK_MOD_H__
#define __ARK_MOD_H__

#include <pthread.h>
#include "ark.h"
#include "bl.h"
#include "bt.h"

#define ARK_SET_PROCESS_INB  1
#define ARK_SET_WRITE        2
#define ARK_SET_FINISH       3
#define ARK_IO_DONE          4
#define ARK_IO_HARVEST       5
#define ARK_IO_SCHEDULE      6
#define ARK_CMD_DONE         7

// operation 
typedef struct _atp {
  _ARK           *_arkp;
  
  uint64_t        klen;
  void           *key;
  uint64_t        vlen;
  void           *val;
  uint64_t        voff;

  uint64_t        pos;
  int64_t         res;

  void           (*cb)(int errcode, uint64_t dt, int64_t res);
  uint64_t        dt;

  int32_t         cmd;
  int32_t         state;
  int32_t         tag;
  int32_t         task;
  int32_t         rc;
  int32_t         error;

  uint64_t        inblen;
  uint64_t        oublen;
  BTP             inb; // input bucket space - aligned
  BTP             inb_orig; // input bucket space
  BTP             oub; // output bucket space - aligned
  BTP             oub_orig; // output bucket space
  uint64_t        vbsize;
  uint8_t        *vb; // value buffer space - aligned
  uint8_t        *vb_orig; // value buffer space
  uint8_t        *vval; // value buffer space
  uint64_t        vblkcnt;
  int64_t         vblk;
  uint64_t        hpos;
  uint64_t        hblk;
  int64_t         nblk;
  uint64_t        blen;
  int64_t         vvlen;
  int32_t         new_key;

} ATP;

typedef struct _aio
{
  EA             *ea;
  int32_t         op;
  void           *addr;
  ark_io_list_t  *blist;
  uint64_t        nblks;
  uint64_t        start;
  uint64_t        new_start;
  uint64_t        cnt;
  int32_t         task;
  uint32_t        io_errno;
  uint32_t        io_done;
} AIO;

#endif
