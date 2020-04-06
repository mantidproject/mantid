# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import (QMainWindow, QFileDialog, QHeaderView, QAbstractItemView, QTableWidgetItem)
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from qtpy import uic
import os
from abc import ABCMeta, abstractmethod
from six import with_metaclass

from mantidqt.widgets import (manageuserdirectories, instrumentselector)
from mantidqt import icons
from mantid.kernel import UsageService, FeatureType, config, logger
from .specifications import RundexSettings


class DrillEventListener(with_metaclass(ABCMeta, object)):
    """
    Defines the elements which a presenter can listen to in this View
    """

    @abstractmethod
    def on_add_row(self, position):
        pass

    @abstractmethod
    def on_del_row(self, row):
        pass

    @abstractmethod
    def on_data_changed(self, row, column, content):
        pass

    @abstractmethod
    def on_process(self, rows):
        pass

    @abstractmethod
    def on_instrument_changed(self, instrument):
        pass

    @abstractmethod
    def on_rundex_loaded(self, rundex):
        pass

    @abstractmethod
    def on_rundex_saved(self, filename):
        pass

    @abstractmethod
    def update_view_from_model(self):
        pass


class DrillView(QMainWindow):

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.setup_header()
        self.setup_table()

        self.settings_listeners = []
        self.buffer = list()  # for copy-paste actions

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.instrumentSelectionChanged.connect(
                self.change_instrument_requested)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)

        self.datadirs.setIcon(icons.get_icon("mdi.folder"))
        self.datadirs.clicked.connect(self.show_directory_manager)

        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)

        self.settings.setIcon(icons.get_icon("mdi.settings"))
        self.settings.clicked.connect(self.show_settings)

        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.paste.clicked.connect(self.paste_rows)

        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.copy.clicked.connect(self.copy_selected_rows)

        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.cut.clicked.connect(self.cut_selected_rows)

        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.erase.clicked.connect(self.erase_selected_rows)

        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.deleterow.clicked.connect(self.del_selected_rows)

        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.addrow.clicked.connect(self.add_row_after)

        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.save.clicked.connect(self.save_rundex)

        self.help.setIcon(icons.get_icon("mdi.help"))
        self.help.clicked.connect(self.display_help)

        self.processRows.setIcon(icons.get_icon("mdi.play"))
        self.processRows.clicked.connect(self.process_selected_rows)

        self.processAll.setIcon(icons.get_icon("mdi.fast-forward"))
        self.processAll.clicked.connect(self.process_all_rows)

        self.stop.setIcon(icons.get_icon("mdi.stop"))

    def setup_table(self):
        table_header = self.table.horizontalHeader()
        table_header.setSectionResizeMode(QHeaderView.ResizeToContents)
        table_header.setDefaultAlignment(Qt.AlignLeft)
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table.cellChanged.connect(
                lambda row, column : self.data_changed(row, column)
                )

    def add_listener(self, listener):
        if not isinstance(listener, DrillEventListener):
            raise ValueError(
                "The listener is not of type DrillEventListener but rather {}".format(type(listener)))
        self.settings_listeners.append(listener)

    def clear_listeners(self):
        self.settings_listeners = []

    def call_settings_listeners(self, target):
        for listener in self.settings_listeners:
            target(listener)


    ###########################################################################
    # table interactions                                                      #
    ###########################################################################

    def add_row(self, position):
        self.table.insertRow(position)

    def del_row(self, position):
        self.table.removeRow(position)

    def erase_row(self, position):
        for column in range(self.table.columnCount()):
            self.table.takeItem(position, column)

    def erase_rows(self, positions):
        for position in positions:
            for column in range(self.table.columnCount()):
                if self.table.item(position, column) is not None:
                    self.table.item(position, column).setText("")

    def get_selected_rows(self):
        selected_rows = self.table.selectionModel().selectedRows()
        rows = [row.row() for row in selected_rows]
        return rows

    def get_last_selected_row(self):
        rows = self.get_selected_rows()
        if rows:
            return rows[-1]
        else:
            return -1

    def get_all_rows(self):
        return range(self.table.rowCount())

    def get_cell_contents(self, row, column):
        cell = self.table.item(row, column)
        if cell:
            return cell.text()
        else:
            return ""

    def set_cell_contents(self, row, column, contents):
        cell = QTableWidgetItem(contents)
        self.table.setItem(row, column, cell)

    def get_row_contents(self, row):
        contents = list()
        for column in range(self.table.columnCount()):
            contents.append(self.get_cell_contents(row, column))
        return contents

    def set_row_contents(self, row, contents):
        column = 0
        for txt in contents:
            self.set_cell_contents(row, column, txt)
            column += 1


    ###########################################################################
    # actions                                                                 #
    ###########################################################################

    def show_settings(self):
        settings = QMainWindow(self)
        uic.loadUi(os.path.join(self.here, self.technique + '_settings.ui'), settings)
        settings.show()

    def copy_selected_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Copy rows button"], False)
        rows = self.get_selected_rows()
        if not rows:
            return
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.get_row_contents(row))

    def cut_selected_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Cut rows button"], False)
        rows = self.get_selected_rows()
        if not rows:
            return
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.get_row_contents(row))
        self.del_selected_rows()

    def paste_rows(self):
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Paste rows button"], False)
        position = self.get_last_selected_row() + 1
        for row_contents in self.buffer:
            self.add_row(position)
            self.call_settings_listeners(
                    lambda listener: listener.on_add_row(position)
                    )
            self.set_row_contents(position, row_contents)
            position += 1

    def add_row_after(self):
        position = self.get_last_selected_row() + 1
        self.add_row(position)
        self.call_settings_listeners(
                lambda listener: listener.on_add_row(position)
                )

    def del_selected_rows(self):
        rows = self.get_selected_rows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.del_row(row)
            self.call_settings_listeners(
                    lambda listener: listener.on_del_row(row)
                    )

    def erase_selected_rows(self):
        rows = self.get_selected_rows()
        for row in rows:
            self.erase_row(row)
            for column in range(self.table.columnCount()):
                self.call_settings_listeners(
                        lambda listener: listener.on_data_changed(row, column,
                            "")
                        )

    def process_selected_rows(self):
        rows = self.get_selected_rows()
        if rows:
            self.call_settings_listeners(
                    lambda listener: listener.on_process(rows)
                    )

    def process_all_rows(self):
        rows = self.get_all_rows()
        if rows:
            self.call_settings_listeners(
                    lambda listener: listener.on_process(rows)
                    )

    def data_changed(self, row, column):
        contents = self.get_cell_contents(row, column)
        self.call_settings_listeners(
                lambda listener: listener.on_data_changed(row, column, contents)
                )

    def load_rundex(self):
        filename = QFileDialog.getOpenFileName(self, 'Load rundex')
        if not filename[0]:
            return
        self.call_settings_listeners(
                lambda listener: listener.on_rundex_loaded(filename[0])
                )

    def save_rundex(self):
        filename = QFileDialog.getSaveFileName(self, 'Save rundex')
        if not filename[0]:
            return
        self.call_settings_listeners(
                lambda listener: listener.on_rundex_saved(filename[0])
                )

    def display_help(self):
        pass

    def keyPressEvent(self, event):
        if (event.key() == Qt.Key_C
                and event.modifiers() == Qt.ControlModifier):
            self.copy_selected_rows()
        elif (event.key() == Qt.Key_X
                and event.modifiers() == Qt.ControlModifier):
            self.cut_selected_rows()
        elif (event.key() == Qt.Key_V
                and event.modifiers() == Qt.ControlModifier):
            self.paste_rows()
        elif (event.key() == Qt.Key_Delete):
            self.del_selected_rows()

    def change_instrument_requested(self, instrument):
        self.call_settings_listeners(
                lambda listener: listener.on_instrument_changed(instrument)
                )

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()

    ###########################################################################
    # for model calls                                                         #
    ###########################################################################

    def set_header(self, techniques):
        self.instrumentselector.setTechniques(techniques)

    def set_table(self, columns, rows_contents):
        self.table.clear()
        self.table.setRowCount(0)
        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)
        if rows_contents:
            self.table.setRowCount(len(rows_contents))
            self.table.cellChanged.disconnect()
            for row in range(len(rows_contents)):
                self.set_row_contents(row, rows_contents[row])

            self.table.cellChanged.connect(
                    lambda row, column : self.data_changed(row, column)
                    )
        elif columns:
            # if model is empty but the instrument is supported, add an empty row
            self.add_row_after()

    def set_technique(self, technique):
        self.technique = technique

    def set_progress(self, n, nmax):
        self.progressBar.setMaximum(nmax)
        self.progressBar.setValue(n)

    def set_disabled(self, state):
        self.instrumentselector.setDisabled(state)
        self.datadirs.setDisabled(state)
        self.load.setDisabled(state)
        self.settings.setDisabled(state)
        self.paste.setDisabled(state)
        self.copy.setDisabled(state)
        self.cut.setDisabled(state)
        self.erase.setDisabled(state)
        self.deleterow.setDisabled(state)
        self.addrow.setDisabled(state)
        self.save.setDisabled(state)
        self.processRows.setDisabled(state)
        self.processAll.setDisabled(state)
        self.table.setDisabled(state)

