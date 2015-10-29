/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test_io.c $                                */
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
#include <sys/ipc.h>
#include <sys/shm.h>

extern char master_dev_path[MC_PATHLEN];
extern char afu_path[MC_PATHLEN];

extern pid_t pid;
extern int dont_displa_err_msg;

static pthread_mutex_t mutex;
static pthread_cond_t condv;

static int g_error;
static __u32 W_READ = 0;
static __u32 W_WRITE = 0;
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
	mc_stat_t l_mc_stat;
	int rc;
	
	pid = getpid();
	pthread_mutex_lock(&mutex);
	rc = mc_stat(p_ctx->mc_hndl,p_ctx->res_hndl,&l_mc_stat);
	pthread_mutex_unlock(&mutex);
	if(rc) {
		g_error = rc;
		fprintf(stderr, "%d : %s : %d : mc_stat failed\n",
		pid, __func__, __LINE__);
		return NULL;
	}
	actual_size = l_mc_stat.size;
	nlba = actual_size * (1 << l_mc_stat.nmask);
	stride = (1 << l_mc_stat.nmask);
	st_lba = nlba/2;
	while(count > 0)
	{
		if(count%2){
			chunk = actual_size - 1;
		}
		else{
			chunk =actual_size +1;
		}
		pthread_mutex_lock(&mutex);
		debug("write st_lba= 0x%lX,nlba=0x%lX\n",st_lba,nlba);
		send_write(p_ctx, st_lba, stride, pid,VLBA);	
		clba = st_lba;
		W_READ = 1;
		(void)mc_size(p_ctx->mc_hndl,p_ctx->res_hndl, chunk, &actual_size);
		pthread_cond_signal(&condv);
		pthread_mutex_unlock(&mutex);
		nlba = actual_size * (1 << l_mc_stat.nmask);;
		st_lba = nlba/2;
		debug("chunk 0x%lX, nlba 0x%lX, st_lba 0x%lX\n",actual_size,nlba,st_lba);
		debug("Loop.. %d remaining............\n",count);
		count--;
	}
	if(rc != 0)
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
    int rc;
	pid = getpid();
	mc_stat_t l_mc_stat;

	pthread_mutex_lock(&mutex);
	rc =mc_stat(p_ctx->mc_hndl,p_ctx->res_hndl, &l_mc_stat);
	w_chunk = l_mc_stat.size;
	pthread_mutex_unlock(&mutex);
    while(count > 0)
    {
		pthread_mutex_lock(&mutex);
		while(W_WRITE != 1){
			pthread_cond_wait(&condv,&mutex);
		}
		
        rc = mc_size(p_ctx->mc_hndl,p_ctx->res_hndl, w_chunk, &actual_size);
		if(actual_size == 0)
		{
			debug("%d : chunk size reduced to 0, now increase by 128\n", pid);
			w_chunk = 128;
			rc = mc_size(p_ctx->mc_hndl,p_ctx->res_hndl, w_chunk, &actual_size);
		}
		debug("%d :mc_size done 0X%lX\n",pid, actual_size);
        nlba = actual_size * (1 << l_mc_stat.nmask);
		if(count % 2){
        	st_lba = nlba/2;
			w_chunk = actual_size/2+1;
		}
		else {
			st_lba = nlba -1;
			w_chunk = actual_size-1;
		}
        clba = st_lba;
		W_WRITE = 0;
		debug("%d : clba 0X%lX: lba range:0X%lX\n",pid, clba, nlba-1);
        pthread_mutex_unlock(&mutex);
        debug("%d : chunk 0x%lX, nlba 0x%lX, st_lba 0x%lX\n",pid, actual_size,nlba,st_lba);
        debug("%d : Loop.. %d remaining ............\n",pid, count);
        count--;
    	if(rc != 0)
    	{
			g_error = rc;
			fprintf(stderr,"%d : failed here %s:%d:%s\n",pid,
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
	__u64 llba =clba;
	__u64 actual_size;
	mc_stat_t l_mc_stat;

	pid = getpid();
	while(count-- > 0){
		debug("%d :about to process 0X%lX lba\n",pid,llba);
		pthread_mutex_lock(&mutex);
		llba =clba;
		debug("%d : Writing for 0X%lX\n",pid,llba);
		mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		actual_size = l_mc_stat.size;
		debug("%d : actual lba range is 0X%lX\n",
			pid, actual_size * (1 << l_mc_stat.nmask)-1);
		rc = send_single_write(p_ctx, llba, pid);
		W_WRITE = 1;
		pthread_cond_signal(&condv);
		pthread_mutex_unlock(&mutex);
		if(rc && (llba >= actual_size * (1 << l_mc_stat.nmask))) {
			printf("%d : expected one, write(0X%lX) and range(0X%lX)\n",
				pid, llba, actual_size * (1 << l_mc_stat.nmask)-1);
			rc = 0;
		}
		if(rc != 0) {
			g_error = rc;
			fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
						__FILE__,__LINE__,__func__);
			//return NULL;
		}

		pthread_mutex_lock(&mutex);
		mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		actual_size = l_mc_stat.size;
		debug("%d : reading for 0X%lX\n",pid, llba);
		debug("%d : actual lba range is 0X%lX\n",
			pid, actual_size*(1 << l_mc_stat.nmask)-1);
		rc = send_single_read(p_ctx, llba);
		if(rc && llba >= actual_size*(1 << l_mc_stat.nmask)) {
			printf("%d : expected one, read (0X%lX) and range(0X%lX)\n",
				pid, llba, actual_size*(1 << l_mc_stat.nmask)-1);
			rc = 0;
		}else {
			rc = rw_cmp_single_buf(p_ctx, llba);
		}
        pthread_mutex_unlock(&mutex);
        if(rc != 0) {
            g_error = rc;
			fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
						__FILE__,__LINE__,__func__);
            //return NULL;
        }
    }
    return 0;
}

