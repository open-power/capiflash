/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/210.c $                                       */
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
extern int g_errno;
extern pid_t pid;

int FREE_SIZE=1;
int DISK_SIZE=1;


int ioctl_7_1_210( int flag )
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    struct flash_disk disks[MAX_FDISK];
    pthread_t thread;
    __u64 nlba=0,stride=0x10;
    get_flash_disks(disks, FDISKS_ALL);
    memset(p_ctx, 0, sizeof(struct ctx));
    // open capi flash disk in read mode first
    strcpy(p_ctx->dev,disks[0].dev);
    pid=getpid();
    debug("%s\n",p_ctx->dev);
    p_ctx->fd = open_dev(p_ctx->dev, O_RDONLY);

    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", p_ctx->dev, errno);
        g_error = -1;
        return -1;
    }

#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "query path failed")
#endif
#ifdef _AIX
        rc = ctx_init_internal(p_ctx, DK_AF_ASSIGN_AFU, p_ctx->devno);
#else
        rc = ctx_init_internal(p_ctx, 0x0,  p_ctx->devno);
        // Linux wouldn't support context creation
        // until disk is opened in O_RDWR mode, Refer def# SW325500
        if (rc == -1 && g_errno == 1) rc=0;
        else rc=-1;
        g_errno=0;
        return rc;
#endif
    //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    nlba=p_ctx->block_size;
    rc = create_resource(p_ctx, nlba, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");


    // now try to write on the disk, should fail.
    rc = send_write(p_ctx, p_ctx->st_lba, stride, pid);
    //CHECK_RC(rc, "send_write failed");

    if ( rc == 0 )
    {
        debug("write operation succeeded in read mode\n");
        //        return 1;
    }
    // continue with closing the disk and opening in write mode
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    // now open the disk in write mode and try to read.
    p_ctx->fd = open_dev(disks[0].dev, O_WRONLY);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[0].dev, errno);
        g_error = -1;
        return -1;
    }

#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "query path failed")
#endif /*_AIX */

#ifdef _AIX
    rc = ctx_init_internal(p_ctx, DK_AF_ASSIGN_AFU, p_ctx->devno);
#else
    rc = ctx_init_internal(p_ctx, 0x1,  p_ctx->devno);
#endif
                //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    nlba=p_ctx->block_size;
    rc = create_resource(p_ctx, nlba, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    debug("E_test_RD_PRM_WRITE\n");
    // write to disk, which need to be read..
    rc = send_write(p_ctx, p_ctx->st_lba, stride, pid);
    CHECK_RC(rc, "send_write failed");
    // send read command.
    //      system("/dtest_final r cflash0 -S '-port_num 0x1 -lunid  0x500507605e839c53' -a 1 -f 1 -b");
    rc = send_read(p_ctx, p_ctx->st_lba, stride);
    // CHECK_RC(rc, "send_read failed");
    if ( rc == 0 )
    {
        debug("read operation succeeded in write only mode\n");
        return 1;
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return 0;
}
