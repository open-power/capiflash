# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: config.linux.mk $
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
	${MAKE} -j10 dep
	${MAKE} -j10 code_pass
	${MAKE} -j10 test

build:
	${MAKE} ship
	${MAKE} docs
	${MAKE} packaging

tests:
	${MAKE} ship

run_fvt:
	${MAKE} ship
	${MAKE} fvt

run_unit:
	${MAKE} ship
	${MAKE} unit


ifneq ($(wildcard /usr/src/gtest),)
  GTESTDIR=/usr/src/gtest
  GTESTINC=/usr/include
else ifneq ($(wildcard ${ROOTPATH}/src/test/framework/gtest-1.7.0),)
  GTESTDIR=${ROOTPATH}/src/test/framework/gtest-1.7.0
  GTESTINC=${GTESTDIR}/include
else ifneq ($(wildcard ${ROOTPATH}/src/test/framework/googletest/googletest),)
  GTESTDIR=${ROOTPATH}/src/test/framework/googletest/googletest
  GTESTINC=${GTESTDIR}/include
endif

#needed to provide linker rpath hints for installed code
DEFAULT_LIB_INSTALL_PATH = /opt/ibm/capikv/lib
#generate VPATH based on these dirs.
VPATH_DIRS=. ${ROOTPATH}/src/common ${ROOTPATH}/obj/lib/ ${ROOTPATH}/img
#generate the VPATH, subbing :'s for spaces
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
VPATH += $(subst $(SPACE),:,$(VPATH_DIRS))

