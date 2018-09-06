#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import mantid # noqa
import stresstesting
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2


class SANS2DLimitEventsTime(stresstesting.MantidStressTest):

    def runTest(self):
        ii.SANS2D()
        ii.MaskFile('MaskSANS2DReductionGUI_LimitEventsTime.txt')
        ii.AssignSample('22048')
        ii.WavRangeReduction()

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '22048rear_1D_1.5_12.5','SANSReductionGUI_LimitEventsTime.nxs'


class SANS2DLimitEventsTimeTest_V2(stresstesting.MantidStressTest):
    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.SANS2D()
        ii2.MaskFile('MaskSANS2DReductionGUI_LimitEventsTime.txt')
        ii2.AssignSample('22048')
        ii2.WavRangeReduction()

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '22048rear_1D_1.5_12.5', 'SANSReductionGUI_LimitEventsTime.nxs'
