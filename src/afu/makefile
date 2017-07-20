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

ALLOW_WARNINGS = yes

ROOTPATH = ../..
EXPFLAGS = -bexpall

ifneq ($(TARGET_PLATFORM),x86_64)
BTESTS = transport_test blk_test blk_eras pvtestauto xlate pblkread
endif

LCFLAGS = -D__FVT__ \
          -I$(ROOTPATH)/src/afu \
          -I$(ROOTPATH)/src/test

ifneq ($(UNAME),AIX)
ifneq ($(TARGET_PLATFORM),x86_64)
MODULE    = afu
OBJS      = afu.o capi_dev_nodes.o
BTESTS   += cxl_afu_dump cxl_afu_status cxl_afu_inject flashgt_vpd_access flashgt_temp
endif
endif

VPATH += ${ROOTPATH}/src/test

include ${ROOTPATH}/config.mk

LINKLIBS += -lcflshcom$(SOVER) -lcflsh_block$(SOVER)
ifneq ($(UNAME),AIX)
LINKLIBS += -lafu$(SOVER) -lcxl
endif