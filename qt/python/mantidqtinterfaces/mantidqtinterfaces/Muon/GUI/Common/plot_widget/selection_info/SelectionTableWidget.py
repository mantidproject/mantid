# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.QSelectionTableView import QSelectionTableView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.selection_info.QSelectionTableModel import (QSelectionTableModel, GROUP_COLUMN,
                                                                                                RUN_COLUMN, WORKSPACE_COLUMN)
from collections import namedtuple

WorkspaceInfo = namedtuple('Workspace', 'runs groups')


class SelectionTableWidget(object):
    """ Selection table widget implemented using a QTableView and QAbstractTableModel
    Based on the model-view pattern https://doc.qt.io/qt-5/model-view-programming.html"""

    def __init__(self, parent, view=None, model=None):
        self._view = view if view else QSelectionTableView(parent)
        self._model = model if model else QSelectionTableModel()
        self._view.setModel(self._model)

    @property
    def widget(self):
        return self._view

    def block_signals(self, state):
        self._view.blockSignals(state)

    def hide_workspace_column(self):
        self._view.hideColumn(WORKSPACE_COLUMN)

    def hide_run_column(self):
        self._view.hideColumn(RUN_COLUMN)

    def hide_group_column(self):
        self._view.hideColumn(GROUP_COLUMN)

    def set_workspaces(self, workspace_names, runs, group_and_pairs):
        self.block_signals(True)
        self._model.set_workspaces(workspace_names, runs, group_and_pairs)
        self.block_signals(False)

    def set_selection_to_last_row(self):
        self._view.set_selection_to_last_row()

    def clear_workspaces(self):
        self._model.clear_workspaces()

    def clear_selection(self):
        self._view.clearSelection()

    def get_selected_rows(self):
        rowSelectionModels = self._view.selectionModel().selectedRows()
        rows = []
        for model in rowSelectionModels:
            rows += [model.row()]
        return rows

    def get_workspace_names_from_row(self, row):
        return self._model.get_workspace_name_information(row)

    def get_workspace_info_from_row(self, row):
        if row > self._model.rowCount():
            return WorkspaceInfo([], [])

        run_numbers = self.get_runs_from_row(row)
        group_and_pairs = self.get_groups_and_pairs_from_row(row)
        return WorkspaceInfo(run_numbers, group_and_pairs)

    def get_runs_from_row(self, row):
        return self._model.get_run_information(row)

    def get_groups_and_pairs_from_row(self, row):
        return self._model.get_group_information(row)

    def setup_slot_for_row_selection_changed(self, slot):
        self._view.clicked.connect(slot)

    def set_slot_for_key_up_down_pressed(self, slot):
        self._view.keyUpDownPressed.connect(slot)

    def get_names_and_rows(self):
        return self._model.get_names_and_rows()

    def set_selected_rows(self, rows):
        self._view.select_rows(rows)
