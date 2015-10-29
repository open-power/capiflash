/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/191.c $                                       */
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

int ioctl_7_1_191( int flag )
{
    int rc,j,i;
    struct ctx myctx;
    struct ctx *p_ctx;
    char *disk_name=NULL, *str=NULL;

    pid = getpid();
    str = (char *) malloc(100);

    disk_name = (char *)getenv("FVT_DEV");
    disk_name = strtok(disk_name,"/");
    disk_name = strtok(NULL,"/");


    p_ctx=&myctx;
    //pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

                    // attach and detach of same LUN
    for (j=0; j<long_run; j++ )
    {
        for (i=0;i<long_run;i++)
        {
                        //this need super pipe IO attach detach
            rc=ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");
            rc = create_res(p_ctx);
            CHECK_RC(rc, "Context init failed");
            rc = close_res(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_release failed");
            rc= ctx_close(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_detach failed");
        }
        debug("i am out of first loopi disk name= %s \n", disk_name);
        //config/unconfig of the disk
#ifdef _AIX
        for (i=0;i<long_run;i++)
        {
            sprintf(str, "rmdev -l %s ", disk_name);
            rc=system(str);

            CHECK_RC(rc,"rmdev of disk failed");
            sprintf(str, "cfgmgr -l %s", disk_name);
            rc=system(str);

            CHECK_RC(rc,"mkdev of disk failed");
        }
#endif
    }

#ifdef _AIX
    // now we do a simple I/O, run dd for long time.Alternatively we can also use the disk to create a VG and then fs
    for (i=0;i<long_run;i++)
    {

        sprintf(str, "dd if=/usr/lib/boot/unix_64 of=/dev/%s", disk_name);
        rc=system(str);

        CHECK_RC(rc,"dd on disk failed");
    }
#endif

    // after dd we need to test for superpipe i/o. We do VLUN first and then DLUN
    rc=create_multiple_vluns(p_ctx);
    if ( rc != 0 )
        flag=1;
    rc=create_direct_lun(p_ctx);
    if ( rc != 0 )
        flag=2;
    free(str);
    if ( flag == 1 )
    {
        printf( "some error with LUN_VIRTUAL \n");
        return 1;
    }
    if ( flag == 2 )
    {
        printf("some error with LUN_DIRECT \n");
        return 1;
    }

    printf("all case succeeded \n");
    return 0;
}