## output libs, objs for userdetails parsers
UD_DIR = ${ROOTPATH}/obj/modules/userdetails
UD_OBJS = ${UD_DIR}*.o ${UD_DIR}/*.so ${UD_DIR}/*.a

PGMDIR  = ${ROOTPATH}/obj/programs
TESTDIR = ${ROOTPATH}/obj/tests
GENDIR  = ${ROOTPATH}/obj/genfiles
IMGDIR  = ${ROOTPATH}/img
PKGDIR  = ${ROOTPATH}/pkg

ifdef MODULE
OBJDIR  = ${ROOTPATH}/obj/modules/${MODULE}
BEAMDIR = ${ROOTPATH}/obj/beam/${MODULE}

EXTRACOMMONFLAGS += -fPIC
ifdef STRICT
        EXTRACOMMONFLAGS += -Weffc++
endif
CUSTOMFLAGS += -D__SURELOCK_MODULE=${MODULE}
#For AIX use the following istead.
#CUSTOMFLAGS += -D_AIX -D__SURELOCK_MODULE=${MODULE}
LIBS += $(addsuffix .so, $(addprefix lib, ${MODULE}))
EXTRAINCDIR += ${GENDIR} ${CURDIR}
else
OBJDIR  = ${ROOTPATH}/obj/surelock
BEAMDIR = ${ROOTPATH}/obj/beam/surelock
EXTRAINCDIR += ${GENDIR} ${CURDIR}
endif

__internal__comma= ,
__internal__empty=
__internal__space=$(__internal__empty) $(__internal__empty)
MAKE_SPACE_LIST = $(subst $(__internal__comma),$(__internal__space),$(1))


ifdef SURELOCK_DEBUG
ifeq ($(SURELOCK_DEBUG),1)
    CUSTOMFLAGS += -DSURELOCK_DEBUG=1
endif
endif


ifeq ($(USE_ADVANCED_TOOLCHAIN),yes)
	CC_RAW = gcc 
	CXX_RAW = g++ 
	CC = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CC_RAW}
	CXX = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CXX_RAW}
	LD = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/gcc
	OBJDUMP = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/objdump
	#this line is very specifically-written to explicitly-place the ATx.x stuff at the FRONT of the VPATH dirs.
	#this is a REQUIREMENT of the advanced toolchain for linux.
	VPATH_DIRS:= ${ADV_TOOLCHAIN_PATH}/lib64 ${VPATH_DIRS}
	#see the ld flags below (search for rpath). This puts the atx.x stuff on the front
	#which is REQUIRED by the toolchain.
	CFLAGS += ${COMMONFLAGS} -Wall ${CUSTOMFLAGS} ${ARCHFLAGS} ${INCFLAGS}
	LDFLAGS = ${COMMONFLAGS} -Wl,-rpath,${ADV_TOOLCHAIN_PATH}/lib64:$(DEFAULT_LIB_INSTALL_PATH)
else
	CC_RAW = gcc 
	CXX_RAW = g++ 
	CC = ${CC_RAW}
	CXX = ${CXX_RAW}
	LD = gcc
	OBJDUMP = objdump
	CFLAGS += ${COMMONFLAGS} -Wall ${CUSTOMFLAGS}  ${ARCHFLAGS} ${INCFLAGS}
	LDFLAGS = ${COMMONFLAGS} -Wl,-rpath,$(DEFAULT_LIB_INSTALL_PATH)
endif

#TODO: Find correct flags for surelock
#moved custom P8 tunings to customrc file.
COMMONFLAGS = ${EXTRACOMMONFLAGS}

ifndef NO_O3
COMMONFLAGS += -O3
endif

#add support for the rev ID header
GITREVISION:=$(shell git rev-list HEAD 2>/dev/null| wc -l)-$(shell git rev-parse --short HEAD 2>/dev/null)
CUSTOMFLAGS += -DGITREVISION='"${GITREVISION}"'

#if ALLOW_WARNINGS is NOT defined, we assume we are compiling production code
#as such, we adhere to strict compile flags. If this is defined then we warn
#but allow the compile to continue.
ifndef ALLOW_WARNINGS
	CFLAGS += -Werror
	CXXFLAGS += -Werror
endif

ifdef COVERAGE
COMMONFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -lgcov
LINKLIBS += -lgcov
endif

ASMFLAGS = ${COMMONFLAGS}
CXXFLAGS += ${CFLAGS} -fno-rtti -fno-exceptions -Wall
#RPATH order is important here. the prepend var lets us throw the ATxx stuff in first if needed.


INCDIR = ${ROOTPATH}/src/include/
_INCDIRS = ${INCDIR} ${EXTRAINCDIR}
INCFLAGS = $(addprefix -I, ${_INCDIRS} )
ASMINCFLAGS = $(addprefix $(lastword -Wa,-I), ${_INCDIRS})

OBJECTS = $(addprefix ${OBJDIR}/, ${OBJS})
LIBRARIES = $(addprefix ${IMGDIR}/, ${LIBS})

ifdef IMGS
IMGS_ = $(addprefix ${IMGDIR}/, ${IMGS})
LIDS = $(foreach lid,$(addsuffix _LIDNUMBER, $(IMGS)),$(addprefix ${IMGDIR}/,$(addsuffix .ruhx, $($(lid)))))
IMAGES = $(addsuffix .bin, ${IMGS_}) $(addsuffix .elf, ${IMGS_}) ${LIDS}
#$(addsuffix .ruhx, ${IMGS_})
endif


${OBJDIR}/%.o: %.C
	@mkdir -p ${OBJDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote .

${OBJDIR}/%.list : ${OBJDIR}/%.o 
	${OBJDUMP} -dCS $@ > $(basename $@).list	

${OBJDIR}/%.o: %.c
	@mkdir -p ${OBJDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote .

${OBJDIR}/%.o : %.S
	@mkdir -p ${OBJDIR}
	${CC} -c ${ASMFLAGS} $< -o $@ ${ASMINCFLAGS} ${INCFLAGS} -iquote .

${OBJDIR}/%.dep : %.C
	@mkdir -p ${OBJDIR};
	@rm -f $@;
	${CXX} -M ${CXXFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$

${OBJDIR}/%.dep : %.c
	@mkdir -p ${OBJDIR};
	@rm -f $@;
	${CC} -M ${CFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$

${OBJDIR}/%.dep : %.S
	@mkdir -p ${OBJDIR};
	@rm -f $@;
	${CC} -M ${ASMFLAGS} $< -o $@.$$$$ ${ASMINCFLAGS} ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$

${IMGDIR}/%.so : ${OBJECTS}
	@mkdir -p ${IMGDIR}
	${LD} -shared -z now ${LDFLAGS} -o $@ $(OBJECTS) $(MODULE_LINKLIBS) ${LIBPATHS}
#	${LD} -shared -z now ${LDFLAGS} -o $@ $(OBJECTS)

${PGMDIR}/%.o : %.c
	@mkdir -p ${PGMDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list
${PGMDIR}/%.o : %.C
	@mkdir -p ${PGMDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list
${PGMDIR}/%.dep : %.C
	@mkdir -p ${PGMDIR};
	@rm -f $@;
	${CXX} -M ${CXXFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${PGMDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$
${PGMDIR}/%.dep : %.c
	@mkdir -p ${PGMDIR};
	@rm -f $@;
	${CC} -M ${CFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${PGMDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$

${TESTDIR}/%.o : %.c
	@mkdir -p ${TESTDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list
${TESTDIR}/%.o : %.C
	@mkdir -p ${TESTDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list
${TESTDIR}/%.dep : %.C
	@mkdir -p ${TESTDIR};
	@rm -f $@;
	${CXX} -M ${CXXFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${TESTDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$
${TESTDIR}/%.dep : %.c
	@mkdir -p ${TESTDIR};
	@rm -f $@;
	${CC} -M ${CFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${TESTDIR}/\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.$$$$

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

%.dep:
	cd ${basename $@} && ${MAKE} dep
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
	$(LINK.o) $(CFLAGS) $(LDFLAGS) $($(@)_PGM_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C-file main, build the *_OFILES into TESTDIR, and link them together
define BIN_TESTS_template
 $(1)_BTEST_OFILES = $(addprefix ${TESTDIR}/, $($(notdir $(1)_OFILES))) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$($(1)_BTEST_OFILES) $(notdir $(1)).c
 ALL_OFILES += $$($(1)_BTEST_OFILES) $(1)
endef
$(foreach bin_test,$(BIN_TESTS),$(eval $(call BIN_TESTS_template,$(bin_test))))

$(BIN_TESTS):
	$(LINK.o) $(CFLAGS) $($(@)_BTEST_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#Build a C++ file that uses gtest, build *_OFILES into TESTDIR, link with gtest_main
define GTESTS_template
 GTEST_DEPS = $(TESTDIR)/gtest-all.o $(TESTDIR)/gtest_main.o
 $(1)_GTESTS_OFILES = $(addprefix $(TESTDIR)/,$($(notdir $(1))_OFILES)) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$(GTEST_DEPS) $$($(1)_GTESTS_OFILES) $(notdir $(1)).C
 ALL_OFILES += $$($(1)_GTESTS_OFILES) $(1)
endef
$(foreach _gtest,$(GTESTS_DIR),$(eval $(call GTESTS_template,$(_gtest))))

$(GTESTS_DIR):
	$(CXX) $(CFLAGS) $(LDFLAGS) $($(@)_GTESTS_OFILES) $(GTEST_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@
#-------------------------------------------------------------------------------

#Build a C++ file that uses gtest, build *_OFILES into TESTDIR, link with gtest_main
define GTESTS_NM_template
 GTEST_NM_DEPS = $(TESTDIR)/gtest-all.o
 $(1)_GTESTS_NM_OFILES = $(addprefix $(TESTDIR)/,$($(notdir $(1))_OFILES)) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$(GTEST_NM_DEPS) $$($(1)_GTESTS_NM_OFILES) $(notdir $(1)).C
 ALL_OFILES += $$($(1)_GTESTS_NM_OFILES) $(1)
endef
$(foreach _gtest_nm,$(GTESTS_NM_DIR),$(eval $(call GTESTS_NM_template,$(_gtest_nm))))

$(GTESTS_NM_DIR):
	$(CXX) $(CFLAGS) $(LDFLAGS) $($(@)_GTESTS_NM_OFILES) $(GTEST_NM_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@
#-------------------------------------------------------------------------------

DEPS += $(addsuffix .dep, ${BIN_TESTS}) $(addsuffix .dep, ${PROGRAMS}) \
        $(addsuffix .dep, ${GTESTS_DIR}) $(OBJECTS:.o=.dep) \
        $(addsuffix .dep, ${GTESTS_NM_DIR})

BEAMOBJS  = $(addprefix ${BEAMDIR}/, ${OBJS:.o=.beam})
GENTARGET = $(addprefix %/, $(1))

${PROGRAMS} ${BIN_TESTS} ${GTESTS_DIR} $(GTESTS_NM_DIR) ${OBJECTS} ${LIBRARIES}: makefile
${LIBRARIES}: ${OBJECTS}
${EXTRA_PARTS} ${PROGRAMS}: ${LIBRARIES}
$(GTESTS_DIR) $(GTESTS_NM_DIR) $(BIN_TESTS): $(GTEST_TARGETS)

dep:       ${SUBDIRS:.d=.dep} ${DEPS}
code_pass: ${SUBDIRS} ${LIBRARIES} ${EXTRA_PARTS} ${PROGRAMS}
test:      ${SUBDIRS:.d=.test} ${BIN_TESTS} ${GTESTS_DIR} ${GTESTS_NM_DIR}
fvt:       ${SUBDIRS:.d=.fvt}
unit:      ${SUBDIRS:.d=.unit}
beam:      ${SUBDIRS:.d=.beamdir} ${BEAMOBJS}

docs: ${ROOTPATH}/src/build/doxygen/doxygen.conf
	@rm -rf ${ROOTPATH}/obj/doxygen/*
	@cd ${ROOTPATH}; doxygen src/build/doxygen/doxygen.conf

install:
	rm -rf ${PKGDIR}/install_root/*
	cd ${ROOTPATH}/src/build/install && ${MAKE}

packaging:
	${MAKE} install
	cd ${ROOTPATH}/src/build/packaging && ${MAKE}

cscope:
	@mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f cscope.* ; \
	    find ../../ -name '*.[CHchS]' -type f -fprint cscope.files; \
	    cscope -bqk)

ctags:
	@mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f tags ; \
	    ctags --recurse=yes --fields=+S ../../src)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
ifneq ($(MAKECMDGOALS),tests)
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
endif

cleanud :
	rm -f ${UD_OBJS}

clean: cleanud ${SUBDIRS:.d=.clean}
	(rm -f ${OBJECTS} ${OBJECTS:.o=.dep} ${OBJECTS:.o=.list} \
	       ${OBJECTS:.o=.o.hash} ${BEAMOBJS} ${LIBRARIES} \
	       ${IMAGES} ${IMAGES:.bin=.list} ${IMAGES:.bin=.syms} \
	       ${IMAGES:.bin=.bin.modinfo} ${IMAGES:.ruhx=.lid} \
	       ${IMAGES:.ruhx=.lidhdr} ${IMAGES:.bin=_extended.bin} \
	       ${IMAGE_EXTRAS} ${TESTDIR}/* \
	       ${EXTRA_OBJS} ${_GENFILES} ${EXTRA_PARTS} ${EXTRA_CLEAN}\
	       ${PROGRAMS} ${ALL_OFILES} \
	       *.a *.o *~* )

cleanall:
	@if [[ -e ${ROOTPATH}/obj ]]; then rm -Rf ${ROOTPATH}/obj/*; fi
	@if [[ -e $(IMGDIR) ]]; then rm -Rf $(IMGDIR)/*; fi
	@if [[ -e $(PKGDIR) ]]; then rm -Rf $(PKGDIR)/*; fi
	@echo "clean done"

ifdef IMAGES
	${MAKE} ${IMAGES} ${IMAGE_EXTRAS}
endif
