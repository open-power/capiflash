/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test_error.c $                             */
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
#include "mc_test.h"
#include <pthread.h>

extern char master_dev_path[MC_PATHLEN];
extern char afu_path[MC_PATHLEN];

extern pid_t pid;
extern __u64 lun_id;
extern __u64 fc_port;
extern uint8_t rc_flags;
extern int dont_displa_err_msg;

typedef int(myfunc)(int);

int child_mc_xlate_error(int cmd);
int child_mc_reg_error(int cmd);
int check_mc_null_params(int cmd);
int mc_invalid_ioarcb(int cmd);
int child_mc_size_error(int cmd);
int mc_test_inter_prcs_ctx_int(int cmd);

int test_mc_error(myfunc test1,int cmd)
{

	int rc;
	if(get_fvt_dev_env()) return -1;

	if(fork() ==0)
	{
		rc = test1(cmd);
		exit(rc);
	}
	wait(&rc);
	if (WIFEXITED(rc)) {
		rc =  WEXITSTATUS(rc);
	}
	return rc;

}

int test_mc_xlate_error(int cmd)
{
	return test_mc_error(&child_mc_xlate_error, cmd);
}
int test_mc_reg_error(int cmd)
{
	return test_mc_error(&child_mc_reg_error, cmd);
}

int test_mc_null_params(int cmd)
{
	return test_mc_error(&check_mc_null_params, cmd);
}

int test_mc_invalid_ioarcb(int cmd)
{
	return test_mc_error(&mc_invalid_ioarcb, cmd);
}

