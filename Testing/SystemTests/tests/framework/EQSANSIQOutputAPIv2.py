# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import systemtesting
import math
import os
from reduction_workflow.instruments.sans.sns_command_interface import (
    AppendDataFile,
    BeamMonitorNormalization,
    EQSANS,
    NoSolidAngle,
    Reduce1D,
    Resolution,
    SetBeamCenter,
    TotalChargeNormalization,
    UseConfig,
    UseConfigMask,
    UseConfigTOFTailsCutoff,
)
from reduction_workflow.instruments.sans.hfir_command_interface import Background, SetTransmission

from mantid.api import mtd, FileFinder
from mantid.kernel import ConfigService
from mantid.simpleapi import Scale

from functools import reduce


def do_cleanup():
    Files = ["EQSANS_4061_event_reduction.log", "EQSANS_1466_event_reduction.log"]
    for filename in Files:
        absfile = FileFinder.getFullPath(filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    return True


class EQSANSIQOutput(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the I(Q) output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that EQSANSTofStructure returns the correct workspace
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        NoSolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        Reduce1D()
        # Scale up to match correct scaling.
        Scale(InputWorkspace="EQSANS_1466_event_Iq", Factor=2777.81, Operation="Multiply", OutputWorkspace="EQSANS_1466_event_Iq")

    def validate(self):
        self.tolerance = 0.2
        mtd["EQSANS_1466_event_Iq"].dataY(0)[0] = 269.687
        mtd["EQSANS_1466_event_Iq"].dataE(0)[0] = 16.4977
        mtd["EQSANS_1466_event_Iq"].dataE(0)[1] = 6.78
        mtd["EQSANS_1466_event_Iq"].dataY(0)[2] = 11.3157
        mtd["EQSANS_1466_event_Iq"].dataE(0)[2] = 1.23419
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSIQOutput.nxs"


class EQSANSBeamMonitor(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the I(Q) output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        NoSolidAngle()
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        BeamMonitorNormalization("SANSBeamFluxCorrectionMonitor.nxs")
        Reduce1D()

    def validate(self):
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        return "EQSANS_1466_event_Iq", "EQSANSBeamMonitor.nxs"


class EQSANSDQPositiveOutput(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that the Q resolution calculation returns positive values
        even when background is larger than signal and I(q) is negative.
        (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0, 0.0, False)
        Background("EQSANS_4061_event.nxs")
        Resolution()
        Reduce1D()

    def validate(self):
        dq = mtd["EQSANS_1466_event_Iq"].dataDx(0)
        for x in dq:
            if x < 0:
                return False
        return True


class EQSANSDQOutput(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that the Q resolution calculation returns positive values
        even when background is larger than signal and I(q) is negative.
        (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_1466_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0, 0.0, False)
        Background("EQSANS_4061_event.nxs")
        Resolution(10)
        Reduce1D()

    def validate(self):
        """
        Reference values were generate using the event-by-event method
        and are slightly different than the ones generated using
        the histogram method.
        The event-by-event method processes each event one-by-one,
        computes dQ for each of them, and averages those dQ for each
        Q bin of the I(Q) distribution.
        """
        dq_ref = [
            0.00252508,
            0.00204973,
            0.00204704,
            0.00225957,
            0.00220887,
            0.00242261,
            0.00238752,
            0.00249037,
            0.0026182,
            0.00265878,
            0.00277257,
            0.00276184,
            0.00290244,
            0.00304704,
            0.00308463,
            0.00321755,
            0.00320598,
            0.00342758,
            0.00348762,
            0.00372852,
            0.00406129,
            0.00397002,
            0.00393542,
            0.00412209,
            0.00440805,
            0.00450745,
            0.00464092,
            0.00478412,
            0.00507242,
            0.00521279,
            0.00532642,
            0.00555341,
            0.00555728,
            0.0060209,
            0.00591925,
            0.00636599,
            0.00653846,
            0.00678156,
            0.0072413,
            0.0072045,
            0.00752474,
            0.00777514,
            0.00813613,
            0.00821731,
            0.0086195,
            0.00885732,
            0.00896786,
            0.00937582,
            0.00941747,
            0.00977716,
            0.0102626,
            0.010588,
            0.0107926,
            0.0109648,
            0.0113631,
            0.0116634,
            0.0122167,
            0.0123624,
            0.0126822,
            0.013181,
            0.0131975,
            0.0132709,
            0.0137282,
            0.0144791,
            0.0141742,
            0.0146757,
            0.0148619,
            0.0150053,
            0.0153739,
            0.0160979,
            0.016513,
            0.0159204,
            0.016845,
            0.0172756,
            0.0176195,
            0.0177233,
            0.0183046,
            0.019295,
            0.0188272,
            0.0188754,
            0.0190667,
            0.0194098,
            0.0198288,
            0.0208396,
            0.0209386,
            0.0215817,
            0.0219187,
            0.0219683,
            0.0230127,
            0.0231818,
            0.0233988,
            0.0233534,
            0.0238844,
            0,
            0,
            0,
            0.0248531,
            0,
            0,
            0,
            0,
        ]

        dq = mtd["EQSANS_1466_event_Iq"].readDx(0)
        diff = [math.fabs(dq_ref[i] - dq[i]) < 0.0001 for i in range(7, 100)]
        output = reduce(lambda x, y: x and y, diff)
        if not output:
            for i, dqi in enumerate(dq):
                print(i, dqi, dq_ref[i], math.fabs(dq_ref[i] - dqi) < 0.0001)
        return output


class EQSANSDQOutput_FS(systemtesting.MantidSystemTest):
    """
    Analysis Tests for EQSANS
    Testing that the Q resolution output of is correct
    """

    def cleanup(self):
        do_cleanup()
        return True

    def runTest(self):
        """
        Check that the Q resolution calculation returns positive values
        even when background is larger than signal and I(q) is negative.
        (Non-physical value that's an experimental edge case)
        """
        configI = ConfigService.Instance()
        configI["facilityName"] = "SNS"
        EQSANS()
        SetBeamCenter(96.29, 126.15)
        AppendDataFile("EQSANS_4061_event.nxs")
        UseConfig(False)
        UseConfigTOFTailsCutoff(False)
        UseConfigMask(False)
        TotalChargeNormalization(normalize_to_beam=False)
        SetTransmission(1.0, 0.0, False)
        Resolution(12)
        Reduce1D()

    def validate(self):
        """
        Reference values were generate using the event-by-event method
        and are slightly different than the ones generated using
        the histogram method.
        The event-by-event method processes each event one-by-one,
        computes dQ for each of them, and averages those dQ for each
        Q bin of the I(Q) distribution.
        """
        dq_ref = [
            0.00343638,
            0.00292027,
            0.00284172,
            0.00356216,
            0.00410584,
            0.00350177,
            0.00352453,
            0.0035258,
            0.0033252,
            0.00333963,
            0.00342883,
            0.00352433,
            0.00363203,
            0.00374885,
            0.00386253,
            0.00399503,
            0.00412582,
            0.00425981,
            0.00440508,
            0.00456234,
            0.00469688,
            0.00484763,
            0.00499924,
            0.00515091,
            0.00531474,
            0.0054862,
            0.00567197,
            0.0058903,
            0.00612266,
            0.00636194,
            0.00659338,
            0.00683826,
            0.00713157,
            0.00741236,
            0.00769484,
            0.00800949,
            0.00832757,
            0.00863584,
            0.00898523,
            0.009315,
            0.00964807,
            0.00999386,
            0.0103381,
            0.010696,
            0.0110678,
            0.0114199,
            0.0117903,
            0.012165,
            0.0125308,
            0.0128986,
            0.0133072,
            0.013671,
            0.0140613,
            0.0144513,
            0.0148358,
            0.0152126,
            0.0155984,
            0.0160445,
            0.0164337,
            0.0167855,
            0.0171935,
            0.0175954,
            0.017969,
            0.0183745,
            0.0187386,
            0.0191184,
            0.0194819,
            0.0198701,
            0.0202481,
            0.0205821,
            0.0209512,
            0.0213435,
            0.0216502,
            0.0221041,
            0.0225051,
            0.0228566,
            0.0232738,
            0.0237153,
            0.0240277,
            0.0245176,
            0.0249624,
            0.0253241,
            0.0257324,
            0.0262129,
            0.02659,
            0.0269067,
            0.0274065,
            0.0279431,
            0.028349,
            0.0285522,
            0.0290219,
            0.0297083,
            0.030095,
            0.0304317,
            0.0307289,
            0.0310205,
            0.0313329,
            0,
            0,
            0,
            0,
        ]

        dq = mtd["EQSANS_4061_event_frame1_Iq"].readDx(0)
        diff = [math.fabs(dq_ref[i] - dq[i]) < 0.0001 for i in range(7, 100)]
        output = reduce(lambda x, y: x and y, diff)

        if not output:
            for i, dqi in enumerate(dq):
                print(i, dqi, dq_ref[i], math.fabs(dq_ref[i] - dqi) < 0.0001)
        return output
