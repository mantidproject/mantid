# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel, \
    WorkspacePlotInformation
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter import PlottingCanvasPresenter
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import QuickEditWidget

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


DEFAULT_X_LIMITS = [0.,15.]
DEFAULT_Y_LIMITS = [-1.,1.]


@start_qapplication
class PlottingCanvasPresenterTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=PlottingCanvasViewInterface)
        self.model = mock.Mock(spec=PlottingCanvasModel)
        self.options = mock.Mock(spec=QuickEditWidget)
        self.context = PlottingContext()
        self.context.set_defaults(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)
        self.presenter = PlottingCanvasPresenter(self.view, self.model, self.options, self.context)
        self._mock_y_min = 2

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def get_mock_y_limits(self, axis_number):
        self._mock_y_min += 1.
        return axis_number, axis_number, self._mock_y_min, self._mock_y_min+5.

    def test_force_autoscale(self):
        subplots = ["fwd", "bwd"]
        axes = [0, 1]
        self.view.get_axis_limits = mock.Mock(side_effect = self.get_mock_y_limits)
        self.presenter.update_y_limits_of_subplots = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(subplots, axes))

        self.presenter.force_autoscale()

        expected_dict = {"fwd": [3., 8.], "bwd": [4,9]}
        self.presenter.update_y_limits_of_subplots.assert_called_once_with(expected_dict, subplots, axes)

    def test_update_y_limits_of_subplots(self):
        subplots = ["fwd", "bwd", "top"]
        y_limits = {"fwd": [3., 8.], "bwd": [4,9], "top":[-1,5]}
        axes = [0, 1, 2]
        self.context.settings.set_condensed(False)
        self.view.set_axis_ylimits = mock.Mock()
        self.presenter._options_presenter.set_plot_y_range = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()
        self.presenter.should_update_all = mock.Mock(return_value = True)

        self.presenter.update_y_limits_of_subplots(y_limits, subplots, axes)
        self.assertEqual(self.view.set_axis_ylimits.call_count, len(subplots))
        self.view.set_axis_ylimits.assert_any_call(0, [3,8])
        self.view.set_axis_ylimits.assert_any_call(1, [4,9])
        self.view.set_axis_ylimits.assert_any_call(2, [-1,5])

        self.assertEqual(self.context.update_ylim.call_count, len(subplots))
        for plot in y_limits.keys():
            self.context.update_ylim.assert_any_call(plot, y_limits[plot])

        # the smallest and largest y values from any subplot
        global_limits = [-1, 9]
        self.context.update_ylim_all.assert_called_once_with(global_limits)
        self.presenter._options_presenter.set_plot_y_range.assert_called_once_with(global_limits)
        self.view.redraw_figure.assert_called_once_with()

    def test_update_y_limits_of_subplots_condensed(self):
        subplots = ["fwd", "bwd", "top"]
        y_limits = {"fwd": [3., 8.], "bwd": [4,9], "top":[-1,5]}
        # max and min y values from any plot
        global_limits = [-1, 9]
        axes = [0, 1, 2]
        # want everything on the exact same scale
        self.context.settings.set_condensed(True)
        self.view.set_axis_ylimits = mock.Mock()
        self.presenter._options_presenter.set_plot_y_range = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()
        self.presenter.should_update_all = mock.Mock(return_value = True)

        self.presenter.update_y_limits_of_subplots(y_limits, subplots, axes)
        self.assertEqual(self.view.set_axis_ylimits.call_count, len(subplots))
        self.view.set_axis_ylimits.assert_any_call(0, global_limits)
        self.view.set_axis_ylimits.assert_any_call(1, global_limits)
        self.view.set_axis_ylimits.assert_any_call(2, global_limits)

        self.assertEqual(self.context.update_ylim.call_count, len(subplots))
        for plot in y_limits.keys():
            self.context.update_ylim.assert_any_call(plot, global_limits)

        self.context.update_ylim_all.assert_called_once_with(global_limits)
        self.presenter._options_presenter.set_plot_y_range.assert_called_once_with(global_limits)
        self.view.redraw_figure.assert_called_once_with()

    def test_update_y_limits_of_subplots_bad_inputs(self):
        subplots = ["fwd", "bwd", "top"]
        y_limits = {"fwd": [3., 8.], "bwd": [4,9], "top":[-1,5]}
        axes = [0, 1, 2]
        self.context.settings.set_condensed(False)
        self.view.set_axis_ylimits = mock.Mock()
        self.presenter._options_presenter.set_plot_y_range = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()
        self.presenter.should_update_all = mock.Mock(return_value = True)

        no_limits = [[], subplots, axes]
        no_subplots = [y_limits, [], axes]
        no_axes = [y_limits, subplots, []]
        all_empty = [[], [], []]

        args_list = [no_limits, no_subplots, no_axes, all_empty]
        for args in args_list:
            self.presenter.update_y_limits_of_subplots(args[0], args[1], args[2])

            self.view.set_axis_ylimits.assert_not_called()
            self.context.update_ylim.assert_not_called()
            self.context.update_ylim_all.assert_not_called()
            self.presenter._options_presenter.set_plot_y_range.assert_not_called()
            self.view.redraw_figure.assert_not_called()

    def test_set_quickedit_from_context(self):
        state = True
        ylims = [0,10]
        xlims = [5, 15]
        self.context.set_error_all(state)
        self.context.set_autoscale_all(state)
        self.context.update_ylim_all(ylims)
        self.context.update_xlim_all(xlims)

        self.options.set_autoscale = mock.Mock()
        self.options.set_errors = mock.Mock()
        self.options.set_plot_x_range = mock.Mock()
        self.options.set_plot_y_range = mock.Mock()

        self.presenter.set_quickedit_from_context()
        self.options.set_autoscale.assert_called_with(state)
        self.options.set_errors.assert_called_with(state)
        self.options.set_plot_x_range.assert_called_with(xlims)
        self.options.set_plot_y_range.assert_called_with(ylims)

    def test_plot_workspaces_removes_workspace_from_plot_if_hold_on_false(self):
        ws_names = ["MUSR6220"]
        ws_indices = [0]
        ws_plot_info = create_test_plot_information(name="MUSR62260; Group; fwd", index=0, axis=0,
                                                                  normalised=False, errors=False, label="MUSR62260;fwd")
        self.view.plotted_workspace_information = [ws_plot_info]
        self.model.create_workspace_plot_information = mock.Mock(return_value=[])
        self.presenter._set_axes_limits_and_titles = mock.Mock()

        self.presenter.plot_workspaces(workspace_names=ws_names, workspace_indices=ws_indices,
                                       hold_on=False, autoscale=False)

        self.view.remove_workspace_info_from_plot.assert_called_once_with([ws_plot_info])
        self.model.create_workspace_plot_information.assert_called_once()

    def test_plot_workspaces_does_not_replot_existing_workspaces(self):
        ws_names = ["MUSR6220"]
        ws_indices = [0]
        ws_plot_info = create_test_plot_information(name="MUSR62260; Group; fwd", index=0, axis=0, normalised=False,
                                                    errors=False, label="MUSR62260;fwd")
        self.view.plotted_workspace_information = [ws_plot_info]
        self.model.create_workspace_plot_information = mock.Mock(return_value=[ws_plot_info])
        self.presenter._set_axes_limits_and_titles = mock.Mock()

        self.presenter.plot_workspaces(workspace_names=ws_names, workspace_indices=ws_indices,
                                       hold_on=False, autoscale=False)

        self.view.add_workspaces_to_plot.assert_called_once_with([])
        self.model.create_workspace_plot_information.assert_called_once()

    def test_plot_workspaces_adds_workspaces_to_plot(self):
        ws_names = ["MUSR6220"]
        ws_indices = [0]
        ws_plot_info = create_test_plot_information(name="MUSR62260; Group; fwd", index=0, axis=0, normalised=False,
                                                    errors=False, label="MUSR62260;fwd")
        self.view.plotted_workspace_information = []
        self.model.create_workspace_plot_information = mock.Mock(return_value=[ws_plot_info])
        self.presenter._set_axes_limits_and_titles = mock.Mock()

        self.presenter.plot_workspaces(workspace_names=ws_names, workspace_indices=ws_indices,
                                       hold_on=False, autoscale=False)

        self.view.add_workspaces_to_plot.assert_called_once_with([ws_plot_info])
        self.model.create_workspace_plot_information.assert_called_once()

    def test_remove_workspace_names_from_plot_calls_passes_on_workspaces_to_view_correctly(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        workspaces = create_test_workspaces(ws_names)

        self.presenter.remove_workspace_names_from_plot(ws_names)

        self.assertEqual(len(ws_names), self.view.remove_workspace_from_plot.call_count)
        self.assertTrue(self.view.remove_workspace_from_plot.call_args_list[0][0][0].equals(workspaces[0], 1e-9))
        self.assertTrue(self.view.remove_workspace_from_plot.call_args_list[1][0][0].equals(workspaces[1], 1e-9))

    def test_convert_to_single_plot_correctly_sets_up_new_plot(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        self.view.plotted_workspaces_and_indices = [ws_names, ws_indices]
        self.view.create_new_plot_canvas = mock.Mock()
        self.presenter.clear_subplots = mock.Mock()
        self.presenter.plot_workspaces = mock.Mock()
        self.options.add_subplot = mock.Mock()
        self.context.update_axis = mock.Mock()

        self.presenter.convert_plot_to_single_plot()

        self.assertEqual(False, self.model.is_tiled)
        self.presenter.clear_subplots.assert_called_once()
        self.options.add_subplot.assert_called_once_with("one")
        self.context.update_axis.assert_called_once_with("one", 0)
        self.view.create_new_plot_canvas.assert_called_once_with(1)
        self.presenter.plot_workspaces.assert_called_once_with(ws_names, ws_indices, hold_on=False, autoscale=False)

    def test_convert_to_tiled_plot_correctly_sets_up_new_plot(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        keys = ["fwd", "bwd"]
        self.presenter.plot_workspaces = mock.Mock()
        self.presenter.clear_subplots = mock.Mock()
        self.view.plotted_workspaces_and_indices = [ws_names, ws_indices]

        self.presenter.convert_plot_to_tiled_plot(keys)

        self.view.create_new_plot_canvas.assert_called_once_with(len(keys))
        self.model.update_tiled_axis_map.assert_called_once_with(keys)
        self.presenter.clear_subplots.assert_called_once()
        self.presenter.plot_workspaces.assert_called_once_with(ws_names, ws_indices, hold_on=False, autoscale=False)

    def test_handle_xlim_changed_for_all(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        xlims = [-1, 1]
        self.view.autoscale_state = False
        self.view.set_axis_xlimits = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.update_xlim_all = mock.Mock()
        self.context.update_xlim = mock.Mock()
        set_axis_xlimits_calls = [mock.call(ws_indices[0], xlims), mock.call(ws_indices[1], xlims)]
        update_xlim_calls = [mock.call(ws_names[0], xlims), mock.call(ws_names[1], xlims)]

        self.presenter._handle_xlim_changed_in_quick_edit_options(xlims)

        self.context.update_xlim_all.assert_called_once_with(xlims)
        self.assertEqual(len(ws_names), self.view.set_axis_xlimits.call_count)
        self.assertEqual(set_axis_xlimits_calls, self.view.set_axis_xlimits.call_args_list)
        self.assertEqual(len(ws_names), self.context.update_xlim.call_count)
        self.assertEqual(update_xlim_calls, self.context.update_xlim.call_args_list)
        self.view.redraw_figure.assert_called_once()

    def test_handle_xlim_change_for_specific_subplot(self):
        ws_names = ["MUSR62260; Group; fwd"]
        ws_indices = [0]
        xlims = [-1, 1]
        self.view.autoscale_state = False
        self.view.set_axis_xlimits = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.update_xlim_all = mock.Mock()
        self.context.update_xlim = mock.Mock()
        set_axis_xlimits_calls = [mock.call(ws_indices[0], xlims)]
        update_xlim_calls = [mock.call(ws_names[0], xlims)]

        self.presenter._handle_xlim_changed_in_quick_edit_options(xlims)

        self.context.update_xlim_all.assert_not_called()
        self.assertEqual(len(ws_names), self.view.set_axis_xlimits.call_count)
        self.assertEqual(set_axis_xlimits_calls, self.view.set_axis_xlimits.call_args_list)
        self.assertEqual(len(ws_names), self.context.update_xlim.call_count)
        self.assertEqual(update_xlim_calls, self.context.update_xlim.call_args_list)
        self.view.redraw_figure.assert_called_once()

    def test_handle_ylim_changed_for_all(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        ylims = [-1, 1]
        self.view.set_axis_ylimits = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.update_ylim_all = mock.Mock()
        self.context.update_ylim = mock.Mock()
        set_axis_ylimits_calls = [mock.call(ws_indices[0], ylims), mock.call(ws_indices[1], ylims)]
        update_ylim_calls = [mock.call(ws_names[0], ylims), mock.call(ws_names[1], ylims)]

        self.presenter._handle_ylim_changed_in_quick_edit_options(ylims)

        self.context.update_ylim_all.assert_called_once_with(ylims)
        self.assertEqual(len(ws_names), self.view.set_axis_ylimits.call_count)
        self.assertEqual(set_axis_ylimits_calls, self.view.set_axis_ylimits.call_args_list)
        self.assertEqual(len(ws_names), self.context.update_ylim.call_count)
        self.assertEqual(update_ylim_calls, self.context.update_ylim.call_args_list)
        self.view.redraw_figure.assert_called_once()

    def test_handle_ylim_change_for_specific_subplot(self):
        ws_names = ["MUSR62260; Group; fwd"]
        ws_indices = [0]
        ylims = [-1, 1]
        self.view.set_axis_ylimits = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.update_ylim_all = mock.Mock()
        self.context.update_ylim = mock.Mock()
        set_axis_ylimits_calls = [mock.call(ws_indices[0], ylims)]
        update_ylim_calls = [mock.call(ws_names[0], ylims)]

        self.presenter._handle_ylim_changed_in_quick_edit_options(ylims)

        self.context.update_ylim_all.assert_not_called()
        self.assertEqual(len(ws_names), self.view.set_axis_ylimits.call_count)
        self.assertEqual(set_axis_ylimits_calls, self.view.set_axis_ylimits.call_args_list)
        self.assertEqual(len(ws_names), self.context.update_ylim.call_count)
        self.assertEqual(update_ylim_calls, self.context.update_ylim.call_args_list)
        self.view.redraw_figure.assert_called_once()

    def test_update_range_specific_plot(self):
        xlims = [-1, 1]
        ylims = [-2, 2]
        self.view.get_xlim_list = xlims
        self.view.get_ylim_list = ylims
        self.presenter._handle_xlim_changed_in_quick_edit_options = mock.Mock()
        self.presenter._handle_ylim_changed_in_quick_edit_options = mock.Mock()
        self.options.get_selection_index = mock.Mock(return_value=0)
        self.options.set_selection_by_index = mock.Mock()
        self.options.get_selection = mock.Mock(return_value=["MUSR62260; Group; fwd"])
        self.options.set_plot_x_range = mock.Mock()
        self.options.set_plot_y_range = mock.Mock()
        self.options.connect_x_range_changed(self.presenter._handle_xlim_changed_in_quick_edit_options)
        self.context.get_xlim = mock.Mock(return_value=xlims)
        self.context.get_ylim = mock.Mock(return_value=ylims)

        self.presenter.update_range()

        self.options.set_plot_x_range.assert_called_once_with(xlims)
        self.options.set_plot_y_range.assert_called_once_with(ylims)

    def test_handle_autoscale_y_axes_all(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        ylims = [-1, 1]
        self.view.get_axis_limits = mock.Mock(return_value=(0, 0, -1, 1))
        self.view.autoscale_selected_y_axis = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.options.autoscale = True
        self.options.set_plot_y_range = mock.Mock()
        self.context.update_autoscale_state = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()
        update_ylims_calls = [mock.call(ws_names[0], ylims), mock.call(ws_names[1], ylims)]

        self.presenter._handle_autoscale_y_axes()

        self.assertEqual(len(ws_names), self.context.update_autoscale_state.call_count)
        self.assertEqual(len(ws_names), self.context.update_ylim.call_count)
        self.context.update_ylim.assert_has_calls(update_ylims_calls)
        self.context.update_ylim_all.assert_called_once_with(ylims)
        self.options.set_plot_y_range.assert_called_once_with(ylims)
        self.view.redraw_figure.assert_called_once()

    def test_handle_autoscale_y_axis_specific_subplot(self):
        ws_names = ["MUSR62260; Group; fwd"]
        ws_indices = [0]
        ylims = [-1, 1]
        self.view.get_axis_limits = mock.Mock(return_value=(0, 0, -1, 1))
        self.view.autoscale_selected_y_axis = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.options.autoscale = True
        self.options.set_plot_y_range = mock.Mock()
        self.context.update_autoscale_state = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()

        self.presenter._handle_autoscale_y_axes()

        self.context.update_autoscale_state.assert_called_once_with(ws_names[0], True)
        self.context.update_ylim.assert_called_once_with(ws_names[0], ylims)
        self.context.update_ylim_all.assert_not_called()
        self.options.set_plot_y_range.assert_called_once_with(ylims)
        self.view.redraw_figure.assert_called_once()

    def test_handle_autoscale_y_axis_false(self):
        ws_names = ["MUSR62260; Group; fwd"]
        ws_indices = [0]
        self.view.get_axis_limits = mock.Mock(return_value=(0, 0, -1, 1))
        self.view.autoscale_selected_y_axis = mock.Mock()
        self.view.redraw_figure = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.options.autoscale = False
        self.options.enable_yaxis_changer = mock.Mock()
        self.options.set_plot_y_range = mock.Mock()
        self.context.update_autoscale_state = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_ylim_all = mock.Mock()

        self.presenter._handle_autoscale_y_axes()

        self.options.enable_yaxis_changer.assert_called_once()
        self.context.update_autoscale_state.assert_called_once_with(ws_names[0], False)
        self.context.update_ylim.assert_not_called()
        self.context.update_ylim_all.assert_not_called()
        self.options.set_plot_y_range.assert_not_called()
        self.view.redraw_figure.assert_not_called()

    def test_handle_error_selection_changed_all(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        self.view.plotted_workspaces_and_indices = [ws_names, ws_indices]
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.options.get_errors = mock.Mock(return_value=True)
        self.context.set_error_all = mock.Mock()
        self.context.update_error_state = mock.Mock()
        self.presenter.replot_workspace_with_error_state = mock.Mock()
        replot_workspace_with_error_state_calls = [mock.call(ws_names[0], True), mock.call(ws_names[1], True)]
        update_error_state_calls = [mock.call(ws_names[0], True), mock.call(ws_names[1], True)]
        self.context.get_axis = mock.Mock()
        self.context.get_axis.side_effect = [0, 1]
        self.model._get_workspace_plot_axis = mock.Mock()
        self.model._get_workspace_plot_axis.side_effect = [0, 1, 0, 1]
        self.presenter.handle_error_selection_changed()
        self.context.set_error_all.assert_called_once_with(True)
        self.context.update_error_state.assert_has_calls(update_error_state_calls)
        self.presenter.replot_workspace_with_error_state.assert_has_calls(replot_workspace_with_error_state_calls)

    def test_handle_error_selection_changed_specific_subplot(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        self.view.plotted_workspaces_and_indices = [ws_names, ws_indices]
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=([ws_names[1]], [ws_indices[1]]))
        self.options.get_errors = mock.Mock(return_value=False)
        self.context.set_error_all = mock.Mock()
        self.context.update_error_state = mock.Mock()
        self.presenter.replot_workspace_with_error_state = mock.Mock()
        replot_workspace_with_error_state_calls = [mock.call(ws_names[1], False)]
        update_error_state_calls = [mock.call(ws_names[1], False)]
        self.context.get_axis = mock.Mock()
        self.context.get_axis.side_effect = [1]
        self.model._get_workspace_plot_axis = mock.Mock()
        self.model._get_workspace_plot_axis.side_effect = [0, 1]
        self.presenter.handle_error_selection_changed()
        self.context.set_error_all.assert_not_called()
        self.context.update_error_state.assert_has_calls(update_error_state_calls)
        self.presenter.replot_workspace_with_error_state.assert_has_calls(replot_workspace_with_error_state_calls)

    def test_handle_subplot_changed_all(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        xlims = [-1, 1]
        ylims = [-1, 1]
        self.view.redraw_figure = mock.Mock()
        self.view.set_axis_xlimits = mock.Mock()
        self.view.set_axis_ylimits = mock.Mock()
        self.view.get_axis_limits = mock.Mock(return_value=(xlims[0], xlims[1], ylims[0], ylims[1]))
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.get_xlim = mock.Mock()
        self.context.get_ylim = mock.Mock()
        self.context.get_autoscale_state = mock.Mock()
        self.context.get_error_state = mock.Mock()
        self.context._xlim_all = xlims
        self.context._ylim_all = ylims
        self.context.get_autoscale_state_all = False
        self.context.get_error_state_all = False
        self.context.update_xlim = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_autoscale_state = mock.Mock()
        self.context.update_error_state = mock.Mock()
        self.options.set_plot_x_range = mock.Mock()
        self.options.set_plot_y_range = mock.Mock()
        self.options.set_autoscale = mock.Mock()
        self.options.enable_yaxis_changer = mock.Mock()
        self.options.set_errors = mock.Mock()

        self.presenter._handle_subplot_changed_in_quick_edit_widget()

        self.options.set_plot_x_range.assert_called_once_with(xlims)
        self.options.set_plot_y_range.assert_called_with(ylims)
        self.options.set_autoscale.assert_called_once_with(True)
        self.options.set_errors.assert_called_once_with(False)

    def test_handle_subplot_changed_specific_sub_plot(self):
        ws_names = ["MUSR62260; Group; fwd"]
        ws_indices = [0]
        xlims = [-1, 1]
        ylims = [-1, 1]
        self.view.redraw_figure = mock.Mock()
        self.view.set_axis_xlimits = mock.Mock()
        self.view.set_axis_ylimits = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))
        self.context.get_xlim = mock.Mock(return_value=xlims)
        self.context.get_ylim = mock.Mock(return_value=ylims)
        self.context.get_autoscale_state = mock.Mock(return_value=False)
        self.context.get_error_state = mock.Mock(return_value=False)
        self.context.update_xlim = mock.Mock()
        self.context.update_ylim = mock.Mock()
        self.context.update_autoscale_state = mock.Mock()
        self.context.update_error_state = mock.Mock()
        self.options.set_plot_x_range = mock.Mock()
        self.options.set_plot_y_range = mock.Mock()
        self.options.set_autoscale = mock.Mock()
        self.options.enable_yaxis_changer = mock.Mock()
        self.options.set_errors = mock.Mock()

        self.presenter._handle_subplot_changed_in_quick_edit_widget()

        self.options.set_plot_x_range.assert_called_once_with(xlims)
        self.options.set_plot_y_range.assert_called_once_with(ylims)
        self.options.set_autoscale.assert_called_once_with(False)
        self.options.set_errors.assert_called_once_with(False)

    def test_set_axes_limits_uses_default_limits_if_quick_edit_options_not_set(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        self.options.get_plot_x_range.return_value = [0, 0]
        self.options.get_plot_y_range.return_value = [0, 0]
        self.model.create_axes_titles.return_value = ["fwd"]
        self.presenter._update_quick_edit_widget = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))

        self.presenter._set_axes_limits_and_titles(False)

        self.view.set_axes_limits.assert_called_once_with(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)

    def test_set_axes_limits_updates_quick_edit_widget(self):
        ws_names = ["MUSR62260; Group; fwd", "MUSR62260; Group; bwd"]
        ws_indices = [0, 1]
        self.options.get_plot_x_range.return_value = [0, 0]
        self.options.get_plot_y_range.return_value = [0, 0]
        self.model.create_axes_titles.return_value = ["fwd"]
        self.presenter._update_quick_edit_widget = mock.Mock()
        self.presenter._get_selected_subplots_from_quick_edit_widget = mock.Mock(return_value=(ws_names, ws_indices))

        self.presenter._set_axes_limits_and_titles(False)

        self.presenter._update_quick_edit_widget.assert_called_once()

    def test_update_quick_edit_widget_subplots_correctly_updates_from_view(self):
        number_of_axes = 2
        self.view.number_of_axes = number_of_axes
        self.model.create_axes_titles.return_value = ["fwd"]
        self.options.get_all_subplots = ["MUSR62260; Group; fwd"]

        self.presenter._update_quick_widget_subplots_menu()

        self.assertEqual(number_of_axes, self.options.add_subplot.call_count)

    def test_create_tiled_plot_correctly_handles_empty_key_list(self):
        self.presenter.create_tiled_plot([])

        self.view.create_new_plot_canvas.assert_called_once_with(1)

    def test_should_update_all(self):
        selected_subplots = ["fwd", "bwd"]
        self.options.get_selection_index.return_value = 0

        self.assertTrue(self.presenter.should_update_all(selected_subplots))

    def test_should_update_all_one_plot(self):
        selected_subplots = ["one"]
        self.options.get_selection_index.return_value = 0

        self.assertTrue(self.presenter.should_update_all(selected_subplots))

    def test_should_update_all_false(self):
        selected_subplots = ["fwd"]
        self.options.get_selection_index.return_value = 1

        self.assertFalse(self.presenter.should_update_all(selected_subplots))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
