/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/_tst_cb.c $                                       */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h> 

#include "ct.h"

#include "cl.h"
#include "bv.h"
#include "tg.h"

#include "arkdb.h"

int iter = 10;
int npool = 4;
int nqueue = 1024;

int nclient = 3;
int nasync  = 128;


int64_t bs = 4096;
int64_t size = 4096 * 1024;
int64_t hc = 1024 * 1024;
int bref = 34;
int grow = 1024;
int basyncs = 256;

int nkey = 64;
int seed = 1234;

int kmn = 0;
int kmx = 8;
int klo = 97;
int khi = 122;
int kmod = -1;
int krng = -1;

int vmn = 0;
int vmx = 16;
int vlo  = 97;
int vhi  = 122;
int vmod = -1;
int vrng = -1;

int cstr_ = 0;

void gen_keyval(int id, int ki, unsigned short *isd, 
		uint64_t *kl, uint8_t *key,
		uint64_t *vl, uint8_t *val,
		int discard) {
  unsigned short sd[3];
  //printf("in (%d,%d)] with [%d %d %d]\n",id, ki,isd[0], isd[1], isd[2]); 
  sd[0] = isd[0];
  sd[1] = isd[1];
  sd[2] = isd[2];
  int i;
  int klen = 0;
  if (id) {
    klen = sprintf((char *)key,"_%d_", id);
  }
  if (ki) {
    klen += sprintf((char *)key+klen,".%d.", ki);
  }
  /* if (ki) { */
  /*   sprintf(key+7, "%6d_", ki */
  /* } */
  int len = kmn + nrand48(sd) % kmod;
  for (i=0; i<len; i++) {
    key[klen++] =  klo + nrand48(sd)%krng;
  }
  key[klen] = 0;
  *kl = klen;

  int vlen = vmn + nrand48(sd)%vmod;
  for(i=0; i<vlen; i++) {
    val[i] = vlo + nrand48(sd)%vrng;
  }
  val[vlen] = 0;;
  *vl = vlen;
  if (discard) {
    isd[0] = sd[0];
    isd[1] = sd[1];
    isd[2] = sd[2];
  }
  //printf("out (%d,%d)] with [%d %d %d]\n",id, ki,isd[0], isd[1], isd[2]); 
}

  
uint64_t vlim = 256;

ARK *ark = NULL;

typedef struct _ct {
  int id;
  unsigned short seed[3];
} CT;

int check_result() { return EINVAL; }



pthread_mutex_t cb_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_del_callback(int errcode, uint64_t dt,int64_t res) {
  int64_t val = *(int64_t*)&dt;
  printf("%sClient set callback %d %"PRIi64", %"PRIi64"%s\n", 
	 val==res ? C_Green : C_Red, 
	 errcode, val, res,
	 C_Reset);
  return;
}

void get_callback(int errcode, uint64_t dt,int64_t res) {
  int64_t val = *(int64_t*)&dt;
  printf("%sClient get callback %d %"PRIi64", %"PRIi64"%s\n", 
	 val==res ? C_Green : C_Red,
	 errcode, val, res,
	 C_Reset);
  return;
}

