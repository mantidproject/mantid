# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel, \
    WorkspacePlotInformation
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter import PlottingCanvasPresenter

from Muon.GUI.Common.contexts.muon_context import MUON_ANALYSIS_DEFAULT_Y_RANGE as  DEFAULT_Y_LIMITS
from Muon.GUI.Common.contexts.muon_context import MUON_ANALYSIS_DEFAULT_X_RANGE as  DEFAULT_X_LIMITS
from Muon.GUI.Common.contexts.plotting_context import PlottingContext

from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view_interface import PlottingCanvasViewInterface
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreateWorkspace, AnalysisDataService


def create_test_plot_information(name, index, axis, normalised, errors, label):
    return WorkspacePlotInformation(workspace_name=name, index=index, axis=axis,
                                    normalised=normalised,
                                    errors=errors, label=label)


def create_test_workspaces(ws_names):
    workspaces = []
    for name in ws_names:
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        workspaces += [CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)]
    return workspaces


@start_qapplication
class PlottingCanvasPresenterTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=PlottingCanvasViewInterface)
        self.model = mock.Mock(spec=PlottingCanvasModel)
        self.context = PlottingContext()
        self.context.set_defaults(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)
        self.figure_options = mock.Mock(spec=QuickEditPresenter)
        self.figure_options.get_selection.return_value = ['1']

        self.presenter = PlottingCanvasPresenter(self.view, self.model, self.figure_options, self.context)
        self.view.plotted_workspace_information = []

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_plot_workspaces_removes_workspaces_from_plot_if_hold_on_false(self):
        workspace_names = ["MUSR6220"]
        workspace_indices = [0]
        self.presenter._set_axes_limits_and_titles = mock.Mock()
        plotted_information = create_test_plot_information("MUSR6220;fwd", index=0, axis=0, normalised=False,
                                                           errors=False,
                                                           label='test')
        self.view.plotted_workspace_information = [plotted_information]
        self.model.create_workspace_plot_information.return_value = []

        self.presenter.plot_workspaces(workspace_names=workspace_names, workspace_indices=workspace_indices,
                                       hold_on=False, autoscale=False)

        self.view.remove_workspace_info_from_plot.assert_called_once_with([plotted_information])

    def test_plot_workspaces_does_not_replot_existing_workspaces(self):
        workspace_names = ["MUSR6220"]
        workspace_indices = [0]
        self.presenter._set_axes_limits_and_titles = mock.Mock()
        plotted_information = create_test_plot_information("MUSR6220;fwd", index=0, axis=0, normalised=False,
                                                           errors=False,
                                                           label='test')
        self.view.plotted_workspace_information = [plotted_information]
        self.model.create_workspace_plot_information.return_value = [plotted_information]

        self.presenter.plot_workspaces(workspace_names=workspace_names, workspace_indices=workspace_indices,
                                       hold_on=False, autoscale=False)

        self.view.add_workspaces_to_plot.assert_called_once_with([])

    def test_plot_workspaces_gets_workspace_info_from_model(self):
        workspace_names = ["MUSR6220"]
        workspace_indices = [0]
        self.presenter._set_axes_limits_and_titles = mock.Mock()
        plot_info = create_test_plot_information("MUSR6220", index=0, axis=0, normalised=False, errors=False,
                                                 label='test')
        self.model.create_workspace_plot_information.return_value = [plot_info]

        self.presenter.plot_workspaces(workspace_names=workspace_names, workspace_indices=workspace_indices,
                                       hold_on=False, autoscale=False)

        self.model.create_workspace_plot_information.assert_called_once()

    def test_plot_workspaces_adds_workspaces_to_plot(self):
        workspace_names = ["MUSR6220"]
        workspace_indices = [0]
        self.presenter._set_axes_limits_and_titles = mock.Mock()
        plot_info = create_test_plot_information("MUSR6220", index=0, axis=0, normalised=False, errors=False,
                                                 label='test')
        self.model.create_workspace_plot_information.return_value = [plot_info]

        self.presenter.plot_workspaces(workspace_names=workspace_names, workspace_indices=workspace_indices,
                                       hold_on=False, autoscale=False)

        self.view.add_workspaces_to_plot.assert_called_once_with([plot_info])

    def test_remove_workspace_names_from_plot_calls_passes_on_workspaces_to_view_correctly(self):
        ws_names = ["MUSR62260;fwd", "MUSR62260;bkwd"]
        workspaces = create_test_workspaces(ws_names)

        self.presenter.remove_workspace_names_from_plot(ws_names)

        self.assertEqual(self.view.remove_workspace_from_plot.call_count, len(ws_names))
        self.assertTrue(self.view.remove_workspace_from_plot.call_args_list[0][0][0].equals(workspaces[0], 1e-9))
        self.assertTrue(self.view.remove_workspace_from_plot.call_args_list[1][0][0].equals(workspaces[1], 1e-9))

    def test_convert_to_single_plot_correctly_sets_up_new_plot(self):
        workspaces = ["MUSR62260;fwd", "MUSR62260;bkwd"]
        indices = [0, 1]
        self.presenter.plot_workspaces = mock.Mock()
        self.view.plotted_workspaces_and_indices = [workspaces, indices]

        self.presenter.convert_plot_to_single_plot()

        self.view.create_new_plot_canvas.assert_called_once_with(1)
        self.presenter.plot_workspaces.assert_called_once_with(workspaces, indices,
                                                               hold_on=False, autoscale=False)

    def test_convert_to_tiled_plot_correctly_sets_up_new_plot(self):
        workspaces = ["MUSR62260;fwd", "MUSR62260;bkwd"]
        indices = [0, 1]
        keys = ["fwd", "bkwd"]
        tiled_by = "Group"
        self.presenter.plot_workspaces = mock.Mock()
        self.view.plotted_workspaces_and_indices = [workspaces, indices]

        self.presenter.convert_plot_to_tiled_plot(keys, tiled_by)

        self.view.create_new_plot_canvas.assert_called_once_with(len(keys))
        self.model.update_tiled_axis_map.assert_called_once_with(keys, tiled_by)
        self.presenter.plot_workspaces.assert_called_once_with(workspaces, indices,
                                                               hold_on=False, autoscale=False)

    def test_set_axes_limits_uses_default_limits_if_quick_edit_options_not_set(self):
        self.figure_options.get_plot_x_range.return_value = [0, 0]
        self.figure_options.get_plot_y_range.return_value = [0, 0]
        self.model.create_axes_titles.return_value = ["fwd"]
        self.presenter._update_quickedit_widget = mock.Mock()

        self.presenter._set_axes_limits_and_titles(False)

        self.view.set_axes_limits.assert_called_once_with(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)

    def test_set_axes_limits_updates_quickedit_widget(self):
        self.figure_options.get_plot_x_range.return_value = [0, 0]
        self.figure_options.get_plot_y_range.return_value = [0, 0]
        self.model.create_axes_titles.return_value = ["fwd"]
        self.presenter._update_quickedit_widget = mock.Mock()

        self.presenter._set_axes_limits_and_titles(False)

        self.presenter._update_quickedit_widget.assert_called_once()

    def test_update_quick_edit_widget_subplots_correctly_updates_from_view(self):
        number_of_axes = 2
        self.view.number_of_axes = number_of_axes

        self.presenter._update_quick_widget_subplots_menu()

        self.assertEqual(self.figure_options.add_subplot.call_count, number_of_axes)

    def test_handle_subplot_changed_updates_quickedit_widget_from_view(self):
        xlims = [0, 1]
        ylims = [-1, 1]
        self.view.get_axis_limits.return_value = [xlims[0], xlims[1], ylims[0], ylims[1]]
        self.model.create_axes_titles.return_value = ["fwd"]
        self.view.number_of_axes = 1

        self.presenter._handle_subplot_changed_in_quick_edit_widget()

        self.figure_options.set_plot_x_range.assert_called_once_with(xlims)
        self.figure_options.set_plot_y_range.assert_called_once_with(ylims)

    def test_create_tiled_plot_correctly_handles_empty_key_list(self):
        self.presenter.create_tiled_plot([], 'Group, Pair')

        self.view.create_new_plot_canvas.assert_called_once_with(1)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
