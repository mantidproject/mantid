import unittest
from mantidsimple import *
import ISISCommandInterface as ISIS

class SANSMaskCommands(unittest.TestCase):
    """
        Tests SANS workflow algorithms
    """
    
    def setUp(self):
        
        self.test_ws_name = "SANSMaskCommands_ws"
        LoadEmptyInstrument('LOQ_Definition_20020226-.xml', self.test_ws_name)
        self.test_ws = mantid[self.test_ws_name]

        ISIS.LOQ()

        self.assertEqual(self.test_ws.getNumberHistograms(), 17778)
        self.assertTrue(self.test_ws.readY(0)[0] > 0)
        
    def test_single_spectrum(self):
        """
            Checks the ISIS specfic mask command for spectra numbers
        """
        spec_nums1 = 7341
        spec_nums2 = 17341

        ISIS.Mask('mask S'+str(spec_nums1))
        ISIS.Mask('MASK S'+str(spec_nums2))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), self.test_ws_name)
        self.assertEqual(self.test_ws.readY(spec_nums1-1)[0], 0)
#        self.assertEqual(self.test_ws.readY(spec_nums2-1)[0], 0)

if __name__ == '__main__':
    unittest.main()