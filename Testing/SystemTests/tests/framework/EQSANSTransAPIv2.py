# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from reduction_workflow.instruments.sans.sns_command_interface import (
    AppendDataFile,
    CombineTransmissionFits,
    EQSANS,
    Reduce1D,
    SetBeamCenter,
    SetTOFTailsCutoff,
    SetTransmission,
    SolidAngle,
    TotalChargeNormalization,
    UseConfig,
    UseConfigMask,
    UseConfigTOFTailsCutoff,
)
from reduction_workflow.instruments.sans.hfir_command_interface import (
    DarkCurrent,
    DirectBeamTransmission,
    NoIQxQy,
    ThetaDependentTransmission,
)
from mantid.api import FileFinder
from mantid.kernel import ConfigService
from mantid.simpleapi import Scale

import os


def do_cleanup():
    Files = ["EQSANS_4061_event_reduction.log", "EQSANS_1466_event_reduction.log"]
    for filename in Files:
        absfile = FileFinder.getFullPath(filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True


class EQSANSTransmission(systemtesting.MantidSystemTest):
    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(False)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        CombineTransmissionFits(True)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamTransmission("EQSANS_1466_event.nxs", "EQSANS_4061_event.nxs", beam_radius=3)
        ThetaDependentTransmission(True)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSTrans.nxs"


class EQSANSTransmissionEvent(EQSANSTransmission):
    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(True)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamTransmission("EQSANS_1466_event.nxs", "EQSANS_4061_event.nxs", beam_radius=3)
        ThetaDependentTransmission(True)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSTransEvent.nxs"


class EQSANSTransmissionDC(systemtesting.MantidSystemTest):
    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that EQSANSTofStructure returns the correct workspace
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(False)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        DarkCurrent("EQSANS_4061_event.nxs")
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamTransmission("EQSANS_1466_event.nxs", "EQSANS_1466_event.nxs", beam_radius=3)
        ThetaDependentTransmission(True)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSTransmissionDC.nxs"


class EQSANSTransmissionCompatibility(EQSANSTransmission):
    """
    Analysis Tests for EQSANS
    Check that the transmission correction can be applied if the
    sample run and transmission runs don't have the same binning
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(True)
        AppendDataFile("EQSANS_1466_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        SetBeamCenter(96.29, 126.15)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamTransmission("EQSANS_4061_event.nxs", "EQSANS_4061_event.nxs", beam_radius=3)
        ThetaDependentTransmission(True)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        # Be more tolerant with the output, mainly because of the errors.
        # The following tolerance check the errors up to the third digit.
        self.tolerance = 0.1
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSTransmissionCompatibility.nxs"


class EQSANSTransmissionFS(systemtesting.MantidSystemTest):
    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that EQSANSTofStructure returns the correct workspace
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(False)
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_4061_event.nxs")
        SolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(0.5, 0.1)
        ThetaDependentTransmission(False)
        Reduce1D()

    def validate(self):
        self.tolerance = 0.000001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_4061_event_frame1_Iq", "EQSANSTransmissionFS.nxs"


class EQSANSDirectTransFS(systemtesting.MantidSystemTest):
    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that EQSANSTofStructure returns the correct workspace
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS(False)
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_4061_event.nxs")
        UseConfig(False)
        SetTOFTailsCutoff(500, 500)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        DirectBeamTransmission("EQSANS_4061_event.nxs", "EQSANS_4061_event.nxs", beam_radius=3)
        ThetaDependentTransmission(False)
        NoIQxQy()
        Reduce1D()
        Scale(InputWorkspace="EQSANS_4061_event_frame1_Iq", Factor=2.0, Operation="Multiply", OutputWorkspace="EQSANS_4061_event_frame1_Iq")

    def validate(self):
        # Relax the tolerance since the reference data is not for that exact
        # scenario but for one that's very close to it.
        self.tolerance = 0.00001
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_4061_event_frame1_Iq", "EQSANSDirectTransFS.nxs"
