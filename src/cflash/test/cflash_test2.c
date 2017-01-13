/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test2.c $                              */
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
#include <stdbool.h>

//extern char master_dev_path[MC_PATHLEN];
extern char cflash_path[MC_PATHLEN];
extern int MAX_RES_HANDLE;

extern pid_t pid;
extern __u8 rrq_c_null;
extern int dont_displa_err_msg;
extern bool long_run_enable;
/*each thread can modify this value incase of any failure */
extern int g_error;

/*
 *  * Serialization is required for a mc_handle
 *  * which is shared by multiple threads.
 *
 */
static pthread_mutex_t mutex;
static pthread_mutex_t counter=PTHREAD_MUTEX_INITIALIZER;
static int count=0;

typedef struct max_res_thread
{
    struct ctx *p_ctx;
    __u64 chunk; /* chunk size to request per thread */
    __u64 stride;
}max_res_thread_t;

void* test_mc_api(void *arg)
{
    max_res_thread_t *res_thrd = (max_res_thread_t *)arg;
    struct ctx *p_ctx = res_thrd->p_ctx;
    int rc=0;
    res_hndl_t res_handl;
    __u64 rsrc_handle;
    __u64 actual_size=0;
    __u64 nlba, st_lba;
    mc_stat_t l_mc_stat;

    __u64 size = res_thrd->chunk;
    __u64 stride = res_thrd->stride;;
    pthread_t pthread_id1 =pthread_self();
    unsigned int pthread_id =(unsigned int)pthread_id1;

    pthread_mutex_lock(&counter);
    count++;
    pthread_mutex_unlock(&counter);

    pthread_mutex_lock(&mutex);
    rc = create_res(p_ctx);
    res_handl = p_ctx->res_hndl;
    rsrc_handle = p_ctx->rsrc_handle;
    pthread_mutex_unlock(&mutex);
    sleep(1);//lets all thread created
    if (rc != 0)
    {
        fprintf(stderr, "thread : 0x%x:create_res: failed,rc %d\n", pthread_id,rc);
        g_error = -1;
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    p_ctx->rsrc_handle = rsrc_handle;
    rc = mc_size1(p_ctx, size, &actual_size);
    l_mc_stat.size = actual_size;
    pthread_mutex_unlock(&mutex);
    if (rc != 0)
    {
        fprintf(stderr, "thread : 0x%x:mc_size: failed,rc %d\n", pthread_id,rc);
        g_error = -1;
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    p_ctx->rsrc_handle = rsrc_handle;
    rc = mc_stat1(p_ctx, &l_mc_stat);
    pthread_mutex_unlock(&mutex);
    if (rc != 0)
    {
        fprintf(stderr, "thread : 0x%x:mc_stat: failed,rc %d\n", pthread_id,rc);
        g_error = -1;
        return NULL;
    }
    size = l_mc_stat.size;
    if (size != actual_size)
    {
        fprintf(stderr,"thread : 0x%x:size mismatched: %lu : %lu\n", pthread_id,size,actual_size);
        g_error = -1;
        return NULL;
    }

    nlba = size * (1 << l_mc_stat.nmask);

    debug("res hnd:%d send IO lba range 0X%"PRIX64"\n",res_handl, nlba);
    for (st_lba = 0; st_lba < nlba; st_lba +=(NUM_CMDS*stride))
    {
        pthread_mutex_lock(&mutex);
        p_ctx->res_hndl = res_handl;
        debug_2("res hnd: %d send IO for 0X%"PRIX64" \n",res_handl, st_lba);

        rc = send_write(p_ctx, st_lba, stride, pthread_id1);
        rc += send_read(p_ctx, st_lba, stride);
        rc += rw_cmp_buf(p_ctx, st_lba);

        pthread_mutex_unlock(&mutex);
        if (rc)
        {
            g_error = rc;
            break;
        }
    }

    sleep(1);
    pthread_mutex_lock(&mutex);
    p_ctx->rsrc_handle = rsrc_handle;
    rc = close_res(p_ctx);
    pthread_mutex_unlock(&mutex);
    if (rc != 0)
    {
        fprintf(stderr, "thread : 0x%x:close_res: failed,rc %d\n", pthread_id,rc);
        g_error = -1;
        return NULL;
    }
    return 0;
}

int mc_max_vdisk_thread()
{
    struct ctx myctx;
    struct pthread_alloc *p_thread_a;
    struct ctx *p_ctx = &myctx;
    pthread_mutexattr_t mattr;
    max_res_thread_t res[MAX_RES_HANDLE];
    pthread_t thread;
    int rc = 0;
    int i;

    pid = getpid();
    //Allocating structures for pthreads.
    p_thread_a = (struct pthread_alloc *) malloc(sizeof(struct pthread_alloc) * MAX_RES_HANDLE);
    if (p_thread_a == NULL)
    {
        fprintf(stderr, " Can not allocate thread structs, errno %d\n", errno);
        return -1;
    }
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx_init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    //initialize the mutex
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);
    //create threads
    for (i=0;i< MAX_RES_HANDLE; ++i)
    {
        res[i].p_ctx = p_ctx;
        res[i].chunk = i+1;
        res[i].stride = 0x1000;

        rc = pthread_create(&(p_thread_a[i].rrq_thread), NULL, &test_mc_api, (void *)&res[i]);
        if (rc)
        {
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            return -1;
        }
    }

    //destroy mutexattr
    pthread_mutexattr_destroy(&mattr);
    //joining
    for (i=0;i< MAX_RES_HANDLE; ++i)
    {
        pthread_join(p_thread_a[i].rrq_thread, NULL);
    }

    //destroy the mutex
    pthread_mutex_destroy(&mutex);
    pthread_cancel(thread);

    ctx_close(p_ctx);
    mc_term();
    //free allocated space
    free(p_thread_a);
    rc = g_error;
    g_error =0;
    return rc;
}

int test_mc_max_size()
{
    int rc = 0;
    struct ctx testctx;
    pthread_t thread;
    __u64 chunks =0x4;
    __u64 actual_size;
    __u64 max_size = 0;
    __u64 rnum;
    __u64 lun_size;
    __u64 stride = 0x1000; //4K
    __u64 st_lba = 0;
    int loop_stride = 1000;
    __u64 nlba;
    mc_stat_t l_mc_stat;
    bool is_stress = false;

    struct ctx  *p_ctx = &testctx;
    unsigned int i;
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG RUN Enabled....\n");
        chunks = 1; //increment one by one
        loop_stride = 10;
        is_stress = true;
    }

    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx_init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    rc = create_res(p_ctx);
    CHECK_RC(rc, "create_res failed");

    //allocate max allow size for a vdisk
    while (1)
    {
        rc = mc_size1(p_ctx,chunks, &actual_size);
        l_mc_stat.size = actual_size;
        if (chunks != actual_size)
        {
            debug("now reaching extreme..chunk(0X%"PRIX64") act(0X%"PRIX64")\n", chunks,actual_size);
            /*rc = mc_stat1(p_ctx, &l_mc_stat);
               CHECK_RC(rc, "mc_stat1 failed");*/
            max_size = l_mc_stat.size;
            break;
        }
        rc = mc_stat1(p_ctx,&l_mc_stat);
        if (rc != 0)
        {
            fprintf(stderr,"mc_stat failed rc = %d\n",rc);
            return -1;
        }
        nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
        debug("chunk(0X%"PRIX64")lba (0X%"PRIX64") i/o@(0X%"PRIX64")\n",
              actual_size,nlba, nlba-(NUM_CMDS*stride));
        rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid);
        if (rc) break;
        rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride);
        if (rc) break;
        rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
        if (rc) break;
        if (is_stress)
            chunks++;
        else
            chunks += 0x4;
    }

    if (max_size == 0)
    {
        debug("lets check more chunk can be allocated\n");
        rc = mc_size1(p_ctx, chunks+1, &actual_size);
        l_mc_stat.size = actual_size;
        debug("new chunk =0X%"PRIX64" & LBAs = 0X%"PRIX64"\n",
              actual_size, actual_size*(1 << l_mc_stat.nmask));
        fprintf(stderr, "some errors happend\n");
        return -1;
    }
    debug("OK, I got the lun size 0X%"PRIX64" & nlba 0X%"PRIX64"\n",
          max_size, max_size*(1 << l_mc_stat.nmask));
    lun_size = (max_size*(1 << l_mc_stat.nmask)*(l_mc_stat.blk_len)/(1024*1024*1024));


    nlba = max_size * (1 << l_mc_stat.nmask);
    if (is_stress)
    {
        printf("%d: now do IO till 0X%"PRIX64" lbas\n", pid, nlba-1);
        for (st_lba = 0; st_lba < nlba; st_lba += NUM_CMDS * stride)
        {
            rc = send_write(p_ctx, st_lba, stride, pid);
            CHECK_RC(rc, "send_write");
            rc = send_read(p_ctx, st_lba, stride);
            CHECK_RC(rc, "send_read");
            rc = rw_cmp_buf(p_ctx, st_lba);
            CHECK_RC(rc, "rw_cmp_buf");
        }
        fflush(stdout);
    }
    //allocate & dallocate max_size
    chunks = max_size;
    while (chunks > 0)
    {
        chunks = chunks/2;
        if (chunks == 0) break;
        rc = mc_size1(p_ctx, chunks, &actual_size);
        l_mc_stat.size = actual_size;
        if (rc != 0 || chunks != actual_size)
        {
            fprintf(stderr,"mc_size api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
        rc = mc_stat1(p_ctx, &l_mc_stat);
        if (rc != 0 || chunks != actual_size)
        {
            fprintf(stderr,"mc_stat api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
        actual_size = l_mc_stat.size;
        if (actual_size == 0) break;
        nlba = actual_size * (1 << l_mc_stat.nmask);
        debug("chunk(0X%"PRIX64")lba (0X%"PRIX64") i/o@(0X%"PRIX64")\n",
              actual_size,nlba, nlba-(NUM_CMDS*stride));
        rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid);
        if (rc) break;
        rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride);
        if (rc) break;
        rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
        if (rc) break;
    }

    for (i=0;i<=max_size;i+=loop_stride)
    {
        rnum = rand()% max_size +1;
        if (rnum*p_ctx->chunk_size > (p_ctx->last_phys_lba +1)) continue;
        rc = mc_size1(p_ctx, rnum, &actual_size);
        l_mc_stat.size = actual_size;

        if ((rc != 0)||(rnum != actual_size))
        {
            fprintf(stderr,"mc_size api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
        rc = mc_stat1(p_ctx, &l_mc_stat);
        if ((rc != 0 )|| (rnum != actual_size))
        {
            fprintf(stderr,"mc_stat api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
        actual_size = l_mc_stat.size;
        nlba = actual_size * (1 << l_mc_stat.nmask);
        debug("chunk(0X%"PRIX64")lba (0X%"PRIX64") i/o@(0X%"PRIX64")\n",
              actual_size,nlba, nlba-(NUM_CMDS*stride));
        rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid);
        if (rc) break;
        rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride);
        if (rc) break;
        rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
        if (rc) break;
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    printf("LUN size is :%lu GB\n",lun_size);
    return rc;
}

int test_one_aun_size()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 aun = 1;
    __u64 actual_size;
    __u64 nlba;
    __u64 size;
    __u64 stride = 0x10; //IO on all LBAs
    pthread_t thread;
    mc_stat_t l_mc_stat;

    pid = getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx_init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "create_res failed");

    rc = mc_size1(p_ctx, aun, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size1 failed");

    rc = mc_stat1(p_ctx,&l_mc_stat);
    if ((rc != 0) || (aun != l_mc_stat.size))
    {
        fprintf(stderr,"mc_get_size failed rc =%d: %lu : %lu\n", rc,aun,actual_size);
        return -1;
    }
    debug("mc_stat:blk_size=0X%X nmask=0X%X size=0X%"PRIX64" flags=0X%"PRIX64"\n",
          l_mc_stat.blk_len, l_mc_stat.nmask, l_mc_stat.size, l_mc_stat.flags);

    nlba = aun*(1 << l_mc_stat.nmask);
    size = nlba*(l_mc_stat.blk_len);
    debug("ONE AUN = %lu(0x%lx) LBAs and One AUN size =%lu(0x%lx)Bytes\n",nlba,nlba,size,size);
    debug("ONE AUN = %lu MB\n",size/(1024*1024));

    rc |= send_single_write(p_ctx, nlba-1, pid);
    rc |= send_single_read(p_ctx,  nlba-1);

    stride = p_ctx->blk_len;
    rc |= send_write(p_ctx, 0, stride, pid);
    rc |= send_read(p_ctx, 0, stride);
    rc |=  rw_cmp_buf(p_ctx, 0);

    rc |= send_write(p_ctx, nlba/2, stride, pid);
    rc |= send_read(p_ctx, nlba/2, stride);
    rc |=  rw_cmp_buf(p_ctx, nlba/2);
    CHECK_RC(rc, "IO");

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return 0;
}

void *exploit_chunk(void *arg)
{
    struct ctx *p_ctx = (struct ctx*)arg;
    //mc_hndl_t mc_hndl = p_ctx->mc_hndl;
    int rc=0;
    int i;
    res_hndl_t res_handl;
    __u64 rsrc_handle;
    __u64 size = 1;
    __u64 actual_size=0;
    //__u64 plba=0;
    __u64 st_lba;
    __u64 nlba;
    int myloop = 1;
    int inner_loop =1;
    __u64 stride = 0x1000;
    mc_stat_t l_mc_stat;

    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        myloop = 10;
        inner_loop = 10;
        stride = 1;
        debug("%d: %s : %d :Regress Outerloop: %d & inner loop:%d\n",
              pid, __func__, __LINE__, myloop, inner_loop);
    }
    while (myloop > 0)
    {
        pthread_mutex_lock(&mutex);
        rc = create_res(p_ctx);
        rsrc_handle = p_ctx->rsrc_handle;
        res_handl = p_ctx->res_hndl;
        pthread_mutex_unlock(&mutex);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:create_res: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            return NULL;
        }
        debug_2("%d : rsh %d started\n", pid, res_handl);
        for (i = 0; i< inner_loop;i++)
        {
            pthread_mutex_lock(&mutex);
            p_ctx->rsrc_handle = rsrc_handle;
            rc = mc_size1(p_ctx, size,&actual_size);
            l_mc_stat.size = actual_size;
            pthread_mutex_unlock(&mutex);
            if (rc != 0)
            {
                fprintf(stderr, "ctx: %d:mc_size: failed,rc %d\n", p_ctx->ctx_hndl,rc);
                g_error = -1;
                return NULL;
            }
            pthread_mutex_lock(&mutex);
            p_ctx->rsrc_handle = rsrc_handle;
            rc = mc_stat1(p_ctx, &l_mc_stat);
            pthread_mutex_unlock(&mutex);
            if (rc != 0)
            {
                fprintf(stderr, "ctx: %d:mc_stat: failed,rc %d\n", p_ctx->ctx_hndl,rc);
                g_error = -1;
                return NULL;
            }
            nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);

            debug_2("%d: R/W started for rsh %d from 0X0 to 0X%"PRIX64"\n",
                    pid, res_handl, nlba-1);
            for (st_lba = 0; st_lba < nlba; st_lba += NUM_CMDS*stride)
            {
                debug_2("%d: start lba 0X%"PRIX64" total lba 0X%"PRIX64" rsh %d\n",
                        pid, st_lba, nlba,res_handl);
                pthread_mutex_lock(&mutex);
                p_ctx->res_hndl = res_handl;
                rc = send_write(p_ctx, st_lba, stride, pid);
                if (rc)
                {
                    mc_stat1(p_ctx, &l_mc_stat);
                    if (size == 0 || (l_mc_stat.size * (1 << l_mc_stat.nmask)) <= st_lba)
                    {
                        printf("%d: Fine, send write(0X%"PRIX64") was out of bounds, MAX LBAs(0X%"PRIX64")\n",
                               pid, st_lba, size * (1 << l_mc_stat.nmask));
                        pthread_mutex_unlock(&mutex);
                        fflush(stdout);
                        break;
                    }
                    else
                    {
                        g_error = -1;
                        fprintf(stderr,"%d: chunk(0X%"PRIX64")IO failed rsh %d st_lba(0X%"PRIX64") range(0X%"PRIX64")\n",
                                pid, size, res_handl, st_lba, nlba-1);
                        pthread_mutex_unlock(&mutex);
                        return NULL;
                    }
                }
                else
                {
                    rc = send_read(p_ctx, st_lba, stride);
                    rc += rw_cmp_buf(p_ctx, st_lba);
                    pthread_mutex_unlock(&mutex);
                    if (rc)
                    {
                        g_error = -1;
                        return NULL;
                    }
                }
            }
            debug_2("%d: R/W done for rsh %d from 0X0 to 0X%"PRIX64"\n",
                    pid, res_handl, nlba-1);
            size = (rand()%10+1)*16;
        }
        pthread_mutex_lock(&mutex);
        p_ctx->rsrc_handle= rsrc_handle;
        rc = mc_stat1(p_ctx, &l_mc_stat);
        pthread_mutex_unlock(&mutex);
        size= l_mc_stat.size;

        sleep(2);
        pthread_mutex_lock(&mutex);
        p_ctx->rsrc_handle= rsrc_handle;
        rc = close_res(p_ctx);
        pthread_mutex_unlock(&mutex);
        debug_2("%d: now closing rsh %d\n", pid, res_handl);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            return NULL;
        }
        myloop--;
        debug("%d: %d loop remains was rsh %d\n", pid, myloop, res_handl);
    }
    return 0;
}
int chunk_regress()
{
    struct ctx_alloc p_ctx_a;
    struct pthread_alloc *p_thread_a;
    struct ctx *p_ctx = &(p_ctx_a.ctx);
    pthread_mutexattr_t mattr;
    pthread_t thread;
    int rc = 0;
    int i;
    int MAX_NUM_THREAD=MAX_RES_HANDLE;

    pid = getpid();
    debug("%d: afu=%s",pid, cflash_path);

    //Allocating structures for pthreads.
    p_thread_a = (struct pthread_alloc *) malloc(sizeof(struct pthread_alloc) * MAX_RES_HANDLE);
    if (p_thread_a == NULL)
    {
        fprintf(stderr, " Can not allocate thread structs, errno %d\n", errno);
        return -1;
    }
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx_init faild");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    //initialize the mutex
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);

    //create threads
    for (i=0;i< MAX_NUM_THREAD; ++i)
    {
        rc = pthread_create(&(p_thread_a[i].rrq_thread), NULL, &exploit_chunk, (void *)p_ctx);
        if (rc)
        {
            pthread_cancel(thread);
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            return -1;
        }
    }

    //destroy mutexattr
    pthread_mutexattr_destroy(&mattr);
    //joining
    for (i=0;i< MAX_NUM_THREAD; ++i)
    {
        pthread_join(p_thread_a[i].rrq_thread, NULL);
    }

    //destroy the mutex
    pthread_mutex_destroy(&mutex);

    pthread_cancel(thread);

    ctx_close(p_ctx);
    mc_term();
    //free allocated space
    free(p_thread_a);
    rc = g_error;
    g_error =0;
    debug("%d: I am returning %d\n", pid, rc);
    return rc;
}

