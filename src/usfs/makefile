# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/usfs/makefile $
#
# IBM Data Engine for NoSQL - Power Systems Edition User Library Project
#
# Contributors Listed Below - COPYRIGHT 2014,2015
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
# IBM_PROLOG_END_TAG

UNAME=$(shell uname)

ROOTPATH = ../..

#if BLOCK_FILEMODE is enabled, then tell the userspace code as much
#pass down as a #define to the underlying code
ifdef BLOCK_FILEMODE_ENABLED
ifeq ($(BLOCK_FILEMODE_ENABLED),1)
    CUSTOMFLAGS += -DBLOCK_FILEMODE_ENABLED
endif
endif

# Incase user want to use semop based locking mechanism ;
# _MASTER_LOCK is required to define here
#CUSTOMFLAGS += -D_MASTER_LOCK

# Incase user want to use unix socket based locking mechanism;
# _MASTER_LOCK_CLIENT and  __MASTER_LOCK both is required to define here
#CUSTOMFLAGS += -D_MASTER_LOCK -D_MASTER_LOCK_CLIENT

MODULE = cflsh_usfs
OBJS   = cflsh_usfs.o cflsh_usfs_utils.o cflsh_usfs_disk.o cflsh_usfs_inode.o cflsh_usfs_client.o cflsh_usfs_wrapper.o
PGMS   = cusfs_serv

ifeq ($(UNAME),AIX)
MODLIBS    = -lcflsh_block
LINKLIBS  += -lcflsh_block -l${MODULE}
EXPFLAGS   = -bE:libcflsh_usfs.exp
else
MODLIBS    = -lcflsh_block$(SOVER) -Wl,--version-script=libcflsh_usfs_exportmap -ldl
LINKLIBS  += -lcflsh_block$(SOVER) -l${MODULE}$(SOVER) -ldl -lrt
LCFLAGS    =
endif

include ${ROOTPATH}/config.mk

