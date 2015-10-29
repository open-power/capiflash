/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_error.c $                         */
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
#include <signal.h>
#include <setjmp.h>

extern int MAX_RES_HANDLE;
extern char cflash_path[MC_PATHLEN];
extern sigjmp_buf sBuf;
extern pid_t pid;
extern int g_error;
extern __u64 lun_id;
extern __u64 fc_port;
extern uint8_t rc_flags;
extern int dont_displa_err_msg;
extern bool bad_address;
extern bool err_afu_intrpt;

typedef int(myfunc)(int);


int mc_invalid_ioarcb(int cmd);
int mc_test_inter_prcs_ctx_int(int cmd);

int test_mc_error(myfunc test1,int cmd)
{

    int rc;
    if (get_fvt_dev_env()) return -1;

    if (fork() ==0)
    {
        rc = test1(cmd);
        exit(rc);
    }
    wait(&rc);

    if (WIFEXITED(rc))
    {
        rc =  WEXITSTATUS(rc);
    }
    return rc;
}

int test_mc_invalid_ioarcb(int cmd)
{
    return test_mc_error(&mc_invalid_ioarcb, cmd);
}
int test_mc_inter_prcs_ctx(int cmd)
{
    return test_mc_error(&mc_test_inter_prcs_ctx_int, cmd);
}


int init_mc(struct ctx *p_ctx, res_hndl_t *res_hndl)
{
    int rc;
    __u64 chunks=16;
    __u64 actual_size=0;

    rc = mc_init();
    CHECK_RC(rc, "mc_init failed");
    debug("mc_init success :%d\n",rc);

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx,chunks, &actual_size);
    CHECK_RC(rc, "mc_size");

    *res_hndl = p_ctx->res_hndl;

    rc = create_res(p_ctx);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size1(p_ctx, chunks, &actual_size);
    CHECK_RC(rc, "mc_size");
    return 0;
}

