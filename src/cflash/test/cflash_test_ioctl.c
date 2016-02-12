/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_ioctl.c $                         */
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

#define SLEEP_FOR_RACE 2
extern int MAX_RES_HANDLE;
extern int g_error;
extern int g_errno;
extern pid_t pid;
extern char cflash_path[MC_PATHLEN];
static int threadRC;
#ifdef _AIX
extern int irPFlag ;
#endif

char *diskList[MC_PATHLEN];
int  diskCount = 0 ;

int call_attach_diff_devno()
{
    int rc=0,i,j=2;
    struct ctx myctx[10];
    struct ctx *p_ctx[10];
    dev64_t devno[10]={ '0' };
    struct flash_disk disks[MAX_FDISK];
    //int cfdisk = MAX_FDISK;
    pid = getpid();
    rc = get_flash_disks(disks, FDISKS_SHARED);
    if ( rc == 0 )
        CHECK_RC(1, "shared disk not found\n");
    for ( i=0;i<10;i++)
    {
        p_ctx[i]=&myctx[i];
    }

    strcpy(p_ctx[0]->dev,disks[0].dev);
#ifdef _AIX
    j=ioctl_dk_capi_query_path_get_path(p_ctx[0],devno);
#endif
    if ( j < 2 )
        return 1;

    for ( i=0; i<j; i++ )
    {
        sleep(10);
        rc = ctx_init2(p_ctx[i], disks[0].dev, 0, devno[i]);

        if ( rc != 0 )
        {       printf( "some error with ctx  Creation \n");
            return 1;
        }
    }
    return 0;
}

int prepDiskList(char *dList)
{
    char *ptrP = NULL;

    ptrP = strtok( dList, "," );

    while (  ptrP != NULL )
    {
        diskList[diskCount]=ptrP;
        diskCount=diskCount+1;
        ptrP=strtok(NULL,",");
    }
    return 0;
}


void * verify_ioctl_thread(void * p_arg)
{
    struct ctx *p_ctx;
    p_ctx = (struct ctx *)p_arg;

    threadRC=0;
    threadRC = ioctl_dk_capi_verify(p_ctx);
    pthread_exit(NULL);
}

#ifdef _AIX
int test_dcqp_ioctl(int flag)   //func@DK_CAPI_QUERY_PATH
{
    int rc;
    int rsv_policy;  // TBD the valid values of reserve policy
    char cmd[200];
    char *sub;
    struct ctx u_ctx;
    struct ctx *p_ctx = &u_ctx;
    struct dk_capi_paths capi_paths;
    struct cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[MAX_PATH-1];
    }capi_paths_struct;
    memset(p_ctx,0, sizeof(u_ctx));
    struct dk_capi_path_info *path_info = NULL;
    strcpy(p_ctx->dev, cflash_path);
    sub = strtok(cflash_path,"/");
    sub = strtok(NULL,"/");
    memset(&capi_paths,0,sizeof(capi_paths));
    memset(&capi_paths_struct,0,sizeof(capi_paths_struct));
    pthread_t thread;
    int i;
    pid = getpid();
    switch ( flag )
    {
        case 1: // TCN@7.1.1
            //TEST_DCQP_VALID_PATH_COUNT
            p_ctx->fd = open_dev(p_ctx->dev, O_RDWR);
            if (p_ctx->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n",p_ctx->dev, errno);
                g_error = -1;
                return -1;
            }
            capi_paths.version = p_ctx->version;
            capi_paths.path_count = 1;
            rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
            debug("%d:flags=0X%"PRIX64" devno=0x%"PRIX64" path_id=%"PRIu16", ret_path_count=%d\n",
                  pid,capi_paths.path_info[0].flags, capi_paths.path_info[0].devno,
                  capi_paths.path_info[0].path_id,capi_paths.returned_path_count);
            if (capi_paths.returned_path_count < 1)
                CHECK_RC(1,"returned_path_count expected was 1+\n");
            break;
        case 2: // TCN@7.1.4
            p_ctx->fd = open_dev(p_ctx->dev, O_RDWR);
            if (p_ctx->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n",p_ctx->dev, errno);
                g_error = -1;
                return -1;
            }
            //TEST_DCQP_DUAL_PATH_COUNT
            capi_paths_struct.path.path_count = MAX_PATH;
            rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths_struct);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
            debug("%d:returned_path_count=%d\n",pid,capi_paths.returned_path_count);
            if (capi_paths_struct.path.returned_path_count > MAX_PATH)
                capi_paths_struct.path.returned_path_count=MAX_PATH;
            path_info = capi_paths_struct.path.path_info;
            for (i=0;i<capi_paths_struct.path.returned_path_count;i++)
            {
                debug("%d:flags=0X%"PRIX64" devno=0x%"PRIX64" path_id=%"PRIu16"\n",
                      pid,path_info[i].flags, path_info[i].devno,path_info[i].path_id);
            }
            break;
        case 3: // TCN@7.1.5
            sprintf(cmd, "chdev -l %s -a reserve_policy=no_reserve",sub);
            debug(":%d cmd=%s\n",pid, cmd);
            rc=system(cmd);
            CHECK_RC(rc, "Setting reserve_policy failed");
            p_ctx->fd = open_dev(p_ctx->dev, O_RDWR);
            if (p_ctx->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n",p_ctx->dev, errno);
                g_error = -1;
                return -1;
            }
#ifdef _AIX
            rc= ioctl_dk_capi_query_path_check_flag(p_ctx,0,0);
#else
            return 1;
            // need to handle for Linux later
#endif
            break;

        case 4:  // TCN@7.1.6
            sprintf(cmd, "chdev -l %s -a reserve_policy=single_path",sub);
            debug(":%d cmd=%s\n",pid, cmd);
            rc = system(cmd);
            CHECK_RC(rc, "Setting reserve_policy failed");
            p_ctx->fd = open_dev(p_ctx->dev, O_RDWR);
            if (p_ctx->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n",p_ctx->dev, errno);
                g_error = -1;
                return -1;
            }
#ifdef _AIX
            rc= ioctl_dk_capi_query_path_check_flag(p_ctx,0,DK_CPIF_RESERVED );
#else
            return 1;
            // need to handle for Linux later
#endif
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc =99;
    }

    close(p_ctx->fd);
    return rc;
}
//7.1.3
int test_dcqp_error_ioctl(int flag)  // func@DK_CAPI_QUERY_PATH  error path
{
    int rc = 0;
    struct ctx u_ctx;
    struct ctx *p_ctx = &u_ctx;
    struct cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[255-1];
    }capi_paths;

    pthread_t thread;
    memset(p_ctx, 0, sizeof(struct ctx));
    memset(&capi_paths, 0, sizeof(capi_paths));
    pid = getpid();
    //open CAPI Flash disk device
    p_ctx->fd = open_dev(cflash_path, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", cflash_path, errno);
        g_error = -1;
        return -1;
    }
    capi_paths.path.path_count = 0;
    debug("%d:DK_CAPI_QUERY_PATHS with 0 path_count\n");
    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    debug("%d: rc =%d of path count 0\n",pid, rc);
    if ( rc == 0 )
        flag=1;
    capi_paths.path.path_count = 255;
    debug("%d:DK_CAPI_QUERY_PATHS with 255 path_count");
    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    debug("%d: rc =%d of path count 255\n",pid, rc);
    if ( rc !=0 )
        flag=1;
    close(p_ctx->fd);
    if ( flag == 1 )
        return 0;
    else
        return 1;
}
#endif

