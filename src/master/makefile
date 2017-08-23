# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/master/makefile $
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
SUBDIRS = test.d

#if BLOCK_FILEMODE is enabled, then tell the block code as much
#pass down as a #define to the underlying code
ifdef BLOCK_FILEMODE_ENABLED
ifeq ($(BLOCK_FILEMODE_ENABLED),1)
    CUSTOMFLAGS += -D_FILEMODE_
endif
endif

EXPFLAGS = -bexpall

export LD_LIBRARY_PATH+=${ROOTPATH}/lib

LIBPATHS = -L${ROOTPATH}/lib

PGMDIR  = ${ROOTPATH}/obj/programs
TESTDIR = ${ROOTPATH}/obj/tests

OBJS   = mclient.o
OBJS64 = mclient.64o
MODULE = mclient

PGMS       = mserv
PROGRAMS   = $(addprefix ${PGMDIR}/, ${PGMS})
#PROGRAMS64 = $(addprefix ${PGMDIR}/, ${PGMS})

BTESTS    = mclient_test
BIN_TESTS = $(addprefix ${TESTDIR}/, ${BTESTS})

ifeq ($(UNAME),AIX)
LINKLIBS +=-lpthreads -lmclient
else
LINKLIBS +=-lpthread  -lmclient -lrt
endif

mserv_OFILES = block_alloc.o
DEPS=$(addprefix $(PGMDIR)/, $(mserv_OFILES:.o=.dep))

EXTRAINCDIR =

include ${ROOTPATH}/config.mk