void *read_io(void *arg)
{
	struct ctx *p_ctx = (struct ctx *)arg;
	__u64 stride;
	int rc;
	mc_stat_t l_mc_stat;

	pthread_mutex_lock(&mutex);
	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	pthread_mutex_unlock(&mutex);
	if(rc) {
		g_error = rc;
		fprintf(stderr, "read_io mc_stat failed\n");
		return NULL;
	}
	stride = (1 << l_mc_stat.nmask);
	while(count-- > 0){
		pthread_mutex_lock(&mutex);
		while ( W_READ != 1){
			pthread_cond_wait(&condv,&mutex);
		}
		W_READ = 0;
		send_read(p_ctx, clba, stride,VLBA);
		rc = rw_cmp_buf(p_ctx, clba);
		pthread_mutex_unlock(&mutex);
		if(rc != 0)
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
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
	mc_stat_t l_mc_stat;
    pthread_t thread;
	pthread_t rhthread[2];

	pthread_mutexattr_t mattr;
	pthread_condattr_t cattr;
	//int i;
	__u64 chunks=512;
	__u64 actual_size;
	
	pthread_mutexattr_init(&mattr);
	pthread_mutex_init(&mutex, &mattr);	

	pthread_condattr_init(&cattr);
	pthread_cond_init(&condv, &cattr);	
	if(test_init(p_ctx) != 0)
	{
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

	if(1 == cmd) { //a thread write_io & another read_io
		pthread_create(&rhthread[0], NULL, write_io, p_ctx);
		pthread_create(&rhthread[1], NULL, read_io, p_ctx);
	}
	else if(2 == cmd) { //a thread rw & another mc size
		clba = (actual_size * (1 << l_mc_stat.nmask))/2;
		pthread_create(&rhthread[0], NULL, rw_io, p_ctx);
		pthread_create(&rhthread[1], NULL, play_size, p_ctx);
	}
	//pthread_create(&rhthread[1], NULL, inc_dec_size, p_ctx);

	pthread_mutexattr_destroy(&mattr);
	pthread_condattr_destroy(&cattr);

	pthread_join(rhthread[0], NULL);
	pthread_join(rhthread[1], NULL);

	pthread_cancel(thread);
	mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
	mc_unregister(p_ctx->mc_hndl);
	ctx_close(p_ctx);
	mc_term();
	rc = g_error;
	g_error = 0;
	return rc;
}

int test_two_ctx_two_thrd(int cmd)
{
    int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread[2];

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    __u64 chunks=480;
    __u64 actual_size;
	mc_stat_t l_mc_stat;

    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);

    pthread_condattr_init(&cattr);
    pthread_cond_init(&condv, &cattr);
	int i;
	for(i = 0;i < 2;i++) {
		if(fork() == 0) {//child process
			if(test_init(p_ctx) != 0)
			{
					exit(-1);
			}
			pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

			rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
			if(rc != 0)	{
				fprintf(stderr, "ctx req failed,ctx=%d,rc %d\n",
					p_ctx->ctx_hndl, rc);
				exit(-1);
			}

			rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
			if (rc != 0) {
				fprintf(stderr, "error opening res_hndl rc %d\n", rc);
				exit(-1);
			}

			rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
			CHECK_RC(rc, "mc_size");

			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
			CHECK_RC(rc, "mc_stat");

			if(1 == cmd) { //a thread write_io & another read_io
				pthread_create(&rhthread[0], NULL, write_io, p_ctx);
				pthread_create(&rhthread[1], NULL, read_io, p_ctx);
			}
			else if(2 == cmd) { //a thread rw & another mc size
				clba = (actual_size * (1 << l_mc_stat.nmask))/2;
				pthread_create(&rhthread[0], NULL, rw_io, p_ctx);
				pthread_create(&rhthread[1], NULL, play_size, p_ctx);
			}

			pthread_mutexattr_destroy(&mattr);
			pthread_condattr_destroy(&cattr);

			pthread_join(rhthread[0], NULL);
			pthread_join(rhthread[1], NULL);

			pthread_cancel(thread);
			mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
			mc_unregister(p_ctx->mc_hndl);
			ctx_close(p_ctx);
			mc_term();
			rc = g_error;
			debug("%d : I am exiting from here .....rc = %d\n", pid, rc);
			exit(rc);
		}
	}
	while((pid =  waitpid(-1,&rc,0)))
	{
		debug("%d :  wait is over for me............rc = %d\n", pid, rc);
		if(rc != 0){
			g_error = -1;
		}
		if(pid == -1) {
			break;
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

	if(test_init(p_ctx) != 0)
	{
			return -1;
	}
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	// Send Report LUNs to get the list of LUNs and LUN ID
	for(i =1; i <= port;i++) {
		rc = send_report_luns(p_ctx, i, &lun_ids,&n_luns);
		if(rc) {
			fprintf(stderr, "Report LUNs failed on FC Port %d\n", i);
		}
		else{
			fc_port = i;	
			break;
		}
	}
	if(rc || n_luns == 0)
	{
		ctx_close(p_ctx);
		return rc;
	}
	debug("Report Lun success, num luns= 0x%x\n",n_luns);
	for(i = 0; i< n_luns;i++)
	{
		rc = send_read_capacity(p_ctx,fc_port,lun_ids[i],&lun_cap, &blk_len);
		if(rc != 0)
		{
			fprintf(stderr,"Read capacity failed,lun id =0x%lX, rc = %d\n",lun_ids[i],rc);
			break;	
		}
		debug("LUN id = 0x%lX Capacity = 0x%lX Blk len = 0x%lX\n",
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


	if(mc_init() != 0)
	{
		fprintf(stderr, "mc_init failed.\n");
		return -1;
	}
	debug("mc_init success.\n");

	if(ctx_init(p_ctx) != 0)
	{
		fprintf(stderr, "Context init failed, errno %d\n", errno);
		return -1;
	}
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *) p_ctx->p_host_map,&p_ctx->mc_hndl);
	if (rc != 0) {
		fprintf(stderr, "error registering(%s) ctx_hndl %d, rc %d\n",
				master_dev_path, p_ctx->ctx_hndl, rc);
		return -1;
	}

	rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
	if (rc != 0) {
		fprintf(stderr, "error opening res_hndl rc %d\n", rc);
		return -1;
	}

	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, chunk, &actual_size);
	if(rc != 0 || actual_size < 1) //might be chunk want to allocate whole lun
	{
		fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
		return -1;
	}
	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");

	pid = getpid();

	stride = (1 << l_mc_stat.nmask);
	nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
	debug("%d : st_lba = 0X0 and range lba = 0X%lX\n", pid, nlba-1);
	for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
		send_write(p_ctx, st_lba, stride, pid,VLBA);
		send_read(p_ctx, st_lba, stride,VLBA);
		rc = rw_cmp_buf(p_ctx, st_lba);
		if(rc){
			fprintf(stderr,"buf cmp failed for vlba 0x%lX,rc =%d\n",
					st_lba,rc);
			break;
		}
	}
	pthread_cancel(thread);
	mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	mc_unregister(p_ctx->mc_hndl);
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
	while(1) {
		for(st_lba =0; st_lba < nlba; st_lba += (NUM_CMDS * stride)){
			//rc = send_single_write(p_ctx,st_lba,pid);
			rc = send_write(p_ctx,st_lba,stride,pid, VLBA);
			if(rc != 0) {
				g_error = rc;
				if(!dont_displa_err_msg)
				fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
							__FILE__,__LINE__,__func__);
				return NULL;
			}
			//rc = send_single_read(p_ctx, st_lba);
			rc = send_read(p_ctx, st_lba, stride, VLBA);
			if(rc != 0) {
				g_error = rc;
				if(!dont_displa_err_msg)
				fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
							__FILE__,__LINE__,__func__);
				return NULL;
			}
			//rc = rw_cmp_single_buf(p_ctx, st_lba);
			rc = rw_cmp_buf(p_ctx, st_lba);
			if(rc != 0) {
				g_error = rc;
				if(!dont_displa_err_msg)
					fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
							__FILE__,__LINE__,__func__);
				return NULL;
			}
		}
		if(N_done) {
			break;
		}
	}
    return 0;
}

