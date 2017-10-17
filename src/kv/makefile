# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/kv/makefile $
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

ROOTPATH = ../..
SUBDIRS  = bin.d test.d

OBJS   = ut.o cl.o ea.o ll.o bv.o iv.o si.o vi.o bl.o bt.o arkdb.o am.o  \
         arp.o arp_set.o arp_get.o arp_del.o arp_exist.o tag.o ea_mod.o queue.o \
         hash.o tg.o ht.o kv_inject.o

MODULE          = arkdb
EXPFLAGS        = -bexpall
MODLIBS         = -lcflsh_block$(SOVER) -lcflshcom$(SOVER) -larkalloc$(SOVER)

include ${ROOTPATH}/config.mk
