python-ethtool
==============

*Python bindings for the ethtool kernel interface*

**This is the new upstream for python-ethtool maintained by Fedora's
Python SIG.**

About
-----

Allows querying and changing of ethernet card settings, such as speed,
port, autonegotiation, and PCI locations.

Usage
-----

``pethtool`` mimics behavior of the ``ethtool`` utility, but does not
support all options.

e.g., to get driver information on the ``eth0`` interface::

    $ pethtool -i eth0
    driver: cdc_ether
    bus-info: usb-0000:00:14.0-4.1.3

Further usage information may be found in the ``pethtool(8)`` manpage.

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

Contributing
------------

Feel free to help us with improving test coverage, porting to Python 3
or anything else. Issues and PRs are welcome.
