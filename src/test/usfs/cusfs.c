/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* bos720 src/bos/usr/ccs/lib/libcflsh_block/tools/blk_test.c 1.2         */
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
#ifdef _AIX
static char sccsid[] = "%Z%%M%   %I%  %W% %G% %U%";
#endif


/*
 * COMPONENT_NAME: (sysxcflashblock) CAPI Flash block library
 *
 * FUNCTIONS: 
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

#ifndef _AIX
#define _GNU_SOURCE
#endif /* !_AIX */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#ifndef _MACOSX 
#include <malloc.h>
#endif /* !_MACOS */
#include <unistd.h>
#include <inttypes.h>
#ifdef _AIX
#include <sys/aio.h>
#else
#include <aio.h>
#endif
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif
#include <sys/stat.h>
#include <sys/statfs.h>
#include <utime.h>
#include <cflsh_usfs.h>
#include <cflsh_usfs_admin.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef TIMELEN
#define   TIMELEN  26           /* Linux does have a define for the minium size of the a timebuf */
                                /* However linux man pages say it is 26                          */
#endif 

#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif

/************************************************************************/
/* Subcommand enumerations                                              */
/************************************************************************/

typedef 
enum {
    CUSFS_APP_CMD_CRFS       = 0x1,  /* Create filesystem                  */
    CUSFS_APP_CMD_RMFS       = 0x2,  /* Remove filesystem                  */
    CUSFS_APP_CMD_MKDIR      = 0x3,  /* Make directory                     */
    CUSFS_APP_CMD_RMDIR      = 0x4,  /* Remove directory                   */
    CUSFS_APP_CMD_LS         = 0x5,  /* List files in directory            */
    CUSFS_APP_CMD_CAT        = 0x6,  /* Cat file                           */
    CUSFS_APP_CMD_STAT       = 0x7,  /* stat a file                        */
    CUSFS_APP_CMD_CHMOD      = 0x8,  /* chmod a file                       */
    CUSFS_APP_CMD_CHGRP      = 0x9,  /* chgrp a file                       */
    CUSFS_APP_CMD_CHOWN      = 0xa,  /* chown a file                       */
    CUSFS_APP_CMD_MV         = 0xb,  /* move a file                        */
    CUSFS_APP_CMD_COPY       = 0xc,  /* copy a file                        */
    CUSFS_APP_CMD_LN         = 0xd,  /* link a file                        */
    CUSFS_APP_CMD_TRUNCATE   = 0xe,  /* truncate a file                    */
    CUSFS_APP_CMD_UTIME      = 0xf,  /* set mod time on a file             */
    CUSFS_APP_CMD_FSCK       = 0x10, /* fsck the filesystem                */
    CUSFS_APP_CMD_TOUCH      = 0x11, /* set mod time on a file to now.     */
    CUSFS_APP_CMD_RMFILE     = 0x12, /* Remove a file.                     */
    CUSFS_APP_CMD_QUERYFS    = 0x13, /* Query a filesystem                 */
    CUSFS_APP_CMD_STATFS     = 0x14, /* Query a filesystem                 */
    CUSFS_APP_CMD_AIO_COPY   = 0x15, /* copy a file doing writes via AIO   */
    CUSFS_APP_CMD_LAST       = 0x16, /* Not valid command                  */
} cusfs_app_cmdline_t;


struct cmd_lookup_s {
    cusfs_app_cmdline_t cmd;
    char *cmd_string;
};


struct cmd_lookup_s cmds[] = {
    {CUSFS_APP_CMD_CRFS,"crfs"},
    {CUSFS_APP_CMD_RMFS,"rmfs"},
    {CUSFS_APP_CMD_MKDIR,"mkdir"},
    {CUSFS_APP_CMD_RMDIR,"rmdir"},
    {CUSFS_APP_CMD_LS,"ls"},
    {CUSFS_APP_CMD_CAT,"cat"},
    {CUSFS_APP_CMD_STAT,"stat"},
    {CUSFS_APP_CMD_CHMOD,"chmod"},
    {CUSFS_APP_CMD_CHGRP,"chgrp"},
    {CUSFS_APP_CMD_CHOWN,"chown"},
    {CUSFS_APP_CMD_MV,"mv"},
    {CUSFS_APP_CMD_COPY,"cp"},
    {CUSFS_APP_CMD_AIO_COPY,"aiocp"},
    {CUSFS_APP_CMD_LN,"ln"},
    {CUSFS_APP_CMD_TRUNCATE,"truncate"},
    {CUSFS_APP_CMD_UTIME,"utime"},
    {CUSFS_APP_CMD_TOUCH,"touch"},
    {CUSFS_APP_CMD_RMFILE,"rm"},
    {CUSFS_APP_CMD_QUERYFS,"queryfs"},
    {CUSFS_APP_CMD_STATFS,"statfs"},
    {CUSFS_APP_CMD_FSCK,"fsck"},
};




/************************************************************************/
/* Global variables adjusted by parsing command line arguments.         */
/************************************************************************/

static char *command = NULL;

static int bflag;               /* indicates the b flag was passed on command
                                 * line which indicates a block/transfer size
                                 * us specified.
                                 */

static int block_size = 0;      /* point to the arg for -b flag */

static int cflag;               /* indicates the c flag was passed on command
                                 * line which indicates a command was 
                                 * specified.
                                 */

static cusfs_app_cmdline_t cmd_op = CUSFS_APP_CMD_LAST;   /* point to the arg for the -c flag */

static int dflag;               /* indicates the d flag was passed on command
                                 * line which indicates a device name was 
                                 * specified.
                                 */
static char *device_name = NULL; /* point to the arg for -d flag */

static int Dflag;               /* indicates the D flag was passed on command
                                 * line which indicates another device name was 
                                 * specified.
                                 */
static char *Device_name = NULL; /* point to the arg for -D flag */


static int force_flag;          /* indicates the f flag was passed on command
                                 * line which indicates a force option is 
                                 * specified.
                                 */


static int gflag;               /* indicates the g flag was passed on command
                                 * line which indicates a group was 
                                 * specified.
                                 */
