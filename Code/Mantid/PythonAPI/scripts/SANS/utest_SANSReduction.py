"""
    Unit tests for SANS reduction
"""
import sys
import unittest
import SANSReduction
import SANSInsts
from mantidsimple import *

class TestReduction(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        SANSReduction.DataPath("/home/m2d/workspace/mantid/Test/Data/SANS2D")
        SANSReduction.UserPath("/home/m2d/workspace/mantid/Test/Data/SANS2D")
        SANSReduction.SANS2D()    
        SANSReduction.Set1D()
        SANSReduction.MaskFile('MASKSANS2D.091A')
        self.ws_name,_ = SANSReduction.AssignSample('992.raw')
                
    def test_move_detector(self):
        # Refactor test: Removing the following line and replacing it by a call to a call to an instrument class method. 
        #v1, v2 = SANSReduction.SetupComponentPositions(SANSReduction.INSTRUMENT.curDetector().name() , self.ws_name, 3.69, 2.59)    
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

        SaveCSV(Filename="tmp_reduction.txt", InputWorkspace=ws, Separator="\t", LineSeparator="\n")
        data = _read_Mantid("tmp_reduction.txt")
        self.assertEquals(len(data), 178)
        self.assertEquals(data[10], [0.024379899999999999, 83.676100000000005, 0.46270299999999998])
        self.assertEquals(data[100], [0.14489299999999999, 3.6894499999999999, 0.023424])
        self.assertEquals(data[50], [0.053831799999999999, 22.861999999999998, 0.112137])


    def test_reduction_front(self):
        SANSReduction.Detector("front-detector")
        ws = SANSReduction.WavRangeReduction()
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmin, [0,0])
        self.assertEquals(SANSReduction._SAMPLE_SETUP._maskrmax, [0,0])

        SaveCSV(Filename="tmp_reduction.txt", InputWorkspace=ws, Separator="\t", LineSeparator="\n")
        data = _read_Mantid("tmp_reduction.txt")
        self.assertEquals(len(data), 122)
        self.assertEquals(data[10], [0.10981100000000001, 6.56454, 0.29157300000000003])
        self.assertEquals(data[100], [0.65261999999999998, 0.70353500000000002, 0.0057024299999999997])
        self.assertEquals(data[50],  [0.24246599999999999, 1.53335, 0.019475699999999999])


class TestInstrument(unittest.TestCase):
    """
        Simple test to check that the API all works out.
        No actual value checking.
    """
    
    def setUp(self):
        self.instrument = SANSInsts.instrument_factory("SANS2D")

        
    def testSANS2D(self):
        instrument = SANSInsts.instrument_factory("SANS2D")
        self.assertEqual(instrument.name, "SANS2D")
        self.assertTrue(instrument.setDetector("front"))
        self.assertFalse(instrument.setDetector("not-a-detector"))


def _read_Mantid(filepath):
    """
        Read in a CVS Mantid file
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Read Q line. Starts with 'A', so remove the first item
        qtoks = f.readline().split()
        qtoks.pop(0)
        
        # Second line is I(q), Starts with 0 to be skipped
        iqtoks = f.readline().split()
        iqtoks.pop(0)
        
        # Third and fourth lines are dummy lines
        f.readline()
        f.readline()
        
        # Fifth line is dI(q). Starts with 0 to be skipped
        diqtoks = f.readline().split()
        diqtoks.pop(0)
        
        for i in range(len(qtoks)-1):
            try:
                q   = float(qtoks[i])
                iq  = float(iqtoks[i])
                diq = float(diqtoks[i])
                data.append([q, iq, diq])
            except:
                print "_read_Mantid:", i, sys.exc_value   
    return data

        
if __name__ == '__main__':
    unittest.main()
    
    