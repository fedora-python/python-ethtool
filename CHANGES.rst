Changelog
=========

Changelog from original RPM specfile
------------------------------------
::

    * Tue Mar 21 2017 Charalampos Stratakis <cstratak@redhat.com> - 0.12-1
    - Release 0.12
    - First attempt at python3 support

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
    - BZ #459549

    * Tue Aug 26 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.3-1
    - Add get_flags method from the first python-ethtool contributor, yay
    - Add pifconfig command, that mimics the ifconfig tool using the
    - bindings available

    * Wed Aug 20 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.2-1
    - Expand description and summary fields, as part of the fedora
    - review process.

    * Tue Jun 10 2008 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-3
    - add dist to the release tag

    * Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-2
    - First build into MRG repo

    * Tue Dec 18 2007 Arnaldo Carvalho de Melo <acme@redhat.com> - 0.1-1
    - Get ethtool code from rhpl 0.212
