# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import unittest

from mantid.api import AnalysisDataService
from mantid.simpleapi import ConjoinWorkspaces, CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.workspacedisplay.matrix.presenter import MatrixWorkspaceDisplay

from qtpy.QtWidgets import QApplication


def create_test_workspace(workspace_name, number_of_spectra=2, ragged=False):
    workspace = CreateWorkspace([0, 1, 2, 3, 4, 2, 3, 4, 5, 6], [0, 1, 2, 3, 4, 3, 4, 5, 6, 7], NSpec=number_of_spectra)
    AnalysisDataService.addOrReplace(workspace_name, workspace)
    if ragged:
        for j in range(number_of_spectra):
            x, y = [], []
            k = 0
            for v in workspace.readX(j):
                if 1 <= v <= 6:
                    x.append(v)
                    y.append(workspace.readY(j)[k])
                k += 1
            tmp = CreateWorkspace(x, y)
            ConjoinWorkspaces(workspace_name, tmp, CheckOverlapping=False)
    return workspace


@start_qapplication
class WorkspaceDisplayViewTest(unittest.TestCase, QtWidgetFinder):
    @classmethod
    def setUpClass(cls):
        cls.normal_workspace = create_test_workspace("normal")
        cls.ragged_workspace = create_test_workspace("ragged", ragged=True)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_that_the_workspace_display_opens_and_closes_ok_with_a_normal_workspace(self):
        presenter = MatrixWorkspaceDisplay(self.normal_workspace)
        presenter.show_view()
        self.assert_widget_created()

        presenter.container.close()

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_that_the_workspace_display_opens_and_closes_ok_with_a_ragged_workspace(self):
        presenter = MatrixWorkspaceDisplay(self.ragged_workspace)
        presenter.show_view()
        self.assert_widget_created()

        presenter.container.close()

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()


if __name__ == '__main__':
    unittest.main()
