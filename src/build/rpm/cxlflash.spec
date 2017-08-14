Name:           cxlflash

#set CXLFLASH_TEST=yes to build the cxlflash-test package
%define cxlflash_test %{getenv:CXLFLASH_TEST}

#set TARGET_VERSION=x.y.rev to use a specific tarball
%define tversion %{getenv:TARGET_VERSION}

%if "NULL%{tversion}" == "NULL"
%define version 4.3.2509
%else
%define version %{tversion}
%endif

Version:        %{version}
Release:        1%{?dist}
Summary:        IBM Data Engine for NoSQL Software Libraries : runtime support

License:        ASL 2.0
URL:            https://github.com/open-power/capiflash
Source0:        https://github.com/open-power/capiflash/archive/%{version}.tar.gz

BuildRoot:     %{buildroot}
BuildRequires: libcxl libcxl-devel systemd libgudev1 help2man doxygen
#Requires:

%description
IBM Data Engine for NoSQL Software Libraries : runtime support

%clean
echo "NO CLEAN"

%prep
%setup -q

%build
export USE_ADVANCED_TOOLCHAIN=no
source env.bash
make cleanall
%if "%{cxlflash_test}" == "yes"
make buildall %{?_smp_mflags}
%else
make %{?_smp_mflags}
%endif

%install
export QA_RPATHS=$[ 0x0001|0x0010 ]
source env.bash
make install_code
%if "%{cxlflash_test}" == "yes"
make install_test
%endif

%files
%{_unitdir}/*
%{_prefix}/lib/udev/*
%{_libdir}/cxlflash/*
%{_libdir}/libafu*
%{_libdir}/libark*
%{_libdir}/libcfl*
%{_libdir}/libprov*
%{_bindir}/afucfg
%{_bindir}/block*
%{_bindir}/cablecheck
%{_bindir}/capi_flash
%{_bindir}/cflash_*
%{_bindir}/cflashutils
%{_bindir}/cxl*
%{_bindir}/flashgt_temp
%{_bindir}/ioppt
%{_bindir}/ioppts.qd
%{_bindir}/kv_perf
%{_bindir}/machine_info
%{_bindir}/provtool
%{_bindir}/run_kv_*
%{_prefix}/include/*
%{_datadir}/cxlflash/*
%{_mandir}/man1/*

%post
%{_libdir}/cxlflash/ext/postinstall
echo "INFO: enabling cxlfd service for LUN Management"
systemctl enable cxlfd || echo "WARNING: Unable to enable the cxlfd service via systemctl. Please enable the cxlfd daemon for LUN management."
systemctl start cxlfd || echo "WARNING: Unable to start the cxlfd service via systemctl. Please enable the cxlfd daemon for LUN management."

%preun
%{_libdir}/cxlflash/ext/preremove
systemctl stop cxlfd || echo "WARNING: Unable to start the cxlfd service via systemctl. Please enable the cxlfd daemon for LUN management."

%package -n cxlflashimage
Summary: IBM Data Engine for NoSQL Software Libraries : firmware image support
Requires: cxlflash

%description -n cxlflashimage
IBM Data Engine for NoSQL Software Libraries : firmware image support

%files -n cxlflashimage
%{_prefix}/lib/firmware/cxlflash/*
%{_bindir}/flash_all_adapters
%{_bindir}/reload_all_adapters
%{_bindir}/flashgt_vpd_access
%{_bindir}/surelock_vpd2rbf
%{_mandir}/man1/flash_all_adapters.1.gz
%{_mandir}/man1/reload_all_adapters.1.gz
%{_mandir}/man1/flashgt_vpd_access.1.gz
%{_mandir}/man1/surelock_vpd2rbf.1.gz

%post -n cxlflashimage
%{_libdir}/cxlflash/ext/postafuinstall

%preun -n cxlflashimage
%{_libdir}/cxlflash/ext/preafuremove

%if "%{cxlflash_test}" == "yes"
%package -n cxlflash-test
Summary: IBM Data Engine for NoSQL Software Libraries : test support
Requires: cxlflash
%description -n cxlflash-test
%files -n cxlflash-test
%{_bindir}/*
%endif

%changelog
* Wed Aug 2 2017 Mike Vageline <mpvagli@us.ibm.com> 4.3.2509-1
- Initial RPM release