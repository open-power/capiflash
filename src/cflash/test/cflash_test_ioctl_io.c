/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_ioctl_io.c $                      */
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
#include "cflash_test.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
extern int MAX_RES_HANDLE;
extern int g_error;
extern pid_t pid;
extern pid_t ppid;
extern bool afu_reset;
extern bool long_run_enable;
extern char cflash_path[MC_PATHLEN];

#ifndef _AIX
extern int manEEHonoff;
#endif

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static __u64 chunks[]={ 4,8,16,32,24,16,20,2,12,1 };
static int imLastContext = 0;
static __u64 chunkRemain = 0;

key_t key = 0x1234;
struct mymsgbuf
{
    long mtype;
    char mtext[2];
};

int test_spio_vlun(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;

    __u64 chunk = 0x10;
    __u64 nlba;
    __u64 stride=0x10000;

    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    if (3 == cmd)
    {
        //IO ON NO RES expect AFURC
        p_ctx->last_lba = chunk * p_ctx->chunk_size -1;
        rc = do_io(p_ctx, stride);
        pthread_cancel(thread);
        ctx_close(p_ctx);
        return rc;
    }

    //create 0 vlun size & later call resize ioctl
    if (1 == cmd)
    {
        //0 size
        debug("%d: create VLUN with 0 size\n", pid);
        rc = create_resource(p_ctx, 0, DK_UVF_ASSIGN_PATH, LUN_VIRTUAL);
        CHECK_RC(rc, "create LUN_VIRTUAL failed");
#ifdef _AIX
        rc = compare_size(p_ctx->last_lba, 0);
#else
        rc = compare_size(p_ctx->last_lba, -1);
#endif
        CHECK_RC(rc, "failed compare_size");
        p_ctx->last_lba=0xFFFF;
        rc = do_io(p_ctx,stride);
        if (rc != 0x13 )
        {
            CHECK_RC(1,"IO should fail with afu_rc=0x13\n");
        }
        else
        {
            fprintf(stderr, "IO failed as expected, don't worry....\n");
            g_error=0;
            rc=0;
        }
    }
    else
    {
        nlba = 1 * (p_ctx->chunk_size);
        rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
        CHECK_RC(rc, "create LUN_VIRTUAL failed");
        rc = compare_size(p_ctx->last_lba, nlba-1);
        CHECK_RC(rc, "failed compare_size");
    }
    nlba = chunk * (p_ctx->chunk_size);
    rc = vlun_resize(p_ctx, nlba);
    CHECK_RC(rc, "vlun_resize failed");
    rc = compare_size(p_ctx->last_lba, nlba-1);
    CHECK_RC(rc, "failed compare_size");

    //i would like to write/read all lbas
    //stride = p_ctx->blk_len;

    rc |= do_io(p_ctx, stride);
    rc |= vlun_resize(p_ctx, 0);
    rc |= vlun_resize(p_ctx, nlba);
    rc |= do_io(p_ctx, stride);

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    rc |= g_error;
    return rc;
}

int test_spio_plun()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    __u64 stride= 0x10000;

    pid = getpid();

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    //for PLUN 2nd argument(lba_size) would be ignored
    rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");
    rc = compare_size(p_ctx->last_lba, p_ctx->last_phys_lba);
    CHECK_RC(rc, "failed compare_size");

    rc = do_io(p_ctx, stride);

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_spio_lun(char *dev, dev64_t devno,
                   __u16 lun_type, __u64 chunk)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    int loop=5;
    int i=0;

    __u64 nlba = 0;
    __u64 stride= 0x1000;

    pid = getpid();

    rc = ctx_init2(p_ctx, dev, DK_AF_ASSIGN_AFU, devno);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    if ( LUN_DIRECT == lun_type)
    {
        rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
        CHECK_RC(rc, "create LUN_DIRECT failed");
        if (long_run_enable) stride=0x100;
        rc = do_io(p_ctx, stride);
    }
    else
    {
        rc = create_resource(p_ctx, nlba,
                             DK_UVF_ALL_PATHS, LUN_VIRTUAL);
        CHECK_RC(rc, "create LUN_VIRTUAL failed");
        nlba = chunk * p_ctx->chunk_size;
        rc = vlun_resize(p_ctx, nlba);
        if (rc == 28)
        {
            fprintf(stderr, "%d:Requested was more..try with half now...\n",pid);
            nlba = nlba/2;
            rc = vlun_resize(p_ctx, nlba);
            if (rc == 28)
            {
                fprintf(stderr, "%d: No space left.. terminate this context..\n",pid);
                return 0;
            }
        }
        CHECK_RC(rc, "vlun_resize failed");
        if (long_run_enable)
        {
            stride=0x1;
            //loop=20;
        }
        while (i++<loop)
        {
            if (long_run_enable)
                printf("%d:IO loop %d(%d) started....\n",pid,i,loop);
            rc = do_io(p_ctx, stride);
            if (rc) break;
        }
    }

    usleep(1000); //let all process do io
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_spio_pluns(int cmd)
{
    int rc;
    int i;
    int cfdisk = 0;
    struct flash_disk fldisks[MAX_FDISK];

    cfdisk = get_flash_disks(fldisks, FDISKS_ALL);
    if (cfdisk <= 0)
    {
        fprintf(stderr, "NO Cflash disks found\n");
        return -1;
    }
    char *str = getenv("LONG_RUN");
    if (str == NULL)
    {
        //non regression
        if (cfdisk > 4)
            cfdisk=4;
    }
    for (i = 0; i<cfdisk; i++)
    {
        if (0 == fork()) //child process
        {
            rc = test_spio_lun(fldisks[i].dev, fldisks[i].devno[0], LUN_DIRECT,0);
            exit(rc);
        }
        if (1 == cmd)
        {
            //IO on a single PLUN
            break;
        }
    }
    rc = wait4all();
    return rc;
}

