/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_io.c $                            */
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
#include <sys/ipc.h>
#include <sys/shm.h>
// We wish to use pthread_tryjoin_np() in linux
#ifndef _AIX
#define __USE_GNU
#endif
#include <pthread.h>

extern char master_dev_path[MC_PATHLEN];
extern char cflash_path[MC_PATHLEN];

extern pid_t pid;
extern int dont_displa_err_msg;

static pthread_mutex_t mutex;
static pthread_cond_t condv1;
static pthread_cond_t condv2;

extern int g_error;
static __u32 W_READ = 0;
static __u32 W_WRITE = 0;
static __u32 W_PLAY = 0;
static __u32 W_RW = 0;
static __u32 hitZero = 0;
static int count=50;

//global allocation of read/write buffer
static struct ctx gs_ctx;

int N_done;
__u64 clba;

void *write_io(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    __u64 actual_size;
    __u64 nlba;
    __u64 stride;
    __u64 st_lba;
    __u64 chunk;
    int rc=0;

    pid = getpid();
    nlba = p_ctx->lun_size;
    actual_size = (p_ctx->lun_size/p_ctx->chunk_size);
    stride = p_ctx->chunk_size;
    st_lba = nlba/2;
    while (count > 0)
    {
        if (count%2)
        {
            chunk = actual_size - 1;
        }
        else
        {
            chunk =actual_size +1;
        }

        pthread_mutex_lock(&mutex);

        debug("%d: vLUN chunk 0x%"PRIX64", nlba 0x%"PRIX64"\n",pid,actual_size,nlba);
        debug("%d: write st_lba= 0x%"PRIX64",nlba=0x%"PRIX64"\n",pid,st_lba,nlba);
        rc |= send_write(p_ctx, st_lba, stride, pid);
        clba = st_lba;
        W_READ = 1;
        count--;
        rc |= mc_size1(p_ctx, chunk, &actual_size);
        pthread_cond_signal(&condv1);

        while ( W_WRITE != 1)
        {
            pthread_cond_wait(&condv2,&mutex);
        }
        W_WRITE = 0;

        pthread_mutex_unlock(&mutex);
        nlba = actual_size * p_ctx->chunk_size;
        st_lba = nlba/2;
        debug("%d: More %d Loop remaining............\n\n",pid,count);
    }
    if (rc != 0)
    {
        g_error = rc;
    }
    return 0;
}


void *play_size(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    __u64 actual_size;
    __u64 nlba;
    __u64 st_lba;
    __u64 w_chunk;
    int rc=0;

    pid = getpid();

    w_chunk = (p_ctx->last_phys_lba+1)/(2*p_ctx->chunk_size) - 2;

    while (count > 0)
    {
        pthread_mutex_lock(&mutex);
        while (W_PLAY != 1)
        {
            pthread_cond_wait(&condv2,&mutex);
        }
        W_PLAY = 0;

        mc_size1(p_ctx, w_chunk, &actual_size);
        if (actual_size == 0)
        {
            hitZero = 1;
            w_chunk = (p_ctx->last_phys_lba+1)/(2*p_ctx->chunk_size) - 2;
            rc |= mc_size1(p_ctx, w_chunk, &actual_size);
            debug("%d: chunk size reduced to 0, now increase to 0X%"PRIX64"\n", pid, actual_size);
        }

        debug("%d: mc_size done 0X%"PRIX64"\n",pid, actual_size);

        nlba = p_ctx->lun_size;

        if (count % 2)
        {
            st_lba = nlba/2;
            w_chunk = actual_size/2+1;
        }
        else
        {
            st_lba = nlba -1;
            w_chunk = actual_size-1;
        }
        clba = st_lba;
        count--;
        debug("%d: clba 0X%"PRIX64": lba range:0X%"PRIX64"\n",pid, clba, nlba-1);

        W_RW = 1;
        pthread_cond_signal(&condv1);
        pthread_mutex_unlock(&mutex);

        debug("%d: chunk 0x%"PRIX64", nlba 0x%"PRIX64", st_lba 0x%"PRIX64"\n",pid, actual_size,nlba,st_lba);
        debug("%d: More %d Loop remaining ............\n",pid, count);

        if (rc != 0)
        {
            g_error = rc;
            fprintf(stderr,"%d: failed here %s:%d:%s\n",pid,
                     __FILE__,__LINE__,__func__);
            pthread_exit(0);
        }
    }
    return 0;
}


