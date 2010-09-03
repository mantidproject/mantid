"""
    Unit tests for SANS reduction
"""
from MantidFramework import *
mtd.initialise()
import sys
import unittest
import SANSInsts
from mantidsimple import *

class TestInstrument(unittest.TestCase):
    """
       Testing the SANSInsts module interface, mostly information
       about the detectors
    """
    
    def setUp(self):
        self.SANS2D = SANSInsts.instrument_factory("SANS2D")
        self.LOQ = SANSInsts.instrument_factory("LOQ")

    def test_default_settings(self):
        self.assertEqual(self.SANS2D.name(), "SANS2D")
        self.assertTrue(self.SANS2D.setDetector("front"))
        self.assertFalse(self.SANS2D.setDetector("not-a-detector"))
        self.assertEqual(self.SANS2D.get_incident_mon(), 2)
        self.assertEqual(self.SANS2D.incid_mon_4_trans_calc, 2)
        self.assertEqual(self.SANS2D.trans_monitor, 3)
        self.assertEqual(self.SANS2D.is_interpolating_norm(), False)
        self.assertEqual(self.SANS2D.use_interpol_trans_calc, False)

        self.assertEqual(self.LOQ.name(), "LOQ")
        self.assertTrue(self.LOQ.setDetector("main-detector-bank"))
        self.assertFalse(self.LOQ.setDetector("not-a-detector"))
        self.assertEqual(self.LOQ.get_incident_mon(), 2)
        self.assertEqual(self.LOQ.incid_mon_4_trans_calc, 2)
        self.assertEqual(self.LOQ.trans_monitor, 3)
        self.assertEqual(self.LOQ.is_interpolating_norm(), False)
        self.assertEqual(self.LOQ.use_interpol_trans_calc, False)
        
if __name__ == '__main__':
    unittest.main()
    
    