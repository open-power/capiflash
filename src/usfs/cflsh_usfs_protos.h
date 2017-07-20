/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_usfs_protos.h 1.6  */
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
/* %Z%%M%       %I%  %W% %G% %U% */


/*
 * COMPONENT_NAME: (sysxcflashufs) CAPI Flash user space file system library
 *
 * FUNCTIONS: API for users of this library.
 *
 * ORIGINS: 27
 *
 *                  -- (                            when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 2015
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#ifndef _H_CFLASH_USFS_PROTOS
#define _H_CFLASH_USFS_PROTOS


/* cflsh_usfs.c internal only protos */


ssize_t _cusfs_rdwr(int fd, struct aiocb64 *aiocbp, 
		    void *buffer,size_t nbytes,int flags);

/* cflsh_usfs_utils.c protos */

#if !defined(__64BIT__) && defined(_AIX)
time64_t cflsh_usfs_time64(time64_t *tp);

#endif /* 32-bit AIX */

void cusfs_setup_trace_files(int new_process);
int cusfs_valid_endianess(void);
void cusfs_get_host_type(void);
void  cusfs_chunk_sigsev_handler (int signum, siginfo_t *siginfo, void *uctx);
void cusfs_prepare_fork (void);
void  cusfs_parent_post_fork (void);
void  cusfs_child_post_fork (void);
void cusfs_trace_log_data_ext(trace_log_ext_arg_t *ext_arg, 
			     FILE *logfp,char *filename, char *function, 
			     uint line_num,char *msg, ...);
void cusfs_trace_log_data_syslog(char *fn, char *fcn, uint ln, char *msg, ...);

void  cusfs_display_stats(cflsh_usfs_t *cufs, int verbosity);
int  cusfs_setup_dump_file(void);
void  cusfs_dump_debug_data(const char *reason,const char *reason_filename,
			   const char *reason_function,
			  int reason_line_num, const char *reason_date);

void cusfs_signal_dump_handler(int signum, siginfo_t *siginfo, void *uctx);
int cusfs_setup_sigsev_dump(void);
int cusfs_setup_sigusr1_dump(void);
cflsh_usfs_os_type_t cusfs_get_os_type(void);
int cusfs_get_fs_block_size(cflsh_usfs_t *cufs, int flags);
int cusfs_get_inode_stats(cflsh_usfs_t *cufs,uint64_t *total_inodes, uint64_t *available_inodes,
			 int flags);
int cusfs_get_inode_table_size(cflsh_usfs_t *cufs, int flags);
cflsh_usfs_t *cusfs_get_cufs(char *device_name, int flags);
void cusfs_release_cufs(cflsh_usfs_t *cufs, int flags);
cflsh_usfs_data_obj_t *cusfs_alloc_file(cflsh_usfs_t *cufs, char *filename, 
					int flags);
void cusfs_free_file(cflsh_usfs_data_obj_t *file);
int cusfs_get_aio(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
		  cflsh_usfs_aio_entry_t **aio,int flags);
int cusfs_free_aio(cflsh_usfs_t *cufs, cflsh_usfs_aio_entry_t *aio,int flags);
int cusfs_issue_aio(cflsh_usfs_t *cufs,  cflsh_usfs_data_obj_t *file, uint64_t start_lba, struct aiocb64 *aiocbp, 
		    void *buffer, size_t nbytes, int flags);
int cusfs_check_active_aio_complete(cflsh_usfs_t *cufs,  int flags);
void cusfs_cleanup_all_completed_aiocbs_for_file(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, int flags);
int cusfs_wait_for_aio_for_file(cflsh_usfs_t *cufs,
				cflsh_usfs_data_obj_t *file,  int flags);
int cusfs_get_status_all_aio_for_aiocb(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
				       struct aiocb64 *aiocbp, int flags);
int cusfs_return_status_all_aio_for_aiocb(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, struct aiocb64 *aiocbp, 
					  int flags);

int cusfs_start_aio_thread_for_cufs(cflsh_usfs_t *cufs,  int flags);
void cusfs_stop_aio_thread_for_cufs(cflsh_usfs_t *cufs,  int flags);
void *cusfs_aio_complete_thread(void *data);

cflsh_usfs_t *cusfs_access_fs(char *device_name, int flags);
void cusfs_add_file_to_hash(cflsh_usfs_data_obj_t *file, int flags);
void cusfs_remove_file_from_hash(cflsh_usfs_data_obj_t *file, int flags);
cflsh_usfs_data_obj_t *cusfs_find_data_obj_from_inode(cflsh_usfs_t *cufs, uint64_t inode_num, int flags);

cflsh_usfs_data_obj_t *cusfs_find_data_obj_from_filename(cflsh_usfs_t *cufs, char *filename, int flags);

int cusfs_rdwr_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags);

