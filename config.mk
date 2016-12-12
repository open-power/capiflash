# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: config.mk $
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

# Select default values if not set
# using customrc.p8elblkkermc config file
#-----------------------------------------
# export MAKECMD=make
# export CUSTOMFLAGS="-mcpu=power8 -mtune=power8"
# export BLOCK_FILEMODE_ENABLED=0
# export BLOCK_MC_ENABLED=1
# export BLOCK_KERNEL_MC_ENABLED=1
# export TARGET_PLATFORM="PPC64EL"
#-----------------------------------------
#if no target arch is set, select current platform for convenience.
ifndef TARGET_PLATFORM
    UNAME=$(shell uname -m)
    ifeq ( $(UNAME),ppc64le)
       TARGET_PLATFORM=PPC64EL
    else
       TARGET_PLATFORM=PPC64BE
    endif
endif
#If the target architecture is set, pass an architecture
#down as a #define to the underlying code
export ARCHFLAGS += -DTARGET_ARCH_${TARGET_PLATFORM}

# ADV_TOOLCHAIN_PATH
AT71PATH=/opt/at7.1
AT90PATH=/opt/at9.0-2-rc2
ifneq ($(AT71PATH),)
    export ADV_TOOLCHAIN_PATH=$AT71PATH
else
 export ADV_TOOLCHAIN_PATH=$AT90PATH
endif
ifeq ( $(ADV_TOOLCHAIN_PATH),)
    USE_ADVANCED_TOOLCHAIN=no
endif

ifeq ( $(USE_ADVANCED_TOOLCHAIN),yes)
   export CUSTOMFLAGS="-mcpu=power8 -mtune=power8"
   export PATH=${ADV_TOOLCHAIN_PATH}/bin:${ADV_TOOLCHAIN_PATH}/sbin:${PATH}
else
   USE_ADVANCED_TOOLCHAIN=no
   export CUSTOMFLAGS="-mcpu=power8"
endif

#
ifndef BLOCK_FILEMODE_ENABLED
   export BLOCK_FILEMODE_ENABLED=0
endif

ifndef BLOCK_MC_ENABLED
   export BLOCK_MC_ENABLED=1
endif

ifndef BLOCK_KERNEL_MC_ENABLED
   export BLOCK_KERNEL_MC_ENABLED=1
endif

#Determine if this a linux or AIX system

UNAME=$(shell uname)
ifeq ($(UNAME),AIX)
include ${ROOTPATH}/config.aix.mk
else
ifeq ($(UNAME),Darwin)
include ${ROOTPATH}/config.mac.mk
else
include ${ROOTPATH}/config.linux.mk
endif
endif