void *client_function(void *arg) {
  CT *ct = (CT*) arg;
  printf("Client %d started seed %d %d %d\n", 
	 ct->id, ct->seed[0],ct->seed[1],ct->seed[2]);

  BV *kvmap = bv_new(nkey);
  unsigned short *kvseed = malloc (nkey * 3 * sizeof(unsigned short));
  int i;
  for (i=0; i<3*nkey; i++) kvseed[i] = nrand48(ct->seed);


  // enough buffer space to support async number of operations
  uint8_t **keys = malloc(nasync * sizeof(uint8_t *));
  uint8_t *keyspace = malloc(nasync * (kmx + 8));
  for(i=0; i<nasync; i++) keys[i] = keyspace + i * (kmx+8);//6 id + 1 _ + 1 \0

  uint8_t **vals = malloc(nasync * sizeof(uint8_t *));
  uint8_t *valspace = malloc(nasync * (vmx + 1));
  for(i=0; i<nasync; i++) vals[i] = valspace + i * (vmx+1);

  uint64_t *klen = malloc(nasync * sizeof(uint64_t));
  uint64_t *vlen = malloc(nasync * sizeof(uint64_t));

  int64_t *expres = malloc(nasync * sizeof(int64_t));

  ARC *arc = NULL;
  int rc = ark_connect_verbose(&arc, ark, nasync);
  if (rc) {
    printf("bad connect return %d\n", rc);
    exit(1);
  }

  TG *tg = tg_init(NULL, nasync);
  int *tgmap = malloc(nasync * sizeof(int));
  for(i=0; i<nasync; i++) tgmap[i] = -1;


  int tag = -1;
  //int erc = -1; //unused
  //int reclaimed = -1; //unused
  //int64_t returned = -1; //unused

  int ki = -1;
  int ai = -1;
  int op = -1;

  // do set/del operations
  for (i=0; i<iter; i++) {
    ki = nrand48(ct->seed) % nkey;

    ai = tg_get(tg);
    op = nrand48(ct->seed) % 2;
    switch (op) {
    case 0: //set
      {
	gen_keyval(ct->id,ki,kvseed + ki*3, klen + ai, keys[ai], vlen + ai, vals[ai],1);
	gen_keyval(ct->id,ki,kvseed + ki*3, klen + ai, keys[ai], vlen + ai, vals[ai],0);
	printf("%s%c(%d,%d)'%s'->'%s' tag [%d,%d]%s\n", C_Yellow,bv_get(kvmap,ki) ? '!' : '+',
	       ct->id, ki, keys[ai], vals[ai], ct->id,ai, C_Reset);
	//	int ark_set_tag(arc, ai, 
	expres[ai] = vlen[ai];
	tag = ark_set_async_cb(arc, klen[ai], keys[ai], vlen[ai], vals[ai], set_del_callback, expres[ai]);
	if (tag < 0) {
	  exit(2);
	} else {
	  tgmap[tag] = ai;
	}
	bv_set(kvmap,ki);
	break;
      }
    case 1: //del
      {
	gen_keyval(ct->id,ki,kvseed + ki*3, klen + ai, keys[ai], vlen + ai, vals[ai],1);
	printf("%s%c(%d,%d)'%s'->'%s' tag [%d,%d]%s\n", C_Yellow, bv_get(kvmap,ki) ? '-' : '~',
	       ct->id, ki, keys[ai], vals[ai], ct->id,ai, C_Reset);
	expres[ai] = bv_get(kvmap,ki) ? vlen[ai] : -1;
	tag = ark_del_async_cb(arc, klen[ai], keys[ai], set_del_callback, expres[ai]);
	tgmap[tag] = ai;
	if (tag<0) {
	  exit(3);
	}
	bv_clr(kvmap,ki);
	break;
      }
    }
  }
  // do get operations
  for (i=0; i<iter; i++) {
    ki = nrand48(ct->seed) % nkey;
    while (1)
    {
      ai = tg_get(tg);
      if (ai >= 0)
      {
        break;
      }
      usleep(5);
    }
    gen_keyval(ct->id,ki,kvseed + ki*3, klen + ai, keys[ai], vlen + ai, vals[ai],0);
    printf("%s%c(%d,%d)'%s'->'%s' tag [%d,%d]%s\n", C_Yellow, bv_get(kvmap,ki) ? '$' : '?',
	   ct->id, ki, keys[ai], vals[ai], ct->id,ai, C_Reset);
    expres[ai] = bv_get(kvmap,ki) ? vlen[ai] : -1;
    tag = ark_get_async_cb(arc,  klen[ai], keys[ai], vlen[ai], vals[ai],0, get_callback, expres[ai]);
    tgmap[tag] = ai;
    if (tag<0) {
      exit(4);
    }
  }

  // print present keys
  for(i=0; i<nkey; i++) {
    if (bv_get(kvmap,i)) {
      gen_keyval(ct->id,i,kvseed + i*3, klen + i, keys[i], vlen + i, vals[i],0);
      printf(":(%d,%d)'%s'->'%s'\n", ct->id,i, keys[i], vals[i]);
    }
  }

  printf("Client %d complete\n", ct->id);
  return NULL;
}

