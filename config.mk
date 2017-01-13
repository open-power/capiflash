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

#select a default target architecture
#if no target arch is set, select PPC64BE for convenience.
ifndef TARGET_PLATFORM
  UNAME=$(shell uname -m)
  ifeq ($(UNAME),x86_64)
    TARGET_PLATFORM=x86_64
  else
    ifeq ($(UNAME),ppc64le)
      TARGET_PLATFORM=PPC64EL
    else
      TARGET_PLATFORM=PPC64BE
    endif
  endif
endif

#If the target architecture is set, pass an architecture
#down as a #define to the underlying code
export ARCHFLAGS += -DTARGET_ARCH_${TARGET_PLATFORM}

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
