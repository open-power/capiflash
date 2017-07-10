/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/cflsh_usfs_disk.h 1.6  */
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
 * COMPONENT_NAME: (sysxcflashusfs) CAPI Flash user space file system library
 *
 * FUNCTIONS: Data structures of the filesystem that reside on 
 *            a CAPI flash disk.
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

#ifndef _H_CFLASH_USFS_DISK
#define _H_CFLASH_USFS_DISK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <memory.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <syslog.h>
#include <utime.h>
#ifdef _AIX
#include <sys/aio.h>
#else
#include <aio.h>
#endif
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#endif /* !_AIX && !_MACOSX */
#include <trace_log.h>
#include <cflash_eras.h>
#ifdef _OS_INTERNAL
#include <sys/capiblock.h>
#else
#include <capiblock.h>
#endif
#include <cflash_tools_user.h>
#include "cflsh_usfs_client.h"

#ifndef TIMELEN
#define   TIMELEN  26           /* Linux does have a define for the minium size of the a timebuf */
                                /* However linux man pages say it is 26                          */
#endif 


/************************************************************************/
/* CAPI block library settings                                          */
/************************************************************************/
#define CFLSH_USFS_DISK_MAX_CMDS  0x2000 /* Maxium number of requests    */
					/* this process will queue to a */
					/* disk for this filesystem.    */

/************************************************************************/
/* Filesystem metadata offsets in 4K sectors                            */
/*                                                                      */
/*                                                                      */
/*           -----------------------                                    */
/*           sector 0                                                   */
/*                                                                      */
/*           UNUSED                                                     */
/*           -----------------------                                    */
/*           sector 1                                                   */
/*                                                                      */
/*           SUPER BLOCK                                                */
/*           -----------------------                                    */
/*           sector 2                                                   */
/*                                                                      */
/*           Free Block Table stats                                     */
/*           -----------------------                                    */
/*           sector 3                                                   */
/*                                                                      */
/*           Inode table stats                                          */
/*           -----------------------                                    */
/*           sector 4-0xf                                               */
/*                                                                      */
/*           UNUSED                                                     */
/*           -----------------------                                    */
/*           sector 0x10 - 0x30                                         */
/*                                                                      */
/*           JOURNAL (128K)                                             */
/*           -----------------------                                    */
/*           sector 0x40 - m                                            */
/*                                                                      */
/*           FREE BLOCK TABLE                                           */
/*           -----------------------                                    */
/*           sector m - n                                               */
/*                                                                      */
/*           INODE TABLE                                                */
/*           -----------------------                                    */
/*           sector n - last lba                                        */
/*                                                                      */
/*           DATA BLOCKs                                                */
/*           -----------------------                                    */
/*                                                                      */
/************************************************************************/


#define CFLSH_USFS_PRIMARY_SB_OFFSET   1   /* Disk sector for primary   */
				          /* super block.              */
#define CFLSH_USFS_BACKUP_SB_OFFSET   100000

#define CFLSH_USFS_FBLK_TBL_STATS_OFFSET  2 /* Disk sector for free     */
                                           /* block table statistics   */
#define CFLSH_USFS_INODE_TBL_STATS_OFFSET 3 /* Disk sector for Inode    */
                                           /* table statistics         */



#define CFLSH_USFS_JOURNAL_START_TABLE_LBA 0x10

#define CFLSH_USFS_JOURNAL_TABLE_SIZE      0x20         /* 128 K Log */

#define CFLSH_USFS_FREE_BLOCK_TABLE_LBA    0x40 

/************************************************************************/
/* Super block data structure for this filesystem.                      */
/************************************************************************/

#define CFLSH_USFS_FORCE_SB_CREATE  0x0001 /* Force the creation of     */
					  /* of superblock even if one */
					  /* already exist.            */


typedef enum {
    CFLSH_OS_TYPE_INVAL        = 0,   /* Invalid OS type                   */
    CFLSH_OS_TYPE_AIX          = 1,   /* AIX OS                            */
    CFLSH_OS_TYPE_LINUX        = 2,   /* Linux OS                          */
    CFLSH_OS_TYPE_MACOSX       = 3,   /* Mac OSX                           */
} cflsh_usfs_os_type_t;


#define CFLSH_USFS_BLOCK_SIZE_MIN   0x1000  /* Minimum data block size       */
					   /* allowed                       */

