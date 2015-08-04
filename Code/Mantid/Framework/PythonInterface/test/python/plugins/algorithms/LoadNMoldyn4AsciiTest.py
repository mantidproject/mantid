#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid.simpleapi import *
from mantid.api import *

import os

class LoadNMoldyn4AsciiTest(unittest.TestCase):

    def setUp(self):
        # This test requires the directory to be provided, this is in the
        # UnitTest directory so do get this from the serch directories
        data_dirs = config['datasearch.directories'].split(';')
        unit_test_data_dir = [p for p in data_dirs if 'UnitTest' in p][0]
        self._data_directory = os.path.join(unit_test_data_dir, 'nmoldyn4_data')


    def _validate_fqt_ws(self, workspace):
        """
        Validates a workspace containing an F(Q, t) function.

        @param workspace Workspace to validate
        """
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 21)
        self.assertEqual(workspace.blocksize(), 100)
        self.assertEqual(str(workspace.getAxis(0).getUnit().symbol()), 'ps')


    def _validate_sqf_ws(self, workspace):
        """
        Validates a workspace containing an S(Q, f) function.

        @param workspace Workspace to validate
        """
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 21)
        self.assertEqual(workspace.blocksize(), 199)
        self.assertEqual(str(workspace.getAxis(0).getUnit().symbol()), 'THz')


    def test_load_single_fqt_function(self):
        """
        Tests loading a single F(Q, t) function.
        """
        function_ws = LoadNMoldyn4Ascii(Directory=self._data_directory,
                                        Functions=['fqt_total'],
                                        OutputWorkspace='__LoadNMoldyn4Ascii_test')
        self._validate_fqt_ws(function_ws)


    def test_load_single_sqf_function(self):
        """
        Tests loading a single S(Q, f) function.
        """
        function_ws = LoadNMoldyn4Ascii(Directory=self._data_directory,
                                        Functions=['sqf_total'],
                                        OutputWorkspace='__LoadNMoldyn4Ascii_test')
        self._validate_sqf_ws(function_ws)


    def test_load_multiple_functions(self):
        """
        Tests loading multiple functions from a data directory.
        """
        function_wsg = LoadNMoldyn4Ascii(Directory=self._data_directory,
                                         Functions=['sqf_total', 'fqt_total'],
                                         OutputWorkspace='__LoadNMoldyn4Ascii_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self._validate_sqf_ws(function_wsg[0])
        self._validate_fqt_ws(function_wsg[1])


    def test_load_multiple_functions_some_skipped(self):
        """
        Tests loading multiple functions from a data directory where some
        functions do not exist.
        """
        function_wsg = LoadNMoldyn4Ascii(Directory=self._data_directory,
                                         Functions=['sqf_total', 'fqt_total', 'sqw_total'],
                                         OutputWorkspace='__LoadNMoldyn4Ascii_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self._validate_sqf_ws(function_wsg[0])
        self._validate_fqt_ws(function_wsg[1])


    def test_load_all_functions_skipped(self):
        """
        Tests error handling when all functions could not be loaded.
        """
        self.assertRaises(RuntimeError,
                          LoadNMoldyn4Ascii,
                          Directory=self._data_directory,
                          Functions=['sqw_total', 'sqw_H'],
                          OutputWorkspace='__LoadNMoldyn4Ascii_test')


    def test_fail_on_no_functions(self):
        """
        Tests error handling when no functions are specified.
        """
        self.assertRaises(RuntimeError,
                          LoadNMoldyn4Ascii,
                          Directory=self._data_directory,
                          OutputWorkspace='__LoadNMoldyn4Ascii_test')


if __name__ == '__main__':
    unittest.main()
