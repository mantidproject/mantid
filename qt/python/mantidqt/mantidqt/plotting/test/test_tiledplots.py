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
from mantid.plots.plotfunctions import manage_workspace_names, get_plot_fig
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
