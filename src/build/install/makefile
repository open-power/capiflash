# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/makefile $
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

APPNAME?=cflash
PREFIX?=${PKGDIR}/install_root
TEST_PREFIX?=${PKGDIR}/test_root
AFU_PREFIX?=${PKGDIR}/afu_root

ifeq ($(UNAME),AIX)
INSTALL=/usr/bin/cp
INSTALL_ROOT=${PREFIX}/usr/local/cxlflash
TEST_ROOT=${TEST_PREFIX}/usr/local/cxlflash
AFU_ROOT=${AFU_PREFIX}/usr/local/cxlflash/afu
AFU_BIN=${AFU_ROOT}
AFU_IMAGES_DIR=${AFU_ROOT}/images
TEST_DIR=${TEST_ROOT}/test
LIC_DIR=${INSTALL_ROOT}/license
ETC_DIR=${INSTALL_ROOT}/etc
EXAMPLES_DIR=${INSTALL_ROOT}/examples
else
INSTALL=install
INSTALL_ROOT=${PREFIX}/usr
AFU_BIN=${AFU_PREFIX}/usr/bin
AFU_IMAGES_DIR=${AFU_PREFIX}/lib/firmware/cxlflash
TEST_DIR=${TEST_PREFIX}/usr/bin
ETC_DIR=${PREFIX}/etc/cxlflash
LIC_DIR=${PREFIX}/usr/share/cxlflash/license
EXAMPLES_DIR=${PREFIX}/usr/src/cxlflash/examples
endif

LIBSRCDIR=${IMGDIR}
TESTSRCDIR=${TESTDIR}
DOCSRCDIR=${ROOTPATH}/obj/doxygen
RESOURCEDIR=${ROOTPATH}/src/build/install/resources
AFUDIR=${ROOTPATH}/src/afu

BIN_DIR=${INSTALL_ROOT}/bin
LIB_DIR=${INSTALL_ROOT}/lib
INC_DIR=${INSTALL_ROOT}/include
SHARE_DIR=${INSTALL_ROOT}/share/cxlflash

MAN_DIR=${PREFIX}/usr/share/man/man3
SYSTEMD_DIR=${PREFIX}/usr/lib/systemd/system
UDEV_RULES_DIR=${PREFIX}/lib/udev/rules.d
IMAGE="${RESOURCEDIR}/corsa*"
IMAGEH="${RESOURCEDIR}/1410*"

ifeq ($(UNAME),AIX)
.PHONY: aixinstall
aixinstall:
	rm -rf ${PREFIX}
	rm -rf ${TEST_PREFIX}
	rm -rf ${AFU_PREFIX}
	${MAKE} cflash cflash-test cflashimage
else
codeinstall:
	rm -rf ${PREFIX}
	rm -rf ${TEST_PREFIX}
	rm -rf ${AFU_PREFIX}
	${MAKE} cflash cflashimage

testinstall:
	rm -rf ${TEST_PREFIX}
	${MAKE} cflash-test
endif



#ifneq ($(TARGET_PLATFORM),x86_64)

