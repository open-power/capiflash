/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_util.c $                          */
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
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <poll.h>
#include <sys/time.h>
#ifdef _AIX
#include <sys/devinfo.h>
#endif

int MAX_RES_HANDLE=16;  /*default 16 MAX_RHT_CONTEXT */

char cflash_path[MC_PATHLEN];

#ifndef _AIX
int  manEEHonoff = 0; // 0 means manual EEH will turned off ; 1 means manual EEH will be on
#define CFLASH_HOST_TYPE_FILE  "/proc/cpuinfo"
#define CFLASH_HOST_BUF_SIZE   256

#endif

sigjmp_buf sBuf;
bool afu_reset = false;
bool bad_address = false;
bool err_afu_intrpt = false;
int dont_displa_err_msg = 0; //0 means display error msg
pid_t pid;
pid_t ppid;
int long_run = 0;
__u8 rrq_c_null = 0; //if 1, then hrrq_current = NULL

#define DATA_SEED 0xdead000000000000ull
#define MAX_TRY_CTX_RESET 0xF

__u64 lun_id = 0x0;
__u32 fc_port = 0x1; //will find the actual one

int g_error=0;
int g_errno=0;
uint8_t rc_flags;
bool long_run_enable=false;

#ifdef _AIX
// Using this flag to determine reserve_policy need to set or not
int  irPFlag=0 ;
#endif

int static rw_cmd_loaded=0;
int static force_dump = 0;

// LIMITED_EEH_TC - user need to set this env var to 
// try out limited EEH during cflash FVT automatic bucket run
// right now, 5 EEH per hour is priority for tests 
// if LIMITED_EEH_TC var is set then only 5 EEH test will be executed 
// with quick regression run
#ifndef _AIX

int quickEEHonoff = 0;

#endif

//bool static cmd_cleared=false;
//set afu device path from env
int get_fvt_dev_env()
{
    char *fvt_dev = getenv("FVT_DEV");
    char *LONG_RN = getenv("LONG_RUN");
    char *quickEEHP = getenv("LIMITED_EEH_TC");

    if (NULL == fvt_dev)
    {
        fprintf(stderr, "FVT_DEV ENV var NOT set, Please set...\n");
        return -1;
    }

#ifndef _AIX

    int rc;
    char * manualEehP   = getenv("MANUAL_EEH");
    rc=system("/opt/ibm/capikv/bin/cxlfstatus | grep -q superpipe");
    CHECK_RC(rc, "Test Setup doesn't have any Flash disk in spio mode");

    // looking for user set value
    if ( NULL != manualEehP )
    {
       manEEHonoff = atoi(manualEehP);
    }

    if ( NULL != quickEEHP )
    {
       quickEEHonoff = atoi(quickEEHP);
    }

#endif

    strcpy(cflash_path, fvt_dev);
    if ( NULL == LONG_RN )
    {
        long_run=2;
    }
    else
    {
        long_run_enable=true;
        long_run=atoi(LONG_RN);
    }
    return 0;
}

/* Return the interrupt number based on the Host type */

int cflash_interrupt_number(void )
{

   int interrupt_number ;

#ifdef _AIX

   interrupt_number = CFLSH_NUM_INTERRUPTS ;

#else

   if( host_type == CFLASH_HOST_PHYP )
    {
       /* copied from Jim's methods :-
         * For linux pHyp the number of interrupts specified
         * by user space should not include the page fault
         * (PSL) handler interrupt). Thus we must substract 1.
         * For AIX or BML, the define CFLSH_NUM_INTERRUPTS
         * is the correct number of interrupts to use.
         */

       interrupt_number = CFLSH_NUM_INTERRUPTS -1;
    }
    else
    {
       interrupt_number = CFLSH_NUM_INTERRUPTS ;
    }

#endif
     return interrupt_number;
}

#ifndef _AIX

void setupFVT_ENV( void )
{ 
     identifyHostType();

     if( host_type == CFLASH_HOST_PHYP )
     {    
         setenv("FVT_ENV", "PVM", 1);
     }
      
    return ;
}

void identifyHostType( void )
{
    FILE *fp;
    char buf[CFLASH_HOST_BUF_SIZE];
    char *platform_line;

   /* Cflash FVT: we are not asking FVT suite to fail/abort based on the result of the function */
    host_type = CFLASH_HOST_NV;   

    if ((fp = fopen(CFLASH_HOST_TYPE_FILE,"r")) == NULL) 
    {
        debug("%d: Can not determine host type, failed to open = %s\n", pid,CFLASH_HOST_TYPE_FILE );
        return;
    }

    while (fgets(buf,CFLASH_HOST_BUF_SIZE,fp)) 
    {
        platform_line = strstr(buf,"platform");

        if ((platform_line) &&
             (strstr(platform_line,":")) ) 
        {
            debug("%d: Platform_line = %s\n", pid, platform_line);
         
            if (strstr(platform_line,"PowerNV")) 
            {
	        debug("%d: BML host type\n", pid);
                host_type = CFLASH_HOST_NV;

                break;
            } 
            else if (strstr(platform_line,"pSeries (emulated by qemu)")) 
            { 
	        debug("%d: PowerKVM host type\n", pid);
                host_type = CFLASH_HOST_KVM;

            } 
            else if (strstr(platform_line,"pSeries")) 
            {
	        debug("%d: pHyp host type\n", pid);
                host_type = CFLASH_HOST_PHYP;

            }
            else 
            {
	        debug("%d: Platform_string = %s\n", pid, platform_line);
            }

            break;
        }
    }

    if ( host_type == CFLASH_HOST_UNKNOWN ) 
    {
        debug("%d: Cant determine the platform type", pid);
    }

    fclose (fp);

    return;
}

#endif


int ctx_init_thread(void *args)
{
    return ctx_init((ctx_p)args);
}

void ctx_close_thread(void *args)
{
    ctx_close((ctx_p)args);
}

int ctx_reinit(struct ctx *p_ctx)
{
    int i;

#ifndef _AIX

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

#endif


    debug("%d:------------- Start ctx_reinit() -------------\n", pid);
    p_ctx->context_id = p_ctx->new_ctx_token;
    p_ctx->ctx_hndl = CTX_HNDLR_MASK & p_ctx->context_id;

#ifndef _AIX

    /* Re-init all mutex to avoid any pending unlock after recovery */

    for (i = 0; i < NUM_CMDS; i++)
    {
        pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
        pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
    }

#endif

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++)
    {
        p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }

    debug("%d: context_id=0X%"PRIX64" ctx_hndl=%d\n",
          pid, p_ctx->context_id, p_ctx->ctx_hndl);

    bzero(p_ctx->p_hrrq_start, NUM_RRQ_ENTRY*sizeof(__u64));
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
    write_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
    write_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);
#ifdef _AIX
    write_64(&p_ctx->p_host_map->endian_ctrl,(__u64)SISL_ENDIAN_CTRL_BE);
#endif
    write_64(&p_ctx->p_host_map->ctx_ctrl, SISL_MSI_SYNC_ERROR);
    write_64(&p_ctx->p_host_map->intr_mask, SISL_ISTATUS_MASK);

    p_ctx->toggle = 1;

    afu_reset = false;

#ifndef _AIX

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);

#endif

    debug("%d:------------- End ctx_reinit() -------------\n", pid);
    return 0;
}

int ctx_init_internal(struct ctx *p_ctx,
                       __u64 flags, dev64_t devno)
{
    int rc=0;
    //void *map;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    int i;
    pid_t mypid;


    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

    for (i = 0; i < NUM_CMDS; i++)
    {
        pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
        pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
    }

    p_ctx->flags = flags;

    p_ctx->work.num_interrupts = cflash_interrupt_number(); // use num_interrupts from AFU desc
    p_ctx->devno = devno;

    //do context attach
    rc = ioctl_dk_capi_attach(p_ctx);
    if (rc)
    {
        fprintf(stderr, "ioctl_dk_capi_attach failed\n");
        g_error = -1;
        return -1;
    }

    /* AFU is running with SQ mode */
    if ( p_ctx->sq_mode_flag == TRUE )
    {
         p_ctx->p_sq_start = &p_ctx->sq_entry[0];
         p_ctx->p_sq_end   = &p_ctx->sq_entry[NUM_RRQ_ENTRY];
         p_ctx->p_sq_curr  = p_ctx->p_sq_start;
    }
    // initialize RRQ pointers
    p_ctx->p_hrrq_start = &p_ctx->rrq_entry[0];
    p_ctx->p_hrrq_end = &p_ctx->rrq_entry[NUM_RRQ_ENTRY - 1];
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;

    p_ctx->toggle = 1;

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++)
    {
        p_ctx->cmd[i].rcb.msi = SISL_MSI_RRQ_UPDATED;
        p_ctx->cmd[i].rcb.rrq = 0x0;
        p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }
#ifdef _AIX
    write_64(&p_ctx->p_host_map->endian_ctrl,(__u64)SISL_ENDIAN_CTRL_BE);
#endif

    // set up RRQ in AFU
    if (rrq_c_null)
    {
        write_64(&p_ctx->p_host_map->rrq_start, (__u64)NULL);
        write_64(&p_ctx->p_host_map->rrq_end, (__u64)NULL);
    }
    else
    {

        write_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
        write_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);

        if ( p_ctx->sq_mode_flag == TRUE )
        {
            write_64(&p_ctx->p_host_map->sq_start, (__u64) p_ctx->p_sq_start );
            write_64(&p_ctx->p_host_map->sq_end, (__u64) p_ctx->p_sq_end );
        }

    }

    mypid = getpid();
    debug("%d: ctx_init() success: p_host_map %p, ctx_hndl %d, rrq_start %p\n",
          mypid, p_ctx->p_host_map, p_ctx->ctx_hndl, p_ctx->p_hrrq_start);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
    return 0;
}
int open_dev(char *dev, int reqflg)
{
    int fd;
    int flag;
    //open CAPI Flash disk device
#ifdef _AIX
    int ext_flag = SC_CAPI_USER_IO;
    //ext_flag |= SC_NO_RESERVE; //check later this flag
    flag = (reqflg & O_ACCMODE) | O_NONBLOCK;
    fd = openx(dev,flag,0,ext_flag);
#else
    flag = reqflg | O_NONBLOCK;
    fd = open(dev,flag);
#endif
    return fd;
}
int ctx_init(struct ctx *p_ctx)
{
#ifdef _AIX
    int rc = 0;
    char str[MC_PATHLEN];
    char diskBuf[MC_PATHLEN];

    if(irPFlag == 1)
    {
      sprintf(str, "chdev -l %s -a reserve_policy=no_reserve",
                  diskWithoutDev(cflash_path,diskBuf));
      rc = system(str);
      CHECK_RC(rc, "reserve_policy changed failed");
      irPFlag = 0;
    }

#endif /*_AIX */
    memset(p_ctx, 0, sizeof(struct ctx));
    if ((p_ctx->fd = open_dev(cflash_path, O_RDWR)) < 0)
    {
        fprintf(stderr,"open failed %s, errno %d\n",cflash_path, errno);
        g_error = -1;
        return -1;
    }
    strcpy(p_ctx->dev, cflash_path);
#ifdef _AIX
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc, "query path failed")
#endif /*_AIX */
        //note: for Linux devno will be ignored
        return ctx_init_internal(p_ctx, DK_AF_ASSIGN_AFU, p_ctx->devno);
}

int ctx_init2(struct ctx *p_ctx, char *dev,
               __u64 flags, dev64_t devno)
{
    memset(p_ctx, 0, sizeof(struct ctx));
    if ((p_ctx->fd = open_dev(dev, O_RDWR)) < 0)
    {
        fprintf(stderr,"open failed %s, errno %d\n",dev, errno);
        g_error = -1;
        return -1;
    }
    strcpy(p_ctx->dev, dev);
    return ctx_init_internal(p_ctx, flags, devno);
}

int ctx_close(struct ctx *p_ctx)
{
    int rc = 0;
#ifndef _AIX
    if (p_ctx->mmio_size != 0)
    {
        rc = munmap((void *)p_ctx->p_host_map, p_ctx->mmio_size);
        if (rc)
            fprintf(stderr, "munmap failed with errno = %d", errno);
    }
#endif
    rc |= ioctl_dk_capi_detach(p_ctx);
    rc |= close(p_ctx->fd);
    debug("%d: close(%d) done.\n", pid, p_ctx->fd);
    p_ctx->mmio_size = 0;
    p_ctx->p_host_map = 0;
    return rc;
}

void clear_waiting_cmds(struct ctx *p_ctx)
{
    int i;
    struct afu_cmd *p_cmd;
    if (!rw_cmd_loaded) return; //rw cmd not loaded
    if (DEBUG)
    {
        fprintf(stderr,
                "%d: Clearing all waiting cmds, some error occurred\n",
                getpid());
    }
    //TBD do context reset
    for (i = 0 ; i < NUM_CMDS; i++)
    {
        p_cmd = &p_ctx->cmd[i];
        pthread_mutex_lock(&p_cmd->mutex);
        p_cmd->sa.host_use_b[0] |= B_DONE;
        p_ctx->cmd[i].sa.ioasc = 1;
        p_ctx->cmd[i].sa.rc.afu_rc =0xFF;
        pthread_cond_signal(&p_cmd->cv);
        pthread_mutex_unlock(&p_cmd->mutex);
    }
    //cmd_cleared = true;
}
void ctx_rrq_intr(struct ctx *p_ctx)
{
    struct afu_cmd *p_cmd;
    // process however many RRQ entries that are read
    debug_2("%d: In ctx_rrq_intr():  toggle_bit= 0X%llx p_ctx->toggle=%x\n",
            pid, (*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT), p_ctx->toggle);

    while ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
               p_ctx->toggle)
    {

        debug_2("------------------------------------------------\n");
        debug_2("%d: Got inside loop within ctx_rrq_intr()\n", pid);
        debug_2("%d: address in p_hrrq_curr=0X%"PRIX64"\n", pid, *p_ctx->p_hrrq_curr);
        debug_2("%d: toggle_bit = 0X%llx p_ctx->toggle=0X%x\n",
                pid, (*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT), p_ctx->toggle);

        p_cmd = (struct afu_cmd*)
                ((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));
        debug_2("%d:cmd_address(IOARCB)=%p\n",pid,p_cmd);

        pthread_mutex_lock(&p_cmd->mutex);
        p_cmd->sa.host_use_b[0] |= B_DONE;
        pthread_cond_signal(&p_cmd->cv);
        pthread_mutex_unlock(&p_cmd->mutex);

        if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end)
        {
            p_ctx->p_hrrq_curr++; /* advance to next RRQ entry */
        }
        else
        {
            /* wrap HRRQ  & flip toggle */
            p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
            p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
            debug_2("%d:wrapping up p_hrrq_curr to p_hrrq_start...\n",pid);
        }
    }
}

void ctx_sync_intr(struct ctx *p_ctx)
{
    __u64 reg;
    __u64 reg_unmasked;

    reg = read_64(&p_ctx->p_host_map->intr_status);
    reg_unmasked = (reg & SISL_ISTATUS_UNMASK);

    if (reg_unmasked == 0)
    {
        if (!dont_displa_err_msg)
        {
            fprintf(stderr,
                    "%d: spurious interrupt, intr_status 0x%016lx, ctx %d\n",
                    pid, reg, p_ctx->ctx_hndl);
        }
        clear_waiting_cmds(p_ctx);
        return;
    }

    if (reg_unmasked == SISL_ISTATUS_PERM_ERR_RCB_READ)
    {
        if (!dont_displa_err_msg)
        {
            fprintf(stderr, "exiting on SISL_ISTATUS_PERM_ERR_RCB_READ\n");
        }
        clear_waiting_cmds(p_ctx);
        //pthread_exit(NULL);
        // ok - this is a signal to stop this thread
    }
    else
    {
        if (!dont_displa_err_msg)
        {
            fprintf(stderr,
                    "%d: unexpected interrupt, intr_status 0x%016lx, ctx %d, exiting test...\n",
                    pid, reg, p_ctx->ctx_hndl);
        }
        clear_waiting_cmds(p_ctx);
        return ;
    }

    write_64(&p_ctx->p_host_map->intr_clear, reg_unmasked);

    return;
}

#ifdef _AIX
void *ctx_rrq_rx(void *arg)
{
    int rc = 0;
    struct ctx *p_ctx = (struct ctx*) arg;
    struct pollfd poll_list[2];
    memset(&poll_list[0], 0, sizeof(poll_list));

    poll_list[CFLASH_ADAP_POLL_INDX].fd = p_ctx->adap_fd;
    poll_list[CFLASH_ADAP_POLL_INDX].events = POLLIN|POLLSYNC|POLLPRI;

    poll_list[CFLASH_DISK_POLL_INDX].fd = p_ctx->fd;
    poll_list[CFLASH_DISK_POLL_INDX].events = POLLIN;
    int max_try_ctx_reset =0;
    while (1)
    {
        rc = poll(poll_list, 2, -1);//no timeout
        debug_2("%d:rc=%d poll_list[CFLASH_ADAP_POLL_INDX].revents=%d\n",
                pid,rc,poll_list[CFLASH_ADAP_POLL_INDX].revents);
        if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLPRI)
        {
            if (afu_reset)
                continue; //no need for any action now
            //bad page fault
            if (bad_address)
                err_afu_intrpt=true; //set to check process
            else
                fprintf(stderr, "%d: adapt poll POLLPRI occured\n",pid);
            p_ctx->flags = DK_QEF_ADAPTER;
            ioctl_dk_capi_query_exception(p_ctx);
            if (DK_AET_EEH_EVENT == p_ctx->adap_except_type)
            {
                afu_reset = true;
                printf("%d: EEH is done.. receieved DK_AET_EEH_EVENT event......\n",pid);
                max_try_ctx_reset=0;
                continue;
            }
            else if (DK_AET_BAD_PF == p_ctx->adap_except_type)
            {
                debug("%d: DK_AET_BAD_PF adap_except received.....\n",pid);
            }
            //Take action based on execption TBD
            if (++max_try_ctx_reset == MAX_TRY_CTX_RESET)
            {
                clear_waiting_cmds(p_ctx);
                max_try_ctx_reset = 0;
            }
            //return NULL;
        }
        if (poll_list[CFLASH_DISK_POLL_INDX].revents & POLLPRI)
        {
            //disk exception
            fprintf(stderr, "%d: disk poll POLLPRI occured\n",pid);
            p_ctx->flags = DK_QEF_ALL_RESOURCE;
            ioctl_dk_capi_query_exception(p_ctx);
            //TBD dump exception detail or forward above
                        //Take action based on execption TBD
            if (++max_try_ctx_reset == MAX_TRY_CTX_RESET)
            {
                clear_waiting_cmds(p_ctx);
                max_try_ctx_reset = 0;
            }
            //return NULL;
        }
        if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLMSG)
        {
            //adapter error interrupt
            //this is SISL_MSI_SYNC_ERROR
            fprintf(stderr, "%d: adopt poll POLLMSG SISL_MSI_SYNC_ERROR occured\n",pid);
            ctx_sync_intr(p_ctx);
            //return NULL;
        }
        if (poll_list[CFLASH_ADAP_POLL_INDX].revents & POLLIN)
        {
            max_try_ctx_reset =0;//reset
            //adapter interrupt for cmd completion
            //this is SISL_MSI_RRQ_UPDATED
            debug_2("%d:adapter POLLIN, SISL_MSI_RRQ_UPDATED\n",pid);
            ctx_rrq_intr(p_ctx);
            debug_2("%d:Returned from ctx_rrq_intr()\n",pid);
        }
    }
    return NULL;
}