#define CFLSH_USFS_DEVICE_BLOCK_SIZE 0x1000 /* physical disk block size      */

typedef struct cflsh_usfs_fs_id_s {
    uint64_t val1;
    uint64_t val2;
    uint64_t val3;
    uint64_t val4;
} cflsh_usfs_fs_id_t;


typedef struct cflsh_usfs_super_block_s {
    uint64_t   start_marker;
#define CLFSH_USFS_SB_SM __EYEC8('c','u','f','s','_','s','b','a')
    uint32_t   version;
    uint32_t   status;
    uint64_t   flags;
#define CFLSH_USFS_SB_MOUNTED  0x001 /* Filesystem is mounted           */
#define CFLSH_USFS_SB_FT_VAL   0x002 /* Free block table is written     */
#define CFLSH_USFS_SB_IT_VAL   0x004 /* Inode table is written          */
#define CFLSH_USFS_SB_JL_VAL   0x008 /* Journal  is written             */
#define CFLSH_USFS_SB_ROOT_DIR 0x010 /* Root directory inode written    */

/* ?? how to handle multiple mounts */
/* ?? Need some indication of endianness of FS creator */
/* ?? need filesystem identifier                      */

    uint64_t   fs_block_size;       /* Block size referenced by inodes */
				    /* This is the smallest            */
				    /* addressable data size in the    */
				    /* filesystem                      */
    uint64_t   num_blocks;          /* Size of file system in blocks   */
    

    uint32_t   disk_block_size;       /* Block size used by disk       */

    uint64_t   inode_table_stats_lba; /* start LBA of inode table */
				     /* statistics.                    */
    uint64_t   inode_table_start_lba; /* Start LBA of Inode table      */
    uint64_t   inode_table_size;      /* Size in disk blocks of        */
				      /* node table                    */

    uint64_t   num_inodes;          /* Number of inodes for this file-  */
				    /* system.                          */
    uint64_t   num_inodes_per_disk_block;/* Number of inodes for this   */
				    /* filesystem.                     */
    uint64_t   free_block_table_stats_lba;/* start LBA of free block table */
				     /* statistics.                    */

    uint64_t   free_block_table_start_lba; /* start LBA of free block  */
				     /* table.                         */
    uint64_t   free_block_table_size;/* Size in disk blocks of         */
				     /* Journal                        */ 

    uint64_t   journal_start_lba;    /* start LBA of Journal           */
    uint64_t   journal_table_size;   /* Size in disk blocks of         */
				     /* Journal                        */
    uint64_t   num_journal_entries;  /* Number of entries in the journal*/
    uint64_t   start_data_blocks;    /* Disk sector where data blocks  */
				     /* start                          */
    uint64_t   fs_start_data_block;  /* Filesystem block where data    */
				     /* starts.                        */
    cflsh_usfs_os_type_t os_type;    /* OS type that created this      */
				     /* filesystem.                    */

#if !defined(__64BIT__) && defined(_AIX)
    time64_t create_time;            /* Creation time of filesystem     */
    time64_t mount_time;             /* Last time filesystem was mounted*/
    time64_t write_time;             /* Last time filesystem was written*/
    time64_t fsck_time;              /* Last time filesystem had fsck   */
                                     /* run.                           */
#else
    time_t create_time;              /* Creation time of filesystem     */
    time_t mount_time;               /* Last time filesystem was mounted*/
    time_t write_time;               /* Last time filesystem was written*/
    time_t fsck_time;                /* Last time filesystem had fsck   */
                                     /* run.                           */
#endif


    /* ??

      permissions, 
      access info

      type,
      locking files per process or not.

      inode number of root directory

    */
    cflsh_usfs_fs_id_t fs_unique_id; /* File system unique id          */

    uint64_t   end_marker;

#define CLFSH_USFS_SB_EM __EYEC8('c','u','f','s','_','s','b','e')
} cflsh_usfs_super_block_t;


/************************************************************************/
/* Free block table data structures for this filesystem.                */
/************************************************************************/

typedef struct cflsh_usfs_free_block_stats_s {
    uint64_t   start_marker;
#define CLFSH_USFS_FB_SM __EYEC8('c','u','f','s','_','f','b','a')
    uint32_t   version;
    uint32_t   status;
    uint64_t   flags;
    uint64_t   total_free_blocks;       /* Total number of free blocks */
					/* in free block table.        */
    uint64_t   available_free_blocks;   /* Available free blocks       */
					/* in free block table.        */


    uint64_t   end_marker;

#define CLFSH_USFS_FB_EM __EYEC8('c','u','f','s','_','f','b','e')
} cflsh_usfs_free_block_stats_t;


