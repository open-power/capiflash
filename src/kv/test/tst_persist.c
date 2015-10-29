/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/test/tst_persist.c $                                   */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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
#include "arkdb.h"

char *device = NULL;

//int main(int argc, char **argv)
int tst_persist_entry(int argc, char **argv)
{
  int i;
  int rc = 0;
  int64_t res;
  //uint64_t flags = ARK_KV_VIRTUAL_LUN;
  uint64_t flags = 0;
  int64_t keycnt = 0;
#define VALUE_BUFFER_SIZE 256
  char val_buffer[VALUE_BUFFER_SIZE];
#define NUM_KEYS  3
  char *keys[NUM_KEYS] = {"abc", "defg", "hijkl" };
  char *vals[NUM_KEYS] = {"123", "4567", "89012" };

  char *anon[] = {NULL,NULL,NULL,NULL};

  CL args[] = {
	   { (char*)"-dev", &device, AR_STR, (char*)"Device to be used for store"},
           { NULL, NULL, 0, NULL}
           };

  int echo = 0;
  (void)cl_parse(argc,argv,args,anon,echo);

  ARK *ark  = NULL;
  rc = ark_create_verbose(device, &ark, 1024 * 1024, 4096, 4096, 20,
                             256, 256, flags|ARK_KV_PERSIST_STORE);
  //rc = ark_create(device, &ark, flags|ARK_KV_PERSIST_STORE);

  if (rc != 0)
  {
    fprintf(stderr, "ark_create failed\n");
    return 1;
  }


  for (i = 0; i < NUM_KEYS; i++)
  {
    rc = ark_set(ark, (uint64_t)strlen(keys[i]), 
                   (void *)keys[i], (uint64_t)strlen(vals[i]),
		   (void *)vals[i], &res);
    if ( rc != 0 )
    {
      printf("ark_set failed for key: %d (%d)\n", i, rc);
      break;
    }
  }

  if ( rc == 0 )
  {
    ark_delete(ark);

    sleep(2);

    rc = ark_create(device, &ark, flags|ARK_KV_PERSIST_LOAD);
    if (rc != 0)
    {
      printf("ark_create persist open failed: %d\n", rc);
    }
  }

  if (rc == 0)
  {
    for (i = 0; i < NUM_KEYS; i++)
    {
      res = -1;
      memset(&(val_buffer[0]), 0, VALUE_BUFFER_SIZE);
      rc = ark_get(ark, (uint64_t)strlen(keys[i]), 
                   (void *)keys[i], VALUE_BUFFER_SIZE,
		   (void *)&(val_buffer[0]), 0, &res);
      if (rc != 0)
      {
        printf("ark_get failed: %d (%d)\n", i, rc);
	break;
      }

      if (res != (uint64_t)strlen(vals[i]))
      {
        printf("ark_get didn't return correct len: exp: %"PRIu64" act: %"PRIu64"\n", (uint64_t)strlen(vals[i]), res);
	break;
      }

      printf("Key: %s, value: %s\n", keys[i], vals[i]);
    }

    rc = ark_count(ark, &keycnt);
    if (rc != 0)
    {
      printf("ark_count failed: %d\n", rc);
    }
    else
    {
      if (keycnt != NUM_KEYS)
      {
        printf("Wrong number of keys: %"PRIu64"\n", keycnt);
	rc = -1;
      }
      else
      {
        printf("Correct number of keys: %"PRIu64"\n", keycnt);
      }
    }
  }

  ark_delete(ark);

  return rc;
}
