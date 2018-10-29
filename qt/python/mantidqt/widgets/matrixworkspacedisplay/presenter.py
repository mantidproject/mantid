# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

from .model import MatrixWorkspaceDisplayModel
from .view import MatrixWorkspaceDisplayView


class MatrixWorkspaceDisplay(object):
    def __init__(self, ws, parent=None, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else MatrixWorkspaceDisplayModel(ws)
        self.view = view if view else MatrixWorkspaceDisplayView(self,
                                                                 parent,
                                                                 self.model.get_name())
        self.setup_tables()
        self.view.set_context_menu_actions(self.view.table_y, self.model._ws.readY)
        self.view.set_context_menu_actions(self.view.table_x, self.model._ws.readX)
        self.view.set_context_menu_actions(self.view.table_e, self.model._ws.readE)

    def setup_tables(self):
        # unpacks the list of models returned from getItemModel
        self.view.set_model(*self.model.get_item_model())

    def action_copy_spectrum_values(self, table, ws_read, *args):
        """
        Copies the values selected by the user to the system's clipboard

        :param table: Table from which the selection will be read
        :param ws_read: The workspace read function, that is used to access the data directly
        :param args: Additional unused parameters passed from Qt
        """
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            # TODO Consider displaying a tooltip that says "No Selection" or something
            return

        selected_rows = selection_model.selectedRows()  # type: list
        row_data = []

        for index in selected_rows:
            row = index.row()
            data = " ".join(map(str, ws_read(row)))

            row_data.append(data)

        self.view.copy_to_clipboard("\n".join(row_data))

    def action_copy_bin_values(self, table, ws_read, *args):
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            # TODO Consider displaying a tooltip that says "No Selection" or something
            return
        selected_columns = selection_model.selectedColumns()  # type: list

        # Qt gives back a QModelIndex, we need to extract the column from it
        num_rows = self.model._ws.getNumberHistograms()
        column_data = []
        for index in selected_columns:
            column = index.column()
            data = [str(ws_read(row)[column]) for row in range(num_rows)]
            column_data.append(data)

        all_string_rows = []
        for i in range(num_rows):
            # Appends ONE value from each COLUMN, this is because the final string is being built vertically
            all_string_rows.append(" ".join([data[i] for data in column_data]))

        # Finally all rows are joined together with a new line at the end of each row
        final_string = "\n".join(all_string_rows)
        # TODO Consider displaying a tooltip that says "Copy Successful" or something
        self.view.copy_to_clipboard(final_string)

    def action_copy_cell(self, table, *args):
        """
        :type table: QTableView
        :param table: The table from which the data will be copied.
        :param args: Arguments passed by Qt. Not used
        :return:
        """
        index = table.selectionModel().currentIndex()
        data = index.sibling(index.row(), index.column()).data()
        self.view.copy_to_clipboard(data)

    @staticmethod
    def action_view_detectors_table(table, *args):
        print("I am in actionViewDetectorsTable")
        index = table.selectionModel().currentIndex()
        print("Currently selected row:", index.row())
        print("Currently selected col:", index.column())
