# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib
import unittest

matplotlib.use("AGG")

import matplotlib.pyplot as plt
import numpy as np

from unittest import mock
import mantid.api
import mantid.plots.axesfunctions as funcs
from mantid.plots.utility import MantidAxType
from mantid.kernel import config
from mantid.simpleapi import (
    CreateWorkspace,
    CreateEmptyTableWorkspace,
    DeleteWorkspace,
    CreateMDHistoWorkspace,
    ConjoinWorkspaces,
    AddTimeSeriesLog,
    CloneWorkspace,
)


class PlotFunctionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.g1da = config["graph1d.autodistribution"]
        config["graph1d.autodistribution"] = "On"
        cls.ws2d_histo = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            Distribution=True,
            YUnitLabel="Counts per $\\AA$",
            UnitX="Wavelength",
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[4, 6, 8],
            OutputWorkspace="ws2d_histo",
        )
        cls.ws2d_histo_negative_errors = CreateWorkspace(
            DataX=[1, 2, 3, 4], DataY=[10, 20, 30, 40], DataE=[1, -2, 3, -4], OutputWorkspace="ws2d_histo_negative_errors"
        )
        cls.ws2d_histo_non_dist = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5],
            DataE=[1, 2, 3, 4],
            NSpec=2,
            Distribution=False,
            YUnitLabel="Counts",
            UnitX="Wavelength",
            OutputWorkspace="ws2d_histo_non_dist",
        )
        cls.ws2d_histo_rag = CreateWorkspace(
            DataX=[1, 2, 3, 4, 5, 2, 4, 6, 8, 10],
            DataY=[2] * 8,
            NSpec=2,
            VerticalAxisUnit="DeltaE",
            VerticalAxisValues=[5, 7, 9],
            OutputWorkspace="ws2d_histo_rag",
        )
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
        cls.ws_spec = CreateWorkspace(
            DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
            DataY=[2, 3, 4, 5, 2, 3],
            DataE=[1, 2, 3, 4, 1, 2],
            NSpec=3,
            OutputWorkspace="ws_spec",
        )
        wp = CreateWorkspace(DataX=[15, 25, 35, 45], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(cls.ws2d_point_uneven, wp, CheckOverlapping=False, CheckMatchingBins=False)
        cls.ws2d_point_uneven = mantid.mtd["ws2d_point_uneven"]
        cls.ws2d_histo_uneven = CreateWorkspace(DataX=[10, 20, 30, 40], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="ws2d_histo_uneven")
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:30:00", Value=15)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:50:00", Value=100.2)

    @classmethod
    def tearDownClass(cls):
        config["graph1d.autodistribution"] = cls.g1da
        DeleteWorkspace("ws2d_histo")
        DeleteWorkspace("ws2d_histo_non_dist")
        DeleteWorkspace("ws_MD_2d")
        DeleteWorkspace("ws_MD_1d")
        DeleteWorkspace("ws2d_point_uneven")

    def test_1d_plots(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, "rs", specNum=1)
        funcs.plot(ax, self.ws2d_histo, specNum=2, linewidth=6)
        funcs.plot(ax, self.ws_MD_1d, "bo")

    def test_1d_bin(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, "rs", specNum=1, axis=MantidAxType.BIN)

    def test_1d_bin_numeric_axis(self):
        fig, ax = plt.subplots()
        x_axis = mantid.api.NumericAxis.create(2)
        x_axis.setValue(0, 3.0)
        x_axis.setValue(1, 5.0)
        ws_local = CloneWorkspace(self.ws2d_histo, StoreInADS=False)
        ws_local.replaceAxis(1, x_axis)
        funcs.plot(ax, ws_local, "rs", specNum=1, axis=MantidAxType.BIN)

    def test_1d_bin_binedge_axis(self):
        fig, ax = plt.subplots()
        x_axis = mantid.api.BinEdgeAxis.create(3)
        x_axis.setValue(0, 3.0)
        x_axis.setValue(1, 5.0)
        x_axis.setValue(2, 7.0)
        ws_local = CloneWorkspace(self.ws2d_histo, StoreInADS=False)
        ws_local.replaceAxis(1, x_axis)
        funcs.plot(ax, ws_local, "rs", specNum=1, axis=MantidAxType.BIN)

    def test_1d_log(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, LogName="my_log")
        ax1 = ax.twiny()
        funcs.plot(ax1, self.ws2d_histo, LogName="my_log", FullTime=True)

    def test_1d_errorbars(self):
        fig, ax = plt.subplots()
        funcs.errorbar(ax, self.ws2d_histo, "rs", specNum=1)
        funcs.errorbar(ax, self.ws2d_histo, specNum=2, linewidth=6)
        funcs.errorbar(ax, self.ws_MD_1d, "bo")

    def test_1d_errorbars_with_negative_errors(self):
        _, ax = plt.subplots()
        funcs.errorbar(ax, self.ws2d_histo_negative_errors, specNum=1)

    def test_1d_scatter(self):
        fig, ax = plt.subplots()
        funcs.scatter(ax, self.ws2d_histo, specNum=1)
        funcs.scatter(ax, self.ws2d_histo, specNum=2)
        funcs.scatter(ax, self.ws_MD_1d)

    def test_2d_contours(self):
        fig, ax = plt.subplots()
        funcs.contour(ax, self.ws2d_histo_rag)
        funcs.contourf(ax, self.ws2d_histo, vmin=0)
        funcs.tricontour(ax, self.ws_MD_2d)
        funcs.tricontourf(ax, self.ws_MD_2d)
        self.assertRaises(ValueError, funcs.contour, ax, self.ws2d_point_uneven)

    def test_2d_pcolors(self):
        fig, ax = plt.subplots()
        funcs.pcolor(ax, self.ws2d_histo_rag)
        funcs.tripcolor(ax, self.ws2d_histo, vmin=0)
        funcs.pcolormesh(ax, self.ws_MD_2d)
        funcs.pcolorfast(ax, self.ws2d_point_uneven, vmin=-1)
        funcs.imshow(ax, self.ws2d_histo)

    def test_imshow_works_with_a_ragged_workspace(self):
        fig, ax = plt.subplots()
        funcs.imshow(ax, self.ws2d_point_uneven)

    def _do_update_colorplot_datalimits(self, color_func):
        fig, ax = plt.subplots()
        mesh = color_func(ax, self.ws2d_histo)
        ax.set_xlim(0.01, 0.05)
        ax.set_ylim(-0.05, 0.05)
        funcs.update_colorplot_datalimits(ax, mesh)
        self.assertAlmostEqual(10.0, ax.get_xlim()[0])

        self.assertAlmostEqual(30.0, ax.get_xlim()[1])
        self.assertAlmostEqual(4.0, ax.get_ylim()[0])
        self.assertAlmostEqual(8.0, ax.get_ylim()[1])

    def test_update_colorplot_datalimits_for_pcolormesh(self):
        self._do_update_colorplot_datalimits(funcs.pcolormesh)

    def test_update_colorplot_datalimits_for_pcolor(self):
        self._do_update_colorplot_datalimits(funcs.pcolor)

    def test_update_colorplot_datalimits_for_imshow(self):
        self._do_update_colorplot_datalimits(funcs.imshow)

    def test_update_colorplot_datalimits_for_x_axis(self):
        """Check that the function is only operating on the x-axis"""
        ax_mock = mock.MagicMock()
        funcs.update_colorplot_datalimits(ax_mock, ax_mock.images, "x")
        ax_mock.update_datalim.assert_called_once_with(mock.ANY, True, False)
        ax_mock.autoscale.assert_called_once_with(axis="x")

    def test_update_colorplot_datalimits_for_y_axis(self):
        """Check that the function is only operating on y-axis"""
        ax_mock = mock.MagicMock()
        funcs.update_colorplot_datalimits(ax_mock, ax_mock.images, "y")
        ax_mock.update_datalim.assert_called_once_with(mock.ANY, False, True)
        ax_mock.autoscale.assert_called_once_with(axis="y")

    def test_update_colorplot_datalimits_for_both_axis(self):
        """Check that the function is only operating on y-axis"""
        ax_mock = mock.MagicMock()
        funcs.update_colorplot_datalimits(ax_mock, ax_mock.images, "both")
        ax_mock.update_datalim.assert_called_once_with(mock.ANY, True, True)
        ax_mock.autoscale.assert_called_once_with(axis="both")

    def test_1d_plots_with_unplottable_type_raises_attributeerror(self):
        table = CreateEmptyTableWorkspace()
        _, ax = plt.subplots()
        self.assertRaises(AttributeError, funcs.plot, ax, table, wkspIndex=0)
        self.assertRaises(AttributeError, funcs.errorbar, ax, table, wkspIndex=0)

    def test_2d_plots_with_unplottable_type_raises_attributeerror(self):
        table = CreateEmptyTableWorkspace()
        _, ax = plt.subplots()
        self.assertRaises(AttributeError, funcs.pcolor, ax, table)
        self.assertRaises(AttributeError, funcs.pcolormesh, ax, table)
        self.assertRaises(AttributeError, funcs.pcolorfast, ax, table)

    def test_1d_indices(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws_MD_2d, indices=(slice(None), 0, 0))
        funcs.plot(ax, self.ws_MD_2d, indices=(0, slice(None), 0))
        funcs.plot(ax, self.ws_MD_2d, indices=(0, 0, slice(None)))
        self.assertRaises(AssertionError, funcs.plot, ax, self.ws_MD_2d, indices=(0, slice(None), slice(None)))
        self.assertRaises(AssertionError, funcs.plot, ax, self.ws_MD_2d)

    def test_1d_slicepoint(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws_MD_2d, slicepoint=(None, 0, 0))
        funcs.plot(ax, self.ws_MD_2d, slicepoint=(0, None, 0))
        funcs.plot(ax, self.ws_MD_2d, slicepoint=(0, 0, None))
        self.assertRaises(AssertionError, funcs.plot, ax, self.ws_MD_2d, slicepoint=(0, None, None))
        self.assertRaises(AssertionError, funcs.plot, ax, self.ws_MD_2d)

    def test_1d_x_axes_label_spectrum_plot(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo_non_dist, "rs", specNum=1, axis=MantidAxType.SPECTRUM)
        self.assertEqual(ax.get_xlabel(), "Wavelength ($\\AA$)")

    def test_1d_x_axes_label_bin_plot(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo_non_dist, "rs", specNum=1, axis=MantidAxType.BIN)
        self.assertEqual(ax.get_xlabel(), "Spectrum")

    def test_1d_x_axes_label_numeric_axis_bin_plot(self):
        fig, ax = plt.subplots()
        x_axis = mantid.api.NumericAxis.create(2)
        x_axis.setValue(0, 3.0)
        x_axis.setValue(1, 5.0)
        ws_local = CloneWorkspace(self.ws2d_histo_non_dist, StoreInADS=False)
        ws_local.replaceAxis(1, x_axis)
        funcs.plot(ax, ws_local, "rs", specNum=1, axis=MantidAxType.BIN)
        self.assertEqual(ax.get_xlabel(), "")

    def test_1d_x_axes_label_numeric_axis_with_unit_bin_plot(self):
        fig, ax = plt.subplots()
        x_axis = mantid.api.NumericAxis.create(2)
        x_axis.setValue(0, 3.0)
        x_axis.setValue(1, 5.0)
        x_axis.setUnit("MomentumTransfer")
        ws_local = CloneWorkspace(self.ws2d_histo_non_dist, StoreInADS=False)
        ws_local.replaceAxis(1, x_axis)
        funcs.plot(ax, ws_local, "rs", specNum=1, axis=MantidAxType.BIN)
        self.assertEqual(ax.get_xlabel(), "q ($\\AA^{-1}$)")

    def test_1d_y_axes_label_auto_distribution_on(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo_non_dist, "rs", specNum=1)
        self.assertEqual(ax.get_ylabel(), "Counts ($\\AA$)$^{-1}$")

    def test_1d_y_axes_label_distribution_workspace_auto_distribution_on(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, "rs", specNum=1)
        self.assertEqual(ax.get_ylabel(), "Counts ($\\AA$)$^{-1}$")

    def test_1d_y_axes_label_auto_distribution_off(self):
        try:
            config["graph1d.autodistribution"] = "Off"
            fig, ax = plt.subplots()
            funcs.plot(ax, self.ws2d_histo_non_dist, "rs", specNum=1)
            self.assertEqual(ax.get_ylabel(), "Counts")
        finally:
            config["graph1d.autodistribution"] = "On"

    def test_1d_y_axes_label_distribution_workspace_auto_distribution_off(self):
        try:
            config["graph1d.autodistribution"] = "Off"
            fig, ax = plt.subplots()
            funcs.plot(ax, self.ws2d_histo, "rs", specNum=1)
            self.assertEqual(ax.get_ylabel(), "Counts ($\\AA$)$^{-1}$")
        finally:
            config["graph1d.autodistribution"] = "On"

    def test_get_data_for_plot_binplot_binedgeaxis(self):
        fig, ax = plt.subplots()
        kwargs = {"wkspIndex": 0, "axis": MantidAxType.BIN}
        x, y, dy, dx, indices, axis, kwargs = funcs._get_data_for_plot(ax, self.ws2d_histo, kwargs)
        self.assertTrue(np.array_equal([5.0, 7.0], x))
        self.assertTrue(np.array_equal([2.0, 4.0], y))

    def test_get_data_for_plot_binplot_spectrumaxis(self):
        fig, ax = plt.subplots()
        kwargs = {"wkspIndex": 0, "axis": MantidAxType.BIN}
        x, y, dy, dx, indices, axis, kwargs = funcs._get_data_for_plot(ax, self.ws_spec, kwargs)
        self.assertTrue(np.array_equal([1, 2, 3], x))
        self.assertTrue(np.array_equal([2.0, 4.0, 2.0], y))


if __name__ == "__main__":
    unittest.main()
