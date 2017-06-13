Changelog
=========

0.13
----
Tue Jun 13 2017 Miro Hrončok <mhroncok@redhat.com>

- First release on PyPI
- Supports both Python 2.7 and 3.5+
- Dropped support for Python 2.6
- Upstream URL changed to https://github.com/fedora-python/python-ethtool
- Introduced a basic README file
- PEP7 and PEP8 (code style) improvements
- Fix compilation errors on modern Fedoras

0.12
----
Tue Mar 21 2017 Charalampos Stratakis <cstratak@redhat.com>

- First attempt at python3 support

0.11
----
Thu May 8 2014 David Sommerseth <davids@redhat.com>

- Improved error handling several places
- Ensure that we get a valid libnl NETLINK connection when connecting
- URL updates to SPEC file

0.10
----
Fri Jan 10 2014 David Sommerseth <davids@redhat.com>

- Not really a full release, but a preliminary release to get more wide testing
- FSF Copyright updates
- Build fixes
- Mostly code cleanup

0.9
---
Wed Dec 11 2013 David Sommerseth <davids@redhat.com>

- Fixed get_active_devices() for IPv6 only interfaces
- Moved from libnl1 to libnl3
- Refactor PyNetlink*Address implementation

0.8
---
Tue Feb 19 2013 David Malcolm <dmalcolm@redhat.com>

- Enable IPv6 in pifethtool example
- Code cleanup, fixing buffer overflows, memory leaks, etc

0.7
---
Mon Apr 11 2011 David Sommerseth <davids@redhat.com>

- Fixed several memory leaks (commit aa2c20e697af, abc7f912f66d)
- Improved error checking towards NULL values(commit 4e928d62a8e3)
- Fixed typo in pethtool --help (commit 710766dc722)
- Only open a NETLINK connection when needed (commit 508ffffbb3c)
- Added man page for pifconfig and pethtool (commit 9f0d17aa532, rhbz#638475)
- Force NETLINK socket to close on fork() using FD_CLOEXEC (commit 1680cbeb40e)

0.6
---
Wed Jan 19 2011 David Sommerseth <davids@redhat.com>

- Don't segfault if we don't receive any address from rtnl_link_get_addr()
- Remove errornous file from MANIFEST
- Added ethtool.version string constant
- Avoid duplicating IPv6 address information
- import sys module in setup.py (Miroslav Suchy)

0.5
---
Mon Aug  9 2010 David Sommerseth <davids@redhat.com>

- Fixed double free issue (commit c52ed2cbdc5b851ebc7b)

0.4
---
Wed Apr 28 2010 David Sommerseth <davids@redhat.com>

- David Sommerseth is now taking over the maintenance of python-ethtool
- New URLs for upstream source code
- Added new API: ethtool.get_interfaces_info() - returns list of etherinfo objects
- Added support retrieving for IPv6 address, using etherinfo::get_ipv6_addresses()

0.3
---
Tue Aug 26 2008 Arnaldo Carvalho de Melo <acme@redhat.com>

- Add get_flags method from the first python-ethtool contributor, yay
- Add pifconfig command, that mimics the ifconfig tool using the
  bindings available

0.2
---
Wed Aug 20 2008 Arnaldo Carvalho de Melo <acme@redhat.com>

- Expand description and summary fields, as part of the fedora
  review process.

0.1
---
Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com>

- Get ethtool code from rhpl 0.212