int test_dca_ioctl(int flag)  // func@DK_CAPI_ATTACH
{
    int rc=0,i;
#ifdef _AIX
    struct dk_capi_attach capi_attach;
#else
    struct dk_cxlflash_attach capi_attach;
#endif
    struct ctx u_ctx, u_ctx_1, u_ctx_bkp;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_1= &u_ctx_1;
    struct ctx *p_ctx_bkp = &u_ctx_bkp;
    struct flash_disk disks[MAX_FDISK];
    //pthread_t thread;
    get_flash_disks(disks, FDISKS_ALL);
    memset(p_ctx, 0, sizeof(struct ctx));
    memset(p_ctx_1, 0, sizeof(struct ctx));
    //memset(p_ctx_bkp, 0, sizeof(struct ctx));
    //open CAPI Flash disk device
    pid = getpid();
    p_ctx->fd = open_dev(cflash_path, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", cflash_path, errno);
        g_error = -1;
        return -1;
    }
    //thread to handle AFU interrupt & events
    // pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#else
    //TBD for linux
#endif
    *p_ctx_bkp = *p_ctx;
    switch ( flag )
    {
        case 1: //TEST_DCA_VALID_ALL_VALUES   TCN@7.1.10
            p_ctx->flags = p_ctx->work.flags;
            p_ctx->work.num_interrupts = 4;
            rc = ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            break;
        case 2: //TEST_DCA_CALL_DIFF_DEVNO_MULTIPLE  TCN@7.1.13
                // this test need to check for dual adapter and then proceed
              //Applicabe for LPAR having mutlipe corsa adapter
            rc = call_attach_diff_devno();
            return rc;
            break;
        case 3:  // TCN@7.1.17
            //TEST_DCA_REUSE_CONTEXT_ALL_CAPI_DISK
            rc = ioctl_dk_capi_attach(p_ctx); //first attach
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            for (i=1; i<=MAX_FDISK; i++) //Mutliple disk testing
            {
                p_ctx_1->fd = open( disks[i].dev, O_RDWR);
                if (p_ctx_1->fd < 0)
                {
                    fprintf(stderr, "open() failed: device %s, errno %d\n", disks[i].dev, errno);
                    g_error = -1;
                }
#ifdef _AIX
                rc = ioctl_dk_capi_query_path(p_ctx_1);
                CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
                p_ctx_1->devno = p_ctx->devno;
                p_ctx_1->flags = DK_AF_REUSE_CTX;
#else
                p_ctx_1->flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
                p_ctx_1->context_id = p_ctx->context_id; //context id of first call attach
                rc = ioctl_dk_capi_attach(p_ctx_1);
                CHECK_RC(rc, "DK_CAPI_ATTACH failed");
                ioctl_dk_capi_detach(p_ctx_1);
                close(p_ctx_1->fd);
            }
            ioctl_dk_capi_detach(p_ctx);
            close(p_ctx->fd);
            return rc;
            break;
        case 4: //TCN@7.1.19
            //TEST_DCA_REUSE_CTX_OF_RELASED_CTX
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#else
            //TBD for linux
#endif
            rc = ioctl_dk_capi_attach(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");

            rc = ioctl_dk_capi_udirect(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

#ifdef _AIX
            capi_attach.flags = DK_AF_REUSE_CTX;
            capi_attach.version = p_ctx_1->version;
            capi_attach.devno = p_ctx_1->devno; //devno of first attach
            capi_attach.ctx_token= p_ctx_1->context_id; //context id of first call attach
#else
            capi_attach.hdr.flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
            capi_attach.hdr.version = p_ctx_1->version;
            capi_attach.context_id = p_ctx_1->context_id; //context id of first call attach
#endif
            capi_attach.num_interrupts = p_ctx_1->work.num_interrupts;
            rc = ioctl_dk_capi_release(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            rc = ioctl_dk_capi_detach(p_ctx_1);
#ifdef _AIX
            rc = ioctl(p_ctx->fd, DK_CAPI_ATTACH, &capi_attach);
#else
            rc = ioctl(p_ctx->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            ioctl_dk_capi_detach(p_ctx);
            close(p_ctx->fd);
            return rc;
    }
    //pthread_cancel(thread);
    ioctl_dk_capi_detach(p_ctx_bkp);
    close(p_ctx->fd);
    return rc;
}

int test_dca_error_ioctl(int flag)  // func@DK_CAPI_ATTACH error path
{
    int rc;
    struct ctx u_ctx, u_ctx_1;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_1=&u_ctx_1;
    struct flash_disk disks[MAX_FDISK];  // flash disk struct
    memset(p_ctx, 0, sizeof(struct ctx));
    memset(p_ctx_1, 0, sizeof(struct ctx));
    int count=get_flash_disks(disks, FDISKS_SAME_ADPTR );
    pid = getpid();
    if (count < 2)
    {
        fprintf(stderr,"%d:Attention:System doesn't fullfil test req,Need 2 disks from a same adapter\n",pid);
        TESTCASE_SKIP("Need disk from same adapter");
        return 0;
    }
    //open CAPI Flash disk device
    strcpy(p_ctx->dev,disks[0].dev);
    strcpy(p_ctx_1->dev,disks[1].dev);
    p_ctx->fd = open_dev(disks[0].dev, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[0].dev, errno);
        g_error = -1;
        return -1;
    }
    p_ctx_1->fd = open_dev(disks[1].dev, O_RDWR);  //Hoping to open second disk
    if (p_ctx_1->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", disks[1].dev, errno);
        g_error = -1;
    }
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#else
    //TBD for linux
#endif
    p_ctx->work.num_interrupts = p_ctx_1->work.num_interrupts = 4;

    switch ( flag )
    {
        case 1: // TCN@7.1.7
            //TEST_DCA_OTHER_DEVNO
            p_ctx->devno = p_ctx->devno + 999 ;  //TBD  Other than devno of  DKPQP
            rc = ioctl_dk_capi_attach(p_ctx);
            if (rc != 0) //Handling for negative test case as pass for invalid
                rc = 0;
            return rc;
            break;
        case 2: //TEST_DCA_Invalid_devno   TCN@7.1.8
            p_ctx->devno = 0x999; // Invalid valid devno TBD
            rc = ioctl_dk_capi_attach(p_ctx);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0;
            else rc = 1;
            return rc;
            break;

        case 3: //TEST_DCA_Invalid_Intrpt_Num   TCN@7.1.9
#ifdef _AIX
            p_ctx->work.num_interrupts = 0x1;  //TBD interrupt value which is invalid
#else
            p_ctx->work.num_interrupts = 0x099; // tested on linux
#endif
            rc = ioctl_dk_capi_attach(p_ctx);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0;
            else rc =1;
            return rc;
            break;
        case 4: //TEST_DCA_Invalid_Flags   TCN@7.1.11
            p_ctx->flags = 0x0999;  //TBD invliad flags other than  //DK_AF_REUSE_CTX
            rc = ioctl_dk_capi_attach(p_ctx);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0;
            else rc = 1;
            return rc;
            break;
        case 5: //TEST_DCA_CALL_TWICE      TCN 7.1.12
            rc = ioctl_dk_capi_attach(p_ctx);  //first time caling
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            rc = ioctl_dk_capi_attach(p_ctx); //second call may pass
            if (rc != 0) //Handling for negative test case as pass for invalid
                rc = 0 ;
            break;
        case 6: // TCN@7.1.14
                  //TEST_DCA_REUSE_CONTEXT_FLAG
                // using lun_type flag as 4 for ignorin all if conditions
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,4);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            break;
        case 7:  // TCN@7.1.15
            //TEST_DCA_REUSE_CONTEXT_FLAG_ON_NEW_PLUN_DISK
            rc = ioctl_dk_capi_attach(p_ctx);  // first attach
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#endif
            rc = ioctl_dk_capi_attach(p_ctx_1);  //attach of plun
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            rc = ioctl_dk_capi_udirect(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

#ifdef _AIX
            p_ctx_1->devno = p_ctx->devno; //devno of first attach
            p_ctx_1->flags = DK_AF_REUSE_CTX;
#else
            p_ctx_1->flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
            p_ctx_1->context_id = p_ctx->context_id; //context id of first call attach
            rc = ioctl_dk_capi_attach(p_ctx_1);
            if (rc != 0) //Handling for negative test case as pass for invalid
                rc = 0 ;
            close(p_ctx_1->fd);
            break;
        case 8: // TCN@7.1.16
            //TEST_DCA_REUSE_CONTEXT_FLAG_ON_NEW_VLUN_DISK
             //TEST_DCA_REUSE_CONTEXT_FLAG_ON_NEW_VLUN_DISK
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,LUN_VIRTUAL);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            close(p_ctx_1->fd);
            break;

        case 9:  // TCN@7.1.18
            //TEST_DCA_REUSE_CONTEXT_OF_DETACH_CTX

            rc = ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
            p_ctx_1->devno = p_ctx->devno; //devno of first attach
#else
            p_ctx_1->flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
            p_ctx_1->context_id = p_ctx->context_id;//context id of first call attach

            rc = ioctl_dk_capi_detach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_DETACH failed");
            rc = ioctl_dk_capi_attach(p_ctx_1);
            if (rc != 0) //Handling for negative test case as pass for invalid
                rc = 0 ;
            close(p_ctx_1->fd);
            break;

        case 10:  // TCN@7.1.20
            //TEST_DCA_REUSE_CONTEXT_ON_NEW_DISK_AFTER_EEH
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,10);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");

            break;
        case 11:
            //TCN@7.1.19
            //reuse flag on released context.
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,3);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            break;
        case 12 :
            // TCN @ 7.1.17
            rc=ioctl_dk_capi_attach_reuse_all_disk();
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            return rc;
        case 13:
            // TCN @ 7.1.215
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,6);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            break;

        case 14:
            // TCN @ 7.1.216
            rc=ioctl_dk_capi_attach_reuse_loop(p_ctx,p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            break;

        case 15:  // TCN@7.1.24  -  TEST_DCRC_EEH_VLUN_RESUSE_CTX
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,11);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");

            break;

        case 16:  // TCN = 7.1.25  -TEST_DCRC_EEH_PLUN_RESUSE_CTX

            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,12);
            CHECK_RC(rc, "DK_CAPI_ATTACH with reuse flag failed");
            break;
        case 17:
            rc=ioctl_dk_capi_attach_reuse(p_ctx,p_ctx_1,13);
            if ( rc == 0 )
            {
                CHECK_RC(1, "DK_CAPI_ATTACH  should have failed with reuse flag ");
            }
            rc=0;
            break;

        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;

    }
    ioctl_dk_capi_detach(p_ctx);
    close(p_ctx->fd);
    return rc;
}

