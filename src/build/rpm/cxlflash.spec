Name:           cxlflash

#set CXLFLASH_TEST=yes to build the cxlflash-test package
%define cxlflash_test %{getenv:CXLFLASH_TEST}

#set TARGET_VERSION=x.y.rev to use a specific tarball
%define tversion %{getenv:TARGET_VERSION}

%if "NULL%{tversion}" == "NULL"
%define version 4.3.2534
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
%{_bindir}/flash_all_adapters
%{_bindir}/reload_all_adapters
%{_bindir}/flashgt_vpd_access
%{_bindir}/surelock_vpd2rbf
%{_prefix}/include/*
%{_prefix}/share/*

%post
%{_libdir}/cxlflash/ext/postinstall
echo "INFO: enabling cxlflash service for LUN Management"
systemctl enable cxlflash || echo "WARNING: Unable to enable the cxlflash service via systemctl. Please enable the cxlflash daemon for LUN management."
systemctl start cxlflash || echo "WARNING: Unable to start the cxlflash service via systemctl. Please enable the cxlflash daemon for LUN management."

%preun
%{_libdir}/cxlflash/ext/preremove
systemctl stop cxlflash || echo "WARNING: Unable to start the cxlflash service via systemctl. Please enable the cxlflash daemon for LUN management."

%if "%{cxlflash_test}" == "yes"
%package -n cxlflash-test
Summary: IBM Data Engine for NoSQL Software Libraries : test support
Requires: cxlflash
%description -n cxlflash-test
%files -n cxlflash-test
%{_bindir}/*fvt*
%{_bindir}/blk_test
%{_bindir}/pvtestauto
%{_bindir}/pblkread
%{_bindir}/transport_test
%{_bindir}/asyncstress
%{_bindir}/_tst_ark
%{_bindir}/run_ioppts
%{_bindir}/multi_process_perf
%{_bindir}/block_perf_check
%{_bindir}/run_regression
%{_bindir}/cflash_inject
%{_bindir}/utils_scripts_tst

%endif

%changelog
* Wed Aug 2 2017 Mike Vageline <mpvagli@us.ibm.com> 4.3.2507-1
- Initial RPM release