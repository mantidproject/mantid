# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_view import PlotDataPaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_presenter import PlotDataPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.selection_info_presenter import SelectionInfoPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class PlotDataPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        selection_info = mock.MagicMock(autoSpec=SelectionInfoPresenter)
        self.model = mock.Mock(autoSpec=PlotDataPaneModel)
        self.model.name = "data"
        self.view = mock.Mock(autoSpec=PlotDataPaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = PlotDataPanePresenter(view=self.view, model=self.model, context=self.context,
                                               figure_presenter=self.figure_presenter, selection_info=selection_info)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_handle_data_type_changed_pair_selected(self):
        self.presenter._check_if_counts_and_pairs_selected = mock.Mock(return_value=True)
        self.presenter.handle_data_updated = mock.Mock()
        self.figure_presenter.force_autoscale = mock.Mock()

        self.presenter.handle_data_type_changed()

        self.presenter.handle_data_updated.assert_not_called()
        self.figure_presenter.force_autoscale.assert_not_called()

    def test_handle_data_type_changed_to_counts(self):

        def count_name(name):
            return name.replace("A", "C")

        self.presenter.selection_info.get_selection.return_value={"fA":0, "bA":1}
        self.view.get_plot_type.return_value = "Counts"
        self.model.get_workspace_list_and_indices_to_plot.return_value = ["fC", "bC"], [0,1]
        self.presenter.plot_lines = mock.Mock()
        self.model.convert_ws_name_to_counts.side_effect = count_name
        self.presenter.selection_info.update_lines.return_value={"fC":0, "bC":1}
        self.presenter._check_if_counts_and_pairs_selected = mock.Mock(return_value=False)
        self.figure_presenter.force_autoscale = mock.Mock()
        self.view.is_raw_plot.return_value = True

        self.presenter.handle_data_type_changed()

        self.assertEqual(self.model.convert_ws_name_to_counts.call_count, 2)
        self.assertEqual(self.model.convert_ws_name_to_asymmetry.call_count, 0)
        self.model.convert_ws_name_to_counts.assert_any_call("fA")
        self.model.convert_ws_name_to_counts.assert_any_call("bA")
        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Counts")
        self.presenter.selection_info.update_lines.assert_called_once_with(["fC", "bC"], [0, 1])
        self.presenter.selection_info.set_selected_rows_from_name(["fC", "bC"])
        self.presenter.plot_lines.assert_called_once_with({"fC":0, "bC":1}, autoscale=True, hold_on=False)

        self.figure_presenter.force_autoscale.assert_called_once()

    def test_handle_data_type_changed_to_asymmetry(self):

        def asymmetry_name(name):
            return name.replace("C", "A")

        self.presenter.selection_info.get_selection.return_value={"fC":0, "bC":1}
        self.view.get_plot_type.return_value = "Asymmetry"
        self.model.get_workspace_list_and_indices_to_plot.return_value = ["fA", "bA"], [0,1]
        self.presenter.plot_lines = mock.Mock()
        self.model.convert_ws_name_to_asymmetry.side_effect = asymmetry_name
        self.presenter.selection_info.update_lines.return_value={"fA":0, "bA":1}
        self.presenter._check_if_counts_and_pairs_selected = mock.Mock(return_value=False)
        self.figure_presenter.force_autoscale = mock.Mock()
        self.view.is_raw_plot.return_value = True

        self.presenter.handle_data_type_changed()

        self.assertEqual(self.model.convert_ws_name_to_counts.call_count, 0)
        self.assertEqual(self.model.convert_ws_name_to_asymmetry.call_count, 2)
        self.model.convert_ws_name_to_asymmetry.assert_any_call("fC")
        self.model.convert_ws_name_to_asymmetry.assert_any_call("bC")
        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Asymmetry")
        self.presenter.selection_info.update_lines.assert_called_once_with(["fA", "bA"], [0, 1])
        self.presenter.selection_info.set_selected_rows_from_name(["fA", "bA"])
        self.presenter.plot_lines.assert_called_once_with({"fA":0, "bA":1}, autoscale=True, hold_on=False)

        self.figure_presenter.force_autoscale.assert_called_once()

    def test_check_if_counts_and_pairs_selected_true(self):
        self.context.group_pair_context.selected_pairs = ["long"]
        self.view.get_plot_type.return_value = "Counts"

        state = self.presenter._check_if_counts_and_pairs_selected()
        self.assertEqual(state, True)
        self.view.set_plot_type.assert_called_once_with("Asymmetry")
        self.view.warning_popup.assert_called_once()

    def test_check_if_counts_and_pairs_selected_false(self):
        self.context.group_pair_context.selected_pairs = []
        self.view.get_plot_type.return_value = "Counts"

        state = self.presenter._check_if_counts_and_pairs_selected()
        self.assertEqual(state, False)
        self.view.set_plot_type.assert_not_called()
        self.view.warning_popup.assert_not_called()

    def test_handle_data_updated(self):
        self.model.get_workspace_list_and_indices_to_plot.return_value = [["fwd", "bwd"], [0,1]]
        self.presenter.selection_info.get_selection.return_value = ["unit", "fwd"]
        self.presenter.add_list_to_plot = mock.Mock()
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Counts"

        self.presenter.selection_info.update_lines.return_value={"fwd":0}
        self.presenter.handle_data_updated(True, False)

        self.assertEqual(self.presenter.selection_info.update_lines.call_count, 1)
        self.presenter.selection_info.update_lines.assert_called_once_with(["fwd", "bwd"],[0,1], ["fwd"])
        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Counts")
        self.presenter.add_list_to_plot.assert_called_once_with(["fwd"],[0], hold=False, autoscale=True)

    def test_handle_added_or_removed_group_or_pair_to_plot_add(self):
        self.presenter.handle_added_group_or_pair_to_plot = mock.Mock()
        self.presenter.handle_removed_group_or_pair_from_plot = mock.Mock()

        info = {"is_added": True, "name":"fwd"}
        self.presenter.handle_added_or_removed_group_or_pair_to_plot(info)

        self.presenter.handle_added_group_or_pair_to_plot.assert_called_once_with()
        self.presenter.handle_removed_group_or_pair_from_plot.assert_not_called()

    def test_handle_added_or_removed_group_or_pair_to_plot_removed(self):
        self.presenter.handle_added_group_or_pair_to_plot = mock.Mock()
        self.presenter.handle_removed_group_or_pair_from_plot = mock.Mock()

        info = {"is_added": False, "name":"fwd"}
        self.presenter.handle_added_or_removed_group_or_pair_to_plot(info)

        self.presenter.handle_added_group_or_pair_to_plot.assert_not_called()
        self.presenter.handle_removed_group_or_pair_from_plot.assert_called_once_with("fwd")

    def test_handle_added_group_or_pair_to_plot_safe(self):
        self.presenter.__check_if_counts_and_pairs_selected = mock.Mock(return_value=False)
        self.presenter.handle_data_updated = mock.Mock()

        self.presenter.handle_added_group_or_pair_to_plot()
        self.presenter.handle_data_updated.assert_called_once()

    def test_handle_added_group_or_pair_to_plot_unsafe(self):
        self.presenter._check_if_counts_and_pairs_selected = mock.Mock(return_value=True)
        self.presenter.handle_data_updated = mock.Mock()

        self.presenter.handle_added_group_or_pair_to_plot()
        self.presenter.handle_data_updated.assert_not_called()

    def test_handle_removed_group_pair_from_plot(self):
        self.model.get_workspaces_to_remove.return_value = ["EMU52; fwd"]
        self.presenter.remove_list_from_plot = mock.Mock()
        self.presenter.handle_data_updated = mock.Mock()
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Counts"

        self.presenter.handle_removed_group_or_pair_from_plot("fwd")
        self.presenter.remove_list_from_plot.assert_called_once_with(["EMU52; fwd"])
        self.presenter.handle_data_updated.assert_called_once()

    def test_handle_use_raw_workspace_changed(self):
        self.presenter.check_if_can_use_rebin = mock.Mock(return_value=True)
        self.presenter.handle_data_updated = mock.Mock()

        self.presenter.handle_use_raw_workspaces_changed()
        self.presenter.handle_data_updated.assert_called_once()

    def test_handle_use_raw_workspace_changed_fail(self):
        self.presenter.check_if_can_use_rebin = mock.Mock(return_value=False)
        self.presenter.handle_data_updated = mock.Mock()

        self.presenter.handle_use_raw_workspaces_changed()
        self.presenter.handle_data_updated.assert_not_called()

    def test_plot_selection_changed(self):
        self.presenter.selection_info.get_selection.return_value ={"fwd":0}
        self.presenter.add_list_to_plot = mock.Mock()
        self.presenter.plot_selection_changed()
        self.presenter.add_list_to_plot.assert_called_once_with(["fwd"], [0], hold=False, autoscale=True)

    def test_plot_selection_changed_fail(self):
        self.presenter.selection_info.get_selection.return_value ={}
        self.presenter.add_list_to_plot = mock.Mock()
        self.presenter.plot_selection_changed()
        self.presenter.add_list_to_plot.assert_not_called()

    def test_plot_lines(self):
        lines = {"fwd":0}
        self.presenter.add_list_to_plot = mock.Mock()

        self.presenter.plot_lines(lines, autoscale=True, hold_on=False)
        self.presenter.add_list_to_plot.assert_called_once_with(["fwd"], [0], hold=False, autoscale=True)

    def test_plot_lines_fail(self):
        lines = {}
        self.presenter.add_list_to_plot = mock.Mock()

        self.presenter.plot_lines(lines, autoscale=True, hold_on=False)
        self.presenter.add_list_to_plot.assert_not_called()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