static int hflag;               /* indicates the h flag was passed on command
                                 * line which indicates a help was 
                                 * specified.
                                 */
static int group_id = 0;        /* point to the arg for -g flag */

static int lflag;               /* indicates the l flag was passed on command
                                 * line which indicates a length was 
                                 * specified.
                                 */
static int length = 0;           /* point to the arg for -l flag */

static int mflag;               /* indicates the m flag was passed on command
                                 * line which indicates a mode was 
                                 * specified.
                                 */
static int mode = 0;            /* point to the arg for -m flag */

static int oflag;               /* indicates the o flag was passed on command
                                 * line which indicates to preserve old settings
                                 * as specified.
                                 */
static int pflag;               /* indicates the p flag was passed on command
                                 * line which indicates a path was 
                                 * specified.
                                 */
static char *path = NULL;        /* point to the arg for -p flag */

static int Pflag;               /* indicates the P flag was passed on command
                                 * line which indicates an (old) path was 
                                 * specified.
                                 */
static char *Path = NULL;        /* point to the arg for -P flag */


static int sflag;               /* indicates the s flag was passed on command
                                 * line which indicates a path was 
                                 * specified.
                                 */

static int tflag;               /* indicates the t flag was passed on command
                                 * line which indicates a time was 
                                 * specified.
                                 */
static int time_val = 0;        /* point to the arg for -t flag */

static int uflag;               /* indicates the u flag was passed on command
                                 * line which indicates a user id was 
                                 * specified.
                                 */
static int user_id = 0;          /* point to the arg for -u flag */

static int fork_flag = 0;       /* indicates the z flag was passed on command
                                 * line which indicates a to fork 
                                 */


char disk_pathname[PATH_MAX];
char disk_pathname2[PATH_MAX];


/*
 * NAME:        Usage
 *
 * FUNCTION:    print usage message and returns
 *
 *
 * INPUTS:
 *    argc  -- INPUT; the argc parm passed into main().
 *    argv  -- INPUT; the argv parm passed into main().
 *
 * RETURNS:
 *              0: Success
 *              -1: Error
 *              
 */
static void
usage(void)
{

    fprintf(stderr,"\n");
    
    fprintf(stderr,"Usage:cusfs -c command\n\n");
    fprintf(stderr,"            -c crfs -d disk_name [ -f ]\n");
    fprintf(stderr,"                     where -f is force flag\n");
    fprintf(stderr,"            -c queryfs -d disk_name\n");
    fprintf(stderr,"            -c statfs -d disk_name\n");
    fprintf(stderr,"            -c rmfs -d disk_name\n");
    fprintf(stderr,"            -c mkdir -p path -d disk_name\n");
    fprintf(stderr,"            -c rmdir -p path -d disk_name\n");
    fprintf(stderr,"            -c rm -p path -d disk_name\n");
    fprintf(stderr,"            -c ls -p path -d disk_name\n");
    fprintf(stderr,"            -c cat -p path -d disk_name\n");
    fprintf(stderr,"            -c stat -p path -d disk_name\n");
    fprintf(stderr,"            -c chmod -m mode -p path -d disk_name\n");
    fprintf(stderr,"            -c chgrp -g group -p path -d disk_name\n");
    fprintf(stderr,"            -c chown -u user_id -p path -d disk_name\n");
    fprintf(stderr,"            -c mv -P old_path -p new_path -d disk_name\n");
    fprintf(stderr,"            -c cp -P old_path [ -D old_disk_name] -p new_path [ -d new_disk_name ] [ -o ] [ -z ] [ -b block_size ]\n");
    fprintf(stderr,"                     where -o is to preserve mode and time stamps\n");
    fprintf(stderr,"                     where -z indicates to fork after opening and stating\n");
    fprintf(stderr,"            -c aiocp -P old_path [ -D old_disk_name] -p new_path [ -d new_disk_name ] [ -o ] [ -z ] [ -b block_size ]\n");
    fprintf(stderr,"                     where -o is to preserve mode and time stamps\n");
    fprintf(stderr,"                     where -z indicates to fork after opening and stating\n");
    fprintf(stderr,"            -c ln  -P old_path -p new_path -d new_disk_name [ -s ]\n");
    fprintf(stderr,"            -c truncate  -p path -d disk_name -l length_bytes\n");
    fprintf(stderr,"            -c utime  -p path -d disk_name -t times\n");
    fprintf(stderr,"            -c touch  -p path -d disk_name\n");
    fprintf(stderr,"            -c fsck -d disk_name\n");
    fprintf(stderr,"            -h : help\n");

    
    return;
}


/*
 * NAME:        parse_args
 *
 * FUNCTION:    Parse command line arguments
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */

