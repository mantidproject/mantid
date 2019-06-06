# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.kernel import InstrumentInfo, ConfigService


class InstrumentInfoTest(unittest.TestCase):
    def _get_test_instrument(self):
        facility = ConfigService.getFacility("ISIS")
        return facility.instrument("CRISP")

    def test_construction_raies_an_error(self):
        self.assertRaises(RuntimeError, InstrumentInfo)

    def test_instrument_attributes(self):
        inst = self._get_test_instrument()

        # Just testing functionality; values can be updated if needed
        self.assertEqual(inst.name(), "CRISP")
        self.assertEqual(inst.shortName(), "CSP")
        self.assertEqual(str(inst), "CSP")
        self.assertEqual(inst.zeroPadding(99777), 5)
        self.assertEqual(inst.zeroPadding(99778), 8)
        self.assertEqual(inst.filePrefix(99777), "CSP")
        self.assertEqual(inst.filePrefix(99778), "CRISP")
        self.assertEqual(inst.delimiter(), "")
        self.assertEqual(str(inst.techniques()), "set('Reflectometry')")
        self.assertEqual(inst.facility().name(), "ISIS")
        self.assertEqual(inst.liveListener(), "ISISHistoDataListener")
        self.assertEqual(inst.liveListener("histo"), "ISISHistoDataListener")
        self.assertRaises(RuntimeError, inst.liveListener, "invalid_name")
        self.assertEqual(inst.liveDataAddress(), "NDXCRISP:6789")
        self.assertEqual(inst.liveDataAddress("histo"), "NDXCRISP:6789")
        self.assertRaises(RuntimeError, inst.liveDataAddress, "invalid_name")
        self.assertTrue(inst.hasLiveListenerInfo())
        self.assertEqual(len(inst.liveListenerInfoList()), 1)


if __name__ == '__main__':
    unittest.main()