int main(int argc, char **argv) {

  char *anon[] = {NULL,NULL,NULL,NULL};
  CL args[] = {{"-n", &iter, AR_INT, "iterations per client"},
	       {"-c", &nclient, AR_INT, "# of clients"},
	       {"-p", &npool, AR_INT, "# of threads in processing pool"},
	       {"-bs", &bs, AR_INT64, "block size"},
	       {"-size", &size, AR_INT64, "inital storage size"},
	       {"-hc", &hc, AR_INT64, "hash count"},
	       {"-vlim", &vlim, AR_INT64, "value limit for bucket store"},
	       {"-bref", &bref, AR_INT, "block ref bits"},
	       {"-grow", &grow, AR_INT, "# of blocks to grow by"},
	       {"-basyncs", &basyncs, AR_INT, "# of async block buffers"},
	       {"-k", &nkey, AR_INT, "# of keys"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {"-q", &nqueue, AR_INT, "# of queue entries per thread in the pool"},
	       {"-a", &nasync, AR_INT, "# of allowed async ops per client"},
	       {"-kmn", &kmn, AR_INT, "kmn"},
	       {"-kmx", &kmx, AR_INT, "kmx"},
	       {"-klo", &klo, AR_INT, "klo"},
	       {"-khi", &khi, AR_INT, "khi"},
	       {"-vmn", &vmn, AR_INT, "vmn"},
	       {"-vmx", &vmx, AR_INT, "vmx"},
	       {"-vlo", &vlo, AR_INT, "vlo"},
	       {"-vhi", &vhi, AR_INT, "vhi"},
	       {"-cstr_", &cstr_, AR_FLG,"c string keys and values"},
	       /* {"-simple", &simple, AR_FLG, "simple test"}, */
	       /* {"-table", &table, AR_FLG, "print table"}, */
	       /* {"-progress", &progress, AR_FLG, "print progress"}, */
	       {NULL, NULL, 0, NULL}};

  int echo = 1;
  cl_parse(argc,argv,args,anon,echo);
  //int acnt = cl_parse(argc,argv,args,anon,echo);//TODO: we are not using tthe arg count here.

  kmod = kmx - kmn + 1;
  krng = khi - klo + 1;

  vmod = vmx - vmn + 1;
  vrng = vhi - vlo + 1;

  int rc = ark_create_verbose(NULL, &ark, size, bs, hc,  
                               npool, nqueue, basyncs, ARK_KV_VIRTUAL_LUN);

  if (rc) {
    printf("bad create return %d\n", rc);
    exit(1);
  }

  CT *ct = malloc(nclient * sizeof(CT));
  pthread_t *clients = malloc(nclient * sizeof(pthread_t));
  
  srand48(seed);

  struct timeval tv;
  struct timeval post_tv;
  uint64_t ops = 0;
  uint64_t post_ops = 0;
  uint64_t io_cnt = 0;
  uint64_t post_io_cnt = 0;

  (void)gettimeofday(&tv, NULL);
  (void)ark_stats(ark, &ops, &io_cnt);

  int i;
  for(i=0; i<nclient; i++) {
    ct[i].id = i;
    ct[i].seed[0] = lrand48();
    ct[i].seed[1] = lrand48();
    ct[i].seed[2] = lrand48();
    pthread_create(clients + i, NULL, client_function, ct + i);
  }
  for(i=0; i<nclient; i++) {
    pthread_join(clients[i],NULL);
  }

  (void)gettimeofday(&post_tv, NULL);
  (void)ark_stats(ark, &post_ops, &post_io_cnt);

  time_t total_secs = post_tv.tv_sec - tv.tv_sec;
  double ops_sec = (post_ops - ops) / total_secs;
  double ios_sec = (post_io_cnt - io_cnt) / total_secs;

  printf("Total OPS: %"PRIu64"\n", (post_ops - ops));
  printf("Total IOs: %"PRIu64"\n", (post_io_cnt - io_cnt));
  printf("OPS/sec: %.2f\n", ops_sec);
  printf("IOs/sec: %.2f\n", ios_sec);

  printf("_tst_cb.t exiting.\n");
  exit(0);
}
