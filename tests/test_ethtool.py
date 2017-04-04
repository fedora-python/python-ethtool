# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#   Copyright (c) 2011, 2012, 2013  Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
#
#   Author: Dave Malcolm <dmalcolm@redhat.com>
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import unittest

import ethtool

from .parse_ifconfig import IfConfig

INVALID_DEVICE_NAME = "I am not a valid device name"

# Screenscrape the output from "ifconfig".  We will use this below to validate
# the results from the "ethtool" module:
ifconfig = IfConfig()
for dev in ifconfig.devices:
    print(dev)


class EthtoolTests(unittest.TestCase):

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #  asserts
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def assertIsString(self, value):
        self.assertTrue(isinstance(value, str))

    def assertIsStringOrNone(self, value):
        if value is not None:
            self.assertTrue(isinstance(value, str))

    def assertIsInt(self, value):
        self.assertTrue(isinstance(value, int))

    def assertRaisesIOError(self, fn, args, errmsg):
        """
        Verify that an IOError is raised, and that the errno and message are
        as expected

        (Python 2.6 and earlier's assertRaises doesn't allow us to check the
        details of the exception that is raised)
        """
        try:
            fn(*args)
        except IOError as e:
            # Check the details of the exception:
            enum, emsg = e.args
            self.assertEqual('[Errno {}] {}'.format(enum, emsg), errmsg)
        else:
            self.fail('IOError was not raised calling %s on %s' % (fn, args))

    def assertRaisesNoSuchDevice(self, fn, *args):
        self.assertRaisesIOError(fn, args, '[Errno 19] No such device')

    def assertIsStringExceptForLoopback(self, fn, devname, errmsg):
        if devname == 'lo':
            self.assertRaisesIOError(fn, (devname, ), errmsg)
        else:
            self.assertIsString(fn(devname))

    def assertEqualIpv4Str(self, ethtooladdr, scrapedaddr):
        if scrapedaddr is None:
            self.assertEqual(ethtooladdr, '0.0.0.0')
        else:
            self.assertEqual(ethtooladdr, scrapedaddr)

    def assertEqualHwAddr(self, ethtooladdr, scrapedaddr):
        if scrapedaddr is None:
            self.assertEqual(ethtooladdr, '00:00:00:00:00:00')
        else:
            self.assertEqual(ethtooladdr, scrapedaddr.lower())

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #  helpers
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def _functions_accepting_devnames(self, devname):
        self.assertIsString(devname)

        scraped = ifconfig.get_device_by_name(devname)

        try:
            broadcast = ethtool.get_broadcast(devname)
        except (OSError, IOError):
            # Some devices might end up with
            # [Errno 99] Cannot assign requested address
            # That's IOError on 2.7, OSError on 3
            assignable = False
        else:
            assignable = True
            self.assertIsString(broadcast)

            # Broadcast is optional in ifconfig output
            if scraped.broadcast:
                self.assertEqualIpv4Str(broadcast, scraped.broadcast)

        self.assertIsStringExceptForLoopback(ethtool.get_businfo, devname,
                                             '[Errno 95] Operation not '
                                             'supported')

        self.assertIsInt(ethtool.get_flags(devname))
        # flagsint cannot be obtained from old ifconfig format
        if not ifconfig.oldFormat:
            self.assertEqual(ethtool.get_flags(devname), scraped.flagsint)
        self.assertIsInt(ethtool.get_gso(devname))
        self.assertIsString(ethtool.get_hwaddr(devname))
        self.assertEqualHwAddr(ethtool.get_hwaddr(devname),
                               scraped.hwaddr)

        if assignable:
            self.assertIsString(ethtool.get_ipaddr(devname))
            self.assertEqual(ethtool.get_ipaddr(devname), scraped.inet)

        self.assertIsStringExceptForLoopback(ethtool.get_module, devname,
                                             '[Errno 95] Operation not '
                                             'supported')

        if assignable:
            self.assertIsString(ethtool.get_netmask(devname))
            self.assertEqual(ethtool.get_netmask(devname),
                             scraped.netmask)

        # Operation is not supported only on loopback device
        if devname == 'lo':
            self.assertRaisesIOError(ethtool.get_ringparam, (devname, ),
                                     '[Errno 95] Operation not supported')

        self.assertIsInt(ethtool.get_sg(devname))
        self.assertIsInt(ethtool.get_ufo(devname))

        self.assertIsInt(ethtool.get_tso(devname))

        # TODO: self.assertIsString(ethtool.set_coalesce(devname))

        # TODO: self.assertIsString(ethtool.set_ringparam(devname))

        # TODO: self.assertIsString(ethtool.set_tso(devname))

    def _verify_etherinfo_object(self, ei):
        self.assertTrue(isinstance(ei, ethtool.etherinfo))
        self.assertIsString(ei.device)

        try:
            scraped = ifconfig.get_device_by_name(ei.device)
        except ValueError:
            scraped = None

        self.assertIsStringOrNone(ei.ipv4_address)
        if scraped:
            self.assertEqual(ei.ipv4_address, scraped.inet)

        self.assertIsStringOrNone(ei.ipv4_broadcast)
        if scraped and scraped.broadcast:
            # Broadcast is optional
            self.assertEqual(ei.ipv4_broadcast, scraped.broadcast)

        self.assertIsInt(ei.ipv4_netmask)
        if scraped:
            self.assertEqual(ei.ipv4_netmask, scraped.get_netmask_bits())

        self.assertIsStringOrNone(ei.mac_address)
        if scraped and scraped.hwaddr and scraped.hwtitle.lower() != 'unspec':
            scraped.hwaddr = scraped.hwaddr.lower()
            self.assertEqualHwAddr(ei.mac_address.lower(), scraped.hwaddr)

        i6s = ei.get_ipv6_addresses()
        for i6 in i6s:
            self.assertTrue(isinstance(i6, ethtool.NetlinkIPaddress))
            self.assertIsString(i6.address)
            self.assertIsInt(i6.netmask)
            self.assertIsString(i6.scope)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #  tests
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def test_invalid_devices(self):
        # Verify sane handling of non-existant devices

        get_fns = ('get_broadcast', 'get_businfo', 'get_coalesce', 'get_flags',
                   'get_gso', 'get_hwaddr', 'get_ipaddr', 'get_module',
                   'get_netmask', 'get_ringparam', 'get_sg', 'get_tso',
                   'get_ufo')
        for fnname in get_fns:
            self.assertRaisesNoSuchDevice(getattr(ethtool, fnname),
                                          INVALID_DEVICE_NAME)

        set_fns = ('set_coalesce', 'set_ringparam', 'set_tso')
        for fnname in set_fns:
            # Currently this fails, with an IOError from
            #   ethtool.c:__struct_desc_from_dict
            # with message:
            #   'Missing dict entry for field rx_coalesce_usecs'
            if False:
                self.assertRaisesNoSuchDevice(getattr(ethtool, fnname),
                                              INVALID_DEVICE_NAME, 42)

    def test_get_interface_info_invalid(self):
        eis = ethtool.get_interfaces_info(INVALID_DEVICE_NAME)
        self.assertEqual(len(eis), 1)
        ei = eis[0]
        self.assertEqual(ei.device, INVALID_DEVICE_NAME)
        self.assertRaisesIOError(
            getattr, (ei, 'ipv4_address'), '[Errno 19] No such device')
        self.assertRaisesIOError(
            getattr, (ei, 'ipv4_netmask'), '[Errno 19] No such device')
        self.assertRaisesIOError(
            getattr, (ei, 'ipv4_broadcast'), '[Errno 19] No such device')
        self.assertRaisesIOError(
            getattr, (ei, 'mac_address'), '[Errno 19] No such device')

    def test_get_interface_info_active(self):
        eis = ethtool.get_interfaces_info(ethtool.get_active_devices())
        for ei in eis:
            self._verify_etherinfo_object(ei)

    def test_get_interface_info_all(self):
        eis = ethtool.get_interfaces_info(ethtool.get_devices())
        for ei in eis:
            self._verify_etherinfo_object(ei)

    def test_get_active_devices(self):
        for devname in ethtool.get_active_devices():
            # Skip these test on tun devices
            if devname.startswith('tun'):
                continue
            self._functions_accepting_devnames(devname)

    def test_etherinfo_objects(self):
        devnames = ethtool.get_devices()
        eis = ethtool.get_interfaces_info(devnames)
        for ei in eis:
            self._verify_etherinfo_object(ei)


if __name__ == '__main__':
    unittest.main()
