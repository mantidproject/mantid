# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_presenter import PlotFreqFitPanePresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.plot_freq_fit_pane_model import PlotFreqFitPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface


class PlotFreqFitPanePresenterTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=PlotFreqFitPaneModel)
        self.model.name = "freq"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)
        self.figure_presenter.force_autoscale = mock.Mock()
        self.presenter = PlotFreqFitPanePresenter(view=self.view, model=self.model, context=self.context,
                                                  fitting_context=self.context.fitting_context,
                                                  figure_presenter=self.figure_presenter)

    # this GUI can never have rebin data
    def test_rebin_does_nothing(self):
        self.context.gui_context["RebinType"]="Fixed"
        self.presenter.handle_rebin_options_changed()
        self.view.set_raw_checkbox_state.assert_not_called()


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