#else
void *ctx_rrq_rx(void *arg)
{
    struct cxl_event *p_event;
    int len;
    struct ctx   *p_ctx = (struct ctx*) arg;

    while (1)
    {
        //
        // read adapt fd & block on any interrupt
        len = read(p_ctx->adap_fd, &p_ctx->event_buf[0],
                   sizeof(p_ctx->event_buf));

        if ((len < 0) && (errno == EIO))
        {
            if (!dont_displa_err_msg)
                fprintf(stderr, "afu has been reset ...\n");

            clear_waiting_cmds(p_ctx);
            afu_reset = true;
                        //sleep some time to retry again
            sleep(10);
            continue;
        }

        p_event = (struct cxl_event *)&p_ctx->event_buf[0];
        while (len >= sizeof(p_event->header))
        {
            if (p_event->header.type == CXL_EVENT_AFU_INTERRUPT)
            {
                switch (p_event->irq.irq)
                {
                    case SISL_MSI_RRQ_UPDATED:
                        ctx_rrq_intr(p_ctx);
                        break;

                    case SISL_MSI_SYNC_ERROR:
                        ctx_sync_intr(p_ctx);
                        break;

                    default:
                        if (!dont_displa_err_msg)
                        {
                            fprintf(stderr, "%d: unexpected irq %d, ctx %d, exiting test...\n",
                                    pid, p_event->irq.irq, p_ctx->ctx_hndl);
                        }
                        clear_waiting_cmds(p_ctx);
                        //return NULL;
                        break;
                }

            }
            else
            {
                switch (p_event->header.type)
                {
                    case CXL_EVENT_RESERVED:
                        if (!dont_displa_err_msg)
                            fprintf(stderr, "%d: CXL_EVENT_RESERVED = size =  0x%x\n",
                                    pid,p_event->header.size);
                        break;
                    case CXL_EVENT_DATA_STORAGE:
                        if (!dont_displa_err_msg)
                            fprintf(stderr, "%d:CAPI_EVENT_DATA_STOARAGE addr = 0x%lx, dsisr = 0x%lx\n",
                                    pid,p_event->fault.addr,p_event->fault.dsisr);

                        //TBD get_intr_status & print
                        break;
                    case CXL_EVENT_AFU_ERROR:
                        if (!dont_displa_err_msg)
                            fprintf(stderr,"%d:CXL_EVENT_AFU_ERROR error = 0x%lx, flags = 0x%x\n",
                                    pid, p_event->afu_error.error,p_event->afu_error.flags);
                        break;
                    default:
                        if (!dont_displa_err_msg)
                        {
                            fprintf(stderr, "%d: Unknown CAPI EVENT type = %d, process_element = 0x%x\n",
                                    pid, p_event->header.type,p_event->header.process_element);
                        }
                }
                g_error =1;
                clear_waiting_cmds(p_ctx);
                err_afu_intrpt = true;
                //return NULL;
                //let user process terminate thread
            }
            len -= p_event->header.size;
            p_event = (struct cxl_event *)
                (((char*)p_event) + p_event->header.size);
        }
    }

    return NULL;
}
#endif

// len in __u64
void fill_buf(__u64* p_buf, unsigned int len,__u64 value)
{
    static __u64 data = DATA_SEED;
    int i;

    for (i = 0; i < len; i += 2)
    {
        p_buf[i] = value;
        p_buf[i + 1] = data++;
    }
}

// Send Report LUNs SCSI Cmd to CAPI Adapter
int send_report_luns(struct ctx *p_ctx, __u32 port_sel,
                      __u64 **lun_ids, __u32 *nluns)
{
    __u32 *p_u32;
    __u64 *p_u64, *lun_id;
    int len;
    int rc;

    memset(&p_ctx->rbuf[0], 0, sizeof(p_ctx->rbuf));
    memset((void *)&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID |
                                       SISL_REQ_FLAGS_HOST_READ);
    p_ctx->cmd[0].rcb.port_sel  = port_sel;
    p_ctx->cmd[0].rcb.lun_id    = 0x0;
    p_ctx->cmd[0].rcb.data_len  = sizeof(p_ctx->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea   = (__u64) &p_ctx->rbuf[0][0];
    p_ctx->cmd[0].rcb.timeout   = 10;   /* 10 Secs */

    p_ctx->cmd[0].rcb.cdb[0]    = 0xA0; /* report lun */

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[6];
    write_32(p_u32, sizeof(p_ctx->rbuf[0])); /* allocation length */

    p_ctx->cmd[0].sa.host_use_b[1] = 0; /* reset retry cnt */
    do
    {
        send_single_cmd(p_ctx);
        rc = wait_single_resp(p_ctx);
        if (rc) return rc;
    } while (check_status(&p_ctx->cmd[0].sa));


    if (p_ctx->cmd[0].sa.host_use_b[0] & B_ERROR)
    {
        return -1;
    }

    // report luns success
    len = read_32((__u32*)&p_ctx->rbuf[0][0]);
    p_u64 = (__u64*)&p_ctx->rbuf[0][8]; /* start of lun list */

    *nluns = len/8;
    lun_id = (__u64 *)malloc((*nluns * sizeof(__u64)));

    if (lun_id == NULL)
    {
        fprintf(stderr, "Report LUNs: ENOMEM\n");
    }
    else
    {
        *lun_ids = lun_id;

        while (len)
        {
            *lun_id = read_64(p_u64++);
            lun_id++;
            len -= 8;
        }
    }

    return 0;
}
// Send Read Capacity SCSI Cmd to the LUN
int send_read_capacity(struct ctx *p_ctx, __u32 port_sel,
                        __u64 lun_id, __u64 *lun_capacity, __u64 *blk_len)
{
    __u32 *p_u32;
    __u64 *p_u64;
    int rc;

    memset(&p_ctx->rbuf[0], 0, sizeof(p_ctx->rbuf));
    memset((void *)&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.req_flags = (SISL_REQ_FLAGS_PORT_LUN_ID |
                                       SISL_REQ_FLAGS_HOST_READ);
    p_ctx->cmd[0].rcb.port_sel  = port_sel;
    p_ctx->cmd[0].rcb.lun_id    = lun_id;
    p_ctx->cmd[0].rcb.data_len  = sizeof(p_ctx->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea   = (__u64) &p_ctx->rbuf[0];
    p_ctx->cmd[0].rcb.timeout   = 10;   /* 10 Secs */

    p_ctx->cmd[0].rcb.cdb[0]    = 0x9E; /* read cap(16) */
    p_ctx->cmd[0].rcb.cdb[1]    = 0x10; /* service action */

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
    write_32(p_u32, sizeof(p_ctx->rbuf[0])); /* allocation length */

    send_single_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    if (rc)
    {
        return rc;
    }

    p_u64 = (__u64*)&p_ctx->rbuf[0][0];
    *lun_capacity = read_64(p_u64);

    p_u32   = (__u32*)&p_ctx->rbuf[0][8];
    *blk_len = read_32(p_u32);

    return 0;
}

// len in __u64
int cmp_buf(__u64* p_buf1, __u64 *p_buf2, unsigned int len)
{
    return memcmp(p_buf1, p_buf2, len*sizeof(__u64));
}

int rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba)
{
    int i;
    //char buf[32];
    //int read_fd, write_fd;
    for (i = 0; i < NUM_CMDS; i++)
    {
        if (cmp_buf((__u64*)&p_ctx->rbuf[i][0], (__u64*)&p_ctx->wbuf[i][0],
                    sizeof(p_ctx->rbuf[i])/sizeof(__u64)))
        {
            printf("%d: miscompare at start_lba 0X%"PRIX64"\n",
                   pid, start_lba);

            hexdump(&p_ctx->rbuf[i][0],0x20,"Read buf");
            hexdump(&p_ctx->wbuf[i][0],0x20,"Write buf");

            return -1;
        }
    }
    return 0;
}

int check_if_can_retry(struct ctx *p_ctx, int old_rc)
{
    int i;
    int rc;
    bool yes_try=false;
    for (i=0;i<NUM_CMDS;i++)
    {
        //will try once more for scsi_rc values
        if (p_ctx->cmd[i].sa.ioasc && p_ctx->cmd[i].sa.rc.scsi_rc)
        {
            yes_try=true;
            break;
        }
    }
    if (!yes_try) return old_rc;
    debug("%d: Trying once more to send IO......\n", pid);
    for (i=0;i<NUM_CMDS;i++)
    {
        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
    }
    rc = send_cmd(p_ctx);
    if (rc) return rc;
    return wait_resp(p_ctx);
}
int send_write(struct ctx *p_ctx, __u64 start_lba,
                __u64 stride,__u64 data)
{
    int i;
    //__u64 *p_u64; //unused
    __u32 *p_u32;
    __u64 lba;
    int rc;

    for (i = 0; i< NUM_CMDS; i++)
    {
        fill_buf((__u64*)&p_ctx->wbuf[i][0],
                 sizeof(p_ctx->wbuf[i])/sizeof(__u64),data);

        memset((void *)&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
        lba = start_lba + i*stride;
        write_lba((__u64*)&p_ctx->cmd[i].rcb.cdb[2],lba);

        p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;

        debug_2("%d : send write lba..: 0X%"PRIX64"\n",(int)data,lba);

        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[i][0];

        p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[i]);
        p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)
        p_ctx->cmd[i].rcb.timeout = 60; //secs

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
        write_32(p_u32, p_ctx->blk_len);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
 
        if ( p_ctx->sq_mode_flag == TRUE )
        {
           p_ctx->cmd[i].rcb.sq_ioasa_ea = (uint64_t)&(p_ctx->cmd[i].sa);
        }
        hexdump(&p_ctx->wbuf[i][0],0x20,"Writing");
    }
    //send_single_cmd(p_ctx);
    rc = send_cmd(p_ctx);
    if (rc) return rc;
    rc = wait_resp(p_ctx);
    return rc;
}

void send_single_cmd(struct ctx *p_ctx)
{

    p_ctx->cmd[0].sa.host_use_b[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    /* make memory updates visible to AFU before MMIO */
    asm volatile( "lwsync" : : );

    // write IOARRIN
    write_64(&p_ctx->p_host_map->ioarrin, (__u64)&p_ctx->cmd[0].rcb);
    rw_cmd_loaded=1;
}

int send_cmd(struct ctx *p_ctx)
{
    int cnt = NUM_CMDS;
    int wait_try=MAX_TRY_WAIT;
    int p_cmd = 0;
    int i;
    __u64 room;

    /* make memory updates visible to AFU before MMIO */
    asm volatile( "lwsync" : : );
    while (cnt)
    {

        if ( p_ctx->sq_mode_flag == TRUE )
        {
           debug_2("%d:Placing IOARCB =0X%"PRIX64" into SQ \n",
                   pid,(__u64)&p_ctx->cmd[p_cmd].rcb);

           hexdump((void *)(&p_ctx->cmd[p_cmd].rcb), sizeof(p_ctx->cmd[p_cmd].rcb),
                   "RCB data writing in SQ........");

           bcopy((void*)&p_ctx->cmd[p_cmd++].rcb,(void*)p_ctx->p_sq_curr,
                  sizeof(sisl_ioarcb_t));

           p_ctx->p_sq_curr++;

           if ( p_ctx->p_sq_curr > p_ctx->p_sq_end )
           {
                p_ctx->p_sq_curr = p_ctx->p_sq_start;
           }

           write_64(&p_ctx->p_host_map->sq_tail, (__u64)p_ctx->p_sq_curr);

           cnt--;
        }
        else 
        {
           
            room = read_64(&p_ctx->p_host_map->cmd_room);
            debug_2("%d:room =0X%"PRIX64"\n",pid,room);
#ifdef _AIX //not to break anything on Linux
#ifndef __64BIT__
		if (0XFFFFFFFF == room)
#else
                if ( -1 == room)
#endif
                {
                   fprintf(stderr,"%d:Failed:cmd_room=-1 afu_reset done/not recovered...\n",pid);
                   usleep(1000);
                   return -1;
                }
#endif
                if (0 == room)
                {
                   usleep(MC_BLOCK_DELAY_ROOM);
                   wait_try--;
                   debug("%d:still pending cmd:%d\n",pid,cnt);
                }
                if (0 == wait_try)
                {
                    fprintf(stderr, "%d : send cmd wait over %d cmd remain\n",
                    pid, cnt);
                    return -1;
                }
               for (i = 0; i < room; i++)
               {
                  // add a usleep here if room=0 ?
                  // write IOARRIN
                  debug_2("%d:Placing IOARCB =0X%"PRIX64" into ioarrin\n",
                    pid,(__u64)&p_ctx->cmd[p_cmd].rcb);
                  hexdump((void *)(&p_ctx->cmd[p_cmd].rcb), sizeof(p_ctx->cmd[p_cmd].rcb),
                    "RCB data writing in ioarrin........");
                  write_64(&p_ctx->p_host_map->ioarrin,
                     (__u64)&p_ctx->cmd[p_cmd++].rcb);
                  wait_try = MAX_TRY_WAIT; //each cmd give try max time
                  if (cnt-- == 1) break;
               }

         }
    }
    rw_cmd_loaded=1;
    return 0;
}

int wait_resp(struct ctx *p_ctx)
{
    int i;
    int rc =0;
    int p_rc = 0;
    __u64 *p_u64;

    for (i = 0; i < NUM_CMDS; i++)
    {
#ifdef _AIX
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec+=15;
        pthread_mutex_lock(&p_ctx->cmd[i].mutex);
        while (p_ctx->cmd[i].sa.host_use_b[0] != B_DONE)
        {
            rc=pthread_cond_timedwait(&p_ctx->cmd[i].cv, &p_ctx->cmd[i].mutex,&ts);
			if(rc == EINVAL){
				fprintf(stderr,"%d:timedout cmd for lba 0X%"PRIX64"\n",
				pid, p_ctx->cmd[i].rcb.cdb[2]);
				p_ctx->cmd[i].sa.host_use_b[0] |=B_DONE;
			}
        }
        pthread_mutex_unlock(&p_ctx->cmd[i].mutex);
#else
        pthread_mutex_lock(&p_ctx->cmd[i].mutex);
        while (p_ctx->cmd[i].sa.host_use_b[0] != B_DONE)
        {
            pthread_cond_wait(&p_ctx->cmd[i].cv, &p_ctx->cmd[i].mutex);
        }
        pthread_mutex_unlock(&p_ctx->cmd[i].mutex);
#endif
        //force_dump=1;
        hexdump((void *)(&p_ctx->cmd[i].rcb), sizeof(p_ctx->cmd[i].rcb), "RCB data........");
        //force_dump=0;
        if (p_ctx->cmd[i].sa.ioasc)
        {
            rc_flags = p_ctx->cmd[i].sa.rc.flags;
            rc = p_ctx->cmd[i].sa.rc.afu_rc |
                    p_ctx->cmd[i].sa.rc.scsi_rc |
                    p_ctx->cmd[i].sa.rc.fc_rc;
            if (!dont_displa_err_msg)
            {

                p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
                if (p_rc != rc)
                {
                    hexdump((void*)(&p_ctx->cmd[i].sa.sense_data),0x20,"Sense data Writing");

                    // copied sense_data for further use
                    memcpy((void *)p_ctx->verify_sense_data,(const void *)p_ctx->cmd[i].sa.sense_data,20);

                    fprintf(stderr,"%d: Req failed @lba=0X%"PRIX64" IOASC = flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n",
                            pid,read_64(p_u64),
                            p_ctx->cmd[i].sa.rc.flags,p_ctx->cmd[i].sa.rc.afu_rc,
                            p_ctx->cmd[i].sa.rc.scsi_rc, p_ctx->cmd[i].sa.rc.fc_rc);
                    p_rc = rc;
                    if (p_ctx->cmd[i].sa.rc.scsi_rc || p_ctx->cmd[i].sa.rc.fc_rc)
                    {
                        force_dump=1;
                        hexdump((void*)(&p_ctx->cmd[i].sa.sense_data),0x20,"Sense data Writing");
                        force_dump=0;
                    }
                }
            }
        }
    }
    rw_cmd_loaded=0;
    return rc;
}

int wait_single_resp(struct ctx *p_ctx)
{
    int rc =0;

    pthread_mutex_lock(&p_ctx->cmd[0].mutex);
    while (p_ctx->cmd[0].sa.host_use_b[0] != B_DONE)
    {
        debug_2("%d: Sleeping in wait_single_resp(), host_use_b[0]=%d\n", pid, p_ctx->cmd[0].sa.host_use_b[0]);
        pthread_cond_wait(&p_ctx->cmd[0].cv, &p_ctx->cmd[0].mutex);
        debug_2("%d: Wokeup in wait_single_resp(), host_use_b[0]=%d\n", pid, p_ctx->cmd[0].sa.host_use_b[0]);
    }
    pthread_mutex_unlock(&p_ctx->cmd[0].mutex);
    debug_2("%d: Got out of wait_single_resp(), host_use_b[0]=%d\n", pid, p_ctx->cmd[0].sa.host_use_b[0]);

    rw_cmd_loaded =0;
    if (p_ctx->cmd[0].sa.ioasc)
    {
        if (!dont_displa_err_msg)
        {
            if (p_ctx->cmd[0].sa.rc.scsi_rc || p_ctx->cmd[0].sa.rc.fc_rc)
            {
                force_dump=1;
                hexdump((void*)(&p_ctx->cmd[0].sa.sense_data),0x20,"Sense data Writing");
                force_dump=0;
            }
            printf("%d:IOASC = flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n",
                   pid,
                   p_ctx->cmd[0].sa.rc.flags,p_ctx->cmd[0].sa.rc.afu_rc,
                   p_ctx->cmd[0].sa.rc.scsi_rc, p_ctx->cmd[0].sa.rc.fc_rc);
        }
        rc_flags = p_ctx->cmd[0].sa.rc.flags;
        rc = p_ctx->cmd[0].sa.rc.afu_rc |
                p_ctx->cmd[0].sa.rc.scsi_rc |
                p_ctx->cmd[0].sa.rc.fc_rc;
        return rc;
    }
    return rc;
}

int send_read(struct ctx *p_ctx, __u64 start_lba,
               __u64 stride)
{
    int i;
    //__u64 *p_u64; //unused
    __u32 *p_u32;
    __u64 lba;
    int rc;

    for (i = 0; i < NUM_CMDS; i++)
    {
        memset(&p_ctx->rbuf[i][0], 0, sizeof(p_ctx->rbuf[i]));

        lba = start_lba + i*stride;

        memset((void *)&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));

        p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)

        write_lba((__u64*)&p_ctx->cmd[i].rcb.cdb[2],lba);

        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
        p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl;

        debug_2("%d: send read for lba : 0X%"PRIX64"\n",pid,lba);

        p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbuf[i]);
        p_ctx->cmd[i].rcb.timeout = 60;
        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbuf[i][0];

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];

        write_32(p_u32, p_ctx->blk_len);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;

        if ( p_ctx->sq_mode_flag == TRUE )
        {
           p_ctx->cmd[i].rcb.sq_ioasa_ea = (uint64_t)&(p_ctx->cmd[i].sa );
        }

    }

    //send_single_cmd(p_ctx);
    rc = send_cmd(p_ctx);
    CHECK_RC(rc,"send_cmd failed...\n");
    rc = wait_resp(p_ctx);
    return rc;
}

// returns 1 if the cmd should be retried, 0 otherwise
// sets B_ERROR flag based on IOASA
int check_status(volatile sisl_ioasa_t *p_ioasa)
{
    // delete urun !!!
    if (p_ioasa->ioasc == 0 ||
        (p_ioasa->rc.flags & SISL_RC_FLAGS_UNDERRUN))
    {
        return 0;
    }
    else
    {
        p_ioasa->host_use_b[0] |= B_ERROR;
    }

    if (p_ioasa->host_use_b[1]++ < 5)
    {
        if (p_ioasa->rc.afu_rc == 0x30)
        {
            // out of data buf
            // #define all, to add the 2nd case!!!
            // do we delay ?
            return 1;
        }

        if (p_ioasa->rc.scsi_rc)
        {
            // retry all SCSI errors
            // but if busy, add a delay
            return 1;
        }
    }

    return 0;
}

int test_init(struct ctx *p_ctx)
{
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
    return 0;
}

/*
 * NAME: hexdump
 *
 * FUNCTION: Display an array of type char in ASCII, and HEX. This function
 *      adds a caller definable header to the output rather than the fixed one
 *      provided by the hexdump function.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This routine is ONLY AVAILABLE IF COMPILED WITH DEBUG DEFINED
 *
 * RETURNS: NONE
 */
void
hexdump(void *data, long len, const char *hdr)
{

    int     i,j,k;
    char    str[18];
    char    *p = (char *)data;
    if (!force_dump)
    {
        //ignore env varible for force dump
        if (!ENABLE_HEXDUMP)
            return;
    }
    i=j=k=0;
    fprintf(stderr, "%s: length=%ld (PID: %d)\n", hdr?hdr:"hexdump()", len, pid);

    /* Print each 16 byte line of data */
    while (i < len)
    {
        if (!(i%16))    /* Print offset at 16 byte bndry */
            fprintf(stderr,"%03x  ",i);

        /* Get next data byte, save ascii, print hex */
        j=(int) p[i++];
        if (j>=32 && j<=126)
            str[k++] = (char) j;
        else
            str[k++] = '.';
        fprintf(stderr,"%02x ",j);

        /* Add an extra space at 8 byte bndry */
        if (!(i%8))
        {
            fprintf(stderr," ");
            str[k++] = ' ';
        }

        /* Print the ascii at 16 byte bndry */
        if (!(i%16))
        {
            str[k] = '\0';
            fprintf(stderr," %s\n",str);
            k = 0;
        }
    }

    /* If we didn't end on an even 16 byte bndry, print ascii for partial
     * line. */
    if ((j = i%16))
    {
        /* First, space over to ascii region */
        while (i%16)
        {
            /* Extra space at 8 byte bndry--but not if we
             * started there (was already inserted) */
            if (!(i%8) && j != 8)
                fprintf(stderr," ");
            fprintf(stderr,"   ");
            i++;
        }
        /* Terminate the ascii and print it */
        str[k]='\0';
        fprintf(stderr,"  %s\n",str);
    }
    fflush(stderr);

    return;
}

