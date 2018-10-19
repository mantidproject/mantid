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

from PyQt5.QtGui import QIcon, QPixmap
from qtpy import QtGui
from qtpy.QtCore import Qt
from qtpy.QtWidgets import (QAbstractItemView, QAction, QHeaderView, QTabWidget, QTableView)

from mantidqt.widgets.matrixworkspacedisplay.pixmaps import copy_xpm, table_xpm, graph_xpm, new_graph_xpm
from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayView(QTabWidget):
    def __init__(self, presenter, parent=None, name=''):
        super(MatrixWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        # These must be initialised here, if initialised outside of a function
        # the widget does not start at all, and Python just exits silently with
        # exit code -1073741819 (0xC0000005), as it is killed by Windows' Data Execution Prevention
        self.COPY_ICON = QIcon(QPixmap(copy_xpm))
        self.VIEW_DETECTORS_ICON = QIcon(QPixmap(table_xpm))
        self.GRAPH_ICON = QIcon(QPixmap(graph_xpm))
        self.NEW_GRAPH_ICON = QIcon(QPixmap(new_graph_xpm))

        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        palette = self.palette()
        palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        self.setPalette(palette)

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)

        self.table_y = QTableView()
        self.table_y.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.addTab(self.table_y, "Y values")

        self.table_x = QTableView()
        self.table_x.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.addTab(self.table_x, "X values")

        self.table_e = QTableView()
        self.table_e.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.addTab(self.table_e, "E values")

        self.connect_table_scrolling(self.table_y, self.table_x, self.table_e)
        self.connect_table_scrolling(self.table_x, self.table_y, self.table_e)
        self.connect_table_scrolling(self.table_e, self.table_y, self.table_x)

        self.resize(600, 400)
        self.show()

    @staticmethod
    def connect_table_scrolling(source_table, receiving_table_1, receiving_table_2):
        """
        Connect the scrolling signals from a source table to two receiving tables. 
        This will change where the receiving tables point to depending on where
        the source table is scrolled to.

        :param source_table: Source of scrolling signals
        :type source_table: QTableView
        :param receiving_table_1: Receiver of the scrolling signals
        :type receiving_table_1: QTableView
        :param receiving_table_2: Receiver of the scrolling signals
        :type receiving_table_2: QTableView
        :return:
        """
        assert receiving_table_1 != receiving_table_2, "The receiving tables cannot be the same"
        repeating_table_message = "Source table cannot be the same as the receiving table, " \
                                  "check the parameters with which the function is called"
        assert source_table != receiving_table_1, repeating_table_message
        assert source_table != receiving_table_2, repeating_table_message

        def _change_horizontal_position(position):
            receiving_table_1.horizontalScrollBar().setValue(position)
            receiving_table_2.horizontalScrollBar().setValue(position)

        source_table.horizontalScrollBar().valueChanged.connect(_change_horizontal_position)

        def _change_vertical_position(position):
            receiving_table_1.verticalScrollBar().setValue(position)
            receiving_table_2.verticalScrollBar().setValue(position)

        source_table.verticalScrollBar().valueChanged.connect(_change_vertical_position)

    def set_context_menu_actions(self, table, ws_read_function):
        """
        Sets up the context menu actions for the table
        :param table: The table whose context menu actions will be set up.
        :param ws_read_function: The read function used to efficiently retrieve data directly from the workspace
        """
        copy_action = QAction(self.COPY_ICON, "Copy", table)
        # sets the first (table) parameter of the copy action callback
        # so that each context menu can copy the data from the correct table
        decorated_copy_action_with_correct_table = partial(self.presenter.action_copy_cell, table)
        copy_action.triggered.connect(decorated_copy_action_with_correct_table)

        view_detectors_table_action = QAction(self.VIEW_DETECTORS_ICON, "View detectors table", table)

        # Decorates the function with the correct table. This is done
        # instead of having to manually check from which table the callback is coming
        decorated_view_action_with_correct_table = partial(self.presenter.action_view_detectors_table, table)
        view_detectors_table_action.triggered.connect(decorated_view_action_with_correct_table)

        table.setContextMenuPolicy(Qt.ActionsContextMenu)
        table.addAction(copy_action)
        table.addAction(view_detectors_table_action)

        horizontalHeader = table.horizontalHeader()
        horizontalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        horizontalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_bin_values = QAction(self.COPY_ICON, "Copy", horizontalHeader)
        copy_bin_values.triggered.connect(partial(self.presenter.action_copy_bin_values, table, ws_read_function))

        copy_bin_to_table_action = QAction(self.COPY_ICON, "Copy bin to table", horizontalHeader)
        copy_bin_to_table_action.triggered.connect(self.not_implemented)
        plot_bin_action = QAction(self.NEW_GRAPH_ICON, "Plot bin (values only)", horizontalHeader)
        plot_bin_action.triggered.connect(self.not_implemented)
        plot_bin_with_errors_action = QAction(self.NEW_GRAPH_ICON, "Plot bin (values + errors)", horizontalHeader)
        plot_bin_with_errors_action.triggered.connect(self.not_implemented)
        separator1 = QAction(horizontalHeader)
        separator1.setSeparator(True)
        separator2 = QAction(horizontalHeader)
        separator2.setSeparator(True)

        horizontalHeader.addAction(copy_bin_values)
        horizontalHeader.addAction(copy_bin_to_table_action)
        horizontalHeader.addAction(separator1)
        horizontalHeader.addAction(view_detectors_table_action)
        horizontalHeader.addAction(separator2)
        horizontalHeader.addAction(plot_bin_action)
        horizontalHeader.addAction(plot_bin_with_errors_action)

        verticalHeader = table.verticalHeader()
        verticalHeader.setContextMenuPolicy(Qt.ActionsContextMenu)
        verticalHeader.setSectionResizeMode(QHeaderView.Fixed)

        copy_spectrum_values = QAction(self.COPY_ICON, "Copy", verticalHeader)
        copy_spectrum_values.triggered.connect(
            partial(self.presenter.action_copy_spectrum_values, table, ws_read_function))

        copy_spectrum_to_table_action = QAction(self.COPY_ICON, "Copy spectrum to table", verticalHeader)
        copy_spectrum_to_table_action.triggered.connect(self.not_implemented)
        plot_spectrum_action = QAction(self.NEW_GRAPH_ICON, "Plot spectrum (values only)", verticalHeader)
        plot_spectrum_action.triggered.connect(self.not_implemented)
        plot_spectrum_with_errors_action = QAction(self.NEW_GRAPH_ICON, "Plot spectrum (values + errors)",
                                                   verticalHeader)
        plot_spectrum_with_errors_action.triggered.connect(self.not_implemented)
        separator1 = QAction(verticalHeader)
        separator1.setSeparator(True)
        separator2 = QAction(verticalHeader)
        separator2.setSeparator(True)

        verticalHeader.addAction(copy_spectrum_values)
        verticalHeader.addAction(copy_spectrum_to_table_action)
        verticalHeader.addAction(separator1)
        verticalHeader.addAction(view_detectors_table_action)
        verticalHeader.addAction(separator2)
        verticalHeader.addAction(plot_spectrum_action)
        verticalHeader.addAction(plot_spectrum_with_errors_action)

    def set_model(self, model_x, model_y, model_e):
        self._set_table_model(self.table_x, model_x, MatrixWorkspaceTableViewModelType.x)
        self._set_table_model(self.table_y, model_y, MatrixWorkspaceTableViewModelType.y)
        self._set_table_model(self.table_e, model_e, MatrixWorkspaceTableViewModelType.e)

    def not_implemented(self):
        raise NotImplementedError("This method is not implemented yet")

    @staticmethod
    def _set_table_model(table, model, expected_model_type):
        assert model.type == expected_model_type, \
            "The model for the table with {0} values has a wrong model type: {1}".format(expected_model_type.upper(),
                                                                                         model.model_type)
        table.setModel(model)

    @staticmethod
    def copy_to_clipboard(data):
        """
        :type data: str
        :param data: The data that will be copied to the clipboard
        :return:
        """
        # This function is likely to be OS specific
        if not sys.platform == "win32":
            warning("Copying to clipboard has not been tested on UNIX systems.")
        cb = QtGui.QGuiApplication.clipboard()
        cb.setText(data, mode=cb.Clipboard)
