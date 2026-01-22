# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import GenerateFlatCellWorkspaceLOQ, CompareWorkspaces, LoadNexus


class GenerateFlatCellWorkspaceLOQTest(systemtesting.MantidSystemTest):
    """Tests the GenerateFlatCellWorkspaceLOQ algorithm"""

    def runTest(self):
        # Load the input data and save into a workspace
        LoadNexus(Filename="LOQ00113953.nxs", OutputWorkspace="input")

        # Apply the algorithm to the input data
        GenerateFlatCellWorkspaceLOQ(InputWorkspace="input", OutputWorkspace="output")

        # Load the output data and save into a workspace
        LoadNexus(Filename="GenerateFlatCellWorkspaceLOQOutput.nxs", OutputWorkspace="expected_output")
        LoadNexus(Filename="GenerateFlatCellWorkspaceLOQMASK.nxs", OutputWorkspace="expected_mask")

        # Compare the mask workspaces
        result, _ = CompareWorkspaces("output_MASK", "expected_mask")
        self.assertTrue(result)

    def requiredFiles(self):
        return ["LOQ00113953.nxs", "GenerateFlatCellWorkspaceLOQOutput.nxs", "GenerateFlatCellWorkspaceLOQMASK.nxs"]

    def validate(self):
        self.tolerance = 1e-2
        return ["output", "expected_output"]
