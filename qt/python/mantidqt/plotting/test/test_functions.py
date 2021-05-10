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
from matplotlib import cm

matplotlib.use('AGG')  # noqa
import matplotlib.pyplot as plt
import numpy as np

# local imports
# register mantid projection
import mantid.plots  # noqa
from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.simpleapi import CreateWorkspace, CreateSampleWorkspace, CreateMDHistoWorkspace
from mantid.kernel import config
from mantid.plots import MantidAxes
from unittest import mock
from mantidqt.dialogs.spectraselectordialog import SpectraSelection
from mantidqt.plotting.functions import (can_overplot, current_figure_or_none, figure_title,
                                         manage_workspace_names, plot, plot_from_names, plot_md_ws_from_names,
                                         pcolormesh_from_names, plot_surface)

IMAGE_PLOT_OPTIONS = {"plots.images.Colormap": "spring", "plots.images.ColorBarScale": "Log",
                      "plots.ShowMinorTicks": "off", "plots.ShowMinorGridlines": "off"}


class MockConfigService(object):
    def __init__(self):
        self.getString = mock.Mock(side_effect=IMAGE_PLOT_OPTIONS.get)


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
    _test_md_ws = None

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceFactory.Instance().create(
                "Workspace2D", NVectors=2, YLength=5, XLength=5)

        if self._test_md_ws is None:
            self._test_md_ws = CreateMDHistoWorkspace(SignalInput='1,2,3,4,2,1',
                                                      ErrorInput='1,1,1,1,1,1',
                                                      Dimensionality=3,
                                                      Extents='-1,1,-1,1,0.5,6.5',
                                                      NumberOfBins='1,1,6',
                                                      Names='x,y,|Q|',
                                                      Units='mm,km,AA^-1',
                                                      OutputWorkspace='test_plot_md_from_names_ws')

    def tearDown(self):
        AnalysisDataService.Instance().clear()
        plt.close('all')

    def test_can_overplot_returns_false_with_no_active_plots(self):
        self.assertFalse(can_overplot())

    def test_can_overplot_returns_true_for_active_line_plot(self):
        plt.plot([1, 2])
        self.assertTrue(can_overplot())

    def test_can_overplot_returns_false_for_active_patch_plot(self):
        plt.pcolormesh(np.arange(9.).reshape(3, 3))
        allowed = can_overplot()
        self.assertFalse(allowed)

    def test_current_figure_or_none_returns_none_if_no_figures_exist(self):
        self.assertEqual(current_figure_or_none(), None)

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
                                      wksp_indices=[1], errors=False, overplot=True,
                                      target_fig=fig)

    def test_plot_md_ws_from_names(self):
        """Test 1 workspace

        :return:
        """
        self._do_plot_md_from_names_test(expected_labels=['test_plot_md_from_names_ws'],
                                         errors=False, overplot=False, target_fig=None)

    @mock.patch('mantidqt.plotting.functions.pcolormesh')
    def test_pcolormesh_from_names_calls_pcolormesh(self, pcolormesh_mock):
        ws_name = 'test_pcolormesh_from_names_calls_pcolormesh-1'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_ws)
        pcolormesh_from_names([ws_name])
        self.assertEqual(1, pcolormesh_mock.call_count)

    def test_scale_is_correct_on_pcolourmesh_of_ragged_workspace(self):
        ws = CreateWorkspace(DataX=[1, 2, 3, 4, 2, 4, 6, 8], DataY=[2] * 8, NSpec=2)
        fig = pcolormesh_from_names([ws])
        self.assertEqual((1.8, 2.2), fig.axes[0].images[0].get_clim())

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

    @mock.patch('mantidqt.plotting.functions.ConfigService', new_callable=MockConfigService)
    def test_pcolor_mesh_from_names_gets_colorbar_scale_from_ConfigService(self, mock_ConfigService):
        ws = CreateSampleWorkspace()

        fig = pcolormesh_from_names([ws])

        mock_ConfigService.getString.assert_any_call('plots.images.ColorBarScale')
        self.assertTrue(isinstance(fig.gca().images[0].colorbar.norm, matplotlib.colors.LogNorm))

    @mock.patch('mantidqt.plotting.functions.ConfigService', new_callable=MockConfigService)
    def test_pcolor_mesh_from_names_gets_colormap_from_ConfigService(self, mock_ConfigService):
        ws = CreateSampleWorkspace()
        spring_colormap = cm.get_cmap('spring')

        fig = pcolormesh_from_names([ws])

        mock_ConfigService.getString.assert_any_call('plots.images.Colormap')
        self.assertEqual(fig.gca().images[0].colorbar.get_cmap(), spring_colormap)

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
        plot([ws], wksp_indices=[0, 1], fig=fig, waterfall=True)
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
        plot([ws], wksp_indices=[0, 1], fig=fig, waterfall=True)
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

    def test_plot_from_names_with_non_plottable_workspaces_returns_None(self):
        table = WorkspaceFactory.Instance().createTable()
        table_name = 'test_plot_from_names_with_non_plottable_workspaces_returns_None'
        AnalysisDataService.Instance().addOrReplace(table_name, table)
        result = plot_from_names([table_name], errors=False, overplot=False)
        self.assertEqual(result, None)

    def test_pcolormesh_from_names_with_non_plottable_workspaces_returns_None(self):
        table = WorkspaceFactory.Instance().createTable()
        table_name = 'test_pcolormesh_from_names_with_non_plottable_workspaces_returns_None'
        AnalysisDataService.Instance().addOrReplace(table_name, table)
        result = pcolormesh_from_names([table_name])
        self.assertEqual(result, None)

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

    def _do_plot_md_from_names_test(self, expected_labels, errors, overplot, target_fig):
        """
        Do plot_md_ws_from_names test (in general)

        :param expected_labels: list of strings as expected labels of a plot (i.e., workspace name)
        :param errors:
        :param overplot:
        :param target_fig:
        :return:
        """
        ws_name = 'test_plot_md_from_names_ws'
        AnalysisDataService.Instance().addOrReplace(ws_name, self._test_md_ws)

        # call method to test
        test_fig = plot_md_ws_from_names([ws_name], errors, overplot, target_fig)

        # Verification: with target figure, new plot will be plotted on the same one
        if target_fig is not None:
            self.assertEqual(target_fig, test_fig)

        # Check lines plotted
        plotted_lines = test_fig.gca().get_legend().get_lines()

        # number of plotted lines must be equal to expected values
        self.assertEqual(len(expected_labels), len(plotted_lines))
        # check legend labels
        for label_part, line in zip(expected_labels, plotted_lines):
            if label_part is not None:
                self.assertTrue(label_part in line.get_label())

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

    def test_colorbar_limits_not_default_values_on_surface_plot_with_monitor(self):
        ws = CreateSampleWorkspace(NumMonitors=1)
        fig = plt.figure()
        plot_surface([ws], fig=fig)
        ax = fig.get_axes()
        cmin, cmax = ax[0].collections[0].get_clim()

        # the colorbar limits default to +-0.1 when it can't find max and min of array
        self.assertNotEqual(cmax, 0.1)
        self.assertNotEqual(cmin, -0.1)


if __name__ == '__main__':
    main()
