# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/cflash/makefile $
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

ROOTPATH = ../..

UNAME=$(shell uname)

USER_DIR = .
SUBDIRS  = test.d
TESTDIR  = ${ROOTPATH}/obj/tests

LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = 

PGMDIR  = ${ROOTPATH}/obj/programs
TESTDIR = ${ROOTPATH}/obj/tests

#cxlflashutil is only used on Linux
ifeq ($(UNAME),AIX)
PGMS       =

#Linux
else
PGMS       = cxlflashutil cxlfd
cxlfd_OFILES = cxlfini.o cxlfcommon.o
cxlflashutil_OFILES = cxlfcommon.o cxlfini.o

#ifeq ...
endif

PROGRAMS   = $(addprefix ${PGMDIR}/, ${PGMS})


all:  $(PROGRAMS)
test: $(BIN_TESTS)

include ${ROOTPATH}/config.mk
