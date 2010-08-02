"""
    Unit tests for SANS reduction
"""
from MantidFramework import *
mtd.initialise()
import sys
import unittest
import SANSReduction
import SANSInsts
import SANSUtility
from mantidsimple import *

class TestReduction(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        SANSReduction.DataPath("../../../../../Test/Data/SANS2D")
        SANSReduction.UserPath("../../../../../Test/Data/SANS2D")
        SANSReduction.SANS2D()    
        SANSReduction.Set1D()
        SANSReduction.MaskFile('MASKSANS2D.091A')
        self.ws_name,_ = SANSReduction.AssignSample('992.raw')
                
    def test_move_detector(self):
        # Refactor test: Removing the following line and replacing it by a call to a call to an instrument class method. 
        #v1, v2 = SANSReduction.SetupComponentPositions(SANSReduction.INSTRUMENT.cur_detector().name() , self.ws_name, 3.69, 2.59)    
        v1, v2 = SANSReduction.INSTRUMENT.set_component_positions(self.ws_name, 3.69, 2.59)    
        self.assertEquals(v1[0],0)
        self.assertEquals(v1[1],0)
        self.assertEquals(v2[0],-3.69)
        self.assertEquals(v2[1],-2.59)
        
    def test_reduction(self):
        SANSReduction.Detector("rear-detector")
        ws = SANSReduction.WavRangeReduction()
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmin, [0,0])
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmax, [-0.084199999999999997, 0.19650000000000001])

        x = mtd[ws].dataX(0)[:len(mtd[ws].dataX(0))]
        y = mtd[ws].dataY(0)
        e = mtd[ws].dataE(0)
        data = zip(x,y,e)

        self.assertEquals(len(data), 178)

        self.assertAlmostEqual(data[10][0], 0.0243799, 0.00001)
        self.assertAlmostEqual(data[10][1], 83.6761, 0.00001)
        self.assertAlmostEqual(data[10][2], 0.462703, 0.00001)

        self.assertAlmostEqual(data[100][0], 0.144893, 0.00001)
        self.assertAlmostEqual(data[100][1], 3.68945, 0.00001)
        self.assertAlmostEqual(data[100][2], 0.023424, 0.00001)

        self.assertAlmostEqual(data[50][0], 0.0538318, 0.00001)
        self.assertAlmostEqual(data[50][1], 22.862, 0.00001)
        self.assertAlmostEqual(data[50][2], 0.112137, 0.00001)


    def test_reduction_front(self):
        SANSReduction.Detector("front-detector")
        ws = SANSReduction.WavRangeReduction()
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmin, [0,0])
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmax, [0,0])

        x = mtd[ws].dataX(0)[:len(mtd[ws].dataX(0))]
        y = mtd[ws].dataY(0)
        e = mtd[ws].dataE(0)
        data = zip(x,y,e)

        self.assertEquals(len(data), 122)

        self.assertAlmostEqual(data[10][0], 0.109811, 0.00001)
        self.assertAlmostEqual(data[10][1], 6.56454, 0.00001)
        self.assertAlmostEqual(data[10][2], 0.291573, 0.00001)

        self.assertAlmostEqual(data[100][0], 0.65262, 0.00001)
        self.assertAlmostEqual(data[100][1], 0.703535, 0.00001)
        self.assertAlmostEqual(data[100][2], 0.00570243, 0.00001)

        self.assertAlmostEqual(data[50][0], 0.242466, 0.00001)
        self.assertAlmostEqual(data[50][1], 1.53335, 0.00001)
        self.assertAlmostEqual(data[50][2], 0.0194757, 0.00001)


class TestInstrument(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        self.instrument = SANSInsts.instrument_factory("SANS2D")

    def test_SANS2D_default_settings(self):
        self.assertEqual(self.instrument.name(), "SANS2D")
        self.assertTrue(self.instrument.setDetector("front"))
        self.assertFalse(self.instrument.setDetector("not-a-detector"))
        self.assertEqual(self.instrument.get_scattering_mon(), 2)
        self.assertEqual(self.instrument.is_scattering_mon_interp(), False)
        
    def test_Detector_bank_spectra(self):
        inst = SANSInsts.SANS2D()
        self.assertTrue(inst.setDetector('low-angle'))
        width, first, end = SANSUtility.GetInstrumentDetails(inst)
        self.assertEqual(width, 192)
        self.assertEqual(first,  9) 
        self.assertEqual(end,  36872)
        self.assertTrue(inst.setDetector('high-angle'))
        width, first, end = SANSUtility.GetInstrumentDetails(inst)
        self.assertEqual(width, 192)
        self.assertEqual(first, 36873) 
        self.assertEqual(end, 73736)

        inst = SANSInsts.LOQ()
        width, first, end = SANSUtility.GetInstrumentDetails(inst)
        self.assertTrue(inst.setDetector('low-angle'))
        self.assertEqual(width, 128)
        self.assertEqual(first, 3) 
        self.assertEqual(end, 16386)
        self.assertTrue(inst.setDetector('high-angle'))
        width, first, end = SANSUtility.GetInstrumentDetails(inst)
        self.assertEqual(width, 128)
        self.assertEqual(first, 16387) 
        self.assertEqual(end, 17792)
        
if __name__ == '__main__':
    unittest.main()
    
    