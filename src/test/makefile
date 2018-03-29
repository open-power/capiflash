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

UNAME    = $(shell uname)
ROOTPATH = ../..
EXPFLAGS = -bexpall
ALLOW_WARNINGS = yes

GTESTS     = run_fvt run_fvt_scripts

LCFLAGS = \
   -D__FVT__ \
   -I$(ROOTPATH)/src/kv \
   -I$(ROOTPATH)/src/kv/test \
   -I$(ROOTPATH)/src/block \
   -I$(ROOTPATH)/src/block/test \
   -I$(ROOTPATH)/src/common \
   -I${GTESTINC}

run_fvt_OFILES = \
 blk_api_tst.o fvt_block.o \
 fvt_kv_utils.o kv_utils_db.o fvt_kv_utils_async_cb.o fvt_kv_utils_sync_pth.o  \
 fvt_kv_utils_ark_io.o fvt_trace.o \
 fvt_kv_tst_simple.o fvt_kv_tst_scenario.o fvt_kv_tst_sync_pth.o \
 fvt_kv_tst_async_cb.o fvt_kv_tst_errors.o fvt_kv_tst_sync_async.o \

run_fvt_scripts_OFILES = fvt_utils_scripts_tst.o

VPATH += \
 ${ROOTPATH}/src/kv \
 ${ROOTPATH}/src/kv/test \
 ${ROOTPATH}/src/block/test \

include ${ROOTPATH}/config.mk
include $(ROOTPATH)/src/test/framework/gtest.objtests.mk

LINKLIBS += -lcflshcom$(SOVER) -lcflsh_block$(SOVER) -larkdb$(SOVER) -larkalloc$(SOVER)

fvt:
	@$(TESTDIR)/run_fvt$(BITS)
