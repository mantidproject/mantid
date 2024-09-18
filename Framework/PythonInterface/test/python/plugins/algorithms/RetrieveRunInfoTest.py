# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.kernel import *
from mantid.api import *
from mantid import config
from mantid.simpleapi import *
from testhelpers import run_algorithm


class RetrieveRunInfoTest(unittest.TestCase):
    class_has_been_set_up = False

    def setUp(self):
        # Set up every time.
        config["default.instrument"] = "IRIS"
        config["default.facility"] = "ISIS"

        # Only set up once.
        if not self.class_has_been_set_up:
            class_has_been_set_up = True

            # Create a workspace that is not a table workspace.
            pre_existing_matrix_workspace_alg = run_algorithm("CreateWorkspace", OutputWorkspace="matrix_ws", DataX="0", DataY="1")
            self.__pre_existing_matrix_workspace_name = pre_existing_matrix_workspace_alg.getPropertyValue("OutputWorkspace")

            # Create an empty table workspace.
            table_workspace_alg = run_algorithm("CreateEmptyTableWorkspace", OutputWorkspace="__empty_table")
            self.__empty_table_workspace_name = table_workspace_alg.getPropertyValue("OutputWorkspace")

            self.__existing_range_of_run_files = "21360, 26173, 38633"
            self.__nonexistant_run_file = "99999"

    def test_wrong_facility_throws(self):
        """Dont expect to be able to support non-ISIS runs."""
        config["default.facility"] = "SNS"
        self.assertRaisesRegex(
            RuntimeError,
            "Only ISIS runs are supported by this alg.",
            run_algorithm,
            "RetrieveRunInfo",
            Runs=self.__existing_range_of_run_files,
            OutputWorkspace="test",
            rethrow=True,
        )

    def test_missing_run_file_throws(self):
        """Check that ALL files are present before proceeding."""
        self.assertRaisesRegex(
            RuntimeError,
            "Unable to find file: search object '99999'",
            run_algorithm,
            "RetrieveRunInfo",
            Runs=self.__nonexistant_run_file,
            OutputWorkspace="test",
            rethrow=True,
        )

    def test_pre_existing_non_table_workspace_throws(self):
        """Only allow TableWorkspaces."""
        self.assertRaisesRegex(
            RuntimeError,
            'Workspace "matrix_ws" already exists. Either delete it, or choose another workspace name.',
            run_algorithm,
            "RetrieveRunInfo",
            Runs=self.__existing_range_of_run_files,
            OutputWorkspace=self.__pre_existing_matrix_workspace_name,
            rethrow=True,
        )

    def test_existing_table_workspace_throws(self):
        """Dont bother trying to append.  If it exists already, we throw."""
        self.assertRaisesRegex(
            RuntimeError,
            'Workspace "__empty_table" already exists. Either delete it, or choose another workspace name.',
            run_algorithm,
            "RetrieveRunInfo",
            Runs=self.__existing_range_of_run_files,
            OutputWorkspace=self.__empty_table_workspace_name,
            rethrow=True,
        )

    def test_output_ws_correct_size(self):
        """Does a standard example return a table with the right amount of
        stuff in it?"""
        run_algorithm("RetrieveRunInfo", Runs=self.__existing_range_of_run_files, OutputWorkspace="test", rethrow=True)

        output_ws = mtd["test"]

        self.assertEqual(output_ws.columnCount(), 5)  # Five log props.
        self.assertEqual(output_ws.rowCount(), 3)  # Three runs.

        DeleteWorkspace(Workspace="test")


if __name__ == "__main__":
    unittest.main()
