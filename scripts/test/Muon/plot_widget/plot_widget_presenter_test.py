# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import FitPlotInformation
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plot_widget.plot_widget_model import PlotWidgetModel
from Muon.GUI.Common.plot_widget.plot_widget_view_interface import PlotWidgetViewInterface
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from Muon.GUI.Common.plot_widget.plot_widget_presenter import PlotWidgetPresenterCommon
from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

from mantid import AnalysisDataService
from mantid.api import WorkspaceFactory
from mantidqt.utils.qt.testing import start_qapplication
from mantid.simpleapi import CreateWorkspace

workspace_list = ['MUSR62260; Group; bottom; Asymmetry; MA',
                  'MUSR62261; Group; bottom; Asymmetry; MA']
indices = [0, 0]


@start_qapplication
class PlotWidgetPresenterCommonTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=PlotWidgetModel)
        self.view = mock.Mock(spec=PlotWidgetViewInterface)
        self.view.warning_popup = mock.MagicMock()
        self.view.setEnabled = mock.MagicMock()
        self.external_plotting_model = mock.Mock(spec=ExternalPlottingModel)
        self.external_plotting_view = mock.Mock(spec=ExternalPlottingView)
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.get_selected_fit_workspaces = mock.MagicMock()

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.model.get_workspace_list_and_indices_to_plot.return_value = [workspace_list, indices]

        self.presenter = PlotWidgetPresenterCommon(view=self.view, model=self.model, context=self.context,
                                                   figure_presenter=self.figure_presenter,
                                                   get_selected_fit_workspaces=self.get_selected_fit_workspaces(),
                                                   external_plotting_view=self.external_plotting_view,
                                                   external_plotting_model=self.external_plotting_model)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    def test_plot_selected_calls_plot_correctly(self):
        autoscale = False
        hold_on = False

        self.presenter.plot_all_selected_data(autoscale, hold_on)

        self.figure_presenter.plot_workspaces.assert_called_once_with(workspace_list,
                                                                      indices,
                                                                      hold_on=autoscale, autoscale=hold_on)

    def test_plot_selected_calls_model_correctly(self):
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Asymmetry"

        self.presenter.plot_all_selected_data(False, False)

        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Asymmetry")

    def test_handle_workspace_deleted_from_ads_calls_delete_workspace_correctly(self):
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [["fwd", "bwd"], []]
        ws = self.create_workspace('fwd')

        self.presenter.handle_workspace_deleted_from_ads(ws)

        self.figure_presenter.remove_workspace_from_plot.assert_called_once_with(ws)

    def test_handle_workspace_deleted_from_ads_does_nothing_if_workspace_not_plotted(self):
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [["fwd", "bwd"], []]
        ws = self.create_workspace('top')

        self.presenter.handle_workspace_deleted_from_ads(ws)

        self.figure_presenter.remove_workspace_from_plot.assert_not_called()

    def test_handle_workspace_replaced_in_ads_calls_update_workspace_correctly(self):
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [["fwd", "bwd"], []]
        ws = self.create_workspace('bwd')

        self.presenter.handle_workspace_replaced_in_ads(ws)

        self.figure_presenter.replace_workspace_in_plot.assert_called_once_with(ws)

    def test_handle_workspace_replaced_in_ads_does_nothing_if_workspace_not_plotted(self):
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [["fwd", "bwd"], []]
        ws = self.create_workspace('top')

        self.presenter.handle_workspace_replaced_in_ads(ws)

        self.figure_presenter.replace_workspace_in_plot.assert_not_called()

    def test_handle_data_updated_calls_plot_correctly(self):
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Asymmetry"
        self.presenter.plot_all_selected_data = mock.Mock()

        self.presenter.handle_data_updated()

        self.presenter.plot_all_selected_data.assert_called_once_with(autoscale=False, hold_on=False)

    def test_handle_plot_type_changed_correctly_calls_plot(self):
        self.view.is_raw_plot.return_value = True
        self.presenter.plot_all_selected_data = mock.Mock()

        self.presenter.handle_plot_type_changed()

        self.presenter.plot_all_selected_data.assert_called_once_with(autoscale=True, hold_on=False)

    def test_handle_plot_type_changed_displays_a_warning_if_trying_to_plot_counts_on_a_pair(self):
        self.context.group_pair_context.selected_pairs = ['long']
        self.view.get_plot_type.return_value = "Counts"
        self.model.counts_plot = "Counts"
        self.presenter.handle_plot_type_changed()

        self.view.warning_popup.assert_called_once_with(
            'Pair workspaces have no counts workspace, plotting Asymmetry')
        self.figure_presenter.plot_workspaces.assert_not_called()
        self.model.get_workspace_list_and_indices_to_plot.assert_not_called()

    def test_switching_to_single_plot_correctly_calls_plot(self):
        self.view.is_tiled_plot.return_value = False

        self.presenter.handle_plot_tiled_state_changed()

        self.figure_presenter.convert_plot_to_single_plot.assert_called_once()

    def test_switching_to_tiled_plot_correctly_calls_plot(self):
        self.view.is_tiled_plot.return_value = True
        self.view.tiled_by.return_value = "Group/pairs"
        self.model.create_tiled_keys.return_value = ["fwd", "bwd"]

        self.presenter.handle_plot_tiled_state_changed()

        self.model.create_tiled_keys.assert_called_once_with("Group/pairs")
        self.figure_presenter.convert_plot_to_tiled_plot.assert_called_once_with(["fwd", "bwd"], "Group/pairs")

    def test_tiled_by_changed_does_nothing_if_not_tiled_plot(self):
        self.view.is_tiled_plot.return_value = False

        self.presenter.handle_tiled_by_type_changed()

        self.model.create_tiled_keys.assert_not_called()

    def test_rebin_options_changed_updates_view(self):
        self.context._do_rebin.return_value = True

        self.presenter.handle_rebin_options_changed()

        self.view.set_raw_checkbox_state.assert_called_once_with(False)

    def test_handle_raw_workspaces_changed_does_nothing_if_no_rebin_options_set(self):
        self.view.is_raw_plot.return_value = False
        self.context._do_rebin.return_value = False

        self.presenter.handle_use_raw_workspaces_changed()

        self.view.warning_popup.assert_called_once_with('No rebin options specified')
        self.model.get_workspace_list_and_indices_to_plot.assert_not_called()

    def test_handle_raw_workspaces_changed_correctly_calls_plot(self):
        self.view.is_raw_plot.return_value = False
        self.context._do_rebin.return_value = True
        self.view.get_plot_type.return_value = "Asymmetry"

        self.presenter.handle_use_raw_workspaces_changed()

        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(False,
                                                                                  "Asymmetry")
        self.figure_presenter.plot_workspaces.assert_called_once_with(workspace_list,
                                                                      indices,
                                                                      hold_on=False, autoscale=False)

    def test_group_added_to_analysis_correctly_calls_plot(self):
        group_name = "fwd"
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Asymmetry"
        self.view.is_tiled_plot.return_value = False
        self.model.get_workspace_and_indices_for_group_or_pair.return_value = [[workspace_list[0]], [0]]

        self.presenter.handle_added_group_or_pair_to_plot(group_name)

        self.model.get_workspace_and_indices_for_group_or_pair.assert_called_once_with(group_name, is_raw=True,
                                                                                       plot_type="Asymmetry")

        self.figure_presenter.plot_workspaces.assert_called_once_with([workspace_list[0]],
                                                                      [0], hold_on=True, autoscale=False)

    def test_group_added_to_analysis_correctly_sets_up_new_tiled_plot_if_tiled_by_group(self):
        group_name = "fwd"
        self.view.is_tiled_plot.return_value = True
        self.view.tiled_by.return_value = "Group/Pair"
        self.model.tiled_by_group = "Group/Pair"
        self.presenter.plot_all_selected_data = mock.Mock()

        self.presenter.handle_added_group_or_pair_to_plot(group_name)

        self.model.create_tiled_keys.assert_called_once_with("Group/Pair")
        self.figure_presenter.create_tiled_plot.assert_called_once()
        self.presenter.plot_all_selected_data.assert_called_once()

    def test_group_removed_from_analysis_correctly_removes_group_from_plot(self):
        group_name = "fwd"
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Asymmetry"
        self.model.get_workspace_and_indices_for_group_or_pair.return_value = [[workspace_list[0]], [0]]

        self.presenter.handle_removed_group_or_pair_to_plot(group_name)

        self.model.get_workspace_and_indices_for_group_or_pair.assert_called_once_with(group_name, is_raw=True,
                                                                                       plot_type="Asymmetry")
        self.figure_presenter.remove_workspace_names_from_plot.assert_called_with([workspace_list[0]])

    def test_group_removed_correctly_sets_up_new_tiled_plot_if_tiled_by_group(self):
        group_name = "fwd"
        self.view.is_tiled_plot.return_value = True
        self.view.tiled_by.return_value = "Group/Pair"
        self.model.tiled_by_group = "Group/Pair"
        self.presenter.plot_all_selected_data = mock.Mock()

        self.presenter.handle_removed_group_or_pair_to_plot(group_name)

        self.model.create_tiled_keys.assert_called_once_with("Group/Pair")
        self.figure_presenter.create_tiled_plot.assert_called_once()
        self.presenter.plot_all_selected_data.assert_called_once()

    def test_handle_instrument_changed_creates_new_plot(self):
        self.presenter.handle_instrument_changed()

        self.figure_presenter.create_single_plot.assert_called_once()

    def test_handle_plot_selected_fits_correctly_calls_plot(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()
        fit = FitInformation(mock.MagicMock(), 'GaussOsc',
                             [StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA', workspace)],
                             StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA; Fitted', table_workspace),
                             mock.Mock())

        self.model.get_fit_workspace_and_indices.return_value = [["MUSR62260; Group; bottom; Asymmetry; MA; Fitted"],
                                                                 [1]]
        fit_information = FitPlotInformation(input_workspaces=["MUSR62260; Group; bottom; Asymmetry; MA"], fit=fit)

        expected_workspace_list = ["MUSR62260; Group; bottom; Asymmetry; MA"] + \
                                  ['MUSR62260; Group; bottom; Asymmetry; MA; Fitted']
        expected_indices = [0] + [1]

        self.presenter.handle_plot_selected_fits([fit_information])

        self.figure_presenter.plot_workspaces.assert_called_once_with(expected_workspace_list, expected_indices,
                                                                      hold_on=False, autoscale=False)

    def test_handle_plot_selected_fits_correctly_calls_model(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()
        fit = FitInformation(mock.MagicMock(), 'GaussOsc',
                             [StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA', workspace)],
                             StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA; Fitted', table_workspace),
                             mock.Mock())

        self.model.get_fit_workspace_and_indices.return_value = [["MUSR62260; Group; bottom; Asymmetry; MA; Fitted"],
                                                                 [1]]

        fit_information = FitPlotInformation(input_workspaces=["MUSR62260; Group; bottom; Asymmetry; MA"], fit=fit)

        self.view.is_plot_diff.return_value = False
        self.presenter.handle_plot_selected_fits([fit_information])
        self.model.get_fit_workspace_and_indices.assert_called_once_with(fit, False)

    def test_handle_external_plot_pressed(self):
        expected_axes = mock.NonCallableMock()
        self.figure_presenter.get_plot_axes.return_value = expected_axes

        self.presenter.handle_external_plot_requested()

        self.figure_presenter.get_plot_axes.assert_called_once()
        self.external_plotting_view.create_external_plot_window.assert_called_once_with(expected_axes)
        self.external_plotting_model.get_plotted_workspaces_and_indices_from_axes.assert_called_once_with(expected_axes)
        self.external_plotting_view.plot_data.assert_called_once()
        self.external_plotting_view.copy_axes_setup.assert_called_once()
        self.external_plotting_view.show.assert_called_once()

    def test_match_raw_selection_True_True(self):
        self.context.fitting_context.fit_to_raw = True
        ws_names = ['MUSR62260; Group; bottom; Asymmetry; MA']
        self.assertEqual(self.presenter.match_raw_selection(ws_names, True), ws_names)

    def test_match_raw_selection_True_False(self):
        self.context.fitting_context.fit_to_raw = True
        ws_names = ['MUSR62260; Group; bottom; Asymmetry; MA']
        ws_rebin_names = ['MUSR62260; Group; bottom; Asymmetry; Rebin; MA']
        self.assertEqual(self.presenter.match_raw_selection(ws_names, False), ws_rebin_names)

    def test_match_raw_selection_False_True(self):
        self.context.fitting_context.fit_to_raw = False
        ws_names = ['MUSR62260; Group; bottom; Asymmetry; MA']
        ws_rebin_names = ['MUSR62260; Group; bottom; Asymmetry; Rebin; MA']
        self.assertEqual(self.presenter.match_raw_selection(ws_rebin_names, True), ws_names)

    def test_match_raw_selection_False_False(self):
        self.context.fitting_context.fit_to_raw = False
        ws_rebin_names = ['MUSR62260; Group; bottom; Asymmetry; Rebin; MA']
        self.assertEqual(self.presenter.match_raw_selection(ws_rebin_names, False), ws_rebin_names)

    def test_tab_enabled_with_data_loaded(self):
        self.presenter.plot_all_selected_data(False, False)
        self.view.setEnabled.assert_called_once_with(True)

    def test_tab_disabled_with_no_data_loaded(self):
        self.model.get_workspace_list_and_indices_to_plot.return_value = [[], indices]
        self.presenter.plot_all_selected_data(False, False)
        self.view.setEnabled.assert_called_once_with(False)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
