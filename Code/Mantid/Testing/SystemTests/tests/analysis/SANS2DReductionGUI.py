#pylint: disable=invalid-name
"""
These tests ensure that all the steps that the SANS Interface GUI performs to reduce SANS data
on the SANS2D instrument is avalailable and is conforming to this test.

Although verbotic, it is all the steps that the GUI calls when asked to perform a full reduction
on SANS2D instrument.

This test also allows an easy comparison of the steps used by the reduction in batch mode and in single mode.

The first 2 Tests ensures that the result provided by the GUI are the same for the minimalistic script.

Test was first created to apply to Mantid Release 3.0.
"""

import sys
import os

if __name__ == "__main__":
  # it is just to allow running this test in Mantid, allowing the following import
    sys.path.append('/apps/mantid/systemtests/StressTestFramework/')

import stresstesting
from mantid.simpleapi import *
import isis_reducer
import ISISCommandInterface as i
import isis_instrument
import isis_reduction_steps
import copy

MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')

def s(obj):
    print '!'+str(obj)+'!',type(obj)

class SANS2DMinimalBatchReduction(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in batch mode
  """
    def __init__(self):
        super(SANS2DMinimalBatchReduction, self).__init__()
        config['default.instrument'] = 'SANS2D'

    def runTest(self):
        import SANSBatchMode as batch
        i.SANS2D()
        i.MaskFile(MASKFILE)
        fit_settings = batch.BatchReduce(BATCHFILE,'.nxs', combineDet='rear')

    def validate(self):
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2
        return "trans_test_rear","SANSReductionGUI.nxs"



class SANS2DMinimalSingleReduction(SANS2DMinimalBatchReduction):
    """Minimal script to perform full reduction in single mode"""
    def runTest(self):
        i.SANS2D()
        i.MaskFile(MASKFILE)
        i.AssignSample('22048')
        i.AssignCan('22023')
        i.TransmissionSample('22041','22024')
        i.TransmissionCan('22024', '22024')
        reduced = i.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')




class SANS2DGUIBatchReduction(SANS2DMinimalBatchReduction):
    """Script executed by SANS GUI Interface to perform Batch Reduction"""

    def checkFloat(self, f1, f2):
        self.assertDelta(f1,f2,0.0001)

    def checkStr(self, s1, s2):
        self.assertTrue(s1==s2, '%s != %s'%(s1,s2))

    def checkObj(self, ob1, ob2):
        self.assertTrue(ob1 == ob2, '%s != %s'%(str(ob1),str(ob2)))

    def checkFirstPart(self):
        self.checkObj(i.ReductionSingleton().instrument.listDetectors(),('rear-detector', 'front-detector'))
        self.checkStr(i.ReductionSingleton().instrument.cur_detector().name() , 'rear-detector')
        self.checkFloat(i.ReductionSingleton().mask.min_radius, 0.041)
        self.checkFloat(i.ReductionSingleton().mask.max_radius, -0.001)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_low, 1.5)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_high, 12.5)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_step, 0.125)
        self.checkStr(i.ReductionSingleton().to_Q.binning,  " .001,.001,.0126,-.08,.2")
        self.checkFloat(i.ReductionSingleton().QXY2,0.05)
        self.checkFloat(i.ReductionSingleton().DQXY, 0.001)
        self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMin('SAMPLE'), 1.5)
        self.checkStr(i.ReductionSingleton().transmission_calculator.fitMethod('SAMPLE'),  'LOGARITHMIC')
        self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMin('CAN'), 1.5)
        self.checkFloat(i.ReductionSingleton().instrument.WAV_RANGE_MIN, 2.0)
        self.checkFloat(i.ReductionSingleton().instrument.WAV_RANGE_MAX, 14.0)
        self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMax('CAN'), 12.5)
        self.checkStr(i.ReductionSingleton().transmission_calculator.fitMethod('CAN'), 'LOGARITHMIC')
        self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMin('SAMPLE'), 1.5)
        self.checkStr(i.ReductionSingleton().transmission_calculator.fitMethod('SAMPLE'), 'LOGARITHMIC')
        self.checkFloat(i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale, 1.0)
        self.checkFloat(i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift, 0.0)
        self.assertTrue(not i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.fitScale)
        self.assertTrue(not i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.fitShift)
        self.assertTrue(not i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.qRangeUserSelected)
        self.checkFloat(i.ReductionSingleton().instrument.get_incident_mon(), 1)
        self.checkFloat(i.ReductionSingleton().instrument.incid_mon_4_trans_calc, 1)
        self.assertTrue(i.ReductionSingleton().instrument.is_interpolating_norm())
        self.assertTrue(i.ReductionSingleton().transmission_calculator.interpolate)
        self.assertTrue("DIRECTM1_15785_12m_31Oct12_v12.dat" in i.ReductionSingleton().instrument.detector_file('rear'))
        self.assertTrue("DIRECTM1_15785_12m_31Oct12_v12.dat" in i.ReductionSingleton().instrument.detector_file('front'))
        self.checkStr(i.ReductionSingleton().prep_normalize.getPixelCorrFile('REAR'), "")
        self.checkStr(i.ReductionSingleton().prep_normalize.getPixelCorrFile('FRONT'), "")
        self.checkFloat(i.ReductionSingleton()._corr_and_scale.rescale, 7.4)
        self.checkFloat(i.ReductionSingleton().instrument.SAMPLE_Z_CORR, 0.053)
        self.assertDelta(i.ReductionSingleton().get_beam_center('rear')[0], 0.15545,0.0001)
        self.checkFloat(i.ReductionSingleton().get_beam_center('rear')[1], -0.16965)
        self.checkFloat(i.ReductionSingleton().get_beam_center('front')[0], 0.15545)
        self.checkFloat(i.ReductionSingleton().get_beam_center('front')[1], -0.16965)
        self.assertTrue(i.ReductionSingleton().to_Q.get_gravity())
        self.checkStr(i.ReductionSingleton().instrument.det_selection, 'REAR')
        self.checkFloat(i.ReductionSingleton().mask.phi_min, -90.0)
        self.checkFloat(i.ReductionSingleton().mask.phi_max, 90.0)
        self.checkStr(i.ReductionSingleton().mask.spec_mask_r, ",H0,H190>H191,H167>H172,V0,V191")
        self.checkStr(i.ReductionSingleton().mask.spec_mask_f, ",H0,H190>H191,V0,V191,H156>H159")
        self.checkStr(i.ReductionSingleton().mask.time_mask,  ";17500 22000")
        self.checkStr(i.ReductionSingleton().mask.time_mask_r, "")
        self.checkStr(i.ReductionSingleton().mask.time_mask_f, "")
        self.checkStr(i.ReductionSingleton().mask.time_mask_f, "")
        self.assertTrue(i.ReductionSingleton().mask.arm_width is None)
        self.assertTrue(i.ReductionSingleton().mask.arm_angle is None)
        self.assertTrue(i.ReductionSingleton().mask.arm_x is None)
        self.assertTrue(i.ReductionSingleton().mask.arm_y is None)
        self.assertTrue(i.ReductionSingleton().mask.phi_mirror)

    def applyGUISettings(self):
        i.ReductionSingleton().instrument.setDetector('rear-detector')
        i.ReductionSingleton().to_Q.output_type='1D'
        i.ReductionSingleton().user_settings.readLimitValues('L/R '+'41 '+'-1 '+'1', i.ReductionSingleton())
        i.LimitsWav(1.5,12.5,0.125,'LIN')
        i.ReductionSingleton().user_settings.readLimitValues('L/Q .001,.001,.0126,-.08,.2', i.ReductionSingleton())
        i.LimitsQXY(0.0,0.05,0.001,'LIN')
        i.SetPhiLimit(-90.0,90.0, True)
        i.SetDetectorFloodFile('','REAR')
        i.SetDetectorFloodFile('','FRONT')
        i.TransFit(mode='Logarithmic', lambdamin='1.5', lambdamax='12.5', selector='BOTH')
        i.SetFrontDetRescaleShift(scale=1.0,shift=0.0)
        i.Gravity(True)
        i.SetSampleOffset('53')
        i.SetMonitorSpectrum('1',True)
        i.SetTransSpectrum('1',True)
        i.SetCentre('155.45','-169.6','rear')
        i.SetCentre('155.45','-169.6','front')
        i.Mask('MASK/CLEAR')
        i.Mask('MASK/CLEAR/TIME')
        i.Mask('MASK/REAR H0')
        i.Mask('MASK/REAR H190>H191')
        i.Mask('MASK/REAR H167>H172')
        i.Mask('MASK/REAR V0')
        i.Mask('MASK/REAR V191')
        i.Mask('MASK/FRONT H0')
        i.Mask('MASK/FRONT H190>H191')
        i.Mask('MASK/FRONT V0')
        i.Mask('MASK/FRONT V191')
        i.Mask('MASK/FRONT H156>H159')
        i.Mask('MASK/TIME 17500 22000')
        i.Mask('L/PHI -90.0 90.0')
        i.SetVerboseMode(True)

    def checkFittingSettings(self, fitdict):
        self.checkFloat(fitdict['scale'], 1.0)
        self.checkFloat(fitdict['shift'], 0.0)



    def initialization(self):
        if i.ReductionSingleton().get_instrument() != 'SANS2D':
            i.ReductionSingleton.clean(isis_reducer.ISISReducer)
        i.ReductionSingleton().set_instrument(isis_instrument.SANS2D())

        i.ReductionSingleton.clean(isis_reducer.ISISReducer)
        i.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
        i.ReductionSingleton().user_settings =isis_reduction_steps.UserFile(MASKFILE)
        i.ReductionSingleton().user_settings.execute(i.ReductionSingleton())
        return i

    def runTest(self):
        self.initialization()

        self.checkFirstPart()

        import SANSBatchMode as batch

        self.applyGUISettings()

        _user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)

        fit_settings={'scale':1.0,'shift':0.0}
        fit_settings = batch.BatchReduce(BATCHFILE,'.nxs', saveAlgs={}, reducer=i.ReductionSingleton().reference(),combineDet='rear')

        self.checkFittingSettings(fit_settings)

    def validate(self):
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2
        return "trans_test_rear","SANSReductionGUI.nxs"

class SANS2DGUIReduction(SANS2DGUIBatchReduction):
    """Script executed by SANS GUI Interface to perform reduction in single mode"""

    def checkAfterLoad(self):
        self.checkFloat(i.ReductionSingleton().get_sample().loader.periods_in_file, 1)
        self.checkFloat(i.ReductionSingleton().background_subtracter.periods_in_file, 1)
        self.checkFloat(i.ReductionSingleton().samp_trans_load.direct.periods_in_file, 1)
        self.checkFloat(i.ReductionSingleton().can_trans_load.direct.periods_in_file,1)
        self.assertTrue(not  i.GetMismatchedDetList())

    def loadSettings(self):
        i.ReductionSingleton().instrument.setDetector('rear-detector')
        i.SetCentre('155.45','-169.6','rear')
        i.SetCentre('155.45','-169.6','front')
        SCATTER_SAMPLE, logvalues = i.AssignSample(r'SANS2D00022048.nxs', reload = True, period = 1)

        i.SetCentre('155.45','-169.6','rear')
        i.SetCentre('155.45','-169.6','front')
        SCATTER_SAMPLE, logvalues = i.AssignCan(r'SANS2D00022023.nxs', reload = True, period = 1)

        t1, t2 = i.TransmissionSample(r'SANS2D00022041.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)

        t1, t2 = i.TransmissionCan(r'SANS2D00022024.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)

    def applySampleSettings(self):
        i.ReductionSingleton().get_sample().geometry.shape = 3
        i.ReductionSingleton().get_sample().geometry.height = 8
        i.ReductionSingleton().get_sample().geometry.width = 8
        i.ReductionSingleton().get_sample().geometry.thickness = 2


    def checkFittingSettings(self):
        settings = {'scale':i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale,
                'shift':i.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift}
        super(SANS2DGUIReduction,self).checkFittingSettings(settings)


    def cleanReduction(self, user_settings):
        i.ReductionSingleton.clean(isis_reducer.ISISReducer)
        i.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
    #i.ReductionSingleton().user_file_path=''
        i.ReductionSingleton().user_settings = user_settings
        i.ReductionSingleton().user_settings.execute(i.ReductionSingleton())



    def singleModePrepare(self):
        self.initialization()

        self.checkFirstPart()

        self.loadSettings()

        self.checkAfterLoad()

        self.applyGUISettings()

        self.applySampleSettings()

    def runTest(self):
        self.singleModePrepare()

        _user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)

        reduced = i.WavRangeReduction(full_trans_wav=False, resetSetup=False)

        self.checkFittingSettings()

        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

        self.cleanReduction(_user_settings_copy)

        _user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)



if __name__ == "__main__":
  #test = SANS2DGUIBatchReduction()
  #test.execute()
    test = SANS2DGUIReduction()
    test.execute()
