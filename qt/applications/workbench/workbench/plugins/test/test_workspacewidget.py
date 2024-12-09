# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import unittest
from unittest import mock

import matplotlib as mpl
from mantid.api import AnalysisDataService
from mantid.simpleapi import (
    CreateEmptyTableWorkspace,
    CreateSampleWorkspace,
    CreateSingleValuedWorkspace,
    CreateMDHistoWorkspace,
    CreateWorkspace,
    ConjoinWorkspaces,
    GroupWorkspaces,
    LoadNexus,
)
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from qtpy.QtWidgets import QMainWindow
from workbench.plugins.workspacewidget import WorkspaceWidget

from mantid.plots.utility import MantidAxType

mpl.use("Agg")

ALGORITHM_HISTORY_WINDOW_TYPE = "AlgorithmHistoryWindow"
ALGORITHM_HISTORY_WINDOW = "mantidqt.widgets.workspacewidget." "algorithmhistorywindow." + ALGORITHM_HISTORY_WINDOW_TYPE
MATRIXWORKSPACE_DISPLAY = "mantidqt.widgets.workspacedisplay.matrix." "presenter.MatrixWorkspaceDisplay"
MATRIXWORKSPACE_DISPLAY_TYPE = "StatusBarView"
SAMPLE_MATERIAL_DIALOG_TYPE = "SampleMaterialDialogView"
SAMPLE_MATERIAL_DIALOG = "mantidqt.widgets.samplematerialdialog.samplematerial_view." + SAMPLE_MATERIAL_DIALOG_TYPE


