# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_presenter import PlotFreqFitPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface


class MockFitInfo(object):
    def __init__(self, name):
        self.fit = "FlatBackground"
        self.input_workspaces = name


class PlotFreqFitPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.context.frequency_context = mock.Mock()
        self.model = mock.Mock(spec=PlotFreqFitPaneModel)
        self.model.name = "Frequency data"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.figure_presenter.force_autoscale = mock.Mock()
        self.presenter = PlotFreqFitPanePresenter(view=self.view, model=self.model, context=self.context,
                                                  fitting_context=self.context.fitting_context,
                                                  figure_presenter=self.figure_presenter)

    def test_handle_data_type_changed(self):
        self.view.get_plot_type = mock.Mock(return_value="Frequency")
        self.presenter.update_freq_units = mock.Mock()
        self.presenter.update_freq_units.notify_subscribers = mock.Mock()
        self.presenter.update_maxent_plot = mock.Mock()
        self.presenter.update_maxent_plot.notify_subscribers = mock.Mock()
        self.context.frequency_context.range = mock.MagicMock(return_value=[1,3])

        self.presenter.handle_data_type_changed()
        self.presenter.update_freq_units.notify_subscribers.assert_called_once_with()
        self.presenter.update_maxent_plot.notify_subscribers.assert_called_once_with()
        self.figure_presenter.set_plot_range.assert_called_once_with([1, 3])
        self.assertEqual(self.context.frequency_context.x_label, "Frequency")

    def test_update_fit_pane_freq(self):
        self.context.frequency_context.unit = mock.Mock(return_value="MHz")
        self.context.frequency_context.range = mock.MagicMock(return_value=[1,3])
        self.presenter.update_freq_units = mock.Mock()
        self.presenter.update_freq_units.notify_subscribers = mock.Mock()

        self.presenter._update_fit_pane()

        self.figure_presenter.set_plot_range.assert_called_once_with([1, 3])
        self.presenter.update_freq_units.notify_subscribers.assert_called_once_with()

        self.view.set_plot_type.assert_called_once_with("Frequency")

    def test_update_fit_pane_field(self):
        self.context.frequency_context.unit = mock.Mock(return_value="Gauss")
        self.context.frequency_context.range = mock.MagicMock(return_value=[1,3])
        self.presenter.update_freq_units = mock.Mock()
        self.presenter.update_freq_units.notify_subscribers = mock.Mock()

        self.presenter._update_fit_pane()

        self.figure_presenter.set_plot_range.assert_called_once_with([1, 3])
        self.presenter.update_freq_units.notify_subscribers.assert_called_once_with()
        self.view.set_plot_type.assert_called_once_with("Field")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
