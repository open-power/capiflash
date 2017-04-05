# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/makefile $
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

ROOTPATH = ..

.NOTPARALLEL:

#common is built first to satisfy dependencies elsewhere. We likely need to support
#different makefile passes for exported libs vs regular programs, libs, and shared libs.
SUBDIRS = lib.d common.d afu.d block.d usfs.d kv.d test.d
ifneq ($(TARGET_PLATFORM),x86_64)
SUBDIRS += build.d cflash.d provisioning.d
endif

BASE_OBJECTS = 

BASE_MODULES = trace errl devicefw scom xscom initservice taskargs \
               pnor i2c  fsi vfs

EXTENDED_MODULES = targeting ecmddatabuffer hwpf fapi hwp plat	\
                   extinitsvc istepdisp hwas fsiscom

DIRECT_BOOT_MODULES = example
RUNTIME_MODULES =
TESTCASE_MODULES = cxxtest testerrl testdevicefw testsyslib \
                   testscom testxscom testtargeting testinitservice testkernel \
                   testhwpf testecmddatabuffer initsvctasktest2 testcxxtest \
                   testpnor testi2c testfsi testvfs testhwas

RELOCATABLE_IMAGE_LDFLAGS = -pie --export-dynamic

include ${ROOTPATH}/config.mk
