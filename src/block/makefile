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
ROOTPATH = ../..

CFLAGS +=

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

ifeq ($(BLOCK_KERNEL_MC_ENABLED),1)

CUSTOMFLAGS += -D_KERNEL_MASTER_CONTXT 

OBJS = cflash_scsi_user.o cflash_tools_user.o cflash_block.o cflash_block_int.o cflash_block_kern_mc.o cflash_block_sisl.o

OBJS64 = cflash_scsi_user.64o cflash_tools_user.64o cflash_block.64o cflash_block_int.64o cflash_block_kern_mc.64o cflash_block_sisl.64o

else

OBJS = cflash_scsi_user.o cflash_tools_user.o cflash_block.o cflash_block_int.o cflash_block_linux.o cflash_block_sisl.o

OBJS64 = cflash_scsi_user.64o cflash_tools_user.64o cflash_block.64o cflash_block_int.64o cflash_block_linux.64o cflash_block_sisl.64o

endif

UNAME=$(shell uname)
ifeq ($(UNAME),AIX)
MODLIBS = -lpthreads
MODULE_LINKLIBS = ${MODLIBS} 
EXPFLAGS = -bE:exportfile
else
ifeq ($(BLOCK_KERNEL_MC_ENABLED),1)
MODLIBS = -lpthread -ludev
else
MODLIBS = -lpthread
endif
MODULE_LINKLIBS = ${MODLIBS} -Wl,--version-script=libcflash_exportmap
endif

LINKLIBS = -l${MODULE}
LIBPATHS = -L${ROOTPATH}/img

SUBDIRS =  test.d

include ${ROOTPATH}/config.mk
