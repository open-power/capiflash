/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_error2.c $                        */
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

#include <pthread.h>
#include "cflash_test.h"
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#define KB 1024

extern int manEEHonoff;
extern int quickEEHonoff;

// do_io() will use this pid.
extern pid_t pid;
extern int g_error;
extern sigjmp_buf sBuf;

// Test Case Starts here .......... !!
void cleanup(struct ctx *p_ctx, pthread_t threadId)
{
    debug("\n\n%d:**************** Start cleanup ****************\n",pid);
    // Useful for some -ve tests. NOOPs if thId is passed as -1.
    if ( -1 != threadId ) pthread_cancel(threadId);
    close_res(p_ctx);
    ctx_close(p_ctx);
    debug("%d:****************** End cleanup ******************\n",pid);
}

// 7.1.185 : EEH while super-pipe IO(VIRTUAL)(root user)
int test_vSpio_eehRecovery(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t threadId, ioThreadId, thread2;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    __u64 chunk;
    __u64 nlba;
    __u64 stride= 0x1000;
    __u64 last_lba;

    // pid used to create unique data patterns & logging from util !
    pid = getpid();
#ifndef _AIX

    if (turnOffTestCase("PVM") &&  cmd == 1  && manEEHonoff == 0)
    {
        TESTCASE_SKIP("Test case not supported in PowerVM env");
        return 0;
    }

    if ( quickEEHonoff == 1 && cmd == 1 )
    {
        TESTCASE_SKIP(" Test case will be skipped as user requested LIMTED EEH run");
        return 0;
    }

#endif
    //ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
    CHECK_RC(rc, "pthread_create failed");

    chunk = (p_ctx->last_phys_lba+1)/p_ctx->chunk_size;
    nlba = chunk * p_ctx->chunk_size;
    //create vlun
    rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    // We wish to do IO in a different thread... Setting up for that !
    p_ioThreadData->p_ctx=p_ctx;
    p_ioThreadData->stride=stride;
    p_ioThreadData->loopCount=1000;
    rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
    CHECK_RC(rc, "do_io_thread() pthread_create failed");

    //Trigger EEH
    rc = do_eeh(p_ctx);
    CHECK_RC(rc, "do_eeh() failed");

    // Wait for IO thread to complete
    pthread_join(ioThreadId, NULL);
#ifndef _AIX
    pthread_cancel(threadId);
#endif

    // Heading for context recovery using ioctl !
    //p_ctx->flags = DK_CAPI_REATTACHED;
    rc = ioctl_dk_capi_recover_ctx(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_recover_ctx failed");

#ifdef _AIX
    if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
    if ( !(DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET & p_ctx->return_flags ))
#endif
        CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

    rc = ctx_reinit(p_ctx);
    CHECK_RC(rc, "ctx_reinit() failed");
#ifndef _AIX
    pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);
#endif

    rc = do_io(p_ctx, stride);
    //SW356037: First IO will not get UA for FlashGT
    if ( is_UA_device( p_ctx->dev ) == TRUE )
    {
         if ( rc == 2 ) rc=0;
         else CHECK_RC(1, "1st IO attempt didn't fail");
    }
    else
    {
         CHECK_RC(rc, "do_io() failed");
         p_ctx->dummy_sense_flag = 1;
    }
#ifdef _AIX
    last_lba = p_ctx->last_phys_lba;
#else
    last_lba = p_ctx->last_lba;
#endif

#ifdef _AIX
    p_ctx->flags = DK_VF_HC_TUR;
    p_ctx->hint = DK_HINT_SENSE;
#else
    p_ctx->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif

    rc = ioctl_dk_capi_verify(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");

#ifdef _AIX
    if ( 0 != p_ctx->return_flags )
        CHECK_RC(1, "ioctl_dk_capi_verify flag verification failed");
#endif
    if ( p_ctx->verify_last_lba != last_lba )
        CHECK_RC(1, "ioctl_dk_capi_verify last_lba verification failed");


    // After adapter reset,
    //  AFU interrupt monitoring thread need to be restarted.
    //rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
    //CHECK_RC(rc, "pthread_create failed");

    // Re-start the io using new context
    if (2 == cmd)
    {
        //its for long run
#ifndef _AIX
        pthread_cancel(thread2);
#endif
        rc = keep_doing_eeh_test(p_ctx);
    }
    else
    {
        //its for one attempt, sanity check of eeh
        debug("%d:Try once more IO & expecting to pass this time..\n",pid);
        rc = do_io(p_ctx, stride);
        CHECK_RC(rc, "do_io() failed");
    }

#ifndef _AIX
    pthread_cancel(thread2);
#endif

    cleanup(p_ctx, threadId);

    return rc;
}

// 7.1.186 : EEH while super-pipe IO(DIRECT)(root user)
int test_dSpio_eehRecovery(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t threadId, ioThreadId, thread2;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    __u64 last_lba;
    __u64 stride= 0x1000;

    // pid used to create unique data patterns & logging from util !
    pid = getpid();

#ifndef _AIX
    if (turnOffTestCase("PVM") &&  cmd == 1  && manEEHonoff == 0)
    {
        TESTCASE_SKIP("Test case not supported in PowerVM env");
        return 0;
    }

    if ( quickEEHonoff == 1 && cmd == 1 )
    {
        TESTCASE_SKIP(" Test case will be skipped as user requested LIMTED EEH run");
        return 0;
    }

#endif

    // ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    // thread to handle AFU interrupt & events
    rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
    CHECK_RC(rc, "pthread_create failed");

    // for PLUN 2nd argument(lba_size) would be ignored
    rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    // We wish to do IO in a different thread... Setting up for that !
    p_ioThreadData->p_ctx=p_ctx;
    p_ioThreadData->stride=stride;
    p_ioThreadData->loopCount=1000;
    rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
    CHECK_RC(rc, "do_io_thread() pthread_create failed");

    //Trigger EEH
    rc = do_eeh(p_ctx);
    CHECK_RC(rc, "do_eeh() failed");

    // Wait for IO thread to complete
    pthread_join(ioThreadId, NULL);
#ifndef _AIX
    pthread_cancel(threadId);
#endif

    // Heading for context recovery using ioctl !
    //p_ctx->flags = DK_CAPI_REATTACHED;
    rc = ioctl_dk_capi_recover_ctx(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_recover_ctx failed");

#ifdef _AIX
    if ( DK_RF_REATTACHED != p_ctx->return_flags )
#else
    if ( !(DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET & p_ctx->return_flags) )
#endif
        CHECK_RC(1, "ioctl_dk_capi_recover_ctx flag verification failed");

    rc = ctx_reinit(p_ctx);
    CHECK_RC(rc, "ctx_reinit() failed");
#ifndef _AIX
    pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);
#endif

    rc = do_io(p_ctx, stride);
    //SW356037: First IO will not get UA for FlashGT
    if ( is_UA_device( p_ctx->dev ) == TRUE )
    {
         if ( rc == 2 ) rc=0;
         else CHECK_RC(1, "1st IO attempt didn't fail");
    }
    else
    {
          CHECK_RC(rc, "do_io() failed");
          p_ctx->dummy_sense_flag = 1;
    }
#ifdef _AIX
    last_lba = p_ctx->last_phys_lba;
#else
    last_lba = p_ctx->last_lba;
#endif

#ifdef _AIX
    p_ctx->flags = DK_VF_HC_TUR;
    p_ctx->hint = DK_HINT_SENSE;
#else
    p_ctx->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif

    rc = ioctl_dk_capi_verify(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");

#ifdef _AIX
    if ( 0 != p_ctx->return_flags )
        CHECK_RC(1, "ioctl_dk_capi_verify flag verification failed");
#endif

    if ( p_ctx->verify_last_lba != last_lba )
        CHECK_RC(1, "ioctl_dk_capi_verify last_lba verification failed");

    // Re-start the io using new context
    if (2 == cmd)
    {
        //its for long run
#ifndef _AIX
        pthread_cancel(thread2);
#endif
        rc = keep_doing_eeh_test(p_ctx);
    }
    else
    {
        //its for one attempt, sanity check of eeh
        stride = 0x1000;
        rc = do_io(p_ctx, stride);
        CHECK_RC(rc, "do_io() failed");
    }
#ifndef _AIX
    pthread_cancel(thread2);
#endif
    cleanup(p_ctx, threadId);

    return rc;
}

// 7.1.194 : Test DK_CAPI_QUERY_PATH & DK_CAPI_ATTACH ioctl for FCP disk
int test_ioctl_fcp()
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;

    char fc_dev[MC_PATHLEN];
    dev64_t fc_devno;

    pid = getpid();

    // Get the FC disk from env.
    rc = get_nonflash_disk(&fc_dev[0], &fc_devno);
    CHECK_RC(rc, "get_nonflash_disk() failed to get FC disk");

    // Open fc disk
    p_ctx->fd = open(fc_dev, O_RDWR);
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", fc_dev, errno);
        return -1;
    }

#ifdef _AIX

    // query path ioctl isn't supported on linux
    // Call query path ioctl
    rc = ioctl_dk_capi_query_path(p_ctx);
    if ( 0 == rc )
        CHECK_RC(1, "ioctl_dk_capi_query_path didn't fail");

    // Verify return values after ioctl
    // TBD: return_path_count need to be added to ctx.
    if ( p_ctx->return_path_count != 0 ||
         p_ctx->return_flags != DK_RF_IOCTL_FAILED )
        CHECK_RC(1, "returned_path_count/return_flags is incorrect..");

#endif /*_AIX */

    // Prepare for attach ioctl
    p_ctx->flags = DK_AF_ASSIGN_AFU;
    p_ctx->work.num_interrupts = cflash_interrupt_number(); // use num_interrupts from AFU desc
#ifdef _AIX
    p_ctx->devno = fc_devno;
#endif /*_AIX */

    // Clear the previous RF before subsequent ioctl
    p_ctx->return_flags=0;

    strcpy(p_ctx->dev, fc_dev);

    //do context attach
    rc = ioctl_dk_capi_attach(p_ctx);
    if ( 0 == rc )
        CHECK_RC(1, "ioctl_dk_capi_attach didn't fail");

    // Verify return values after ioctl
    if ( p_ctx->return_flags != DK_RF_IOCTL_FAILED )
        CHECK_RC(1, "return_flags is incorrect..");

    // If we reach here, we return success.
    return 0;
}