LIC_FILES := $(notdir $(shell ls ${RESOURCEDIR}/license/*.txt 2>/dev/null))
${LIC_FILES}:
	@mkdir -p ${LIC_DIR}
ifeq ($(UNAME),AIX)
	${INSTALL} ${RESOURCEDIR}/license/$@ ${LIC_DIR}
else
	${INSTALL} -m 644 ${RESOURCEDIR}/license/$@ ${LIC_DIR}
endif


MAN3_FILES := afu.h.3 cxl.h.3 capiblock.h.3 libcxl.h.3 arkdb.h.3 ark.h.3 cflash_tools_user.h.3
${MAN3_FILES}:
	@mkdir -p ${MAN_DIR}
	@if [[ -e ${DOCSRCDIR}/man/man3/$@ ]]; \
    then echo ${INSTALL} ${DOCSRCDIR}/man/man3/$@ ${MAN_DIR};\
              ${INSTALL} ${DOCSRCDIR}/man/man3/$@ ${MAN_DIR}; fi

#endif

cflash: ${LIC_FILES} ${MAN3_FILES}
	@mkdir -p ${BIN_DIR}
	@mkdir -p ${ETC_DIR}
	@mkdir -p ${LIB_DIR}
	@mkdir -p ${LIC_DIR}
	@mkdir -p ${SHARE_DIR}
	@mkdir -p ${EXAMPLES_DIR}
	@mkdir -p -m 755 ${INC_DIR}
ifneq ($(UNAME),AIX)
	@mkdir -p ${UDEV_RULES_DIR}
	@mkdir -p ${SYSTEMD_DIR}
	@mkdir -p ${MAN_DIR}
endif
ifeq ($(UNAME),AIX)
	${INSTALL} ${ROOTPATH}/src/kv/arkdb.h ${INC_DIR}
	${INSTALL} ${ROOTPATH}/src/include/zmalloc.h ${INC_DIR}
	${INSTALL} ${ROOTPATH}/src/include/capiblock.h ${INC_DIR}
	${INSTALL} ${ROOTPATH}/src/include/cflash_tools_user.h ${INC_DIR}
else
	${INSTALL} -m 644 ${ROOTPATH}/src/kv/arkdb.h ${INC_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/include/zmalloc.h ${INC_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/include/capiblock.h ${INC_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/include/cflash_tools_user.h ${INC_DIR}
endif
ifeq ($(UNAME),AIX)
	${INSTALL} ${TESTSRCDIR}/blockio64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blocksio64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockr0io64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockr0sio64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blocklistio64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockplistio64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blocksio ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockr0io ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockr0sio ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockio ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blocklistio ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blockplistio ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_async64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_sync64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_persist64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_benchmark64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/kv_perf64 ${BIN_DIR}
	${INSTALL} ${ROOTPATH}/src/test/ioppt ${BIN_DIR}
	${INSTALL} ${ROOTPATH}/src/test/ioppts.qd ${BIN_DIR}
else
ifneq ($(TARGET_PLATFORM),x86_64)
	${INSTALL} -s ${PGMDIR}/cxlfd ${BIN_DIR}
	${INSTALL} -s ${PGMDIR}/cxlflashutil ${BIN_DIR}
	${INSTALL} -s ${PGMDIR}/provtool ${BIN_DIR}
endif
	${INSTALL} -s ${TESTSRCDIR}/blockio ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/blocksio ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/blockr0io ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/blockr0sio ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/blocklistio ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/blockplistio ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/run_kv_sync ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/run_kv_async ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/run_kv_persist ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/run_kv_benchmark ${BIN_DIR}
	${INSTALL} -s ${TESTSRCDIR}/kv_perf ${BIN_DIR}
	${INSTALL}    ${ROOTPATH}/src/test/ioppt ${BIN_DIR}
	${INSTALL}    ${ROOTPATH}/src/test/ioppts.qd ${BIN_DIR}

	${INSTALL} ${RESOURCEDIR}/cflash_plinks ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cflash_getslot ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlfrefreshluns ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cablecheck ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlfsetlunmode ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlfstatus ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cflashutils ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/setup ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cflash_version ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/machine_info ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cflash_logs_cron ${BIN_DIR}


	@#TEMPORARY install - remove this after kernel driver can run workaround
	${INSTALL} ${RESOURCEDIR}/afucfg ${BIN_DIR}


	@#Dev Permissions for cxl - set mode to 644 explicitly
	${INSTALL} -m 644 ${RESOURCEDIR}/80-cxl.rules ${UDEV_RULES_DIR}
	${INSTALL} -m 644 ${RESOURCEDIR}/80-cxlflash.rules ${UDEV_RULES_DIR}

	@if [[ -e ${ROOTPATH}/src/build/version.txt ]]; then \
       echo "${INSTALL} ${ROOTPATH}/src/build/version.txt ${SHARE_DIR}"; \
       ${INSTALL} -m 644 ${ROOTPATH}/src/build/version.txt ${SHARE_DIR}; \
     else \
       echo "${INSTALL} ${TESTSRCDIR}/version.txt ${SHARE_DIR}"; \
       ${INSTALL} -m 644 ${TESTSRCDIR}/version.txt ${SHARE_DIR}; \
     fi
	${INSTALL} -m 644 ${RESOURCEDIR}/afu_versions ${SHARE_DIR}
	${INSTALL} ${RESOURCEDIR}/cflash_configure ${PKGDIR}
	${INSTALL} ${RESOURCEDIR}/cflash_configure ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlffdc.debug ${BIN_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlffdc.cleardebug ${BIN_DIR}
endif
	
	@#Libs
ifeq ($(UNAME),AIX)
	${INSTALL} ${LIBSRCDIR}/lib*.a ${LIB_DIR}
else
ifeq ($(TARGET_PLATFORM),x86_64)
	${INSTALL} -s ${LIBSRCDIR}/libark*.so ${LIB_DIR}
	${INSTALL} -s ${LIBSRCDIR}/libcf*.so ${LIB_DIR}
else
	${INSTALL} -s ${LIBSRCDIR}/lib*.so ${LIB_DIR}
endif
endif

	@#Sample Code
ifeq ($(UNAME),AIX)
	${INSTALL} ${ROOTPATH}/src/block/bin/blockio.c ${EXAMPLES_DIR}
	${INSTALL} ${ROOTPATH}/src/block/bin/blocklistio.c ${EXAMPLES_DIR}
	${INSTALL} ${ROOTPATH}/src/block/bin/blockplistio.c ${EXAMPLES_DIR}
	${INSTALL} ${ROOTPATH}/src/kv/bin/run_kv_async.c ${EXAMPLES_DIR}
	${INSTALL} ${ROOTPATH}/src/kv/bin/run_kv_sync.c ${EXAMPLES_DIR}
	${INSTALL} ${ROOTPATH}/src/kv/bin/run_kv_persist.c ${EXAMPLES_DIR}
else
	${INSTALL} -m 644 ${ROOTPATH}/src/block/bin/blockio.c ${EXAMPLES_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/block/bin/blocklistio.c ${EXAMPLES_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/block/bin/blockplistio.c ${EXAMPLES_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/kv/bin/run_kv_async.c ${EXAMPLES_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/kv/bin/run_kv_sync.c ${EXAMPLES_DIR}
	${INSTALL} -m 644 ${ROOTPATH}/src/kv/bin/run_kv_persist.c ${EXAMPLES_DIR}

	@#SystemD Daemons
	${INSTALL} -m 644 ${RESOURCEDIR}/cxlfd.service ${SYSTEMD_DIR}

	@#Linux Readme
	${INSTALL} -m 644 ${RESOURCEDIR}/readme.txt ${SHARE_DIR}
endif

cflash-test:
	@#TEST ONLY CONTENT - DO NOT SHIP
	@mkdir -p ${TEST_DIR}
ifneq ($(TARGET_PLATFORM),x86_64)
	${INSTALL} ${TESTSRCDIR}/blk_test ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/pvtestauto ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/pblkread ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/transport_test ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_cflash_fvt ${TEST_DIR}
endif
	${INSTALL} ${TESTSRCDIR}/asyncstress ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/_tst_ark ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/run_fvt_kv ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_fvt ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_block_fvt ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_async_multi ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_io ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_mcio ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_mc_aio ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf2 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf_check ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf_tool ${TEST_DIR}
	${INSTALL} ${ROOTPATH}/src/test/run_ioppts ${TEST_DIR}
	${INSTALL} ${ROOTPATH}/src/test/multi_process_perf ${TEST_DIR}
	${INSTALL} ${ROOTPATH}/src/block/test/block_perf_check ${TEST_DIR}
	${INSTALL} ${ROOTPATH}/src/test/run_regression ${TEST_DIR}
	@#Enable factory flash if the test image is installed
	${INSTALL} ${RESOURCEDIR}/flash_factory_image ${TEST_DIR}


ifeq ($(UNAME),AIX)
	${INSTALL} ${TESTSRCDIR}/asyncstress64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/_tst_ark64 ${BIN_DIR}
	${INSTALL} ${TESTSRCDIR}/blk_test64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/pvtestauto64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/pblkread64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/transport_test64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_cflash_fvt64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_fvt_kv64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_fvt64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_block_fvt64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/run_kv_async_multi64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_io64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_mcio64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_mc_aio64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf_check64 ${TEST_DIR}
	${INSTALL} ${TESTSRCDIR}/fvt_ark_perf_tool64 ${TEST_DIR}
else
ifneq ($(TARGET_PLATFORM),x86_64)
	${INSTALL} -s ${TESTSRCDIR}/cxl_afu_inject ${TEST_DIR}
	${INSTALL} -s ${RESOURCEDIR}/flashgt_nvme_override ${TEST_DIR}
endif
	${INSTALL} ${RESOURCEDIR}/cflash_inject ${TEST_DIR}
	${INSTALL} ${ROOTPATH}/src/test/utils_scripts_tst ${TEST_DIR}
	${INSTALL} -s ${TESTSRCDIR}/run_fvt_scripts ${TEST_DIR}
endif


FW_FILES := $(notdir $(shell ls ${RESOURCEDIR}/BTV*)) $(notdir $(shell ls ${RESOURCEDIR}/1410*))

${FW_FILES}:
#ifneq ($(TARGET_PLATFORM),x86_64)
	@mkdir -p ${AFU_IMAGES_DIR}
ifeq ($(UNAME),AIX)
	@if [[ -e ${RESOURCEDIR}/$@ ]]; then \
         echo ${INSTALL} ${RESOURCEDIR}/$@ ${AFU_IMAGES_DIR}; \
              ${INSTALL} ${RESOURCEDIR}/$@ ${AFU_IMAGES_DIR}; \
         if [[ $@ =~ gz ]]; then gunzip ${AFU_IMAGES_DIR}/$@; fi; \
     fi
else
	@if [[ -e ${RESOURCEDIR}/$@ ]]; then \
         echo ${INSTALL} ${RESOURCEDIR}/$@ ${AFU_IMAGES_DIR}; \
              ${INSTALL} -m 644 ${RESOURCEDIR}/$@ ${AFU_IMAGES_DIR}; \
         if [[ $@ =~ gz ]]; then gunzip ${AFU_IMAGES_DIR}/$@; fi; \
     fi
	@if [[ -e ${AFU_IMAGES_DIR}/$(subst .gz,,$(@)) ]]; then \
         cd ${AFU_IMAGES_DIR}; /usr/bin/md5sum $(subst .gz,,$(@)) >> afu_fw.md5sum; \
         if [[ ! -e $@ ]]; then gzip -n $(subst .gz,,$(@)); fi; \
     fi
#endif
endif

CXLDIR=/usr/lib/powerpc64le-linux-gnu

cflashimage: ${FW_FILES}
	@mkdir -p ${AFU_BIN}
	@mkdir -p ${SHARE_DIR}
	@mkdir -p ${AFU_IMAGES_DIR}
	${INSTALL} ${RESOURCEDIR}/cxlffdc ${AFU_BIN}
ifneq ($(UNAME),AIX)
ifneq ($(TARGET_PLATFORM),x86_64)
	${INSTALL} ${CXLDIR}/$(shell readlink ${CXLDIR}/libcxl.so) ${LIB_DIR}
	${INSTALL} -s ${TESTSRCDIR}/cxl_afu_dump ${AFU_BIN}
	${INSTALL} -s ${TESTSRCDIR}/cxl_afu_status ${AFU_BIN}
	${INSTALL} -s ${TESTSRCDIR}/flashgt_temp ${AFU_BIN}
	${INSTALL} -s ${TESTSRCDIR}/flashgt_vpd_access ${AFU_BIN}
endif
	${INSTALL} -m 644 ${RESOURCEDIR}/afu_versions ${AFU_IMAGES_DIR}/.remove
	${INSTALL} -m 644 ${RESOURCEDIR}/blacklist-cxlflash.conf ${SHARE_DIR}
	${INSTALL} -m 644 ${AFUDIR}/vpd_FlashGT.csv ${SHARE_DIR}
	${INSTALL} -m 644 ${AFUDIR}/vpd_FlashGTplus.csv ${SHARE_DIR}
	${INSTALL} ${AFUDIR}/surelock_vpd2rbf ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/capi_flash ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_reset ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_perst ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_perf ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_capacity ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_devices ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_temp ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_wear ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_stick ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_perfcheck ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cflash_mfg_init ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/flash_all_adapters ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/reload_all_adapters ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/psl_trace_dump ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cxl_flash_vm ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/cxl_afu_fmt_unit ${AFU_BIN}
	${INSTALL} ${RESOURCEDIR}/flashgt_nvme_nsq ${AFU_BIN}
endif

include ${ROOTPATH}/config.mk
