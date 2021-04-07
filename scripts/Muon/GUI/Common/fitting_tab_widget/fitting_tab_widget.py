# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_presenter import FittingTabPresenter
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FittingTabModel
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext

MUON_ANALYSIS_FITTING_OPTIONS = ["Run", "Group/Pair"]
FREQUENCY_DOMAIN_ANALYSIS_FITTING_OPTIONS = []


class FittingTabWidget(object):
    def __init__(self, context, parent):
        is_frequency_domain = isinstance(context, FrequencyDomainAnalysisContext)
        fitting_options = FREQUENCY_DOMAIN_ANALYSIS_FITTING_OPTIONS if is_frequency_domain\
            else MUON_ANALYSIS_FITTING_OPTIONS

        self.fitting_tab_view = FittingTabView(simultaneous_item_list=fitting_options, is_frequency_domain=is_frequency_domain,
                                               parent=parent)
        self.fitting_tab_model = FittingTabModel(context)
        self.fitting_tab_presenter = FittingTabPresenter(self.fitting_tab_view, self.fitting_tab_model, context)

        self.fitting_tab_view.set_slot_for_fit_generator_clicked(self.fitting_tab_presenter.handle_fit_generator_clicked)
        self.fitting_tab_view.set_slot_for_display_workspace_changed(self.fitting_tab_presenter.handle_display_workspace_changed)
        self.fitting_tab_view.set_slot_for_display_workspace_changed(self.fitting_tab_presenter.handle_plot_guess_changed)
        self.fitting_tab_view.set_slot_for_simul_fit_by_changed(self.fitting_tab_presenter.handle_fit_by_changed)
        self.fitting_tab_view.set_slot_for_simul_fit_specifier_changed(self.fitting_tab_presenter.handle_fit_specifier_changed)
        self.fitting_tab_view.set_slot_for_use_raw_changed(self.fitting_tab_presenter.handle_use_rebin_changed)
        self.fitting_tab_view.set_slot_for_fit_type_changed(self.fitting_tab_presenter.handle_fit_type_changed)
        self.fitting_tab_view.set_slot_for_fit_button_clicked(self.fitting_tab_presenter.handle_fit_clicked)
        self.fitting_tab_view.set_slot_for_start_x_updated(self.fitting_tab_presenter.handle_start_x_updated)
        self.fitting_tab_view.set_slot_for_end_x_updated(self.fitting_tab_presenter.handle_end_x_updated)
        self.fitting_tab_view.set_slot_for_minimiser_changed(self.fitting_tab_presenter.handle_minimiser_changed)
        self.fitting_tab_view.set_slot_for_evaluation_type_changed(self.fitting_tab_presenter.handle_evaluation_type_changed)

        self.fitting_tab_view.function_browser.functionStructureChanged.connect(
            self.fitting_tab_presenter.handle_function_structure_changed)
        self.fitting_tab_view.function_name_line_edit.textChanged.connect(
            self.fitting_tab_presenter.handle_fit_name_changed_by_user)
        self.fitting_tab_view.undo_fit_button.clicked.connect(self.fitting_tab_presenter.handle_undo_fit_clicked)
        context.update_view_from_model_notifier.add_subscriber(self.fitting_tab_presenter.update_view_from_model_observer)
        self.fitting_tab_view.tf_asymmetry_mode_checkbox.stateChanged.connect(self.fitting_tab_presenter.handle_tf_asymmetry_mode_changed)
        self.fitting_tab_view.plot_guess_checkbox.stateChanged.connect(self.fitting_tab_presenter.handle_plot_guess_changed)
        self.fitting_tab_view.function_browser.parameterChanged.connect(self.fitting_tab_presenter.handle_function_parameter_changed)
