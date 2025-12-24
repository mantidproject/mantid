# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import FlatCell, LoadAscii, LoadNexus


class FlatCellTest(systemtesting.MantidSystemTest):
    """Tests the FlatCell algorithm"""

    def runTest(self):
        # Load the input data and save into a workspace
        LoadNexus(Filename="LOQ00113953.nxs", OutputWorkspace="FlatCellEventInput")

        # Apply the algorithm to the input data
        FlatCell(InputWorkspace="FlatCellEventInput", OutputWorkspace="FlatCellOutput")

        # Load the output data and save into a workspace
        LoadAscii(Filename="flatcell_output.csv", OutputWorkspace="FlatCellExpectedOutput", Unit="Dimensionless")

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def requiredFiles(self):
        return ["LOQ00113953.nxs", "flatcell_output.csv"]

    def validate(self):
        self.tolerance = 0.07
        return ("FlatCellOutput", "FlatCellExpectedOutput")
