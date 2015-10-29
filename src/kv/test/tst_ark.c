/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_ark.c $                                       */
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
#include <stdint.h>
#include <inttypes.h>

#include "cl.h"
#include "bv.h"
#include "arkdb.h"
#include <ark.h>

int iter = 10;
int keys = 10;
int seed = 1234;

int simple = 0;
int cstr  = 0;
int table = 0;
int progress = 0;
int chk = 0;

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

uint64_t vlim = 256;

char *dev = NULL;

int vdf = 8;

int64_t total_keys = 0;

uint64_t klen = 0;
uint8_t *kval = NULL;

uint64_t vlen = 0;
uint8_t *vval = NULL;

uint64_t glen = 0;
uint8_t *gval = NULL;

uint64_t size = 1024 * 1024;

uint64_t bs = 4096;
//uint64_t bc = 1024;

unsigned short s0[3];
unsigned short s1[3];

BV *bvk = NULL;
unsigned short *bvs = NULL;

/* BT *bti = NULL; */
/* BT *bto = NULL; */
/* BT *btx = NULL; */

int64_t bt_set_ret = 0;
#define BT_SET(k,v)						\
  ark_set(ark,(uint64_t)(strlen(k)+1), (uint8_t *)(k), (uint64_t)(strlen(v)+1), (uint8_t *)(v), &bt_set_ret); \
  printf("%s %"PRIi64" = Set '%s' -> '%s'\n",bt_set_ret==strlen(v)+1 ? "OK " : "ERR", bt_set_ret, k, v); \
  if (strlen(v)+1!=bt_set_ret) return(99)


int64_t bt_exi_ret = 0;
#define BT_EXI(k,e)							\
  ark_exists(ark,(uint64_t)(strlen(k)+1), (uint8_t *)(k), &bt_exi_ret);		\
  printf("%s %"PRIi64" = Exists '%s'\n",e==bt_exi_ret ? "OK " : "ERR", bt_exi_ret, k); \
  if (e!=bt_exi_ret) return(99)


int64_t bt_del_ret = 0;
#define BT_DEL(k,rc)							\
  ark_del(ark,(uint64_t)(strlen(k)+1), (uint8_t *)(k), &bt_del_ret );		\
  printf("%s %"PRIi64" = Del '%s'\n",bt_del_ret==rc ? "OK " : "ERR", bt_del_ret, k); \


int64_t bt_get_ret = 0;
#define BT_GET(k,v)							\
  ark_get(ark,(uint64_t)(strlen(k)+1), (uint8_t*)(k), (uint64_t)vmx, vval, 0, &bt_get_ret); \
  printf("%s %"PRIi64" = Get '%s'\n", \
    ((v==NULL && bt_get_ret==-1) || \
     ((strlen(v)+1)==bt_get_ret && memcmp(v,vval,bt_get_ret)==0)) ? "OK" : "ERR", \
     bt_get_ret, k);                        \

void gen_key(int ki) {
  int i;
  s1[0] = bvs[ki*3+0];
  s1[1] = bvs[ki*3+1];
  s1[2] = bvs[ki*3+2];
  klen = kmn + nrand48(s1)%kmod;
  for(i=0; i<(int)klen; i++) {
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
  for(i=0; i<(int)vlen; i++) {
    vval[i] = vlo + nrand48(s1)%vrng;
  }
  vval[vlen] = 0;
}
void adv_key_val(int ki) {
  gen_key_val(ki);
  bvs[ki*3+0] =  s1[0];
  bvs[ki*3+1] =  

s1[1];
  bvs[ki*3+2] =  s1[2];
  gen_key_val(ki);
}

#define PRINT_GET(ki)  if (progress) printf("Ki %d Get kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'\n", \
         ki, klen, vlen,bvi,                    \
         cstr ? (char*)kval : "")                   \

#define PRINT_SET(ki)   if (progress) printf("Ki %d Set kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'->'%s'\n",        \
         ki, klen, vlen,bvi,                    \
         cstr ? (char*)kval : "",                   \
         cstr ? (char*)vval : "")

#define PRINT_DEL(ki) if (progress) printf("Ki %d Del kl = %"PRIu64", vl = %"PRIu64" bvi %d '%s'\n", \
                 ki, klen, vlen, bvi,           \
                 cstr ? (char*)kval : "");

/* #define SWAPBKT       btx = bti;      bti = bto;      bto = btx */

int bref = 34;
int grow = 1024;
uint64_t hc = 16;

