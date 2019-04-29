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
from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, Gravity, Set1D, AssignSample,
                                                         TransmissionSample, AssignCan, TransmissionCan,
                                                         CompWavRanges, UseCompatibilityMode)


class SANS2DWaveloopsTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile('MASKSANS2D.091A')
        Gravity(True)
        Set1D()

        AssignSample('992.raw')
        TransmissionSample('988.raw', '987.raw')
        AssignCan('993.raw')
        TransmissionCan('989.raw', '987.raw')
        CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        # testing one of the workspaces that is produced, best not to choose the
        # first one in produced by the loop as this is the least error prone
        return '992rear_1D_7.0_11.0', 'SANS2DWaveloops.nxs'
