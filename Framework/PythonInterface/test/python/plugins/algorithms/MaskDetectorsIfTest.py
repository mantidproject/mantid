# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import MaskDetectorsIf, CreateWorkspace


class MaskDetectorsIfInputTest(unittest.TestCase):
    def testValidateStartWorkspaceIndexInputs(self):
        # StartWorkspaceIndex > Number of Histograms
        ws = CreateWorkspace(DataX=[0, 1], DataY=[1] * 10, NSpec=10)
        self.assertRaisesRegex(
            ValueError,
            r"StartWorkspaceIndex should be greater than or equal to 0 and less than 9\. Value provided is invalid\.",
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace=ws,
            StartWorkspaceIndex=80,
        )

    def testValidateEndWorkspaceIndexInputs(self):
        # EndWorkspaceIndex > Number of Histograms
        ws = CreateWorkspace(DataX=[0, 1], DataY=[1] * 10, NSpec=10)
        self.assertRaisesRegex(
            ValueError,
            r"EndWorkspaceIndex should be greater than 0 and less than 10\. Value provided is invalid\.",
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace=ws,
            StartWorkspaceIndex=8,
            EndWorkspaceIndex=50,
        )

    def testValidateInputs(self):
        # EndWorkspaceIndex < StartWorkspaceIndex
        ws = CreateWorkspace(DataX=[0, 1], DataY=[1] * 10, NSpec=10)
        self.assertRaisesRegex(
            RuntimeError,
            r"EndWorkspaceIndex: EndWorkspaceIndex should be more than StartWorkspaceIndex\. Specify a value greater than 8\.",
            MaskDetectorsIf,
            InputWorkspace=ws,
            OutputWorkspace=ws,
            StartWorkspaceIndex=8,
            EndWorkspaceIndex=5,
        )


if __name__ == "__main__":
    unittest.main()
