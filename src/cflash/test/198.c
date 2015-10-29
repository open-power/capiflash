/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/198.c $                                    */
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

// create VLUN
int create_vluns_198(char *dev, dev64_t devno,
                      __u16 lun_type, struct ctx *p_ctx, pthread_t thread)
{
    int rc,i,flag=0;

    __u64 nlba;
    __u64 stride= 0x10000;
    int long_run = atoi(getenv("LONG_RUN"));
    nlba=p_ctx->lun_size;
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    if ( LUN_VIRTUAL == lun_type)
    {
        rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
        CHECK_RC(rc, "create LUN_VIRTUAL failed");
    }

    for ( i=0; i<long_run; i++ )
    {
        rc = do_io(p_ctx, stride);
        if ( rc != 0 )
            flag=1;
    }

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return flag;
}

int ioctl_7_1_198( int flag )
{
    int rc,i;
    struct ctx myctx[2];
    struct ctx *p_ctx[2];
    struct flash_disk fldisks[MAX_FDISK];
    __u64 temp;
    flag=0;
    pid=getpid();

    get_flash_disks(fldisks,  FDISKS_ALL);

    sleep(10);
    i=1;
    p_ctx[i]=&myctx[i];
    rc = ctx_init(p_ctx[i]);
    //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
    rc = create_resource(p_ctx[i], p_ctx[i]->chunk_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "Context init failed");

    if ( 0 == fork() )
    {
        i=0;
        p_ctx[i]=&myctx[i];
        rc = ctx_init(p_ctx[i]);
        //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
        rc = create_resource(p_ctx[i], p_ctx[i]->chunk_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
        CHECK_RC(rc, "Context init failed");

        //    rc = create_vluns_198(fldisks[0].dev, fldisks[0].devno[0],
        //                          LUN_VIRTUAL, p_ctx[i],thread[i]);

        sleep(1);
        debug("%lu\n", p_ctx[1]->context_id);
        // use the resource handler of process 2 with context_token of process 1 and call all ioctls
        temp=p_ctx[0]->context_id;
        p_ctx[0]->context_id=p_ctx[1]->context_id;
        // resize
        rc = vlun_resize(p_ctx[0], (p_ctx[0]->chunk_size)*2);
        // CHECK_RC(rc, "vlun_resize failed");
                                 // detach
        if ( rc == 0 )
            flag=1;
        rc = ioctl_dk_capi_detach(p_ctx[0]);
        // CHECK_RC(rc, "ctx_close failed");
        if ( rc == 0 )
            flag=1;

        // release
        rc = ioctl_dk_capi_release(p_ctx[0]);
        if ( rc == 0 )
            flag=1;
        // CHECK_RC(rc, "close_res failed");
                                 //lun_create
        rc = create_resource(p_ctx[0], p_ctx[0]->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
        if ( rc == 0 )
            flag=1;
        // CHECK_RC(rc, "create LUN_VIRTUAL failed");
        // log report
        rc = ioctl_dk_capi_log(p_ctx[0],"");
        if ( rc == 0 )
            flag=1;
        // CHECK_RC(rc, "ioctl_dk_capi_log failed");
        // dk_exception
        rc=ioctl_dk_capi_query_exception(p_ctx[0]);
        if ( rc == 0 )
            flag=1;
        //  CHECK_RC(rc, "ioctl_dk_capi_exception failed");
        //dk_recover
        rc=ioctl_dk_capi_recover_ctx(p_ctx[0]);
        if ( rc == 0 )
            flag=1;
        // CHECK_RC(rc, "ioctl_dk_capi_recover failed");
        //using attach againd
#ifdef _AIX
        p_ctx[0]->flags=DK_AF_REUSE_CTX;
#else
        p_ctx[0]->flags=DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
        rc=ioctl_dk_capi_attach(p_ctx[0]);
        if ( rc == 0 )
            flag=1;
        //CHECK_RC(rc, "ioctl_dk_capi_attach failed");
        debug("flag=%d\n",flag);
        p_ctx[0]->context_id=temp;
        p_ctx[0]->context_id=temp;
        close_res(p_ctx[0]);
        ctx_close(p_ctx[0]);
        exit(flag);
    }
    sleep(30);
    close_res(p_ctx[1]);
    ctx_close(p_ctx[1]);
    return flag;
}

