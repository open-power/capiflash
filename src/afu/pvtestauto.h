/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/pvtestauto.h $                                       */
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
#ifndef _H_PVTESTAUTO_OBJ
#define _H_PVTESTAUTO_OBJ

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/errno.h>
#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif
#include <limits.h>
#include <pthread.h>
#include <cflash_tools_user.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif



#define  SEQUENTIAL 		 0x1
#define  RANDOM 		 0x2
#define  MAX_NUM_BLOCKS 	 64
#define  NORM_READ		 1     /* normal reads */
#define  EXT_READ		 2     /* extended reads */
#define  NORM_WRITE		 1     /* normal writes */
#define  EXT_WRITE		 2     /* extended writes */
#define  BLK_SIZE                4096  /* Number of bytes in one block */
#define  DESTRUCTIVE 		 0x01  /* Data on device can be modified */
#define  NON_DESTRUCTIVE 	 0x02  /* Data can't be modified on device. */
#define  READ_ONLY   		 0x03  /* Data can only be read. */
#define  NUM_IDS 		 30    /* Number of opens allowed */
#define  MAX_NUM_THREADS 	4096
#define  PV_RW_AWAR	 	1
#define  PV_RW_COMP	 	2
#define  PV_AREAD_ONLY	 	3
#define  PV_AWRITE_ONLY	 	4





long long low_lba;	     /* lowest lba to use */
long long high_lba;	     /* highest lba to use	 */
long long lba;		     /* current lba */
int       num_blocks;	     /* number of blocks to use in a operation */
int       dev_block_size;    /* device block size in bytes */
int       num_loops;         /* number of loops to make */
int       loop_forever;      /* if 1 test will run for ever till killed. */
uint8_t   destr;	     /* Destructive is 1 , Write only 2, Read only 3 */
uint8_t	  seq_or_rand;	     /* Sequential is 1, random is 2 */
int       blk_size;          /* Devices block size */
int       verbose;           /* if 1 print I/O progress */
int       virtual_flag;      /* if set open virtual luns, otherwise open phys */
int	  thread_flag;	     /* multi-thread test */
int	  num_threads;	     /* no of threads */
uint32_t  thread_count;	     /* thread count */
uint64_t  block_number;	     /* lba no       */
int	  num_opens;	     /* open counts  */


/*
 * Basic fields need for devices
 */
int		open_cnt;	        /* # of times opened w o close*/
char 		dev_name [PATH_MAX]; /* name of device including dev */

/*
 * Buffers for I/O interface to an application
 */
char    	*wbuf;		   /* buffer for data to write */
char		*rbuf;		   /* buffer for data to read */
char		*tbuf;		   /* buffer for temporary data	 */

/*
 * Buffers for I/O interface to an application
 */
int	        show_errors;	   /* if true error are displayed */

typedef struct th_data {
    long long	t_low_lba;	     /* lowest lba to use */
    long long	t_high_lba;	     /* highest lba to use */
    int         ret;
    int         errcode;
} th_data_t;


typedef struct pv_thread_data {
    chunk_id_t chunk_id;
    int         flags;
    size_t      size;
    int		num_loops;
    long long	low_lba;	     /* lowest lba to use */
    long long	high_lba;	     /* highest lba to use	 */
    uint64_t  	block_number;	     /* lba no       */
    long long  	t_lbasz;	     /* range size to test */
    th_data_t   t_data[MAX_NUM_THREADS];
} pv_thread_data_t;


extern char *pv_verbosity;
extern char *dev_path;
extern chunk_id_t chunks[];
extern void *pv_data_buf;
extern void *pv_comp_data_buf;
extern void *pv_temp_buf;



#define DEBUG_0(A)                              \
    do                                          \
    {                                           \
        if (verbose)                      \
        {                                       \
            fprintf(stderr,A);                  \
            fflush(stderr);                     \
        }                                       \
    } while (0)

#define DEBUG_1(A,B)                            \
    do                                          \
    {                                           \
        if (verbose)                      \
        {fprintf(stderr,A,B);fflush(stderr);}   \
    } while (0)

#define DEBUG_2(A,B,C)                          \
    do                                          \
    {                                           \
        if (verbose)                      \
        {fprintf(stderr,A,B,C);fflush(stderr);} \
    } while (0)

#define DEBUG_3(A,B,C,D)                        \
    do                                          \
    {                                           \
        if (verbose)                      \
        {fprintf(stderr,A,B,C,D);fflush(stderr);} \
    } while (0)

#define DEBUG_4(A,B,C,D,E)                          \
    do                                              \
    {                                               \
        if (verbose)                          \
        {fprintf(stderr,A,B,C,D,E);fflush(stderr);} \
    } while (0)

/* -------------------
* Function prototypes
* -------------------
*/
	
void errno_process();
void errno_value();
void errno_parse(int err);
int  detect_mismatch_r_t(int);
int  open_dev(int,int);
int  close_dev();
int  dev_init();
int  dev_open();
int  run();
int  read_dev(int);
int  write_dev(int);

int rd_wrt_rd(uint64_t, uint64_t,uint64_t, int,uint8_t );
int seqtest(uint64_t, int, uint64_t, uint64_t, int, uint8_t );
int rndtest(uint64_t, int, uint64_t, uint64_t, int, uint8_t );
int set_sz(chunk_id_t chunk_id, size_t size, int flags);
int get_phys_lun_sz(chunk_id_t chunk_id, size_t *size, int flags);
int get_sz(chunk_id_t chunk_id, size_t *size, int flags);
int run_pv_test(int *ret, int *err);



#endif
