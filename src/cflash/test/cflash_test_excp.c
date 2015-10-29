/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_excp.c$                           */
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
#ifdef _AIX
#include <sys/poll.h>
#else
#include <poll.h>
#include <signal.h>
#endif
#include <sys/select.h>
#include <sys/types.h>

extern int g_error;
extern char cflash_path[];
extern pid_t pid;

#define POLL_TIME_OUT -1


#ifdef _MOREDEBUG
#define MOREDEBUG_FLAG 1
#else
#define MOREDEBUG_FLAG 0
#endif

#ifdef _AIX

#define POLL_FD_INIT(pctx,myFds)                          \
struct pollfd (myFds)[2] = { { (pctx)->adap_fd, POLLPRI, 0},\
                      { (pctx)->fd, POLLPRI, 0} }
#define POLL_CALL(myFds)  (poll(&((myFds)[0]),2,POLL_TIME_OUT))


#define REVENTS_COMP(myFds, DISK_INDX, ADAP_INDX) \
        POLLPRI == myFds[DISK_INDX].revents || \
                POLLPRI == myFds[ADAP_INDX].revents
#else

#define POLL_FD_INIT(pctx,myFds)                          \
struct pollfd (myFds)[2] = { { (pctx)->fd, POLLPRI, 0} }

#define POLL_CALL(myFds)  (poll(&((myFds)[0]),1,POLL_TIME_OUT))

#define REVENTS_COMP(myFds, DISK_INDX, ADAP_INDX) \
    POLLPRI ==  myFds[DISK_INDX].revents
#endif

