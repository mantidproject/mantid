# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantid.py3compat.mock import patch

from Muon.GUI.Common.plotting_widget.plotting_widget_model import PlotWidgetModel
from mantid.simpleapi import *

tolerance = 1e-7


class PlottingWidgetModelTest(unittest.TestCase):
    def setUp(self):
        self.model = PlotWidgetModel()

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    @patch('mantid.plots.MantidAxes')
    def test_add_workspace_to_plot_calls_plotting_method(self, mock_axis):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        workspace_object = self.create_workspace(workspace)
        self.model._update_legend = mock.MagicMock()
        workspace_index = [0]
        errors = True
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label', 'wkspIndex': 0}

        self.model.add_workspace_to_plot(mock_axis, workspace, workspace_index, errors, plot_kwargs)

        mock_axis.errorbar.assert_called_once_with(mock.ANY, **plot_kwargs)
        # check correct workspace is plotted
        ws = workspace_object
        self.assertEqual(mock_axis.errorbar.call_args[0][0].name(), ws.name())
        self.assertTrue(ws.equals(mock_axis.errorbar.call_args[0][0], tolerance))

    @patch('mantid.plots.MantidAxes')
    def test_remove_workspace_from_plot_calls_remove_workspace_artists(self, mock_axis):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model.plotted_workspaces = [workspace]
        self.model.number_of_axes = 1
        workspace_object = self.create_workspace(workspace)
        self.model._update_legend = mock.MagicMock()

        self.model.remove_workspace_from_plot(workspace, [mock_axis])

        mock_axis.remove_workspace_artists.assert_called_once()
        self.assertTrue(mock_axis.remove_workspace_artists.call_args[0][0].equals(workspace_object, tolerance))
        self.assertEqual(self.model.plotted_workspaces, [])

    @patch('mantid.plots.MantidAxes')
    def test_remove_workspace_from_plot_calls_does_not_call_remove_artists_if_workspace_not_plotted(self, mock_axis):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model.plotted_workspaces = [workspace]
        self.model.number_of_axes = 1
        self.model._update_legend = mock.MagicMock()

        self.model.remove_workspace_from_plot('test', [mock_axis])

        mock_axis.remove_workspace_artists.assert_not_called()

    @patch('mantid.plots.MantidAxes')
    def test_workspace_deleted_from_ADS_removes_workspace_from_plot(self, mock_axis):
        workspace = 'MUSR62260; Group; bottom; Asymmetry; MA'
        self.model.plotted_workspaces = [workspace]
        self.model.number_of_axes = 1
        workspace_object = self.create_workspace(workspace)
        self.model._update_legend = mock.MagicMock()

        self.model.workspace_deleted_from_ads(workspace_object, [mock_axis])

        mock_axis.remove_workspace_artists.assert_called_once()
        self.assertTrue(mock_axis.remove_workspace_artists.call_args[0][0].equals(workspace_object, tolerance))
        self.assertEqual(self.model.plotted_workspaces, [])

    def test_plot_logic_no_ws_to_plot(self):
        workspace = ''
        plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': 'label'}
        errors = True
        axes = [mock.MagicMock()]

        self.model.plotted_workspaces = ['ws1', 'ws2', 'ws3']
        self.model.add_workspace_to_plot(axes, workspace, [0], errors, plot_kwargs)

        self.assertEquals(self.model.plotted_workspaces, ['ws1', 'ws2', 'ws3'])
        self.assertEquals(self.model.plotted_workspaces_inverse_binning, {})
        self.assertEquals(self.model.plotted_fit_workspaces, [])

    @mock.patch('Muon.GUI.Common.plotting_widget.plotting_widget_model.PlotWidgetModel.remove_workspace_from_plot')
    def test_clear_plot_model_correctly_clears_model(self, mock_remove_workspace):
        workspaces = ['test1', 'test2', 'test3']
        fit_workspaces = ['test4']
        axes = [mock.MagicMock()]
        self.model.plotted_workspaces = workspaces
        self.model.plotted_fit_workspaces = fit_workspaces

        self.model.clear_plot_model(axes)

        self.assertEqual(mock_remove_workspace.call_count, 4)
        call_list = [mock.call(workspaces[0], axes),
                     mock.call(workspaces[1], axes),
                     mock.call(workspaces[2], axes),
                     mock.call(fit_workspaces[0], axes)]

        mock_remove_workspace.assert_has_calls(call_list)
        self.assertEqual(self.model.plotted_workspaces, [])
        self.assertEqual(self.model.plotted_fit_workspaces, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
