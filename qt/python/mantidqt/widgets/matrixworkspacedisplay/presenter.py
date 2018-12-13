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

from mantid.plots.utility import MantidAxType
from mantidqt.widgets.common.table_copying import copy_bin_values, copy_cells, copy_spectrum_values, \
    show_no_selection_to_copy_toast
from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModelType
from .model import MatrixWorkspaceDisplayModel
from .view import MatrixWorkspaceDisplayView


class MatrixWorkspaceDisplay(object):
    NO_SELECTION_MESSAGE = "No selection"
    COPY_SUCCESSFUL_MESSAGE = "Copy Successful"
    A_LOT_OF_THINGS_TO_PLOT_MESSAGE = "You selected {} spectra to plot. Are you sure you want to plot that many?"
    NUM_SELECTED_FOR_CONFIRMATION = 10

    def __init__(self, ws, plot=None, parent=None, model=None, view=None):
        """
        Creates a display for the provided workspace.

        :param ws: Workspace to be displayed
        :param plot: Plotting function that will be used to plot workspaces. Passed in as parameter to allow mocking
        :param parent: Parent of the widget
        :param model: Model to be used by the widget. Passed in as parameter to allow mocking
        :param view: View to be used by the widget. Passed in as parameter to allow mocking
        """
        # Create model and view, or accept mocked versions
        self.model = model if model else MatrixWorkspaceDisplayModel(ws)
        self.view = view if view else MatrixWorkspaceDisplayView(self,
                                                                 parent,
                                                                 self.model.get_name())

        self.plot = plot
        self.setup_tables()

        self.view.set_context_menu_actions(self.view.table_y)
        self.view.set_context_menu_actions(self.view.table_x)
        self.view.set_context_menu_actions(self.view.table_e)

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
        copy_spectrum_values(table, ws_read)

    def action_copy_bin_values(self, table):
        ws_read = self._get_ws_read_from_type(table.model().type)
        num_rows = self.model._ws.getNumberHistograms()
        copy_bin_values(table, ws_read, num_rows)

    def action_copy_cells(self, table):
        copy_cells(table)

    def _do_action_plot(self, table, axis, get_index, plot_errors=False):
        if self.plot is None:
            raise ValueError("Trying to do a plot, but no plotting class dependency was injected in the constructor")
        selection_model = table.selectionModel()
        if not selection_model.hasSelection():
            show_no_selection_to_copy_toast()
            return

        if axis == MantidAxType.SPECTRUM:
            selected = selection_model.selectedRows()  # type: list
        else:
            selected = selection_model.selectedColumns()  # type: list

        if len(selected) > self.NUM_SELECTED_FOR_CONFIRMATION and not self.view.ask_confirmation(
                self.A_LOT_OF_THINGS_TO_PLOT_MESSAGE.format(len(selected))):
            return

        plot_kwargs = {"capsize": 3} if plot_errors else {}
        plot_kwargs["axis"] = axis

        ws_list = [self.model._ws]
        self.plot(ws_list, wksp_indices=[get_index(index) for index in selected], errors=plot_errors,
                  plot_kwargs=plot_kwargs)

    def action_plot_spectrum(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row())

    def action_plot_spectrum_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.SPECTRUM, lambda index: index.row(), plot_errors=True)

    def action_plot_bin(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column())

    def action_plot_bin_with_errors(self, table):
        self._do_action_plot(table, MantidAxType.BIN, lambda index: index.column(), plot_errors=True)

    def action_keypress_copy(self, table):
        selectionModel = table.selectionModel()
        if not selectionModel.hasSelection():
            show_no_selection_to_copy_toast()
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
        else:
            raise ValueError("Unknown TableViewModel type {}".format(type))