int test_mc_size_error(int cmd)
{
	return test_mc_error(&child_mc_size_error, cmd);
}
int test_mc_inter_prcs_ctx(int cmd)
{
	return test_mc_error(&mc_test_inter_prcs_ctx_int, cmd);
}
int child_mc_xlate_error(int cmd)
{
	int rc;
	struct ctx myctx;
	struct ctx *p_ctx = &myctx;
	int invalid=0;
	__u64 plba;
	__u64 size;
	mc_stat_t l_mc_stat;

	if(mc_init() !=0 ) {
	fprintf(stderr, "mc_init failed.\n");
	return -1;
    	}
	debug("mc_init success.\n");

	rc = ctx_init(p_ctx);
	if(rc != 0)
	{
		fprintf(stderr, "Context init failed, errno %d\n", errno);
		return -1;
	}	

	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
   	(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
   	if(rc != 0) {
		fprintf(stderr, "mc_register: failed. ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
		return -1;
   	}

	rc = mc_open(p_ctx->mc_hndl,MC_RDWR,&p_ctx->res_hndl);
	if(rc != 0) {
        fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
        return -1;
	}	

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");
	if(1 == cmd) //without mc_size
	{
		rc = mc_xlate_lba(p_ctx->mc_hndl, p_ctx->res_hndl, 0,&plba);
		rc = rc ? 1:0;
	}
	else
	{
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,1,&size);
		if(2 == cmd) //MCH NULL
		{
			rc = mc_xlate_lba(NULL,p_ctx->res_hndl,0,&plba);
			debug("MCH NULL rc = %d\n",rc);
			rc = rc ? 2:0;
		}
		else if(3 == cmd) //invalid RCH
		{
			rc = mc_xlate_lba(p_ctx->mc_hndl,(p_ctx->res_hndl +4),0,&plba);
			rc = rc ? 3:0;
		}
		else if(4 == cmd) //invalid VLBA
		{
			rc = mc_xlate_lba(p_ctx->mc_hndl,p_ctx->res_hndl,((1 << l_mc_stat.nmask)+5),&plba);
			rc = rc ? 4:0;
		}
		else if(5 == cmd) //NULL to plba
		{
			rc = mc_xlate_lba(p_ctx->mc_hndl,p_ctx->res_hndl,0,NULL);
			rc = rc ? 5:0;
		}
		else if(6 == cmd) //diff MCH(no mc_open) & RCH with mc_size
		{
			struct ctx tctx;
			struct ctx *p_tctx= &tctx;
			rc = ctx_init(p_tctx);
			rc = mc_register(master_dev_path, p_tctx->ctx_hndl,
			(volatile __u64 *)p_tctx->p_host_map,&p_tctx->mc_hndl);
			rc = mc_open(p_tctx->mc_hndl,MC_RDWR,&p_tctx->res_hndl);
			rc = mc_xlate_lba(p_tctx->mc_hndl,p_ctx->res_hndl,0,&plba);
			rc = rc ? 6:0;
			mc_close(p_tctx->mc_hndl,p_tctx->res_hndl);
			mc_unregister(p_tctx->mc_hndl);
			ctx_close(p_tctx);
		}
		else if(7 == cmd) //invaliud MCH
		{
			rc = mc_xlate_lba((mc_hndl_t)&invalid,p_ctx->res_hndl,0,&plba);	
			rc = rc ? 7:0;
		}
		
	}

	mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	if(8 == cmd) //after mc_close 
	{
		rc = mc_xlate_lba(p_ctx->mc_hndl,p_ctx->res_hndl,0,&plba);
		rc = rc ? 8:0;
	}
	mc_unregister(p_ctx->mc_hndl);
	ctx_close(p_ctx);
	mc_term();
	return rc;
}

int child_mc_reg_error(int cmd)
{
	int rc;
	struct ctx myctx;
	struct ctx *p_ctx = &myctx;
	__u64 *map=(__u64 *)0xabcdf;
	__u64 actual_size=0;
	__u64 stride;
	__u64 st_lba =0;
	__u64 nlba;
	mc_hndl_t new_mc_hndl, dup_mc_hndl;
	int rc1, rc2, rc3, rc4, rc5;
	pthread_t thread;
	mc_stat_t l_mc_stat;
	__u64 size = 128;

	if(mc_init() !=0 ) {
		fprintf(stderr, "mc_init failed.\n");
		return -1;
    }
	debug("mc_init success.\n");

	rc = ctx_init(p_ctx);
	if(rc != 0) {
		fprintf(stderr, "Context init failed, errno %d\n", errno);
		return -1;
	}
	pid = getpid();
	if(1 == cmd) //mc_reg with NULL MMIOP
	{
		pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				NULL,&p_ctx->mc_hndl);
		if(rc) return rc;
		rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
		if(rc) return rc;
		rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
		if(rc) return rc;
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, 2, &actual_size);
		if(rc) return rc;
		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		if(rc) return rc;
		pid = getpid();
		stride = (1 << l_mc_stat.nmask);
		st_lba = (actual_size * (1 << l_mc_stat.nmask))-1;
		rc = send_write(p_ctx, st_lba ,stride, pid, VLBA);	
		if(rc) return rc;
		rc = send_read(p_ctx, st_lba ,stride, VLBA);
		if(rc) return rc;
		rc = rw_cmp_buf(p_ctx, st_lba);
		rc = rc ? 1:0;
	}
	else if(2 == cmd) //NULL device path
	{
		rc = mc_register(NULL, p_ctx->ctx_hndl,
				(volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
		rc = rc ? 2:0;
	}
	else if(3 == cmd) //with afu_path device 
	{
		rc = mc_register(afu_path, p_ctx->ctx_hndl,
				(volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
		rc = rc ? 3:0;
	}
	else if(4 == cmd) //with invalid device  path
	{
		rc = mc_register("/dev/cxl/afu50.0m", p_ctx->ctx_hndl,
				(volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
		rc = rc ? 4:0;
	}
	else if(5 == cmd) //with invalid ctx hndl(not assigned)
	{
		debug("actual ctx hndl :%d\n", p_ctx->ctx_hndl);
		p_ctx->ctx_hndl = p_ctx->ctx_hndl + 4;
		debug("invalid ctx hndl :%d\n", p_ctx->ctx_hndl);
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
		rc = rc ? 5:0;
	}
	else if(6 == cmd) //with invalid mmap adress
	{
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)map,&p_ctx->mc_hndl);
		rc = rc ? 6:0;
	}
	else if(7 == cmd) //twice mc_reg
	{
		pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)p_ctx->p_host_map, &p_ctx->mc_hndl);
		CHECK_RC(rc, "mc_register");

		rc = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
		CHECK_RC(rc, "mc_open");

		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, size, &actual_size);
		CHECK_RC(rc, "mc_size");

		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);

		//twice mc_register on same ctx
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)p_ctx->p_host_map, &new_mc_hndl);
		//send write on 1st mc hndl
		rc1 = send_single_write(p_ctx, 0, pid);
		//do mc_size & open on old mc_reg
		rc2 = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
		rc3 = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, size, &actual_size);
		rc4 = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		rc5 = mc_hdup(p_ctx->mc_hndl, &dup_mc_hndl);
		debug("mc_hdup  rc is : %d\n", rc5);

		//now do mc_unreg on old one
		rc = mc_unregister(p_ctx->mc_hndl);
		CHECK_RC(rc, "mc_unregister");
		//do everything on new mc hndl
		p_ctx->mc_hndl = new_mc_hndl;
		rc = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
		CHECK_RC(rc, "mc_open");

		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, size, &actual_size);
		CHECK_RC(rc, "mc_size");

		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
		stride = 1 << l_mc_stat.nmask;
		for(st_lba = 0; st_lba < nlba; st_lba += (stride * NUM_CMDS)) {
			rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
			CHECK_RC(rc, "send_write");
		}
		if(rc1 && rc2 && rc3 && rc4 && rc5) {
			rc = 7;
		}
		pthread_cancel(thread);
		mc_unregister(p_ctx->mc_hndl);
	}
	else if(8 == cmd) //mc_reg twice from 2 diff process
	{

		if(fork() == 0) {//mc_reg in child process as well
			pid = getpid();
			rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)p_ctx->p_host_map, &p_ctx->mc_hndl);
			sleep(1);
			rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
			if(!rc) {
				fprintf(stderr, "%d : mc_open should fail rc = %d\n", pid, rc);
				exit(-1);
			}
			else {
				debug("%d : mc_open failed as expectd\n", pid);
			}
			rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, 2, &actual_size);
			if(!rc) {
				fprintf(stderr, "%d : mc_size should fail rc = %d\n", pid, rc);
				exit(-1);
			}
			else {
				debug("%d : mc_size failed as expectd\n", pid);
			}
			rc = rc ? 8:0;
			exit(rc);
		}
		else
		{
			sleep(1); //let child proc cal mc_reg 1str
			rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
			(volatile __u64 *)p_ctx->p_host_map, &p_ctx->mc_hndl);
			CHECK_RC(rc, "mc_register");

			pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
			rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
			CHECK_RC(rc, "mc_open");
			rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, 2, &actual_size);
			CHECK_RC(rc, "mc_mc_size");
			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
			CHECK_RC(rc, "mc_stat");

			st_lba = (actual_size * (1 << l_mc_stat.nmask))-1;
			rc += send_single_write(p_ctx, st_lba, pid);
			wait(&rc);
			pthread_cancel(thread);
			if (WIFEXITED(rc)) {
				rc =  WEXITSTATUS(rc);
				rc = rc ? 8:0;
			}
			mc_unregister(p_ctx->mc_hndl);
		}
	}

	ctx_close(p_ctx);	
	if(9 == cmd) //mc_reg with closed ctx
	{
		pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
		printf("calling mc_reg api after ctx close..\n");
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
			(volatile __u64 *)p_ctx->p_host_map, &p_ctx->mc_hndl);
		rc = rc ? 9:0;
	}

	mc_term();
	return rc;
}

