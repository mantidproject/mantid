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
from mantid.plots.plotfunctions import add_tiled_axes, create_subplots, manage_workspace_names, get_plot_fig, plot_on_axis
from mantidqt.plotting.functions import plot_from_names


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

    def test_add_tiled_axes_preserves_existing_axes_and_adds_new_axes(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=2)

        new_axes = add_tiled_axes(fig, axes_num=2)

        self.assertEqual(4, len(fig.axes))
        self.assertEqual(2, len(new_axes))
        self.assertEqual(list(axes), fig.axes[:2])
        self.assertEqual(new_axes, fig.axes[2:])

    def test_add_tiled_axes_does_not_reset_layout_engine_if_figure_has_colorbar(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)
        image = axes[0].imshow([[0, 1], [1, 0]])
        fig.colorbar(image, ax=axes[0])

        new_axes = add_tiled_axes(fig, axes_num=1)

        self.assertEqual(3, len(fig.axes))
        self.assertEqual(1, len(new_axes))

    def test_add_tiled_axes_with_colorbar_figure_draws_after_adding_line_axis(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)
        image = axes[0].imshow([[0, 1], [1, 0]])
        fig.colorbar(image, ax=axes[0])

        add_tiled_axes(fig, axes_num=1)

        fig.canvas.draw()

    def test_add_tiled_axes_repeatedly_uses_unique_positions_up_to_four_axes(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)

        add_tiled_axes(fig, axes_num=1)
        add_tiled_axes(fig, axes_num=1, vertical=True)
        add_tiled_axes(fig, axes_num=1)

        plot_axes = [ax for ax in fig.axes if ax.get_label() != "<colorbar>"]
        positions = {(ax._mantid_tiled_row, ax._mantid_tiled_col) for ax in plot_axes}
        self.assertEqual(4, len(plot_axes))
        self.assertEqual(positions, {(0, 0), (0, 1), (1, 0), (1, 1)})
        self.assertEqual((axes[0]._mantid_tiled_row, axes[0]._mantid_tiled_col), (0, 0))

    def test_add_tiled_axes_repeatedly_with_colorbars_uses_unique_positions_up_to_four_axes(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)
        image = axes[0].imshow([[0, 1], [1, 0]])
        fig.colorbar(image, ax=axes[0])

        add_tiled_axes(fig, axes_num=1)
        add_tiled_axes(fig, axes_num=1, vertical=True)
        add_tiled_axes(fig, axes_num=1)

        plot_axes = [ax for ax in fig.axes if ax.get_label() != "<colorbar>"]
        positions = {(ax._mantid_tiled_row, ax._mantid_tiled_col) for ax in plot_axes}
        self.assertEqual(4, len(plot_axes))
        self.assertEqual(positions, {(0, 0), (0, 1), (1, 0), (1, 1)})
        fig.canvas.draw()

    def test_labelled_colorbar_is_ignored_when_adding_tiled_axes(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)
        image = axes[0].imshow([[0, 1], [1, 0]])
        colorbar = fig.colorbar(image, ax=axes[0])
        colorbar.ax.set_label("<colorbar>")

        new_axes = add_tiled_axes(fig, axes_num=1)

        self.assertEqual(fig.axes[1].get_label(), "<colorbar>")
        self.assertEqual((fig.axes[0]._mantid_tiled_row, fig.axes[0]._mantid_tiled_col), (0, 0))
        self.assertEqual((new_axes[0]._mantid_tiled_row, new_axes[0]._mantid_tiled_col), (0, 1))

    def test_plot_on_axis_only_plots_on_supplied_axis(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=2)

        plot_on_axis(["test_ws"], axes[1], wksp_indices=[0])

        self.assertEqual(0, len(axes[0].get_lines()))
        self.assertEqual(1, len(axes[1].get_lines()))
        self.assertEqual(2, len(fig.axes))

    def test_add_tiled_axes_vertical_adds_new_axes_to_free_positions_first(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=2)

        new_axes = add_tiled_axes(fig, axes_num=2, vertical=True)

        self.assertEqual(4, len(fig.axes))
        self.assertEqual(2, len(new_axes))
        self.assertEqual(new_axes, fig.axes[2:])
        self.assertEqual(list(axes), fig.axes[:2])
        self.assertEqual(axes[0].get_subplotspec().rowspan.start, 0)
        self.assertEqual(axes[0].get_subplotspec().colspan.start, 0)
        self.assertEqual(axes[1].get_subplotspec().rowspan.start, 0)
        self.assertEqual(axes[1].get_subplotspec().colspan.start, 1)
        self.assertEqual(new_axes[0].get_subplotspec().rowspan.start, 1)
        self.assertEqual(new_axes[0].get_subplotspec().colspan.start, 0)
        self.assertEqual(new_axes[1].get_subplotspec().rowspan.start, 1)
        self.assertEqual(new_axes[1].get_subplotspec().colspan.start, 1)

    def test_add_tiled_axes_vertical_after_horizontal_add_uses_free_position_below_first_axis(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)
        horizontal_axes = add_tiled_axes(fig, axes_num=1)

        vertical_axes = add_tiled_axes(fig, axes_num=1, vertical=True)

        self.assertEqual(3, len(fig.axes))
        self.assertEqual(axes[0].get_subplotspec().rowspan.start, 0)
        self.assertEqual(axes[0].get_subplotspec().colspan.start, 0)
        self.assertEqual(horizontal_axes[0].get_subplotspec().rowspan.start, 0)
        self.assertEqual(horizontal_axes[0].get_subplotspec().colspan.start, 1)
        self.assertEqual(vertical_axes[0].get_subplotspec().rowspan.start, 1)
        self.assertEqual(vertical_axes[0].get_subplotspec().colspan.start, 0)

    def test_add_tiled_axes_vertical_adds_second_axis_below_existing_axis(self):
        fig, axes = get_plot_fig(overplot=False, axes_num=1)

        new_axes = add_tiled_axes(fig, axes_num=1, vertical=True)

        self.assertEqual(2, len(fig.axes))
        self.assertEqual(1, len(new_axes))
        self.assertEqual(list(axes), fig.axes[:1])
        self.assertEqual(new_axes, fig.axes[1:])
        self.assertEqual(axes[0].get_subplotspec().rowspan.start, 0)
        self.assertEqual(axes[0].get_subplotspec().colspan.start, 0)
        self.assertEqual(new_axes[0].get_subplotspec().rowspan.start, 1)
        self.assertEqual(new_axes[0].get_subplotspec().colspan.start, 0)

    def test_create_subplots_vertical_places_second_axis_below_first_axis(self):
        _, axes, nrows, ncols = create_subplots(2, vertical=True)

        self.assertEqual(2, nrows)
        self.assertEqual(1, ncols)
        self.assertEqual(axes[0][0].get_subplotspec().rowspan.start, 0)
        self.assertEqual(axes[0][0].get_subplotspec().colspan.start, 0)
        self.assertEqual(axes[1][0].get_subplotspec().rowspan.start, 1)
        self.assertEqual(axes[1][0].get_subplotspec().colspan.start, 0)

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

    def _create_plot(self, names, errors, overplot, wksp_indices=None, plot_type=SpectraSelection.Tiled, fig=None):
        selection = SpectraSelection(names)
        selection.wksp_indices = wksp_indices or [0]
        selection.plot_type = plot_type
        self.get_spectra_selection_mock.return_value = selection

        return plot_from_names(names=names, errors=errors, overplot=overplot, fig=fig)


if __name__ == "__main__":
    main()