void fill_send_write(struct ctx *p_ctx, __u64 vlba,
                      __u64 data, __u64 stride)
{
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 lba;
    int i;

    for (i = 0 ; i < NUM_CMDS; i++)
    {
        lba  = i * stride;

        fill_buf((__u64*)&p_ctx->wbuf[i][0],
                 sizeof(p_ctx->wbuf[i])/sizeof(__u64),data);

        memset((void *)&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
        p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];


        p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
        p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
        write_lba(p_u64, lba);

        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[0][0];

        p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0]);
        p_ctx->cmd[i].rcb.cdb[0] = 0x8A;

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
        write_32(p_u32, p_ctx->blk_len);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
    }
}
void fill_send_read(struct ctx *p_ctx, __u64 lba)
{
    __u64 *p_u64;
    __u32 *p_u32;

    memset(&p_ctx->rbuf[0][0], 0, sizeof(p_ctx->rbuf[0]));

    memset((void *)&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
    p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
    write_lba(p_u64, lba);
    debug("send read for vlba =0X%"PRIX64"\n",lba);

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->rbuf[0][0];

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

    write_32(p_u32, p_ctx->blk_len);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;
}

int place_bad_addresses(struct ctx *p_ctx, int action)
{
    int cnt = NUM_CMDS;
    int wait_try=MAX_TRY_WAIT;
    int p_cmd = 0;
    int i;
    __u64 room;
    __u64 baddr = 0x1234;

    /* make memory updates visible to AFU before MMIO */
    asm volatile( "lwsync" : : );
    if (2 == action)
    {
        // set up bad HRRQ address
        write_64(&p_ctx->p_host_map->rrq_start, (__u64)0x456123);
        write_64(&p_ctx->p_host_map->rrq_end, (__u64)0x895e6fe);
        bad_address= true;
    }

    if (3 == action)
    {
        //cmd_room violation
        room = read_64(&p_ctx->p_host_map->cmd_room);
        debug("%d:placing %d cmds in 0X%"PRIX64" cmd_room...\n",pid,NUM_CMDS,room);
        for (i=0;i<NUM_CMDS ;i++)
            write_64(&p_ctx->p_host_map->ioarrin,
                     (__u64)&p_ctx->cmd[i].rcb);
        bad_address= true;
        return 0;
    }
    while (cnt)
    {
        room = read_64(&p_ctx->p_host_map->cmd_room);
        if (0 == room)
        {
            usleep(MC_BLOCK_DELAY_ROOM);
            wait_try--;
        }
        if (0 == wait_try)
        {
            fprintf(stderr, "%d: send cmd wait over %d cmd remain\n",
                    pid, cnt);
            return -1;
        }
        for (i = 0; i < room; i++)
        {
            // add a usleep here if room=0 ?
            // write IOARRIN
            if (1 == action)
            {
                //bad RCB address
                write_64(&p_ctx->p_host_map->ioarrin, baddr*i);
                bad_address= true;
            }
            else
            {
                write_64(&p_ctx->p_host_map->ioarrin,
                         (__u64)&p_ctx->cmd[p_cmd++].rcb);
            }
            wait_try = MAX_TRY_WAIT; //each cmd give try max time
            if (cnt-- == 1) break;
        }
    }
    return 0;
}

int handle_bad_ioasa(struct ctx *p_ctx, __u64 data)
{
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 lba=0;
    sisl_ioarcb_t *rcb;
    rcb = (sisl_ioarcb_t *)malloc(sizeof(sisl_ioarcb_t));

    fill_buf((__u64*)&p_ctx->wbuf[0][0],
             sizeof(p_ctx->wbuf[0])/sizeof(__u64),data);

    memset(&(rcb->cdb[0]), 0, sizeof(rcb->cdb));
    p_u64 = (__u64*)&(rcb->cdb[2]);

    rcb->res_hndl = p_ctx->res_hndl;
    rcb->req_flags = SISL_REQ_FLAGS_RES_HNDL;
    rcb->req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
    write_lba(p_u64, lba);

    rcb->data_ea = (__u64) &p_ctx->wbuf[0][0];

    rcb->data_len = sizeof(p_ctx->wbuf[0]);
    rcb->cdb[0] = 0x8A;

    p_u32 = (__u32*)&(rcb->cdb[10]);
    write_32(p_u32, p_ctx->blk_len);

    asm volatile( "lwsync" : : );
    write_64(&p_ctx->p_host_map->ioarrin,(__u64)rcb);
    return 0;

}
int mc_invalid_ioarcb(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=32;
    __u64 actual_size=0;
    __u64 vlba =0;
    __u32 *p_u32;
    __u64 stride;
    __u64 *p_u64;
    pthread_t thread;
    mc_stat_t l_mc_stat;
    int i;

    pid = getpid();

    signal(SIGABRT, sig_handle);
    signal(SIGSEGV, sig_handle);
    rc = mc_init();
    CHECK_RC(rc, "mc_init failed");
    debug("mc_init success :%d\n",rc);

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    if (15 == cmd)
    {
        //PLBA out of range
        rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
        CHECK_RC(rc, "opening res_hndl");
        actual_size = (p_ctx->last_lba+1)/p_ctx->chunk_size;
    }
    else
    {
        p_ctx->flags = DK_UVF_ALL_PATHS;
        rc = create_res(p_ctx);
        CHECK_RC(rc, "opening res_hndl");
        rc = mc_size1(p_ctx,chunks, &actual_size);
        CHECK_RC(rc, "mc_size");
    }

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");
    stride  = 1 << l_mc_stat.nmask;

    vlba = (actual_size * (1 << l_mc_stat.nmask))-1;
    fill_send_write(p_ctx, vlba, pid, stride);
    for (i = 0; i < NUM_CMDS; i++)
    {
        if (1 == cmd)
        {
            //invalid upcode
            debug("invalid upcode(0xFA) action = %d\n",cmd);
            p_ctx->cmd[i].rcb.cdb[0] = 0xFA;
        }
        else if (2 == cmd)
        {
            //EA = NULL
            debug("EA = NULL action = %d\n",cmd);
            p_ctx->cmd[i].rcb.data_ea = (__u64)NULL;
#ifdef _AIX
            bad_address = true;
#endif
        }
        else if (3 == cmd)
        {
            //invalid flgas
            p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
            p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
            debug("invalid flag = 0X%X\n",p_ctx->cmd[i].rcb.req_flags);
        }
        else if (5 == cmd)
        {
            //SISL_AFU_RC_RHT_INVALID
            p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl + 2;
        }
        else if ( 6 == cmd)
        {
            //SISL_AFU_RC_RHT_OUT_OF_BOUNDS
            p_ctx->cmd[i].rcb.res_hndl   = MAX_RES_HANDLE;
        }
        else if (7 == cmd)
        {
            //invalid address for page fault
            debug("setting EA = 0x1234 to generate error page fault\n");
            p_ctx->cmd[i].rcb.data_ea = (__u64)0x1234;
#ifdef _AIX
            bad_address = true;
#endif
        }
        else if (8 == cmd)
        {
            //invalid ctx_id
            debug("%d:  sending invalid ctx id\n", pid);
            p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl +10;
        }
        else if (9 == cmd)
        {
            //test flag underrun
            p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0])/2;
        }
        else if (10 == cmd)
        {
            // test flag overrun
            p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0]);
            p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
            write_32(p_u32, 2);
        }
        else if (11 == cmd)
        {
            //rc scsi_rc_check
            p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
            write_32(p_u32, p_ctx->blk_len +1);
        }
        else if (12 == cmd)
        {
            //data len 0 in ioarcb
            p_ctx->cmd[i].rcb.data_len = 0;
        }
        else if (13 == cmd)
        {
            //NUM  BLK to write 0
            p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
            write_32(p_u32, 0);
        }
        else if ((14 == cmd) || (15 == cmd))
        {
            //test out of range LBAs
            p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
            vlba += i+1;
            write_lba(p_u64, vlba);
        }
    }

    //test BAD IOARCB, IOASA & CMD room violation
    if (cmd >= 100)
    {
        if (100 == cmd)
        {
            //bad RCB
            place_bad_addresses(p_ctx, 1);
            usleep(1000);
            if (err_afu_intrpt) //cool expected res
                rc = 100;
            else rc = -1;
            goto END;
        }
        else if (101 == cmd)
        {
            //bad IOASA
            handle_bad_ioasa(p_ctx, pid);
            usleep(1000); //sleep sometime to process rcb cmd by AFU
            //And let handle rrq event
            //how to handle error, rrq thread should throw some error
            return -1;
        }
        else if (102 == cmd)
        {
            //cmd_room violation
            place_bad_addresses(p_ctx, 3);
            usleep(1000);
#ifdef _AIX
            if (err_afu_intrpt) //cool expected res
                rc = 102;
            else rc = -1;
            goto END;
#endif
        }
        else if (103 == cmd)
        {
            //bad HRRQ
            place_bad_addresses(p_ctx, 2);
            usleep(1000);
            if (err_afu_intrpt) //cool expected res
                rc = 103;
            else rc = -1;
            goto END;
        }
    }
    else
    {
        send_cmd(p_ctx);
    }
    rc = wait_resp(p_ctx);
    if ( cmd >= 9 && cmd <= 13)
    {
        if (!rc_flags)
        {
            if (!dont_displa_err_msg)
                fprintf(stderr, "%d: Expecting rc flags non zero\n", pid);
            rc = -1;
        }
    }
    if (4 == cmd)
    {
        //invalid fc port & lun id
        debug("invalid fc port(0xFF)&lun id(0X1200), action=%d",cmd);
        fill_send_write(p_ctx, vlba, pid, stride);
        for (i = 0; i < NUM_CMDS; i++)
        {
            p_ctx->cmd[i].rcb.lun_id = 0x12000;
            p_ctx->cmd[i].rcb.port_sel = 0xff;
        }
        //send_single_cmd(p_ctx);
        send_cmd(p_ctx);
        rc = wait_resp(p_ctx);
    }
