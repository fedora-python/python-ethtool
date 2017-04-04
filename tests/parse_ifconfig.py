# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#   Copyright (c) 2012 Red Hat, Inc. All rights reserved.
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
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Screenscraper for the output of "ifconfig"
# The relevant sources for ifconfig can be found in e.g.:
#   net-tools-1.60/lib/interface.c
# within ife_print_long()

import os
import re
from subprocess import Popen, PIPE
import unittest

__all__ = ['IfConfig']

# any whitespace:
ws = '\s+'

# any non-whitespace, as a delimited group:
group_nonws = '(\S+)'

# a decimal number, as a delimited group:
group_dec = '([0-9]+)'

# hexadecimal number of the form "0xf2600000"
group_o_hex = '0x([0-9a-f]+)'

# hexadecimal number without the "0x" prefix, e.g. "f2600000"
group_hex = '([0-9a-f]+)'

dot = r'\.'

dec = '[0-9]+'

group_ip4addr = ('(' + dec + dot +
                 dec + dot +
                 dec + dot +
                 dec + ')')


def parse_ip4addr(addr):
    import socket
    return (socket.inet_aton(addr))


class IfConfig:
    """
    Wrapper around a single invocation of "ifconfig"
    """

    def __init__(self, stdout=None, debug=False):
        if stdout is not None:
            self.stdout = stdout
            self.stderr = ''
        else:
            env = os.environ.copy()
            env.update(LANG='C.utf8')
            p = Popen('ifconfig', stdout=PIPE, stderr=PIPE,
                      env=env, universal_newlines=True)
            self.stdout, self.stderr = p.communicate()
            if self.stderr != '':
                raise ValueError(
                    'stderr from ifconfig was nonempty:\n%s' % self.stderr)
        if 0:
            print('self.stdout: %r' % self.stdout)
        self.devices = []
        curdev = None
        for line in self.stdout.splitlines():
            if 0:
                print('line: %r' % line)

            if line == '':
                continue

            # Start of a device entry, e.g:
            #  'lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 16436'
            # old ifconfig format:
            #  'lo        Link encap:Local Loopback'
            m = re.match(r'(\w+): (.+)', line)
            mo = re.match(r'(\w+)\s+(.+)', line)
            if m:
                self.oldFormat = False
                devname = m.group(1)
                curdev = Device(devname, debug)
                self.devices.append(curdev)
                curdev._parse_rest_of_first_line(m.group(2))
                continue

            if mo:
                self.oldFormat = True
                devname = mo.group(1)
                curdev = Device(devname, debug)
                self.devices.append(curdev)
                curdev._parse_rest_of_first_line_old(mo.group(2))
                continue

            if self.oldFormat:
                curdev._parse_line_old(line)
            else:
                curdev._parse_line(line)

    def get_device_by_name(self, devname):
        for dev in self.devices:
            if dev.name == devname:
                return dev
        raise ValueError('device not found: %r' % devname)


