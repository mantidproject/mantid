import unittest

import numpy as np
from mock import Mock

from mantid.plots.helperfunctions import _dim2array, _get_wksp_index_and_spec_num, get_distribution, \
    get_wksp_index_dist_and_label, points_from_boundaries, validate_args
from mantid.plots.utility import MantidAxType
from mantid.simpleapi import CreateSampleWorkspace, CreateSingleValuedWorkspace, CreateMD

class MockMantidAxes:
    def __init__(self):
        self.set_xlabel = Mock()


class HelperfunctionsTest(unittest.TestCase):
    def test_validate_args_success(self):
        ws = CreateSampleWorkspace()
        result = validate_args(ws)
        self.assertEqual(True, result)

    def test_get_distribution(self):
        ws = CreateSampleWorkspace()
        result = get_distribution(ws)
        self.assertEqual((False, {}), result)

    def test_get_distribution_from_kwargs(self):
        ws = CreateSampleWorkspace()
        result = get_distribution(ws, distribution=True)
        self.assertEqual((True, {}), result)
        result = get_distribution(ws, distribution=False)
        self.assertEqual((False, {}), result)

    def test_points_from_boundaries(self):
        arr = np.array([2, 4, 6, 8])
        result = points_from_boundaries(arr)

        self.assertTrue(np.array_equal([3, 5, 7], result))

    def test_points_from_boundaries_raise_length_less_than_2(self):
        arr = np.array([1])
        self.assertRaises(ValueError, points_from_boundaries, arr)

    def test_points_from_boundaries_raise_not_numpy_array(self):
        arr = [1, 2, 3, 4]
        self.assertRaises(AssertionError, points_from_boundaries, arr)

    def test_dim2array(self):
        class MockIMDDimension:
            def __init__(self):
                self.getMinimum = Mock(return_value=1)
                self.getMaximum = Mock(return_value=10)
                self.getNBins = Mock(return_value=3)

        mock_dimension = MockIMDDimension()
        result = _dim2array(mock_dimension)
        self.assertTrue(np.array_equal([1., 4., 7., 10.], result))

    def test_get_wksp_index_and_spec_num_with_specNum_axis_spectrum(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace when traversing the SPECTRUM axis

        This test provides a spectrum number and expects the workspace index to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis, specNum=3)
        self.assertEqual(2, res_workspace_index)
        self.assertEqual(3, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_with_specNum_axis_bin(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace when traversing the BIN axis

        This test provides a spectrum number and expects the workspace index to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis, specNum=3)
        self.assertEqual(2, res_workspace_index)
        self.assertEqual(3, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_1_histogram_axis_spectrum(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace with 1 histogram,
        when traversing the SPECTRUM axis
        """
        ws = CreateSingleValuedWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis)
        self.assertEqual(0, res_workspace_index)
        self.assertEqual(None, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_1_histogram_axis_bin(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace with 1 histogram,
        when traversing the BIN axis
        """
        ws = CreateSingleValuedWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis)
        self.assertEqual(0, res_workspace_index)
        self.assertEqual(None, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_with_wkspIndex_axis_bin(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace, when traversing the BIN axis

        This test provides a workspace index, and expects the spectrum number to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis, wkspIndex=5)
        self.assertEqual(5, res_workspace_index)
        self.assertEqual(None, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_with_wkspIndex_axis_spectrum(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace, when traversing the SPECTRUM axis

        This test provides a workspace index, and expects the spectrum number to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_spectrum_number, res_kwargs = _get_wksp_index_and_spec_num(ws, axis, wkspIndex=5)
        self.assertEqual(5, res_workspace_index)
        self.assertEqual(6, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_error_with_both(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace

        This test checks that an error is shown when both a spectrum number and a workspace index are passed in
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM  # doesn't matter for this test
        self.assertRaises(RuntimeError, _get_wksp_index_and_spec_num, ws, axis, wkspIndex=5, specNum=3)

    def test_get_wksp_index_and_spec_num_error_with_none(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace

        This test checks that an error is shown when neither spectrum number nor workspace index is passed in
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM  # doesn't matter for this test
        self.assertRaises(RuntimeError, _get_wksp_index_and_spec_num, ws, axis)

    def test_get_wksp_index_dist_and_label(self):
        """
        Tests that the workspace index, distribution and label are correctly retrieved.

        The label changes depending on the axis.
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_distribution, res_kwargs = get_wksp_index_dist_and_label(ws, axis, wkspIndex=1)

        self.assertEqual(1, res_workspace_index)
        self.assertEqual(False, res_distribution)
        self.assertEqual(res_kwargs['label'], 'ws: spec 2')

        axis = MantidAxType.BIN
        res_workspace_index, res_distribution, res_kwargs = get_wksp_index_dist_and_label(ws, axis, wkspIndex=1)

        self.assertEqual(1, res_workspace_index)
        self.assertEqual(False, res_distribution)
        self.assertEqual(res_kwargs['label'], 'ws: bin 1')

    def test_get_md_data(self):
        ws = CreateSampleWorkspace()
        ws1 = CreateSampleWorkspace()
        mdws = CreateMD([ws, ws1])


