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

#######################################################################################
# env.bash sets USE_ADVANCED_TOOLCHAIN=no, so no rpath is set
#######################################################################################

SHELL=/bin/bash

.PHONY: default

default:
	${MAKE} -j10 SKIP_TEST=1 dep
	${MAKE} -j10 SKIP_TEST=1 code_pass
	${MAKE} -j10 SKIP_TEST=1 bin
ifneq ($(CXLFLASH_DOC),no)
	${MAKE} docs
endif
ifeq ($(CXLFLASH_ALL),yes)
	${MAKE} -j10 test_pass
else
	@if [[ dir_$(notdir $(PWD)) = dir_test ]]; then ${MAKE} -j10 test_pass; fi
endif

buildall: default
	${MAKE} -j10 test_pass

run_fvt:
	${MAKE} fvt

run_unit:
	${MAKE} unit

prod:
	${MAKE} LDFLAGS=${OPT_LDFLAGS} -j10 SKIP_TEST=1 code_pass
	${MAKE} LDFLAGS=${OPT_LDFLAGS} -j10 SKIP_TEST=1 bin

prodall: prod
	${MAKE} LDFLAGS=${OPT_LDFLAGS} -j10 test_pass
	${MAKE} docs

prodpkgs: prod
	${MAKE} pkg_code
	${MAKE} pkg_image

allpkgs: prodall
	${MAKE} pkg_code
	${MAKE} pkg_image
	${MAKE} pkg_test

configure:
	@sudo -E $(SURELOCKROOT)/src/build/install/resources/cflash_depends
	@$(SURELOCKROOT)/src/build/install/resources/gtest_add

installsb: default
ifneq ($(CXLFLASH_ALL),no)
	${MAKE} -j10 test_pass
	${MAKE} install_test
endif
	${MAKE} install_code
	@echo ""
	@echo "INSTALLing from $(SURELOCKROOT)"
	@sudo -E $(SURELOCKROOT)/src/build/install/resources/cflash_installsb $(SURELOCKROOT)

install:
	${MAKE} install_code
ifeq ($(CXLFLASH_ALL),yes)
	${MAKE} install_image
	${MAKE} install_test
endif

test:

#this target queries the GITREVISION to be used
setversion:
	$(info GITREVISION=${GITREVISION})

#ensure build env is setup
ifndef SURELOCKROOT
$(error run: "source env.bash")
else
$(shell mkdir -p ${SURELOCKROOT}/obj/tests)
endif

#setup package version
VERSIONMAJOR=5
VERSIONMINOR=0
SOVER=-0

FS=$(findstring surelock-sw, ${SURELOCKROOT})
GIT_TREE=$(shell git rev-parse --is-inside-work-tree 2>/dev/null)

#if this is surelock-sw, extract and save rev-list
#else this is a tarball or capiflash, so use version.txt
ifeq (surelock-sw:true,${FS}:${GIT_TREE})
  GITREVISION=$(shell git rev-list HEAD 2>/dev/null | wc -l)
  $(shell echo ${VERSIONMAJOR}.${VERSIONMINOR}.${GITREVISION} > ${SURELOCKROOT}/obj/tests/version.txt)
else
  GITREVISION=$(shell cat ${SURELOCKROOT}/src/build/version.txt 2>/dev/null)
  GITREVISION:=$(lastword $(subst ., ,${GITREVISION}))
endif

#generate VPATH based on these dirs.
VPATH_DIRS=. ${ROOTPATH}/src/common ${ROOTPATH}/obj/lib/ ${ROOTPATH}/lib
#generate the VPATH, subbing :'s for spaces
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
VPATH += $(subst $(SPACE),:,$(VPATH_DIRS))

