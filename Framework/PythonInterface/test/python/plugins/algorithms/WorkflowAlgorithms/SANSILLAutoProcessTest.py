# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SANSILLAutoProcess, config, mtd
from mantid.api import WorkspaceGroup, MatrixWorkspace


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

    def test_noSampleRun(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess)

    def test_noOutputWs(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462")

    def test_wrongAbsorberDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          AbsorberRuns="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongBeamDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          BeamRuns="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongContainerDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          ContainerRuns="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongMaskDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          MaskFiles="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongReferenceDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          ReferenceFiles="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongSensivityDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          SensitivityMaps="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongFluxDim(self):
        self.assertRaises(RuntimeError, SANSILLAutoProcess,
                          SampleRuns="010462",
                          FluxRuns="010462,010462",
                          OutputWorkspace="ws")

    def test_wrongParametersDim(self):
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

    def test_wrongTransmissionDim(self):
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

    def test_minimalProcessing(self):
        ws = SANSILLAutoProcess(SampleRuns="010462")

        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), 1)
        item = ws.getItem(0)
        self.assertTrue(isinstance(item, MatrixWorkspace))
        self.assertEqual(item.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEquals(item.getNumberHistograms(), 1)


if __name__ == "__main__":
    unittest.main()