int rw_cmp_buf_cloned(struct ctx *p_ctx, __u64 start_lba)
{
    int i;
    for (i = 0; i < NUM_CMDS; i++)
    {
        if (cmp_buf_cloned((__u64*)&p_ctx->rbuf[i][0],
                           sizeof(p_ctx->rbuf[i])/sizeof(__u64)))
        {
            printf("%d: clone miscompare at start_lba 0X%"PRIX64"\n",
                   pid, start_lba);
            return -1;
        }
    }
    return 0;
}

// len in __u64
int cmp_buf_cloned(__u64* p_buf, unsigned int len)
{
    static __u64 data = DATA_SEED;
    int i;

    for (i = 0; i < len; i += 2)
    {
        if (!(p_buf[i] == ppid && p_buf[i + 1] == data++))
        {
            return -1;
        }
    }
    return 0;
}

int send_rw_rcb(struct ctx *p_ctx, struct rwbuf *p_rwb,
                 __u64 start_lba, __u64 stride,
                int align, int where)
{
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba;
    int rc;
    int i;
    __u32 ea;
    pid = getpid();
    if (0 == where)//begining
        ea = align;
    else //from end of the block
        ea = 0x1000 - align;
    for (i = 0; i< NUM_CMDS; i++)
    {
        debug("%d: EA = %p with 0X%X alignment\n",pid, &p_rwb->wbuf[i][ea], ea);
        fill_buf((__u64*)&p_rwb->wbuf[i][ea],
                 sizeof(p_rwb->wbuf[i])/(2*sizeof(__u64)),pid);
        memset((void *)&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));
        vlba = start_lba + i*stride;
        p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
        p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
        write_lba(p_u64, vlba);
        debug_2("%d: send write for 0X%"PRIX64"\n", pid, vlba);

        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_rwb->wbuf[i][ea];

        p_ctx->cmd[i].rcb.data_len = sizeof(p_rwb->wbuf[i])/2;
        p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
        write_32(p_u32, p_ctx->blk_len);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
        hexdump(&p_rwb->wbuf[i][ea],0x20,"Write buf");
    }

    rc = send_cmd(p_ctx);
    if (rc) return rc;
    //send_single_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    if (rc) return rc;
    //fill send read

    for (i = 0; i< NUM_CMDS; i++)
    {
        memset(&p_rwb->rbuf[i][ea], 0, sizeof(p_rwb->rbuf[i])/2);

        vlba = start_lba + i*stride;
        memset((void *)&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));

        p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
        p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
        p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl;
        write_lba(p_u64, vlba);
        debug_2("%d: send read for 0X%"PRIX64"\n", pid, vlba);

        p_ctx->cmd[i].rcb.data_len = sizeof(p_rwb->rbuf[i])/2;
        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_rwb->rbuf[i][ea];

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];

        write_32(p_u32, p_ctx->blk_len);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
    }
    rc = send_cmd(p_ctx);
    if (rc) return rc;
    //send_single_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    if (rc) return rc;
    //do cmp r/w buf
    for (i = 0; i < NUM_CMDS; i++)
    {
        vlba = start_lba + i*stride;
        if (cmp_buf((__u64*)&p_rwb->rbuf[i][ea], (__u64*)&p_rwb->wbuf[i][ea],
                    sizeof(p_ctx->rbuf[i])/(2 * sizeof(__u64))))
        {
            printf("%d: miscompare at start_lba 0X%"PRIX64"\n",
                   pid, vlba);
            force_dump=1;
            hexdump(&p_rwb->rbuf[i][ea],0x20,"Read buf");
            hexdump(&p_rwb->wbuf[i][ea],0x20,"Write buf");
            force_dump=0;
            return -1;
        }
    }
    return 0;
}

int send_rw_shm_rcb(struct ctx *p_ctx, struct rwshmbuf *p_rwb,
                     __u64 vlba)
{
    __u64 *p_u64;
    __u32 *p_u32;
    int rc;

    fill_buf((__u64*)&p_rwb->wbuf[0][0],
             sizeof(p_rwb->wbuf[0])/(sizeof(__u64)),pid);
    memset((void *)&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];
    p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
    write_lba(p_u64, vlba);
    debug_2("%d: send write for 0X%"PRIX64"\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_rwb->wbuf[0][0];

    p_ctx->cmd[0].rcb.data_len = sizeof(p_rwb->wbuf[0]);
    p_ctx->cmd[0].rcb.cdb[0] = 0x8A; // write(16)

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
    write_32(p_u32, p_ctx->blk_len);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    if (rc) return rc;

    memset(&p_rwb->rbuf[0][0], 0, sizeof(p_rwb->rbuf[0]));

    memset((void *)&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
    p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
    write_lba(p_u64, vlba);
    debug_2("%d: send read for 0X%"PRIX64"\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_len = sizeof(p_rwb->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_rwb->rbuf[0][0];

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

    write_32(p_u32, p_ctx->blk_len);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    if (rc) return rc;
    //do cmp r/w buf

    if (cmp_buf((__u64*)&p_rwb->rbuf[0][0], (__u64*)&p_rwb->wbuf[0][0],
                sizeof(p_rwb->rbuf[0])/(sizeof(__u64))))
    {
        printf("%d: miscompare at start_lba 0X%"PRIX64"\n",
               pid, vlba);
        hexdump(&p_rwb->rbuf[0][0],0x20,"Read buf");
        hexdump(&p_rwb->wbuf[0][0],0x20,"Write buf");
        return -1;
    }
    return 0;
}

int do_large_io(struct ctx *p_ctx, struct rwlargebuf *rwbuf, __u64 size)
{
    __u64 st_lba = p_ctx->st_lba;;
    int rc = 0;
    pid = getpid();
    __u32 blocks = size/(p_ctx->block_size);
    debug("%d: st_lba=0X%"PRIX64" last_lba=0X%"PRIX64" blocks=0X%"PRIX32" transfer_size=0X%"PRIX64"\n",
          pid,st_lba,p_ctx->last_lba,blocks,size);
    for (;st_lba <= p_ctx->last_lba; st_lba += (NUM_CMDS*blocks))
    {
        rc = send_rw_lsize(p_ctx, rwbuf, st_lba, blocks);
        if (rc) break;
    }
    debug("%d:-------transfer_size=0X%"PRIX64" done with rc=%d lba=0X%"PRIX64"-----\n",
          pid,size,rc,st_lba);
    return rc;
}
int send_rw_lsize(struct ctx *p_ctx, struct rwlargebuf *p_rwb,
                   __u64 start_lba, __u32 blocks)
{
    //__u64 *p_u64; //unused
    __u32 *p_u32;
    __u64 vlba;
    int rc;
    int i;
    __u32 buf_len = blocks * p_ctx->block_size;
    for (i = 0; i< NUM_CMDS; i++)
    {
        fill_buf((__u64*)p_rwb->wbuf[i],buf_len/sizeof(__u64),pid);
        debug_2("%d: write buf address=%p\n",pid,p_rwb->wbuf[i]);
        memset((void *)&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));
        vlba = start_lba + i*blocks;
        write_lba((__u64*)&p_ctx->cmd[i].rcb.cdb[2],vlba);

        debug_2("%d: send write for 0X%"PRIX64"\n", pid, vlba);
        p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;

        p_ctx->cmd[i].rcb.data_ea = (__u64)p_rwb->wbuf[i];

        p_ctx->cmd[i].rcb.data_len = buf_len;
        p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
        write_32(p_u32, blocks); //how many blocks

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
        hexdump(p_rwb->wbuf[i],0x20,"Write buf");
    }

    rc = send_cmd(p_ctx);
    if (rc) return rc;
    //send_single_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    if (rc) return rc;
    //fill send read

    for (i = 0; i< NUM_CMDS; i++)
    {
        memset(p_rwb->rbuf[i], 0, buf_len/sizeof(__u64));
        //bzero(p_rwb->rbuf[i], buf_len/sizeof(__u64));
        //fill_buf((__u64*)p_rwb->rbuf[i],buf_len/sizeof(__u64),pid);
        debug_2("%d: Read buf address=%p\n",pid,p_rwb->rbuf[i]);

        vlba = start_lba + i*blocks;
        memset((void *)&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));

        p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
        write_lba((__u64*)&p_ctx->cmd[i].rcb.cdb[2],vlba);
        debug_2("%d: send read for 0X%"PRIX64"\n", pid, vlba);

        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
        p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl;

        p_ctx->cmd[i].rcb.data_len = buf_len;
        p_ctx->cmd[i].rcb.data_ea = (__u64)p_rwb->rbuf[i];

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];

        write_32(p_u32, blocks);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
    }
    rc = send_cmd(p_ctx);
    if (rc) return rc;
    //send_single_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    if (rc) return rc;
    char buf[20];
    int rfd, wfd;
    //do cmp r/w buf
    for (i = 0; i < NUM_CMDS; i++)
    {
        vlba = start_lba + i*blocks;
        //if (cmp_buf((__u64*)p_rwb->rbuf[i], (__u64*)p_rwb->wbuf[i],
        //          buf_len/sizeof(__u64)));
        if (memcmp(p_rwb->rbuf[i], p_rwb->wbuf[i],
                   buf_len))
        {
            sprintf(buf,"read.%d",pid);
            rfd = open(buf,O_RDWR|O_CREAT, 0600);
            sprintf(buf,"write.%d",pid);
            wfd = open(buf,O_RDWR|O_CREAT, 0600);
            write(rfd,p_rwb->rbuf[i],buf_len);
            write(wfd,p_rwb->rbuf[i],buf_len);
            close(rfd);
            close(wfd);
            printf("%d: miscompare at start_lba 0X%"PRIX64"\n",
                   pid, vlba);
            force_dump=1;
            hexdump(p_rwb->rbuf[i],0x20,"Read buf");
            hexdump(p_rwb->wbuf[i],0x20,"Write buf");
            force_dump=0;
            return -1;
        }
    }
    return 0;
}

int send_single_write(struct ctx *p_ctx, __u64 vlba, __u64 data)
{
    __u64 *p_u64;
    __u32 *p_u32;
    int rc;

    fill_buf((__u64*)&p_ctx->wbuf[0][0],
             sizeof(p_ctx->wbuf[0])/(sizeof(__u64)), data);
    memset((void *)&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];
    p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
    write_lba(p_u64, vlba);
    debug_2("%d: send write for 0X%"PRIX64"\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->wbuf[0][0];

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->wbuf[0]);
    p_ctx->cmd[0].rcb.cdb[0] = 0x8A; // write(16)

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
    write_32(p_u32, p_ctx->blk_len);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    return rc;
}

int send_single_read(struct ctx *p_ctx, __u64 vlba)
{
    __u64 *p_u64;
    __u32 *p_u32;
    int rc;

    memset(&p_ctx->rbuf[0][0], 0, sizeof(p_ctx->rbuf[0]));

    memset((void *)&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
    p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
    write_lba(p_u64, vlba);
    debug_2("%d: send read for 0X%"PRIX64"\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->rbuf[0][0];

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

    write_32(p_u32, p_ctx->blk_len);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    return rc;
}

int rw_cmp_single_buf(struct ctx *p_ctx, __u64 vlba)
{
    if (cmp_buf((__u64*)&p_ctx->rbuf[0][0], (__u64*)&p_ctx->wbuf[0][0],
                sizeof(p_ctx->rbuf[0])/(sizeof(__u64))))
    {
        printf("%d: miscompare at start_lba 0X%"PRIX64"\n",
               pid, vlba);
        hexdump(&p_ctx->rbuf[0][0],0x20,"Read buf");
        hexdump(&p_ctx->wbuf[0][0],0x20,"Write buf");
        return -1;
    }
    return 0;
}

#ifdef _AIX
//IOCTL from here
int ioctl_dk_capi_query_path(struct ctx *p_ctx)
{
    int rc;
    int i;
    int first_path = 0;
    struct cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[MAX_PATH-1];
    }capi_paths;

    struct dk_capi_path_info *path_info = NULL;
    memset(&capi_paths, 0, sizeof(capi_paths));

    capi_paths.path.version = p_ctx->version;
    capi_paths.path.path_count = MAX_PATH;

    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");
    }
    if (capi_paths.path.returned_path_count == 0)
    {
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");
    }
    if (capi_paths.path.returned_path_count > MAX_PATH)
    {
        fprintf(stderr,"got more paths than provided issued=%d ret=%d\n",
                MAX_PATH,capi_paths.path.returned_path_count);
        capi_paths.path.returned_path_count = MAX_PATH;
    }

    path_info = capi_paths.path.path_info;
    //get 1st reserved path
    for (i = 0; i < capi_paths.path.returned_path_count; i++)
    {
        if ((path_info[i].flags & DK_CPIF_RESERVED) &&
            !(path_info[i].flags & DK_CPIF_FAILED))
        {
            first_path = i;
            break;
        }
    }
    p_ctx->devno = path_info[first_path].devno;
    p_ctx->path_id = path_info[first_path].path_id;
    p_ctx->path_id_mask = 1 << path_info[first_path].path_id;
    p_ctx->return_flags = capi_paths.path.return_flags;
    p_ctx->return_path_count = capi_paths.path.returned_path_count;
    debug("%d:----------- DK_CAPI_QUERY_PATHS Info -----------\n", pid);
    debug("%d:dev=%s return_path_count=%d return_flags=0X%"PRIX64"\n",
          pid, p_ctx->dev, p_ctx->return_path_count,p_ctx->return_flags);
    debug("%d:1st Path info: path_id=%d path_id_mask=%d\n",
          pid, p_ctx->path_id, p_ctx->path_id_mask);
    debug("%d:----------- End DK_CAPI_QUERY_PATHS ---------------\n",pid);
    return rc;
}
#endif /* _AIX */

int ioctl_dk_capi_attach(struct ctx *p_ctx)
{
    int rc;

#ifdef _AIX
    struct dk_capi_attach capi_attach;
    struct devinfo iocinfo;
    memset(&iocinfo, 0, sizeof(iocinfo));
#else
    struct dk_cxlflash_attach capi_attach;
#endif
    memset(&capi_attach, 0, sizeof(capi_attach));

    debug("%d:----------- Start DK_CAPI_ATTACH ----------\n", pid);
#ifdef _AIX
    capi_attach.version = p_ctx->version;
    capi_attach.flags = p_ctx->flags;
    capi_attach.devno = p_ctx->devno;
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" num_interrupts=%d devno=0X%"PRIX64" context_id=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->work.num_interrupts,p_ctx->devno,p_ctx->context_id);
#else
    capi_attach.hdr.version = p_ctx->version;
    capi_attach.hdr.flags = p_ctx->flags;
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" num_interrupts=%d context_id=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->work.num_interrupts,p_ctx->context_id);
#endif
    capi_attach.num_interrupts = p_ctx->work.num_interrupts;

#ifdef _AIX
    rc = ioctl(p_ctx->fd, DK_CAPI_ATTACH, &capi_attach);
#else
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
    debug("%d:... DK_CAPI_ATTACH called ...\n", pid);
    if (rc)
    {
        g_errno=errno;
        CHECK_RC(errno, "DK_CAPI_ATTACH failed");
    }

    p_ctx->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
    p_ctx->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
    p_ctx->context_id = capi_attach.ctx_token;
    p_ctx->last_phys_lba = capi_attach.last_phys_lba;
    p_ctx->chunk_size = capi_attach.chunk_size;
    p_ctx->devno = capi_attach.devno;
    //get max_xfer
    rc = ioctl(p_ctx->fd, IOCINFO, &iocinfo);
    if (rc)
    {
        CHECK_RC(errno, "Iocinfo failed with errno\n");
    }
    //p_ctx->max_xfer = iocinfo.un.capi_io.max_transfer;
    //TBD
    p_ctx->max_xfer = iocinfo.un.scdk64.lo_max_request;
    if (iocinfo.flags  & DF_LGDSK)
    {
        p_ctx->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
    }
    p_ctx->return_flags = capi_attach.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#else
    if (p_ctx->flags != DK_CXLFLASH_ATTACH_REUSE_CONTEXT)
    {
        // no need for REUSE flag
        p_ctx->p_host_map = mmap(NULL,p_ctx->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED,
                                 capi_attach.adap_fd,0);
        if (p_ctx->p_host_map == MAP_FAILED)
        {
            fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                    p_ctx->mmio_size, errno);
            CHECK_RC(1,"mmap failed");
        }
    }
    p_ctx->context_id = capi_attach.context_id;
    p_ctx->last_phys_lba = capi_attach.last_lba;
    p_ctx->max_xfer = capi_attach.max_xfer;
    p_ctx->chunk_size = NUM_BLOCKS;
    p_ctx->return_flags = capi_attach.hdr.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#endif

    //default rwbuff handling 4K, Large trasnfer handled exclusive
    p_ctx->blk_len = BLOCK_SIZE/p_ctx->block_size;
    p_ctx->adap_fd = capi_attach.adap_fd;

    p_ctx->ctx_hndl = CTX_HNDLR_MASK & p_ctx->context_id;
    p_ctx->unused_lba = p_ctx->last_phys_lba +1;

#ifdef _AIX
    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64" ret_devno=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer,p_ctx->devno);
#else
    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer);
#endif

#ifdef DK_CXLFLASH_APP_CLOSE_ADAP_FD
    // if context attach returns DK_CXLFLASH_APP_CLOSE_ADAP_FD flag 
    if (p_ctx->return_flags & DK_CXLFLASH_APP_CLOSE_ADAP_FD) 
    {
        p_ctx->close_adap_fd_flag = TRUE; 
    }

#endif

#ifdef DK_CXLFLASH_CONTEXT_SQ_CMD_MODE
     if ( p_ctx->return_flags & DK_CXLFLASH_CONTEXT_SQ_CMD_MODE )
     {
          p_ctx->sq_mode_flag = TRUE ;
     }
#endif

    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx->adap_fd,p_ctx->return_flags);
    debug("%d:------------- End DK_CAPI_ATTACH -------------\n", pid);

    // Surelock GA2 supports only 4K block disks
    // Can be removed later on new release.
    if ( p_ctx->block_size != 0x1000 )
    {
        printf("%d: Only 4K block size disk supported. Exiting.", pid);
        exit(1);
    }
    return rc;
}

int ioctl_dk_capi_detach(struct ctx *p_ctx)
{
    int rc;
#ifdef _AIX
    struct dk_capi_detach capi_detach;
#else
    struct dk_cxlflash_detach capi_detach;
#endif
    p_ctx->flags = 0; //not yet defined
    memset(&capi_detach, 0, sizeof(capi_detach));
    debug("%d:--------------- Start DK_CAPI_DETACH -------------\n",pid);

#ifdef _AIX
    capi_detach.version = p_ctx->version;
    capi_detach.flags = p_ctx->flags;
#else
    capi_detach.hdr.version = p_ctx->version;
    capi_detach.hdr.flags = p_ctx->flags;
#endif
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" ctx_id=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->context_id);
#ifdef _AIX
    capi_detach.ctx_token = p_ctx->context_id;
    capi_detach.devno = p_ctx->devno;
    debug("%d:devno=0X%"PRIX64"\n",pid,p_ctx->devno);
    rc = ioctl(p_ctx->fd, DK_CAPI_DETACH, &capi_detach);
#else
    capi_detach.context_id = p_ctx->context_id;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_DETACH, &capi_detach);
#endif
    debug("%d:... DK_CAPI_DETACH called ...\n",pid);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_DETACH failed");
    }
#ifdef _AIX
    p_ctx->return_flags = capi_detach.return_flags;
#else
    p_ctx->return_flags = capi_detach.hdr.return_flags;
#endif

    // if DK_CXLFLASH_APP_CLOSE_ADAP_FD flag returns during attach
    if (p_ctx->close_adap_fd_flag == TRUE)
    {
       close(p_ctx->adap_fd);
    }
    debug("%d:return_flag=0X%"PRIX64"\n",pid,p_ctx->return_flags);

    debug("%d:--------------- End DK_CAPI_DETACH -------------\n",pid);
    return rc;
}