// 7.1.199 : Try to map & access MMIO space beyond the assigned 64KB.
int test_mmio_errcase(int cnum )
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    void * p_mmapAddr;
    struct sigaction pgHandle;

    pid = getpid();

    // ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    // for PLUN 2nd argument(lba_size) would be ignored
    rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    // -----  signal handler code ------

    if (sigsetjmp(sBuf, 0) == 1)
    {
        goto xerror;
    }
    pgHandle.sa_handler = handleSignal;
    sigemptyset(&pgHandle.sa_mask);
    pgHandle.sa_flags = 0;

    if ( sigaction(SIGSEGV, &pgHandle, NULL) < 0)
    {
        CHECK_RC(1, "sigaction() failed");
    }

    //  -------- Negative test : Start ---------------------------
    switch (cnum)
    {
            // We will attempt IO beyond mmio_size and it should fail
        case TEST_MMIO_ERRCASE1 :
            p_mmapAddr = memset((void *)(p_ctx->p_host_map),
                                1, 2048*(p_ctx->mmio_size));
            if ( p_mmapAddr == (void *)(p_ctx->p_host_map))
                CHECK_RC(1, "memset did not fail");
            break;
        case TEST_MMIO_ERRCASE2 :
            // mmap() beyond mmio_size should fail....
            p_mmapAddr = mmap((void *)p_ctx->p_host_map,
                              (p_ctx->mmio_size)*2048,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              p_ctx->adap_fd, 0);
            if ( MAP_FAILED != p_mmapAddr )
                CHECK_RC(1, "mmap() did not fail");
            break;
            // attempting IO just beyond mmio_size - only 10KB extra
            // it should fail
        case TEST_MMIO_ERRCASE3 :
            // defect #SW311014 for reference
            p_mmapAddr = memset((void *)(p_ctx->p_host_map),
                                1, ((p_ctx->mmio_size)+65*KB));
            if ( p_mmapAddr == (void *)(p_ctx->p_host_map))
                CHECK_RC(1, "memset did not fail");
            break;
    }

    //  -------- Negative test : End ---------------------------

    // Just a quick sanity check for mmap() success case
    //  -------- Sanity: Start ---------------------------

