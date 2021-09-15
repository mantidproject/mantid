# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from unittest import mock

from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel


from Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView


@start_qapplication
class PlotWidgetTest(unittest.TestCase):

    @mock.patch('Muon.GUI.Common.plot_widget.base_pane.base_pane_view.BasePaneView')
    @mock.patch('Muon.GUI.Common.plot_widget.main_plot_widget_presenter.MainPlotWidgetPresenter.get_plot_mode')
    def setUp(self, plot_mode_mock, basepane_mock):
        self.context = setup_context()
        self.count = 0
        self.data_canvas = mock.Mock(spec=PlottingCanvasWidget)
        self.data_canvas.get_quick_edit_info = mock.Mock(return_value=["one", [3.0, 8.0], 2, [-0.323, 0.436], False])
        self.fit_canvas = mock.Mock(spec=PlottingCanvasWidget)
        basepane_mock_return = mock.MagicMock()
        basepane_mock.return_value = basepane_mock_return
        plot_mode_mock.side_effect = self.change_plot_mode
        self.plot_widget = MuonAnalysisPlotWidget(self.context)
        self.plot_widget.data_model = mock.Mock(spec=PlotDataPaneModel)
        self.plot_widget.fit_model = mock.Mock(spec=PlotTimeFitPaneModel)
        self.plot_widget.view = mock.Mock(spec=MainPlotWidgetView)
        self.plot_widget.models = mock.Mock()

    def change_plot_mode(self):
        if self.count == 0:
            self.count += 1
            return self.data_canvas
        else:
            return self.fit_canvas

    def test_handle_plot_mode_changed_by_user_correct_x_axis(self):
        self.plot_widget.handle_plot_mode_changed_by_user()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
