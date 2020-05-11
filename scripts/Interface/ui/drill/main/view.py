# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QMainWindow, QFileDialog, QHeaderView, \
                           QAbstractItemView, QTableWidgetItem, QMessageBox
from qtpy.QtGui import QIcon
from qtpy.QtCore import *
from qtpy import uic

from mantidqt.widgets import manageuserdirectories, instrumentselector
from mantid.kernel import UsageService, FeatureType, config, logger
from mantidqt import icons

class DrillView(QMainWindow):

    # Signals that the view can send and data that they include
    instrument_changed = Signal(str)       # the instrument
    technique_changed = Signal(int)        # the technique index
    row_added = Signal(int)                # the row index
    row_deleted = Signal(int)              # the row index
    data_changed = Signal(int, int, str)   # the row and column indexes and the contents
    process = Signal(list)                 # the list of row indexes
    process_stopped = Signal()
    rundex_loaded = Signal(str)            # the path and filename
    rundex_saved = Signal(str)             # the path and filename

    def __init__(self):
        super(DrillView, self).__init__()
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, 'main.ui'), self,
                'Interface.ui.drill.main.DrillTableWidget')
        self.setup_header()
        self.setup_table()

        self.buffer = list()  # for row cut-copy-paste

    def setup_header(self):
        """
        Setup the window header. Set the buttons icons and connect the signals.
        """
        self.actionLoadRundex.triggered.connect(self.load_rundex)
        self.actionSaveRundex.triggered.connect(self.save_rundex)
        self.actionManageDirectories.triggered.connect(self.show_directory_manager)
        self.actionSettings.triggered.connect(self.show_settings)
        self.actionAddRow.triggered.connect(self.add_row_after)
        self.actionDelRow.triggered.connect(self.del_selected_rows)
        self.actionCopyRow.triggered.connect(self.copy_selected_rows)
        self.actionCutRow.triggered.connect(self.cut_selected_rows)
        self.actionPasteRow.triggered.connect(self.paste_rows)
        self.actionErase.triggered.connect(self.erase_selected_cells)
        self.actionProcessRow.triggered.connect(self.process_selected_rows)
        self.actionProcessAll.triggered.connect(self.process_all_rows)
        self.actionStopProcessing.triggered.connect(
                lambda : self.process_stopped.emit()
                )

        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.toolbar.insertWidget(0, self.instrumentselector, 0, Qt.AlignLeft)
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
        self.erase.clicked.connect(self.erase_selected_cells)

        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.deleterow.clicked.connect(self.del_selected_rows)

        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.addrow.clicked.connect(self.add_row_after)

        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.save.clicked.connect(self.save_rundex)

        self.help.setIcon(icons.get_icon("mdi.help"))
        self.help.clicked.connect(self.display_help)

        self.fill.setIcon(icons.get_icon("mdi.arrow-expand-down"))
        self.fill.clicked.connect(self.automatic_filling)

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
        self.table.rowAdded.connect(
                lambda position : self.row_added.emit(position)
                )
        self.table.rowDeleted.connect(
                lambda position : self.row_deleted.emit(position)
                )
        self.table.cellChanged.connect(
                lambda row, column :
                self.data_changed.emit(row, column,
                                       self.table.getCellContents(row, column))
                )

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
        rows = self.table.getSelectedRows()
        if not rows:
            return
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.table.getRowContents(row))

    def cut_selected_rows(self):
        """
        Cut the selected rows.
        """
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Cut rows button"], False)
        rows = self.table.getSelectedRows()
        if not rows:
            return
        self.buffer = list()
        for row in rows:
            self.buffer.append(self.table.getRowContents(row))
        self.del_selected_rows()

    def paste_rows(self):
        """
        Paste the buffer in new rows after the selected ones. If no row
        selected, paste at the end of the table.
        """
        UsageService.registerFeatureUsage(
                FeatureType.Feature, ["Drill", "Paste rows button"], False)
        position = self.table.getLastSelectedRow()
        if (position == -1):
            position = self.table.getLastRow()
        position += 1
        for row_contents in self.buffer:
            self.table.addRow(position)
            self.table.setRowContents(position, row_contents)
            position += 1

    def add_row_after(self):
        """
        Add a row after the selected ones. If no row selected, the row is added
        at the end of the table.
        """
        position = self.table.getLastSelectedRow()
        if position == -1:
            position = self.table.getLastRow()
        self.table.addRow(position + 1)

    def del_selected_rows(self):
        """
        Delete the selected rows.
        """
        rows = self.table.getSelectedRows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.table.deleteRow(row)

    def erase_selected_cells(self):
        """
        Erase the contents of the selected cells.
        """
        indexes = self.table.getSelectedCells()
        for (r, c) in indexes:
            self.table.eraseCell(r, c)

    def process_selected_rows(self):
        """
        Ask for the processing of the selected rows.
        """
        rows = self.table.getSelectedRows()
        if not rows:
            rows = self.table.getRowsFromSelectedCells()
        if rows:
            self.process.emit(rows)

    def process_all_rows(self):
        """
        Ask for the processing of all the rows.
        """
        rows = self.table.getAllRows()
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
        self.rundex_saved.emit(filename[0])

    def display_help(self):
        pass

    def automatic_filling(self):
        """
        Copy (and increment) the contents of the first selected cell in the
        other ones. If a numors string is detected in the first cell, the
        numors values are incremented by the number found in the ui spinbox
        associated with this action.
        """
        def inc(numors, i):
            """
            Increment a numors string by i.
            For example, for increment by 1:
            "1000,2000,3000"  ->  "1001,2001,3001"
            "1000+2000,3000"  ->  "1001+2001,3001"
            "1000:2000,3000"  ->  "2001-3001,3001"
            "1000-2000,3000"  ->  "2001-3001,3001"

            Args:
                numors (str): a numors string
                i (int): increment value

            Returns:
                str: A string that represents the incremented numors
            """
            try:
                return str(int(numors) + i)
            except:
                if ((',' in numors) or ('+' in numors)):
                    c = ',' if ',' in numors else '+'
                    splitted = numors.split(c)
                    out = list()
                    for e in splitted:
                        out.append(inc(e, i))
                        if out[-1] == e:
                            return numors
                    return c.join(out)
                elif ((':' in numors) or ('-' in numors)):
                    c = ':' if ':' in numors else '-'
                    splitted = numors.split(c)
                    try:
                        mini = min(int(splitted[0]), int(splitted[1]))
                        maxi = max(int(splitted[0]), int(splitted[1]))
                        if (i > 0):
                            r0 = maxi + i
                            r1 = 2 * maxi + i - mini
                            r0 = 0 if r0 < 0 else r0
                            r1 = 0 if r1 < 0 else r1
                            return str(r0) + c + str(r1)
                        else:
                            r0 = 2 * mini + i - maxi
                            r1 = mini + i
                            r0 = 0 if r0 < 0 else r0
                            r1 = 0 if r1 < 0 else r1
                            return str(r0) + c + str(r1)
                    except:
                        return numors
                else:
                    return numors

        increment = self.increment.value()
        cells = self.table.getSelectedCells()
        if not cells:
            return
        # increment or copy the content of the previous cell
        for i in range(1, len(cells)):
            contents = self.table.getCellContents(cells[i-1][0], cells[i-1][1])
            self.table.setCellContents(cells[i][0], cells[i][1],
                                       inc(contents, increment))

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
            self.erase_selected_cells()

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
        if rows_contents:
            self.blockSignals(True)
            self.table.setRowCount(len(rows_contents))
            for row in range(len(rows_contents)):
                self.table.setRowContents(row, rows_contents[row])
            self.blockSignals(False)
        else:
            self.table.addRow(0)

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
        self.table.clearSelection()

    def set_row_processing(self, row):
        """
        Set a row as currently processing.

        Args:
            row (int): the row index
        """
        self.table.setRowProcessingColor(row)

    def set_row_done(self, row):
        """
        Set a row as done with success.

        Args:
            row (int): the row index
        """
        self.table.setRowOkColor(row)

    def set_row_error(self, row):
        """
        Set a row as done with error.

        Args:
            row (int): the row index
        """
        self.table.setRowErrorColor(row)

    def set_popup(self, elements):
        """
        Display a popup window that represents errors repported by the row
        processing.

        Args:
            elements (list(tuple(int, str))): list of error messages and the
                                              corresponding row
        """
        window = QMessageBox(self)
        window.setWindowTitle("Error")
        text = str()
        for e in elements:
            text += ("Row " + str(int(e[0]) + 1) + ":\n")
            text += e[1]
            text += '\n\n'
        window.setText(text[:-2])
        window.show()

