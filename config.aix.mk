# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: config.aix.mk $
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

SHELL=/bin/bash

ship:
	${MAKE} code_pass
	@if [[ $(notdir $(PWD)) = test ]]; then ${MAKE} test; fi

docs:
	@echo "WARNING: Skipping docs step"

tests:
	${MAKE} ship
	${MAKE} test

run_fvt:
	${MAKE} tests
	${MAKE} fvt

run_unit:
	${MAKE} tests
	${MAKE} unit

#generate VPATH based on these dirs.
VPATH_DIRS=. ${ROOTPATH}/src/common ${ROOTPATH}/obj/lib/ ${ROOTPATH}/img /lib
#generate the VPATH, subbing :'s for spaces
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
VPATH += $(subst $(SPACE),:,$(VPATH_DIRS))

## output libs, objs for userdetails parsers
UD_DIR = ${ROOTPATH}/obj/modules/userdetails
UD_OBJS = ${UD_DIR}*.o ${UD_DIR}/*.so ${UD_DIR}/*.a

ARFLAGS =-X32_64 rv
LIBPATHS=-L${ROOTPATH}/img

LDFLAGS_PROGRAMS = -L${LIBPATHS} -lc ${LINKLIBS}
LDFLAGS64_PROGRAMS = -b64 ${LDFLAGS_PROGRAMS}

PGMDIR  = ${ROOTPATH}/obj/programs
TESTDIR = ${ROOTPATH}/obj/tests
GENDIR  = ${ROOTPATH}/obj/genfiles
IMGDIR  = ${ROOTPATH}/img
PKGDIR  = ${ROOTPATH}/pkg

ifdef MODULE
OBJDIR  = ${ROOTPATH}/obj/modules/${MODULE}
BEAMDIR = ${ROOTPATH}/obj/beam/${MODULE}

MEMBER = shr.o
MEMBER64 = shr_64.o

#EXTRACOMMONFLAGS += -fPIC
ifdef STRICT
        EXTRACOMMONFLAGS += -Weffc++
endif
#CUSTOMFLAGS += -D__SURELOCK_MODULE=${MODULE}
#For AIX use the following istead.
#Suppress infinite loop warnings on AIX (1500-010)
CUSTOMFLAGS += -qcpluscmt -Dinline=__inline -D_AIX -D__SURELOCK_MODULE=${MODULE} -qsuppress=1500-010
CUSTOMFLAGS64 += -q 64 ${CUSTOMFLAGS}
LIBS += $(addsuffix .so, $(addprefix lib, ${MODULE}))
LIBS64 += $(addsuffix .64so, $(addprefix lib, ${MODULE}))
AR_LIBS += $(addsuffix .a, $(addprefix lib, ${MODULE}))
EXTRAINCDIR += ${GENDIR} ${CURDIR}
#EXPFLAGS = -bE:exportfile
LDFLAGS = -bnoentry -bM:SRE $(EXPFLAGS) ${LIBPATHS} -lc
#LDFLAGS = -bnoentry -bM:SRE -bexpall ${LIBPATHS} -lc
LDFLAGS64 = -b64 ${LDFLAGS}
LIBRARIES = $(addprefix ${OBJDIR}/, ${MEMBER})
ifdef OBJS64
LIBRARIES64 = $(addprefix ${OBJDIR}/, ${MEMBER64})
endif
else
OBJDIR  = ${ROOTPATH}/obj/surelock
BEAMDIR = ${ROOTPATH}/obj/beam/surelock

CUSTOMFLAGS += -qcpluscmt -Dinline=__inline -D_AIX
CUSTOMFLAGS64 += -q64 -qcpluscmt -Dinline=__inline -D_AIX
EXTRAINCDIR += ${GENDIR} ${CURDIR}
LDFLAGS = -L${LIBPATHS} -lc ${LINKLIBS}
LDFLAGS64 = -b64 ${LDFLAGS}
#GTESTFDIR=${ROOTPATH}/src/test
endif

__internal__comma= ,
__internal__empty=
__internal__space=$(__internal__empty) $(__internal__empty)
MAKE_SPACE_LIST = $(subst $(__internal__comma),$(__internal__space),$(1))

ifdef SURELOCK_DEBUG
ifeq ($(SURELOCK_DEBUG),1)
    CUSTOMFLAGS += -DHOSTBOOT_DEBUG=1
else
ifndef MODULE
ifneq (,$(filter kernel,$(call MAKE_SPACE_LIST, $(HOSTBOOT_DEBUG))))
    CUSTOMFLAGS += -DHOSTBOOT_DEBUG=kernel
endif
else
ifneq (,$(filter $(MODULE), $(call MAKE_SPACE_LIST, $(HOSTBOOT_DEBUG))))
    CUSTOMFLAGS += -DHOSTBOOT_DEBUG=$(MODULE)
endif
endif
endif
endif

ifeq ($(USE_ADVANCED_TOOLCHAIN),yes)
	CC_RAW = cc 
	CXX_RAW = xlC_r
	CC = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CC_RAW}
	CXX = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CXX_RAW}
	LD = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/ld

else
	CC_RAW = cc 
	CXX_RAW = xlC_r
	CC = ${CC_RAW}
	CXX = ${CXX_RAW}
	LD = ld

endif

#TODO:  need to figure out if we can run beam... MCH
BEAMVER = beam-3.5.2
BEAMPATH = /afs/rch/projects/esw/beam/${BEAMVER}
BEAMCMD = i686-mcp6-jail ${BEAMPATH}/bin/beam_compile
BEAMFLAGS = \
    --beam::source=${BEAMPATH}/tcl/beam_default_parms.tcl \
    --beam::source=${ROOTPATH}/src/build/beam/compiler_c_config.tcl \
    --beam::source=${ROOTPATH}/src/build/beam/compiler_cpp_config.tcl \
    --beam::exit0 \
    -o /dev/null

#TODO: Find correct flags for surelock - copied from POWER7 Hostboot for now
COMMONFLAGS = ${EXTRACOMMONFLAGS}

ifndef NO_O3
COMMONFLAGS += -O3
endif

#add support for the rev ID header
GITREVISION:=$(shell git rev-list HEAD | wc -l | sed -e 's/^ *//')-$(shell git rev-parse --short HEAD)
CUSTOMFLAGS += -DGITREVISION='"${GITREVISION}"'


