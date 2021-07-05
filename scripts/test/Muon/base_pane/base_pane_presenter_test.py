# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class BasePanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=BasePaneModel)
        self.model.name = "test"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.view.setEnabled = mock.MagicMock()
        self.external_plotting_model = mock.Mock(spec=ExternalPlottingModel)
        self.external_plotting_view = mock.Mock(spec=ExternalPlottingView)
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.figure_presenter.force_autoscale = mock.Mock()

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = BasePanePresenter(view=self.view, model=self.model, context=self.context,
                                                   figure_presenter=self.figure_presenter,
                                                   external_plotting_view=self.external_plotting_view,
                                                   external_plotting_model=self.external_plotting_model)

    def create_workspace(self, name):
        x_range = range(1, 100)
        y_range = [x * x for x in x_range]
        return CreateWorkspace(DataX=x_range, DataY=y_range, OutputWorkspace=name)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_setup_view_connections(self):
        # this will be called as part of setup
        # so expect the calles to have been made
        self.view.on_tiled_by_type_changed.assert_called_once_with(self.presenter.handle_tiled_by_type_changed)
        self.view.on_plot_tiled_checkbox_changed.assert_called_once_with(self.presenter.handle_plot_tiled_state_changed)
        self.view.on_external_plot_pressed.assert_called_once_with(self.presenter.handle_external_plot_requested)
        self.view.on_plot_type_changed.assert_called_once_with(self.presenter.handle_data_type_changed)

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

    def test_added_or_removed_plot_add(self):
        plot_info ={"is_added": True, "name": "fwd"}
        self.presenter.add_list_to_plot = mock.Mock()

        self.presenter.handle_added_or_removed_plot(plot_info)
        self.presenter.add_list_to_plot.assert_called_once_with(["fwd"],[0])

    def test_added_or_removed_plot_remove(self):
        plot_info ={"is_added": False, "name": "fwd"}
        self.presenter.remove_list_from_plot = mock.Mock()

        self.presenter.handle_added_or_removed_plot(plot_info)
        self.presenter.remove_list_from_plot.assert_called_once_with(["fwd"])

    def test_add_list_to_plot_no_autoscale(self):
        self.presenter._update_tile_plot = mock.Mock()
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [["fwd", "bwd"], [0,1]]
        self.presenter._update_tile_plot = mock.Mock()
        hold = True
        autoscale = False
        self.presenter.add_list_to_plot(["top"],[2], hold, autoscale)

        self.presenter._update_tile_plot.assert_called_once()
        self.figure_presenter.force_autoscale.assert_not_called()
        self.figure_presenter.plot_workspaces.assert_called_once_with(["top"], [2],
                                                                      hold, autoscale)

    def test_add_list_to_plot_force_autoscale(self):
        self.presenter._update_tile_plot = mock.Mock()
        self.figure_presenter.get_plotted_workspaces_and_indices.return_value = [[], []]
        self.presenter._update_tile_plot = mock.Mock()
        hold = True
        autoscale = False
        self.presenter.add_list_to_plot(["top"],[0], hold, autoscale)

        self.presenter._update_tile_plot.assert_called_once()
        self.figure_presenter.force_autoscale.assert_called_once()
        self.figure_presenter.plot_workspaces.assert_called_once_with(["top"], [0],
                                                                      hold, autoscale)

    def test_remove_list_from_plot(self):
        self.presenter._update_tile_plot = mock.Mock()
        self.presenter.remove_list_from_plot(["top"])

        self.presenter._update_tile_plot.assert_called_once()
        self.figure_presenter.remove_workspace_names_from_plot.assert_called_once_with(["top"])

    def test_rebin_options_changed_updates_view(self):
        self.context._do_rebin.return_value = True

        self.presenter.handle_rebin_options_changed()

        self.view.set_raw_checkbox_state.assert_called_once_with(False)

    def test_handle_check_if_can_use_rebin_only_raw_data(self):
        self.view.is_raw_plot.return_value = False
        self.context._do_rebin.return_value = False

        self.assertEqual(self.presenter.check_if_can_use_rebin(), False)
        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_handle_check_if_can_use_rebin_rebinned_data(self):
        self.view.is_raw_plot.return_value = False
        self.context._do_rebin.return_value = True

        self.assertEqual(self.presenter.check_if_can_use_rebin(), True)
        self.view.warning_popup.assert_not_called()

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
        self.figure_presenter.convert_plot_to_tiled_plot.assert_called_once_with(["fwd", "bwd"])

    def test_tiled_by_changed_does_nothing_if_not_tiled_plot(self):
        self.view.is_tiled_plot.return_value = False
        self.model.create_tiled_keys = mock.Mock()

        self.presenter.handle_tiled_by_type_changed()
        self.model.create_tiled_keys.assert_not_called()

    def test_handle_tiled_by_type_changed(self):
        self.view.is_tiled_plot.return_value = True
        self.view.tiled_by.return_value = "Asymmetry"
        self.model.create_tiled_keys.return_value = ["fwd", "bwd"]

        self.presenter.handle_tiled_by_type_changed()
        self.figure_presenter.convert_plot_to_tiled_plot.assert_called_once_with(["fwd", "bwd"])

    def test_update_tiled_plot_not_tiled(self):
        self.view.is_tiled_plot.return_value = False
        self.view.tiled_by.return_value = "Asymmetry"
        self.model.create_tiled_keys.return_value = ["fwd", "bwd"]
        self.figure_presenter._handle_autoscale_y_axes = mock.Mock()

        self.presenter._update_tile_plot()
        self.figure_presenter.create_tiled_plot.assert_not_called()
        self.figure_presenter._handle_autoscale_y_axes.assert_not_called()

    def test_update_tiled_plot_is_tiled(self):
        self.view.is_tiled_plot.return_value = True
        self.view.tiled_by.return_value = "Asymmetry"
        self.model.create_tiled_keys.return_value = ["fwd", "bwd"]
        self.figure_presenter._handle_autoscale_y_axes = mock.Mock()

        self.presenter._update_tile_plot()
        self.figure_presenter.create_tiled_plot.assert_called_once_with(["fwd", "bwd"])
        self.figure_presenter._handle_autoscale_y_axes.assert_called_once()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
