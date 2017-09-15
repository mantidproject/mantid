from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import LiveListenerInfo, ConfigService


class LiveListenerInfoTest(unittest.TestCase):
    def _get_test_listener(self):
        facility = ConfigService.getFacility("ISIS")
        return facility.instrument("CRISP").liveListenerInfo()

    def test_construction_raies_an_error(self):
        self.assertRaises(RuntimeError, LiveListenerInfo)

    def test_listener_attributes(self):
        info = self._get_test_listener()

        # Just testing functionality; values can be updated if needed
        self.assertEquals(info.name(), "histo")
        self.assertEquals(info.listener(), "ISISHistoDataListener")
        self.assertEquals(info.address(), "NDXCRISP:6789")


if __name__ == '__main__':
    unittest.main()