int ioctl_dk_capi_udirect(struct ctx *p_ctx)
{
    int rc;
#ifdef _AIX
    struct dk_capi_udirect udirect;
#else
    struct dk_cxlflash_udirect udirect;
#endif
    memset(&udirect, 0, sizeof(udirect));

    debug("%d:----------- Start DK_CAPI_USER_DIRECT ----------\n", pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" ctx_id:0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->context_id);
#ifdef _AIX
    udirect.version = p_ctx->version;
    udirect.flags = p_ctx->flags;
    udirect.devno = p_ctx->devno;
    udirect.ctx_token = p_ctx->context_id;
    udirect.path_id_mask = p_ctx->path_id_mask;
    debug("%d:devno=0X%"PRIX64" path_id_mask=%d\n",pid,p_ctx->devno,p_ctx->path_id_mask);
    rc = ioctl(p_ctx->fd, DK_CAPI_USER_DIRECT, &udirect);
#else
    udirect.hdr.version = p_ctx->version;

    // TBD : flag is not defined in Linux
    //udirect.hdr.flags = p_ctx->flags;
    udirect.context_id = p_ctx->context_id;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_USER_DIRECT, &udirect);
#endif

    debug("%d:... DK_CAPI_USER_DIRECT called ...\n",pid);
    if (rc)
    {
        p_ctx->last_lba = udirect.last_lba;
        p_ctx->rsrc_handle = udirect.rsrc_handle;
#ifdef _AIX
        p_ctx->return_flags = udirect.return_flags;
#else
        p_ctx->return_flags = udirect.hdr.return_flags;
#endif

        p_ctx->res_hndl = RES_HNDLR_MASK & p_ctx->rsrc_handle;
        debug("%d:res_hndl=0X%"PRIX64" last_lba=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
              pid,p_ctx->rsrc_handle,p_ctx->last_lba,p_ctx->return_flags);

        CHECK_RC(errno, "DK_CAPI_USER_DIRECT failed");
    }

    p_ctx->last_lba = udirect.last_lba;
    p_ctx->rsrc_handle = udirect.rsrc_handle;
#ifdef _AIX
    p_ctx->return_flags = udirect.return_flags;
#else
    p_ctx->return_flags = udirect.hdr.return_flags;
#endif

    p_ctx->res_hndl = RES_HNDLR_MASK & p_ctx->rsrc_handle;
    debug("%d:res_hndl=0X%"PRIX64" last_lba=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
          pid,p_ctx->rsrc_handle,p_ctx->last_lba,p_ctx->return_flags);

    debug("%d:-------- End DK_CAPI_USER_DIRECT ----------\n",pid);
    return rc;
}

int ioctl_dk_capi_uvirtual(struct ctx *p_ctx)
{
    int rc;
#ifdef _AIX
    struct dk_capi_uvirtual uvirtual;
#else
    struct dk_cxlflash_uvirtual uvirtual;
#endif
    memset(&uvirtual, 0, sizeof(uvirtual));

    debug("%d:----------- Start DK_CAPI_USER_VIRTUAL -------------\n", pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" ctx_id:0X%"PRIX64" lun_size=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->context_id,p_ctx->lun_size);
#ifdef _AIX
    uvirtual.version = p_ctx->version;
    uvirtual.devno = p_ctx->devno;
    uvirtual.vlun_size = p_ctx->lun_size;
    uvirtual.ctx_token = p_ctx->context_id;
    uvirtual.path_id_mask = p_ctx->path_id_mask;
    uvirtual.flags = p_ctx->flags;
    debug("%d:devno=0X%"PRIX64" path_id_mask=%d\n",pid,p_ctx->devno,p_ctx->path_id_mask);
    rc = ioctl(p_ctx->fd, DK_CAPI_USER_VIRTUAL, &uvirtual);
#else
    uvirtual.hdr.version = p_ctx->version;
    //TBD enabled flag once defined
    //uvirtual.hdr.flags = p_ctx->flags;
    uvirtual.context_id = p_ctx->context_id;
    uvirtual.lun_size = p_ctx->lun_size;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_USER_VIRTUAL, &uvirtual);
#endif

    debug("%d:... DK_CAPI_USER_VIRTUAL called ...\n",pid);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_USER_VIRTUAL failed");
    }

    p_ctx->rsrc_handle = uvirtual.rsrc_handle;
    p_ctx->last_lba = uvirtual.last_lba;
#ifdef _AIX
    p_ctx->return_flags = uvirtual.return_flags;
#else
    p_ctx->return_flags = uvirtual.hdr.return_flags;
#endif

    p_ctx->res_hndl = RES_HNDLR_MASK & p_ctx->rsrc_handle;

    debug("%d:res_hndl=0X%"PRIX64" last_lba=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
          pid,p_ctx->rsrc_handle,p_ctx->last_lba,p_ctx->return_flags);

    debug("%d:--------------- End DK_CAPI_USER_VIRTUAL -------------\n",pid);

    return rc;
}

int ioctl_dk_capi_release(struct ctx *p_ctx)
{
    int rc;
#ifdef _AIX
    struct dk_capi_release release;
#else
    struct dk_cxlflash_release release;
#endif
    p_ctx->flags = 0; //not yet defined
    memset(&release, 0, sizeof(release));

    release.rsrc_handle = p_ctx->rsrc_handle;
    debug("%d:----------- Start DK_CAPI_RELEASE -------------\n",pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" ctx_id:0X%"PRIX64" rsrc_handle=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->context_id,p_ctx->rsrc_handle);
#ifdef _AIX
    release.version = p_ctx->version;
    release.flags = p_ctx->flags;
    release.devno = p_ctx->devno;
    release.ctx_token = p_ctx->context_id;
    debug("%d:devno=0X%"PRIX64"\n",pid,p_ctx->devno);
    rc = ioctl(p_ctx->fd, DK_CAPI_RELEASE, &release);
#else
    release.hdr.version = p_ctx->version;
    release.hdr.flags = p_ctx->flags;
    release.context_id = p_ctx->context_id;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_RELEASE, &release);
#endif
    debug("%d:... DK_CAPI_RELEASE called ...\n",pid);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_RELEASE failed");
    }

#ifdef _AIX
    p_ctx->return_flags = release.return_flags;
#else
    p_ctx->return_flags = release.hdr.return_flags;
#endif
    debug("%d:return_flag=0X%"PRIX64"\n",pid,p_ctx->return_flags);

    debug("%d:--------- End DK_CAPI_RELEASE -------------\n",pid);
    return rc;
}

int ioctl_dk_capi_vlun_resize(struct ctx *p_ctx)
{
    int rc;
#ifdef _AIX
    struct dk_capi_resize resize;
#else
    struct dk_cxlflash_resize resize;
#endif
    p_ctx->flags = 0; //not yet defined
    memset(&resize, 0, sizeof(resize));
    resize.rsrc_handle = p_ctx->rsrc_handle;

    debug("%d:------------- Start DK_CAPI_VLUN_RESIZE -------------\n",pid);
    debug("%d:dev=%s fd=%d flags=0X%"PRIX64" ctx_id:0X%"PRIX64" rsrc_handle=0X%"PRIX64" req_size=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->flags,p_ctx->context_id,
          p_ctx->rsrc_handle, p_ctx->req_size);
#ifdef _AIX
    resize.version = p_ctx->version;
    resize.flags = p_ctx->flags;
    resize.ctx_token = p_ctx->context_id;
    resize.devno = p_ctx->devno;
    resize.vlun_size = p_ctx->req_size;
    debug("%d:devno=0X%"PRIX64"\n",pid,p_ctx->devno);
    rc = ioctl(p_ctx->fd, DK_CAPI_VLUN_RESIZE, &resize);
#else
    resize.hdr.version = p_ctx->version;
    resize.context_id = p_ctx->context_id;
    resize.req_size = p_ctx->req_size;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_VLUN_RESIZE, &resize);
#endif
    debug("%d:... DK_CAPI_VLUN_RESIZE called ...\n",pid);
    if (rc)
    {
        debug("%d:lun_size=0X%"PRIX64" last_lba=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
              pid,p_ctx->lun_size,p_ctx->last_lba,p_ctx->return_flags);

        CHECK_RC(errno, "DK_CAPI_VLUN_RESIZE failed");
    }

#ifdef _AIX
    p_ctx->return_flags = resize.return_flags;
#else
    p_ctx->return_flags = resize.hdr.return_flags;
#endif
    p_ctx->last_lba = resize.last_lba;
    p_ctx->lun_size = resize.last_lba + 1;
    if ( p_ctx->req_size == 0 ) p_ctx->lun_size = 0;

    debug("%d:lun_size=0X%"PRIX64" last_lba=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
          pid,p_ctx->lun_size,p_ctx->last_lba,p_ctx->return_flags);

    debug("%d:--------- End DK_CAPI_RESIZE ----------\n",pid);
    return rc;
}

int ioctl_dk_capi_verify(struct ctx *p_ctx)
{
    int rc;
    g_errno=0; // Reset errno before using.
#ifdef _AIX
    struct dk_capi_verify verify;
#else
    struct dk_cxlflash_verify verify;

    /* dummy_sense_data is using Unit attention sense data;
       dummy_sense_data will be used when HINT is set and user
       dont have such valid sense_data */

    char * dummy_sense_data = "p\000\006\000\000\000\000\n\000\000\000\000)\000\000\000\000";

#endif
    memset(&verify, 0, sizeof(verify));
    verify.hint = p_ctx->hint;

    debug("%d:--------- Start DK_CAPI_VERIFY ----------\n",pid);
#ifdef _AIX
    memcpy(verify.sense_data,p_ctx->verify_sense_data,DK_VERIFY_SENSE_LEN);

    verify.version = p_ctx->version;
    verify.path_id = p_ctx->path_id;
    verify.flags = p_ctx->flags;
    rc = ioctl(p_ctx->fd, DK_CAPI_VERIFY, &verify);
#else
    // Copying sense_data
    if ( p_ctx->dummy_sense_flag == 1 )
    {
        memcpy((void *)verify.sense_data,(const void *)dummy_sense_data,
               DK_CXLFLASH_VERIFY_SENSE_LEN);
        memcpy((void *)p_ctx->verify_sense_data,(const void *)dummy_sense_data,
               DK_CXLFLASH_VERIFY_SENSE_LEN);
    }
    else
    {
        memcpy((void *)verify.sense_data,(const void *)p_ctx->verify_sense_data,
               DK_CXLFLASH_VERIFY_SENSE_LEN);
    }

    verify.context_id = p_ctx->context_id;
    verify.rsrc_handle = p_ctx->rsrc_handle;
    verify.hdr.version = p_ctx->version;
    verify.hdr.flags = p_ctx->flags;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_VERIFY, &verify);
#endif

    if (rc)
    {
        g_errno=errno;
        debug("%d: ioctl failed with errno=%d & rc=%d\n", pid, g_errno, rc);
    }

#ifdef _AIX
    p_ctx->return_flags = verify.return_flags;
#else
    p_ctx->return_flags = verify.hdr.return_flags;
#endif

    p_ctx->verify_last_lba = verify.last_lba;

    debug("%d: dev=%s fd=%d ",pid,p_ctx->dev,p_ctx->fd);

#ifndef _AIX
    debug("\n%d: ctx_id=0X%"PRIX64" res_hndl=0X%"PRIX64" \n",
          pid,p_ctx->context_id,p_ctx->rsrc_handle);
#else
    debug(" path_id=%d\n", p_ctx->path_id);
#endif

    debug("%d: verify_last_lba=0X%"PRIX64" hint=0X%"PRIX64" input_flag=0X%"PRIX64" return_flag=0X%"PRIX64"\n",
          pid,p_ctx->verify_last_lba,p_ctx->hint,p_ctx->flags,p_ctx->return_flags);
    if (DEBUG)
    {
        force_dump=1;
        hexdump(&p_ctx->verify_sense_data,0x20,"Sense data for verify ioctl");
        force_dump=0;
    }

    debug("%d:--------- End DK_CAPI_VERIFY ----------\n",pid);
    return g_errno;
}

#ifdef _AIX
int ioctl_dk_capi_log(struct ctx *p_ctx, char *s_data)
{
    int rc;
#ifdef _AIX
    struct dk_capi_log log;
#else
    struct dk_cxlflash_log log;
#endif
    memset(&log, 0, sizeof(log));

    log.rsrc_handle = p_ctx->rsrc_handle;
    log.reason = p_ctx->reason;
#ifdef _AIX
    log.version = p_ctx->version;
    log.flags = p_ctx->flags;
    log.path_id = p_ctx->path_id;
    log.devno = p_ctx->devno;
    log.ctx_token = p_ctx->context_id;
    rc = ioctl(p_ctx->fd, DK_CAPI_LOG_EVENT, &log);
#else
    //log.context_id = p_ctx->context_id;
    log.hdr.version = p_ctx->version;
    log.hdr.flags = p_ctx->flags;
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_LOG_EVENT, &log);
#endif
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_LOG_EVENT failed");
    }
#ifdef _AIX
    p_ctx->return_flags = log.return_flags;
#else
    p_ctx->return_flags = log.hdr.return_flags;
#endif
    strncpy(s_data, (char *)&(log.sense_data[0]), 512);
    s_data[512]='\0';
    return rc;
}
#else
//dummy function
int ioctl_dk_capi_log(struct ctx *p_ctx, char *s_data)
{
    return -1;
}
#endif
int ioctl_dk_capi_recover_ctx(struct ctx *p_ctx)
{
    int rc;
     
    debug("%d:--------Start DK_CAPI_RECOVER_CTX ---------\n",pid);
#ifdef _AIX
    struct dk_capi_recover_context recv_ctx;
    memset(&recv_ctx, 0,sizeof(struct dk_capi_recover_context));

    debug("%d:dev=%s fd=%d flags=0X%"PRIX64" ctx_id:0X%"PRIX64" devno=0X%"PRIX64" reason=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->flags,
          p_ctx->context_id,p_ctx->devno,p_ctx->reason);
    recv_ctx.version = p_ctx->version;
    recv_ctx.devno = p_ctx->devno;
    recv_ctx.flags = p_ctx->flags;
    recv_ctx.ctx_token = p_ctx->context_id;
    recv_ctx.reason = p_ctx->reason;

    rc = ioctl(p_ctx->fd, DK_CAPI_RECOVER_CTX, &recv_ctx);
    debug("%d:... DK_CAPI_RECOVER_CTX called ...\n", pid);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_RECOVER_CTX failed");
    }

    p_ctx->new_ctx_token = recv_ctx.new_ctx_token;
    p_ctx->p_host_map =(volatile struct sisl_host_map *)recv_ctx.mmio_start;
    p_ctx->mmio_size = recv_ctx.mmio_size;
    p_ctx->return_flags = recv_ctx.return_flags;
#else

    int old_adap_fd ;

    struct dk_cxlflash_recover_afu recv_ctx;
    memset(&recv_ctx, 0, sizeof(recv_ctx));

    old_adap_fd  = p_ctx->adap_fd;
    recv_ctx.hdr.version = p_ctx->version;
    recv_ctx.hdr.flags = p_ctx->flags;
    recv_ctx.context_id = p_ctx->context_id;
    recv_ctx.reason = p_ctx->reason;
    debug("%d:dev=%s fd=%d flags=0X%"PRIX64" ctx_id:0X%"PRIX64" adap_fd=%d reason=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->flags,
          p_ctx->context_id,p_ctx->adap_fd,p_ctx->reason);
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_RECOVER_AFU, &recv_ctx);
    debug("%d:... DK_CAPI_RECOVER_CTX called ...\n", pid);
    if (rc)
        CHECK_RC(errno, "DK_CXLFLASH_RECOVER_AFU failed");

    p_ctx->return_flags = recv_ctx.hdr.return_flags;
    p_ctx->adap_fd = recv_ctx.adap_fd;
    p_ctx->mmio_size = recv_ctx.mmio_size;
    p_ctx->new_ctx_token = recv_ctx.context_id;

    if (p_ctx->return_flags == DK_CXLFLASH_RECOVER_AFU_CONTEXT_RESET)
    {
        debug("%d: 1st do munmap then mmap fresh mmio size with new adap_fd\n",pid);
        rc = munmap((void *)p_ctx->p_host_map, p_ctx->mmio_size);
        if (rc)
            fprintf(stderr, "munmap failed with errno = %d", errno);
        else debug("%d: munmap() succeeded for older mmio space..\n", pid);

        p_ctx->p_host_map = mmap(NULL,p_ctx->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED,
                                 p_ctx->adap_fd,0);
        if (p_ctx->p_host_map == MAP_FAILED)
        {
            fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                    p_ctx->mmio_size, errno);
            CHECK_RC(1,"mmap failed");
        }
        else debug("%d: New mmap() returned success..\n", pid);

        // if context attach returns DK_CXLFLASH_APP_CLOSE_ADAP_FD flag
        if(p_ctx->close_adap_fd_flag == TRUE)
        {
           close(old_adap_fd);
        }

    }
    else
    {
        debug("%d:Recovery Action NOT Needed.. \n", pid);
    }

#endif /* _AIX */
    debug("%d:new_ctx_token=0X%"PRIX64" adap_fd=%d mmio=%p mmio_size=0X%"PRIX64" return_flags=0X%"PRIX64"\n",
          pid,p_ctx->new_ctx_token,p_ctx->adap_fd,p_ctx->p_host_map,p_ctx->mmio_size,
          p_ctx->return_flags);
    debug("%d:------------- End DK_CAPI_RECOVER_CTX -------------\n", pid);
    return rc;
}

int ioctl_dk_capi_query_exception(struct ctx *p_ctx)
{
    int rc = 0;
    debug("--------started DK_CAPI_QUERY_EXCEPTIONS ---------\n");
#ifdef _AIX
    struct dk_capi_exceptions exceptions;
    memset(&exceptions, 0, sizeof(struct dk_capi_exceptions));

    exceptions.version = p_ctx->version;
    exceptions.ctx_token = p_ctx->context_id;
    exceptions.devno     = p_ctx->devno;
    exceptions.rsrc_handle = p_ctx->rsrc_handle;
    exceptions.flags = p_ctx->flags;

    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_EXCEPTIONS, &exceptions);

    debug("-------- O/p of DK_CAPI_QUERY_EXCEPTIONS ----------\n");
    // I will keep this for debug
    if (DEBUG)
    {
        printf(" int rc=%d\n",rc);
        printf(" uint16_t version = %d\n",exceptions.version);
        printf(" uint64_t flags = 0x%llx\n",exceptions.flags);
        printf(" uint64_t return_flags = 0x%llx\n",exceptions.return_flags);
        printf(" dev64_t devno = 0x%llx\n",exceptions.devno);
        printf(" uint64_t ctx_token = 0x%llx\n",exceptions.ctx_token);
        printf(" uint64_t rsrc_handle = 0x%llx\n",exceptions.rsrc_handle);
        printf(" uint64_t exceptions = 0x%llx\n",exceptions.exceptions);
        printf(" uint64_t adap_except_type = 0x%llx\n",exceptions.adap_except_type);
        printf(" uint64_t adap_except_time = 0x%llx\n",exceptions.adap_except_time);
        printf(" uint64_t adap_except_data = 0x%llx\n",exceptions.adap_except_data);
        printf(" uint64_t adap_except_count = 0x%llx\n",exceptions.adap_except_count);
        printf(" uint64_t last_lba = 0x%llx\n",exceptions.last_lba);
    }

    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_QUERY_EXCEPTIONS failed");
    }

    p_ctx->return_flags = exceptions.return_flags;
    p_ctx->exceptions = exceptions.exceptions;

    p_ctx->adap_except_type = exceptions.adap_except_type;
    p_ctx->adap_except_data =  exceptions.adap_except_data;
#endif /* _AIX */

    debug("-------- End of DK_CAPI_QUERY_EXCEPTIONS ---------\n");

    return rc;
}

int create_res(struct ctx *p_ctx)
{
    return ioctl_dk_capi_uvirtual(p_ctx);
}
int close_res(struct ctx *p_ctx)
{
    return ioctl_dk_capi_release(p_ctx);
}

int mc_stat1(struct ctx *p_ctx, mc_stat_t *stat)
{
    __u64 size;
    stat->nmask = 0;
    size = p_ctx->chunk_size;
    stat->blk_len = p_ctx->block_size;
    while (size)
    {
        size = size>>1;
        stat->nmask++;
    }
    stat->nmask--;
    debug("%d:mc_stat1 chunk=0X%"PRIX64" nmask=0X%X\n",pid,stat->size,stat->nmask);
    return 0;
}

int mc_size1(struct ctx *p_ctx, __u64 chunk, __u64 *actual_size)
{
    int rc;
    p_ctx->req_size = (chunk * (p_ctx->chunk_size));
    debug("%d mc_size1 chunk=0X%"PRIX64" lun_size=0X%"PRIX64" req_size=0X%"PRIX64"\n",
          pid,chunk,p_ctx->lun_size,p_ctx->req_size);
    rc = ioctl_dk_capi_vlun_resize(p_ctx);
    CHECK_RC(rc, "ioctl_dk_capi_vlun_resize failed\n");
    *actual_size = p_ctx->lun_size/p_ctx->chunk_size;
    return rc;
}

int create_resource(struct ctx *p_ctx, __u64 nlba,
                     __u64 flags, __u16 lun_type)
{
    p_ctx->flags = flags;
    p_ctx->lun_size = nlba;

    if (LUN_VIRTUAL == lun_type)
        return ioctl_dk_capi_uvirtual(p_ctx);
    else
        return ioctl_dk_capi_udirect(p_ctx);
}

int vlun_resize(struct ctx *p_ctx, __u64 nlba)
{
    p_ctx->req_size = nlba;
    return ioctl_dk_capi_vlun_resize(p_ctx);
}

//dummy function
int mc_init()
{
    return 0;
}

