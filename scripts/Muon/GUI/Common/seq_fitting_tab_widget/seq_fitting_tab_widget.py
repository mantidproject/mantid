# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_view import SeqFittingTabView
from Muon.GUI.Common.seq_fitting_tab_widget.seq_fitting_tab_presenter import SeqFittingTabPresenter


class SeqFittingTabWidget(object):
    def __init__(self, context, model, parent):
        self.seq_fitting_tab_view = SeqFittingTabView(parent)
        self.seq_fitting_tab_model = model

        self.seq_fitting_tab_presenter = SeqFittingTabPresenter(self.seq_fitting_tab_view, self.seq_fitting_tab_model,
                                                                context)

        self.seq_fitting_tab_view.setup_slot_for_fit_selected_button(self.seq_fitting_tab_presenter.
                                                                     handle_fit_selected_pressed)
        self.seq_fitting_tab_view.setup_slot_for_sequential_fit_button(self.seq_fitting_tab_presenter.
                                                                       handle_sequential_fit_all_pressed)
        self.seq_fitting_tab_view.setup_slot_for_table_parameter_changed(
            self.seq_fitting_tab_presenter.handle_updated_fit_parameter_in_table)

        self.seq_fitting_tab_view.setup_slot_for_table_selection_changed(
            self.seq_fitting_tab_presenter.handle_fit_selected_in_table)

        self.seq_fitting_tab_view.setup_slot_for_key_up_down(
            self.seq_fitting_tab_presenter.handle_fit_selected_in_table)

        self.seq_fitting_tab_view.setup_slot_for_key_enter_pressed(
            self.seq_fitting_tab_presenter.handle_sequential_fit_requested)