int mc_size_regress_internal()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=MAX_RES_HANDLE;
    __u64 actual_size=0;
    //__u64 nlba; //unused
    __u64 stride=0x1000;
    __u32 i;
    int mc_size_regrss_l = 1;
    mc_stat_t l_mc_stat;
    pthread_t thread;
    pid = getpid();

    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG RUN Enabled....\n");
        mc_size_regrss_l = 100;
        stride = 0x1;
    }
    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    for (i = 1 ; i <= mc_size_regrss_l; i++)
    {
        chunks = chunks/2;
        rc = mc_size1(p_ctx,chunks, &actual_size);
        l_mc_stat.size = actual_size;
        CHECK_RC(rc, "mc_size");
        if (actual_size)
        {
            rc=do_io(p_ctx, stride);
            CHECK_RC(rc,"IO Failed..\n");
        }
        rc = mc_size1(p_ctx, 0, &actual_size);
        l_mc_stat.size = actual_size;
        CHECK_RC(rc, "mc_size");
        if ( i % 10 == 0)
        {
            system("date");
            printf("%d: loop %d(%d) done...\n", pid, i, mc_size_regrss_l);
        }
        fflush(stdout);
    }
    pthread_cancel(thread);
    return 0;
}

int mc_test_chunk_regress(int cmd)
{
    int rc;
    int i;
    //pid_t pid1;
    int max_p= MAX_OPENS;
#ifndef _AIX
    /* By default max_p will be set to BML max user context value */
    if ( host_type == CFLASH_HOST_PHYP )
    {
        max_p = MAX_OPENS_PVM ;
    }
#endif
    pid = getpid();
    MAX_RES_HANDLE=get_max_res_hndl_by_capacity(cflash_path);
    if (MAX_RES_HANDLE <= 0)
    {
        fprintf(stderr,"Unable to run max_ctx_max_res.. refere prior error..\n");
        fflush(stdout);
        return -1;
    }
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        system("date");
        printf("%d: Do %s Regress for %d context processes\n",
               pid, __func__, max_p);
        if (4 == cmd)
            debug("mc_size api(0 to value & viceversa) Regress 1000 loops...\n");
        fflush(stdout);
    }
    for (i = 0; i< max_p;i++)
    {
        if (fork() == 0)
        {
            debug("%d process created.........................................\n",i+1);
            usleep(1000);
            if (1 == cmd) // chunk regress
                rc = chunk_regress();
            else if (4 == cmd) //mc_size regress
                rc = mc_size_regress_internal();
            else //ctx regress create &destroy with io & wo io
                rc = mc_test_ctx_regress(cmd);

            if (rc )
            {
                debug("%d: exiting with rc = %d\n", pid, rc);
            }

            fflush(stdout);
            exit(rc);
        }
    }

    rc=wait4all();
    fflush(stdout);
    /*rc = g_error;
       g_error = 0;*/
    return rc;
}

