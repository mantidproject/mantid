# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_view import DuelPlotMaxentPaneView
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_model import DuelPlotMaxentPaneModel
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.duel_plot_maxent_pane.duel_plot_maxent_pane_presenter import DuelPlotMaxentPanePresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService


class DuelPlotMaxentPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=DuelPlotMaxentPaneModel)
        self.model.name = "data"
        self.view = mock.Mock(spec=DuelPlotMaxentPaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = DuelPlotMaxentPanePresenter(view=self.view, model=self.model, context=self.context,
                                                     figure_presenter=self.figure_presenter)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_change_time_plot(self):
        self.presenter.clear = mock.Mock()
        self.presenter.change_time_plot("Groups")

        self.presenter.clear.assert_called_once()
        self.model.set_if_groups.assert_called_once_with(True)

    def test_change_time_plot_false(self):
        self.presenter.clear = mock.Mock()
        self.presenter.change_time_plot("detectors")

        self.presenter.clear.assert_called_once()
        self.model.set_if_groups.assert_called_once_with(False)

    def test_handle_maxent_data_updated(self):
        self.presenter.add_data_to_plots = mock.Mock()
        name = "test"

        self.presenter.handle_maxent_data_updated(name)
        self.assertEqual(self.presenter._maxent_ws_name, name)
        self.model.set_run_from_name.assert_called_once_with(name)
        self.presenter.add_data_to_plots.assert_called_once_with()

    def test_add_data_to_plots(self):
        ws = ["unit", "test"]
        indices = [1,3]
        self.presenter.handle_time_data_updated = mock.Mock(return_value=(["a"],[0]))
        self.model.add_reconstructed_data.return_value = (ws, indices)
        self.presenter.add_list_to_plot = mock.Mock()
        self.figure_presenter._options_presenter = mock.Mock()
        self.figure_presenter._options_presenter.set_selection_by_index = mock.Mock()

        self.presenter.add_data_to_plots()
        self.presenter.handle_time_data_updated.assert_called_once_with()
        self.model.add_reconstructed_data.assert_called_once_with(["a"],[0])
        self.presenter.add_list_to_plot.assert_called_once_with(ws, indices, hold=False, autoscale=True)
        self.figure_presenter._options_presenter.set_selection_by_index.assert_called_once_with(1)
        self.figure_presenter.set_plot_range.assert_called_once_with([0,1000])

    def test_add_data_to_plots_with_maxent(self):
        ws = ["unit", "test"]
        self.presenter._maxent_ws_name = "maxent"
        indices = [1,3]
        self.presenter.handle_time_data_updated = mock.Mock(return_value=(["a"],[0]))
        # cant pass values as they are changed in the function being tested
        self.model.add_reconstructed_data.return_value = (["unit", "test"], [1,3])
        self.presenter.add_list_to_plot = mock.Mock()
        self.figure_presenter._options_presenter = mock.Mock()
        self.figure_presenter._options_presenter.set_selection_by_index = mock.Mock()

        self.presenter.add_data_to_plots()
        self.presenter.handle_time_data_updated.assert_called_once_with()
        self.model.add_reconstructed_data.assert_called_once_with(["a"],[0])
        self.presenter.add_list_to_plot.assert_called_once_with(ws+["maxent"], indices+[0], hold=False, autoscale=True)
        self.figure_presenter._options_presenter.set_selection_by_index.assert_called_once_with(1)
        self.figure_presenter.set_plot_range.assert_called_once_with([0,1000])

    def test_handle_reconstructed_data_updated(self):
        data_dict = {"table": "unit", "ws": "test"}
        self.presenter.update_selection = mock.Mock()
        self.model.create_options.return_value = ["fwd","bwd"]

        self.presenter.handle_reconstructed_data_updated(data_dict)
        self.model.set_reconstructed_data.assert_called_once_with("test", "unit")
        self.view.update_selection.assert_called_once_with(["fwd", "bwd"])
        self.presenter.update_selection.assert_called_once_with()

    def test_handle_reconstructed_data_updated_no_table(self):
        data_dict = {"ws": "test"}
        self.presenter.update_selection = mock.Mock()
        self.model.create_options.return_value = ["fwd","bwd"]

        self.presenter.handle_reconstructed_data_updated(data_dict)
        self.model.set_reconstructed_data.assert_not_called()
        self.view.update_selection.assert_called_once_with(["fwd", "bwd"])
        self.presenter.update_selection.assert_called_once_with()

    def test_handle_reconstructed_data_updated_no_ws(self):
        data_dict = {"table": "unit"}
        self.presenter.update_selection = mock.Mock()
        self.model.create_options.return_value = ["fwd","bwd"]

        self.presenter.handle_reconstructed_data_updated(data_dict)
        self.model.set_reconstructed_data.assert_not_called()
        self.view.update_selection.assert_called_once_with(["fwd", "bwd"])
        self.presenter.update_selection.assert_called_once_with()

    def test_update_selection(self):
        self.assertEqual(self.model.set_selection.call_count, 1)

        self.presenter.add_data_to_plots = mock.Mock()
        type(self.view).get_selection_for_plot = mock.PropertyMock(return_value="top")
        self.presenter.update_selection()

        self.assertEqual(self.model.set_selection.call_count, 2)
        self.model.set_selection.assert_any_call("top")
        self.presenter.add_data_to_plots.assert_called_once_with()

    def test_clear(self):
        self.presenter._maxent_ws_name = "maxent"
        self.assertEqual(self.presenter._maxent_ws_name, "maxent")
        self.presenter.clear_subplots = mock.Mock()

        self.presenter.clear()
        self.model.clear_data.asseert_called_once_with()
        self.assertEqual(self.presenter._maxent_ws_name, None)
        self.view.update_selection.aseert_called_once_with([])
        self.presenter.clear_subplots.assert_called_once_with()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