void *rw_io(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    int rc = 0;
    __u64 llba;

    pid = getpid();

    while (count > 0)
    {
        pthread_mutex_lock(&mutex);

        llba = clba;
        if ( hitZero !=1 )
        {
            debug("\n%d: ------------------------------------------------\n",pid);
            debug("%d: Writing @lba 0X%"PRIX64"",pid,llba);
            debug(", Max lba range is upto 0X%"PRIX64"\n",
                  p_ctx->lun_size-1 );
            rc |= send_single_write(p_ctx, llba, pid);
        }

        W_PLAY = 1;
        pthread_cond_signal(&condv2);
        pthread_mutex_unlock(&mutex);

        if (rc && (llba >= p_ctx->lun_size-1))
        {
            debug("%d: Resetting RC, write(0X%"PRIX64"), max range(0X%"PRIX64")\n",
                  pid, llba, p_ctx->lun_size-1);
            rc = 0;
        }
        if (rc != 0)
        {
            g_error = rc;
            fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
                     __FILE__,__LINE__,__func__);
            return NULL;
        }

        pthread_mutex_lock(&mutex);
        while ( W_RW != 1 )
        {
            pthread_cond_wait(&condv1,&mutex);
        }
        W_RW = 0;

        if ( hitZero !=1 )
        {
            debug("%d: Reading from 0X%"PRIX64"",pid, llba);
            debug(", Max lba range is upto 0X%"PRIX64"\n",
                  p_ctx->lun_size-1);
            debug("%d: ------------------------------------------------\n",pid);
            rc |= send_single_read(p_ctx, llba);
        }

        if (rc && llba >= p_ctx->lun_size-1)
        {
            debug("%d: Resetting RC, read (0X%"PRIX64"), max range(0X%"PRIX64")\n",
                  pid, llba, p_ctx->lun_size-1);
            rc = 0;
        }
        else
        {
            if ( hitZero !=1 ) rc |= rw_cmp_single_buf(p_ctx, llba);
        }

        hitZero = 0; // Reset at end of read cycle.
        W_PLAY = 1;
        pthread_cond_signal(&condv2);
        pthread_mutex_unlock(&mutex);
        if (rc != 0)
        {
            g_error = rc;
            fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                     __FILE__,__LINE__,__func__);
            return NULL;
        }
    }
    return 0;
}

void *read_io(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    __u64 stride;
    int rc=0;

    stride = p_ctx->chunk_size;
    while (count > 0)
    {
        pthread_mutex_lock(&mutex);
        while ( W_READ != 1)
        {
            pthread_cond_wait(&condv1,&mutex);
        }
        W_READ = 0;
        debug("%d: read clba= 0x%"PRIX64"\n",pid,clba);
        rc |= send_read(p_ctx, clba, stride);
        rc |= rw_cmp_buf(p_ctx, clba);

        W_WRITE = 1;
        pthread_cond_signal(&condv2);
        pthread_mutex_unlock(&mutex);
        if (rc != 0)
        {
            g_error = rc;
            return NULL;
        }
    }
    return 0;
}

