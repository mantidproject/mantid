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
        Detector("rear-detector")
        Gravity(True)
        MaskFile("../../../Test/Data/SANS2D/MASKSANS2Doptions.091A")        
        AssignCan('993.raw')
        TransmissionSample('988.raw', '987.raw')
        TransmissionCan('989.raw', '987.raw')
        
        AppendDataFile('SANS2D00000992.raw')
        Reduce1D()
        
                
        SANSReduction.DataPath("../../../Test/Data/SANS2D")
        SANSReduction.UserPath("../../../Test/Data/SANS2D/")
        SANSReduction.SANS2D()
        SANSReduction.Set1D()
        SANSReduction.Gravity(True)
        SANSReduction.Detector("rear-detector")
        SANSReduction.MaskFile('MASKSANS2Doptions.091A')
        SANSReduction.AssignCan('993.raw')
        SANSReduction.TransmissionSample('988.raw', '987.raw')
        SANSReduction.TransmissionCan('989.raw', '987.raw')
        
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
        
        self.assertEqual(ReductionSingleton().instrument.SAMPLE_Z_CORR,
                         SANSReduction.INSTRUMENT.SAMPLE_Z_CORR)
        
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
        
        self.assertEqual(ReductionSingleton().RMIN,
                         SANSReduction.RMIN)
        self.assertEqual(ReductionSingleton().RMAX,
                         SANSReduction.RMAX)
        self.assertEqual(ReductionSingleton().DEF_RMIN,
                         SANSReduction.DEF_RMIN)
        self.assertEqual(ReductionSingleton().DEF_RMAX,
                         SANSReduction.DEF_RMAX)

        self.assertEqual(ReductionSingleton().WAV1,
                         SANSReduction.WAV1)
        self.assertEqual(ReductionSingleton().WAV2,
                         SANSReduction.WAV2)
        self.assertEqual(ReductionSingleton().DWAV,
                         SANSReduction.DWAV)
        self.assertEqual(ReductionSingleton().Q_REBIN,
                         SANSReduction.Q_REBIN)
        self.assertEqual(ReductionSingleton().QXY2,
                         SANSReduction.QXY2)
        self.assertEqual(ReductionSingleton().DQY,
                         SANSReduction.DQY)
        
        self.assertEqual(ReductionSingleton().instrument.incid_mon_4_trans_calc,
                         SANSReduction.INSTRUMENT.incid_mon_4_trans_calc)
        
        self.assertEqual(ReductionSingleton().instrument._incid_monitor,
                         SANSReduction.INSTRUMENT._incid_monitor)
        self.assertEqual(ReductionSingleton().instrument._incid_monitor,
                         SANSReduction.INSTRUMENT._incid_monitor)
        
        self.assertEqual(ReductionSingleton()._beam_finder._beam_center_x,
                         SANSReduction.XBEAM_CENTRE)
        self.assertEqual(ReductionSingleton()._beam_finder._beam_center_y,
                         SANSReduction.YBEAM_CENTRE)

        self.assertEqual(ReductionSingleton().RESCALE,
                         SANSReduction.RESCALE)
        self.assertEqual(ReductionSingleton()._use_gravity,
                         SANSReduction.GRAVITY)
        self.assertEqual(ReductionSingleton().BACKMON_START,
                         SANSReduction.BACKMON_START)
        self.assertEqual(ReductionSingleton().BACKMON_END,
                         SANSReduction.BACKMON_END)
        
        self.assertEqual(ReductionSingleton().instrument.cur_detector().name(),
                         SANSReduction.INSTRUMENT.cur_detector().name())
        
        # Mask
        self.assertEqual(ReductionSingleton()._mask._timemask,
                         SANSReduction.TIMEMASKSTRING)
        self.assertEqual(ReductionSingleton()._mask._timemask_r,
                         SANSReduction.TIMEMASKSTRING_R)
        self.assertEqual(ReductionSingleton()._mask._timemask_f,
                         SANSReduction.TIMEMASKSTRING_F)
        self.assertEqual(ReductionSingleton()._mask._specmask,
                         SANSReduction.SPECMASKSTRING)
        self.assertEqual(ReductionSingleton()._mask._specmask_r,
                         SANSReduction.SPECMASKSTRING_R)
        self.assertEqual(ReductionSingleton()._mask._specmask_f,
                         SANSReduction.SPECMASKSTRING_F)
        
        # Can run
        #self.assertEqual(ReductionSingleton()._background_subtracter.SCATTER_CAN,
        #                 SANSReduction.SCATTER_CAN)
        self.assertEqual(ReductionSingleton()._background_subtracter.PERIOD_NOS,
                         SANSReduction.PERIOD_NOS)     
        #self.assertEqual(ReductionSingleton()._background_subtracter.SCATTER_SAMPLE,
        #                 SANSReduction.SCATTER_SAMPLE)
        self.assertEqual(ReductionSingleton()._transmission_calculator.TRANS_SAMPLE,
                         SANSReduction.TRANS_SAMPLE)
        self.assertEqual(ReductionSingleton()._transmission_calculator.TRANS_CAN,
                         SANSReduction.TRANS_CAN)
        self.assertEqual(ReductionSingleton()._transmission_calculator.DIRECT_SAMPLE,
                         SANSReduction.DIRECT_SAMPLE)
        self.assertEqual(ReductionSingleton()._transmission_calculator.DIRECT_CAN,
                         SANSReduction.DIRECT_CAN)
        self.assertEqual(ReductionSingleton().instrument._marked_dets,
                         SANSReduction.INSTRUMENT._marked_dets)


if __name__ == '__main__':
    unittest.main()
    
    
        
        