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

        self.model.plot(workspace_list, subplot_title, 'Time', False, 'Muon Analysis')

        mock_plot.assert_called_once_with(mock.ANY, wksp_indices=[0], window_title=subplot_title, errors=True,
                                          plot_kwargs={'distribution': True, 'autoscale_on_update': False})
        self.assertEqual(str(mock_plot.call_args[0][0][0]), str(workspace_object_list[0]))
        self.assertEqual(str(mock_plot.call_args[0][0][1]), str(workspace_object_list[1]))

    def test_plot_logic_no_plot(self):
        mock_plot = mock.MagicMock()
        self.model.plot_figure = mock_plot
        workspace_list = []
        subplot_title = 'MUSR62260 bottom'

        self.assertEquals(self.model.plot(workspace_list, subplot_title, 'Time', False, 'Muon Analysis'), mock_plot)
        self.assertEquals(self.model.plotted_workspaces, [])
        self.assertEquals(self.model.plotted_workspaces_inverse_binning, {})
        self.assertEquals(self.model.plotted_fit_workspaces, [])

    @mock.patch('Muon.GUI.Common.home_plot_widget.home_plot_widget_model.plot')
    def test_plot_logic_force_new(self, mock_plot):
        workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA', 'MUSR62261; Group; bottom; Asymmetry; MA']
        workspace_object_list = [self.create_workspace(workspace) for workspace in workspace_list]
        subplot_title = 'MUSR62260 bottom'
        self.model.set_x_lim = mock.Mock()
        self.model.plot(workspace_list, subplot_title, 'Time', False, 'Muon Analysis')

        mock_plot.assert_called_once_with(mock.ANY, wksp_indices=[0], window_title=subplot_title, errors=True,
                                          plot_kwargs={'distribution': True, 'autoscale_on_update': False})

        self.assertEquals(self.model.plot_figure.clear.call_count, 0)
        self.assertEquals(self.model.set_x_lim.call_count, 1)
        workspace_list = ['MUSR62260; Group; top; Asymmetry; MA', 'MUSR62261; Group; top; Asymmetry; MA']
        workspace_object_list = [self.create_workspace(workspace) for workspace in workspace_list]
        subplot_title = 'MUSR62260 top'

        self.model.plot(workspace_list, subplot_title, 'Time', True, 'Muon Analysis')
        self.assertEquals(self.model.plot_figure.clear.call_count, 1)
        self.assertEquals(self.model.set_x_lim.call_count, 2)

        self.assertEquals(mock_plot.call_count, 2)
        mock_plot.assert_any_call(mock.ANY, wksp_indices=[0], fig=self.model.plot_figure, window_title=subplot_title,
                                  plot_kwargs={'distribution': True, 'autoscale_on_update': False}, errors=True)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