#define DEBUG_MORE(tell_me, ...) \
{ \
    if(MOREDEBUG_FLAG == 1)\
    printf("---------:%s:%s:%d--" tell_me \
    "---------\n",__FILE__,__FUNCTION__,__LINE__,## __VA_ARGS__); \
}


#define MAX_LENGTH        1024
#define MSG_LENGTH        512

struct exceptionPacket
{
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    struct ctx * excpCtx;
};


static int exceptionDoneFlag = 0;

void * do_poll_for_event( void * args)
{

    debug("------ call do_poll_for_event() -----------\n");

    struct exceptionPacket * excpPrt = args;
    POLL_FD_INIT(excpPrt->excpCtx,myFds);

    DEBUG_MORE("Start do_poll_for_event thread");
    while (1)
    {
        /*assumption -1 timeout for infinite poll*/
        if (POLL_CALL(myFds) < 0)
        {
            DEBUG_MORE("poll failed");
            perror("poll failed");
            exceptionDoneFlag=1;
            return NULL;
        }

        if ( REVENTS_COMP(myFds, CFLASH_DISK_POLL_INDX, CFLASH_ADAP_POLL_INDX))
        {
            pthread_mutex_lock( &excpPrt->mutex );
            exceptionDoneFlag=1;
            if (POLLPRI == myFds[CFLASH_DISK_POLL_INDX].revents)
                debug("Disk POLLPRI ....\n");
            if (POLLPRI == myFds[CFLASH_ADAP_POLL_INDX].revents)
                debug("Adap POLLPRI ....\n");
            DEBUG_MORE("lock acuired and flag set");
            pthread_cond_signal( &excpPrt->cv);
            pthread_mutex_unlock(&excpPrt->mutex);

        }
        else
        {
            debug(" ..... its not POLLPRI or might be timed out...... \n");
            debug(".....  disk.revents = %d ...  \n",myFds[CFLASH_DISK_POLL_INDX].revents);
            debug(".....  adap.revents = %d ...   \n",myFds[CFLASH_ADAP_POLL_INDX].revents);
            if ( POLL_TIME_OUT > 0 )
                debug("..... timed out in %d sec ..... \n",POLL_TIME_OUT /1000 );
            pthread_mutex_lock( &excpPrt->mutex );
            exceptionDoneFlag=1;
            g_error=-1;
            pthread_cond_signal( &excpPrt->cv);
            pthread_mutex_unlock(&excpPrt->mutex);
        }



    }
}

int test_dcqexp_ioctl(int cnum)
{

    DEBUG_MORE("inside test_dcqexp_ioctl");
    int rc=0;
#ifdef _AIX
    __u64 stride=0x10;

    struct ctx u_ctx;
    struct exceptionPacket excpVar;
    struct exceptionPacket * excpPrt =&excpVar;

    pthread_t thread_intr;
    uint64_t  verify_exception;

    struct ctx *p_ctx = &u_ctx;
    struct ctx *p_ctx_backup = &u_ctx;
    struct dk_capi_exceptions exceptions;
    char errorMsg[MSG_LENGTH];
    pthread_t thread;
    pthread_mutexattr_t mattrVar;
    pthread_condattr_t cattrVar;

    pthread_mutexattr_init(&mattrVar);
    pthread_condattr_init(&cattrVar);

    pthread_mutex_init(&excpPrt->mutex , &mattrVar);
    pthread_cond_init(&excpPrt->cv , &cattrVar);

    __u64 chunk   =0;
    __u64 nlba    =0;

    memset(p_ctx, 0, sizeof(struct ctx));
    memset(excpPrt, 0, sizeof(struct exceptionPacket));
    memset(errorMsg, 0, MSG_LENGTH+1);

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    pthread_create(&thread_intr, NULL, ctx_rrq_rx, p_ctx);

    excpPrt->excpCtx = p_ctx ;

    /* Started do_poll_for_event thread until desired exception generated*/
    pthread_create(&thread,NULL,do_poll_for_event, excpPrt);

    sleep(5); // its rare but still avoiding race condition

    switch (cnum)
    {
        case EXCP_VLUN_DISABLE: // 7.1.230

            nlba = p_ctx->last_phys_lba + 1;

            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");

            debug(" ----------- Please unmap disk from the host now -------\n");

            *p_ctx_backup=*p_ctx;

            debug(" ------ Let the I/O start and then do UA stuff at texan--------\n");

            do
            {
                rc = do_io(p_ctx, stride);
                if (rc !=0)
                {
                    debug("rc=%d,IO failed..... bye from loop\n",rc);
                    break;
                }
                else
                {
                    debug("rc=%d,IO succeeded \n",rc);
                }
                *p_ctx=*p_ctx_backup;

            }while ( rc ==0);


            g_error=0;

            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "dk_capi_verify FAILED\n");

            pthread_mutex_lock( &excpPrt->mutex );

            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            p_ctx->flags=DK_QEF_ALL_RESOURCE;


            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");
            verify_exception=DK_CE_PATH_LOST|DK_CE_VERIFY_IN_PROGRESS;
            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                debug("%d: expected : 0x%llx and recieved : 0x%llx\n", pid, verify_exception, p_ctx->exceptions);
                strcpy(errorMsg, "Fail:EXCP_VLUN_DISABLE:bad excp");
                goto xerror;
            }

            break ;

        case EXCP_PLUN_DISABLE: // 7.1.230

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            debug(" ----------- Please unmap disk from the host now -------\n");

            *p_ctx_backup=*p_ctx;

            debug(" ------ Let the I/O start and then do UA stuff at texan--------\n");

            do
            {
                rc = do_io(p_ctx, stride);
                if (rc !=0)
                {
                    debug("rc=%d,IO failed..... bye from loop\n",rc);
                    break;
                }
                else
                {
                    debug("rc=%d,IO succeeded \n",rc);
                }
                *p_ctx=*p_ctx_backup;

            }while ( rc ==0);


            g_error=0;

            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "dk_capi_verify FAILED\n");

            pthread_mutex_lock( &excpPrt->mutex );

            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");
            verify_exception=DK_CE_PATH_LOST|DK_CE_VERIFY_IN_PROGRESS;
            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_VLUN_DISABLE:bad excp");
                goto xerror;
            }

            break ;
        case EXCP_VLUN_VERIFY: // 7.1.232 //7.1.225

            chunk = 0x10;
            rc = create_resource(p_ctx, 0, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");
            nlba = chunk * (p_ctx->chunk_size);
            rc = vlun_resize(p_ctx, nlba);

            //TBD input need to check once
            // Heading for verification using ioctl
            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
            //strcpy(p_ctx->sense_data,"TBD");

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "failed : ioctl_dk_capi_verify()");
            pthread_mutex_lock( &excpPrt->mutex );

            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            // reset the flag
            exceptionDoneFlag=0;
            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            rc = ioctl_dk_capi_query_exception(p_ctx);

            CHECK_RC(rc, "dk_capi_query FAILED\n");

            verify_exception=DK_CE_VERIFY_IN_PROGRESS|DK_CE_VERIFY_SUCCEEDED;

            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_VLUN_VERIFY:bad excp");
                goto xerror;

            }


            break;

        case EXCP_PLUN_VERIFY : // 7.1.232 // 7.1.225
            rc=ioctl_dk_capi_udirect(p_ctx);
            CHECK_RC(rc, "PLUN resource failed \n");

            //TBD input need to check once
            // Heading for verification using ioctl
            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "failed : ioctl_dk_capi_verify()");

            pthread_mutex_lock( &excpPrt->mutex );

            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            // reset the flag
            exceptionDoneFlag=0;
            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            verify_exception=DK_CE_VERIFY_IN_PROGRESS|DK_CE_VERIFY_SUCCEEDED;

            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_VLUN_VERIFY:bad excp");
                goto xerror;

            }

            break ;

        case EXCP_VLUN_INCREASE : //7.1.231
            rc = create_resource(p_ctx, 0, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");
            // Just increasing by 10 chunk
            nlba = 10 * (p_ctx->chunk_size);

            rc = vlun_resize(p_ctx, nlba);
            CHECK_RC(rc, "vlun_resize failedi\n");

            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            if ( p_ctx->exceptions != DK_CE_SIZE_CHANGE )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_PLUN_VERIFY:bad excp");
                goto xerror;
            }


            break;

        case EXCP_VLUN_REDUCE : //7.1.233
            //  taking all the vlun
            nlba = p_ctx->last_phys_lba + 1;

            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");

            debug("---------- Waiting at poll().. Please decrease Disk size in texan box -----\n");
            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;

            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            if ( p_ctx->exceptions != DK_CE_VLUN_TRUNCATED)
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_PLUN_VERIFY:bad excp");
                goto xerror;
            }

            break;

        case  EXCP_VLUN_UATTENTION : // going to manual 7.1.234

            nlba = p_ctx->last_phys_lba + 1;

            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);

            CHECK_RC(rc, "Create resource failed \n");
            p_ctx_backup=p_ctx;


            debug(" ------ Let the I/O start and then do UA stuff at texan--------\n");

            do
            {
                rc = do_io(p_ctx, stride);
                if (rc !=0)
                {
                    debug("rc=%d,IO failed..... bye from loop\n",rc);
                    break;
                }
                else
                {
                    debug("rc=%d,IO succeeded \n",rc);
                }
                p_ctx=p_ctx_backup;

            }while ( rc ==0);

            g_error=0;
            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
            rc = ioctl_dk_capi_verify(p_ctx);
            debug("rc = %d , g_error =%d\n",rc,g_error);
            CHECK_RC(rc, "dk_capi_verify FAILED\n");

            debug(" -------- I am waiting at poll() for POLLPRI ---------- \n");

            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }
            pthread_mutex_unlock(&excpPrt->mutex);


            p_ctx->flags=DK_QEF_ALL_RESOURCE;

            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            if ( p_ctx->exceptions != (DK_CE_UA_RECEIVED|DK_CE_VERIFY_IN_PROGRESS|DK_CE_VERIFY_SUCCEEDED|DK_CE_SIZE_CHANGE) )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_VLUN_ATTENTION:bad excp");
                goto xerror;
            }

            break;

        case EXCP_PLUN_UATTENTION :

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");
            p_ctx_backup=p_ctx;


            debug(" ------ Let the I/O start and then do UA stuff at texan--------\n");

            do
            {
                rc = do_io(p_ctx, stride);
                if (rc !=0)
                {
                    debug("rc=%d,IO failed..... bye from loop\n",rc);
                    break;
                }
                else
                {
                    debug("rc=%d,IO succeeded \n",rc);
                }
                p_ctx=p_ctx_backup;

            }while ( rc ==0);

            g_error=0;
            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = DK_HINT_SENSE;
            rc = ioctl_dk_capi_verify(p_ctx);
            debug("rc = %d , g_error =%d\n",rc,g_error);
            CHECK_RC(rc, "dk_capi_verify FAILED\n");

            debug(" -------- I am waiting at poll() for POLLPRI ---------- \n");


            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }
            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;

            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");
            if ( p_ctx->exceptions != (DK_CE_UA_RECEIVED|DK_CE_VERIFY_IN_PROGRESS|DK_CE_VERIFY_SUCCEEDED|DK_CE_SIZE_CHANGE) )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_PLUN_UATTENTION:bad excp");
                goto xerror;
            }

            break;

        case EXCP_EEH_SIMULATION : // 7.1.229

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "Create resource failed \n");

            rc = do_eeh(p_ctx);
            CHECK_RC(rc, "do_eeh() failed");

            p_ctx->flags = DK_VF_HC_TUR;

            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "failed : ioctl_dk_capi_verify()");


            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            pthread_mutex_unlock(&excpPrt->mutex);


            p_ctx->flags=DK_QEF_ADAPTER;
            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            verify_exception=DK_CE_ADAPTER_EXCEPTION|DK_CE_VERIFY_IN_PROGRESS | DK_CE_VERIFY_SUCCEEDED ;
            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                strcpy(errorMsg, "Fail:EXCP_EEH_SIMULATION:bad excp");
                goto xerror;
            }

            // EEH code is still not tested

            if ( p_ctx->adap_except_count != 0 )
            {
                rc=255; // Non zero rc value
                strcpy(errorMsg, "Fail:EXCP_EEH_SIMULATION:bad excp");
                goto xerror;
            }

            if ( p_ctx->adap_except_type != DK_AET_EEH_EVENT|DK_AET_BAD_PF|DK_AET_AFU_ERROR )
            {
                rc=255;
                strcpy(errorMsg, "Fail:EXCP_EEH_SIMULATION:bad excp");
                goto xerror;
            }


            break;
        case EXCP_DISK_INCREASE : //7.1.226

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            debug("---------- Please increase Disk size in texan box -----\n");
            debug("---------- You have 15 secs to do that -----\n");
            sleep(15);
            debug("---------- Sleep over. Moving on... -----\n");

            p_ctx->flags = DK_VF_HC_TUR;
            p_ctx->hint = 0;
            rc = ioctl_dk_capi_verify(p_ctx);
            CHECK_RC(rc, "dk_capi_verify FAILED\n");

            pthread_mutex_lock( &excpPrt->mutex );
            while ( exceptionDoneFlag!=1)
            {
                pthread_cond_wait(&excpPrt->cv,&excpPrt->mutex);
            }

            pthread_mutex_unlock(&excpPrt->mutex);

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            rc = ioctl_dk_capi_query_exception(p_ctx);
            CHECK_RC(rc, "dk_capi_query FAILED\n");

            verify_exception=DK_CE_VERIFY_IN_PROGRESS|DK_CE_VERIFY_SUCCEEDED|DK_CE_SIZE_CHANGE;
            if ( p_ctx->exceptions != verify_exception )
            {
                rc=255; /* Non zero rc value */
                debug("%d: expected : 0x%llx and recieved : 0x%llx\n", pid, verify_exception, p_ctx->exceptions);
                strcpy(errorMsg, "Fail:EXCP_DISK_INCREASE:bad excp");
                goto xerror;
            }

            break;

        default:
            rc = -1;
            break;
    }

