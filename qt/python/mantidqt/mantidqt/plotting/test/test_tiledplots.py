# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
# std imports
from unittest import TestCase, main

# third party imports
import matplotlib

matplotlib.use("AGG")
import matplotlib.pyplot as plt
from matplotlib import _pylab_helpers

# local imports
# register mantid projection
import mantid.plots  # noqa
from mantid.api import AnalysisDataService, WorkspaceFactory
from unittest import mock
from mantidqt.dialogs.spectraselectordialog import SpectraSelection
from mantid.plots.plotfunctions import (
    add_axes_to_existing_mantid_tiled_figure,
    get_plot_fig,
    manage_workspace_names,
    stored_mantid_tiled_axis_position,
)
from mantidqt.plotting.functions import plot_from_names, share_tiled_axes


# Avoid importing the whole of mantid for a single mock of the workspace class
class FakeWorkspace(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


@manage_workspace_names
def workspace_names_dummy_func(workspaces):
    return workspaces


def set_figure_as_current_figure(initial_fig):
    """
    This creates an initial figure and then manually adds it to the figure manager singleton. This is required for
    functions such as gcf() to work when not plotting with a GUI backend
    """
    fig_manager = mock.MagicMock()
    fig_manager.canvas.figure = initial_fig
    _pylab_helpers.Gcf.figs.update({1: fig_manager})


class TiledPlotsTest(TestCase):
    _test_ws = None
    _test_ws_2 = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)
        if self._test_ws_2 is None:
            self.__class__._test_ws_2 = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)

        AnalysisDataService.addOrReplace("test_ws", self._test_ws)
        AnalysisDataService.addOrReplace("test_ws_2", self._test_ws_2)
        self._test_ws.getAxis(0).setUnit("TOF")
        self._test_ws_2.getAxis(0).setUnit("TOF")
        self._test_ws.getAxis(1).setUnit("Empty")
        self._test_ws_2.getAxis(1).setUnit("Empty")
        self._test_ws.setYUnit("Counts")
        self._test_ws_2.setYUnit("Counts")

        self.get_spectra_selection_patcher = mock.patch("mantidqt.plotting.functions.get_spectra_selection")
        self.addCleanup(self.get_spectra_selection_patcher.stop)
        self.get_spectra_selection_mock = self.get_spectra_selection_patcher.start()

    def tearDown(self):
        AnalysisDataService.Instance().clear()
        plt.close("all")

    def test_get_plot_fig_returns_same_figure_and_axes_if_figure_provided_and_overplot_is_true(self):
        initial_fig = plt.Figure()

        fig, axes = get_plot_fig(overplot=True, fig=initial_fig)

        self.assertEqual(fig, initial_fig)
        self.assertEqual(0, len(fig.axes))

    def test_get_plot_fig_return_same_figure_with_updated_axis_number(self):
        initial_fig = plt.Figure()

        fig, axes = get_plot_fig(overplot=False, fig=initial_fig, axes_num=5)

        self.assertEqual(fig, initial_fig)
        self.assertEqual(6, len(fig.axes))

    def test_get_plot_fig_with_no_figure_provided_and_overplot_is_true_returns_new_figure(self):
        initial_fig = plt.Figure()
        set_figure_as_current_figure(initial_fig)

        # If not figure is provided we provide a user a new figure plot, since this allows
        # overplotting in a loop, which requires there to be a figure with an associated axis
        fig, axes = get_plot_fig(overplot=True)

        self.assertNotEqual(fig, initial_fig)
        self.assertNotEqual(0, len(fig.axes))

    def test_no_figure_provided_and_overplot_is_false_returns_new_figure_with_correct_axes_number(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=5)

        self.assertEqual(6, len(fig.axes))

    def test_get_plot_fig_with_overplot_true_maintains_axes_scales(self):
        ax = plt.gca()
        ax.set_xscale("log")
        ax.set_yscale("log")

        fig, axes = get_plot_fig(overplot=True)

        self.assertEqual(axes[0].get_xscale(), "log")
        self.assertEqual(axes[0].get_yscale(), "log")

    def test_add_axes_to_existing_mantid_tiled_figure_preserves_mantid_tiled_axis_position(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)

        new_axes = add_axes_to_existing_mantid_tiled_figure(fig, axes_num=1)

        self.assertEqual(2, len(fig.axes))
        self.assertEqual(1, len(new_axes))
        self.assertEqual((0, 0), (axes[0]._mantid_tiled_row, axes[0]._mantid_tiled_col))
        self.assertEqual((0, 1), (new_axes[0]._mantid_tiled_row, new_axes[0]._mantid_tiled_col))

    def test_add_axes_to_existing_mantid_tiled_figure_requires_existing_mantid_tiled_axis_position(self):
        fig = plt.figure()
        fig.add_subplot(111, projection="mantid")

        with self.assertRaisesRegex(ValueError, "stored Mantid tile position"):
            add_axes_to_existing_mantid_tiled_figure(fig, axes_num=1)

    def test_stored_mantid_tiled_axis_position_returns_stored_position(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)

        self.assertEqual((0, 0), stored_mantid_tiled_axis_position(axes[0], nrows=1, ncols=1))

    def test_stored_mantid_tiled_axis_position_requires_stored_position(self):
        fig = plt.figure()
        axis = fig.add_subplot(111, projection="mantid")

        with self.assertRaisesRegex(ValueError, "stored Mantid tile position"):
            stored_mantid_tiled_axis_position(axis, nrows=1, ncols=1)

    def test_stored_mantid_tiled_axis_position_requires_position_inside_layout(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)

        with self.assertRaisesRegex(ValueError, "outside the new tiled layout"):
            stored_mantid_tiled_axis_position(axes[0], nrows=0, ncols=1)

    def test_tiled_plot_from_multiple_workspaces_no_errors(self):
        workspaces = ["test_ws", "test_ws_2"]

        fig = self._create_plot(names=workspaces, errors=False, overplot=False)

        self.assertEqual(len(fig.axes), 2)
        self.assertEqual(list(fig.axes[0].tracked_workspaces.keys()), ["test_ws"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[0].tracked_workspaces["test_ws"]], [0])
        self.assertEqual(list(fig.axes[1].tracked_workspaces.keys()), ["test_ws_2"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[1].tracked_workspaces["test_ws_2"]], [0])

    def test_tiled_plot_from_multiple_workspaces_with_errors(self):
        workspaces = ["test_ws", "test_ws_2"]

        fig = self._create_plot(names=workspaces, errors=True, overplot=False)

        self.assertEqual(len(fig.axes), 2)
        self.assertEqual(list(fig.axes[0].tracked_workspaces.keys()), ["test_ws"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[0].tracked_workspaces["test_ws"]], [0])
        self.assertEqual(list(fig.axes[1].tracked_workspaces.keys()), ["test_ws_2"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[1].tracked_workspaces["test_ws_2"]], [0])

    def test_overplotting_onto_a_tiled_plot(self):
        workspaces = ["test_ws", "test_ws_2"]
        fig = self._create_plot(names=workspaces, errors=True, overplot=False)

        fig = self._create_plot(
            names=["test_ws"], errors=True, overplot=fig.axes[1], wksp_indices=[0, 1], plot_type=SpectraSelection.Individual, fig=fig
        )

        self.assertEqual(len(fig.axes), 2)
        self.assertEqual(list(fig.axes[0].tracked_workspaces.keys()), ["test_ws"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[0].tracked_workspaces["test_ws"]], [0])
        self.assertEqual(list(fig.axes[1].tracked_workspaces.keys()).sort(), ["test_ws", "test_ws_2"].sort())
        self.assertEqual([artist.workspace_index for artist in fig.axes[1].tracked_workspaces["test_ws_2"]], [0])
        self.assertEqual([artist.workspace_index for artist in fig.axes[1].tracked_workspaces["test_ws"]], [0, 1])

    def test_plotting_tiled_plot_with_multiple_workspaces_and_multiple_indices(self):
        workspaces = ["test_ws", "test_ws_2"]

        fig = self._create_plot(names=workspaces, errors=True, overplot=False, wksp_indices=[0, 1])

        self.assertEqual(len(fig.axes), 4)
        self.assertEqual(list(fig.axes[0].tracked_workspaces.keys()), ["test_ws"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[0].tracked_workspaces["test_ws"]], [0])
        self.assertEqual(list(fig.axes[2].tracked_workspaces.keys()), ["test_ws_2"])
        self.assertEqual([artist.workspace_index for artist in fig.axes[2].tracked_workspaces["test_ws_2"]], [0])

    def test_share_tiled_axes_shares_both_axes_across_tiles_and_preserves_combined_limits(self):
        fig = self._create_plot(names=["test_ws", "test_ws_2"], errors=False, overplot=False)
        fig.axes[0].set_xlim(1, 2)
        fig.axes[1].set_xlim(10, 20)
        fig.axes[0].set_ylim(1, 2)
        fig.axes[1].set_ylim(10, 20)

        share_tiled_axes(fig)

        self.assertTrue(fig.axes[0].get_shared_y_axes().joined(fig.axes[0], fig.axes[1]))
        self.assertTrue(fig.axes[0].get_shared_x_axes().joined(fig.axes[0], fig.axes[1]))
        self.assertEqual((1, 20), fig.axes[0].get_xlim())
        self.assertEqual((1, 20), fig.axes[0].get_ylim())

    def test_share_tiled_axes_requires_matching_units(self):
        self._test_ws_2.getAxis(0).setUnit("Wavelength")
        x_mismatch_figure = self._create_plot(names=["test_ws", "test_ws_2"], errors=False, overplot=False)

        share_tiled_axes(x_mismatch_figure)

        self.assertFalse(x_mismatch_figure.axes[0].get_shared_x_axes().joined(x_mismatch_figure.axes[0], x_mismatch_figure.axes[1]))
        self.assertTrue(x_mismatch_figure.axes[0].get_shared_y_axes().joined(x_mismatch_figure.axes[0], x_mismatch_figure.axes[1]))

        self._test_ws_2.getAxis(0).setUnit("TOF")
        self._test_ws_2.setYUnit("Events")
        y_mismatch_figure = self._create_plot(names=["test_ws", "test_ws_2"], errors=False, overplot=False)

        share_tiled_axes(y_mismatch_figure)

        self.assertTrue(y_mismatch_figure.axes[0].get_shared_x_axes().joined(y_mismatch_figure.axes[0], y_mismatch_figure.axes[1]))
        self.assertFalse(y_mismatch_figure.axes[0].get_shared_y_axes().joined(y_mismatch_figure.axes[0], y_mismatch_figure.axes[1]))

    def test_share_tiled_axes_makes_new_axis_inherit_existing_scale_and_limits(self):
        fig = self._create_plot(names=["test_ws", "test_ws_2"], errors=False, overplot=False)
        fig.axes[0].set_ylim(2, 8)
        fig.axes[1].set_yscale("log")
        fig.axes[1].set_ylim(0.1, 100)

        share_tiled_axes(fig, new_axes=[fig.axes[1]])

        self.assertTrue(fig.axes[0].get_shared_y_axes().joined(fig.axes[0], fig.axes[1]))
        self.assertEqual("linear", fig.axes[1].get_yscale())
        self.assertEqual((2, 8), fig.axes[1].get_ylim())

    def test_share_tiled_axes_preserves_existing_limits_across_multiple_new_axes(self):
        third_workspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)
        third_workspace.getAxis(0).setUnit("TOF")
        third_workspace.setYUnit("Counts")
        AnalysisDataService.addOrReplace("test_ws_3", third_workspace)
        fig = self._create_plot(names=["test_ws", "test_ws_2", "test_ws_3"], errors=False, overplot=False)
        fig.axes[0].set_ylim(2, 8)
        fig.axes[1].set_ylim(10, 20)
        fig.axes[2].set_ylim(30, 40)

        share_tiled_axes(fig, new_axes=fig.axes[1:3])

        self.assertEqual((2, 8), fig.axes[2].get_ylim())

    def _create_plot(self, names, errors, overplot, wksp_indices=None, plot_type=SpectraSelection.Tiled, fig=None):
        selection = SpectraSelection(names)
        selection.wksp_indices = wksp_indices or [0]
        selection.plot_type = plot_type
        self.get_spectra_selection_mock.return_value = selection

        return plot_from_names(names=names, errors=errors, overplot=overplot, fig=fig)


if __name__ == "__main__":
    main()
