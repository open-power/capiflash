//  IBM_PROLOG_BEGIN_TAG
//  This is an automatically generated prolog.
//
//  $Source: src/include/fuse_common_compat.h $
//
//  IBM CONFIDENTIAL
//
//  COPYRIGHT International Business Machines Corp. 2017
//
//  p1
//
//  Object Code Only (OCO) source materials
//  Licensed Internal Code Source Materials
//  IBM Surelock Licensed Internal Code
//
//  The source code for this program is not published or other-
//  wise divested of its trade secrets, irrespective of what has
//  been deposited with the U.S. Copyright Office.
//
//  Origin: 30
//
//  IBM_PROLOG_END
/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.
*/

/* these definitions provide source compatibility to prior versions.
   Do not include this file directly! */

struct fuse_file_info_compat {
	int flags;
	unsigned long fh;
	int writepage;
	unsigned int direct_io : 1;
	unsigned int keep_cache : 1;
};

int fuse_mount_compat25(const char *mountpoint, struct fuse_args *args);

int fuse_mount_compat22(const char *mountpoint, const char *opts);

int fuse_mount_compat1(const char *mountpoint, const char *args[]);

void fuse_unmount_compat22(const char *mountpoint);
