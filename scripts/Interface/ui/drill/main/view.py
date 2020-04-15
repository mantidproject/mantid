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
from .DrillHeaderView import DrillHeaderView
from .DrillItemDelegate import DrillItemDelegate

class DrillView(QMainWindow):

    instrument_changed = Signal(str)
    row_added = Signal(int)
    row_deleted = Signal(int)
    data_changed = Signal(int, int)
    process = Signal(list)
    process_stopped = Signal()
    rundex_loaded = Signal(str)
    rundex_saved = Signal(str)

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.setup_header()
        self.setup_table()

    def setup_header(self):
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)
        self.instrumentselector.instrumentSelectionChanged.connect(
                lambda i : self.instrument_changed.emit(i)
                )

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
        self.stop.clicked.connect(
                lambda : self.process_stopped.emit()
                )

    def setup_table(self):
        header = DrillHeaderView()
        self.table.setHorizontalHeader(header)
        table_header = self.table.horizontalHeader()
        table_header.setDefaultAlignment(Qt.AlignLeft)
        table_header.setSectionResizeMode(QHeaderView.Interactive)
        delegate = DrillItemDelegate(self.table)
        self.table.setItemDelegate(delegate)
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table.cellChanged.connect(
                lambda row, column : self.data_changed.emit(row, column)
                )

    ###########################################################################
    # table interactions                                                      #
    ###########################################################################

    def add_row(self, position):
        self.table.insertRow(position)
        self.row_added.emit(position)

    def del_row(self, position):
        self.table.removeRow(position)
        self.row_deleted.emit(position)

    def erase_row(self, position):
        for column in range(self.table.columnCount()):
            self.table.takeItem(position, column)
            self.data_changed.emit(position, column)

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
            self.set_row_contents(position, row_contents)
            position += 1

    def add_row_after(self):
        position = self.get_last_selected_row() + 1
        self.add_row(position)

    def del_selected_rows(self):
        rows = self.get_selected_rows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.del_row(row)

    def erase_selected_rows(self):
        rows = self.get_selected_rows()
        for row in rows:
            self.erase_row(row)

    def process_selected_rows(self):
        rows = self.get_selected_rows()
        if rows:
            self.process.emit(rows)

    def process_all_rows(self):
        rows = self.get_all_rows()
        if rows:
            self.process.emit(rows)

    def load_rundex(self):
        filename = QFileDialog.getOpenFileName(
                self, 'Load rundex', '.', "Rundex (*.mrd);;All files (*.*)"
                )
        if not filename[0]:
            return
        self.rundex_loaded.emit(filename[0])

    def save_rundex(self):
        filename = QFileDialog.getSaveFileName(
                self, 'Save rundex', '.', "Rundex (*.mrd)"
                )
        if not filename[0]:
            return
        self.rundex_saved_emit(filename[0])

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

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()

    ###########################################################################
    # for model calls                                                         #
    ###########################################################################

    def set_available_instruments(self, techniques):
        self.instrumentselector.setTechniques(techniques)

    def set_table(self, columns):
        self.table.clear()
        self.table.setRowCount(0)
        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)
        self.table.resizeColumnsToContents()

    def fill_table(self, rows_contents):
        self.table.setRowCount(len(rows_contents))
        for row in range(len(rows_contents)):
            self.set_row_contents(row, rows_contents[row])

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