int mc_test_chunk_regress_long()
{
    int rc;
    int i;
    int lrun=1;
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG RUN Enabled....\n");
        lrun = 100;
        printf("%d: Do %s Regress loop : %d\n",
               pid, __func__, lrun);
        fflush(stdout);
    }
    pid = getpid();
    for (i = 1; i <= lrun; i++)
    {
        debug("Loop %d(%d) started...\n", i, lrun);
        rc = mc_test_chunk_regress(1);
        debug("Loop %d(%d) done...\n", i, lrun);
        if (i%10 == 0)
        {
            system("date");
            printf("Loop %d(%d) done...\n", i, lrun);
            fflush(stdout);
        }
        if (rc)
        {
            fprintf(stderr, "Loop %d is failed with rc = %d\n", i, rc);
            break;
        }
    }
    return rc;
}
int mc_test_chunk_regress_both_afu()
{
    int rc;
    int i;
    //pid_t pid;
    int max_p = 4;
    char l_afu[MC_PATHLEN];
    //char l_master[MC_PATHLEN];
    char buffer[MC_PATHLEN];
    char *str;
    char *afu_1 = "0.0s";
    char *afu_2 = "1.0s";
    strcpy(l_afu, cflash_path);
    //strcpy(l_master, master_dev_path);
    for (i = 0; i < max_p; i++)
    {
        if (i%2)
        {
            strcpy(cflash_path, l_afu);
            //strcpy(master_dev_path, l_master);
        }
        else
        {
            str = strstr(l_afu, afu_1);
            if (str == NULL)
            {
                //ENV var set with 1.0
                strncpy(buffer, l_afu, strlen(l_afu)-strlen(afu_2));
                buffer[strlen(l_afu)-strlen(afu_2)]='\0';
                strcat(buffer, afu_1);
            }
            else
            {
                strncpy(buffer, l_afu, strlen(l_afu)-strlen(afu_1));
                buffer[strlen(l_afu)-strlen(afu_1)]='\0';
                strcat(buffer, afu_2);
            }
            strcpy(cflash_path, buffer);
            //strncpy(master_dev_path, cflash_path, strlen(afu_path)-1);
            //master_dev_path[strlen(cflash_path)-1] ='\0';
            //strcat(master_dev_path, "m");
        }
        if (fork() == 0)
        {
            rc =chunk_regress();
            exit(rc);
        }
    }

    wait4all();

    rc = g_error;
    g_error = 0;

    return rc;
}
int test_mix_in_out_bound_lba()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    int myloop = 2;
    __u64 chunks=256;
    __u64 actual_size=0;
    __u64 st_lba,nlba;
    __u64 stride;
    mc_stat_t l_mc_stat;
    pthread_t thread;
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG RUN Enabled....\n");
        myloop = 100;
    }

    pid = getpid();
    rc = mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx, chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    nlba = actual_size * (1 << l_mc_stat.nmask);
    stride = 1 << l_mc_stat.nmask;
    debug("%d: chunk(0X%"PRIX64") & lba range(0X%"PRIX64")\n", pid, actual_size, nlba-1);
    while (myloop-- > 0)
    {
        for (st_lba =0; st_lba < nlba; st_lba += (NUM_CMDS * stride))
        {
            //in bound
            send_write(p_ctx, st_lba, stride, pid);
            send_read(p_ctx, st_lba, stride);
            //out bound
            send_write(p_ctx, nlba + st_lba, stride, pid);
            send_read(p_ctx, nlba + st_lba, stride);
        }
    }
    pthread_cancel(thread);
    ctx_close(p_ctx);
    mc_term();
    fflush(stdout);
    return 0;
}

