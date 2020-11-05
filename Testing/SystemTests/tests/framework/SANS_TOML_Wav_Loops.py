# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import *
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSTomlWavLoops_SANS2D(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()
        MaskFile('Mask_SANS2D_091_wav_loops.toml')
        Set1D()

        AssignSample('992.raw')
        TransmissionSample('988.raw', '987.raw')
        AssignCan('993.raw')
        TransmissionCan('989.raw', '987.raw')
        WavRangeReduction()

    def validate(self):
        self.disableChecking.append('Instrument')
        return "992_rear_1DPhi-45.0_45.0", "SANS_TOML_Wav_Loops_ref.nxs"
