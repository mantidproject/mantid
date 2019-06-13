# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
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
        self.assertEqual(info.name(), "histo")
        self.assertEqual(info.listener(), "ISISHistoDataListener")
        self.assertEqual(info.address(), "NDXCRISP:6789")


if __name__ == '__main__':
    unittest.main()