int test_onectx_twothrd(int cmd)
{
    int rc=0;
    struct timespec ts;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    char *fn1=NULL;
    char *fn2=NULL;
    pthread_t thread;
    pthread_t rhthread[2];

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    __u64 chunks;
    __u64 actual_size;

    pid=getpid();

    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);

    pthread_condattr_init(&cattr);
    pthread_cond_init(&condv1, &cattr);
    pthread_cond_init(&condv2, &cattr);
    if (test_init(p_ctx) != 0)
    {
        return -1;
    }

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "create_res failed");

    chunks = (p_ctx->last_phys_lba+1)/(p_ctx->chunk_size) - 2;

    rc = mc_size1(p_ctx,chunks, &actual_size);
    CHECK_RC(rc, "mc_size1");

    if (1 == cmd)
    {
        //a thread write_io & another read_io
        pthread_create(&rhthread[0], NULL, write_io, p_ctx);
        pthread_create(&rhthread[1], NULL, read_io, p_ctx);
        fn1="write_io";
        fn2="read_io";
    }
    else if (2 == cmd)
    {
        //a thread rw & another mc size
        clba = (actual_size * p_ctx->chunk_size)/2;
        pthread_create(&rhthread[0], NULL, rw_io, p_ctx);
        pthread_create(&rhthread[1], NULL, play_size, p_ctx);
        fn1="rw_io";
        fn2="play_size";
    }
    //pthread_create(&rhthread[1], NULL, inc_dec_size, p_ctx);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);

#ifndef _AIX
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        CHECK_RC(1, "clock_gettime() failed");
    }

    ts.tv_sec += 10;
    if ( ETIMEDOUT == pthread_timedjoin_np(rhthread[0], NULL, &ts))
        debug("%d: thread fn. %s() timed out.. Exceeded 10 secs, so terminated !\n", pid, fn1);

    ts.tv_sec += 2;
    if ( ETIMEDOUT == pthread_timedjoin_np(rhthread[1], NULL, &ts))
        debug("%d: thread fn. %s() timed out.. Exceeded 12 secs, so terminated !\n", pid, fn2);
#else
    pthread_join(rhthread[0], NULL);
    pthread_join(rhthread[1], NULL);
#endif

    pthread_cancel(thread);

    close_res(p_ctx);
    //mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
    mc_term();
    rc = g_error;
    g_error = 0;
    return rc;
}

int test_two_ctx_two_thrd(int cmd)
{
    int i, rc=0;
    char *fn1=NULL;
    char *fn2=NULL;
    struct timespec ts;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread[2];

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    __u64 chunks;
    __u64 actual_size;

    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);

    pthread_condattr_init(&cattr);
    pthread_cond_init(&condv1, &cattr);
    pthread_cond_init(&condv2, &cattr);
    for (i = 0;i < 2;i++)
    {
        if (fork() == 0)
        {
            //child process

            pid = getpid();

            if (test_init(p_ctx) != 0)
            {
                exit(-1);
            }
            chunks = (p_ctx->last_phys_lba+1)/(p_ctx->chunk_size*2) - 2;
            pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

            rc = create_res(p_ctx);
            if (rc != 0)
            {
                fprintf(stderr, "error opening res_hndl rc %d\n", rc);
                exit(-1);
            }

            rc = mc_size1(p_ctx,chunks, &actual_size);
            CHECK_RC(rc, "mc_size1");

            if (1 == cmd)
            {
                //a thread write_io & another read_io
                pthread_create(&rhthread[0], NULL, write_io, p_ctx);
                pthread_create(&rhthread[1], NULL, read_io, p_ctx);
                fn1="write_io";
                fn2="read_io";
            }
            else if (2 == cmd)
            {
                //a thread rw & another mc size
                clba = (actual_size * p_ctx->chunk_size)/2;
                pthread_create(&rhthread[0], NULL, rw_io, p_ctx);
                pthread_create(&rhthread[1], NULL, play_size, p_ctx);
                fn1="rw_io";
                fn2="play_size";
            }

            pthread_mutexattr_destroy(&mattr);
            pthread_condattr_destroy(&cattr);

#ifndef _AIX
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                CHECK_RC(1, "clock_gettime() failed");
            }

            ts.tv_sec += 10;
            if ( ETIMEDOUT == pthread_timedjoin_np(rhthread[0], NULL, &ts))
                debug("%d: thread fn. %s() timed out.. Exceeded 10 secs, so terminated !\n", pid, fn1);

            ts.tv_sec += 2;
            if ( ETIMEDOUT == pthread_timedjoin_np(rhthread[1], NULL, &ts))
                debug("%d: thread fn. %s() timed out.. Exceeded 12 secs, so terminated !\n", pid, fn2);
#else
            pthread_join(rhthread[0], NULL);
            pthread_join(rhthread[1], NULL);
#endif
            pthread_cancel(thread);
            close_res(p_ctx);
            //mc_unregister(p_ctx->mc_hndl);
            ctx_close(p_ctx);
            mc_term();
            rc = g_error;
            debug("%d: I am exiting from here .....rc = %d\n", pid, rc);
            exit(rc);
        }
    }
    while ((pid =  waitpid(-1,&rc,0)))
    {
        if (pid == -1)
        {
            break;
        }

        rc =  WEXITSTATUS(rc);
        debug("%d:  wait is over for me............rc = %d\n", pid, rc);
        if (rc != 0)
        {
            g_error = -1;
        }
    }
    rc = g_error;
    g_error=0;
    return rc;
}

