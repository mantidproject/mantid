# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import, division, print_function

import matplotlib.pyplot as plt
from mantid import plots
from .model import MatrixWorkspaceDisplayModel
from .view import MatrixWorkspaceDisplayView


class MatrixWorkspaceDisplay(object):
    NO_SELECTION_MESSAGE = "No selection"
    COPY_SUCCESSFUL_MESSAGE = "Copy Successful"
    A_LOT_OF_SPECTRA_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"

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
            self.show_no_selection_to_copy_toast()
            return
        selected_rows = selection_model.selectedRows()  # type: list
        row_data = []

        for index in selected_rows:
            row = index.row()
            data = " ".join(map(str, ws_read(row)))

            row_data.append(data)

        self.view.copy_to_clipboard("\n".join(row_data))
        self.show_successful_copy_toast()

    def show_no_selection_to_copy_toast(self):
        self.view.show_mouse_toast(self.NO_SELECTION_MESSAGE)

    def show_successful_copy_toast(self):
        self.view.show_mouse_toast(self.COPY_SUCCESSFUL_MESSAGE)

    def action_copy_bin_values(self, table, ws_read, *args):
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.show_no_selection_to_copy_toast()
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
        self.view.copy_to_clipboard(final_string)
        self.show_successful_copy_toast()

    def action_copy_cell(self, table, *args):
        """
        :type table: QTableView
        :param table: The table from which the data will be copied.
        :param args: Arguments passed by Qt. Not used
        :return:
        """
        model = table.selectionModel()
        if not model.hasSelection():
            self.show_no_selection_to_copy_toast()
            return

        index = model.currentIndex()
        data = index.sibling(index.row(), index.column()).data()
        self.view.copy_to_clipboard(data)
        self.show_successful_copy_toast()

    def action_plot_spectrum(self, table, *args):
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.show_no_selection_to_copy_toast()
            return
        selected_rows = selection_model.selectedRows()  # type: list

        if len(selected_rows) > 10 and not self.view.ask_confirmation(
                self.A_LOT_OF_SPECTRA_TO_PLOT_MESSAGE.format(len(selected_rows))):
            return

        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
        for index in selected_rows:
            ax.plot(self.model._ws,
                    label='{}-{}'.format(self.model.get_name(), self.model.get_spectrum_label(index.row())),
                    wkspIndex=index.row())
        ax.legend()
        fig.show()

    def action_plot_spectrum_with_errors(self, table, *args):
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.show_no_selection_to_copy_toast()
            return
        selected_rows = selection_model.selectedRows()  # type: list

        if len(selected_rows) > 10 and not self.view.ask_confirmation(
                self.A_LOT_OF_SPECTRA_TO_PLOT_MESSAGE.format(len(selected_rows))):
            return

        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
        for index in selected_rows:
            row = index.row()
            ax.errorbar(self.model._ws, wkspIndex=row)
            ax.plot(self.model._ws,
                    label='{}-{}'.format(self.model.get_name(), self.model.get_spectrum_label(index.row())),
                    wkspIndex=row)
        ax.legend()
        fig.show()

    #
    # @staticmethod
    # def action_view_detectors_table(table, *args):
    #     print("I am in actionViewDetectorsTable")
    #     index = table.selectionModel().currentIndex()
    #     print("Currently selected row:", index.row())
    #     print("Currently selected col:", index.column())