int test_dcrc_ioctl(int flag)  // func@DK_CAPI_RECOVER_CTX
{
    int rc,i;
    struct ctx u_ctx, u_ctx_1,u_ctx_bkp,  array_ctx[3];;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_1 = &u_ctx_1;
    struct ctx  *P_ctx_bkp = &u_ctx_bkp;
    struct ctx *p_array_ctx[3];
    struct flash_disk disks[MAX_FDISK];
    pthread_t thread, thread2;
    pthread_t thread_new[3];
    pthread_t ioThreadId;
    pthread_t ioThreadId_new[3];
    int threadCleanup=0;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t ioThreadData_new[3];
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    do_io_thread_arg_t * p_ioThreadData_new[3];
    __u64 nlba =  NUM_BLOCKS;
    __u64 stride= 0x1000;

    pid = getpid();

    if ( flag == 4 )
    {
        prepDiskList(cflash_path);

        if ( diskCount <= 1 )
        {
            debug("WARNING : need to export FVT_DEV=/dev/d1,/dev/d2 \n");
            debug("run_cflash_fvt automatically finds it now \n");

            diskCount = get_flash_disks( disks, FDISKS_SAME_ADPTR );
            if ( diskCount < 2 )
	    {
               TESTCASE_SKIP("Need Two disk from same Adapter");
               return 0 ;
            }

            diskList[0] = malloc( strlen(disks[0].dev)+1);
            if ( NULL == diskList[0])
               CHECK_RC(1, "malloc() failed");

	    diskList[1] = malloc( strlen(disks[1].dev)+1);
            if ( NULL == diskList[1])
               CHECK_RC(1, "malloc() failed");

            strcpy(diskList[0],disks[0].dev);
            strcpy(diskList[1],disks[1].dev); 

	}
        memset(p_ctx_1, 0, sizeof(struct ctx));
        memset(p_ctx, 0, sizeof(struct ctx));

        if ((p_ctx->fd = open_dev(diskList[0], O_RDWR)) < 0)
        {
            fprintf(stderr,"open failed %s, errno %d\n",cflash_path, errno);
            g_error = -1;
            return -1;
        }

        strcpy(p_ctx->dev,diskList[0]);

#ifdef _AIX

        rc = ioctl_dk_capi_query_path(p_ctx);
        CHECK_RC(rc, "query path failed");

#endif /*_AIX */

        rc = ctx_init_internal(p_ctx, DK_AF_ASSIGN_AFU, p_ctx->devno);
        CHECK_RC(rc, "ctx_init_internal failed");
    }

    else
    {
        get_flash_disks(disks, FDISKS_ALL);
        memset(p_ctx_1, 0, sizeof(struct ctx));
        rc = ctx_init(p_ctx);
        CHECK_RC(rc, "Context init failed");
    }

    *P_ctx_bkp  = *p_ctx;
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    switch ( flag)
    {
        case 1: //TCN@7.1.22
            //TEST_DCRC_WITH_NO_EEH
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifndef _AIX
            p_ctx->flags=DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET; //clear any oustanding flag
#else
            p_ctx->flags=0;
#endif
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
            break;
        case 2:  //TCN@7.1.28
            //TEST_DCRC_DETACHED_CTX
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            pthread_cancel(thread);

            rc = ioctl_dk_capi_detach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_DETACH failed");
            p_ctx->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            if (rc != 0) //Handling for negative test
                rc = 0 ;
            //            pthread_cancel(thread);
            close(p_ctx->fd);
            return rc;
            break;
        case 3:  // TCN@ 7.1.21  - TEST_DCRC_EEH_VLUN
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif
                CHECK_RC(1, "recover ctx, expected DK_RF_REATTACHED");
            p_ctx->context_id = p_ctx->new_ctx_token;
            break;
        case 4:  // TCN@7.1.23   - TEST_DCRC_EEH_PLUN_MULTI_VLUN

            for ( i =0; i < 3; i ++ )
            {
                p_array_ctx[i] = &array_ctx[ i ];
            }

            rc =ioctl_dk_capi_udirect(p_ctx);  //plun
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

            p_ctx_1->fd = open_dev(diskList[1], O_RDWR);  //Hoping to open second disk
            if (p_ctx_1->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n", disks[1].dev, errno);
                g_error = -1;
            }

            for (i=0; i<3; i++) // multiple vluns creation on second disk
            {

                memset(p_array_ctx[i], 0, sizeof(struct ctx));
                strcpy(p_array_ctx[i]->dev,diskList[1]);
                p_array_ctx[i]->fd = p_ctx_1->fd;
#ifdef _AIX
                rc = ioctl_dk_capi_query_path(p_array_ctx[i]);
                CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#else
                //TBD for linux
#endif
                rc = ctx_init_internal(p_array_ctx[i], DK_AF_ASSIGN_AFU, p_array_ctx[i]->devno);
                CHECK_RC(rc, "DK_CAPI_ATTACH failed");
                rc = create_resource(p_array_ctx[i], (p_array_ctx[i]->last_phys_lba+1)/10, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            }

            for (i=0; i<3; i++)
            {
                pthread_create(&thread_new[i],NULL, ctx_rrq_rx,p_array_ctx[i]);
            }


            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=stride;
            p_ioThreadData->loopCount=1000;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");
            for (i=0; i<3; i++)
            {
                p_ioThreadData_new[i]=&ioThreadData_new[i];
                p_ioThreadData_new[i]->p_ctx=p_array_ctx[i];
                p_ioThreadData_new[i]->stride=stride;
                p_ioThreadData_new[i]->loopCount=1000;
                rc = pthread_create(&ioThreadId_new[i],NULL, do_io_thread, (void *)p_ioThreadData_new[i]);
                CHECK_RC(rc, "do_io_thread() pthread_create failed");
            }

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");

            pthread_cancel(ioThreadId);

            for (i=0; i<3; i++)
            {
                pthread_cancel(ioThreadId_new[i]);
            }

#ifndef _AIX

            pthread_cancel(thread);
            threadCleanup = 1;

            for (i=0; i<3; i++)
            {
                pthread_cancel(thread_new[i]);
            }
#endif

#ifndef _AIX
            p_ctx->flags = DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET ;
#endif
            rc =ioctl_dk_capi_recover_ctx(p_ctx);  //recovering plun
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");

#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif
                CHECK_RC(1, "recover ctx, expected DK_RF_REATTACHED");


            for (i=0; i<3; i++) // multiple vluns recover with reattach
            {

#ifndef _AIX
                p_array_ctx[i]->flags = DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET ;
#endif
                rc =ioctl_dk_capi_recover_ctx(p_array_ctx[i]);  //recovering vlun
                CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");

#ifdef _AIX
                if (p_array_ctx[i]->return_flags != DK_RF_REATTACHED)
#else
                if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_array_ctx[i]->return_flags)
#endif
                    CHECK_RC(1, "recover ctx, expected DK_RF_REATTACHED");

            }

            rc = ctx_reinit(p_ctx);
            CHECK_RC(rc, "ctx_reinit() failed");

            for (i=0; i<3; i++)
            {
                rc = ctx_reinit(p_array_ctx[i]);
                CHECK_RC(rc, "ctx_reinit() failed");
            }

#ifndef _AIX

            debug("------------------ intr hndl1 started -------------------------\n");

            pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);

            /* to avoid race condition for handler and IO thread */
            sleep(SLEEP_FOR_RACE);

#endif
            rc = do_io(p_ctx, stride);
            if ( rc == 2) rc=0;
            else CHECK_RC(rc, "1st IO attempt didn't fail");

#ifdef _AIX
            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
#else
            p_ctx->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");

            rc = do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io() failed");

#ifndef _AIX
            pthread_cancel(thread2);
            debug("------------------ intr hndl1 cancelled -------------------------\n");
#endif

#ifndef _AIX

            debug("------------------ intr hndl2 started -------------------------\n");
            pthread_create(&thread_new[0],NULL, ctx_rrq_rx,p_array_ctx[0]);

            /* to avoid race condition for handler and IO thread */
            sleep(SLEEP_FOR_RACE);

#endif

            rc=do_io(p_array_ctx[0],stride);
            if ( rc == 2) rc=0;
            else CHECK_RC(rc, "1st IO attempt didn't fail");

#ifdef _AIX
            p_array_ctx[0]->flags = DK_VF_HC_TUR;
            p_array_ctx[0]->hint = DK_HINT_SENSE;
#else
            p_array_ctx[0]->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif
            rc = ioctl_dk_capi_verify(p_array_ctx[0]);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");

            rc=do_io(p_array_ctx[0],stride);
            CHECK_RC(rc, "IO attempt fail");

#ifndef _AIX
            pthread_cancel(thread_new[0]);
            debug("------------------ intr hndl2 cancelled -------------------------\n");
#endif

            for (i=1; i<3;  i++)
            {
#ifndef _AIX
                debug("------------------ intr hndl%d started -----------------------\n",(i+2));
                pthread_create(&thread_new[i],NULL, ctx_rrq_rx,p_array_ctx[i]);

                /* to avoid race condition for handler and IO thread */
                sleep(SLEEP_FOR_RACE);
#endif

                rc=do_io(p_array_ctx[i],stride);
                CHECK_RC(rc, "IO failed ");
#ifndef _AIX

                pthread_cancel(thread_new[i]);
                debug("------------------ intr hndl%d cancelled -----------------------\n",(i+2));
#endif

            }


#ifdef _AIX

            for (i=0; i<3; i++)
            {
                pthread_cancel(thread_new[i]);
            }
#endif
            break;
        case 5:  // TCN@7.1.24  -  TEST_DCRC_EEH_VLUN_RESUSE_CTX
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            p_ctx_1->fd = open_dev(disks[1].dev, O_RDWR);  //Hoping  to open second disk
            if (p_ctx_1->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n", disks[1].dev, errno);
                g_error = -1;
            }
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
            p_ctx_1->devno = p_ctx->devno; //devno of first attach
#else
            p_ctx_1->flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
            p_ctx_1->context_id = p_ctx->context_id; //context id of first call attach
            rc = ioctl_dk_capi_attach(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            p_ctx_1->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
            break;
        case 6:  // TCN = 7.1.25  -TEST_DCRC_EEH_PLUN_RESUSE_CTX

            rc =ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            p_ctx_1->fd = open(disks[1].dev, O_RDWR);  //Hoping  to open second disk
            if (p_ctx_1->fd < 0)
            {
                fprintf(stderr, "open() failed: device %s, errno %d\n", disks[1].dev, errno);
                g_error = -1;
            }
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
            p_ctx_1->devno = p_ctx->devno; //devno of first attach
            p_ctx_1->flags = DK_AF_REUSE_CTX;
#else
            p_ctx_1->flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
#endif
            p_ctx_1->context_id = p_ctx->context_id; //context id of first call attach
            rc = ioctl_dk_capi_attach(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            p_ctx_1->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx_1);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
            break;
        case 7: // TCN = 7.1.26 - TEST_DCRC_EEH_VLUN_RESIZE
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            nlba = p_ctx->last_phys_lba + 1;
            rc = vlun_resize(p_ctx, nlba); // vlun size in terms of blocks
            CHECK_RC(rc, "vlun_resize failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            p_ctx->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);
            CHECK_RC(rc, "ctx_reinit() failed");

            break;
        case 8: // TCN 7.1.27  - TEST_DCRC_EEH_VLUN_RELEASE
            rc =ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            p_ctx->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);
            CHECK_RC(rc, "ctx_reinit() failed");
            pthread_cancel(thread);
            rc|=ctx_close(p_ctx);
            return rc;

            break;
        case 9:  //TCN@7.1.112
            rc =ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            p_ctx->devno = 0x9999; //TBD invalid dev no.
            p_ctx->flags = DK_RF_REATTACHED ;
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            if (rc == 22)
                rc = 0;
            else
                rc = 1;

            break;
        case 10:  //TCN@7.1.113
            // invalid flag will succeed, as flag feild is not defined for recover
            rc =ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            p_ctx->flags = 0x09999 ; //TBD invalid flags
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            if (rc == 0)
                rc = 0;
            else
                rc = 1;
            break;
        case 11: //TCN@7.1.114
            // reason feild of recover is not implemented. thus will not fail for invalid date
            rc =ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            p_ctx->reason = 0x09999 ; //TBD invalid reason
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            if (rc == 0)
                rc = 0;
            else
                rc = 1;

            break;
        case 12: //TCN@7.1.115
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            stride=0x10;

            // We wish to do IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=stride;
            p_ioThreadData->loopCount=1000;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");

            // Wait for IO thread to complete
            pthread_join(ioThreadId, NULL);
#ifndef _AIX
            pthread_cancel(thread);
#endif

            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);
            CHECK_RC(rc, "ctx_reinit() failed");
#ifndef _AIX
            pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);
