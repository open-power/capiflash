# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/kv/bin/makefile $
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
SUBDIRS  =

#test code != production code, so allow warnings here.
ALLOW_WARNINGS = yes

LINKLIBS = -lcflshcom -lcflsh_block -larkdb -larkalloc
VPATH   += $(ROOTPATH)/src/kv

BTESTS   = _tst_ark _tst_persist test_perf \
           res_pers run_kv_async run_kv_sync run_kv_async_multi \
           run_kv_benchmark run_kv_persist kv_perf

_tst_ark_OFILES     = tst_ark.o
_tst_persist_OFILES = tst_persist.o

LCFLAGS = \
   -D__FVT__ -D__STDC_FORMAT_MACROS \
   -I$(ROOTPATH)/src/kv \
   -I$(ROOTPATH)/src/kv/test

include ${ROOTPATH}/config.mk
