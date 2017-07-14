/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/cflash_block_protos.h $                             */
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

#ifndef _H_CFLASH_BLOCK_PROTO
#define _H_CFLASH_BLOCK_PROTO
/************************************************************************/
/* Function prototypes                                                  */
/************************************************************************/
#ifdef _COMMON_INTRPT_THREAD
int cblk_start_common_intrpt_thread(cflsh_chunk_t *chunk);
void *cblk_intrpt_thread(void *data);
#endif
void *cblk_async_recv_thread(void *data);
#ifdef _REMOVE
void  cblk_wait_for_read_alarm(int signum, siginfo_t *siginfo, void *uctx);
#endif /* _REMOVE */
void cblk_get_host_type(void);
void  cblk_chunk_sigsev_handler (int signum, siginfo_t *siginfo, void *uctx);
void cblk_reopen_after_fork(cflsh_chunk_t *chunk, int flags);
void cblk_prepare_fork (void);
void  cblk_parent_post_fork (void);
void  cblk_child_post_fork (void);
void  cblk_chunk_init_cache (cflsh_chunk_t *chunk, size_t chunk_size);
void  cblk_chunk_free_cache (cflsh_chunk_t *chunk);
void cblk_chunk_flush_cache (cflsh_chunk_t *chunk);
void  cblk_init_mc_interface(void);
void  cblk_cleanup_mc_interface(void);
char *cblk_find_parent_dev(char *device_name);
char *cblk_get_udid(char *device_name);
int  cblk_chunk_attach_process_map_path(cflsh_chunk_t *chunk, int path_index,int mode, 
					dev64_t adap_devno,  char *path_dev_name, int fd, int arch_type,
					int *cleanup_depth, int assign_path);
int  cblk_chunk_attach_process_map (cflsh_chunk_t *chunk, int mode, int *cleanup_depth);
int  cblk_chunk_get_mc_device_resources(cflsh_chunk_t *chunk, int *cleanup_depth);
void  cblk_chunk_free_mc_device_resources(cflsh_chunk_t *chunk);
void  cblk_chunk_unmap (cflsh_chunk_t *chunk,int force);
void  cblk_chunk_detach (cflsh_chunk_t *chunk,int force);
void cblk_setup_trace_files(int new_process);
int cblk_valid_endianess(void);

cflsh_path_t *cblk_get_path(cflsh_chunk_t *chunk, dev64_t adap_devno,char *path_name,cflsh_block_chunk_type_t type,int num_cmds, 
			    cflsh_afu_in_use_t *in_use, int share);
int cblk_update_path_type(cflsh_chunk_t *chunk, cflsh_path_t *path, cflsh_block_chunk_type_t type);
void cblk_release_path(cflsh_chunk_t *chunk, cflsh_path_t *path);
cflsh_block_chunk_type_t cblk_get_chunk_type(const char *path, int arch_type);
int  cblk_set_fcn_ptrs(cflsh_path_t *path);
cflash_cmd_err_t cblk_process_cmd(cflsh_chunk_t *chunk,int path_index, cflsh_cmd_mgm_t *cmd);
chunk_id_t cblk_get_chunk(int flags,int max_num_cmds);
int cblk_find_free_cmd(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t **cmd,int flags);
int cblk_get_buf_cmd(cflsh_chunk_t *chunk,void **buf, size_t buf_len, 
		     cflsh_cmd_mgm_t **cmd);
int cblk_get_lun_id(cflsh_chunk_t *chunk);
int cblk_get_lun_capacity(cflsh_chunk_t *chunk);
void cblk_open_cleanup_wait_thread(cflsh_chunk_t *chunk);
void cblk_chunk_open_cleanup(cflsh_chunk_t *chunk, int cleanup_depth);
int cblk_listio_arg_verify(chunk_id_t chunk_id,
			   cblk_io_t *issue_io_list[],int issue_items,
			   cblk_io_t *pending_io_list[], int pending_items, 
			   cblk_io_t *wait_io_list[],int wait_items,
			   cblk_io_t *completion_io_list[],int *completion_items, 
			   uint64_t timeout,int flags);
int cblk_chk_cmd_bad_page(cflsh_chunk_t *chunk, uint64_t bad_page_addr);
void cblk_fail_all_cmds(cflsh_chunk_t *chunk);
void cblk_halt_all_cmds(cflsh_chunk_t *chunk, int path_index, int all_paths);
void cblk_resume_all_halted_cmds(cflsh_chunk_t *chunk, int increment_retries, 
				 int path_index, int all_paths);
void cblk_reset_context_shared_afu(cflsh_afu_t *afu);
int cblk_retry_new_path(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd, int delay_needed_same_afu);
void cblk_trace_log_data_ext(trace_log_ext_arg_t *ext_arg, FILE *logfp,char *filename, char *function, 
			     uint line_num,char *msg, ...);
