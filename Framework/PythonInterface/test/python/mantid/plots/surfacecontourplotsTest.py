# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantid.api import MatrixWorkspace
from mantid.plots.surfacecontourplots import _create_workspace_for_group_plot
from mantid.simpleapi import CreateWorkspace
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

    def test_create_workspace_for_group_plot_returns_matrix_workspace(self):
        ws = _create_workspace_for_group_plot(plot_type=SpectraSelection.Contour,
                                              workspaces=self.workspaces,
                                              plot_index=0,
                                              log_name="Workspace index",
                                              custom_log_values=[])

        self.assertTrue(isinstance(ws, MatrixWorkspace))