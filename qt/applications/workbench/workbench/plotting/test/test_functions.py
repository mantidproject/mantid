#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__  import absolute_import

# std imports
from unittest import TestCase, main
try:
    from unittest import mock
except ImportError:
    import mock

# third party imports
from mantid.api import AnalysisDataService, WorkspaceFactory
import matplotlib
matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
from mantidqt.dialogs.spectraselectordialog import SpectraSelection
import numpy as np

# local imports
from workbench.plotting.functions import (can_overplot, current_figure_or_none, figure_title,
                                          plot, plot_from_names, pcolormesh_from_names)


# Avoid importing the whole of mantid for a single mock of the workspace class
class FakeWorkspace(object):

    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


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

    @mock.patch('workbench.plotting.functions.get_spectra_selection')
    @mock.patch('workbench.plotting.functions.plot')
    def test_plot_from_names_calls_plot(self, get_spectra_selection_mock, plot_mock):
        ws_name = 'test_plot_from_names_calls_plot-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        selection = SpectraSelection([self._test_ws])
        selection.wksp_indices = [0]
        get_spectra_selection_mock.return_value = selection
        plot_from_names([ws_name], errors=False, overplot=False)

        self.assertEqual(1, plot_mock.call_count)

    @mock.patch('workbench.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_single_line_plot_for_valid_name(self, get_spectra_selection_mock):
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1"], wksp_indices=[0],
                                      errors=False, overplot=False)

    @mock.patch('workbench.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_single_error_plot_for_valid_name(self, get_spectra_selection_mock):
        fig = self._do_plot_from_names_test(get_spectra_selection_mock,
                                            # matplotlib does not set labels on the lines for error plots
                                            expected_labels=[None, None, None],
                                            wksp_indices=[0], errors=True, overplot=False)
        self.assertEqual(1, len(fig.gca().containers))

    @mock.patch('workbench.plotting.functions.get_spectra_selection')
    def test_plot_from_names_produces_overplot_for_valid_name(self, get_spectra_selection_mock):
        # make first plot
        plot([self._test_ws], wksp_indices=[0])
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1", "spec 2"],
                                      wksp_indices=[1], errors=False, overplot=True)

    @mock.patch('workbench.plotting.functions.get_spectra_selection')
    def test_plot_from_names_within_existing_figure(self, get_spectra_selection_mock):
        # make existing plot
        fig = plot([self._test_ws], wksp_indices=[0])
        self._do_plot_from_names_test(get_spectra_selection_mock, expected_labels=["spec 1", "spec 2"],
                                      wksp_indices=[1], errors=False, overplot=False,
                                      target_fig=fig)

    @mock.patch('workbench.plotting.functions.pcolormesh')
    def test_pcolormesh_from_names_calls_pcolormesh(self, pcolormesh_mock):
        ws_name = 'test_pcolormesh_from_names_calls_pcolormesh-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        pcolormesh_from_names([ws_name])

        self.assertEqual(1, pcolormesh_mock.call_count)

    def test_pcolormesh_from_names(self):
        ws_name = 'test_pcolormesh_from_names-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        fig = pcolormesh_from_names([ws_name])

        self.assertEqual(1, len(fig.gca().collections))

    def test_pcolormesh_from_names_using_existing_figure(self):
        ws_name = 'test_pcolormesh_from_names-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        target_fig = plt.figure()
        fig = pcolormesh_from_names([ws_name], fig=target_fig)

        self.assertEqual(fig, target_fig)
        self.assertEqual(1, len(fig.gca().collections))

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

        plotted_lines = fig.gca().get_lines()
        self.assertEqual(len(expected_labels), len(plotted_lines))
        for label_part, line in zip(expected_labels, plotted_lines):
            if label_part is not None:
                self.assertTrue(label_part in line.get_label(),
                                msg="Label fragment '{}' not found in line label".format(label_part))
        return fig


if __name__ == '__main__':
    main(buffer=False, verbosity=2)