//dummy function
int mc_term()
{
    return 0;
}
#ifdef _AIX
//IOCTL from here
int set_cflash_paths(struct flash_disk *disk)
{
    int rc=0;
    int i;
    int fd;
    int index=0;
    struct cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[MAX_PATH-1];
    }capi_paths;

    struct dk_capi_path_info *path_info = NULL;
    memset(&capi_paths, 0, sizeof(capi_paths));

    capi_paths.path.path_count = MAX_PATH;
    fd = open_dev(disk->dev, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr,"open %s failed\n",disk->dev);
        return -1;
    }
    rc = ioctl(fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    close(fd);
    if (rc)
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");

    if (capi_paths.path.returned_path_count == 0)
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");

    if (capi_paths.path.returned_path_count > MAX_PATH)
    {
        fprintf(stderr,"got more paths than provided issued=%d ret=%d\n",
                MAX_PATH,capi_paths.path.returned_path_count);
        capi_paths.path.returned_path_count = MAX_PATH;
    }

    path_info = capi_paths.path.path_info;
    for (i = 0; i < capi_paths.path.returned_path_count; i++)
    {
        /*if ((path_info[i].flags & DK_CPIF_RESERVED) &&
            !(path_info[i].flags & DK_CPIF_FAILED))*/
        {
            debug("%d:dev=%s devno=0X%"PRIX64" path_id=%"PRIu16" flags=0X%"PRIX64"\n",
                  pid,disk->dev,path_info[i].devno,
                  path_info[i].path_id,path_info[i].flags);
            disk->devno[index] = path_info[i].devno;
            disk->path_id[index] = path_info[i].path_id;
            disk->path_id_mask[index] = 1<<path_info[i].path_id;
            index++;
        }
    }
    disk->path_count = index;
    return rc;
}
#endif /* _AIX */

int get_flash_disks(struct flash_disk disks[], int type)
{
    int count =0;
    int rc=0;
    int i=0;
    FILE *fptr;
    char *p_file;
    char buf[10];
    char *ptrP = NULL;
    char * export_disks;
    int  exportFlag = 0;
#ifdef _AIX
    const char *cmd ="lsdev -c disk -s capidev -t extflash |awk '{print $1}'>/tmp/flist";
#else
    const char *cmd ="/opt/ibm/capikv/bin/cxlfstatus | \
			grep superpipe | sort -u -k5 | \
			awk '{print $1}' | tr -d ':' >/tmp/flist";
#endif

    rc = system(cmd);
    if (rc) return 0;

    if ( type == FDISKS_ALL )
    {
        debug("%d:cmd=%s rc=%d\n",pid, cmd, rc);
        p_file="/tmp/flist";
    }
    else if ( type == FDISKS_SAME_ADPTR )
    {
        p_file="/tmp/flist_sameAdap";
        debug("%d: List of capi disks from same adapter present in %s file\n", pid, p_file);

        export_disks = (char *)getenv("FVT_DEV_SAME_ADAP");
        if ( NULL != export_disks)
           exportFlag = 1; // variable is exported 
    }
    else if ( type == FDISKS_SHARED )
    {
        p_file="/tmp/flist_disk_shared";
        debug("%d: List of capi disks shared with different adapter in %s file\n", pid, p_file);

        export_disks = (char *)getenv("FVT_DEV_SHARED");
        if ( NULL != export_disks)
            exportFlag = 1; // variable is exported
    }
    else
    {
        p_file="/tmp/flist_diffAdap";
        debug("%d: List of capi disks from diff adapter present in %s file\n", pid, p_file);
 
        export_disks = (char *)getenv("FVT_DEV_DIFF_ADAP");
        if ( NULL != export_disks)
           exportFlag = 1; // variable is exported
    }

    debug("%d: List of all unique capi disks present in /tmp/flist\n", pid);
    debug("%d: Refer /tmp/flist file for sample format\n", pid);

#ifndef _AIX
    system("rm /tmp/flist_sameAdap /tmp/flist_diffAdap /tmp/flist_disk_shared >/dev/null 2>&1");
#endif
    if( exportFlag == 0)
    {
      fptr = fopen(p_file, "r");
      if (NULL == fptr )
      {
#ifdef _AIX
        fprintf(stderr,"%d: --------------------------------------------------------\n", pid);
        fprintf(stderr,"%d: Error opening file %s\n", pid, p_file);
        fprintf(stderr,"%d: Retry after fixing/verifying the file: %s\n", pid, p_file);
        fprintf(stderr,"%d: --------------------------------------------------------\n", pid);
        return 0;
#else
        debug("%d: ---------- Automatically populating the disk in adapter -------\n", pid);
        if ( type == FDISKS_SAME_ADPTR )
        {
            rc = diskInSameAdapater(p_file);
            if (rc)
            {
                fprintf(stderr,"%d: failed in diskInSameAdapater()--\n", pid);
                return 0;
            }

            fptr = fopen(p_file, "r");
            if (NULL == fptr)
            {
                fprintf(stderr,"%d: Error opening file %s\n", pid, p_file);
                fprintf(stderr,"%d: test aborted \n",pid);
                return 0;
            }
            // reseting errno
            if (errno)
                errno = 0;

        }

        else if ( type == FDISKS_DIFF_ADPTR )
        {
            rc = diskInDiffAdapater(p_file);
            if (rc)
            {
                fprintf(stderr,"%d: failed in diskInDiffAdapater()--\n", pid);
                return 0;
            }

            fptr = fopen(p_file, "r");
            if (NULL == fptr)
            {
                fprintf(stderr,"%d: Error opening file %s\n", pid, p_file);
                fprintf(stderr,"%d: test aborted \n",pid);
                return 0;
            }
            // reseting errno
            if (errno)
                errno = 0;
        }
#endif
       }

       while (fgets(buf,10, fptr) != NULL)
       {
           i=0;
          while (i < 10)
          {
            if (buf[i] =='\n')
            {
                buf[i]='\0';
                break;
            }
            i++;
          }
        sprintf(disks[count].dev,"/dev/%s",buf);
#ifdef _AIX
        //for LINUX devno & path_id have no meaning
        //will be ignored
        set_cflash_paths(&disks[count]);
#endif
        count++;
        if (MAX_FDISK == count)
            break;
      }
      fclose(fptr);
    }
    else
    {
       ptrP = strtok( export_disks, "," );
       while (  ptrP != NULL )
       {
           sprintf(disks[count].dev,"%s",ptrP);
           count = count +1;
           ptrP=strtok(NULL,",");
       } 

    }
    return count;
}


//wait all child processes to finish their task
int wait4all()
{
    int rc;
    pid_t mypid;

    while ((mypid = waitpid(-1, &rc, 0)))
    {
        if (mypid == -1)
        {
            break;
        }
        if (WIFEXITED(rc))
        {
            rc =  WEXITSTATUS(rc);
            if (rc)
                g_error = -1;
        }
        else
        {
            fprintf(stderr, "%d : abnormally terminated\n", mypid);
            g_error =-1;
        }
        debug("pid %d exited with rc=%d\n", mypid,rc);
        fflush(stdout);


    }
    rc = g_error;
    g_error = 0;
    return rc;
}

int wait4allOnlyRC()
{
    int rc = 0;
    int rc1 = 0;
    pid_t mypid;
    int nerror = 0;
    int rc_fail_cnt=0;
    char * noIOP   = getenv("NO_IO");

    while ((mypid = waitpid(-1, &rc, 0)))
    {
        debug("pid %d waitpid() status =%d\n", mypid,rc);
        if (mypid == -1)
        {
            break;
        }
        if (WIFEXITED(rc))
        {
            rc1 |=  WEXITSTATUS(rc);
            if (rc1)
            {
               /* It should start failing whenver it hits the first fail */
               fprintf(stderr, "%d : Returned Failure RC = %d\n", mypid,rc); 
               nerror = -1;
               rc_fail_cnt = rc_fail_cnt +1; 
               // IO can fail with UA only one time after Recover. 
               // if rc_fail_cnt = 1 ; then rc_fail_cnt has the authority to change
               // nerror = 0 else test case will be marked as failed.
               // First IO failure scenario can only happen if we are running with 
               // IO. In case of NO IO and non-SurelockFC , we should expect no failure 
               if ( rc_fail_cnt == 1 &&  noIOP == NULL && is_UA_device(cflash_path) == TRUE )
               {
                   debug("pid %d Reset cnt %d for nerror status =%d\n", mypid,rc_fail_cnt,nerror);
                   nerror = 0;
                   rc1 = 0;
               }
            }
           
        }
        else
        {
            fprintf(stderr, "%d : abnormally terminated\n", mypid);
            nerror =-1;
        }
        debug("pid %d exited with rc=%d\n", mypid,rc);
        fflush(stdout);

    }

    // Ideally rc_fail_cnt should be 1 ; if it is zero. that IO never failed
    // with UA. which is not expected case.
    // In case of NO IO, we should expect no failure. rc_fail_cnt should be 0
    if ( rc_fail_cnt == 0 && noIOP == NULL && is_UA_device(cflash_path) == TRUE )
    {
       debug("pid %d First IO did not fail with UA, So test failed \n", getpid());
       nerror = 255;
    }
    
    rc = nerror;
    nerror = 0;
    return rc;
}

int do_internal_io(struct ctx *p_ctx, __u64 stride, bool iocompare)
{
    __u64 st_lba= p_ctx->st_lba;
    __u64 remain;
    int rc = 0;
    debug("%d: IO st_lba = 0X%"PRIX64" and last_lba = 0X%"PRIX64"\n",
          pid, st_lba,p_ctx->last_lba);
    if (st_lba >= p_ctx->last_lba)
    {
        fprintf(stderr, "%d: Failed st_lba should be less than last_lba\n", pid);
        return -1;
    }
    //adjust lbas to rw boundary LBAs & within range
    remain = (p_ctx->last_lba+1-st_lba)%(NUM_CMDS*stride);
    if (remain)
    {
        rc = send_write(p_ctx, st_lba, stride, pid);
        CHECK_RC(rc, "send_write failed");
        rc = send_read(p_ctx, st_lba, stride);
        CHECK_RC(rc, "send_read failed");
        if (iocompare)
        {
            rc = rw_cmp_buf(p_ctx, st_lba);
            if (rc)
            {
                fprintf(stderr,"buf cmp failed for lba 0x%"PRIX64",rc =%d\n",
                        st_lba,rc);
                return rc;
            }
        }
        st_lba+= remain;
        debug("%d: adjusting 0X%"PRIX64" lba with st_lba=0X%"PRIX64"\n",pid,remain,st_lba);
    }

    for (; st_lba <= p_ctx->last_lba; st_lba += (NUM_CMDS*stride))
    {
        rc = send_write(p_ctx, st_lba, stride, pid);
        CHECK_RC(rc, "send_write failed");
        rc = send_read(p_ctx, st_lba, stride);
        CHECK_RC(rc, "send_read failed");
        if (iocompare)
        {
            rc = rw_cmp_buf(p_ctx, st_lba);
            if (rc)
            {
                fprintf(stderr,"buf cmp failed for lba 0X%"PRIX64",rc =%d\n",
                        st_lba,rc);
                break;
            }
        }
    }
    return rc;
}

int do_io(struct ctx *p_ctx, __u64 stride)
{
    int rc;
    rc = do_internal_io(p_ctx, stride, true); //true means do IO compare
    debug("%d:IO done with rc=%d\n",pid,rc);
    return rc;
}
int do_io_nocompare(struct ctx *p_ctx, __u64 stride)
{
    return do_internal_io(p_ctx, stride, false); //0 NO IO compare
}

int get_max_res_hndl_by_capacity(char *dev)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx=&myctx;
    __u64 chunks;
    int m_res_pr_ctx=0;

    pid=getpid();
    memset(p_ctx, 0, sizeof(myctx));
    strcpy(p_ctx->dev, dev);
    if ((p_ctx->fd = open_dev(p_ctx->dev, O_RDWR)) < 0)
    {
        fprintf(stderr,"open failed %s, errno %d\n",cflash_path, errno);
        return -1;
    }
#ifdef _AIX
    rc=ioctl_dk_capi_query_path(p_ctx);
#endif
    p_ctx->work.num_interrupts =cflash_interrupt_number();
    rc = ioctl_dk_capi_attach(p_ctx);
    if (rc)
    {
        close(p_ctx->fd);
        return -1;
    }
    chunks = (p_ctx->last_phys_lba)/(p_ctx->chunk_size);
    m_res_pr_ctx=chunks/MAX_OPENS;
    debug("%d:chunks=0X%"PRIX64" m_res_pr_ctx=%d\n",pid,chunks,m_res_pr_ctx);
    ctx_close(p_ctx);
    if (m_res_pr_ctx == 0)
    {
        printf("%d:Minimum disk capacity required=%luGB\n",
               pid,(MAX_OPENS * p_ctx->block_size * p_ctx->chunk_size)/(1024*1024*1024));
        return m_res_pr_ctx;
    }
    debug("%d:Max res handle per ctx=%d\n",pid,m_res_pr_ctx);
    return m_res_pr_ctx;
}


__u64 get_disk_last_lba(char *dev, dev64_t devno, uint64_t *chunk_size)
{
    __u64 last_lba = 0;
    int rc;
    struct ctx myctx;

    memset(&myctx, 0, sizeof(struct ctx));
    myctx.fd = open_dev(dev, O_RDWR);
    if (myctx.fd < 0)
    {
        fprintf(stderr,"%d: %s opened failed\n",pid, dev);
        return 0;
    }
    strcpy(myctx.dev, dev);
    myctx.devno = devno;
    myctx.flags = DK_AF_ASSIGN_AFU;
    myctx.work.num_interrupts =cflash_interrupt_number();
    rc = ioctl_dk_capi_attach(&myctx);
    last_lba = myctx.last_phys_lba;
    *chunk_size = myctx.chunk_size;
    rc |= ctx_close(&myctx);
    if (rc) return 0;
    return last_lba;
}

#ifdef _AIX
int ioctl_dk_capi_query_path_check_flag(struct ctx *p_ctx,
                                        int flag1, int flag2)
{
    int rc;
    struct  cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[MAX_PATH-1];
    }capi_paths;

    struct dk_capi_path_info *path_info = NULL;
    memset(&capi_paths, 0, sizeof(capi_paths));

    capi_paths.path.version = p_ctx->version;
    capi_paths.path.path_count = MAX_PATH;

    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    //TBD, handled for one path id, handle for all
    p_ctx->devno = path_info[0].devno;
    CHECK_RC(rc, "DK_CAPI_QUERY_PATHS failed");
    path_info = capi_paths.path.path_info;
    // check for flag if ioctl passed
    if ( path_info[0].flags == flag1 && path_info[1].flags == flag2 )
    {
        rc=0;
    }
    else if ( path_info[0].flags == flag2  && path_info[1].flags == flag1 )
    {
        rc=0;
    }
    else
        rc=1;
    return rc;
}
#endif


int do_eeh(struct ctx *p_ctx)
{
    int len;
#ifdef _AIX
    if (!DEBUG)
        printf("%d: ....... Waiting EEH/AFU_RESET should be done Manually.........\n",pid);
    while (1)
    {
        if (DEBUG)
            printf("%d: ....... Waiting EEH/AFU_RESET should be done Manually.........\n",pid);
        //1st option
        /*len = read(p_ctx->adap_fd, &p_ctx->event_buf[0],
                        sizeof(p_ctx->event_buf));
           if((len < 0) && (errno == EIO))
           {
                   afu_reset = true;
           }*/
        //2nd option
        __u64 room = read_64(&p_ctx->p_host_map->cmd_room);
#ifndef __64BIT__
		if (room == 0XFFFFFFFF)
#else
        if (room == -1)
#endif
        {
            usleep(1000);//just give chance to call exception query
            afu_reset = true;
        }
        if (afu_reset)
        {
            printf("%d: EEH/AFU_RESET is done.......\n",pid);
            //usleep(1000) //just give chance to call exception query
            clear_waiting_cmds(p_ctx);
            break;
        }
        sleep(1);
    }

#else
    //linux, we read adapter register in ctx_rrq_rx thread
    //it will tell us if afu_rest done,
    //but some cases if no ctx_rrq_rx started
    //better read adapter register here

    int rc            = 0;
    eehCmd_t eehCmdVar;
    eehCmd_t *eehCmdP = &eehCmdVar;
    pthread_t thread_eeh;
    pthread_mutexattr_t mattrVar;
    pthread_condattr_t cattrVar;
    char tmpBuff[MAXBUFF];

    if (  manEEHonoff != 0 )
    {
        while (1)
        {
            printf("%d: ....... Waiting EEH should be done Manually.........\n",pid);
            len = read(p_ctx->adap_fd, &p_ctx->event_buf[0],
                       sizeof(p_ctx->event_buf));
            if ((len < 0) && (errno == EIO))
            {
                printf("%d: EEH is done.......\n",pid);
                afu_reset=true;
                break;
            }
            sleep(1);
        }
    }
    else
    {
        eehCmdP->ieehLoop = 1;
        pthread_mutexattr_init(&mattrVar);
        pthread_condattr_init(&cattrVar);
        pthread_mutex_init(&eehCmdP->eeh_mutex , &mattrVar);
        pthread_cond_init(&eehCmdP->eeh_cv , &cattrVar);


        rc = diskToPCIslotConv(p_ctx->dev , tmpBuff );
        CHECK_RC(rc, "diskToPCIslotConv failed \n");

        rc = prepEEHcmd( tmpBuff, eehCmdP->cmdToRun);
        CHECK_RC(rc, " prepEEHcmd failed \n");

        eehCmdP->eehSync = 0;

        debug("%d:---------------- Going to trigger EEH --------------------\n",pid);

        pthread_create(&thread_eeh,NULL,do_trigger_eeh_cmd, eehCmdP);

        pthread_mutex_lock( &eehCmdP->eeh_mutex );

        while ( eehCmdP->eehSync != 1)
        {
            pthread_cond_wait(&eehCmdP->eeh_cv, &eehCmdP->eeh_mutex);
        }

        pthread_cancel(thread_eeh);

        pthread_mutex_unlock( &eehCmdP->eeh_mutex);

        pthread_mutexattr_destroy(&mattrVar);
        pthread_condattr_destroy(&cattrVar);


        while (1)
        {
            printf("%d: ....... Reading the adap_fd for EEH event.........\n",pid);
            len = read(p_ctx->adap_fd, &p_ctx->event_buf[0],
                       sizeof(p_ctx->event_buf));
            if ((len < 0) && (errno == EIO))
            {
                printf("%d: EEH is done.......\n",pid);
                afu_reset=true;
                break;
            }
            sleep(1);
        }


    }
#endif
    return 0;
}

void * do_trigger_eeh_cmd( void * arg )
{

#ifndef _AIX
    int iCnt           = 0;
    int rc             = 0;
    eehCmd_t * eehCmdP = arg;

    const char *configCmdP = "echo 10000000  > /sys/kernel/debug/powerpc/eeh_max_freezes";

    if ( eehCmdP->ieehLoop <= 0 )
        eehCmdP->ieehLoop = 1 ;


    pthread_mutex_lock( &eehCmdP->eeh_mutex );

    rc = system(configCmdP);
    if (rc)
    {
        g_error = -1;
        fprintf(stderr,"%d: Failed in %s \n",pid,configCmdP);
    }

    for (iCnt =0 ; iCnt < eehCmdP->ieehLoop ; iCnt++)
    {
        sleep(2);

        rc = system(eehCmdP->cmdToRun);
        if (rc)
        {
            g_error = -1;
            fprintf(stderr,"%d: Failed in %s \n",pid,eehCmdP->cmdToRun);
        }
    }

    eehCmdP->eehSync = 1;
    pthread_cond_signal( &eehCmdP->eeh_cv);
    pthread_mutex_unlock( &eehCmdP->eeh_mutex );

    return 0;
#endif

}

int do_poll_eeh(struct ctx *p_ctx)
{
#ifndef _AIX
    int len;
    printf("%d: ....... Waiting EEH should be done........\n",pid);

    while (1)
    {
        len = read(p_ctx->adap_fd, &p_ctx->event_buf[0],
                   sizeof(p_ctx->event_buf));
        if ((len < 0) && (errno == EIO))
        {
            printf("%d: EEH is done.......\n",pid);
            afu_reset=true;
            break;
        }
        sleep(1);
    }
#endif

    return 0;
}


bool check_afu_reset(struct ctx *p_ctx)
{
#ifdef _AIX
    __u64 room = read_64(&p_ctx->p_host_map->cmd_room);
    if (room == -1)
    {
        afu_reset = true;
        printf("%d: EEH/AFU_RESET is done.......\n",pid);
    }
#endif
    return afu_reset;
}

int compare_size(uint64_t act, uint64_t exp)
{
    int rc = 0;
    if (act != exp)
    {
        fprintf(stderr,"%d: Failed in compare_size():\
                act=0X%"PRIX64" exp=0X%"PRIX64"\n",
                pid, act, exp);
        g_error=-1;
        rc = -1;
    }
    return rc;
}