int test_rw_close_hndl(int cmd)
{
	int rc=0;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t thread;
    pthread_t rhthread;
	mc_stat_t l_mc_stat;
	__u64 chunks=128;
	__u64 actual_size;
	
	if(test_init(p_ctx) != 0)
	{
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

	clba = actual_size * (1 << l_mc_stat.nmask);
	pthread_create(&rhthread, NULL, only_rw_io, p_ctx);
	sleep(1);

	if(1 == cmd) { //while IO close RHT
		mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
	}else if(2 == cmd) { //while IO unreg MC HNDL
		mc_unregister(p_ctx->mc_hndl);
	}else if(3 == cmd) { //While IO ctx close
		munmap((void*)p_ctx->p_host_map, 0x10000);
		close(p_ctx->afu_fd);
		//ctx_close(p_ctx);
	}

	N_done = 1; //tell pthread that -ve test performed
	sleep(1);
	pthread_cancel(rhthread);
	//pthread_join(rhthread, NULL);
	N_done = 0;

	pthread_cancel(thread);
	// do proper closing 
	if(1 == cmd) {
		mc_unregister(p_ctx->mc_hndl);
		ctx_close(p_ctx);
	} else if (2 == cmd) {
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
    if(test_init(p_ctx) != 0)
    {
        exit(-1);
    }
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
		(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
	if(rc != 0) {
		fprintf(stderr, "ctx _reg failed, ctx_hndl %d,rc %d\n",
				p_ctx->ctx_hndl, rc );
		exit(-1);
	}

	rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
	if (rc != 0) {
		fprintf(stderr, "error opening res_hndl rc %d\n", rc);
		exit(-1);
	}

	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
    CHECK_RC(rc, "mc_size");

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

	clba = actual_size * (1 << l_mc_stat.nmask);
	pthread_create(&rhthread, NULL, only_rw_io, p_ctx);

	if(mypid  == 0) { //child process do err ctx
		debug("child pid is :%d\n",getpid());
		sleep(1); //let thrd do some io
		printf("error ctx pid is %d\n",getpid());
		if(1 == cmd) { //while IO close RHT
			mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
		}else if(2 == cmd) { //while IO unreg MC HNDL
			mc_unregister(p_ctx->mc_hndl);
		}else if(3 == cmd) { //While IO ctx close
			//munmap((void*)p_ctx->p_host_map, 0x10000);
			close(p_ctx->afu_fd);
			//ctx_close(p_ctx);
		}
		sleep(1);
		N_done = 1; //tell pthread that -ve test performed
		pthread_cancel(rhthread);
		//pthread_join(rhthread, NULL);
		N_done = 0;
		debug("%d : exiting with rc = %d\n", pid, g_error);
		pthread_cancel(thread);
		exit(g_error);
	}
	else {
		debug("Good ctx pid is : %d\n",getpid());
		sleep(2); //main process sleep but thrd keep running
		N_done = 1; //
		pthread_join(rhthread, NULL);
		N_done = 0;

		// do proper closing
		pthread_cancel(thread);
		mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
		mc_unregister(p_ctx->mc_hndl);
		ctx_close(p_ctx);
		debug("%d : waiting for child process %d\n", pid, mypid);
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

	
	if(1 == cmd) //16 byte ea alignment
		offset = 16;
	else if(2 == cmd) //128 byte ea alignment
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
    rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
        (volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
    CHECK_RC(rc, "ctx reg failed");

    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
    CHECK_RC(rc, "opening res_hndl");

    rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
    CHECK_RC(rc, "mc_size");
	
	if(chunks != actual_size)
	{	
		CHECK_RC(1, "doesn't have enough chunk space");
	}
	st_lba = 0;

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
    CHECK_RC(rc, "mc_stat");

	nlba = actual_size * (1 << l_mc_stat.nmask);
	debug("EA alignment from begining of 4K\n");
	for(a=offset; a <= max; a+=offset){
		debug("send alignment offset : %u\n",a);
		rc = send_rw_rcb(p_ctx, p_rwb, st_lba, stride, a, 0);
		if(rc) break;
		rc = send_rw_rcb(p_ctx, p_rwb, nlba/2, stride, a, 0);
		if(rc) break;
		rc = send_rw_rcb(p_ctx, p_rwb, nlba-(NUM_CMDS * stride), stride, a, 0);
		if(rc) break;
	}
	//CHECK_RC(rc, "send_rw_rcb");
	debug("EA alignment from end of a 4K\n");
	for(a=offset; a <= max; a+=offset){
		debug("send alignment offset from last : %u\n", a);
		rc = send_rw_rcb(p_ctx, p_rwb, st_lba, stride, a, 1);
		if(rc) break;
		rc = send_rw_rcb(p_ctx, p_rwb, nlba/2, stride, a, 1);
		if(rc) break;
		rc = send_rw_rcb(p_ctx, p_rwb, nlba-(NUM_CMDS * stride), stride, a, 1);
		if(rc) break;
	}
	pthread_cancel(thread);
	mc_unregister(p_ctx->mc_hndl);
	ctx_close(p_ctx);
	free(p_rwb);
	mc_term();
	if(rc!=0 && cmd == 3)
		return 3;
	return rc;
}

int mc_test_rwbuff_global()
{
    int rc;
    struct ctx *p_ctx = &gs_ctx;
    __u64 chunks=256;
    __u64 actual_size=0;
	__u64 st_lba;
	__u64 stride;
	__u64 nlba;
	pthread_t thread;
	mc_stat_t l_mc_stat;

	
	pid = getpid();

	rc =mc_init();
	CHECK_RC(rc, "mc_init failed");
	
	rc = ctx_init(p_ctx);
	CHECK_RC(rc, "ctx init failed");
	
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

	nlba = actual_size * (1 << l_mc_stat.nmask);
	stride = (1 << l_mc_stat.nmask);
	for(st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS * stride)) {
		rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
        CHECK_RC(rc, "send_write");
        rc = send_read(p_ctx, st_lba, stride,VLBA);
        CHECK_RC(rc, "send_read");
        rc = rw_cmp_buf(p_ctx, st_lba);
        CHECK_RC(rc, "rw_cmp_buf");
    }
	pthread_cancel(thread);
	rc = mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
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

    rc =mc_stat(p_ctx->mc_hndl,p_ctx->res_hndl, &l_mc_stat);
	w_chunk = l_mc_stat.size;
    while(myloop-- > 0)
    {
		w_chunk +=128;
		debug("%d : doing mc size from 0X%lX to 0X%lX\n", pid, actual_size, w_chunk);
		rc = mc_size(p_ctx->mc_hndl,p_ctx->res_hndl, w_chunk, &actual_size);
		if(rc != 0) {
			g_error = rc;
			N_done = 1;
			fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
						__FILE__,__LINE__,__func__);
			return NULL;
		}
		w_chunk -=128;
		debug("%d : doing mc size from 0X%lX to 0X%lX\n", pid, actual_size, w_chunk);
		rc = mc_size(p_ctx->mc_hndl,p_ctx->res_hndl, w_chunk, &actual_size);
		if(rc != 0) {
			g_error = rc;
			N_done = 1;
			fprintf(stderr, "%d : failed here %s:%d:%s\n",pid,
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

	for(i =0 ;i < 4; i++){
		if(fork() == 0) {
			sleep(1); //lets all process get created
			rc = test_init(p_ctx);
			CHECK_RC(rc, "test init");
			pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);

			rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
				(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
			CHECK_RC(rc, "mc_register");

			rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
			CHECK_RC(rc, "mc_open");

			rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
			CHECK_RC(rc, "mc_size");

			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
			CHECK_RC(rc, "mc_stat");

			clba = (actual_size * (1 << l_mc_stat.nmask));
			pthread_create(&rhthread[0], NULL, only_rw_io, p_ctx);
			pthread_create(&rhthread[1], NULL, only_play_size, p_ctx);

			pthread_join(rhthread[0], NULL);
			pthread_join(rhthread[1], NULL);

			pthread_cancel(thread);
			mc_close(p_ctx->mc_hndl,p_ctx->res_hndl);
			mc_unregister(p_ctx->mc_hndl);
			ctx_close(p_ctx);
			mc_term();
			rc = g_error;
			exit(rc);
		}
	}
	while((pid =  waitpid(-1,&rc,0)))
    {
		if (WIFEXITED(rc)) {
			rc =  WEXITSTATUS(rc);
			if(rc != 0){
				g_error = -1;
			}
        }
        debug("pid %d exited...\n",pid);

		if(pid == -1) {
			break;
        }
    }
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
    __u64 chunks=256;
    __u64 actual_size=0;
    __u64 st_lba;
	__u64 nlba;
    __u64 stride = 0x1000;
    pthread_t thread;
	pid_t cpid;
	mc_stat_t l_mc_stat;

	int shmid;
	key_t key=2345;
	char *shm;
	pid = getpid();

	if((shmid = shmget(key,sizeof(struct rwshmbuf), IPC_CREAT | 0666)) < 0){
		fprintf(stderr, "shmget failed\n");
		return -1;
	}

	if((shm = shmat(shmid, NULL, 0)) == (char *)-1) {
		fprintf(stderr, "shmat failed\n");
		return -1;
	}
	debug("%d : shared region created\n",pid);
	//lets create a child process to keep reading shared area
	cpid = fork();
	if(cpid == 0){
		pid = getpid();
		if((shmid = shmget(key,sizeof(struct rwshmbuf), IPC_CREAT | 0666)) < 0){
			fprintf(stderr, "shmget failed\n");
			exit(-1);
		}

		if((shm = shmat(shmid, NULL, 0)) == (char *)-1) {
			fprintf(stderr, "shmat failed\n");
			exit(-1);
		}
		debug("%d: child started accessing shared memory...\n",pid);
		while(1) {
			memcpy(&l_rwb, shm, sizeof(struct rwshmbuf));
		}
	}

	p_rwb = (struct rwshmbuf *)shm;

    rc =mc_init();
    CHECK_RC(rc, "mc_init failed");

    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "ctx init failed");

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

	nlba = actual_size * (1 << l_mc_stat.nmask);
	debug("%d : started IO where rwbuf in shared memory lba range(0X%lX)\n", pid, nlba-1);
	for(st_lba =0; st_lba < nlba; st_lba += stride) {
		rc = send_rw_shm_rcb(p_ctx, p_rwb, st_lba);
		CHECK_RC(rc, "send_rw_rcb");
	}
	debug("%d : IO is done now \n", pid);

	debug("%d : now time to kill child %d \n", pid, cpid);
	kill(cpid, SIGKILL);

	shmdt(shm);
	pthread_cancel(thread);
	mc_unregister(p_ctx->mc_hndl);
    ctx_close(p_ctx);
    mc_term();
    return 0;
}