#endif
            rc=do_io(p_ctx, stride);
            if ( rc == 2 ) rc=0;
            else CHECK_RC(1, "1st IO attempt didn't fail");

            rc=do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io() failed");
#ifndef _AIX
            pthread_cancel(thread2);
#endif
            break;
        case 13:
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

            // We wish to do IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=stride;
            p_ioThreadData->loopCount=1000;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");

            // Wait for IO thread to complete
            pthread_join(ioThreadId, NULL);
#ifndef _AIX
            pthread_cancel(thread);
#endif

            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");
#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);
            CHECK_RC(rc, "ctx_reinit() failed");
#ifndef _AIX
            pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);
#endif
            rc=do_io(p_ctx, stride);
            if ( rc == 2 ) rc=0;
            else CHECK_RC(1, "1st IO attempt didn't fail");

            rc=do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io() failed");
#ifndef _AIX
            pthread_cancel(thread2);
#endif

            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;
    }
    if ( threadCleanup == 0 )
    {
        pthread_cancel(thread);
    }
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_dcud_ioctl(int flag)  // func@DK_CAPI_USER_DIRECT
{
    int rc,i;
    struct ctx u_ctx;
    struct ctx *p_ctx = &u_ctx;
    struct validatePckt valVar;
    struct validatePckt *nPtr=&valVar;
    pthread_t thread;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    switch ( flag )
    {
        case 1: // TCN@7.1.32
            //TEST_DCUD_VALID_CTX_VALID_DEVNO

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_UDIRECT failed");
            // expected input
            nPtr->ctxPtr=p_ctx;
            nPtr->obCase=CASE_PLUN;
#ifdef _AIX
            nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
#else
            nPtr->expt_return_flags=0;
#endif
            nPtr->expt_last_lba=p_ctx->last_phys_lba;
            rc = validateFunction(nPtr);
            CHECK_RC(rc, "ValidateFunction failed");
            break;

        case 2: // TCN@7.1.37
            //TEST_DCUD_WITH_VLUN_CREATED_DESTROYED_ON_SAME_DISK

            rc = create_resource(p_ctx, 0,DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "DK_CAPI UVIRTUAL failed");

            // expected input
            nPtr->ctxPtr=p_ctx;
#ifdef _AIX
            nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
            nPtr->expt_last_lba=0;
#else
            nPtr->expt_last_lba=-1;
            nPtr->expt_return_flags=0;
#endif
            nPtr->obCase=CASE_VLUN;
            rc = validateFunction(nPtr);
            CHECK_RC(rc, "ValidateFunction failed");

            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_UDIRECT failed");
            // expected input
            nPtr->ctxPtr=p_ctx;
#ifdef _AIX
            nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
#else
            nPtr->expt_return_flags=0;
#endif
            nPtr->expt_last_lba=p_ctx->last_phys_lba;
            nPtr->obCase=CASE_PLUN;
            rc = validateFunction(nPtr);
            CHECK_RC(rc, "ValidateFunction failed");
            break;

        case 3:   // TCN@7.1.38
            //TEST_DCUD_IN_LOOP
            for (i=0; i<100; i++)
            {
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "DK_CAPI_UDIRECT  failed");
                nPtr->ctxPtr=p_ctx;
                nPtr->obCase=CASE_PLUN;
#ifdef _AIX
                nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
#else
                nPtr->expt_return_flags=0;
#endif
                nPtr->expt_last_lba=p_ctx->last_phys_lba;
                rc = validateFunction(nPtr);
                CHECK_RC(rc, "ValidateFunction failed");
                rc = ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            }
            pthread_cancel(thread);
            ctx_close(p_ctx);
            return rc;
            break;

        case 4: // TCN@7.1.39
            //TEST_DCUD_PATH_ID_MASK_VALUES

            p_ctx->path_id_mask = 0; //bit 0 to path 0,
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_UDIRECT  failed");
            nPtr->ctxPtr=p_ctx;
            nPtr->obCase=CASE_PLUN;
#ifdef _AIX
            nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
#else
            nPtr->expt_return_flags=0;
#endif
            nPtr->expt_last_lba=p_ctx->last_phys_lba;
            rc = validateFunction(nPtr);
            CHECK_RC(rc, "ValidateFunction failed");

            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");

            p_ctx->path_id_mask = 1; // bit 1 to path 1
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_UDIRECT  failed");
            nPtr->ctxPtr=p_ctx;
            nPtr->obCase=CASE_PLUN;
#ifdef _AIX
            nPtr->expt_return_flags=DK_RF_PATH_ASSIGNED;
#else
            nPtr->expt_return_flags=0;
#endif
            nPtr->expt_last_lba=p_ctx->last_phys_lba;
            rc = validateFunction(nPtr);
            CHECK_RC(rc, "ValidateFunction failed");

            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            return rc;
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;
    }
    pthread_cancel(thread);
    rc = ioctl_dk_capi_release(p_ctx);
    ctx_close(p_ctx);
    return rc;

}

int test_dcud_error_ioctl(int flag)  // DK_CAPI_USER_DIRECT  error path
{
    int rc;
    struct ctx u_ctx, u_ctx_bkp ;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_bkp = &u_ctx_bkp;
    pid = getpid();

#ifdef _AIX
    if ( flag == 4 )
    {
       irPFlag = 1; //TEST_DCUD_TWICE_SAME_CONTX_DEVNO 
    }
#endif

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    *p_ctx_bkp=*p_ctx;

    switch ( flag)
    {
        case 1: // TCN@7.1.29
            //TEST_DCUD_INVALID_DEVNO_VALID_CONTX

            p_ctx->devno = 0x999;  //TBD invalid devno
            rc=ioctl_dk_capi_udirect(p_ctx);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0 ;
            else rc = 1;
            break;
        case 2:   // TCN@7.1.30
            //TEST_DCUD_INVALID_CONTX_VALID_DEVNO
            p_ctx->context_id = 0x3fff7b29; //invalid context ID
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0 ;
            else rc = 1;
            // moving to correct context id
            p_ctx->context_id = p_ctx_bkp->context_id;
            rc=ctx_close(p_ctx);
            return rc;
            break;
        case 3: // TCN@7.1.33
            //TEST_DCUD_FLAGS
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "compare failed");
            //TBD some more validation will be added later
            rc=close_res(p_ctx);
            CHECK_RC(rc, "close res failed");
            rc|=ctx_close(p_ctx);
            return rc;
            break;
        case 4:   //  TCN@7.1.34
            //TEST_DCUD_TWICE_SAME_CONTX_DEVNO
            //OK as per MATT,We only restrict exclusivity between pLUN/vLUN
            //on the same disk. You are free to create multiple pLUNs over the same
            //disk but are responsible for serializing access and enforcing security
            //as each context will have a window into the same data.
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_UDIRECT failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "compare failed");
            rc |= create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            rc |= compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "compare failed");
            //TBD some more validation will be added later
            rc|=ctx_close(p_ctx);
            return rc;
            break;
        case 5: // TCN@7.1.35
            //TEST_DCUD_WITH_VLUN_ALREADY_CREATED_ON_SAME_DISK
            rc = create_resource(p_ctx, 0,DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifdef _AIX
            rc = compare_size(p_ctx->last_lba, 0);
#else
            rc = compare_size(p_ctx->last_lba, -1);
#endif
            CHECK_RC(rc, "compare failed");

            // TBD more validation will be added later
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0 ;
            // TBD more validation will be added later
            rc |= close_res(p_ctx);
            CHECK_RC(rc, "close res failed");
            rc |= ctx_close(p_ctx);
            return rc;
            break;
        case 6: // TCN@ 7.1.36
            //TEST_DCUD_WITH_PLUN_ALREADY_CREATED_ON_SAME_DISK

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "compare failed");

            // TBD more validation will be added later
            // second call
            rc = create_resource(p_ctx, 0,DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            if (rc == 22)
                rc =0;
            //TBD more validation will be added later;IN_USE_DIRECT flag check
            rc = close_res(p_ctx);
            CHECK_RC(rc, "close res failed");
            rc = ctx_close(p_ctx);
            CHECK_RC(rc, "p_ctx failed");
            return rc;
            break;
        case 7: //7.1.40
            //TEST_DCUD_PATH_ID_MASK_VALUES
            p_ctx->path_id_mask = 0xff; //invalid path_id_mask
            rc = create_resource(p_ctx, 0, 0, LUN_DIRECT);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0 ;
            else rc = 1;
            ctx_close(p_ctx);
            return rc;
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;
    }
    ctx_close(p_ctx_bkp);
    return rc;
}
int test_dcuv_ioctl(int flag)  //  func@DK_CAPI_USER_VIRTUAL
{
    int rc,i,j;
    //int max_lun_size;
    struct ctx u_ctx;
    struct ctx *p_ctx = &u_ctx;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

#ifdef _AIX
    // Most of the tests are vLUN related
    // so, default flag is set to it.
    p_ctx->flags = DK_UVF_ALL_PATHS;
#endif

    switch ( flag )
    {
            //TEST_DCUV_LUN_VLUN_SIZE_ZERO
        case 1:  // TCN @7.1.44
            p_ctx->lun_size = 0;
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifdef _AIX
            rc = compare_size(p_ctx->last_lba, 0);
#else
            rc = compare_size(p_ctx->last_lba, -1);
#endif
            CHECK_RC(rc, "ERROR: last_lba check failed");
            break;
        case 2:         // TCN@7.1.46
            //TEST_DCUV_WITH_VLUN_ALREADY_CREATED_ON_SAME_DISK
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifdef _AIX
            rc = compare_size(p_ctx->last_lba, 0);
#else
            rc = compare_size(p_ctx->last_lba, -1);
#endif
            CHECK_RC(rc, "ERROR: p_ctx->last_lba check failed");
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifdef _AIX
            rc = compare_size(p_ctx->last_lba, 0);
#else
            rc = compare_size(p_ctx->last_lba, -1);
#endif
            CHECK_RC(rc, "ERROR: p_ctx_1->last_lba check failed");
            break;
        case 3: // TCN@ 7.1.48
            //TEST_DCUV_WITH_MULTIPLE_VLUNS_ON_SAME_CAPACITY_SAME_DISK

            //begin with failure rc for case we can't enter loop
            rc = -1;
            for ( i = 2*NUM_BLOCKS, j = 0;
                  i <= p_ctx->last_phys_lba+1 && j < 16;
                  i += 2*NUM_BLOCKS, j++)
            {
                p_ctx->lun_size = NUM_BLOCKS; // no. of blocks
                rc = ioctl_dk_capi_uvirtual(p_ctx);
                CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
                rc = compare_size(p_ctx->last_lba, p_ctx->lun_size-1);
                CHECK_RC(rc, "ERROR: p_ctx->last_lba check failed");

                rc = ioctl_dk_capi_uvirtual(p_ctx);
                CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");

                rc = compare_size(p_ctx->last_lba, p_ctx->lun_size-1);
                CHECK_RC(rc, "ERROR: p_ctx->last_lba check failed");
            }
            break;
        case 4: // TCN@ 7.1.49
            //TEST_DCUV_TWICE_SAME_CONTX_ID_DEVNO
            p_ctx->lun_size = NUM_BLOCKS;
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->lun_size-1);
            CHECK_RC(rc, "ERROR: last_lba check failed");

            p_ctx->lun_size = p_ctx->last_phys_lba + 1 - NUM_BLOCKS;
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->lun_size-1);
            CHECK_RC(rc, "ERROR: last_lba check failed");

            break;
        case 5: // TCN@7.1.50
            //TEST_DCUV_VLUN_MAX_SIZE

            p_ctx->lun_size = p_ctx->last_phys_lba + 1;// max. no. block in hdisk
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "ERROR: last_lba check failed");
            break;
        case 6: // TCN@7.1.52
            //TEST_DCUV_PLUN_CREATED_DESTROYED_SAME_DISK
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            rc=ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_UDIRECT failed");
            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");

            p_ctx->lun_size = NUM_BLOCKS;
            p_ctx->flags = DK_UVF_ALL_PATHS;
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI UVIRTUAL failed");
            rc = compare_size(p_ctx->last_lba, p_ctx->lun_size-1);
            CHECK_RC(rc, "ERROR: last_lba check failed");
            break;
        case 7: // TCN@7.1.55
            //TEST_DCUV_PATH_ID_MASK_VALUES
            p_ctx->flags = 0; //let path_id_mask decide
            p_ctx->path_id_mask = 1; //bit 1 to path 0,
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            p_ctx->path_id_mask = 2; // bit 2 to path 1
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            if ( p_ctx->return_path_count < 2 && rc == 0 ) rc = 1;
            CHECK_RC(rc, "path_count < 2 & vLUN creations didn't fail");
            break;
        case 8: //TCN@7.1.56
            //TEST_DCUV_INVALID_PATH_ID_MASK_VALUES
            p_ctx->path_id_mask = 0xA;
            p_ctx->flags = 0x0; //let path_id_mask mask should use
            rc=ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0;
            CHECK_RC(rc, "ioctl didn't fail with invalid path_id_mask");
            break;

        case 9:  // TCN@7.1.57
            //TEST_DCUV_IN_LOOP
            for (i=0; i<100; i++)  // Calling ioctl uvirtual in loop
            {
                rc=ioctl_dk_capi_uvirtual(p_ctx);
                CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
#ifdef _AIX
                rc = compare_size(p_ctx->last_lba, 0);
#else
                rc = compare_size(p_ctx->last_lba, -1);
#endif
                CHECK_RC(rc, "ERROR: last_lba check failed");
                rc = ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc, "DK_CAPI_RELEASE failed");

                p_ctx->lun_size = p_ctx->last_phys_lba + 1;
                rc=ioctl_dk_capi_uvirtual(p_ctx);
                CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
                rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
                CHECK_RC(rc, "ERROR: last_lba check failed");
                rc = ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc, "DK_CAPI_RELEASE failed");

                p_ctx->lun_size = 0;
            }
            break;
    }
    close_res(p_ctx);
    ctx_close(p_ctx);

    return rc;
}

