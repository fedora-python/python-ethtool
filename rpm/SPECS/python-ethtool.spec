%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_ver: %define python_ver %(%{__python} -c "import sys ; print sys.version[:3]")}

Summary: ethtool python bindings
Name: python-ethtool
Version: 0.2
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.bz2
License: GPLv2+
Group: System Environment/Libraries
BuildRequires: python-devel
BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
ethtool python bindings

%prep
%setup -q

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}%{_sbindir}
cp -f ethtool-cmd.py %{buildroot}%{_sbindir}/pethtool

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING
%{_sbindir}/pethtool
%{python_sitearch}/ethtool.so
%if "%{python_ver}" >= "2.5"
%{python_sitearch}/*.egg-info
%endif

%changelog
* Tue Jun 10 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-3
- add dist to the release tag

* Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-2
- First build into MRG repo

* Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-1
- Get ethtool code from rhpl 0.212
