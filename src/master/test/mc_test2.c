/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test2.c $                                  */
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
#include <stdbool.h>

extern char master_dev_path[MC_PATHLEN];
extern char afu_path[MC_PATHLEN];

extern pid_t pid;
extern __u8 rrq_c_null;
extern int dont_displa_err_msg;
/*each thread can modify this value incase of any failure */
static int g_error =0;

/*
 *  * Serialization is required for a mc_handle
 *  * which is shared by multiple threads.
 *  
 */
static pthread_mutex_t mutex; 
static pthread_mutex_t counter=PTHREAD_MUTEX_INITIALIZER;
static int count=0;

void* test_mc_api(void *arg);
int do_basic_setting(struct ctx *p_ctx);

void* test_mc_api(void *arg)
{
	struct ctx *p_ctx = (struct ctx*)arg;
	mc_hndl_t mc_hndl = p_ctx->mc_hndl;
	int rc=0;
	res_hndl_t res_handl;
	__u64 size ;
	__u64 actual_size=0;
	__u64 plba=0;
	__u64 nlba, st_lba;
	__u64 stride;
	mc_stat_t l_mc_stat;

	pthread_t pthread_id1 =pthread_self();
	unsigned int pthread_id =(unsigned int)pthread_id1;
	size = (rand()%10+1)*16;

	pthread_mutex_lock(&mutex);
	rc = mc_open(mc_hndl, MC_RDWR, &res_handl);
	pthread_mutex_unlock(&mutex);
	if(rc != 0) {
		fprintf(stderr, "thread : 0x%x:mc_open: failed,rc %d\n", pthread_id,rc);
		g_error = -1;
		return NULL;
	}
	pthread_mutex_lock(&counter);
	count++;
	pthread_mutex_unlock(&counter);

	pthread_mutex_lock(&mutex);
	rc = mc_size(mc_hndl, res_handl, size,&actual_size);
	pthread_mutex_unlock(&mutex);
	if(rc != 0) {
		fprintf(stderr, "thread : 0x%x:mc_size: failed,rc %d\n", pthread_id,rc);
		g_error = -1;
		return NULL;
	}

	pthread_mutex_lock(&mutex);
	rc = mc_stat(mc_hndl, res_handl, &l_mc_stat);
	pthread_mutex_unlock(&mutex);
	if(rc != 0) {
		fprintf(stderr, "thread : 0x%x:mc_stat: failed,rc %d\n", pthread_id,rc);
		g_error = -1;
		return NULL;
	}
	size = l_mc_stat.size;
	if(size != actual_size)
	{
		fprintf(stderr,"thread : 0x%x:size mismatched: %lu : %lu\n", pthread_id,size,actual_size);
		g_error = -1;
		return NULL;
	}
	
	nlba = size * (1 << l_mc_stat.nmask);
	pthread_mutex_lock(&mutex);
	rc = mc_xlate_lba(mc_hndl, res_handl, nlba-1,&plba);
	pthread_mutex_unlock(&mutex);
	if(rc != 0) {
		fprintf(stderr, "thread : 0x%x:mc_xlate_lba: failed,rc %d\n", pthread_id,rc);
		g_error = -1;
		return NULL;
	}

	stride = 1 << l_mc_stat.nmask;
	for(st_lba = 0; st_lba < nlba; st_lba +=(NUM_CMDS*stride))
	{
		pthread_mutex_lock(&mutex);
		p_ctx->res_hndl = res_handl;
		debug("res hnd: %d send write for 0X%lX \n",res_handl, st_lba);
		
		rc = send_write(p_ctx, st_lba, stride, pthread_id1, VLBA);
		rc += send_read(p_ctx, st_lba, stride,VLBA);
		rc += rw_cmp_buf(p_ctx, st_lba);
		
		pthread_mutex_unlock(&mutex);
		if(rc) {
			g_error = rc;
			break;
		}
	}
	
	pthread_mutex_lock(&mutex);
	rc = mc_close(mc_hndl, res_handl);
	pthread_mutex_unlock(&mutex);
	if(rc != 0) {
		fprintf(stderr, "thread : 0x%x:mc_close: failed,rc %d\n", pthread_id,rc);
		g_error = -1;
		return NULL;
	}
	return 0;
}

int mc_max_vdisk_thread()
{
	struct ctx_alloc p_ctx_a;
	struct pthread_alloc *p_thread_a;
	struct ctx *p_ctx = &(p_ctx_a.ctx);
	pthread_mutexattr_t mattr;
	pthread_t thread;
	int rc = 0;
	int i;
	
	if(mc_init() !=0 ) {
		fprintf(stderr, "mc_init failed.\n");
		return -1;
	}
	debug("mc_init success.\n");
	
	//Allocating structures for pthreads.
	p_thread_a = (struct pthread_alloc *) malloc(sizeof(struct pthread_alloc) * MAX_NUM_THREADS);
	if(p_thread_a == NULL) {
		fprintf(stderr, " Can not allocate thread structs, errno %d\n", errno);
		return -1;
	}
	rc = ctx_init(p_ctx);
	if(rc != 0)
	{
		fprintf(stderr, "Context initialization failed, errno %d\n", errno);
		return -1;
	}

	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	debug("master_dev_path : %s\n",master_dev_path);
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl, (volatile __u64 *)p_ctx->p_host_map,
					&p_ctx->mc_hndl);
	if(rc != 0) {
		fprintf(stderr, "mc_register: failed. Error registering ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc ); 
		return -1;
	}
	debug_2("%d : ctx hand & mc hnd: %d & %p\n",
		pid, p_ctx->ctx_hndl, p_ctx->mc_hndl);

	//initialize the mutex
	pthread_mutexattr_init(&mattr);
	pthread_mutex_init(&mutex, &mattr); 
	//create threads
	for(i=0;i< MAX_NUM_THREADS; ++i)
	{
		rc = pthread_create(&(p_thread_a[i].rrq_thread), NULL, &test_mc_api, (void *)p_ctx);
		if(rc) {
			fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
			return -1;
		}
	}

	//destroy mutexattr
	pthread_mutexattr_destroy(&mattr);
	//joining
	for(i=0;i< MAX_NUM_THREADS; ++i)
	{
		pthread_join(p_thread_a[i].rrq_thread, NULL);
	}
	
	//destroy the mutex
	pthread_mutex_destroy(&mutex);
	
	pthread_cancel(thread);
	debug_2("%d : ctx hand & mc hnd: %d & %p\n",
		pid, p_ctx->ctx_hndl, p_ctx->mc_hndl);
	rc = mc_unregister(p_ctx->mc_hndl);
	if(rc != 0)
	{
		fprintf(stderr, "mc_unregister failed for mc_hdl %p\n", p_ctx->mc_hndl);
		return -1;
	}
	
	ctx_close(p_ctx);
	mc_term();
	//free allocated space
	free(p_thread_a);
	rc = g_error;
	g_error =0;
	return rc; 
}

int do_basic_setting(struct ctx *p_ctx)
{
	int rc;	
	rc = ctx_init(p_ctx);
    if (rc != 0) {
        fprintf(stderr, "error instantiating ctx, rc %d\n", rc);
        return -1;
    }

    rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
                     (volatile __u64 *) p_ctx->p_host_map,
                     &p_ctx->mc_hndl);
    if (rc != 0) {
        fprintf(stderr, "error registering ctx_hndl %d, rc %d\n",
                p_ctx->ctx_hndl, rc);
        return -1;
    }
	return 0;
}

