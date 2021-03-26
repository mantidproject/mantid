# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from mantidqt.widgets.workspacedisplay.table.presenter_base import TableWorkspaceDataPresenterBase
from mantidqt.widgets.workspacedisplay.table.tableworkspace_item import create_table_item


class TableWorkspaceDataPresenterStandard(TableWorkspaceDataPresenterBase):
    def __init__(self, view, model):
        super().__init__(view, model)

    def load_data(self, table):
        num_rows = self.model.get_number_of_rows()
        data_model = table.model()
        data_model.setRowCount(num_rows)

        num_cols = self.model.get_number_of_columns()
        data_model.setColumnCount(num_cols)

        for col in range(num_cols):
            column_data = self.model.get_column(col)
            editable = self.model.is_editable_column(col)
            for row in range(num_rows):
                data_model.setItem(row, col, self.create_item(column_data[row], editable))

    def update_column_headers(self):
        # deep copy the original headers so that they are not changed by the appending of the label
        column_headers = self.model.original_column_headers()
        num_headers = len(column_headers)
        data_model = self.view.model()
        data_model.setColumnCount(num_headers)

        extra_labels = self.model.build_current_labels()
        if len(extra_labels) > 0:
            for index, label in extra_labels:
                column_headers[index] += str(label)

        data_model.setHorizontalHeaderLabels(column_headers)

    @staticmethod
    def create_item(data, editable):
        """Create a QStandardItemModel for the data
        :param data: The typed data to store
        :param editable: True if it should be editable in the view
        """
        return create_table_item(data, editable)
