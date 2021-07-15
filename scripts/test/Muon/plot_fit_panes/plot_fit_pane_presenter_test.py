# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.MuonAnalysis.plot_widget.plot_time_fit_pane_model import PlotTimeFitPaneModel
from Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter
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
        self.context = mock.MagicMock()
        self.model = mock.Mock(spec=PlotTimeFitPaneModel)
        self.model.name = "data"
        self.view = mock.Mock(spec=BasePaneView)
        self.view.warning_popup = mock.MagicMock()
        self.figure_presenter = mock.Mock(spec=PlottingCanvasPresenterInterface)

        self.presenter = PlotFitPanePresenter(view=self.view, model=self.model, context=self.context,
                                              fitting_context=self.context.fitting_context,
                                              figure_presenter=self.figure_presenter)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_handle_plot_selected_fit(self):
        fits = []
        a= MockFitInfo(["unit", "test"])
        fits.append(a)
        self.model.get_fit_workspace_and_indices = mock.MagicMock(return_value=(["fit"], [0,1]))
        self.view.is_raw_plot.return_value = mock.MagicMock(False)
        self.view.is_plot_diff.return_value = mock.MagicMock(False)
        self.figure_presenter.plot_workspaces = mock.MagicMock()
        self.presenter.handle_plot_selected_fits(fits)

    def test_handle_use_raw_workspace_changed(self):
        self.presenter.check_if_can_use_rebin = mock.Mock(return_value=True)
        self.presenter.handle_plot_selected_fits = mock.Mock()
        self.presenter._current_fit_info = mock.Mock()

        self.presenter.handle_use_raw_workspaces_changed()
        self.presenter.handle_plot_selected_fits.assert_called_once_with(self.presenter._current_fit_info)

    def test_handle_use_raw_workspace_changed_fail(self):
        self.presenter.check_if_can_use_rebin = mock.Mock(return_value=False)
        self.presenter.handle_plot_selected_fits = mock.Mock()

        self.presenter.handle_use_raw_workspaces_changed()
        self.presenter.handle_plot_selected_fits.assert_not_called()

    def test_match_raw_selection(self):
        self.context.fitting_context._fit_to_raw = True
        name1 = "MUSR62260; Group; unit; Asymmetry; MA"
        names = self.presenter.match_raw_selection([name1], False)
        self.assertEqual(names, ["MUSR62260; Group; unit; Asymmetry; Rebin; MA"])

    def test_match_raw_selection_fit_rebin(self):
        self.context.fitting_context._fit_to_raw = False
        name1 = "MUSR62260; Group; unit; Asymmetry; MA"
        names = self.presenter.match_raw_selection([name1], False)
        self.assertEqual(names, ["MUSR62260; Group; unit; Asymmetry; Rebin; MA"])

    def test_match_raw_selection_plot_raw(self):
        self.context.fitting_context._fit_to_raw = True
        name1 = "MUSR62260; Group; unit; Asymmetry; MA"
        names = self.presenter.match_raw_selection([name1], True)
        self.assertEqual(names, ["MUSR62260; Group; unit; Asymmetry; MA"])

    def test_match_raw_selection_fit_rebin_plot_raw(self):
        self.context.fitting_context._fit_to_raw = False
        name1 = "MUSR62260; Group; unit; Asymmetry; MA"
        names = self.presenter.match_raw_selection([name1], True)
        self.assertEqual(names, ["MUSR62260; Group; unit; Asymmetry; MA"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
