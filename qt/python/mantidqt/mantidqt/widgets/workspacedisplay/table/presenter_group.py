# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from mantidqt.widgets.workspacedisplay.table.presenter_base import TableWorkspaceDataPresenterBase


class TableWorkspaceDataPresenterGroup(TableWorkspaceDataPresenterBase):
    def __init__(self, view, model):
        super().__init__(view, model)
        self.sort_data = None

    def load_data(self, table):
        table.model().load_data(self.model)
        if self.sort_data is not None:
            self.sort(*self.sort_data)

    def update_column_headers(self):
        # deep copy the original headers so that they are not changed by the appending of the label
        column_headers = self.model.original_column_headers()
        extra_labels = self.model.build_current_labels()
        if len(extra_labels) > 0:
            for index, label in extra_labels:
                column_headers[index] += str(label)

        self.view.model().setHorizontalHeaderLabels(column_headers)

    def delete_rows(self, selected_rows):
        group_ws_rows = []
        model = self.view.model()
        for row in selected_rows:
            group_index = int(model.data(model.index(row, 0)))
            ws_index = int(model.data(model.index(row, 1)))
            group_ws_rows.append((group_index, ws_index))
        self.model.delete_rows(group_ws_rows)

    def sort(self, selected_column, sort_ascending):
        self.view.sortBySelectedColumn(selected_column, sort_ascending)
        self.sort_data = (selected_column, sort_ascending)
