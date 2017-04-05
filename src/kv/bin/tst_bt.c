/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_bt.c $                                        */
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
#include <unistd.h>
#include <inttypes.h>

#include "cl.h"
#include "bv.h"
#include "bt.h"

int iter = 10;
int keys = 10;
int seed = 1234;

int simple = 0;
int cstr  = 0;
int table = 0;
int progress = 0;

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

int vdf = 8;


uint64_t klen = 0;
uint8_t *kval = NULL;

uint64_t vlen = 0;
uint8_t *vval = NULL;

uint64_t glen = 0;
uint8_t *gval = NULL;

int bs = (1024 * 1024);

unsigned short s0[3];
unsigned short s1[3];

BV *bvk = NULL;
unsigned short *bvs = NULL;

BT *bti = NULL;
BT *bti_orig = NULL;
BT *bto = NULL;
BT *bto_orig = NULL;
BT *btx = NULL;

int64_t bt_set_ret = 0;
#define BT_SET(k,v)						\
  {                                                             \
  uint64_t ovlen = 0;                                           \
  bt_set_ret = bt_set(bto,bti,(uint64_t)(strlen(k)+1), (uint8_t *)(k), (uint64_t)(strlen(v)+1), (uint8_t *)(v), &ovlen); \
  printf("%s %"PRIi64" = Set '%s' -> '%s'\n",bt_set_ret==strlen(v)+1 ? "OK " : "ERR", bt_set_ret, k, v); \
  bt_cstr(bto);								\
  btx= bti; bti=bto; bto=btx;						\
  if (strlen(v)+1!=bt_set_ret) return(99);                              \
  }


int64_t bt_exi_ret = 0;
#define BT_EXI(k,e)							\
  bt_exi_ret =  bt_exists(bti,(uint64_t)(strlen(k)+1), (uint8_t *)(k)); \
  printf("%s %"PRIi64" = Exists '%s'\n",e==bt_exi_ret ? "OK " : "ERR", bt_exi_ret, k); \
  bt_cstr(bti);								\
  if (e!=bt_exi_ret) return(99)
  

int64_t bt_del_ret = 0;
#define BT_DEL(k,rc)							\
  bt_del_ret = bt_del(bto,bti,(uint64_t)(strlen(k)+1), (uint8_t *)(k));	\
  printf("%s %"PRIi64" = Del '%s'\n",bt_del_ret==rc ? "OK " : "ERR", bt_del_ret, k); \
  bt_cstr(bto);							\
  btx= bti; bti=bto; bto=btx						

int64_t bt_get_ret = 0;
#define BT_GET(k,v)							\
  bt_get_ret = bt_get(bti,(uint64_t)(strlen(k)+1), (uint8_t*)(k), vval); \
  printf("%s %"PRIi64" = Get '%s'\n", \
    ((v==NULL && bt_get_ret==-1) || \
     ((strlen(v)+1)==bt_get_ret && memcmp(v,vval,bt_get_ret)==0)) ? "OK" : "ERR", \
	 bt_get_ret, k);
 
void gen_key(int ki) {
  int i;
  s1[0] = bvs[ki*3+0];
  s1[1] = bvs[ki*3+1];
  s1[2] = bvs[ki*3+2];
  klen = kmn + nrand48(s1)%kmod;
  for(i=0; i<klen; i++) {
    kval[i] = klo + nrand48(s1)%krng;
  }
  if (ki>0) {
    kval[klen++] = '~';
    klen += sprintf(((char*)kval)+klen, "%d", ki);
  }

  kval[klen] = 0;
}
void gen_key_val(int ki) {
  int i;
  gen_key(ki);
  vlen = vmn + nrand48(s1)%vmod;
  for(i=0; i<vlen; i++) {
    vval[i] = vlo + nrand48(s1)%vrng;
  }
  vval[vlen] = 0;
}
void adv_key_val(int ki) {
  gen_key_val(ki);
  bvs[ki*3+0] =  s1[0];
  bvs[ki*3+1] =  s1[1];
  bvs[ki*3+2] =  s1[2];
  gen_key_val(ki);
}

