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
    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]
        cls._data_search_dirs = config["datasearch.directories"]
        config.appendDataSearchSubDir("ILL/D11/")
        config.appendDataSearchSubDir("ILL/D16/")
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"

    def tearDown(self):
        mtd.clear()

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._data_search_dirs

    def test_noSampleRun(self):
        self.assertRaises(TypeError, SANSILLAutoProcess)

    def test_noOutputWs(self):
        self.assertRaises(TypeError, SANSILLAutoProcess, SampleRuns="010462")

    def test_wrongAbsorberDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Absorber runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            AbsorberRuns="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongBeamDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Beam runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            BeamRuns="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongContainerDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Container runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            ContainerRuns="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongMaskDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Mask runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            MaskFiles="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongReferenceDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Reference runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            ReferenceFiles="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongSensitivityDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Sensitivity runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            SensitivityMaps="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongFluxDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of Flux runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            FluxRuns="010462,010462",
            OutputWorkspace="ws",
        )

    def test_wrongParametersDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of MaxQxy values: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            MaxQxy="1,1",
            OutputWorkspace="ws",
        )
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of DeltaQ values: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            DeltaQ="1,1",
            OutputWorkspace="ws",
        )
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of BeamRadius values: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            BeamRadius="1,1",
            OutputWorkspace="ws",
        )

    def test_wrongTransmissionDim(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of SampleTransmission runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            SampleTransmissionRuns="010462,010462",
            OutputWorkspace="ws",
        )
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of ContainerTransmission runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            ContainerTransmissionRuns="010462,010462",
            OutputWorkspace="ws",
        )
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of TransmissionBeam runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            TransmissionBeamRuns="010462,010462",
            OutputWorkspace="ws",
        )
        self.assertRaisesRegex(
            RuntimeError,
            "Wrong number of TransmissionAbsorber runs: 2. Provide one or as many as sample runs: 1.",
            SANSILLAutoProcess,
            SampleRuns="010462",
            TransmissionAbsorberRuns="010462,010462",
            OutputWorkspace="ws",
        )

    def test_flux(self):
        ws = SANSILLAutoProcess(SampleRuns="010462", FluxRuns="010462")

        self.assertTrue(isinstance(ws, WorkspaceGroup))

    def test_one_q_binning_params(self):
        ws = SANSILLAutoProcess(SampleRuns="3674, 3677, 3680", OutputBinning="0.01")
        self.assertTrue(ws)

    def test_minimalProcessing(self):
        ws = SANSILLAutoProcess(SampleRuns="010462")

        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), 1)
        item = ws.getItem(0)
        self.assertTrue(isinstance(item, MatrixWorkspace))
        self.assertEqual(item.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(item.getNumberHistograms(), 1)


if __name__ == "__main__":
    unittest.main()
