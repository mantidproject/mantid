# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QMainWindow, QFileDialog, QHeaderView, \
                           QAbstractItemView, QTableWidgetItem
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from qtpy import uic

from mantidqt.widgets import manageuserdirectories, instrumentselector
from mantid.kernel import UsageService, FeatureType, config, logger
from mantidqt import icons

from .DrillHeaderView import DrillHeaderView
from .DrillItemDelegate import DrillItemDelegate

class DrillView(QMainWindow):

    # Signals that the view can send and data that they include
    instrument_changed = Signal(str)  # the instrument
    technique_changed = Signal(int)   # the technique index
    row_added = Signal(int)           # the row index
    row_deleted = Signal(int)         # the row index
    data_changed = Signal(int, int)   # the row and column indexes
    process = Signal(list)            # the list of row indexes
    process_stopped = Signal()
    rundex_loaded = Signal(str)       # the path and filename
    rundex_saved = Signal(str)        # the path and filename

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, 'main.ui'), self)
        self.setup_header()
        self.setup_table()

        self.buffer = list()  # for row cut-copy-paste

    def setup_header(self):
        """
        Setup the window header. Set the buttons icons and connect the signals.
        """
        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.headerLeft.addWidget(self.instrumentselector, 0, Qt.AlignLeft)
        self.instrumentselector.instrumentSelectionChanged.connect(
                lambda i : self.instrument_changed.emit(i)
                )

        self.techniqueSelector.activated.connect(
                lambda t : self.technique_changed.emit(t)
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
        """
        Setup the main table widget.
        """
        header = DrillHeaderView()
        header.setSectionsClickable(True)
        header.setHighlightSections(True)
        self.table.setHorizontalHeader(header)
        table_header = self.table.horizontalHeader()
        table_header.setDefaultAlignment(Qt.AlignLeft)
        table_header.setSectionResizeMode(QHeaderView.Interactive)
        delegate = DrillItemDelegate(self.table)
        self.table.setItemDelegate(delegate)
        self.table.cellChanged.connect(
                lambda row, column : self.data_changed.emit(row, column)
                )

    ###########################################################################
    # table interactions                                                      #
    ###########################################################################

    def add_row(self, position):
        """
        Add a row in the table at a given valid postion.

        Args:
            position (int): row index
        """
        n_rows = self.table.rowCount()
        if ((position < 0) or (position > n_rows)):
            return
        self.table.insertRow(position)
        self.row_added.emit(position)

    def del_row(self, position):
        """
        Delete a row at a given position (if this row exists).

        Args:
            position(int): row index
        """
        n_rows = self.table.rowCount()
        if ((position < 0) or (position >= n_rows)):
            return
        self.table.removeRow(position)
        self.row_deleted.emit(position)

    def erase_row(self, position):
        """
        Erase the contents of a whole row (if it exists).

        Args:
            position (int): row index
        """
        n_rows = self.table.rowCount()
        if ((position < 0) or (position >= n_rows)):
            return
        for column in range(self.table.columnCount()):
            self.table.takeItem(position, column)
            self.data_changed.emit(position, column)

    def get_selected_rows(self):
        """
        Get the list of currently selected rows.

        Returns:
            list(int): list of selected rows indexes
        """
        selected_rows = self.table.selectionModel().selectedRows()
        rows = [row.row() for row in selected_rows]
        return rows

    def get_selected_cells(self):
        """
        Get the coordinates of the selected cells.

        Returns:
            list(tuple(int, int)): the coordinates (row, column) of the
                selected cells
        """
        selected_indexes = self.table.selectionModel().selectedIndexes()
        return [(i.row(), i.column()) for i in selected_indexes]

    def get_last_selected_row(self):
        """
        Get the further down selected row.

        Returns:
            int: the row index, -1 if no row selected.
        """
        rows = self.get_selected_rows()
        if rows:
            return rows[-1]
        else:
            return -1

    def get_all_rows(self):
        """
        Get the list of all rows indexes.

        Returns:
            list(int): list of rows indexes
        """
        return list(range(self.table.rowCount()))

    def get_cell_contents(self, row, column):
        """
        Get the contents of a given cell as a string.

        Args:
            row (int): row index
            column (int): column index

        Returns:
            str: cell contents
        """
        cell = self.table.item(row, column)
        if cell:
            return cell.text()
        else:
            return ""

    def set_cell_contents(self, row, column, contents):
        """
        Set the content of an existing cell.

        Args:
            row (int): row index
            column (int): column index
            contents (str): cell contents
        """
        n_rows = self.table.rowCount()
        n_columns = self.table.columnCount()
        if ((row < 0) or (row >= n_rows) \
            or (column < 0) or (column >= n_columns)):
            return
        cell = QTableWidgetItem(contents)
        self.table.setItem(row, column, cell)

    def get_row_contents(self, row):
        """
        Get the contents of a whole row.

        Args:
            row (int): row index

        Returns:
            list(str): the row contents
        """
        contents = list()
        for column in range(self.table.columnCount()):
            contents.append(self.get_cell_contents(row, column))
        return contents

    def set_row_contents(self, row, contents):
        """
        Set the content of an existing row.

        Args:
            row (int): row index
            contents (list(str)): contents
        """
        n_rows = self.table.rowCount()
        if ((row < 0) or (row >= n_rows)):
            return
        column = 0
        for txt in contents:
            self.set_cell_contents(row, column, txt)
            column += 1

    ###########################################################################
    # actions                                                                 #
    ###########################################################################

    def show_settings(self):
        """
        Show settings window according to the selected technique.
        """
        settings = QMainWindow(self)
        technique = self.techniqueSelector.currentText()
        uic.loadUi(os.path.join(self.here, technique + '_settings.ui'), settings)
        settings.show()

    def copy_selected_rows(self):
        """
        Copy the selected rows in a local buffer.
        """
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Copy rows button"], False)
        rows = self.get_selected_rows()
        if not rows:
            return
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.get_row_contents(row))

    def cut_selected_rows(self):
        """
        Cut the selected rows.
        """
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
        """
        Paste the buffer in new rows.
        """
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Paste rows button"], False)
        position = self.get_last_selected_row() + 1
        for row_contents in self.buffer:
            self.add_row(position)
            self.set_row_contents(position, row_contents)
            position += 1

    def add_row_after(self):
        """
        Add a row after the selected ones.
        """
        position = self.get_last_selected_row() + 1
        self.add_row(position)

    def del_selected_rows(self):
        """
        Delete the selected rows.
        """
        rows = self.get_selected_rows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.del_row(row)

    def erase_selected_rows(self):
        """
        Erase the contents of the selected rows.
        """
        rows = self.get_selected_rows()
        for row in rows:
            self.erase_row(row)

    def process_selected_rows(self):
        """
        Ask for the processing of the selected rows.
        """
        rows = self.get_selected_rows()
        if rows:
            self.process.emit(rows)

    def process_all_rows(self):
        """
        Ask for the processing of all the rows.
        """
        rows = self.get_all_rows()
        if rows:
            self.process.emit(rows)

    def load_rundex(self):
        """
        Ask for the loading of a rundex file.
        """
        filename = QFileDialog.getOpenFileName(
                self, 'Load rundex', '.', "Rundex (*.mrd);;All files (*.*)"
                )
        if not filename[0]:
            return
        self.rundex_loaded.emit(filename[0])

    def save_rundex(self):
        """
        Ask for the saving of the table in a rundex file.
        """
        filename = QFileDialog.getSaveFileName(
                self, 'Save rundex', '.', "Rundex (*.mrd)"
                )
        if not filename[0]:
            return
        self.rundex_saved_emit(filename[0])

    def display_help(self):
        pass

    def keyPressEvent(self, event):
        """
        Deal with key pressed events.

        Args:
            event (QPressEvent): the key event
        """
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
        """
        Open the Mantid user directories manager.
        """
        manageuserdirectories.ManageUserDirectories(self).exec_()

    ###########################################################################
    # for model calls                                                         #
    ###########################################################################

    def set_available_instruments(self, techniques):
        """
        Change the available instruments in the comboxbox based on the
        supported techniques.

        Args:
            techniques (list(str)): list of supported techniques
        """
        self.instrumentselector.setTechniques(techniques)

    def set_available_techniques(self, techniques):
        """
        Set the available techniques in the comboxbox.

        Args:
            techniques (list(str)): list of techniques
        """
        self.techniqueSelector.clear()
        self.techniqueSelector.addItems(techniques)

    def set_technique(self, technique):
        self.techniqueSelector.setCurrentIndex(technique)

    def set_table(self, columns):
        """
        Set the table header.

        Args:
            columns (list(str)): list of columns titles
        """
        self.table.clear()
        self.table.setRowCount(0)
        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)
        self.table.resizeColumnsToContents()

    def fill_table(self, rows_contents):
        """
        Fill the table.

        Args:
            rows_contents (list(list(str))): list of rows contents
        """
        self.table.setRowCount(len(rows_contents))
        for row in range(len(rows_contents)):
            self.set_row_contents(row, rows_contents[row])

    def set_progress(self, n, nmax):
        """
        Update the progress bar.

        Args:
            n (int): current value to display
            nmax (int): max value of the progress bar
        """
        self.progressBar.setMaximum(nmax)
        self.progressBar.setValue(n)

    def set_disabled(self, state):
        """
        Disable the drill interface to avoid modifying the input table during
        a processing.

        Args:
            state (bool): 'True' to disable, 'False' to enable again
        """
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

