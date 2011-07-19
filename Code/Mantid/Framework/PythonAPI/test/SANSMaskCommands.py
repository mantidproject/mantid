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

        # Use the LOQ setttings
        ISIS.LOQ()

        # Test the LOQ instrument is what it suppose to be 
        self.assertEqual(self.test_ws.getNumberHistograms(), 17778)
        self.assertTrue(self.test_ws.readY(0)[0] > 0)
        
    def test_single_spectrum(self):
        """
            Checks the ISIS specfic mask command for spectra numbers
        """
        spec_nums1 = 7341
        spec_nums2 = 17341

        # flags some whole spectra for masking
        ISIS.Mask('mask S'+str(spec_nums1))
        ISIS.Mask('MASK S'+str(spec_nums2))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), self.test_ws_name)
        # note spectrum index is one less than the spectrum number for the main detector
        self.assertEqual(self.test_ws.readY(spec_nums1-1)[0], 0)
        # for the front detector (HAB) the offset is 5
        self.assertEqual(self.test_ws.readY(spec_nums2-5)[0], 0)
        
    def test_masking_timebins(self):
        """
            Checks the ISIS specfic mask command for spectra numbers
        """
        
        #wsName = 
        ws=Load("LOQ48094.raw","testMaskingTimebins")[0]
       

        # flags some whole spectra for masking
        ISIS.Mask('mask/T '+str(3500)+' '+str(3700))
        ISIS.Mask('mask/time '+str(13500)+' '+str(13700))

        ISIS.ReductionSingleton().mask.execute(ISIS.ReductionSingleton(), "testMaskingTimebins")
        # note spectrum index is one less than the spectrum number for the main detector
        self.assertEqual(ws.readY(0)[0], 0)
        self.assertEqual(ws.readY(0)[1], 0)
        self.assertEqual(ws.readY(0)[3], 218284)       
        # for the front detector (HAB) the offset is 5
        self.assertEqual(ws.readY(0)[54], 0)       

if __name__ == '__main__':
    unittest.main()