int compare_flags(uint64_t act, uint64_t exp)
{
    int rc = 0;
    return rc;
    // TBD: disable this code if return flags testing is to be skipped
    if (act != exp)
    {
        fprintf(stderr,"%d: Failed in compare_flags():\
                act=0X%"PRIX64" exp=0X%"PRIX64"\n",
                pid, act, exp);
        g_error=-1;
        rc = -1;
    }
    //return rc;
}

int validateFunction(struct validatePckt * newVar)
{
    int rc=0;

    switch (newVar->obCase)
    {
        case CASE_PLUN :
            rc = compare_size(newVar->ctxPtr->last_lba, newVar->expt_last_lba);
            //TBD - flag check will be enable it later
            //if ( newVar->ctxPtr->return_flags != newVar->expt_return_flags )
            //{
            // rc=-1;
            //}
            break;
        case CASE_VLUN :
            rc = compare_size(newVar->ctxPtr->last_lba, newVar->expt_last_lba);
            //TBD - flag check will be enable it later
            //if ( newVar->ctxPtr->return_flags != newVar->expt_return_flags )
            //{
            //   rc=-1;
            //}
            break;
        default :
            rc=-1;

    }
    return rc;
}

int diskToPCIslotConv( char * diskName , char * pciSlotP)
{

    int iCount =0;
    int rc     =0;
    int iTer   =0;
    int iKey   =0;

    FILE *fileP;
    char tmpBuff[MAXBUFF];
    char npBuff[MAXNP][MAXBUFF];
    char blockCheckP[MAXBUFF];

    const char *initCmdP = "lspci -v | grep \"Processing accelerators\" | awk '{print $1}' > /tmp/trashFile";

    rc = system(initCmdP);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in lspci \n",pid);
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");

    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    while (fgets(tmpBuff,MAXBUFF, fileP) != NULL)
    {
        while (iTer < MAXBUFF)
        {
            if (tmpBuff[iTer] =='\n')
            {
                tmpBuff[iTer]='\0';
                break;
            }
            iTer++;
        }

        // only supporting for scsi_generic device

        sprintf(blockCheckP,"ls -l /sys/bus/pci/devices/"
                    "%s/pci***:**/***:**:**.*/host*/"
                    "target*:*:*/*:*:*:*/ | grep -w \"scsi_generic\" >/dev/null 2>&1",tmpBuff);
        rc = system(blockCheckP);

        if ( rc == 0 )
        {

            iKey = strlen(diskName)-3 ;

            sprintf(npBuff[iCount],"ls -l /sys/bus/pci/devices/"
                        "%s/pci***:**/***:**:**.*/host*/"
                        "target*:*:*/*:*:*:*/scsi_generic | grep %s >/dev/null 2>&1",tmpBuff,&diskName[iKey]);

            rc = system(npBuff[iCount]);
            if ( rc == 0 )
            {
                fclose(fileP);
                break;
            }

            iCount++;
        }
    }

    sprintf(npBuff[iCount],"cat /sys/bus/pci/devices"
                "/%s/devspec > /tmp/trashFile",tmpBuff);

    rc = system(npBuff[iCount]);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: failed to find PCI devspec \n",pid);
        rc = EINVAL ;
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");
    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    if ( NULL == fgets(tmpBuff,MAXBUFF, fileP) )
    {
        fprintf(stderr,"%d: Error in file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    if ( fclose(fileP) == EOF )
    {
        fprintf(stderr,"%d: Error closin the file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }


    sprintf(npBuff[iCount],"cat /proc/device-tree"
                "%s/ibm,loc-code > /tmp/trashFile", tmpBuff);

    rc = system(npBuff[iCount]);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: failed to find PCI devspec \n",pid);
        rc = EINVAL ;
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");
    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    if ( NULL == fgets(tmpBuff,MAXBUFF, fileP) )
    {
        fprintf(stderr,"%d: Error in file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    iTer=0;

    while (iTer < MAXBUFF)
    {
        if (tmpBuff[iTer] ==' ')
        {
            tmpBuff[iTer]='\0';
            break;
        }
        iTer++;
    }

    if ( fclose(fileP) == EOF )
    {
        fprintf(stderr,"%d: Error closin the file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }


    // Need to do error handling stuff
    strncpy( pciSlotP, tmpBuff,strlen(tmpBuff)+1);

xerror:
    return rc;
}

int prepEEHcmd( char * pciSlotP, char * cmd )
{
    int rc        =0;
    int ikeyFind  =0;

    ikeyFind  = strlen(pciSlotP) - KEYLEN;

    if ( !strcmp(&pciSlotP[ikeyFind] , "P1-C7"))
    {

        sprintf(cmd,"echo 1 > /sys/kernel/debug/powerpc/"
                    "PCI0000/err_injct_outbound");
    }
    else if ( !strcmp(&pciSlotP[ikeyFind] , "P1-C6"))
    {

        sprintf(cmd,"echo 1 > /sys/kernel/debug/powerpc/"
                    "PCI0002/err_injct_outbound");

    }
    else if ( !strcmp(&pciSlotP[ikeyFind] , "P1-C5"))
    {

        sprintf(cmd,"echo 1 > /sys/kernel/debug/powerpc/"
                    "PCI0004/err_injct_outbound");

    }
    else if ( !strcmp(&pciSlotP[ikeyFind] , "P1-C3"))
    {

        sprintf(cmd,"echo 1 > /sys/kernel/debug/powerpc/"
                    "PCI0006/err_injct_outbound");


    }
    else if ( !strcmp(&pciSlotP[ikeyFind] , "P1-C2"))
    {

        sprintf(cmd,"echo 1 > /sys/kernel/debug/powerpc/"
                    "PCI0007/err_injct_outbound");

    }
    else
    {

        fprintf(stderr,"%d: fail in prepEEHcommad() \n", pid);
        rc = EINVAL;

    }


    return rc;
}


int ioctl_dk_capi_clone(struct ctx *p_ctx,uint64_t src_ctx_id,int src_adap_fd)
{
    int rc = 0;
#ifndef _AIX
    struct dk_cxlflash_clone clone;
    memset(&clone, 0, sizeof(clone));
    clone.context_id_src = src_ctx_id;
    clone.context_id_dst = p_ctx->context_id;
    clone.adap_fd_src = src_adap_fd;
    clone.hdr.version = p_ctx->version;
    debug("%d:----------- Start DK_CXLFLASH_VLUN_CLONE ----------\n", pid);
    debug("%d:src_ctx_id=0X%"PRIX64" dst_ctx_id=0X%"PRIX64" src_adap_fd=%d\n",
          pid,src_ctx_id,p_ctx->context_id,src_adap_fd);
    rc =ioctl(p_ctx->fd,DK_CXLFLASH_VLUN_CLONE, &clone);
    if (rc)
        CHECK_RC(errno, "DK_CXLFLASH_VLUN_CLONE failed with errno\n");

    // if context attach returns DK_CXLFLASH_APP_CLOSE_ADAP_FD flag
    if ( p_ctx->close_adap_fd_flag == TRUE )
    {
       close(src_adap_fd);
    }
    debug("%d:----------- Done DK_CXLFLASH_VLUN_CLONE ----------\n", pid);
#endif
    return rc;
}

int capi_open_close( struct ctx *p_ctx, char *dev )
{
    int rc=0;
    //open CAPI Flash disk device
#ifdef _AIX
    //p_ctx->fd = open_dev(dev, O_RDWR);
    p_ctx->fd = open(dev, O_RDWR);
#else
    p_ctx->fd = open(dev, O_RDWR);
#endif
    if (p_ctx->fd < 0)
    {
        fprintf(stderr, "open() failed: device %s, errno %d\n", dev, errno);
        return -1;
    }
    // close CAPI Flash disk device
    rc=close(p_ctx->fd);
    return rc;
}

// to create LUN_DIRECT
int create_direct_lun( struct ctx *p_ctx )
{
    int rc,i=0;

    pthread_t thread;
    __u64 stride= 0x10000, nlba=0;
    rc = ctx_init(p_ctx);

    CHECK_RC(rc, "Context init failed");
    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    rc = create_resource(p_ctx, nlba, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");
    // do io on context
    for ( i=0; i< long_run; i++)
    {
        rc=do_io(p_ctx, stride);
        if ( rc != 0 )
            return rc;
    }
    return rc;
}

// to create multiple context

// create VLUN
int create_vluns(char *dev, dev64_t devno,
                  __u16 lun_type, __u64 chunk,struct ctx *p_ctx)
{
    int rc,i=0,flag=0;
    pthread_t thread;
    __u64 stride= 0x10;
    __u64 nlba=0;


    pthread_t  ioThreadId;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t *p_ioThreadData;



    p_ioThreadData=&ioThreadData;
    pid = getpid();
    rc = ctx_init2(p_ctx, dev, DK_AF_ASSIGN_AFU, devno);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    // create VLUN
    nlba = chunk * (p_ctx->chunk_size);
    rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_DIRECT failed");
    for ( i=0; i<long_run; i++ )
    {
        p_ioThreadData->p_ctx=p_ctx;
        p_ioThreadData->stride=stride;
        p_ioThreadData->loopCount=5;
        rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
        CHECK_RC(rc, "do_io_thread() pthread_create failed");
        if ( rc != 0 )
            flag=1;
    }
    pthread_cancel(thread);
    sleep(10);
    close_res(p_ctx);
    ctx_close(p_ctx);
    debug(" FLAG = %d\n", flag );
    return flag;
}

int create_multiple_vluns(struct ctx *p_ctx)
{
    int rc,j;
    int cfdisk = 0;
    struct flash_disk fldisks[MAX_FDISK];
    __u64 chunks[10] ={ 1,3,9,15,26,20,8,10,7,4 };
    cfdisk = get_flash_disks(fldisks, FDISKS_ALL);
    if (cfdisk < 1)
    {
        fprintf(stderr,"NO flash disks found\n");
        return -1;
    }
    // use the first disk
    //create atleast 10 chunks on each on PLUN
    for (j=0; j < 10; j++)
    {
        if (0 == fork()) //child process
        {
            rc = create_vluns(fldisks[0].dev, fldisks[0].devno[0],
                              LUN_VIRTUAL,chunks[j],p_ctx);

            exit(rc);
        }
    }

    rc=wait4all();
    return rc;
}

#ifdef _AIX
int ioctl_dk_capi_query_path_get_path(struct ctx *p_ctx, dev64_t devno1[])
{
    int rc=0;
    int i;

    struct cflash_paths
    {
        struct dk_capi_paths path;
        struct dk_capi_path_info paths[MAX_PATH];
    }capi_paths;

    struct dk_capi_path_info *path_info = NULL;
    memset(&capi_paths, 0, sizeof(capi_paths));

    if ((p_ctx->fd = open_dev(p_ctx->dev,O_RDWR)) < 0)
    {
        fprintf(stderr,"open failed %s, errno %d\n",p_ctx->dev, errno);
        g_error = -1;
        return -1;
    }

    capi_paths.path.version = p_ctx->version;
    capi_paths.path.path_count = MAX_PATH;

    rc = ioctl(p_ctx->fd, DK_CAPI_QUERY_PATHS, &capi_paths);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");
    }

    printf("capi_paths.path.returned_path_count=%d\n", capi_paths.path.returned_path_count);
    if (capi_paths.path.returned_path_count == 0)
    {
        CHECK_RC(errno, "DK_CAPI_QUERY_PATHS failed");
    }
    if (capi_paths.path.returned_path_count > MAX_PATH)
    {
        fprintf(stderr,"got more paths than provided issued=%d ret=%d\n",
                MAX_PATH,capi_paths.path.returned_path_count);
        capi_paths.path.returned_path_count = MAX_PATH;
    }

    path_info = capi_paths.path.path_info;
    //get 1st reserved path
    for (i = 0; i < capi_paths.path.returned_path_count; i++)
    {
        devno1[i] = path_info[i].devno;
        printf("devno[%d]=%x", i,devno1[i]);
    }
    /* if (capi_paths.path.return_flags)
       {
           CHECK_RC(1, "DK_CAPI_QUERY_PATHS failed unexpected return_flags");
       } */
    close(p_ctx->fd);
    return  capi_paths.path.returned_path_count;
}
#endif

int get_nonflash_disk(char * dev, dev64_t * devno)
{
    // TBD : cleanup later.
    // Till then workaround like this while compile.
    strcpy(dev, "/dev/sda");
    return 1;
}

void * do_io_thread(void * p_arg)
{
    do_io_thread_arg_t * thData;
    int counter;

    thData=(do_io_thread_arg_t *)p_arg;
    counter=thData->loopCount;

    while (counter--)
    {
        // do super pipe write. We just don't care about IO in this test.
        do_io(thData->p_ctx, thData->stride);
        if (afu_reset) break;
    }
    return NULL;
}

int traditional_io(int disk_num)
{
    int rc;

    char *disk_name, *str=NULL;
    struct flash_disk disks[MAX_FDISK];  // flash disk struct
    get_flash_disks(disks, FDISKS_ALL);
    pid = getpid();
    str = (char *) malloc(100);
    if ( disk_num == 1 )
    {
        disk_name = strtok(disks[0].dev,"/");
        disk_name = strtok(NULL,"/");
    }
    else
    {       disk_name = strtok(disks[1].dev,"/");
        disk_name = strtok(NULL,"/");
    }
    sprintf(str, "dd if=/usr/lib/boot/unix_64 of=/dev/%s >/tmp/read_write.log 2>&1 &", disk_name);
    debug("%s\n", str );
    rc=system(str);
    sleep(60);
    rc=system("cat /tmp/read_write.log  | grep -i \"do not allow\" ");
    if ( rc == 0 )
        return 1;
    return 0;
}


int ioctl_dk_capi_attach_reuse(struct ctx *p_ctx,struct ctx *p_ctx_1, __u16 lun_type )
{
    int rc,io=0;
    pthread_t thread;
#ifdef _AIX
    struct dk_capi_attach capi_attach;
    struct devinfo iocinfo;
    memset(&iocinfo, 0, sizeof(iocinfo));
#else
    struct dk_cxlflash_attach capi_attach;
#endif
    memset(&capi_attach, 0, sizeof(capi_attach));

    p_ctx->flags = DK_AF_ASSIGN_AFU;
    debug("%d:----------- Start First  DK_CAPI_ATTACH ----------\n", pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags);
    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer);
    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx->adap_fd,p_ctx->return_flags);
#ifdef _AIX
    capi_attach.version = p_ctx->version;
    capi_attach.flags = p_ctx->flags;
    capi_attach.devno = p_ctx->devno;
    debug("%d:devno=0X%"PRIX64"\n",pid,p_ctx->devno);
#else
    capi_attach.hdr.version = p_ctx->version;
    capi_attach.hdr.flags = p_ctx->flags;
#endif
    capi_attach.num_interrupts = p_ctx->work.num_interrupts;
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64" num_interrupts=%d context_id=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags,
          p_ctx->work.num_interrupts,p_ctx->context_id);

#ifdef _AIX
    rc = ioctl(p_ctx->fd, DK_CAPI_ATTACH, &capi_attach);
#else
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
    debug("%d:...First  DK_CAPI_ATTACH called ...\n", pid);
    if (rc)
    {
        CHECK_RC(errno, "FIRST DK_CAPI_ATTACH failed");
    }

    p_ctx->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
    p_ctx->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
    p_ctx->context_id = capi_attach.ctx_token;
    p_ctx->last_phys_lba = capi_attach.last_phys_lba;
    p_ctx->chunk_size = capi_attach.chunk_size;
    //get max_xfer
    rc = ioctl(p_ctx->fd, IOCINFO, &iocinfo);
    if (rc)
    {
        CHECK_RC(errno, "Iocinfo failed with errno\n");
    }
    //p_ctx->max_xfer = iocinfo.un.capi_io.max_transfer;
    //TBD
    p_ctx->max_xfer = iocinfo.un.scdk64.lo_max_request;
    if (iocinfo.flags  & DF_LGDSK)
    {
        p_ctx->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
    }
    p_ctx->return_flags = capi_attach.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#else
    p_ctx->p_host_map = mmap(NULL,p_ctx->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED,
                             capi_attach.adap_fd,0);
    if (p_ctx->p_host_map == MAP_FAILED)
    {
        fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                p_ctx->mmio_size, errno);
        CHECK_RC(1,"mmap failed");
    }
    p_ctx->context_id = capi_attach.context_id;
    p_ctx->last_phys_lba = capi_attach.last_lba;
    p_ctx->max_xfer = capi_attach.max_xfer;
    p_ctx->chunk_size = NUM_BLOCKS;
    p_ctx->return_flags = capi_attach.hdr.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#endif

    //default rwbuff handling 4K, Lorge trasnfer handled exclusive
    p_ctx->blk_len = BLOCK_SIZE/p_ctx->block_size;
    p_ctx->adap_fd = capi_attach.adap_fd;

    p_ctx->ctx_hndl = CTX_HNDLR_MASK & p_ctx->context_id;
    p_ctx->unused_lba = p_ctx->last_phys_lba +1;

    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer);

    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx->adap_fd,p_ctx->return_flags);
    debug("%d:------------- End FIRST  DK_CAPI_ATTACH -------------\n", pid);

    ctx_init_reuse(p_ctx);
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);


    // will create a LUN if required
    p_ctx->lun_size=p_ctx->chunk_size;
    if ( LUN_VIRTUAL == lun_type  || 11 == lun_type )
    {
        io=1;
        rc=ioctl_dk_capi_uvirtual(p_ctx);
        CHECK_RC(errno, "ioctl_dk_capi_uvirtual failed");
    }
    else if ( LUN_DIRECT == lun_type || 12 == lun_type )
    {
        io=1;
        rc=ioctl_dk_capi_udirect(p_ctx);
        CHECK_RC(errno, "ioctl_dk_capi_udirect failed");
    }
    else if ( 3 == lun_type  )
    {
        rc=ioctl_dk_capi_uvirtual(p_ctx);
        CHECK_RC(errno, "ioctl_dk_capi_uvirtual failed");
        rc=ioctl_dk_capi_release(p_ctx);
        CHECK_RC(errno, "ioctl_dk_capi_release failed");
    }
    else if ( 10  == lun_type )
    {
        io=0;
        rc=ioctl_dk_capi_uvirtual(p_ctx);
        CHECK_RC(errno, "ioctl_dk_capi_uvirtual failed");
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

    }

    debug("%d:----------- Start DK_CAPI_ATTACH with REUSE flag ----------\n", pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64"\n",
          pid,p_ctx_1->dev,p_ctx_1->fd,p_ctx_1->version,p_ctx_1->flags);
    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
          p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);
#ifdef _AIX
    p_ctx_1->context_id=p_ctx->context_id;
    capi_attach.ctx_token=p_ctx->context_id;
    capi_attach.flags = DK_AF_REUSE_CTX;
    rc = ioctl(p_ctx_1->fd, DK_CAPI_ATTACH, &capi_attach);
#else
    p_ctx_1->context_id=p_ctx->context_id;
    capi_attach.context_id=p_ctx->context_id;
    capi_attach.hdr.flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
    rc = ioctl(p_ctx_1->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
    debug("%d:... DK_CAPI_ATTACH called with REUSE flag...\n", pid);
    if (rc)
    {
        CHECK_RC(errno, "DK_CAPI_ATTACH with REUSE flag failed");
    }

    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
          p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);

    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx_1->adap_fd,p_ctx_1->return_flags);
    debug("%d:------------- End DK_CAPI_ATTACH with REUSE flag -------------\n", pid);


    p_ctx_1->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
    p_ctx_1->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
    p_ctx_1->context_id = capi_attach.ctx_token;
    p_ctx_1->last_phys_lba = capi_attach.last_phys_lba;
    p_ctx_1->chunk_size = capi_attach.chunk_size;
    //get max_xfer
    rc = ioctl(p_ctx_1->fd, IOCINFO, &iocinfo);
    if (rc)
    {
        CHECK_RC(errno, "Iocinfo failed with errno\n");
    }
    //p_ctx_1->max_xfer = iocinfo.un.capi_io.max_transfer;
    //TBD
    p_ctx_1->max_xfer = iocinfo.un.scdk64.lo_max_request;
    if (iocinfo.flags  & DF_LGDSK)
    {
        p_ctx_1->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
    }
    p_ctx_1->return_flags = capi_attach.return_flags;
    p_ctx_1->block_size = capi_attach.block_size;
#else
    p_ctx_1->p_host_map = mmap(NULL,p_ctx_1->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED, //p_ctx->adap_fd,0);
                               capi_attach.adap_fd,0);
    if (p_ctx_1->p_host_map == MAP_FAILED)
    {
        fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                p_ctx_1->mmio_size, errno);
        CHECK_RC(1,"mmap failed");
    }
    p_ctx_1->context_id = capi_attach.context_id;
    p_ctx_1->last_phys_lba = capi_attach.last_lba;
    p_ctx_1->max_xfer = capi_attach.max_xfer;
    p_ctx_1->chunk_size = NUM_BLOCKS;
    p_ctx_1->return_flags = capi_attach.hdr.return_flags;
    p_ctx_1->block_size = capi_attach.block_size;
