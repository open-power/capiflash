/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arp.h $                                                */
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

#ifndef __ARP_H__
#define __ARP_H__


void *pool_function(void *arg);

int ark_enq_cmd(int cmd, _ARK *_arkp,uint64_t klen,void *key,uint64_t vbuflen,void *vbuf,uint64_t voff,
                void (*cb)(int errcode, uint64_t dt,int64_t res), uint64_t dt, int32_t pthr, int *ptag);

int ark_wait_tag(_ARK *_arkp, int tag, int *errcode, int64_t *res);

void ark_set_finish(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_write(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_process(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_process_inb(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_set_start(_ARK *_arkp, int tid, tcb_t *tcbp);

void ark_get_start(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_get_finish(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_get_process(_ARK *_arkp, int tid, tcb_t *tcbp);

void ark_del_start(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_del_process(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_del_finish(_ARK *_arkp, int32_t tid, tcb_t *tcbp);

void ark_exist_start(_ARK *_arkp, int tid, tcb_t *tcbp);
void ark_exist_finish(_ARK *_arkp, int tid, tcb_t *tcbp);

int ea_async_io_schedule(_ARK   *_arkp,
                         int32_t tid,
                         tcb_t  *tcbp,
                         iocb_t *iocbp);
int ea_async_io_harvest(_ARK   *_arkp,
                        int32_t tid,
                        tcb_t  *tcbp,
                        iocb_t *iocbp,
                        rcb_t  *iorcbp);


#endif