int test_lun_discovery(int cmd)
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    int fc_port =0;
    __u64 *lun_ids;
    __u32 n_luns;
    int port=2;
    int i;
    __u64 lun_cap,blk_len;

    if (test_init(p_ctx) != 0)
    {
        return -1;
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    // Send Report LUNs to get the list of LUNs and LUN ID
    for (i =1; i <= port;i++)
    {
        rc = send_report_luns(p_ctx, i, &lun_ids,&n_luns);
        if (rc)
        {
            fprintf(stderr, "Report LUNs failed on FC Port %d\n", i);
        }
        else
        {
            fc_port = i;
            break;
        }
    }
    if (rc || n_luns == 0)
    {
        ctx_close(p_ctx);
        return rc;
    }
    debug("Report Lun success, num luns= 0x%x\n",n_luns);
    for (i = 0; i< n_luns;i++)
    {
        rc = send_read_capacity(p_ctx,fc_port,lun_ids[i],&lun_cap, &blk_len);
        if (rc != 0)
        {
            fprintf(stderr,"Read capacity failed,lun id =0x%"PRIX64", rc = %d\n",lun_ids[i],rc);
            break;
        }
        debug("LUN id = 0x%"PRIX64" Capacity = 0x%"PRIX64" Blk len = 0x%"PRIX64"\n",
              lun_ids[i],lun_cap,blk_len);
    }
    free(lun_ids);
    pthread_cancel(thread);
    ctx_close(p_ctx);
    return rc;

}

int test_vdisk_io()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;

    __u64 chunk = 256;
    __u64 nlba;
    __u64 actual_size;
    __u64 st_lba =0;
    __u64 stride;
    mc_stat_t l_mc_stat;


    if (mc_init() != 0)
    {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
    debug("mc_init success.\n");

    if (ctx_init(p_ctx) != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        return -1;
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    /*rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
                            (volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
       if (rc != 0) {
               fprintf(stderr, "error registering(%s) ctx_hndl %d, rc %d\n",
                               master_dev_path, p_ctx->ctx_hndl, rc);
               return -1;
       }*/

    rc = create_res(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
    }

    rc = mc_size1(p_ctx, chunk, &actual_size);
    l_mc_stat.size = actual_size;
    if (rc != 0 || actual_size < 1) //might be chunk want to allocate whole lun
    {
        fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
        return -1;
    }
    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat1");

    pid = getpid();

    stride = (1 << l_mc_stat.nmask);
    nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
    debug("%d: st_lba = 0X0 and range lba = 0X%"PRIX64"\n", pid, nlba-1);
    for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride))
    {
        send_write(p_ctx, st_lba, stride, pid);
        send_read(p_ctx, st_lba, stride);
        rc = rw_cmp_buf(p_ctx, st_lba);
        if (rc)
        {
            fprintf(stderr,"buf cmp failed for vlba 0x%"PRIX64",rc =%d\n",
                    st_lba,rc);
            break;
        }
    }
    pthread_cancel(thread);
    close_res(p_ctx);
    //mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
    return rc;
}

