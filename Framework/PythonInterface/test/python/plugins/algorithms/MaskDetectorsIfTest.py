# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import MaskDetectorsIf, LoadNexus
import tempfile

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class MaskDetectorsIfInputTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def testValidateStartWorkspaceIndexInputs(self):
        # StartWorkspaceIndex > Number of Histograms
        ws = LoadNexus("GenerateFlatCellWorkspaceLOQOutput.nxs")
        self.assertRaises(
            RuntimeError,
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace="TestWs",
            StartWorkspaceIndex=80000000,
            EndWorkspaceIndex=5,
        )

    def testValidateEndWorkspaceIndexInputs(self):
        # EndWorkspaceIndex > Number of Histograms
        ws = LoadNexus("GenerateFlatCellWorkspaceLOQOutput.nxs")
        self.assertRaises(
            RuntimeError,
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace="TestWs",
            StartWorkspaceIndex=80,
            EndWorkspaceIndex=500000000,
        )

    def testValidateInputs(self):
        # EndWorkspaceIndex < StartWorkspaceIndex
        ws = LoadNexus("GenerateFlatCellWorkspaceLOQOutput.nxs")
        self.assertRaises(
            RuntimeError,
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace="TestWs",
            StartWorkspaceIndex=80,
            EndWorkspaceIndex=50,
        )


if __name__ == "__main__":
    unittest.main()
