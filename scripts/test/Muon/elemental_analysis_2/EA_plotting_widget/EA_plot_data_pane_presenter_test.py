import unittest
from unittest import mock

from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import EAPlotDataPaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_presenter import \
    EAPlotDataPanePresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter import PlottingCanvasPresenter
from mantid import AnalysisDataService

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class EAPlotDataPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=EAPlotDataPaneModel)
        self.model.name = "data"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenter)

        self.context.group_context.selected_groups = ["9999; Detector 1"]

        self.presenter = EAPlotDataPanePresenter(view=self.view, model=self.model, context=self.context,
                                                 figure_presenter=self.figure_presenter)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_handle_data_type_changed(self):
        self.presenter.handle_data_updated = mock.Mock()
        self.figure_presenter.force_autoscale = mock.Mock()

        self.presenter.handle_data_type_changed()

        self.presenter.handle_data_updated.assert_called_once_with(autoscale=True, hold_on=False)
        self.figure_presenter.force_autoscale.assert_called_once()

    def test_handle_data_updated(self):
        self.model.get_workspace_list_and_indices_to_plot.return_value = ["9999; Detector 2", "9999; Detector 3"], [0,
                                                                                                                    0]
        self.presenter.add_list_to_plot = mock.Mock()
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Delayed"

        self.presenter.handle_data_updated(True, False)

        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Delayed")
        self.presenter.add_list_to_plot.assert_called_once_with(["9999; Detector 2", "9999; Detector 3"], [0, 0],
                                                                hold=False, autoscale=True)

    def test_handle_added_or_removed_group_to_plot_add(self):
        self.presenter.handle_added_group_to_plot = mock.Mock()
        self.presenter.handle_removed_group_from_plot = mock.Mock()

        info = {"is_added": True, "name": "9999; Detector 2"}
        self.presenter.handle_added_or_removed_group_to_plot(info)

        self.presenter.handle_added_group_to_plot.assert_called_once_with()
        self.presenter.handle_removed_group_from_plot.assert_not_called()

    def test_handle_added_or_removed_group_to_plot_removed(self):
        self.presenter.handle_added_group_to_plot = mock.Mock()
        self.presenter.handle_removed_group_from_plot = mock.Mock()

        info = {"is_added": False, "name": "9999; Detector 3"}
        self.presenter.handle_added_or_removed_group_to_plot(info)

        self.presenter.handle_added_group_to_plot.assert_not_called()
        self.presenter.handle_removed_group_from_plot.assert_called_once_with("9999; Detector 3")

    def test_handle_added_group_plot_safe(self):
        self.presenter.handle_data_updated = mock.Mock()

        self.presenter.handle_added_group_to_plot()
        self.presenter.handle_data_updated.assert_called_once()

    def test_handle_removed_group_from_plot(self):
        self.model.get_workspaces_to_remove.return_value = ["9999; Detector 2"]
        self.presenter.remove_list_from_plot = mock.Mock()
        self.presenter.handle_data_updated = mock.Mock()
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Counts"

        self.presenter.handle_removed_group_from_plot("9999; Detector 2")
        self.presenter.remove_list_from_plot.assert_called_once_with(["9999; Detector 2"])
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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
