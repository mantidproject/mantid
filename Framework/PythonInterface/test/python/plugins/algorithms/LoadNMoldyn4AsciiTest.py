# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods,invalid-name
import unittest
from mantid.api import mtd, MatrixWorkspace, WorkspaceGroup
from mantid.kernel import config
from mantid.simpleapi import LoadNMoldyn4Ascii

import os


class LoadNMoldyn4AsciiTest(unittest.TestCase):
    """
    Note that the test files used here are not axactly in the same format as
    what nMoldyn produces as CMake can't handle files with brackets or commas.
    """

    def setUp(self):
        # This test requires the directory to be provided, this is in the
        # UnitTest directory so do get this from the serch directories
        data_dirs = config["datasearch.directories"].split(";")
        unit_test_data_dir = [p for p in data_dirs if "UnitTest" in p][0]
        self._data_directory = os.path.join(unit_test_data_dir, "nmoldyn4_data")

    def _validate_fqt_ws(self, workspace):
        """
        Validates a workspace containing an F(Q, t) function.

        @param workspace Workspace to validate
        """
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 21)
        self.assertEqual(workspace.blocksize(), 100)
        self.assertEqual(str(workspace.getAxis(0).getUnit().unitID()), "TOF")
        self.assertEqual(str(workspace.getAxis(1).getUnit().unitID()), "MomentumTransfer")

    def _validate_sqf_ws(self, workspace):
        """
        Validates a workspace containing an S(Q, f) function.

        @param workspace Workspace to validate
        """
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 21)
        self.assertEqual(workspace.blocksize(), 199)
        self.assertEqual(str(workspace.getAxis(0).getUnit().unitID()), "Energy")
        self.assertEqual(str(workspace.getAxis(1).getUnit().unitID()), "MomentumTransfer")

    def test_load_single_fqt_function(self):
        """
        Tests loading a single F(Q, t) function.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions=["fqt_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 1)
        self.assertTrue(function_wsg.contains("fqt_total"))
        self._validate_fqt_ws(mtd["fqt_total"])

    def test_load_single_sqf_function(self):
        """
        Tests loading a single S(Q, f) function.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions=["sqf_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 1)
        self.assertTrue(function_wsg.contains("sqf_total"))
        self._validate_sqf_ws(mtd["sqf_total"])

    def test_load_multiple_functions_list_short_name(self):
        """
        Tests loading multiple functions from a data directory giving the short
        function name in a Python list.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions=["sqf_total", "fqt_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains("sqf_total"))
        self.assertTrue(function_wsg.contains("fqt_total"))
        self._validate_sqf_ws(mtd["sqf_total"])
        self._validate_fqt_ws(mtd["fqt_total"])

    def test_load_multiple_functions_list_full_name(self):
        """
        Tests loading multiple functions from a data directory giving the full
        function name as a Python list.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions=["sq,f_total", "fq,t_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains("sqf_total"))
        self.assertTrue(function_wsg.contains("fqt_total"))
        self._validate_sqf_ws(mtd["sqf_total"])
        self._validate_fqt_ws(mtd["fqt_total"])

    def test_load_multiple_functions_string_short_name(self):
        """
        Tests loading multiple functions from a data directory giving the short
        function name as a string.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions="sqf_total,fqt_total", OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains("sqf_total"))
        self.assertTrue(function_wsg.contains("fqt_total"))
        self._validate_sqf_ws(mtd["sqf_total"])
        self._validate_fqt_ws(mtd["fqt_total"])

    def test_load_multiple_functions_some_skipped(self):
        """
        Tests loading multiple functions from a data directory where some
        functions do not exist.
        """
        function_wsg = LoadNMoldyn4Ascii(
            Directory=self._data_directory, Functions=["sqf_total", "fqt_total", "sqw_total"], OutputWorkspace="__LoadNMoldyn4Ascii_test"
        )
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains("sqf_total"))
        self.assertTrue(function_wsg.contains("fqt_total"))
        self._validate_sqf_ws(mtd["sqf_total"])
        self._validate_fqt_ws(mtd["fqt_total"])

    def test_load_all_functions_skipped(self):
        """
        Tests error handling when all functions could not be loaded.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Failed to load any functions for data",
            LoadNMoldyn4Ascii,
            Directory=self._data_directory,
            Functions=["sqw_total", "sqw_H"],
            OutputWorkspace="__LoadNMoldyn4Ascii_test",
        )

    def test_fail_on_no_functions(self):
        """
        Tests error handling when no functions are specified.
        """
        self.assertRaisesRegex(
            RuntimeError,
            "Must specify at least one function to load",
            LoadNMoldyn4Ascii,
            Directory=self._data_directory,
            OutputWorkspace="__LoadNMoldyn4Ascii_test",
        )


if __name__ == "__main__":
    unittest.main()
