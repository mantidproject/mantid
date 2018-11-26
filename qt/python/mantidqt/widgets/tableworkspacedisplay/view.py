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
from qtpy.QtWidgets import (QAction, QHeaderView, QMenu, QMessageBox, QTableView, QTableWidget)

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

        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        # palette = self.palette()
        # palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        # self.setPalette(palette)

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)

        self.resize(600, 400)
        self.show()

    def doubleClickedHeader(self):
        print("Double clicked WOO")

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.Copy):
            self.presenter.action_keypress_copy(self)

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
        # horizontalHeader.setSectionResizeMode(QHeaderView.Fixed)

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
        main_menu = QMenu()
        plot = QMenu("Plot...", main_menu)
        plot_line = QAction(self.GRAPH_ICON, "Line", plot)
        plot_line.triggered.connect(partial(self.presenter.action_plot, PlotType.LINEAR))

        plot_scatter = QAction(self.GRAPH_ICON, "Scatter", plot)
        plot_scatter.triggered.connect(partial(self.presenter.action_plot, PlotType.SCATTER))

        plot_line_and_points = QAction(self.GRAPH_ICON, "Line + Symbol", plot)
        plot_line_and_points.triggered.connect(partial(self.presenter.action_plot, PlotType.LINE_AND_SYMBOL))

        plot.addAction(plot_line)
        plot.addAction(plot_scatter)
        plot.addAction(plot_line_and_points)
        main_menu.addMenu(plot)

        copy_bin_values = QAction(self.COPY_ICON, "Copy", main_menu)
        copy_bin_values.triggered.connect(self.presenter.action_copy_bin_values)

        set_as_x = QAction(self.TBD, "Set as X", main_menu)
        set_as_x.triggered.connect(self.presenter.action_set_as_x)

        statistics_on_columns = QAction(self.STATISTICS_ON_ROW, "Statistics on Columns", main_menu)
        statistics_on_columns.triggered.connect(self.presenter.action_statistics_on_columns)

        hide_selected = QAction(self.TBD, "Hide Selected", main_menu)
        hide_selected.triggered.connect(self.presenter.action_hide_selected)

        show_all_columns = QAction(self.TBD, "Show All Columns", main_menu)
        show_all_columns.triggered.connect(self.presenter.action_show_all_columns)

        sort_ascending = QAction(self.TBD, "Sort Ascending", main_menu)
        sort_ascending.triggered.connect(partial(self.presenter.action_sort_ascending, Qt.AscendingOrder))

        sort_descending = QAction(self.TBD, "Sort Descending", main_menu)
        sort_descending.triggered.connect(partial(self.presenter.action_sort_ascending, Qt.DescendingOrder))

        main_menu.addAction(copy_bin_values)
        main_menu.addAction(self.make_separator(main_menu))
        main_menu.addAction(set_as_x)
        main_menu.addAction(self.make_separator(main_menu))
        main_menu.addAction(statistics_on_columns)
        main_menu.addAction(self.make_separator(main_menu))
        main_menu.addAction(hide_selected)
        main_menu.addAction(show_all_columns)
        main_menu.addAction(self.make_separator(main_menu))
        main_menu.addAction(sort_ascending)
        main_menu.addAction(sort_descending)

        main_menu.exec_(self.mapToGlobal(position))

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