#ifdef _AIX
    if ((7 == cmd || 2 == cmd)&& (err_afu_intrpt))
        rc = 7;
#endif
END:
    pthread_cancel(thread);
    close_res(p_ctx);
    //mc_unregister(p_ctx->mc_hndl);
    //xerror:
    ctx_close(p_ctx);
    mc_term();
    return rc;
}
int test_mc_invalid_opcode()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=10;
    __u64 actual_size=0;
    __u64 vlba =0;
    __u64 *p_u64;
    __u32 *p_u32;
    mc_stat_t l_mc_stat;
    pthread_t thread;

    if (mc_init() !=0 )
    {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
    debug("mc_init success.\n");

    rc = ctx_init(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        return -1;
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    /*rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
       (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
       if(rc != 0)
       {
           fprintf(stderr, "ctx _reg failed, ctx_hndl %d,rc %d\n",p_ctx->ctx_hndl, rc );
           return -1;
       }*/

    rc = create_res(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
    }

    rc = mc_size1(p_ctx,chunks, &actual_size);
    CHECK_RC(rc, "mc_size");

    rc = mc_stat1(p_ctx, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

    pid = getpid();
    vlba = (actual_size * (1 << l_mc_stat.nmask))-1;
    fill_buf((__u64*)&p_ctx->wbuf[0][0],
             sizeof(p_ctx->wbuf[0])/sizeof(__u64),pid);

    memset((void *)&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));
    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
    write_lba(p_u64, vlba);

    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->wbuf[0][0];

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->wbuf[0]);
    p_ctx->cmd[0].rcb.cdb[0] = 0xFA; // invalid opcode

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
    write_32(p_u32, 8); // 8 LBAs for 4K

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;
    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    return rc;
}