/************************************************************************/
/* Inode data structures for this filesystem.                           */
/************************************************************************/



typedef struct cflsh_usfs_inode_table_stats_s {
    uint64_t   start_marker;
#define CLFSH_USFS_IT_SM __EYEC8('c','u','f','s','_','i','t','a')
    uint32_t   version;
    uint32_t   status;
    uint64_t   flags;
    uint64_t   total_inodes;            /* Total number of inodes in   */
					/* inode table.                */
    uint64_t   available_inodes;        /* Available inodes in inode   */
					/* table.                      */


    uint64_t   end_marker;

#define CLFSH_USFS_IT_EM __EYEC8('c','u','f','s','_','i','t','e')
} cflsh_usfs_inode_table_stats_t;




#ifdef _REMOVE
typedef struct cflsh_usfs_inode_ptr_s {
    uint64_t   flags;
    uint64_t   start_lba;
    uint64_t   end_lba;
} cflsh_usfs_inode_ptr_t;
#endif /* _REMOVE */

typedef struct cflsh_usfs_inode_ptr_s {
    uint64_t   lba;
} cflsh_usfs_inode_ptr_t;


enum {
    CFLSH_USFS_INODE_HDR_INVAL  = 0,   /* Invalid Inode header type         */
    CFLSH_USFS_INODE_HDR_SINGLE = 1,   /* Single indirect inode header      */
    CFLSH_USFS_INODE_HDR_DOUBLE = 2,   /* Double indirect inode header      */
    CFLSH_USFS_INODE_HDR_TRIPLE = 3,   /* Triple indirect inode header      */

} cflash_usfs_inode_hdr_type_t;

#ifdef _REMOVE

typedef struct cflsh_usfs_inode_indirect_hdr_s {
    uint64_t   start_marker;
#define CLFSH_USFS_INODE_IND_SM __EYEC8('c','u','f','s','i','t','i','h')
    uint64_t   flags;
    uint32_t   version;
    uint32_t   status;
    cflash_usfs_inode_hdr_type_t type;
    uint32_t   num_inode_ptrs;
    cflsh_usfs_inode_ptr_t inode_ptr[1];
} cflsh_usfs_inode_indirect_hdr_t;
#endif /* _REMOVE */

#define CFLSH_USFS_INODE_FLG_INDIRECT_ENTRIES 0x1  /* data blocks are    */
						  /* single indirect enteries */
#define CFLSH_USFS_INODE_FLG_D_INDIRECT_ENTRIES 0x2/* data blocks are     */
						  /* double indirect enteries */

#define CFLSH_USFS_INODE_FLG_NEW_INDIRECT_BLK 0x1


#define CFLSH_USFS_NUM_DIRECT_INODE_PTRS 12
#define CFLSH_USFS_INDEX_SINGLE_INDIRECT 12
#define CFLSH_USFS_INDEX_DOUBLE_INDIRECT 13
#define CFLSH_USFS_INDEX_TRIPLE_INDIRECT 14



#define CFLSH_USFS_NUM_TOTAL_INODE_PTRS 15

typedef 
enum {
    CFLSH_USFS_INODE_FILE_INVAL     = 0,   /* Invalid Inode header type      */
    CFLSH_USFS_INODE_FILE_DIRECTORY = 1,   /* Directory                      */
    CFLSH_USFS_INODE_FILE_REGULAR   = 2,   /* Regular file                   */
    CFLSH_USFS_INODE_FILE_SYMLINK   = 3,   /* Symbolic link file             */
    CFLSH_USFS_INODE_FILE_SPECIAL   = 4,   /* Special file                   */
} cflsh_usfs_inode_file_type_t;

#define CFLSH_USFS_SYM_LINK_MAX 256

