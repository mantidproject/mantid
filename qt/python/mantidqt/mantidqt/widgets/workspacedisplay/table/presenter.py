# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of mantidqt package.
from functools import partial
from qtpy.QtCore import Qt

from mantid.api import mtd
from mantid.kernel import logger
from mantid.plots.utility import legend_set_draggable
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from mantidqt.widgets.workspacedisplay.data_copier import DataCopier
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.group_model import GroupTableWorkspaceDisplayModel
from mantidqt.widgets.workspacedisplay.table.plot_type import PlotType
from mantidqt.widgets.workspacedisplay.table.presenter_batch import TableWorkspaceDataPresenterBatch
from mantidqt.widgets.workspacedisplay.table.presenter_standard import TableWorkspaceDataPresenterStandard
from mantidqt.widgets.workspacedisplay.table.table_model import TableModel
from mantidqt.widgets.workspacedisplay.table.view import TableWorkspaceDisplayView
from mantidqt.widgets.workspacedisplay.table.group_table_model import GroupTableModel
from mantidqt.widgets.workspacedisplay.table.presenter_group import TableWorkspaceDataPresenterGroup
from mantidqt.widgets.workspacedisplay.table.group_view import GroupTableWorkspaceDisplayView


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
    NO_ASSOCIATED_YERR_FOR_EACH_Y_MESSAGE = (
        "Column '{}' does not have an associated Y error column."
        "\n\nPlease set it by doing: Right click on column ->"
        " Set error for Y -> The label shown on the Y column"
    )
    PLOT_FUNCTION_ERROR_MESSAGE = "One or more of the columns being plotted contain invalid data for Matplotlib.\n\nError message:\n{}"
    INVALID_DATA_WINDOW_TITLE = "Invalid data - Mantid Workbench"
    COLUMN_DISPLAY_LABEL = "Column {}"

    def __init__(
        self,
        ws,
        parent=None,
        window_flags=Qt.Window,
        plot=None,
        model=None,
        view=None,
        name=None,
        ads_observer=None,
        container=None,
        window_width=600,
        window_height=400,
        batch=False,
        group=False,
    ):
        """
        Creates a display for the provided workspace.

        :param ws: Workspace to be displayed
        :param parent: Parent of the widget
        :param window_flags: An optional set of window flags
        :param plot: Plotting function that will be used to plot workspaces. This requires Matplotlib directly.
                     Passed in as parameter to allow mocking
        :param model: Model to be used by the widget. Passed in as parameter to allow mocking
        :param view: View to be used by the widget. Passed in as parameter to allow mocking
        :param name: Custom name for the window
        :param ads_observer: ADS observer to be used by the presenter. If not provided the default
                             one is used. Mainly intended for testing.
        """
        view, model = self.create_table(ws, parent, window_flags, model, view, batch, group)
        self.view = view
        self.model = model
        self.name = name if name else model.get_name()
        self.container = (
            container
            if container
            else StatusBarView(
                parent,
                view,
                self.name,
                window_width=window_width,
                window_height=window_height,
                window_flags=window_flags,
                presenter=self,
            )
        )

        DataCopier.__init__(self, self.container.status_bar)

        self.parent = parent
        self.plot = plot
        self.group = group

        self.ads_observer = ads_observer if ads_observer else WorkspaceDisplayADSObserver(self, observe_group_update=self.group)

        self.presenter.refresh()

    def show_view(self):
        self.container.show()

    def create_table(self, ws, parent, window_flags, model, view, batch, group):
        if group:
            view, model = self._create_table_group(ws, parent, window_flags, view, model)
        elif batch:
            view, model = self._create_table_batch(ws, parent, window_flags, view, model)
        else:
            view, model = self._create_table_standard(ws, parent, window_flags, view, model)
        view.set_context_menu_actions(view)
        return view, model

    def _create_table_standard(self, ws, parent, window_flags, view, model):
        model = model if model is not None else TableWorkspaceDisplayModel(ws)
        view = view if view else TableWorkspaceDisplayView(presenter=self, parent=parent, window_flags=window_flags)
        self.presenter = TableWorkspaceDataPresenterStandard(model, view)
        return view, model

    def _create_table_batch(self, ws, parent, window_flags, view, model):
        model = model if model is not None else TableWorkspaceDisplayModel(ws)
        table_model = TableModel(parent=parent, data_model=model)
        view = (
            view if view else TableWorkspaceDisplayView(presenter=self, parent=parent, window_flags=window_flags, table_model=table_model)
        )
        self.presenter = TableWorkspaceDataPresenterBatch(model, view)
        return view, model

    def _create_table_group(self, ws, parent, window_flags, view, model):
        model = model if model is not None else GroupTableWorkspaceDisplayModel(ws)
        table_model = GroupTableModel(model, view)
        view = (
            view
            if view
            else GroupTableWorkspaceDisplayView(presenter=self, parent=parent, window_flags=window_flags, table_model=table_model)
        )
        self.presenter = TableWorkspaceDataPresenterGroup(model, view)
        return view, model

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        try:
            TableWorkspaceDisplayModel.supports(ws)
        except ValueError:
            try:
                GroupTableWorkspaceDisplayModel.supports(ws)
            except ValueError:
                raise ValueError("The workspace type is not supported: {0}".format(ws))

    def _update_group_model(self, group_name):
        self.presenter.view.blockSignals(True)
        ws = mtd[group_name]
        self.presenter.model = GroupTableWorkspaceDisplayModel(ws)
        self.presenter.load_data(self.presenter.view)
        self.presenter.view.blockSignals(False)

    def replace_workspace(self, workspace_name, workspace):
        model = self.presenter.model
        if not self.group and model.workspace_equals(workspace_name) and not model.block_model_replace:
            self.presenter.view.blockSignals(True)
            self.presenter.model = TableWorkspaceDisplayModel(workspace)
            self.presenter.load_data(self.presenter.view)
            self.presenter.view.blockSignals(False)

        if (
            self.group
            and not model.block_model_replace
            and (model.workspace_equals(workspace_name) or workspace_name in model.get_child_names())
        ):
            self._update_group_model(model.get_name())

    def group_update(self, workspace_name, workspace):
        model = self.presenter.model
        if (
            self.group
            and not model.block_model_replace
            and (model.workspace_equals(workspace_name) or workspace_name in model.get_child_names())
        ):
            self._update_group_model(model.get_name())

    def close(self, workspace_name):
        if self.current_workspace_equals(workspace_name):
            self.clear_observer()
            self.container.emit_close()

        if self.group and workspace_name in self.presenter.model.get_child_names():
            self.clear_observer()
            self.container.emit_close()

    def action_copy_cells(self):
        self.copy_cells(self.presenter.view)

    def action_copy_bin_values(self):
        self.copy_cells(self.presenter.view)

    def action_copy_spectrum_values(self):
        self.copy_cells(self.presenter.view)

    def action_keypress_copy(self):
        self.copy_cells(self.presenter.view)

    def action_delete_row(self):
        selection_model = self.presenter.view.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            return

        selected_rows = selection_model.selectedRows()
        if not self.group:
            selected_rows_list = [index.row() for index in selected_rows]
            selected_rows_str = ",".join([str(row) for row in selected_rows_list])
            self.presenter.model.delete_rows(selected_rows_str)
        else:
            selected_rows_list = [index.row() for index in selected_rows]
            self.presenter.delete_rows(selected_rows_list)

    def _get_selected_columns(self, max_selected=None, message_if_over_max=None):
        selection_model = self.presenter.view.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            raise ValueError("No selection")

        selected_columns = selection_model.selectedColumns()
        num_selected_columns = len(selected_columns)

        if max_selected and message_if_over_max and num_selected_columns > max_selected:
            # if over the maximum allowed selection
            self.presenter.view.show_warning(message_if_over_max)
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

        stats = self.presenter.model.get_statistics(selected_columns)
        if not self.group:
            TableWorkspaceDisplay(stats, parent=self.parent, name="Column Statistics of {}".format(self.name))

    def action_hide_selected(self):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return
        for column_index in selected_columns:
            self.presenter.view.hideColumn(column_index)

    def action_show_all_columns(self):
        for column_index in range(self.presenter.view.columnCount()):
            self.presenter.view.showColumn(column_index)

    def _action_set_as(self, add_to_list_func, type):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        for col in selected_columns:
            add_to_list_func(col)
            self.presenter.model.set_column_type(col, type)

        self.presenter.update_column_headers()

    def action_set_as_x(self):
        self._action_set_as(self.presenter.model.marked_columns.add_x, 1)

    def action_set_as_y(self):
        self._action_set_as(self.presenter.model.marked_columns.add_y, 2)

    def action_set_as_y_err(self, related_y_column):
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
            err_column = ErrorColumn(selected_column, related_y_column)
        except ValueError as e:
            self.presenter.view.show_warning(str(e))
            return

        removed_items = self.presenter.model.marked_columns.add_y_err(err_column)
        # if a column other than the one the user has just picked as a y err column has been affected,
        # reset it's type to None
        for col in removed_items:
            if col != selected_column:
                self.presenter.model.set_column_type(int(col), 0)
        self.presenter.model.set_column_type(selected_column, 5, related_y_column)
        self.presenter.update_column_headers()

    def action_set_as_none(self):
        self._action_set_as(self.presenter.model.marked_columns.remove, 0)

    def action_sort(self, sort_ascending):
        """
        :type sort_ascending: bool
        :param sort_ascending: Whether to sort ascending
        """
        try:
            selected_column = self._get_selected_columns(1, self.TOO_MANY_SELECTED_TO_SORT)
        except ValueError:
            return

        self.presenter.model.sort(selected_column, sort_ascending)

        if self.group:
            self.presenter.sort(selected_column, sort_ascending)

    def action_plot(self, plot_type):
        try:
            selected_columns = self._get_selected_columns()
        except ValueError:
            return

        x_cols = list(set(selected_columns).intersection(self.presenter.model.marked_columns.as_x))
        num_x_cols = len(x_cols)
        # if there is more than 1 column marked as X in the selection
        # -> show toast to the user and do nothing
        if num_x_cols > 1:
            self.presenter.view.show_warning(self.TOO_MANY_SELECTED_FOR_X)
            return
        elif num_x_cols == 1:
            # Only 1 X column present in the current selection model
            # -> Use it as X for the plot
            selected_x = x_cols[0]
        else:
            # No X column present in the current selection model
            # -> Use the first column marked as X (if present)
            if len(self.presenter.model.marked_columns.as_x) == 0:
                # If no columns are marked as X show user message and exit
                self.presenter.view.show_warning(self.NO_COLUMN_MARKED_AS_X)
                return
            selected_x = self.presenter.model.marked_columns.as_x[0]

        try:
            # Remove the X column from the selected columns, this is
            # in case a column is being used as both X and Y
            selected_columns.remove(selected_x)
        except ValueError:
            pass

        if len(selected_columns) == 0:
            self.presenter.view.show_warning(self.CANNOT_PLOT_AGAINST_SELF_MESSAGE)
            return

        self._do_plot(selected_columns, selected_x, plot_type)

    def _is_error_plot(self, plot_type):
        return plot_type == PlotType.LINEAR_WITH_ERR or plot_type == PlotType.SCATTER_WITH_ERR

    def _do_plot(self, selected_columns, selected_x, plot_type):
        if self._is_error_plot(plot_type):
            yerr = self.presenter.model.marked_columns.find_yerr(selected_columns)
            # remove the Y error columns if they are in the selection for plotting
            # this prevents them from being treated as Y columns
            for err_col in yerr.values():
                try:
                    selected_columns.remove(err_col)
                except ValueError:
                    # the column is not contained within the selected one
                    pass
            if len(yerr) != len(selected_columns):
                column_headers = self.presenter.model.original_column_headers()
                self.presenter.view.show_warning(
                    self.NO_ASSOCIATED_YERR_FOR_EACH_Y_MESSAGE.format(",".join([column_headers[col] for col in selected_columns]))
                )
                return
        x = self.presenter.model.get_column(selected_x)

        fig, ax = self.plot.subplots(subplot_kw={"projection": "mantid"})
        if fig.canvas.manager is not None:
            fig.canvas.manager.set_window_title(self.presenter.model.get_name())
        ax.set_xlabel(self.presenter.model.get_column_header(selected_x))
        ax.wsName = self.presenter.model.get_name()

        plot_func = self._get_plot_function_from_type(ax, plot_type)
        kwargs = {}
        for column in selected_columns:
            # if the errors are being plotted, retrieve the data for the column
            if self._is_error_plot(plot_type):
                yerr_column = yerr[column]
                yerr_column_data = self.presenter.model.get_column(yerr_column)
                kwargs["yerr"] = yerr_column_data

            y = self.presenter.model.get_column(column)
            column_label = self.presenter.model.get_column_header(column)
            try:
                plot_func(x, y, label=self.COLUMN_DISPLAY_LABEL.format(column_label), **kwargs)
            except ValueError as e:
                error_message = self.PLOT_FUNCTION_ERROR_MESSAGE.format(e)
                logger.error(error_message)
                self.presenter.view.show_warning(error_message, self.INVALID_DATA_WINDOW_TITLE)
                return

            ax.set_ylabel(column_label)
        legend_set_draggable(ax.legend(), True)
        fig.show()

    def _get_plot_function_from_type(self, ax, type):
        if type == PlotType.LINEAR:
            plot_func = ax.plot
        elif type == PlotType.SCATTER:
            plot_func = ax.scatter
        elif type == PlotType.LINE_AND_SYMBOL:
            plot_func = partial(ax.plot, marker="o")
        elif type == PlotType.LINEAR_WITH_ERR:
            plot_func = ax.errorbar
        elif type == PlotType.SCATTER_WITH_ERR:
            plot_func = partial(ax.errorbar, fmt="o")
        else:
            raise ValueError("Plot Type: {} not currently supported!".format(type))
        return plot_func

    def get_columns_marked_as_y(self):
        return self.presenter.model.marked_columns.as_y[:]
