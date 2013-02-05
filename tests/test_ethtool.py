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

import sys
import unittest
from test.test_support import run_unittest # requires python-test subpackage on Fedora/RHEL

import ethtool

from parse_ifconfig import IfConfig

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
        self.assert_(isinstance(value, str))

    def assertIsStringOrNone(self, value):
        if value is not None:
            self.assert_(isinstance(value, str))

    def assertIsInt(self, value):
        self.assert_(isinstance(value, int))

    def assertRaisesIOError(self, fn, args, errmsg):
        """
        Verify that an IOError is raised, and that the errno and message are
        as expected

        (Python 2.6 and earlier's assertRaises doesn't allow us to check the
        details of the exception that is raised)
        """
        try:
            fn(*args)
        except IOError, e:
            # Check the details of the exception:
            self.assertEquals(e.args, (errmsg, ))
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

            self.assertIsString(ethtool.get_broadcast(devname))
            self.assertEqualIpv4Str(ethtool.get_broadcast(devname),
                                    scraped.broadcast)

            self.assertIsStringExceptForLoopback(ethtool.get_businfo, devname,
                                                 '[Errno 95] Operation not supported')

            # ethtool.get_coalesce(devname)
            # this gives me:
            #   IOError: [Errno 95] Operation not supported
            # on my test box

            self.assertIsInt(ethtool.get_flags(devname))
            # flagsint cannot be obtained from old ifconfig format
            if not ifconfig.oldFormat:
                self.assertEqual(ethtool.get_flags(devname), scraped.flagsint)
            self.assertIsInt(ethtool.get_gso(devname))
    
            self.assertIsString(ethtool.get_hwaddr(devname))
            self.assertEqualHwAddr(ethtool.get_hwaddr(devname),
                                   scraped.hwaddr)
            self.assertIsString(ethtool.get_ipaddr(devname))
            self.assertEqual(ethtool.get_ipaddr(devname), scraped.inet)

            self.assertIsStringExceptForLoopback(ethtool.get_module, devname,
                                                 '[Errno 95] Operation not supported')
    
            self.assertIsString(ethtool.get_netmask(devname))
            self.assertEqual(ethtool.get_netmask(devname),
                             scraped.netmask)
    
            #self.assertRaisesIOError(ethtool.get_ringparam, (devname, ),
            #                         '[Errno 95] Operation not supported')
    
            # Disabling until BZ#703089 is investigated
            #self.assertIsInt(ethtool.get_sg(devname))
            #self.assertIsInt(ethtool.get_ufo(devname))
    
            self.assertIsInt(ethtool.get_tso(devname))
    
    
            #TODO: self.assertIsString(ethtool.set_coalesce(devname))
    
            #TODO: self.assertIsString(ethtool.set_ringparam(devname))

            #TODO: self.assertIsString(ethtool.set_tso(devname))

    def _verify_etherinfo_object(self, ei):
        self.assert_(isinstance(ei, ethtool.etherinfo))
        self.assertIsString(ei.device)

        try:
            scraped = ifconfig.get_device_by_name(ei.device)
        except ValueError:
            scraped = None

        self.assertIsStringOrNone(ei.ipv4_address)
        if scraped:
            self.assertEqual(ei.ipv4_address, scraped.inet)

        self.assertIsStringOrNone(ei.ipv4_broadcast)
        if scraped:
            self.assertEqual(ei.ipv4_broadcast, scraped.broadcast)

        self.assertIsInt(ei.ipv4_netmask)
        if scraped:
            self.assertEqual(ei.ipv4_netmask, scraped.get_netmask_bits())

        self.assertIsStringOrNone(ei.mac_address)
        if scraped:
            if scraped.hwaddr:
                scraped.hwaddr = scraped.hwaddr.lower()
            self.assertEqualHwAddr(ei.mac_address.lower(), scraped.hwaddr)

        i6s = ei.get_ipv6_addresses()
        for i6 in i6s:
            self.assert_(isinstance(i6, ethtool.etherinfo_ipv6addr))
            self.assertIsString(i6.address)
            self.assertIsInt(i6.netmask)
            self.assertIsString(i6.scope)
            self.assertEquals(str(i6),
                              '[%s] %s/%i' % (i6.scope, 
                                              i6.address,
                                              i6.netmask))

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
        self.assertEquals(len(eis), 1)
        ei = eis[0]
        self.assertEquals(ei.device, INVALID_DEVICE_NAME)
        self.assertEquals(ei.ipv4_address, None)
        self.assertEquals(ei.ipv4_broadcast, None)
        self.assertEquals(ei.ipv4_netmask, 0)
        self.assertEquals(ei.mac_address, None)

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
            self._functions_accepting_devnames(devname)
                       
    def test_etherinfo_objects(self):
        devnames = ethtool.get_devices()
        eis = ethtool.get_interfaces_info(devnames)
        for ei in eis:
            self._verify_etherinfo_object(ei)


if __name__ == '__main__':
    try:
        # if count provided do a soak test, to detect leaking resources
        count = int(sys.argv[1])
        for i in range(count):
            run_unittest(EthtoolTests)
            if i % (count / 10) == 0:
                sys.stderr.write("%s %%\n" % (i * 100 / count))
    except:
        run_unittest(EthtoolTests)
