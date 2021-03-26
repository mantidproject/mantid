# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
import systemtesting
import mantid  # noqa
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import (SANS2D, MaskFile, SetDetectorOffsets, Gravity, Set1D,
                                                         UseCompatibilityMode, AssignSample, WavRangeReduction)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DFrontNoGravTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile('MASKSANS2D_094i_RKH.txt')
        SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()

        AssignSample('2500.nxs')

        WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500_front_1D_4.6_12.85', 'SANS2DFrontNoGrav.nxs'


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DWithExtraLengthGravityTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        MaskFile('MASKSANS2D_094i_RKH.txt')
        SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)

        extraLength = 1
        Gravity(True, extraLength)
        Set1D()
        AssignSample('2500.nxs')
        WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')
        return '2500_front_1D_4.6_12.85', 'SANS2DWithExtraLengthGravity.nxs'
