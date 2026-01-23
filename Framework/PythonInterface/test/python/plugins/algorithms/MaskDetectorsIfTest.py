# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import MaskDetectorsIf, CreateWorkspace
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
        ws = CreateWorkspace(DataX=[0, 1], DataY=[1] * 10, NSpec=10)
        self.assertRaises(
            RuntimeError,
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace=ws,
            StartWorkspaceIndex=80,
            EndWorkspaceIndex=5,
        )

    def testValidateInputs(self):
        # EndWorkspaceIndex < StartWorkspaceIndex
        ws = CreateWorkspace(DataX=[0, 1], DataY=[1] * 10, NSpec=10)
        self.assertRaises(
            RuntimeError,
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace=ws,
            StartWorkspaceIndex=8,
            EndWorkspaceIndex=5,
        )


if __name__ == "__main__":
    unittest.main()