/*
 * Function : test_mc_good_error_afu_dev
 * return : 0 success else failure
 *
 * Run Good path test case like Chunk regress on one AFU dev
 * And Error path test case on another AFU dev
 * Make sure good path test case should run smoothly
 * Doesn't bother about error test cases
 */
int test_mc_good_error_afu_dev()
{
    int rc = 0;;
    int status=0;
    int i;
    int lloop = 1;
    //char buffer[MC_PATHLEN];
    int j;
    struct flash_disk fldisks[MAX_FDISK];
    int cfdisk;

    cfdisk = get_flash_disks(fldisks, FDISKS_ALL);
    if (cfdisk < 2)
    {
        TESTCASE_SKIP("Failed,need 2 flash disks ");
        return 0;
    }

    char *str1 = getenv("LONG_RUN");
    if (str1 != NULL)
    {
        printf("LONG RUN Enabled....\n");
        lloop = 100;
    }
    pid = getpid();
    debug("%d: Good path on %s\n", pid, cflash_path);
    //now create a child process & do error path
    if (fork() == 0)
    {
        dont_displa_err_msg = 1;
        pid = getpid();

        strcpy(cflash_path, fldisks[1].dev);
        debug("%d: Error path on disk %s\n", pid, cflash_path);
        debug("%d: error path process started..\n", pid);
        for (i = 0; i < lloop; i++)
        {
            debug("%d: starting loop %d(%d)\n", pid, i , lloop);
            rc = test_mix_in_out_bound_lba();

            for (j=1; j<=13; j++)
            {
                rc = test_mc_invalid_ioarcb(j);
            }
            rc = mc_test_engine(TEST_MC_RW_CLS_RSH);
            //rc = mc_test_engine(TEST_MC_UNREG_MC_HNDL);
        }
        debug("%d: error path process exited..\n", pid);
        exit(rc);
    }
    else
    {
        debug("%d: Good path process started..\n", pid);
        //good path on fldisks[0]
        strcpy(cflash_path, fldisks[0].dev);
        for (i = 0; i < lloop; i++)
        {
            rc += chunk_regress();
        }
        wait(&status);
        debug("%d: Good path process exited with rc=%d..\n", pid,rc);
    }
    return rc;
}

