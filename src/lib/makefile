# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/lib/makefile $
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

all: 

OBJS = 

SUBDIRS = arkalloc.d

code_pass: ${IMGDIR}/libcxl.so

${IMGDIR}/libcxl.so:
	@echo "WARNING: TODO: Need to pick up libcxl.so from external toolchain dirs. This is temporary."
	mkdir -p ${IMGDIR}
	@if [ "${TARGET_PLATFORM}" = "PPC64EL" ]; then \
	echo "Copying little-endian libcxl"; \
	cp libcxl-le/libcxl.so ${IMGDIR}; \
	else \
	echo "Copying big-endian libcxl"; \
	cp libcxl-be/libcxl.so ${IMGDIR}; \
	fi
	@echo "Done patching"


include ${ROOTPATH}/config.mk