class Device:
    """
    Wrapper around a device entry within the output of "ifconfig"
    """

    def __init__(self, name, debug=False):
        self.name = name
        self.debug = debug

        self.flagsint = None
        self.flagsstr = None
        self.mtu = None
        self.metric = None
        self.outfill = None
        self.keepalive = None

        self.inet = None
        self.netmask = None
        self.broadcast = None
        self.destination = None

        self.inet6 = None
        self.prefixlen = None
        self.scopeid = None

        self.hwname = None
        self.hwaddr = None
        self.txqueuelen = None
        self.hwtitle = None

        self.rxpackets = None
        self.rxbytes = None

        self.rxerrors = None
        self.rxdropped = None
        self.rxoverruns = None
        self.rxframe = None

        self.rxcompressed = None

        self.txpackets = None
        self.txbytes = None

        self.txerrors = None
        self.txdropped = None
        self.txoverruns = None
        self.txcarrier = None
        self.txcollisions = None

        self.txcompressed = None

        self.interrupt = None
        self.baseaddr = None
        self.memstart = None
        self.memend = None
        self.dma = None

    def get_netmask_bits(self):
        # Convert a dotted netmask string to a bitcount int
        # e.g. from "255.255.252.0" to 22:
        if not self.netmask:
            return 0
        packed = parse_ip4addr(self.netmask)
        # count bits in "packed":
        result = 0
        for ch in packed:
            ch = ord(ch) if isinstance(ch, str) else ch
            while ch:
                if ch & 1:
                    result += 1
                ch //= 2
        return result

    def __repr__(self):
        return ('Device(name=%(name)r, flagsint=%(flagsint)r, '
                'flagsstr=%(flagsstr)r, mtu=%(mtu)r)'
                % (self.__dict__))

    def _debug(self, groups):
        if self.debug:
            print('groups: %r' % (groups,))

    def _parse_rest_of_first_line(self, text):
        m = re.match(r'flags=([0-9]+)<(.*)>  mtu ([0-9]+)(.*)', text)
        if not m:
            raise ValueError('unable to parse: %r' % text)
        self._debug(m.groups())
        self.flagsint = int(m.group(1))
        self.flagsstr = m.group(2)
        self.mtu = int(m.group(3))

        # It might have "  outfill %d  keepalive %d":
        m = re.match(ws
                     + 'outfill' + ws + group_dec + ws
                     + 'keepalive' + ws + group_dec,
                     m.group(4))
        if m:
            self._debug(m.groups())
            self.outfill = int(m.group(1))
            self.keepalive = int(m.group(2))

    def _parse_rest_of_first_line_old(self, text):
        m = re.match('Link encap:(\w+ ?\w*)\s*(HWaddr )?(\S*)', text)
        if not m:
            raise ValueError('unable to parse: %r' % text)
        self.hwtitle = m.group(1).strip()
        if m.group(2):
            self.hwaddr = m.group(3)

    def _parse_line(self, line):
        m = re.match(ws
                     + 'inet ' + group_ip4addr + ws
                     + 'netmask ' + group_ip4addr + '(.*)',
                     line)
        if m:
            self._debug(m.groups())
            self.inet = m.group(1)
            self.netmask = m.group(2)
            # "broadcast" and "destination" are both optional:
            line = m.group(3)
            m = re.match(ws + 'broadcast ' + group_ip4addr + '(.*)', line)
            if m:
                self.broadcast = m.group(1)
                line = m.group(2)

            m = re.match(ws + 'destination ' + group_nonws, line)
            if m:
                self.destination = m.group(1)
            return

        m = re.match(ws
                     + 'inet6 ' + group_nonws + ws
                     + 'prefixlen' + ws + group_dec + ws
                     + 'scopeid ' + group_nonws,
                     line)
        if m:
            self._debug(m.groups())
            self.inet6 = m.group(1)
            self.prefixlen = int(m.group(2))
            self.scopeid = m.group(3)
            return

        # e.g. (with hwaddr):
        #  '        ether ff:00:11:22:33:42  txqueuelen 1000  (Ethernet)'
        m = re.match(ws
                     + group_nonws + ws + group_nonws + ws
                     + 'txqueuelen' + ws + group_dec + ws
                     + '\((.*)\)', line)
        if m:
            self._debug(m.groups())
            self.hwname = m.group(1)
            self.hwaddr = m.group(2)
            self.txqueuelen = int(m.group(3))
            self.hwtitle = m.group(4)
            return

        # e.g. (without hwaddr):
        #  '        loop  txqueuelen 0  (Local Loopback)'
        m = re.match(ws
                     + group_nonws + ws
                     + 'txqueuelen' + ws + group_dec + ws
                     + '\((.*)\)', line)
        if m:
            self._debug(m.groups())
            self.hwname = m.group(1)
            self.hwaddr = None
            self.txqueuelen = int(m.group(2))
            self.hwtitle = m.group(3)
            return

        # e.g. '        RX packets 5313261  bytes 6062336615 (5.6 GiB)'
        m = re.match(ws
                     + 'RX packets ' + group_dec + ws
                     + 'bytes ' + group_dec + ws
                     + '\(.*\)',
                     line)
        if m:
            self._debug(m.groups())
            self.rxpackets = int(m.group(1))
            self.rxbytes = int(m.group(2))
            return

        # e.g. '        RX errors 0  dropped 0  overruns 0  frame 0'
        m = re.match(ws
                     + 'RX errors ' + group_dec + ws
                     + 'dropped ' + group_dec + ws
                     + 'overruns ' + group_dec + ws
                     + 'frame ' + group_dec,
                     line)
        if m:
            self._debug(m.groups())
            self.rxerrors = int(m.group(1))
            self.rxdropped = int(m.group(2))
            self.rxoverruns = int(m.group(3))
            self.rxframe = int(m.group(4))
            return

        # e.g. '        TX packets 2949524  bytes 344092625 (328.1 MiB)'
        m = re.match(ws
                     + 'TX packets ' + group_dec + ws
                     + 'bytes ' + group_dec + ws
                     + '\(.*\)',
                     line)
        if m:
            self._debug(m.groups())
            self.txpackets = int(m.group(1))
            self.txbytes = int(m.group(2))
            return

        # e.g. '        TX errors 0  dropped 0 overruns 0  carrier 0
        # collisions 0'
        m = re.match(ws
                     + 'TX errors ' + group_dec + ws
                     + 'dropped ' + group_dec + ws
                     + 'overruns ' + group_dec + ws
                     + 'carrier ' + group_dec + ws
                     + 'collisions ' + group_dec,
                     line)
        if m:
            self._debug(m.groups())
            self.txerrors = int(m.group(1))
            self.txdropped = int(m.group(2))
            self.txoverruns = int(m.group(3))
            self.txcarrier = int(m.group(4))
            self.txcollisions = int(m.group(5))
            return

        # e.g.'        device interrupt 20  memory 0xf2600000-f2620000  '
        m = re.match(ws
                     + 'device' + ws + '(.*)',
                     line)
        if m:
            self._debug(m.groups())
            line = m.group(1)
            m = re.match('interrupt ' + group_dec + ws + '(.*)',
                         line)
            if m:
                self.interrupt = int(m.group(1))
                line = m.group(2)

            m = re.match('base ' + group_o_hex + ws + '(.*)',
                         line)
            if m:
                self.baseaddr = int(m.group(1), 16)
                line = m.group(2)

            m = re.match('memory ' + group_o_hex + '-' +
                         group_hex + ws + '(.*)',
                         line)
            if m:
                self.memstart = int(m.group(1), 16)
                self.memend = int(m.group(2), 16)
                line = m.group(3)

            m = re.match('dma ' + group_o_hex,
                         line)
            if m:
                self.dma = int(m.group(1), 16)

            return

        raise ValueError('parser could not handle line: %r' % line)

    def _parse_line_old(self, line):
        m = re.match(ws
                     + 'inet addr:' + group_ip4addr
                     + '(.*)',
                     line)
        if m:
            self._debug(m.groups())
            self.inet = m.group(1)
            # "destination (P-t-P)" and "Bcast" are both optional:
            line = m.group(2)
            m = re.match(ws + 'P-t-P:' + group_ip4addr + '(.*)', line)
            if m:
                self.destination = m.group(1)
                line = m.group(2)

            m = re.match(ws + 'Bcast:' + group_ip4addr + '(.*)', line)
            if m:
                self.broadcast = m.group(1)
                line = m.group(2)

            m = re.match(ws + 'Mask:' + group_ip4addr, line)
            if m:
                self.netmask = m.group(1)
            return

        m = re.match(ws
                     + 'inet6 addr: ' + group_nonws + '/'
                     + group_dec + ws
                     + 'Scope:' + group_nonws,
                     line)
        if m:
            self._debug(m.groups())
            self.inet6 = m.group(1)
            self.prefixlen = int(m.group(2))
            self.scopeid = m.group(3)
            return

        # UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
        m = re.match(ws + '(.*)'
                     + '  MTU:' + group_dec + ws
                     + 'Metric:' + group_dec
                     + '(.*)', line)
        if m:
            self._debug(m.groups())
            self.flagsstr = m.group(1)
            self.mtu = int(m.group(2))
            self.metric = int(m.group(3))

            # It might have "  Outfill:%d  Keepalive:%d":
            m = re.match(ws
                         + 'Outfill:' + group_dec + ws
                         + 'Keepalive:' + group_dec,
                         m.group(4))
            if m:
                self._debug(m.groups())
                self.outfill = int(m.group(1))
                self.keepalive = int(m.group(2))
            return

        # RX packets:4458926 errors:0 dropped:0 overruns:0 frame:0
        m = re.match(ws
                     + 'RX packets:' + group_dec + ws
                     + 'errors:' + group_dec + ws
                     + 'dropped:' + group_dec + ws
                     + 'overruns:' + group_dec + ws
                     + 'frame:' + group_dec,
                     line)
        if m:
            self._debug(m.groups())
            self.rxpackets = int(m.group(1))
            self.rxerrors = int(m.group(2))
            self.rxdropped = int(m.group(3))
            self.rxoverruns = int(m.group(4))
            self.rxframe = int(m.group(5))
            return

        # "             compressed:%lu\n"
        m = re.match(ws + 'compressed:' + group_dec, line)
        if m:
            self.rxcompressed = int(m.group(1))
            return

        # TX packets:3536982 errors:0 dropped:0 overruns:0 carrier:0
        m = re.match(ws
                     + 'TX packets:' + group_dec + ws
                     + 'errors:' + group_dec + ws
                     + 'dropped:' + group_dec + ws
                     + 'overruns:' + group_dec + ws
                     + 'carrier:' + group_dec,
                     line)
        if m:
            self._debug(m.groups())
            self.txpackets = int(m.group(1))
            self.txerrors = int(m.group(2))
            self.txdropped = int(m.group(3))
            self.txoverruns = int(m.group(4))
            self.txcarrier = int(m.group(5))
            return

        # "          collisions:%lu compressed:%lu txqueuelen:%d "
        m = re.match(ws + 'collisions:' + group_dec + ws + '(.*)', line)
        if m:
            self._debug(m.groups())
            self.txcollisions = int(m.group(1))
            line = m.group(2)
            m = re.match('compressed:' + group_dec + ws + '(.*)', line)
            if m:
                self.txcompressed = int(m.group(1))
                line = m.group(2)

            m = re.match('txqueuelen:' + group_dec, line)
            if m:
                self.txqueuelen = int(m.group(1))
            return

        # RX bytes:3380060233 (3.1 GiB)  TX bytes:713438255 (680.3 MiB)
        m = re.match(ws + 'RX bytes:' + group_dec + ws + '\(.*\)' + ws
                     + 'TX bytes:' + group_dec + ws + '\(.*\)',
                     line)
        if m:
            self._debug(m.groups())
            self.rxbytes = int(m.group(1))
            self.txbytes = int(m.group(2))
            return

        # Interrupt:17 Memory:da000000-da012800
        m = re.match(ws + 'Interrupt:' + group_dec + '\s*(.*)', line)
        if m:
            self._debug(m.groups())
            self.interrupt = int(m.group(1))
            line = m.group(2)

        m = re.match('Base address:' + group_o_hex + '\s*(.*)', line)
        if m:
            self._debug(m.groups())
            self.baseaddr = int(m.group(1), 16)
            line = m.group(2)

        m = re.match('Memory:' + group_hex + '-' + group_hex + '\s*(.*)', line)
        if m:
            self._debug(m.groups())
            self.memstart = int(m.group(1), 16)
            self.memend = int(m.group(2), 16)
            line = m.group(3)

        m = re.match('DMA chan:' + group_hex, line)
        if m:
            self._debug(m.groups())
            self.dma = int(m.group(1), 16)

        return

        raise ValueError('parser could not handle line: %r' % line)

