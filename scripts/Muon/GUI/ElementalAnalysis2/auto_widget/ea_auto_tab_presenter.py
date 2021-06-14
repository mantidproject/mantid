# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_table import EAAutoPopupTable
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import REFITTED_PEAKS_WS_SUFFIX, PEAKS_WS_SUFFIX, \
    MATCH_GROUP_WS_SUFFIX
from mantidqt.utils.observer_pattern import GenericObserver
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, check_if_workspace_exist
from Muon.GUI.Common import message_box


class EAAutoTabPresenter(object):

    def __init__(self, context, view, model, match_table):
        self.view = view
        self.model = model
        self.context = context
        self.match_table_presenter = match_table

        self.popup_table = None

        self.setup_observers()
        self.setup_notifier()

    def setup_notifier(self):
        self.view.find_peaks_notifier.add_subscriber(self.find_peaks_observer)
        self.view.show_peaks_table_notifier.add_subscriber(self.show_peaks_table_observer)
        self.view.show_matches_table_notifier.add_subscriber(self.show_matches_table_observer)
        self.view.clear_matches_table_notifier.add_subscriber(self.clear_matches_table_observer)
        self.model.update_match_table_notifier.add_subscriber(self.update_match_table_observer)
        self.model.update_view_notifier.add_subscriber(self.update_view_observer)

    def setup_observers(self):
        self.find_peaks_observer = GenericObserver(self.run_find_peak_algorithms)
        self.show_peaks_table_observer = GenericObserver(
            lambda: self.show_table(self.view.show_peaks_table_combobox.currentText()))
        self.show_matches_table_observer = GenericObserver(
            lambda: self.show_table(self.view.show_matches_table_combobox.currentText()))
        self.clear_matches_table_observer = GenericObserver(self.clear_match_table)
        self.update_match_table_observer = GenericObserver(self.update_match_table)
        self.update_view_observer = GenericObserver(self.update_view)

        self.enable_tab_observer = GenericObserver(self.enable_tab)
        self.disable_tab_observer = GenericObserver(self.disable_tab)
        self.group_change_observer = GenericObserver(self.update_view)

    def run_find_peak_algorithms(self):
        parameters = self.view.get_parameters_for_find_peaks()
        if parameters is None:
            return
        self.model.handle_peak_algorithms(parameters)

    def show_table(self, table_name):
        if table_name == "":
            message_box.warning("ERROR : No selected table", None)
            return
        elif not check_if_workspace_exist(table_name):
            message_box.warning(f"ERROR : {table_name} Table does not exist", None)
            return

        self.popup_table = EAAutoPopupTable(table_name)

        table = retrieve_ws(table_name)
        columns = table.getColumnNames()
        self.popup_table.create_table(columns)
        table_entries = self.extract_rows(table_name)
        for entry in table_entries:
            self.popup_table.add_entry_to_table(entry)
        self.popup_table.show()

    def update_match_table(self):
        while not self.model.table_entries.empty():
            self.match_table_presenter.update_table(self.model.table_entries.get())

    def update_view(self):
        """
        Checks context for loaded workspaces and add to values find peak combobox
        Checks all tables in load run's groups and add to show peaks and show matches combobox
        """
        group_names = self.context.group_context.group_names
        find_peak_workspaces = {}

        for group in group_names:
            group_workspace, detector = self.model.split_run_and_detector(group)
            if group_workspace not in find_peak_workspaces:
                find_peak_workspaces[group_workspace] = ["All"]

            find_peak_workspaces[group_workspace].append(detector)

        show_peaks_options = {}
        show_matches_options = {}
        all_runs = list(find_peak_workspaces)

        for run in all_runs:
            group_ws = retrieve_ws(run)
            workspace_names = group_ws.getNames()
            for name in workspace_names:
                if name.endswith(REFITTED_PEAKS_WS_SUFFIX) or name.endswith(PEAKS_WS_SUFFIX):
                    if name.split(";")[0] not in show_peaks_options:
                        show_peaks_options[name.split(";")[0]] = [name]
                    else:
                        show_peaks_options[name.split(";")[0]].append(name)
                    continue
                if name.endswith(MATCH_GROUP_WS_SUFFIX):
                    matches_group = retrieve_ws(name)
                    if name.split(";")[0] not in show_matches_options:
                        show_matches_options[name.split(";")[0]] = matches_group.getNames()
                    else:
                        show_matches_options[name.split(";")[0]].extend(matches_group.getNames())

        self.view.add_options_to_find_peak_combobox(find_peak_workspaces)
        self.view.add_options_to_show_peak_combobox(show_peaks_options)
        self.view.add_options_to_show_matches_combobox(show_matches_options)

        peak_label_info = self.model.current_peak_table_info
        # Update peak info label
        if peak_label_info["workspace"] is not None and peak_label_info["number_of_peaks"] is not None:
            self.view.set_peak_info(**peak_label_info)

    def enable_tab(self):
        self.view.enable()

    def disable_tab(self):
        self.view.disable()

    def clear_match_table(self):
        self.match_table_presenter.clear_table()

    def extract_rows(self, table_name):
        """
        Copies information in a table given the name of the table
        """
        table = retrieve_ws(table_name)
        table_data = table.toDict()
        table_entries = []

        for i in range(table.rowCount()):
            table_entries.append([])
            for column in table_data:
                table_entries[-1].append(str(table_data[column][i]))
        return table_entries
