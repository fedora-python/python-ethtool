import unittest
import ethtool
import sys
import os
from io import TextIOWrapper, BytesIO
from imp import load_source

pifc = load_source('pifc', 'scripts/pifconfig')
peth = load_source('peth', 'scripts/pethtool')


def find_suitable_device():
    active_devices = ethtool.get_active_devices()
    for device in active_devices:
        device_flags = ethtool.get_flags(device)
        str_flags = pifc.flags2str(device_flags)
        try:
            wireless_protocol = ethtool.get_wireless_protocol(device)
        except Exception:
            wireless_protocol = None

        if 'UP' in str_flags and 'RUNNING' in str_flags \
                and 'BROADCAST' in str_flags and not wireless_protocol:
            return device

    return None


loopback = 'lo'
device = find_suitable_device()


class ScriptsTests(unittest.TestCase):

    def setUp(self):
        self._old_stdout = sys.stdout
        self._stdout = sys.stdout = TextIOWrapper(BytesIO(), sys.stdout.encoding)

    def _output(self):
        self._stdout.seek(0)
        return self._stdout.read()

    def tearDown(self):
        sys.stdout = self._old_stdout
        self._stdout.close()

    def test_flags2str(self):
        self.assertEqual(pifc.flags2str(0), '')
        self.assertEqual(pifc.flags2str(73), 'UP LOOPBACK RUNNING')
        self.assertEqual(pifc.flags2str(4305), 'UP POINTOPOINT RUNNING NOARP MULTICAST')
        self.assertEqual(pifc.flags2str(4163), 'UP BROADCAST RUNNING MULTICAST')

    # Tests for loopback

    def test_driver_lo(self):
        self.assertIsNone(peth.show_driver(loopback))
        self.assertEqual(self._output(),
                         'driver: not implemented\nbus-info: not available\n'
                         )

    def test_show_ring_lo(self):
        self.assertIsNone(peth.show_ring(loopback))
        self.assertEqual(self._output(),
                         'Ring parameters for {}:\n  NOT supported!\n'.format(loopback)
                         )

    def test_show_coalesce_lo(self):
        self.assertIsNone(peth.show_coalesce(loopback))
        self.assertEqual(self._output(),
                         'Coalesce parameters for {}:\n  NOT supported!\n'.format(loopback)
                         )

    def test_show_offload_lo(self):
        self.assertIsNone(peth.show_offload(loopback))
        self.assertEqual(self._output(),
                         '''scatter-gather: on
tcp segmentation offload: on
udp fragmentation offload: on
generic segmentation offload: on
'''
                         )

    # Tests for another device
    if device:
        def test_driver_eth(self):
            self.assertIsNone(peth.show_driver(device))
            expected_lines_start = ['driver: ', 'bus-info: ']
            lines = self._output().split('\n')
            for expected_start, line in zip(expected_lines_start, lines):
                self.assertTrue(line.startswith(expected_start))

        def test_show_ring_eth(self):
            self.assertIsNone(peth.show_ring(device))
            expected_lines_start = ['Ring parameters for ',
                                    'Pre-set maximums:', 'RX:', 'RX Mini:',
                                    'RX Jumbo:', 'TX', 'Current hardware settings:',
                                    'RX:', 'RX Mini:', 'RX Jumbo:', 'TX:']
            lines = self._output().split('\n')
            for expected_start, line in zip(expected_lines_start, lines):
                self.assertTrue(line.startswith(expected_start))

        @unittest.skipIf('TRAVIS' in os.environ and os.environ['TRAVIS'] == 'true',
                         'Skipping this test on Travis CI because show '
                         'coalesce is not supported on ethernet device in VM.')
        def test_show_coalesce_eth(self):
            self.assertIsNone(peth.show_coalesce(device))
            expected_lines_start = ['Coalesce parameters for',
                                    'Adaptive RX:',
                                    'stats-block-usecs:',
                                    'sample-interval:',
                                    'pkt-rate-low:',
                                    'pkt-rate-high:',
                                    'rx-usecs:',
                                    'rx-frames:',
                                    'rx-usecs-irq:',
                                    'rx-frames-irq:',
                                    'tx-usecs:',
                                    'tx-frames:',
                                    'tx-usecs-irq:',
                                    'tx-frames-irq:',
                                    'rx-usecs-low:',
                                    'rx-frame-low:',
                                    'tx-usecs-low:',
                                    'tx-frame-low:',
                                    'rx-usecs-high:',
                                    'rx-frame-high:',
                                    'tx-usecs-high:',
                                    'tx-frame-high:']

            lines = self._output().split('\n')
            for expected_start, line in zip(expected_lines_start, lines):
                self.assertTrue(line.startswith(expected_start))

        def test_show_offload_eth(self):
            self.assertIsNone(peth.show_offload(device))
            expected_lines_start = ['scatter-gather:',
                                    'tcp segmentation offload:',
                                    'udp fragmentation offload:',
                                    'generic segmentation offload:']
            lines = self._output().split('\n')
            for expected_start, line in zip(expected_lines_start, lines):
                self.assertTrue(line.startswith(expected_start))


if __name__ == '__main__':
    unittest.main()