int test_mc_clone_api(__u32 flags)
{
	int rc;
	struct ctx testctx;
	mc_hndl_t mc_hndl_old;
	mc_hndl_t mc_hndl;
	pid_t child_pid;
	__u64 chunks =512;
	__u64 newchunks = 0X100;
	__u64 actual_size;
	__u64 stride;
	__u64 st_lba;
	__u64 nlba;
	struct ctx  *p_ctx = &testctx;
	pthread_t thread;
	mc_stat_t l_mc_stat;

	
	if(mc_init() !=0 ) {
		fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
   debug("mc_init success.\n");

	if(do_basic_setting(p_ctx) != 0) {
		return -1;
	}

	mc_hndl = p_ctx->mc_hndl;
	rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
	mc_unregister(mc_hndl);
	
	pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
	rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &(p_ctx->res_hndl));
	if (rc != 0) {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
	}
	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
             chunks, &actual_size);
	if (rc != 0 || chunks != actual_size) {
		fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
		return -1;
	}

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");
	pid = getpid();
	
	nlba = actual_size * (1 << l_mc_stat.nmask);
	stride = 1 << l_mc_stat.nmask;
	for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
		rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
		if(rc != 0)
		{
			fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
			return rc;
		}
	}
	pthread_cancel(thread);
	child_pid = fork();
	if(child_pid == 0)
	{
		mc_init();
		//child process
		mc_hndl_old = p_ctx->mc_hndl;

		if(do_basic_setting(p_ctx) != 0)
    	{
        	exit(-1);
		}
		mc_hndl = p_ctx->mc_hndl;
		rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
		mc_unregister(mc_hndl);
		//do mc_clone 
		rc = mc_clone(p_ctx->mc_hndl, mc_hndl_old, flags);
		if (rc != 0) {
            	fprintf(stderr, "error cloning rc %d\n", rc);
            	exit(-1);
        }

		pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);

		// close parent inherited interfaces
		rc = mc_unregister(mc_hndl_old);
		//ctx_close(p_ctx);
		rc = mc_stat(p_ctx->mc_hndl,p_ctx->res_hndl,&l_mc_stat);
		if (rc != 0) {
			fprintf(stderr, "error in mc_stat rc %d\n", rc);
			exit(-1);
        }
		actual_size = l_mc_stat.size;
		if(chunks != actual_size)
		{
			fprintf(stderr, "size mismatch rc %d\n", rc);
			exit(-1);
		}

		if(flags & MC_RDONLY ){ //RDWR & RDONLY both works
		debug("%d : Do read for clone cmp \n", getpid());
		for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
			rc = send_read(p_ctx, st_lba, stride, VLBA);
			if(rc != 0) {
				fprintf(stderr,"send read failed 0X%lX vlba\n",st_lba);
				exit(rc);
				}
			rc = rw_cmp_buf_cloned(p_ctx, st_lba);
			if(rc){
				exit(rc);
				}
			}
		}
		pid = getpid();
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
                    newchunks,&actual_size);
		debug("pid:%u newchunks:%lu actual_size: %lu\n",pid,newchunks,actual_size);
        if (rc != 0 || newchunks != actual_size) {
            fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
            exit(-1);
        }

		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
		CHECK_RC(rc, "mc_stat");

		nlba = newchunks * (1 << l_mc_stat.nmask);;
		if(flags == MC_RDWR) {
		for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)){
			rc = send_write(p_ctx, st_lba, stride, pid,VLBA);
			if(rc){
				fprintf(stderr,"send write failed @ 0X%lX vlba\n",st_lba);
				exit(rc);
			}
			rc = send_read(p_ctx, st_lba, stride,VLBA);
			if(rc){
				fprintf(stderr,"send read failed @ 0X%lX vlba\n",st_lba);
				exit(rc);
			}
			rc = rw_cmp_buf(p_ctx, st_lba);
			if(rc){
				exit(rc);
				}
			}	
		}
		if(flags ==  MC_WRONLY) { //WRONLY & RDWR both works
		for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)){
			rc = send_write(p_ctx, st_lba, stride, pid,VLBA);
			if(rc){
				fprintf(stderr,"send write failed @ 0X%lX vlba\n",st_lba);
				exit(rc);
				}
			}
		}
		newchunks = 0;
		debug("Calling again mc_size with 0 value\n");
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
                    0,&actual_size);

		debug("pid:%u newchunks:%lu actual_size: %lu\n",pid,newchunks,actual_size);
        if (rc != 0 || newchunks != actual_size) {
            fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
            exit(-1);
        }
		pthread_cancel(thread);
		mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
		mc_unregister(p_ctx->mc_hndl);
		exit(0);
	}
	else
	{
		//let child process done cloning
		sleep(2);
		pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
					newchunks,&actual_size);
		if (rc != 0 || newchunks != actual_size) {
        	fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
        	return -1;
    	}
        if (rc != 0 || newchunks != actual_size) {
            fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
			return -1;
        }

        pid = getpid();
		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
        CHECK_RC(rc, "mc_stat");
        nlba = newchunks * (1 << l_mc_stat.nmask);;
        for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)){
            rc = send_write(p_ctx, st_lba, stride, pid,VLBA);
            if(rc){
                fprintf(stderr,"send write failed @ 0X%lX vlba\n",st_lba);
				return rc;
            }
            rc = send_read(p_ctx, st_lba, stride,VLBA);
            if(rc){
                fprintf(stderr,"send read failed @ 0X%lX vlba\n",st_lba);
				return rc;
            }
            rc = rw_cmp_buf(p_ctx, st_lba);
            if(rc){
                exit(rc);
				return rc;
            }
        }

		pthread_cancel(thread);
		mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
		mc_unregister(p_ctx->mc_hndl);
		debug("pid = %u, cpid = %u\n", pid, child_pid);
		fflush(stdout);
		child_pid = wait(&rc);
		if (WIFEXITED(rc)) {
			rc =  WEXITSTATUS(rc);
        }
		debug("%d terminated, signalled %s, signo %d\n",
                   child_pid, WIFSIGNALED(rc) ? "yes" : "no", WTERMSIG(rc));
        fflush(stdout);
	}
	ctx_close(p_ctx);
	mc_term();
	return rc;	
}

int test_mc_max_size()
{
	int rc;
    struct ctx testctx;
	pthread_t thread;
    __u64 chunks =0x1000;
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
	if((str != NULL) && !strcmp(str, "TRUE")) {
		chunks = 1; //increment one by one
		loop_stride = 10;
		is_stress = true;
	}

	pid = getpid();
    if(mc_init() !=0 ) {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
    debug("mc_init success.\n");

    if(do_basic_setting(p_ctx) != 0)
    {
        return -1;
    }

	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &(p_ctx->res_hndl));
    if (rc != 0) {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
    }
	//allocate max allow size for a vdisk
	while(1)
	{
    	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
             chunks, &actual_size);
		if(rc != 0)
		{
			fprintf(stderr,"mc_size failed rc=%d\n",rc);
			return -1;
		}
		if(chunks != actual_size)
		{
			debug("now reaching extreme..chunk(0X%lX) act(0X%lX)\n", chunks,actual_size);
			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
			if(rc != 0)
			{
				fprintf(stderr,"mc_stat failed rc = %d\n",rc);
				return -1;
			}
			max_size = l_mc_stat.size;
			break;
		}
		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
		if(rc != 0)
    	{
			fprintf(stderr,"mc_stat failed rc = %d\n",rc);
			return -1;
       	}
		nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
		debug("chunk(0X%lX)lba (0X%lX) i/o@(0X%lX)\n",
			actual_size,nlba, nlba-(NUM_CMDS*stride));
		rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid, VLBA);
		if(rc) break;
		rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride, VLBA);
		if(rc) break;
		rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
		if(rc) break;
		if(is_stress)
			chunks++;
		else
			chunks += 0x1000;
	}

	if(max_size == 0)
	{
		debug("lets check more chunk can be allocated\n");
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
             chunks+1, &actual_size);
		debug("new chunk =0X%lX & LBAs = 0X%lX\n",
					actual_size, actual_size*(1 << l_mc_stat.nmask));
		fprintf(stderr, "some errors happend\n");
		return -1;
	}
	debug("OK, I got the lun size 0X%lX & nlba 0X%lX\n",
			max_size, max_size*(1 << l_mc_stat.nmask));
	lun_size = (max_size*(1 << l_mc_stat.nmask)*(l_mc_stat.blk_len)/(1024*1024*1024));


	nlba = max_size * (1 << l_mc_stat.nmask);
	if(is_stress) {
		printf("%d : now do IO till 0X%lX lbas\n", pid, nlba-1);
		for(st_lba = 0; st_lba < nlba; st_lba += NUM_CMDS * stride) {
			rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
			CHECK_RC(rc, "send_write");
			rc = send_read(p_ctx, st_lba, stride, VLBA);
			CHECK_RC(rc, "send_read");
			rc = rw_cmp_buf(p_ctx, st_lba);
			CHECK_RC(rc, "rw_cmp_buf");
		}
	}
	//allocate & dallocate max_size
	chunks = max_size;
	while(chunks > 0)
	{
		chunks = chunks/2;
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
			chunks, &actual_size);
		if(rc != 0 || chunks != actual_size)
		{
			fprintf(stderr,"mc_size api failed.. rc=%d\n",rc);
			fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
			return -1;
		}
		rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
		if(rc != 0 || chunks != actual_size)
		{
			fprintf(stderr,"mc_stat api failed.. rc=%d\n",rc);
			fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
			return -1;
		}
		actual_size = l_mc_stat.size;
		if(actual_size == 0) break;
		nlba = actual_size * (1 << l_mc_stat.nmask);
		debug("chunk(0X%lX)lba (0X%lX) i/o@(0X%lX)\n",
			actual_size,nlba, nlba-(NUM_CMDS*stride));
		rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid, VLBA);
		if(rc) break;
		rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride, VLBA);
		if(rc) break;
		rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
		if(rc) break;
	}

	for(i=0;i<=max_size;i+=loop_stride)
	{
		rnum = rand()% max_size +1;
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
            rnum, &actual_size);

		if((rc != 0)||(rnum != actual_size))
        {
            fprintf(stderr,"mc_size api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
        rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
		if((rc != 0 )|| (rnum != actual_size))
        {
            fprintf(stderr,"mc_stat api failed.. rc=%d\n",rc);
            fprintf(stderr,"expected & actual : %lu & %lu\n",chunks,actual_size);
            return -1;
        }
		actual_size = l_mc_stat.size;
		nlba = actual_size * (1 << l_mc_stat.nmask);
		debug("chunk(0X%lX)lba (0X%lX) i/o@(0X%lX)\n",
			actual_size,nlba, nlba-(NUM_CMDS*stride));
		rc = send_write(p_ctx, nlba-(NUM_CMDS*stride), stride, pid, VLBA);
		if(rc) break;
		rc += send_read(p_ctx, nlba-(NUM_CMDS*stride), stride, VLBA);
		if(rc) break;
		rc = rw_cmp_buf(p_ctx, nlba-(NUM_CMDS*stride));
		if(rc) break;
	}
	pthread_cancel(thread);
	mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	mc_unregister(p_ctx->mc_hndl);
	ctx_close(p_ctx);
	printf("LUN size is :%lu GB\n",lun_size);
	return rc;
}

