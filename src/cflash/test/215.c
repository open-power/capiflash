/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/215.c $                                       */
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
// to create multiple context

// create VLUN
int create_vluns_215(char *dev, dev64_t devno,
                      __u16 lun_type, __u64 chunk, struct ctx *p_ctx)
{
    int rc,i,flag=0;

    __u64 nlba;
    __u64 stride= 0x10000;
    nlba=p_ctx->lun_size;
    if ( LUN_DIRECT == lun_type)
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
    close_res(p_ctx);
    ctx_close(p_ctx);
    return flag;
}

int create_multiple_vluns_215(struct ctx *p_ctx)
{
    int rc,j;
    struct flash_disk fldisks[MAX_FDISK];
    __u64 chunks[10];

    get_flash_disks(fldisks,  FDISKS_ALL);
    // use the first disk
    //create atleast 10 chunks on each on PLUN
    for (j=0; j < 10; j++)
    {
        if (0 == fork()) //child process
        {
            rc = create_vluns_215(fldisks[0].dev, fldisks[0].devno[0],
                                  LUN_VIRTUAL,chunks[j], p_ctx);
            exit(rc);
        }
    }

    rc = wait4all();
    return rc;
}

int ioctl_7_1_215( int flag )
{
    int rc;
    struct ctx myctx, myctx1,myctx2;
    struct ctx *p_ctx = &myctx;
    struct ctx *p_ctx1 = &myctx1, *temp=&myctx2;
    pthread_t thread;
    struct flash_disk disks[MAX_FDISK];
    int cfdisk = 0;

    __u64 flags=0;

    memset(temp, 0, sizeof(struct ctx));

    cfdisk = get_flash_disks(disks, FDISKS_SAME_ADPTR);
    if (cfdisk < 2)
    {
        fprintf(stderr,"Must have 2 flash disks..\n");
        TESTCASE_SKIP("Need disk from same adapter");
        return 0;
    }

    // creating first context
    rc = ctx_init2(p_ctx, disks[0].dev, flags, p_ctx->devno);
    // initializing a dummy context for second disk, to use it as temporary, context
    rc = ctx_init2(p_ctx, disks[1].dev, flags, p_ctx->devno);
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    // now using REUSE flag to extend the created context to another disk.
#ifdef _AIX
    p_ctx->flags=DK_AF_REUSE_CTX;
#else
    p_ctx->flags=DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif

    temp->fd=p_ctx->fd;
    p_ctx->fd=p_ctx1->fd;  // initiating fd to second disk.
    rc=ioctl_dk_capi_attach(p_ctx);  // doing detach
    CHECK_RC(rc, "Context with REUSE failed");

    // if reuse succeeded. detach the context
    p_ctx->flags=0;
    p_ctx->fd=temp->fd;  // re initiating fd to first disk
    rc=ioctl_dk_capi_detach(p_ctx);
    CHECK_RC(rc, "Context detach after  REUSE failed");

    /* spawn the VLUNs */
    p_ctx->fd=p_ctx1->fd;  // reinitiating fd to second disk
    rc=create_multiple_vluns_215(p_ctx);
    pthread_cancel(thread);
    close_res(p_ctx1);
    ctx_close(p_ctx1);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;

}


