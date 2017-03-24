python-ethtool
==============

*Python bindings for the ethtool kernel interface*

``python-ethtool`` allows querying and changing of ethernet card settings,
such as speed, port, autonegotiation, and PCI locations.

**This is the new upstream for python-ethtool maintained by Fedora's
Python SIG.**

Usage
-----

``pethtool`` mimics behavior of the ``ethtool`` utility, but does not
support all options.

e.g., to get driver information on the ``eth0`` interface::

    $ pethtool -i eth0
    driver: cdc_ether
    bus-info: usb-0000:00:14.0-4.1.3

Analogically, ``pifconfig`` mimics ``ifconfig`` in usage.  It may be
used to view information on an interface::

    $ pifconfig lo
    lo        
          inet addr:127.0.0.1   Mask:255.0.0.0
	      inet6 addr: ::1/128 Scope: host
	      UP LOOPBACK RUNNING


Further usage information may be found in the respective manpages for
`pethtool <https://github.com/fedora-python/python-ethtool/blob/master/man/pethtool.8.asciidoc>`_
and
`pifconfig <https://github.com/fedora-python/python-ethtool/blob/master/man/pifconfig.8.asciidoc>`_.

Tests
-----

Tests may be run by ``tox``.  However, they're currently failing, see
`issue #11 <https://github.com/fedora-python/python-ethtool/issues/11>`_.

Authors
-------

* Harald Hoyer
* Arnaldo Carvalho de Melo
* David Sommerseth

Current maintainers
-------------------

* Lumír Balhar <lbalhar@redhat.com>
* Miro Hrončok <mhroncok@redhat.com>
* Charalampos Stratakis <cstratak@redhat.com>
* Sanqui <dlabsky@redhat.com>

Contributing
------------

Feel free to help us with improving test coverage, porting to Python 3
or anything else.
Issues and PRs `on GitHub <https://github.com/fedora-python/python-ethtool>`_
are welcome.

License
-------

``python-ethtool`` is free software put under the
GNU General Public License v2.0, see
`COPYING <https://github.com/fedora-python/python-ethtool/blob/master/COPYING>`_.

