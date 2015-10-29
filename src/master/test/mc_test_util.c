/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test_util.c $                              */
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
#include <sys/time.h>

char master_dev_path[MC_PATHLEN];
char afu_path[MC_PATHLEN];

int dont_displa_err_msg = 0; //0 means display error msg
pid_t pid;
__u8 rrq_c_null = 0; //if 1, then hrrq_current = NULL

#define DATA_SEED 0xdead000000000000ull

__u64 lun_id = 0x0;
__u32 fc_port = 0x1; //will find the actual one

int g_error=0;
uint8_t rc_flags;
//set master dev path & afu device path from env
int get_fvt_dev_env()
{
    char *fvt_dev = getenv("FVT_DEV");
    if(NULL == fvt_dev) {
		fprintf(stderr, "FVT_DEV ENV var NOT set, Please set...\n");
        return -1;
    }
    strcpy(afu_path, fvt_dev);
    strncpy(master_dev_path, afu_path, strlen(afu_path)-1);
	master_dev_path[strlen(afu_path)-1]='\0';
    strcat(master_dev_path, "m");
    return 0;
}

int ctx_init_thread(void *args)
{
  return ctx_init((ctx_p)args);
}

void ctx_close_thread(void *args)
{
  ctx_close((ctx_p)args);
}

int ctx_init(struct ctx *p_ctx)
{
  int rc=0;
  void *map;
  pthread_mutexattr_t mattr;
  pthread_condattr_t cattr;
  int i;
  pid_t mypid;

  memset(p_ctx, 0, sizeof(struct ctx));
  pthread_mutexattr_init(&mattr);
  pthread_condattr_init(&cattr);

  for(i = 0; i < NUM_CMDS; i++) {
    pthread_mutex_init(&p_ctx->cmd[i].mutex, &mattr);
    pthread_cond_init(&p_ctx->cmd[i].cv, &cattr);
  }

  // open non-master device
  p_ctx->afu_fd = open(afu_path, O_RDWR);
  if(p_ctx->afu_fd < 0) {
    fprintf(stderr, "open() failed: device %s, errno %d\n", afu_path, errno);
    g_error = -1;
    return -1;
  }

  p_ctx->work.flags = CXL_START_WORK_NUM_IRQS;
  p_ctx->work.num_interrupts = 4; // use num_interrupts from AFU desc

  rc = ioctl(p_ctx->afu_fd,CXL_IOCTL_START_WORK, &p_ctx->work);
  if(rc != 0) {
    fprintf(stderr, "ioctl() failed: start command failure on AFU, errno %d\n", errno);
    g_error = -1;
    return -1;
  }

  rc = ioctl(p_ctx->afu_fd,CXL_IOCTL_GET_PROCESS_ELEMENT, &p_ctx->ctx_hndl);
  if(rc != 0) {
    fprintf(stderr, "ioctl() failed: get process element failure on AFU, errno %d\n", errno);
    g_error = -1;
    return -1;
  }

  // mmap 64KB host transport MMIO space of this context
  map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, p_ctx->afu_fd, 0);
  if(map == MAP_FAILED) {
    fprintf(stderr, "mmap() failed: errno %d\n", errno);
    g_error = -1;
    return -1;
  }

  p_ctx->p_host_map = (volatile struct sisl_host_map *)map;

  // initialize RRQ pointers
  p_ctx->p_hrrq_start = &p_ctx->rrq_entry[0];
  p_ctx->p_hrrq_end = &p_ctx->rrq_entry[NUM_RRQ_ENTRY - 1];
  if(rrq_c_null)
    p_ctx->p_hrrq_curr = NULL;
  else
    p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
  
  p_ctx->toggle = 1;

  // initialize cmd fields that never change
  for (i = 0; i < NUM_CMDS; i++) {
    p_ctx->cmd[i].rcb.msi = SISL_MSI_RRQ_UPDATED;
    p_ctx->cmd[i].rcb.rrq = 0x0;
    p_ctx->cmd[i].rcb.ctx_id = p_ctx->ctx_hndl;
  }

  // set up RRQ in AFU
  write_64(&p_ctx->p_host_map->rrq_start, (__u64) p_ctx->p_hrrq_start);
  write_64(&p_ctx->p_host_map->rrq_end, (__u64) p_ctx->p_hrrq_end);

  mypid = getpid();
  debug("%d : ctx_init() success: p_host_map %p, ctx_hndl %d, rrq_start %p\n",
  mypid, p_ctx->p_host_map, p_ctx->ctx_hndl, p_ctx->p_hrrq_start);

  pthread_mutexattr_destroy(&mattr);
  pthread_condattr_destroy(&cattr);
  return 0;
}

