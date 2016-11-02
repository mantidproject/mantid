from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *
import os
import math

class LoadNMoldyn4Ascii1DTest(unittest.TestCase):
    def setUp(self):
        data_dirs = config['datasearch.directories'].split(';')
        unit_test_data_dir = [p for p in data_dirs if 'UnitTest' in p][0]
        self._data_directory = os.path.join(unit_test_data_dir, 'nmoldyn4_data1D')

    # ============================== Validation ======================================#

    def _validate_dos_ws(self, workspace):
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 1)
        self.assertEqual(workspace.blocksize(), 999)
        self.assertEqual(str(workspace.getAxis(0).getUnit().unitID()), 
                         'Energy_inWavenumber')

    def _validate_dos_total_convolution(self, workspace):
        data_y = workspace.readY(0)
        self.assertTrue(math.isnan(data_y[0]))
        self.assertAlmostEqual(data_y[-1], 8.64166846648649e-06)
        self.assertAlmostEqual(data_y[500], 0.000294421598059313)

    def _validate_vacf_ws(self, workspace):
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertEqual(workspace.getNumberHistograms(), 1)
        self.assertEqual(workspace.blocksize(), 999)
        self.assertEqual(str(workspace.getAxis(0).getUnit().unitID()), 'TOF')

    # ================================ Test cases ======================================#

    def test_load_single_dos_function(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='dos_total', 
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 1)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(dos_total)'))
        self._validate_dos_ws(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])

    def test_load_single_vacf_function(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='vacf_total', 
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 1)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(vacf_total)'))
        self._validate_vacf_ws(mtd['__LoadNMoldyn4Ascii1D_test(vacf_total)'])

    def test_load_multiple_functions(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='vacf_total, dos_total', 
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(vacf_total)'))
        self._validate_vacf_ws(mtd['__LoadNMoldyn4Ascii1D_test(vacf_total)'])
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(dos_total)'))
        self._validate_dos_ws(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])

    def test_load_dos_function_and_convolute(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='dos_total',
                                           ResolutionConvolution='TOSCA',
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 1)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(dos_total)'))
        self._validate_dos_ws(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])
        self._validate_dos_total_convolution(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])

    def test_load_multiple_functions_some_skipped(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='vacf_total, dos_total, something', 
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(vacf_total)'))
        self._validate_vacf_ws(mtd['__LoadNMoldyn4Ascii1D_test(vacf_total)'])
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(dos_total)'))
        self._validate_dos_ws(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])
        
    def test_load_multiple_functions_some_convoluted(self):
        function_wsg = LoadNMoldyn4Ascii1D(Directory=self._data_directory, 
                                           Functions='dos_total, vacf_total',
                                           ResolutionConvolution='TOSCA',
                                           OutputWorkspace='__LoadNMoldyn4Ascii1D_test')
        self.assertTrue(isinstance(function_wsg, WorkspaceGroup))
        self.assertEqual(len(function_wsg), 2)
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(vacf_total)'))
        self._validate_vacf_ws(mtd['__LoadNMoldyn4Ascii1D_test(vacf_total)'])
        self.assertTrue(function_wsg.contains('__LoadNMoldyn4Ascii1D_test(dos_total)'))
        self._validate_dos_ws(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])
        self._validate_dos_total_convolution(mtd['__LoadNMoldyn4Ascii1D_test(dos_total)'])            

    # ============================= Test failure cases =========================================#

    def test_load_all_functions_skipped(self):
        self.assertRaises(RuntimeError,
                          LoadNMoldyn4Ascii1D,
                          Directory=self._data_directory,
                          Functions='something, somethingelse',
                          OutputWorkspace='__LoadNMoldyn4Ascii_test')

    def test_fail_on_no_functions(self):
        self.assertRaises(RuntimeError,
                          LoadNMoldyn4Ascii1D,
                          Directory=self._data_directory,
                          OutputWorkspace='__LoadNMoldyn4Ascii1D_test')

if __name__ == '__main__':
    unittest.main()
