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
        self.setup_table()
        self.view.set_context_menu_actions(self.view.table_y, self.model._ws.readY)
        self.view.set_context_menu_actions(self.view.table_x, self.model._ws.readX)
        self.view.set_context_menu_actions(self.view.table_e, self.model._ws.readE)

    def clicked(self):
        self.update_stats()
        self.plot_logs()

    def doubleClicked(self, i):
        # print the log, later this should display the data in a table workspace or something
        log_text = self.view.get_row_log_name(i.row())
        log = self.model.get_log(log_text)
        print('# {}'.format(log.name))
        print(log.valueAsPrettyStr())
        if self.model.is_log_plottable(log_text):
            self.view.new_plot_log(self.model.get_ws(), self.model.get_exp(), log_text)

    def changeExpInfo(self):
        selected_rows = self.view.get_selected_row_indexes()
        self.model.set_exp(self.view.get_exp())
        self.setup_table()
        self.view.set_selected_rows(selected_rows)
        self.update_stats()
        self.plot_logs()

    def update_stats(self):
        selected_rows = self.view.get_selected_row_indexes()
        if len(selected_rows) == 1:
            stats = self.model.get_statistics(self.view.get_row_log_name(selected_rows[0]))
            if stats:
                self.view.set_statistics(stats)
            else:
                self.view.clear_statistics()
        else:
            self.view.clear_statistics()

    def plot_logs(self):
        selected_rows = self.view.get_selected_row_indexes()
        to_plot = [row for row in selected_rows if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def setup_table(self):
        # unpacks the list of models returned from getItemModel
        self.view.set_model(*self.model.get_item_model())

    def action_copy_spectrum_values(self, table, ws_read, *args):
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            # TODO Consider displaying a tooltip that says "No Selection" or something
            return
        selected_rows = selection_model.selectedRows()  # type: list
        row_data = []

        for index in selected_rows:
            row = index.row()
            data = " ".join([str(x) for x in ws_read(row)])

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
        self.copy_to_clipboard(data)

    @staticmethod
    def action_view_detectors_table(table, *args):
        print("I am in actionViewDetectorsTable")
        index = table.selectionModel().currentIndex()
        print("Currently selected row:", index.row())
        print("Currently selected col:", index.column())
