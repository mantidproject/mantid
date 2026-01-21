# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
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
        LoadNexus(Filename="LOQ00113953.nxs", OutputWorkspace="FlatCellEventInput")

        # Apply the algorithm to the input data
        GenerateFlatCellWorkspaceLOQ(InputWorkspace="FlatCellEventInput", OutputWorkspace="FlatCellActualOutput")

        # Load the output data and save into a workspace
        LoadNexus(Filename="FlatCellOutput.nxs", OutputWorkspace="FlatCellExpectedOutput")
        LoadNexus(Filename="FlatCellMasked.nxs", OutputWorkspace="FlatCellMaskedOutput")

        # Compare the workspaces
        result, _ = CompareWorkspaces("FlatCellActualOutput", "FlatCellExpectedOutput")
        self.assertTrue(result)
        result, _ = CompareWorkspaces("maskedWS", "FlatCellMaskedOutput")
        self.assertTrue(result)

    def requiredFiles(self):
        return ["LOQ00113953.nxs", "FlatCellMasked.nxs", "FlatCellOutput.nxs"]

    def validate(self):
        return True