## output libs, objs for userdetails parsers
UD_DIR = ${ROOTPATH}/obj/modules/userdetails
UD_OBJS = ${UD_DIR}*.o ${UD_DIR}/*.so ${UD_DIR}/*.a

PGMDIR        = ${ROOTPATH}/obj/programs
TESTDIR       = ${ROOTPATH}/obj/tests
PKGDIR        = ${ROOTPATH}/pkg
GTESTS_DIR    = $(addprefix $(TESTDIR)/, $(GTESTS))
GTESTS_NM_DIR = $(addprefix $(TESTDIR)/, $(GTESTS_NO_MAIN))
BIN_TESTS     = $(addprefix ${TESTDIR}/, ${BTESTS})
PROGRAMS      = $(addprefix ${PGMDIR}/, ${PGMS})
BITS          =
CFLAGS        =
LDFLAGS       = -Wl,-Bsymbolic-functions -Wl,-z,relro,-z,now

ifeq (redhat,${TARGET_OS})
IMGDIR        = ${ROOTPATH}/lib64
LIBPATHS      = -L${ROOTPATH}/lib64
SLIB          = ${SURELOCKROOT}/lib64
else
IMGDIR        = ${ROOTPATH}/lib
LIBPATHS      = -L${ROOTPATH}/lib
SLIB          = ${SURELOCKROOT}/lib
endif

ifneq (${SURELOCKROOT}/lib,$(findstring ${SURELOCKROOT}/lib,${LD_LIBRARY_PATH}))
export LD_LIBRARY_PATH+=:${SLIB}:$(strip $(wildcard /usr/lib/powerpc*gnu)):/usr/lib64
endif

ifdef MODULE
OBJDIR            = ${ROOTPATH}/obj/modules/${MODULE}
BEAMDIR           = ${ROOTPATH}/obj/beam/${MODULE}
ifdef STRICT
EXTRACOMMONFLAGS += -Weffc++
endif
MODFLAGS          = -D__SURELOCK_MODULE=${MODULE}
LIBS             += $(addsuffix .so, $(addprefix lib, ${MODULE}))
else
OBJDIR            = ${ROOTPATH}/obj/surelock
BEAMDIR           = ${ROOTPATH}/obj/beam/surelock
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
	CC      = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/gcc
	CXX     = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/g++
	LD      = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/gcc
	OBJDUMP = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/objdump
	#this line is very specifically-written to explicitly-place the ATx.x stuff at the FRONT of the VPATH dirs.
	#this is a REQUIREMENT of the advanced toolchain for linux.
	VPATH_DIRS:= ${ADV_TOOLCHAIN_PATH}/lib64 ${VPATH_DIRS}
	#see the ld flags below (search for rpath). This puts the atx.x stuff on the front
	#which is REQUIRED by the toolchain.
	CFLAGS  += ${COMMONFLAGS} -Wall ${CUSTOMFLAGS} ${ARCHFLAGS} ${MODFLAGS}
    LDFLAGS += ${COMMONFLAGS} -Wl,-rpath,${ADV_TOOLCHAIN_PATH}/lib64
else
	CC  = gcc
	CXX = g++
	LD  = gcc
	OBJDUMP = objdump
	CFLAGS  += ${COMMONFLAGS} -Wall ${CUSTOMFLAGS}  ${ARCHFLAGS} ${MODFLAGS}
    LDFLAGS += ${COMMONFLAGS} -Wl,-rpath,
endif

OPT_LDFLAGS="${LDFLAGS}:/usr/lib:/usr/lib64"

ifdef DBG
	CFLAGS += -g
	NO_O3=1
endif

#TODO: Find correct flags for surelock
#moved custom P8 tunings to customrc file.
COMMONFLAGS = ${EXTRACOMMONFLAGS}

ifndef NO_O3
COMMONFLAGS += -O3
endif

CFLAGS   += ${LCFLAGS}
CFLAGS   += -DGITREVISION='"${GITREVISION}"'
CFLAGS   += -Wno-unused-result -fstack-protector-strong -Wformat
LINKLIBS += -lpthread -ludev
MODLIBS  += -lpthread -ludev

#if ALLOW_WARNINGS is NOT defined, we assume we are compiling production code
#as such, we adhere to strict compile flags. If this is defined then we warn
#but allow the compile to continue.
ifndef ALLOW_WARNINGS
	CFLAGS   += -Werror
	CXXFLAGS += -Werror
endif

ifdef COVERAGE
COMMONFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS     += -lgcov
LINKLIBS    += -lgcov
endif

ASMFLAGS = ${COMMONFLAGS}
CXXFLAGS += ${CFLAGS} -fno-rtti -fno-exceptions -Wall

INCDIR = . ${ROOTPATH}/src/include /usr/include /usr/include/misc
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
	${CXX} -fPIC -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

${OBJDIR}/%.o: %.c
	@mkdir -p ${OBJDIR}
	${CC} -fPIC -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

${OBJDIR}/%.o : %.S
	@mkdir -p ${OBJDIR}
	${CC} -c ${ASMFLAGS} $< -o $@ ${ASMINCFLAGS} ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

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
	${LD} -shared ${LDFLAGS} -Wl,-soname,$(notdir $(@:.so=))$(SOVER).so -o $(@:.so=)$(SOVER).so $(OBJECTS) $(MODLIBS) ${LIBPATHS}
	@chmod -x $(@:.so=)$(SOVER).so

${PGMDIR}/%.o : %.c
	@mkdir -p ${PGMDIR}
	${CC} -fPIE -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

${PGMDIR}/%.o : %.C
	@mkdir -p ${PGMDIR}
	${CXX} -fPIE -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

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
	${CC} -fPIE -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

${TESTDIR}/%.o : %.C
	@mkdir -p ${TESTDIR}
	${CXX} -fPIE -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote . -fverbose-asm -Wa,-acdnhl=$(@:.o=.s)

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
	@if [[ A${SKIP_TEST} = A1 && ${@:.dep=} = test ]]; then\
       echo "make: SKIP test";\
     else\
       echo "make: dep";\
       cd ${basename $@} && ${MAKE} dep;\
     fi
%.code_pass:
	@if [[ A${SKIP_TEST} = A1 && ${@:.code_pass=} = test ]]; then\
       echo "make: SKIP test";\
     else\
       echo "make: code_pass";\
       cd ${basename $@} && ${MAKE} code_pass;\
     fi
%.bin:
	@if [[ A${SKIP_TEST} = A1 && ${@:.bin=} = test ]]; then\
       echo "make: SKIP test";\
     else\
       echo "make: bin";\
       cd ${basename $@} && ${MAKE} bin;\
     fi
%.test_pass:
	cd ${basename $@} && ${MAKE} test_pass
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
	$(LINK.o) $(CFLAGS) -pie $(LDFLAGS) $($(@)_PGM_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#-------------------------------------------------------------------------------
#Build a C-file main, build the *_OFILES into TESTDIR, and link them together
define BIN_TESTS_template
 $(1)_BTEST_OFILES = $(addprefix ${TESTDIR}/, $($(notdir $(1)_OFILES))) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$($(1)_BTEST_OFILES) $(notdir $(1)).c
 ALL_OFILES += $$($(1)_BTEST_OFILES) $(1)
endef
$(foreach bin_test,$(BIN_TESTS),$(eval $(call BIN_TESTS_template,$(bin_test))))

$(BIN_TESTS):
	$(LINK.o) $(CFLAGS) -pie $(LDFLAGS) $($(@)_BTEST_OFILES) $(LINKLIBS) ${LIBPATHS} -o $@

#Build a C++ file that uses gtest, build *_OFILES into TESTDIR, link with gtest_main
define GTESTS_template
 GTEST_DEPS = $(TESTDIR)/gtest-all.o $(TESTDIR)/gtest_main.o
 $(1)_GTESTS_OFILES = $(addprefix $(TESTDIR)/,$($(notdir $(1))_OFILES)) $(addprefix $(TESTDIR)/, $(notdir $1)).o
 $(1): $$(GTEST_DEPS) $$($(1)_GTESTS_OFILES) $(notdir $(1)).C
 ALL_OFILES += $$($(1)_GTESTS_OFILES) $(1)
endef
$(foreach _gtest,$(GTESTS_DIR),$(eval $(call GTESTS_template,$(_gtest))))

$(GTESTS_DIR):
	$(CXX) $(CFLAGS) -pie $(LDFLAGS) $($(@)_GTESTS_OFILES) $(GTEST_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@
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
	$(CXX) $(CFLAGS) -pie $(LDFLAGS) $($(@)_GTESTS_NM_OFILES) $(GTEST_NM_DEPS) $(LINKLIBS) ${LIBPATHS} -o $@
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

dep:       ${SUBDIRS:.d=.dep}       ${DEPS}
code_pass: ${SUBDIRS:.d=.code_pass} ${LIBRARIES} ${EXTRA_PARTS} ${PROGRAMS}
bin:       ${SUBDIRS:.d=.bin}       ${BIN_TESTS}
test_pass: ${SUBDIRS:.d=.test_pass} ${GTESTS_DIR} ${GTESTS_NM_DIR}
fvt:       ${SUBDIRS:.d=.fvt}
unit:      ${SUBDIRS:.d=.unit}
beam:      ${SUBDIRS:.d=.beamdir}   ${BEAMOBJS}

docs: ${ROOTPATH}/src/build/doxygen/doxygen.conf
	@rm -rf ${ROOTPATH}/obj/doxygen/*
	@mkdir -p ${ROOTPATH}/obj/doxygen/*
ifneq ($(TARGET_PLATFORM),x86_64)
	@cd ${ROOTPATH}; doxygen src/build/doxygen/doxygen.conf
endif

bins:
	${MAKE} -j10 bin

tests:
	${MAKE} -j10 test_pass

install_code:
	cd ${ROOTPATH}/src/build/install && ${MAKE} cxlflash

install_image:
	cd ${ROOTPATH}/src/build/install && ${MAKE} cxlflashimage

install_test:
	cd ${ROOTPATH}/src/build/install && ${MAKE} cxlflash-test

pkg_code: install_code
	cd ${ROOTPATH}/src/build/packaging && ${MAKE} cxlflash

pkg_image: install_image
	cd ${ROOTPATH}/src/build/packaging && ${MAKE} cxlflashimage

pkg_test: install_test
	cd ${ROOTPATH}/src/build/packaging && ${MAKE} cxlflash-test

pkg_tar:
	cd ${ROOTPATH}/src/build/packaging && ${MAKE} tarpkgs

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

cleandir: cleanud ${SUBDIRS:.d=.clean}
	(rm -f ${OBJECTS} ${OBJECTS:.o=.dep} ${OBJECTS:.o=.list} \
	       ${OBJECTS:.o=.o.hash} ${BEAMOBJS} ${LIBRARIES} \
	       ${IMAGES} ${IMAGES:.bin=.list} ${IMAGES:.bin=.syms} \
	       ${IMAGES:.bin=.bin.modinfo} ${IMAGES:.ruhx=.lid} \
	       ${IMAGES:.ruhx=.lidhdr} ${IMAGES:.bin=_extended.bin} \
	       ${IMAGE_EXTRAS} ${TESTDIR}/* \
	       ${EXTRA_OBJS} ${_GENFILES} ${EXTRA_PARTS} ${EXTRA_CLEAN}\
	       ${PROGRAMS} ${ALL_OFILES} \
	       *.a *.o *~* )

clean:
	@if [[ -e ${ROOTPATH}/obj ]]; then rm -Rf ${ROOTPATH}/obj/*; fi
	@if [[ -e ${ROOTPATH}/build ]]; then rm -Rf ${ROOTPATH}/build/*; fi
	@if [[ -e $(IMGDIR) ]]; then rm -Rf $(IMGDIR)/*; fi
	@if [[ -e $(PKGDIR) ]]; then rm -Rf $(PKGDIR)/*; fi
	@if [[ -e ${ROOTPATH}/doxywarnings.log ]]; then rm -f *.log; fi
	@echo "clean done"

cleanall: clean

ifdef IMAGES
	${MAKE} ${IMAGES} ${IMAGE_EXTRAS}
endif
