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
        self.assertEquals(inst.name(), "CRISP")
        self.assertEquals(inst.shortName(), "CSP")
        self.assertEquals(str(inst), "CSP")
        self.assertEquals(inst.zeroPadding(99777), 5)
        self.assertEquals(inst.zeroPadding(99778), 8)
        self.assertEquals(inst.filePrefix(99777), "CSP")
        self.assertEquals(inst.filePrefix(99778), "CRISP")
        self.assertEquals(inst.delimiter(), "")
        self.assertEquals(str(inst.techniques()), "set('Reflectometry')")
        self.assertEquals(inst.facility().name(), "ISIS")
        self.assertEquals(inst.liveListener(), "ISISHistoDataListener")
        self.assertEquals(inst.liveListener("histo"), "ISISHistoDataListener")
        self.assertRaises(RuntimeError, inst.liveListener, "invalid_name")
        self.assertEquals(inst.liveDataAddress(), "NDXCRISP:6789")
        self.assertEquals(inst.liveDataAddress("histo"), "NDXCRISP:6789")
        self.assertRaises(RuntimeError, inst.liveDataAddress, "invalid_name")
        self.assertTrue(inst.hasLiveListenerInfo())
        self.assertEqual(len(inst.liveListenerInfoList()), 1)


if __name__ == '__main__':
    unittest.main()

