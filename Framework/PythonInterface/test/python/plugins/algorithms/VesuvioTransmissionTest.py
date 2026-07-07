# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import VesuvioTransmission


class VesuvioTransmissionTest(unittest.TestCase):
    def test_run_input_validation(self):
        # Runs contain just one value
        self.assertRaisesRegex(
            RuntimeError,
            "VesuvioTransmission-v1: Some invalid Properties found: \n"
            " Runs: For Grouping='TimeScan', Runs must be a simple integer range, e.g. '12345-12355'.",
            VesuvioTransmission,
            OutputWorkspace="transmission_ws",
            Runs="58386",
            EmptyRuns="57580-57603",
            Grouping="TimeScan",
        )

        # Lower bound of Runs is greater than the upper bound
        self.assertRaisesRegex(
            RuntimeError,
            "VesuvioTransmission-v1: Some invalid Properties found: \n"
            " Runs: For Grouping='TimeScan', the upper run number must be > lower run number.",
            VesuvioTransmission,
            OutputWorkspace="transmission_ws",
            Runs="58386-58380",
            EmptyRuns="57580-57603",
            Grouping="TimeScan",
        )

        # Runs contain non integer values
        self.assertRaisesRegex(
            RuntimeError,
            "VesuvioTransmission-v1: Some invalid Properties found: \n"
            " Runs: For Grouping='TimeScan', Runs must be a simple integer range, e.g. '12345-12355'.",
            VesuvioTransmission,
            OutputWorkspace="transmission_ws",
            Runs="a-b",
            EmptyRuns="57580-57603",
            Grouping="TimeScan",
        )


if __name__ == "__main__":
    unittest.main()