int cusfs_read_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags);

int cusfs_read_data_object_for_inode_no(cflsh_usfs_t *cufs, uint64_t inode_num, 
				      void **buf, int *buf_size, int flags);

int cusfs_write_data_object(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, void *buf, int *buf_size, int flags);
cflsh_usfs_data_obj_t *cusfs_lookup_data_obj(cflsh_usfs_t *cufs, 
					   char *pathname, int flags);
char *cusfs_get_only_filename_from_diskpath(char *disk_pathname, int flags);
cflsh_usfs_t *cusfs_find_fs_for_disk(char *device_name, int flags);
cflsh_usfs_t *cusfs_find_fs_from_path(char *disk_pathname, int flags);
cflsh_usfs_data_obj_t *cusfs_open_disk_filename(char *disk_pathname, int flags);
int cusfs_flush_inode_timestamps(cflsh_usfs_t *cusfs, cflsh_usfs_data_obj_t *file);

int cusfs_get_disk_lbas_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file,
				     uint64_t offset, size_t num_bytes, 
				     cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
				     int flags);
void cusfs_fork_manager(void);

void cusfs_aio_cleanup(cflsh_usfs_t *cufs);

void cusfs_seek_read_ahead(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file,
			   uint64_t start_offset, int flags);

/* cflsh_usfs_disk.c protos */
cflsh_usfs_fs_id_t cusfs_generate_fs_unique_id(void);
int cusfs_get_disk_block_no_from_fs_block_no(cflsh_usfs_t *cufs, uint64_t fs_block_no, uint64_t *disk_block_no);
uint64_t cusfs_get_free_block_table_size(cflsh_usfs_t *cufs, 
					uint64_t num_blocks,
					int flags);
int cusfs_create_superblock(cflsh_usfs_t *cufs, int flags);
int cusfs_update_superblock(cflsh_usfs_t *cufs, int flags);
int cusfs_remove_superblock(cflsh_usfs_t *cufs, int flags);
int cusfs_validate_superblock(cflsh_usfs_t *cufs, int flags);
int cusfs_query_superblock(cflsh_usfs_t *cufs, cflsh_usfs_super_block_t *superblock,int flags);
int cusfs_create_free_block_table(cflsh_usfs_t *cufs, int flags);
int cusfs_create_inode_table(cflsh_usfs_t *cufs, int flags);
int cusfs_create_journal(cflsh_usfs_t *cufs, int flags);
int cusfs_add_journal_event(cflsh_usfs_t *cufs, 
			   cflsh_journal_entry_type_t journal_type,
			   uint64_t block_no, uint64_t inode_num,
			   uint64_t old_file_size, uint64_t new_file_size,
			   uint64_t directory_inode_num,
			   int flags);
int cusfs_flush_journal_events(cflsh_usfs_t *cufs, int flags);
int cusfs_get_free_table_stats(cflsh_usfs_t *cufs,uint64_t *total_blocks, uint64_t *available_blocks,
			 int flags);
uint64_t cusfs_get_data_blocks(cflsh_usfs_t *cufs, int *num_blocks,int flags);
int cusfs_get_data_blocks_list(cflsh_usfs_t *cufs, uint64_t *num_blocks, uint64_t **block_list, int flags);
int cusfs_release_data_blocks(cflsh_usfs_t *cufs, uint64_t lba,int num_blocks,int flags);
int cusfs_add_directory_entry(cflsh_usfs_t *cufs,uint64_t directory_inode_num,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags);
cflsh_usfs_inode_t *cusfs_find_inode_num_from_directory_by_name(cflsh_usfs_t *cufs,
							      char *name,char *buf, 
							      int buf_size,int flags);
int cusfs_remove_directory_entry(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags);
int cusfs_check_directory_empty(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags);
int cusfs_list_files_directory(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode, 
			     char *filename,int flags);
int cusfs_get_next_directory_entry(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *directory, 
			    cflsh_usfs_directory_entry_t *return_dir_entry, cflash_offset_t *offset,
			    int flags);
int cusfs_initialize_directory(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode,int flags);
int cusfs_clear_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_inode_t *inode,int flags);

cflsh_usfs_data_obj_t *cusfs_create_data_obj(cflsh_usfs_t *cufs, 
					   char *filename, 
					   cflsh_usfs_inode_file_type_t inode_type,   
					   uint64_t parent_directory_inode_num,
					   mode_t mode_flags, uid_t uid, gid_t gid,
					   char *sym_link_name,
					   int flags);

int cusfs_free_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *file,int flags);
int cusfs_mv_data_obj(cflsh_usfs_t *cufs,cflsh_usfs_data_obj_t *file,char *new_full_pathname,int flags);

cflsh_usfs_inode_t *cusfs_find_root_inode(cflsh_usfs_t *cufs, int flags);


