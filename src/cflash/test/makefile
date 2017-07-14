# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/cflash/test/makefile $
#
# IBM Data Engine for NoSQL - Power Systems Edition User Library Project
#
# Contributors Listed Below - COPYRIGHT 2015
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

ALLOW_WARNINGS = yes

ROOTPATH = ../../..
SUBDIRS  =
GTESTS   = run_cflash_fvt

run_cflash_fvt_OFILES = \
  fvt_cflash.o cflash_test_ioctl_io.o cflash_test_engine.o cflash_test_error.o \
  cflash_test_io.o cflash_test_util.o cflash_test2.o \
  cflash_test_ioctl.o cflash_test_error2.o cflash_test_excp.o manual.o \
  119.o 174.o 174_175.o 188.o 191.o 192.o 196.o 197.o 198.o 209.o 210.o 211.o 215.o \
  216.o 203.o

LCFLAGS = -g -D__FVT__ -I$(GTESTINC)

include ${ROOTPATH}/config.mk
include $(ROOTPATH)/src/test/framework/gtest.objtests.mk

DEPS=$(addprefix $(TESTDIR)/, $(run_cflash_fvt_OFILES:.o=.dep))

unit:
	-@if [[ -e /dev/cxl ]]; then \
       LD_LIBRARY_PATH=${ROOTPATH}/img $(TESTDIR)/run_cflash_fvt --gtest_output=xml:$(TESTDIR)/cflash_fvt_results.xml; \
    else \
      echo "SKIPPING run_cflash_fvt"; \
    fi
