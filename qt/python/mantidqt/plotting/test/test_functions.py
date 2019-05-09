# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import

# std imports
from unittest import TestCase, main

# third party imports
import matplotlib
matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
import numpy as np

# local imports
# register mantid projection
import mantid.plots  # noqa
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.plots import MantidAxes
from mantid.py3compat import mock
from mantidqt.dialogs.spectraselectordialog import SpectraSelection
from mantidqt.plotting.functions import (can_overplot, current_figure_or_none, figure_title,
                                         manage_workspace_names, plot, plot_from_names,
                                         pcolormesh_from_names)


# Avoid importing the whole of mantid for a single mock of the workspace class
class FakeWorkspace(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


@manage_workspace_names
def workspace_names_dummy_func(workspaces):
    return workspaces


class FunctionsTest(TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)

    def tearDown(self):
        AnalysisDataService.Instance().clear()
        plt.close('all')

    def test_can_overplot_returns_false_with_no_active_plots(self):
        self.assertFalse(can_overplot()[0])

    def test_can_overplot_returns_true_for_active_line_plot(self):
        plt.plot([1, 2])
        self.assertTrue(can_overplot()[0])

    def test_can_overplot_returns_false_for_active_patch_plot(self):
        plt.pcolormesh(np.arange(9.).reshape(3,3))
        allowed, msg = can_overplot()
        self.assertFalse(allowed)
        self.assertTrue(len(msg) > 0)

    def test_current_figure_or_none_returns_none_if_no_figures_exist(self):
        self.assertTrue(current_figure_or_none() is None)

    def test_figure_title_with_single_string(self):
        self.assertEqual("test-1", figure_title("test", 1))

    def test_figure_title_with_list_of_strings(self):
        self.assertEqual("first-10", figure_title(["first", "second"], 10))

    def test_figure_title_with_single_workspace(self):
        self.assertEqual("fake-5", figure_title(FakeWorkspace("fake"), 5))

    def test_figure_title_with_workspace_list(self):
        self.assertEqual("fake-10", figure_title((FakeWorkspace("fake"),
                                                  FakeWorkspace("nextfake")), 10))

    def test_figure_title_with_empty_list_raises_assertion(self):
        with self.assertRaises(AssertionError):
            figure_title([], 5)

    def test_that_plot_can_accept_workspace_names(self):
        ws_name1 = "some_workspace"
        AnalysisDataService.Instance().addOrReplace(ws_name1, self._test_ws)

        try:
            result_workspaces = workspace_names_dummy_func([ws_name1])
        except ValueError:
            self.assertFalse(True, "Passing workspace names should not raise a value error.")
        else:
            # The list of workspace names we pass in should have been converted
            # to a list of workspaces
            self.assertNotEqual(result_workspaces, [ws_name1])

    @mock.patch('mantidqt.plotting.functions.get_spectra_selection')
    @mock.patch('mantidqt.plotting.functions.plot')
    def test_plot_from_names_calls_plot(self, get_spectra_selection_mock, plot_mock):
        ws_name = 'test_plot_from_names_calls_plot-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        selection = SpectraSelection([self._test_ws])
        selection.wksp_indices = [0]
        get_spectra_selection_mock.return_value = selection
        plot_from_names([ws_name], errors=False, overplot=False)

        self.assertEqual(1, plot_mock.call_count)

    @mock.patch('mantidqt.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_single_line_plot_for_valid_name(self, get_spectra_selection_mock):
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1"], wksp_indices=[0],
                                      errors=False, overplot=False)

    @mock.patch('mantidqt.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_single_error_plot_for_valid_name(self, get_spectra_selection_mock):
        fig = self._do_plot_from_names_test(get_spectra_selection_mock,
                                            # matplotlib does not set labels on the lines for error plots
                                            expected_labels=[],
                                            wksp_indices=[0], errors=True, overplot=False)
        self.assertEqual(1, len(fig.gca().containers))

    @mock.patch('mantidqt.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_overplot_for_valid_name(self, get_spectra_selection_mock):
        # make first plot
        plot([self._test_ws], wksp_indices=[0])
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1", "spec 2"],
                                      wksp_indices=[1], errors=False, overplot=True)

    @mock.patch('mantidqt.plotting.functions.get_spectra_selection')
    def test_plot_from_names_within_existing_figure(self, get_spectra_selection_mock):
        # make existing plot
        fig = plot([self._test_ws], wksp_indices=[0])
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1", "spec 2"],
                                      wksp_indices=[1], errors=False, overplot=False,
                                      target_fig=fig)

    @mock.patch('mantidqt.plotting.functions.pcolormesh')
    def test_pcolormesh_from_names_calls_pcolormesh(self, pcolormesh_mock):
        ws_name = 'test_pcolormesh_from_names_calls_pcolormesh-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        pcolormesh_from_names([ws_name])

        self.assertEqual(1, pcolormesh_mock.call_count)

    def test_pcolormesh_from_names(self):
        ws_name = 'test_pcolormesh_from_names-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        fig = pcolormesh_from_names([ws_name])

        self.assertEqual(1, len(fig.gca().images))

    def test_pcolormesh_from_names_using_existing_figure(self):
        ws_name = 'test_pcolormesh_from_names-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        target_fig = plt.figure()
        fig = pcolormesh_from_names([ws_name], fig=target_fig)

        self.assertEqual(fig, target_fig)
        self.assertEqual(1, len(fig.gca().images))

    def test_workspace_can_be_plotted_on_top_of_scripted_plots(self):
        fig = plt.figure()
        plt.plot([0, 1], [0, 1])
        ws = self._test_ws
        plot([ws], wksp_indices=[1], fig=fig, overplot=True)
        ax = plt.gca()
        self.assertEqual(len(ax.lines), 2)

    def test_title_preserved_when_workspace_plotted_on_scripted_plot(self):
        fig = plt.figure()
        plt.plot([0, 1], [0, 1])
        plt.title("My Title")
        ws = self._test_ws
        plot([ws], wksp_indices=[1], fig=fig, overplot=True)
        ax = plt.gca()
        self.assertEqual("My Title", ax.get_title())

    def test_different_line_colors_when_plotting_over_scripted_fig(self):
        fig = plt.figure()
        plt.plot([0, 1], [0, 1])
        ws = self._test_ws
        plot([ws], wksp_indices=[1], fig=fig, overplot=True)
        ax = plt.gca()
        line_colors = [line.get_color() for line in ax.get_lines()]
        self.assertNotEqual(line_colors[0], line_colors[1])

    def test_workspace_tracked_when_plotting_over_scripted_fig(self):
        fig = plt.figure()
        plt.plot([0, 1], [0, 1])
        ws = self._test_ws
        plot([ws], wksp_indices=[1], fig=fig, overplot=True)
        ax = plt.gca()
        self.assertIn(ws.name(), ax.tracked_workspaces)

    def test_from_mpl_axes_success_with_default_args(self):
        plt.figure()
        plt.plot([0, 1], [0, 1])
        plt.plot([0, 2], [0, 2])
        ax = plt.gca()
        mantid_ax = MantidAxes.from_mpl_axes(ax)
        self.assertEqual(len(mantid_ax.lines), 2)
        self.assertIsInstance(mantid_ax, MantidAxes)

    # ------------- Failure tests -------------

    def test_plot_from_names_with_non_plottable_workspaces_returns_None(self):
        table = WorkspaceFactory.Instance().createTable()
        table_name = 'test_plot_from_names_with_non_plottable_workspaces_returns_None'
        AnalysisDataService.Instance().addOrReplace(table_name, table)
        result = plot_from_names([table_name], errors=False, overplot=False)
        self.assertTrue(result is None)

    def test_pcolormesh_from_names_with_non_plottable_workspaces_returns_None(self):
        table = WorkspaceFactory.Instance().createTable()
        table_name = 'test_pcolormesh_from_names_with_non_plottable_workspaces_returns_None'
        AnalysisDataService.Instance().addOrReplace(table_name, table)
        result = pcolormesh_from_names([table_name])
        self.assertTrue(result is None)

    def test_that_manage_workspace_names_raises_on_mix_of_workspaces_and_names(self):
        ws = ["some_workspace", self._test_ws]
        AnalysisDataService.Instance().addOrReplace("some_workspace", self._test_ws)
        self.assertRaises(TypeError, workspace_names_dummy_func(ws))

    # ------------- Private -------------------
    def _do_plot_from_names_test(self, get_spectra_selection_mock, expected_labels,
                                 wksp_indices, errors, overplot, target_fig=None):
        ws_name = 'test_plot_from_names-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)

        selection = SpectraSelection([self._test_ws])
        selection.wksp_indices = wksp_indices
        get_spectra_selection_mock.return_value = selection
        fig = plot_from_names([ws_name], errors, overplot, target_fig)
        if target_fig is not None:
            self.assertEqual(target_fig, fig)

        plotted_lines = fig.gca().get_legend().get_lines()
        self.assertEqual(len(expected_labels), len(plotted_lines))
        for label_part, line in zip(expected_labels, plotted_lines):
            if label_part is not None:
                self.assertTrue(label_part in line.get_label(),
                                msg="Label fragment '{}' not found in line label".format(label_part))
        return fig


if __name__ == '__main__':
    main()
