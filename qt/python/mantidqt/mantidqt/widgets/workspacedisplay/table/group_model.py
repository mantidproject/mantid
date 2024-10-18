# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# coding=utf-8
#  This file is part of the mantidqt package.
from mantid.api import WorkspaceGroup
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.marked_columns import MarkedColumns
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceColumnTypeMapping, TableWorkspaceDisplayModel
from collections import defaultdict


class GroupTableWorkspaceDisplayModel(TableWorkspaceDisplayModel):
    EDITABLE_COLUMN_NAMES = ["h", "k", "l"]

    ALLOWED_WORKSPACE_TYPES = [WorkspaceGroup]

    @classmethod
    def supports(cls, ws: WorkspaceGroup):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        if not any(isinstance(ws, allowed_type) for allowed_type in cls.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

    def __init__(self, ws: WorkspaceGroup):
        """
        Initialise the model with the workspace
        :param ws: Workspace to be used for providing data
        :raises ValueError: if the workspace is not supported
        """
        self.supports(ws)

        self.ws: WorkspaceGroup = ws
        self.group_name = self.ws.name()
        self.child_names = self.ws.getNames()
        self.ws_num_rows = sum(peakWs.rowCount() for peakWs in ws)
        self.ws_num_cols = 0
        if self.ws.size() != 0:
            self.ws_num_cols = self.ws[0].columnCount() + 2
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()
        self.block_model_replace = False
        self._row_mapping = self._make_row_mapping()
        if self.ws_num_cols:
            self._load_col_types()

    def _make_row_mapping(self):
        """
        Create mapping between table row index and workspace group and spectrum index
        """
        row_index = 0
        row_mapping = []
        for group_index, peaksWs in enumerate(self.ws):
            group_start = row_index
            group_end = row_index + len(peaksWs)
            row_mapping.extend([(group_index, index - group_start) for index in range(group_start, group_end)])
            row_index += len(peaksWs)
        return row_mapping

    def _load_col_types(self):
        for col in range(2, len(self._original_column_headers)):
            plot_type = self.ws[0].getPlotType(col - 2)
            if plot_type == TableWorkspaceColumnTypeMapping.X:
                self.marked_columns.add_x(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.Y:
                self.marked_columns.add_y(col)
            elif plot_type == TableWorkspaceColumnTypeMapping.YERR:
                err_for_column = self.ws[0].getLinkedYCol(col - 2)
                if err_for_column >= 0:
                    self.marked_columns.add_y_err(ErrorColumn(col, err_for_column + 2))

    def build_current_labels(self):
        return self.marked_columns.build_labels()

    def get_name(self):
        return self.group_name

    def get_child_names(self):
        return self.child_names

    def get_column_headers(self):
        if self.ws.size() != 0:
            return ["WS Index", "Group Index"] + self.ws[0].getColumnNames()
        return []

    def get_column(self, index):
        column_data = []

        for i, ws_item in enumerate(self.ws):
            if index == 0:
                column_data.extend([i] * ws_item.rowCount())
            elif index == 1:
                column_data.extend([i for i in range(ws_item.rowCount())])
            else:
                column_data.extend(ws_item.column(index - 2))

        return column_data

    def is_editable_column(self, icol):
        return self.get_column_headers()[icol] in self.EDITABLE_COLUMN_NAMES

    def set_column_type(self, col, type, linked_col_index=-1):
        self.ws[0].setPlotType(col - 2, type, linked_col_index - 2 if linked_col_index != -1 else linked_col_index)

    def get_cell(self, row, column):
        group_index, ws_index = self._row_mapping[row]

        if column == 0:
            return ws_index

        if column == 1:
            return group_index

        column = column - 2

        return self.ws[group_index].cell(ws_index, column)

    def set_cell_data(self, row, col, data, is_v3d):
        group_index, ws_index = row
        col_name = self.get_column_header(col)

        p = self.ws[group_index].getPeak(ws_index)

        if col_name == "h":
            p.setH(data)
        elif col_name == "k":
            p.setK(data)
        elif col_name == "l":
            p.setL(data)

    def delete_rows(self, selected_rows):
        from mantid.simpleapi import DeleteTableRows

        row_to_ws_index = defaultdict(list)
        for group_index, ws_index in selected_rows:
            row_to_ws_index[group_index].append(ws_index)

        for group_index in row_to_ws_index:
            DeleteTableRows(self.ws[group_index], ",".join(map(str, row_to_ws_index[group_index])))

    def get_statistics(self, selected_columns):
        from mantid.simpleapi import StatisticsOfTableWorkspace

        stats = StatisticsOfTableWorkspace(self.ws, selected_columns)
        return stats

    def sort(self, column_index, sort_ascending):
        from mantid.simpleapi import SortPeaksWorkspace

        column_name = self.get_column_header(column_index)

        SortPeaksWorkspace(InputWorkspace=self.ws, OutputWorkspace=self.ws, ColumnNameToSortBy=column_name, SortAscending=sort_ascending)
