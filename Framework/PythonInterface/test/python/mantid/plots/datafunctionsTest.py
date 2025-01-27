# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import datetime
import unittest

import matplotlib

matplotlib.use("AGG")
from matplotlib.pyplot import figure
import numpy as np

import mantid.api
import mantid.plots.datafunctions as funcs
from unittest.mock import Mock
from mantid.kernel import config, ConfigService
from mantid.plots.utility import MantidAxType
from mantid.simpleapi import (
    AddSampleLog,
    AddTimeSeriesLog,
    ConjoinWorkspaces,
    CreateMDHistoWorkspace,
    CreateSampleWorkspace,
    CreateSingleValuedWorkspace,
    CreateWorkspace,
    DeleteWorkspace,
    LoadRaw,
)


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
                mdws = CreateMDHistoWorkspace(
                    Dimensionality=2,
                    Extents="-3,3,-10,10",
                    SignalInput=S,
                    ErrorInput=ERR,
                    NumberOfBins="10,10",
                    Names="Dim1,Dim2",
                    Units="MomentumTransfer,EnergyTransfer",
                )
            else:
                S = range(0, 1000)
                ERR = range(0, 1000)
                mdws = CreateMDHistoWorkspace(
                    Dimensionality=3,
                    Extents="-3,3,-10,10,-20,20",
                    SignalInput=S,
                    ErrorInput=ERR,
                    NumberOfBins="10,10,10",
                    Names="Dim1,Dim2,Dim3",
                    Units="MomentumTransfer,EnergyTransfer,EnergyTransfer",
                )
            return func(self, mdws)

        return wrapper

    return function_wrapper


class DataFunctionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.g1da = config["graph1d.autodistribution"]
        config["graph1d.autodistribution"] = "On"
        cls.ws2d_non_distribution = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            Distribution=False,
            UnitX="Wavelength",
            YUnitLabel="Counts per microAmp.hour",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_non_distribution",
        )
        cls.ws2d_distribution = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5, 6],
            DataE=[1, 2, 3, 4, 6],
            NSpec=1,
            Distribution=True,
            UnitX="Wavelength",
            YUnitLabel="Counts per microAmp.hour",
            OutputWorkspace="ws2d_distribution",
        )
        cls.ws2d_histo = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            Distribution=True,
            UnitX="Wavelength",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_histo",
        )
        cls.ws2d_point = CreateWorkspace(DataX=[1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4], DataY=[2] * 12, NSpec=3, OutputWorkspace="ws2d_point")
        cls.ws1d_point = CreateWorkspace(DataX=[1, 2], DataY=[1, 2], NSpec=1, Distribution=False, OutputWorkspace="ws1d_point")
        cls.ws2d_histo_rag = CreateWorkspace(
            DataX=[1, 2, 3, 4, 5, 2, 4, 6, 8, 10],
            DataY=[2] * 8,
            NSpec=2,
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[5, 7, 9],
            OutputWorkspace="ws2d_histo_rag",
        )
        cls.ws2d_point_rag = CreateWorkspace(DataX=[1, 2, 3, 4, 2, 4, 6, 8], DataY=[2] * 8, NSpec=2, OutputWorkspace="ws2d_point_rag")
        cls.ws_MD_2d = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-3,3,-10,10,-1,1",
            SignalInput=range(25),
            ErrorInput=range(25),
            NumberOfEvents=10 * np.ones(25),
            NumberOfBins="5,5,1",
            Names="Dim1,Dim2,Dim3",
            Units="MomentumTransfer,EnergyTransfer,Angstrom",
            OutputWorkspace="ws_MD_2d",
        )
        cls.ws_MD_1d = CreateMDHistoWorkspace(
            Dimensionality=3,
            Extents="-3,3,-10,10,-1,1",
            SignalInput=range(5),
            ErrorInput=range(5),
            NumberOfEvents=10 * np.ones(5),
            NumberOfBins="1,5,1",
            Names="Dim1,Dim2,Dim3",
            Units="MomentumTransfer,EnergyTransfer,Angstrom",
            OutputWorkspace="ws_MD_1d",
        )
        cls.ws2d_point_uneven = CreateWorkspace(DataX=[10, 20, 30], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="ws2d_point_uneven")
        cls.ws2d_high_counting_detector = CreateWorkspace(
            DataX=[1, 2, 3, 4] * 1000,
            DataY=[2] * 4 * 12 + [200] * 4 + [2] * 987 * 4,
            NSpec=1000,
            OutputWorkspace="ws2d_high_counting_detector",
        )
        wp = CreateWorkspace(DataX=[15, 25, 35, 45], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(cls.ws2d_point_uneven, wp, CheckOverlapping=False, CheckMatchingBins=False)
        cls.ws2d_point_uneven = mantid.mtd["ws2d_point_uneven"]
        cls.ws2d_histo_uneven = CreateWorkspace(DataX=[10, 20, 30, 40], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="ws2d_histo_uneven")
        wp = CreateWorkspace(DataX=[15, 25, 35, 45, 55], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(cls.ws2d_histo_uneven, wp, CheckOverlapping=False, CheckMatchingBins=False)
        cls.ws2d_histo_uneven = mantid.mtd["ws2d_histo_uneven"]
        newYAxis = mantid.api.NumericAxis.create(3)
        newYAxis.setValue(0, 10)
        newYAxis.setValue(1, 15)
        newYAxis.setValue(2, 25)
        cls.ws2d_histo_uneven.replaceAxis(1, newYAxis)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:30:00", Value=15)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:50:00", Value=100.2)

    @classmethod
    def tearDownClass(cls):
        config["graph1d.autodistribution"] = cls.g1da
        DeleteWorkspace("ws2d_non_distribution")
        DeleteWorkspace("ws2d_distribution")
        DeleteWorkspace("ws2d_histo")
        DeleteWorkspace("ws2d_point")
        DeleteWorkspace("ws1d_point")
        DeleteWorkspace("ws_MD_2d")
        DeleteWorkspace("ws_MD_1d")
        DeleteWorkspace("ws2d_histo_rag")
        DeleteWorkspace("ws2d_point_rag")
        DeleteWorkspace("ws2d_point_uneven")
        DeleteWorkspace("ws2d_histo_uneven")

    def test_get_wksp_index_dist_and_label(self):
        # fail case
        self.assertRaises(RuntimeError, funcs.get_wksp_index_dist_and_label, self.ws2d_histo)
        # get info from a 2d workspace
        index, dist, kwargs = funcs.get_wksp_index_dist_and_label(self.ws2d_histo, specNum=2)
        self.assertEqual(index, 1)
        self.assertTrue(dist)
        self.assertEqual(kwargs["label"], "ws2d_histo: 7")
        # get info from default spectrum in the 1d case
        index, dist, kwargs = funcs.get_wksp_index_dist_and_label(self.ws1d_point, wkspIndex=0)
        self.assertEqual(index, 0)
        self.assertFalse(dist)
        self.assertEqual(kwargs["label"], "ws1d_point: spec 1")

    def test_legend_uses_label_if_first_axis_is_text_axis(self):
        ws = CreateWorkspace([1], [1], NSpec=1)
        axis = mantid.api.TextAxis.create(1)
        ws.replaceAxis(1, axis)
        ws.getAxis(1).setLabel(0, "test")
        index, dist, kwargs = funcs.get_wksp_index_dist_and_label(ws, specNum=1)
        self.assertEqual(kwargs["label"], "ws: test")

    def test_legend_uses_label_if_first_axis_is_numeric_axis(self):
        ws = CreateWorkspace([1], [1], NSpec=1)
        axis = mantid.api.NumericAxis.create(1)
        ws.replaceAxis(1, axis)
        ws.getAxis(1).setValue(0, 50)
        index, dist, kwargs = funcs.get_wksp_index_dist_and_label(ws, specNum=1)
        self.assertEqual(kwargs["label"], "ws: 50")

    def test_get_axes_labels(self):
        axs = funcs.get_axes_labels(self.ws2d_histo)
        self.assertEqual(axs, ("($\\AA$)$^{-1}$", "Wavelength ($\\AA$)", "Energy transfer ($meV$)"))

    def test_y_units_correct_on_distribution_workspace(self):
        ws = self.ws2d_distribution
        labels = funcs.get_axes_labels(ws)
        self.assertEqual(labels[0], "Counts (microAmp.hour)$^{-1}$")

    def test_y_units_for_distribution_and_autodist_off_with_latex(self):
        ws = self.ws2d_distribution
        labels = funcs.get_axes_labels(ws, normalize_by_bin_width=False, use_latex=True)
        self.assertEqual(labels[0], "Counts (microAmp.hour)$^{-1}$")

    def test_y_units_for_non_distribution_and_autodist_on_with_ascii(self):
        ws = self.ws2d_non_distribution
        labels = funcs.get_axes_labels(ws, normalize_by_bin_width=True, use_latex=False)
        self.assertEqual(labels[0], "Counts per microAmp.hour per Angstrom")

    def test_y_units_for_non_distribution_and_autodist_on_with_latex(self):
        ws = self.ws2d_non_distribution
        labels = funcs.get_axes_labels(ws, normalize_by_bin_width=True)
        self.assertEqual(labels[0], "Counts (microAmp.hour $\\AA$)$^{-1}$")

    def test_y_units_for_non_distribution_with_no_x_or_y_unit_and_autodist_on(self):
        ws = self.ws1d_point
        labels = funcs.get_axes_labels(ws, normalize_by_bin_width=True, use_latex=True)
        self.assertEqual(labels[0], "")

    def test_y_units_for_non_distribution_with_no_x_unit_and_autodist_on(self):
        ws = self.ws2d_point
        ws.setYUnit("Counts")
        labels = funcs.get_axes_labels(ws, normalize_by_bin_width=True, use_latex=True)
        self.assertEqual(labels[0], "Counts")
        ws.setYUnit("")

    def test_get_axes_label_2d_MDWS(self):
        axs = funcs.get_axes_labels(self.ws_MD_2d)
        # should get the first two dimension labels only
        self.assertEqual(axs, ("Intensity", "Dim1 ($\\AA^{-1}$)", "Dim2 (EnergyTransfer)", ""))

    def test_get_axes_label_2d_MDWS_indices(self):
        axs = funcs.get_axes_labels(self.ws_MD_2d, indices=(0, slice(None), 0))
        # should get the first two dimension labels only
        self.assertEqual(axs, ("Intensity", "Dim2 (EnergyTransfer)", "Dim1=-2.4; Dim3=0.0;"))

    def test_get_data_uneven_flag(self):
        flag, kwargs = funcs.get_data_uneven_flag(self.ws2d_histo_rag, axisaligned=True, other_kwarg=1)
        self.assertTrue(flag)
        self.assertEqual(kwargs, {"other_kwarg": 1})
        flag, kwargs = funcs.get_data_uneven_flag(self.ws2d_histo_rag, other_kwarg=2)
        self.assertFalse(flag)
        self.assertEqual(kwargs, {"other_kwarg": 2})
        flag, kwargs = funcs.get_data_uneven_flag(self.ws2d_histo_uneven, axisaligned=False, other_kwarg=3)
        self.assertTrue(flag)
        self.assertEqual(kwargs, {"other_kwarg": 3})

    def test_boundaries_from_points(self):
        centers = np.array([1.0, 2.0, 4.0, 8.0])
        bounds = funcs.boundaries_from_points(centers)
        self.assertTrue(np.array_equal(bounds, np.array([0.5, 1.5, 3, 6, 10])))

    def test_points_from_boundaries(self):
        bounds = np.array([1.0, 3, 4, 10])
        centers = funcs.points_from_boundaries(bounds)
        self.assertTrue(np.array_equal(centers, np.array([2.0, 3.5, 7])))

    def test_get_spectrum_distribution_workspace(self):
        # Since the workspace being plotted is a distribution, we should not
        # divide by bin width whether or not normalize_by_bin_width is True
        x, y, dy, dx = funcs.get_spectrum(self.ws2d_histo, 1, True, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x, np.array([15.0, 25.0])))
        self.assertTrue(np.array_equal(y, np.array([4, 5])))
        self.assertTrue(np.array_equal(dy, np.array([3, 4])))
        self.assertEqual(dx, None)

        x, y, dy, dx = funcs.get_spectrum(self.ws2d_histo, 0, False, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x, np.array([15.0, 25.0])))
        self.assertTrue(np.array_equal(y, np.array([2, 3])))
        self.assertTrue(np.array_equal(dy, np.array([1, 2])))
        self.assertEqual(dx, None)
        # fail case - try to find spectrum out of range
        self.assertRaises(RuntimeError, funcs.get_spectrum, self.ws2d_histo, 10, True)

    def test_get_spectrum_non_distribution_workspace(self):
        # get data divided by bin width
        x, y, dy, dx = funcs.get_spectrum(self.ws2d_non_distribution, 1, normalize_by_bin_width=True, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x, np.array([15.0, 25.0])))
        self.assertTrue(np.array_equal(y, np.array([0.4, 0.5])))
        self.assertTrue(np.array_equal(dy, np.array([0.3, 0.4])))
        self.assertEqual(dx, None)
        # get data not divided by bin width
        x, y, dy, dx = funcs.get_spectrum(self.ws2d_non_distribution, 1, normalize_by_bin_width=False, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x, np.array([15.0, 25.0])))
        self.assertTrue(np.array_equal(y, np.array([4, 5])))
        self.assertTrue(np.array_equal(dy, np.array([3, 4])))
        self.assertEqual(dx, None)
        # fail case - try to find spectrum out of range
        self.assertRaises(RuntimeError, funcs.get_spectrum, self.ws2d_non_distribution, 10, True)

    def test_get_md_data2d_bin_bounds(self):
        x, y, data = funcs.get_md_data2d_bin_bounds(self.ws_MD_2d, mantid.api.MDNormalization.NoNormalization)
        # logger.error(str(coords))
        np.testing.assert_allclose(x, np.array([-3, -1.8, -0.6, 0.6, 1.8, 3]), atol=1e-10)
        np.testing.assert_allclose(y, np.array([-10, -6, -2, 2, 6, 10.0]), atol=1e-10)
        np.testing.assert_allclose(data, np.arange(25).reshape(5, 5), atol=1e-10)

    def test_get_md_data2d_bin_centers(self):
        x, y, data = funcs.get_md_data2d_bin_centers(self.ws_MD_2d, mantid.api.MDNormalization.NumEventsNormalization)
        np.testing.assert_allclose(x, np.array([-2.4, -1.2, 0, 1.2, 2.4]), atol=1e-10)
        np.testing.assert_allclose(y, np.array([-8, -4, 0, 4, 8]), atol=1e-10)
        np.testing.assert_allclose(data, np.arange(25).reshape(5, 5) * 0.1, atol=1e-10)

    def test_get_md_data2d_bin_centers_transpose(self):
        """
        Same as the test above but should be the transpose
        """
        x, y, data = funcs.get_md_data2d_bin_centers(self.ws_MD_2d, mantid.api.MDNormalization.NumEventsNormalization, transpose=True)
        np.testing.assert_allclose(x, np.array([-8, -4, 0, 4, 8]), atol=1e-10)
        np.testing.assert_allclose(y, np.array([-2.4, -1.2, 0, 1.2, 2.4]), atol=1e-10)
        np.testing.assert_allclose(data, np.arange(25).reshape(5, 5).T * 0.1, atol=1e-10)

    def test_get_md_data1d(self):
        coords, data, err = funcs.get_md_data1d(self.ws_MD_1d, mantid.api.MDNormalization.NumEventsNormalization)
        np.testing.assert_allclose(coords, np.array([-8, -4, 0, 4, 8]), atol=1e-10)

    def test_get_matrix_2d_data_rect(self):
        # contour from aligned point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([[1, 2, 3, 4], [1, 2, 3, 4], [1, 2, 3, 4]]))
        np.testing.assert_allclose(y, np.array([[1, 1, 1, 1], [2, 2, 2, 2], [3, 3, 3, 3]]))
        # mesh from aligned point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point, True, histogram2D=True)
        np.testing.assert_allclose(
            x, np.array([[0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5]])
        )
        np.testing.assert_allclose(
            y, np.array([[0.5, 0.5, 0.5, 0.5, 0.5], [1.5, 1.5, 1.5, 1.5, 1.5], [2.5, 2.5, 2.5, 2.5, 2.5], [3.5, 3.5, 3.5, 3.5, 3.5]])
        )
        # contour from aligned histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([[15, 25], [15, 25]]))
        np.testing.assert_allclose(y, np.array([[5, 5], [7, 7]]))
        # mesh from aligned histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo, True, histogram2D=True)
        np.testing.assert_allclose(x, np.array([[10, 20, 30], [10, 20, 30], [10, 20, 30]]))
        np.testing.assert_allclose(y, np.array([[4, 4, 4], [6, 6, 6], [8, 8, 8]]))

    def test_get_matrix_2d_data_rect_transpose(self):
        # same as the test above bur should be the transpose
        # contour from aligned point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point, True, histogram2D=False, transpose=True)
        np.testing.assert_allclose(y, np.array([[1, 2, 3, 4], [1, 2, 3, 4], [1, 2, 3, 4]]).T)
        np.testing.assert_allclose(x, np.array([[1, 1, 1, 1], [2, 2, 2, 2], [3, 3, 3, 3]]).T)
        # mesh from aligned point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point, True, histogram2D=True, transpose=True)
        np.testing.assert_allclose(
            y, np.array([[0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5], [0.5, 1.5, 2.5, 3.5, 4.5]]).T
        )
        np.testing.assert_allclose(
            x, np.array([[0.5, 0.5, 0.5, 0.5, 0.5], [1.5, 1.5, 1.5, 1.5, 1.5], [2.5, 2.5, 2.5, 2.5, 2.5], [3.5, 3.5, 3.5, 3.5, 3.5]]).T
        )
        # contour from aligned histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo, True, histogram2D=False, transpose=True)
        np.testing.assert_allclose(y, np.array([[15, 25], [15, 25]]).T)
        np.testing.assert_allclose(x, np.array([[5, 5], [7, 7]]).T)
        # mesh from aligned histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo, True, histogram2D=True, transpose=True)
        np.testing.assert_allclose(y, np.array([[10, 20, 30], [10, 20, 30], [10, 20, 30]]).T)
        np.testing.assert_allclose(x, np.array([[4, 4, 4], [6, 6, 6], [8, 8, 8]]).T)

    def test_get_matrix_2d_data_rag(self):
        # contour from ragged point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point_rag, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([[1, 2, 3, 4], [2, 4, 6, 8]]))
        np.testing.assert_allclose(y, np.array([[1, 1, 1, 1], [2, 2, 2, 2]]))
        # contour from ragged histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo_rag, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([[1.5, 2.5, 3.5, 4.5], [3, 5, 7, 9]]))
        np.testing.assert_allclose(y, np.array([[6, 6, 6, 6], [8, 8, 8, 8]]))
        # mesh from ragged point data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_point_rag, True, histogram2D=True)
        np.testing.assert_allclose(x, np.array([[0.5, 1.5, 2.5, 3.5, 4.5], [1, 3, 5, 7, 9], [1, 3, 5, 7, 9]]))
        np.testing.assert_allclose(y, np.array([[0.5, 0.5, 0.5, 0.5, 0.5], [1.5, 1.5, 1.5, 1.5, 1.5], [2.5, 2.5, 2.5, 2.5, 2.5]]))
        # mesh from ragged histo data
        x, y, z = funcs.get_matrix_2d_data(self.ws2d_histo_rag, True, histogram2D=True)
        np.testing.assert_allclose(x, np.array([[1, 2, 3, 4, 5], [2, 4, 6, 8, 10], [2, 4, 6, 8, 10]]))
        np.testing.assert_allclose(y, np.array([[5, 5, 5, 5, 5], [7, 7, 7, 7, 7], [9, 9, 9, 9, 9]]))
        # check that fails for uneven data
        self.assertRaises(ValueError, funcs.get_matrix_2d_data, self.ws2d_point_uneven, True)

    def test_get_matrix_2d_data_ragged(self):
        # contour from ragged point data
        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_point_rag, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]))
        np.testing.assert_allclose(y, np.array([0.5, 1.5, 2.5]))
        # contour from ragged histo data
        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_histo_rag, True, histogram2D=False)
        np.testing.assert_allclose(x, np.array([1.5, 2.4375, 3.375, 4.3125, 5.25, 6.1875, 7.125, 8.0625, 9.0]))
        np.testing.assert_allclose(y, np.array([4.0, 6.0, 8.0, 10.0]))
        # mesh from ragged point data
        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_point_rag, True, histogram2D=True)
        np.testing.assert_allclose(x, np.array([0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5]))
        np.testing.assert_allclose(y, np.array([0.5, 1.5, 2.5]))
        # mesh from ragged histo data
        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_histo_rag, True, histogram2D=True)
        np.testing.assert_allclose(x, np.array([1.03125, 1.96875, 2.90625, 3.84375, 4.78125, 5.71875, 6.65625, 7.59375, 8.53125, 9.46875]))
        np.testing.assert_allclose(y, np.array([4.0, 6, 8.0, 10.0]))
        # check that fails for uneven data
        self.assertRaises(ValueError, funcs.get_matrix_2d_data, self.ws2d_point_uneven, True)

    def test_get_matrix_2d_data_ragged_with_extent(self):
        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_point_rag, False, histogram2D=True, extent=[2, 4, 1, 2], xbins=8, ybins=5)
        np.testing.assert_allclose(x, np.array([2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4]))
        np.testing.assert_allclose(y, np.array([1, 1.25, 1.5, 1.75, 2.0]))

        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_histo_rag, False, histogram2D=True, extent=[2, 6, 5, 9], xbins=8, ybins=5)
        np.testing.assert_allclose(x, np.array([2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6]))
        np.testing.assert_allclose(y, np.array([5, 6, 7, 8, 9]))

        x, y, z = funcs.get_matrix_2d_ragged(self.ws2d_histo_rag, False, histogram2D=True, extent=[2, 6, -5, 9], xbins=8, ybins=5)
        np.testing.assert_allclose(x, np.array([2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6]))
        np.testing.assert_allclose(y, np.array([-5, -1.5, 2, 5.5, 9]))
        # first z array is off the spectrum axis scale so should be masked
        np.testing.assert_array_equal(z[0].data, [np.nan] * 8)
        np.testing.assert_array_equal(z[0].mask, [1] * 8)
        np.testing.assert_array_equal(z[4].data, [2.0] * 8)
        np.testing.assert_array_equal(z[4].mask, [0] * 8)

    def test_get_matrix_2d_ragged_when_transpose_is_true(self):
        x, y, z_transposed = funcs.get_matrix_2d_ragged(
            self.ws2d_histo_rag, False, histogram2D=True, extent=[5, 9, 2, 6], xbins=8, ybins=5, transpose=True
        )

        np.testing.assert_allclose(x, np.array([5, 6, 7, 8, 9]))
        np.testing.assert_allclose(y, np.array([2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6]))

        _, _, z = funcs.get_matrix_2d_ragged(
            self.ws2d_histo_rag, False, histogram2D=True, extent=[2, 6, 5, 9], xbins=8, ybins=5, transpose=False
        )
        np.testing.assert_allclose(z_transposed, z.T)

    def test_get_matrix2d_ragged_with_maxpooling(self):
        x, y, z = funcs.get_matrix_2d_ragged(
            self.ws2d_high_counting_detector, False, histogram2D=True, extent=[1, 4, 1, 1000], xbins=4, ybins=20, maxpooling=True
        )

        # 12th spectra is high counting and will be the first entry in the data when we are using maxpooling
        np.testing.assert_allclose(z[0], self.ws2d_high_counting_detector.readY(12))

    def test_get_matrix2d_ragged_without_maxpooling(self):
        x, y, z = funcs.get_matrix_2d_ragged(
            self.ws2d_high_counting_detector, False, histogram2D=True, extent=[1, 4, 1, 1000], xbins=4, ybins=20, maxpooling=False
        )

        # 12th spectra is high counting but will skipped if we don't use maxpooling
        np.testing.assert_allclose(z[0], self.ws2d_high_counting_detector.readY(0))

    def test_get_uneven_data(self):
        # even points
        x, y, z = funcs.get_uneven_data(self.ws2d_point_rag, True)
        np.testing.assert_allclose(x[0], np.array([0.5, 1.5, 2.5, 3.5, 4.5]))
        np.testing.assert_allclose(x[1], np.array([1, 3, 5, 7, 9]))
        np.testing.assert_allclose(y[0], np.array([0.5, 1.5]))
        np.testing.assert_allclose(y[1], np.array([1.5, 2.5]))
        np.testing.assert_allclose(z[0], np.array([2, 2, 2, 2]))
        np.testing.assert_allclose(z[1], np.array([2, 2, 2, 2]))
        # even histo
        x, y, z = funcs.get_uneven_data(self.ws2d_histo_rag, True)
        np.testing.assert_allclose(x[0], np.array([1, 2, 3, 4, 5]))
        np.testing.assert_allclose(x[1], np.array([2, 4, 6, 8, 10]))
        np.testing.assert_allclose(y[0], np.array([5, 7]))
        np.testing.assert_allclose(y[1], np.array([7, 9]))
        np.testing.assert_allclose(z[0], np.array([2, 2, 2, 2]))
        np.testing.assert_allclose(z[1], np.array([2, 2, 2, 2]))
        # uneven points
        x, y, z = funcs.get_uneven_data(self.ws2d_point_uneven, True)
        np.testing.assert_allclose(x[0], np.array([5, 15, 25, 35]))
        np.testing.assert_allclose(x[1], np.array([10, 20, 30, 40, 50]))
        np.testing.assert_allclose(y[0], np.array([0.5, 1.5]))
        np.testing.assert_allclose(y[1], np.array([1.5, 2.5]))
        np.testing.assert_allclose(z[0], np.array([1, 2, 3]))
        np.testing.assert_allclose(z[1], np.array([1, 2, 3, 4]))
        # uneven histo
        x, y, z = funcs.get_uneven_data(self.ws2d_histo_uneven, True)
        np.testing.assert_allclose(x[0], np.array([10, 20, 30, 40]))
        np.testing.assert_allclose(x[1], np.array([15, 25, 35, 45, 55]))
        np.testing.assert_allclose(y[0], np.array([10, 15]))
        np.testing.assert_allclose(y[1], np.array([15, 25]))
        np.testing.assert_allclose(z[0], np.array([1, 2, 3]))
        np.testing.assert_allclose(z[1], np.array([1, 2, 3, 4]))

    def test_get_sample_logs_with_full_time(self):
        x, y, FullTime, LogName, units, kwargs = funcs.get_sample_log(self.ws2d_histo, LogName="my_log", FullTime=True)
        self.assertEqual(x[0], datetime.datetime(2010, 1, 1, 0, 0, 0))
        self.assertEqual(x[1], datetime.datetime(2010, 1, 1, 0, 30, 0))
        self.assertEqual(x[2], datetime.datetime(2010, 1, 1, 0, 50, 0))
        np.testing.assert_allclose(y, np.array([100, 15, 100.2]))
        self.assertTrue(FullTime)
        self.assertEqual(LogName, "my_log")
        self.assertEqual(units, "")
        self.assertEqual(kwargs, {})

    def test_get_sample_logs_with_relative_time_and_no_start_time(self):
        x, y, FullTime, LogName, units, kwargs = funcs.get_sample_log(self.ws2d_histo, LogName="my_log", FullTime=False)
        self.assertEqual(x[0], 0)
        self.assertEqual(x[1], 30 * 60)
        self.assertEqual(x[2], 50 * 60)
        np.testing.assert_allclose(y, np.array([100, 15, 100.2]))
        self.assertFalse(FullTime)
        self.assertEqual(LogName, "my_log")
        self.assertEqual(units, "")
        self.assertEqual(kwargs, {})

    def test_get_sample_logs_with_relative_time_and_start_time_later_than_first_log(self):
        start_time = "2010-01-01T00:00:19"
        AddSampleLog(self.ws2d_histo, LogName="run_start", LogText=start_time)
        x, y, FullTime, LogName, units, kwargs = funcs.get_sample_log(self.ws2d_histo, LogName="my_log", FullTime=False)
        self.assertEqual(x[0], -19)
        self.assertEqual(x[1], 1781)
        self.assertEqual(x[2], 2981)
        np.testing.assert_allclose(y, np.array([100, 15, 100.2]))
        self.assertFalse(FullTime)
        self.assertEqual(LogName, "my_log")
        self.assertEqual(units, "")
        self.assertEqual(kwargs, {})

    def test_validate_args_success(self):
        ws = CreateSampleWorkspace()
        result = funcs.validate_args(ws)
        self.assertEqual(True, result)

    def test_get_distribution(self):
        ws = CreateSampleWorkspace()
        result = funcs.get_distribution(ws)
        self.assertEqual((False, {}), result)

    def test_get_distribution_from_kwargs(self):
        ws = CreateSampleWorkspace()
        result = funcs.get_distribution(ws, distribution=True)
        self.assertEqual((True, {}), result)
        result = funcs.get_distribution(ws, distribution=False)
        self.assertEqual((False, {}), result)

    def test_get_distribution_returns_true_for_distribution_workspace(self):
        result = funcs.get_distribution(self.ws2d_distribution, distribution=False)
        self.assertEqual((True, {}), result)

    def test_points_from_boundaries_raise_length_less_than_2(self):
        arr = np.array([1])
        self.assertRaises(ValueError, funcs.points_from_boundaries, arr)

    def test_points_from_boundaries_raise_not_np_array(self):
        arr = [1, 2, 3, 4]
        self.assertRaises(AssertionError, funcs.points_from_boundaries, arr)

    def test_dim2array(self):
        class MockIMDDimension:
            def __init__(self):
                self.getMinimum = Mock(return_value=1)
                self.getMaximum = Mock(return_value=10)
                self.getNBins = Mock(return_value=3)

        mock_dimension = MockIMDDimension()
        result = funcs._dim2array(mock_dimension)
        self.assertTrue(np.array_equal([1.0, 4.0, 7.0, 10.0], result))

    def test_get_wksp_index_and_spec_num_with_specNum_axis_spectrum(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace when traversing the SPECTRUM axis

        This test provides a spectrum number and expects the workspace index to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_spectrum_number, res_kwargs = funcs._get_wksp_index_and_spec_num(ws, axis, specNum=3)
        self.assertEqual(2, res_workspace_index)
        self.assertEqual(3, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_with_specNum_axis_bin(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace when traversing the BIN axis

        This test provides a spectrum number and expects the workspace index to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_spectrum_number, res_kwargs = funcs._get_wksp_index_and_spec_num(ws, axis, specNum=3)
        self.assertEqual(2, res_workspace_index)
        self.assertEqual(3, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_throws_for_histo_with_one_bin_if_wkspIndex_and_specNum_not_set(self):
        ws = CreateSingleValuedWorkspace()
        axis = MantidAxType.SPECTRUM
        self.assertRaises(RuntimeError, funcs._get_wksp_index_and_spec_num, ws, axis)

    def test_get_wksp_index_and_spec_num_with_wkspIndex_axis_bin(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace, when traversing the BIN axis

        This test provides a workspace index, and expects the spectrum number to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_spectrum_number, res_kwargs = funcs._get_wksp_index_and_spec_num(ws, axis, wkspIndex=5)
        self.assertEqual(5, res_workspace_index)
        self.assertEqual(None, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_with_wkspIndex_axis_spectrum(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace, when traversing the SPECTRUM axis

        This test provides a workspace index, and expects the spectrum number to be correct
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM
        res_workspace_index, res_spectrum_number, res_kwargs = funcs._get_wksp_index_and_spec_num(ws, axis, wkspIndex=5)
        self.assertEqual(5, res_workspace_index)
        self.assertEqual(6, res_spectrum_number)

    def test_get_wksp_index_and_spec_num_error_with_both(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace

        This test checks that an error is shown when both a spectrum number and a workspace index are passed in
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM  # doesn't matter for this test
        self.assertRaises(RuntimeError, funcs._get_wksp_index_and_spec_num, ws, axis, wkspIndex=5, specNum=3)

    def test_get_wksp_index_and_spec_num_error_with_none(self):
        """
        Test getting the WorkspaceIndex and Spectrum Number for a Workspace

        This test checks that an error is shown when neither spectrum number nor workspace index is passed in
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.SPECTRUM  # doesn't matter for this test
        self.assertRaises(RuntimeError, funcs._get_wksp_index_and_spec_num, ws, axis)

    def test_get_wksp_index_dist_and_label_for_bins(self):
        """
        Tests that the workspace index, distribution and label are correctly retrieved.

        The label changes depending on the axis.
        """
        ws = CreateSampleWorkspace()
        axis = MantidAxType.BIN
        res_workspace_index, res_distribution, res_kwargs = funcs.get_wksp_index_dist_and_label(ws, axis, wkspIndex=1)

        self.assertEqual(1, res_workspace_index)
        self.assertEqual(False, res_distribution)
        self.assertEqual(res_kwargs["label"], "ws: bin 1")

        res_workspace_index, res_distribution, res_kwargs = funcs.get_wksp_index_dist_and_label(ws, axis, wkspIndex=0)

        self.assertEqual(0, res_workspace_index)
        self.assertEqual(False, res_distribution)
        self.assertEqual(res_kwargs["label"], "ws: bin 0")

    @add_md_workspace_with_data
    def test_get_md_data_no_error(self, mdws):
        dim_arrays, data, err = funcs.get_md_data(mdws, normalization=None)
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
        dim_arrays, data, err = funcs.get_md_data(mdws, normalization=None, withError=True)
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
        x, y, dy, dx = funcs.get_spectrum(ws, 3, normalize_by_bin_width=False, withDy=False, withDx=False)
        self.assertTrue(np.array_equal([13.5, 14.5, 15.5], x))
        self.assertTrue(np.array_equal([10.0, 11.0, 12.0], y))
        self.assertIsNone(dy)
        self.assertIsNone(dx)

    @add_workspace_with_data
    def test_get_spectrum_with_dy_dx(self, ws):
        x, y, dy, dx = funcs.get_spectrum(ws, 3, normalize_by_bin_width=False, withDy=True, withDx=True)

        self.assertTrue(np.array_equal([13.5, 14.5, 15.5], x))
        self.assertTrue(np.array_equal([10.0, 11.0, 12.0], y))
        self.assertTrue(np.array_equal([10.0, 11.0, 12.0], dy))
        self.assertTrue(np.array_equal([10.0, 11.0, 12.0], dx))

    @add_workspace_with_data
    def test_get_bins_no_dy(self, ws):
        x, y, dy, dx = funcs.get_bins(ws, 1, withDy=False)
        self.assertTrue(np.array_equal([0, 1, 2, 3], x))
        self.assertTrue(np.array_equal([2.0, 5.0, 8.0, 11.0], y))
        self.assertIsNone(dy)

    @add_workspace_with_data
    def test_get_bins_with_dy(self, ws):
        x, y, dy, dx = funcs.get_bins(ws, 1, withDy=True)
        self.assertTrue(np.array_equal([0, 1, 2, 3], x))
        self.assertTrue(np.array_equal([2.0, 5.0, 8.0, 11.0], y))
        self.assertTrue(np.array_equal([2.0, 5.0, 8.0, 11.0], dy))

    def test_get_bins_with_a_ragged_workspace(self):
        x, y, dy, dx = funcs.get_bins(self.ws2d_point_uneven, 3, withDy=True)

        self.assertTrue(np.array_equal([1], x))
        self.assertTrue(np.array_equal([4.0], y))
        self.assertTrue(np.array_equal([0.0], dy))

    @add_md_workspace_with_data(dimensions=3)
    def test_get_md_data2d_bin_bounds_raises_AssertionException_too_many_dims(self, mdws):
        self.assertRaises(AssertionError, funcs.get_md_data2d_bin_bounds, mdws, False)

    @add_md_workspace_with_data(dimensions=3)
    def test_get_md_data2d_bin_bounds_indices(self, mdws):
        x, y, z = funcs.get_md_data2d_bin_bounds(mdws, False, (0, slice(None), slice(None)))
        np.testing.assert_allclose(range(-10, 12, 2), x)
        np.testing.assert_allclose(range(-20, 24, 4), y)

    @add_md_workspace_with_data(dimensions=3)
    def test_get_md_data2d_bin_bounds_indices2(self, mdws):
        x, y, z = funcs.get_md_data2d_bin_bounds(mdws, False, (slice(None), 0, slice(None)))
        np.testing.assert_allclose(np.arange(-3, 3.6, 0.6), x, atol=1e-14)
        np.testing.assert_allclose(range(-20, 24, 4), y)

    def test_pointToIndex(self):
        d = self.ws_MD_2d.getDimension(0)
        self.assertEqual(funcs.pointToIndex(d, -10), 0)
        self.assertEqual(funcs.pointToIndex(d, -3), 0)
        self.assertEqual(funcs.pointToIndex(d, 0), 2)
        self.assertEqual(funcs.pointToIndex(d, 3), 4)
        self.assertEqual(funcs.pointToIndex(d, 10), 4)

    def test_get_indices(self):
        indices, kwargs = funcs.get_indices(self.ws_MD_2d)
        self.assertIsNone(indices)
        self.assertNotIn("label", kwargs)

        self.assertRaises(AssertionError, funcs.get_indices, self.ws_MD_2d, indices=(0, slice(None)))
        self.assertRaises(AssertionError, funcs.get_indices, self.ws_MD_2d, slicepoint=(0, None))
        self.assertRaises(ValueError, funcs.get_indices, self.ws_MD_2d, indices=(1, 2), slicepoint=(3, 4))

        indices, kwargs = funcs.get_indices(self.ws_MD_2d, indices=(1, slice(None), slice(None)))
        np.testing.assert_equal(indices, (1, slice(None), slice(None)))
        self.assertIn("label", kwargs)
        self.assertEqual(kwargs["label"], "ws_MD_2d: Dim1=-1.2")

        indices, kwargs = funcs.get_indices(self.ws_MD_2d, slicepoint=(-1, None, None))
        np.testing.assert_equal(indices, (1, slice(None), slice(None)))
        self.assertIn("label", kwargs)
        self.assertEqual(kwargs["label"], "ws_MD_2d: Dim1=-1.2")

    def _create_artist(self, errors=False):
        fig = figure()
        ax = fig.add_subplot(111)
        if errors:
            artist = ax.errorbar([0, 1], [0, 1], yerr=[0.1, 0.1])
        else:
            artist = ax.plot([0, 1], [0, 1])[0]
        return artist

    def test_errorbars_hidden_returns_true_for_non_errorbar_container_object(self):
        self.assertTrue(mantid.plots.datafunctions.errorbars_hidden(Mock()))

    def test_errorbars_hidden_returns_correctly_on_errorbar_container(self):
        container = self._create_artist(errors=True)
        self.assertFalse(mantid.plots.datafunctions.errorbars_hidden(container))
        [caps.set_visible(False) for caps in container[1] if container[1]]
        [bars.set_visible(False) for bars in container[2]]
        self.assertTrue(mantid.plots.datafunctions.errorbars_hidden(container))

    def test_errorbars_hidden_returns_true_on_container_with_invisible_connecting_line(self):
        container = self._create_artist(errors=True)
        container[0].set_visible(False)
        [caps.set_visible(False) for caps in container[1] if container[1]]
        [bars.set_visible(False) for bars in container[2]]
        self.assertTrue(mantid.plots.datafunctions.errorbars_hidden(container))

    def test_get_bin_indices_returns_a_range_with_no_monitors(self):
        ws = self.ws2d_histo
        bin_indices = funcs.get_bin_indices(ws)
        self.assertTrue(isinstance(bin_indices, range))

    def test_get_bin_indices_returns_a_numpy_ndarray_with_monitors(self):
        ConfigService.Instance().setString("default.facility", "ISIS")
        ws = LoadRaw("GEM40979", SpectrumMin=1, SpectrumMax=102)
        bin_indices = funcs.get_bin_indices(ws)
        self.assertTrue(isinstance(bin_indices, np.ndarray))
        ConfigService.Instance().setString("default.facility", " ")

    def test_errorbar_bounds_if_some_errors_nan(self):
        fig = figure()
        ax = fig.add_subplot(111)
        masked_errors = np.ma.masked_invalid([np.nan, 0.1])
        container = ax.errorbar([0, 1], [0, 1], xerr=masked_errors, yerr=masked_errors)
        mantid.plots.datafunctions.get_errorbar_bounds(container)

    def test_errorbar_bounds_if_all_errors_nan(self):
        fig = figure()
        ax = fig.add_subplot(111)
        masked_errors = np.ma.masked_invalid([np.nan, np.nan])
        container = ax.errorbar([0, 1], [0, 1], xerr=masked_errors, yerr=masked_errors)
        mantid.plots.datafunctions.get_errorbar_bounds(container)


if __name__ == "__main__":
    unittest.main()