@start_qapplication
class WorkspaceWidgetTest(unittest.TestCase, QtWidgetFinder):
    @classmethod
    def setUpClass(cls):
        cls.ws_widget = WorkspaceWidget(QMainWindow())
        mat_ws = CreateSampleWorkspace()
        table_ws = CreateEmptyTableWorkspace()
        group_ws = GroupWorkspaces([mat_ws, table_ws])
        single_val_ws = CreateSingleValuedWorkspace(5, 6)

        # Create ragged workspace
        ws2d_ragged = CreateWorkspace(DataX=[10, 20, 30], DataY=[1, 2, 3], NSpec=1, OutputWorkspace="Ragged")
        temp = CreateWorkspace(DataX=[15, 25, 35, 45], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(ws2d_ragged, temp, CheckOverlapping=False, CheckMatchingBins=False)
        ws2d_ragged = AnalysisDataService.retrieve("Ragged")

        cls.w_spaces = [mat_ws, table_ws, group_ws, single_val_ws]
        cls.ws_names = ["MatWS", "TableWS", "GroupWS", "SingleValWS"]
        # create md workspace
        md_ws = CreateMDHistoWorkspace(
            SignalInput="1,2,3,4,2,1",
            ErrorInput="1,1,1,1,1,1",
            Dimensionality=3,
            Extents="-1,1,-1,1,0.5,6.5",
            NumberOfBins="1,1,6",
            Names="x,y,|Q|",
            Units="mm,km,AA^-1",
            OutputWorkspace="MDHistoWS1D",
        )
        # self.w_spaces = [mat_ws, table_ws, group_ws, single_val_ws, md_ws]
        # self.ws_names = ['MatWS', 'TableWS', 'GroupWS', 'SingleValWS', 'MDHistoWS1D']
        for ws_name, ws in zip(cls.ws_names, cls.w_spaces):
            cls.ws_widget._ads.add(ws_name, ws)
        cls.ws_names.append(md_ws.name())
        cls.w_spaces.append(md_ws)
        cls.ws_names.append(ws2d_ragged.name())
        cls.w_spaces.append(ws2d_ragged)

    @classmethod
    def tearDownClass(cls):
        cls.ws_widget._ads.clear()

    def test_algorithm_history_window_opens_with_workspace(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + ".show", lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[0]])
        self.assert_widget_type_exists(ALGORITHM_HISTORY_WINDOW_TYPE)

    def test_algorithm_history_window_doesnt_open_with_workspace_group(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + ".show", lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[2]])
        self.assert_widget_type_doesnt_exist(ALGORITHM_HISTORY_WINDOW_TYPE)

    def test_algorithm_history_window_opens_multiple(self):
        """
        There are 6 objects in ADS.  But 1 of them is WorkspaceGroup
        This sets total number of history windows to 5.

        :return:
        """
        with mock.patch(ALGORITHM_HISTORY_WINDOW + ".show", lambda x: None):
            self.ws_widget._do_show_algorithm_history(self.ws_names)
        self.assert_number_of_widgets_matching(ALGORITHM_HISTORY_WINDOW_TYPE, 5)

    def test_detector_table_shows_with_workspace(self):
        with mock.patch(MATRIXWORKSPACE_DISPLAY + ".show_view", lambda x: None):
            self.ws_widget._do_show_detectors([self.ws_names[0]])
        self.assert_widget_type_exists(MATRIXWORKSPACE_DISPLAY_TYPE)

    def test_detector_table_shows_with_a_workspace_missing_an_efixed(self):
        workspace_name = "TOSCA26911_show_detectors"
        LoadNexus(Filename=workspace_name + ".nxs", OutputWorkspace=workspace_name)

        with mock.patch(MATRIXWORKSPACE_DISPLAY + ".show_view", lambda x: None):
            self.ws_widget._do_show_detectors([workspace_name])
        self.assert_widget_type_exists(MATRIXWORKSPACE_DISPLAY_TYPE)

    @mock.patch("workbench.plugins.workspacewidget.plot", autospec=True)
    def test_plot_with_plot_bin(self, mock_plot):
        self.ws_widget._ads.add(self.ws_names[0], self.w_spaces[0])
        self.ws_widget._do_plot_bin([self.ws_names[0]], False, False)
        mock_plot.assert_called_once_with(mock.ANY, errors=False, overplot=False, wksp_indices=[0], plot_kwargs={"axis": MantidAxType.BIN})

    @mock.patch("workbench.plugins.workspacewidget.plot_from_names", autospec=True)
    def test_plot_with_plot_spectrum(self, mock_plot_from_names):
        self.ws_widget._do_plot_spectrum([self.ws_names[0]], False, False)
        mock_plot_from_names.assert_called_once_with([self.ws_names[0]], False, False, advanced=False)

    @mock.patch("workbench.plugins.workspacewidget.plot_md_ws_from_names", autospec=True)
    def test_plot_with_1d_mdhistoworkspace(self, mock_plot_md_from_names):
        self.ws_widget._do_plot_1d_md([self.ws_names[4]], False, False)
        mock_plot_md_from_names.assert_called_once_with([self.ws_names[4]], False, False)

    @mock.patch("workbench.plugins.workspacewidget.pcolormesh", autospec=True)
    def test_plot_with_plot_colorfill(self, mock_plot_colorfill):
        self.ws_widget._do_plot_colorfill([self.ws_names[0]])
        mock_plot_colorfill.assert_called_once_with(mock.ANY)

    @mock.patch("workbench.plugins.workspacewidget.plot_from_names", autospec=True)
    def test_plot_with_plot_advanced(self, mock_plot_from_names):
        self.ws_widget._do_plot_spectrum([self.ws_names[0]], False, False, advanced=True)
        mock_plot_from_names.assert_called_once_with([self.ws_names[0]], False, False, advanced=True)

    @mock.patch("mantidqt.plotting.functions.plot_contour", autospec=True)
    def test_plot_with_plot_contour(self, mock_plot_contour):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type="contour")
        mock_plot_contour.assert_called_once_with([self.ws_names[0]])

    @mock.patch("mantidqt.plotting.functions.plot_surface", autospec=True)
    def test_plot_with_plot_surface(self, mock_plot_surface):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type="surface")
        mock_plot_surface.assert_called_once_with([self.ws_names[0]])

    @mock.patch("mantidqt.plotting.functions.plot_wireframe", autospec=True)
    def test_plot_with_plot_wireframe(self, mock_plot_wireframe):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type="wireframe")
        mock_plot_wireframe.assert_called_once_with([self.ws_names[0]])

    def test_double_click_with_single_value_ws_shows_data(self):
        with mock.patch(MATRIXWORKSPACE_DISPLAY + ".show_view", lambda x: None):
            self.ws_widget._action_double_click_workspace(self.ws_names[3])
        self.assert_widget_type_exists(MATRIXWORKSPACE_DISPLAY_TYPE)

    @mock.patch("workbench.plugins.workspacewidget.plot_from_names", autospec=True)
    def test_double_click_with_ragged_ws_calls_plot_from_names(self, mock_plot_from_names):
        self.assertTrue(self.w_spaces[5].isRaggedWorkspace())
        self.ws_widget._action_double_click_workspace(self.ws_names[5])
        mock_plot_from_names.assert_called_once_with([self.ws_names[5]], errors=False, overplot=False, show_colorfill_btn=True)

    def test_sample_material_opens_with_single_workspace_name(self):
        """
        Sample material dialog should work with a single workspace selected
        """
        single_ws_list = [self.ws_names[0]]
        with mock.patch(SAMPLE_MATERIAL_DIALOG + ".show", lambda x: None):
            self.ws_widget._do_sample_material(single_ws_list)
        self.assert_widget_type_exists(SAMPLE_MATERIAL_DIALOG_TYPE)

    def test_sample_material_doesnt_open_with_multiple_workspace_names(self):
        """
        Sample material dialog should not open if multiple workspaces are selected.
        """
        with mock.patch(SAMPLE_MATERIAL_DIALOG + ".show", lambda x: None):
            self.ws_widget._do_sample_material(self.ws_names)
        self.assert_widget_type_doesnt_exist(SAMPLE_MATERIAL_DIALOG_TYPE)

    @mock.patch("mantidqt.plotting.sample_shape.plot_sample_container_and_components", autospec=True)
    def test_show_sample_plots_with_single_workspace_name(self, mock_plot_sample_container_and_components):
        """
        Sample material dialog should work with a single workspace selected
        """
        self.ws_widget._show_sample_shape([self.ws_names[0]])
        mock_plot_sample_container_and_components.assert_called_once_with(self.ws_names[0])

    @mock.patch("mantidqt.plotting.sample_shape.plot_sample_container_and_components", autospec=True)
    def test_show_sample_doesnt_plot_with_multiple_workspace_names(self, mock_plot_sample_container_and_components):
        """
        Sample material dialog should not open if multiple workspaces are selected.
        """
        self.ws_widget._show_sample_shape(self.ws_names)
        mock_plot_sample_container_and_components.assert_not_called()

    def test_empty_workspaces(self):
        self.ws_widget._ads.clear()
        self.assertEqual(self.ws_widget.empty_of_workspaces(), True)
        CreateSampleWorkspace(OutputWorkspace="ws")
        self.assertEqual(self.ws_widget.empty_of_workspaces(), False)


if __name__ == "__main__":
    unittest.main()
