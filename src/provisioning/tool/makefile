# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/provisioning/tool/makefile $
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

ALLOW_WARNINGS = yes

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTPATH}/img
LIBPATHS = -L${ROOTPATH}/img
LINKLIBS = -lprov -lcxl

PGMDIR  = ${ROOTPATH}/obj/programs
TESTDIR = ${ROOTPATH}/obj/tests

PGMS       = provtool afuinject
PROGRAMS   = $(addprefix ${PGMDIR}/, ${PGMS})



provtool_OFILES = 

all:  $(BIN_TESTS)
test: $(BIN_TESTS)

# AIX only
ifeq ($(UNAME),AIX)


CXXFLAGS  +=$(CFLAGS)
LINKLIBS  +=
GTESTS_DIR =

#Linux only
else
LINKLIBS+=

endif

include ${ROOTPATH}/config.mk
