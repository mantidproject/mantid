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


# class ColumnState(Enum):
#     NONE = 0
#     Y = 1
#     X = 2
#     Y_ERR = 3
#
#
# class TableColumn:
#     X_LABEL = "[X]"
#     Y_LABEL = "[Y]"
#     Y_ERR_LABEL = "[YErr->Y{}]"
#
#     def __init__(self, state=ColumnState.NONE, error_for_column=None):
#         self.state = state
#         # If the column is marked as an error column,
#         # this will store the index of the column for which the error is being stored
#         self.error_for_column = error_for_column
#
#     def __str__(self):
#         if self.state == ColumnState.X:
#             return self.X_LABEL
#         elif self.state == ColumnState.Y:
#             return self.Y_LABEL
#         elif self.state == ColumnState.Y_ERR:
#             return self.Y_ERR_LABEL
#         elif self.state == ColumnState.NONE:
#             return ""
#

class ErrorColumn:
    def __init__(self, source_column, error_for_column, label_index):
        self.source_column = source_column
        self.error_for_column = error_for_column
        self.label_index = label_index

    def __eq__(self, other):
        if isinstance(other, ErrorColumn):
            return self.error_for_column == other.error_for_column or self.source_column == other.source_column
        elif isinstance(other, int):
            return self.source_column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))

    def __cmp__(self, other):
        if isinstance(other, ErrorColumn):
            return self.source_column == other.source_column or self.error_for_column == other.error_for_column
        elif isinstance(other, int):
            return self.source_column == other
        else:
            raise RuntimeError("Unhandled comparison logic with type {}".format(type(other)))


class MarkedColumns:
    X_LABEL = "[X{}]"
    Y_LABEL = "[Y{}]"
    Y_ERR_LABEL = "[Y{}_YErr]"

    def __init__(self):
        self.as_x = []
        self.as_y = []
        self.as_y_err = []  # type: list[ErrorColumn]
        # self.as_x_err = []

    def _add(self, col_index, add_to, remove_from):
        assert all(
            add_to is not remove for remove in remove_from), "Can't add and remove from the same list at the same time!"
        self._remove(col_index, remove_from)

        if col_index not in add_to:
            add_to.append(col_index)

    def _remove(self, col_index, remove_from):
        for list in remove_from:
            num_contained = list.count(col_index)
            for i in range(num_contained):
                try:
                    list.remove(col_index)
                except ValueError:
                    break
        # if the column previously had a Y Err associated with it -> this will remove it from the YErr list
        self._remove_associated_yerr_columns(col_index)

    def add_x(self, col_index):
        self._add(col_index, self.as_x, [self.as_y, self.as_y_err])

    def add_y(self, col_index):
        self._add(col_index, self.as_y, [self.as_x, self.as_y_err])

    def add_y_err(self, err_column):
        # remove all labels for the column index
        len_before_remove = len(self.as_y)
        self._remove(err_column, [self.as_x, self.as_y, self.as_y_err])

        # Check if the length of the list with columns marked Y has shrunk
        # -> This means that columns have been removed, and the label_index is now _wrong_
        # and has to be decremented to match the new label index correctly
        # TODO test this edge case: mark all columns Y, remove one that is not the last one!
        # TODO test: mark 3 columns as Y, set the first one to YErr it should have label YErr->Y1
        # TODO test: mark 3 columns as Y, set the middle one to YErr it should have label YErr->Y1
        len_after_remove = len(self.as_y)
        if err_column.error_for_column > err_column.source_column and len_after_remove < len_before_remove:
            err_column.label_index -= (len_before_remove - len_after_remove)
        self.as_y_err.append(err_column)

    def remove_from_all(self, col_index):
        self._remove(col_index, [self.as_x, self.as_y, self.as_y_err])

    def _remove_associated_yerr_columns(self, col_index):
        # we can only have 1 Y Err for Y, so iterating and removing's iterator invalidation is not an
        # issue as the code will exit immediately after the removal
        for col in self.as_y_err:
            if col.error_for_column == col_index:
                self.as_y_err.remove(col)
                break

    def _make_labels(self, list, label):
        return [(col_num, label.format(index),) for index, col_num in enumerate(list)]

    def build_labels(self):
        extra_labels = []
        extra_labels.extend(self._make_labels(self.as_x, self.X_LABEL))
        extra_labels.extend(self._make_labels(self.as_y, self.Y_LABEL))
        err_labels = [(err_col.source_column, self.Y_ERR_LABEL.format(err_col.label_index),) for index, err_col in
                      enumerate(self.as_y_err)]
        extra_labels.extend(err_labels)
        return extra_labels

    def find_yerr(self, selected_columns):
        yerr_for_col = {}

        # for each selected column
        for col in selected_columns:
            # find the marked error column
            for yerr_col in self.as_y_err:
                # if found append the YErr's source column - so that the data from the columns
                # can be retrieved for plotting the errors
                if yerr_col.error_for_column == col:
                    yerr_for_col[col] = yerr_col.source_column

        return yerr_for_col


