# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/block/makefile $
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

UNAME    = $(shell uname)
ROOTPATH = ../..

#enable file mode by default, if the env var is not set.
#to disable file mode, set BLOCK_FILEMODE_ENABLED=0 in surelock-sw/customrc
#or export BLOCK_FILEMODE_ENABLED=0 prior to building
ifndef BLOCK_FILEMODE_ENABLED
    BLOCK_FILEMODE_ENABLED=1
endif

#if BLOCK_FILEMODE is enabled, then tell the block code as much
#pass down as a #define to the underlying code
ifdef BLOCK_FILEMODE_ENABLED
ifeq ($(BLOCK_FILEMODE_ENABLED),1)
    CUSTOMFLAGS += -DBLOCK_FILEMODE_ENABLED 
endif
endif

#if BLOCK_MC is enabled, then tell the block code as much
#pass down as a #define to the underlying code
ifdef BLOCK_MC_ENABLED
ifeq ($(BLOCK_MC_ENABLED),1)
    CUSTOMFLAGS += -D_MASTER_CONTXT
endif
endif

MODULE = cflsh_block

OBJS = cflash_scsi_user.o cflash_tools_user.o cflash_block.o cflash_block_int.o\
       cflash_block_sisl.o cflash_block_cgroup.o cflash_block_raid0.o\
       cflash_block_eras_client.o cflash_block_eras_serv.o

ifeq ($(BLOCK_KERNEL_MC_ENABLED),1)
CUSTOMFLAGS += -D_KERNEL_MASTER_CONTXT
OBJS += cflash_block_kern_mc.o
else
OBJS += cflash_block_linux.o
endif

ifeq ($(UNAME),AIX)
EXPFLAGS = -bE:exportfile
else
LCFLAGS         = -Wno-format-security
MODULE_LINKLIBS = -Wl,--version-script=libcflash_exportmap
endif

LINKLIBS = -l${MODULE}

SUBDIRS =  bin.d test.d

include ${ROOTPATH}/config.mk