#endif

    //default rwbuff handling 4K, Lorge trasnfer handled exclusive
    p_ctx_1->blk_len = BLOCK_SIZE/p_ctx_1->block_size;
    p_ctx_1->adap_fd = p_ctx->adap_fd;//capi_attach.adap_fd;

    p_ctx_1->ctx_hndl = CTX_HNDLR_MASK & p_ctx_1->context_id;
    p_ctx_1->unused_lba = p_ctx_1->last_phys_lba +1;

    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
          p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);

    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx_1->adap_fd,p_ctx_1->return_flags);
    debug("%d:------------- End DK_CAPI_ATTACH -------------\n", pid);
    if ( 11  == lun_type || 12 == lun_type )
    {
        io=0;
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

    }
    // doing I/O
    if ( io == 1  )
        rc = do_io(p_ctx, 0x1000);
    pthread_cancel(thread);
    return rc;
}
void handleSignal(int sigNo)
{
    debug("\n%d:****** Signal Handler called ******\n",pid);

    switch (sigNo)
    {
        case SIGSEGV :
            debug("\n%d:------- caught SIGSEGV --------\n\n",pid);
            siglongjmp(sBuf, 1);
            break;
        case SIGABRT:
            debug("\n%d:------- caught SIGABRT --------\n\n",pid);
            siglongjmp(sBuf, 1);
            break;
        default :
            debug("\n\n%d:FAIL------- No a expected Signal -----\n",pid);
            g_error=255;
            break;
    }

    return ;
}
void sig_handle(int sig)
{
    printf("-------- Sig %d recieved ............\n", sig);
    printf("exiting from process %d gracefully... \n",getpid());
    exit(10);
}

int do_write_or_read(struct ctx *p_ctx, __u64 stride, int do_W_R)
{
    __u64 st_lba=p_ctx->st_lba;
    int i;
    int rc = 0;

    if ( do_W_R == 1 )
    {
        debug("%d: WRITE Ops\n", pid);
    }
    else
    {
        debug("%d: READ & DATA COMPARE Ops\n", pid);
    }

    if ( do_W_R == 1 )
    {
        rc = send_write(p_ctx, st_lba, stride, pid);
        CHECK_RC(rc, "send_write failed");
        force_dump=1;
        for (i=0;i<NUM_CMDS;i++)
            hexdump(&p_ctx->wbuf[i][0],0x20,"Write buf");
        force_dump=0;

    }
    else
    {
        rc = send_read(p_ctx, st_lba, stride);
        CHECK_RC(rc, "send_read failed");
        force_dump=1;
        for (i=0;i<NUM_CMDS;i++)
            hexdump(&p_ctx->rbuf[i][0],0x20,"Read buf");

        rc = rw_cmp_buf(p_ctx, st_lba);
        if (rc)
        {
            fprintf(stderr,"buf cmp failed for lba 0X%"PRIX64",rc =%d\n",
                    st_lba,rc);
        }
    }

    force_dump=0;
    return rc;
}

int allocate_buf(struct rwlargebuf *rwbuf, __u64 size)
{
    int i;
    for (i=0; i <NUM_CMDS; i++)
    {
        rwbuf->wbuf[i] = (char *)malloc(size);
        rwbuf->rbuf[i] = (char *)malloc(size);
        if ( rwbuf->wbuf[i] == NULL || rwbuf->rbuf[i] == NULL)
            return -1;
        //strncpy(rwbuf->rbuf[i], rwbuf->wbuf[i], size);
    }
    return 0;
}

void deallocate_buf(struct rwlargebuf *rwbuf)
{
    int i;
    for (i=0; i <NUM_CMDS; i++)
    {
        free(rwbuf->wbuf[i]);
        free(rwbuf->rbuf[i]);
    }
}

int ioctl_dk_capi_attach_reuse_all_disk( )
{
    int rc=0,i;
    struct ctx *new_ctx[10],myctx[10] ;
    int cfdisk;
    struct flash_disk disks[MAX_FDISK];  // flash disk struct

    cfdisk = get_flash_disks(disks, FDISKS_SAME_ADPTR );
    pid = getpid();

#ifdef _AIX
    struct dk_capi_attach capi_attach;
    struct devinfo iocinfo;
    memset(&iocinfo, 0, sizeof(iocinfo));
#else
    struct dk_cxlflash_attach capi_attach;
#endif
    memset(&capi_attach, 0, sizeof(capi_attach));
    for ( i=0; i<cfdisk; i++ )
    {
        new_ctx[i]=&myctx[i];
        memset(new_ctx[i], 0, sizeof(struct ctx));
    }


    for ( i=0;i<cfdisk;i++)
    {

        debug("ATTACH NO. %d ************* out of %d--- disk name = %s\n", i, cfdisk,disks[i].dev );
        strcpy(new_ctx[i]->dev,disks[i].dev);
        new_ctx[i]->fd = open_dev(disks[i].dev, O_RDWR);  //Hoping to open second disk
        if (new_ctx[i]->fd < 0)
        {
            fprintf(stderr, "open() failed: device %s, errno %d\n", disks[i].dev, errno);
            g_error = -1;
        }

        if ( i == 0 )
        {
            //thread to handle AFU interrupt & events
            //pthread_create(&thread, NULL, ctx_rrq_rx, new_ctx[i]);
#ifdef _AIX
            rc = ioctl_dk_capi_query_path(new_ctx[i]);
            CHECK_RC(rc, "DK_CAPI_QUERY_PATH failed");
#endif
        }

#ifdef _AIX
        new_ctx[i]->work.num_interrupts = 5;
#else
        new_ctx[i]->work.num_interrupts  = cflash_interrupt_number();
        //TBD for linux
#endif

        if ( i == 0 )
        {
#ifdef _AIX
            capi_attach.version = new_ctx[i]->version;
            capi_attach.flags = DK_AF_ASSIGN_AFU;
            capi_attach.devno = new_ctx[i]->devno;
            debug("%d:devno=0X%"PRIX64"\n",pid,new_ctx[i]->devno);
#else
            capi_attach.hdr.version = new_ctx[i]->version;
            capi_attach.hdr.flags = DK_AF_ASSIGN_AFU;
#endif
            capi_attach.num_interrupts = new_ctx[i]->work.num_interrupts;

#ifdef _AIX
            rc = ioctl(new_ctx[i]->fd, DK_CAPI_ATTACH, &capi_attach);
#else
            rc = ioctl(new_ctx[i]->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
            debug("%d:...First  DK_CAPI_ATTACH called ...\n", pid);
            if (rc)
            {
                CHECK_RC(errno, "FIRST DK_CAPI_ATTACH failed");
            }
        }
        else
        {
#ifdef _AIX
            new_ctx[i]->context_id=new_ctx[0]->context_id;
            new_ctx[i]->flags = capi_attach.flags =  DK_AF_REUSE_CTX;
            debug("%d:----------- Start DK_CAPI_ATTACH with REUSE flag ----------\n", pid);
            debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64"\n",
                  pid,new_ctx[i]->dev,new_ctx[i]->fd,new_ctx[i]->version,new_ctx[i]->flags);
            rc = ioctl(new_ctx[i]->fd, DK_CAPI_ATTACH, &capi_attach);
#else
            new_ctx[i]->context_id=new_ctx[0]->context_id;
            capi_attach.hdr.flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
            rc = ioctl(new_ctx[i]->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
            debug("%d:... DK_CAPI_ATTACH called with REUSE flag...\n", pid);
            if (rc)
            {
                CHECK_RC(errno, "DK_CAPI_ATTACH with REUSE flag failed");
            }
        }

        debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
              pid,new_ctx[i]->p_host_map,new_ctx[i]->mmio_size,new_ctx[i]->context_id,
              new_ctx[i]->last_phys_lba,new_ctx[i]->block_size,new_ctx[i]->chunk_size,new_ctx[i]->max_xfer);

        debug("%d:------------- End DK_CAPI_ATTACH with REUSE flag -------------\n", pid);


        new_ctx[i]->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
        new_ctx[i]->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
        new_ctx[i]->context_id = capi_attach.ctx_token;
        new_ctx[i]->last_phys_lba = capi_attach.last_phys_lba;
        new_ctx[i]->chunk_size = capi_attach.chunk_size;
        //get max_xfer
        rc = ioctl(new_ctx[i]->fd, IOCINFO, &iocinfo);
        if (rc)
        {
            CHECK_RC(errno, "Iocinfo failed with errno\n");
        }
        //new_ctx[i]->max_xfer = iocinfo.un.capi_io.max_transfer;
        //TBD
        new_ctx[i]->max_xfer = iocinfo.un.scdk64.lo_max_request;
        if (iocinfo.flags  & DF_LGDSK)
        {
            new_ctx[i]->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
        }
        new_ctx[i]->return_flags = capi_attach.return_flags;
        new_ctx[i]->block_size = capi_attach.block_size;
#else
        new_ctx[i]->p_host_map = mmap(NULL,new_ctx[i]->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED,
                                      capi_attach.adap_fd,0);
        if (new_ctx[i]->p_host_map == MAP_FAILED)
        {
            fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                    new_ctx[i]->mmio_size, errno);
            CHECK_RC(1,"mmap failed");
        }
        new_ctx[i]->context_id = capi_attach.context_id;
        new_ctx[i]->last_phys_lba = capi_attach.last_lba;
        new_ctx[i]->max_xfer = capi_attach.max_xfer;
        new_ctx[i]->chunk_size = NUM_BLOCKS;
        new_ctx[i]->return_flags = capi_attach.hdr.return_flags;
        new_ctx[i]->block_size = capi_attach.block_size;
#endif

        //default rwbuff handling 4K, Lorge trasnfer handled exclusive
        new_ctx[i]->blk_len = BLOCK_SIZE/new_ctx[i]->block_size;
        new_ctx[i]->adap_fd = new_ctx[i]->adap_fd;//capi_attach.adap_fd;

        new_ctx[i]->ctx_hndl = CTX_HNDLR_MASK & new_ctx[i]->context_id;
        new_ctx[i]->unused_lba = new_ctx[i]->last_phys_lba +1;

        debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
              pid,new_ctx[i]->p_host_map,new_ctx[i]->mmio_size,new_ctx[i]->context_id,
              new_ctx[i]->last_phys_lba,new_ctx[i]->block_size,new_ctx[i]->chunk_size,new_ctx[i]->max_xfer);

        debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,new_ctx[i]->adap_fd,new_ctx[i]->return_flags);
        debug("%d:------------- End DK_CAPI_ATTACH -------------\n", pid);

    }
    //ctx_close(new_ctx[0]);  crash @ fp_ioctl+000080c
    ioctl_dk_capi_detach(new_ctx[0]);
    for ( i=0;i<cfdisk;i++)
    {
        //      printf("closing %s and fd=%d\n",new_ctx[i]->dev,new_ctx[i]->fd);
        close(new_ctx[i]->fd);
        //      printf("closing done %s and fd=%d\n",new_ctx[i]->dev,new_ctx[i]->fd);
    }
    return rc;
}

int ioctl_dk_capi_attach_reuse_loop(struct ctx *p_ctx,struct ctx *p_ctx_1 )
{
    int rc,i;
    struct ctx u_ctx,u_ctx_1;
    struct ctx *temp=&u_ctx,*temp1=&u_ctx_1 ;

#ifdef _AIX
    struct dk_capi_attach capi_attach;
    struct devinfo iocinfo;
    memset(&iocinfo, 0, sizeof(iocinfo));
#else
    struct dk_cxlflash_attach capi_attach;
#endif
    memset(&capi_attach, 0, sizeof(capi_attach));
    // taking temp
    temp->fd=p_ctx->fd;
    strcpy(temp->dev,p_ctx->dev);
    temp1->fd=p_ctx_1->fd;
    strcpy(temp1->dev,p_ctx_1->dev);
    p_ctx->flags = DK_AF_ASSIGN_AFU;
    debug("%d:----------- Start First  DK_CAPI_ATTACH ----------\n", pid);
    debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64"\n",
          pid,p_ctx->dev,p_ctx->fd,p_ctx->version,p_ctx->flags);
    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer);
    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx->adap_fd,p_ctx->return_flags);
#ifdef _AIX
    capi_attach.version = p_ctx->version;
    capi_attach.flags = p_ctx->flags;
    capi_attach.devno = p_ctx->devno;
    debug("%d:devno=0X%"PRIX64"\n",pid,p_ctx->devno);
#else
    capi_attach.hdr.version = p_ctx->version;
    capi_attach.hdr.flags = p_ctx->flags;
#endif
    capi_attach.num_interrupts = p_ctx->work.num_interrupts;

#ifdef _AIX
    rc = ioctl(p_ctx->fd, DK_CAPI_ATTACH, &capi_attach);
#else
    rc = ioctl(p_ctx->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
    debug("%d:...First  DK_CAPI_ATTACH called ...\n", pid);
    if (rc)
    {
        CHECK_RC(errno, "FIRST DK_CAPI_ATTACH failed");
    }

    p_ctx->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
    p_ctx->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
    p_ctx->context_id = capi_attach.ctx_token;
    p_ctx->last_phys_lba = capi_attach.last_phys_lba;
    p_ctx->chunk_size = capi_attach.chunk_size;
    //get max_xfer
    rc = ioctl(p_ctx->fd, IOCINFO, &iocinfo);
    if (rc)
    {
        CHECK_RC(errno, "Iocinfo failed with errno\n");
    }
    //p_ctx->max_xfer = iocinfo.un.capi_io.max_transfer;
    //TBD
    p_ctx->max_xfer = iocinfo.un.scdk64.lo_max_request;
    if (iocinfo.flags  & DF_LGDSK)
    {
        p_ctx->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
    }
    p_ctx->return_flags = capi_attach.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#else
    p_ctx->p_host_map = mmap(NULL,p_ctx->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED,
                             capi_attach.adap_fd,0);
    if (p_ctx->p_host_map == MAP_FAILED)
    {
        fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                p_ctx->mmio_size, errno);
        CHECK_RC(1,"mmap failed");
    }
    p_ctx->context_id = capi_attach.context_id;
    p_ctx->last_phys_lba = capi_attach.last_lba;
    p_ctx->max_xfer = capi_attach.max_xfer;
    p_ctx->chunk_size = NUM_BLOCKS;
    p_ctx->return_flags = capi_attach.hdr.return_flags;
    p_ctx->block_size = capi_attach.block_size;
#endif

    //default rwbuff handling 4K, Lorge trasnfer handled exclusive
    p_ctx->blk_len = BLOCK_SIZE/p_ctx->block_size;
    p_ctx->adap_fd = capi_attach.adap_fd;

    p_ctx->ctx_hndl = CTX_HNDLR_MASK & p_ctx->context_id;
    p_ctx->unused_lba = p_ctx->last_phys_lba +1;

    debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
          pid,p_ctx->p_host_map,p_ctx->mmio_size,p_ctx->context_id,
          p_ctx->last_phys_lba,p_ctx->block_size,p_ctx->chunk_size,p_ctx->max_xfer);

    debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx->adap_fd,p_ctx->return_flags);
    debug("%d:------------- End FIRST  DK_CAPI_ATTACH -------------\n", pid);


    for ( i = 1; i < 10 ; i++ )
    {
        debug("************************************%d**************************\n",i);
        // detach the REUSED CONTEXT
        if ( i%2==0 )
        {
            strcpy(p_ctx->dev,temp->dev);
            p_ctx->fd=temp->fd;
            rc = ioctl_dk_capi_detach(p_ctx);
            CHECK_RC(rc,"dk_capi_detach on reuse flag failed\n");
            if ( rc == 0 )
            {
                // try creating VLUN on another fd
                p_ctx_1->flags=DK_UVF_ALL_PATHS;
                p_ctx_1->lun_size=p_ctx_1->chunk_size;
                rc=ioctl_dk_capi_uvirtual(p_ctx_1);
                CHECK_RC(rc,"dk_capi_detach on reuse flag failed\n");
                                //do_io();
                rc=ioctl_dk_capi_release(p_ctx_1);
                CHECK_RC(rc,"dk_capi_release on reuse ctx failed\n");
            }
            strcpy(p_ctx_1->dev,temp->dev);
            p_ctx_1->fd=temp->fd;
        }
        if ( i>2 && i%2==1 )
        {
            strcpy(p_ctx_1->dev,temp1->dev);
            p_ctx_1->fd=temp1->fd;
            rc = ioctl_dk_capi_detach(p_ctx_1);
            CHECK_RC(rc,"dk_capi_detach on reuse flag failed\n");
            if ( rc == 0 )
            {
                p_ctx->flags=DK_UVF_ALL_PATHS;
                // try creating VLUN on another fd
                p_ctx->lun_size=p_ctx->chunk_size;
                rc=ioctl_dk_capi_uvirtual(p_ctx);
                CHECK_RC(rc,"dk_capi_detach on reuse flag failed\n");
                                //do_io();
                rc=ioctl_dk_capi_release(p_ctx);
                CHECK_RC(rc,"dk_capi_release on reuse ctx failed\n");
            }
        }




        debug("%d:----------- Start DK_CAPI_ATTACH with REUSE flag ----------\n", pid);
        debug("%d:dev=%s fd=%d Ver=%u flags=0X%"PRIX64"\n",
              pid,p_ctx_1->dev,p_ctx_1->fd,p_ctx_1->version,p_ctx_1->flags);
        debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
              pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
              p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);

#ifdef _AIX
        p_ctx_1->devno=p_ctx->devno;
        p_ctx_1->context_id=p_ctx->context_id;
        capi_attach.flags = DK_AF_REUSE_CTX;
        rc = ioctl(p_ctx_1->fd, DK_CAPI_ATTACH, &capi_attach);
#else
        p_ctx_1->context_id=p_ctx->context_id;
        capi_attach.hdr.flags = DK_CXLFLASH_ATTACH_REUSE_CONTEXT;
        rc = ioctl(p_ctx_1->fd, DK_CXLFLASH_ATTACH, &capi_attach);
#endif
        debug("%d:... DK_CAPI_ATTACH called with REUSE flag...\n", pid);
        if (rc)
        {
            CHECK_RC(errno, "DK_CAPI_ATTACH with REUSE flag failed");
        }

        debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
              pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
              p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);

        debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx_1->adap_fd,p_ctx_1->return_flags);
        debug("%d:------------- End DK_CAPI_ATTACH with REUSE flag -------------\n", pid);


        p_ctx_1->mmio_size = capi_attach.mmio_size;
#ifdef _AIX
        p_ctx_1->p_host_map =(volatile struct sisl_host_map *)capi_attach.mmio_start;
        p_ctx_1->context_id = capi_attach.ctx_token;
        p_ctx_1->last_phys_lba = capi_attach.last_phys_lba;
        p_ctx_1->chunk_size = capi_attach.chunk_size;
        //get max_xfer
        rc = ioctl(p_ctx_1->fd, IOCINFO, &iocinfo);
        if (rc)
        {
            CHECK_RC(errno, "Iocinfo failed with errno\n");
        }
        //p_ctx_1->max_xfer = iocinfo.un.capi_io.max_transfer;
        //TBD
        p_ctx_1->max_xfer = iocinfo.un.scdk64.lo_max_request;
        if (iocinfo.flags  & DF_LGDSK)
        {
            p_ctx_1->max_xfer |= (uint64_t)(iocinfo.un.scdk64.hi_max_request << 32);
        }
        p_ctx_1->return_flags = capi_attach.return_flags;
        p_ctx_1->block_size = capi_attach.block_size;
#else
        p_ctx_1->p_host_map = mmap(NULL,p_ctx_1->mmio_size,PROT_READ|PROT_WRITE, MAP_SHARED, //p_ctx->adap_fd,0);
                                   capi_attach.adap_fd,0);
        if (p_ctx_1->p_host_map == MAP_FAILED)
        {
            fprintf(stderr,"map failed for 0x%lx mmio_size %d errno\n",
                    p_ctx_1->mmio_size, errno);
            CHECK_RC(1,"mmap failed");
        }
        p_ctx_1->context_id = capi_attach.context_id;
        p_ctx_1->last_phys_lba = capi_attach.last_lba;
        p_ctx_1->max_xfer = capi_attach.max_xfer;
        p_ctx_1->chunk_size = NUM_BLOCKS;
        p_ctx_1->return_flags = capi_attach.hdr.return_flags;
        p_ctx_1->block_size = capi_attach.block_size;
