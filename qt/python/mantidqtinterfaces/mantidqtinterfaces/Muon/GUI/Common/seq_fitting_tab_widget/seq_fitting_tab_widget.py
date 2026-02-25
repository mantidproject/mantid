# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_view import SeqFittingTabView
from mantidqtinterfaces.Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter import SeqFittingTabPresenter


class SeqFittingTabWidget(object):
    def __init__(self, context, model, parent, view=None):
        self.seq_fitting_tab_view = view or SeqFittingTabView(parent)
        self.seq_fitting_tab_model = model

        self.seq_fitting_tab_presenter = SeqFittingTabPresenter(self.seq_fitting_tab_view, self.seq_fitting_tab_model, context)

        self.seq_fitting_tab_view.setup_slot_for_fit_selected_button(self.seq_fitting_tab_presenter.handle_fit_selected_pressed)
        self.seq_fitting_tab_view.setup_slot_for_sequential_fit_button(self.seq_fitting_tab_presenter.handle_sequential_fit_pressed)

        self.seq_fitting_tab_view.fit_table.set_slot_for_parameter_changed(
            self.seq_fitting_tab_presenter.handle_updated_fit_parameter_in_table
        )

        self.seq_fitting_tab_view.fit_table.setup_slot_for_row_selection_changed(
            self.seq_fitting_tab_presenter.handle_fit_selected_in_table
        )

        self.seq_fitting_tab_view.fit_table.set_slot_for_key_up_down_pressed(self.seq_fitting_tab_presenter.handle_fit_selected_in_table)

        is_frequency_domain = isinstance(context, FrequencyDomainAnalysisContext)
        if is_frequency_domain:
            self.seq_fitting_tab_view.fit_table.hide_run_column()
            self.seq_fitting_tab_view.fit_table.hide_group_column()
        else:
            self.seq_fitting_tab_view.hide_data_type_combo_box()
            self.seq_fitting_tab_view.fit_table.hide_workspace_column()

        context.deleted_plots_notifier.add_subscriber(self.seq_fitting_tab_presenter.selected_workspaces_observer)
