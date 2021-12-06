# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.selection_info_view import SelectionInfoView
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (get_group_or_pair_from_name,get_run_number_from_workspace_name)


class SelectionInfoPresenter(object):
    def __init__(self, context, parent):
        self._lines = {}
        self._view = SelectionInfoView(parent)
        self._view.hide()
        self._context = context

    def setup_slot_for_row_selection_changed(self, slot):
        self._view.selection_table.setup_slot_for_row_selection_changed(slot)

    def set_selected_rows_from_name(self, names):
        names_and_rows = self._view.selection_table.get_names_and_rows()
        rows = [ names_and_rows[name] for name in names]
        self._view.selection_table.set_selected_rows(rows)

    def update_lines(self, ws_list, indicies, to_plot=[]):
        self._lines = {}
        for name, index in zip(ws_list, indicies):
            self._lines[name]=index
        return self.handle_selected_workspaces_changed(ws_list, to_plot)

    def handle_selected_workspaces_changed(self, workspace_names, to_plot=[]):
        runs, groups_and_pairs = self.get_runs_groups_and_pairs(workspace_names)
        self._view.selection_table.set_workspaces(workspace_names, runs, groups_and_pairs)
        if to_plot:
            self.set_selected_rows_from_name(to_plot)
        if len(self._view.selection_table.get_selected_rows()) == 0:
            self._view.selection_table.set_selection_to_last_row()
        return self.get_selection()

    def get_selection(self):
        rows = self._view.selection_table.get_selected_rows()
        lines_to_plot = {}
        for row in rows:
            name = self._view.selection_table.get_workspace_names_from_row(row)
            lines_to_plot[name] = self._lines[name]
        return lines_to_plot

    def get_runs_groups_and_pairs(self, ws_names):
        group_pair =[]
        runs = []
        for name in ws_names:
            group_pair.append(get_group_or_pair_from_name(name))
            runs.append(get_run_number_from_workspace_name(name,self._context._data_context.instrument))
        return runs, group_pair
