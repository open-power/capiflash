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

.NOTPARALLEL:

UNAME=$(shell uname)

# Where to find user code.(Relative path(from the makefile) preferred for portability)
ROOTPATH = ../..
USER_DIR = .
SUBDIRS  = usfs.d ffdc.d vpd.d
TESTDIR  = ${ROOTPATH}/obj/tests
EXPFLAGS = -bexpall

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflshcom -lcflsh_block -larkdb -larkalloc


BTESTS   = asyncstress asyncstress2

ifneq ($(TARGET_PLATFORM),x86_64)
BTESTS   += transport_test blk_test pvtestauto xlate pblkread
OBJS      = afu.o capi_dev_nodes.o
OBJS64    = afu.64o capi_dev_nodes.64o
MODULE    = afu
LINKLIBS += -lafu
endif

BIN_TESTS=$(addprefix ${TESTDIR}/, ${BTESTS})

GTESTS = run_fvt run_fvt_utils_scripts
GTESTS_DIR = $(addprefix $(TESTDIR)/, $(GTESTS))

ifeq ($(UNAME),AIX) # AIX only
LINKLIBS+=-lpthreads

BTESTS64     = $(addsuffix 64, ${BTESTS})
BIN_TESTS64  = $(addprefix ${TESTDIR}/, ${BTESTS64})
GTESTS64     = $(addsuffix 64, ${GTESTS})
GTESTS64_DIR = $(addprefix $(TESTDIR)/, $(GTESTS64))
BITS         = 64

else #Linux only
LINKLIBS+=-lpthread -ludev
BTESTS  +=flashgt_temp
endif

run_fvt_OFILES = \
 blk_api_tst.o fvt_block.o \
 fvt_kv_utils.o kv_utils_db.o fvt_kv_utils_async_cb.o fvt_kv_utils_sync_pth.o  \
 fvt_kv_utils_ark_io.o fvt_trace.o \
 fvt_kv_tst_simple.o fvt_kv_tst_scenario.o fvt_kv_tst_sync_pth.o \
 fvt_kv_tst_async_cb.o fvt_kv_tst_errors.o fvt_kv_tst_sync_async.o \
 fvt_utils_scripts_tst.o

run_fvt_utils_scripts_OFILES = \
 fvt_utils_scripts_tst.o

CFLAGS += \
   -D__FVT__ -Wno-unused-result\
   -I$(ROOTPATH)/src/kv \
   -I$(ROOTPATH)/src/kv/test \
   -I$(ROOTPATH)/src/block \
   -I$(ROOTPATH)/src/common \
   -I${GTESTINC} 
CXXFLAGS+=$(CFLAGS)

VPATH += \
 ${ROOTPATH}/src/kv \
 ${ROOTPATH}/src/kv/test \
 ${ROOTPATH}/src/block/test \

include ${ROOTPATH}/config.mk
include $(ROOTPATH)/src/test/framework/gtest.objtests.mk

fvt:
	@-if [[ $(UNAME) != "AIX" ]]; then \
       $(TESTDIR)/run_fvt; fi