typedef struct cflsh_usfs_inode_s {
    uint64_t   start_marker;
#define CLFSH_USFS_INODE_SM __EYEC8('c','u','f','s','_','i','t','a')
    uint32_t   version;
    uint32_t   status;
    uint64_t   flags;
#define CFLSH_USFS_INODE_INUSE    0x0001 /* Inode entry in use. */
#define CFLSH_USFS_INODE_ROOT_DIR 0x0002 /* This is the root    */
					/* directory of the    */
					/* filesystem          */

    uint64_t   file_size;             /* File size in bytes */
    uint64_t   index;

    mode_t     mode_flags; /* 0777 bits used as r,w,x permissions */

    uid_t      uid;                   /* local uid */ 
    gid_t      gid;                   /* local gid */

    uint32_t   reserved1;
    uint32_t   reserved2;

    cflsh_usfs_inode_file_type_t type;


    //?? Add union so this space can be used for inlined data
    //   such a symlinks
    union {
	uint64_t ptrs[CFLSH_USFS_NUM_TOTAL_INODE_PTRS];
	char sym_link_path[CFLSH_USFS_SYM_LINK_MAX];
    };


    uint64_t directory_inode;   /* Inode index of directory */
    uint64_t num_links;         /* Number of links to this  */
#if !defined(__64BIT__) && defined(_AIX)
    time64_t ctime;             /* Inode change time - when */
                                /* inode was last modified  */
    time64_t mtime;             /* Last time file content   */
				/* was modified.            */
    time64_t atime;             /* Last time file was       */
				/* accessed.                */ 
#else
    time_t ctime;               /* Inode change time - when */
                                /* inode was last modified  */
    time_t mtime;               /* Last time file content   */
				/* was modified.            */
    time_t atime;               /* Last time file was       */
				/* accessed.                */ 
#endif  
    /* ??

      timestamp for creating inode
      data block for attributes.
    */

    /*
     * Need to ensure inode is exactly 512 bytes in size.
     * Larger sizes could be used in the future provided
     * that they evenly fit in 4096.
     * This allows a 4096 disk block to evenly fit inodes in 
     * it, without any of them spanning disk block boundaries. 
     * The inode processing code in this library assumes 
     * inodes do not span disk block boundaries.
     */

    char pad[144];


    uint64_t   end_marker;
#define CLFSH_USFS_INODE_EM __EYEC8('c','u','f','s','_','i','t','e')
} cflsh_usfs_inode_t;



/************************************************************************/
/* free block table  data structures for this filesystem.               */
/************************************************************************/

typedef struct cflsh_usfs_free_block_tble_hdr_s {
    uint64_t   start_marker;
#define CLFSH_USFS_FREE_TBL_SM __EYEC8('c','u','f','s','_','f','t','h')
    uint64_t   flags;
#define CFLSH_USFS_FBLK_TBL_COMPLETE 0x0001 /* Free table has been completed */
					   /* This means it has been        */
                                           /* fully populated on disk       */
    uint32_t   version;
    uint32_t   status;
    uint64_t   num_entries;
    uint64_t   entry[1];
} cflsh_usfs_free_block_tble_hdr_t;


/************************************************************************/
/* Journal data structures for this filesystem.                           */
/************************************************************************/


/* Journal entry types */
typedef enum {
    CFLSH_USFS_JOURNL_E_INVAL            = 0,   /* Invalid Journal type           */
    CFLSH_USFS_JOURNAL_E_ADD_BLK         = 1,   /* Get Block Table Entry          */
    CFLSH_USFS_JOURNAL_E_FREE_BLK        = 2,   /* Free Block Table Entry         */
    CFLSH_USFS_JOURNAL_E_GET_INODE       = 3,   /* Free Inode                     */
    CFLSH_USFS_JOURNAL_E_FREE_INODE      = 4,   /* Free Inode                     */
    CFLSH_USFS_JOURNAL_E_ADD_INODE_DBLK  = 5,   /* Add data block to Inode        */
    CFLSH_USFS_JOURNAL_E_FREE_INODE_DBLK = 6,   /* Free data block from Inode     */
    CFLSH_USFS_JOURNAL_E_FREE_DIR_E      = 7,   /* Free entry from directory      */
    CFLSH_USFS_JOURNAL_E_INODE_FSIZE     = 8,   /* Inode file_size change         */
} cflsh_journal_entry_type_t;

