# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.plotting_widget.plotting_widget_model import PlotWidgetModel
from mantid.simpleapi import CreateWorkspace


class PlottingWidgetModelTest(unittest.TestCase):
    def setUp(self):
        plot_figure = mock.MagicMock()
        self.model = PlotWidgetModel(plot_figure)

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    @mock.patch('Muon.GUI.Common.plotting_widget.plotting_widget_model.plot')
    def test_plot_plots_workspace_list(self, mock_plot):
        workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA', 'MUSR62261; Group; bottom; Asymmetry; MA']
        workspace_object_list = [self.create_workspace(workspace) for workspace in workspace_list]
        subplot_title = 'MUSR62260 bottom'

        self.model.plot(workspace_list, subplot_title, 'Time', 'Muon Analysis')

        mock_plot.assert_called_once_with(mock.ANY, wksp_indices=[0], window_title=subplot_title, errors=True,
                                          overplot=True,
                                          fig=self.model.plot_figure,
                                          plot_kwargs={'distribution': True, 'autoscale_on_update': False})

        self.assertEqual(str(mock_plot.call_args[0][0][0]), str(workspace_object_list[0]))
        self.assertEqual(str(mock_plot.call_args[0][0][1]), str(workspace_object_list[1]))

    def test_plot_logic_no_ws_to_plot(self):
        workspace_list = []
        subplot_title = 'MUSR62260 bottom'
        self.plotted_workspaces = ['ws1', 'ws2', 'ws3']

        self.model.plot(workspace_list, subplot_title, 'Time', 'Muon Analysis')

        self.assertEquals(self.model.plotted_workspaces, [])
        self.assertEquals(self.model.plotted_workspaces_inverse_binning, {})
        self.assertEquals(self.model.plotted_fit_workspaces, [])



if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