class TableWorkspaceDisplay(object):
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    TOO_MANY_SELECTED_FOR_X = "Too many columns are selected to use as X. Please select only 1."
    TOO_MANY_SELECTED_TO_SORT = "Too many columns are selected to sort by. Please select only 1."
    TOO_MANY_SELECTED_FOR_PLOT = "Too many columns are selected to plot. Please select only 1."
    NUM_SELECTED_FOR_CONFIRMATION = 10
    NO_COLUMN_MARKED_AS_X = "No columns marked as X."

    def __init__(self, ws, plot=None, parent=None, model=None, view=None, name=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else TableWorkspaceDisplayModel(ws)
        self.name = self.model.get_name() if name is None else name
        self.view = view if view else TableWorkspaceDisplayView(self, parent, self.name)
        self.parent = parent
        self.plot = plot
        self.view.set_context_menu_actions(self.view)

        self.marked_columns = MarkedColumns()
        self.original_column_headers = self.model.get_column_headers()
        # self.column_states = [TableColumn() for i in range(len(self.original_column_headers))]
        self.update_column_headers()
        self.load_data(self.view)

    def update_column_headers(self):
        """
        :param extra_labels: Extra labels to be appended to the column headers.
                             Expected format: [(id, label), (2, "X"),...]
        :type extra_labels: List[Tuple[int, str]]
        :return:
        """
        # deep copy the original headers so that they are not changed by the appending of the label
        column_headers = self.original_column_headers[:]
        num_headers = len(column_headers)
        self.view.setColumnCount(num_headers)

        extra_labels = self.marked_columns.build_labels()
        if len(extra_labels) > 0:
            for index, label in extra_labels:
                column_headers[index] += str(label)

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

    def _action_set_as(self, add_to_list_func):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        for col in selected_columns:
            add_to_list_func(col)

        self.update_column_headers()

    def action_set_as_x(self):
        self._action_set_as(self.marked_columns.add_x)

    def action_set_as_y(self):
        self._action_set_as(self.marked_columns.add_y)

    def action_set_as_y_err(self, error_for_column, label_index):
        """

        :param error_for_column: The real index of the column for which the error is being marked
        :param label_index: The index present in the label of the column for which the error is being marked
                            This will be the number in <ColumnName>[Y10] -> the 10
        """
        try:
            selected_columns = self._get_selected_columns(1, "Too many selected to set as Y Error")
        except ValueError:
            return

        selected_column = selected_columns[0]
        if selected_column == error_for_column:
            show_mouse_toast("Cannot set Y column to be its own YErr.")
            return

        err_column = ErrorColumn(selected_column, error_for_column, label_index)
        self.marked_columns.add_y_err(err_column)

        self.update_column_headers()

    def action_set_as_none(self):
        self._action_set_as(self.marked_columns.remove_from_all)

    def action_sort_ascending(self, order):
        try:
            selected_columns = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        selected_column = selected_columns[0]
        self.view.sortByColumn(selected_column, order)

    def action_plot(self, type):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        # TODO check if the selected column for plotting is YERR and NOPE out if so
        # better check: if the selected columns IS NOT Y then nope out
        # currently it silently ignores it -> maybe we want to do that so people can spam PLOT EVERYTHING
        # if -> a column in the selection is X or YErr ABORT MISSION with a toast :cheer:
        # it should be possible to reuse the check below as it is similar (is it really tho -> only part of it is similar)
        x_cols = list(set(selected_columns).intersection(self.marked_columns.as_x))
        num_x_cols = len(x_cols)
        # if there is more than 1 column marked as X in the selection
        # -> show toast to the user and do nothing
        if num_x_cols > 1:
            show_mouse_toast(self.TOO_MANY_SELECTED_FOR_X)
            return
        elif num_x_cols == 1:
            # Only 1 X column present in the current selection model
            # -> Use it as X for the plot
            selected_x = x_cols[0]
        else:
            # No X column present in the current selection model
            # -> Use the first column marked as X (if present)
            if len(self.marked_columns.as_x) == 0:
                # If no columns are marked as X show user message and exit
                show_mouse_toast(self.NO_COLUMN_MARKED_AS_X)
                return
            selected_x = self.marked_columns.as_x[0]

        try:
            # Remove the X column from the selected columns, this is
            # in case a column is being used as both X and Y
            selected_columns.remove(selected_x)
        except ValueError:
            pass

        if len(selected_columns) == 0:
            show_mouse_toast("Cannot plot column against itself.")
            return

        self._do_plot(selected_columns, selected_x, type)

    def _do_plot(self, selected_columns, selected_x, type):

        if type == PlotType.LINEAR_WITH_ERR:
            yerr = self.marked_columns.find_yerr(selected_columns)  # type: dict[(int, int)]
            if len(yerr) != len(selected_columns):
                show_mouse_toast("There is no associated YErr for each selected Y column.")
                return
        x = self.model.get_column(selected_x)
        fig, ax = self.plot.subplots(subplot_kw={'projection': 'mantid'})
        ax.set_xlabel(self.model.get_column_header(selected_x))
        plot_func = self._get_plot_function_from_type(ax, type)
        for column in selected_columns:
            y = self.model.get_column(column)
            column_label = self.model.get_column_header(column)
            if type == PlotType.LINEAR_WITH_ERR:
                yerr_column = yerr[column]
                yerr_column_data = self.model.get_column(yerr_column)
                plot_func(x, y, label='Column {}'.format(column_label), yerr=yerr_column_data)
            else:
                plot_func(x, y, label='Column {}'.format(column_label))
            ax.set_ylabel(column_label)
        ax.legend()
        fig.show()

    def _get_plot_function_from_type(self, ax, type):
        if type == PlotType.LINEAR:
            plot_func = ax.plot
        elif type == PlotType.SCATTER:
            plot_func = ax.scatter
        elif type == PlotType.LINE_AND_SYMBOL:
            plot_func = partial(ax.plot, marker='o')
        elif type == PlotType.LINEAR_WITH_ERR:
            plot_func = ax.errorbar
        else:
            raise ValueError("Plot Type: {} not currently supported!".format(type))
        return plot_func

    def get_columns_marked_as_y(self):
        return self.marked_columns.as_y
