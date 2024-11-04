# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.features.add_feature import AddFeature
from mantidqtinterfaces.Muon.GUI.Common.model_fitting_tab_widget.model_fitting_tab_widget import ModelFittingTabWidget


MODELANALYSIS = "model_analysis"
TABONLY = 1
TABANDPLOT = 2


class AddModelAnalysis(AddFeature):
    """
    Add model analysis to the GUI
    """

    def __init__(self, GUI, feature_dict):
        super().__init__(GUI, feature_dict)

    def _get_features(self, feature_dict):
        features = []
        if MODELANALYSIS in feature_dict.keys() and feature_dict[MODELANALYSIS] == TABONLY:
            features.append(TABONLY)
        if MODELANALYSIS in feature_dict.keys() and feature_dict[MODELANALYSIS] == TABANDPLOT:
            features.append(TABANDPLOT)
        return features

    def _add_features(self, GUI):
        if TABANDPLOT in self.feature_list:
            GUI.model_fitting_tab = ModelFittingTabWidget(GUI.context, GUI)
            GUI.plot_widget.create_model_fit_pane()

        elif TABONLY in self.feature_list:
            GUI.model_fitting_tab = ModelFittingTabWidget(GUI.context, GUI)

    def add_to_tab(self, GUI):
        if TABONLY in self.feature_list or TABANDPLOT in self.feature_list:
            GUI.tabs.addTabWithOrder(GUI.model_fitting_tab.model_fitting_tab_view, "Model Fitting")

    def add_observers_to_feature(self, GUI):
        if TABONLY in self.feature_list or TABANDPLOT in self.feature_list:
            GUI.results_tab.results_tab_presenter.results_table_created_notifier.add_subscriber(
                GUI.model_fitting_tab.model_fitting_tab_presenter.results_table_created_observer
            )
            GUI.disable_notifier.add_subscriber(GUI.model_fitting_tab.model_fitting_tab_view.disable_tab_observer)

            GUI.enable_notifier.add_subscriber(GUI.model_fitting_tab.model_fitting_tab_view.enable_tab_observer)

            GUI.context.data_context.instrumentNotifier.add_subscriber(
                GUI.model_fitting_tab.model_fitting_tab_presenter.instrument_changed_notifier
            )

    def set_feature_observables(self, GUI):
        if TABONLY in self.feature_list or TABANDPLOT in self.feature_list:
            GUI.model_fitting_tab.model_fitting_tab_presenter.enable_editing_notifier.add_subscriber(GUI.enable_observer)

            GUI.model_fitting_tab.model_fitting_tab_presenter.disable_editing_notifier.add_subscriber(GUI.disable_observer)

            # plotting
        if TABANDPLOT in self.feature_list:
            GUI.model_fitting_tab.model_fitting_tab_presenter.remove_plot_guess_notifier.add_subscriber(
                GUI.plot_widget.model_fit_mode.remove_plot_guess_observer
            )

            GUI.model_fitting_tab.model_fitting_tab_presenter.update_plot_guess_notifier.add_subscriber(
                GUI.plot_widget.model_fit_mode.update_plot_guess_observer
            )
            GUI.model_fitting_tab.model_fitting_tab_presenter.selected_fit_results_changed.add_subscriber(
                GUI.plot_widget.model_fit_mode.plot_selected_fit_observer
            )

            GUI.model_fitting_tab.model_fitting_tab_presenter.update_plot_x_range_notifier.add_subscriber(
                GUI.plot_widget.model_fit_mode.update_x_range_observer
            )

            GUI.model_fitting_tab.model_fitting_tab_presenter.update_override_tick_labels_notifier.add_subscriber(
                GUI.plot_widget.model_fit_mode.update_override_tick_labels_observer
            )