void *max_ctx_res(void *arg)
{
	int rc;
	res_hndl_t my_res_hndl[MAX_RES_HANDLE];
	struct ctx *p_ctx = (struct ctx *)arg;
	int i;
	pthread_t pthread_id = pthread_self();
	__u64 size = 16;
	__u64 actual_size;
	__u64 nlba, st_lba;
	__u64 stride;
	mc_stat_t l_mc_stat;

	pid = getpid();
	rc = ctx_init(p_ctx);
	if(rc != 0)
	{
		fprintf(stderr, "Context init failed, errno %d\n", errno);
		g_error =-1;
		return NULL;
	}

	//pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
		(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
	if(rc != 0) {
        fprintf(stderr, "mc_register: failed. ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
		g_error =-1;
		return NULL;
	}
	debug("thread:%lx ctx hand %d & mc hnd:%p\n", pthread_id,p_ctx->ctx_hndl, p_ctx->mc_hndl);

	//open max allowed res for a context
	size = (rand()%10+1)*16;
	for(i=0;i<MAX_RES_HANDLE;i++)
	{
		rc = mc_open(p_ctx->mc_hndl,MC_RDWR,&my_res_hndl[i]);
		if(rc != 0) {
		fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
		g_error = -1;
		return NULL;
    		}
		debug("ctx:%d res hndl:%u\n",p_ctx->ctx_hndl,my_res_hndl[i]);
	}
	
	for(i=0;i<MAX_RES_HANDLE;i++)
	{
		rc = mc_size(p_ctx->mc_hndl,my_res_hndl[i],size,&actual_size);
		if(rc != 0) {
        	fprintf(stderr, "thread : %lx:mc_size: failed,rc %d\n", pthread_id,rc);
        	g_error = -1;
        	return NULL;
		}
		rc = mc_stat(p_ctx->mc_hndl,my_res_hndl[i], &l_mc_stat);
		if(rc != 0) {
            fprintf(stderr, "thread : %lx:mc_stat: failed,rc %d\n", pthread_id,rc);
            g_error = -1;
            return NULL;
        }
		nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
		stride = 1 << l_mc_stat.nmask;
		nlba = 0; //NO IO here
		for(st_lba = 0;st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
			rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
			if((rc != 0) && (actual_size == 0)){
				printf("%d : Fine, IO @(0X%lX) but range is(0X%lX)\n",pid, st_lba, nlba-1);
				size = 16;
				break;
			}else {
				fprintf(stderr,"%d : Send write failed @ (0X%lX) LBA\n", pid, st_lba);
				g_error = -1;
				return NULL;
			}
			rc = send_read(p_ctx, st_lba, stride, VLBA);
			rc += rw_cmp_buf(p_ctx, st_lba);
			if(rc){
				g_error = -1;
				return NULL;
			}
		}

		debug("ctx:%d res_hand:%d size:%lu\n",p_ctx->ctx_hndl,my_res_hndl[i],actual_size);
		size += 16;
	}

	for(i=0;i<MAX_RES_HANDLE;i++)
	{
		rc = mc_close(p_ctx->mc_hndl,my_res_hndl[i]);
		if(rc != 0) {
		fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
		g_error = -1;
		return NULL;
    	}
	}
	
	rc = mc_unregister(p_ctx->mc_hndl);
    if(rc != 0)
    {
        fprintf(stderr, "mc_unregister failed for mc_hdl %p\n", p_ctx->mc_hndl);
		g_error = -1;
		return NULL;
    }
	debug("mc unregistered for ctx:%d\n",p_ctx->ctx_hndl);
	return 0;
}

int test_max_ctx_n_res()
{
	int rc;
	int i;
	pthread_t threads[MAX_OPENS];
	struct ctx *p_ctx;

	if(mc_init() !=0 ) {
	fprintf(stderr, "mc_init failed.\n");
	return -1;
	}

	debug("mc_init success.\n");
	
	rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*MAX_OPENS);
	if(rc != 0)
	{
		fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
		return -1;
	}

	//Creating threads for ctx_init
	for(i = 0; i < MAX_OPENS; i++) {
	rc = pthread_create(&threads[i],NULL, (void *)&max_ctx_res, (void *)&p_ctx[i]);
	if(rc) {
		fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
		free(p_ctx);
		return -1;
		}
	}

	//joining
	for(i = 0; i < MAX_OPENS; i++) {
		pthread_join(threads[i], NULL);
	}
	for(i = 0; i < MAX_OPENS; i++) {
		ctx_close(&p_ctx[i]);
	}
	free(p_ctx);
	rc = g_error;
	g_error = 0;
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
	__u64 stride = 0x1000;
	pthread_t thread;
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
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
		(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
	if(rc != 0) {
        fprintf(stderr, "mc_register: failed. ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
		return -1;
	}
	debug("ctx hand %d & mc hnd:%p\n", p_ctx->ctx_hndl, p_ctx->mc_hndl);
	rc = mc_open(p_ctx->mc_hndl,MC_RDWR,&p_ctx->res_hndl);
	if(rc != 0) {
		fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
		return -1;
	}
	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, aun, &actual_size);
	if(rc != 0) {
		fprintf(stderr,"mc_size: failed,rc %d\n", rc);
		return -1;
	}

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
	if((rc != 0) || (aun != l_mc_stat.size))
	{
		fprintf(stderr,"mc_get_size failed rc =%d: %lu : %lu\n", rc,aun,actual_size);
		return -1;
	}
	debug("mc_stat:\nblk_len = 0X%X\nnmask = 0X%X\nsize = 0X%lX\nflags = 0X%lX\n",
			l_mc_stat.blk_len, l_mc_stat.nmask, l_mc_stat.size, l_mc_stat.flags);
	//printf("ctx:%d res_hand:%d vdisk unit: %lu\n",p_ctx->ctx_hndl,p_ctx->res_hndl,actual_size);
	nlba = aun*(1 << l_mc_stat.nmask);
	size = nlba*(l_mc_stat.blk_len);
	debug("ONE AUN = %lu(0x%lx) LBAs and One AUN size =%lu(0x%lx)Bytes\n",nlba,nlba,size,size); 
	debug("ONE AUN = %lu MB\n",size/(1024*1024));

	pid = getpid();
	rc = send_single_write(p_ctx, nlba-1, pid);
	rc = send_single_read(p_ctx,  nlba-1);
	stride = LBA_BLK;
	rc = send_write(p_ctx, nlba/2, stride, pid, VLBA);
	rc += send_read(p_ctx, nlba/2, stride, VLBA);
	rc +=  rw_cmp_buf(p_ctx, nlba/2);
	CHECK_RC(rc, "IO");

	pthread_cancel(thread);
	rc = mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	if(rc != 0) {
		fprintf(stderr,"mc_close failed rc=%d\n",rc);
		return -1;
	}
	rc = mc_unregister(p_ctx->mc_hndl);
	if(rc != 0) {
		fprintf(stderr,"mc_unregister failed rc=%d\n",rc);
		return -1;
	}
	ctx_close(p_ctx);	
	return 0;
}

int test_mc_clone_error(__u32 oflg, __u32 cnflg)
{
	int rc,rc1,rc2;
	struct ctx testctx;
	mc_hndl_t mc_hndl_old;
	mc_hndl_t mc_hndl;
	pid_t child_pid;
	pid_t pid;
	__u64 chunks =16;
	__u64 newchunks = 32;
	__u64 actual_size;
	__u64 stride = 0x1000;
	__u64 nlba=0;
	__u64 st_lba=0;
	struct ctx  *p_ctx = &testctx;
	pthread_t thread;
	mc_stat_t l_mc_stat;

	if(mc_init() !=0 ) {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
    debug("mc_init success.\n");

	if(do_basic_setting(p_ctx) != 0)
	{
		return -1;
	}
	
	mc_hndl = p_ctx->mc_hndl;
    rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
    mc_unregister(mc_hndl);

	pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
	rc = mc_open(p_ctx->mc_hndl, oflg, &(p_ctx->res_hndl));
	if (rc != 0) {
        fprintf(stderr, "error opening res_hndl rc %d\n", rc);
        return -1;
    }
	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
             chunks, &actual_size);
    if (rc != 0 || chunks != actual_size) {
        fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
		return -1;
    }

	pid = getpid();
	if(oflg & MC_WRONLY) {//true for MC_WRONLY & MC_RDWR
		for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
			rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
        	if(rc != 0)
        	{
				fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
            	return rc;
        	}
    	}
	}

	pthread_cancel(thread);
	child_pid = fork();
	if(child_pid == 0)
	{
		//child process
		mc_hndl_old = p_ctx->mc_hndl;
		if(do_basic_setting(p_ctx) != 0)
		{
			exit(-1);
		}

		//do mc_clone 
		mc_hndl = p_ctx->mc_hndl;
		rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
		mc_unregister(mc_hndl);

		rc = mc_clone(p_ctx->mc_hndl, mc_hndl_old, cnflg);
		if (rc != 0) {
			fprintf(stderr, "error cloning rc %d\n", rc);
			exit(-1);
        	}

		// close parent inherited interfaces
		rc = mc_unregister(mc_hndl_old);

		pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
		rc = mc_stat(p_ctx->mc_hndl,p_ctx->res_hndl,&l_mc_stat);
		if (rc != 0) {
			fprintf(stderr, "error in mc_stat rc %d\n", rc);
			exit(-1);
        }
		pid = getpid();
		//printf("pid:%u chunks:%lu actual_size: %lu\n",pid,chunks,actual_size);
		actual_size = l_mc_stat.size;
		if(chunks != actual_size)
		{
			fprintf(stderr, "size mismatch rc %d\n", rc);
            		exit(-1);
		}

		nlba = chunks * (1 << l_mc_stat.nmask);
		st_lba = nlba/2;
		if(cnflg == MC_RDONLY){
			rc = send_write(p_ctx, st_lba, stride, pid,VLBA);
			if(rc == 0){
				fprintf(stderr,"write should fail MC_RDONLY clone 0X%lX vlba\n",st_lba);
				exit(-1);
			}
			g_error = rc;
		}
		else if(cnflg == MC_WRONLY)
		{
			rc = send_read(p_ctx,st_lba,stride,VLBA);
			if(rc == 0){
				fprintf(stderr,"read should fail for MC_WRONLY clone 0X%lX vlba\n",st_lba);
				exit(-1);
			}
			g_error = rc;
		}
		else if(cnflg == MC_RDWR)
		{
			rc1 = send_write(p_ctx, st_lba, stride, pid,VLBA);
			rc2 = send_read(p_ctx,st_lba,stride,VLBA);
			if((rc1 == 0) && (rc2 == 0))
			{
				fprintf(stderr,"Read/Write should fail @ 0X%lX vlba\n",st_lba);
				exit(-1);
			}
			if(rc1)
				g_error = rc1;
			else
				g_error = rc2;
		}
		
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
                    newchunks,&actual_size);
		//printf("pid:%u newchunks:%lu actual_size: %lu\n",pid,newchunks,actual_size);
        if (rc != 0 || newchunks != actual_size) {
            fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
            exit(-1);
        }
		newchunks = 0;
		//printf("Calling again mc_size with 0 value\n");
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
                    0,&actual_size);

		//printf("pid:%u newchunks:%lu actual_size: %lu\n",pid,newchunks,actual_size);
        if (rc != 0 || newchunks != actual_size) {
            fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
            exit(-1);
        }
		pthread_cancel(thread);
		mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
		mc_unregister(p_ctx->mc_hndl);
		exit(g_error);
	}
	else
	{
		//let child process done cloning
		sleep(2);
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,
					newchunks,&actual_size);
		if (rc != 0 || newchunks != actual_size) {
        	fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
        	return -1;
    	}
		debug("pid:%u newchunks:%lu actual_size: %lu\n",pid,newchunks,actual_size);
		mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
		mc_unregister(p_ctx->mc_hndl);
		debug("pid = %u, cpid = %u\n", pid, child_pid);
		fflush(stdout);
		child_pid = wait(&rc);
		if (WIFEXITED(rc)) {
        	rc =  WEXITSTATUS(rc);
        }
		debug("%d terminated, signalled %s, signo %d\n",
                   child_pid, WIFSIGNALED(rc) ? "yes" : "no", WTERMSIG(rc));
        	fflush(stdout);
	}
	ctx_close(p_ctx);	
	mc_term();
	return rc;
}

