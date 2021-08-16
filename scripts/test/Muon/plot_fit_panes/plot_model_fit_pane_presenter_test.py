# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.Common.plot_widget.model_fit_pane.plot_model_fit_pane_presenter import PlotModelFitPanePresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from mantid import AnalysisDataService

from mantidqt.utils.qt.testing import start_qapplication


class MockFitInfo(object):
    def __init__(self, name):
        self.fit = "FlatBackground"
        self.input_workspaces = name


@start_qapplication
class PlotFitPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = setup_context()
        self.model = mock.Mock(spec=PlotTimeFitPaneModel)
        self.model.name = "data"
        # manually add pane
        self.context.plot_panes_context.add_pane(self.model.name)

        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.view.disable_plot_raw_option()
        self.view.hide_plot_type = mock.Mock()
        self.view.hide_plot_raw = mock.Mock()
        self.view.hide_tiled_by = mock.Mock()

        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.figure_presenter.set_errors = mock.Mock()
        self.figure_presenter.set_plot_range = mock.Mock()

        self.presenter = PlotModelFitPanePresenter(view=self.view, model=self.model, context=self.context,
                                                   fitting_context=self.context.fitting_context,
                                                   figure_presenter=self.figure_presenter)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_that_autoscale_and_errors_are_turned_on(self):
        self.figure_presenter.set_errors.assert_called_once_with(True)

    def test_that_the_expected_options_are_hidden_and_disabled(self):
        self.view.disable_plot_raw_option.assert_called_with()
        self.view.hide_plot_type.assert_called_with()
        self.view.hide_plot_raw.assert_called_with()
        self.view.hide_tiled_by.assert_called_with()

    def test_that_update_x_plot_range_will_update_the_x_range_with_a_ten_percent_margin(self):
        limits = [-1.0, 1.0]

        self.presenter.update_x_plot_range(limits)

        self.figure_presenter.set_plot_range.assert_called_once_with([-1.2, 1.2])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
