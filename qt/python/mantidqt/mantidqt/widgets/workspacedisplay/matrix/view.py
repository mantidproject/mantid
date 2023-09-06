# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from functools import partial

from qtpy import QtGui
from qtpy.QtCore import Qt
from qtpy.QtGui import QKeySequence
from qtpy.QtWidgets import QAbstractItemView, QAction, QHeaderView, QMenu, QMessageBox, QTabWidget, QTableView

import mantidqt.icons
from mantidqt.widgets.workspacedisplay.matrix.delegate import CustomTextElidingDelegate
from mantidqt.widgets.workspacedisplay.matrix.table_view_model import MatrixWorkspaceTableViewModelType
from mantidqt.plotting.functions import can_overplot

# Constants
ELIDE_NCHARS_RIGHT = 3


class MatrixWorkspaceTableView(QTableView):
    def __init__(self, parent):
        super(MatrixWorkspaceTableView, self).__init__(parent)
        self.setSelectionBehavior(QAbstractItemView.SelectItems)

        header = self.horizontalHeader()
        header.sectionDoubleClicked.connect(self.handle_double_click)

        self.setItemDelegate(CustomTextElidingDelegate(ELIDE_NCHARS_RIGHT))

    def resizeEvent(self, event):
        super(MatrixWorkspaceTableView, self).resizeEvent(event)

        header = self.horizontalHeader()
        header.setSectionResizeMode(QHeaderView.Interactive)

    def handle_double_click(self, section):
        header = self.horizontalHeader()
        header.resizeSection(section, header.defaultSectionSize())


