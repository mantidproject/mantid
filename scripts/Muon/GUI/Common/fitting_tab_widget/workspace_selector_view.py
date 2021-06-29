# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext

ui_workspace_selector, _ = load_ui(__file__, "workspace_selector.ui")

frequency_domain = "Frequency "


class WorkspaceSelectorView(QtWidgets.QDialog, ui_workspace_selector):
    def __init__(self, current_runs, instrument, current_workspaces, fit_to_raw, plot_type, context, parent_widget=None):
        super(QtWidgets.QDialog, self).__init__(parent=parent_widget)
        self.setupUi(self)
        self.current_runs = current_runs
        self.current_workspaces = current_workspaces
        self.instrument = instrument
        self.context = context
        self.rebin = not fit_to_raw

        self.groups_and_pairs = 'All'
        self.runs = 'All'

        self.select_button.clicked.connect(self.accept)
        self.cancel_button.clicked.connect(self.reject)

        self.list_selector_widget = ListSelectorView(self)
        self.list_selector_layout.addWidget(self.list_selector_widget, 0, 1, 4, 1)
        self.list_selector_layout.setContentsMargins(0, 0, 0, 0)
        self.list_selector_presenter = ListSelectorPresenter(self.list_selector_widget, self.get_workspace_list())
        self.update_list()
        self.list_selector_presenter.update_view_from_model()

        self.group_pair_line_edit.editingFinished.connect(self.handle_group_pair_selection_changed)
        self.run_line_edit.editingFinished.connect(self.handle_run_edit_changed)
        self.phase_quad_checkbox.stateChanged.connect(self.handle_phase_quad_changed)

        self.time_domain_combo.addItem(frequency_domain+FREQUENCY_EXTENSIONS["MOD"])
        self.time_domain_combo.addItem(frequency_domain+FREQUENCY_EXTENSIONS["MAXENT"])
        self.time_domain_combo.addItem(frequency_domain+FREQUENCY_EXTENSIONS["RE"])
        self.time_domain_combo.addItem(frequency_domain+FREQUENCY_EXTENSIONS["IM"])

        self.time_domain_combo.currentIndexChanged.connect(self.time_or_freq)
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            index = self.time_domain_combo.findText(plot_type)
            if index == -1:
                index = 0
            self.time_domain_combo.setEnabled(True)
            self.time_domain_combo.setCurrentIndex(index)

    def get_workspace_list(self):
        filtered_list = self.context.get_names_of_workspaces_to_fit(runs='All', group_and_pair='All',
                                                                    rebin=self.rebin, freq = "All")

        filtered_list += self.context.get_names_of_workspaces_to_fit(runs='All', group_and_pair='All',
                                                                     rebin=self.rebin, freq = "None")

        filtered_list = [item for item in filtered_list if item not in self.current_workspaces]

        filtered_list = self.current_workspaces + filtered_list

        model_dict = {}
        for index, item in enumerate(filtered_list):
            model_dict.update({item: [index, item in self.current_workspaces, True]})

        return model_dict

# make get_names_of_workspaces_to_fit handle if freq or time, default to time
    def get_exclusion_list(self):

        filtered_list = self.context.get_names_of_workspaces_to_fit(runs=self.runs,
                                                                    group_and_pair=self.groups_and_pairs,
                                                                    rebin=self.rebin, freq = self.is_it_freq)

        excluded_list = self.context.get_names_of_workspaces_to_fit(runs='All', group_and_pair='All',
                                                                    rebin=self.rebin, freq="None")
        # add frequency list to excluded - needed for searching
        excluded_list += self.context.get_names_of_workspaces_to_fit(runs='All', group_and_pair='All',
                                                                     rebin=self.rebin, freq="All")

        excluded_list = [item for item in excluded_list if item not in filtered_list]

        return excluded_list

    def handle_group_pair_selection_changed(self):
        new_value = str(self.group_pair_line_edit.text())
        if not new_value:
            new_value = 'All'

        self.groups_and_pairs = new_value

        self.update_list()

    def handle_run_edit_changed(self):
        new_value = str(self.run_line_edit.text())
        if not new_value:
            new_value = 'All'

        self.runs = new_value

        self.update_list()

    def handle_phase_quad_changed(self):
        self.update_list()

    def update_list(self):
        workspace_list = self.get_exclusion_list()
        self.list_selector_presenter.update_filter_list(workspace_list)

    def get_selected_list(self):
        return self.list_selector_presenter.get_selected_items()

    @property
    def phasequad(self):
        return self.phase_quad_checkbox.isChecked()

    @staticmethod
    def get_selected_data(current_runs, instrument, current_workspaces, fit_to_raw, plot_type, context, parent):
        dialog = WorkspaceSelectorView(current_runs, instrument, current_workspaces, fit_to_raw, plot_type, context, parent)

        result = dialog.exec_()

        selected_list = dialog.get_selected_list()

        return(selected_list, result == QtWidgets.QDialog.Accepted)

    @property
    def is_it_freq(self):
        if frequency_domain in self.time_domain_combo.currentText():
            return self.time_domain_combo.currentText()[len(frequency_domain):]
        return "None"

    def time_or_freq(self):
        self.update_list()