void ctx_close(struct ctx *p_ctx)
{
    munmap((void*)p_ctx->p_host_map, 0x10000);
    close(p_ctx->afu_fd);
}

void clear_waiting_cmds(struct ctx *p_ctx)
{
	int i;
	struct afu_cmd *p_cmd;
	if(DEBUG) {
		fprintf(stderr,
		"%d : Clearing all waiting cmds, some error occurred\n",
			getpid());
	}
	for(i = 0 ; i < NUM_CMDS; i++) {
		p_cmd = &p_ctx->cmd[i];
		pthread_mutex_lock(&p_cmd->mutex);
		p_cmd->sa.host_use_b[0] |= B_DONE;
		pthread_cond_signal(&p_cmd->cv);
		pthread_mutex_unlock(&p_cmd->mutex);
	}
}
void ctx_rrq_intr(struct ctx *p_ctx) {
    struct afu_cmd *p_cmd;

    // process however many RRQ entries that are ready
    while ((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
       p_ctx->toggle) {
    p_cmd = (struct afu_cmd*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));

    pthread_mutex_lock(&p_cmd->mutex);
    p_cmd->sa.host_use_b[0] |= B_DONE;
    pthread_cond_signal(&p_cmd->cv);
    pthread_mutex_unlock(&p_cmd->mutex);

    if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end) {
        p_ctx->p_hrrq_curr++; /* advance to next RRQ entry */
    }
    else { /* wrap HRRQ  & flip toggle */
        p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
        p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
    }
    }
}