void *exploit_chunk(void *arg)
{
	struct ctx *p_ctx = (struct ctx*)arg;
    mc_hndl_t mc_hndl = p_ctx->mc_hndl;
    int rc=0;
	int i;
    res_hndl_t res_handl;
    __u64 size = 16;
    __u64 actual_size=0;
    __u64 plba=0;
	__u64 st_lba;
	__u64 nlba;
	int myloop = 5;
	int inner_loop =2;
	__u64 stride;
	mc_stat_t l_mc_stat;
	bool is_regress = false;
	
	char *str = getenv("LONG_RUN");
	if((str != NULL) && !strcmp(str, "TRUE")) {
		myloop = 100;
		inner_loop = 1000;
		is_regress = true;
		debug("%d : %s : %d :Regress Outerloop: %d & inner loop:%d\n",
				pid, __func__, __LINE__, myloop, inner_loop);
	}
	while(myloop > 0){
	pthread_mutex_lock(&mutex);
    rc = mc_open(mc_hndl, MC_RDWR, &res_handl);
    pthread_mutex_unlock(&mutex);
    if(rc != 0) {
        fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
        g_error = -1;
        return NULL;
    }
	debug_2("%d : rsh %d started\n", pid, res_handl);
	for(i = 0; i< inner_loop;i++){
	pthread_mutex_lock(&mutex);
    rc = mc_size(mc_hndl, res_handl, size,&actual_size);
    pthread_mutex_unlock(&mutex);
    if(rc != 0) {
        fprintf(stderr, "ctx: %d:mc_size: failed,rc %d\n", p_ctx->ctx_hndl,rc);
        g_error = -1;
        return NULL;
    	}
	pthread_mutex_lock(&mutex);
    rc = mc_stat(mc_hndl, res_handl, &l_mc_stat);
    pthread_mutex_unlock(&mutex);
    if(rc != 0) {
        fprintf(stderr, "ctx: %d:mc_stat: failed,rc %d\n", p_ctx->ctx_hndl,rc);
        g_error = -1;
        return NULL;
		}
	nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
	stride = 1 << l_mc_stat.nmask;
	if(is_regress)
		stride = 0x1000;

	debug_2("%d : R/W started for rsh %d from 0X0 to 0X%lX\n",
			pid, res_handl, nlba-1);
	for(st_lba = 0; st_lba < nlba; st_lba += NUM_CMDS*stride) {
		debug_2("%d : start lba 0X%lX total lba 0X%lX rsh %d\n",
				pid, st_lba, nlba,res_handl);
		pthread_mutex_lock(&mutex);
		p_ctx->res_hndl = res_handl;
		rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
		if(rc) {
			mc_stat(mc_hndl, res_handl, &l_mc_stat);
			if(size == 0 || (l_mc_stat.size * (1 << l_mc_stat.nmask)) <= st_lba) {
				printf("%d : Fine, send write(0X%lX) was out of bounds, MAX LBAs(0X%lX)\n",
					pid, st_lba, size * (1 << l_mc_stat.nmask));
				pthread_mutex_unlock(&mutex);
				break;
			}else {
				g_error = -1;
				fprintf(stderr,"%d : chunk(0X%lX)IO failed rsh %d st_lba(0X%lX) range(0X%lX)\n",
						pid, size, res_handl, st_lba, nlba-1);
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
		}else {
			rc = send_read(p_ctx, st_lba, stride,VLBA);
			rc += rw_cmp_buf(p_ctx, st_lba);
			pthread_mutex_unlock(&mutex);
			if(rc){
				g_error = -1;
				return NULL;
			}
		}
	}
		debug_2("%d : R/W done for rsh %d from 0X0 to 0X%lX\n",
			pid, res_handl, nlba-1);
		size = (rand()%10+1)*16;
	}
	pthread_mutex_lock(&mutex);
    rc = mc_stat(mc_hndl, res_handl, &l_mc_stat);
    pthread_mutex_unlock(&mutex);
	size= l_mc_stat.size;
	if(size != 0){
		pthread_mutex_lock(&mutex);
		rc = mc_xlate_lba(mc_hndl, res_handl, 0,&plba);
		pthread_mutex_unlock(&mutex);
		if(rc != 0) {
			fprintf(stderr, "ctx: %d:mc_xlate_lba: failed,rc %d\n", p_ctx->ctx_hndl,rc);
			g_error = -1;
			return NULL;
    	}
	}
	pthread_mutex_lock(&mutex);
    rc = mc_close(mc_hndl, res_handl);
    pthread_mutex_unlock(&mutex);
	debug_2("%d : now closing rsh %d\n", pid, res_handl);
    if(rc != 0) {
        fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
        g_error = -1;
        return NULL;
    	}
	myloop--;
	debug("%d : %d loop remains was rsh %d\n", pid, myloop, res_handl);
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

	pid = getpid();
	debug("%d : afu=%s and master=%s\n",pid, afu_path, master_dev_path);

    if(mc_init() !=0 ) {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
    debug("%d : mc_init success.\n", pid);

    //Allocating structures for pthreads.
    p_thread_a = (struct pthread_alloc *) malloc(sizeof(struct pthread_alloc) * MAX_NUM_THREADS);
    if(p_thread_a == NULL) {
        fprintf(stderr, " Can not allocate thread structs, errno %d\n", errno);
        return -1;
    }
    rc = ctx_init(p_ctx);
    if(rc != 0)
    {
        fprintf(stderr, "Context initialization failed, errno %d\n", errno);
        return -1;
    }
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
    rc = mc_register(master_dev_path, p_ctx->ctx_hndl, (volatile __u64 *)p_ctx->p_host_map,
                    &p_ctx->mc_hndl);
    if(rc != 0) {
        fprintf(stderr, "mc_register: failed. Error registering ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
        return -1;
    }
    debug_2("%d : ctx hand & mc hnd: %d & %p\n",
		pid, p_ctx->ctx_hndl, p_ctx->mc_hndl);

    //initialize the mutex
    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&mutex, &mattr);

    //create threads
    for(i=0;i< MAX_NUM_THREADS; ++i)
    {
        rc = pthread_create(&(p_thread_a[i].rrq_thread), NULL, &exploit_chunk, (void *)p_ctx);
        if(rc) {
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            return -1;
        }
    }

    //destroy mutexattr
    pthread_mutexattr_destroy(&mattr);
    //joining
    for(i=0;i< MAX_NUM_THREADS; ++i)
    {
        pthread_join(p_thread_a[i].rrq_thread, NULL);
    }

    //destroy the mutex
    pthread_mutex_destroy(&mutex);

	pthread_cancel(thread);
    rc = mc_unregister(p_ctx->mc_hndl);
    if(rc != 0)
    {
        fprintf(stderr, "mc_unregister failed for mc_hdl %p\n", p_ctx->mc_hndl);
        return -1;
    }

    ctx_close(p_ctx);
    mc_term();
    //free allocated space
    free(p_thread_a);
    rc = g_error;
    g_error =0;
	debug("%d : I am returning %d\n", pid, rc);
    return rc;
}

int mc_size_regress_internal()
{
	int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=32;
    __u64 actual_size=0;
    __u64 nlba;
    __u64 stride = LBA_BLK;
	__u32 i;
	int mc_size_regrss_l = 10;
	mc_stat_t l_mc_stat;
    pthread_t thread;
	pid = getpid();

	char *str = getenv("LONG_RUN");
	if((str != NULL) && !strcmp(str, "TRUE")) {
		mc_size_regrss_l = 8000;
	}
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

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");

	for(i = 1 ; i <= mc_size_regrss_l; i++) {
		chunks = rand()%100 +1;
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,chunks, &actual_size);
		CHECK_RC(rc, "mc_size");
		if(actual_size) {
			nlba = actual_size * (1 << l_mc_stat.nmask);
			debug("%d : IO from 0X%lX to 0X%lX\n",
				pid, nlba-LBA_BLK*NUM_CMDS, nlba-1);
			rc = send_write(p_ctx, nlba-LBA_BLK*NUM_CMDS, stride, pid,VLBA);
			CHECK_RC(rc, "send_write");
			rc = send_read(p_ctx, nlba-LBA_BLK*NUM_CMDS, stride,VLBA);
			CHECK_RC(rc, "send_read");
			rc = rw_cmp_buf(p_ctx, nlba-LBA_BLK*NUM_CMDS);
			CHECK_RC(rc, "send_read");
		}
		rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl,0, &actual_size);
		CHECK_RC(rc, "mc_size");
		if( i % 1000 == 0)
			printf("%d : loop %d(%d) done...\n", pid, i, mc_size_regrss_l);
	}
	pthread_cancel(thread);
	return 0;
}

int mc_test_chunk_regress(int cmd)
{
	int rc;
	int i;
	pid_t pid1;
	int max_p= 2;

	pid = getpid();
	char *str = getenv("LONG_RUN");
	if((str != NULL) && !strcmp(str, "TRUE")) {
		max_p = MAX_OPENS;
		debug("%d : Do %s Regress for %d context processes\n",
				pid, __func__, max_p);
		if(4 == cmd)
			debug("mc_size api(0 to value & viceversa) Regress 1000 loops...\n");
	}
	for(i = 0; i< max_p;i++)
	{
		if(fork() == 0)
		{
			debug("%d process created...\n",i+1);
			usleep(1000);
			if(1 == cmd) // chunk regress
				rc = chunk_regress();
			else if(4 == cmd) //mc_size regress
				rc = mc_size_regress_internal();
			else //ctx regress create &destroy with io & wo io
				rc = mc_test_ctx_regress(cmd);

			if(rc)
				debug("%d : exiting with rc = %d\n", pid, rc);

			fflush(stdout);
			exit(rc);
		}
	}

	while((pid1 =  waitpid(-1,&rc,0)))
	{
		if (WIFEXITED(rc)) {
			rc =  WEXITSTATUS(rc);
			if(rc)
				g_error = -1;
		}
		
		if(WIFSIGNALED(rc)) {
			debug("%d :  killed by %d signal\n", pid1, WTERMSIG(rc));
			if(WCOREDUMP(rc))
				fprintf(stderr, "%d :  was core dupmed ...\n", pid1);
		}

		debug("pid %d exited with rc = %d\n", pid1, rc);

		if(pid1 == -1)
			break;
	}
	fflush(stdout);
	rc = g_error;
	g_error = 0;
	return rc;
}

int mc_test_chunk_regress_long()
{
	int rc;
	int i;
	int lrun=2;
	char *str = getenv("LONG_RUN");
	if((str != NULL) && !strcmp(str, "TRUE")) {
		lrun = 100;
		printf("%d : Do %s Regress loop : %d\n",
			pid, __func__, lrun);
			fflush(stdout);
	}
	pid = getpid();
	for(i = 1; i <= lrun; i++){
		debug("Loop %d(%d) started...\n", i, lrun);
		rc = mc_test_chunk_regress(1);
		debug("Loop %d(%d) done...\n", i, lrun);
		if(i%10 == 0){
			printf("Loop %d(%d) done...\n", i, lrun);
			fflush(stdout);
		}
		if(rc){
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
	pid_t pid;
	int max_p = 4;
	char l_afu[MC_PATHLEN];
	char l_master[MC_PATHLEN];
	char buffer[MC_PATHLEN];
	char *str;
	char *afu_1 = "0.0s";
	char *afu_2 = "1.0s";
	strcpy(l_afu, afu_path);
	strcpy(l_master, master_dev_path);
	for(i = 0; i < max_p; i++) {
		if(i%2){
			strcpy(afu_path, l_afu);
			strcpy(master_dev_path, l_master);
		}
		else {
			str = strstr(l_afu, afu_1);
			if(str == NULL) {//ENV var set with 1.0
				strncpy(buffer, l_afu, strlen(l_afu)-strlen(afu_2));
				buffer[strlen(l_afu)-strlen(afu_2)]='\0';
				strcat(buffer, afu_1);
			}else {
				strncpy(buffer, l_afu, strlen(l_afu)-strlen(afu_1));
				buffer[strlen(l_afu)-strlen(afu_1)]='\0';
				strcat(buffer, afu_2);
			}
			strcpy(afu_path, buffer);
			strncpy(master_dev_path, afu_path, strlen(afu_path)-1);
			master_dev_path[strlen(afu_path)-1] ='\0';
			strcat(master_dev_path, "m");
		}
	  if(fork() == 0) {
		rc =chunk_regress();
		exit(rc);
	  }
	}

	while((pid = waitpid(-1, &rc, 0))) {
		if(rc != 0) {
			g_error = -1;
		}
		debug("pid %d exited. \n", pid);
		fflush(stdout);

		if(pid == -1) {
			break;
		}
	}

	rc = g_error;
	g_error = 0;

	return rc;
}

int test_mc_lun_size(int cmd)
{
	int rc;
	int rc1 =0;
	struct ctx myctx;
	struct ctx *p_ctx = &myctx;
	__u64 max_chunks = 0xFFFFFF0000000000;
	__u64 actual_size;
	__u64 nlba;
	__u64 size;
	__u64 plba = 0;
	__u64 stride;
	__u64 st_lba =0;
	pthread_t thread;
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
	pthread_create(&thread,NULL,ctx_rrq_rx, p_ctx);
	rc = mc_register(master_dev_path, p_ctx->ctx_hndl,
		(volatile __u64 *)p_ctx->p_host_map,&p_ctx->mc_hndl);
	if(rc != 0) {
        fprintf(stderr, "mc_register: failed. ctx_hndl %d, rc %d\n",p_ctx->ctx_hndl, rc );
		return -1;
	}
	debug("ctx hand %d & mc hnd:%p\n", p_ctx->ctx_hndl, p_ctx->mc_hndl);
	rc = mc_open(p_ctx->mc_hndl,MC_RDWR,&p_ctx->res_hndl);
	if(rc != 0) {
		fprintf(stderr, "ctx: %d:mc_open: failed,rc %d\n", p_ctx->ctx_hndl,rc);
		return -1;
	}
	rc = mc_size(p_ctx->mc_hndl, p_ctx->res_hndl, max_chunks, &actual_size);
	if(rc != 0) {
		fprintf(stderr,"mc_size: failed,rc %d\n", rc);
		return -1;
	}

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl,&l_mc_stat);
	if(rc != 0)
	{
		fprintf(stderr,"mc_get_size failed rc =%d\n", rc);
		return -1;
	}
	//printf("ctx:%d res_hand:%d vdisk unit: %lu\n",p_ctx->ctx_hndl,p_ctx->res_hndl,actual_size);
	stride = 1 << l_mc_stat.nmask;
	actual_size = l_mc_stat.size;
	nlba = actual_size * (1 << l_mc_stat.nmask);
	size = nlba * l_mc_stat.blk_len;
	debug("LUN = %lu(0x%lx) LBAs and One LUN size =%lu(0x%lx)Bytes\n",nlba,nlba,size,size); 
	printf("LUN = %lu GB\n",size/(1024*1024*1024));

	if(1 == cmd) { //Test PLBA out of range
		plba = nlba;
		pid = getpid();
		rc1 = send_write(p_ctx, plba, stride, pid, NO_XLATE);
	}else if(2 == cmd) {
		//start writing plba 
		nlba = (1 << l_mc_stat.nmask);
		//nlba = 10 * (1 << l_mc_stat.nmask);
		for(st_lba = 0; st_lba < nlba; st_lba++) {
			rc = send_write(p_ctx, st_lba, stride, st_lba, NO_XLATE);
			if(rc) {
				fprintf(stderr,"send write failed for 0X%lX PLBA\n",st_lba);
				return rc;
			}
		}
		//now start reading vlba
		for(st_lba = 0; st_lba < nlba; st_lba++) {
			rc = send_read(p_ctx, st_lba, stride, VLBA);
			CHECK_RC(rc, "send_read");
			rc = mc_xlate_lba(p_ctx->mc_hndl, p_ctx->res_hndl, st_lba, &plba);
			CHECK_RC(rc, "mc_xlate_lba");

			if(memcmp((__u64*)&p_ctx->rbuf[0][0], &plba, sizeof(__u64)))
			{
				fprintf(stderr, "miscompare of 0X%lX & data \n",plba);
				return -1;
			}
		} 
	}
	pthread_cancel(thread);
	rc = mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	if(rc != 0) {
		fprintf(stderr,"mc_close failed rc=%d\n",rc);
		return -1;
	}
	rc = mc_unregister(p_ctx->mc_hndl);
	if(rc != 0) {
		fprintf(stderr,"mc_unregister failed rc=%d\n",rc);
		return -1;
	}
	ctx_close(p_ctx);	
	if(1 == cmd){
		return rc1;
	}
	return 0;
}

int test_mc_dup_api()
{
	int rc;
	struct ctx *p_ctx;
	struct ctx *main_p_ctx;
	struct ctx *dup_p_ctx;
	res_hndl_t my_res_hndl[MAX_RES_HANDLE];
	__u64 size = 1;
	__u64 actual_size;
	int i;

	if(mc_init() !=0 ) {
    fprintf(stderr, "mc_init failed.\n");
    return -1;
    }
    debug("mc_init success.\n");

	rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*2);
    if(rc != 0)
    {
        fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
        return -1;
    }

	main_p_ctx = &p_ctx[0];
	dup_p_ctx = &p_ctx[1];

	rc = ctx_init(main_p_ctx);
    if(rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        return -1;
    }
	rc = ctx_init(dup_p_ctx);
    if(rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        return -1;
    }

    rc = mc_register(master_dev_path, main_p_ctx->ctx_hndl,
        (volatile __u64 *)main_p_ctx->p_host_map,&main_p_ctx->mc_hndl);
    if(rc != 0) {
        fprintf(stderr, "mc_register: failed. ctx_hndl %d, rc %d\n",main_p_ctx->ctx_hndl, rc );
        return -1;
    }
	debug("ctx hand %d & mc hnd:%p\n", main_p_ctx->ctx_hndl, main_p_ctx->mc_hndl);

	rc = mc_dup(main_p_ctx->mc_hndl,dup_p_ctx->mc_hndl);
	if(rc != 0) {
        fprintf(stderr, "mc_dup: failed. rc %d\n",rc );
		ctx_close(main_p_ctx);
		ctx_close(dup_p_ctx);
		mc_term();
        return -1;
    }
	dup_p_ctx->mc_hndl = main_p_ctx->mc_hndl;

	for(i = 0; i< MAX_RES_HANDLE; i++)
	{
		rc = mc_open(main_p_ctx->mc_hndl,MC_RDWR,&my_res_hndl[i]);
		if(rc != 0) {
			fprintf(stderr, "mc_open: failed,rc %d\n",rc);
			return -1;
		}
		rc = mc_size(main_p_ctx->mc_hndl,my_res_hndl[i],size,&actual_size);
		if(rc != 0) {
            fprintf(stderr, "mc_size: failed,rc %d\n", rc);
			return -1;
        }
        if(size != actual_size)
        {
            fprintf(stderr,"ctx: %d:size mismatched: %lu : %lu\n", main_p_ctx->ctx_hndl,size,actual_size);
            return -1;
        }
		
	}
	for(i = 0; i< MAX_RES_HANDLE; i++)
	{
		rc = mc_close(main_p_ctx->mc_hndl,my_res_hndl[i]);
		if(rc != 0) {
			fprintf(stderr, "mc_open: failed,rc %d\n",rc);
			return -1;
		}
	}
	mc_unregister(main_p_ctx->mc_hndl);
	if(rc != 0) {
        fprintf(stderr,"mc_unregister failed rc=%d\n",rc);
        return -1;
    }

	ctx_close(main_p_ctx);
	ctx_close(dup_p_ctx);
	mc_term();
	return 0;
	
}

int test_mc_clone_many_rht()
{
    int rc;
	int i;
    struct ctx testctx;
    mc_hndl_t mc_hndl_old;
	mc_hndl_t mc_hndl;
    pid_t child_pid;
    __u64 chunks =16;
    __u64 actual_size =0;
    __u64 stride=0;
    __u64 st_lba;
    __u64 nlba;
    struct ctx  *p_ctx = &testctx;
    pthread_t thread;
	res_hndl_t my_rsh[MAX_RES_HANDLE];
	res_hndl_t RES_CLOSED = 0xFFFF;
	res_hndl_t closed[5] = {1,5,8,10,14};
	mc_stat_t l_mc_stat;

    if(mc_init() !=0 ) {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }
        debug("mc_init success.\n");

    if(do_basic_setting(p_ctx) != 0) {
        return -1;
    }

	mc_hndl = p_ctx->mc_hndl;
    rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
    mc_unregister(mc_hndl);

    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
	for(i = 0; i < MAX_RES_HANDLE; i++)
	{
		chunks = (i+1)*16;
		rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &my_rsh[i]);
		if (rc != 0) {
			fprintf(stderr, "error opening res_hndl rc %d\n", rc);
			return rc;
		}
		rc = mc_size(p_ctx->mc_hndl, my_rsh[i], chunks, &actual_size);
		if (rc !=0) {
			fprintf(stderr, "error sizing res_hndl rc %d\n", rc);
			return rc;
		}
    }
	
	for(i = 0; i < 5; i++)
	{
		rc = mc_close(p_ctx->mc_hndl, my_rsh[closed[i]]);
		if(rc) {
			fprintf(stderr, "error close res_hndl rc %d\n", rc);
			return rc;
		}
		my_rsh[closed[i]] = RES_CLOSED;
    }
    
    pid = getpid();
	for(i = 0; i < MAX_RES_HANDLE; i++) {
		if(RES_CLOSED == my_rsh[i]){
			continue;
		}
		rc = mc_stat(p_ctx->mc_hndl, my_rsh[i], &l_mc_stat);
		if(rc) {
			fprintf(stderr, "mc_stat failed, rc = %d\n",rc);
			return -1;
		}
		actual_size = l_mc_stat.size;
		stride = (1 << l_mc_stat.nmask);
		nlba = actual_size * (1 << l_mc_stat.nmask);
		p_ctx->res_hndl = my_rsh[i];
		for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
			rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
			if(rc != 0)
			{
				fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
				return rc;
			}
		}
    }
	pthread_cancel(thread);
    child_pid = fork();
    if(child_pid == 0)
    {
        //child process
        mc_hndl_old = p_ctx->mc_hndl;
        if(do_basic_setting(p_ctx) != 0)
        {
            exit(-1);
        }
		mc_hndl = p_ctx->mc_hndl;
		rc = mc_hdup(mc_hndl, &p_ctx->mc_hndl);
		mc_unregister(mc_hndl);

        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
        //do mc_clone
        rc = mc_clone(p_ctx->mc_hndl, mc_hndl_old, MC_RDWR);
        if (rc != 0) {
                fprintf(stderr, "error cloning rc %d\n", rc);
                exit(-1);
        }

        // close parent inherited interfaces
        rc = mc_unregister(mc_hndl_old);
		for(i = 0; i < MAX_RES_HANDLE; i++) {
			if(RES_CLOSED == my_rsh[i]){
				continue;
				}
			rc = mc_stat(p_ctx->mc_hndl, my_rsh[i], &l_mc_stat);
			if(rc) {
				fprintf(stderr, "mc_stat failed, rc = %d\n",rc);
				exit(-1);
			}
			nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
			p_ctx->res_hndl = my_rsh[i];
			for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
				rc = send_read(p_ctx, st_lba, stride, VLBA);
				rc += rw_cmp_buf_cloned(p_ctx, st_lba);
				if(rc != 0)
				{
					fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
					exit(rc);
				}
			}
		}

		pthread_cancel(thread);
		for(i = 0; i < MAX_RES_HANDLE; i++) {
			if(RES_CLOSED == my_rsh[i]){
				continue;
			}
			mc_close(p_ctx->mc_hndl, my_rsh[i]);
		}
        mc_unregister(p_ctx->mc_hndl);
        exit(0);
    }
    else
    {
        //let child process done cloning
        sleep(2);
		//lets open all
		chunks = 16;
        pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
		for(i = 0; i < 5; i++) {
			rc = mc_open(p_ctx->mc_hndl,MC_RDWR, &p_ctx->res_hndl);
			if(rc) {
				fprintf(stderr, "mc open failed, rc = %d\n", rc);
				return -1;
			}
			closed[i]=p_ctx->res_hndl;
			rc = mc_size(p_ctx->mc_hndl,p_ctx->res_hndl,chunks,&actual_size);
			if(rc) {
				fprintf(stderr, "mc size failed, rc = %d\n", rc);
				return -1;
			}
		 chunks = (i+1)*16;
		}
		
		for(i = 0; i < MAX_RES_HANDLE; i++) {
			if(RES_CLOSED == my_rsh[i]){
				continue;
			}
			p_ctx->res_hndl = my_rsh[i];
			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
			if(rc) {
				fprintf(stderr, "mc_stat failed, rc = %d\n",rc);
				exit(-1);
			}
			actual_size = l_mc_stat.size;
			nlba = actual_size * (1 << l_mc_stat.nmask);
			
			for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
				rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
				rc += send_read(p_ctx, st_lba, stride, VLBA);
				rc += rw_cmp_buf(p_ctx, st_lba);
				if(rc != 0)
				{
					fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
					return rc;
				}
			}
		}
		for(i = 0; i < 5; i++) {
			p_ctx->res_hndl = closed[i];
			rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
			if(rc) {
				fprintf(stderr, "mc_stat failed, rc = %d\n",rc);
				exit(-1);
			}
			nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
			
			for (st_lba = 0; st_lba < nlba; st_lba += (NUM_CMDS*stride)) {
				rc = send_write(p_ctx, st_lba, stride, pid, VLBA);
				rc += send_read(p_ctx, st_lba, stride, VLBA);
				rc += rw_cmp_buf(p_ctx, st_lba);
				if(rc != 0)
				{
					fprintf(stderr,"send write failed 0X%lX vlba\n",st_lba);
					return rc;
				}
			}
		}

		pthread_cancel(thread);
        for(i = 0; i < MAX_RES_HANDLE; i++) {
			if(RES_CLOSED == my_rsh[i]){
				continue;
			}
			mc_close(p_ctx->mc_hndl, my_rsh[i]);
		}
		for(i = 0; i < 5; i++) {
			mc_close(p_ctx->mc_hndl, closed[i]);
		}
        mc_unregister(p_ctx->mc_hndl);
        debug("pid = %u, cpid = %u\n", pid, child_pid);
        fflush(stdout);
        child_pid = wait(&rc);
        if (WIFEXITED(rc)) {
            rc =  WEXITSTATUS(rc);
        }
        debug("%d terminated, signalled %s, signo %d\n",
                   child_pid, WIFSIGNALED(rc) ? "yes" : "no", WTERMSIG(rc));
        fflush(stdout);
    }
    ctx_close(p_ctx);
    mc_term();
    return rc;
}

