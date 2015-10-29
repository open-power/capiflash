/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/196.c $                                       */
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

//int MAX_LIMIT=1;

// creating thread for creation VLUN or PLUN
void *create_lun1(void *arg )
{
    struct ctx *p_ctx = (struct ctx *)arg;
    int rc;
    __u64 stride=0x8;
    rc = create_resource(p_ctx, p_ctx->lun_size, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    if ( rc == 0 )
    {
        rc = do_io(p_ctx, stride);
        if ( rc !=0 )
        {   fprintf(stderr,"io failed on attached LUN\n");
            //TBD Fix this return 2;
            return NULL;
        }
    }
    //TBD Fix this return 1;
    //return 1;
    return NULL;
}

int ioctl_7_1_196()
{
    int rc,i,j;
    struct ctx myctx[21],myctx_1, myctx_2;
    struct ctx *p_ctx[21],*p_ctx_1,*p_ctx_2;
    __u64 stride=0x1000,st_lba=0;
    pthread_t thread[20];
    struct flash_disk disks[MAX_FDISK];
    int cfdisk = MAX_FDISK;

    pid = getpid();

    cfdisk = get_flash_disks(disks, FDISKS_SAME_ADPTR);
    //need to check the number of disks
    if (cfdisk < 2)
    {
        fprintf(stderr,"Must have 2 flash disks..\n");
        return -1;
    }
    // creating first context

    for (i=0;i<21;i++)
    {
        p_ctx[i]=&myctx[i];
    }
    p_ctx_1=&myctx_1;
    p_ctx_2=&myctx_2;
    debug("1ST PROCEDURE\n");
    // using p_ctx[[0] for LUN direct for firect disk
    /*    rc = ctx_init2(p_ctx[0], disks[0].dev, DK_AF_ASSIGN_AFU, disks[0].devno[0]);
        pthread_create(&thread[0], NULL, ctx_rrq_rx, p_ctx[0]);
     */
    /*    rc = create_resource(p_ctx[0], 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
        CHECK_RC(rc, "create LUN_DIRECT failed");
     */
    // creating another 19 context LUN VIRTUAL
    for ( i=2;i<21;i++)
    {
        sleep(2);
        rc = ctx_init2(p_ctx[i], disks[1].dev, DK_AF_ASSIGN_AFU, disks[1].devno[0]);
        rc=create_resource(p_ctx[i], p_ctx[i]->chunk_size, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
    }


    // do context reuse for direct LUN
    strcpy(p_ctx[0]->dev,disks[0].dev);
    strcpy(p_ctx[1]->dev,disks[1].dev);
    p_ctx[0]->fd = open_dev(disks[0].dev, O_RDWR);
    if (p_ctx[0]->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[0].dev, errno);
        g_error = -1;
        return -1;
    }
    p_ctx[1]->fd = open_dev(disks[1].dev, O_RDWR);  //Hoping to open second disk
    if (p_ctx[1]->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[1].dev, errno);
        g_error = -1;
    }
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx[0]);
    CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#else
    //TBD for linux
#endif
    p_ctx[0]->work.num_interrupts = p_ctx[1]->work.num_interrupts = 4;


    rc=ioctl_dk_capi_attach_reuse(p_ctx[0],p_ctx[1],LUN_DIRECT);

    //         CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");


    if ( rc != 0 )
    {
        fprintf(stderr,"LUN DIRECT got attached to new disk with VLUN, should have succeeded");
        return rc;
    }


    // initiate I/O on all the LUNs
    for (i=2;i<21;i++)
    {
        pthread_create(&thread[i], NULL, ctx_rrq_rx, p_ctx[i]);
        rc = do_io(p_ctx[i], stride);
    }
    if ( rc != 0 )
    {       fprintf(stderr,"io on some LUN failed");
        return rc;
    }

    for (i=2;i<21;i++)
    {
        pthread_cancel(thread[i]);
        close_res(p_ctx[i]);
    }

    ctx_close(p_ctx[2]);
    debug("2nd PROCEDURE\n");

    // procedure 2 of the same case
    debug("%d: ........Phase 1 done.. Starting 2nd Phase........\n",getpid());
    memset(p_ctx_1, 0, sizeof(struct ctx));

    memset(p_ctx_2, 0, sizeof(struct ctx));
    // open the first flash disk in write mode and create a DIRECT LUN
    p_ctx_1->fd = open_dev(disks[0].dev, O_WRONLY);
    if (p_ctx_1->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[0].dev, errno);
        return -1;
    }
    rc = ctx_init2(p_ctx_1, disks[0].dev, DK_AF_ASSIGN_AFU, disks[0].devno[0]);
    pthread_create(&thread[0], NULL, ctx_rrq_rx, p_ctx_1);
    CHECK_RC(rc, "create context failed");

    rc = create_resource(p_ctx_1, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    // open the same flash disk in read mode again.
    p_ctx_2->fd = open_dev(disks[0].dev, O_RDONLY);
    if (p_ctx_2->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[0].dev, errno);
        return -1;
    }
    rc = ctx_init2(p_ctx_2, disks[0].dev, DK_AF_ASSIGN_AFU, disks[0].devno[0]);
    pthread_create(&thread[1], NULL, ctx_rrq_rx, p_ctx_2);
    CHECK_RC(rc, "create context failed");
    rc = create_resource(p_ctx_2, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    // now write to the disk and then read
    for (st_lba = 0; st_lba <= p_ctx_1->last_lba; st_lba += (NUM_CMDS*stride))
    {
        rc = send_write(p_ctx_1, st_lba, stride, pid);
        CHECK_RC(rc, "send_write failed");
        rc = send_read(p_ctx_2, st_lba, stride);
        CHECK_RC(rc, "send_read failed");
        if (rc !=0 )
        {
            rc = rw_cmp_buf(p_ctx_1, st_lba);
            if (rc != 0)
            {
                fprintf(stderr,"buf cmp failed for lba 0x%lX,rc =%d\n",st_lba,rc);
                break;
            }
        }
    }
    if ( rc != 0 )
        return rc;

    for (i=0;i<2;i++)
    {
        pthread_cancel(thread[i]);
    }

    close_res(p_ctx_1);
    ctx_close(p_ctx_1);
    close_res(p_ctx_2);
    ctx_close(p_ctx_2);

    debug("3rd PROCEDURE\n");


    debug("%d: ........Phase 2 done.. Starting 3rd Phase........\n",getpid());
    // case 3 of the same case
    // creating multiple process for LUN_DIRECT creation.
    for (j=0;j<long_run;j++)
    {
        for (i=0; i<20;i++)
        {
            if ( 0 == fork())
            { rc = ctx_init(p_ctx[i]);
                CHECK_RC(rc, "Context init failed");
                // CHECK_RC(rc, "Context init failed");
                //thread to handle AFU interrupt & events

                rc = create_resource(p_ctx[i], 0, DK_UDF_ASSIGN_PATH , LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                // do io on context
                pthread_create(&thread[i], NULL, ctx_rrq_rx, p_ctx[i]);
                stride=0x1000;
                sleep(2);
                do_io(p_ctx[i], stride);
                pthread_cancel(thread[i]);
                close_res(p_ctx[i]);
                return(rc);
            }
        }
        wait4all();
    }

    return 0;

}
