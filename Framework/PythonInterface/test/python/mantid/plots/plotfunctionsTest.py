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
import unittest

# third party imports
import matplotlib

matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
import numpy as np

# local imports
# register mantid projection
import mantid.plots  # noqa
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import config
from mantid.plots import MantidAxes
from mantid.py3compat import mock
from mantid.plots.plotfunctions import (figure_title,
                                         manage_workspace_names, plot)


# Avoid importing the whole of mantid for a single mock of the workspace class
class FakeWorkspace(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


@manage_workspace_names
def workspace_names_dummy_func(workspaces):
    return workspaces


class FunctionsTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceFactory.Instance().create(
                "Workspace2D", NVectors=2, YLength=5, XLength=5)

    def tearDown(self):
        AnalysisDataService.Instance().clear()
        plt.close('all')

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
            self.fail("Passing workspace names should not raise a value error.")
        else:
            # The list of workspace names we pass in should have been converted
            # to a list of workspaces
            self.assertNotEqual(result_workspaces, [ws_name1])

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

    def test_that_plot_spectrum_has_same_y_label_with_and_without_errorbars(self):
        auto_dist = config['graph1d.autodistribution']
        try:
            config['graph1d.autodistribution'] = 'Off'
            self._compare_errorbar_labels_and_title()
        finally:
            config['graph1d.autodistribution'] = auto_dist

    def test_that_plot_spectrum_has_same_y_label_with_and_without_errorbars_normalize_by_bin_width(self):
        auto_dist = config['graph1d.autodistribution']
        try:
            config['graph1d.autodistribution'] = 'On'
            self._compare_errorbar_labels_and_title()
        finally:
            config['graph1d.autodistribution'] = auto_dist

    def test_setting_waterfall_to_true_makes_waterfall_plot(self):
        fig = plt.figure()
        ws = self._test_ws
        plot([ws], wksp_indices=[0,1], fig=fig, waterfall=True)
        ax = plt.gca()

        self.assertTrue(ax.is_waterfall())

    def test_cannot_make_waterfall_plot_with_one_line(self):
        fig = plt.figure()
        ws = self._test_ws
        plot([ws], wksp_indices=[1], fig=fig, waterfall=True)
        ax = plt.gca()

        self.assertFalse(ax.is_waterfall())

    def test_overplotting_onto_waterfall_plot_maintains_waterfall(self):
        fig = plt.figure()
        ws = self._test_ws
        plot([ws], wksp_indices=[0,1], fig=fig, waterfall=True)
        # Overplot one of the same lines.
        plot([ws], wksp_indices=[0], fig=fig, overplot=True)
        ax = plt.gca()

        # Check that the lines which would be the same in a non-waterfall plot are different.
        self.assertNotEqual(ax.get_lines()[0].get_xdata()[0], ax.get_lines()[2].get_xdata()[0])
        self.assertNotEqual(ax.get_lines()[0].get_ydata()[0], ax.get_lines()[2].get_ydata()[0])

    def test_overplotting_onto_waterfall_plot_with_filled_areas_adds_another_filled_area(self):
        fig = plt.figure()
        ws = self._test_ws
        plot([ws], wksp_indices=[0, 1], fig=fig, waterfall=True)
        ax = plt.gca()
        ax.set_waterfall_fill(True)
        plot([ws], wksp_indices=[0], fig=fig, overplot=True)

        fills = [collection for collection in ax.collections
                 if isinstance(collection, matplotlib.collections.PolyCollection)]

        self.assertEqual(len(fills), 3)

    # ------------- Failure tests -------------
    def test_that_manage_workspace_names_raises_on_mix_of_workspaces_and_names(self):
        ws = ["some_workspace", self._test_ws]
        AnalysisDataService.Instance().addOrReplace("some_workspace", self._test_ws)
        self.assertRaises(TypeError, workspace_names_dummy_func(ws))

    # ------------- Private -------------------
    def _compare_errorbar_labels_and_title(self):
        ws = self._test_ws
        ws.setYUnitLabel("MyLabel")
        ws.getAxis(0).setUnit("TOF")
        for distribution_ws in [True, False]:
            ws.setDistribution(distribution_ws)
            ax = plot([ws], wksp_indices=[1]).get_axes()[0]
            err_ax = plot([ws], wksp_indices=[1], errors=True).get_axes()[0]
            # Compare y-labels
            self.assertEqual(ax.get_ylabel(), err_ax.get_ylabel())
            # Compare x-labels
            self.assertEqual(ax.get_xlabel(), err_ax.get_xlabel())
            # Compare title
            self.assertEqual(ax.get_title(), err_ax.get_title())

if __name__ == '__main__':
    unittest.main()
