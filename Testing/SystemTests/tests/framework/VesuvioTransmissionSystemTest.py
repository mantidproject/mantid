# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import VesuvioTransmission
from mantid.api import mtd


class VesuvioTransmissionSystemTest(systemtesting.MantidSystemTest):
    """Tests the VesuvioTransmission algorithm"""

    def runTest(self):
        # Run the VesuvioTransmission algorithm - grouping SumOfAllRuns
        VesuvioTransmission(
            OutputWorkspace="vesuvio_transmission_sum_of_all_runs",
            Runs="58386-58396",
            EmptyRuns="57580-57603",
            Grouping="SumOfAllRuns",
            Target="Energy",
            Rebin=True,
            RebinParameters=[0.6, -0.03, 1.0e6],
            CalculateXS=True,
            InvertMonitors=False,
            SmoothIncidentSpectrum=False,
        )

        # Run the VesuvioTransmission algorithm - grouping TimeScan
        VesuvioTransmission(
            OutputWorkspace="vesuvio_transmission_time_scan",
            Runs="58386-58396",
            EmptyRuns="57580-57603",
            Grouping="TimeScan",
            Target="Energy",
            Rebin=False,
            CalculateXS=True,
            InvertMonitors=False,
            SmoothIncidentSpectrum=False,
        )

    def validateMethod(self):
        return "ValidateWorkspaceToNexus"

    def requiredFiles(self):
        return [
            "VESUVIO00057580.raw",
            "VESUVIO00057581.raw",
            "VESUVIO00057582.raw",
            "VESUVIO00057583.raw",
            "VESUVIO00057584.raw",
            "VESUVIO00057585.raw",
            "VESUVIO00057586.raw",
            "VESUVIO00057587.raw",
            "VESUVIO00057588.raw",
            "VESUVIO00057589.raw",
            "VESUVIO00057590.raw",
            "VESUVIO00057591.raw",
            "VESUVIO00057592.raw",
            "VESUVIO00057593.raw",
            "VESUVIO00057594.raw",
            "VESUVIO00057595.raw",
            "VESUVIO00057596.raw",
            "VESUVIO00057597.raw",
            "VESUVIO00057598.raw",
            "VESUVIO00057599.raw",
            "VESUVIO00057600.raw",
            "VESUVIO00057601.raw",
            "VESUVIO00057602.raw",
            "VESUVIO00057603.raw",
            "VESUVIO00058386.raw",
            "VESUVIO00058387.raw",
            "VESUVIO00058388.raw",
            "VESUVIO00058389.raw",
            "VESUVIO00058390.raw",
            "VESUVIO00058391.raw",
            "VESUVIO00058392.raw",
            "VESUVIO00058393.raw",
            "VESUVIO00058394.raw",
            "VESUVIO00058395.raw",
            "VESUVIO00058396.raw",
            "VesuvioTransmissionSumOfAllRunsExpectedOutput.nxs",
            "VesuvioTransmissionSumOfAllRunsExpectedOutputXS.nxs",
            "VesuvioTransmissionTimeScanExpectedOutput.nxs",
            "VesuvioTransmissionTimeScanExpectedOutputXS.nxs",
        ]

    def validate(self):
        self.checkInstrument = False
        self.nanEqual = True
        return (
            "vesuvio_transmission_sum_of_all_runs",
            "VesuvioTransmissionSumOfAllRunsExpectedOutput.nxs",
            "vesuvio_transmission_sum_of_all_runs_XS",
            "VesuvioTransmissionSumOfAllRunsExpectedOutputXS.nxs",
            "vesuvio_transmission_time_scan",
            "VesuvioTransmissionTimeScanExpectedOutput.nxs",
            "vesuvio_transmission_time_scan_XS",
            "VesuvioTransmissionTimeScanExpectedOutputXS.nxs",
        )

    def cleanup(self):
        mtd.clear()