int ctx_null_rrq()
{
	struct ctx myctx;
	struct ctx *p_ctx = &myctx;
	int rc;
	__u64 chunks=16;
    __u64 actual_size=0;
    __u64 st_lba;
    __u64 stride;
	__u64 nlba;
	mc_stat_t l_mc_stat;
	pthread_t thread;

	pid = getpid();
	rc = mc_init();
	CHECK_RC(rc, "mc_init");
	
	rrq_c_null =1; //make hrrq_current = NULL
	rc = ctx_init(p_ctx);
	CHECK_RC(rc, "ctx_init");
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

	rc = mc_stat(p_ctx->mc_hndl, p_ctx->res_hndl, &l_mc_stat);
	CHECK_RC(rc, "mc_stat");

	nlba = actual_size * (1 << l_mc_stat.nmask);
	stride = (1 << l_mc_stat.nmask);
	for(st_lba =0; st_lba < nlba; st_lba +=(NUM_CMDS * stride)) {
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
	debug("%d : properly exiting now\n", getpid());
	fflush(stdout);
	return rc;
}
int test_many_ctx_one_rrq_curr_null()
{
	int rc = 0;
	pid_t pid;
	int max_p= 3;
	int i;
	pid_t er_pid;
    for(i = 0; i< max_p;i++) {
        if(fork() == 0) {
			debug("%d : good process created\n", getpid());
            rc = chunk_regress();
			debug("%d : good process exiting now\n", getpid());
            exit(rc);
        }
        //sleep(1);
    }
	//now create another ctx with null rrq
	er_pid = fork();
	if(er_pid == 0) {
		debug("%d : ctx_null_rrq process created\n", getpid());
		rc = ctx_null_rrq();
		debug("%d : ctx_null_rrq process exiting now \n", getpid());
		exit(rc);
	}

    while((pid =  waitpid(-1,&rc,0))) {
		if (WIFEXITED(rc)) {
			rc =  WEXITSTATUS(rc);
			if((rc != 0)&& (er_pid != pid)){ //ignore error process rc
				g_error = -1;
			}
		}
        debug("pid %d exited with rc =%d \n", pid, rc);

		if(pid == -1)
			break;
    }
    rc = g_error;
    g_error = 0;
	fflush(stdout);
    return rc;
}

int test_all_afu_devices()
{
	int rc;
	pid_t mypid;
    int i = 0;
	char afu_dev[MC_PATHLEN];
	int afu_1 = 4; //length of 0.0s or 1.0s
	int fd;

	strncpy(afu_dev, afu_path, strlen(afu_path)-afu_1);
	afu_dev[strlen(afu_path)-afu_1]='\0';
	while(1)
	{
		sprintf(afu_path, "%s%d.0s", afu_dev, i);
		sprintf(master_dev_path, "%s%d.0m", afu_dev, i);
		fd = open(afu_path, O_RDWR);
		if(fd < 0) {
			debug("No more afu devices left\n");
			break;
		}
		close(fd);
		if(fork() == 0){
			printf("Running IO on AFU %s\n", afu_path);
			rc = chunk_regress();
			exit(rc);
		}
		i++;
	}
 
    while((mypid =  waitpid(-1,&rc,0)))
    {
        if(rc != 0){
            g_error = -1;
        }
        debug("pid %d exited...\n", mypid);

		if(mypid == -1) {
			break;
        }
    }
    rc = g_error;
    g_error = 0;
	fflush(stdout);
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
	if((str != NULL) && !strcmp(str, "TRUE")) {
		myloop = 100;
	}

	pid = getpid();
    rc = mc_init();
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
	stride = 1 << l_mc_stat.nmask;
	debug("%d : chunk(0X%lX) & lba range(0X%lX)\n", pid, actual_size, nlba-1);
	while(myloop-- > 0) {
		for(st_lba =0; st_lba < nlba; st_lba += (NUM_CMDS * stride)) {
			//in bound
			send_write(p_ctx, st_lba, stride, pid, VLBA);
			send_read(p_ctx, st_lba, stride,VLBA);
			//out bound
			send_write(p_ctx, nlba +st_lba, stride, pid, VLBA);
			send_read(p_ctx, nlba +st_lba, stride,VLBA);
		}
	}
	pthread_cancel(thread);
	mc_unregister(p_ctx->mc_hndl);
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
	int lloop = 5;
	char buffer[MC_PATHLEN];
    char *str;
    char *afu_1 = "0.0s";
    char *afu_2 = "1.0s";
	int j;

	char *str1 = getenv("LONG_RUN");
	if((str1 != NULL) && !strcmp(str1, "TRUE")) {
		lloop = 100;
	}
	pid = getpid();
	debug("%d : Good path on %s\n", pid, afu_path);
	//now create a child process & do error path
	if(fork() == 0)
	{
		dont_displa_err_msg = 1;
		pid = getpid();
		str = strstr(afu_path, afu_1);
		if(str == NULL) {//ENV var set with 1.0
			strncpy(buffer, afu_path, strlen(afu_path)-strlen(afu_2));
			buffer[strlen(afu_path)-strlen(afu_2)]='\0';
			strcat(buffer, afu_1);
        }else {
			strncpy(buffer, afu_path, strlen(afu_path)-strlen(afu_1));
			buffer[strlen(afu_path)-strlen(afu_1)]='\0';
			strcat(buffer, afu_2);
        }

		strcpy(afu_path, buffer);
		strncpy(master_dev_path, afu_path, strlen(afu_path)-1);
		master_dev_path[strlen(afu_path)-1]='\0';
		strcat(master_dev_path, "m");
		debug("%d : Error path on afu %s and master %s\n",
			pid, afu_path, master_dev_path);
		debug("%d: error path process started..\n", pid);
		for(i = 0; i < lloop; i++) {
			debug("%d : starting loop %d(%d)\n", pid, i , lloop);
			rc = test_mix_in_out_bound_lba();

			for(j=1; j<=13; j++) {
				rc = test_mc_invalid_ioarcb(j);
			}
			rc = mc_test_engine(TEST_MC_RW_CLS_RSH);
			rc = mc_test_engine(TEST_MC_UNREG_MC_HNDL);
		}
		debug("%d: error path process exited..\n", pid);
		exit(rc);
	} else {
		debug("%d: Good path process started..\n", pid);
		for(i = 0; i < lloop; i++) {
			rc += chunk_regress();
		}
		debug("%d: Good path process exited..\n", pid);
		wait(&status);
	}
	return rc;
}

int mc_test_ctx_regress(int cmd)
{
	int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    __u64 chunks=32;
    __u64 actual_size=0;
    __u64 nlba;
    __u64 stride = LBA_BLK;
	mc_stat_t l_mc_stat;
    pthread_t thread;

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

	if(3 == cmd){
		nlba = actual_size * (1 << l_mc_stat.nmask);
		rc = send_write(p_ctx, nlba-LBA_BLK*NUM_CMDS, stride, pid,VLBA);
		CHECK_RC(rc, "send_write");
		rc = send_read(p_ctx, nlba-LBA_BLK*NUM_CMDS, stride,VLBA);
		CHECK_RC(rc, "send_read");
		rc = rw_cmp_buf(p_ctx, nlba-LBA_BLK*NUM_CMDS);
		CHECK_RC(rc, "rw_cmp_buf");
	}
	pthread_cancel(thread);
	mc_close(p_ctx->mc_hndl, p_ctx->res_hndl);
	mc_unregister(p_ctx->mc_hndl);
	ctx_close(p_ctx);
	return rc;
}

int test_mc_regress_ctx_crt_dstr(int cmd)
{
    int rc;
    int i;
    int lrun=10;
	char *str = getenv("LONG_RUN");
	if((str != NULL) && !strcmp(str, "TRUE")) {
		lrun = 8000;
		printf("%s : %d : Regress loop : %d\n", __func__, __LINE__, lrun);
	}
    for(i = 1; i <= lrun; i++){
        debug("Loop %d(%d) started...\n", i, lrun);
		if(1 == cmd) //mc_regress_ctx_crt_dstr without io
			rc = mc_test_chunk_regress(2);
		else //mc_regress_ctx_crt_dstr with io
			rc = mc_test_chunk_regress(3);

        debug("Loop %d(%d) done...\n", i, lrun);
		if(i%100 == 0) {
			printf("%d : Loop %d(%d) done...\n", getpid(), i, lrun);
		}
        if(rc){
            fprintf(stderr, "Loop %d is failed with rc = %d\n", i, rc);
            break;
        }
    }
    return rc;
}
