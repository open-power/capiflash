# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/test/ffdc/makefile $
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

#this application originates in surelock-afu/bringup/src and must be shipped for FFDC gathering

.NOTPARALLEL:

UNAME=$(shell uname)

# Where to find user code.(Relative path(from the makefile) preferred for portability)
ROOTPATH = ../../..
USER_DIR = .
SUBDIRS  =
TESTDIR  = ${ROOTPATH}/obj/tests
EXPFLAGS = -bexpall

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lcflshcom -lcflsh_block -larkdb -larkalloc

ifneq ($(UNAME),AIX)

ifneq ($(TARGET_PLATFORM),x86_64)

BTESTS    = cxl_afu_status cxl_afu_dump cxl_afu_inject
BIN_TESTS = $(addprefix ${TESTDIR}/, ${BTESTS})
LINKLIBS += -lpthread -ludev -lafu -lcxl
OBJS      = afu.o capi_dev_nodes.o

endif

endif

CFLAGS = -D__FVT__ -Wno-unused-result \
         -I$(ROOTPATH)/src/common     \
         -I$(ROOTPATH)/src/test/ffdc

VPATH += ${ROOTPATH}/src/test/ffdc

include ${ROOTPATH}/config.mk
