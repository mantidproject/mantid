#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class SANS2DFrontNoGrav(stresstesting.MantidStressTest):

    def runTest(self):

        ii.SANS2D()
        ii.MaskFile('MASKSANS2D_094i_RKH.txt')
        ii.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        ii.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        ii.Gravity(False)
        ii.Set1D()

        ii.AssignSample('2500.nxs')

        ii.WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500front_1D_4.6_12.85','SANS2DFrontNoGrav.nxs'


class SANS2DWithExtraLengthGravity(stresstesting.MantidStressTest):
    def runTest(self):
        ii.SANS2D()
        ii.MaskFile('MASKSANS2D_094i_RKH.txt')
        ii.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        ii.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)

        extraLength = 1
        ii.Gravity(True, extraLength)
        ii.Set1D()
        ii.AssignSample('2500.nxs')
        ii.WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500front_1D_4.6_12.85','SANS2DWithExtraLengthGravity.nxs'


class SANS2DFrontNoGravTest_V2(stresstesting.MantidStressTest):
    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile('MASKSANS2D_094i_RKH.txt')
        ii2.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        ii2.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        ii2.Gravity(False)
        ii2.Set1D()

        ii2.AssignSample('2500.nxs')

        ii2.WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500front_1D_4.6_12.85', 'SANS2DFrontNoGrav.nxs'


class SANS2DWithExtraLengthGravityTest_V2(stresstesting.MantidStressTest):
    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile('MASKSANS2D_094i_RKH.txt')
        ii2.SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        ii2.SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)

        extraLength = 1
        ii2.Gravity(True, extraLength)
        ii2.Set1D()
        ii2.AssignSample('2500.nxs')
        ii2.WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500front_1D_4.6_12.85','SANS2DWithExtraLengthGravity.nxs'
