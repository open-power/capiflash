# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/packaging/makefile $
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
ifeq ($(UNAME),AIX)
all: aixcapikv aixcapikv-test aixafuimage 
else
all: ibmcapikv ibmcapikv-test afuimage
endif

VERSIONMAJOR=2
VERSIONMINOR=0
VERSION=${VERSIONMAJOR}.${VERSIONMINOR}-${GITREVISION}

ibmcapikv:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	fpm -f -s dir -t rpm -n $@ -v ${VERSION} \
		--depends 'advance-toolchain-at7.1-runtime' \
		--depends 'libudev1' \
		-C ./install_root \
		--after-install ${SURELOCKROOT}/src/build/install/resources/postinstall .; \
	fpm -f -s dir -t deb -n $@ -v ${VERSION} \
		--depends 'advance-toolchain-at7.1-runtime' \
		--depends 'libudev1' \
		-C ./install_root \
		--after-install ${SURELOCKROOT}/src/build/install/resources/postinstall .; \
	tar -cvzf $@-${GITREVISION}.tar.gz -C ./install_root .

ibmcapikv-test:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	fpm -f -s dir -t rpm -n $@ -v ${VERSION} -C ./test_root \
		--depends 'ibmcapikv = ${VERSION}' \
		--depends 'libudev-dev' .; \
	fpm -f -s dir -t deb -n $@ -v ${VERSION} -C ./test_root \
		--depends 'ibmcapikv = ${VERSION}' \
		--depends 'libudev-dev' .; \
	tar -cvzf $@-${GITREVISION}.tar.gz -C ./test_root .

afuimage:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	fpm -f -s dir -t deb -a all -n $@ -v ${VERSION} -C ./afu_root \
		--after-install ${SURELOCKROOT}/src/build/install/resources/postafuinstall .

aixcapikv:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	tar -cvf ibmcapikv_${VERSION}.ppc64be.tar -C ./install_root .

aixcapikv-test:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	tar -cvf ibmcapikv-test_${VERSION}.ppc64be.tar -C ./test_root .

aixafuimage:
	mkdir -p ${PKGDIR}
	cd ${PKGDIR}; \
	tar -cvf afuimage_${VERSION}.ppc64be.tar -C ./afu_root .

include ${ROOTPATH}/config.mk
