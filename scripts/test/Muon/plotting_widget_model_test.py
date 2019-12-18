# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from Muon.GUI.Common.plotting_widget.plotting_widget_model import PlotWidgetModel
from mantid.simpleapi import *

tolerance = 1e-7


class PlottingWidgetModelTest(unittest.TestCase):
    def setUp(self):
        self.model = PlotWidgetModel()
        self.axis = mock.MagicMock()

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    @mock.patch('Muon.GUI.Common.plotting_widget.plotting_widget_model.PlotWidgetModel._do_single_plot')
    def test_add_workspace_to_plot_calls_plotting_method(self, mock_single_plot):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        workspace_object = self.create_workspace(workspace)
        self.model._update_legend = mock.MagicMock()
        workspace_index = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}
        # add workspace to the plot
        self.model.add_workspace_to_plot(self.axis, workspace, workspace_index, errors, plot_kwargs)

        mock_single_plot.assert_called_once_with(self.axis, mock.ANY, workspace_index, True,
                                                 plot_kwargs)

        # check correct workspace is plotted
        ws = workspace_object
        self.assertEqual(mock_single_plot.call_args[0][1][0].name(), ws.name())
        self.assertTrue(ws.equals(mock_single_plot.call_args[0][1][0], tolerance))

    def test_remove_workspace_from_plot(self):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model.plotted_workspaces = [workspace]
        self.model.number_of_axes = 1
        self.axis[0].remove_workspace_artist = mock.MagicMock()
        workspace_object = self.create_workspace(workspace)
        self.model._update_legend = mock.MagicMock()

        self.model.remove_workspace_from_plot(workspace, [self.axis])

        self.axis[0].remove_workspace_artist.assert_called_once_with(workspace_object)

    # test workspace removed

    # test workspace deleted

    def test_plot_logic_no_ws_to_plot(self):
        workspace = ''
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}
        errors = True
        self.plotted_workspaces = ['ws1', 'ws2', 'ws3']
        self.model.add_workspace_to_plot(self.axis, workspace, [0], errors, plot_kwargs)

        self.assertEquals(self.model.plotted_workspaces, [])
        self.assertEquals(self.model.plotted_workspaces_inverse_binning, {})
        self.assertEquals(self.model.plotted_fit_workspaces, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
