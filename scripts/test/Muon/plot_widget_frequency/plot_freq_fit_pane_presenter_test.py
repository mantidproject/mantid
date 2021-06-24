# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_presenter import PlotFreqFitPanePresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class PlotFreqFitPanePresenterrTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=PlotFreqFitPaneModel)
        self.model.name = "data"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)

        self.context.group_pair_context.selected_groups = ['bottom']
        self.context.group_pair_context.selected_pairs = []

        self.presenter = PlotFreqFitPanePresenter(view=self.view, model=self.model, context=self.context,
                                                   figure_presenter=self.figure_presenter)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_handle_data_updated(self):
        self.model.get_workspace_list_and_indices_to_plot.return_value = ["fwd","bwd"],[0,1]
        self.presenter.add_list_to_plot = mock.Mock()
        self.view.is_raw_plot.return_value = True
        self.view.get_plot_type.return_value = "Counts"

        self.presenter.handle_data_updated(True, False)

        self.model.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Counts")
        self.presenter.add_list_to_plot.assert_called_once_with(["fwd","bwd"],[0,1], hold=False, autoscale=True)

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