int test_dcuv_error_ioctl(int flag)  // func@DK_CAPI_USER_VIRTUAL error path
{
    int rc=0, itr=0;
    uint64_t blks_consumed=0;
    struct ctx u_ctx, u_ctx_bkp;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_bkp = &u_ctx_bkp;
    pthread_t thread;
    pid = getpid();
    //struct dk_capi_uvirtual uvirtual;
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    // Most of the tests are vLUN related
    // so, default flag is set to it.
    p_ctx->flags = DK_UVF_ALL_PATHS;

    *p_ctx_bkp=*p_ctx;
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    printf("--------------------- case number %d -----------------------\n",flag);
    debug("entering switch case:rc =%d , errno = %d\n",rc,errno);

    switch ( flag )
    {
        case 1: // TCN@7.1.41
            //TEST_DCUV_INVALID_DEVNO_VALID_CONTX
            p_ctx->devno = 0x9999;//TBD wrong dev number
            rc=ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_LUN_NOT_FOUND);
            break;

        case 2: // TCN@7.1.42
            //TEST_DCUV_INVALID_CONTX_INVALID_DEVNO

            p_ctx->devno = 0x9999;//TBD wrong dev number
            p_ctx->context_id = 0x9999; //Invalid contx ID
            rc=ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_ATTACH_NOT_FOUND);
            break;

        case 3: // TCN@7.1.43
            //TEST_DCUV_VALID_DEVNO_INVALID_CONTX_ID
            p_ctx->context_id = 0x99; //Invalid contx ID
            rc=ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_ATTACH_NOT_FOUND);
            break;

        case 4: // TCN@7.1.45
            //TEST_DCUV_PLUN_ALREADY_CREATED_SAME_DISK
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            rc= ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

            p_ctx->flags = DK_UVF_ALL_PATHS;
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            // TBD: harden flags while testing
            rc |= compare_flags(p_ctx->return_flags, DK_RF_DUPLICATE);

            close_res(p_ctx);
            break;

        case 5: // TCN@7.1.47
            //TEST_DCUV_WITH_NO_FURTHER_VLUN_CAPACITY

            // Max vLUN is 16 per ctx, so adjust size to be
            // big enough to start failing before last limit
            p_ctx->lun_size = (p_ctx->last_phys_lba )/0xA;
            // calling multiple times vlun virtual to exhaust hdisk
            while ( rc == 0 && itr < 16 )
            {
                rc = ioctl_dk_capi_uvirtual(p_ctx);
                if ( rc == 0 ) blks_consumed += (p_ctx->last_lba + 1);
                p_ctx->last_lba = 0; // clear it off for next iteration !
                itr++; // Just to keep track we didn't hit max vLUN/ctx limit.
            }

            debug("%d:blks_consumed=0X%"PRIX64"\n", pid, blks_consumed);
            if ( blks_consumed < (p_ctx->last_phys_lba - p_ctx->chunk_size ))
            {
                p_ctx->lun_size = ( p_ctx->last_phys_lba - blks_consumed ) - 0x1000 ;
                rc = ioctl_dk_capi_uvirtual(p_ctx);
                if ( rc != 0 )
                    CHECK_RC(rc, "Failed to exhaust hdisk using vLUNs");
            }

            p_ctx->lun_size = p_ctx->chunk_size;
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 28 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_CHUNK_ALLOC);

            close_res(p_ctx);
            break;

        case 6: // TCN@7.1.51
            //TEST_DCUV_VLUN_SIZE_MORE_THAN_DISK_SIZE
            p_ctx->lun_size = p_ctx->last_phys_lba + 2; // more than disk size
#ifndef _AIX
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            debug("ioctl_dk_capi_uvirtual:rc =%d , errno = %d\n\n",rc,errno);
            CHECK_RC(rc, "ioctl_dk_capi_uvirtual failed");
            rc |= compare_flags(p_ctx->last_lba, p_ctx->last_phys_lba);
#else
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 28 )
            {
                rc = 1;
                CHECK_RC(rc, "unexpected result : ioctl_dk_capi_uvirtual failed");
            }
            else
            {
                rc = 0; // Reset RC
            }
            rc |= compare_flags(p_ctx->return_flags, DK_RF_CHUNK_ALLOC);
#endif
            debug("compare_flags:rc =%d , errno = %d\n",rc,errno);
            break;

        case 7: // TCN@7.1.53
            //TEST_DCUV_WITH_CTX_OF_PLUN
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            rc= ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");

            p_ctx->flags = DK_UVF_ALL_PATHS;
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_PATH_ASSIGNED);

            close_res(p_ctx);
            break;

        case 8: // TCN@7.1.54
            //TEST_DCUD_WITH_CTX_OF_VLUN
            rc= ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");

            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            rc= ioctl_dk_capi_udirect(p_ctx);
            if ( rc != 22 ) rc = 1;
            else rc = 0; // Reset RC
            rc |= compare_flags(p_ctx->return_flags, DK_RF_PATH_ASSIGNED);

            close_res(p_ctx);
            break;

        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc =99;
    }

    pthread_cancel(thread);
    ctx_close(p_ctx_bkp);
    return rc;
}

//7.1.2
int test_all_ioctl_invalid_version(void) //TEST_IOCTL_INVALID_VERSIONS
{
    int rc=0,i,final_rc=22;
    for (i=1; i<=11; i++)
    {
        rc = test_invalid_version_ioctl(i);
        if (rc != 22)
            final_rc=-1;

    }
    return final_rc;

}

