# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/afu/makefile $
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

.NOTPARALLEL:

UNAME=$(shell uname)

# Where to find user code.(Relative path(from the makefile) preferred for portability)
ROOTPATH = ../..
USER_DIR = .
TESTDIR  = ${ROOTPATH}/obj/tests
EXPFLAGS = -bexpall

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflshcom -lcflsh_block

ifneq ($(TARGET_PLATFORM),x86_64)
BTESTS  += transport_test blk_test pvtestauto xlate pblkread
endif

ifeq ($(UNAME),AIX) # AIX only
BTESTS64     = $(addsuffix 64, ${BTESTS})
BIN_TESTS64  = $(addprefix ${TESTDIR}/, ${BTESTS64})
BITS         = 64
LINKLIBS    += -lpthreads
else
ifneq ($(TARGET_PLATFORM),x86_64)
BTESTS   += cxl_afu_dump cxl_afu_status cxl_afu_inject flashgt_vpd_access flashgt_temp
OBJS      = afu.o capi_dev_nodes.o
OBJS64    = afu.64o capi_dev_nodes.64o
MODULE    = afu
LINKLIBS += -lpthread -ludev -lafu -lcxl
endif
endif

BIN_TESTS=$(addprefix ${TESTDIR}/, ${BTESTS})

CFLAGS += \
   -D__FVT__ -Wno-unused-result \
   -I$(ROOTPATH)/src/afu \
   -I$(ROOTPATH)/src/test
CXXFLAGS+=$(CFLAGS)

VPATH += ${ROOTPATH}/src/test

include ${ROOTPATH}/config.mk
