#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import mantid # noqa
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2

# Test is giving odd results on Linux, but only this 2D one.


class SANSLOQCan2D(stresstesting.MantidStressTest):

    def runTest(self):

        ii.LOQ()
        ii.Set2D()
        ii.Detector("main-detector-bank")
        ii.MaskFile('MASK.094AA')
    # apply some small artificial shift
        ii.SetDetectorOffsets('REAR', -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)
        ii.Gravity(True)

        ii.AssignSample('99630.RAW')
        ii.AssignCan('99631.RAW')

        ii.WavRangeReduction(None, None, False)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
    #when comparing LOQ files you seem to need the following
        self.disableChecking.append('Axes')

        return '99630main_2D_2.2_10.0','SANSLOQCan2D.nxs'


class SANSLOQCan2DTest_V2(stresstesting.MantidStressTest):

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LOQ()
        ii2.Set2D()
        ii2.Detector("main-detector-bank")
        ii2.MaskFile('MASK.094AA')
        # apply some small artificial shift
        ii2.SetDetectorOffsets('REAR', -1.0, 1.0, 0.0, 0.0, 0.0, 0.0)
        ii2.Gravity(True)

        ii2.AssignSample('99630.RAW')  # They file seems to be named wrongly.
        ii2.AssignCan('99631.RAW')  # The file seems to be named wrongly.

        ii2.WavRangeReduction(None, None, False)

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        #when comparing LOQ files you seem to need the following
        self.disableChecking.append('Axes')

        return '53615main_2D_2.2_10.0','SANSLOQCan2D.nxs'