#ifndef _AIX

    // IO attempt directly.
    memset((void *)(p_ctx->p_host_map), 1, p_ctx->mmio_size);

    rc = munmap( (void *)p_ctx->p_host_map, p_ctx->mmio_size );
    CHECK_RC(rc, "munmap() failed");
#endif
    //  -------- Sanity: End ---------------------------
    cleanup(p_ctx, -1);

xerror:
    // Test Passed if we return from here !
    return g_error;
}

// Signal handler for test_spio_killprocess
void callme(int sig_num)
{
    printf("Got Signal : %d from child.. Proceeding now..\n",sig_num);
}

// 7.1.200 : Send signal to kill process when it has cmds queued.
int test_spio_killprocess()
{
    int rc;
    int isFailed=0;
    int i, nTimes;
    pid_t cpid;
    int cstat;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    struct sigaction action;
    sigset_t sigset;
    pthread_t threadId;
    __u64 chunk = 0x10;
    __u64 nlba;
    __u64 stride=0x1000;

    pid = getpid();

    sigemptyset(&sigset);
    sigprocmask(SIG_SETMASK, &sigset, NULL);

    // Set up the signal handler
    action.sa_handler = callme;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGUSR1, &action, NULL) < 0)
        CHECK_RC(1, "sigaction() failed");

    char *str = getenv("LONG_RUN");
    if (str == NULL) nTimes=10;
    else nTimes=100;

    for (i=0; i<nTimes; i++)
    {
        rc = fork();
        if ( rc == -1 ) CHECK_RC(1, "fork() failed");

        // child process
        if ( rc == 0 )
        {
            debug("...... Child process: Iteration : %d .....\n",i);
            // pid used to create unique data patterns & logging from util !
            pid = getpid();

            //ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            //thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");

            // Test with vluns one after another
            if ( i < nTimes/5 )
            {
                nlba = chunk * p_ctx->chunk_size;
                //create vlun
                rc = create_resource(p_ctx,nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }
            // Test with pluns one after another
            else if ( i > nTimes/5 && i < 2*nTimes/5 )
            {
                // Create PLUN
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                stride=0x10000;
            }
            // Test with vluns/pluns alternately !
            else if ( i % 2 )
            {
                nlba = chunk * p_ctx->chunk_size;
                //create vlun
                rc = create_resource(p_ctx,nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
                stride=0x1000;
            }
            else
            {
                // Create PLUN
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                stride=0x10000;
            }

            rc = do_io(p_ctx, stride);

            // Signal parent to kill itself after this point.
            kill(getppid(), SIGUSR1);

            // Exit at this point if we failed in initial IO
            CHECK_RC_EXIT(rc, "Initial IO failed");

            // Keep driving IO till killed
            for (;;) do_io(p_ctx, stride);
        }
        // parent process
        else
        {
            pid = getpid();

            cpid = rc;
            // Wait for child to complete at-least 1 successful IO.
            pause();

            // Let the child IO go on some more time !
            sleep(1);

            // Send signal 9 - process can't ignore it;
            kill(cpid, 9);

            // Probe child's exit status.
            if ( wait(&cstat) == -1 )
                CHECK_RC(1, "Failed while wait() for child");

            // We don't expect child to exit itself
            if (WIFEXITED(cstat)) isFailed = 1;
            else if (WIFSIGNALED(cstat))
            {
                // We expect this !
                debug("%d :  killed by %d signal\n", cpid, WTERMSIG(cstat));
                if (WCOREDUMP(cstat))
                    fprintf(stderr, "%d :  was core dupmed ...\n", cpid);
            }

            debug("pid %d exited with rc = %d\n", cpid, cstat);
        }
    }

    return isFailed;
}

// 7.1.201 : Queue up commands, do not wait for completion and
// exit w/o detach/close,
// and do detach/close while commands in queue
int test_spio_exit()
{
    int rc;
    int isFailed=0;
    int i, nTimes;
    pid_t cpid;
    int cstat;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t threadId, ioThreadId;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    __u64 chunk = 0x10;
    __u64 nlba;
    __u64 stride= 0x10000;

    for (i=0, nTimes=50; i<nTimes; i++)
    {
        rc = fork();
        if ( rc == -1 ) CHECK_RC(1, "fork() failed");

        // child process
        if ( rc == 0 )
        {
            signal(SIGABRT, sig_handle);
            signal(SIGSEGV, sig_handle);
            // pid used to create unique data patterns & logging from util !
            pid = getpid();
            debug("...... Child process: Iteration : %d .....\n",i);
            //ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            //thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");

            // Test with vluns one after another
            if ( i < nTimes/5 )
            {
                nlba = chunk * p_ctx->chunk_size;
                //create vlun
                rc = create_resource(p_ctx,nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }
            // Test with pluns one after another
            else if ( i > nTimes/5 && i < 2*nTimes/5 )
            {
                // Create PLUN
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                stride = 0x10000;
            }
            // Test with vluns/pluns alternately !
            else if ( i % 2 )
            {
                nlba = chunk * p_ctx->chunk_size;
                //create vlun
                rc = create_resource(p_ctx,nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }
            else
            {
                // Create PLUN
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
                stride = 0x10000;
            }

            // Make sure at-least 1 IO is successful before proceeding !
            rc = do_io(p_ctx, stride);
            CHECK_RC_EXIT(rc, "Initial IO attempt failed");

            // We wish to do IO in a different thread... Setting up for that !
            p_ioThreadData->p_ctx=p_ctx;
            p_ioThreadData->stride=stride;
            p_ioThreadData->loopCount=100;
            rc = pthread_create(&ioThreadId,NULL,
                                do_io_thread, (void *)p_ioThreadData);
            CHECK_RC_EXIT(rc, "do_io_thread() pthread_create failed");

            // Sleep for a sec before exiting
            sleep(1);

            if ( i % 2 )
            {
                debug("%d:Exiting w/o detach/close",pid);
            }
            else
            {
                debug("%d:Exiting after detach/close",pid);
                cleanup(p_ctx, threadId);
            }

            exit(10);
        }
        // parent process
        else
        {
            pid = getpid();

            cpid = rc;

            // Probe child's exit status.
            if ( wait(&cstat) == -1 )
                CHECK_RC(1, "Failed while wait() for child");

            // We expect child to exit itself
            if (WIFEXITED(cstat))
            {
                debug("Exiting w/o getting killed %d \n",cpid);
                // We expect child to exit with rc 10 only !
                if ( WEXITSTATUS(cstat) != 10 ) isFailed=1;
            }
            else if (WIFSIGNALED(cstat))
            {
                //isFailed=1;
                debug("%d :  killed by %d signal\n", cpid, WTERMSIG(cstat));
                if (WCOREDUMP(cstat)) //expected if exiting without cancelling poll thread
                    fprintf(stderr, "%d :  was core dupmed ...\n", cpid);
            }

            debug("pid %d exited with rc = %d\n", cpid, cstat);
        }
    }

    return isFailed;
}

// 7.1.202 : Try to send a ctx_id in some ioctl before attach
// (i.e. when no ctx is established). & some more scenarios
int test_ioctl_spio_errcase()
{
    int rc;
    int itr, type;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    char *fvt_dev;
    pthread_t threadId;
    __u64 nlba; // stride; //stride not used

    pid = getpid();

    memset((void *)p_ctx, 0, sizeof(struct ctx));

    // Case1
    // ---------- IOCTL error cases w/o context : Start ----------
    fvt_dev = getenv("FVT_DEV");
    if (NULL == fvt_dev)
    {
        fprintf(stderr, "FVT_DEV ENV var NOT set, Please set...\n");
        return -1;
    }

    strcpy(p_ctx->dev, fvt_dev);

    //open CAPI Flash disk device
    debug("Going to open CAPI Flash disk \n");
    p_ctx->fd = open_dev(fvt_dev, O_RDWR);
    if (p_ctx->fd < 0) CHECK_RC(1, "capi device open() failed");

#ifdef _AIX
    // Get the devno. : only needed for AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "query path ioctl failed");

#endif

#ifdef _AIX
    p_ctx->work.num_interrupts = 5; // use num_interrupts from AFU desc
#else
    p_ctx->work.num_interrupts = cflash_interrupt_number(); // use num_interrupts from AFU desc
#endif /*_AIX*/

    p_ctx->context_id = 0x1;

    rc = ioctl_dk_capi_detach(p_ctx);
    if ( 22 != rc ) CHECK_RC(1, "context detach ioctl did not fail");

    p_ctx->flags = DK_UDF_ASSIGN_PATH;

    rc = ioctl_dk_capi_udirect(p_ctx);
    if ( 22 != rc ) CHECK_RC(1, "pLun creation did not fail");

    g_error=0; // reset-ing the g_error
    close(p_ctx->fd);

    debug("Done. Close the fd \n");
    // ---------- IOCTL error cases w/o context : End ----------

    // Test for both pLun & vLun !
    for (itr=0; itr<2; itr++)
    {
        debug("\n\n\n%d:-------- Start Itr# %d -------\n", pid, itr);
        // Test Case2 & Case3
        for (type=0; type<2; type++)
        {
            debug("\n%d:-------- Start test type# %d -------\n", pid, type);
            // ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            // thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");

            if ( 0 == itr )
            {
                // create plun
                rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
                CHECK_RC(rc, "create LUN_DIRECT failed");
            }
            else
            {
                nlba = p_ctx->chunk_size;
                // create vlun
                rc = create_resource(p_ctx,nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
                CHECK_RC(rc, "create LUN_VIRTUAL failed");
            }

            if ( 0 == type )
            {
                // -------- Case2: IO error cases after context detach -------
                pthread_cancel(threadId);
                rc = ioctl_dk_capi_detach(p_ctx);
                CHECK_RC(rc, "detach ioctl failed");
            }
            else
            {
                // -------- Case3: IO error cases after fd close -------
                pthread_cancel(threadId);
                rc = close(p_ctx->fd);
                CHECK_RC(rc, "close(fd) failed");
                debug("%d: close(p_ctx->fd) i.e. close(%d): done !\n", pid, p_ctx->fd);
            }

            // Reset rc if we reach this point.
            rc = 0;
            g_error=0;
            debug("rc %d, g_error =%d, errno =%d\n" , rc , g_error, errno );
            //open CAPI Flash disk device again for clean using ioctls
            p_ctx->fd = open_dev(fvt_dev, O_RDWR);
            debug("After disk reopned -- rc %d, g_error =%d, errno =%d\n" , rc , g_error, errno );
            if (p_ctx->fd < 0) CHECK_RC(1, "capi device open() failed");
            debug("%d: disk re-opened, new fd: %d\n", pid, p_ctx->fd);
            cleanup(p_ctx, -1);
        }
    }

    return 0;
}

// 7.1.217 : create two context for same flash disks shared between 2 adapters
int test_cfdisk_ctxs_diff_devno()
{
    int nDisk;
    int rc=0;
    struct flash_disk cfDisk[2];
    struct ctx myctx1, myctx2;
    struct ctx *p_ctx1 = &myctx1;
    struct ctx *p_ctx2 = &myctx2;

    pid = getpid();

    nDisk = get_flash_disks(cfDisk, FDISKS_DIFF_ADPTR);
    if (nDisk < 2)
    {
        fprintf(stderr,"Failed to find 2 flash disks from diff adapter..\n");
        return -1;
    }
    // On AIX both dev will have same name
    // On Linux both dev will have diff name

    rc = ctx_init2(p_ctx1, cfDisk[0].dev, DK_AF_ASSIGN_AFU, cfDisk[0].devno[0]);
    CHECK_RC(rc, "p_ctx1 Context init failed");

    rc = ctx_init2(p_ctx2, cfDisk[1].dev, DK_AF_ASSIGN_AFU, cfDisk[1].devno[0]);
    CHECK_RC(rc, "p_ctx2 Context init failed");

    rc = create_resource(p_ctx1, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT for p_ctx1 failed");

    rc = create_resource(p_ctx2, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT for p_ctx2 failed");

    cleanup(p_ctx1, -1);
    cleanup(p_ctx2, -1);

    return 0;
}

// 7.1.218 : Pass context token to different process & do REUSE
int test_attach_reuse_diff_proc()
{
    int rc=0;
    int nDisk;
    struct flash_disk cfDisk[2];
    int cstat;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;

    pid = getpid();

    nDisk = get_flash_disks(cfDisk, FDISKS_SAME_ADPTR);
    if (nDisk < 2)
    {
        fprintf(stderr,"Failed to find 2 flash disks from same adapter..\n");
        TESTCASE_SKIP("Need disk from same adapter");
        return 0;
    }

    // sanity check for AIX!
    //#ifdef _AIX
    //  if ( cfDisk[0].devno != cfDisk[1].devno ) return 1;
    //#endif

    rc = ctx_init2(p_ctx, cfDisk[0].dev, DK_AF_ASSIGN_AFU, cfDisk[0].devno[0]);
    CHECK_RC(rc, "p_ctx Context init failed");

    rc = fork();
    if ( rc == -1 ) CHECK_RC(1, "fork() failed");

    // child process
    if ( rc == 0 )
    {
        pid = getpid();

#ifdef _AIX
        rc = ctx_init2(p_ctx, cfDisk[1].dev,
                       DK_AF_REUSE_CTX, cfDisk[0].devno[0]);
        if ( 0 == rc )
            CHECK_RC_EXIT(1, "Context init with DK_AF_REUSE_CTX did not fail");
#else
        rc = ctx_init2(p_ctx, cfDisk[1].dev,
                       DK_CXLFLASH_ATTACH_REUSE_CONTEXT, cfDisk[0].devno[0]);
        if ( 0 == rc )
            CHECK_RC_EXIT(1, "Context init with DK_CXLFLASH_ATTACH_REUSE_CONTEXT did not fail");
#endif

        exit(0);
    }
    else
    {
        // Probe child's exit status.
        if ( wait(&cstat) == -1 )
            CHECK_RC(1, "Failed while wait() for child");

        // We expect child to exit itself
        if (WIFEXITED(cstat))
        {
            // We expect child to exit with rc 0 only !
            if ( WEXITSTATUS(cstat) != 0 ) rc=1;
            else rc=0;
        }
    }

    cleanup(p_ctx, -1);

    return rc;
}

// 7.1.219 : Pass context token to different process & do detach/release.
int test_detach_diff_proc()
{
    int rc=0;
    int cstat;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;

    pid = getpid();

    //ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    rc = fork();
    if ( rc == -1 ) CHECK_RC(1, "fork() failed");

    // child process
    if ( rc == 0 )
    {
        pid = getpid();

        rc = ctx_close(p_ctx);
        if ( 22 != rc )
            CHECK_RC_EXIT(1, "Context detach did not fail");

        exit(0);
    }
    else
    {
        // Probe child's exit status.
        if ( wait(&cstat) == -1 )
            CHECK_RC(1, "Failed while wait() for child");

        // We expect child to exit itself
        if (WIFEXITED(cstat))
        {
            // We expect child to exit with rc 0 only !
            if ( WEXITSTATUS(cstat) != 0 ) rc=1;
            else rc=0;
        }
    }

    rc |= ctx_close(p_ctx);

    return rc;
}
