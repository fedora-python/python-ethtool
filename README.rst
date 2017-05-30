Python ethtool module
=====================

*Python bindings for the ethtool kernel interface*

The Python ``ethtool`` module allows querying and changing of ethernet card settings,
such as speed, port, autonegotiation, and PCI locations.

**This is the new upstream for python-ethtool maintained by Fedora's
Python SIG.**

Usage
-----

``ethtool`` may be used as a Python library::

    >>> import ethtool
    >>> ethtool.get_active_devices()
    ['lo', 'enp0s31f6', 'wlp4s0', 'virbr0', 'docker0', 'virbr1', 'eth0', 'tun0']
    >>> ethtool.get_ipaddr('lo')
    '127.0.0.1'

The ``ethtool`` package also provides the ``pethtool`` and ``pifconfig`` utilities.  More example usage may be gathered from their sources,
`pethtool.py <https://github.com/fedora-python/python-ethtool/blob/master/scripts/pethtool>`_
and
`pifconfig.py <https://github.com/fedora-python/python-ethtool/blob/master/scripts/pethtool>`_.


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

Tests may be run by ``tox``.

Authors
-------

* Andy Grover
* Antoni S. Puimedon
* Arnaldo Carvalho de Melo
* Bohuslav Kabrda
* Braňo Náter
* Dave Malcolm
* David S. Miller
* David Sommerseth
* Harald Hoyer
* Charalampos Stratakis
* Jeff Garzik
* Lumir Balhar
* Miro Hrončok
* Miroslav Suchý
* Ruben Kerkhof
* Sanqui
* Yaakov Selkowitz

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

The Python ``ethtool`` project is free software distributed under the terms of
the GNU General Public License v2.0, see
`COPYING <https://github.com/fedora-python/python-ethtool/blob/master/COPYING>`_.