/* cflsh_usfs_inode.c */

cflsh_usfs_inode_t *cusfs_get_inode(cflsh_usfs_t *cufs, uint64_t parent_directory_inode_num,
				  cflsh_usfs_inode_file_type_t inode_type,
				  mode_t mode_flags, uid_t uid, gid_t gid, 
				  uint64_t blocks[], int num_blocks, uint64_t file_size,
				  char *sym_link_name,
				  int flags);

int cusfs_release_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags);
int cusfs_release_inode_and_data_blocks(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags);
cflsh_usfs_inode_t *cusfs_get_inode_from_index(cflsh_usfs_t *cufs, uint64_t inode_num,int flags);
int cusfs_update_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,int flags);

uint64_t cusfs_get_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				     uint64_t blocks[], int *num_blocks, 
				     int flags);
int cusfs_update_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				   uint64_t inode_ptr_block_no,
					uint64_t blocks[], int *num_blocks, 
				   int flags);
int cusfs_add_blocks_to_indirect_inode_ptr(cflsh_usfs_t *cufs, 
					  uint64_t inode_ptr_block_no,
					uint64_t blocks[], int *num_blocks, 
					  int flags);
int cusfs_remove_indirect_inode_ptr(cflsh_usfs_t *cufs, 
				   uint64_t inode_ptr_block_no, 
				   int flags);

int cusfs_get_inode_block_no_from_inode_index(cflsh_usfs_t *cufs, uint64_t inode_index,uint64_t *lba);
int cusfs_get_inode_ptr_from_file_offset(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
					uint64_t offset, int *indirect_inode_ptr, int flags);
int cusfs_rdwr_block_inode_pointers(cflsh_usfs_t *cufs, 
				   cflsh_usfs_inode_t *inode,
				   void *buffer,
				   uint64_t fs_lba, int nblocks, int flags);

int cusfs_get_disk_lbas_from_inode_ptr_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode, 
						   uint64_t offset, size_t num_bytes, 
						   cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
						   int flags);
int cusfs_get_all_disk_lbas_from_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   cflsh_usfs_disk_lba_e_t **disk_lbas, uint64_t *num_disk_lbas,
					   int flags);
int cusfs_get_inode_ptr_from_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
					uint64_t offset, size_t num_bytes, 
					int *start_inode_ptr_index,
					int *start_inode_ptr_byte_offset,
					int *end_inode_ptr_index,
					int *end_inode_ptr_byte_offset,
					int *num_inode_ptr_indices,
					 int flags);
int cusfs_find_disk_lbas_in_inode_ptr_for_transfer(cflsh_usfs_t *cufs, cflsh_usfs_data_obj_t *file, 
						  int inode_ptr_index,
						  uint64_t *start_disk_lba, 
						  uint64_t *num_disk_blocks,
						  int start_inode_ptr,
						  int end_inode_ptr,
						  int transfer_start_offset,
						  int transfer_end_offset, 
						  int flags);
int cusfs_num_data_blocks_single_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t single_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags);
int cusfs_num_data_blocks_double_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t double_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags);
int cusfs_num_data_blocks_triple_indirect_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						   uint64_t triple_indirect_inode_fs_lba, 
						   uint64_t *num_fs_blocks, int flags);
int cusfs_num_data_blocks_in_inode(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
				  uint64_t *num_fs_blocks, int *level,int flags);
uint64_t cusfs_get_next_inode_data_block(cflsh_usfs_t *cufs, uint64_t **block_list, 
					uint64_t *block_list_index,uint64_t num_needed_blocks, int flags);
int cusfs_add_fs_blocks_to_single_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t single_indirect_inode_fs_lba, 
					   uint64_t *num_fs_blocks, int flags);
int cusfs_add_fs_blocks_to_double_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t double_indirect_inode_fs_lba, 
					     uint64_t *num_fs_blocks, int flags);
int cusfs_add_fs_blocks_to_triple_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t triple_indirect_inode_fs_lba, 
					     uint64_t *num_fs_blocks, int flags);
int cusfs_add_fs_blocks_to_inode_ptrs(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
				     uint64_t num_fs_lbas,int flags);
int cusfs_remove_fs_blocks_from_single_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t single_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags);
int cusfs_remove_fs_blocks_from_double_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t double_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags);
int cusfs_remove_fs_blocks_from_triple_inode_ptr(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
						uint64_t triple_indirect_inode_fs_lba, 
						uint64_t *num_fs_blocks, int flags);
int cusfs_remove_fs_blocks_from_inode_ptrs(cflsh_usfs_t *cufs, cflsh_usfs_inode_t *inode,
					  uint64_t num_fs_lbas,int flags);
#endif /* _H_CFLASH_USFS_PROTOS */
