# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import os
import re
import numpy as np
import systemtesting
from mantid.simpleapi import GenerateFlatCellWorkspaceLOQ, CompareWorkspaces, LoadNexus
from mantid.api import mtd


class GenerateFlatCellWorkspaceLOQTest(systemtesting.MantidSystemTest):
    """Tests the GenerateFlatCellWorkspaceLOQ algorithm"""

    def runTest(self):
        # Load the input data and save into a workspace
        LoadNexus(Filename="LOQ00114168.nxs", OutputWorkspace="input")

        # Apply the algorithm to the input data
        self.filename = "flatcell_rkf_output.txt"
        GenerateFlatCellWorkspaceLOQ(InputWorkspace="input", OutputWorkspace="output", OutputRKHFilePath=self.filename)

        # Load the output data and save into a workspace
        LoadNexus(Filename="GenerateFlatCellWorkspaceLOQExpectedOutput.nxs", OutputWorkspace="expected_output")
        LoadNexus(Filename="GenerateFlatCellWorkspaceLOQExpectedMask.nxs", OutputWorkspace="expected_mask")

        # Compare the mask workspaces
        result, _ = CompareWorkspaces("output_MASK", "expected_mask")
        self.assertTrue(result)

        # Compare the RKH files
        self.assertTrue(self._match_rkh_files("GenerateFlatCellWorkspaceLOQExpectedRKH.nxs", self.filename))

    def _match_rkh_files(self, path_to_reference, path_to_output):
        with open(path_to_reference, "r") as r, open(path_to_output, "r") as o:
            reference_file_lines = r.readlines()[2:]
            output_file_lines = o.readlines()[2:]

        pattern = r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?"

        for ref, out in zip(reference_file_lines, output_file_lines):
            ref_arr = np.array([float(x) for x in re.findall(pattern, ref)])
            out_arr = np.array([float(x) for x in re.findall(pattern, out)])
            if not np.allclose(ref_arr, out_arr):
                return False

        return True

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def requiredFiles(self):
        return [
            "LOQ00114168.nxs",
            "GenerateFlatCellWorkspaceLOQExpectedOutput.nxs",
            "GenerateFlatCellWorkspaceLOQExpectedMask.nxs",
            "GenerateFlatCellWorkspaceLOQExpectedRKH.nxs",
        ]

    def validate(self):
        self.tolerance = 1e-2
        return ("output", "expected_output")

    def cleanup(self):
        if os.path.exists(self.filename):
            os.remove(self.filename)
        mtd.clear()