typedef struct cflsh_usfs_journal_entry_s {
    uint64_t   start_marker;
#define CLFSH_USFS_JOURNAL_E_SM __EYEC8('c','u','f','s','_','j','n','e')
    cflsh_journal_entry_type_t type;
    uint32_t reserved;                   /* Reserved for future use           */
    union {
	struct {                         /* For Add/Free Block Table Entry   */
	    uint64_t block_no;
	} blk;
	struct {                         /* For Free Inode Entry             */
	    uint64_t inode_num;
	} inode;
	struct {                          /* For Inode data block            */
	    uint64_t inode_num;
	    uint64_t block_num;
	} inode_dblk;
	struct {                          /* For Inode file size change      */
	    uint64_t old_file_size;
	    uint64_t new_file_size;
	} inode_fsize;
	struct {
	    uint64_t directory_inode_num;
	    uint64_t file_inode_num;
	} dir;

    };

} cflsh_usfs_journal_entry_t;


typedef struct cflsh_usfs_journal_hdr_s {
    uint64_t   start_marker;
#define CLFSH_USFS_JOURNAL_SM  __EYEC8('c','u','f','s','_','j','l','h')
    uint32_t   version;
    uint32_t   status;
    uint64_t   flags;
    uint64_t   reserved;
    uint64_t   num_entries;
    cflsh_usfs_journal_entry_t entry[1];
    
    
} cflsh_usfs_journal_hdr_t;

typedef enum {
    CFLSH_USFS_JOURNL_EV_INVAL    = 0,  /* Invalid Journal event type     */
    CFLSH_USFS_JOURNAL_EV_ADD     = 1,  /* Add/write event to journal     */
    CFLSH_USFS_JOURNAL_EV_REMOVE  = 2,  /* Remove event from journal      */

} cflash_journal_event_type_t;


/************************************************************************/
/* cflsh_usfs_journal_ev - The data structure for a journaled events */
/*                                                                      */
/* .                                                                    */
/************************************************************************/


typedef struct cflsh_usfs_journal_ev_s {
    struct cflsh_usfs_journal_ev_s *prev;  /* Previous chunk in list      */
    struct cflsh_usfs_journal_ev_s *next;  /* Next chunk in list          */
    
    int flags;
    
    cflsh_usfs_journal_entry_t journal_entry;
    uint64_t                  offset;     /* In case of a remove event   */
                                          /* this contains the offset of */
                                          /* the event to be removed     */
    eye_catch4b_t  eyec;                  /* Eye catcher                 */

} cflsh_usfs_journal_ev_t;

#define CFLSH_USFS_DIR_MAX_NAME  256
/************************************************************************/
/* Directory data structures for this filesystem.                       */
/************************************************************************/

typedef struct cflsh_usfs_directory_entry_s {
    uint32_t version;
    uint32_t flags;
#define CFLSH_USFS_DIR_VAL   0x0001  /* Valid directory entry */
    uint64_t inode_number;
    uint64_t reserved;
    cflsh_usfs_inode_file_type_t type;
    char filename[CFLSH_USFS_DIR_MAX_NAME];

} cflsh_usfs_directory_entry_t;


typedef struct cflsh_usfs_directory_s {
    uint64_t   start_marker;
#define CLFSH_USFS_DIRECTORY_SM   __EYEC8('c','u','f','s','_','d','i','r')
    uint64_t   flags;
    uint32_t   version;
    uint32_t   status;
    uint64_t   num_entries;
    cflsh_usfs_directory_entry_t entry[1];
    
    
} cflsh_usfs_directory_t;



/************************************************************************/
/* Disk lba list entry                                                  */
/************************************************************************/
typedef struct cflsh_usfs_disk_lba_e_s {
    uint64_t start_lba;               /* Starting disk LBA/sector for   */
                                      /* for this request.              */
    uint64_t num_lbas;                /* Number of disk LBAs/sectors    */
} cflsh_usfs_disk_lba_e_t;


/************************************************************************/
/* Filesystem lba list entry                                            */
/************************************************************************/
typedef struct cflsh_usfs_fs_lba_e_s {
    uint64_t start_lba;               /* Starting fs LBA/sector for      */
                                      /* for this request.               */
    uint64_t num_lbas;                /* Number of fs LBAs/sectors       */
} cflsh_usfs_fs_lba_e_t;


/************************************************************************/
/* Miscellaneous                                                        */
/************************************************************************/

/* flags for cusfs_add_fs_blocks_to_inode_ptrs_cleanup */

#define CUSFS_UPDATE_SINGLE_INODE 0x2
#define CUSFS_UPDATE_DOUBLE_INODE 0x4
#define CUSFS_UPDATE_TRIPLE_INODE 0x8

#endif /* _H_CFLASH_USFS_DISK */