void fill_send_write(struct ctx *p_ctx, __u64 vlba,
        __u64 data, __u64 stride, __u32 flags)
{
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 plba;
	int i;

	for(i = 0 ; i < NUM_CMDS; i++) {
		vlba  = i * stride;

    fill_buf((__u64*)&p_ctx->wbuf[i][0],
        sizeof(p_ctx->wbuf[i])/sizeof(__u64),data);

    memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
    p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

	if(flags & VLBA){
		p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
		p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
		p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
		write_64(p_u64, vlba); // write(16) Virtual LBA
	}else {
		p_ctx->cmd[i].rcb.lun_id = lun_id;
		p_ctx->cmd[i].rcb.port_sel = fc_port; // either FC port
		p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_HOST_WRITE;
		if(flags & NO_XLATE){
			plba = vlba;
		}
		else {
			(void)mc_xlate_lba(p_ctx->mc_hndl, p_ctx->res_hndl, vlba, &plba);
		}
		write_64(p_u64, plba); // physical LBA#
	}
    p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[0][0];

    p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0]);
    p_ctx->cmd[i].rcb.cdb[0] = 0x8A;

    p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
    write_32(p_u32, LBA_BLK);

    p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[i].sa.ioasc = 0;
	}
}
void fill_send_read(struct ctx *p_ctx, __u64 vlba,
         __u32 flags)
{
	__u64 *p_u64;
	__u32 *p_u32;
	__u64 plba;
	
	memset(&p_ctx->rbuf[0][0], 0, sizeof(p_ctx->rbuf[0]));

	memset(&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

	p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    if (flags & VLBA){
        p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
        p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
        write_64(p_u64, vlba); // Read(16) Virtual LBA
		debug("send read for vlba =0x%lX\n",vlba);

    }
    else
    {
		p_ctx->cmd[0].rcb.lun_id = lun_id;
		p_ctx->cmd[0].rcb.port_sel = fc_port; // either FC port
		p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_HOST_READ;
		if(flags & NO_XLATE){
			plba = vlba;
		}
		else {
			(void)mc_xlate_lba(p_ctx->mc_hndl, p_ctx->res_hndl, vlba, &plba);
		}
        write_64(p_u64, plba); // physical LBA#
		debug("send read for plba =0x%lX\n",plba);
    }

	p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->rbuf[0]);
	p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->rbuf[0][0];

	p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

	write_32(p_u32, LBA_BLK);

	p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[0].sa.ioasc = 0;
}
int mc_invalid_ioarcb(int cmd)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=16;
    __u64 actual_size=0;
    __u64 vlba =0;
	__u32 *p_u32;
	__u64 stride;
    pthread_t thread;
	mc_stat_t l_mc_stat;
	int i;

	rc = mc_init();
	CHECK_RC(rc, "mc_init failed");
    debug("mc_init success :%d\n",rc);

    rc = ctx_init(p_ctx);
	CHECK_RC(rc, "Context init failed");

    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
	CHECK_RC(rc, "ctx reg failed");

    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
	CHECK_RC(rc, "opening res_hndl");

    rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
	CHECK_RC(rc, "mc_size");

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");
	stride  = 1 << l_mc_stat.nmask;
	
    pid = getpid();
    vlba = (actual_size * (1 << l_mc_stat.nmask))-1;
	fill_send_write(p_ctx, vlba, pid, stride, VLBA);
	for(i = 0; i < NUM_CMDS; i++) {
	if (1 == cmd){ //invalid upcode
		debug("invalid upcode(0xFA) action = %d\n",cmd);
		p_ctx->cmd[i].rcb.cdb[0] = 0xFA;
	}else if (2 == cmd) {//EA = NULL
		debug("EA = NULL action = %d\n",cmd);
		p_ctx->cmd[i].rcb.data_ea = (__u64)NULL;
	}else if(3 == cmd){ //invalid flgas
		p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
		p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
		debug("invalid flag = 0X%X\n",p_ctx->cmd[i].rcb.req_flags);
	}else if(5 == cmd) {//SISL_AFU_RC_RHT_INVALID
		p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl + 2;
	}else if( 6 == cmd) {//SISL_AFU_RC_RHT_OUT_OF_BOUNDS
		p_ctx->cmd[i].rcb.res_hndl   = MAX_RES_HANDLE;
	}else if(7 == cmd) { //invalid address for page fault
		debug("setting EA = 0x1234 to generate error page fault\n");
		p_ctx->cmd[i].rcb.data_ea = (__u64)0x1234;
	}else if(8 == cmd) { //invalid ctx_id
		debug("%d :  sending invalid ctx id\n", pid);
		 p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl +10;
	}else if(9 == cmd) { //test flag underrun
		p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0])/2;
	}else if(10 == cmd) {// test flag overrun
		p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[0]) +2;
	}else if(11 == cmd) { //rc scsi_rc_check
		p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
		write_32(p_u32, LBA_BLK +1);
	}else if(12 == cmd) { //data len 0 in ioarcb
		p_ctx->cmd[i].rcb.data_len = 0;
	}else if(13 == cmd) { //NUM  BLK to write 0
		p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
		write_32(p_u32, 0);
	}
	}
	//send_single_cmd(p_ctx);
	send_cmd(p_ctx);
	//rc = wait_single_resp(p_ctx);
	rc = wait_resp(p_ctx);
	if( cmd >= 9 && cmd <= 13) {
		if(!rc_flags) {
			if(!dont_displa_err_msg)
				fprintf(stderr, "%d : Expecting rc flags non zero\n", pid);
			rc = -1;
		}
	}
	if(4 == cmd) {//invalid fc port & lun id
		debug("invalid fc port(0xFF)&lun id(0X1200), action=%d",cmd);
		fill_send_write(p_ctx, vlba, pid, stride, PLBA);
		for(i = 0; i < NUM_CMDS; i++) {
			p_ctx->cmd[i].rcb.lun_id = 0x12000;
			p_ctx->cmd[i].rcb.port_sel = 0xff;
		}
		//send_single_cmd(p_ctx);
		send_cmd(p_ctx);
		rc = wait_resp(p_ctx);
	}
	pthread_cancel(thread);
	mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
	mc_unregister(p_ctx->mc_hndl);
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

    if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init failed.\n");
    return -1;
        }
    debug("mc_init success.\n");

    rc = ctx_init(p_ctx);
    if(rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        return -1;
    }
    pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
    if(rc != 0)
    {
        fprintf(stderr, "ctx _reg failed, ctx_hndl %d,rc %d\n",p_ctx->ctx_hndl, rc );
        return -1;
    }

    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
    if (rc != 0) {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
    }

    rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
	CHECK_RC(rc, "mc_size");

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");
	
	pid = getpid();
	vlba = (actual_size * (1 << l_mc_stat.nmask))-1;
	fill_buf((__u64*)&p_ctx->wbuf[0][0],
		sizeof(p_ctx->wbuf[0])/sizeof(__u64),pid);

	memset(&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));
	p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

	p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
	p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
	p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
	write_64(p_u64, vlba); // write(16) Virtual LBA

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

