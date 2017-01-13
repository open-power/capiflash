# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/master/test/makefile $
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
TESTDIR  = ${ROOTPATH}/obj/tests

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflshcom -lcflsh_block -lmclient

BTESTS=
BIN_TESTS=$(addprefix ${TESTDIR}/, ${BTESTS})

GTESTS = run_master_fvt
GTESTS_DIR = $(addprefix $(TESTDIR)/, $(GTESTS))

# AIX only
ifeq ($(UNAME),AIX)
LINKLIBS+=-lpthreads

GTESTS64     = $(addsuffix 64, $(GTESTS))
GTESTS64_DIR = $(addprefix $(TESTDIR)/, $(GTESTS64))

#Linux only
else
LINKLIBS+=-lpthread

endif

run_master_fvt_OFILES = fvt_master.o mc_test.o mc_test2.o \
			mc_test_engine.o mc_test_error.o \
			mc_test_io.o mc_test_util.o

DEPS=$(addprefix $(TESTDIR)/, $(run_master_fvt_OFILES:.o=.dep))

CFLAGS += \
	-g \
   -D__FVT__\
   -I$(ROOTPATH)/src/master \
   -I$(ROOTPATH)/${GTESTINC}
CXXFLAGS+=$(CFLAGS)

#VPATH += ${ROOTPATH}/src/master

include ${ROOTPATH}/config.mk

include $(ROOTPATH)/src/test/framework/gtest.objtests.mk

unit:
	-@if [[ -e /dev/cxl ]]; then \
       $(TESTDIR)/run_master_fvt  --gtest_output=xml:$(TESTDIR)/master_fvt_results.xml; \
    else \
      echo "SKIPPING run_master_fvt"; \
    fi