class MatrixWorkspaceDisplayView(QTabWidget):
    def __init__(self, presenter, parent=None, window_flags=Qt.Window):
        super(MatrixWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        self.COPY_ICON = mantidqt.icons.get_icon("mdi.content-copy")
        self.GRAPH_ICON = mantidqt.icons.get_icon("mdi.chart-line")
        self.TABLE_ICON = mantidqt.icons.get_icon("mdi.table")

        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        palette = self.palette()
        palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        self.setPalette(palette)

        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self.setWindowFlags(window_flags)

        self.active_tab_index = 0
        self.currentChanged.connect(self.set_scroll_position_on_new_focused_tab)

        # local list to keep track of the added tabs
        self.tabs = []

        self.table_y = self.add_table("Y values")
        self.table_x = self.add_table("X values")
        self.table_e = self.add_table("E values")

        if self.presenter.hasDx:
            self.table_dx = self.add_table("dX values")

    def add_table(self, label):
        tab = MatrixWorkspaceTableView(self)

        self.addTab(tab, label)
        self.tabs.append(tab)
        return tab

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.Copy):
            self.presenter.action_keypress_copy(self.tabs[self.currentIndex()])
        super(MatrixWorkspaceDisplayView, self).keyPressEvent(event)

    def get_active_tab(self):
        return self.tabs[self.active_tab_index]

    def set_scroll_position_on_new_focused_tab(self, new_tab_index):
        """
        Updates the new focused tab's scroll position to match the old one.
        :type new_tab_index: int
        :param new_tab_index: The widget position index in the parent's widget list
        """
        old_tab = self.widget(self.active_tab_index)
        new_tab = self.widget(new_tab_index)

        new_tab.horizontalScrollBar().setValue(old_tab.horizontalScrollBar().value())
        new_tab.verticalScrollBar().setValue(old_tab.verticalScrollBar().value())
        self.active_tab_index = new_tab_index

    def set_context_menu_actions(self, table):
        """
        Sets up the context menu actions for the table
        :type table: QTableView
        :param table: The table whose context menu actions will be set up.
        :param ws_read_function: The read function used to efficiently retrieve data directly from the workspace
        """
        copy_action = QAction(self.COPY_ICON, "Copy", table)
        # sets the first (table) parameter of the copy action callback
        # so that each context menu can copy the data from the correct table
        decorated_copy_action_with_correct_table = partial(self.presenter.action_copy_cells, table)
        copy_action.triggered.connect(decorated_copy_action_with_correct_table)

        table.setContextMenuPolicy(Qt.ActionsContextMenu)
        table.addAction(copy_action)

        horizontalHeader = table.horizontalHeader()
        horizontalHeader.setContextMenuPolicy(Qt.CustomContextMenu)
        horizontalHeader.setSectionResizeMode(QHeaderView.Fixed)
        horizontalHeader.customContextMenuRequested.connect(self.bin_context_menu_opened)

        verticalHeader = table.verticalHeader()
        verticalHeader.setContextMenuPolicy(Qt.CustomContextMenu)
        verticalHeader.setSectionResizeMode(QHeaderView.Fixed)
        verticalHeader.customContextMenuRequested.connect(self.spectra_context_menu_opened)

    def set_model(self, model_x, model_y, model_e, model_dx):
        self._set_table_model(self.table_x, model_x, MatrixWorkspaceTableViewModelType.x)
        self._set_table_model(self.table_y, model_y, MatrixWorkspaceTableViewModelType.y)
        self._set_table_model(self.table_e, model_e, MatrixWorkspaceTableViewModelType.e)
        if self.presenter.hasDx:
            self._set_table_model(self.table_dx, model_dx, MatrixWorkspaceTableViewModelType.dx)

    @staticmethod
    def _set_table_model(table, model, expected_model_type):
        assert model.type == expected_model_type, "The model for the table with {0} values has a wrong model type: {1}".format(
            expected_model_type.upper(), model.model_type
        )
        table.setModel(model)

    def ask_confirmation(self, message, title="Mantid Workbench"):
        """
        :param message:
        :return:
        """
        reply = QMessageBox.question(self, title, message, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False

    def setup_bin_context_menu(self, table):
        context_menu = QMenu(self)
        self.setup_copy_bin_actions(context_menu, table)
        self.setup_plot_bin_actions(context_menu, table)
        return context_menu

    def setup_spectra_context_menu(self, table):
        context_menu = QMenu(self)
        self.setup_copy_spectrum_actions(context_menu, table)
        self.setup_plot_spectrum_actions(context_menu, table)
        return context_menu

    def bin_context_menu_opened(self, position):
        """
        Open the context menu in the correct location
        :param position: The position to open the menu, e.g. where
                         the mouse button was clicked
        """
        table = self.currentWidget()
        context_menu = self.setup_bin_context_menu(table)
        header = table.horizontalHeader()
        # If you right-click on a column header, then select that column, unless you're already clicking
        # inside a selected column
        index_of_selected_column = header.logicalIndexAt(position)
        if index_of_selected_column not in [x.column() for x in table.selectionModel().selectedColumns()]:
            table.selectColumn(index_of_selected_column)
        context_menu.exec_(header.mapToGlobal(position))

    def spectra_context_menu_opened(self, position):
        """
        Open the context menu in the correct location
        :param position: The position to open the menu, e.g. where
                         the mouse button was clicked
        """
        table = self.currentWidget()
        context_menu = self.setup_spectra_context_menu(table)
        header = table.verticalHeader()
        # If you right-click on a row header, then select that row, unless you're already clicking
        # inside a selected row
        index_of_selected_row = header.logicalIndexAt(position)
        if index_of_selected_row not in [x.row() for x in table.selectionModel().selectedRows()]:
            table.selectRow(index_of_selected_row)
        context_menu.exec_(header.mapToGlobal(position))

    def setup_plot_bin_actions(self, context_menu, table):
        plot_bin_action = QAction(self.GRAPH_ICON, "Plot bin (values only)", self)
        plot_bin_action.triggered.connect(partial(self.presenter.action_plot_bin, table))
        plot_bin_with_errors_action = QAction(self.GRAPH_ICON, "Plot bin (values + errors)", self)
        plot_bin_with_errors_action.triggered.connect(partial(self.presenter.action_plot_bin_with_errors, table))
        overplot_bin_action = QAction(self.GRAPH_ICON, "Overplot bin (values only)", self)
        overplot_bin_action.triggered.connect(partial(self.presenter.action_overplot_bin, table))
        overplot_bin_with_errors_action = QAction(self.GRAPH_ICON, "Overplot bin (values + errors)", self)
        overplot_bin_with_errors_action.triggered.connect(partial(self.presenter.action_overplot_bin_with_errors, table))
        overplot_bin_action.setEnabled(can_overplot())
        overplot_bin_with_errors_action.setEnabled(can_overplot())
        separator = QAction(self)
        separator.setSeparator(True)
        list(
            map(
                context_menu.addAction,
                [plot_bin_action, plot_bin_with_errors_action, separator, overplot_bin_action, overplot_bin_with_errors_action],
            )
        )

    def setup_plot_spectrum_actions(self, context_menu, table):
        plot_spectrum_action = QAction(self.GRAPH_ICON, "Plot spectrum (values only)", self)
        plot_spectrum_action.triggered.connect(partial(self.presenter.action_plot_spectrum, table))
        plot_spectrum_with_errors_action = QAction(self.GRAPH_ICON, "Plot spectrum (values + errors)", self)
        plot_spectrum_with_errors_action.triggered.connect(partial(self.presenter.action_plot_spectrum_with_errors, table))
        overplot_spectrum_action = QAction(self.GRAPH_ICON, "Overplot spectrum (values only)", self)
        overplot_spectrum_action.triggered.connect(partial(self.presenter.action_overplot_spectrum, table))
        overplot_spectrum_with_errors_action = QAction(self.GRAPH_ICON, "Overplot spectrum (values + errors)", self)
        overplot_spectrum_with_errors_action.triggered.connect(partial(self.presenter.action_overplot_spectrum_with_errors, table))
        overplot_spectrum_action.setEnabled(can_overplot())
        overplot_spectrum_with_errors_action.setEnabled(can_overplot())
        separator = QAction(self)
        separator.setSeparator(True)
        list(
            map(
                context_menu.addAction,
                [
                    plot_spectrum_action,
                    plot_spectrum_with_errors_action,
                    separator,
                    overplot_spectrum_action,
                    overplot_spectrum_with_errors_action,
                ],
            )
        )

    def setup_copy_bin_actions(self, context_menu, table):
        copy_bin_values = QAction(self.COPY_ICON, "Copy", self)
        copy_bin_values.triggered.connect(partial(self.presenter.action_copy_bin_values, table))
        copy_bin_to_table = QAction(self.TABLE_ICON, "Copy bin to table", self)
        copy_bin_to_table.triggered.connect(partial(self.presenter.action_copy_bin_to_table, table))
        separator = QAction(self)
        separator.setSeparator(True)
        list(map(context_menu.addAction, [copy_bin_values, copy_bin_to_table, separator]))

    def setup_copy_spectrum_actions(self, context_menu, table):
        copy_spectrum_values = QAction(self.COPY_ICON, "Copy", self)
        copy_spectrum_values.triggered.connect(partial(self.presenter.action_copy_spectrum_values, table))
        copy_spectrum_to_table = QAction(self.TABLE_ICON, "Copy spectrum to table", self)
        copy_spectrum_to_table.triggered.connect(partial(self.presenter.action_copy_spectrum_to_table, table))
        separator = QAction(self)
        separator.setSeparator(True)
        list(map(context_menu.addAction, [copy_spectrum_values, copy_spectrum_to_table, separator]))
