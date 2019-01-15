import unittest

import numpy
import numpy as np
from mock import Mock

from mantid.plots.helperfunctions import _dim2array, _get_wksp_index_and_spec_num, get_distribution, \
    get_wksp_index_dist_and_label, points_from_boundaries, validate_args, get_md_data, get_spectrum, get_bins, \
    get_md_data2d_bin_bounds
from mantid.plots.utility import MantidAxType
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, CreateSingleValuedWorkspace, CreateMDHistoWorkspace


def add_workspace_with_data(func):
    def wrapper(self):
        dataX = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]
        dataY = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
        dataE = dataY
        dX = dataY

        ws = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4, UnitX="Wavelength", Dx=dX)
        return func(self, ws)

    return wrapper


def add_md_workspace_with_data(dimensions=2):
    def function_wrapper(func):
        def wrapper(self):
            if dimensions == 2:
                S = range(0, 100)
                ERR = range(0, 100)
                mdws = CreateMDHistoWorkspace(Dimensionality=2, Extents='-3,3,-10,10', SignalInput=S, ErrorInput=ERR,
                                              NumberOfBins='10,10', Names='Dim1,Dim2',
                                              Units='MomentumTransfer,EnergyTransfer')
            else:
                S = range(0, 1000)
                ERR = range(0, 1000)
                mdws = CreateMDHistoWorkspace(Dimensionality=3, Extents='-3,3,-10,10,-20,20', SignalInput=S,
                                              ErrorInput=ERR,
                                              NumberOfBins='10,10,10', Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,EnergyTransfer')
            return func(self, mdws)
        return wrapper
    return function_wrapper


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

    @add_md_workspace_with_data
    def test_get_md_data_no_error(self, mdws):
        dim_arrays, data, err = get_md_data(mdws, normalization=None)
        self.assertEqual(11, len(dim_arrays[0]))
        self.assertEqual(-3, dim_arrays[0][0])
        self.assertEqual(3, dim_arrays[0][-1])

        self.assertEqual(11, len(dim_arrays[1]))
        self.assertEqual(-10, dim_arrays[1][0])
        self.assertEqual(10, dim_arrays[1][-1])

        self.assertTrue(all(len(d) == 10 for d in data))
        self.assertEqual(0.0, data[0][0])
        self.assertEqual(99.0, data[-1][-1])

        self.assertIsNone(err)

    @add_md_workspace_with_data
    def test_get_md_data_with_error(self, mdws):
        dim_arrays, data, err = get_md_data(mdws, normalization=None, withError=True)
        self.assertEqual(11, len(dim_arrays[0]))
        self.assertEqual(-3, dim_arrays[0][0])
        self.assertEqual(3, dim_arrays[0][-1])

        self.assertEqual(11, len(dim_arrays[1]))
        self.assertEqual(-10, dim_arrays[1][0])
        self.assertEqual(10, dim_arrays[1][-1])

        self.assertTrue(all(len(d) == 10 for d in data))
        self.assertEqual(0.0, data[0][0])
        self.assertEqual(99.0, data[-1][-1])

        self.assertTrue(all(len(e) == 10 for e in err))
        self.assertEqual(0.0, err[0][0])
        self.assertEqual(99.0, err[-1][-1])

    @add_workspace_with_data
    def test_get_spectrum_no_dy_dx(self, ws):
        x, y, dy, dx = get_spectrum(ws, 3, distribution=False, withDy=False, withDx=False)
        self.assertTrue(numpy.array_equal([13.5, 14.5, 15.5], x))
        self.assertTrue(numpy.array_equal([10.0, 11.0, 12.0], y))
        self.assertIsNone(dy)
        self.assertIsNone(dx)

    @add_workspace_with_data
    def test_get_spectrum_with_dy_dx(self, ws):
        x, y, dy, dx = get_spectrum(ws, 3, distribution=False, withDy=True, withDx=True)

        self.assertTrue(numpy.array_equal([13.5, 14.5, 15.5], x))
        self.assertTrue(numpy.array_equal([10.0, 11.0, 12.0], y))
        self.assertTrue(numpy.array_equal([10.0, 11.0, 12.0], dy))
        self.assertTrue(numpy.array_equal([10.0, 11.0, 12.0], dx))

    @add_workspace_with_data
    def test_get_bins_no_dy(self, ws):
        x, y, dy, dx = get_bins(ws, 1, withDy=False)
        self.assertTrue(numpy.array_equal([0, 1, 2, 3], x))
        self.assertTrue(numpy.array_equal([2.0, 5.0, 8.0, 11.0], y))
        self.assertIsNone(dy)

    @add_workspace_with_data
    def test_get_bins_with_dy(self, ws):
        x, y, dy, dx = get_bins(ws, 1, withDy=True)
        self.assertTrue(numpy.array_equal([0, 1, 2, 3], x))
        self.assertTrue(numpy.array_equal([2.0, 5.0, 8.0, 11.0], y))
        self.assertTrue(numpy.array_equal([2.0, 5.0, 8.0, 11.0], dy))

    @add_md_workspace_with_data()
    def test_get_md_data2d_bin_bounds(self, mdws):
        coord1, coord2, data = get_md_data2d_bin_bounds(mdws, False)
        self.assertEqual(11, len(coord1))
        self.assertEqual(-3, coord1[0])
        self.assertEqual(3, coord1[-1])

        self.assertEqual(11, len(coord2))
        self.assertEqual(-10, coord2[0])
        self.assertEqual(10, coord2[-1])

        self.assertTrue(all(len(d) == 10 for d in data))
        self.assertEqual(0.0, data[0][0])
        self.assertEqual(99.0, data[-1][-1])

    @add_md_workspace_with_data(dimensions=3)
    def test_get_md_data2d_bin_bounds_raises_AssertionException(self, mdws):
        self.assertRaises(AssertionError, get_md_data2d_bin_bounds, mdws, False)