int test_invalid_version_ioctl(int flag)
{
    int rc,i;
    struct ctx u_ctx, u_ctx_bkp;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_bkp = &u_ctx_bkp;
    uint16_t invalid_version[3] = { 1,99, 1000 };
    char sense_data[10];
    pid = getpid();
    memset(p_ctx, 0, sizeof(struct ctx));
    //open CAPI Flash disk device
    p_ctx->fd = open_dev(cflash_path, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", cflash_path, errno);
        g_error = -1;
        return -1;
    }
    strcpy(p_ctx->dev, cflash_path);
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "DK_CAPI_QUERY_PATHH failed");
#endif
    p_ctx->work.num_interrupts = 4;
    switch ( flag )
    {
#ifdef _AIX
        case 1: //TEST_DCUV_INVALID_VERSION
            for ( i=0; i<3; i++)
            {
                debug("%d: ioctl query path with %d version\n",pid, invalid_version[i]);
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_query_path(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }
            break;
#endif
        case 2 :  //TEST_DCA_Invalid_Version
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            for (i=0; i<3; i++)
            {
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_attach(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }
            break;
        case 3: //TEST_DCUD_INVALID_VERSION
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            *p_ctx_bkp = *p_ctx;
            for (i=0; i<3; i++)
            {
                p_ctx->version= invalid_version[i];
                rc = ioctl_dk_capi_udirect(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }

            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 4: //TEST_DCUV_INVALID_VERSION
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            p_ctx->flags = DK_UVF_ALL_PATHS;
            *p_ctx_bkp = *p_ctx;
            for (i=0; i<3; i++)
            {
                p_ctx->version= invalid_version[i];
                rc = ioctl_dk_capi_uvirtual(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 5:  //TEST_DCVR_INVALID_VERSIONS -DK_CAPI_VLUN_RESIZE
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH failed");
            p_ctx->flags = DK_UVF_ALL_PATHS;
            rc = ioctl_dk_capi_uvirtual(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_VIRTUAL failed");
            *p_ctx_bkp = *p_ctx;
            for ( i=0; i<3; i++)
            {
                p_ctx->version= invalid_version[i];
                rc = ioctl_dk_capi_vlun_resize(p_ctx);
                if (!rc) break;
            }
            ioctl_dk_capi_release(p_ctx_bkp);
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 6:  // TEST_DCR_INVALID_VERSION
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            *p_ctx_bkp = *p_ctx;
            for (i=0; i<3; i++)
            {
                rc = ioctl_dk_capi_udirect(p_ctx);
                p_ctx->version= invalid_version[i];
                rc = ioctl_dk_capi_release(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
                *p_ctx = *p_ctx_bkp;//preserv valid version for udirect
            }
            ioctl_dk_capi_detach(p_ctx);
            break;
        case 7: //TEST_DCD_INVALID _VERSION
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
            *p_ctx_bkp = *p_ctx;
            for (i=0; i<3; i++)
            {
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_detach(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 8: // TEST_DCL_INVALID
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
#ifdef _AIX
            p_ctx->flags = DK_LF_TEMP;
#endif
            //p_ctx->reason = ??//TBD
            *p_ctx_bkp = *p_ctx;
            for (i=0; i<3; i++)
            {
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_log(p_ctx, sense_data);
                if (rc == 0) //Handling for negative test
                    break;
            }
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 9:
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
            *p_ctx_bkp = *p_ctx;
            //generate_unexpected_error(); //TBD
#ifdef _AIX
            p_ctx->flags = DK_VF_HC_INQ;
            p_ctx->hint = DK_HINT_SENSE;
#endif
            for (i=0; i<3; i++)
            {
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_verify(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;

            }
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 10:        //TEST_DCRC_INVALID_VERSIONS, need to TBD more in fuctionality
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
            *p_ctx_bkp = *p_ctx;
            //rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            for (i=0; i<3; i++)
            {
                p_ctx->version = invalid_version[i];
                rc = ioctl_dk_capi_recover_ctx(p_ctx);
                if (rc == 0) //Handling for negative test
                    break;
            }
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        case 11: //TEST_DCQE_INVALID_VERSIONS
            p_ctx->flags = DK_AF_ASSIGN_AFU;
            rc=ioctl_dk_capi_attach(p_ctx);
            CHECK_RC(rc, "DK_CAPI_ATTACH Failed");
            p_ctx->flags = DK_UDF_ASSIGN_PATH;
            rc = ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "DK_CAPI_USER_DIRECT failed");
#ifdef _AIX
            p_ctx->flags = DK_QEF_ADAPTER;
#endif
            *p_ctx_bkp = *p_ctx;
            generate_unexpected_error(); //TBD
            for (i=0; i<3; i++)
            {
                p_ctx->version  = invalid_version[i];
                rc = ioctl_dk_capi_query_exception(p_ctx);
                if (rc == 0) break;
            }
            ioctl_dk_capi_release(p_ctx_bkp);
            ioctl_dk_capi_detach(p_ctx_bkp);
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            return 99;
    }
    close(p_ctx->fd);
    return rc;
}

int test_dcd_ioctl( int flag )  // func@DK_CAPI_DETACH
{
    int rc;
    struct ctx u_ctx, u_ctx_bkp;
    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_bkp = &u_ctx_bkp;
    struct flash_disk disks[MAX_FDISK];
    __u64 nlba;
    get_flash_disks(disks, FDISKS_ALL);
    memset(p_ctx, 0, sizeof(struct ctx));
    pid = getpid();
    p_ctx->fd = open_dev(cflash_path, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", cflash_path, errno);
        g_error = -1;
        return -1;
    }
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
    p_ctx->work.num_interrupts = 5;
#else
    p_ctx->work.num_interrupts = 4;
#endif /*_AIX*/

    p_ctx->flags = DK_AF_ASSIGN_AFU;
    rc = ioctl_dk_capi_attach(p_ctx);
    CHECK_RC(rc, "DK_CAPI_ATTACH failed");
    nlba = p_ctx->chunk_size;
    *p_ctx_bkp = *p_ctx;
    switch ( flag )
    {
        case 1:  // Invalid TC
            p_ctx->context_id = 0x000; //TBD invalid ctx id
            p_ctx->devno = 0x000; //TBD invalid devno
            rc = ioctl_dk_capi_detach(p_ctx);
            if (rc != 0) //Handling for negative test case as pass for
                rc = 0;
            break;

        case 2: // TCN@7.1.83
            p_ctx->devno=0xdead;
            rc = ioctl_dk_capi_detach(p_ctx);
            if (rc != 0) //Handling for negative test case as pass for
                rc = 0;
            break;
        case 3: // TCN@7.1.84
            p_ctx->context_id = 0xdeaddead;
            rc = ioctl_dk_capi_detach(p_ctx);
            if (rc != 0) //Handling for negative test case as pass
                rc = 0;
            break;
        case 4:  // TCN@7.1.85
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");
            rc = ioctl_dk_capi_detach(p_ctx); //first detach
            CHECK_RC(rc,"DK_CAPI_DETACH failed");
            rc = ioctl_dk_capi_detach(p_ctx);  //second detach fail
            if (rc != 0) //Handling for negative test case as pass
                rc = 0;
            return rc;
            break;
            //new case of calling detach when no lun is created on context
        case 5:  //TCN@7.1.86
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = ioctl_dk_capi_detach(p_ctx); //first detach
            CHECK_RC(rc,"DK_CAPI_DETACH failed");
            rc = ioctl_dk_capi_detach(p_ctx);  //second detach fail
            if (rc != 0) //Handling for negative test case as pass
                rc = 0;
            return rc;
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;
    }
    ioctl_dk_capi_detach(p_ctx_bkp);  // clean up
    close(p_ctx->fd);
    return rc;
}


int test_dcr_ioctl( int flag )  //  func@DK_CAPI_RELEASE
{
    int rc,i,rc1;
    struct ctx myctx, myctx_bkp;
    struct ctx *p_ctx = &myctx;
    struct ctx *p_ctx_bkp = &myctx_bkp;
    uint64_t resources[MAX_RES_HANDLE];
    __u64 nlba;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    memset(p_ctx_bkp, 0, sizeof(struct ctx));
    nlba = p_ctx->chunk_size;
    switch ( flag )
    {
        case 1: //TCN@7.1.75
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            *p_ctx_bkp = *p_ctx;  //backuping the defaults contents
            p_ctx->devno=0xdeaddead;
            rc = ioctl_dk_capi_release(p_ctx);
            break;

        case 2: //TCN@7.1.76
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            *p_ctx_bkp = *p_ctx;  //backuping the defaults contents
            p_ctx->devno=0x1000;
            p_ctx->context_id= 0x9999;
            rc = ioctl_dk_capi_release(p_ctx);
            break;

        case 3: //TCN@7.1.77
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            *p_ctx_bkp = *p_ctx;  //backuping the defaults contents
            p_ctx->context_id= 0x1239999;
            rc = ioctl_dk_capi_release(p_ctx);
            break;

        case 4://TCN@7.1.78
              //No vlun or plun creation calling directly release ioctl
            rc = ioctl_dk_capi_release(p_ctx);
            break;

        case 5: //TCN@7.1.79
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");
            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            rc = ioctl_dk_capi_release(p_ctx); // second release, should fail
            ctx_close(p_ctx);
            return rc;
            break;

        case 6: //TCN@7.1.80
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = ioctl_dk_capi_release(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            rc = ioctl_dk_capi_release(p_ctx); // second release, should fail
            ctx_close(p_ctx);
            return rc;
            break;

        case 7: //TCN@7.1.81
            for (i=0; i<MAX_RES_HANDLE; i++)
            {
                nlba = (i+1)*p_ctx->chunk_size;
                rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
                rc =compare_size(p_ctx->last_lba, nlba-1);
                CHECK_RC(rc, "size compare failed");
                resources[i]=p_ctx->rsrc_handle;
            }
            for (i=0; i<MAX_RES_HANDLE; i++)
            {
                p_ctx->rsrc_handle = resources[i];
                rc = ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc, "DK_CAPI_RELEASE failed");
            }
            rc |= g_error;
            ctx_close(p_ctx);
            return rc;
            break;
        case 8://7.1.82
            //invalid resource handler
            for (i=0; i<MAX_RES_HANDLE; i++)
            {
                nlba = (i+1)*p_ctx->chunk_size;
                rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                rc = compare_size(p_ctx->last_lba, nlba-1);
                CHECK_RC(rc, "size compare failed");
                resources[i]=p_ctx->rsrc_handle;
            }
            for (i=0; i<MAX_RES_HANDLE; i++)
            {
                p_ctx->rsrc_handle = 0xdead + i;
                rc = ioctl_dk_capi_release(p_ctx);
                if (!rc)
                {
                    ctx_close(p_ctx_bkp);
                    return -1;
                }
            }
            //now release actual one
            for (i=0; i<MAX_RES_HANDLE; i++)
            {
                p_ctx->rsrc_handle = resources[i];
                rc1 = ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc1, "DK_CAPI_RELEASE failed");
            }
            ctx_close(p_ctx_bkp);
            return rc;
        default :
            fprintf(stderr," Nothing to do with default call\n");
            rc = 99;
    }
    close_res(p_ctx_bkp);
    ctx_close(p_ctx_bkp);
    return(rc);

}

int test_dcvr_ioctl( int flag ) // func@DK_CAPI_VLUN_RESIZE
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 nlba;
    __u64 new_nlba = 0x100;
    pthread_t thread;
    int i;
    uint64_t chunk;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    nlba = p_ctx->chunk_size;
    switch ( flag )
    {
        case 1: //TCN@7.1.64
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            // new size id more than disk size
            new_nlba= nlba+15; //vlun size is not a factor of 256MB chunk
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc = compare_size(p_ctx->last_lba, (2*nlba-1));
            break;

        case 2: //TCN@7.1.65
            nlba = 15 * p_ctx->chunk_size;
            rc=create_resource(p_ctx, 0, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = vlun_resize(p_ctx, nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc = compare_size(p_ctx->last_lba, nlba-1);
            CHECK_RC(rc, "size compare failed");
            rc = vlun_resize(p_ctx, nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc = compare_size(p_ctx->last_lba, nlba-1);
            break;
        case 3: //TCN@7.1.66
            nlba = 10 * p_ctx->chunk_size;
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            new_nlba=nlba - p_ctx->chunk_size; //shrink the vlun
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, new_nlba-1);
            break;

        case 4: //TCN@7.1.67
            nlba = 7 * p_ctx->chunk_size;
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            new_nlba= nlba + 2*(p_ctx->lun_size); //increase the vlun size
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, new_nlba-1);
            break;

        case 5: //TCN@7.1.68
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            new_nlba = p_ctx->last_phys_lba; //total no. of blocks in hdisk
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            break;

        case 6: //TCN@7.1.70
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            // incrementing in mutliples of 256MB
            for (new_nlba=nlba;new_nlba <=(p_ctx->last_phys_lba + 1);new_nlba+=nlba)
            {
                rc = vlun_resize(p_ctx, new_nlba);
                CHECK_RC(rc, "vlun_resize failed");
                rc=compare_size(p_ctx->last_lba, new_nlba-1);
                if (rc)break;
            }
            break;

        case 7: //TCN@7.1.71
            new_nlba = p_ctx->last_phys_lba + 1; //total size of disk
            rc=create_resource(p_ctx, new_nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc=compare_size(p_ctx->last_lba, new_nlba-1);
            CHECK_RC(rc, "size compare failed");
            while ( new_nlba >= p_ctx->chunk_size)
            {
                rc = vlun_resize(p_ctx, new_nlba);
                CHECK_RC(rc, "vlun_resize failed");
                rc=compare_size(p_ctx->last_lba, new_nlba-1);
                new_nlba -= p_ctx->chunk_size; //decreasing in mutliples of 256 Mb
                if (rc)break;
            }
            break;

        case 8: //TCN@7.1.72
            nlba = 17 * p_ctx->chunk_size;
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = vlun_resize(p_ctx, nlba);
            CHECK_RC(rc, "vlun_resize failed");
            new_nlba = nlba + 100;  //  nlba< new_nlba < nlba+NUM_BLOCKS
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc =compare_size(p_ctx->last_lba, nlba+p_ctx->chunk_size-1);
            break;

        case 9: //TCN@7.1.73
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            new_nlba = nlba - 100;  //  nlba > new_nlba < NUM_BLOCKS
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, nlba-1);
            break;

        case 10: //TCN@7.1.74
            rc=create_resource(p_ctx, 0, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = vlun_resize(p_ctx, p_ctx->last_phys_lba+1);
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "size compare failed");
            chunk = p_ctx->last_lba/p_ctx->chunk_size;
            for (i=0; i<100; i++)
            {
                nlba = (rand()%chunk)*(p_ctx->chunk_size);
                rc = vlun_resize(p_ctx, nlba);
                CHECK_RC(rc, "vlun_resize failed");
                if (nlba)rc=compare_size(p_ctx->last_lba, nlba-1);
                if (rc)break;
            }
            break;
        case 11: //new test case G_MC_test_DCVR_ZERO_Vlun_size
            nlba = p_ctx->last_phys_lba+1;
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc=compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "size compare failed");
            rc = vlun_resize(p_ctx,0);
            CHECK_RC(rc, "vlun_resize failed");
            break;
        default :
            rc=99;
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    rc |= g_error;
    return(rc);
}

int test_dcvr_error_ioctl( int flag ) // func@DK_CAPI_VLUN_RESIZE error path
{
    int rc,i;
    struct ctx myctx, array_ctx[3];
    struct ctx *p_ctx = &myctx;
    struct ctx *p_array_ctx = array_ctx;
    __u64 nlba;
    __u64 new_nlba = 0x100 ;  //TBD define the appropriate value
    pthread_t thread;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    nlba = p_ctx->chunk_size;
    array_ctx[0] = *p_ctx; //backup
    switch ( flag )
    {
        case 1:         //TCN@7.1.58
            rc=create_resource(p_ctx, nlba, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            p_ctx->devno=0x1000;
            nlba = 2 * nlba;
            rc = vlun_resize(p_ctx, nlba);
            if (rc == 22) rc = 0;
            else rc = 1;
            break;
        case 2:  //TCN@7.1.59
            rc=create_resource(p_ctx, nlba, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            p_ctx->devno = 1000;
            p_ctx->context_id = 9999;
            nlba = 2 * nlba;
            rc = vlun_resize(p_ctx, nlba);
            if (rc == 22) rc = 0;
            else rc = 1;
            break;
        case 3: //TCN@7.1.60
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            p_ctx->context_id= 10010;
            nlba = 2 * nlba;
            rc = vlun_resize(p_ctx, nlba);
            break;
        case 4:  //TCN@7.1.61
            rc = vlun_resize(p_ctx, nlba);
            break;
        case 5: //TCN@7.1.62
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");
            rc = vlun_resize(p_ctx, nlba);
            break;

        case 6: //TCN@7.1.63
            rc=create_resource(p_ctx, nlba, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            new_nlba=(p_ctx->last_phys_lba + 3);
            rc = vlun_resize(p_ctx, new_nlba);
#ifdef _AIX
            if ( rc != 0 )
                rc=0;
            else
                CHECK_RC(1, "vlun_resize did not fail");
#else
            CHECK_RC(rc, "vlun_resize failed");
            rc=compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
            CHECK_RC(rc, "size compare failed");
#endif
            break;
        case 7: //TCN@7.1.69
            rc = create_resource(p_ctx, nlba,DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            for (i=1; i<3; i++) //create 2 more ctx additional
            {
                rc = ctx_init(&p_array_ctx[i]);
                CHECK_RC(rc, "Context init failed");
                rc = create_resource(&p_array_ctx[i], nlba,DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }
#ifdef _AIX
            //new_nlba is equal to size of CAPI flash disk
            new_nlba = p_ctx->last_phys_lba + 1; //total no. of blocks in hdisk
             //last_block * block size = hdisk size
            rc = vlun_resize(p_ctx, new_nlba);
            if ( rc == 0 )
                CHECK_RC(1, "create LUN_VIRTUAL failed");
#endif
            // 4*nlba is used up already in 4 vLuns
            new_nlba = p_ctx->last_phys_lba + 1 - 4*nlba ;
            rc = vlun_resize(p_ctx, new_nlba);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");

            // existing vLun can increase by new_nlba
            new_nlba = new_nlba - 1; // i.e. last block no.

            rc=compare_size(p_ctx->last_lba, new_nlba);
            CHECK_RC(rc, "size compare failed");
            break;
        default:
            rc= 99;
    }
    pthread_cancel(thread);
    close_res(&p_array_ctx[0]);
    ctx_close(&p_array_ctx[0]);
    return(rc);
}

int test_dcv_ioctl( int flag )  // func@DK_CAPI_VERIFY

{
    int rc;
    int i;
    struct ctx uctx;
    struct ctx uctx1;
    struct ctx *p_ctx = &uctx;
    struct ctx *p_ctx1 = &uctx1;
    pthread_t thread, ioThreadId, verifyThreadId;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    __u64 nlba =  NUM_BLOCKS;
    uint64_t exp_last_lba;
    //__u64 stride= 0x1000;

    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

#ifdef _AIX
    exp_last_lba = p_ctx->last_phys_lba;
#endif

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    debug("-------------------- Starting case num : %d @test_dcv_ioctl()--------------------------\n", flag);

    switch ( flag )
    {
        case 1:// TCN@7.1.92
            //After unexpected error,call the ioctl DK_CAPI_VERIFY
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");//TBD  for unexpected error creation

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");

#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);

            CHECK_RC(rc, "ctx_reinit() failed");

            p_ctx->flags=DK_VF_LUN_RESET;
            rc = ioctl_dk_capi_verify(p_ctx);
            // TBD: may also need to return flag checks !!
            if ( p_ctx->verify_last_lba != p_ctx->last_phys_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }

            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            break;
        case 2:// When no error is present, call the ioctl DK_CAPI_VERIFY.
                //TCN@7.1.93
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            break;
        case 3: //TCN@7.1.94
            // After an unexpected error, call the ioctl DK_CAPI_VERIFY. While ioctl is still verifying the disk, call for VLUN create, detach etc. should fail
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");

#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);

            CHECK_RC(rc, "ctx_reinit() failed");

            // A new resorce will be created soon
            // copying the p_ctx image
            memcpy((void *)p_ctx1,(const void *)p_ctx,sizeof(struct ctx));

            p_ctx->flags=DK_VF_LUN_RESET;

            // Let verify ioctl execute in different thread.
            pthread_create(&verifyThreadId, NULL, verify_ioctl_thread, p_ctx);

            //Need to see the feasiblity
            rc=create_resource(p_ctx1, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            if (rc != 0)
            {
                debug("%d: LUN_VIRTUAL FAILED\n",pid);
            }
            rc |= ioctl_dk_capi_vlun_resize(p_ctx);
            if (rc != 0)
            {
                debug("%d:vlun_resize FAILED \n", pid);
            }

            rc |=ioctl_dk_capi_release(p_ctx);
            if (rc != 0)
            {
                debug("%d:dk_capi_release1 FAILED \n",pid);
            }
            rc |=ioctl_dk_capi_release(p_ctx1);
            if (rc != 0)
            {
                debug("%d:dk_capi_release2 FAILED \n",pid);
            }

            // Wait for verify thread to complete
            pthread_join(verifyThreadId, NULL);
            // We don't expext verify ioctl to return failure.
            if (threadRC != 0 )
            {
                rc|=threadRC;
                debug("%d:verify ioctl failed \n",pid);
            }

            pthread_cancel(thread);
            rc|=ctx_close(p_ctx);

            return rc;

            break;

        case 4: // To call ioctl DK_CAPI_VERIFY with DK_VF_LUN_RESET flag
                //TCN@7.1.95
            for (i=0; i<3; i++) // multiple vluns creation
            {
                rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_LUN_RESET;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;

        case 5: //TCN@7.1.96
            // To call ioctl DK_CAPI_VERIFY with DK_VF_HC_TUR flag  ( a test unit ready )
            for (i=0; i<3; i++) // multiple vluns creation
            {
                rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");

            }

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_HC_TUR ;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 6: // TCN@7.1.97
            // To call ioctl DK_CAPI_VERIFY with DK_VF_HC_INQ flag ( standard enquiry )
            for (i=0; i<3; i++) // multiple vluns creation
            {
                rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_HC_INQ ;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 7: // TCN@7.1.98
                // To call ioctl DK_CAPI_VERIFY with DK_HINT_SENSE flag and valid sense date
            for (i=0; i<3; i++) // multiple vluns creation
            {
                rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

#ifdef _AIX
            p_ctx->hint=DK_HINT_SENSE;
#else
            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 8:  // TCN@7.1.99
            // To call ioctl DK_CAPI_VERIFY with DK_VF_LUN_RESET flag
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_LUN_RESET;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 9:  // TCN@7.1.100
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "LUN DIRECT creation failed");

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_HC_TUR;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;

        case 10 : //TCN@7.1.101
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "LUN DIRECT creation failed");

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            p_ctx->flags=DK_VF_HC_INQ;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 11:  //TCN@7.1.102
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

#ifdef _AIX
            p_ctx->hint=DK_HINT_SENSE;
#else
            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
#ifndef _AIX
            exp_last_lba = p_ctx->last_lba;
#endif
            if ( p_ctx->verify_last_lba != exp_last_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
            pthread_cancel(ioThreadId);
            break;
        case 12: // TCN@7.1.103
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            // We wish to do backgroud IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=0x10;
            p_ioThreadData->loopCount=5;
            rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
            CHECK_RC(rc, "do_io_thread() pthread_create failed");

            // Sleep for a while before issuing verify ioctl.
            usleep(5);

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");

            // waiting I/O complete
            pthread_join(ioThreadId, NULL);


            rc =ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "DK_CAPI_RECOVER_CTX failed");

#ifdef _AIX
            if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
            if ( DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags )
#endif
                CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

            rc = ctx_reinit(p_ctx);

            CHECK_RC(rc, "ctx_reinit() failed");

            rc = do_io(p_ctx, 0x5);
            if ( rc == 0 )
            {
                CHECK_RC(1, "IO did not fail\n");
            }

            // Sleep for a while before issuing verify ioctl.
            sleep(2);

#ifdef _AIX
            p_ctx->hint=DK_HINT_SENSE;
#else
            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif
            p_ctx->flags=DK_VF_LUN_RESET;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            // TBD: may also need to return flag checks !!
            if ( p_ctx->verify_last_lba != p_ctx->last_phys_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }

            break;
        default:
            return 99;
    }
    pthread_cancel(thread);
    rc|=close_res(p_ctx);
    rc|=ctx_close(p_ctx);
    return rc;
}

int test_dcv_error_ioctl( int flag )  // func@DK_CAPI_VERIFY error path
{
    int rc;
    __u64 nlba =  NUM_BLOCKS;
    struct ctx uctx, uctx_bkp;
    struct ctx *p_ctx = &uctx;
    struct ctx *p_ctx_bkp = &uctx_bkp;
    pthread_t thread;

    pid = getpid();
    rc = ctx_init(p_ctx);

    *p_ctx_bkp = *p_ctx;
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    switch ( flag )
    {
        case 1:  //TCN@7.1.87
            // To test invalid path_id for ioctl DK_CAPI_VERIFY
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            p_ctx->path_id=1000;
            rc = ioctl_dk_capi_verify(p_ctx);
            if (rc == 22) //Handling for negative test case as pass for invalid
                rc = 0;
            else rc=1;
            break;

        case 2:  //TCN@7.1.88
            // To test invalid flags for ioctl DK_CAPI_VERIFY
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            p_ctx->flags= 0x8;
            // 0x4 is max value for supported flag.
            // And, any bit higher than it is ignored
            // So, the flag 0x8 is treated as 0
            // and verify ioctl route to default behavior.
            rc = ioctl_dk_capi_verify(p_ctx);
            break;
        case 3: //TCN@7.1.89
            // To test invalid rsrc_handle for ioctl DK_CAPI_VERIFY
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc=close_res(p_ctx);
            CHECK_RC(rc, "failed to release resource");
            rc = ioctl_dk_capi_verify(p_ctx);
#ifndef _AIX
            if (rc == g_errno && g_errno == 22) //Handling for negative test case as pass for invalid
                rc = 0;
            else rc=1;
            g_errno = 0;
#else
            // AIX wouldn't take ctx/res info for this ioctl
            if ( p_ctx->verify_last_lba != p_ctx->last_phys_lba )
            {
                debug("%d: last_lba returned by verify ioctl() isn't correct: ERROR\n", pid);
                rc=1;
            }
#endif
            break;
        default:
            rc=99;
    }
    pthread_cancel(thread);
    close_res(p_ctx_bkp);
    rc|=ctx_close(p_ctx_bkp);
    return rc;
}
#ifdef _AIX
int test_dcle_ioctl( int flag )  // func@DK_CAPI_LOG_EVENT
{
    int rc,i;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 nlba =  NUM_BLOCKS;
    __u64 stride= 0x1000;
    //__u64 new_nlba = 0x10000 ;
    pthread_t thread;
    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    switch (flag)
    {
            // Call ioctl with valid field, one errpt report should be generated.
        case 1: //TCN@7.1.104
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc = ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif

                debug("-----------ctx_reinit called -------------------------\n");

            ctx_reinit(p_ctx);

#ifndef _AIX

            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            rc = ioctl_dk_capi_log(p_ctx,"DISK OPERATION ERROR");
            CHECK_RC(rc, "ioctl_dk_capi_log failed");
            debug("%d\n",  p_ctx->return_flags);
            if ( p_ctx->return_flags ==  0 ) //TBD for return errors
                rc=0;
            else
                rc=1;
            break;

        case 2: //TCN@7.1.108
                        // Call ioctl DK_CAPI_LOG_EVENT with DK_LF_TEMP flag
            for (i=0; i<3; i++)
            {
                rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }
            do_io(p_ctx, stride);
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc = ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif

                debug("-----------ctx_reinit called -------------------------\n");

            ctx_reinit(p_ctx);

#ifndef _AIX

            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");

            p_ctx->flags=DK_LF_TEMP;
            rc = ioctl_dk_capi_log(p_ctx,"DISK OPERATION ERROR"); //TBD
            CHECK_RC(rc, "ioctl_dk_capi_log failed");
            if ( p_ctx->return_flags == 0 ) //TBD for return errors
                rc=0;
            else
                rc=1;
            break;

        case 3:  //TCN@7.1.109
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            do_io(p_ctx, stride);
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc = ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif

                debug("-----------ctx_reinit called -------------------------\n");

            ctx_reinit(p_ctx);

#ifndef _AIX

            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            p_ctx->flags=DK_LF_PERM;
            rc = ioctl_dk_capi_log(p_ctx,"DISK OPERATION ERROR");
            CHECK_RC(rc, "ioctl_dk_capi_log failed");
            debug("%d\n",  p_ctx->return_flags);
            if ( p_ctx->return_flags ==  0 ) //TBD for return errors
                rc=0;
            else
                rc=1;
            break;

        case 4: // TCN@7.1.110  DK_FL_HW_ERR
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            do_io(p_ctx, stride);
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc = ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif

                debug("-----------ctx_reinit called -------------------------\n");

            ctx_reinit(p_ctx);

#ifndef _AIX

            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            p_ctx->flags=DK_LF_HW_ERR;
            rc = ioctl_dk_capi_log(p_ctx,"DISK OPERATION ERROR");
            CHECK_RC(rc, "ioctl_dk_capi_log failed");
            debug("%d\n",  p_ctx->return_flags);
            if ( p_ctx->return_flags == 0 ) //TBD for return errors
                rc=0;
            else
                rc=1;
            break;

        case 5:  // TCN@7.1.110 DK_FL_SW_ERR
                // Call ioctl DK_CAPI_LOG_EVENT with DK_FL_HW_ERR flag
            rc=create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            do_io(p_ctx, stride);
            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "Failed to do EEH injection");
            rc = ioctl_dk_capi_recover_ctx(p_ctx);
            CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
            if (p_ctx->return_flags != DK_RF_REATTACHED)
#else
            if (DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET != p_ctx->return_flags)
#endif

                debug("-----------ctx_reinit called -------------------------\n");

            ctx_reinit(p_ctx);

#ifndef _AIX

            p_ctx->hint=DK_CXLFLASH_VERIFY_HINT_SENSE;
            // if dummy_sense_flag is set;
            // a dummy sense data will be copied into ioctl input
            p_ctx->dummy_sense_flag=1; // if dummy_sense_flag is set;

#endif

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "ioctl_dk_capi_verify failed");
            p_ctx->flags=DK_LF_SW_ERR;
            rc = ioctl_dk_capi_log(p_ctx,"DISK OPERATION ERROR");
            CHECK_RC(rc, "ioctl_dk_capi_log failed");
            debug("%d\n",  p_ctx->return_flags);
            if ( p_ctx->return_flags == 0 ) //TBD for return errors
                rc=0;
            else
                rc=1;
            break;
        default:
            fprintf(stderr," Nothing to do with default call\n");
            rc =  99;
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}
#endif
int generate_unexpected_error(void)
{
    fprintf(stderr,"Yet to define the unxpected func");
    return 99;
}