int test_spio_vluns(int cmd)
{
    int rc;
    int i,j;
    int cfdisk = 0;
    struct flash_disk fldisks[MAX_FDISK];
    //__u64 chunk;

    cfdisk = get_flash_disks(fldisks, FDISKS_ALL);
    char *str = getenv("LONG_RUN");
    if (str == NULL)
    {
        //non regression
        if (cfdisk > 3)
            cfdisk=3;
    }
    for (i = 0; i< cfdisk; i++)
    {
        //create atleast 10 chunks on each on PLUN
        for (j=0; j < 10; j++)
        {
            if (0 == fork()) //child process
            {
                rc = test_spio_lun(fldisks[i].dev, fldisks[i].devno[0],
                                   LUN_VIRTUAL,chunks[j%10]);
                exit(rc);
            }
        }
    }
    rc = wait4all();
    return rc;
}

int test_spio_direct_virtual()
{
    int rc=0;
    int j;
    int cfdisk = 0;
    struct flash_disk fldisks[MAX_FDISK];
    //__u64 chunk;
    int count = 1;
    int procces=10;
    int index = 0; //to switch flash disks for VLUN & PLUN IO

    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG_RUN enabled...\n");
        count = 10;
    }

    cfdisk = get_flash_disks(fldisks, FDISKS_ALL);
    if (cfdisk < 2)
    {
        TESTCASE_SKIP("Must have 2 flash disks..");
        return 0;
    }

    while (count-- >0)
    {
        if (0 == fork())
        {
            rc = test_spio_lun(fldisks[index].dev,
                               fldisks[index].devno[0],LUN_DIRECT,0);
            exit(rc);
        }
        //create atleast 10 chunks on each on PLUN
        index = (index+1)%2;
        for (j=0; j < procces; j++)
        {
            if (0 == fork()) //child process
            {
                //here you can change the path ids later
                rc = test_spio_lun(fldisks[index].dev, fldisks[index].devno[0],
                                   LUN_VIRTUAL,chunks[j]);
                exit(rc);
            }
        }
        rc = wait4all();
        CHECK_RC(rc, "wait4all failed");
        printf("%d loop remain................\n",count);
    }
    return rc;
}

