# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_table import EAAutoPopupTable
from mantidqt.utils.observer_pattern import GenericObserver
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, check_if_workspace_exist
from Muon.GUI.Common import message_box


class EAAutoTabPresenter(object):

    def __init__(self, context, view, model, match_table):
        self.view = view
        self.model = model
        self.context = context
        self.match_table_presenter = match_table

        self.popup_match_table = None
        self.popup_peak_table = None

        self.find_peaks_observer = GenericObserver(self.run_find_peak_algorithms)
        self.show_peaks_table_observer = GenericObserver(self.show_peaks_table)
        self.show_match_table_observer = GenericObserver(self.show_match_table)
        self.update_match_table_observer = GenericObserver(self.update_match_table)
        self.update_view_observer = GenericObserver(self.update_view)

        self.enable_tab_observer = GenericObserver(self.enable_tab)
        self.disable_tab_observer = GenericObserver(self.disable_tab)

        self.setup_notifier()

    def setup_notifier(self):
        self.view.find_peaks_notifier.add_subscriber(self.find_peaks_observer)
        self.view.show_peaks_table_notifier.add_subscriber(self.show_peaks_table_observer)
        self.view.show_match_table_notifier.add_subscriber(self.show_match_table_observer)
        self.model.update_match_table_notifier.add_subscriber(self.update_match_table_observer)
        self.model.update_view_notifier.add_subscriber(self.update_view_observer)

    def run_find_peak_algorithms(self):
        parameters = self.view.get_parameters_for_find_peaks()
        if parameters is None:
            return
        self.model.handle_peak_algorithms(parameters)

    def show_peaks_table(self):
        parameter = self.view.show_peaks_table_combobox.currentText()
        if parameter == "":
            message_box.warning("ERROR : No selected table", None)
            return
        elif not check_if_workspace_exist(parameter):
            message_box.warning(f"ERROR : {parameter} Table does not exist", None)
            return
        if self.popup_peak_table is not None:
            del self.popup_peak_table
            self.popup_peak_table = EAAutoPopupTable(parameter)
            self.popup_peak_table.show()
        else:
            self.popup_peak_table = EAAutoPopupTable(parameter)
            self.popup_peak_table.show()

    def show_match_table(self):
        parameter = self.view.show_match_table_combobox.currentText()
        if parameter == "":
            message_box.warning("ERROR : No selected table", None)
            return
        elif not check_if_workspace_exist(parameter):
            message_box.warning(f"ERROR : {parameter} Table does not exist", None)
            return
        if self.popup_match_table is not None:
            del self.popup_match_table
            self.popup_match_table = EAAutoPopupTable(parameter)
            self.popup_match_table.show()
        else:
            self.popup_match_table = EAAutoPopupTable(parameter)
            self.popup_match_table.show()

    def update_match_table(self):
        while not self.model.table_entries.empty():
            self.match_table_presenter.update_table(self.model.table_entries.get())

    def update_view(self):
        # Check context for loaded workspaces and add to values find peak combobox
        # check all tables in ads or in context and add to show peaks and show matches combobox
        print("update view")
        group_names = self.context.group_context.group_names
        self.view.add_options_to_find_peak_combobox(group_names)
        all_runs = []
        for group in group_names:
            all_runs.append(self.model.split_run_and_detector(group)[0])

        show_peaks_options = []
        # show_matches_option = []
        all_runs = list(set(all_runs))
        for run in all_runs:
            group = retrieve_ws(run)
            workspace_names = group.getNames()
            for name in workspace_names:
                if len(name) > 15:
                    if name[-15:] == "_refitted_peaks":
                        show_peaks_options.append(name)
                        continue
                if len(name) > 6:
                    if name[-6:] == "_peaks":
                        show_peaks_options.append(name)

        self.view.add_options_to_show_peak_combobox(show_peaks_options)

        peak_label_info = self.model.current_peak_table_info
        # Update peak info label
        if peak_label_info["workspace"] is not None and peak_label_info["number_of_peaks"] is not None:
            self.view.set_peak_info(**peak_label_info)

    def enable_tab(self):
        self.view.enable()

    def disable_tab(self):
        self.view.disable()
