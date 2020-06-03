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

import matplotlib as mpl
mpl.use('Agg')  # noqa

from unittest import mock
from mantid.simpleapi import (CreateEmptyTableWorkspace, CreateSampleWorkspace,
                              GroupWorkspaces)
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from qtpy.QtWidgets import QMainWindow, QApplication
from workbench.plugins.workspacewidget import WorkspaceWidget
from mantid.plots.utility import MantidAxType

ALGORITHM_HISTORY_WINDOW_TYPE = "AlgorithmHistoryWindow"
ALGORITHM_HISTORY_WINDOW = "mantidqt.widgets.workspacewidget." \
                           "algorithmhistorywindow." + ALGORITHM_HISTORY_WINDOW_TYPE
MATRIXWORKSPACE_DISPLAY = "mantidqt.widgets.workspacedisplay.matrix." \
                          "presenter.MatrixWorkspaceDisplay"
MATRIXWORKSPACE_DISPLAY_TYPE = "StatusBarView"
app = QApplication([])


@start_qapplication
class WorkspaceWidgetTest(unittest.TestCase, QtWidgetFinder):

    @classmethod
    def setUpClass(cls):
        cls.ws_widget = WorkspaceWidget(QMainWindow())
        mat_ws = CreateSampleWorkspace()
        table_ws = CreateEmptyTableWorkspace()
        group_ws = GroupWorkspaces([mat_ws, table_ws])
        cls.w_spaces = [mat_ws, table_ws, group_ws]
        cls.ws_names = ['MatWS', 'TableWS', 'GroupWS']
        for ws_name, ws in zip(cls.ws_names, cls.w_spaces):
            cls.ws_widget._ads.add(ws_name, ws)

    def test_algorithm_history_window_opens_with_workspace(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[0]])
        self.assert_widget_type_exists(ALGORITHM_HISTORY_WINDOW_TYPE)

    def test_algorithm_history_window_doesnt_open_with_workspace_group(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[2]])
        self.assert_widget_type_doesnt_exist(ALGORITHM_HISTORY_WINDOW_TYPE)

    def test_algorithm_history_window_opens_multiple(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history(self.ws_names)
        self.assert_number_of_widgets_matching(ALGORITHM_HISTORY_WINDOW_TYPE, 2)

    def test_detector_table_shows_with_workspace(self):
        with mock.patch(MATRIXWORKSPACE_DISPLAY + '.show_view', lambda x: None):
            self.ws_widget._do_show_detectors([self.ws_names[0]])
        self.assert_widget_type_exists(MATRIXWORKSPACE_DISPLAY_TYPE)

    @mock.patch('workbench.plugins.workspacewidget.plot', autospec=True)
    def test_plot_with_plot_bin(self, mock_plot):
        self.ws_widget._do_plot_bin([self.ws_names[0]], False, False)
        mock_plot.assert_called_once_with(mock.ANY,errors=False, overplot=False, wksp_indices=[0],
                                          plot_kwargs={'axis': MantidAxType.BIN})

    @mock.patch('workbench.plugins.workspacewidget.plot_from_names', autospec=True)
    def test_plot_with_plot_spectrum(self, mock_plot_from_names):
        self.ws_widget._do_plot_spectrum([self.ws_names[0]], False, False)
        mock_plot_from_names.assert_called_once_with([self.ws_names[0]], False, False, advanced=False)

    @mock.patch('workbench.plugins.workspacewidget.pcolormesh', autospec=True)
    def test_plot_with_plot_colorfill(self, mock_plot_colorfill):
        self.ws_widget._do_plot_colorfill([self.ws_names[0]])
        mock_plot_colorfill.assert_called_once_with(mock.ANY)

    @mock.patch('workbench.plugins.workspacewidget.plot_from_names', autospec=True)
    def test_plot_with_plot_advanced(self, mock_plot_from_names):
        self.ws_widget._do_plot_spectrum([self.ws_names[0]], False, False, advanced=True)
        mock_plot_from_names.assert_called_once_with([self.ws_names[0]], False, False, advanced=True)

    @mock.patch('mantidqt.plotting.functions.plot_contour', autospec=True)
    def test_plot_with_plot_contour(self, mock_plot_contour):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type='contour')
        mock_plot_contour.assert_called_once_with([self.ws_names[0]])

    @mock.patch('mantidqt.plotting.functions.plot_surface', autospec=True)
    def test_plot_with_plot_surface(self, mock_plot_surface):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type='surface')
        mock_plot_surface.assert_called_once_with([self.ws_names[0]])

    @mock.patch('mantidqt.plotting.functions.plot_wireframe', autospec=True)
    def test_plot_with_plot_wireframe(self, mock_plot_wireframe):
        self.ws_widget._do_plot_3D([self.ws_names[0]], plot_type='wireframe')
        mock_plot_wireframe.assert_called_once_with([self.ws_names[0]])


if __name__ == '__main__':
    unittest.main()
