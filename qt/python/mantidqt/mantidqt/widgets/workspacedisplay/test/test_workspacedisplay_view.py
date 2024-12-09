# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest
from unittest import mock

from mantid.api import AnalysisDataService
from mantid.simpleapi import ConjoinWorkspaces, CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay

from qtpy.QtWidgets import QApplication

# Import sip after Qt. Modern versions of PyQt ship an internal sip module
# located at PyQt5X.sip. Importing PyQt first sets a shim sip module to point
# to the correct place
import sip


def create_test_workspace(workspace_name, ragged=False):
    CreateWorkspace([0, 1, 2, 3, 4, 2, 3, 4, 5, 6], [0, 1, 2, 3, 4, 3, 4, 5, 6, 7], NSpec=2, OutputWorkspace=workspace_name)
    if ragged:
        CreateWorkspace([1, 2, 3, 4], [1, 2, 3, 4], NSpec=1, OutputWorkspace="__temp1")
        CreateWorkspace([2, 3, 4, 5, 6], [3, 4, 5, 6, 7], NSpec=1, OutputWorkspace="__temp2")
        ConjoinWorkspaces(workspace_name, "__temp1", CheckOverlapping=False, CheckMatchingBins=False)
        ConjoinWorkspaces(workspace_name, "__temp2", CheckOverlapping=False, CheckMatchingBins=False)
    return AnalysisDataService.retrieve(workspace_name)


@start_qapplication
class WorkspaceDisplayViewTest(unittest.TestCase, QtWidgetFinder):
    @classmethod
    def setUpClass(cls):
        cls.workspace = create_test_workspace("non-ragged")
        cls.ragged_workspace = create_test_workspace("ragged", ragged=True)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def tearDown(self):
        if not sip.isdeleted(self.presenter.container):
            self.presenter.container.close()
            QApplication.sendPostedEvents()

    def test_that_the_workspace_display_opens_and_closes_ok_with_a_non_ragged_workspace(self):
        self.presenter = MatrixWorkspaceDisplay(self.workspace)
        self.presenter.show_view()

        self.assert_widget_created()

        self.presenter.container.close()
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_that_the_workspace_display_opens_and_closes_ok_with_a_ragged_workspace(self):
        self.presenter = MatrixWorkspaceDisplay(self.ragged_workspace)
        self.presenter.show_view()

        self.assert_widget_created()

        self.presenter.container.close()
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_that_copy_spectrum_to_table_does_not_cause_an_error_with_a_non_ragged_workspace(self):
        with mock.patch("qtpy.QtCore.QItemSelectionModel.selectedRows") as patch:
            self.presenter = MatrixWorkspaceDisplay(self.workspace)
            self.presenter.show_view()
            table = self.presenter.view.currentWidget()

            patch.return_value = [table.model().index(0, 0)]

            self.presenter.action_copy_spectrum_to_table(table)

    def test_that_copy_spectrum_to_table_does_not_cause_an_error_with_a_ragged_workspace(self):
        with mock.patch("qtpy.QtCore.QItemSelectionModel.selectedRows") as patch:
            self.presenter = MatrixWorkspaceDisplay(self.ragged_workspace)
            self.presenter.show_view()
            table = self.presenter.view.currentWidget()

            patch.return_value = [table.model().index(0, 0), table.model().index(1, 0), table.model().index(2, 0)]

            self.presenter.action_copy_spectrum_to_table(table)


if __name__ == "__main__":
    unittest.main()
