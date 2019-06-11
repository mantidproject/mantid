# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import GuiTest
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


class HomePlotWidgetModelTest(GuiTest):
    def setUp(self):
        self.model = HomePlotWidgetModel(MultiPlotWindow)

    def create_workspace(self):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return MuonWorkspaceWrapper(CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace='OutputWorkspace', StoreInADS=False), 'workspace_name')

    def test_create_new_plot_creates_new_plot_with_correct_title(self):
        workspace = self.create_workspace()

        plot_window = self.model.plot([workspace], 'subplot_name')

        self.assertEqual(type(plot_window), MultiPlotWindow)
        self.assertEqual(plot_window.window.windowTitle(), 'Muon Analysis')

    def test_create_new_plot_adds_workspaces_in_workspace_list(self):
        workspace = self.create_workspace()

        plot_window = self.model.plot([workspace], 'Test plot')

        self.assertTrue(plot_window.multi_plot.has_subplot('Test plot'))


if __name__ == '__main__':
    unittest.main()