void *only_rw_io(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    __u64 stride = 0x1000; //4K
    int rc;
    __u64 nlba = clba;
    __u64 st_lba;

    pid = getpid();
    while (1)
    {
        for (st_lba =0; st_lba < nlba; st_lba += (NUM_CMDS * stride))
        {
            //rc = send_single_write(p_ctx,st_lba,pid);
            rc = send_write(p_ctx,st_lba,stride,pid);
            if (rc != 0)
            {
                g_error = rc;
                if (!dont_displa_err_msg)
                    fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                             __FILE__,__LINE__,__func__);
                return NULL;
            }
            //rc = send_single_read(p_ctx, st_lba);
            rc = send_read(p_ctx, st_lba, stride);
            if (rc != 0)
            {
                g_error = rc;
                if (!dont_displa_err_msg)
                    fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                             __FILE__,__LINE__,__func__);
                return NULL;
            }
            //rc = rw_cmp_single_buf(p_ctx, st_lba);
            rc = rw_cmp_buf(p_ctx, st_lba);
            if (rc != 0)
            {
                g_error = rc;
                if (!dont_displa_err_msg)
                    fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                             __FILE__,__LINE__,__func__);
                return NULL;
            }
        }
        if (N_done)
        {
            break;
        }
    }
    return 0;
}

void *cancel_thread(void *arg)
{
    sleep(2);
    pthread_t *thread=arg;
    pthread_cancel(*thread);
    return NULL;
}
int test_rw_close_hndl(int cmd)
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread;
#ifdef _AIX
    pthread_t thread1;
