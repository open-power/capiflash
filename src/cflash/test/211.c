/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/211.c $                                       */
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

struct ctx *p_ctx,*p_ctx1;
struct ctx myctx, myctx1;

// creating thread for creation VLUN or PLUN
void *create_lun(void *arg)
{
    int *ptr = (int *)arg;
    int flag = *ptr;
    //    int rc;
    if ( flag == LUN_DIRECT )
        create_resource(p_ctx, p_ctx->lun_size, DK_UDF_ASSIGN_PATH, flag);
    else
        create_resource(p_ctx, p_ctx->lun_size, DK_UVF_ALL_PATHS, flag);
    //    return &rc;
    return NULL;
}


int ioctl_7_1_211( int flag )
{
    int rc,i;
    struct ctx myctx;
    struct ctx *p_ctx =&myctx;
    for (i=0;i<2;i++)
    {
        if (0 == fork())
        {
            //child process
            pid =getpid();
            rc = ctx_init(p_ctx);
            CHECK_RC_EXIT(rc, "ctx_init failed\n");
            usleep(1000);
            if ( i == 0 )
                rc = create_resource(p_ctx, p_ctx->lun_size, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            else
                rc = create_resource(p_ctx, p_ctx->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            sleep(1);
            close_res(p_ctx);
            ctx_close(p_ctx);
            exit(rc);
        }
    }
    rc =  wait4all();
    return rc;
}
