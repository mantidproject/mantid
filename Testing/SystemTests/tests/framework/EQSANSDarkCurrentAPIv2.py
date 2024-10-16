# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid import FileFinder
from mantid.kernel import ConfigService
from mantid.simpleapi import Scale
from reduction_workflow.instruments.sans.sns_command_interface import (
    AppendDataFile,
    EQSANS,
    PerformFlightPathCorrection,
    Reduce1D,
    SetBeamCenter,
    SetTOFTailsCutoff,
    SolidAngle,
    TotalChargeNormalization,
    UseConfig,
    UseConfigMask,
    UseConfigTOFTailsCutoff,
)
from reduction_workflow.instruments.sans.hfir_command_interface import DarkCurrent, SetTransmission

import os


class EQSANSDarkCurrent(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the I(Q) output of is correct
    """

    def cleanup(self):
        absfile = FileFinder.getFullPath("EQSANS_1466_event_reduction.log")
        if os.path.exists(absfile):
            os.remove(absfile)
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        # The new version of dark current subtraction only works on histograms
        EQSANS(False)
        SolidAngle()
        SetBeamCenter(96.29, 126.15)
        PerformFlightPathCorrection(False)
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        SetTOFTailsCutoff(low_cut=0.00, high_cut=0.00)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0, 0.0, False)
        DarkCurrent("EQSANS_4061_event.nxs")
        AppendDataFile("EQSANS_1466_event.nxs")
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        self.tolerance = 1.0
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return "EQSANS_1466_event_Iq", "EQSANSDarkCurrent.nxs"
