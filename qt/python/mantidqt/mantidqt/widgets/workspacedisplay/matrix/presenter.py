# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtCore import Qt

from mantid.api import ITableWorkspace
from mantid.plots.utility import MantidAxType
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from mantidqt.widgets.workspacedisplay.data_copier import DataCopier
from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModelType
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from mantidqt.widgets.workspacedisplay.status_bar_view import StatusBarView
from .model import MatrixWorkspaceDisplayModel
from .view import MatrixWorkspaceDisplayView

# import of CreateEmptyTableWorkspace is inside method where it is used to avoid
# starting the mantid framework in unit tests when this module is imported


class MatrixWorkspaceDisplay(ObservingPresenter, DataCopier):
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    NUM_SELECTED_FOR_CONFIRMATION = 10

    def __init__(
        self,
        ws,
        plot=None,
        parent=None,
        window_flags=Qt.Window,
        model=None,
        view=None,
        ads_observer=None,
        container=None,
        window_width=600,
        window_height=400,
    ):
        """
        Creates a display for the provided workspace.

        :param ws: Workspace to be displayed
        :param plot: Plotting function that will be used to plot workspaces. Passed in as parameter to allow mocking
        :param parent: Parent of the widget
        :param model: Model to be used by the widget. Passed in as parameter to allow mocking
        :param view: View to be used by the widget. Passed in as parameter to allow mocking
        :param ads_observer: ADS observer to be used by the presenter. If not provided the default
                             one is used. Mainly intended for testing.
        """
        self.hasDx = any([ws.hasDx(i) for i in range(ws.getNumberHistograms())])

        # Create model and view, or accept mocked versions
        self.model = model or MatrixWorkspaceDisplayModel(ws)
        self.view = view or MatrixWorkspaceDisplayView(self, parent, window_flags)
        self.container = container or StatusBarView(
            parent,
            self.view,
            self.model.get_name(),
            window_width=window_width,
            window_height=window_height,
            presenter=self,
            window_flags=window_flags,
        )

        super(MatrixWorkspaceDisplay, self).__init__(self.container.status_bar)
        self.plot = plot

        self.ads_observer = ads_observer or WorkspaceDisplayADSObserver(self)

        self.setup_tables()

        self.view.set_context_menu_actions(self.view.table_y)
        self.view.set_context_menu_actions(self.view.table_x)
        self.view.set_context_menu_actions(self.view.table_e)
        if self.hasDx:
            self.view.set_context_menu_actions(self.view.table_dx)

        # connect to replace_signal signal to handle replacement of the workspace
        self.container.replace_signal.connect(self.action_replace_workspace)
        self.container.rename_signal.connect(self.action_rename_workspace)

    def show_view(self):
        self.container.show()

    def action_replace_workspace(self, workspace_name, workspace):
        if self.model.workspace_equals(workspace_name):
            self.model = MatrixWorkspaceDisplayModel(workspace)
            self.setup_tables()

    def action_rename_workspace(self, workspace_name):
        self.model.set_name(workspace_name)

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        return MatrixWorkspaceDisplayModel.supports(ws)

    def setup_tables(self):
        # unpacks the list of models returned from getItemModel
        self.view.set_model(*self.model.get_item_model())

    def action_copy_spectrum_values(self, table):
        ws_read = self._get_ws_read_from_type(table.model().type)
        self.copy_spectrum_values(table, ws_read)

    def action_copy_bin_values(self, table):
        ws_read = self._get_ws_read_from_type(table.model().type)
        num_rows = self.model._ws.getNumberHistograms()
        self.copy_bin_values(table, ws_read, num_rows)

    def action_copy_spectrum_to_table(self, table):
        selected_rows = [i.row() for i in table.selectionModel().selectedRows()]
        if not selected_rows:
            self.notify_no_selection_to_copy()
            return
        ws = table.model().ws
        row_sizes = [ws.getNumberBins(row) for row in selected_rows]
        table_ws = _create_empty_table_workspace(name=ws.name() + "_spectra", num_rows=max(row_sizes))
        num_col = 4 if self.hasDx else 3
        for i, (row, row_size) in enumerate(zip(selected_rows, row_sizes)):
            table_ws.addColumn("double", "XS" + str(row))
            table_ws.addColumn("double", "YS" + str(row))
            table_ws.addColumn("double", "ES" + str(row))

            col_x = num_col * i
            col_y = num_col * i + 1
            col_e = num_col * i + 2

            data_y = ws.readY(row)
            data_x = ws.readX(row)
            data_e = ws.readE(row)

            for j in range(row_size):
                table_ws.setCell(j, col_x, data_x[j])
                table_ws.setCell(j, col_y, data_y[j])
                table_ws.setCell(j, col_e, data_e[j])

            # if there is DX data, add a column for it
            if self.hasDx:
                table_ws.addColumn("double", "DXS" + str(row))
                col_dx = num_col * i + 3
                data_dx = ws.readDx(row)
                for j in range(row_size):
                    table_ws.setCell(j, col_dx, data_dx[j])

    def action_copy_bin_to_table(self, table):
        selected_cols = [i.column() for i in table.selectionModel().selectedColumns()]
        if not selected_cols:
            self.notify_no_selection_to_copy()
            return
        ws = table.model().ws
        num_rows = ws.getNumberHistograms()
        table_ws = _create_empty_table_workspace(name=ws.name() + "_bins", num_rows=num_rows)
        table_ws.addColumn("double", "X")
        num_cols = 3 if self.hasDx else 2
        for i, col in enumerate(selected_cols):
            table_ws.addColumn("double", "YB" + str(col))
            table_ws.addColumn("double", "YE" + str(col))

            col_y = num_cols * i + 1
            col_e = num_cols * i + 2

            for j in range(num_rows):
                data_y = ws.readY(j)
                data_e = ws.readE(j)

                if i == 0:
                    if ws.axes() > 1:
                        table_ws.setCell(j, 0, ws.getAxis(1).getValue(j))
                    else:
                        table_ws.setCell(j, 0, j)
                _set_cell_if_exists(table_ws, j, col_e, data_e, col)
                _set_cell_if_exists(table_ws, j, col_y, data_y, col)

            if self.hasDx:
                table_ws.addColumn("double", "XE" + str(col))
                col_dx = num_cols * i + 3
                for j in range(num_rows):
                    data_dx = ws.readDx(j)
                    _set_cell_if_exists(table_ws, j, col_dx, data_dx, col)

    def action_copy_cells(self, table):
        self.copy_cells(table)

    def _do_action_plot(self, table, axis, get_index, plot_errors=False, overplot=False):
        if self.plot is None:
            raise ValueError("Trying to do a plot, but no plotting class dependency was injected in the constructor")
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            self.notify_no_selection_to_copy()
            return

        if axis == MantidAxType.SPECTRUM:
            selected = selection_model.selectedRows()  # type: list
        else:
            selected = selection_model.selectedColumns()  # type: list

        if len(selected) > self.NUM_SELECTED_FOR_CONFIRMATION and not self.view.ask_confirmation(
            self.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(len(selected))
        ):
            return

        plot_kwargs = {"capsize": 3} if plot_errors else {}
        plot_kwargs["axis"] = axis

        ws_list = [self.model._ws]
        self.plot(
            ws_list, wksp_indices=[get_index(index) for index in selected], errors=plot_errors, overplot=overplot, plot_kwargs=plot_kwargs
        )

    def action_plot_spectrum(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row())

    def action_plot_spectrum_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row(), plot_errors=True)

    def action_overplot_spectrum(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row(), plot_errors=False, overplot=True)

    def action_overplot_spectrum_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row(), plot_errors=True, overplot=True)

    def action_plot_bin(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column())

    def action_plot_bin_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column(), plot_errors=True)

    def action_overplot_bin(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column(), plot_errors=False, overplot=True)

    def action_overplot_bin_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column(), plot_errors=True, overplot=True)

    def action_keypress_copy(self, table):
        selectionModel = table.selectionModel()
        if not selectionModel.hasSelection():
            self.notify_no_selection_to_copy()
            return

        if len(selectionModel.selectedRows()) > 0:
            self.action_copy_spectrum_values(table)
        elif len(selectionModel.selectedColumns()) > 0:
            self.action_copy_bin_values(table)
        else:
            self.action_copy_cells(table)

    def _get_ws_read_from_type(self, type):
        if type == MatrixWorkspaceTableViewModelType.y:
            return self.model._ws.readY
        elif type == MatrixWorkspaceTableViewModelType.x:
            return self.model._ws.readX
        elif type == MatrixWorkspaceTableViewModelType.e:
            return self.model._ws.readE
        elif type == MatrixWorkspaceTableViewModelType.dx:
            return self.model._ws.readDx
        else:
            raise ValueError("Unknown TableViewModel type {}".format(type))


# utility functions
def _create_empty_table_workspace(name: str, num_rows: int) -> ITableWorkspace:
    """Create and empty table with the given number of rows

    :param name: The name of the workspace in the ADS
    :param num_rows: Number of rows for the new table
    :return: A new tableworkspace
    """
    # keep import here to avoid framework initialization in tests
    from mantid.simpleapi import CreateEmptyTableWorkspace

    table = CreateEmptyTableWorkspace(OutputWorkspace=name)
    table.setRowCount(num_rows)
    return table


def _set_cell_if_exists(
    target_table_ws,
    target_row_index,
    target_col_index,
    data_col,
    data_col_index,
):
    # ragged workspaces could have data rows shorter than the number of columns
    if data_col_index < len(data_col):
        target_table_ws.setCell(target_row_index, target_col_index, data_col[data_col_index])
