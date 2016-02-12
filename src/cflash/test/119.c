/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/119.c $                                            */
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

#include "cflash_test.h"
#include <pthread.h>

extern int g_error;
extern pid_t pid;
extern int long_run;


// create VLUN
int create_vluns_119(char *dev, dev64_t devno_1,
                     struct ctx *p_ctx)
{
    int rc,i;
    pthread_t thread;
    __u64 chunk = 0x10;
    __u64 stride= 0x8000, nlba=0;
    __u64 flags=0 ;//TBD

    pid = getpid();
    rc = ctx_init2(p_ctx, dev, flags, devno_1);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    // create VLUN
    nlba = chunk * (p_ctx->chunk_size);
    p_ctx->flags=DK_UVF_ALL_PATHS;
    rc = create_resource(p_ctx, nlba, p_ctx->flags, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");
    for ( i=0; i< long_run; i++)
    {
        stride=0x1000;
        rc = do_io(p_ctx, stride);
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int create_lun_direct_120(char *dev, struct ctx *p_ctx, dev64_t devno_1)
{
    int rc,i=0;

    pthread_t thread;
    __u64 stride= 0x100, nlba=0, flags=0;
    debug("create_lun_direct_120\n");

    rc = ctx_init2(p_ctx, dev, flags, devno_1);
    CHECK_RC(rc, "Context init failed");
    // CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    rc = create_resource(p_ctx, nlba, DK_UDF_ASSIGN_PATH , LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");
    // do io on context
    for ( i=0; i< long_run; i++)
    {
        stride=0x1000;
        rc = do_io(p_ctx, stride);
    }
    sleep(3);
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}


int ioctl_7_1_119_120( int flag )
{

    int rc=0,i,j=2;
    struct ctx myctx[10];
    struct ctx *p_ctx[10];
    dev64_t devno[10]={ '0' };
    struct flash_disk fdisk[MAX_FDISK];
    //int cfdisk = MAX_FDISK;
    pid = getpid();
    rc = get_flash_disks(fdisk, FDISKS_SHARED);
    if( rc == 0 )
      {
          TESTCASE_SKIP("Required Shared disk coming from  two adapter");
          return 0;
      }

    for ( i=0;i<10;i++)
    {
        p_ctx[i]=&myctx[i];
    }

    rc=0;
    // I will use the first disk here for the path
    // need to call dk_query_first to get all path
    // i am assuming my disk to come from 2 adapter for now. This test can be enhanced for any number later.
    // get number of path
#ifdef _AIX
    strcpy(p_ctx[0]->dev,fdisk[0].dev);
    j=ioctl_dk_capi_query_path_get_path(p_ctx[0],devno);
#endif
    switch ( flag )
    {
        case 119:
            for ( i=0; i<j; i++ )
            {
                sleep(1);
                if ( 0 == fork() )
                {
                    rc=create_vluns_119(fdisk[0].dev,devno[i],p_ctx[i]);
                    if ( rc != 0 )
                    {       printf( "some error with LUN_VIRTUAL Creation \n");
                        rc=1;
                    }
                    exit(rc);
                }
            }            break;
        case 120:
            for ( i=0; i<j; i++ )
            {
                sleep(1);
                if ( 0 == fork() )
                {
                    rc=create_lun_direct_120(fdisk[0].dev,p_ctx[i],devno[i]);
                    if ( rc != 0 )
                    {  printf("some error with LUN_DIRECT \n");
                        rc=1;
                    }
                    exit(rc);
                }
            }
    }
    rc=wait4all();
    return rc;
}


