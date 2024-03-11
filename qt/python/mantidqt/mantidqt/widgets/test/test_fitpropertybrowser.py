# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib

matplotlib.use("AGG")

import numpy as np
import time

from mantid.api import AnalysisDataService, WorkspaceFactory
from unittest.mock import MagicMock, Mock, patch
from mantid import ConfigService
from mantid.simpleapi import CreateSampleWorkspace, CreateWorkspace
from mantidqt.plotting.functions import plot
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from mantidqt.widgets.fitpropertybrowser.fitpropertybrowser import FitPropertyBrowser
from workbench.plotting.figuremanager import FigureManagerADSObserver
from mantid.plots.utility import MantidAxType, zoom

from qtpy.QtWidgets import QDockWidget


class MockConfigService(object):
    def __init__(self):
        self.setString = StrictMock()


@start_qapplication
class FitPropertyBrowserTest(unittest.TestCase):
    def tearDown(self):
        AnalysisDataService.clear()

    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.normaliseData")
    def test_normalise_data_set_on_fit_menu_shown(self, normaliseData_mock):
        for normalised in [True, False]:
            ws_artist_mock = Mock(is_normalized=normalised, workspace_index=0)
            axes_mock = Mock(tracked_workspaces={"ws_name": [ws_artist_mock]})
            property_browser = self._create_widget()
            with patch.object(property_browser, "get_axes", lambda: axes_mock):
                with patch.object(property_browser, "workspaceName", lambda: "ws_name"):
                    property_browser.getFitMenu().aboutToShow.emit()
            property_browser.normaliseData.assert_called_once_with(normalised)
            normaliseData_mock.reset_mock()

    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.normaliseData")
    def test_normalise_data_set_to_false_for_distribution_workspace(self, normaliseData_mock):
        fig, canvas, _ = self._create_and_plot_matrix_workspace("ws_name", distribution=True)
        property_browser = self._create_widget(canvas=canvas)
        with patch.object(property_browser, "workspaceName", lambda: "ws_name"):
            property_browser.getFitMenu().aboutToShow.emit()
        property_browser.normaliseData.assert_called_once_with(False)

    def test_workspace_index_selector_updates_if_new_curve_added(self):
        fig, canvas, ws = self._create_and_plot_matrix_workspace("ws_name", distribution=True)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setWorkspaceName("ws_name")
        plot([ws], spectrum_nums=[3], overplot=True, fig=fig)
        property_browser.setWorkspaceIndex(2)
        self.assertEqual(property_browser.workspaceIndex(), 2)
        property_browser.hide()

    def test_workspace_index_selector_updates_if_curve_removed(self):
        fig, canvas, ws = self._create_and_plot_matrix_workspace("ws_name", distribution=True)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setWorkspaceName("ws_name")
        plot([ws], spectrum_nums=[3], overplot=True, fig=fig)
        property_browser.show()
        # remove first spectrum
        fig.axes[0].tracked_workspaces["ws_name"].pop(0)
        fig.canvas.draw()
        # we removed the workspaceIndex 0 line, so spinbox should now show 2.
        self.assertEqual(property_browser.workspaceIndex(), 2)
        property_browser.hide()

    def test_fit_curves_removed_when_workspaces_deleted(self):
        fig, canvas, _ = self._create_and_plot_matrix_workspace(name="ws")
        property_browser = self._create_widget(canvas=canvas)

        manager_mock = Mock()
        manager_mock.canvas = canvas
        observer = FigureManagerADSObserver(manager=manager_mock)  # noqa: F841

        for plot_diff in [True, False]:
            # create fake fit output results
            matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
            tableWorkspace = WorkspaceFactory.createTable()
            AnalysisDataService.Instance().addOrReplace("ws_Workspace", matrixWorkspace)
            AnalysisDataService.Instance().addOrReplace("ws_Parameters", tableWorkspace)
            AnalysisDataService.Instance().addOrReplace("ws_NormalisedCovarianceMatrix", tableWorkspace)

            property_browser.plotDiff = Mock(return_value=plot_diff)
            property_browser.fitting_done_slot("ws_Workspace")

            if plot_diff:
                self.assertEqual(3, len(fig.get_axes()[0].lines))
            else:
                self.assertEqual(2, len(fig.get_axes()[0].lines))

            AnalysisDataService.Instance().remove("ws_Workspace")
            AnalysisDataService.Instance().remove("ws_Parameters")
            AnalysisDataService.Instance().remove("ws_NormalisedCovarianceMatrix")

            self.assertEqual(1, len(fig.get_axes()[0].lines))

    def test_fit_result_workspaces_are_added_to_browser_when_fitting_done(self):
        name = "ws"
        fig, canvas, _ = self._create_and_plot_matrix_workspace(name)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setOutputName(name)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        tableWorkspace = WorkspaceFactory.createTable()
        AnalysisDataService.Instance().addOrReplace(name + "_Workspace", matrixWorkspace)
        AnalysisDataService.Instance().addOrReplace(name + "_Parameters", tableWorkspace)
        AnalysisDataService.Instance().addOrReplace(name + "_NormalisedCovarianceMatrix", tableWorkspace)

        property_browser.fitting_done_slot(name + "_Workspace")
        workspaceList = property_browser.getWorkspaceList()

        self.assertEqual(3, workspaceList.count())
        self.assertEqual(name + "_NormalisedCovarianceMatrix", workspaceList.item(0).text())
        self.assertEqual(name + "_Parameters", workspaceList.item(1).text())
        self.assertEqual(name + "_Workspace", workspaceList.item(2).text())

    def test_fitting_done_follows_ads_clear_fitting_done_another(self):
        name_1 = "ws_1"
        fig, canvas, _ = self._create_and_plot_matrix_workspace(name_1)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setOutputName(name_1)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        tableWorkspace = WorkspaceFactory.createTable()
        AnalysisDataService.Instance().addOrReplace(name_1 + "_Workspace", matrixWorkspace)
        AnalysisDataService.Instance().addOrReplace(name_1 + "_Parameters", tableWorkspace)
        AnalysisDataService.Instance().addOrReplace(name_1 + "_NormalisedCovarianceMatrix", tableWorkspace)

        property_browser.fitting_done_slot(name_1 + "_Workspace")
        workspaceList = property_browser.getWorkspaceList()
        self.assertEqual(3, workspaceList.count())

        # Removing the specific workspaces created during the test
        AnalysisDataService.remove(name_1)
        AnalysisDataService.remove(name_1 + "_Workspace")
        AnalysisDataService.remove(name_1 + "_Parameters")
        AnalysisDataService.remove(name_1 + "_NormalisedCovarianceMatrix")

        name_2 = "ws_2"
        ws = CreateWorkspace(OutputWorkspace=name_2, DataX=np.arange(10), DataY=2 + np.arange(10), NSpec=5)
        property_browser.do_plot(ws)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        tableWorkspace = WorkspaceFactory.createTable()
        AnalysisDataService.Instance().addOrReplace(name_2 + "_Workspace", matrixWorkspace)
        AnalysisDataService.Instance().addOrReplace(name_2 + "_Parameters", tableWorkspace)
        AnalysisDataService.Instance().addOrReplace(name_2 + "_NormalisedCovarianceMatrix", tableWorkspace)

        property_browser.fitting_done_slot(name_2 + "_Workspace")
        self.assertEqual(property_browser.fit_result_ws_name, name_2 + "_Workspace")
        self.assertEqual(3, property_browser.getWorkspaceList().count())

    def test_fit_result_matrix_workspace_in_browser_is_viewed_when_clicked(self):
        from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay

        name = "ws"
        fig, canvas, _ = self._create_and_plot_matrix_workspace(name)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setOutputName(name)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        AnalysisDataService.Instance().addOrReplace(name + "_Workspace", matrixWorkspace)
        tableWorkspace = WorkspaceFactory.createTable()
        AnalysisDataService.Instance().addOrReplace(name + "_Parameters", tableWorkspace)

        property_browser.fitting_done_slot(name + "_Workspace")
        wsList = property_browser.getWorkspaceList()
        TableWorkspaceDisplay.show_view = Mock()

        # click on table workspace
        item = wsList.item(0).text()
        property_browser.workspaceClicked.emit(item)
        self.assertEqual(1, TableWorkspaceDisplay.show_view.call_count)

    def test_workspaces_removed_from_workspace_list_widget_if_deleted_from_ADS(self):
        name = "ws"
        fig, canvas_mock, _ = self._create_and_plot_matrix_workspace(name)
        property_browser = self._create_widget(canvas=canvas_mock)
        property_browser.setOutputName(name)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        AnalysisDataService.Instance().addOrReplace(name + "_Workspace", matrixWorkspace)
        tableWorkspace = WorkspaceFactory.createTable()
        AnalysisDataService.Instance().addOrReplace(name + "_Parameters", tableWorkspace)

        property_browser.fitting_done_slot(name + "_Workspace")
        AnalysisDataService.Instance().remove(name + "_Parameters")
        property_browser.postDeleteHandle(name + "_Parameters")

        wsList = property_browser.getWorkspaceList()
        self.assertEqual(1, len(wsList))

    def test_plot_limits_are_not_changed_when_plotting_fit_lines_autoscale_true(self):
        fig, canvas, _ = self._create_and_plot_matrix_workspace()
        ax_limits = fig.get_axes()[0].axis()
        canvas.draw()
        widget = self._create_widget(canvas=canvas)
        fit_ws_name = "fit_ws"
        CreateSampleWorkspace(OutputWorkspace=fit_ws_name)
        widget.fitting_done_slot(fit_ws_name)
        self.assertEqual(ax_limits, fig.get_axes()[0].axis())

    def test_plot_limits_are_not_changed_when_plotting_fit_lines_autoscale_false(self):
        fig, canvas, ws = self._create_and_plot_matrix_workspace()
        ax = fig.get_axes()[0]
        full_ax_limits = ax.axis()
        # zoom in x2 based on the central x and y values and recapture limits
        # note, zoom turns autoscale off
        zoom(ax, 0.5 * (full_ax_limits[0] + full_ax_limits[1]), 0.5 * (full_ax_limits[2] + full_ax_limits[3]), 2)
        ax_limits = ax.axis()
        canvas.draw()
        widget = self._create_widget(canvas=canvas)
        fit_ws_name = "fit_ws"
        # create fit workspace containing 3 spectra (data\calc\diff)
        CreateWorkspace(OutputWorkspace=fit_ws_name, DataX=[ax_limits[0], ax_limits[1]], DataY=[1] * 6, NSpec=3, Distribution=False)
        widget.fitting_done_slot(fit_ws_name)
        self.assertEqual(ax_limits, fig.get_axes()[0].axis())
        # user picks x range bigger than current zoom (but still within ws data range). Plot limits still don't change
        CreateWorkspace(
            OutputWorkspace=fit_ws_name, DataX=[full_ax_limits[0], full_ax_limits[1]], DataY=[1] * 6, NSpec=3, Distribution=False
        )
        widget.fitting_done_slot(fit_ws_name)
        self.assertEqual(ax_limits, fig.get_axes()[0].axis())

    @patch("matplotlib.pyplot.get_figlabels")
    def test_output_workspace_name_changes_if_more_than_one_plot_of_same_workspace(self, figure_labels_mock):
        # create a workspace
        ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)
        AnalysisDataService.Instance().addOrReplace("workspace", ws)
        ws_window_names = ["workspace-1", "workspace-2"]
        figure_labels_mock.return_value = ws_window_names
        output_name = []
        # plot it twice
        for i in [0, 1]:
            fig = plot([ws], spectrum_nums=[1])
            fig.canvas.manager.get_window_title = Mock(return_value=ws_window_names[i])
            browser = self._create_widget(canvas=fig.canvas)
            # don't want the widget to actually show in test
            QDockWidget.show = Mock()
            browser.show()
            output_name.append(browser.outputName())
        self.assertNotEqual(output_name[0], output_name[1])

    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.setPeakFwhmOf")
    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.isParameterExplicitlySetOf")
    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.getWidthParameterNameOf")
    def test_set_peak_initial_fwhm(self, mock_getWidthName, mock_parameterSet, mock_setFwhm):
        property_browser = self._create_widget()
        mock_getWidthName.side_effect = lambda prefix: "S" if prefix == "f0" else ""
        mock_parameterSet.return_value = True
        fwhm = 1.0
        # check width isn't set if already set when initialised (as can happen for B2B exp funcs)
        property_browser._set_peak_initial_fwhm("f0", fwhm)
        mock_setFwhm.assert_not_called()
        # fwhm should be set if width parameter not specified for that peak func
        property_browser._set_peak_initial_fwhm("f1", fwhm)
        mock_setFwhm.assert_called_once_with("f1", fwhm)

    def test_new_fit_browser_has_correct_peak_type_when_not_changing_global_default(self):
        """
        Change the default peak type in a fit browser without specifying that the global value should change and check
        that a new fit browser still has the default value from the config.
        """
        default_peak_type = ConfigService["curvefitting.defaultPeak"]

        fit_browser = self._create_widget_with_interactive_tool()

        # Find a peak type different to the default in the config.
        new_peak_type = None
        for peak_name in fit_browser.registeredPeaks():
            if peak_name != default_peak_type:
                new_peak_type = peak_name
                break

        self.assertIsNotNone(new_peak_type)

        # Setting the peak type for a fit browser should update the default peak type for only that fit browser
        fit_browser.tool.action_peak_added(new_peak_type)
        self.assertEqual(fit_browser.defaultPeakType(), new_peak_type)

        # Make sure the default peak type in the config didn't change.
        self.assertEqual(ConfigService["curvefitting.defaultPeak"], default_peak_type)

        # A new fit browser should have the default peak type from the config.
        new_fit_browser = self._create_widget_with_interactive_tool()
        self.assertNotEqual(new_fit_browser.defaultPeakType(), fit_browser.defaultPeakType())
        self.assertEqual(new_fit_browser.defaultPeakType(), default_peak_type)

    @patch("mantidqt.widgets.fitpropertybrowser.interactive_tool.ConfigService", new_callable=MockConfigService)
    def test_new_fit_browser_has_correct_peak_type_when_changing_global_default(self, mock_config_service):
        """
        Check that the config service default peak is updated when explicitly requesting it from the fit browser
        interactive tool.
        """
        fit_browser = self._create_widget_with_interactive_tool()
        fit_browser.tool.action_peak_added("Lorentzian", set_global_default=True)

        mock_config_service.setString.assert_called_once_with("curvefitting.defaultPeak", "Lorentzian")

    def test_fit_property_browser_correctly_updates_contents_from_fitted_func(self):
        fig, canvas, ws = self._create_and_plot_matrix_workspace("ws_name", distribution=True)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.show()
        property_browser.setStartX(0)
        property_browser.setEndX(1)
        property_browser.loadFunction("name=LinearBackground,A0=0,A1=0")

        # run the fit
        property_browser.fit()

        # we need to sleep for a bit, as when the fit is complete the gui still needs to be updated
        time.sleep(2)

        # the new function is stored in the browsers PropertyHandler
        func = property_browser.currentHandler().ifun()

        # it should show a linear line with intercept 2 and gradient 1
        self.assertEqual(str(func.getFunction(0)), "name=LinearBackground,A0=2,A1=1")

    def test_fit_property_browser_correctly_handles_bin_plots(self):
        # create & plot workspace
        fig, canvas, ws = self._create_and_plot_matrix_workspace("ws_name", distribution=True)

        # set bin plot kwargs
        plot_kwargs = {"axis": MantidAxType.BIN}

        # overplot bin plot
        plot([ws], wksp_indices=[0], plot_kwargs=plot_kwargs, fig=fig, overplot=True)

        property_browser = self._create_widget(canvas=canvas)

        # if only 1 spectra is returned, bin spectra has been correctly excluded, spectra correctly included
        self.assertEqual(len(property_browser._get_allowed_spectra()), 1)

        # remove valid spectra plot, leaving a single bin plot
        fig.get_axes()[0].remove_artists_if(lambda artist: artist.get_label() == "ws_name: spec 1")

        # check no spectra is now returned.
        self.assertFalse(property_browser._get_allowed_spectra())

    # Private helper functions
    @classmethod
    def _create_widget(cls, canvas=MagicMock(), toolbar_manager=Mock()):
        return FitPropertyBrowser(canvas, toolbar_manager)

    @classmethod
    @patch("mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowserBase.show")
    def _create_widget_with_interactive_tool(cls, _):
        """
        Need to mock some functions and call "show()" in order for the fit browser to create its FitInteractiveTool
        object.
        """
        _, canvas, _ = cls._create_and_plot_matrix_workspace()
        fit_browser = FitPropertyBrowser(canvas=canvas, toolbar_manager=Mock())
        # Mock these functions so that we can call show().
        canvas.draw = Mock()
        fit_browser._get_allowed_spectra = Mock(return_value=True)
        fit_browser._get_table_workspace = Mock(return_value=False)
        fit_browser._add_spectra = Mock()
        fit_browser.set_output_window_names = Mock(return_value=None)
        # Need to call show() to set up the FitInteractiveTool, but we've mocked the superclass show() function
        # so it won't actually show the widget.
        fit_browser.show()
        return fit_browser

    @classmethod
    def _create_and_plot_matrix_workspace(cls, name="workspace", distribution=False):
        ws = CreateWorkspace(OutputWorkspace=name, DataX=np.arange(10), DataY=2 + np.arange(10), NSpec=5, Distribution=distribution)
        fig = plot([ws], spectrum_nums=[1])
        canvas = fig.canvas
        return fig, canvas, ws


if __name__ == "__main__":
    unittest.main()