#endif

        //default rwbuff handling 4K, Lorge trasnfer handled exclusive
        p_ctx_1->blk_len = BLOCK_SIZE/p_ctx_1->block_size;
        p_ctx_1->adap_fd = p_ctx->adap_fd;//capi_attach.adap_fd;

        p_ctx_1->ctx_hndl = CTX_HNDLR_MASK & p_ctx_1->context_id;
        p_ctx_1->unused_lba = p_ctx_1->last_phys_lba +1;

        debug("%d:mmio=%p mmio_size=0X%"PRIX64" ctx_id=0X%"PRIX64" last_lba=0X%"PRIX64" block_size=0X%"PRIX64" chunk_size=0X%"PRIX64" max_xfer=0X%"PRIX64"\n",
              pid,p_ctx_1->p_host_map,p_ctx_1->mmio_size,p_ctx_1->context_id,
              p_ctx_1->last_phys_lba,p_ctx_1->block_size,p_ctx_1->chunk_size,p_ctx_1->max_xfer);

        debug("%d:adap_fd=%d return_flag=0X%"PRIX64"\n",pid,p_ctx_1->adap_fd,p_ctx_1->return_flags);
        debug("%d:------------- End DK_CAPI_ATTACH -------------\n", pid);

    }

    return rc;
}

int set_spio_mode()
{
    int count =0;
    int rc=0;
    int i=0;
    FILE *fptr;
    char *p_file;
    char buf[64];
    char cmdstr[1024];

#ifndef _AIX
    const char *cmd ="/opt/ibm/capikv/bin/cxlfstatus | grep legacy \
                        | awk '{print $NF}' | sort -u > /tmp/idlist";
#else
    const char *cmd=NULL;
    return 0;
#endif

    rc = system(cmd);
    if (rc) return 0;

    debug("%d: List of all capi disks ids present in /tmp/idlist\n", pid);

    p_file="/tmp/idlist";
    fptr = fopen(p_file, "r");
    if (NULL == fptr)
    {
        fprintf(stderr,"%d: --------------------------------------------------------\n", pid);
        fprintf(stderr,"%d: Error opening file %s\n", pid, p_file);
        fprintf(stderr,"%d: --------------------------------------------------------\n", pid);
        return 0;
    }
    while (fgets(buf,64, fptr) != NULL)
    {
        while (i < 64)
        {
            if (buf[i] =='\n')
            {
                buf[i]='\0';
                break;
            }
            i++;
        }

        sprintf(cmdstr,"/opt/ibm/capikv/bin/cxlfsetlunmode %s 1",buf);
        rc |= system(cmdstr);

        sprintf(cmdstr,"grep -q %s /opt/ibm/capikv/etc/sioluntable || echo %s >> /opt/ibm/capikv/etc/sioluntable", buf, buf);
        rc |= system(cmdstr);

        rc |= system("/opt/ibm/capikv/bin/cxlfrefreshluns");

        count++;
    }

    if (rc) fprintf(stderr,"%d: Error while setting spio mode\n", pid);

    fclose(fptr);
    return count;
}

int keep_doing_eeh_test(struct ctx *p_ctx)
{
    int rc;
    pthread_t ioThreadId;
    pthread_t thread;
    pthread_t thread2;
    do_io_thread_arg_t ioThreadData;
    do_io_thread_arg_t * p_ioThreadData=&ioThreadData;
    p_ioThreadData->p_ctx=p_ctx;
    p_ioThreadData->stride=1;
    p_ioThreadData->loopCount=0x7fffffff; //unlimited
    while (1)
    {
#ifndef _AIX
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
#endif
        debug("%d: Things look good, start IO & wait next EEH event\n",pid);
        rc = pthread_create(&ioThreadId,NULL, do_io_thread, (void *)p_ioThreadData);
        CHECK_RC(rc, "do_io_thread() pthread_create failed");
        //Trigger EEH
        do_eeh(p_ctx);

        // Wait for IO thread to complete
        pthread_join(ioThreadId, NULL);

#ifndef _AIX
        pthread_cancel(thread);
#endif

        rc = ioctl_dk_capi_recover_ctx(p_ctx);
        CHECK_RC(rc, "ctx reattached failed");
#ifdef _AIX
        if (!(p_ctx->return_flags & DK_RF_REATTACHED))
            CHECK_RC(1, "recover ctx, expected DK_RF_REATTACHED");
        p_ctx->flags = DK_VF_HC_INQ;
        p_ctx->hint = DK_HINT_SENSE;
#endif
        ctx_reinit(p_ctx);
        usleep(1000);

#ifndef _AIX
        pthread_create(&thread2, NULL, ctx_rrq_rx, p_ctx);
#endif
        //better to use io(get failed with UA) rather than verify
        //otherwise do call verify ioctl on all paths
        rc=do_io(p_ctx,0x10000);
        if (rc == 0x2)
        {
            fprintf(stderr,"%d:expected to fail for UA, dont worry....\n",pid);
        }
        rc = ioctl_dk_capi_verify(p_ctx);
        CHECK_RC(rc, "ioctl_dk_capi_verify failed");
        usleep(1000);
   
        rc=do_io(p_ctx,0x1000);
        CHECK_RC(rc, "2nd IO request failed\n");
#ifndef _AIX
        pthread_cancel(thread2);
#endif
    }
    return 0;
}

int ctx_init_reuse(struct ctx *p_ctx)
{
    //void *map;
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    int i;
    pid_t mypid;


    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);

    for (i = 0; i < NUM_CMDS; i++)
    {
        pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
        pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
    }

    // initialize RRQ pointers
    p_ctx->p_hrrq_start = &p_ctx->rrq_entry[0];
    p_ctx->p_hrrq_end = &p_ctx->rrq_entry[NUM_RRQ_ENTRY - 1];
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;

    p_ctx->toggle = 1;

    // initialize cmd fields that never change
    for (i = 0; i < NUM_CMDS; i++)
    {
        p_ctx->cmd[i].rcb.msi = SISL_MSI_RRQ_UPDATED;
        p_ctx->cmd[i].rcb.rrq = 0x0;
        p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
    }
#ifdef _AIX
    write_64(&p_ctx->p_host_map->endian_ctrl,(__u64)SISL_ENDIAN_CTRL_BE);
#endif

    // set up RRQ in AFU
    if (rrq_c_null)
    {
        write_64(&p_ctx->p_host_map->rrq_start, (__u64)NULL);
        write_64(&p_ctx->p_host_map->rrq_end, (__u64)NULL);
    }
    else
    {
        write_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
        write_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);
    }

    mypid = getpid();
    debug("%d: ctx_init() success: p_host_map %p, ctx_hndl %d, rrq_start %p\n",
          mypid, p_ctx->p_host_map, p_ctx->ctx_hndl, p_ctx->p_hrrq_start);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
    return 0;
}

void displayBuildinfo()
{
    int i=0;
    static int entrycounter=0;
    FILE *fptr;
    char buf[1024];
    char cmd[1024];

    if ( entrycounter > 0 ) return;
    entrycounter++;

    printf("Kernel Level:\n");
    printf("-------------------------------------------\n");
    fflush(stdout);
    system("uname -a");
    system("dpkg -l | grep -w `uname -a | awk '{print $3}'|cut -f2 -d-` | grep -i linux");
    printf("\ncat /opt/ibm/capikv/version.txt:\n");
    printf("-------------------------------------------\n");
    fflush(stdout);
    system("cat /opt/ibm/capikv/version.txt");
    printf("\nAFU level:\n");
    printf("-------------------------------------------\n");
    fflush(stdout);
    system("ls /dev/cxl/afu[0-9]*.0m > /tmp/afuF");

    fptr = fopen("/tmp/afuF", "r");
    while (fgets(buf,1024, fptr) != NULL)
    {
        i=0;
        while (i < 1024)
        {
            if (buf[i] =='\n')
            {
                buf[i]='\0';
                break;
            }
            i++;
        }

        sprintf(cmd, "/opt/ibm/capikv/afu/cxl_afu_dump %s | grep Version", buf);
        system(cmd);
    }
    fclose(fptr);
    printf("-------------------------------------------\n");
    fflush(stdout);
    system("update_flash -d");
    printf("-------------------------------------------\n\n");
    fflush(stdout);
}

int allDiskToArray( char ** allDiskArrayP, int * diskCountP)
{

    int i=0;
    FILE *fptr;
    char *p_file="/tmp/flist";
    char buf[MAXBUFF];

    fptr = fopen(p_file, "r");
    if (NULL == fptr)
    {
        fprintf(stderr,"%d: --------------------------------------------------------\n", pid);
        fprintf(stderr,"%d: Error opening file %s\n", pid, p_file);

        return -1;
    }

    *diskCountP = 0;
    while (fgets(buf,MAXBUFF, fptr) != NULL)
    {
        i=0;
        while (i < MAXBUFF)
        {
            if (buf[i] =='\n')
            {
                buf[i]='\0';
                break;
            }
            i++;
        }
        allDiskArrayP[*diskCountP] = malloc(strlen(buf)+1);

        strcpy(allDiskArrayP[*diskCountP],buf);

        *diskCountP=*diskCountP+1;
    }

    return 0;
}

int diskInSameAdapater( char * p_file )
{
    int rc     =0;
#ifndef _AIX
    int iCount =0;
    int iTer   =0;

    FILE *fileP;

    char tmpBuff[MAXBUFF];
    char npBuff[MAXNP][MAXBUFF];
    char * allDiskArray [MAXBUFF];
    char sameAdapDisk[MAXNP][MAXBUFF];

    int diskCount = 0;
    int smCnt  = 0;
    int allCnt   = 0;

    const char *initCmdP = "/opt/ibm/capikv/bin/cxlfstatus | grep superpipe | awk '{print $2}' | cut -d: -f1 | sort -u  > /tmp/trashFile";

    rc = system(initCmdP);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in lspci \n",pid);
        goto xerror ;
    }

    rc = allDiskToArray(allDiskArray, &diskCount );

    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in allDiskToArray \n",pid);
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");

    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    while (fgets(tmpBuff,MAXBUFF, fileP) != NULL  && smCnt < 2 )
    {
        iTer=0;
        while (iTer < MAXBUFF)
        {
            if (tmpBuff[iTer] =='\n')
            {
                tmpBuff[iTer]='\0';
            }
            iTer++;
        }

        // reset the smCnt value

        smCnt = 0;

        for ( allCnt=0; allCnt < diskCount ;allCnt++)
        {
           sprintf(npBuff[iCount]," /opt/ibm/capikv/bin/cxlfstatus | grep %s |"
            "awk '{print $2}' | cut -d: -f1 | grep %s >/dev/null 2>&1",allDiskArray[allCnt],tmpBuff);

            rc = system(npBuff[iCount]);
            if ( rc == 0 )
            {
               strcpy(sameAdapDisk[smCnt], allDiskArray[allCnt] );
               smCnt++;
            }
        }

         iCount++;
    }

    if ( fclose(fileP) == EOF )
    {
        fprintf(stderr,"%d: Error closin the file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    if ( smCnt < 2 )
    {
        rc = -1;
        goto xerror;
    }

    for ( iCount = 0; iCount < smCnt ; iCount++ )
    {
        sprintf(tmpBuff,"echo %s >> %s", sameAdapDisk[iCount],p_file);
        rc = system(tmpBuff);
        if ( rc != 0 )
        {
            rc = EINVAL;
            goto xerror;
        }
    }

xerror:
#endif
    return rc;

}

int diskInDiffAdapater( char * p_file )
{

    int rc     =0;
#ifndef _AIX
    int iCount =0;
    int iTer   =0;

    FILE *fileP;

    char tmpBuff[MAXBUFF];
    char npBuff[MAXNP][MAXBUFF];
    char blockCheckP[MAXBUFF];
    char * allDiskArray [MAXBUFF];
    char diffAdapDisk[MAXNP][MAXBUFF];

    int diskCount = 0;
    int diffCnt  = 0;
    int allCnt   = 0;

    const char *initCmdP = "lspci -v | grep \"Processing accelerators\" | awk '{print $1}' > /tmp/trashFile";

    rc = system(initCmdP);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in lspci \n",pid);
        goto xerror ;
    }

    rc = allDiskToArray(allDiskArray, &diskCount );

    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in allDiskToArray \n",pid);
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");

    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    fileP = fopen("/tmp/trashFile", "r");

    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    while (fgets(tmpBuff,MAXBUFF, fileP) != NULL  && diffCnt < 2 )
    {
        iTer=0;
        while (iTer < MAXBUFF)
        {
            if (tmpBuff[iTer] =='\n')
            {
                tmpBuff[iTer]='\0';
            }
            iTer++;
        }

        // only supporting for scsi_generic device now
        sprintf(blockCheckP,"ls -l /sys/bus/pci/devices/"
                    "%s/pci***:**/***:**:**.*/host*/"
                    "target*:*:*/*:*:*:*/ | grep -w \"scsi_generic\" >/dev/null 2>&1",tmpBuff);
        rc = system(blockCheckP);
        if ( rc == 0 )
        {
            for ( allCnt=0; allCnt < diskCount ;allCnt++)
            {
                sprintf(npBuff[iCount],"ls -l /sys/bus/pci/devices/"
                            "%s/pci***:**/***:**:**.*/host*/"
                            "target*:*:*/*:*:*:*/scsi_generic | grep %s >/dev/null 2>&1",tmpBuff,allDiskArray[allCnt]);

                rc = system(npBuff[iCount]);
                if ( rc == 0 )
                {
                    strcpy(diffAdapDisk[diffCnt], allDiskArray[allCnt] );
                    diffCnt++;
                    break;
                }
            }

            iCount++;
        }
    }

    if ( fclose(fileP) == EOF )
    {
        fprintf(stderr,"%d: Error closin the file /tmp/trashFile \n", pid);
        rc = EINVAL ;
        goto xerror ;
    }

    if ( diffCnt < 2 )
    {
        rc = -1;
        goto xerror;
    }

    for ( iCount = 0; iCount < diffCnt ; iCount++ )
    {
        sprintf(tmpBuff,"echo %s >> %s", diffAdapDisk[iCount],p_file);
        rc = system(tmpBuff);
        if ( rc != 0 )
        {
            rc = EINVAL;
            goto xerror;
        }

    }
xerror:
#endif
    return rc;
}

#ifdef _AIX

int setRUnlimited()
{
    struct rlimit val;

    val.rlim_cur = RLIM_INFINITY;
    val.rlim_max = RLIM_INFINITY;

    if (setrlimit(RLIMIT_DATA, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);
       return 1;
    }

    if (setrlimit(RLIMIT_AS, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);
       return 1;
    }
    if (setrlimit(RLIMIT_CORE, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }
    if (setrlimit(RLIMIT_CPU, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }
    if (setrlimit(RLIMIT_FSIZE, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }
    if (setrlimit(RLIMIT_STACK, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }
    if (setrlimit(RLIMIT_RSS, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }

        if (setrlimit(RLIMIT_NOFILE, &val) != 0) {
       printf("setRlimit failed with errno=%d\n", errno);

       return 1;
    }

    return 0;
}

char * diskWithoutDev(char * source , char * destination )
{
    char temp[MC_PATHLEN];
    strcpy(temp,source);
    destination = strtok(temp,"/");
    destination = strtok(NULL,"/");
    return destination;
}


#endif

#ifndef _AIX

int turnOffTestCase( char * reqEnv)
{
    char *tmP = NULL;
    int    rc =0; 

    char *envName = (char *)getenv("FVT_ENV");

    if ( NULL != envName )
    {
       tmP = strtok( envName, "," );

       while (  tmP != NULL )
       {
          if (!strcmp(tmP, reqEnv))
          {
             rc = 1 ;
             break; 
          }      

        tmP=strtok(NULL,",");
       }
    }

    return rc;
}

int diskSizeCheck(char * diskName , float recDiskSize)
{
   float diskSize = 0.0 ;
   int rc = 0 ;
   FILE *filePtr = NULL;
   char tmBuff[1024];

   sprintf(tmBuff,"lsscsi -sg | grep %s | grep GB >/dev/null 2>&1",diskName);
   if( system( tmBuff ))
   { 
      sprintf(tmBuff," lsscsi -sg | grep %s | awk -F' ' '{print $NF}' | sed 's/TB//g'", diskName);
      filePtr = popen(tmBuff, "r");
      fscanf(filePtr, "%f", &diskSize);
      diskSize = diskSize * 1024; // convert to GB 
   }
   else
   {
      sprintf(tmBuff," lsscsi -sg | grep %s | awk -F' ' '{print $NF}' | sed 's/GB//g'", diskName);
      filePtr = popen(tmBuff, "r");
      fscanf(filePtr, "%f", &diskSize);
   }

   pclose(filePtr);

   if(  diskSize <  recDiskSize)
   {
      rc = 1; // User will be warned as disk size is less or lsscsi package is not installed
      printf(" *********WARNING : Recommended  disk size for this test %f GB and used disk size is %f"
      " OR SYSTEM does not have lsscsi package installed**********************\n",recDiskSize,diskSize);
   }

   debug("***** disk size required for this test %f GB and used disk size is %f GB****\n",recDiskSize,diskSize);

   return rc ;
}

#endif

int is_UA_device( char * diskName ) 
{
#ifndef _AIX
    int rc     =0;
    int iTer   =0;
    int iKey   =0;
    int found = FALSE ;
    FILE *fileP;
    char tmpBuff[MAXBUFF];
    char blockCheckP[MAXBUFF];

    /*
       is_UA_device: this function will look for devices which can recieve Unit attention. 
       Right now, Surelock FC is only one to get Unit attention ; however we can add more 
       device ids - lspci -v | grep -E "<ID> | <ID2>" to inform the caller of this function 
    */

    const char *initCmdP = "lspci -v | grep -E \"04cf\" | awk '{print $1}' > /tmp/adapFile";

    rc = system(initCmdP);
    if ( rc != 0)
    {
        fprintf(stderr,"%d: Failed in lspci \n",pid);
        return FALSE; 
    }

    fileP = fopen("/tmp/adapFile", "r");
    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/adapFile \n", pid);
        return FALSE ;
    }

    while (fgets(tmpBuff,MAXBUFF, fileP) != NULL)
    {
        iTer = 0;
        while (iTer < MAXBUFF)
        {
            if (tmpBuff[iTer] =='\n')
            {
                tmpBuff[iTer]='\0';
                break;
            }
            iTer++;
        }
        // only supporting for scsi_generic device

        iKey = strlen(diskName)-4 ;
        sprintf(blockCheckP,"ls -d /sys/devices/*/*/"
        "%s/*/*/host*/target*/*:*:*:*/scsi_generic/* | grep -w %s >/dev/null 2>&1",tmpBuff,&diskName[iKey]);
        rc = system(blockCheckP);
        if ( rc == 0 )
        {
           found = TRUE; 
           printf(".................... is_UA_device() : %s is UA device ........ \n", diskName);
           fclose(fileP);
           break;
        }
     }

    return found; 
#else
    //TBD: AIX related changes will be done in seprate code drop
    return TRUE;
#endif
}

#ifndef _AIX

#define CXLSTAT "/opt/ibm/capikv/bin/cxlfstatus"

int diskToWWID (char * WWID)
{
    char * d_name;
    char cmdToRun[MAXBUFF];
    int rc = 0;       
    FILE *fileP = NULL;
    int i =0;

    d_name = strrchr( cflash_path, '/') + 1;

    sprintf(cmdToRun,"%s | grep %s | awk -F \" \" '{ print $5 }' > /tmp/WWID", CXLSTAT, d_name);
    rc = system(cmdToRun);
    if (rc)
      return rc;  
     
    fileP = fopen("/tmp/WWID", "r");
    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/WWID \n", pid);
        rc = EINVAL;
        return rc;
    }

    if ( NULL == fgets(WWID,MAXBUFF, fileP) )
    {
        fprintf(stderr,"%d: Error in file /tmp/WWID \n", pid);
        rc = EINVAL;
        fclose(fileP);
        return rc;
    }

    while( i < MAXBUFF )
    {
        if ( WWID[i]=='\n' )
        {
           WWID[i]='\0';
           break;
        }

        i++;
    }

    fclose(fileP);

    return rc;
              
}

int WWIDtoDisk (char * WWID)
{
    char diskNameBuff[MAXBUFF];
    char cmdToRun[MAXBUFF];
    FILE *fileP = NULL;
    int rc = 0;
    int i =0; 
    
    sprintf(cmdToRun,"%s | grep %s | awk '{ print $1 }' | head -1 | sed -e 's/://g' > /tmp/cflash_disk", 
                    CXLSTAT, WWID);

    rc = system(cmdToRun);

    if (rc)
      return rc;  
     
    fileP = fopen("/tmp/cflash_disk", "r");
    if (NULL == fileP)
    {
        fprintf(stderr,"%d: Error opening file /tmp/cflash_disk \n", pid);
        rc = EINVAL;
        return rc;
    }

    if ( NULL == fgets(diskNameBuff,MAXBUFF, fileP) )
    {
        fprintf(stderr,"%d: Error in file /tmp/cflash_disk \n", pid);
        rc = EINVAL;
        fclose(fileP);
        return rc;
    }

    while( i < MAXBUFF )
    {
        if ( diskNameBuff[i]=='\n' )
        {
           diskNameBuff[i]='\0';
           break;
        }

        i++;
    }

    sprintf(cflash_path,"/dev/%s", diskNameBuff); 

    fclose(fileP);
    return rc;
              
}

#endif
