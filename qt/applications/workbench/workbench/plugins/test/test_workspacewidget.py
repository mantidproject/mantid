# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

import unittest
try:
    import mock
except ImportError:
    from unittest import mock

from qtpy.QtWidgets import QMainWindow, QApplication

from mantid.simpleapi import (CreateEmptyTableWorkspace, CreateWorkspace,
                              GroupWorkspaces)
from mantidqt.utils.qt.testing import GuiTest
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
import matplotlib as mpl
mpl.use('Agg')  # noqa
from workbench.plugins.workspacewidget import WorkspaceWidget


ALGORITHM_HISTORY_WINDOW = "mantidqt.widgets.workspacewidget." \
                           "algorithmhistorywindow.AlgorithmHistoryWindow"

app = QApplication([])


class WorkspaceWidgetTest(GuiTest, QtWidgetFinder):

    @classmethod
    def setUpClass(cls):
        cls.ws_widget = WorkspaceWidget(QMainWindow())
        cls.ahw_type_name = 'AlgorithmHistoryWindow'
        mat_ws = CreateWorkspace([1], [2])
        table_ws = CreateEmptyTableWorkspace()
        group_ws = GroupWorkspaces([mat_ws, table_ws])
        cls.w_spaces = [mat_ws, table_ws, group_ws]
        cls.ws_names = ['MatWS', 'TableWS', 'GroupWS']
        for ws_name, ws in zip(cls.ws_names, cls.w_spaces):
            cls.ws_widget._ads.add(ws_name, ws)

    def test_algorithm_history_window_opens_with_workspace(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[0]])
        self.assert_widget_type_exists(self.ahw_type_name)

    def test_algorithm_history_window_doesnt_open_with_workspace_group(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history([self.ws_names[2]])
        self.assert_widget_type_doesnt_exist(self.ahw_type_name)

    def test_algorithm_history_window_opens_multiple(self):
        with mock.patch(ALGORITHM_HISTORY_WINDOW + '.show', lambda x: None):
            self.ws_widget._do_show_algorithm_history(self.ws_names)
        self.assert_number_of_widgets_matching(self.ahw_type_name, 2)


if __name__ == '__main__':
    unittest.main()
