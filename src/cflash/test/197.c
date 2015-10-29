/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/197.c  $                                      */
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


int ioctl_7_1_197( int flag )
{
    int rc;
    struct ctx myctx, myctx1, myctx2;
    struct ctx *p_ctx = &myctx;
    struct ctx *p_ctx1 = &myctx1, *temp=&myctx2;
    pthread_t thread,thread1;
    pid = getpid();
    flag=0;
    // creating first context
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    // creating another context
    rc = ctx_init(p_ctx1);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread1, NULL, ctx_rrq_rx, p_ctx1);

    // Creating resource for both ctx token
    rc = create_resource(p_ctx, p_ctx->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    rc = create_resource(p_ctx1, p_ctx1->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    rc = create_resource(p_ctx1, p_ctx1->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    // use the resource handler of process 2 with context_token of process 1 and call all ioctls
    temp->rsrc_handle=p_ctx->rsrc_handle;
    p_ctx->rsrc_handle=p_ctx1->rsrc_handle;
    // resize
    rc = vlun_resize(p_ctx, (p_ctx->lun_size)*2);
    if ( rc == 0 )
        flag=1;
                            // detach
    // release
    rc = ioctl_dk_capi_release(p_ctx);
    if ( rc == 0 )
        flag=1;
                            //lun_create
    rc = create_resource(p_ctx, p_ctx->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    if ( rc != 0 )
        flag=1;
    // log report
#ifdef _AIX
    rc = ioctl_dk_capi_log(p_ctx,"");
    if ( rc == 0 )
        flag=1;
    // dk_exception
    rc=ioctl_dk_capi_query_exception(p_ctx);
    if ( rc == 0 )
        flag=1;
#endif
    //dk_recover
    rc=ioctl_dk_capi_recover_ctx(p_ctx);
    if ( rc != 0 )
        flag=1;
    //using attach again
#ifdef _AIX
    p_ctx->flags=DK_AF_REUSE_CTX;
#else
    p_ctx->flags=DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
    rc=ioctl_dk_capi_attach(p_ctx);
    if ( rc == 0 )
        flag=1;
    rc = ioctl_dk_capi_detach(p_ctx);
    if ( rc != 0 )
        flag=1;
    p_ctx->context_id=temp->context_id;
    pthread_cancel(thread);
    pthread_cancel(thread1);
    close_res(p_ctx1);
    ctx_close(p_ctx1);
    return flag;
}


