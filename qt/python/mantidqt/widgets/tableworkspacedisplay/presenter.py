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

from functools import partial

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QTableWidgetItem

from mantid.simpleapi import DeleteTableRows, StatisticsOfTableWorkspace
from mantidqt.widgets.common.table_copying import copy_cells, show_mouse_toast, show_no_selection_to_copy_toast
from mantidqt.widgets.tableworkspacedisplay.plot_type import PlotType
from .model import TableWorkspaceDisplayModel
from .view import TableWorkspaceDisplayView


class TableItem(QTableWidgetItem):
    def __lt__(self, other):
        try:
            # if the data can be parsed as numbers then compare properly, otherwise default to the Qt implementation
            return float(self.data(Qt.DisplayRole)) < float(other.data(Qt.DisplayRole))
        except:
            return super(TableItem, self).__lt__(other)


class TableWorkspaceDisplay(object):
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    TOO_MANY_SELECTED_FOR_X = "Too many columns are selected to set as X. Please select only 1."
    TOO_MANY_SELECTED_TO_SORT = "Too many columns are selected to sort by. Please select only 1."
    TOO_MANY_SELECTED_FOR_PLOT = "Too many columns are selected to plot. Please select only 1."
    NUM_SELECTED_FOR_CONFIRMATION = 10
    NO_COLUMN_MARKED_AS_X = "No column marked as X."

    def __init__(self, ws, plot=None, parent=None, model=None, view=None, name=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else TableWorkspaceDisplayModel(ws)
        self.name = self.model.get_name() if name is None else name
        self.view = view if view else TableWorkspaceDisplayView(self, parent, self.name)
        self.parent = parent
        self.plot = plot
        self.view.set_context_menu_actions(self.view)
        self.update_column_headers()
        # self.view.setHorizontalHeaderLabels(["{}[Y]".format(x) for x in column_headers])
        self.load_data(self.view)

        self.column_marked_as_x = None

    def update_column_headers(self, extra_labels=None):
        """
        :param extra_labels: Extra labels to be appended to the column headers.
                             Expected format: [(id, label), (2, "X"),...]
        :type extra_labels: List[Tuple[int, str]]
        :return:
        """
        column_headers = self.model.get_column_headers()
        num_headers = len(column_headers)
        self.view.setColumnCount(num_headers)

        if extra_labels:
            for index, label in extra_labels:
                column_headers[index] += label

        self.view.setHorizontalHeaderLabels(column_headers)

    def load_data(self, table):
        num_rows = self.model.get_number_of_rows()
        table.setRowCount(num_rows)

        num_cols = self.model.get_number_of_columns()
        for col in range(num_cols):
            column_data = self.model.get_column(col)
            for row in range(num_rows):
                item = TableItem(str(column_data[row]))
                table.setItem(row, col, item)

    def action_copy_cells(self):
        copy_cells(self.view)

    def action_copy_bin_values(self):
        copy_cells(self.view)

    def action_copy_spectrum_values(self):
        copy_cells(self.view)

    def action_keypress_copy(self):
        copy_cells(self.view)

    def action_delete_row(self):
        selection_model = self.view.selectionModel()
        if not selection_model.hasSelection():
            show_no_selection_to_copy_toast()
            return

        selected_rows = selection_model.selectedRows()
        selected_rows_list = [index.row() for index in selected_rows]
        selected_rows_str = ",".join([str(row) for row in selected_rows_list])

        DeleteTableRows(self.model.ws, selected_rows_str)
        # Reverse the list so that we delete in order from bottom -> top
        # this prevents the row index from shifting up when deleting rows above
        for row in reversed(selected_rows_list):
            self.view.removeRow(row)

    def _get_selected_columns(self, max_selected=None, message_if_over_max=None):
        selection_model = self.view.selectionModel()
        if not selection_model.hasSelection():
            show_no_selection_to_copy_toast()
            raise ValueError("No selection")

        selected_columns = selection_model.selectedColumns()
        num_selected_columns = len(selected_columns)

        if max_selected and message_if_over_max and num_selected_columns > max_selected:
            # if over the maximum allowed selection
            show_mouse_toast(message_if_over_max)
            raise ValueError("Too many selected")
        elif num_selected_columns == 0:
            # if no columns are selected
            show_no_selection_to_copy_toast()
            raise ValueError("No selection")
        else:
            return [index.column() for index in selected_columns]

    def action_statistics_on_columns(self):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return
        stats = StatisticsOfTableWorkspace(self.model.ws, selected_columns)
        TableWorkspaceDisplay(stats, parent=self.parent, name="Column Statistics of {}".format(self.name))

    def action_hide_selected(self):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return
        for column_index in selected_columns:
            self.view.hideColumn(column_index)

    def action_show_all_columns(self):
        for column_index in range(self.view.columnCount()):
            self.view.showColumn(column_index)

    def action_set_as_x(self):
        try:
            selected_columns = self._get_selected_columns(1, self.TOO_MANY_SELECTED_FOR_X)
        except ValueError:
            return

        self.column_marked_as_x = selected_columns[0]

        self.update_column_headers([(self.column_marked_as_x, "[X]")])

    def action_sort_ascending(self, order):
        try:
            selected_columns = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        selected_column = selected_columns[0]
        self.view.sortByColumn(selected_column, order)

    def action_plot(self, type):
        if self.column_marked_as_x is None:
            show_mouse_toast(self.NO_COLUMN_MARKED_AS_X)
            return

        try:
            selected_columns = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        x = self.model.get_column(self.column_marked_as_x)
        fig, ax = self.plot.subplots(subplot_kw={'projection': 'mantid'})
        ax.set_xlabel(self.model.get_column_header(self.column_marked_as_x))

        if type == PlotType.LINEAR:
            plot_func = ax.plot
        elif type == PlotType.SCATTER:
            plot_func = ax.scatter
        elif type == PlotType.LINE_AND_SYMBOL:
            plot_func = partial(ax.plot, marker='o')
        else:
            raise ValueError("Plot Type: {} not currently supported!".format(type))

        for column in selected_columns:
            if column == self.column_marked_as_x:
                # TODO log that `column` has been skipped as it's the same as X
                continue
            y = self.model.get_column(column)
            column_label = self.model.get_column_header(column)
            plot_func(x, y, label='Column {}'.format(column_label))
            ax.set_ylabel(column_label)
        ax.legend()
        fig.show()

    # def _do_action_plot(self, table, axis, get_index, plot_errors=False):
    #     if self.plot is None:
    #         raise ValueError("Trying to do a plot, but no plotting class dependency was injected in the constructor")
    #     selection_model = table.selectionModel()
    #     if not selection_model.hasSelection():
    #         self.show_no_selection_to_copy_toast()
    #         return
    #
    #     if axis == MantidAxType.SPECTRUM:
    #         selected = selection_model.selectedRows()  # type: list
    #     else:
    #         selected = selection_model.selectedColumns()  # type: list
    #
    #     if len(selected) > self.NUM_SELECTED_FOR_CONFIRMATION and not self.view.ask_confirmation(
    #             self.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(len(selected))):
    #         return
    #
    #     plot_kwargs = {"capsize": 3} if plot_errors else {}
    #     plot_kwargs["axis"] = axis
    #
    #     ws_list = [self.model._ws]
    #     self.plot(ws_list, wksp_indices=[get_index(index) for index in selected], errors=plot_errors,
    #               plot_kwargs=plot_kwargs)
    #
    # def action_plot_spectrum(self, table):
    #     self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row())
    #
    # def action_plot_spectrum_with_errors(self, table):
    #     self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row(), plot_errors=True)
    #
    # def action_plot_bin(self, table):
    #     self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column())
    #
    # def action_plot_bin_with_errors(self, table):
    #     self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column(), plot_errors=True)

    # def _get_ws_read_from_type(self, type):
    #     if type == TableWorkspaceTableViewModelType.y:
    #         return self.model._ws.readY
    #     elif type == TableWorkspaceTableViewModelType.x:
    #         return self.model._ws.readX
    #     elif type == TableWorkspaceTableViewModelType.e:
    #         return self.model._ws.readE
    #     else:
    #         raise ValueError("Unknown TableViewModel type {}".format(type))