int mc_test_ctx_regress(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=MAX_RES_HANDLE;
    __u64 actual_size=0;
    //__u64 nlba; //unused
    __u64 stride;
    mc_stat_t l_mc_stat;
    pthread_t thread;

    pid = getpid();
    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    sleep(1);
    CHECK_RC(rc, "ctx init failed");
    if (long_run_enable)
        stride = p_ctx->blk_len;
    else
        stride = 0x1000;
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx,chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    if (3 == cmd)
    {
        rc=do_io(p_ctx,stride);
    }
    pthread_cancel(thread);
    sleep(1);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_mc_regress_ctx_crt_dstr(int cmd)
{
    int rc;
    int i;
    int lrun=1;
    char *str = getenv("LONG_RUN");
    if (str != NULL)
    {
        printf("LONG RUN Enabled....\n");
        lrun = 400; //for 4 Hrs
        if (3 == cmd)
            lrun = 100;
        system("date");
        printf("%s : %d : Regress loop : %d\n", __func__, __LINE__, lrun);
        fflush(stdout);
    }
    for (i = 1; i <= lrun; i++)
    {
        debug("Loop %d(%d) started...\n", i, lrun);
        if (1 == cmd) //mc_regress_ctx_crt_dstr without io
            rc = mc_test_chunk_regress(2);
        else //mc_regress_ctx_crt_dstr with io
            rc = mc_test_chunk_regress(3);

        debug("Loop %d(%d) done...\n", i, lrun);
        if (i%10 == 0)
        {
            system("date");
            printf("%d: Loop %d(%d) done...\n", getpid(), i, lrun);
            fflush(stdout);
        }
        if (rc)
        {
            fprintf(stderr, "Loop %d is failed with rc = %d\n", i, rc);
            break;
        }
    }
    return rc;
}
