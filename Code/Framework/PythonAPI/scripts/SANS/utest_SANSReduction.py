"""
    Unit tests for SANS reduction
"""
from MantidFramework import *
mtd.initialise()
import sys
import unittest
import SANSReduction
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

if __name__ == '__main__':
    unittest.main()
    
    