# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.home_plot_widget.home_plot_widget_model import HomePlotWidgetModel
from mantid.simpleapi import CreateWorkspace


class HomeTabPlotModelTest(unittest.TestCase):
    def setUp(self):
        self.model = HomePlotWidgetModel()

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    @mock.patch('Muon.GUI.Common.home_plot_widget.home_plot_widget_model.plot')
    def test_plot_creates_new_plot_window_and_plots_workspace_list(self, mock_plot):

        workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA', 'MUSR62261; Group; bottom; Asymmetry; MA']
        workspace_object_list = [self.create_workspace(workspace) for workspace in workspace_list]
        subplot_title = 'MUSR62260 bottom'

        self.model.plot(workspace_list, subplot_title)

        mock_plot.assert_called_once_with(mock.ANY, spectrum_nums=[1], window_title=subplot_title)
        self.assertEqual(str(mock_plot.call_args[0][0][0]), str(workspace_object_list[0]))
        self.assertEqual(str(mock_plot.call_args[0][0][1]), str(workspace_object_list[1]))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
