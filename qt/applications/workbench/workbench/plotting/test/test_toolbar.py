# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import patch

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.plotting import functions
from workbench.plotting.figuremanager import MantidFigureCanvas, FigureManagerWorkbench
from workbench.plotting.toolbar import WorkbenchNavigationToolbar
from mantid.plots.plotfunctions import plot
from mantid.simpleapi import CreateSampleWorkspace, CreateMDHistoWorkspace


@start_qapplication
class ToolBarTest(unittest.TestCase):
    """
    Test that the grids on/off toolbar button has the correct state when creating a plot in various different cases.
    """

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_waterfall_buttons_correctly_disabled_for_non_waterfall_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])

        self.assertFalse(self._is_button_enabled(fig, "waterfall_offset_amount"))
        self.assertFalse(self._is_button_enabled(fig, "waterfall_reverse_order"))
        self.assertFalse(self._is_button_enabled(fig, "waterfall_fill_area"))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_waterfall_buttons_correctly_enabled_for_waterfall_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        ws = CreateSampleWorkspace()
        plot([ws], wksp_indices=[0, 1], fig=fig, waterfall=True)

        self.assertTrue(self._is_button_enabled(fig, "waterfall_offset_amount"))
        self.assertTrue(self._is_button_enabled(fig, "waterfall_reverse_order"))
        self.assertTrue(self._is_button_enabled(fig, "waterfall_fill_area"))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_fitbrowser_and_superplot_disabled_for_MDHisto_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        # make workspace
        ws_histo = CreateMDHistoWorkspace(
            SignalInput="1,1",
            ErrorInput="1,1",
            Dimensionality=3,
            Extents="-1,1,-1,1,-1,1",
            NumberOfBins="2,1,1",
            Names="H,K,L",
            Units="rlu,rlu,rlu",
        )

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot(ws_histo)

        self.assertFalse(self._is_button_enabled(fig, "toggle_fit"))
        self.assertFalse(self._is_button_enabled(fig, "toggle_superplot"))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_fitbrowser_and_superplot_disabled_for_non_MantidAxes(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots()
        axes.plot([-10, 10], [1, 2])

        self.assertFalse(self._is_button_enabled(fig, "toggle_fit"))
        self.assertFalse(self._is_button_enabled(fig, "toggle_superplot"))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_line_color_selection_buttons_correctly_enabled_for_contour_wireframe_plots(self, mock_qappthread):
        """Checks that line_colour selection button is correctly enabled for wireframe and contour plots"""
        mock_qappthread.return_value = mock_qappthread

        plot_types = ["wireframe", "contour"]
        for plot_type in plot_types:
            ws = CreateSampleWorkspace()
            plot_function = getattr(functions, f"plot_{plot_type}", None)
            self.assertIsNotNone(plot_function)
            fig = plot_function([ws])

            # line_colour button should be enabled for contour and wireframe plots
            self.assertTrue(self._is_button_enabled(fig, "line_colour"))
            self.assertFalse(self._is_button_enabled(fig, "toggle_fit"))
            self.assertFalse(self._is_button_enabled(fig, "toggle_superplot"))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_plot_with_no_grid(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])
        # Grid button should be OFF because we have not enabled the grid.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_plot_with_grid(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])
        axes.grid()
        # Grid button should be ON because we enabled the grid.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_plot_with_grid_using_kwargs(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])
        # Set the grid on using kwargs in tick_params, like the plot script generator.
        axes.tick_params(axis="x", which="major", **{"gridOn": True})
        axes.tick_params(axis="y", which="major", **{"gridOn": True})

        # Grid button should be ON because we enabled the grid on both axes.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_plot_with_only_x_grid_using_kwargs(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])
        # Set the grid on using kwargs in tick_params, like the plot script generator.
        axes.tick_params(axis="x", which="major", **{"gridOn": True})

        # Grid button should be OFF because we only enabled the grid on one axis.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_tiled_plot_with_no_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={"projection": "mantid"})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        # None of the subplots have grids, so grid button should be toggled OFF.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_checked_for_tiled_plot_with_all_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={"projection": "mantid"})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
            ax.grid()
        # All subplots have grids, so button should be toggled ON.
        self.assertTrue(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_unchecked_for_tiled_plot_with_some_grids(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={"projection": "mantid"})
        for ax in fig.get_axes():
            ax.plot([-10, 10], [1, 2])
        # Only show major grid on 3/4 of the subplots.
        axes[0][0].grid()
        axes[0][1].grid()
        axes[1][0].grid()
        # Grid button should be OFF because not all subplots have grids.
        self.assertFalse(self._is_grid_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_plot_script_generator_is_enabled_for_non_square_tiled_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(ncols=2, nrows=2, subplot_kw={"projection": "mantid"})
        # Simulate a 2x2 grid showing three plots
        axes[0][0].plot([-10, 10], [1, 2])
        axes[0][1].plot([-10, 10], [1, 2])
        axes[1][0].plot([-10, 10], [1, 2])
        axes[1][1].axis("off")

        self.assertTrue(self._is_button_enabled(fig, "generate_plot_script"))

    def test_is_colorbar(self):
        """Verify the functionality of _is_colorbar, which determines whether a set of axes is a colorbar."""
        ws = CreateSampleWorkspace()
        fig = plt.figure()
        fig = functions.plot_surface([ws], fig=fig)
        axes = fig.get_axes()
        # First set of axes is the surface plot
        self.assertFalse(WorkbenchNavigationToolbar._is_colorbar(axes[0]))
        # Second set of axes is the colorbar
        self.assertTrue(WorkbenchNavigationToolbar._is_colorbar(axes[1]))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_grid_button_state_for_3d_plots(self, mock_qappthread):
        """Check that the gris on/off toolbar button is correctly checked or unchecked for different types of 3D plot"""
        plot_types = ["surface", "wireframe", "contour"]
        for plot_type in plot_types:
            # Check that the button state is correct when the grids are on and off.
            for is_grid in (True, False):
                self._test_grid_button_state_for_3d_plot(plot_type, is_grid, mock_qappthread)

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_hide_button_hides_window(self, mock_qappthread):
        """Check that the window is hidden when hide plot button is pressed"""
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])

        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        fig_manager.window.show()
        fig_manager.toolbar.hide_plot()

        self.assertTrue(fig_manager.window.isHidden())

    def _test_grid_button_state_for_3d_plot(self, plot_type, is_grid, mock_qappthread):
        """Check whether grid button check state is correct for a given plot type"""
        mock_qappthread.return_value = mock_qappthread
        ws = CreateSampleWorkspace()
        plot_function = getattr(functions, f"plot_{plot_type}", None)
        self.assertIsNotNone(plot_function)
        fig = plot_function([ws])
        ax = fig.get_axes()
        # Explicitly set grids on or off. In plots with a colour bar, ax[1] is the colour bar.
        ax[0].grid(is_grid)
        # Create a figure manager with a toolbar and check that the grid toggle button has the correct state.
        self.assertEqual(self._is_grid_button_checked(fig), is_grid, "Wrong grid button toggle state for " + plot_type + " plot")

    @classmethod
    def _is_grid_button_checked(cls, fig):
        """
        Create the figure manager and check whether its toolbar is toggled on or off for the given figure.
        We have to explicitly call set_button_visibility() here, which would otherwise be called within the show()
        function.
        """
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        return fig_manager.toolbar._actions["toggle_grid"].isChecked()

    @classmethod
    def _is_button_enabled(cls, fig, button):
        """
        Create the figure manager and check whether its toolbar is toggled on or off for the given figure.
        We have to explicitly call set_button_visibility() here, which would otherwise be called within the show()
        function.
        """
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        return fig_manager.toolbar._actions[button].isEnabled()


if __name__ == "__main__":
    unittest.main()
