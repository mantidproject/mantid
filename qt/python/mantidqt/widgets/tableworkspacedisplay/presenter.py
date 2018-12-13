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

from mantid.simpleapi import DeleteTableRows, StatisticsOfTableWorkspace
from mantidqt.widgets.common.table_copying import copy_cells, show_mouse_toast, show_no_selection_to_copy_toast
from mantidqt.widgets.tableworkspacedisplay.error_column import ErrorColumn
from mantidqt.widgets.tableworkspacedisplay.plot_type import PlotType
from mantidqt.widgets.tableworkspacedisplay.workbench_table_widget_item import WorkbenchTableWidgetItem
from .model import TableWorkspaceDisplayModel
from .view import TableWorkspaceDisplayView


class TableWorkspaceDisplay(object):
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    TOO_MANY_SELECTED_FOR_X = "Too many columns are selected to use as X. Please select only 1."
    TOO_MANY_SELECTED_TO_SORT = "Too many columns are selected to sort by. Please select only 1."
    TOO_MANY_SELECTED_FOR_PLOT = "Too many columns are selected to plot. Please select only 1."
    NUM_SELECTED_FOR_CONFIRMATION = 10
    NO_COLUMN_MARKED_AS_X = "No columns marked as X."

    def __init__(self, ws, plot=None, parent=None, model=None, view=None, name=None):
        """
        Creates a display for the provided workspace.

        :param ws: Workspace to be displayed
        :param parent: Parent of the widget
        :param plot: Plotting function that will be used to plot workspaces. This requires MatPlotLib directly.
                     Passed in as parameter to allow mocking
        :param model: Model to be used by the widget. Passed in as parameter to allow mocking
        :param view: View to be used by the widget. Passed in as parameter to allow mocking
        :param name: Custom name for the window
        """
        # Create model and view, or accept mocked versions
        self.model = model if model else TableWorkspaceDisplayModel(ws)
        self.name = self.model.get_name() if name is None else name
        self.view = view if view else TableWorkspaceDisplayView(self, parent, self.name)
        self.parent = parent
        self.plot = plot
        self.view.set_context_menu_actions(self.view)

        self.update_column_headers()
        self.load_data(self.view)

        # connect to cellChanged signal after the data has been loaded
        # all consecutive triggers will be from user actions
        self.view.itemChanged.connect(self.handleItemChanged)

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        return TableWorkspaceDisplayModel.supports(ws)

    def handleItemChanged(self, item):
        """
        :type item: WorkbenchTableWidgetItem
        :param item:
        :return:
        """
        try:
            self.model.set_cell_data(item.row(), item.column(), item.data(Qt.DisplayRole))
            item.update()
        except ValueError:
            show_mouse_toast("Error: Trying to set non-numeric data into a numeric column.")
        except Exception as x:
            show_mouse_toast("Unknown error occurred: {}".format(x))
        finally:
            item.reset()

    def update_column_headers(self):
        """
        :param extra_labels: Extra labels to be appended to the column headers.
                             Expected format: [(id, label), (2, "X"),...]
        :type extra_labels: List[Tuple[int, str]]
        :return:
        """
        # deep copy the original headers so that they are not changed by the appending of the label
        column_headers = self.model.original_column_headers()
        num_headers = len(column_headers)
        self.view.setColumnCount(num_headers)

        extra_labels = self.model.build_current_labels()
        if len(extra_labels) > 0:
            for index, label in extra_labels:
                column_headers[index] += str(label)

        self.view.setHorizontalHeaderLabels(column_headers)

    def load_data(self, table):
        num_rows = self.model.get_number_of_rows()
        table.setRowCount(num_rows)

        num_cols = self.model.get_number_of_columns()

        # the table should be editable if the ws is not PeaksWS
        editable = not self.model.is_peaks_workspace()

        for col in range(num_cols):
            column_data = self.model.get_column(col)
            for row in range(num_rows):
                item = WorkbenchTableWidgetItem(column_data[row], editable=editable)
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
        self._action_set_as(self.model.marked_columns.add_x)

    def action_set_as_y(self):
        self._action_set_as(self.model.marked_columns.add_y)

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
        try:
            err_column = ErrorColumn(selected_column, error_for_column, label_index)
        except ValueError as e:
            show_mouse_toast(e.message)
            return

        self.model.marked_columns.add_y_err(err_column)
        self.update_column_headers()

    def action_set_as_none(self):
        self._action_set_as(self.model.marked_columns.remove)

    def action_sort_ascending(self, order):
        try:
            selected_columns = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        selected_column = selected_columns[0]
        self.view.sortByColumn(selected_column, order)

    def action_plot(self, plot_type):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        # TODO check if the selected column for plotting is YERR and NOPE out if so
        # better check: if the selected columns ARE NOT Y then nope out
        # currently it silently ignores it -> maybe we want to do that so people can spam PLOT EVERYTHING
        # if -> a column in the selection is X or YErr ABORT MISSION with a toast :cheer:
        # it should be possible to reuse the check below as it is similar
        # (is it really tho -> only part of it is similar)
        x_cols = list(set(selected_columns).intersection(self.model.marked_columns.as_x))
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
            if len(self.model.marked_columns.as_x) == 0:
                # If no columns are marked as X show user message and exit
                show_mouse_toast(self.NO_COLUMN_MARKED_AS_X)
                return
            selected_x = self.model.marked_columns.as_x[0]

        try:
            # Remove the X column from the selected columns, this is
            # in case a column is being used as both X and Y
            selected_columns.remove(selected_x)
        except ValueError:
            pass

        if len(selected_columns) == 0:
            show_mouse_toast("Cannot plot column against itself.")
            return

        self._do_plot(selected_columns, selected_x, plot_type)

    def _do_plot(self, selected_columns, selected_x, plot_type):
        if plot_type == PlotType.LINEAR_WITH_ERR:
            yerr = self.model.marked_columns.find_yerr(selected_columns)
            if len(yerr) != len(selected_columns):
                show_mouse_toast("There is no associated YErr for each selected Y column.")
                return
        x = self.model.get_column(selected_x)

        fig, ax = self.plot.subplots(subplot_kw={'projection': 'mantid'})
        fig.canvas.set_window_title(self.model.get_name())
        ax.set_xlabel(self.model.get_column_header(selected_x))

        plot_func = self._get_plot_function_from_type(ax, plot_type)
        kwargs = {}
        for column in selected_columns:
            if plot_type == PlotType.LINEAR_WITH_ERR:
                yerr_column = yerr[column]
                yerr_column_data = self.model.get_column(yerr_column)
                kwargs["yerr"] = yerr_column_data

            y = self.model.get_column(column)
            column_label = self.model.get_column_header(column)
            try:
                plot_func(x, y, label='Column {}'.format(column_label), **kwargs)
            except ValueError as e:
                #     TODO log error?
                self.view.show_warning(
                    "One or more of the columns being plotted contain invalid data for MatPlotLib."
                    "\n\nError message:\n{}".format(e), "Invalid data - Mantid Workbench")
                return

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
        return self.model.marked_columns.as_y[:]
