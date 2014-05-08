%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_ver: %define python_ver %(%{__python} -c "import sys ; print sys.version[:3]")}

Summary: Ethernet settings python bindings
Name: python-ethtool
Version: 0.11
Release: 1%{?dist}
URL: https://fedorahosted.org/python-ethtool/
Source: https://fedorahosted.org/releases/p/y/python-ethtool/python-ethtool-%{version}.tar.bz2
License: GPLv2
Group: System Environment/Libraries
BuildRequires: python-devel libnl3-devel asciidoc
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Python bindings for the ethtool kernel interface, that allows querying and
changing of Ethernet card settings, such as speed, port, auto-negotiation, and
PCI locations.

%prep
%setup -q

%build
%{__python} setup.py build
a2x -d manpage -f manpage man/pethtool.8.asciidoc
a2x -d manpage -f manpage man/pifconfig.8.asciidoc

%install
rm -rf %{buildroot}
%{__python} setup.py install --skip-build --root %{buildroot}
mkdir -p %{buildroot}%{_sbindir}  %{buildroot}%{_mandir}/man8
cp -p pethtool.py %{buildroot}%{_sbindir}/pethtool
cp -p pifconfig.py %{buildroot}%{_sbindir}/pifconfig
%{__gzip} -c man/pethtool.8 > %{buildroot}%{_mandir}/man8/pethtool.8.gz
%{__gzip} -c man/pifconfig.8 > %{buildroot}%{_mandir}/man8/pifconfig.8.gz

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING
%{_sbindir}/pethtool
%{_sbindir}/pifconfig
%doc %{_mandir}/man8/*
%{python_sitearch}/ethtool.so
%if "%{python_ver}" >= "2.5"
%{python_sitearch}/*.egg-info
%endif

%changelog
* Thu May 8 2014 David Sommerseth <davids@redhat.com> - 0.11-1
- Improved error handling several places
- Ensure that we get a valid libnl NETLINK connection when connecting
- URL updates to SPEC file

* Fri Jan 10 2014 David Sommerseth <davids@redhat.com> - 0.10-1
- Not really a full release, but a preliminary release to get more wide testing
- FSF Copyright updates
- Build fixes
- Mostly code cleanup

* Wed Dec 11 2013 David Sommerseth <davids@redhat.com> - 0.9-1
- Fixed get_active_devices() for IPv6 only interfaces
- Moved from libnl1 to libnl3
- Refactor PyNetlink*Address implementation

* Tue Feb 19 2013 David Malcolm <dmalcolm@redhat.com> - 0.8-1
- Enable IPv6 in pifethtool example
- Code cleanup, fixing buffer overflows, memory leaks, etc

* Mon Apr 11 2011 David Sommerseth <davids@redhat.com> - 0.7-1
- Fixed several memory leaks (commit aa2c20e697af, abc7f912f66d)
- Improved error checking towards NULL values(commit 4e928d62a8e3)
- Fixed typo in pethtool --help (commit 710766dc722)
- Only open a NETLINK connection when needed (commit 508ffffbb3c)
- Added man page for pifconfig and pethtool (commit 9f0d17aa532, rhbz#638475)
- Force NETLINK socket to close on fork() using FD_CLOEXEC (commit 1680cbeb40e)

* Wed Jan 19 2011 David Sommerseth <davids@redhat.com> - 0.6-1
- Don't segfault if we don't receive any address from rtnl_link_get_addr()
- Remove errornous file from MANIFEST
- Added ethtool.version string constant
- Avoid duplicating IPv6 address information
- import sys module in setup.py (Miroslav Suchy)

* Mon Aug  9 2010 David Sommerseth <davids@redhat.com> - 0.5-1
- Fixed double free issue (commit c52ed2cbdc5b851ebc7b)

* Wed Apr 28 2010 David Sommerseth <davids@redhat.com> - 0.4-1
- David Sommerseth is now taking over the maintenance of python-ethtool
- New URLs for upstream source code
- Added new API: ethtool.get_interfaces_info() - returns list of etherinfo objects
- Added support retrieving for IPv6 address, using etherinfo::get_ipv6_addresses()

* Fri Sep  5 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.3-2
- Rewrote build and install sections as part of the fedora review process
  BZ #459549

* Tue Aug 26 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.3-1
- Add get_flags method from the first python-ethtool contributor, yay
- Add pifconfig command, that mimics the ifconfig tool using the
  bindings available

* Wed Aug 20 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.2-1
- Expand description and summary fields, as part of the fedora
  review process.

* Tue Jun 10 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-3
- add dist to the release tag

* Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-2
- First build into MRG repo

* Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-1
- Get ethtool code from rhpl 0.212
