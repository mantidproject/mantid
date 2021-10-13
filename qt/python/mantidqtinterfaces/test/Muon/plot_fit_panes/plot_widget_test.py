# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from unittest import mock

from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel


from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_presenter import MainPlotWidgetPresenter


@start_qapplication
class PlotWidgetTest(unittest.TestCase):

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view.BasePaneView')
    def setUp(self, basepane_mock):
        self.context = setup_context()
        self.count = 0
        user_mocked_vals = "one", [3.0, 8.0], 2, [-0.323, 0.436], False
        self.data_canvas = self.set_plot_values(user_mocked_vals)
        self.fit_canvas = mock.Mock(spec=PlottingCanvasWidget)
        basepane_mock_return = mock.MagicMock()
        basepane_mock.return_value = basepane_mock_return
        self.plot_widget = MuonAnalysisPlotWidget(self.context)
        self.plot_widget.data_model = mock.Mock(spec=PlotDataPaneModel)
        self.plot_widget.fit_model = mock.Mock(spec=PlotTimeFitPaneModel)
        self.plot_widget.view = mock.Mock(spec=MainPlotWidgetView)
        self.plot_widget.models = mock.Mock()
        self.plot_widget._current_plot_mode = self.change_plot_mode()
        self.plot_widget.presenter = mock.Mock(spec=MainPlotWidgetPresenter)
        self.plot_widget.plotting_canvas_widgets["Plot Data"] = self.data_canvas
        self.plot_widget.plotting_canvas_widgets["Fit Data"] = self.fit_canvas

    def change_plot_mode(self):
        if self.count == 0:
            self.count += 1
            return "Plot Data"
        else:
            return "Fit Data"

    #sets the values of the Plotting Canvas Widget
    def set_plot_values(self, mocked_vals):
        mock_injector = mock.NonCallableMock()
        mock_injector.get_quick_edit_info = mocked_vals
        return mock_injector

    def test_handle_plot_mode_changed_by_user_correct_x_axis(self):
        self.assertEqual(self.plot_widget._current_plot_mode, "Plot Data")
        self.plot_widget.presenter.get_plot_mode = self.change_plot_mode()

        self.plot_widget.handle_plot_mode_changed_by_user()

        self.assertEqual(self.plot_widget._current_plot_mode, "Fit Data")
        self.plot_widget.plotting_canvas_widgets["Fit Data"].set_quick_edit_info.assert_called_once_with\
            ("one", [3.0, 8.0], 2, [-0.323, 0.436], False)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
