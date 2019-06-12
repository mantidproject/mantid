# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantid.simpleapi import CreateWorkspace


class HomeTabPlotModelTest(unittest.TestCase):
    def setUp(self):
        self.mock_plot_window = mock.MagicMock()
        self.plotting_manager = mock.MagicMock(return_value=self.mock_plot_window)
        self.context = mock.MagicMock()
        self.model = HomePlotWidgetModel(self.plotting_manager)

    def create_workspace(self):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return MuonWorkspaceWrapper(CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace='OutputWorkspace', StoreInADS=False), 'workspace_name')


    def test_plot_creates_new_plot_window_and_plots_workspace_list(self):
        workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA', 'MUSR62261; Group; bottom; Asymmetry; MA']
        subplot_title = 'MUSR62260 bottom'

        self.model.plot(workspace_list, subplot_title)

        self.plotting_manager.assert_called_once_with('Muon Analysis', close_callback=self.model._close_plot)

    def test_create_new_plot_adds_workspaces_in_workspace_list(self):
        workspace = self.create_workspace()

        plot_window = self.model.plot([workspace], 'Test plot')

        self.assertTrue(plot_window.multi_plot.has_subplot('Test plot'))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
