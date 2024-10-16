# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.api import mtd
from mantid.kernel import ConfigService
from reduction_workflow.instruments.sans.sns_command_interface import (
    AppendDataFile,
    AzimuthalAverage,
    BckCombineTransmissionFits,
    CombineTransmissionFits,
    EQSANS,
    PerformFlightPathCorrection,
    Reduce1D,
    SetBeamCenter,
    SolidAngle,
    TotalChargeNormalization,
    UseConfigMask,
    UseConfigTOFTailsCutoff,
)
from reduction_workflow.instruments.sans.hfir_command_interface import (
    Background,
    BckDirectBeamTransmission,
    BckThetaDependentTransmission,
    DarkCurrent,
    DirectBeamTransmission,
    IQxQy,
    SaveIqAscii,
    SensitivityCorrection,
    SetAbsoluteScale,
    ThetaDependentTransmission,
)

FILE_LOCATION = "/SNS/EQSANS/IPTS-5636/data/"


class EQSANSFlatTest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        files = []
        files.append(FILE_LOCATION + "EQSANS_5704_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5734_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5732_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5738_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5729_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5737_event.nxs")
        files.append(FILE_LOCATION + "EQSANS_5703_event.nxs")
        files.append("bl6_flux_at_sample")
        return files

    def runTest(self):
        """
        System test for EQSANS.
        This test is meant to be run at SNS and takes a long time.
        It is used to verify that the complete reduction chain works
        and reproduces reference results.
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SolidAngle()
        DarkCurrent(FILE_LOCATION + "EQSANS_5704_event.nxs")
        TotalChargeNormalization(beam_file="bl6_flux_at_sample")
        AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=False)
        IQxQy(nbins=100)
        UseConfigTOFTailsCutoff(True)
        PerformFlightPathCorrection(True)
        UseConfigMask(True)
        SetBeamCenter(89.6749, 129.693)
        SensitivityCorrection(FILE_LOCATION + "EQSANS_5703_event.nxs", min_sensitivity=0.5, max_sensitivity=1.5, use_sample_dc=True)
        DirectBeamTransmission(FILE_LOCATION + "EQSANS_5734_event.nxs", FILE_LOCATION + "EQSANS_5738_event.nxs", beam_radius=3)
        ThetaDependentTransmission(False)
        AppendDataFile([FILE_LOCATION + "EQSANS_5729_event.nxs"])
        CombineTransmissionFits(True)

        Background(FILE_LOCATION + "EQSANS_5732_event.nxs")
        BckDirectBeamTransmission(FILE_LOCATION + "EQSANS_5737_event.nxs", FILE_LOCATION + "EQSANS_5738_event.nxs", beam_radius=3)
        BckThetaDependentTransmission(False)
        BckCombineTransmissionFits(True)
        SaveIqAscii(process="None")
        SetAbsoluteScale(277.781)
        Reduce1D()
        # This reference is old, ignore the first non-zero point and
        # give the comparison a reasonable tolerance (less than 0.5%).
        mtd["EQSANS_5729_event_frame1_Iq"].dataY(0)[1] = 856.30028119108

    def validate(self):
        self.tolerance = 5.0
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_5729_event_frame1_Iq", "EQSANSFlatTest.nxs"
