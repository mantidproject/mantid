# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from mantidqt.widgets.workspacedisplay.table.presenter_base import TableWorkspaceDataPresenterBase


class TableWorkspaceDataPresenterBatch(TableWorkspaceDataPresenterBase):
    def __init__(self, view, model):
        super().__init__(view, model)

    def load_data(self, table):
        table.model().load_data(self.model)

    def update_column_headers(self):
        # deep copy the original headers so that they are not changed by the appending of the label
        column_headers = self.model.original_column_headers()
        table_item_model = self.view.model()
        extra_labels = self.model.build_current_labels()
        if len(extra_labels) > 0:
            for index, label in extra_labels:
                column_headers[index] += str(label)

        table_item_model.setHorizontalHeaderLabels(column_headers)
