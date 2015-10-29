/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/blk_tst.h $                                    */
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
#ifndef __BLK_H__
#define __BLK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _MACOSX 
#include <malloc.h>
#endif /* !_MACOS */
#include <unistd.h>
#include <capiblock.h>
#include <cflash_tools_user.h>


extern int blk_verbosity;
extern char *dev_path;
extern chunk_id_t chunks[];
extern void *blk_fvt_data_buf;
extern void *blk_fvt_comp_data_buf;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAX_OPENS 512
#define MAX_LUNS 512
#define MAX_NUM_THREADS 4096
#define BLK_FVT_BUFSIZE 4096
#define NUM_LIST_IO 500

#define FILESZ      4096*4096*64


#define FV_READ   	0                       
#define FV_WRITE  	1  
#define FV_AREAD  	2                       
#define FV_AWRITE 	3                       
#define FV_RW_COMP 	4                   
#define FV_RW_AWAR 	5                   

/* io_flags used in blk_fvt_io, to force conditions */
#define FV_ALIGN_BUFS           0x1
#define FV_ARESULT_BLOCKING     0x2
#define FV_ARESULT_NEXT_TAG     0x4
#define FV_NO_INRPT		0x8


#define DEBUG_0(A)                              \
    do                                          \
    {                                           \
        if (blk_verbosity)                      \
        {                                       \
            fprintf(stderr,A);                  \
            fflush(stderr);                     \
        }                                       \
    } while (0)

#define DEBUG_1(A,B)                            \
    do                                          \
    {                                           \
        if (blk_verbosity)                      \
        {fprintf(stderr,A,B);fflush(stderr);}   \
    } while (0)

#define DEBUG_2(A,B,C)                          \
    do                                          \
    {                                           \
        if (blk_verbosity)                      \
        {fprintf(stderr,A,B,C);fflush(stderr);} \
    } while (0)

#define DEBUG_3(A,B,C,D)                        \
    do                                          \
    {                                           \
        if (blk_verbosity)                      \
        {fprintf(stderr,A,B,C,D);fflush(stderr);} \
    } while (0)

#define DEBUG_4(A,B,C,D,E)                          \
    do                                              \
    {                                               \
        if (blk_verbosity)                          \
        {fprintf(stderr,A,B,C,D,E);fflush(stderr);} \
    } while (0)

typedef struct blk_thread_status {
     int ret;
     int errcode;
} blk_thread_status_t;

typedef struct blk_thread_data {
    chunk_id_t chunk_id[MAX_LUNS];
    blk_thread_status_t status;
    int		flags;
    size_t	size;
} blk_thread_data_t;



int blk_fvt_alloc_bufs(int size);
void blk_open_tst(int *id, int max_reqs, int *er_no, int opn_cnt, int flags, int mode);

void blk_open_tst_inv_path(const char* path,int *id, int max_reqs, int *er_no, int opn_cnt, int flags, int mode);

void blk_open_tst_close ( int id);

void blk_open_tst_cleanup ();

void blk_close_tst(int id, int *ret, int *er_no, int close_flag);

void blk_fvt_get_set_lun_size(chunk_id_t id, size_t *size, int sz_flags, int get_set_size_flag, int *ret, int *err);

void blk_fvt_io (chunk_id_t id, int cmd, uint64_t lba, size_t nblocks, int *ret, int *err, int io_flag, int open_flag);

int blk_fvt_setup(int size);
int multi_lun_setup();

void blk_fvt_cmp_buf(int size, int *ret);
void blk_get_statistics (chunk_id_t id, int flags, int *ret, int *err);
void blk_thread_tst(int *ret, int *err);
void blk_multi_luns_thread_tst(int *ret, int *err);
void blocking_io_tst (chunk_id_t id, int *ret, int *err);
void io_perf_tst (chunk_id_t id, int *ret, int *err);
int fork_and_clone(int *ret, int *err, int mode);
int fork_and_clone_mode_test(int *ret, int *err, int pmode, int cmode);
int fork_and_clone_my_test(int *ret, int *err, int pmode, int cmode);
void blk_fvt_intrp_io_tst(chunk_id_t id, int testflag, int open_flag, int *ret, int *err); 
void check_astatus(chunk_id_t id, int *tag, int arflag, int open_flag, int io_flag, cblk_arw_status_t *ardwr_status, int *rc, int *err);
void initialize_blk_tests();
void terminate_blk_tests();
void blk_list_io_test(chunk_id_t id, int cmd, int t_type, int uflags, uint64_t timeout, int *er_no, int *ret, int num_listio);
void blk_list_io_arg_test(chunk_id_t id, int arg_tst, int *err, int *ret);
int poll_arw_stat(cblk_io_t *cblk_issue_io, int num_listio);
int poll_completion(int t_type, int *cmplt, cblk_io_t *wait_io[], cblk_io_t *pending_io[], cblk_io_t *cmplt_io[]);
int check_completions(cblk_io_t *io , int num_listio);
int blk_fvt_alloc_list_io_bufs(int size);
int blk_fvt_alloc_large_xfer_io_bufs(size_t nblks);
char *find_parent(char *device_name);
int validate_share_context();
int max_context(int *ret, int *err, int reqs, int cntx, int flags, int mode);
int child_open(int c, int reqs, int flags, int mode);
#endif
