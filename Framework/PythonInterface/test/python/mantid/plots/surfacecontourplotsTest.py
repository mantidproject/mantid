# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.api import AnalysisDataService, MatrixWorkspace
from mantid.plots.surfacecontourplots import _create_workspace_for_group_plot
from mantid.simpleapi import CreateEmptyTableWorkspace, CreateWorkspace
from mantidqt.dialogs.spectraselectordialog import SpectraSelection


class SurfaceContourPlotsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.workspaces = []
        for i in range(1, 4):
            ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                 DataY=[2, 3, 4, 5, 3, 5],
                                 DataE=[1, 2, 3, 4, 1, 1],
                                 NSpec=3,
                                 OutputWorkspace=f'ws{i}')
            cls.workspaces.append(ws)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.Instance().clear()

    def test_create_workspace_for_group_plot_returns_matrix_workspace(self):
        ws = _create_workspace_for_group_plot(plot_type=SpectraSelection.Contour,
                                              workspaces=self.workspaces,
                                              plot_index=0,
                                              log_name="Workspace index",
                                              custom_log_values=[])

        self.assertTrue(isinstance(ws, MatrixWorkspace))

    def test_create_workspace_for_group_plot_raises_error_if_list_of_workspaces_is_empty(self):
        self.assertRaises(RuntimeError, _create_workspace_for_group_plot,
                          plot_type=SpectraSelection.Contour,
                          workspaces=[],
                          plot_index=0,
                          log_name="Workspace index",
                          custom_log_values=[])

    def test_create_workspace_for_group_plot_raises_error_if_workspaces_have_different_x_data(self):
        ws = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30, 40, 50, 60],
                             DataY=[2, 3, 4, 5, 3, 5, 6, 2, 3],
                             DataE=[1, 2, 3, 4, 1, 1, 2, 3, 1],
                             NSpec=3)

        workspaces = self.workspaces + [ws]

        self.assertRaises(RuntimeError, _create_workspace_for_group_plot,
                          plot_type=SpectraSelection.Contour,
                          workspaces=workspaces,
                          plot_index=0,
                          log_name="Workspace index",
                          custom_log_values=[])

    def test_create_workspace_for_group_plot_raises_error_if_group_contains_non_matrix_workspaces(self):
        table = CreateEmptyTableWorkspace()

        workspaces = self.workspaces + [table]

        self.assertRaises(RuntimeError, _create_workspace_for_group_plot,
                          plot_type=SpectraSelection.Contour,
                          workspaces=workspaces,
                          plot_index=0,
                          log_name="Workspace index",
                          custom_log_values=[])

    def test_create_workspace_for_group_plot_raises_error_when_workspace_index_is_too_high(self):
        self.assertRaises(RuntimeError, _create_workspace_for_group_plot,
                          plot_type=SpectraSelection.Contour,
                          workspaces=self.workspaces,
                          plot_index=5,
                          log_name="Workspace index",
                          custom_log_values=[])


if __name__ == '__main__':
    unittest.main()