xerror:

    pthread_mutexattr_destroy(&mattrVar);
    pthread_condattr_destroy(&cattrVar);
    pthread_cancel(thread);
    pthread_cancel(thread_intr);
    close_res(p_ctx);
    ctx_close(p_ctx);
    CHECK_RC(rc, errorMsg);
#endif
    return rc;

}

int test_dcqexp_invalid(int cnum)
{
    int imFailed=0;
#ifdef _AIX
    int rc=0;
    struct ctx u_ctx;
    struct ctx *p_ctx = &u_ctx;
    char errorMsg[MSG_LENGTH];
    struct dk_capi_exceptions exceptions;

    __u64 chunk = 10;
    __u64 nlba =  chunk * NUM_BLOCKS;

    memset(p_ctx, 0, sizeof(struct ctx));
    memset(errorMsg, 0, MSG_LENGTH+1);

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    switch (cnum)
    {
        case EXCP_INVAL_DEVNO :

            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");

            p_ctx->flags=DK_QEF_ALL_RESOURCE;

            exceptions.version = p_ctx->version;
            exceptions.ctx_token = p_ctx->ctx_hndl;
            exceptions.rsrc_handle = p_ctx->res_hndl;
            exceptions.flags =p_ctx->flags;

            exceptions.devno=0x0000FFFF ;  // invalid dev no
            rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_EXCEPTIONS, &exceptions);

            if (rc == 0 )
            {
                imFailed = 1 ;
                strcpy(errorMsg, "Fail:EXCP_INVAL_DEVNO ");
                goto xerror ;
            }

            break;

        case EXCP_INVAL_CTXTKN :

            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "Create resource failed \n");

            p_ctx->flags=DK_QEF_ALL_RESOURCE;
            exceptions.version = p_ctx->version;
            exceptions.ctx_token = 0x0000FFFF; // invalid context no
            exceptions.rsrc_handle = p_ctx->res_hndl;
            exceptions.flags =p_ctx->flags;

            exceptions.devno=p_ctx->devno ;
            rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_EXCEPTIONS, &exceptions);

            if (rc == 0 )
            {
                imFailed = 1 ;
                strcpy(errorMsg, "Fail:EXCP_INVAL_CTXTKN ");
                goto xerror ;
            }


            break;

        case EXCP_INVAL_RSCHNDL :

            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "Create resource failed \n");

            exceptions.version = p_ctx->version;
            exceptions.ctx_token = p_ctx->ctx_hndl;
            exceptions.rsrc_handle = p_ctx->res_hndl;  // this no more valid
            exceptions.flags =p_ctx->flags;
            exceptions.devno=p_ctx->devno ;
            rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_EXCEPTIONS, &exceptions);

            if (rc == 0 )
            {
                imFailed = 1 ;
                strcpy(errorMsg, "Fail:EXCP_INVAL_CTXTKN ");
                goto xerror ;
            }

            break ;

        default:
            rc = -1;
            break;
    }



xerror :
    close_res(p_ctx);
    ctx_close(p_ctx);
    CHECK_RC(imFailed , errorMsg);
#endif
    return imFailed;

}