#endif
    mc_stat_t l_mc_stat;
    __u64 chunks=128;
    __u64 actual_size;

    dont_displa_err_msg=1;

    pid = getpid();
    signal(SIGSEGV, sig_handle);
    signal(SIGABRT, sig_handle);
    if (test_init(p_ctx) != 0)
    {
        return -1;
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    CHECK_RC(rc, "create_res");

    rc = mc_size1(p_ctx,chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size1");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat1");

    clba = actual_size * (1 << l_mc_stat.nmask);
    pthread_create(&rhthread, NULL, only_rw_io, p_ctx);
    sleep(1);

    if (1 == cmd)
    {
        //while IO close RHT
        debug("%d:closing res handler:0X%"PRIX64"\n",pid,p_ctx->rsrc_handle);
        close_res(p_ctx);
    }
    else if (2 == cmd)
    {
        //close disk only
#ifdef _AIX //close sys call wil wait until poll thred not killed
        pthread_create(&thread1,NULL,cancel_thread,&rhthread);
#endif
        debug("%d:closing disk only.... \n",pid);
        close(p_ctx->fd);
    }
    else if (3 == cmd)
    {
        //While IO ctx close
#ifdef _AIX
        // we need to close the poll()thread
        // before detach call; please refer defect#983973,seq#17
        pthread_cancel(thread);
#endif
        debug("%d:detach context:0X%"PRIX64"\n",pid,p_ctx->context_id);
        ioctl_dk_capi_detach(p_ctx);
    }
    else if (4 == cmd)
    {
        //only for Linux
        debug("%d:do unmap mmio during IO....\n",pid);
        munmap((void *)p_ctx->p_host_map, p_ctx->mmio_size);
    }

    N_done = 1; //tell pthread that -ve test performed
    debug("%d:sleeping for 2 secs before context detach & disk close\n",pid);
    sleep(2);
    pthread_cancel(rhthread);
    //pthread_join(rhthread, NULL);
    N_done = 0;

#ifdef _AIX
    if (3 != cmd)
    {
      pthread_cancel(thread);
    }
#else
    pthread_cancel(thread);
#endif

    // do proper closing
    if (cmd == 1)
    {
        ctx_close(p_ctx);
    }
    mc_term();
    rc = g_error;
    g_error = 0;
    return rc;
}
int test_good_ctx_err_ctx(int cmd)
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread;
    mc_stat_t l_mc_stat;

    __u64 chunks=128;
    __u64 actual_size;

    pid_t mypid = fork();
    //let both  process do basic things
    if (test_init(p_ctx) != 0)
    {
        exit(-1);
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    rc = create_res(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        exit(-1);
    }

    rc = mc_size1(p_ctx,chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    clba = actual_size * (1 << l_mc_stat.nmask);
    pthread_create(&rhthread, NULL, only_rw_io, p_ctx);

    if (mypid  == 0)
    {
        //child process do err ctx
        debug("child pid is :%d\n",getpid());
        sleep(1); //let thrd do some io
        printf("error ctx pid is %d\n",getpid());
        if (1 == cmd)
        {
            //while IO close RHT
            close_res(p_ctx);
            /*}else if(2 == cmd) { //while IO unreg MC HNDL
                    mc_unregister(p_ctx->mc_hndl);*/
        }
        else if (3 == cmd)
        {
            //While IO ctx close
            //munmap((void*)p_ctx->p_host_map, 0x10000);
            close(p_ctx->fd);
            //ctx_close(p_ctx);
        }
        sleep(1);
        N_done = 1; //tell pthread that -ve test performed
        pthread_cancel(rhthread);
        //pthread_join(rhthread, NULL);
        N_done = 0;
        debug("%d: exiting with rc = %d\n", pid, g_error);
        pthread_cancel(thread);
        exit(g_error);
    }
    else
    {
        debug("Good ctx pid is : %d\n",getpid());
        sleep(2); //main process sleep but thrd keep running
        N_done = 1; //
        pthread_join(rhthread, NULL);
        N_done = 0;

        // do proper closing
        pthread_cancel(thread);
        close_res(p_ctx);
        //mc_unregister(p_ctx->mc_hndl);
        ctx_close(p_ctx);
        debug("%d: waiting for child process %d\n", pid, mypid);
        wait(&rc);
    }
    rc = g_error;
    g_error = 0;
    mc_term();
    return rc;
}

int test_mc_ioarcb_ea_alignment(int cmd)
{
    int rc;
    int a;
    struct rwbuf *p_rwb;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=128;
    __u64 actual_size=0;
    __u64 st_lba, nlba;
    __u64 stride = 0x1000;
    int offset;
    pthread_t thread;
    int max;
    mc_stat_t l_mc_stat;


    if (1 == cmd) //16 byte ea alignment
        offset = 16;
    else if (2 == cmd) //128 byte ea alignment
        offset = 128;
    else //invalid ea alignment
        offset = 5;

    max = offset * 10; //try for next 10 offset
    pid = getpid();
    rc = posix_memalign((void **)&p_rwb, 0x1000, sizeof( struct rwbuf ));
    CHECK_RC(rc, "rwbuf allocation failed");
    debug("initial buf address : %p\n",p_rwb);
    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    /*rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
       CHECK_RC(rc, "ctx reg failed");
     */
    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx,chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size");

    if (chunks != actual_size)
    {
        CHECK_RC(1, "doesn't have enough chunk space");
    }
    st_lba = 0;

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    nlba = actual_size * (1 << l_mc_stat.nmask);
    debug("EA alignment from begining of 4K\n");
    for (a=offset; a <= max; a+=offset)
    {
        debug("send alignment offset : %u\n",a);
        rc = send_rw_rcb(p_ctx, p_rwb, st_lba, stride, a, 0);
        if (rc) break;
        rc = send_rw_rcb(p_ctx, p_rwb, nlba/2, stride, a, 0);
        if (rc) break;
        rc = send_rw_rcb(p_ctx, p_rwb, nlba-(NUM_CMDS * stride), stride, a, 0);
        if (rc) break;
    }
    //CHECK_RC(rc, "send_rw_rcb");
    debug("EA alignment from end of a 4K\n");
    for (a=offset; a <= max; a+=offset)
    {
        debug("send alignment offset from last : %u\n", a);
        rc = send_rw_rcb(p_ctx, p_rwb, st_lba, stride, a, 1);
        if (rc) break;
        rc = send_rw_rcb(p_ctx, p_rwb, nlba/2, stride, a, 1);
        if (rc) break;
        rc = send_rw_rcb(p_ctx, p_rwb, nlba-(NUM_CMDS * stride), stride, a, 1);
        if (rc) break;
    }
    pthread_cancel(thread);
    //mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
    free(p_rwb);
    mc_term();
    if (rc!=0 && cmd == 3)
        return 3;
    return rc;
}

int mc_test_rwbuff_global(int cmd)
{
    int rc;
    struct ctx *p_ctx = &gs_ctx;
    __u64 chunks=64;
    __u64 actual_size=0;
    __u64 st_lba;
    __u64 stride;
    __u64 nlba;
    pthread_t thread;
    mc_stat_t l_mc_stat;

    if (2 == cmd)
    {
        //allocate from heap
        //p_ctx = (struct ctx *)malloc(sizeof(struct ctx));
        //IOARCB req 16 Byte Allignment
        posix_memalign((void**)&p_ctx, 16, sizeof(struct ctx));
        if (NULL == p_ctx)
        {
            fprintf(stderr,"Mem allocation failed\n");
            return -1;
        }
    }
    pid = getpid();

    rc =mc_init();
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
    stride = (1 << l_mc_stat.nmask);
    for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS * stride))
    {
        rc = send_write(p_ctx, st_lba, stride, pid);
        CHECK_RC(rc, "send_write");
        rc = send_read(p_ctx, st_lba, stride);
        CHECK_RC(rc, "send_read");
        rc = rw_cmp_buf(p_ctx, st_lba);
        CHECK_RC(rc, "rw_cmp_buf");
    }
    pthread_cancel(thread);
    ctx_close(p_ctx);
    if (2 == cmd)
    {
        //deallocate from heap
        free(p_ctx);
    }
    mc_term();
    return rc;
}

