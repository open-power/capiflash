# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/block/test/makefile $
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

ALLOW_WARNINGS = yes

ROOTPATH = ../../..
SUBDIRS  =
VPATH    = ${ROOTPATH}/src/common
LCFLAGS  = -g -D__FVT__ -I$(ROOTPATH)/src/block -I$(GTESTINC)
GTESTS   = run_block_fvt

run_block_fvt_OFILES = fvt_block.o blk_api_tst.o cflash_tools_user.o
DEPS = $(addprefix $(TESTDIR)/, $(run_block_fvt_OFILES:.o=.dep))

include ${ROOTPATH}/config.mk
include $(ROOTPATH)/src/test/framework/gtest.objtests.mk

LINKLIBS += -lcflsh_block$(SOVER) -lm

ifeq ($(BLOCK_FILEMODE_ENABLED),1)
GFILTER="--gtest_filter=*_FM_*"
endif
ifeq ($(BLOCK_MC_ENABLED), 1)
GFILTER="--gtest_filter=*_UMC_*"
endif

unit:
ifneq ($(FVT_DEV),)
	-$(TESTDIR)/run_block_fvt$(BITS) ${GFILTER} --gtest_output=xml:$(TESTDIR)/block_fvt_results.xml;
else
	@echo "FVT_DEV is unset for BLOCK_FILEMODE_ENABLED=1"
endif