int check_mc_null_params(int cmd)
{
	int rc;
	struct ctx myctx;
	struct ctx *p_ctx = &myctx;
	__u64 size = 10;
	__u64 actual_size=0;

	if(mc_init() !=0 ) {
		fprintf(stderr, "mc_init failed.\n");
		return -1;
    }
	debug("mc_init success.\n");

	rc = ctx_init(p_ctx);
	if(rc != 0)
	{
		fprintf(stderr, "Context init failed, errno %d\n", errno);
		return -1;
	}
	if(1 == cmd) { //mc_reg with NULL MCH
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
			(volatile __u64 *) p_ctx->p_host_map,NULL);
		rc = rc ? 1:0;
	}
	else {
		rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
			(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
		if(rc != 0) {
			fprintf(stderr, "ctx _reg failed, ctx_hndl %d,rc %d\n",
					p_ctx->ctx_hndl, rc );
			return -1;
		}
		if(2 == cmd){
			rc = mc_unregister(NULL);
			rc = rc ? 2:0;
		}
		else if(3 == cmd){  //mc_open NULL
			rc = mc_open(NULL, MC_RDWR, &p_ctx->res_hndl);
			rc = rc ? 3:0;
		}
		else if (4 == cmd) {
			rc = mc_hdup(NULL, p_ctx->mc_hndl);
			rc = rc ? 4:0;
		}
		else {
			rc = mc_open(p_ctx->mc_hndl, MC_RDWR, &p_ctx->res_hndl);
			if(5 == cmd) {
				rc = mc_close(NULL, p_ctx->res_hndl);
				rc = rc ? 5:0;
			}else if( 6 == cmd) {
				rc = mc_size(NULL, p_ctx->res_hndl, size, &actual_size);
				rc = rc ? 6:0;
			}else if(7 == cmd) {
				rc = mc_clone(NULL, p_ctx->mc_hndl, MC_RDWR);
				rc = rc ? 7:0;
			}
		}
	}
	return rc;
}

