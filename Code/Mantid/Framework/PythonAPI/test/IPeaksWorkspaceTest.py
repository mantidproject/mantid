import unittest

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *

class IPeaksWorkspaceTest(unittest.TestCase):
    """
    Test the python interface to PeaksWorkspace's
    """
    
    def setUp(self):
        LoadEventNexus(Filename='CNCS_7860_event.nxs', OutputWorkspace='cncs')
        CreatePeaksWorkspace(InstrumentWorkspace='cncs', OutputWorkspace='peaks')
        pass
    
    def test_interface(self):
        """ Rudimentary test to get peak and get/set some values """ 
        pws = mtd['peaks']
        self.assertEqual(pws.getNumberPeaks(), 1)
        p = pws.getPeak(0)
        
        # Try a few IPeak get/setters. Not everything.
        p.setH(234)
        self.assertEqual(p.getH(), 234)
        p.setIntensity(456)
        p.setSigmaIntensity(789)
        self.assertEqual(p.getIntensity(), 456)
        self.assertEqual(p.getSigmaIntensity(), 789)
        
        # Finally try to remove a peak
        pws.removePeak(0)
        self.assertEqual(pws.getNumberPeaks(), 0)
        
if __name__ == '__main__':
    unittest.main()

    
