/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/bt.h $                                                 */
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
#ifndef __BT_H__
#define __BT_H__

typedef struct _bt {
  uint64_t len;
  uint64_t cnt;
  uint64_t max;
  uint64_t def;
  uint64_t dlen;
  uint8_t  data[];
} BT;

typedef BT * BTP;

// data is layed out as
// kl0 vl0 k0... v0... kl1 vl1 k1... v1... etc. 
// when vl > max then the length of the v is really def so
// only def bytes are present but the get/set/delroutines report vl on success
// since there is a max on val sizes entire vals are returned in a buffer the user has made sufficiently large

#define BT_OK 0
#define BT_FAIL -1

BT *bt_new(uint64_t dlen, uint64_t vmx,  uint64_t vdf, uint64_t *btsize, BT **bt_orig);
int bt_growif(BT **bt, BT **bt_orig, uint64_t *lim, uint64_t size);
void bt_delete(BT *bt);
int bt_init(BT *bt);

int64_t bt_exists(BT *bt, uint64_t kl, uint8_t *k);

int bt_set(BT *bto, BT *bti, uint64_t kl, uint8_t *k, uint64_t vl, uint8_t *v, uint64_t *ovlen);

int64_t bt_get(BT *bti, uint64_t kl, uint8_t *k, uint8_t *v);

int64_t bt_del(BT *bto, BT *bti, uint64_t kl, uint8_t *k);

int64_t bt_del_def(BT *bto, BT *bti, uint64_t kl, uint8_t *k, uint8_t *ref, uint64_t *ovlen);

void bt_dump(BT *bt);
void bt_cstr(BT *bt);

// NYI

#endif