void *only_play_size(void *arg)
{
    struct ctx *p_ctx = (struct ctx *)arg;
    __u64 actual_size;
    __u64 w_chunk;
    int rc;
    int myloop = count * 10;
    mc_stat_t l_mc_stat;

    l_mc_stat.size = p_ctx->lun_size/p_ctx->chunk_size;
    rc =mc_stat1(p_ctx, &l_mc_stat);
    w_chunk = l_mc_stat.size;
    while (myloop-- > 0)
    {
        w_chunk +=128;
        debug("%d: doing mc size from 0X%"PRIX64" to 0X%"PRIX64"\n", pid, actual_size, w_chunk);
        rc = mc_size1(p_ctx, w_chunk, &actual_size);
        if (rc != 0)
        {
            g_error = rc;
            N_done = 1;
            fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                     __FILE__,__LINE__,__func__);
            return NULL;
        }
        w_chunk -=128;
        debug("%d: doing mc size from 0X%"PRIX64" to 0X%"PRIX64"\n", pid, actual_size, w_chunk);
        rc = mc_size1(p_ctx, w_chunk, &actual_size);
        if (rc != 0)
        {
            g_error = rc;
            N_done = 1;
            fprintf(stderr, "%d: failed here %s:%d:%s\n",pid,
                     __FILE__,__LINE__,__func__);
            return NULL;
        }
    }
    N_done = 1; //now tell other thread, i m done
    return 0;
}