# ifconfig = IfConfig()
# for dev in ifconfig.devices:
#    print(dev)


class ParserTests(unittest.TestCase):
    def test_full(self):
        # Parse a complex pre-canned output from ifconfig
        # This is the output of "ifconfig" on this machine, sanitized
        # to remove stuff that identifies it
        full = '''
eth1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 1.12.123.124  netmask 255.255.252.0  broadcast 1.12.123.255
        inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        ether ff:00:11:22:33:42  txqueuelen 1000  (Ethernet)
        RX packets 5329294  bytes 6074440831 (5.6 GiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 2955574  bytes 344607935 (328.6 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
''' + '        device interrupt 20  memory 0xf2600000-f2620000  ' + '''

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 16436
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 0  (Local Loopback)
        RX packets 1562620  bytes 524530977 (500.2 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 1562620  bytes 524530977 (500.2 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

virbr0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 1.12.123.124  netmask 255.255.252.0  broadcast 1.12.123.255
        ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        RX packets 88730  bytes 5140566 (4.9 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 177583  bytes 244647206 (233.3 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

vnet0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        RX packets 538  bytes 172743 (168.6 KiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 26593  bytes 1379054 (1.3 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

vnet1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        RX packets 71567  bytes 5033151 (4.7 MiB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 216553  bytes 200424748 (191.1 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
'''
        ifconfig = IfConfig(stdout=full)
        self.assertEqual(len(ifconfig.devices), 5)

        # Verify eth1:
        eth1 = ifconfig.devices[0]
        # eth1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        self.assertEqual(eth1.name, 'eth1')
        self.assertEqual(eth1.flagsint, 4163)
        self.assertEqual(eth1.flagsstr, 'UP,BROADCAST,RUNNING,MULTICAST')
        self.assertEqual(eth1.mtu, 1500)
        # inet 1.12.123.124  netmask 255.255.252.0  broadcast 1.12.123.255
        self.assertEqual(eth1.inet, '1.12.123.124')
        self.assertEqual(eth1.netmask, '255.255.252.0')
        self.assertEqual(eth1.broadcast, '1.12.123.255')
        # inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        self.assertEqual(eth1.inet6, 'ffff::ffff:ffff:ffff:ffff')
        self.assertEqual(eth1.prefixlen, 64)
        self.assertEqual(eth1.scopeid, '0x20<link>')
        #        ether ff:00:11:22:33:42  txqueuelen 1000  (Ethernet)
        self.assertEqual(eth1.hwname, 'ether')
        self.assertEqual(eth1.hwaddr, 'ff:00:11:22:33:42')
        self.assertEqual(eth1.txqueuelen, 1000)
        self.assertEqual(eth1.hwtitle, 'Ethernet')
        #        RX packets 5329294  bytes 6074440831 (5.6 GiB)
        self.assertEqual(eth1.rxpackets, 5329294)
        self.assertEqual(eth1.rxbytes, 6074440831)
        #        RX errors 0  dropped 0  overruns 0  frame 0
        self.assertEqual(eth1.rxerrors, 0)
        self.assertEqual(eth1.rxdropped, 0)
        self.assertEqual(eth1.rxoverruns, 0)
        self.assertEqual(eth1.rxframe, 0)
        #        TX packets 2955574  bytes 344607935 (328.6 MiB)
        self.assertEqual(eth1.txpackets, 2955574)
        self.assertEqual(eth1.txbytes, 344607935)
        #        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
        self.assertEqual(eth1.txerrors, 0)
        self.assertEqual(eth1.txdropped, 0)
        self.assertEqual(eth1.txoverruns, 0)
        self.assertEqual(eth1.txcarrier, 0)
        self.assertEqual(eth1.txcollisions, 0)
        #        device interrupt 20  memory 0xf2600000-f2620000
        self.assertEqual(eth1.interrupt, 20)

        # Verify lo:
        lo = ifconfig.devices[1]
        self.assertEqual(lo.name, 'lo')
        # lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 16436
        self.assertEqual(lo.flagsint, 73)
        self.assertEqual(lo.flagsstr, 'UP,LOOPBACK,RUNNING')
        self.assertEqual(lo.mtu, 16436)
        #         inet 127.0.0.1  netmask 255.0.0.0
        self.assertEqual(lo.inet, '127.0.0.1')
        self.assertEqual(lo.netmask, '255.0.0.0')
        self.assertEqual(lo.broadcast, None)
        #         inet6 ::1  prefixlen 128  scopeid 0x10<host>
        self.assertEqual(lo.inet6, '::1')
        self.assertEqual(lo.prefixlen, 128)
        self.assertEqual(lo.scopeid, '0x10<host>')
        #         loop  txqueuelen 0  (Local Loopback)
        self.assertEqual(lo.hwname, 'loop')
        self.assertEqual(lo.txqueuelen, 0)
        self.assertEqual(lo.hwtitle, 'Local Loopback')
        #         RX packets 1562620  bytes 524530977 (500.2 MiB)
        #         RX errors 0  dropped 0  overruns 0  frame 0
        #         TX packets 1562620  bytes 524530977 (500.2 MiB)
        #         TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

        # Verify virbr0:
        virbr0 = ifconfig.devices[2]
        self.assertEqual(virbr0.name, 'virbr0')
        # virbr0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        #  inet 1.12.123.124  netmask 255.255.252.0  broadcast 1.12.123.255
        #  ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        #  RX packets 88730  bytes 5140566 (4.9 MiB)
        #  RX errors 0  dropped 0  overruns 0  frame 0
        #  TX packets 177583  bytes 244647206 (233.3 MiB)
        #  TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

        # Verify vnet0:
        vnet0 = ifconfig.devices[3]
        self.assertEqual(vnet0.name, 'vnet0')
        # vnet0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        #  inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        #  ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        #  RX packets 538  bytes 172743 (168.6 KiB)
        #  RX errors 0  dropped 0  overruns 0  frame 0
        #  TX packets 26593  bytes 1379054 (1.3 MiB)
        #  TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

        # Verify vnet1:
        vnet1 = ifconfig.devices[4]
        self.assertEqual(vnet1.name, 'vnet1')
        # vnet1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        #  inet6 ffff::ffff:ffff:ffff:ffff  prefixlen 64  scopeid 0x20<link>
        #  ether ff:00:11:22:33:42  txqueuelen 0  (Ethernet)
        #  RX packets 71567  bytes 5033151 (4.7 MiB)
        #  RX errors 0  dropped 0  overruns 0  frame 0
        #  TX packets 216553  bytes 200424748 (191.1 MiB)
        #  TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

        fullOld = '''
eth0      Link encap:Ethernet  HWaddr 00:11:22:33:44:55
          inet addr:1.12.123.124  Bcast:1.12.123.255  Mask:255.255.252.0
          inet6 addr: dddd::dddd:dddd:dddd:dddd:dddd:dddd/64 Scope:Site
          inet6 addr: eeee::eeee:eeee:eeee:eeee:eeee:eeee/64 Scope:Global
          inet6 addr: ffff::ffff:ffff:ffff:ffff/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:4458926 errors:0 dropped:0 overruns:0 frame:0
          TX packets:3536982 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:3380060233 (3.1 GiB)  TX bytes:713438255 (680.3 MiB)
          Interrupt:17 Memory:da000000-da012800

lo        Link encap:Local Loopback
          inet addr:127.0.0.1  Mask:255.0.0.0
          inet6 addr: ::1/128 Scope:Host
          UP LOOPBACK RUNNING  MTU:16436  Metric:1
          RX packets:805234 errors:0 dropped:0 overruns:0 frame:0
          TX packets:805234 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:0
          RX bytes:132177529 (126.0 MiB)  TX bytes:132177529 (126.0 MiB)
'''
        ifconfig = IfConfig(stdout=fullOld)
        self.assertEqual(len(ifconfig.devices), 2)

        # Verify eth0:
        eth0 = ifconfig.devices[0]
        # eth0      Link encap:Ethernet  HWaddr 00:11:22:33:44:55
        self.assertEqual(eth0.name, 'eth0')
        self.assertEqual(eth0.hwtitle, 'Ethernet')
        self.assertEqual(eth0.hwaddr, '00:11:22:33:44:55')
        # inet addr:1.12.123.124  Bcast:1.12.123.255  Mask:255.255.252.0
        self.assertEqual(eth0.inet, '1.12.123.124')
        self.assertEqual(eth0.netmask, '255.255.252.0')
        self.assertEqual(eth0.broadcast, '1.12.123.255')
        #       inet6 addr: dddd::dddd:dddd:dddd:dddd:dddd:dddd/64 Scope:Site
        # self.assertEqual(eth0.inet6, 'dddd::dddd:dddd:dddd:dddd:dddd:dddd')
        # self.assertEqual(eth0.prefixlen, 64)
        # self.assertEqual(eth0.scopeid, 'Site')
        #       inet6 addr: eeee::eeee:eeee:eeee:eeee:eeee:eeee/64 Scope:Global
        # self.assertEqual(eth0.inet6, 'eeee::eeee:eeee:eeee:eeee:eeee:eeee')
        # self.assertEqual(eth0.prefixlen, 64)
        # self.assertEqual(eth0.scopeid, 'Global')
        #           inet6 addr: ffff::ffff:ffff:ffff:ffff/64 Scope:Link
        self.assertEqual(eth0.inet6, 'ffff::ffff:ffff:ffff:ffff')
        self.assertEqual(eth0.prefixlen, 64)
        self.assertEqual(eth0.scopeid, 'Link')
        #          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
        self.assertEqual(eth0.flagsstr, 'UP BROADCAST RUNNING MULTICAST')
        self.assertEqual(eth0.mtu, 1500)
        self.assertEqual(eth0.metric, 1)
        #          RX packets:4458926 errors:0 dropped:0 overruns:0 frame:0
        self.assertEqual(eth0.rxpackets, 4458926)
        self.assertEqual(eth0.rxerrors, 0)
        self.assertEqual(eth0.rxdropped, 0)
        self.assertEqual(eth0.rxoverruns, 0)
        self.assertEqual(eth0.rxframe, 0)
        #          TX packets:3536982 errors:0 dropped:0 overruns:0 carrier:0
        self.assertEqual(eth0.txpackets, 3536982)
        self.assertEqual(eth0.txerrors, 0)
        self.assertEqual(eth0.txdropped, 0)
        self.assertEqual(eth0.txoverruns, 0)
        self.assertEqual(eth0.txcarrier, 0)
        #          collisions:0 txqueuelen:1000
        self.assertEqual(eth0.txcollisions, 0)
        self.assertEqual(eth0.txqueuelen, 1000)
        # RX bytes:3380060233 (3.1 GiB)  TX bytes:713438255 (680.3 MiB)
        self.assertEqual(eth0.rxbytes, 3380060233)
        self.assertEqual(eth0.txbytes, 713438255)
        #          Interrupt:17 Memory:da000000-da012800
        self.assertEqual(eth0.interrupt, 17)
        self.assertEqual(eth0.memstart, 0xda000000)
        self.assertEqual(eth0.memend, 0xda012800)

        # Verify lo:
        lo = ifconfig.devices[1]
        # lo        Link encap:Local Loopback
        self.assertEqual(lo.name, 'lo')
        self.assertEqual(lo.hwtitle, 'Local Loopback')
        self.assertEqual(lo.hwaddr, None)
        #          inet addr:127.0.0.1  Mask:255.0.0.0
        self.assertEqual(lo.inet, '127.0.0.1')
        self.assertEqual(lo.netmask, '255.0.0.0')
        self.assertEqual(lo.broadcast, None)
        #          inet6 addr: ::1/128 Scope:Host
        self.assertEqual(lo.inet6, '::1')
        self.assertEqual(lo.prefixlen, 128)
        self.assertEqual(lo.scopeid, 'Host')
        #          UP LOOPBACK RUNNING  MTU:16436  Metric:1
        self.assertEqual(lo.flagsstr, 'UP LOOPBACK RUNNING')
        self.assertEqual(lo.mtu, 16436)
        self.assertEqual(lo.metric, 1)
        #          RX packets:805234 errors:0 dropped:0 overruns:0 frame:0
        self.assertEqual(lo.rxpackets, 805234)
        #          TX packets:805234 errors:0 dropped:0 overruns:0 carrier:0
        self.assertEqual(lo.txpackets, 805234)
        #          collisions:0 txqueuelen:0
        # RX bytes:132177529 (126.0 MiB)  TX bytes:132177529 (126.0 MiB)
        self.assertEqual(lo.rxbytes, 132177529)
        self.assertEqual(lo.txbytes, 132177529)

    def parse_single_device(self, stdout, debug=False):
        ifconfig = IfConfig(stdout, debug)
        self.assertEqual(len(ifconfig.devices), 1)
        return ifconfig.devices[0]

    def test_parsing_flags_line(self):
        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n')
        self.assertEqual(dev.name, 'foo')
        self.assertEqual(dev.flagsint, 4163)
        self.assertEqual(dev.flagsstr, 'UP,BROADCAST,RUNNING,MULTICAST')
        self.assertEqual(dev.mtu, 1500)

        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500  '
            'outfill 32  keepalive 96\n')
        self.assertEqual(dev.mtu, 1500)
        self.assertEqual(dev.outfill, 32)
        self.assertEqual(dev.keepalive, 96)

        # old format
        dev = self.parse_single_device(
            'foo      Link encap:Ethernet  HWaddr F0:F0:F0:F0:F0:F0\n'
            '         UP BROADCAST RUNNING MULTICAST  MTU:500  Metric:2  '
            'Outfill:55  Keepalive:66\n')
        self.assertEqual(dev.flagsstr, 'UP BROADCAST RUNNING MULTICAST')
        self.assertEqual(dev.mtu, 500)
        self.assertEqual(dev.metric, 2)
        self.assertEqual(dev.outfill, 55)
        self.assertEqual(dev.keepalive, 66)

    def test_parsing_ip(self):
        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        inet 1.12.123.124  netmask 255.255.252.0  broadcast '
            '1.12.123.255\n')
        self.assertEqual(dev.inet, '1.12.123.124')
        self.assertEqual(dev.netmask, '255.255.252.0')
        self.assertEqual(dev.get_netmask_bits(), 22)
        self.assertEqual(dev.broadcast, '1.12.123.255')
        self.assertEqual(dev.destination, None)

        dev = self.parse_single_device(
            'lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 16436\n'
            '        inet 127.0.0.1  netmask 255.0.0.0\n')
        self.assertEqual(dev.inet, '127.0.0.1')
        self.assertEqual(dev.netmask, '255.0.0.0')
        self.assertEqual(dev.get_netmask_bits(), 8)

        self.assertEqual(dev.broadcast, None)
        self.assertEqual(dev.destination, None)

        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        inet 1.1.1.1  netmask 255.255.255.254  destination '
            '1.12.123.255\n')
        self.assertEqual(dev.inet, '1.1.1.1')
        self.assertEqual(dev.netmask, '255.255.255.254')
        self.assertEqual(dev.broadcast, None)
        self.assertEqual(dev.destination, '1.12.123.255')

        # old format
        dev = self.parse_single_device(
            'foo      Link encap:Ethernet  HWaddr F0:F0:F0:F0:F0:F0\n'
            '       inet addr:1.2.3.4  P-t-P:10.20.30.40  '
            'Mask:255.255.254.0\n')
        self.assertEqual(dev.inet, '1.2.3.4')
        self.assertEqual(dev.destination, '10.20.30.40')
        self.assertEqual(dev.broadcast, None)
        self.assertEqual(dev.get_netmask_bits(), 23)
        self.assertEqual(dev.netmask, '255.255.254.0')

    def test_parsing_device_line(self):
        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        device interrupt 20  memory 0xf2600000-f2620000  \n')
        self.assertEqual(dev.interrupt, 20)
        self.assertEqual(dev.baseaddr, None)
        self.assertEqual(dev.memstart, 0xf2600000)
        self.assertEqual(dev.memend, 0xf2620000)
        self.assertEqual(dev.dma, None)

        # but the fields in the "device" field are optional:
        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        device \n')
        self.assertEqual(dev.interrupt, None)
        self.assertEqual(dev.baseaddr, None)
        self.assertEqual(dev.memstart, None)
        self.assertEqual(dev.memend, None)
        self.assertEqual(dev.dma, None)

        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        device memory 0xf2600000-f2620000  \n')
        self.assertEqual(dev.interrupt, None)
        self.assertEqual(dev.baseaddr, None)
        self.assertEqual(dev.memstart, 0xf2600000)
        self.assertEqual(dev.memend, 0xf2620000)
        self.assertEqual(dev.dma, None)

        # and there could potentially be "base" and "dma" fields:
        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        device base 0xdeadbeef  \n')
        self.assertEqual(dev.interrupt, None)
        self.assertEqual(dev.baseaddr, 0xdeadbeef)
        self.assertEqual(dev.memstart, None)
        self.assertEqual(dev.memend, None)
        self.assertEqual(dev.dma, None)

        dev = self.parse_single_device(
            'foo: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500\n'
            '        device interrupt 20  dma 0xbad4f00d\n')
        self.assertEqual(dev.interrupt, 20)
        self.assertEqual(dev.baseaddr, None)
        self.assertEqual(dev.memstart, None)
        self.assertEqual(dev.memend, None)
        self.assertEqual(dev.dma, 0xbad4f00d)

        # old format
        dev = self.parse_single_device(
            'foo      Link encap:Ethernet  HWaddr F0:F0:F0:F0:F0:F0\n'
            '         Interrupt:20 Base address:0xdeadbeef DMA chan:bad4f00d\n'
            '         collisions:13 compressed:11 \n')
        self.assertEqual(dev.interrupt, 20)
        self.assertEqual(dev.baseaddr, 0xdeadbeef)
        self.assertEqual(dev.dma, 0xbad4f00d)
        self.assertEqual(dev.txcollisions, 13)
        self.assertEqual(dev.txcompressed, 11)

    def test_parse_ip4addr(self):
        self.assertEqual(parse_ip4addr('1.1.1.1'), b'\x01\x01\x01\x01')
        self.assertEqual(parse_ip4addr('127.0.0.1'), b'\x7f\x00\x00\x01')

    def test_local(self):
        # Actually invoke ifconfig locally, and parse whatever it emits:
        IfConfig()


if __name__ == '__main__':
    unittest.main()
