Summary:   Steve's DSP library for telephony spans.
Name:      spandsp
Version:   0.0.3
Release:   1
License:   GPL
URL:       http://www.soft-switch.org/spandsp
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Source:    http://www.soft-switch.org/spandsp/spandsp-0.0.3.tar.gz

Group:     System Environment/Libraries
Obsoletes: spandsp
Docdir:    %{_prefix}/doc

%description
spandsp is a library for DSP in telephony spans. It can perform many of the
common DSP functions, such as the generation and detection of DTMF and
supervisory tones.

%package devel
Summary:   header files and libraries needed for development with spandsp.
Group:     Development/Libraries
Requires:  spandsp = %{version}
Obsoletes: spandsp-devel
PreReq:    /sbin/install-info

%description devel
This package includes the header files and libraries needed for
developing programs using spandsp.

%prep
%setup

automake
%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%_prefix --sysconfdir=/etc
make

%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT/%{_prefix} sysconfdir=$RPM_BUILD_ROOT/etc install

%clean
rm -rf $RPM_BUILD_ROOT

%post   -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%{_prefix}/lib/lib*.so.*

%files devel
%defattr(-, root, root)

%{_prefix}/lib/lib*.a
%{_prefix}/lib/lib*.la
%{_prefix}/lib/lib*.so
%{_prefix}/include/*

%{_prefix}/share/%{name}/global-tones.xml
%{_prefix}/share/%{name}/tones.dtd

%changelog
%changelog
* Sat Oct 16 2004 Steve Underwood <steveu@coppice.org> 0.0.2-1
- Preparing for 0.0.2 release

* Thu Apr 15 2004 Steve Underwood <steveu@coppice.org> 0.0.1-1
- Initial version