CFLAGS += ${COMMONFLAGS} -g \
	 ${CUSTOMFLAGS} \
	  ${ARCHFLAGS} \
	${INCFLAGS}
CFLAGS64 += ${COMMONFLAGS} -g \
	 ${CUSTOMFLAGS64} \
	  ${ARCHFLAGS} \
	${INCFLAGS}
ASMFLAGS = ${COMMONFLAGS} 

# TODO: avoid LD error for XCOFF64 with 32 bit objets
#CXXFLAGS += ${CFLAGS64}
CXXFLAGS += ${CFLAGS}
#LDFLAGS = --sort-common ${COMMONFLAGS}


ifdef USE_PYTHON
    TESTGEN = ${ROOTPATH}/src/usr/cxxtest/cxxtestgen.py
else
    TESTGEN = ${ROOTPATH}/src/usr/cxxtest/cxxtestgen.pl
endif

INCDIR = /usr/include /usr/include/sys ${ROOTPATH}/src/include/
_INCDIRS = ${INCDIR} ${EXTRAINCDIR}
INCFLAGS = $(addprefix -I, ${_INCDIRS} )
ASMINCFLAGS = $(addprefix $(lastword -Wa,-I), ${_INCDIRS})

OBJECTS = $(addprefix ${OBJDIR}/, ${OBJS})
OBJECTS64 = $(addprefix ${OBJDIR}/, ${OBJS64})
AR_LIBRARIES = $(addprefix ${IMGDIR}/, ${AR_LIBS})

