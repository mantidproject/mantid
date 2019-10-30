# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest

import matplotlib
matplotlib.use('AGG')  # noqa

from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.py3compat.mock import MagicMock, Mock, patch
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.plotting.functions import plot
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.fitpropertybrowser.fitpropertybrowser import FitPropertyBrowser
from testhelpers import assertRaisesNothing
from workbench.plotting.figuremanager import FigureManagerADSObserver

from qtpy import PYQT5


@start_qapplication
class FitPropertyBrowserTest(unittest.TestCase):
    def tearDown(self):
        AnalysisDataService.clear()

    def test_initialization_does_not_raise(self):
        assertRaisesNothing(self, self._create_widget)

    @patch('mantidqt.widgets.fitpropertybrowser.fitpropertybrowser.FitPropertyBrowser.normaliseData'
           )
    def test_normalise_data_set_on_fit_menu_shown(self, normaliseData_mock):
        for normalised in [True, False]:
            ws_artist_mock = Mock(is_normalized=normalised, workspace_index=0)
            axes_mock = Mock(tracked_workspaces={'ws_name': [ws_artist_mock]})
            property_browser = self._create_widget()
            with patch.object(property_browser, 'get_axes', lambda: axes_mock):
                with patch.object(property_browser, 'workspaceName', lambda: 'ws_name'):
                    property_browser.getFitMenu().aboutToShow.emit()
            property_browser.normaliseData.assert_called_once_with(normalised)
            normaliseData_mock.reset_mock()

    def test_plot_guess_plots_for_table_workspaces(self):
        table = WorkspaceFactory.createTable()
        table.addColumn('double', 'X', 1)
        table.addColumn('double', 'Y', 2)
        for i in range(1, 10):
            table.addRow([0.1 * i, 5])
        name = "table_name"
        AnalysisDataService.Instance().addOrReplace(name, table)
        property_browser = self._create_widget()
        property_browser.getFittingFunction = Mock(return_value='name=FlatBackground')
        property_browser.workspaceName = Mock(return_value=name)
        property_browser.startX = Mock(return_value=0.15)
        property_browser.endX = Mock(return_value=0.95)
        property_browser.get_axes().get_legend = Mock(return_value=None)
        property_browser.plot_guess()

        self.assertEqual(1, property_browser.get_axes().plot.call_count)

    def test_fit_curves_removed_when_workspaces_deleted(self):
        fig, canvas = self._create_and_plot_matrix_workspace(name="ws")
        property_browser = self._create_widget(canvas=canvas)

        manager_mock = Mock()
        manager_mock.canvas = canvas
        observer = FigureManagerADSObserver(manager=manager_mock)  # noqa: F841

        for plot_diff in [True, False]:
            # create fake fit output results
            matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D",
                                                                 NVectors=3,
                                                                 YLength=5,
                                                                 XLength=5)
            tableWorkspace = WorkspaceFactory.createTable()
            AnalysisDataService.Instance().addOrReplace("ws_Workspace", matrixWorkspace)
            AnalysisDataService.Instance().addOrReplace("ws_Parameters", tableWorkspace)
            AnalysisDataService.Instance().addOrReplace("ws_NormalisedCovarianceMatrix",
                                                        tableWorkspace)

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
        fig, canvas = self._create_and_plot_matrix_workspace(name)
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

    def test_fit_result_matrix_workspace_in_browser_is_viewed_when_clicked(self):
        if not PYQT5:
            self.skipTest("MatrixWorkspaceDisplay and TableWorkspaceDisplay cannot be "
                          "imported in qt4 so the test fails with an error.")
        from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay

        name = "ws"
        fig, canvas = self._create_and_plot_matrix_workspace(name)
        property_browser = self._create_widget(canvas=canvas)
        property_browser.setOutputName(name)

        # create fake fit output results
        matrixWorkspace = WorkspaceFactory.Instance().create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        AnalysisDataService.Instance().addOrReplace(name + "_Workspace", matrixWorkspace)

        property_browser.fitting_done_slot(name + "_Workspace")
        wsList = property_browser.getWorkspaceList()

        # click on matrix workspace
        MatrixWorkspaceDisplay.show_view = Mock()
        item = wsList.item(0).text()
        property_browser.workspaceClicked.emit(item)
        self.assertEqual(1, MatrixWorkspaceDisplay.show_view.call_count)

    def test_fit_parameter_table_workspaces_in_browser_is_viewed_when_clicked(self):
        if not PYQT5:
            self.skipTest("MatrixWorkspaceDisplay and TableWorkspaceDisplay cannot be "
                          "imported in qt4 so the test fails with an error.")
        from mantidqt.widgets.workspacedisplay.table.presenter import TableWorkspaceDisplay

        name = "ws"
        fig, canvas = self._create_and_plot_matrix_workspace(name)
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
        fig, canvas_mock = self._create_and_plot_matrix_workspace(name)
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

    def test_plot_limits_are_not_changed_when_plotting_fit_lines(self):
        fig, canvas = self._create_and_plot_matrix_workspace()
        ax_limits = fig.get_axes()[0].axis()
        widget = self._create_widget(canvas=canvas)
        fit_ws_name = "fit_ws"
        CreateSampleWorkspace(OutputWorkspace=fit_ws_name)
        widget.fitting_done_slot(fit_ws_name)
        self.assertEqual(ax_limits, fig.get_axes()[0].axis())

    def test_plot_limits_are_not_changed_when_plotting_guess(self):
        fig, canvas = self._create_and_plot_matrix_workspace()
        ax_limits = fig.get_axes()[0].axis()
        widget = self._create_widget(canvas=canvas)
        widget.plot_guess()
        self.assertEqual(ax_limits, fig.get_axes()[0].axis())

    # Private helper functions
    def _create_widget(self, canvas=MagicMock(), toolbar_manager=Mock()):
        return FitPropertyBrowser(canvas, toolbar_manager)

    def _create_and_plot_matrix_workspace(self, name = "workspace"):
        ws = WorkspaceFactory.Instance().create("Workspace2D", NVectors=2, YLength=5, XLength=5)
        AnalysisDataService.Instance().addOrReplace(name, ws)
        fig = plot([ws], spectrum_nums=[1])
        canvas = fig.canvas
        return fig, canvas


if __name__ == '__main__':
    unittest.main()
