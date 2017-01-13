# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/test/makefile $
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

# Where to find user code.(Relative path(from the makefile) preferred for portability)
ROOTPATH = ../../..
USER_DIR = .

TESTDIR  = ${ROOTPATH}/obj/tests

EXPFLAGS = -bexpall

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflsh_block -lcflsh_usfs


BTESTS   =cusfs
BIN_TESTS=$(addprefix ${TESTDIR}/, ${BTESTS})


ifeq ($(UNAME),AIX) # AIX only
LINKLIBS+=-lpthreads

BTESTS64     = $(addsuffix 64, ${BTESTS})
BIN_TESTS64  = $(addprefix ${TESTDIR}/, ${BTESTS64})
BITS             = 64

else #Linux only
LINKLIBS+=-lpthread -ludev
endif

CFLAGS += \
   -D__FVT__\
   -I$(ROOTPATH)/src/block \
   -I$(ROOTPATH)/src/common
CXXFLAGS+=$(CFLAGS)

VPATH += \
 ${ROOTPATH}/src/kv \
 ${ROOTPATH}/src/kv/test \
 ${ROOTPATH}/src/block/test \

include ${ROOTPATH}/config.mk