/*
 * create two ctx process & 2 resource handler each ctx
 * use diff ctx handler in diff process, get another process
 * ctx handler through PIPE.
 */
int mc_test_inter_prcs_ctx_int(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    res_hndl_t res_hndl;
    ctx_hndl_t ctx_hndl;
    int pdes[2];
    pid_t cpid;
    pthread_t thread;
    __u64 stride = 0x1000;
    int i;
    //create pipe, child open for write
    // parent open for read

    pipe(pdes);
    cpid = fork();
    if ( 0 == cpid)
    {
        //child one running
        pid = getpid();
        debug("%d: child do init_mc \n", pid);
        rc = init_mc(p_ctx, &res_hndl);
        if (rc)
        {
            fprintf(stderr, "%d: exiting due to init_mc\n:", pid);
            exit(rc);
        }
        //do write into pipe & wait until parent kill me
        close(pdes[0]); //close read des
        write(pdes[1], &p_ctx->ctx_hndl, sizeof(ctx_hndl_t));
        while (1);
    }
    else
    {
        //parent
        close(pdes[1]); //close write des
        //lets child do there work & wait for me
        sleep(1);
        pid = getpid();
        rc = init_mc(p_ctx, &res_hndl);
        if (rc)
        {
            kill(cpid, SIGKILL);
            return rc;
        }
        pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
        read(pdes[0], &ctx_hndl, sizeof(ctx_hndl_t));
        fill_send_write(p_ctx, 0, pid, stride);
        //set another process ctx
        debug("%d: use child(%d)process ctx hndl: %d\n", pid, cpid, ctx_hndl);
        for (i = 0; i< NUM_CMDS; i++)
        {
            p_ctx->cmd[i].rcb.ctx_id = ctx_hndl;
        }
        if (2 == cmd)
        {
            //another test is to close one of my ctx res hndl
            //and use child ctx handler here
            //(child has opened 2 res handler)
            p_ctx->res_hndl = res_hndl;
            close_res(p_ctx);
            debug("%d: close res_hndl(%d) but child (%d)has opened\n",
                  pid, res_hndl, cpid);
            for (i = 0; i< NUM_CMDS; i++)
            {
                p_ctx->cmd[i].rcb.res_hndl   = res_hndl;
            }
        }
        send_cmd(p_ctx);
        rc = wait_resp(p_ctx);
        kill(cpid, SIGKILL);
        pthread_cancel(thread);
    }
    return rc;
}

