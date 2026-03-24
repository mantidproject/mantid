# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from unittest.mock import patch
import numpy as np

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.plotting import functions
from workbench.plotting.figuremanager import MantidFigureCanvas, FigureManagerWorkbench
from workbench.plotting.toolbar import WorkbenchNavigationToolbar
from mantid.plots.plotfunctions import plot
from mantid.simpleapi import CreateSampleWorkspace, CreateMDHistoWorkspace, Load, mtd, LoadSampleShape, CreateWorkspace
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import mantid.simpleapi as msa
from unittest.mock import MagicMock, PropertyMock
from mantid.plots import MantidAxes
from matplotlib.colors import LogNorm


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
    def test_button_checked_for_plot_with_no_crosshair(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.plot([-10, 10], [1, 2])
        # crosshair button should be OFF because we have not enabled the crosshair.
        self.assertFalse(self._is_crosshair_button_checked(fig))
        # crosshair button should be visible when there is only 1 axis
        self.assertTrue(self._is_crosshair_button_visible(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_for_tiled_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots(2)
        axes[0].plot([-10, 10], [1, 2])
        axes[1].plot([3, 2, 1], [1, 2, 3])
        # crosshair button should be hidden because this is a tiled plot
        self.assertTrue(self._is_crosshair_button_visible(fig))
        self.assertFalse(self._is_crosshair_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_enabled_for_contour_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        data = Load("SANSLOQCan2D.nxs")
        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        axes.imshow(data, origin="lower", cmap="viridis", aspect="auto")
        axes.contour(data, levels=np.linspace(10, 60, 6), colors="yellow", alpha=0.5)
        # crosshair button should be visible because this is a contour plot
        self.assertTrue(self._is_crosshair_button_visible(fig))
        self.assertFalse(self._is_crosshair_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_hidden_for_3d_surface_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        data = Load("MUSR00015189.nxs")
        data = mtd["data_1"]  # Extract individual workspace from group
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})
        ax.plot_surface(data, cmap="viridis")
        # crosshair button should be hidden
        self.assertFalse(self._is_crosshair_button_visible(fig))
        self.assertFalse(self._is_crosshair_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_hidden_for_3d_wireframe_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        msa.config.setFacility("SNS")
        data = Load("MAR11060.nxs")
        fig, ax = plt.subplots(subplot_kw={"projection": "mantid3d"})
        ax.plot_wireframe(data)
        # crosshair button should be hidden
        self.assertFalse(self._is_crosshair_button_visible(fig))
        self.assertFalse(self._is_crosshair_button_checked(fig))

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_button_hidden_for_3d_mesh_plots(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread
        ws = CreateSampleWorkspace()
        ws = LoadSampleShape(ws, "tube.stl")
        sample = ws.sample()
        shape = sample.getShape()
        mesh = shape.getMesh()
        mesh_polygon = Poly3DCollection(mesh, facecolors=["g"], edgecolors=["b"], alpha=0.5, linewidths=0.1)
        fig, axes = plt.subplots(subplot_kw={"projection": "mantid3d"})
        axes.add_collection3d(mesh_polygon)
        axes.set_mesh_axes_equal(mesh)

        # crosshair button should be hidden
        self.assertFalse(self._is_crosshair_button_visible(fig))
        self.assertFalse(self._is_crosshair_button_checked(fig))

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

    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_back_and_forward_button_state(self, mock_qappthread):
        mock_qappthread.return_value = mock_qappthread

        fig, axes = plt.subplots()
        axes.plot([-10, 10], [1, 2])

        self.assertFalse(self._is_button_enabled(fig, "on_back_clicked"))
        self.assertFalse(self._is_button_enabled(fig, "on_forward_clicked"))

    @patch("matplotlib.backend_bases.NavigationToolbar2.back")
    @patch("matplotlib.backend_bases.NavigationToolbar2.forward")
    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_back_navigating_mpl_stack_on_fresh_plot(self, mock_qappthread, mock_forward, mock_back):
        mock_qappthread.return_value = mock_qappthread
        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        ws = CreateWorkspace(DataX="1,2,3,4,5", DataY="-4,1,-2,0,0", StoreInADS=True)
        cfill = axes.pcolormesh(ws)
        cfill.set_norm(LogNorm(vmin=0.0001, vmax=1.0))
        fig.colorbar(cfill, ax=[axes])

        fig_manager = self._press_key(fig, "c")
        self.assertFalse(fig_manager.toolbar._actions["on_back_clicked"].isEnabled())
        self.assertFalse(fig_manager.toolbar._actions["on_forward_clicked"].isEnabled())
        mock_back.assert_called_once()
        mock_forward.assert_not_called()

    @patch("matplotlib.backend_bases.NavigationToolbar2.back")
    @patch("matplotlib.backend_bases.NavigationToolbar2.forward")
    @patch("workbench.plotting.figuremanager.QAppThreadCall")
    def test_forward_navigating_mpl_stack_on_fresh_plot(self, mock_qappthread, mock_forward, mock_back):
        mock_qappthread.return_value = mock_qappthread
        fig, axes = plt.subplots(subplot_kw={"projection": "mantid"})
        ws = CreateWorkspace(DataX="1,2,3,4,5", DataY="-4,1,-2,0,0", StoreInADS=True)
        cfill = axes.pcolormesh(ws)
        cfill.set_norm(LogNorm(vmin=0.0001, vmax=1.0))
        fig.colorbar(cfill, ax=[axes])

        fig_manager = self._press_key(fig, "v")
        self.assertFalse(fig_manager.toolbar._actions["on_back_clicked"].isEnabled())
        self.assertFalse(fig_manager.toolbar._actions["on_forward_clicked"].isEnabled())
        mock_back.assert_not_called()
        mock_forward.assert_called_once()

    @classmethod
    def _press_key(cls, fig, key):
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        fig_manager.toolbar.set_buttons_visibility(fig)

        key_press_event = MagicMock(inaxes=MagicMock(spec=MantidAxes, collections=[], creation_args=[{}]))
        type(key_press_event).key = PropertyMock(return_value=key)
        key_press_event.inaxes = MagicMock()
        key_press_event.inaxes.get_lines.return_value = ["fake_line"]
        fig_manager._fig_interaction.on_key_press(key_press_event)
        return fig_manager

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

    @classmethod
    def _is_crosshair_button_checked(cls, fig):
        """
        Create the figure manager and check whether its toolbar is toggled on or off for the given figure.
        We have to explicitly call set_button_visibility() here, which would otherwise be called within the show()
        function.
        """
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        return fig_manager.toolbar._actions["toggle_crosshair"].isChecked()

    @classmethod
    def _is_crosshair_button_visible(cls, fig):
        """
        Create the figure manager and check whether its toolbar is visible for the given figure.
        We have to explicitly call set_button_visibility() here, which would otherwise be called within the show()
        function.
        """
        canvas = MantidFigureCanvas(fig)
        fig_manager = FigureManagerWorkbench(canvas, 1)
        # This is only called when show() is called on the figure manager, so we have to manually call it here.
        fig_manager.toolbar.set_buttons_visibility(fig)
        return fig_manager.toolbar._actions["toggle_crosshair"].isVisible()


if __name__ == "__main__":
    unittest.main()