void  cblk_display_stats(cflsh_chunk_t *chunk, int verbosity);

int  cblk_setup_dump_file(void);

void  cblk_dump_debug_data(const char *reason, const char *reason_filename,const char *reason_function, 
			   int, const char *reason_date);
int cblk_setup_sigusr1_dump(void);
int cblk_setup_sigsev_dump(void);
int cblk_query_ue(void *buf, size_t len);
void cblk_check_cmd_data_buf_for_ue(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd);
void cblk_clear_poison_bits(void *buf, size_t len);
void cblk_clear_poison_bits_chunk(cflsh_chunk_t *chunk, int path_index, int all_paths);
void  cblk_afu_dump(cflsh_chunk_t *chunk, int flags);

#ifdef BLOCK_FILEMODE_ENABLED
void cblk_filemode_io(cflsh_chunk_t *chunk, cflsh_cmd_mgm_t *cmd);
#endif /* BLOCK_FILEMODE_ENABLED */
cflash_cmd_err_t cblk_process_sense_data(cflsh_chunk_t *chunk,cflsh_cmd_mgm_t *cmd,struct request_sense_data *sense_data);
cflsh_block_chunk_type_t   cblk_get_os_chunk_type(const char *path, int arch_type);
int cblk_read_os_specific_intrpt_event(cflsh_chunk_t *chunk, int path_index,cflsh_cmd_mgm_t **cmd,int *cmd_complete,
				       size_t *transfer_size, struct pollfd poll_list[]);
int  cblk_chunk_set_mc_size(cflsh_chunk_t *chunk, size_t nblocks);
int  cblk_mc_clone(cflsh_chunk_t *chunk, int mode,int flags);
cflash_block_check_os_status_t cblk_check_os_adap_err(cflsh_chunk_t *chunk, int path_index);
void cblk_notify_mc_err(cflsh_chunk_t *chunk,  int path_index,int error_num,
			uint64_t out_rc,cflash_block_notify_reason_t reason, 
			  cflsh_cmd_mgm_t *cmd);
int cblk_verify_mc_lun(cflsh_chunk_t *chunk,  cflash_block_notify_reason_t reason, 
			  cflsh_cmd_mgm_t *cmd, 
		       struct request_sense_data *sense_data);

/* cflash_block_sisl.c protos */
int cblk_init_sisl_fcn_ptrs(cflsh_path_t *path);



/* cflash_block_raid0.c protos */

chunk_r0_id_t cblk_r0_open(const char    *pdevs,
                           int             max_num_requests,
                           int             mode,
                           chunk_ext_arg_t ext,
                           int             flags);
int cblk_r0_close(chunk_r0_id_t id, int flags);
int cblk_r0_get_stats(chunk_r0_id_t id, chunk_stats_t *stats, int flags);
int cblk_r0_get_lun_size(chunk_r0_id_t id, size_t *nblocks);
int cblk_r0_get_size(chunk_r0_id_t id, size_t *nblocks);
int cblk_r0_set_size(chunk_r0_id_t id, size_t nblocks);
int cblk_r0_read(chunk_r0_id_t  id,
                 void           *buf,
                 cflash_offset_t lba,
                 size_t          nblocks,
                 int             flags);
int cblk_r0_write(chunk_r0_id_t  id,
                  void           *buf,
                  cflash_offset_t lba,
                  size_t          nblocks,
                  int             flags);
int cblk_r0_unmap(chunk_r0_id_t  id,
                  void           *buf,
                  cflash_offset_t lba,
                  size_t          nblocks,
                  int             flags);
int cblk_r0_aread(chunk_r0_id_t     id,
                  void              *buf,
                  cflash_offset_t    lba,
                  size_t             nblocks,
                  cflsh_cg_tag_t    *tag,
                  cblk_arw_status_t *status,
                  int                 flags);
int cblk_r0_awrite(chunk_r0_id_t     id,
                   void              *buf,
                   cflash_offset_t    lba,
                   size_t             nblocks,
                   cflsh_cg_tag_t    *tag,
                   cblk_arw_status_t *status,
                   int                flags);
int cblk_r0_aunmap(chunk_r0_id_t     id,
                   void              *buf,
                   cflash_offset_t    lba,
                   size_t             nblocks,
                   cflsh_cg_tag_t    *tag,
                   cblk_arw_status_t *status,
                   int                flags);
int cblk_r0_aresult(chunk_r0_id_t  id,
                    cflsh_cg_tag_t *tag,
                    uint64_t       *status,
                    int             flags);

/* cflash_block_eras_serv.c protos */

int cblk_start_eras_thread(int flags);
void cblk_stop_eras_thread(int flags);
int cblk_serv_xfer_data(int fd, int op, void *buf, ssize_t exp_size);

#endif /* _H_CFLASH_BLOCK_PROTO */