int test_scsi_cmds()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunk = 16;
    pthread_t thread;
    __u64 stride = 0x10;
    __u64 nlba;
    uint8_t opcode[]={ 0x00,0xA0,0x09E,0x12,0x03,0x1B,0x5A,0x55 };
    int index;
    pid = getpid();
    rc = ctx_init(p_ctx);
    int i;
    CHECK_RC(rc, "Context init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

    p_ctx->flags = DK_UVF_ALL_PATHS;
    p_ctx->lun_size = chunk * p_ctx->chunk_size;
    rc = create_res(p_ctx);
    CHECK_RC(rc, "create_res failed");

    nlba = p_ctx->last_lba+1;
    for (index=0;index <sizeof(opcode);index++)
    {
        debug("%d:sending scsi cmd=0X%"PRIX8" ........\n",pid,opcode[index]);
        fill_send_write(p_ctx, nlba, pid, stride);
        for (i =0;i<NUM_CMDS;i++)
        {
            p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
            p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
            p_ctx->cmd[i].rcb.cdb[0] = opcode[index];
        }
        send_cmd(p_ctx);
        rc = wait_resp(p_ctx);
#ifndef _AIX
        if (rc != 0x21)
        {
            fprintf(stderr,"%d:failed rc =%d for scsi cmd=0X%"PRIX8",exptd rc=0x21\n",
                    pid,rc,opcode[index]);
            break;
        }
#endif
        debug("%d:rc =%d for scsi cmd=0X%"PRIX8" ........\n",pid,rc,opcode[index]);
        usleep(1000);
    }
    pthread_cancel(thread);
    ctx_close(p_ctx);
    return rc;
}
int test_ctx_reset()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx= &myctx;
    pthread_t thread;
    __u64 buf_size = 0x2000000; //32MB
    __u64 chunk = 10;
    __u64 stride = 0x1000;
    struct rwlargebuf rwbuf;
    int i;

    pid=getpid();
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx_init failed");
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    rc = create_resource(p_ctx,chunk*p_ctx->chunk_size,DK_UVF_ASSIGN_PATH,LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");

    //do bad EA
    if (1)
    {
        debug("%d: ........place bad EA....\n", pid);
        fill_send_write(p_ctx, 0, pid, stride);
        for (i = 0; i < NUM_CMDS; i++)
        {
            p_ctx->cmd[i].rcb.data_ea = (__u64)0x1234;
        }
        bad_address = true;
        send_cmd(p_ctx);
        rc = wait_resp(p_ctx);
        sleep(1);
        //normal IO
        bad_address = false;
        debug("%d: .........after bad EA, do normal IO....\n", pid);
        rc = do_io(p_ctx, stride);
        CHECK_RC(rc,"Normal IO failed after bad EA");

        //do bad RCB
        debug("%d: .........place bad RCB....\n", pid);
        bad_address = true;
        place_bad_addresses(p_ctx, 1);
        sleep(2);
        //normal IO
        debug("%d: ......after bad RCB, do normal IO....\n", pid);
        bad_address = false;
        rc = do_io(p_ctx, stride);
        CHECK_RC(rc,"Normal IO failed after bad RCB");
#ifdef _AIX
        system("ulimit -d unlimited");
        system("ulimit -s unlimited");
        system("ulimit -m unlimited");
#endif
    }
    //do large _transfer
    debug("%d: Do large transfer ....\n", pid);
    rc = allocate_buf(&rwbuf, buf_size);
    CHECK_RC(rc, "memory allocation failed");
    rc = do_large_io(p_ctx, &rwbuf, buf_size);
    deallocate_buf(&rwbuf);
    buf_size = 0x100000; //4k
    rc = allocate_buf(&rwbuf, buf_size);
    CHECK_RC(rc, "memory allocation failed");
    //normal io
    debug("%d: after large transfer,do normal IO ....\n", pid);
    rc = do_io(p_ctx, 0x10000);
    //rc = do_large_io(p_ctx, &rwbuf, buf_size);
    CHECK_RC(rc,"Normal IO failed after large transfer");

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}

