# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, AssignSample, WavRangeReduction,
                                                         UseCompatibilityMode)


class SANS2DLimitEventsTimeTest_TOML(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile('SANS2DReductionGUI_LimitEventsTime.toml')
        AssignSample('22048')
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '22048_rear_1D_1.5_12.5', 'SANSReductionGUI_LimitEventsTime.nxs'
