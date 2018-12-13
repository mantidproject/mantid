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

from functools import partial

from qtpy import QtGui
from qtpy.QtCore import Qt
from qtpy.QtGui import QKeySequence
from qtpy.QtWidgets import (QAction, QHeaderView, QMenu, QMessageBox, QTableWidget)

import mantidqt.icons
from mantidqt.widgets.tableworkspacedisplay.plot_type import PlotType


class TableWorkspaceDisplayView(QTableWidget):
    def __init__(self, presenter, parent=None, name=''):
        super(TableWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        self.COPY_ICON = mantidqt.icons.get_icon("fa.files-o")
        self.DELETE_ROW = mantidqt.icons.get_icon("fa.minus-square-o")
        self.STATISTICS_ON_ROW = mantidqt.icons.get_icon('fa.fighter-jet')
        self.GRAPH_ICON = mantidqt.icons.get_icon('fa.line-chart')
        self.TBD = mantidqt.icons.get_icon('fa.question')

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)

        self.resize(600, 400)
        self.show()

    def doubleClickedHeader(self):
        print("Double clicked WOO")

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.Copy):
            self.presenter.action_keypress_copy()
            return
        elif event.key() == Qt.Key_F2 or event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
            self.edit(self.currentIndex())
            return

    def set_context_menu_actions(self, table):
        """
        Sets up the context menu actions for the table
        :type table: QTableView
        :param table: The table whose context menu actions will be set up.
        :param ws_read_function: The read function used to efficiently retrieve data directly from the workspace
        """
        copy_action = QAction(self.COPY_ICON, "Copy", table)
        copy_action.triggered.connect(self.presenter.action_copy_cells)

        table.setContextMenuPolicy(Qt.ActionsContextMenu)
        table.addAction(copy_action)

        horizontalHeader = table.horizontalHeader()
        horizontalHeader.setContextMenuPolicy(Qt.CustomContextMenu)
        horizontalHeader.customContextMenuRequested.connect(self.custom_context_menu)

        verticalHeader = table.verticalHeader()
        verticalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        verticalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_spectrum_values = QAction(self.COPY_ICON, "Copy", verticalHeader)
        copy_spectrum_values.triggered.connect(self.presenter.action_copy_spectrum_values)

        delete_row = QAction(self.DELETE_ROW, "Delete Row", verticalHeader)
        delete_row.triggered.connect(self.presenter.action_delete_row)

        separator2 = self.make_separator(verticalHeader)

        verticalHeader.addAction(copy_spectrum_values)
        verticalHeader.addAction(separator2)
        verticalHeader.addAction(delete_row)

    def custom_context_menu(self, position):
        menu_main = QMenu()
        plot = QMenu("Plot...", menu_main)
        plot_line = QAction(self.GRAPH_ICON, "Line", plot)
        plot_line.triggered.connect(partial(self.presenter.action_plot, PlotType.LINEAR))

        plot_line_with_yerr = QAction(self.GRAPH_ICON, "Line with Y Errors", plot)
        plot_line_with_yerr.triggered.connect(partial(self.presenter.action_plot, PlotType.LINEAR_WITH_ERR))

        plot_scatter = QAction(self.GRAPH_ICON, "Scatter", plot)
        plot_scatter.triggered.connect(partial(self.presenter.action_plot, PlotType.SCATTER))

        plot_line_and_points = QAction(self.GRAPH_ICON, "Line + Symbol", plot)
        plot_line_and_points.triggered.connect(partial(self.presenter.action_plot, PlotType.LINE_AND_SYMBOL))

        plot.addAction(plot_line)
        plot.addAction(plot_line_with_yerr)
        plot.addAction(plot_scatter)
        plot.addAction(plot_line_and_points)
        menu_main.addMenu(plot)

        copy_bin_values = QAction(self.COPY_ICON, "Copy", menu_main)
        copy_bin_values.triggered.connect(self.presenter.action_copy_bin_values)

        set_as_x = QAction(self.TBD, "Set as X", menu_main)
        set_as_x.triggered.connect(self.presenter.action_set_as_x)

        set_as_y = QAction(self.TBD, "Set as Y", menu_main)
        set_as_y.triggered.connect(self.presenter.action_set_as_y)

        set_as_none = QAction(self.TBD, "Set as None", menu_main)
        set_as_none.triggered.connect(self.presenter.action_set_as_none)

        statistics_on_columns = QAction(self.STATISTICS_ON_ROW, "Statistics on Columns", menu_main)
        statistics_on_columns.triggered.connect(self.presenter.action_statistics_on_columns)

        hide_selected = QAction(self.TBD, "Hide Selected", menu_main)
        hide_selected.triggered.connect(self.presenter.action_hide_selected)

        show_all_columns = QAction(self.TBD, "Show All Columns", menu_main)
        show_all_columns.triggered.connect(self.presenter.action_show_all_columns)

        sort_ascending = QAction(self.TBD, "Sort Ascending", menu_main)
        sort_ascending.triggered.connect(partial(self.presenter.action_sort_ascending, Qt.AscendingOrder))

        sort_descending = QAction(self.TBD, "Sort Descending", menu_main)
        sort_descending.triggered.connect(partial(self.presenter.action_sort_ascending, Qt.DescendingOrder))

        menu_main.addAction(copy_bin_values)
        menu_main.addAction(self.make_separator(menu_main))
        menu_main.addAction(set_as_x)
        menu_main.addAction(set_as_y)

        marked_y_cols = self.presenter.get_columns_marked_as_y()
        num_y_cols = len(marked_y_cols)

        # If any columns are marked as Y then generate the set error menu
        if num_y_cols > 0:
            menu_set_as_y_err = QMenu("Set error for Y...")
            for col in range(num_y_cols):
                set_as_y_err = QAction(self.TBD, "Y{}".format(col), menu_main)
                # the column index of the column relative to the whole table, this is necessary
                # so that later the data of the column marked as error can be retrieved
                real_column_index = marked_y_cols[col]
                # col here holds the index in the LABEL (multiple Y columns have labels Y0, Y1, YN...)
                # this is NOT the same as the column relative to the WHOLE table
                set_as_y_err.triggered.connect(partial(self.presenter.action_set_as_y_err, real_column_index, col))
                menu_set_as_y_err.addAction(set_as_y_err)
            menu_main.addMenu(menu_set_as_y_err)

        menu_main.addAction(set_as_none)
        menu_main.addAction(self.make_separator(menu_main))
        menu_main.addAction(statistics_on_columns)
        menu_main.addAction(self.make_separator(menu_main))
        menu_main.addAction(hide_selected)
        menu_main.addAction(show_all_columns)
        menu_main.addAction(self.make_separator(menu_main))
        menu_main.addAction(sort_ascending)
        menu_main.addAction(sort_descending)

        menu_main.exec_(self.mapToGlobal(position))

    def make_separator(self, horizontalHeader):
        separator1 = QAction(horizontalHeader)
        separator1.setSeparator(True)
        return separator1

    @staticmethod
    def copy_to_clipboard(data):
        """
        Uses the QGuiApplication to copy to the system clipboard.

        :type data: str
        :param data: The data that will be copied to the clipboard
        :return:
        """
        cb = QtGui.QGuiApplication.clipboard()
        cb.setText(data, mode=cb.Clipboard)

    def ask_confirmation(self, message, title="Mantid Workbench"):
        """
        :param message:
        :return:
        """
        reply = QMessageBox.question(self, title, message, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False

    def show_warning(self, message, title="Mantid Workbench"):
        QMessageBox.warning(self, title, message)
