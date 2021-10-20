# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.dock.dockable_tabs import DetachableTabWidget
from mantidqtinterfaces.Muon.GUI.Common.features.model_analysis import AddModelAnalysis
from mantidqtinterfaces.Muon.GUI.Common.results_tab_widget.results_tab_widget import ResultsTabWidget
from mantidqtinterfaces.Muon.GUI.Common.model_fitting_tab_widget.model_fitting_tab_widget import ModelFittingTabWidget
from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import MuonAnalysisPlotWidget
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable


TABANDPLOT = {"model_analysis":2}
TAB = {"model_analysis":1}
DONOTHING = {"model_analysis":0}


class AddModelAnalysisTest(unittest.TestCase):

    def setUp(self):
        self.GUI = mock.Mock()
        self.GUI.tabs = mock.MagicMock(autospec=DetachableTabWidget)
        self.GUI.disable_notifier = mock.MagicMock(autospec=GenericObservable)
        self.GUI.enable_notifier = mock.MagicMock(autospec=GenericObservable)
        self.GUI.disable_observer = mock.MagicMock(autospec=GenericObserver)
        self.GUI.enable_observer = mock.MagicMock(autospec=GenericObserver)
        self.GUI.results_tab = mock.MagicMock(autospec=ResultsTabWidget)
        self.GUI.plot_widget = mock.MagicMock(autospec=MuonAnalysisPlotWidget)
        self.GUI.context = mock.Mock()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_get_features_just_tab(self, mock_model):
        AddModelAnalysis(self.GUI, TAB)
        mock_model.assert_called_once_with(self.GUI.context, self.GUI)
        self.GUI.plot_widget.create_model_fit_pane.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_get_features_tab_and_plot(self, mock_model):
        AddModelAnalysis(self.GUI, TABANDPLOT)
        mock_model.assert_called_once_with(self.GUI.context, self.GUI)
        self.GUI.plot_widget.create_model_fit_pane.assert_called_once_with()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_get_features_do_nothing(self, mock_model):
        AddModelAnalysis(self.GUI, DONOTHING)
        mock_model.assert_not_called()
        self.GUI.plot_widget.create_model_fit_pane.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_to_tab(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TABANDPLOT)
        add.add_to_tab(self.GUI)
        self.GUI.tabs.addTabWithOrder.assert_called_once_with(self.GUI.model_fitting_tab.model_fitting_tab_view, "Model Fitting")

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_to_tab_just_tab(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TAB)
        add.add_to_tab(self.GUI)
        self.GUI.tabs.addTabWithOrder.assert_called_once_with(self.GUI.model_fitting_tab.model_fitting_tab_view, "Model Fitting")

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_to_tab_DONOTHING(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, DONOTHING)
        add.add_to_tab(self.GUI)
        self.GUI.tabs.addTabWithOrder.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_observers_to_feature(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TABANDPLOT)
        add.add_observers_to_feature(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        self.GUI.results_tab.results_tab_presenter.results_table_created_notifier.\
                add_subscriber.assert_called_once_with(mock_model.return_value.model_fitting_tab_presenter.results_table_created_observer)
        self.GUI.disable_notifier.add_subscriber.assert_called_once_with(
                mock_model.return_value.model_fitting_tab_view.disable_tab_observer)
        self.GUI.enable_notifier.add_subscriber.assert_called_once_with(mock_model.return_value.model_fitting_tab_view.enable_tab_observer)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_observers_to_feature_just_tab(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TAB)
        add.add_observers_to_feature(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        self.GUI.results_tab.results_tab_presenter.results_table_created_notifier.\
                add_subscriber.assert_called_once_with(mock_model.return_value.model_fitting_tab_presenter.results_table_created_observer)
        self.GUI.disable_notifier.add_subscriber.assert_called_once_with(
                mock_model.return_value.model_fitting_tab_view.disable_tab_observer)
        self.GUI.enable_notifier.add_subscriber.assert_called_once_with(mock_model.return_value.model_fitting_tab_view.enable_tab_observer)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_observers_to_feature_nothing(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, DONOTHING)
        add.add_observers_to_feature(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        self.GUI.results_tab.results_tab_presenter.results_table_created_notifier.add_subscriber.assert_not_called()
        self.GUI.disable_notifier.add_subscriber.assert_not_called()
        self.GUI.enable_notifier.add_subscriber.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_feature_observables(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TABANDPLOT)
        add.set_feature_observables(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        mock_model.return_value.model_fitting_tab_presenter.enable_editing_notifier.\
                add_subscriber.assert_called_once_with(self.GUI.enable_observer)
        mock_model.return_value.model_fitting_tab_presenter.disable_editing_notifier.\
                add_subscriber.assert_called_once_with(self.GUI.disable_observer)

        mock_model.return_value.model_fitting_tab_presenter.remove_plot_guess_notifier.\
                add_subscriber.assert_called_once_with(self.GUI.plot_widget.model_fit_mode.remove_plot_guess_observer)
        mock_model.return_value.model_fitting_tab_presenter.update_plot_guess_notifier.\
                add_subscriber.assert_called_once_with(self.GUI.plot_widget.model_fit_mode.update_plot_guess_observer)
        mock_model.return_value.model_fitting_tab_presenter.selected_fit_results_changed.\
                add_subscriber.assert_called_once_with(self.GUI.plot_widget.model_fit_mode.plot_selected_fit_observer)
        mock_model.return_value.model_fitting_tab_presenter.update_plot_x_range_notifier.\
                add_subscriber.assert_called_once_with(self.GUI.plot_widget.model_fit_mode.update_x_range_observer)
        mock_model.return_value.model_fitting_tab_presenter.update_override_tick_labels_notifier\
                .add_subscriber.assert_called_once_with(self.GUI.plot_widget.model_fit_mode.update_override_tick_labels_observer)

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_feature_observables_just_tab(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, TAB)
        add.set_feature_observables(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        mock_model.return_value.model_fitting_tab_presenter.enable_editing_notifier.add_subscriber.assert_called_once_with(
                self.GUI.enable_observer)
        mock_model.return_value.model_fitting_tab_presenter.disable_editing_notifier.add_subscriber.assert_called_once_with(
                self.GUI.disable_observer)

        mock_model.return_value.model_fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_plot_guess_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.selected_fit_results_changed.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_plot_x_range_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_override_tick_labels_notifier.add_subscriber.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.Common.features.model_analysis.ModelFittingTabWidget')
    def test_add_feature_observables_DONOTHING(self, mock_model):
        mock_model.return_value = mock.MagicMock(autospec = ModelFittingTabWidget)
        add = AddModelAnalysis(self.GUI, DONOTHING)
        add.set_feature_observables(self.GUI)
        # the mock_model.return_value replaces the GUI.model_fitting
        mock_model.return_value.model_fitting_tab_presenter.enable_editing_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.disable_editing_notifier.add_subscriber.assert_not_called()

        mock_model.return_value.model_fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_plot_guess_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.selected_fit_results_changed.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_plot_x_range_notifier.add_subscriber.assert_not_called()
        mock_model.return_value.model_fitting_tab_presenter.update_override_tick_labels_notifier.add_subscriber.assert_not_called()


if __name__ == '__main__':
    unittest.main()