static int
parse_args(int argc, char **argv)
{
    extern int   optind;
    extern char *optarg;
    int rc = 0;
    int c;
    int i;


    bflag     = FALSE;
    cflag     = FALSE;
    dflag     = FALSE;
    Dflag     = FALSE;
    gflag     = FALSE;
    lflag     = FALSE;
    mflag     = FALSE;
    oflag     = FALSE;
    pflag     = FALSE;
    Pflag     = FALSE;
    sflag     = FALSE;
    tflag     = FALSE;
    uflag     = FALSE;
    force_flag = FALSE;
    fork_flag = FALSE;

    
    /* 
     * Get parameters
     */
    while ((c = getopt(argc,argv,"b:c:d:D:g:l:m:p:P:t:u:fhsz")) != EOF)
    {  
        switch (c)
        { 

        case 'b' :
	    if (optarg) {

		block_size = atoi(optarg);

	   
		bflag = TRUE;
            } else {

                fprintf(stderr,"-b flag requires block_size \n");
                rc = EINVAL;

            }

	    break;
        case 'c' :
            if (optarg) {
                
                command = optarg;
                
		i = 0;
		while (i < CUSFS_APP_CMD_LAST) {

		    if (!strcmp(cmds[i].cmd_string,command)) {

			cmd_op = cmds[i].cmd;
			break;
		    }
		    i++;
		}
		if (cmd_op == CUSFS_APP_CMD_LAST) {
                    fprintf(stderr,"Invalid cmd %s for -c flag\n",command);
                    
                    usage();
                    rc = -1;
                } else {
                    cflag = TRUE;
                }
            } else {
                
                
                fprintf(stderr,"-c flag requires a value to be supplied\n");
                
            }
            break;
        case 'd' :
	    device_name = optarg;
            if (device_name) {
                dflag = TRUE;
            } else {

                fprintf(stderr,"-d flag requires a device name \n");
                rc = EINVAL;

            }
	    break;

        case 'D' :
	    Device_name = optarg;
            if (Device_name) {
                Dflag = TRUE;
            } else {

                fprintf(stderr,"-D flag requires a device name \n");
                rc = EINVAL;

            }
	    break;

        case 'f' :
	   
	    force_flag = TRUE;
	    break;


        case 'h' :
	   
            usage();
	    break;


        case 'g' :
	    if (optarg) {

		group_id = atoi(optarg);

		gflag = TRUE;
            } else {

                fprintf(stderr,"-g flag requires mode \n");
                rc = EINVAL;

            }
	    break;


        case 'l' :
	    if (optarg) {

		length = atoi(optarg);

		lflag = TRUE;
            } else {

                fprintf(stderr,"-l flag requires length \n");
                rc = EINVAL;

            }
	    break;


        case 'm' :
	    if (optarg) {

		mode = atoi(optarg);

		mflag = TRUE;
            } else {

                fprintf(stderr,"-m flag requires mode \n");
                rc = EINVAL;

            }
	    break;

        case 'o' :
	   
	    oflag = TRUE;
	    break;

        case 'p' :
	    path = optarg;
            if (path) {
                pflag = TRUE;
            } else {

                fprintf(stderr,"-p flag requires a path \n");
                rc = EINVAL;

            }
	    break;
        case 'P' :
	    Path = optarg;
            if (Path) {
                Pflag = TRUE;
            } else {

                fprintf(stderr,"-P flag requires a path \n");
                rc = EINVAL;

            }
	    break;


        case 's' :
	   
	    sflag = TRUE;
	    break;

        case 't' :
	    if (optarg) {

		time_val = atoi(optarg);

		tflag = TRUE;
            } else {

                fprintf(stderr,"-t flag requires length \n");
                rc = EINVAL;

            }
	    break;

        case 'u' :
	    if (optarg) {

		user_id = atoi(optarg);

		uflag = TRUE;
            } else {

                fprintf(stderr,"-u flag requires mode \n");
                rc = EINVAL;

            }
	    break;
        case 'z' :
	   
	    fork_flag = TRUE;
	    break;
        default: 
            usage();
            break;
        }/*switch*/
    }/*while*/

    if (!cflag) {
        fprintf(stderr,"The -c flag is required to specify a command\n");
        usage();
        rc = EINVAL;
	return rc;
    }

    if ((!dflag) &&
	(cmd_op != CUSFS_APP_CMD_COPY) &&
	(cmd_op != CUSFS_APP_CMD_AIO_COPY)) {
        fprintf(stderr,"The -d flag is required to specify a device name\n");
        usage();
        rc = EINVAL;
	return rc;
    }

    if ((!pflag) &&
	((cmd_op == CUSFS_APP_CMD_MKDIR) ||
	 (cmd_op == CUSFS_APP_CMD_RMDIR) ||
	 (cmd_op == CUSFS_APP_CMD_RMFILE) ||
	 (cmd_op == CUSFS_APP_CMD_LS) ||
	 (cmd_op == CUSFS_APP_CMD_CAT) ||
	 (cmd_op == CUSFS_APP_CMD_STAT) ||
	 (cmd_op == CUSFS_APP_CMD_CHMOD) ||
	 (cmd_op == CUSFS_APP_CMD_CHGRP) ||
	 (cmd_op == CUSFS_APP_CMD_CHOWN) ||
	 (cmd_op == CUSFS_APP_CMD_MV) ||
	 (cmd_op == CUSFS_APP_CMD_COPY) ||
	 (cmd_op == CUSFS_APP_CMD_AIO_COPY) ||
	 (cmd_op == CUSFS_APP_CMD_LN) ||
	 (cmd_op == CUSFS_APP_CMD_TRUNCATE) ||
	 (cmd_op == CUSFS_APP_CMD_TOUCH) ||
	 (cmd_op == CUSFS_APP_CMD_UTIME))) {


        fprintf(stderr,"The -p flag is required to specify the path for this command %s\n",command);
        usage();
        rc = EINVAL;

    }

    if ((!Pflag) &&
	((cmd_op == CUSFS_APP_CMD_MV) ||
	 (cmd_op == CUSFS_APP_CMD_COPY) ||
	 (cmd_op == CUSFS_APP_CMD_AIO_COPY) ||
	 (cmd_op == CUSFS_APP_CMD_LN))) {


        fprintf(stderr,"The -P flag is required to specify the old path for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;
    }

    if ((!tflag) &&
	(cmd_op == CUSFS_APP_CMD_UTIME)) {

        fprintf(stderr,"The -t flag is required to specify the time for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }

    if ((!lflag) &&
	(cmd_op == CUSFS_APP_CMD_TRUNCATE)) {

        fprintf(stderr,"The -l flag is required to specify the length for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }
    if (!(Dflag || dflag) &&
	((cmd_op == CUSFS_APP_CMD_COPY) ||
	 (cmd_op == CUSFS_APP_CMD_AIO_COPY))) {

        fprintf(stderr,"Either -d or -D flag is required to specify the for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }

    if ((!mflag) &&
	(cmd_op == CUSFS_APP_CMD_CHMOD)) {

        fprintf(stderr,"The -m flag is required to specify the mode for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }

    if ((!gflag) &&
	(cmd_op == CUSFS_APP_CMD_CHGRP)) {

        fprintf(stderr,"The -g flag is required to specify the group for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }

    if ((!uflag) &&
	(cmd_op == CUSFS_APP_CMD_CHOWN)) {

        fprintf(stderr,"The -u flag is required to specify the user_id for this command %s\n",command);
        usage();
        rc = EINVAL;
	return rc;

    }


    return (rc);


}/*parse_args*/


/*
 * NAME:        common_seek
 *
 * FUNCTION:    Seeks either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
#ifdef _AIX
offset_t common_seek(int fd, offset_t Offset, int Whence, int capi_ufs) 
{
    offset_t rc;

    if (capi_ufs) {
	rc = cusfs_llseek(fd,Offset,Whence);

    } else {
	rc = llseek(fd,Offset,Whence);

    }


    return rc;
}
#else

off_t common_seek(int fd, off_t Offset, int Whence, int capi_ufs) 
{
    off_t rc;

    if (capi_ufs) {
	rc = cusfs_lseek(fd,Offset,Whence);

    } else {
	rc = lseek(fd,Offset,Whence);

    }


    return rc;
}
#endif

/*
 * NAME:        common_fstat
 *
 * FUNCTION:    fstat either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int common_fstat(int fd, struct stat *Buffer, int capi_ufs) 
{
    int rc;

    if (capi_ufs) {
	rc = cusfs_fstat(fd,Buffer);

    } else {
	rc = fstat(fd,Buffer);

    }


    return rc;
}

/*
 * NAME:        common_read
 *
 * FUNCTION:    read either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int common_read(int fd, void *buffer,size_t nbytes,int flags, int capi_ufs) 
{
    int rc;

    if (capi_ufs) {
	rc = cusfs_read(fd,buffer,nbytes);

    } else {
	rc = read(fd,buffer,nbytes);

    }


    return rc;
}

/*
 * NAME:        common_write
 *
 * FUNCTION:    write either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int common_write(int fd, void *buffer,size_t nbytes,int flags, int capi_ufs) 
{
    int rc;

    if (capi_ufs) {
	rc = cusfs_write(fd,buffer,nbytes);

    } else {
	rc = write(fd,buffer,nbytes);

    }


    return rc;
}


/*
 * NAME:        common_write
 *
 * FUNCTION:    write either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
#ifdef _AIX
int common_aio_write(int fd, void *buffer,size_t nbytes,offset_t aio_offset,int flags, int capi_ufs) 
#else
int common_aio_write(int fd, void *buffer,size_t nbytes,off_t aio_offset,int flags, int capi_ufs)
#endif 
{
    int rc;
    struct aiocb64 aiocb_arg;
    int retry = 0;

    bzero(&aiocb_arg,sizeof(aiocb_arg));

    aiocb_arg.aio_offset = aio_offset;
    aiocb_arg.aio_buf = buffer;
    aiocb_arg.aio_nbytes = nbytes;
    aiocb_arg.aio_fildes = fd;

    if (capi_ufs) {
	rc = cusfs_aio_write64(&aiocb_arg);

    } else {
	rc = aio_write64(&aiocb_arg);

    }


    if (rc < 0) {

	return rc;
    }

    while (retry < 10000) {
	
	if (capi_ufs) {
	    rc = cusfs_aio_error64(&aiocb_arg);

	} else {
	    rc = aio_error64(&aiocb_arg);

	}

	if ((rc == 0) ||
	    (rc != EINPROGRESS)) {

	    break;
	}

	usleep(1000);
	retry++;

    }

    if (!rc) {

	
	if (capi_ufs) {
	    rc = cusfs_aio_return64(&aiocb_arg);

	} else {
	    rc = aio_return64(&aiocb_arg);

	}
    }
    return rc;
}


/*
 * NAME:        common_fstat
 *
 * FUNCTION:    close either a file in either the 
 *              CAPI Flash filesystem or the traditional
 *              filesystem.
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int common_close(int fd, int capi_ufs) 
{
    int rc;

    if (capi_ufs) {
	rc = cusfs_close(fd);

    } else {
	rc = close(fd);

    }


    return rc;
}

/*
 * NAME:        create_fs
 *
 * FUNCTION:    Create Userspace filesystem on a CAPI flash disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int create_fs(void) 
{
    int rc = 0;
    int flags = 0;


    if (force_flag) {

	flags = CUSFS_FORCE_CREATE_FS;
    }

    rc = cusfs_create_fs(device_name,flags);


    return rc;
}

/*
 * NAME:        query_fs
 *
 * FUNCTION:    Query a userspace filesystem from a CAPI flash disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int query_fs(void) 
{
    int rc = 0;
    struct cusfs_query_fs query;
    char timebuf[TIMELEN+1];

    bzero(&query,sizeof(query));


    rc = cusfs_query_fs(device_name,&query,0);

    if ((!rc) ||
#ifdef _AIX
	(errno == ECORRUPT)) {
#else
	(errno == EBADR)) {
#endif

	printf("version          = 0x%x\n",query.version);

	if (query.flags & CUSFS_QRY_DIF_ENDIAN) {

	    printf("flags            = 0x%x: !Different endianess!\n",query.flags);
	} else {
	    printf("flags            = 0x%x\n",query.flags);
	}


	printf("os_type          = 0x%x\n",query.os_type);
	printf("fs_block_size    = 0x%"PRIX64"\n",query.fs_block_size);
	printf("disk_block_size  = 0x%"PRIX64"\n",query.disk_block_size);
	printf("num_blocks       = 0x%"PRIX64"\n",query.num_blocks);
	printf("free_table_size  = 0x%"PRIX64"\n",query.free_block_table_size);
	printf("inode_table_size = 0x%"PRIX64"\n",query.inode_table_size);
	printf("create_time      = %s",ctime_r(&(query.create_time),timebuf));
	printf("write_time       = %s",ctime_r(&(query.write_time),timebuf));
	printf("mount_time       = %s",ctime_r(&(query.mount_time),timebuf));
	printf("fsck_time        = %s",ctime_r(&(query.fsck_time),timebuf));

    }

    return rc;
}


/*
 * NAME:        stat_fs
 *
 * FUNCTION:    Query a userspace filesystem from a CAPI flash disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int stat_fs(void) 
{
    int rc = 0;
    struct statfs statfs;


    bzero(&statfs,sizeof(statfs));


    rc = cusfs_statfs(device_name,&statfs);

    if (!rc) {
#ifdef _AIX
	printf("version          = 0x%x\n",statfs.f_version);
	printf("f_type           = 0x%x\n",statfs.f_type);
	printf("f_fsize          = 0x%lx\n",statfs.f_fsize);
	printf("f_fname          = %s\n",statfs.f_fname);
#else
	printf("f_frsize          = 0x%lx\n",statfs.f_frsize);

#endif 
	printf("f_bsize          = 0x%lx\n",statfs.f_bsize);
	printf("f_blocks         = 0x%lx\n",statfs.f_blocks);
	printf("f_bfree          = 0x%lx\n",statfs.f_bfree);
	printf("f_bavail         = 0x%lx\n",statfs.f_bavail);
	printf("f_files          = 0x%lx\n",statfs.f_files);
	printf("f_ffree          = 0x%lx\n",statfs.f_ffree);

    }

    return rc;
}

/*
 * NAME:        remove_fs
 *
 * FUNCTION:    Remove Userspace filesystem from a CAPI flash disk
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int remove_fs(void) 
{
    int rc = 0;

    rc = cusfs_remove_fs(device_name,0);

    return rc;
}


/*
 * NAME:        mk_directory
 *
 * FUNCTION:    Make a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int mk_directory(void) 
{
    int rc = 0;

    rc = cusfs_mkdir(disk_pathname,mode);

    return rc;
}

/*
 * NAME:        rm_directory
 *
 * FUNCTION:    Remove a directory
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int rm_directory(void) 
{
    int rc = 0;


    rc = cusfs_rmdir(disk_pathname);

    return rc;
}


/*
 * NAME:        rm_file
 *
 * FUNCTION:    Remove a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int rm_file(void) 
{
    int rc = 0;


    rc = cusfs_unlink(disk_pathname);

    return rc;
}


/*
 * NAME:        list_files
 *
 * FUNCTION:    List files (ls)
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int list_files(void) 
{
    int rc = 0;
#ifdef _AIX
    DIR64 *directory;
    struct dirent64 file;
    struct dirent64 *result = &file;
    struct stat64 stats;
#else
    DIR *directory;
    struct dirent file;
    struct dirent *result = &file;
    struct stat stats;
#endif 
    char stat_name[PATH_MAX];
    char timebuf[TIMELEN+1];
    char time_string[TIMELEN+1];

#ifdef _AIX
    directory = cusfs_opendir64(disk_pathname);

#else
    directory = cusfs_opendir(disk_pathname);
#endif

    if (directory == NULL) {

	fprintf(stderr,"opendir64 returned null pointer with errno = %d\n",errno);
	return -1;
    }

    printf("Directory name = %s\n",path);

    printf("mode      \tlinks\tuid\tgid\tmtime                   \tsize\tinode\tfilename\n");

#ifdef _AIX
    while ((cusfs_readdir64_r(directory,&file,&result) == 0) && (result != NULL)) {
#else
    while ((cusfs_readdir_r(directory,&file,&result) == 0) && (result != NULL)) {
#endif 

	bzero(&stats,sizeof(stats));

	sprintf(stat_name,"%s%s",disk_pathname,file.d_name);


#ifdef _AIX
	rc = cusfs_stat64(stat_name,&stats);
#else
	rc = cusfs_stat(stat_name,&stats);
#endif

	if (rc) {
	    break;
	}
	
	sprintf(time_string,"%s",ctime_r(&(stats.st_mtime),timebuf));
	
	/*
	 * Remove newline from time_string
	 */
	
	time_string[strlen(time_string) - 1] = '\0';


	/*
	 * ?? This needs to be cleaned up
	 */
	
	if (stats.st_mode & S_IFDIR) {
	    printf("d");
	} else if ( S_ISLNK(stats.st_mode)) {
	    printf("l");
	} else {
	    printf("-");
	}

	if (stats.st_mode & S_IRUSR) {
	    printf("r");
	} else {
	    printf("-");
	}

	if (stats.st_mode & S_IWUSR) {
	    printf("w");
	} else {
	    printf("-");
	}


	if (stats.st_mode & S_IXUSR) {
	    printf("x");
	} else {
	    printf("-");
	}

	if (stats.st_mode & S_IRGRP) {
	    printf("r");
	} else {
	    printf("-");
	}

	if (stats.st_mode & S_IWGRP) {
	    printf("w");
	} else {
	    printf("-");
	}


	if (stats.st_mode & S_IXGRP) {
	    printf("x");
	} else {
	    printf("-");
	}


	if (stats.st_mode & S_IROTH) {
	    printf("r");
	} else {
	    printf("-");
	}

	if (stats.st_mode & S_IWOTH) {
	    printf("w");
	} else {
	    printf("-");
	}


	if (stats.st_mode & S_IXOTH) {
	    printf("x");
	} else {
	    printf("-");
	}

#if !defined(__64BIT__) && defined(_AIX)
	printf("\t%d\t%d\t%d\t%s\t0x%"PRIX64"\t0x%x\t%s\n",
	       (int)stats.st_nlink,stats.st_uid, stats.st_gid,
	       time_string,
	       stats.st_size,stats.st_ino,file.d_name);	
#else
	printf("\t%d\t%d\t%d\t%s\t0x%"PRIX64"\t0x%"PRIX64"\t%s\n",
	       (int)stats.st_nlink,stats.st_uid, stats.st_gid,
	       time_string,
	       stats.st_size,stats.st_ino,file.d_name);	
#endif 
	
    } /*while */

#ifdef _AIX
    if (cusfs_closedir64(directory)) {
#else
    if (cusfs_closedir(directory)) {
#endif

	fprintf(stderr,"closedir64 failed with errno = %d\n",errno);
    }


#ifdef _REMOVE
    rc = cusfs_list_files(disk_pathname,0);
#endif

    return rc;
}

/*
 * NAME:        move_file
 *
 * FUNCTION:    Move a file 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int move_file(void) 
{
    int rc = 0;

    rc = cusfs_rename(disk_pathname,disk_pathname2);

    return rc;
}


/*
 * NAME:        copy_file
 *
 * FUNCTION:    Copy a file 
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int copy_file(void) 
{
    int rc = 0;
    int src_fd;
    int dest_fd;
    int src_capi;
    int dest_capi;
    void *buf;
    int buf_size = 4096;
    int64_t remaining_size;
    mode_t mode;
    struct statfs usfs_statfs;
    struct stat stats;
    pid_t pid;
#ifdef _AIX
    offset_t offset;
#else
    off_t offset;
#endif





    bzero(&stats,sizeof(stats));

    if (Dflag) {
	/*
	 * A device name was specified, so this
	 * file is on a CAPI flash fileystem.
	 */


	/*
	 * Get filesystem optimal block size
	 */

	bzero(&usfs_statfs,sizeof(usfs_statfs));

	rc = cusfs_statfs(disk_pathname2,&usfs_statfs);

	if (rc) {

	    fprintf(stderr,"statfs of %s failed with errno = %d\n",
		    disk_pathname2,errno);
	    return -1;
	}

	if (bflag) {
	    buf_size = block_size;
	} else {
	    buf_size = usfs_statfs.f_bsize;
	}

	src_capi = TRUE;

	src_fd = cusfs_open(disk_pathname2,O_RDONLY,0);

    } else {

	/*
	 * A device was not specified. So
	 * this is from a traditional 
	 * filesystem.
	 */

	src_capi = FALSE;

	src_fd = open(Path,O_RDONLY,0);
    }

    if (src_fd < 0) {

	fprintf(stderr,"Open of %s failed with errno = %d\n",
		disk_pathname,errno);
	return -1;
	    
    }

    rc = common_fstat(src_fd,&stats,src_capi);

    if (dflag) {
	/*
	 * A device name was specified, so this
	 * file is on a CAPI flash fileystem.
	 */
	
	dest_capi = TRUE;

	/*
	 * Get filesystem optimal block size
	 */

	bzero(&usfs_statfs,sizeof(usfs_statfs));

	rc = cusfs_statfs(disk_pathname,&usfs_statfs);

	if (rc) {

	    fprintf(stderr,"statfs of %s failed with errno = %d\n",
		    disk_pathname,errno);
	    return -1;
	}

	if (Dflag) {
	    buf_size = MIN(usfs_statfs.f_bsize,buf_size);
	} else {
	    buf_size = usfs_statfs.f_bsize;
	}

	if (oflag) {

	    /*
	     * Caller has requested
	     * we preserve permissions.
	     */

	    // TODO?? Need more work here to preserve time stamps, uid, gid etc.


	    mode = stats.st_mode;
	} else {

	    mode = (S_IRWXU | S_IRWXG | S_IRWXO);
	}

	dest_fd = cusfs_open(disk_pathname,(O_WRONLY|O_CREAT),mode);

	rc = cusfs_ftruncate64(dest_fd,stats.st_size);

	if (rc) {

	    fprintf(stderr,"cusf_ftruncate64 of %s failed with errno = %d\n",
		    disk_pathname2,errno);
	    common_close(src_fd,src_capi);
	    return -1;

	}
    } else {

	/*
	 * A device was not specified. So
	 * this is from a traditional 
	 * filesystem.
	 */

	dest_capi = FALSE;

	dest_fd = open(path,(O_WRONLY|O_CREAT),0);

	rc = ftruncate64(dest_fd,stats.st_size);

	if (rc) {

	    fprintf(stderr,"ftruncate64 of %s failed with errno = %d\n",
		    path,errno);
	    common_close(src_fd,src_capi);
	    return -1;

	}
    }

    if (dest_fd < 0) {

	fprintf(stderr,"Open of %s failed with errno = %d\n",
		disk_pathname2,errno);
	return -1;
	    
    }


    if (fork_flag) {

	if ((pid = fork()) < 0) {  /* fork failed */
	    perror("Fork failed \n");

	}                 
	else if (pid > 0)  {               /* parents fork call */

	    fprintf(stderr,"Fork succeeded \n");


	    /*
	     * Let parent sleep long enough for child to start up.
	     */

	    sleep(100);

	    return 0;
	}

	/* 
	 * Only child get here
	 */



    }

    offset = common_seek(src_fd,0,SEEK_SET,src_capi);
    offset = common_seek(dest_fd,0,SEEK_SET,dest_capi);
	
    if (offset != 0) {

	fprintf(stderr,"seek to 0 of %s failed with errno = %d,\n",
		disk_pathname2,errno);
	return -1;

    }

    buf = malloc(buf_size);

    if (buf == NULL) {

	fprintf(stderr,"failed to malloc buf with errno = %d",errno);
    }

    bzero(buf,buf_size);

    remaining_size = stats.st_size;

    while (remaining_size > 0) {

	rc = common_read(src_fd,buf,buf_size,0,src_capi);

	if (rc <= 0) {
	    fprintf(stderr,"Read failed with rc = %d, but buf_size = %d with errno = %d\n",
		    rc,buf_size,errno);

	    break;
	    
	}

	remaining_size -= rc;
	
	rc = common_write(dest_fd,buf,rc,0,dest_capi);
	
    }

    if (remaining_size == 0) {

	rc = 0;
    }

    if (common_close(src_fd,src_capi)) {


	fprintf(stderr,"Close source file failed with errno = %d\n",errno);
    
    }


    if (common_close(dest_fd,dest_capi)) {


	fprintf(stderr,"Close destination file failed with errno = %d\n",errno);
    
    }

    return rc;
}

/*
 * NAME:        aio_copy_file
 *
 * FUNCTION:    Copy a file using asynchronous writes
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int aio_copy_file(void) 
{
    int rc = 0;
    int src_fd;
    int dest_fd;
    int src_capi;
    int dest_capi;
    void *buf;
    int buf_size = 4096;
    int64_t remaining_size;
    mode_t mode;
    struct statfs usfs_statfs;
    struct stat stats;
    pid_t pid;
#ifdef _AIX
    offset_t offset;
    offset_t aio_offset;
#else
    off_t offset;
    off_t aio_offset;
#endif





    bzero(&stats,sizeof(stats));

    if (Dflag) {
	/*
	 * A device name was specified, so this
	 * file is on a CAPI flash fileystem.
	 */


	/*
	 * Get filesystem optimal block size
	 */

	bzero(&usfs_statfs,sizeof(usfs_statfs));

	rc = cusfs_statfs(disk_pathname2,&usfs_statfs);

	if (rc) {

	    fprintf(stderr,"statfs of %s failed with errno = %d\n",
		    disk_pathname2,errno);
	    return -1;
	}

	buf_size = usfs_statfs.f_bsize;

	src_capi = TRUE;

	src_fd = cusfs_open(disk_pathname2,O_RDONLY,0);

    } else {

	/*
	 * A device was not specified. So
	 * this is from a traditional 
	 * filesystem.
	 */

	src_capi = FALSE;

	src_fd = open(Path,O_RDONLY,0);
    }

    if (src_fd < 0) {

	fprintf(stderr,"Open of %s failed with errno = %d\n",
		disk_pathname,errno);
	return -1;
	    
    }

    rc = common_fstat(src_fd,&stats,src_capi);

    if (dflag) {
	/*
	 * A device name was specified, so this
	 * file is on a CAPI flash fileystem.
	 */
	
	dest_capi = TRUE;

	/*
	 * Get filesystem optimal block size
	 */

	bzero(&usfs_statfs,sizeof(usfs_statfs));

	rc = cusfs_statfs(disk_pathname,&usfs_statfs);

	if (rc) {

	    fprintf(stderr,"statfs of %s failed with errno = %d\n",
		    disk_pathname,errno);
	    return -1;
	}

	if (Dflag) {
	    buf_size = MIN(usfs_statfs.f_bsize,buf_size);
	} else {
	    buf_size = usfs_statfs.f_bsize;
	}

	if (oflag) {

	    /*
	     * Caller has requested
	     * we preserve permissions.
	     */

	    // TODO?? Need more work here to preserve time stamps, uid, gid etc.


	    mode = stats.st_mode;
	} else {

	    mode = (S_IRWXU | S_IRWXG | S_IRWXO);
	}

	dest_fd = cusfs_open(disk_pathname,(O_WRONLY|O_CREAT),mode);

	rc = cusfs_ftruncate64(dest_fd,stats.st_size);

	if (rc) {

	    fprintf(stderr,"cusf_ftruncate64 of %s failed with errno = %d\n",
		    disk_pathname2,errno);
	    common_close(src_fd,src_capi);
	    return -1;

	}
    } else {

	/*
	 * A device was not specified. So
	 * this is from a traditional 
	 * filesystem.
	 */

	dest_capi = FALSE;

	dest_fd = open(path,(O_WRONLY|O_CREAT),0);

	rc = ftruncate64(dest_fd,stats.st_size);

	if (rc) {

	    fprintf(stderr,"ftruncate64 of %s failed with errno = %d\n",
		    path,errno);
	    common_close(src_fd,src_capi);
	    return -1;

	}
    }

    if (dest_fd < 0) {

	fprintf(stderr,"Open of %s failed with errno = %d\n",
		disk_pathname2,errno);
	return -1;
	    
    }



    if (fork_flag) {

	if ((pid = fork()) < 0) {  /* fork failed */
	    perror("Fork failed \n");

	}                 
	else if (pid > 0)  {               /* parents fork call */


	    fprintf(stderr,"Fork succeeded \n");
	    

	    /*
	     * Let parent sleep long enough for child to start up.
	     */

	    sleep(100);

	    return 0;
	}

	/* 
	 * Only child get here
	 */

    }



    offset = common_seek(src_fd,0,SEEK_SET,src_capi);
    aio_offset = common_seek(dest_fd,0,SEEK_SET,dest_capi);
	
    if (offset != 0) {

	fprintf(stderr,"seek to 0 of %s failed with errno = %d,\n",
		disk_pathname2,errno);
	return -1;

    }

    buf = malloc(buf_size);

    if (buf == NULL) {

	fprintf(stderr,"failed to malloc buf with errno = %d",errno);
    }

    bzero(buf,buf_size);

    remaining_size = stats.st_size;

    while (remaining_size > 0) {

	rc = common_read(src_fd,buf,buf_size,0,src_capi);

	if (rc <= 0) {
	    fprintf(stderr,"Read failed with rc = %d, but buf_size = %d with errno = %d\n",
		    rc,buf_size,errno);

	    break;
	    
	}

	remaining_size -= rc;
	
	rc = common_aio_write(dest_fd,buf,rc,aio_offset,0,dest_capi);

	if (rc < 0) {

	    break;
	} else {
	    aio_offset += remaining_size;
	}
	
    }

    if (remaining_size == 0) {

	rc = 0;
    }

    if (common_close(src_fd,src_capi)) {


	fprintf(stderr,"Close source file failed with errno = %d\n",errno);
    
    }


    if (common_close(dest_fd,dest_capi)) {


	fprintf(stderr,"Close destination file failed with errno = %d\n",errno);
    
    }

    return rc;
}


/*
 * NAME:        stat_file
 *
 * FUNCTION:    Get statistics of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int stat_file(void) 
{
    int rc = 0;
    int fd;
    char timebuf[TIMELEN+1];
    struct stat stat;

    bzero(&stat,sizeof(stat));

    fd = cusfs_open(disk_pathname,O_RDONLY,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);
	return -1;
	    
    }

    rc = cusfs_fstat(fd,&stat);

    if (!rc) {

	printf("st_ino = 0x%"PRIX64"\n",stat.st_ino);
	printf("st_mode = %o\n",stat.st_mode);
	printf("st_nlink = 0x%"PRIX64"\n",stat.st_nlink);
	printf("st_uid = %d\n",stat.st_uid);
	printf("st_gid = %d\n",stat.st_gid);
	printf("st_blksize = 0x%"PRIX64"\n",stat.st_blksize);
	printf("st_atime = %s\n",ctime_r(&(stat.st_atime),timebuf));
	printf("st_mtime = %s\n",ctime_r(&(stat.st_mtime),timebuf));
	printf("st_ctime = %s\n",ctime_r(&(stat.st_ctime),timebuf));
#if !defined(__64BIT__) && defined(_AIX)
	printf("st_size = 0x%x\n",stat.st_size);
#else
	printf("st_size = 0x%"PRIX64"\n",stat.st_size);
#endif


    }
    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }

    return rc;
}



/*
 * NAME:        link_file
 *
 * FUNCTION:    Link (hard/sym) a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int link_file(void) 
{
    int rc = 0;

    rc = cusfs_create_link(disk_pathname,disk_pathname2,mode,user_id,group_id,0);

    return rc;
}

/*
 * NAME:        chmod_file
 *
 * FUNCTION:    Change mode of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int chmod_file(void) 
{
    int rc = 0;
    int fd;


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);
	return -1;
	    
    }

    rc = cusfs_fchmod(fd,mode );

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }


    return rc;
}


/*
 * NAME:        chgrp_file
 *
 * FUNCTION:    Change grp of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int chgrp_file(void) 
{
    int rc = 0;
    int fd;


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);
	return -1;
	    
    }

    rc = cusfs_fchown(fd,-1,group_id);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }


    return rc;
}


/*
 * NAME:        chown_file
 *
 * FUNCTION:    Change owner of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int chown_file(void) 
{
    int rc = 0;
    int fd;


    fd = cusfs_open(disk_pathname,O_RDWR,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);
	return -1;
	    
    }

    rc = cusfs_fchown(fd,user_id,-1);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }

    return rc;
}


/*
 * NAME:        truncate_file
 *
 * FUNCTION:    Truncate a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int truncate_file(void) 
{
    int rc = 0;
    int fd;

    fd = cusfs_open(disk_pathname,O_RDONLY,0);

    if (fd < 0) {

	fprintf(stderr,"Open failed with errno = %d\n",errno);
	return -1;
	    
    }

    rc = cusfs_ftruncate(fd,length);

    if (cusfs_close(fd)) {


	fprintf(stderr,"Close failed with errno = %d\n",errno);
    }

    return rc;
}


/*
 * NAME:        utime_file
 *
 * FUNCTION:    Change time of a file
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int utime_file(void) 
{
    int rc = 0;
    struct utimbuf times;

    
    //?? convert time to utimbuf 

    times.actime = time_val;
    times.modtime = time_val;

    rc = cusfs_utime(disk_pathname,&times);

    return rc;
}

/*
 * NAME:        touch_file
 *
 * FUNCTION:    Change time of a file to now
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int touch_file(void) 
{
    int rc = 0;
    struct utimbuf times;
    time_t curtime;

    

    curtime = time(NULL);
    times.actime = curtime;
    times.modtime = curtime;
	
    rc = cusfs_utime(disk_pathname,&times);

    return rc;
}

/*
 * NAME:        fsck
 *
 * FUNCTION:    fsck on filesystem
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error.
 *              
 */
int fsck(void) 
{
    int rc = 0;

    rc = cusfs_fsck(device_name,0);

    return rc;
}


/*
 * NAME:        main
 *
 * FUNCTION:    
 *
 *
 * INPUTS:
 *              NONE
 *
 * RETURNS:
 *              0 success, otherwise error
 *              
 */

int main (int argc, char **argv)
{
    int rc = 0;       /* Return code                 */
   /* parse the input args & handle syntax request quickly */
    rc = parse_args( argc, argv );
    if (rc)
    {   
        return (0);
    }


    if ( hflag)
    {   
        usage();
        return (0);
    }

    if (pflag & dflag) {
	sprintf(disk_pathname,"%s%s%s",device_name,
		CUSFS_DISK_NAME_DELIMINATOR,path);
    }

    if (Pflag & Dflag) {
	sprintf(disk_pathname2,"%s%s%s",Device_name,
		CUSFS_DISK_NAME_DELIMINATOR,Path);
    }

    cusfs_init(NULL,0);

    switch(cmd_op) {
      case CUSFS_APP_CMD_CRFS:
	rc = create_fs();
	break;
      case CUSFS_APP_CMD_RMFS:
	rc = remove_fs();
	break;
      case CUSFS_APP_CMD_QUERYFS:
	rc = query_fs();
	break;
      case CUSFS_APP_CMD_STATFS:
	rc = stat_fs();
	break;
      case CUSFS_APP_CMD_MKDIR:
	rc = mk_directory();
	break;
      case CUSFS_APP_CMD_RMDIR:
	rc = rm_directory();
	break;
      case CUSFS_APP_CMD_LS:
	rc = list_files();
	break;
      case CUSFS_APP_CMD_MV:
	rc = move_file();
	break;
      case CUSFS_APP_CMD_COPY:
	rc = copy_file();
	break;
      case CUSFS_APP_CMD_AIO_COPY:
	rc = aio_copy_file();
	break;
      case CUSFS_APP_CMD_STAT:
	rc = stat_file();
	break;
      case CUSFS_APP_CMD_LN:
	rc = link_file();
	break;
      case CUSFS_APP_CMD_CHMOD:
	rc = chmod_file();
	break;
      case CUSFS_APP_CMD_CHGRP:
	rc = chgrp_file();
	break;
      case CUSFS_APP_CMD_CHOWN:
	rc = chown_file();
	break;
      case CUSFS_APP_CMD_TRUNCATE:
	rc = truncate_file();
	break;
      case CUSFS_APP_CMD_UTIME:
	rc = utime_file();
	break;
      case CUSFS_APP_CMD_TOUCH:
	rc = touch_file();
	break;
      case CUSFS_APP_CMD_RMFILE:
	rc = rm_file();
	break;
      case CUSFS_APP_CMD_FSCK:
	rc = fsck();
	break;
      default:
	fprintf(stderr,"Invalid command 0x%x\n",cmd_op);
    }

    if (rc) {
	fprintf(stderr,"Errno = %d\n",errno);
    }

    cusfs_term(NULL,0);


    return rc;

}
