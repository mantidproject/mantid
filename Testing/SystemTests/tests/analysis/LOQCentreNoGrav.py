#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
import mantid # noqa
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class LOQCentreNoGrav(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):

        ii.LOQ()
        ii.Set1D()
        ii.Detector("rear-detector")
        ii.MaskFile('MASK.094AA')
        ii.Gravity(False)

        ii.AssignSample('54431.raw')
        ii.TransmissionSample('54435.raw', '54433.raw')
        ii.AssignCan('54432.raw')
        ii.TransmissionCan('54434.raw', '54433.raw')

        ii.FindBeamCentre(60,200, 9)

        ii.WavRangeReduction(3, 9, ii.DefaultTrans)

    def validate(self):
        self.disableChecking.append('Instrument')
        return '54431main_1D_3.0_9.0','LOQCentreNoGravSearchCentreFixed.nxs'


class LOQCentreNoGravDefineCentre(stresstesting.MantidStressTest):
    def runTest(self):

        ii.LOQ()

        ii.Set1D()
        ii.Detector("rear-detector")
        ii.MaskFile('MASK.094AA')
        ii.Gravity(False)
        ii.SetCentre(324.765, 327.670)

        ii.AssignSample('54431.raw')
        ii.TransmissionSample('54435.raw', '54433.raw')
        ii.AssignCan('54432.raw')
        ii.TransmissionCan('54434.raw', '54433.raw')

        ii.WavRangeReduction(3, 9, ii.DefaultTrans)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map becauseit isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return '54431main_1D_3.0_9.0','LOQCentreNoGrav_V2.nxs'


class LOQCentreNoGrav_V2(stresstesting.MantidStressTest):
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LOQ()

        ii2.Set1D()
        ii2.Detector("rear-detector")
        ii2.MaskFile('MASK.094AA')
        ii2.Gravity(False)

        ii2.AssignSample('54431.raw')
        ii2.TransmissionSample('54435.raw', '54433.raw')
        ii2.AssignCan('54432.raw')
        ii2.TransmissionCan('54434.raw', '54433.raw')

        ii2.FindBeamCentre(60, 200, 9)

        ii2.WavRangeReduction(3, 9, ii2.DefaultTrans)

    def validate(self):
        self.disableChecking.append('Instrument')
        return '54431main_1D_3.0_9.0','LOQCentreNoGravSearchCentreFixed.nxs'


class LOQCentreNoGravDefineCentreTest_V2(stresstesting.MantidStressTest):
    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LOQ()
        ii2.Set1D()
        ii2.Detector("rear-detector")
        ii2.MaskFile('MASK.094AA')
        ii2.Gravity(False)
        ii2.SetCentre(324.765, 327.670)

        ii2.AssignSample('54431.raw')
        ii2.TransmissionSample('54435.raw', '54433.raw')
        ii2.AssignCan('54432.raw')
        ii2.TransmissionCan('54434.raw', '54433.raw')

        ii2.WavRangeReduction(3, 9, ii2.DefaultTrans)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map becauseit isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return '54431main_1D_3.0_9.0', 'LOQCentreNoGrav_V2.nxs'