#define PRINT_GET(ki)  if (progress) printf("Ki %d Get kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'\n", \
	     ki, klen, vlen,bvi,					\
	     cstr ? (char*)kval : "")					\
	
#define PRINT_SET(ki)	if (progress) printf("Ki %d Set kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'->'%s'\n",		\
	     ki, klen, vlen,bvi,					\
	     cstr ? (char*)kval : "",					\
	     cstr ? (char*)vval : "")
	
#define PRINT_DEL(ki) if (progress) printf("Ki %d Del kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'\n", \
			     ki, klen, vlen, bvi,			\
			     cstr ? (char*)kval : "");			

#define SWAPBKT       btx = bti;      bti = bto;      bto = btx

int tst_bt_entry(int argc, char **argv)
{
  int i;

  char *anon[] = {NULL,NULL,NULL,NULL};
  CL args[] = {{"-n", &iter, AR_INT, "iterations per thread"},
	       {"-k", &keys, AR_INT, "# of keys"},
	       {"-s", &seed, AR_INT, "random seed"},
	       {"-b", &bs, AR_INT, "bucket size"},
	       {"-simple", &simple, AR_FLG, "simple test"},
	       {"-cstr", &cstr, AR_FLG,"c string keys and values"},
	       {"-table", &table, AR_FLG, "print table"},
	       {"-progress", &progress, AR_FLG, "print progress"},
	       { "-kmn", &kmn, AR_INT, "kmn"},
	       { "-kmx", &kmx, AR_INT, "kmx"},
	       { "-klo", &klo, AR_INT, "klo"},
	       { "-khi", &khi, AR_INT, "khi"},
	       { "-vmn", &vmn, AR_INT, "vmn"},
	       { "-vmx", &vmx, AR_INT, "vmx"},
	       { "-vlo", &vlo, AR_INT, "vlo"},
	       { "-vhi", &vhi, AR_INT, "vhi"},
	       { "-vdf", &vdf, AR_INT, "vdf"},
	       {NULL, NULL, 0, NULL}};

  int echo = 1;
  (void)cl_parse(argc,argv,args,anon,echo);

  kmod = kmx - kmn + 1;
  krng = khi - klo + 1;

  vmod = vmx - vmn + 1;
  vrng = vhi - vlo + 1;

  srand48(seed);
  s0[0] = lrand48();
  s0[1] = lrand48();
  s0[2] = lrand48();
  
  bvk = bv_new(keys);
  bvs = malloc(3 * keys * sizeof(unsigned short));

  kval = malloc(kmx);
  vval = malloc(vmx);
  gval = malloc(vmx);

  for (i=0; i<3*keys; i++) bvs[i] = lrand48();

  bti = bt_new(bs,vmx,vdf, NULL, &(bti_orig));
  bto = bt_new(bs,vmx,vdf, NULL, &(bto_orig));

  int64_t rc = -1;

  int64_t get_cnt  = 0;
  int64_t get_succ = 0;
  int64_t get_fail = 0;
  int64_t get_err  = 0;

  int64_t set_cnt  = 0;
  int64_t set_succ = 0;
  int64_t set_fail = 0;
  int64_t set_err  = 0;

  int64_t del_cnt  = 0;
  int64_t del_succ = 0;
  int64_t del_fail = 0;
  int64_t del_err  = 0;


  if (simple) {
    //addinitial
    BT_SET("abc", "defg");
    BT_SET("hijkl", "mnopqrst");

    BT_EXI("abc", 5);
    BT_EXI("cba", -1);

    BT_SET("uv", "wxyz12");
    BT_SET("3456", "7");
    BT_SET("8", "90");
    // replace a few
    BT_SET("uv", "!@#$");
    BT_SET("8", "%^&*(");
    BT_SET("8", ")_+");
    // add some
    BT_SET("ABCDEFGHIJKLM", "NOPQRSTUVWXYZ");
    // delete some
    BT_DEL("8", 4);
    BT_DEL("3456", 2);
    // delete nonexist
    BT_DEL("Ui", -1);
    // delete the rest
    BT_DEL("ABCDEFGHIJKLM", 14);
    BT_DEL("hijkl", 9);
    BT_DEL("uv", 5);
    BT_DEL("abc", 5);
    //addinitial
    BT_SET("abc", "defg");
    BT_SET("hijkl", "mnopqrst");
    BT_SET("uv", "wxyz12");
    BT_SET("3456", "7");
    BT_SET("8", "90");
    // replace a few
    BT_SET("uv", "!@#$");
    BT_SET("8", "%^&*(");
    BT_SET("8", ")_+");
    // add some
    BT_SET("ABCDEFGHIJKLM", "NOPQRSTUVWXYZ");
    // delete some
    BT_DEL("8", 4);
    BT_DEL("3456", 2);
    // delete nonexist
    BT_DEL("Ui",-1);
    // do some gets
    BT_GET("hijkl", "mnopqrst");
    // BT_GET("Nope", NULL);
    BT_GET("uv", "!@#$");
    //get out
    return(0);
  }
  int bvi;
  for(i=0; i<iter; i++) {
    
    int ki = nrand48(s0) % keys;
    int op = nrand48(s0) % 3;
    uint64_t ovlen;
    bvi = bv_get(bvk,ki);
    if (table) {if (cstr) bt_cstr(bti); else bt_dump(bti);}
    switch (op) {
    case 0: // get
      {
	get_cnt++;
	gen_key_val(ki);
	PRINT_GET(ki);
	rc = bt_get(bti,klen,kval,gval);
	if (bvi==1 && rc==vlen && memcmp(vval,gval,vlen)==0) {
	  get_succ++;
	} else if (bvi==0 && rc==-1) {
	  get_fail++;
	} else {
	  printf("ERR: Get rc = %"PRIi64" bvi = %d\n", rc, bvi);
	  get_err++;
	}
	break;
      }
    case 1: // set
      {
	set_cnt++;
	adv_key_val(ki);
	PRINT_SET(ki);
	rc = bt_set(bto, bti,klen,kval,vlen,vval, &ovlen);
	//if (rc != vlen) {
	//  if (rc!=-1) {
	//    printf("ERR: didn't store value, rc = %"PRIi64"\n",rc);
	//    set_err++;
	//  } else {
	//    set_fail++;
	//  }
	//} else {
	  bv_set(bvk,ki);
	  set_succ++;
	//}
	SWAPBKT;
	break;
      }
    case 2: // del
      {
	del_cnt++;
	gen_key_val(ki);
	PRINT_DEL(ki);
	rc = bt_del(bto,bti,klen,kval);					
	if (bvi) {
	  if (rc != vlen) {
	    printf("ERR: bt_del rc = %"PRIi64" expected %"PRIi64"\n", rc, vlen);
	    del_err++;
	  } else {
	    bv_clr(bvk,ki);
	    del_succ++;
	  }
	} else {
	  if (rc != -1) {
	    printf("ERR: bt_del rc = %"PRIi64" expected %d\n", rc, -1);
	    del_err++;
	  } else {
	    del_fail++;
	  }
	}
	SWAPBKT;     
	break;
      }
    default:
      return(1);
    }
  }
  printf("Summary:\n");
  printf("Set %"PRIu64" pass = %"PRIu64", fail = %"PRIu64", error = %"PRIu64"\n", set_cnt, set_succ, set_fail, set_err);
  printf("Get %"PRIu64" pass = %"PRIu64", fail = %"PRIu64", error = %"PRIu64"\n", get_cnt, get_succ, get_fail, get_err);
  printf("Del %"PRIu64" pass = %"PRIu64", fail = %"PRIu64", error = %"PRIu64"\n", del_cnt, del_succ, del_fail, del_err);

  return(get_err + set_err + del_err);
}
