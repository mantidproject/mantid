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

import sys
from functools import partial
from logging import warning

from qtpy import QtGui
from qtpy.QtCore import QPoint, Qt
from qtpy.QtGui import QCursor, QFont, QFontMetrics, QKeySequence
from qtpy.QtWidgets import (QAbstractItemView, QAction, QHeaderView, QMessageBox, QTabWidget, QTableView, QToolTip)

import mantidqt.icons
from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayView(QTabWidget):
    def __init__(self, presenter, parent=None, name=''):
        super(MatrixWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        self.COPY_ICON = mantidqt.icons.get_icon("fa.files-o")
        self.GRAPH_ICON = mantidqt.icons.get_icon('fa.line-chart')

        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        palette = self.palette()
        palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        self.setPalette(palette)

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)

        self.active_tab_index = 0
        self.currentChanged.connect(self.set_scroll_position_on_new_focused_tab)

        # local list to keep track of the added tabs
        self.tabs = []

        self.table_y = self.add_table("Y values")
        self.table_x = self.add_table("X values")
        self.table_e = self.add_table("E values")

        self.resize(600, 400)
        self.show()

    def add_table(self, label):
        tab = QTableView()
        tab.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.addTab(tab, label)
        self.tabs.append(tab)
        return tab

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.Copy):
            self.presenter.action_keypress_copy(self.tabs[self.currentIndex()])
        super(MatrixWorkspaceDisplayView, self).keyPressEvent(event)

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
        horizontalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        horizontalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_bin_values = QAction(self.COPY_ICON, "Copy", horizontalHeader)
        copy_bin_values.triggered.connect(partial(self.presenter.action_copy_bin_values, table))

        plot_bin_action = QAction(self.GRAPH_ICON, "Plot bin (values only)", horizontalHeader)
        plot_bin_action.triggered.connect(partial(self.presenter.action_plot_bin, table))
        plot_bin_with_errors_action = QAction(self.GRAPH_ICON, "Plot bin (values + errors)", horizontalHeader)
        plot_bin_with_errors_action.triggered.connect(partial(self.presenter.action_plot_bin_with_errors, table))
        separator1 = QAction(horizontalHeader)
        separator1.setSeparator(True)

        horizontalHeader.addAction(copy_bin_values)
        horizontalHeader.addAction(separator1)
        horizontalHeader.addAction(plot_bin_action)
        horizontalHeader.addAction(plot_bin_with_errors_action)

        verticalHeader = table.verticalHeader()
        verticalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        verticalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_spectrum_values = QAction(self.COPY_ICON, "Copy", verticalHeader)
        copy_spectrum_values.triggered.connect(
            partial(self.presenter.action_copy_spectrum_values, table))

        plot_spectrum_action = QAction(self.GRAPH_ICON, "Plot spectrum (values only)", verticalHeader)
        plot_spectrum_action.triggered.connect(partial(self.presenter.action_plot_spectrum, table))
        plot_spectrum_with_errors_action = QAction(self.GRAPH_ICON, "Plot spectrum (values + errors)",
                                                   verticalHeader)
        plot_spectrum_with_errors_action.triggered.connect(
            partial(self.presenter.action_plot_spectrum_with_errors, table))
        separator1 = QAction(verticalHeader)
        separator1.setSeparator(True)

        verticalHeader.addAction(copy_spectrum_values)
        verticalHeader.addAction(separator1)
        verticalHeader.addAction(plot_spectrum_action)
        verticalHeader.addAction(plot_spectrum_with_errors_action)

    def set_model(self, model_x, model_y, model_e):
        self._set_table_model(self.table_x, model_x, MatrixWorkspaceTableViewModelType.x)
        self._set_table_model(self.table_y, model_y, MatrixWorkspaceTableViewModelType.y)
        self._set_table_model(self.table_e, model_e, MatrixWorkspaceTableViewModelType.e)

    @staticmethod
    def _set_table_model(table, model, expected_model_type):
        assert model.type == expected_model_type, \
            "The model for the table with {0} values has a wrong model type: {1}".format(expected_model_type.upper(),
                                                                                         model.model_type)
        table.setModel(model)

    @staticmethod
    def copy_to_clipboard(data):
        """
        Uses the QGuiApplication to copy to the system clipboard.

        :type data: str
        :param data: The data that will be copied to the clipboard
        :return:
        """
        # This function is likely to be OS specific
        if not sys.platform == "win32":
            warning("Copying to clipboard has not been tested on UNIX systems.")
        cb = QtGui.QGuiApplication.clipboard()
        cb.setText(data, mode=cb.Clipboard)

    def show_mouse_toast(self, message):
        # Creates a text with empty space to get the height of the rendered text - this is used
        # to provide the same offset for the tooltip, scaled relative to the current resolution and zoom.
        font_metrics = QFontMetrics(QFont(" "))
        # The height itself is divided by 2 just to reduce the offset so that the tooltip is
        # reasonably position relative to the cursor
        QToolTip.showText(QCursor.pos() + QPoint(font_metrics.height() / 2, 0), message)

    def ask_confirmation(self, message, title="Mantid Workbench"):
        """
        :param message:
        :return:
        """
        reply = QMessageBox.question(self, title, message, QMessageBox.Yes, QMessageBox.No)
        return True if reply == QMessageBox.Yes else False
