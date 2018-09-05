#pylint: disable=invalid-name,attribute-defined-outside-init
"""
These tests ensure that all the steps that the SANS Interface GUI performs to reduce SANS data
on the SANS2D instrument is avalailable and is conforming to this test.

Although verbotic, it is all the steps that the GUI calls when asked to perform a full reduction
on SANS2D instrument.

This test also allows an easy comparison of the steps used by the reduction in batch mode and in single mode.

The first 2 Tests ensures that the result provided by the GUI are the same for the minimalistic script.

Test was first created to apply to Mantid Release 3.0.
"""

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
import isis_reducer
import isis_instrument
import isis_reduction_steps
import copy
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2

MASKFILE = FileFinder.getFullPath('MaskSANS2DReductionGUI.txt')
BATCHFILE = FileFinder.getFullPath('sans2d_reduction_gui_batch.csv')


def s(obj):
    print('!'+str(obj)+'!',type(obj))


class SANS2DMinimalBatchReduction(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in batch mode
  """

    def __init__(self):
        super(SANS2DMinimalBatchReduction, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        import SANSBatchMode as batch
        ii.SANS2D()
        ii.MaskFile(MASKFILE)
        batch.BatchReduce(BATCHFILE,'.nxs', combineDet='rear')

    def validate(self):
        self.disableChecking.append('Instrument')
        return "trans_test_rear_1D_1.5_12.5","SANSReductionGUI.nxs"


class SANS2DMinimalSingleReduction(SANS2DMinimalBatchReduction):
    """Minimal script to perform full reduction in single mode"""

    def runTest(self):
        ii.SANS2D()
        ii.MaskFile(MASKFILE)
        ii.AssignSample('22048')
        ii.AssignCan('22023')
        ii.TransmissionSample('22041','22024')
        ii.TransmissionCan('22024', '22024')
        reduced = ii.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear_1D_1.5_12.5')


class SANS2DGUIBatchReduction(SANS2DMinimalBatchReduction):
    """Script executed by SANS GUI Interface to perform Batch Reduction"""

    def checkFloat(self, f1, f2):
        self.assertDelta(f1,f2,0.0001)

    def checkStr(self, s1, s2):
        self.assertTrue(s1==s2, '%s != %s'%(s1,s2))

    def checkObj(self, ob1, ob2):
        self.assertTrue(ob1 == ob2, '%s != %s'%(str(ob1),str(ob2)))

    def checkFirstPart(self):
        self.checkObj(ii.ReductionSingleton().instrument.listDetectors(),('rear-detector', 'front-detector'))
        self.checkStr(ii.ReductionSingleton().instrument.cur_detector().name() , 'rear-detector')
        self.checkFloat(ii.ReductionSingleton().mask.min_radius, 0.041)
        self.checkFloat(ii.ReductionSingleton().mask.max_radius, -0.001)
        self.checkFloat(ii.ReductionSingleton().to_wavelen.wav_low, 1.5)
        self.checkFloat(ii.ReductionSingleton().to_wavelen.wav_high, 12.5)
        self.checkFloat(ii.ReductionSingleton().to_wavelen.wav_step, 0.125)
        self.checkStr(ii.ReductionSingleton().to_Q.binning,  " .001,.001,.0126,-.08,.2")
        self.checkFloat(ii.ReductionSingleton().QXY2,0.05)
        self.checkFloat(ii.ReductionSingleton().DQXY, 0.001)
        self.checkFloat(ii.ReductionSingleton().transmission_calculator.lambdaMin('SAMPLE'), 1.5)
        self.checkStr(ii.ReductionSingleton().transmission_calculator.fitMethod('SAMPLE'),  'LOGARITHMIC')
        self.checkFloat(ii.ReductionSingleton().transmission_calculator.lambdaMin('CAN'), 1.5)
        self.checkFloat(ii.ReductionSingleton().instrument.WAV_RANGE_MIN, 2.0)
        self.checkFloat(ii.ReductionSingleton().instrument.WAV_RANGE_MAX, 14.0)
        self.checkFloat(ii.ReductionSingleton().transmission_calculator.lambdaMax('CAN'), 12.5)
        self.checkStr(ii.ReductionSingleton().transmission_calculator.fitMethod('CAN'), 'LOGARITHMIC')
        self.checkFloat(ii.ReductionSingleton().transmission_calculator.lambdaMin('SAMPLE'), 1.5)
        self.checkStr(ii.ReductionSingleton().transmission_calculator.fitMethod('SAMPLE'), 'LOGARITHMIC')
        self.checkFloat(ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale, 1.0)
        self.checkFloat(ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift, 0.0)
        self.assertTrue(not ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.fitScale)
        self.assertTrue(not ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.fitShift)
        self.assertTrue(not ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.qRangeUserSelected)
        self.checkFloat(ii.ReductionSingleton().instrument.get_incident_mon(), 1)
        self.checkFloat(ii.ReductionSingleton().instrument.incid_mon_4_trans_calc, 1)
        self.assertTrue(ii.ReductionSingleton().instrument.is_interpolating_norm())
        self.assertTrue(ii.ReductionSingleton().transmission_calculator.interpolate)
        self.assertTrue("DIRECTM1_15785_12m_31Oct12_v12.dat" in ii.ReductionSingleton().instrument.detector_file('rear'))
        self.assertTrue("DIRECTM1_15785_12m_31Oct12_v12.dat" in ii.ReductionSingleton().instrument.detector_file('front'))
        self.checkStr(ii.ReductionSingleton().prep_normalize.getPixelCorrFile('REAR'), "")
        self.checkStr(ii.ReductionSingleton().prep_normalize.getPixelCorrFile('FRONT'), "")
        self.checkFloat(ii.ReductionSingleton()._corr_and_scale.rescale, 7.4)
        self.checkFloat(ii.ReductionSingleton().instrument.SAMPLE_Z_CORR, 0.053)
        self.assertDelta(ii.ReductionSingleton().get_beam_center('rear')[0], 0.15545,0.0001)
        self.checkFloat(ii.ReductionSingleton().get_beam_center('rear')[1], -0.16965)
        self.checkFloat(ii.ReductionSingleton().get_beam_center('front')[0], 0.15545)
        self.checkFloat(ii.ReductionSingleton().get_beam_center('front')[1], -0.16965)
        self.assertTrue(ii.ReductionSingleton().to_Q.get_gravity())
        self.checkStr(ii.ReductionSingleton().instrument.det_selection, 'REAR')
        self.checkFloat(ii.ReductionSingleton().mask.phi_min, -90.0)
        self.checkFloat(ii.ReductionSingleton().mask.phi_max, 90.0)
        self.checkStr(ii.ReductionSingleton().mask.spec_mask_r, ",H0,H190>H191,H167>H172,V0,V191")
        self.checkStr(ii.ReductionSingleton().mask.spec_mask_f, ",H0,H190>H191,V0,V191,H156>H159")
        self.checkStr(ii.ReductionSingleton().mask.time_mask,  ";17500 22000")
        self.checkStr(ii.ReductionSingleton().mask.time_mask_r, "")
        self.checkStr(ii.ReductionSingleton().mask.time_mask_f, "")
        self.checkStr(ii.ReductionSingleton().mask.time_mask_f, "")
        self.assertTrue(ii.ReductionSingleton().mask.arm_width is None)
        self.assertTrue(ii.ReductionSingleton().mask.arm_angle is None)
        self.assertTrue(ii.ReductionSingleton().mask.arm_x is None)
        self.assertTrue(ii.ReductionSingleton().mask.arm_y is None)
        self.assertTrue(ii.ReductionSingleton().mask.phi_mirror)

    def applyGUISettings(self):
        ii.ReductionSingleton().instrument.setDetector('rear-detector')
        ii.ReductionSingleton().to_Q.output_type='1D'
        ii.ReductionSingleton().user_settings.readLimitValues('L/R '+'41 '+'-1 '+'1', ii.ReductionSingleton())
        ii.LimitsWav(1.5,12.5,0.125,'LIN')
        ii.ReductionSingleton().user_settings.readLimitValues('L/Q .001,.001,.0126,-.08,.2', ii.ReductionSingleton())
        ii.LimitsQXY(0.0,0.05,0.001,'LIN')
        ii.SetPhiLimit(-90.0,90.0, True)
        ii.SetDetectorFloodFile('','REAR')
        ii.SetDetectorFloodFile('','FRONT')
        ii.TransFit(mode='Logarithmic', lambdamin='1.5', lambdamax='12.5', selector='BOTH')
        ii.SetFrontDetRescaleShift(scale=1.0,shift=0.0)
        ii.Gravity(True)
        ii.SetSampleOffset('53')
        ii.SetMonitorSpectrum('1',True)
        ii.SetTransSpectrum('1',True)
        ii.SetCentre('155.45','-169.6','rear')
        ii.SetCentre('155.45','-169.6','front')
        ii.Mask('MASK/CLEAR')
        ii.Mask('MASK/CLEAR/TIME')
        ii.Mask('MASK/REAR H0')
        ii.Mask('MASK/REAR H190>H191')
        ii.Mask('MASK/REAR H167>H172')
        ii.Mask('MASK/REAR V0')
        ii.Mask('MASK/REAR V191')
        ii.Mask('MASK/FRONT H0')
        ii.Mask('MASK/FRONT H190>H191')
        ii.Mask('MASK/FRONT V0')
        ii.Mask('MASK/FRONT V191')
        ii.Mask('MASK/FRONT H156>H159')
        ii.Mask('MASK/TIME 17500 22000')
        ii.Mask('L/PHI -90.0 90.0')
        ii.SetVerboseMode(True)

    def checkFittingSettings(self, fitdict):
        self.checkFloat(fitdict['scale'], 1.0)
        self.checkFloat(fitdict['shift'], 0.0)

    def initialization(self):
        if ii.ReductionSingleton().get_instrument() != 'SANS2D':
            ii.ReductionSingleton.clean(isis_reducer.ISISReducer)
        ii.ReductionSingleton().set_instrument(isis_instrument.SANS2D())

        ii.ReductionSingleton.clean(isis_reducer.ISISReducer)
        ii.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
        ii.ReductionSingleton().user_settings =isis_reduction_steps.UserFile(MASKFILE)
        ii.ReductionSingleton().user_settings.execute(ii.ReductionSingleton())
        return ii

    def runTest(self):
        self.initialization()

        self.checkFirstPart()

        import SANSBatchMode as batch

        self.applyGUISettings()

        fit_settings={'scale':1.0,'shift':0.0}
        fit_settings = batch.BatchReduce(BATCHFILE,'.nxs', saveAlgs={}, reducer=ii.ReductionSingleton().reference(),combineDet='rear')

        self.checkFittingSettings(fit_settings)

    def validate(self):
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2
        self.disableChecking.append('Instrument')
        return "trans_test_rear_1D_1.5_12.5","SANSReductionGUI.nxs"


class SANS2DGUIReduction(SANS2DGUIBatchReduction):
    """Script executed by SANS GUI Interface to perform reduction in single mode"""

    def checkAfterLoad(self):
        self.checkFloat(ii.ReductionSingleton().get_sample().loader.periods_in_file, 1)
        self.checkFloat(ii.ReductionSingleton().background_subtracter.periods_in_file, 1)
        self.checkFloat(ii.ReductionSingleton().samp_trans_load.direct.periods_in_file, 1)
        self.checkFloat(ii.ReductionSingleton().can_trans_load.direct.periods_in_file,1)
        self.assertTrue(not  ii.GetMismatchedDetList())

    def loadSettings(self):
        ii.ReductionSingleton().instrument.setDetector('rear-detector')
        ii.SetCentre('155.45','-169.6','rear')
        ii.SetCentre('155.45','-169.6','front')
        SCATTER_SAMPLE, logvalues = ii.AssignSample(r'SANS2D00022048.nxs', reload = True, period = 1)

        ii.SetCentre('155.45','-169.6','rear')
        ii.SetCentre('155.45','-169.6','front')
        SCATTER_SAMPLE, logvalues = ii.AssignCan(r'SANS2D00022023.nxs', reload = True, period = 1)

        t1, t2 = ii.TransmissionSample(r'SANS2D00022041.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)

        t1, t2 = ii.TransmissionCan(r'SANS2D00022024.nxs', r'SANS2D00022024.nxs', period_t=1, period_d=1)

    def applySampleSettings(self):
        ii.ReductionSingleton().get_sample().geometry.shape = 3
        ii.ReductionSingleton().get_sample().geometry.height = 8
        ii.ReductionSingleton().get_sample().geometry.width = 8
        ii.ReductionSingleton().get_sample().geometry.thickness = 2

    def checkFittingSettings(self):
        settings = {'scale':ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.scale,
                    'shift':ii.ReductionSingleton().instrument.getDetector('FRONT').rescaleAndShift.shift}
        super(SANS2DGUIReduction,self).checkFittingSettings(settings)

    def cleanReduction(self, user_settings):
        ii.ReductionSingleton.clean(isis_reducer.ISISReducer)
        ii.ReductionSingleton().set_instrument(isis_instrument.SANS2D())
    #ii.ReductionSingleton().user_file_path=''
        ii.ReductionSingleton().user_settings = user_settings
        ii.ReductionSingleton().user_settings.execute(ii.ReductionSingleton())

    def singleModePrepare(self):
        self.initialization()

        self.checkFirstPart()

        self.loadSettings()

        self.checkAfterLoad()

        self.applyGUISettings()

        self.applySampleSettings()

    def runTest(self):
        self.singleModePrepare()

        _user_settings_copy = copy.deepcopy(ii.ReductionSingleton().user_settings)

        reduced = ii.WavRangeReduction(full_trans_wav=False, resetSetup=False)

        self.checkFittingSettings()

        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear_1D_1.5_12.5')

        self.cleanReduction(_user_settings_copy)

        _user_settings_copy = copy.deepcopy(ii.ReductionSingleton().user_settings)


class SANS2DMinimalBatchReductionTest_V2(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in batch mode
    """
    def __init__(self):
        super(SANS2DMinimalBatchReductionTest_V2, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile(MASKFILE)
        ii2.BatchReduce(BATCHFILE, '.nxs', combineDet='rear')

    def validate(self):
        self.disableChecking.append('Instrument')
        return "trans_test_rear", "SANSReductionGUI.nxs"


class SANS2DMinimalSingleReductionTest_V2(stresstesting.MantidStressTest):
    """Minimal script to perform full reduction in single mode"""

    def __init__(self):
        super(SANS2DMinimalSingleReductionTest_V2, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile(MASKFILE)
        ii2.AssignSample('22048')
        ii2.AssignCan('22023')
        ii2.TransmissionSample('22041', '22024')
        ii2.TransmissionCan('22024', '22024')
        reduced = ii2.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace='trans_test_rear')

    def validate(self):
        self.disableChecking.append('Instrument')
        return "trans_test_rear", "SANSReductionGUI.nxs"


class SANS2DSearchCentreGUI_V2(stresstesting.MantidStressTest):
    """Minimal script to perform FindBeamCentre"""

    def __init__(self):
        super(SANS2DSearchCentreGUI_V2, self).__init__()
        config['default.instrument'] = 'SANS2D'
        self.tolerance_is_reller = True
        self.tolerance = 1.0e-2

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile(MASKFILE)
        ii2.AssignSample('22048')
        ii2.AssignCan('22023')
        ii2.TransmissionSample('22041', '22024')
        ii2.TransmissionCan('22024', '22024')
        centre = ii2.FindBeamCentre(rlow=41.0, rupp=280.0, xstart=float(150)/1000., ystart=float(-160)/1000.,
                                    tolerance=0.0001251, MaxIter=3, reduction_method=True)
        self.assertDelta(centre['pos2'], -0.145, 0.0001)
        self.assertDelta(centre['pos1'], 0.15, 0.0001)

    def validate(self):
        # there is no workspace to be checked against
        return True


if __name__ == "__main__":
  #test = SANS2DGUIBatchReduction()
  #test.execute()
    test = SANS2DGUIReduction()
    test.execute()
