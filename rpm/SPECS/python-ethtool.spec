%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_ver: %define python_ver %(%{__python} -c "import sys ; print sys.version[:3]")}

Summary: Ethernet settings python bindings
Name: python-ethtool
Version: 0.3
Release: 2%{?dist}
URL: http://git.kernel.org/?p=linux/kernel/git/acme/python-ethtool.git
Source: http://userweb.kernel.org/~acme/python-ethtool/%{name}-%{version}.tar.bz2
License: GPLv2
Group: System Environment/Libraries
BuildRequires: python-devel
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Python bindings for the ethtool kernel interface, that allows querying and
changing of ethernet card settings, such as speed, port, autonegotiation, and
PCI locations.

%prep
%setup -q

%build
%{__python} setup.py build

%install
rm -rf %{buildroot}
%{__python} setup.py install --skip-build --root %{buildroot}
mkdir -p %{buildroot}%{_sbindir}
cp -p pethtool.py %{buildroot}%{_sbindir}/pethtool
cp -p pifconfig.py %{buildroot}%{_sbindir}/pifconfig

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING
%{_sbindir}/pethtool
%{_sbindir}/pifconfig
%{python_sitearch}/ethtool.so
%if "%{python_ver}" >= "2.5"
%{python_sitearch}/*.egg-info
%endif

%changelog
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
