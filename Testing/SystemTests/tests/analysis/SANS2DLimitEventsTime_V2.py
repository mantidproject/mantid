# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid  # noqa
from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, AssignSample, WavRangeReduction,
                                                         UseCompatibilityMode)


class SANS2DLimitEventsTimeTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile('MaskSANS2DReductionGUI_LimitEventsTime.txt')
        AssignSample('22048')
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '22048rear_1D_1.5_12.5', 'SANSReductionGUI_LimitEventsTime.nxs'