int test_fc_port_reset_vlun()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    int ioCounter=0;
    __u64 nlba;
    __u64 stride=0x1;

    pid = getpid();
#ifdef _AIX
    memset(p_ctx, 0, sizeof(myctx));
    strcpy(p_ctx->dev, cflash_path);
    if ((p_ctx->fd =open_dev(p_ctx->dev, O_RDWR)) < 0)
    {
        fprintf(stderr,"open %s failed, errno=%d\n",p_ctx->dev,errno);
        return -1;
    }
    rc = ioctl_dk_capi_query_path(p_ctx);
    CHECK_RC(rc,"dk_capi_query_path failed..\n");
    rc = ctx_init_internal(p_ctx, 0,p_ctx->devno);
#else
    rc = ctx_init(p_ctx);
#endif
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

    nlba = 1 * (p_ctx->chunk_size);
    rc = create_resource(p_ctx, nlba, 0, LUN_VIRTUAL);
    CHECK_RC(rc, "create LUN_VIRTUAL failed");
    rc = compare_size(p_ctx->last_lba, nlba-1);
    CHECK_RC(rc, "failed compare_size");

    debug("-- Going to start IO.Please do chportfc -reset <pnum> at texan --\n");

    debug("rc=%d,g_error=%d\n",rc,g_error);
    do
    {
        rc = do_io(p_ctx, stride);
        if (rc !=0 )
        {
            debug("rc=%d,ioCounter=%d,IO failed..... \n",rc,ioCounter);
            if ( ioCounter==1 )
            {
                debug("rc=%d, Going to verify.... \n",rc);

                p_ctx->flags=DK_VF_LUN_RESET;
#ifdef _AIX
                p_ctx->hint = DK_HINT_SENSE;
#else
                p_ctx->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif
                rc = ioctl_dk_capi_verify(p_ctx);
                CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");
            }
            else
            { if (ioCounter > 1)
                {
                    rc=-1; // IO failed third time
                    break;
                }
            }

        }
        else
        {
            debug("rc=%d,IO succeeded \n",rc);
            g_error=0;
        }

        ioCounter++;
        rc|=g_error;
        sleep(3);

    } while ( rc !=0);

    debug("rc=%d,g_error=%d\n",rc,g_error);

    if ( ioCounter <= 1)
    {
        debug("WARNING: Test case not excuted properly... Please rerun\n");
        rc =255;
    }

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    rc |= g_error;
    return rc;
}

int test_fc_port_reset_plun()
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    __u64 stride= 0x100;
    int ioCounter=0;

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

    debug("-- Going to start IO.Please do chportfc -reset <pnum> at texan --\n");
    do
    {

        rc = do_io(p_ctx, stride);
        if (rc !=0 )
        {
            debug("rc=%d,ioCounter=%d,IO failed..... \n",rc,ioCounter);
            if ( ioCounter==1 )
            {
                debug("rc=%d, Going to verify.... \n",rc);

                p_ctx->flags=DK_VF_LUN_RESET;
#ifdef _AIX
                p_ctx->hint = DK_HINT_SENSE;
#else
                p_ctx->hint = DK_CXLFLASH_VERIFY_HINT_SENSE;
#endif
                rc = ioctl_dk_capi_verify(p_ctx);
                CHECK_RC(rc, "ioctl_dk_capi_verify failed\n");
            }
            else
            { if (ioCounter > 1)
                {
                    rc=-1; // IO failed third time
                    break;
                }
            }

        }
        else
        {
            debug("rc=%d,IO succeeded \n",rc);
            g_error=0;
        }

        ioCounter++;
        rc|=g_error;
        sleep(3);

    } while ( rc !=0);

    debug("rc=%d,g_error=%d\n",rc,g_error);

    if ( ioCounter <= 1)
    {
        debug("WARNING: Test case not excuted properly... Please rerun\n");
        rc =255;
    }

    pthread_cancel(thread);
    close_res(p_ctx);
    ctx_close(p_ctx);
    return rc;
}
