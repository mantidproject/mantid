import unittest
import os

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *


class LoadLiveDataTest(unittest.TestCase):
    """
    Test LoadLiveData when passing a python snippet to it.
    """
    
    def setUp(self):
        mtd.clearData()
        pass
        
    def test_chunkProcessing(self):
        code = """
Rebin(InputWorkspace=input,Params='40e3,1e3,60e3',OutputWorkspace=output)
"""
        LoadLiveData(Instrument='FakeEventDataListener', ProcessingScript=code, 
                     OutputWorkspace='fake')

        ws = mtd['fake']        
        # The rebin call in the code made 20 bins
        self.assertEquals( len(ws.readY(0)), 20 )
        # First bin is correct
        self.assertAlmostEqual(ws.readX(0)[0], 40e3, 3)
        
        
    def test_PostProcessing(self):
        code = """
Rebin(InputWorkspace=input,Params='40e3,1e3,60e3',OutputWorkspace=output)
"""
        LoadLiveData(Instrument='FakeEventDataListener', PostProcessingScript=code, 
                     AccumulationWorkspace='fake_accum', OutputWorkspace='fake')

        ws = mtd['fake']        
        # The rebin call in the code made 20 bins
        self.assertEquals( len(ws.readY(0)), 20 )
        # First bin is correct
        self.assertAlmostEqual(ws.readX(0)[0], 40e3, 3)
        

if __name__ == '__main__':
    unittest.main()

