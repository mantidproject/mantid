# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from ISISCommandInterface import (
    AssignCan,
    AssignSample,
    CompWavRanges,
    Gravity,
    MaskFile,
    SANS2D,
    Set1D,
    TransmissionCan,
    TransmissionSample,
)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DWaveloops(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()
        MaskFile("MASKSANS2D.091A")
        Gravity(True)
        Set1D()

        AssignSample("992.raw")
        TransmissionSample("988.raw", "987.raw")
        AssignCan("993.raw")
        TransmissionCan("989.raw", "987.raw")

        CompWavRanges([3, 5, 7, 11], False)

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        # testing one of the workspaces that is produced, best not to choose the
        # first one in produced by the loop as this is the least error prone
        return "992rear_1D_7.0_11.0", "SANS2DWaveloops.nxs"
