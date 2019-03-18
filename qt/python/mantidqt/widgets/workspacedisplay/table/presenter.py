# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from __future__ import absolute_import, division, print_function

from functools import partial

from qtpy.QtCore import Qt

from mantid.kernel import logger
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from mantidqt.widgets.workspacedisplay.data_copier import DataCopier
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.plot_type import PlotType
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView
from mantidqt.widgets.workspacedisplay.table.workbench_table_widget_item import WorkbenchTableWidgetItem


class TableWorkspaceDisplay(ObservingPresenter, DataCopier):
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    TOO_MANY_SELECTED_FOR_X = "Too many columns are selected to use as X. Please select only 1."
    TOO_MANY_SELECTED_TO_SORT = "Too many columns are selected to sort by. Please select only 1."
    TOO_MANY_SELECTED_FOR_PLOT = "Too many columns are selected to plot. Please select only 1."
    NUM_SELECTED_FOR_CONFIRMATION = 10
    NO_COLUMN_MARKED_AS_X = "No columns marked as X."
    ITEM_CHANGED_INVALID_DATA_MESSAGE = "Error: Trying to set invalid data for the column."
    ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE = "Unknown error occurred: {}"
    TOO_MANY_TO_SET_AS_Y_ERR_MESSAGE = "Too many selected to set as Y Error"
    CANNOT_PLOT_AGAINST_SELF_MESSAGE = "Cannot plot column against itself."
    NO_ASSOCIATED_YERR_FOR_EACH_Y_MESSAGE = "Column '{}' does not have an associated Y error column." \
                                            "\n\nPlease set it by doing: Right click on column ->" \
                                            " Set error for Y -> The label shown on the Y column"
    PLOT_FUNCTION_ERROR_MESSAGE = "One or more of the columns being plotted contain invalid data for Matplotlib.\n\nError message:\n{}"
    INVALID_DATA_WINDOW_TITLE = "Invalid data - Mantid Workbench"
    COLUMN_DISPLAY_LABEL = 'Column {}'

    def __init__(self, ws, plot=None, parent=None, model=None, view=None, name=None, ads_observer=None, container=None,
                 window_width=600, window_height=400):
        """
        Creates a display for the provided workspace.

        :param ws: Workspace to be displayed
        :param parent: Parent of the widget
        :param plot: Plotting function that will be used to plot workspaces. This requires Matplotlib directly.
                     Passed in as parameter to allow mocking
        :param model: Model to be used by the widget. Passed in as parameter to allow mocking
        :param view: View to be used by the widget. Passed in as parameter to allow mocking
        :param name: Custom name for the window
        :param ads_observer: ADS observer to be used by the presenter. If not provided the default
                             one is used. Mainly intended for testing.
        """
        self.model = model if model else TableWorkspaceDisplayModel(ws)
        self.name = name if name else self.model.get_name()
        self.view = view if view else TableWorkspaceDisplayView(self, parent)
        self.container = container if container else StatusBarView(parent, self.view, self.name,
                                                                   window_width=window_width,
                                                                   window_height=window_height,
                                                                   presenter=self)

        DataCopier.__init__(self, self.container.status_bar)

        self.parent = parent
        self.plot = plot
        self.view.set_context_menu_actions(self.view)

        self.ads_observer = ads_observer if ads_observer else WorkspaceDisplayADSObserver(self)

        self.update_column_headers()
        self.load_data(self.view)

        # connect to cellChanged signal after the data has been loaded
        self.view.itemChanged.connect(self.handleItemChanged)

    def show_view(self):
        self.container.show()

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        return TableWorkspaceDisplayModel.supports(ws)

    def replace_workspace(self, workspace_name, workspace):
        if self.model.workspace_equals(workspace_name):
            # stops triggering itemChanged signal while the data is being reloaded
            self.view.blockSignals(True)

            self.model = TableWorkspaceDisplayModel(workspace)
            self.load_data(self.view)
            self.view.blockSignals(False)

            self.view.emit_repaint()

    def handleItemChanged(self, item):
        """
        :type item: WorkbenchTableWidgetItem
        """
        try:
            self.model.set_cell_data(item.row(), item.column(), item.data(Qt.DisplayRole), item.is_v3d)
            item.update()
        except ValueError:
            self.view.show_warning(self.ITEM_CHANGED_INVALID_DATA_MESSAGE)
        except Exception as x:
            self.view.show_warning(self.ITEM_CHANGED_UNKNOWN_ERROR_MESSAGE.format(x))
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
        self.copy_cells(self.view)

    def action_copy_bin_values(self):
        self.copy_cells(self.view)

    def action_copy_spectrum_values(self):
        self.copy_cells(self.view)

    def action_keypress_copy(self):
        self.copy_cells(self.view)

    def action_delete_row(self):
        selection_model = self.view.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            return

        selected_rows = selection_model.selectedRows()
        selected_rows_list = [index.row() for index in selected_rows]
        selected_rows_str = ",".join([str(row) for row in selected_rows_list])

        self.model.delete_rows(selected_rows_str)

    def _get_selected_columns(self, max_selected=None, message_if_over_max=None):
        selection_model = self.view.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            raise ValueError("No selection")

        selected_columns = selection_model.selectedColumns()
        num_selected_columns = len(selected_columns)

        if max_selected and message_if_over_max and num_selected_columns > max_selected:
            # if over the maximum allowed selection
            self.view.show_warning(message_if_over_max)
            raise ValueError("Too many selected")
        elif num_selected_columns == 0:
            # if no columns are selected
            self.notify_no_selection_to_copy()
            raise ValueError("No selection")
        else:
            col_selected = [index.column() for index in selected_columns]
            if max_selected == 1:
                return col_selected[0]
            return col_selected

    def action_statistics_on_columns(self):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        stats = self.model.get_statistics(selected_columns)
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

    def action_set_as_y_err(self, related_y_column, label_index):
        """

        :param related_y_column: The real index of the column for which the error is being marked
        :param label_index: The index present in the label of the column for which the error is being marked
                            This will be the number in <ColumnName>[Y10] -> the 10
        """
        try:
            selected_column = self._get_selected_columns(1, self.TOO_MANY_TO_SET_AS_Y_ERR_MESSAGE)
        except ValueError:
            return

        try:
            err_column = ErrorColumn(selected_column, related_y_column, label_index)
        except ValueError as e:
            self.view.show_warning(str(e))
            return

        self.model.marked_columns.add_y_err(err_column)
        self.update_column_headers()

    def action_set_as_none(self):
        self._action_set_as(self.model.marked_columns.remove)

    def action_sort(self, sort_ascending):
        """
        :type sort_ascending: bool
        :param sort_ascending: Whether to sort ascending
        """
        try:
            selected_column = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        self.model.sort(selected_column, sort_ascending)

    def action_plot(self, plot_type):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        x_cols = list(set(selected_columns).intersection(self.model.marked_columns.as_x))
        num_x_cols = len(x_cols)
        # if there is more than 1 column marked as X in the selection
        # -> show toast to the user and do nothing
        if num_x_cols > 1:
            self.view.show_warning(self.TOO_MANY_SELECTED_FOR_X)
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
                self.view.show_warning(self.NO_COLUMN_MARKED_AS_X)
                return
            selected_x = self.model.marked_columns.as_x[0]

        try:
            # Remove the X column from the selected columns, this is
            # in case a column is being used as both X and Y
            selected_columns.remove(selected_x)
        except ValueError:
            pass

        if len(selected_columns) == 0:
            self.view.show_warning(self.CANNOT_PLOT_AGAINST_SELF_MESSAGE)
            return

        self._do_plot(selected_columns, selected_x, plot_type)

    def _is_error_plot(self, plot_type):
        return plot_type == PlotType.LINEAR_WITH_ERR or plot_type == PlotType.SCATTER_WITH_ERR

    def _do_plot(self, selected_columns, selected_x, plot_type):
        if self._is_error_plot(plot_type):
            yerr = self.model.marked_columns.find_yerr(selected_columns)
            # remove the Y error columns if they are in the selection for plotting
            # this prevents them from being treated as Y columns
            for err_col in yerr.values():
                try:
                    selected_columns.remove(err_col)
                except ValueError:
                    # the column is not contained within the selected one
                    pass
            if len(yerr) != len(selected_columns):
                column_headers = self.model.original_column_headers()
                self.view.show_warning(self.NO_ASSOCIATED_YERR_FOR_EACH_Y_MESSAGE.format(
                    ",".join([column_headers[col] for col in selected_columns])))
                return
        x = self.model.get_column(selected_x)

        fig, ax = self.plot.subplots()
        fig.canvas.set_window_title(self.model.get_name())
        ax.set_xlabel(self.model.get_column_header(selected_x))

        plot_func = self._get_plot_function_from_type(ax, plot_type)
        kwargs = {}
        for column in selected_columns:
            # if the errors are being plotted, retrieve the data for the column
            if self._is_error_plot(plot_type):
                yerr_column = yerr[column]
                yerr_column_data = self.model.get_column(yerr_column)
                kwargs["yerr"] = yerr_column_data

            y = self.model.get_column(column)
            column_label = self.model.get_column_header(column)
            try:
                plot_func(x, y, label=self.COLUMN_DISPLAY_LABEL.format(column_label), **kwargs)
            except ValueError as e:
                error_message = self.PLOT_FUNCTION_ERROR_MESSAGE.format(e)
                logger.error(error_message)
                self.view.show_warning(error_message, self.INVALID_DATA_WINDOW_TITLE)
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
        elif type == PlotType.SCATTER_WITH_ERR:
            plot_func = partial(ax.errorbar, fmt='o')
        else:
            raise ValueError("Plot Type: {} not currently supported!".format(type))
        return plot_func

    def get_columns_marked_as_y(self):
        return self.model.marked_columns.as_y[:]