int child_mc_size_error(int cmd)
{
	int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 size=0;
	int invalid=0;

	pid = getpid();
    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");

    rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
    CHECK_RC(rc, "ctx reg failed");

    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
    CHECK_RC(rc, "opening res_hndl");

	if(1 == cmd) { //invalid MCH
		rc = mc_size((mc_hndl_t)&invalid, p_ctx->res_hndl,1,&size);
		rc = rc ? 1:0;
	} else if( 2 == cmd) { //invalid RSH
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl+20,1,&size);
		rc = rc ? 2:0;
	} else if(3 == cmd) { //NULL size
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,1, NULL);
		rc = rc ? 3:0;
	} else if(4 == cmd) { //after mc_close
		mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,1, &size);
		rc = rc ? 4:0;
	} else if(5 == cmd) { //after mc_unregister
		mc_unregister(p_ctx->mc_hndl);
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,1, &size);
		rc = rc ? 5:0;
	}
	ctx_close(p_ctx);
    mc_term();
	return rc;
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

    rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
    CHECK_RC(rc, "ctx reg failed");

    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
    CHECK_RC(rc, "mc_size");

	rc = mc_open(p_ctx->mc_hndl,MC_RDWR, res_hndl);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size(p_ctx->mc_hndl, *res_hndl, chunks, &actual_size);
    CHECK_RC(rc, "mc_size");
	return 0;
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
	if( 0 == cpid) { //child one running
		pid = getpid();
		debug("%d : child do init_mc \n", pid);
		rc = init_mc(p_ctx, &res_hndl);
		if(rc) {
			fprintf(stderr, "%d : exiting due to init_mc\n:", pid);
			exit(rc);
		}
		//do write into pipe & wait until parent kill me
		close(pdes[0]); //close read des
		write(pdes[1], &p_ctx->ctx_hndl, sizeof(ctx_hndl_t));
		while(1);
	} else { //parent
		close(pdes[1]); //close write des
		//lets child do there work & wait for me
		sleep(1);
		pid = getpid();
		rc = init_mc(p_ctx, &res_hndl);
		if(rc) {
			kill(cpid, SIGKILL);
			return rc;
		}
		pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
		read(pdes[0], &ctx_hndl, sizeof(ctx_hndl_t));
		fill_send_write(p_ctx, 0, pid, stride, VLBA);
		//set another process ctx
		debug("%d : use child(%d)process ctx hndl: %d\n", pid, cpid, ctx_hndl);
		for(i = 0; i< NUM_CMDS; i++) {
			p_ctx->cmd[i].rcb.ctx_id = ctx_hndl;
		}
		if(2 == cmd) {
			//another test is to close one of my ctx res hndl
			//and use child ctx handler here
			//(child has opened 2 res handler)
			mc_close(p_ctx->mc_hndl, res_hndl);
			debug("%d : close res_hndl(%d) but child (%d)has opened\n",
					pid, res_hndl, cpid);
			for(i = 0; i< NUM_CMDS; i++) {
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
