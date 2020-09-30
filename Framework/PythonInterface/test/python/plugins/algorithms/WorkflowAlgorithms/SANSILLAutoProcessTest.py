# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SANSILLAutoProcess, config, mtd


class SANSILLAutoProcessTest(unittest.TestCase):

    def setUp(self):
        config.appendDataSearchSubDir("ILL/D11/")
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"

    def tearDown(self):
        config["default.facility"] = self._facility
        config["default.instrument"] = self._instrument
        mtd.clear()

    def test_validateInut(self):
        # no sample runs
        self.assertRaises(RuntimeError, SANSILLAutoProcess)

        # no output workspace
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462")

        # minimum
        SANSILLAutoProcess(SampleRuns="010462", OutputWorkspace="ws")

        # sample dim
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          AbsorberRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          BeamRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          ContainerRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          MaskFiles="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          ReferenceFiles="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          SensitivityMaps="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          FluxRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          MaxQxy="1,1",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          DeltaQ="1,1",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          BeamRadius="1,1",
                          OutputWorkspace="ws")

        # single transmission
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462,010462",
                          SampleTransmissionRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462,010462",
                          ContainerTransmissionRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462,010462",
                          TransmissionBeamRuns="010462,010462",
                          OutputWorkspace="ws")
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462,010462",
                          TransmissionAbsorberRuns="010462,010462",
                          OutputWorkspace="ws")


if __name__ == "__main__":
    unittest.main()