int test_mc_rw_size_parallel()
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread[2];
    mc_stat_t l_mc_stat;
    int i;

    __u64 chunks=64;
    __u64 actual_size;

    for (i =0 ;i < 4; i++)
    {
        if (fork() == 0)
        {
            sleep(1); //lets all process get created
            rc = test_init(p_ctx);
            CHECK_RC(rc, "test init");
            pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

            /*rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
                    (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
               CHECK_RC(rc, "mc_register");
             */
            rc = create_res(p_ctx);
            CHECK_RC(rc, "create_res");

            rc = mc_size1(p_ctx,chunks, &actual_size);
            l_mc_stat.size = actual_size;
            CHECK_RC(rc, "mc_size");

            rc = mc_stat1(p_ctx, &l_mc_stat);
            CHECK_RC(rc, "mc_stat");

            clba = (actual_size * (1 << l_mc_stat.nmask));
            pthread_create(&rhthread[0], NULL, only_rw_io, p_ctx);
            pthread_create(&rhthread[1], NULL, only_play_size, p_ctx);

            pthread_join(rhthread[0], NULL);
            pthread_join(rhthread[1], NULL);

            pthread_cancel(thread);
            close_res(p_ctx);
            //mc_unregister(p_ctx->mc_hndl);
            ctx_close(p_ctx);
            mc_term();
            rc = g_error;
            exit(rc);
        }
    }
    wait4all();
    rc = g_error;
    g_error = 0;
    return rc;
}

int test_mc_rwbuff_shm()
{
    int rc = 0;
    struct rwshmbuf l_rwb;
    struct rwshmbuf *p_rwb;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=64;
    __u64 actual_size=0;
    __u64 st_lba;
    __u64 nlba;
    __u64 stride = 0x100;
    pthread_t thread;
    pid_t cpid;
    mc_stat_t l_mc_stat;

    int shmid;
    key_t key=2345;
    char *shm;
    pid = getpid();

    if ((shmid = shmget(key,sizeof(struct rwshmbuf), IPC_CREAT | 0666)) < 0)
    {
        fprintf(stderr, "shmget failed\n");
        return -1;
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
    {
        fprintf(stderr, "shmat failed\n");
        return -1;
    }
    debug("%d : shared region created\n",pid);
    //lets create a child process to keep reading shared area
    cpid = fork();
    if (cpid == 0)
    {
        pid = getpid();
        if ((shmid = shmget(key,sizeof(struct rwshmbuf), IPC_CREAT | 0666)) < 0)
        {
            fprintf(stderr, "shmget failed\n");
            exit(-1);
        }

        if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
        {
            fprintf(stderr, "shmat failed\n");
            exit(-1);
        }
        debug("%d: child started accessing shared memory...\n",pid);
        while (1)
        {
            memcpy(&l_rwb, shm, sizeof(struct rwshmbuf));
        }
    }

    p_rwb = (struct rwshmbuf *)shm;

    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    /*rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
       CHECK_RC(rc, "ctx reg failed");
     */
    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx,chunks, &actual_size);
    l_mc_stat.size = actual_size;
    CHECK_RC(rc, "mc_size");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    nlba = actual_size * (1 << l_mc_stat.nmask);
    debug("%d: started IO where rwbuf in shared memory lba range(0X%"PRIX64")\n", pid, nlba-1);
    for (st_lba =0; st_lba < nlba; st_lba += stride)
    {
        rc = send_rw_shm_rcb(p_ctx, p_rwb, st_lba);
        CHECK_RC(rc, "send_rw_rcb");
    }
    debug("%d: IO is done now \n", pid);

    debug("%d: now time to kill child %d \n", pid, cpid);
    kill(cpid, SIGKILL);

    shmdt(shm);
    pthread_cancel(thread);
    //mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
    mc_term();
    return 0;
}