int tst_ark_entry(int argc, char **argv)
{
  int i;
  int cnt_rc;
  int close_rc;
  int64_t cnt;
  //uint64_t flags = ARK_KV_VIRTUAL_LUN; //unused for now

  char *anon[] = {NULL,NULL,NULL,NULL};

  CL args[] = {
           { (char*)"-n", &iter, AR_INT, (char*)"iterations per thread"},
           { (char*)"-k", &keys, AR_INT, (char*)"# of keys"},
           { (char*)"-s", &seed, AR_INT, (char*)"random seed"},
           { (char*)"-bs", &bs, AR_INT64, (char*)"block size"},
           //          {"-bc", &bc, AR_INT64, (char*)"block count (for growing)"},
           { (char*)"-hc", &hc, AR_INT64, (char*)"hash count"},
           { (char*)"-size", &size, AR_INT64, (char*)"inital storage size"},
           { (char*)"-simple", &simple, AR_FLG, (char*)"simple test"},
           { (char*)"-cstr", &cstr, AR_FLG, (char*)"c string keys and values"},
           { (char*)"-table", &table, AR_FLG, (char*)"print table"},
           { (char*)"-progress", &progress, AR_FLG, (char*)"print progress"},
           { (char*)"-kmn", &kmn, AR_INT, (char*)"kmn"},
           { (char*)"-kmx", &kmx, AR_INT, (char*)"kmx"},
           { (char*)"-klo", &klo, AR_INT, (char*)"klo"},
           { (char*)"-khi", &khi, AR_INT, (char*)"khi"},
           { (char*)"-vmn", &vmn, AR_INT, (char*)"vmn"},
           { (char*)"-vmx", &vmx, AR_INT, (char*)"vmx"},
           { (char*)"-vlo", &vlo, AR_INT, (char*)"vlo"},
           { (char*)"-vhi", &vhi, AR_INT, (char*)"vhi"},
           { (char*)"-vdf", &vdf, AR_INT, (char*)"vdf"},
           { (char*)"-bref", &bref, AR_INT, (char*)"block ref bits"},
           { (char*)"-grow", &grow, AR_INT, (char*)"# of blocks to grow by"},
           { (char*)"-vlim", &vlim, AR_INT64, (char*)"value limit for bucket store"},
	   { (char*)"-dev", &dev, AR_STR, (char*)"Device to be used for store"},
	   { (char*)"-chk", &chk, AR_FLG, (char*)"Run key count check"},
           { NULL, NULL, 0, NULL}
           };

  int echo = 0;
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
  bvs = (unsigned short*)malloc(3 * keys * sizeof(unsigned short));

  kval = (uint8_t*)malloc(kmx);
  vval = (uint8_t*)malloc(vmx);
  gval = (uint8_t*)malloc(vmx);

  for (i=0; i<3*keys; i++) bvs[i] = lrand48();

  ARK *ark  = NULL;
  int rc_ark = ark_create_verbose(dev, &ark, size, bs, hc, 
                                  4, 256, 256,
                                  ARK_KV_VIRTUAL_LUN);

  if (rc_ark != 0)
  {
    fprintf(stderr, "ark_create failed\n");
    return 1;
  }

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

  int64_t cnt_cnt  = 0;
  int64_t cnt_succ = 0;
  int64_t cnt_fail = 0;

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
    //BT_GET("Nope", NULL);
    BT_GET("uv", "!@#$");
    BT_GET("ABCDEFGHIJKLM", "NOPQRSTUVWXYZ");
    //get out
    return(0);
  }
  int bvi;
  for(i=0; i<iter; i++) {

    int ki = nrand48(s0) % keys;
    int op = nrand48(s0) % 4;
    bvi = bv_get(bvk,ki);
    //    if (table) {if (cstr) bt_cstr(bti); else bt_dump(bti);}
    switch (op) {
    case 0: // get
      {
	get_cnt++;
	gen_key_val(ki);
	PRINT_GET(ki);
	ark_get(ark,klen,kval,vmx,gval,0, &rc);
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
	ark_set(ark,klen,kval,vlen,vval, &rc);
	if (rc != vlen) {
	  if (rc!=-1) {
	    printf("ERR: didn't store value, rc = %"PRIi64"\n",rc);
	    set_err++;
	  } else {
	    set_fail++;
	  }
	} else {
	  bv_set(bvk,ki);
	  set_succ++;
          total_keys++;
	}
	//	SWAPBKT;
	break;
      }
    case 2: // del
      {
	del_cnt++;
	gen_key_val(ki);
 	PRINT_DEL(ki);
	ark_del(ark,klen,kval,&rc);
	if (bvi) {
	  if (rc != vlen) {
	    printf("ERR: bt_del rc = %"PRIi64" expected %"PRIi64"\n", rc, vlen);
	    del_err++;
	  } else {
	    bv_clr(bvk,ki);
	    del_succ++;
	    total_keys--;
	  }
	} else {
	  if (rc != -1) {
	    printf("ERR: bt_del rc = %"PRIi64" expected %d\n", rc, -1);
	    del_err++;
	  } else {
	    del_fail++;
	  }
	}
	//	SWAPBKT;     
	break;
      }
    case 3: // count
      {
        // Decrement the op count so that we can still do the
	// same number of other ops
        i--;
        if (chk)
	{
          cnt_cnt++;
	  cnt_rc = ark_count(ark, &cnt);
	  if (cnt_rc == 0)
	  {
            if ( cnt == total_keys)
	    {
              cnt_succ++;
	    }
	    else
	    {
              printf("ERR: ark_count return %"PRIi64", expect %"PRIi64"\n", cnt, total_keys);
	    cnt_fail++;
	    }
	  }
	  else
	  {
            printf("ERR: ark_count failed: %d\n", cnt_rc);
	    cnt_fail++;
	  }
	}
        break;
      }
    default:
      return(1);
    }
    //printf("\nBlocks %"PRIu64" / %"PRIu64" (%g)\n", ark->blkused, ark->bcount, (double)ark->blkused / (double)ark->bcount);
  }
  printf("Summary:\n");
  printf("Set %"PRIu64" stored    = %9"PRIu64", not stored = %9"PRIu64", error = %9"PRIu64"\n", set_cnt, set_succ, set_fail, set_err);
  printf("Get %"PRIu64" retrieved = %9"PRIu64", not found  = %9"PRIu64", error = %9"PRIu64"\n", get_cnt, get_succ, get_fail, get_err);
  printf("Del %"PRIu64" removed   = %9"PRIu64", not found  = %9"PRIu64", error = %9"PRIu64"\n", del_cnt, del_succ, del_fail, del_err);
  if (chk)
  {
    printf("Cnt %"PRIu64" success   = %9"PRIu64", error      = %9"PRIu64"\n", cnt_cnt, cnt_succ, cnt_fail);
  }

  close_rc = ark_delete(ark);
  if (close_rc != 0)
  {
    printf("ARK_DELETE failed: %d\n", close_rc);
  }

  return(get_err + set_err + del_err);
}
