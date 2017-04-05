# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/block/bin/makefile $
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

ROOTPATH = ../../..
USER_DIR = .
SUBDIRS  =
BITS =
TESTDIR  = ${ROOTPATH}/obj/tests

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflsh_block -lm

BTESTS=blockio blocklistio blockplistio blocksio blockr0io blockr0sio asyncstress
BIN_TESTS=$(addprefix ${TESTDIR}/, ${BTESTS})

# AIX only
ifeq ($(UNAME),AIX)
LINKLIBS +=-lpthreads
BITS=64

BTESTS64    = $(addsuffix 64, ${BTESTS})
BIN_TESTS64 = $(addprefix ${TESTDIR}/, ${BTESTS64})

#Linux only
else
LINKLIBS+=-lpthread -ludev
endif

VPATH += \
 ${ROOTPATH}/src/common

CFLAGS += \
   -g -D__FVT__ -Wno-unused-result \
   -I$(ROOTPATH)/src/block \
   -I$(ROOTPATH)/src/include

CXXFLAGS+=$(CFLAGS)

include ${ROOTPATH}/config.mk
