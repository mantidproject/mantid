# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import FlatCell, LoadAscii


class FlatCellTest(systemtesting.MantidSystemTest):
    """Tests the FlatCell algorithm"""

    def runTest(self):
        # Load the input data and save into a workspace
        LoadAscii(Filename="flatcell_input.csv", OutputWorkspace="FlatCellInput")

        # Apply the algorithm to the input data
        FlatCell(InputWorkspace="FlatCellInput", OutputWorkspace="FlatCellInput")

        # Load the output data and save into a workspace
        LoadAscii(Filename="flatcell_output.csv", OutputWorkspace="FlatCellOutput")

        # # Compare the results
        # (result, _messages) = CompareWorkspaces("FlatCellInput", "FlatCellOutput", Tolerance=1e-2)
        # self.assertEqual(result, True)

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def requiredFiles(self):
        return ["flatcell_input.csv", "flatcell_output.csv"]

    def validate(self):
        self.tolerance = 1e-2
        return ("FlatCellInput", "FlatCellOutput")
