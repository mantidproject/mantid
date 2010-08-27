"""
    Unit tests for ISIS SANS reduction
"""
from MantidFramework import *
mtd.initialise()
from mantidsimple import *

import unittest
from ISISCommandInterface import *

import SANSReduction


class TestInstrument(unittest.TestCase):
    """
       Testing the SANSInsts module interface, mostly information
       about the detectors
    """
    
    def test_sans2d_command(self):
        SANS2D()
        self.assertEqual(ReductionSingleton().instrument.__class__.__name__, "SANS2D")
        
    def test_loq_command(self):
        LOQ()
        self.assertEqual(ReductionSingleton().instrument.__class__.__name__, "LOQ")
        
    def test_reduction(self):
        DataPath("../../../Test/Data/SANS2D")
        SANS2D()
        
    def test_mask_file(self):
        SANS2D()
        DataPath("../../../Test/Data/SANS2D")
        UserPath("../../../Test/Data/SANS2D/")
        ReductionSingleton().read_mask_file("../../../Test/Data/SANS2D/MASKSANS2Doptions.091A")        
    
                
        SANSReduction.DataPath("../../../Test/Data/SANS2D")
        SANSReduction.UserPath("../../../Test/Data/SANS2D/")
        SANSReduction.SANS2D()
        SANSReduction.Set1D()
        SANSReduction.Detector("rear-detector")
        SANSReduction.MaskFile('MASKSANS2Doptions.091A')
        
        self.assertEqual(ReductionSingleton().instrument.FRONT_DET_Z_CORR,
                         SANSReduction.INSTRUMENT.FRONT_DET_Z_CORR)
        
        self.assertEqual(ReductionSingleton().instrument.FRONT_DET_Y_CORR,
                         SANSReduction.INSTRUMENT.FRONT_DET_Y_CORR)
        
        self.assertEqual(ReductionSingleton().instrument.FRONT_DET_X_CORR,
                         SANSReduction.INSTRUMENT.FRONT_DET_X_CORR)
        
        self.assertEqual(ReductionSingleton().instrument.FRONT_DET_ROT_CORR,
                         SANSReduction.INSTRUMENT.FRONT_DET_ROT_CORR)
        
        self.assertEqual(ReductionSingleton().instrument.REAR_DET_Z_CORR,
                         SANSReduction.INSTRUMENT.REAR_DET_Z_CORR)

        self.assertEqual(ReductionSingleton().instrument.REAR_DET_X_CORR,
                         SANSReduction.INSTRUMENT.REAR_DET_X_CORR)
        
        self.assertEqual(ReductionSingleton()._user_path,
                         SANSReduction.USER_PATH)
        
        self.assertEqual(ReductionSingleton().DIRECT_BEAM_FILE_F,
                         SANSReduction.DIRECT_BEAM_FILE_F)
        
        self.assertEqual(ReductionSingleton().DIRECT_BEAM_FILE_R,
                         SANSReduction.DIRECT_BEAM_FILE_R)
        
        self.assertEqual(ReductionSingleton().SAMPLE_WIDTH,
                         SANSReduction.SAMPLE_WIDTH)
        
        self.assertEqual(ReductionSingleton().SAMPLE_HEIGHT,
                         SANSReduction.SAMPLE_HEIGHT)
        
        self.assertEqual(ReductionSingleton().SAMPLE_THICKNESS,
                         SANSReduction.SAMPLE_THICKNESS)
        
        self.assertEqual(ReductionSingleton().PHIMIN,
                         SANSReduction.PHIMIN)
        
        self.assertEqual(ReductionSingleton().PHIMAX,
                         SANSReduction.PHIMAX)
        
        self.assertEqual(ReductionSingleton().PHIMIRROR,
                         SANSReduction.PHIMIRROR)
        
        # TRANSMISSION
        self.assertEqual(ReductionSingleton()._transmission_calculator._lambda_min,
                         SANSReduction.TRANS_WAV1)
        
        self.assertEqual(ReductionSingleton()._transmission_calculator._lambda_max,
                         SANSReduction.TRANS_WAV2)
        
        self.assertEqual(ReductionSingleton()._transmission_calculator._fit_method,
                         SANSReduction.TRANS_FIT)
        


if __name__ == '__main__':
    unittest.main()
    
    
        
        