void* res_thread(void *arg)
{
    int rc;
    struct ctx *p_ctx = (struct ctx *)arg;
    res_hndl_t res_hndl;
    __u64 rsrc_handle;
    __u64 stride = 0x1000;

    pthread_mutex_lock(&mutex);
    rc = create_resource(p_ctx, p_ctx->lun_size, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    res_hndl = p_ctx->res_hndl;
    rsrc_handle = p_ctx->rsrc_handle;
    if (rc)
    {
        g_error = -1;
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    p_ctx->res_hndl = res_hndl;
    rc = do_io(p_ctx, stride);
    if (rc)
    {
        g_error = -1;
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    pthread_mutex_unlock(&mutex);
    sleep(1);

    // Closing the resource after IO done
    sleep(2);
    pthread_mutex_lock(&mutex);
    p_ctx->rsrc_handle = rsrc_handle;
    rc = close_res(p_ctx);
    pthread_mutex_unlock(&mutex);
    return 0;
}

int create_ctx_process(char *dev, dev64_t devno, __u64 chunk)
{
    int rc;
    struct ctx my_ctx;
    struct ctx *p_ctx = &my_ctx;
    g_error=0;
    pid = getpid();
    pthread_t threads[MAX_RES_HANDLE];
    pthread_t intr_thread;
    //__u64 flags;
    int i;
   
    rc = ctx_init2(p_ctx, dev, DK_AF_ASSIGN_AFU, devno);
    CHECK_RC(rc, "Context init failed");
    // interrupt handler per context
    rc = pthread_create(&intr_thread, NULL, ctx_rrq_rx, p_ctx);

    p_ctx ->lun_size = chunk * p_ctx->chunk_size;
    for (i = 0; i <MAX_RES_HANDLE; i++)
    {
        if ( MAX_RES_HANDLE == i+1 && imLastContext == 1 )
        { // this is last resource of last context
            debug("%d:create_ctx_process: last res ==> %d of last contxt \n", pid,i+1);
            p_ctx ->lun_size = ( chunk + chunkRemain )*p_ctx->chunk_size;
        }
        rc = pthread_create(&threads[i], NULL, res_thread, p_ctx);
        CHECK_RC(rc, "pthread_create failed");
    }
    //wait all threads to get complete
    for (i = 0; i <MAX_RES_HANDLE; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_cancel(intr_thread);

    rc = g_error;
    g_error = 0;
    return rc;
}

int max_ctx_max_res(int cmd)
{
    int rc;
    int i;
    struct ctx myctx;
    struct ctx *p_ctx=&myctx;
    int max_p = MAX_OPENS; // we can change MAX_OPEN Value in cflash_tests.h

#ifndef _AIX
    /* By default max_p will be set to BML max user context value */
    if ( host_type == CFLASH_HOST_PHYP )
    {
        max_p = MAX_OPENS_PVM ;
    }
#endif

    memset(p_ctx, 0, sizeof(struct ctx));
    __u64 chunk = 0;
    __u64 lba;
    MAX_RES_HANDLE=get_max_res_hndl_by_capacity(cflash_path);
    if (MAX_RES_HANDLE <= 0)
    {
        fprintf(stderr,"Unable to run max_ctx_max_res.. refere prior error..\n");
        return -1;
    }
    strcpy(p_ctx->dev, cflash_path);
#ifdef _AIX
    p_ctx->fd =open_dev(p_ctx->dev, O_RDWR);
    if (p_ctx->fd <  0)
    {
        fprintf(stderr,"%s open failed...\n",p_ctx->dev);
        return -1;
    }
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "query ioctl failed");
    close(p_ctx->fd);
#endif
    uint64_t chunk_size;
    if (2 == cmd)
    {
        lba = get_disk_last_lba(p_ctx->dev, p_ctx->devno, &chunk_size);
        if (lba == 0)
        {
            fprintf(stderr, "Failed to get last_lba of %s\n", p_ctx->dev);
            return -1;
        }
        chunk =  (lba+1)/chunk_size;
        //chunks per context( each context will 16 VLUNS same size)
        chunk = chunk/(max_p * MAX_RES_HANDLE);
        if ( chunk == 0 )
        {
            fprintf(stderr,"chunk-> 0X%"PRIX64" should >= max_p * MAX_RES_HANDLE=>%d\n",
                    chunk,(max_p * MAX_RES_HANDLE));
            return -1;
        }
        chunkRemain = chunk%(max_p * MAX_RES_HANDLE);
    }

    for (i = 0; i< max_p; i++)
    {
        if (0 == fork())
        {
            //child process
            usleep(10000);
            if (2 == cmd)
            {
                if (i == max_p-1 )
                {
                    // this is the last context
                    imLastContext=1;
                }
                rc = create_ctx_process(p_ctx->dev, p_ctx->devno,chunk);
            }
            else
            {
                rc = create_ctx_process(p_ctx->dev, p_ctx->devno,1);
            }
            debug("%d: exiting with rc=%d\n",getpid(), rc);
            exit(rc);
        }
    }
    rc = wait4all();
    return rc;
}

int do_attach_detach(char *dev, dev64_t devno, __u16 lun_type)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunk = 20;
    __u64 nlba;
    int count = 20;
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        count = 100000;
        printf("LONG_RUN enabled...loop=%d\n",count);
        fflush(stdout);
    }
    pid = getpid();

    while (count-- >0)
    {
        rc = ctx_init2(p_ctx, dev, DK_AF_ASSIGN_AFU, devno);
        CHECK_RC(rc, "Context init failed");
        if (LUN_VIRTUAL == lun_type)
        {
            chunk = rand()%16;
            debug("%d: initial chunk size=%ld \n",pid,chunk);
            //create 0 vlun size & later call resize ioctl
            rc = create_resource(p_ctx, chunk, DK_UVF_ALL_PATHS, lun_type);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            chunk = rand()%32;
            debug("%d: chunk before vlun_resize=%ld \n",pid,chunk);
            nlba = chunk * p_ctx->chunk_size;
            rc = vlun_resize(p_ctx, nlba);
            CHECK_RC(rc, "vlun_resize failed");
        }
        else
        {
            rc = create_resource(p_ctx,0, DK_UDF_ASSIGN_PATH, lun_type);
            CHECK_RC(rc, "create LUN_DIRECT failed");
        }
        close_res(p_ctx);
        ctx_close(p_ctx);
        if (count%500 == 0)
            printf("%d: loop remains....\n",count);
        fflush(stdout);
    }
    return 0;
}

int test_spio_attach_detach(int cmd)
{
    int rc;
    int i;
    int cfdisk = 0;
    struct flash_disk disks[MAX_FDISK];
    __u64 chunk = 32;

    cfdisk = get_flash_disks(disks, FDISKS_ALL);
    if (cfdisk < 2)
    {
        TESTCASE_SKIP("Must have 2 flash disks..");
        return 0;
    }

#ifndef _AIX
    if( diskSizeCheck( cflash_path , RECO_DISK_SIZE ))
    {
       TESTCASE_SKIP("DISK SIZE is less than required \n");
       return 0;
    }
#endif
    
    if (fork() == 0)
    {
        //child process
        if (cmd == 1)
        {
            //virtual IO
            for (i=0;i<cfdisk;i++)
            {
                if (!strcmp(cflash_path, disks[i].dev))
                {
                    strcpy(disks[0].dev,disks[i].dev);
                    disks[0].devno[0] = disks[i].devno[0];
                }
            }
            rc = create_ctx_process(disks[0].dev,disks[0].devno[0],chunk);
        }
        else
        {
            //direct IO
            rc = test_spio_lun(disks[1].dev, disks[1].devno[0], LUN_DIRECT,0);
        }
        exit(rc);
    }
    //let parent process do detach & attach for another context id
    if (cmd == 1)
    {
        rc = do_attach_detach(disks[0].dev, disks[0].devno[0], LUN_VIRTUAL);
    }
    else
    {
        rc = do_attach_detach(disks[0].dev, disks[0].devno[0], LUN_DIRECT);
    }
    rc |= wait4all();
    return rc;
}

int test_large_transfer()
{
    int rc;
    struct ctx my_ctx;
    struct ctx *p_ctx = &my_ctx;
    pthread_t thread;
    struct rwlargebuf rwbuf;
    __u64 chunk=2; // do io on last 2 chunks on a plun

    __u64 buf_size[] =
    {
        0x1000, //4KB
                           0x4000, //16KB
                           0x10000, //64KB
                           0x40000, //256KB
                           0x800000, //8MB
                           0x1000000 }; //16MB

    int i;
    //Large trasnfer size is for PLUN not Vluns(4K only) as per Jim
    pid = getpid();
#ifdef _AIX
    rc = setRUnlimited();
    CHECK_RC(rc, "setRUnlimited failed");
#endif
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    rc = create_resource(p_ctx,0,DK_UDF_ASSIGN_PATH,LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");
    p_ctx->st_lba= p_ctx->last_lba +1 -(chunk*p_ctx->chunk_size);
    if (long_run_enable) p_ctx->st_lba=0; //let do IO on complete plun
    for (i=0;i< sizeof(buf_size)/sizeof(__u64);i++)
    {
        rc = allocate_buf(&rwbuf, buf_size[i]);
        CHECK_RC(rc, "memory allocation failed");
        printf("%d: do large io size=0X%"PRIX64"\n",pid, buf_size[i]);
        rc = do_large_io(p_ctx, &rwbuf, buf_size[i]);
        deallocate_buf(&rwbuf);
        if (rc) break; //get out from here
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_large_trnsfr_boundary()
{
    int rc;
    struct ctx my_ctx;
    struct ctx *p_ctx = &my_ctx;
    pthread_t thread;
    struct rwlargebuf rwbuf;
    __u64 buf_size = 0x1000000; //16MB
    __u64 chunk = 10;

    pid = getpid();
#ifdef _AIX
    system("ulimit -d unlimited");
    system("ulimit -s unlimited");
    system("ulimit -m unlimited");
#endif
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    //do RW last cmd with crossed LBA boundary
    //i.e. last_lba size is 0x100;
    //do send rw with 0x10 & cross limit of 0x100

    rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    rc = allocate_buf(&rwbuf, buf_size);
    CHECK_RC(rc, "memory allocation failed");
    //to make sure last cmd rw beyond boundary
    p_ctx->st_lba = p_ctx->last_lba - (chunk * p_ctx->chunk_size);
    p_ctx->st_lba = p_ctx->st_lba +20 ;

    rc = do_large_io(p_ctx, &rwbuf, buf_size);
    deallocate_buf(&rwbuf);

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

//int create_res_hndl_afu_reset(char *dev, dev64_t devno, __u64 chunk)
int create_res_hndl_afu_reset(bool do_recover, bool last)
{
    int rc;
    struct ctx my_ctx;
    struct ctx *p_ctx = &my_ctx;
    //int i;
    pthread_t thread;
    __u64 chunk = 0x1;
    __u64 stride= 0x1;
    int msgid;
    struct mymsgbuf msg_buf;
    pthread_t ioThreadId;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    // we have to export "NO_IO; if we want to avoid IO
    char * noIOP   = getenv("NO_IO");

    pid = getpid();
#ifdef _AIX
    memset(p_ctx,0,sizeof(my_ctx));
    strcpy(p_ctx->dev,cflash_path);
    if ((p_ctx->fd = open_dev(p_ctx->dev, O_RDWR)) < 0)
    {
        fprintf(stderr,"open failed %s, errno %d\n",p_ctx->dev, errno);
        return -1;
    }
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_query_path failed...\n");
    rc = ctx_init_internal(p_ctx, 0, p_ctx->devno);
#else
    rc = ctx_init(p_ctx);
#endif
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    if ( noIOP == NULL )
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    //create 0 vlun size & later call resize ioctl
    rc = create_resource(p_ctx, chunk * (p_ctx->chunk_size),
                         0, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    //last new process send message to waiting process
    //that new ctx created now you can try to reattach
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0 )
    {
        fprintf(stderr, "%d: msgget() failed before msgsnd()\n", pid);
        return -1;
    }
    memset(&msg_buf, 0, sizeof(struct mymsgbuf));
    if (last)
    {
        goto end;
    }
    if ( noIOP == NULL )
    {
        p_ioThreadData->p_ctx=p_ctx;
        p_ioThreadData->stride=stride;
        p_ioThreadData->loopCount=0x100000; // Need this to go on 10 secs
        debug("%d: things look good, doing IO...\n",pid);
        rc =pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
        CHECK_RC(rc, "do_io_thread() pthread_create failed");
    }
#ifdef _AIX
    rc = do_eeh(p_ctx);
#else
    rc = do_poll_eeh(p_ctx);
#endif
    if ( noIOP == NULL )
    {
        pthread_join(ioThreadId, NULL);
    }
#ifndef _AIX //for linux
    if ( noIOP == NULL )
        pthread_cancel(thread);
#endif

    //We here after EEH done
    if (do_recover)
    {
        //do if recover true
        debug("%d: woow EEH is done recovering...\n",pid);
        rc = ioctl_dk_capi_recover_ctx(p_ctx);
        CHECK_RC(rc, "ctx reattached failed");
        msg_buf.mtype =2;
        strcpy(msg_buf.mtext, "K");
        if (msgsnd(msgid, &msg_buf, 2, IPC_NOWAIT) < 0)
        {
            fprintf(stderr, "%d: msgsnd failed\n", pid);
            return -1;
        }
#ifdef _AIX
        if (p_ctx->return_flags != DK_RF_REATTACHED)
            CHECK_RC(1, "recover ctx, expected DK_RF_REATTACHED");
        p_ctx->flags = DK_VF_HC_TUR;
        p_ctx->hint = DK_HINT_SENSE;
#endif
        fflush(stdout);
        g_error=0; //reset any prev error might caught while EEH
        ctx_reinit(p_ctx);
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
#ifndef _AIX //for linux
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
#endif
    }
    else
    {
        //last one is
        /*msgid = msgget(key, IPC_CREAT | 0666);
           if(msgid < 0 ){
                   fprintf(stderr, "%d: msgget() failed before msgrcv()\n", pid);
                   return -1;
           }
           debug("%d: Going to wait at msgrcv()..\n", pid);
           fflush(stdout);
           if(msgrcv(msgid, &msg_buf, 2, 1, 0) < 0) {
                   fprintf(stderr, "%d: msgrcv failed with errno %d\n", pid, errno);
                   return -1;
           }

           debug("%d: Got out of msgrcv()..EEH is done, Try to recover....\n",pid);
         */
        //as per today(9/28/2015) discussion with Sanket that
        //new attach will fail until holding context not exited
        //hope same apply for Linux as well
        return 100;
        /*rc = ioctl_dk_capi_recover_ctx(p_ctx);
           if(rc) return 100; //this to make sure recover failed
           else {
                   fprintf(stderr,"%d:com'on recover should fail here...\n",pid);
                   return 1; // we don't want to try IO anyway
           }*/
    }

end:
    if ( noIOP == NULL )
    {
        stride=0x1;
        rc = do_io(p_ctx, stride);
        CHECK_RC(rc, "IO failed after EEH/recover");
    }
    if ( noIOP == NULL )
        pthread_cancel(thread);
    sleep(1);
    fflush(stdout);
    sleep(5); // additional time to be safe !
    rc=close_res(p_ctx);

    sleep(5); // Don't let child exit to keep max ctx alive
    rc |= ctx_close(p_ctx);
    CHECK_RC(rc,"ctx close or close_res failed\n");
    rc |= g_error ; // if g_error is set here inform caller 
    return rc;
}

int max_ctx_rcvr_except_last_one()
{
    int max_p = MAX_OPENS;
    int i;
    int rc;
    bool do_recover = true;
    int msgid;
    struct mymsgbuf msg_buf;
#ifndef _AIX
    char tmpBuff[MAXBUFF];
    char cmdToRun[MAXBUFF];
    const char *configCmdP = "echo 10000000  > /sys/kernel/debug/powerpc/eeh_max_freezes";

    if (turnOffTestCase("PVM") && manEEHonoff == 0)
    {
        TESTCASE_SKIP("Test case not supported in PowerVM env");
        return 0;
    }

#endif
    system("ipcrm -Q 0x1234 >/dev/null 2>&1");

    for (i = 0; i < max_p-1; i++)
    {
        if (0 == fork())
        {
            //child process
            rc = create_res_hndl_afu_reset(do_recover, false);
            exit(rc);
        }
    }
    if (0 == fork())
    {
        rc = create_res_hndl_afu_reset(false, false);
        if (100 == rc) rc = 0; //it was expected to failed with 100
        else rc=1;
        exit(rc);
    }

    //Wait for all all child to reach do_eeh() call.
    sleep(10);
#ifdef _AIX
    printf("%d:.....MAIN: BOSS do EEH now manually.......\n",getpid());
#else

    if (manEEHonoff != 0)
    {
         printf("%d:.....MAIN: BOSS do EEH now manually.......\n",getpid());
    } 
    else 
    {
        printf("%d:.....MAIN: doing  EEH now .......\n",getpid());

        rc = diskToPCIslotConv(cflash_path, tmpBuff );
        CHECK_RC(rc, "diskToPCIslotConv failed \n");

        rc = prepEEHcmd( tmpBuff, cmdToRun);
        CHECK_RC(rc, "prepEEHcmd failed \n");

        rc = system(configCmdP);
        if (rc)
        {
            g_error = -1;
            fprintf(stderr,"%d: Failed in %s \n",pid,configCmdP);
            exit(1);
        }

        debug("%d ---------- Command : %s----------------\n",pid,cmdToRun);
        rc = system(cmdToRun);
        if (rc)
        {
            g_error = -1;
            fprintf(stderr,"%d: Failed in %s \n",pid,cmdToRun);
            exit(1);
        }


    }
#endif

    sleep(10); // Let the max ctx recover before we proceed.

    printf("%d:.....MAIN: Hope EEH is done by now..\n",getpid());
    //now create new ctx
    //send true argument means, do create a msg queue
    //and inform waiting last process to perform reattach
    if (0 == fork())
    {
        pid=getpid();
        printf("%d: process created to attach after eeh...\n",pid);
        msgid = msgget(key, IPC_CREAT | 0666);
        if (msgid < 0 )
        {
            fprintf(stderr, "%d: msgget() failed before msgsnd()\n", pid);
            exit(-1);
        }
        if (msgrcv(msgid, &msg_buf, 2, 2, 0) < 0)
        {
            fprintf(stderr, "%d: msgrcv failed with errno %d\n", pid, errno);
            exit(-1);
        }
        printf("%d:OK Recover done now start new attach.......\n",pid);
        rc = create_res_hndl_afu_reset(true, true);
        exit(rc);
    }
    rc = wait4allOnlyRC();
    printf("%d: rc for wait4all(): %d\n", getpid(), rc);
    system("ipcrm -Q 0x1234");
    return rc;
}

int no_recover_and_ioctl()
{
    int rc;
    struct ctx my_ctx;
    struct ctx *p_ctx = &my_ctx;
    //__u64 flags;
    pthread_t thread;
    __u64 chunk = 0x1;
    __u64 stride= 0x1;
    pthread_t ioThreadId;

#ifdef _AIX
    //these are unused on Linux
    int msgid;
    struct mymsgbuf msg_buf;
#endif
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;

    char * noIOP   = getenv("NO_IO");

    pid = getpid();
    printf("%d:no_recover_and_ioctl process created...\n",pid);
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");


    rc = create_resource(p_ctx, chunk *(p_ctx->chunk_size),
                         DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    if ( noIOP == NULL )
    {
        //thread to handle AFU interrupt & events
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
        p_ioThreadData->p_ctx=p_ctx;
        p_ioThreadData->stride=stride;
        p_ioThreadData->loopCount=100;
        rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
        CHECK_RC(rc, "do_io_thread() pthread_create failed");
    }

#ifdef _AIX
    rc = do_eeh(p_ctx);
#else
    rc = do_poll_eeh(p_ctx);
#endif
    if ( noIOP == NULL )
    {
        pthread_join(ioThreadId, NULL);
    }
    if ( noIOP == NULL )
        pthread_cancel(thread);
#ifdef _AIX
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0 )
    {
        fprintf(stderr, "%d: msgget() failed before msgsnd()\n", pid);
        return -1;
    }
    if (msgrcv(msgid, &msg_buf, 2, 2, 0) < 0)
    {
        fprintf(stderr, "%d: msgrcv failed with errno %d\n", pid, errno);
        return -1;
    }
    sleep(1);
    rc = create_resource(p_ctx, p_ctx->chunk_size,
                         DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    rc |= vlun_resize(p_ctx, 2*p_ctx->chunk_size);
    rc |= close_res(p_ctx);
    rc |= ctx_close(p_ctx);
#else
    // For the lost context, we will create another new.
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    rc = create_resource(p_ctx, chunk *(p_ctx->chunk_size),
                         DK_UVF_ALL_PATHS, LUN_VIRTUAL);

    pthread_cancel(thread);
#endif
    return rc;
}

int max_ctx_rcvr_last_one_no_rcvr()
{
    int max_p = MAX_OPENS;
    int i;
    int rc;
    int pid=getpid();
    bool do_recover = true;
#ifndef _AIX
    char tmpBuff[MAXBUFF];
    char cmdToRun[MAXBUFF];
    const char *configCmdP = "echo 10000000  > /sys/kernel/debug/powerpc/eeh_max_freezes";

    if (turnOffTestCase("PVM") &&  manEEHonoff == 0)
    {
        TESTCASE_SKIP("Test case not supported in PowerVM env");
        return 0;
    }

#endif

    for (i = 0; i < max_p-1; i++)
    {
        if (0 == fork())
        {
            //child process
            rc = create_res_hndl_afu_reset(do_recover, false);
            exit(rc);
        }
    }

    if (0 == fork())
    {
        rc = no_recover_and_ioctl();
#ifdef _AIX
        if (rc == 46)
            rc = 0;
        else
            rc = 1;
#endif
        exit(rc);
    }
    //now do eeh
#ifdef _AIX
    sleep(10); // Let all child reach do_eeh() call.
#else
    sleep(4);
#endif

#ifdef _AIX
    printf("%d:.....MAIN: BOSS do EEH now manually.......\n",getpid());
#else
    if( manEEHonoff != 0)
    {
        printf("%d:.....MAIN: BOSS do EEH now manually.......\n",getpid());
    }
    else
    {
        printf("%d:.....MAIN: doing  EEH now .......\n",getpid());
        rc = diskToPCIslotConv(cflash_path, tmpBuff );
        CHECK_RC(rc, "diskToPCIslotConv failed \n");

        rc = prepEEHcmd( tmpBuff, cmdToRun);
        CHECK_RC(rc, "prepEEHcmd failed \n");

        rc = system(configCmdP);
        if (rc)
        {
            g_error = -1;
            fprintf(stderr,"%d: Failed in %s \n",pid,configCmdP);
            exit(1);
        }

        printf("%d ---------- Command : %s----------------\n",pid,cmdToRun);

        rc = system(cmdToRun);
        if (rc)
        {
            g_error = -1;
            fprintf(stderr,"%d: Failed in %s \n",pid,cmdToRun);
            exit(1);
        }

    }
#endif

    rc = wait4allOnlyRC();
    printf("%d: rc for wait4all(): %d\n", pid, rc);
    return rc;
}

int test_clone_ioctl(int cmd)
{
    struct ctx myctx;
    int i;
    pid_t cpid;
    struct ctx *p_ctx=&myctx;
    uint64_t nlba;
    uint64_t st_lba;
    uint64_t stride=0x1000;
    int rc=0;
    uint64_t src_ctx_id;
    uint64_t src_adap_fd;
    pthread_t thread;
    uint64_t resource[MAX_RES_HANDLE];
    uint64_t RES_CLOSED=-1;
    int cl_index[5]={ 1,7,10,12,15 };
    pid = getpid();

#ifndef _AIX
    if( diskSizeCheck( cflash_path , 128 ))
    {
       TESTCASE_SKIP("DISK SIZE is less than required \n");
       return 0;
    }
#endif
    rc =ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    p_ctx->flags = DK_UVF_ALL_PATHS;
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        p_ctx->lun_size = (i+1)*p_ctx->chunk_size;
        rc = create_res(p_ctx);
        CHECK_RC(rc, "create res failed");
        resource[i]=p_ctx->rsrc_handle;
    }
    for (i=0;i<5;i++)
    {
        p_ctx->rsrc_handle= resource[cl_index[i]];
        close_res(p_ctx);
        resource[cl_index[i]]= RES_CLOSED;
    }
    for (i=0; i<MAX_RES_HANDLE;i++)
    {
        if (RES_CLOSED == resource[i])
            continue;
        nlba = (i+1)*p_ctx->chunk_size;
        p_ctx->rsrc_handle = resource[i];
        p_ctx->res_hndl = p_ctx->rsrc_handle & RES_HNDLR_MASK;
        for (st_lba=0;st_lba<nlba;st_lba += (NUM_CMDS*stride))
        {
            rc = send_write(p_ctx,st_lba,stride,pid);
            CHECK_RC(rc, "send write failed\n");
        }
    }
    //write done cancel thread now
    pthread_cancel(thread);
    cpid = fork();
    if (cpid == 0)
    {
        //child process
        pid = getpid();
        ppid = getppid();
        //take backup parent ctx_id
        src_ctx_id= p_ctx->context_id;
        src_adap_fd = p_ctx->adap_fd;
        //do unmap parent mmio 1st
        rc =munmap((void *)p_ctx->p_host_map, p_ctx->mmio_size);
        CHECK_RC_EXIT(rc, "munmap failed\n");
        //do fresh attach for child
        rc = ctx_init_internal(p_ctx,DK_AF_ASSIGN_AFU,p_ctx->devno);
        CHECK_RC_EXIT(rc, "ctx_init_internal failed");
        pthread_create(&thread, NULL,ctx_rrq_rx,p_ctx);
        //do clone
        rc = ioctl_dk_capi_clone(p_ctx, src_ctx_id,src_adap_fd);
        CHECK_RC_EXIT(rc, "clone ioctl failed");
        //do read data
        for (i=0; i< MAX_RES_HANDLE;i++)
        {
            if (RES_CLOSED == resource[i])
                continue;
            p_ctx->rsrc_handle = resource[i];
            p_ctx->res_hndl = p_ctx->rsrc_handle & RES_HNDLR_MASK;
            nlba = (i+1)*p_ctx->chunk_size;
            for (st_lba=0;st_lba<nlba; st_lba+=(NUM_CMDS*stride))
            {
                rc = send_read(p_ctx,st_lba,stride);
                CHECK_RC_EXIT(rc,"send_read failed\n");
                rc = rw_cmp_buf_cloned(p_ctx, st_lba);
                CHECK_RC_EXIT(rc,"rw_cmp_buf_cloned failed\n");
            }
        }
        sleep(1);
        //now create closed resources
        p_ctx->flags = DK_UVF_ALL_PATHS;
        for (i=0; i < 5;i++)
        {
            p_ctx->lun_size = (cl_index[i]+1)*p_ctx->chunk_size;
            rc = create_res(p_ctx);
            CHECK_RC_EXIT(rc,"res_create failed\n");
            resource[cl_index[i]] = p_ctx->rsrc_handle;
        }
        //do io on new resources
        p_ctx->st_lba = 0;
        for (i=0;i<5;i++)
        {
            p_ctx->last_lba = ((cl_index[i]+1)*p_ctx->chunk_size) -1;
            p_ctx->res_hndl = resource[cl_index[i]] & RES_HNDLR_MASK;
            rc = do_io(p_ctx, stride);
            CHECK_RC_EXIT(rc, "do_io failed\n");
        }
        pthread_cancel(thread);
        ctx_close(p_ctx);
        exit(0);
    } //child process end
    else
    {
        //create pthread
        sleep(1); //let child process do clone & read written data
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
        //do open closed res
        //now create closed resources
        p_ctx->flags = DK_UVF_ALL_PATHS;
        for (i=0; i < 5;i++)
        {
            p_ctx->lun_size = (cl_index[i]+1)*p_ctx->chunk_size;
            rc = create_res(p_ctx);
            CHECK_RC_EXIT(rc,"res_create failed\n");
            resource[cl_index[i]] = p_ctx->rsrc_handle;
        }
        //do resize all resources & IO
        for (i=0;i<MAX_RES_HANDLE;i++)
        {
            p_ctx->req_size = (rand()%MAX_RES_HANDLE +1) * p_ctx->chunk_size;
            p_ctx->rsrc_handle = resource[i];
            p_ctx->res_hndl = p_ctx->rsrc_handle & RES_HNDLR_MASK;
            rc = ioctl_dk_capi_vlun_resize(p_ctx);
            CHECK_RC(rc, "dk_capi_resize_ioctl failed\n");
            rc = do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io failed\n");
        }
        //close res
        for (i=0;i<MAX_RES_HANDLE;i++)
        {
            p_ctx->rsrc_handle = resource[i];
            rc = close_res(p_ctx);
            CHECK_RC(rc, "cose_res failed\n");
        }

        pthread_cancel(thread);
        ctx_close(p_ctx);
        rc = wait4all();
    }
    return rc;
}

int max_ctx_cross_limit()
{
    int i;
    int rc = 0;
    struct ctx myctx;
    struct ctx *p_ctx=&myctx;
    pid = getpid();
    int max_p = MAX_OPENS;

#ifndef _AIX
    /* By default max_p will be set to BML max user context value */
    if ( host_type == CFLASH_HOST_PHYP )
    {
        max_p = MAX_OPENS_PVM ;
    }
#endif
    //int max_p = 1;
    for (i=0; i<max_p;i++)
    {
        if (0==fork())
        {
           pid=getpid();
           rc =ctx_init(p_ctx);
           sleep(10);
           exit(rc);
        }
    }
    sleep(5);
    rc=ctx_init(p_ctx);
    if(rc)
    {
      fprintf(stderr,"%d:expectd to fail....\n",pid);
      g_error=0;
      rc = 0;
    }
    else rc =10;
    rc = wait4all();
    return rc;
}

int max_ctx_on_plun(int cmd)
{
    int i;
    int rc = 0;
    struct ctx myctx;
    struct ctx *p_ctx=&myctx;
    pid = getpid();
    pthread_t thread;
    int max_p = MAX_OPENS;

#ifndef _AIX
    /* By default max_p will be set to BML max user context value */
    if ( host_type == CFLASH_HOST_PHYP )
    {
        max_p = MAX_OPENS_PVM ;
    }
#endif

    for (i=0; i<max_p;i++)
    {
        if (0==fork())
        {
            //child process
            pid = getpid();
            debug("%d: ......process %d created...\n",pid,i);
            memset(p_ctx, 0, sizeof(myctx));
            strcpy(p_ctx->dev, cflash_path);
            if ((p_ctx->fd = open_dev(p_ctx->dev, O_RDWR)) < 0)
            {
                fprintf(stderr,"open failed %s, errno %d\n",cflash_path, errno);
                exit(rc);
            }
#ifdef _AIX
            rc |= ioctl_dk_capi_query_path(p_ctx);
            rc|=ctx_init_internal(p_ctx, 0, p_ctx->devno);
#else
            rc|=ctx_init_internal(p_ctx, 0x2, p_ctx->devno);
#endif
            if (2 == cmd)
                rc |=create_resource(p_ctx,0,0,LUN_VIRTUAL);
            if (3 == cmd)
                rc |=create_resource(p_ctx,0,0,LUN_DIRECT);
            if (4 == cmd)
            {
                //do io all vluns created on path_id_mask
                pthread_create(&thread, NULL,ctx_rrq_rx,p_ctx);
                rc |= create_resource(p_ctx,p_ctx->chunk_size,0,LUN_VIRTUAL);
                rc |= do_io(p_ctx,0x10);

                pthread_cancel(thread);
            }
            sleep(10); //lets all context get created
            if ( 1 != cmd )
                rc|=close_res(p_ctx);
            rc|=ctx_close(p_ctx);
            debug("%d:.exiting with rc=%d\n",pid,rc);
            exit(rc);
        }
    }
    rc=wait4all();
    return rc;
}

int max_vlun_on_a_ctx()
{
    int i;
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx=&myctx;
    pid = getpid();
#ifndef _AIX   
    if( diskSizeCheck( cflash_path , RECO_DISK_SIZE ))
    {
       TESTCASE_SKIP("DISK SIZE is less than required \n");
       return 0;
    }
#endif

    rc=ctx_init(p_ctx);
    __u64 vluns[MAX_VLUNS];
    for (i=0;i<MAX_VLUNS;i++)
    {
        rc = create_resource(p_ctx,p_ctx->chunk_size,0,LUN_VIRTUAL);
        CHECK_RC(rc, "create_resource Failed\n");
        vluns[i]=p_ctx->rsrc_handle;
    }
    for (i=0;i<MAX_VLUNS;i++)
    {
        p_ctx->rsrc_handle=vluns[i];
        rc=close_res(p_ctx);
        CHECK_RC(rc, "close_res failed\n");
    }
    rc = ctx_close(p_ctx);
    return rc;
}