ifdef IMGS
IMGS_ = $(addprefix ${IMGDIR}/, ${IMGS})
LIDS = $(foreach lid,$(addsuffix _LIDNUMBER, $(IMGS)),$(addprefix ${IMGDIR}/,$(addsuffix .ruhx, $($(lid)))))
IMAGES = $(addsuffix .bin, ${IMGS_}) $(addsuffix .elf, ${IMGS_}) ${LIDS}
#$(addsuffix .ruhx, ${IMGS_})
IMAGE_EXTRAS = $(addprefix ${IMGDIR}/, hbotStringFile)
endif


${OBJDIR}/%.o ${OBJDIR}/%.list : %.C
	@mkdir -p ${OBJDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${OBJDIR}/%.o ${OBJDIR}/%.list : %.c
	@mkdir -p ${OBJDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${OBJDIR}/%.64o ${OBJDIR}/%.list : %.C
	@mkdir -p ${OBJDIR}
	${CXX} -q64 -c ${CXXFLAGS64} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${OBJDIR}/%.64o ${OBJDIR}/%.list : %.c
	@mkdir -p ${OBJDIR}
	${CC} -q64 -c ${CFLAGS64} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${OBJDIR}/%.o : %.S
	@mkdir -p ${OBJDIR}
	${CC} -c ${ASMFLAGS} $< -o $@ ${ASMINCFLAGS} ${INCFLAGS}

ifdef MODULE
${OBJDIR}/${MEMBER} : ${OBJECTS}
	@mkdir -p ${IMGDIR}
	${LD} ${LDFLAGS} ${MODLIBS} -o $@ $(OBJECTS)


${OBJDIR}/${MEMBER64} : ${OBJECTS64}
	@mkdir -p ${IMGDIR}
	${LD} ${LDFLAGS64} ${MODLIBS} -o $@ $(OBJECTS64)
endif

${IMGDIR}/%.a : ${LIBRARIES} ${LIBRARIES64}
	@mkdir -p ${IMGDIR}
	$(AR) $(ARFLAGS) $@ $(LIBRARIES) $(LIBRARIES64)
	-@ ($(RANLIB) -X32_64 $@ || true) >/dev/null 2>&1

${PGMDIR}/%.o : %.c
	@mkdir -p ${PGMDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${PGMDIR}/%.o : %.C
	@mkdir -p ${PGMDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${PGMDIR}/%.64o : %.c
	@mkdir -p ${PGMDIR}
	${CC} -q64 -c ${CFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${PGMDIR}/%.64o : %.C
	@mkdir -p ${PGMDIR}
	${CXX} -q64 -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${TESTDIR}/%.o : %.c
	@mkdir -p ${TESTDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${TESTDIR}/%.o : %.C
	@mkdir -p ${TESTDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${TESTDIR}/64obj/%.o : %.c
	@mkdir -p ${TESTDIR}/64obj
	${CC} -q64 -c ${CFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)
${TESTDIR}/64obj/%.o : %.C
	@mkdir -p ${TESTDIR}/64obj
	${CXX} -q64 -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -qmakedep -MF $(@:.o=.u)

${BEAMDIR}/%.beam : %.C
	@mkdir -p ${BEAMDIR}
	${BEAMCMD} -I ${INCDIR} ${CXXFLAGS} ${BEAMFLAGS} $< \
	    --beam::complaint_file=$@ --beam::parser_file=/dev/null

${BEAMDIR}/%.beam : %.c
	@mkdir -p ${BEAMDIR}
	${BEAMCMD} -I ${INCDIR} ${CXXFLAGS} ${BEAMFLAGS} $< \
	    --beam::complaint_file=$@ --beam::parser_file=/dev/null

${BEAMDIR}/%.beam : %.S
	echo Skipping ASM file.

%.d:
	cd ${basename $@} && ${MAKE} code_pass
%.test:
	cd ${basename $@} && ${MAKE} test
%.fvt:
	cd ${basename $@} && ${MAKE} fvt
%.unit:
	cd ${basename $@} && ${MAKE} unit
%.clean:
	cd ${basename $@} && ${MAKE} clean
%.beamdir:
	cd ${basename $@} && ${MAKE} beam

#Build a C-file main, build the *_OFILES into OBJDIR, and link them together
define PROGRAMS_template
 $(1)_PGM_OFILES = $(addprefix ${PGMDIR}/, $($(notdir $(1)_OFILES))) $(addprefix $(PGMDIR)/, $(notdir $1)).o
 $(1): $$($(1)_PGM_OFILES) $(notdir $(1)).c
 ALL_OFILES += $$($(1)_PGM_OFILES) $(1)
endef
$(foreach pgm,$(PROGRAMS),$(eval $(call PROGRAMS_template,$(pgm))))

$(PROGRAMS):
	@mkdir -p ${PGMDIR}
	$(CC) $(CFLAGS) $(LDFLAGS_PROGRAMS) $($(@)_PGM_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

define PROGRAMS64_template
 $(1)_PGMS64_OFILES = $(addprefix ${PGMDIR}/, $($(notdir $(1)_OFILES))) $(addprefix $(PGMDIR)/, $(notdir $(1:64=))).64o
 $(1): $$($(1)_PGMS64_OFILES) $(notdir $(1:64=)).c
 ALL_OFILES += $$($(1)_PGMS64_OFILES) $(1)
endef

$(foreach pgm64,$(PROGRAMS64),$(eval $(call PROGRAMS64_template,$(pgm64))))

$(PROGRAMS64):
	$(CC) -q64 $(CFLAGS) $(LDFLAGS64_PROGRAMS) $($(@)_PGMS64_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C-file main, build the *_OFILES into TESTDIR, and link them together
define BIN_TESTS_template
 $(1)_BTEST_OFILES = $(addprefix ${TESTDIR}/, $($(notdir $(1)_OFILES))) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$($(1)_BTEST_OFILES) $(notdir $(1)).c
 ALL_OFILES += $$($(1)_BTEST_OFILES) $(1)
endef
$(foreach bin_test,$(BIN_TESTS),$(eval $(call BIN_TESTS_template,$(bin_test))))

$(BIN_TESTS):
	$(CC) $(CFLAGS) $(LDFLAGS_PROGRAMS) $($(@)_BTEST_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
define BIN_TESTS64_template
 $(1)_BTEST_OFILES = $(addprefix $(TESTDIR)/64obj/, $($(notdir $(1:64=)_OFILES))) $(addprefix $(TESTDIR)/64obj/, $(notdir $(1:64=))).o
 $(1): $$($(1)_BTEST_OFILES) $(notdir $(1:64=)).c
 ALL_OFILES += $$($(1)_BTEST_OFILES)  $(1)
endef

$(foreach bin_test,$(BIN_TESTS64),$(eval $(call BIN_TESTS64_template,$(bin_test))))

$(BIN_TESTS64):
	$(CC) -q64 $(CFLAGS) $(LDFLAGS64_PROGRAMS) $($(@)_BTEST_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C++ file that uses gtest, build the *_OFILES defined, and link with gtest_main

define GTESTS_template
 GTESTS_DEPS = $(TESTDIR)/gtest-all.o $(TESTDIR)/gtest_main.o
 $(1)_GTESTS_OFILES = $(addprefix $(TESTDIR)/,$($(notdir $(1))_OFILES)) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$(GTESTS_DEPS) $$($(1)_GTESTS_OFILES) $(notdir $(1)).C
 ALL_OFILES += $$($(1)_GTESTS_OFILES) $(1)
endef
$(foreach _gtest,$(GTESTS_DIR),$(eval $(call GTESTS_template,$(_gtest))))

$(GTESTS_DIR):
	$(CXX) $(CFLAGS) $(LDFLAGS_PROGRAMS) $($(@)_GTESTS_OFILES) $(GTESTS_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
define GTESTS64_template
    GTESTS64_DEPS = $(TESTDIR)/64obj/gtest-all.o $(TESTDIR)/64obj/gtest_main.o
    $(1)_GTESTS64_OFILES = $(addprefix $(TESTDIR)/64obj/,$($(notdir $(1:64=))_OFILES)) $(addprefix $(TESTDIR)/64obj/, $(notdir $(1:64=))).o
    $(1): $$(GTESTS64_DEPS) $$($(1)_GTESTS64_OFILES) $(notdir $(1:64=)).C
    ALL_OFILES += $$($(1)_GTESTS64_OFILES) $(1)
endef
$(foreach _gtest,$(GTESTS64_DIR),$(eval $(call GTESTS64_template,$(_gtest))))

$(GTESTS64_DIR):
	$(CXX) -q64 $(CFLAGS) $(LDFLAGS64_PROGRAMS) $($(@)_GTESTS64_OFILES) $(GTESTS64_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C++ file that uses gtest, build *_OFILES into TESTDIR, link with gtest
define GTESTS_NM_template
 GTEST_NM_DEPS = $(TESTDIR)/gtest-all.o
 $(1)_GTESTS_NM_OFILES = $(addprefix $(TESTDIR)/,$($(notdir $(1))_OFILES)) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$(GTEST_NM_DEPS) $$($(1)_GTESTS_NM_OFILES) $(notdir $(1)).C
 ALL_OFILES += $$($(1)_GTESTS_NM_OFILES) $(1)
endef
$(foreach _gtest_nm,$(GTESTS_NM_DIR),$(eval $(call GTESTS_NM_template,$(_gtest_nm))))

$(GTESTS_NM_DIR):
	$(CXX) $(CFLAGS) $(LDFLAGS_PROGRAMS) $($(@)_GTESTS_NM_OFILES) $(GTEST_NM_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C++ file that uses gtest, build *_OFILES into TESTDIR, link with gtest
define GTESTS64_NM_template
 GTESTS64_NM_DEPS = $(TESTDIR)/64obj/gtest-all.o
 $(1)_GTESTS64_NM_OFILES = $(addprefix $(TESTDIR)/64obj/,$($(notdir $(1:64=))_OFILES)) $(addprefix $(TESTDIR)/64obj/, $(notdir $(1:64=))).o
 $(1): $$(GTESTS64_NM_DEPS) $$($(1)_GTESTS64_NM_OFILES) $(notdir $(1:64=)).C
 ALL_OFILES += $$($(1)_GTESTS64_NM_OFILES) $(1)
endef
$(foreach _gtest64_nm,$(GTESTS64_NM_DIR),$(eval $(call GTESTS64_NM_template,$(_gtest64_nm))))

$(GTESTS64_NM_DIR):
	$(CXX) -q64 $(CFLAGS) $(LDFLAGS64_PROGRAMS) $($(@)_GTESTS64_NM_OFILES) $(GTESTS64_NM_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------

DEPS += $(addsuffix .u, ${BIN_TESTS})       \
        $(addsuffix .u, ${BIN_TESTS64})     \
        $(addsuffix .u, ${PROGRAMS})        \
        $(addsuffix .u, ${PROGRAMS64})      \
        $(addsuffix .u, ${GTESTS_DIR})      \
        $(addsuffix .u, ${GTESTS64_DIR})    \
        $(addsuffix .u, ${GTESTS_NM_DIR})   \
        $(addsuffix .u, ${GTESTS64_NM_DIR}) \
        $(OBJECTS:.o=.u)                    \
        $(OBJECTS64:.64o=.u)

BEAMOBJS = $(addprefix ${BEAMDIR}/, ${OBJS:.o=.beam})
GENTARGET = $(addprefix %/, $(1))

${PROGRAMS} ${BIN_TESTS} ${GTESTS_DIR} ${GTESTS64_DIR} $(GTESTS_NM_DIR) ${OBJECTS} ${OBJECTS64} ${LIBRARIES} ${LIBRARIES64} ${AR_LIBRARIES}: makefile
${LIBRARIES} ${LIBRARIES64} ${AR_LIBRARIES}: ${OBJECTS} ${OBJECTS64}
${EXTRA_PARTS} ${PROGRAMS} ${PROGRAMS64} : ${LIBRARIES} ${LIBRARIES64} ${AR_LIBRARIES}
$(GTESTS_DIR) $(GTESTS64_DIR) $(GTESTS_NM_DIR) $(GTESTS64_NM_DIR) $(BIN_TESTS) $(BIN_TESTS64): $(GTEST_TARGETS)

code_pass: ${SUBDIRS} ${LIBRARIES} ${LIBRARIES64} ${AR_LIBRARIES} ${EXTRA_PARTS} ${PROGRAMS} ${PROGRAMS64}
test:      ${SUBDIRS:.d=.test} ${BIN_TESTS} ${BIN_TESTS64} ${GTESTS_DIR} $(GTESTS64_DIR) $(GTESTS_NM_DIR) $(GTESTS64_NM_DIR)
fvt:       ${SUBDIRS:.d=.fvt}
unit:      ${SUBDIRS:.d=.unit}
beam:      ${SUBDIRS:.d=.beamdir} ${BEAMOBJS}

install:
	rm -rf ${PKGDIR}/install_root/*
	cd ${ROOTPATH}/src/build/install && ${MAKE}
#	@echo "WARNING: Skipping install phase"
packaging:
	cd ${ROOTPATH}/src/build/packaging && ${MAKE}
#	@echo "WARNING: Skipping packaging phase"

cscope:
	@mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f cscope.* ; \
	    find ../../ -name '*.[CHchS]' -type f -print > cscope.files; \
	    cscope -bq)

ctags:
	@mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f tags ; \
	    ctags  ../../src)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
ifneq ($(MAKECMDGOALS),unit)
ifneq ($(MAKECMDGOALS),fvt)
ifneq ($(MAKECMDGOALS),run_unit)
ifneq ($(MAKECMDGOALS),run_fvt)
ifneq ($(MAKECMDGOALS),install)
ifneq ($(MAKECMDGOALS),packaging)
    -include $(DEPS)
endif
endif
endif
endif
endif
endif
endif
endif

cleanud :
	rm -f ${UD_OBJS}

clean: cleanud ${SUBDIRS:.d=.clean}
	(rm -f ${OBJECTS} ${OBJECTS:.o=.u} ${OBJECTS:.o=.list} \
	       ${OBJECTS64} ${OBJECTS64:.o=.u} ${OBJECTS64:.o=.list}  \
	       ${OBJECTS:.o=.o.hash} ${BEAMOBJS} ${LIBRARIES} ${LIBRARIES64} ${AR_LIBRARIES} \
	       ${IMAGES} ${IMAGES:.bin=.list} ${IMAGES:.bin=.syms} \
	       ${IMAGES:.bin=.bin.modinfo} ${IMAGES:.ruhx=.lid} \
	       ${IMAGES:.ruhx=.lidhdr} ${IMAGES:.bin=_extended.bin} \
	       ${IMAGE_EXTRAS} ${TESTDIR}/* \
	       ${EXTRA_OBJS} ${_GENFILES} ${EXTRA_PARTS} ${EXTRA_CLEAN}\
	       ${PROGRAMS} ${PROGRAMS64} ${ALL_OFILES} \
	       *.a *.o *~* )

cleanall:
	rm -Rf ${ROOTPATH}/obj/*
	rm -Rf $(IMGDIR)/*
	rm -Rf $(PKGDIR)/*

ifdef IMAGES
	${MAKE} ${IMAGES} ${IMAGE_EXTRAS}
endif
