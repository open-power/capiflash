# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: config.mac.mk $
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

all:
	${MAKE} gen_pass
	${MAKE} code_pass

#generate VPATH based on these dirs.
VPATH_DIRS=. ${ROOTPATH}/src/common ${ROOTPATH}/obj/lib/ ${ROOTPATH}/img /usr/lib
#generate the VPATH, subbing :'s for spaces
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
VPATH += $(subst $(SPACE),:,$(VPATH_DIRS))

## output libs, objs for userdetails parsers
UD_DIR = ${ROOTPATH}/obj/modules/userdetails
UD_OBJS = ${UD_DIR}*.o ${UD_DIR}/*.so ${UD_DIR}/*.a

LIBPATHS=${ROOTPATH}/img

ifdef MODULE
OBJDIR = ${ROOTPATH}/obj/modules/${MODULE}
BEAMDIR = ${ROOTPATH}/obj/beam/${MODULE}
GENDIR = ${ROOTPATH}/obj/genfiles
TESTDIR = ${ROOTPATH}/obj/tests
IMGDIR = ${ROOTPATH}/img
PKGDIR = ${ROOTPATH}/pkg
#GTESTFDIR=${ROOTPATH}/src/test
EXTRACOMMONFLAGS += 
ifdef STRICT
        EXTRACOMMONFLAGS += -Weffc++
endif
CUSTOMFLAGS += -D_MACOSX -D__SURELOCK_MODULE=${MODULE}
#CUSTOMFLAGS += -D__SURELOCK_MODULE=${MODULE}
LIBS += $(addsuffix .so, $(addprefix lib, ${MODULE}))
EXTRAINCDIR += ${GENDIR} ${CURDIR}
else
OBJDIR = ${ROOTPATH}/obj/surelock
BEAMDIR = ${ROOTPATH}/obj/beam/surelock
GENDIR = ${ROOTPATH}/obj/genfiles
IMGDIR = ${ROOTPATH}/img
PKGDIR = ${ROOTPATH}/pkg
TESTDIR = ${ROOTPATH}/obj/tests
EXTRAINCDIR += ${GENDIR} ${CURDIR}
CUSTOMFLAGS += -D_MACOSX
#GTESTFDIR=${ROOTPATH}/src/test
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

TRACEPP = ${ROOTPATH}/src/build/trace/tracepp
CUSTOM_LINKER = i686-mcp6-jail ${CUSTOM_LINKER_EXE}
JAIL = ppc64-mcp75-jail

ifeq ($(USE_ADVANCED_TOOLCHAIN),yes)
	CC_RAW = gcc 
	CXX_RAW = g++ 
	CC = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CC_RAW}
	CXX = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/${CXX_RAW}
	LD = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/ld
	OBJDUMP = ${JAIL} ${ADV_TOOLCHAIN_PATH}/bin/objdump
else
	CC_RAW = gcc 
	CXX_RAW = g++ 
	CC = ${CC_RAW}
	CXX = ${CXX_RAW}
	LD = ld
	OBJDUMP = objdump
endif

#TODO: Find correct flags for surelock - copied from POWER7 Hostboot for now
COMMONFLAGS = -O3 ${EXTRACOMMONFLAGS}
CFLAGS += ${COMMONFLAGS} -g \
	 ${CUSTOMFLAGS} ${ARCHFLAGS} \
	${INCFLAGS}
ASMFLAGS = ${COMMONFLAGS} 
CXXFLAGS = ${CFLAGS} -fno-rtti -fno-exceptions 
LDFLAGS = -lc ${MODLIBS} -macosx_version_min 10.6

ifdef USE_PYTHON
    TESTGEN = ${ROOTPATH}/src/usr/cxxtest/cxxtestgen.py
else
    TESTGEN = ${ROOTPATH}/src/usr/cxxtest/cxxtestgen.pl
endif

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


${OBJDIR}/%.o ${OBJDIR}/%.list : %.C
	mkdir -p ${OBJDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote .


${OBJDIR}/%.o ${OBJDIR}/%.list : %.c
	mkdir -p ${OBJDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote .


${OBJDIR}/%.o : %.S
	mkdir -p ${OBJDIR}
	${CC} -c ${ASMFLAGS} $< -o $@ ${ASMINCFLAGS} ${INCFLAGS} -iquote .

${OBJDIR}/%.dep : %.C
	mkdir -p ${OBJDIR}; \
	rm -f $@; \
	${CXX} -M ${CXXFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

${OBJDIR}/%.dep : %.c
	mkdir -p ${OBJDIR}; \
	rm -f $@; \
	${CC} -M ${CFLAGS} $< -o $@.$$$$ ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

${OBJDIR}/%.dep : %.S
	mkdir -p ${OBJDIR}; \
	rm -f $@; \
	${CC} -M ${ASMFLAGS} $< -o $@.$$$$ ${ASMINCFLAGS} ${INCFLAGS} -iquote .; \
	sed 's,\($*\)\.o[ :]*,${OBJDIR}/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

${IMGDIR}/%.so : ${OBJECTS}
	mkdir -p ${IMGDIR}
	${LD} -dylib /usr/lib/dylib1.o -L${LIBPATHS} ${LDFLAGS} -o $@ $(OBJECTS)  

${TESTDIR}/%.o : %.c
	mkdir -p ${TESTDIR}
	${CC} -c ${CFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list
${TESTDIR}/%.o : %.C
	mkdir -p ${TESTDIR}
	${CXX} -c ${CXXFLAGS} $< -o $@ ${INCFLAGS} -iquote .
	${OBJDUMP} -dCS $@ > $(basename $@).list

%.d: ${OBJECTS}
	cd ${basename $@} && ${MAKE} code_pass

%.gen_pass:
	cd ${basename $@} && ${MAKE} gen_pass

%.test:
	cd ${basename $@} && ${MAKE} test

#packaging generally requires the code pass to be complete
%.packaging: 
	cd ${basename $@} && ${MAKE} packaging

#install generally requires the code pass to be complete
%.install: all
	cd ${basename $@} && ${MAKE} install
%.clean:
	cd ${basename $@} && ${MAKE} clean

%.beamdir:
	cd ${basename $@} && ${MAKE} beam

#create a make function that we can use to generically create a program with extra libs
define PROGRAM_template
 $(1): $$($(1)_OFILES) $$($(1)_EXTRA_LIBS:%=lib%.so)
 ALL_OFILES += $$($(1)_OFILES)
endef

$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))

$(PROGRAMS):
	$(LINK.o) $^ $(LDLIBS) -o $@

$(BIN_TESTS):
	mkdir -p ${PGMDIR}
	${CC} -c ${CFLAGS} ${INCFLAGS} -iquote . -c $@.c -o $(PGMDIR)/$@.o
	$(LINK.o) $(CFLAGS) $(LDFLAGS) $(LINKLIBS) ${LIBPATHS} $(PGMDIR)/$@.o -o $(PGMDIR)/$@

code_pass: ${OBJECTS} ${SUBDIRS} ${LIBRARIES} ${EXTRA_PARTS} ${PROGRAMS}
ifdef IMAGES
	${MAKE} ${IMAGES} ${IMAGE_EXTRAS}
endif

gen_pass:
	mkdir -p ${GENDIR}
	${MAKE} GEN_PASS

_GENFILES = $(addprefix ${GENDIR}/, ${GENFILES})
GEN_PASS: ${_GENFILES} ${SUBDIRS:.d=.gen_pass}

GENTARGET = $(addprefix %/, $(1))

${BEAMDIR}/%.beam : %.C
	mkdir -p ${BEAMDIR}
	${BEAMCMD} -I ${INCDIR} ${CXXFLAGS} ${BEAMFLAGS} $< \
	    --beam::complaint_file=$@ --beam::parser_file=/dev/null

${BEAMDIR}/%.beam : %.c
	mkdir -p ${BEAMDIR}
	${BEAMCMD} -I ${INCDIR} ${CXXFLAGS} ${BEAMFLAGS} $< \
	    --beam::complaint_file=$@ --beam::parser_file=/dev/null

${BEAMDIR}/%.beam : %.S
	echo Skipping ASM file.

BEAMOBJS = $(addprefix ${BEAMDIR}/, ${OBJS:.o=.beam})
beam: ${SUBDIRS:.d=.beamdir} ${BEAMOBJS}

cleanud :
	rm -f ${UD_OBJS}

test: ${SUBDIRS:.d=.test}


.PHONY: install
install: ${SUBDIRS:.d=.install}


.PHONY: packaging
packaging: ${SUBDIRS:.d=.packaging}

clean: cleanud ${SUBDIRS:.d=.clean}
	(rm -rf ${OBJECTS} ${OBJECTS:.o=.dep} ${OBJECTS:.o=.list} \
	       ${OBJECTS:.o=.o.hash} ${BEAMOBJS} ${LIBRARIES} \
	       ${IMAGES} ${IMAGES:.bin=.list} ${IMAGES:.bin=.syms} \
	       ${IMAGES:.bin=.bin.modinfo} ${IMAGES:.ruhx=.lid} \
	       ${IMAGES:.ruhx=.lidhdr} ${IMAGES:.bin=_extended.bin} \
	       ${IMAGE_EXTRAS} ${TESTDIR}/* \
	       ${EXTRA_OBJS} ${_GENFILES} ${EXTRA_PARTS} ${EXTRA_CLEAN}\
	       $gtest.a gtest_main.a *.o *unit_test-kv_results* *~* )

cscope:
	mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f cscope.* ; \
	    find ../../ -name '*.[CHchS]' -type f -print > cscope.files; \
	    cscope -bqk)

ctags:
	mkdir -p ${ROOTPATH}/obj/cscope
	(cd ${ROOTPATH}/obj/cscope ; rm -f tags ; \
	    ctags ../../src)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),gen_pass)
ifneq ($(MAKECMDGOALS),GEN_PASS)
    -include $(OBJECTS:.o=.dep)
endif
endif
endif