void ctx_sync_intr(struct ctx *p_ctx) {
    __u64 reg;
    __u64 reg_unmasked;

    reg = read_64(&p_ctx->p_host_map->intr_status);
    reg_unmasked = (reg & SISL_ISTATUS_UNMASK);

    if (reg_unmasked == 0) {
		if(!dont_displa_err_msg) {
			fprintf(stderr,
				"%d: spurious interrupt, intr_status 0x%016lx, ctx %d\n",
				pid, reg, p_ctx->ctx_hndl);
		}
		clear_waiting_cmds(p_ctx);
		return;
    }

    if (reg_unmasked == SISL_ISTATUS_PERM_ERR_RCB_READ) {
		if(!dont_displa_err_msg) {
			fprintf(stderr, "exiting on SISL_ISTATUS_PERM_ERR_RCB_READ\n");
		}
		clear_waiting_cmds(p_ctx);
		pthread_exit(NULL);
    // ok - this is a signal to stop this thread
    }
    else {
		if(!dont_displa_err_msg) {
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
void *ctx_rrq_rx(void *arg) {
  struct cxl_event *p_event;
  int len;
  struct ctx   *p_ctx = (struct ctx*) arg;

  while (1) {
      //
      // read afu fd & block on any interrupt
      len = read(p_ctx->afu_fd, &p_ctx->event_buf[0],
         sizeof(p_ctx->event_buf));

      if (len < 0) {
		if(!dont_displa_err_msg)
			fprintf(stderr, "afu has been reset, exiting...\n");
		clear_waiting_cmds(p_ctx);
		return NULL;
      }

      p_event = (struct cxl_event *)&p_ctx->event_buf[0];
      while (len >= sizeof(p_event->header)) {
      if (p_event->header.type == CXL_EVENT_AFU_INTERRUPT) {
          switch(p_event->irq.irq) {
          case SISL_MSI_RRQ_UPDATED:
          ctx_rrq_intr(p_ctx);
          break;

          case SISL_MSI_SYNC_ERROR:
          ctx_sync_intr(p_ctx);
          break;

          default:
			if(!dont_displa_err_msg) {
          fprintf(stderr, "%d: unexpected irq %d, ctx %d, exiting test...\n",
              pid, p_event->irq.irq, p_ctx->ctx_hndl);
			}
			clear_waiting_cmds(p_ctx);
			return NULL;
          break;
          }

      }
      else if (p_event->header.type == CXL_EVENT_DATA_STORAGE) {
			if(!dont_displa_err_msg)
				fprintf(stderr, "failed, exiting on CXL_EVENT_DATA_STORAGE\n");
			clear_waiting_cmds(p_ctx);
			return NULL;
          // this is a signal to terminate this thread
      }
      else {
			if(!dont_displa_err_msg) {
			fprintf(stderr, "%d: unexpected event %d, ctx %d, exiting test...\n",
				pid, p_event->header.type, p_ctx->ctx_hndl);
			}
			clear_waiting_cmds(p_ctx);
			return NULL;
      }

      len -= p_event->header.size;
      p_event = (struct cxl_event *)
          (((char*)p_event) + p_event->header.size);
      }
  }

  return NULL;
}
void *ctx_rrq_rx_old(void *arg)
{
    struct cxl_event *p_event;
    struct ctx *p_ctx = (struct ctx *)arg;
    struct afu_cmd *p_cmd;

    while(1){
        // read afu fd block on RRQ written interrupt
        p_event = (struct cxl_event *)p_ctx->event_buf;
        read(p_ctx->afu_fd, p_event, sizeof(p_ctx->event_buf));
        if(p_event->header.type == CXL_EVENT_AFU_INTERRUPT){
            debug_2("%d : recd afu_intr %d\n", pid, p_event->irq.irq);
            fflush(stdout);
        }


        // process however many RRQ entries that are ready
        while((*p_ctx->p_hrrq_curr & SISL_RESP_HANDLE_T_BIT) ==
            p_ctx->toggle) {
			//printf("now coming inside the second loop...\n");
			//printf("0X%lX\n",*p_ctx->p_hrrq_curr);
            p_cmd = (struct afu_cmd*)((*p_ctx->p_hrrq_curr) & (~SISL_RESP_HANDLE_T_BIT));
            pthread_mutex_lock(&p_cmd->mutex);
            p_cmd->sa.host_use[0] |= B_DONE;
            pthread_cond_signal(&p_cmd->cv);
            pthread_mutex_unlock(&p_cmd->mutex);

            if (p_ctx->p_hrrq_curr < p_ctx->p_hrrq_end) {
				//printf("p_ctx->p_hrrq_curr++..\n");
                p_ctx->p_hrrq_curr++; /* advance to next RRQ entry */
            }
            else{/* wrap HRRQ  & flip toggle */
                p_ctx->p_hrrq_curr = p_ctx->p_hrrq_start;
                p_ctx->toggle ^= SISL_RESP_HANDLE_T_BIT;
				//printf("else part p_ctx->toggle = %u\n", p_ctx->toggle);
            }
        }
    }
    return NULL;
}


// len in __u64
void fill_buf(__u64* p_buf, unsigned int len,__u64 value)
{
  static __u64 data = DATA_SEED;
  int i;

  for (i = 0; i < len; i += 2) {
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
    memset(&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

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
    do {
        send_single_cmd(p_ctx);
        rc = wait_single_resp(p_ctx);
		if(rc) return rc;
    } while(check_status(&p_ctx->cmd[0].sa));


    if (p_ctx->cmd[0].sa.host_use_b[0] & B_ERROR) {
        return -1;
    }

    // report luns success
    len = read_32((__u32*)&p_ctx->rbuf[0][0]);
    p_u64 = (__u64*)&p_ctx->rbuf[0][8]; /* start of lun list */

	*nluns = len/8;
    lun_id = (__u64 *)malloc((*nluns * sizeof(__u64)));

    if (lun_id == NULL) {
        fprintf(stderr, "Report LUNs: ENOMEM\n"); 
    } else {
        *lun_ids = lun_id;

        while (len) {
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
    memset(&p_ctx->cmd[0].rcb.cdb[0], 0, sizeof(p_ctx->cmd[0].rcb.cdb));

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
    if(rc){
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

int rw_cmp_buf(struct ctx *p_ctx, __u64 start_lba) {
    int i;
    //char buf[32];
    //int read_fd, write_fd;
    for (i = 0; i < NUM_CMDS; i++) {
        if (cmp_buf((__u64*)&p_ctx->rbuf[i][0], (__u64*)&p_ctx->wbuf[i][0],
                    sizeof(p_ctx->rbuf[i])/sizeof(__u64))) {
            printf("%d: miscompare at start_lba 0X%lX\n",
                   pid, start_lba);

			/*
            sprintf(buf, "read.%d", pid);
            read_fd = open(buf, O_RDWR|O_CREAT);
            sprintf(buf, "write.%d", pid);
            write_fd = open(buf, O_RDWR|O_CREAT);

            write(read_fd, &p_ctx->rbuf[i][0], sizeof(p_ctx->rbuf[i]));
            write(write_fd, &p_ctx->wbuf[i][0], sizeof(p_ctx->wbuf[i]));

            close(read_fd);
            close(write_fd);
			*/
			hexdump(&p_ctx->rbuf[i][0],0x20,"Read buf");	
			hexdump(&p_ctx->wbuf[i][0],0x20,"Write buf");	

           return -1;
        }
    }
    return 0;
}

int send_write(struct ctx *p_ctx, __u64 start_lba,
		__u64 stride,__u64 data,__u32 flags)
{
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba, plba;
    int rc;

    for(i = 0; i< NUM_CMDS; i++){
        fill_buf((__u64*)&p_ctx->wbuf[i][0],
            sizeof(p_ctx->wbuf[i])/sizeof(__u64),data);

        memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));
        vlba = start_lba + i*stride;
        p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

        if(flags & VLBA){
            p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
            p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
            p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
            write_64(p_u64, vlba); // write(16) Virtual LBA
			debug_2("%d : send write vlba..: 0X%lX\n",(int)data,vlba);
        }
        else
        {
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
			debug_2("%d : send write plba..: 0x%lX\n",(int)data,plba);
        }

        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->wbuf[i][0];

        p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->wbuf[i]);
        p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
        write_32(p_u32, LBA_BLK); 

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
		hexdump(&p_ctx->wbuf[i][0],0x20,"Writing");
    }
    //send_single_cmd(p_ctx);
    rc = send_cmd(p_ctx);
	if(rc) return rc;
    rc = wait_resp(p_ctx);
    return rc;
}

void send_single_cmd(struct ctx *p_ctx) {

    p_ctx->cmd[0].sa.host_use_b[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    /* make memory updates visible to AFU before MMIO */
    asm volatile ( "lwsync" : : );

    // write IOARRIN
    write_64(&p_ctx->p_host_map->ioarrin, (__u64)&p_ctx->cmd[0].rcb);
}

int send_cmd(struct ctx *p_ctx) {
    int cnt = NUM_CMDS;
	int wait_try=MAX_TRY_WAIT;
	int p_cmd = 0;
    int i;
    __u64 room;

    /* make memory updates visible to AFU before MMIO */
    asm volatile ( "lwsync" : : );

    while (cnt) {
		room = read_64(&p_ctx->p_host_map->cmd_room);
        //room = NUM_CMDS +1; //p_ctx->p_host_map->cmd_room;
		if(0 == room) {
			usleep(MC_BLOCK_DELAY_ROOM);
			wait_try--;
		}
		if(0 == wait_try) {
			fprintf(stderr, "%d : send cmd wait over %d cmd remain\n",
					pid, cnt);
			return -1;
		}
        for (i = 0; i < room; i++) { // add a usleep here if room=0 ?
            // write IOARRIN
            write_64(&p_ctx->p_host_map->ioarrin,
                (__u64)&p_ctx->cmd[p_cmd++].rcb);
			wait_try = MAX_TRY_WAIT; //each cmd give try max time
            if (cnt-- == 1) break;
        }
    }
	return 0;
}

int wait_resp(struct ctx *p_ctx)
{
    int i;
	int rc =0;
	int p_rc = 0;
	__u64 *p_u64;

    for (i = 0; i < NUM_CMDS; i++) {
        pthread_mutex_lock(&p_ctx->cmd[i].mutex);
        while (p_ctx->cmd[i].sa.host_use[0] != B_DONE) {
            pthread_cond_wait(&p_ctx->cmd[i].cv, &p_ctx->cmd[i].mutex);
        }
        pthread_mutex_unlock(&p_ctx->cmd[i].mutex);

        if (p_ctx->cmd[i].sa.ioasc) {
			rc_flags = p_ctx->cmd[i].sa.rc.flags;
			rc = p_ctx->cmd[i].sa.rc.afu_rc |
				p_ctx->cmd[i].sa.rc.scsi_rc |
				p_ctx->cmd[i].sa.rc.fc_rc;
			if(!dont_displa_err_msg) {
			hexdump(&p_ctx->cmd[i].sa.sense_data,0x20,"Sense data Writing");
			p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
			if(p_rc != rc) {
				debug("%d : Request was failed 0X%lX vlba\n", pid, read_64(p_u64));
				printf("%d : IOASC = flags 0x%x, afu_rc 0x%x, scsi_rc 0x%x, fc_rc 0x%x\n",
                pid,
                p_ctx->cmd[i].sa.rc.flags,p_ctx->cmd[i].sa.rc.afu_rc,
                p_ctx->cmd[i].sa.rc.scsi_rc, p_ctx->cmd[i].sa.rc.fc_rc);
				p_rc = rc;
				}
			}
        }
    }
    return rc;
}

int wait_single_resp(struct ctx *p_ctx)
{
    int rc =0;

	pthread_mutex_lock(&p_ctx->cmd[0].mutex);
	while (p_ctx->cmd[0].sa.host_use[0] != B_DONE) {
		pthread_cond_wait(&p_ctx->cmd[0].cv, &p_ctx->cmd[0].mutex);
	}
	pthread_mutex_unlock(&p_ctx->cmd[0].mutex);

	if (p_ctx->cmd[0].sa.ioasc) {
		if(!dont_displa_err_msg) {
		//hexdump(&p_ctx->cmd[0].sa.sense_data,0x20,"Sense data Writing");
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
		__u64 stride, __u32 flags)
{
    int i;
    __u64 *p_u64;
    __u32 *p_u32;
    __u64 vlba;
    __u64 plba;
    int rc;

    for (i = 0; i < NUM_CMDS; i++) {
        memset(&p_ctx->rbuf[i][0], 0, sizeof(p_ctx->rbuf[i]));

        vlba = start_lba + i*stride;

        memset(&p_ctx->cmd[i].rcb.cdb[0], 0, sizeof(p_ctx->cmd[i].rcb.cdb));

        p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
        p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];

		if (flags & VLBA){
        	p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
        	p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
        	p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl;
        	write_64(p_u64, vlba); // Read(16) Virtual LBA
			debug_2("%d : send read for vlba : 0X%lX\n",pid,vlba);
		}
		else {
        	p_ctx->cmd[i].rcb.lun_id = lun_id;
        	p_ctx->cmd[i].rcb.port_sel = fc_port; // either FC port
        	p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_HOST_READ;
			if(flags & NO_XLATE){
				plba = vlba;
			}
			else {
				(void)mc_xlate_lba(p_ctx->mc_hndl, p_ctx->res_hndl, vlba, &plba);
			}
        	write_64(p_u64, plba); // physical LBA#
			debug_2("%d :send read for plba =0x%lX\n",pid,plba);
    	}

        p_ctx->cmd[i].rcb.data_len = sizeof(p_ctx->rbuf[i]);
        p_ctx->cmd[i].rcb.data_ea = (__u64) &p_ctx->rbuf[i][0];

        p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];

        write_32(p_u32, LBA_BLK);

        p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
        p_ctx->cmd[i].sa.ioasc = 0;
    }

    //send_single_cmd(p_ctx);
    rc = send_cmd(p_ctx);
    rc = wait_resp(p_ctx);
    return rc;
}

// returns 1 if the cmd should be retried, 0 otherwise
// sets B_ERROR flag based on IOASA
int check_status(sisl_ioasa_t *p_ioasa)
{
    // delete urun !!!
    if (p_ioasa->ioasc == 0 ||
        (p_ioasa->rc.flags & SISL_RC_FLAGS_UNDERRUN)) {
        return 0;
    }
    else {
        p_ioasa->host_use_b[0] |= B_ERROR;
    }

    if (p_ioasa->host_use_b[1]++ < 5) {
        if (p_ioasa->rc.afu_rc == 0x30) {
            // out of data buf
            // #define all, to add the 2nd case!!!
            // do we delay ?
            return 1;
        }

        if (p_ioasa->rc.scsi_rc) {
            // retry all SCSI errors
            // but if busy, add a delay
            return 1;
        }
    }

    return 0;
}

int test_init(struct ctx *p_ctx)
{
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
	if(!ENABLE_HEXDUMP)
		return;

        i=j=k=0;
        fprintf(stderr, "%s: length=%ld\n", hdr?hdr:"hexdump()", len);

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
        if ((j = i%16)) {
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

int rw_cmp_buf_cloned(struct ctx *p_ctx, __u64 start_lba) {
	int i;
	for (i = 0; i < NUM_CMDS; i++) {
		if (cmp_buf_cloned((__u64*)&p_ctx->rbuf[i][0],
			sizeof(p_ctx->rbuf[i])/sizeof(__u64))) {
			printf("%d: clone miscompare at start_lba 0X%lX\n",
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

  for (i = 0; i < len; i += 2) {
    if (!(p_buf[i] == pid && p_buf[i + 1] == data++)) {
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
	if(0 == where)//begining
		ea = align;
	else //from end of the block 
		ea = 0x1000 - align;
	for(i = 0; i< NUM_CMDS; i++){
		debug("%d : EA = %p with 0X%X alignment\n",pid, &p_rwb->wbuf[i][ea], ea);
		fill_buf((__u64*)&p_rwb->wbuf[i][ea],
			sizeof(p_rwb->wbuf[i])/(2*sizeof(__u64)),pid);
		memset(&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));
		vlba = start_lba + i*stride;
		p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
		p_ctx->cmd[i].rcb.res_hndl = p_ctx->res_hndl;
		p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
		p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
		write_64(p_u64, vlba);
		debug_2("%d : send write for 0X%lX\n", pid, vlba);
		
		p_ctx->cmd[i].rcb.data_ea = (__u64) &p_rwb->wbuf[i][ea];

		p_ctx->cmd[i].rcb.data_len = sizeof(p_rwb->wbuf[i])/2;
		p_ctx->cmd[i].rcb.cdb[0] = 0x8A; // write(16)

		p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];
		write_32(p_u32, LBA_BLK);

		p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
		p_ctx->cmd[i].sa.ioasc = 0;
		hexdump(&p_rwb->wbuf[i][ea],0x20,"Write buf");
	}
	
	rc = send_cmd(p_ctx);
	if(rc) return rc;
	//send_single_cmd(p_ctx);
	rc = wait_resp(p_ctx);
	if(rc) return rc;
	//fill send read
	
	for(i = 0; i< NUM_CMDS; i++){
		memset(&p_rwb->rbuf[i][ea], 0, sizeof(p_rwb->rbuf[i])/2);
		
		vlba = start_lba + i*stride;
		memset(&p_ctx->cmd[i].rcb.cdb[0],0,sizeof(p_ctx->cmd[i].rcb.cdb));
		
		p_ctx->cmd[i].rcb.cdb[0] = 0x88; // read(16)
		p_u64 = (__u64*)&p_ctx->cmd[i].rcb.cdb[2];
		
		p_ctx->cmd[i].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
		p_ctx->cmd[i].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
		p_ctx->cmd[i].rcb.res_hndl   = p_ctx->res_hndl;
		write_64(p_u64, vlba);
		debug_2("%d : send read for 0X%lX\n", pid, vlba);
		
		p_ctx->cmd[i].rcb.data_len = sizeof(p_rwb->rbuf[i])/2;
		p_ctx->cmd[i].rcb.data_ea = (__u64) &p_rwb->rbuf[i][ea];

		p_u32 = (__u32*)&p_ctx->cmd[i].rcb.cdb[10];

		write_32(p_u32, LBA_BLK);

		p_ctx->cmd[i].sa.host_use[0] = 0; // 0 means active
		p_ctx->cmd[i].sa.ioasc = 0;
	}
	rc = send_cmd(p_ctx);
	if(rc) return rc;
	//send_single_cmd(p_ctx);
	rc = wait_resp(p_ctx);
	if(rc) return rc;
	//do cmp r/w buf
	for (i = 0; i < NUM_CMDS; i++) {
		vlba = start_lba + i*stride;
		if (cmp_buf((__u64*)&p_rwb->rbuf[i][ea], (__u64*)&p_rwb->wbuf[i][ea],
                    sizeof(p_ctx->rbuf[i])/(2 * sizeof(__u64)))) {
            printf("%d: miscompare at start_lba 0X%lX\n",
                   pid, vlba);
			hexdump(&p_rwb->rbuf[i][ea],0x20,"Read buf");
            hexdump(&p_rwb->wbuf[i][ea],0x20,"Write buf");
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
	memset(&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

	p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];
	p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
	p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
	p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
	write_64(p_u64, vlba);
	debug_2("%d : send write for 0X%lX\n", pid, vlba);

	p_ctx->cmd[0].rcb.data_ea = (__u64) &p_rwb->wbuf[0][0];

	p_ctx->cmd[0].rcb.data_len = sizeof(p_rwb->wbuf[0]);
	p_ctx->cmd[0].rcb.cdb[0] = 0x8A; // write(16)

	p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
	write_32(p_u32, LBA_BLK);

	p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    if(rc) return rc;

	memset(&p_rwb->rbuf[0][0], 0, sizeof(p_rwb->rbuf[0]));

	memset(&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

	p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
	p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

	p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
	p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
	p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
	write_64(p_u64, vlba);
	debug_2("%d : send read for 0X%lX\n", pid, vlba);

	p_ctx->cmd[0].rcb.data_len = sizeof(p_rwb->rbuf[0]);
	p_ctx->cmd[0].rcb.data_ea = (__u64) &p_rwb->rbuf[0][0];

	p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

	write_32(p_u32, LBA_BLK);

	p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
	p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    if(rc) return rc;
	//do cmp r/w buf

	if (cmp_buf((__u64*)&p_rwb->rbuf[0][0], (__u64*)&p_rwb->wbuf[0][0],
				sizeof(p_rwb->rbuf[0])/(sizeof(__u64)))) {
		printf("%d: miscompare at start_lba 0X%lX\n",
			   pid, vlba);
		hexdump(&p_rwb->rbuf[0][0],0x20,"Read buf");
		hexdump(&p_rwb->wbuf[0][0],0x20,"Write buf");
		return -1;
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
    memset(&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];
    p_ctx->cmd[0].rcb.res_hndl = p_ctx->res_hndl;
    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_WRITE;
    write_64(p_u64, vlba);
    debug_2("%d : send write for 0X%lX\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->wbuf[0][0];

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->wbuf[0]);
    p_ctx->cmd[0].rcb.cdb[0] = 0x8A; // write(16)

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];
    write_32(p_u32, LBA_BLK);

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

    memset(&p_ctx->cmd[0].rcb.cdb[0],0,sizeof(p_ctx->cmd[0].rcb.cdb));

    p_ctx->cmd[0].rcb.cdb[0] = 0x88; // read(16)
    p_u64 = (__u64*)&p_ctx->cmd[0].rcb.cdb[2];

    p_ctx->cmd[0].rcb.req_flags = SISL_REQ_FLAGS_RES_HNDL;
    p_ctx->cmd[0].rcb.req_flags |= SISL_REQ_FLAGS_HOST_READ;
    p_ctx->cmd[0].rcb.res_hndl   = p_ctx->res_hndl;
    write_64(p_u64, vlba);
    debug_2("%d : send read for 0X%lX\n", pid, vlba);

    p_ctx->cmd[0].rcb.data_len = sizeof(p_ctx->rbuf[0]);
    p_ctx->cmd[0].rcb.data_ea = (__u64) &p_ctx->rbuf[0][0];

    p_u32 = (__u32*)&p_ctx->cmd[0].rcb.cdb[10];

    write_32(p_u32, LBA_BLK);

    p_ctx->cmd[0].sa.host_use[0] = 0; // 0 means active
    p_ctx->cmd[0].sa.ioasc = 0;

    send_single_cmd(p_ctx);
    rc = wait_single_resp(p_ctx);
    return rc;
}

int rw_cmp_single_buf(struct ctx *p_ctx, __u64 vlba)
{
		if (cmp_buf((__u64*)&p_ctx->rbuf[0][0], (__u64*)&p_ctx->wbuf[0][0],
                sizeof(p_ctx->rbuf[0])/(sizeof(__u64)))) {
        printf("%d: miscompare at start_lba 0X%lX\n",
               pid, vlba);
        hexdump(&p_ctx->rbuf[0][0],0x20,"Read buf");
        hexdump(&p_ctx->wbuf[0][0],0x20,"Write buf");
        return -1;
    }
    return 0;
}
