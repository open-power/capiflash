/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/190.c $                                       */
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
extern int MAX_RES_HANDLE;
extern int g_error;
extern pid_t pid;

//int MAX_LIMIT=1;



void *max_ctx_res(void *arg)
{

#ifndef _AIX

    return 1;

#endif
    int rc;
    res_hndl_t my_res_hndl[MAX_RES_HANDLE];
    struct ctx *p_ctx = (struct ctx *)arg;
    int i;
    int long_run = atoi(getenv("LONG_RUN"));
    pthread_t pthread_id = pthread_self();
    __u64 size = 16;
    __u64 actual_size;
    __u64 nlba, st_lba;
    __u64 stride;
    mc_stat_t l_mc_stat;

    pid = getpid();
    rc = ctx_init(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        g_error =-1;
        return NULL;
    }

    //open max allowed res for a context
    size = (rand()%10+1)*16;
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        rc = mc_open(p_ctx->res_hndl,O_RDWR,&my_res_hndl[i]);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            return NULL;
        }
        debug("ctx:%d res hndl:%u\n",p_ctx->ctx_hndl,my_res_hndl[i]);
    }
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        rc = mc_size(p_ctx->res_hndl,my_res_hndl[i],size,&actual_size);
        if (rc != 0)
        {
            fprintf(stderr, "thread : %lx:mc_size: failed,rc %d\n", pthread_id,rc);
            g_error = -1;
            return NULL;
        }
        rc = mc_stat(p_ctx->res_hndl,my_res_hndl[i], &l_mc_stat);
        if (rc != 0)
        {
            fprintf(stderr, "thread : %lx:mc_stat: failed,rc %d\n", pthread_id,rc);
            g_error = -1;
            return NULL;
        }
        nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
        stride = 1 << l_mc_stat.nmask;
        nlba = 0; //NO IO here
        for (st_lba = 0;st_lba < nlba; st_lba += (NUM_CMDS*stride))
        {
            rc = send_write(p_ctx, st_lba, stride, pid);
            if ((rc != 0) && (actual_size == 0))
            {
                printf("%d : Fine, IO @(0X%lX) but range is(0X%lX)\n",pid, st_lba, nlba-1);
                size = 16;
                break;
            }
            else
            {
                fprintf(stderr,"%d : Send write failed @ (0X%lX) LBA\n", pid, st_lba);
                g_error = -1;
                return NULL;
            }
            rc = send_read(p_ctx, st_lba, stride);
            rc += rw_cmp_buf(p_ctx, st_lba);
            if (rc)
            {
                g_error = -1;
                return NULL;
            }
        }

        debug("ctx:%d res_hand:%d size:%lu\n",p_ctx->ctx_hndl,my_res_hndl[i],actual_size);
        size += 16;
    }
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        rc = mc_close(p_ctx->res_hndl,my_res_hndl[i]);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            return NULL;
        }
    }

    rc = mc_unregister(p_ctx->res_hndl);
    if (rc != 0)
    {
        fprintf(stderr, "mc_unregister failed for mc_hdl %p\n", p_ctx->res_hndl);
        g_error = -1;
        return NULL;
    }
    debug("mc unregistered for ctx:%d\n",p_ctx->ctx_hndl);
    return 0;
}
int max_ctx_n_res_190_1()
{
    int rc;
    int i;
    pthread_t threads[MAX_OPENS];
    struct ctx *p_ctx[MAX_OPENS];
    int MAX_CTX;
    int long_run = atoi(getenv("LONG_RUN"));

    if (mc_init() !=0 )
    {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }

    debug("mc_init success.\n");

    rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*MAX_OPENS);
    if (rc != 0)
    {
        fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
        return -1;
    }

    //Creating threads for ctx_init with nchan value
    // calculate how many ctx can be created

    MAX_CTX=MAX_OPENS/2;

    for (i = 0; i < MAX_CTX; i++)
    {
        rc = pthread_create(&threads[i],NULL, &max_ctx_res, (void *)p_ctx[i]);
        if (rc)
        {
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            free(p_ctx);
            return -1;
        }
    }

    //joining
    for ( i=0; i< long_run;i++)
    {
        for (i = 0; i < MAX_CTX; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }
    for (i = 0; i < MAX_CTX; i++)
    {
        ctx_close(p_ctx[i]);
    }
    free(p_ctx);
    rc = g_error;
    g_error = 0;
    return rc;
}


int max_ctx_n_res_190_2(  )
{
    int rc;
    int i;
    pthread_t threads[MAX_OPENS];
    struct ctx *p_ctx[MAX_OPENS];
    int MAX_CTX;
    int long_run = atoi(getenv("LONG_RUN"));
    if (mc_init() !=0 )
    {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }

    debug("mc_init success.\n");

    // calculate how many ctx can be created

    MAX_CTX=MAX_OPENS/2;


    rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*MAX_CTX);
    if (rc != 0)
    {
        fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
        return -1;
    }

    for (i = 0; i < MAX_CTX; i++)
    {
        rc=ctx_init(p_ctx[i]);
        if (rc != 0)
        {
            fprintf(stderr, "Context init failed, errno %d\n", errno);
            g_error =-1;
            return NULL;
        }
    }

    for ( i=0;i<long_run;i++)
    {

        rc = pthread_create(&threads[i],NULL, &max_ctx_res, (void *)p_ctx[i]);
        if (rc)
        {
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            free(p_ctx);
            return -1;
        }


        //joining

        for (i = 0; i < MAX_CTX; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

    rc = g_error;
    g_error = 0;
    return rc;
}


int ioctl_7_1_190(int flag)
{
    int rc;
    __u64 flags;
    int i=0,j=0;
    int long_run = atoi(getenv("LONG_RUN"));

    // start the test for each group

    if (0 == fork()) //child process
    {
        rc = max_ctx_n_res_190_1();
        exit(rc);
    }
    for ( i=0; i<long_run;i++)
    {
        if ( 0 == fork())
        {
            rc = max_ctx_n_res_190_2();
            exit(rc);
        }
    }

    return 0;
}
