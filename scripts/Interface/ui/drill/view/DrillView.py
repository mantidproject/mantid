# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QMainWindow, QFileDialog, QMessageBox, QDialog
from qtpy.QtCore import *
from qtpy import uic

from mantidqt.widgets import manageuserdirectories, instrumentselector
from mantid.kernel import config  # noqa
from mantidqt import icons
from mantidqt.interfacemanager import InterfaceManager

from ..presenter.DrillPresenter import DrillPresenter


class DrillView(QMainWindow):

    ###########################################################################
    # Signals                                                                 #
    ###########################################################################

    """
    Sent when the instrument changes.
    Args:
        str: instrument name
    """
    instrumentChanged = Signal(str)

    """
    Sent when the acquisition mode changes.
    Args:
        str: acquisition mode name
    """
    acquisitionModeChanged = Signal(str)

    """
    Sent when the cycle number and experiment ID are set.
    Args:
        str, str: cycle number, experiment ID
    """
    cycleAndExperimentChanged = Signal(str, str)

    """
    Sent when a row is added to the table.
    Args:
        int: row index
    """
    rowAdded = Signal(int)

    """
    Sent when a row is deleted from the table.
    Args:
        int: row index
    """
    rowDeleted = Signal(int)

    """
    Sent when a cell contents has changed.
    Args:
        int: row index
        int: column index
        str: new cell contents
    """
    dataChanged = Signal(int, int, str)

    """
    Sent when the user asks for a row processing.
    Args:
        int: row index
    """
    process = Signal(list)

    """
    Sent when the user wants to stop the current processing.
    """
    processStopped = Signal()

    """
    Sent when the user asks for data loading from file.
    Args:
        str: rundex file name
    """
    rundexLoaded = Signal(str)

    """
    Sent when the user aks for data saving.
    Args:
        str: file name
    """
    rundexSaved = Signal(str)

    """
    Sent when the user asks for the settings window.
    """
    showSettings = Signal()

    # colors for the table rows
    OK_COLOR = "#3f00ff00"
    ERROR_COLOR = "#3fff0000"
    PROCESSING_COLOR = "#3fffff00"

    def __init__(self):
        super(DrillView, self).__init__(None, Qt.Window)
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, 'ui/main.ui'), self)
        self.setup_header()
        self.setup_table()
        self.setFocus()

        self.buffer = list()  # for cells cut-copy-paste
        self.bufferShape = tuple() # (n_rows, n_columns) shape of self.buffer
        self.invalidCells = set()
        self.coloredRows = set()
        self.errorRows = set()
        self.rundexFile = None

        self._presenter = DrillPresenter(self)

    def show(self):
        """
        Override QMainWindow::show. It calls the base class method and
        updates the instrument.
        """
        super(DrillView, self).show()
        self._changeInstrument(self.instrumentselector.currentText())

    def setup_header(self):
        """
        Setup the window header. Set the buttons icons and connect the signals.
        """
        self.actionLoadRundex.triggered.connect(self.load_rundex)
        self.actionSaveAs.triggered.connect(self.saveRundexAs)
        self.actionSave.triggered.connect(self.saveRundex)
        self.actionManageDirectories.triggered.connect(self.show_directory_manager)
        self.actionSettings.triggered.connect(self.showSettings.emit)
        self.actionClose.triggered.connect(self.close)
        self.actionAddRow.triggered.connect(self.add_row_after)
        self.actionDelRow.triggered.connect(self.del_selected_rows)
        self.actionCopyRow.triggered.connect(self.copySelectedCells)
        self.actionCutRow.triggered.connect(self.cutSelectedCells)
        self.actionPasteRow.triggered.connect(self.pasteCells)
        self.actionErase.triggered.connect(self.eraseSelectedCells)
        self.actionProcessRow.triggered.connect(self.process_selected_rows)
        self.actionProcessAll.triggered.connect(self.process_all_rows)
        self.actionStopProcessing.triggered.connect(self.processStopped.emit)
        self.actionHelp.triggered.connect(self.helpWindow)

        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.setToolTip("Instrument")
        self.toolbar.insertWidget(0, self.instrumentselector, 0, Qt.AlignLeft)
        self.instrumentselector.instrumentSelectionChanged.connect(
                self._changeInstrument)

        self.modeSelector.currentTextChanged.connect(
                self._changeAcquisitionMode)

        self.cycleNumber.editingFinished.connect(self._changeCycleOrExperiment)
        self.experimentId.editingFinished.connect(self._changeCycleOrExperiment)

        self.datadirs.setIcon(icons.get_icon("mdi.folder"))
        self.datadirs.clicked.connect(self.show_directory_manager)

        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.load_rundex)

        self.settings.setIcon(icons.get_icon("mdi.settings"))
        self.settings.clicked.connect(self.showSettings.emit)

        self.paste.setIcon(icons.get_icon("mdi.content-paste"))
        self.paste.clicked.connect(self.pasteCells)

        self.copy.setIcon(icons.get_icon("mdi.content-copy"))
        self.copy.clicked.connect(self.copySelectedCells)

        self.cut.setIcon(icons.get_icon("mdi.content-cut"))
        self.cut.clicked.connect(self.cutSelectedCells)

        self.erase.setIcon(icons.get_icon("mdi.eraser"))
        self.erase.clicked.connect(self.eraseSelectedCells)

        self.deleterow.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.deleterow.clicked.connect(self.del_selected_rows)

        self.addrow.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.addrow.clicked.connect(self.add_row_after)

        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.save.clicked.connect(self.saveRundexAs)

        self.help.setIcon(icons.get_icon("mdi.help"))
        self.help.clicked.connect(self.helpWindow)

        self.fill.setIcon(icons.get_icon("mdi.arrow-expand-down"))
        self.fill.clicked.connect(self.automatic_filling)

        self.processRows.setIcon(icons.get_icon("mdi.play"))
        self.processRows.clicked.connect(self.process_selected_rows)

        self.processAll.setIcon(icons.get_icon("mdi.fast-forward"))
        self.processAll.clicked.connect(self.process_all_rows)

        self.stop.setIcon(icons.get_icon("mdi.stop"))
        self.stop.clicked.connect(self.processStopped.emit)

    def setup_table(self):
        """
        Setup the main table widget.
        """
        self.table.cellChanged.connect(self.on_cell_changed)

    def closeEvent(self, event):
        """
        Override QWidget::closeEvent. Called when the drill view is closed.
        Close all QDialog child.

        Args:
            event (QCloseEvent): the close event
        """
        if self.isWindowModified():
            self._saveDataQuestion()
        children = self.findChildren(QDialog)
        for child in children:
            child.close()
        super(DrillView, self).closeEvent(event)

    ###########################################################################
    # actions                                                                 #
    ###########################################################################

    def _saveDataQuestion(self):
        """
        Opens a popup message asking if the user wants to save its data.
        """
        q = QMessageBox.question(self, "DrILL: Unsaved data", "You have "
                                 + "unsaved modifications, do you want to save "
                                 + "them before?")
        if (q == QMessageBox.Yes):
            self.saveRundex()

    def _changeInstrument(self, instrument):
        """
        Triggered when the instrument selection changed.

        Args:
            instrument(str): instrument name
        """
        if self.isHidden():
            return
        if self.isWindowModified():
            self._saveDataQuestion()
        self.instrumentChanged.emit(instrument)

    def _changeAcquisitionMode(self, acquisitionMode):
        """
        Triggered when the acquisition mode is changed.

        Args:
            acquisitionMode(str): acquisition mode name
        """
        if self.isWindowModified():
            self._saveDataQuestion()
        self.acquisitionModeChanged.emit(acquisitionMode)

    def _changeCycleOrExperiment(self):
        """
        Triggered when editing cycle number or experiment ID field.
        """
        cycle = self.cycleNumber.text()
        exp = self.experimentId.text()
        if (cycle and exp):
            self.cycleAndExperimentChanged.emit(cycle, exp)
        self.setWindowModified(True)

    def _getSelectionShape(self, selection):
        """
        Get the shape of the selection, the number of rows and the number of
        columns.

        Args:
            selection (list(tuple(int, int))): list of selected cells indexes

        Returns:
            tuple(int, int): selection shape (n_rows, n_col), (0, 0) if the
                             selection is empty or discontinuous
        """
        if not selection:
            return (0, 0)
        rmin = selection[0][0]
        rmax = rmin
        cmin = selection[0][1]
        cmax = cmin
        for item in selection:
            if item[0] > rmax:
                rmax = item[0]
            if item[1] > cmax:
                cmax = item[1]
        shape = (rmax - rmin + 1, cmax - cmin + 1)
        if shape[0] * shape[1] != len(selection):
            return (0, 0)
        else:
            return shape

    def copySelectedCells(self):
        """
        Copy in the local buffer the content of the selected cells. The
        selection has to be valid, otherwise the buffer is not modified.
        If the selection concerns only empty cells, the buffer is not modified.
        """
        cells = self.table.getSelectedCells()
        if not cells:
            return
        cells.sort()
        shape = self._getSelectionShape(cells)
        if shape == (0, 0):
            QMessageBox.warning(self, "Selection error",
                                "Please select adjacent cells")
            return
        tmpBufferShape = shape
        tmpBuffer = list()
        empty = True
        for cell in cells:
            contents = self.table.getCellContents(cell[0], cell[1])
            if contents:
                empty = False
            tmpBuffer.append(contents)
        # avoid filling the buffer with *only* empty values
        if not empty:
            self.buffer = tmpBuffer
            self.bufferShape = tmpBufferShape

    def cutSelectedCells(self):
        """
        Copy in the local buffer the content of the selected cells and empty
        them. If the selection is not valid or concerns only empty cells, the
        buffer is not modified.
        """
        self.copySelectedCells()
        self.eraseSelectedCells()

    def pasteCells(self):
        """
        Paste the buffer in selected cells. The shape of the selected cells has
        to match the buffer shape except if:
        - the buffer contains only one cell. In that case, its contents will be
          pasted in every selected cells
        - the buffer contains only one row and the selection has the same number
          of columns. In that case, the row will be repeated in the selection.
        """
        if not self.buffer:
            return
        cells = self.table.getSelectedCells()
        if not cells:
            QMessageBox.warning(self, "Paste error", "No cell selected")
            return
        cells.sort()
        shape = self._getSelectionShape(cells)
        if self.bufferShape == (1, 1) and shape != (0, 0):
            for cell in cells:
                self.table.setCellContents(cell[0], cell[1], self.buffer[0])
        elif shape == self.bufferShape and shape != (0, 0):
            for i in range(len(cells)):
                self.table.setCellContents(cells[i][0], cells[i][1],
                                           self.buffer[i])
        elif ((self.bufferShape[0] == 1) and (shape[1] == self.bufferShape[1])
                and (shape != (0, 0))):
            for i in range(len(cells)):
                self.table.setCellContents(cells[i][0], cells[i][1],
                                           self.buffer[i % shape[1]])
        elif self.buffer and shape != self.bufferShape and shape != (0, 0):
            QMessageBox.warning(self, "Paste error",
                                "The selection does not correspond to the "
                                + "clipboard contents ("
                                + str(self.bufferShape[0]) + " rows * "
                                + str(self.bufferShape[1]) + " columns)")

    def eraseSelectedCells(self):
        """
        Erase the contents of the selected cells.
        """
        indexes = self.table.getSelectedCells()
        for (r, c) in indexes:
            self.table.eraseCell(r, c)

    def add_row_after(self):
        """
        Add row(s) after the selected ones. If no row selected, the row(s)
        is(are) added at the end of the table. The number of row to add is
        taken from the ui spinbox.
        """
        position = self.table.getLastSelectedRow()
        if position == -1:
            position = self.table.getLastRow()
        n = self.nrows.value()
        for i in range(n):
            self.table.addRow(position + 1)
            self.rowAdded.emit(position + 1)
            position += 1
            self.setWindowModified(True)

    def del_selected_rows(self):
        """
        Delete the selected rows.
        """
        rows = self.table.getSelectedRows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.table.deleteRow(row)
            self.rowDeleted.emit(row)
            self.setWindowModified(True)

    def process_selected_rows(self):
        """
        Ask for the processing of the selected rows. If the selected rows
        contain invalid values, this function display an error message dialog.
        """
        rows = self.table.getSelectedRows()
        if not rows:
            rows = self.table.getRowsFromSelectedCells()
        if rows:
            for cell in self.invalidCells:
                if cell[0] in rows:
                    QMessageBox.warning(self, "Error", "Please check the "
                                        + "parameters value before processing")
                    return
            self.errorRows = set()
            self.process.emit(rows)

    def process_all_rows(self):
        """
        Ask for the processing of all the rows. If the rows contain invalid
        values, this function display an error message dialog.
        """
        rows = self.table.getAllRows()
        if rows:
            for cell in self.invalidCells:
                if cell[0] in rows:
                    QMessageBox.warning(self, "Error", "Please check the "
                                        + "parameters value before processing")
                    return
            self.errorRows = set()
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
        self.rundexLoaded.emit(filename[0])
        self.setWindowModified(False)

    def saveRundexAs(self):
        """
        Ask for the saving of the table in a rundex file.
        """
        filename = QFileDialog.getSaveFileName(
                self, 'Save rundex', './*.mrd',
                "Rundex (*.mrd);;All files (*.*)"
                )
        if not filename[0]:
            return
        self.rundexSaved.emit(filename[0])
        self.setWindowModified(False)
        self.rundexFile = filename[0]

    def saveRundex(self):
        """
        Save in the current rundex file. The current file is the one which has
        been used to import or export the current data.
        """
        if self.rundexFile:
            self.rundexSaved.emit(self.rundexFile)
            self.setWindowModified(False)
        else:
            self.saveRundexAs()

    def helpWindow(self):
        """
        Popup the help window.
        """
        InterfaceManager().showHelpPage(
                "qthelp://org.mantidproject/doc/interfaces/DrILL.html")

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
            self.copySelectedCells()
        elif (event.key() == Qt.Key_X
                and event.modifiers() == Qt.ControlModifier):
            self.cutSelectedCells()
        elif (event.key() == Qt.Key_V
                and event.modifiers() == Qt.ControlModifier):
            self.pasteCells()
        elif (event.key() == Qt.Key_Delete):
            self.eraseSelectedCells()

    def show_directory_manager(self):
        """
        Open the Mantid user directories manager.
        """
        manageuserdirectories.ManageUserDirectories(self).exec_()

    def on_cell_changed(self, row, column):
        if row in self.coloredRows:
            self.table.removeRowBackground(row)
            self.coloredRows.remove(row)

        self.dataChanged.emit(row, column,
                              self.table.getCellContents(row, column))
        self.setWindowModified(True)

    ###########################################################################
    # for model calls                                                         #
    ###########################################################################

    def set_available_modes(self, modes):
        """
        Set the available acquisition modes in the comboxbox.

        Args:
            modes (list(str)): list of acquisition modes
        """
        self.modeSelector.blockSignals(True)
        self.modeSelector.clear()
        self.modeSelector.addItems(modes)
        self.modeSelector.blockSignals(False)

    def set_acquisition_mode(self, mode):
        """
        Set the current acquisition mode in the combobox.

        Args:
            mode (str): acquisition mode
        """
        self.modeSelector.blockSignals(True)
        self.modeSelector.setCurrentText(mode)
        self.modeSelector.blockSignals(False)

    def setCycleAndExperiment(self, cycle, experiment):
        """
        Set the cycle number and the experiment ID.

        Args:
            cycle (str): cycle number
            experiment (str): experiment ID
        """
        self.cycleNumber.setText(cycle)
        self.experimentId.setText(experiment)
        self._changeCycleOrExperiment()

    def set_table(self, columns, tooltips=None):
        """
        Set the table header.

        Args:
            columns (list(str)): list of columns titles
        """
        self.columns = columns
        self.table.clear()
        self.invalidCells = set()
        self.coloredRows = set()
        self.table.setRowCount(0)
        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)
        if tooltips:
            self.table.setColumnHeaderToolTips(tooltips)
        self.table.resizeColumnsToContents()
        self.setWindowModified(False)

    def fill_table(self, rows_contents):
        """
        Fill the table.

        Args:
            rows_contents (list(list(str))): list of rows contents
        """
        if (not self.table.columnCount()):
            return
        if rows_contents:
            self.blockSignals(True)
            self.table.setRowCount(len(rows_contents))
            for row in range(len(rows_contents)):
                self.table.setRowContents(row, rows_contents[row])
            self.blockSignals(False)
        else:
            self.table.addRow(0)
            self.rowAdded.emit(0)

    def setRundexFile(self, filename):
        """
        Set the current rundex filename.

        Args:
            filename: rundex file name.
        """
        self.rundexFile = filename

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
        self.modeSelector.setDisabled(state)
        self.cycleNumber.setDisabled(state)
        self.experimentId.setDisabled(state)
        self.datadirs.setDisabled(state)
        self.load.setDisabled(state)
        self.settings.setDisabled(state)
        self.paste.setDisabled(state)
        self.copy.setDisabled(state)
        self.cut.setDisabled(state)
        self.erase.setDisabled(state)
        self.deleterow.setDisabled(state)
        self.nrows.setDisabled(state)
        self.addrow.setDisabled(state)
        self.save.setDisabled(state)
        self.increment.setDisabled(state)
        self.fill.setDisabled(state)
        self.help.setDisabled(state)
        self.processRows.setDisabled(state)
        self.processAll.setDisabled(state)
        self.stop.setDisabled(not state)
        self.table.setDisabled(state)
        self.table.clearSelection()
        if state:
            self.table.setCursor(Qt.WaitCursor)
        else:
            self.table.setCursor(Qt.ArrowCursor)

    def set_row_processing(self, row):
        """
        Set a row as currently processing.

        Args:
            row (int): the row index
        """
        if row not in self.coloredRows:
            self.coloredRows.add(row)
        self.table.setRowBackground(row, self.PROCESSING_COLOR)

    def set_row_done(self, row):
        """
        Set a row as done with success.

        Args:
            row (int): the row index
        """
        if row not in self.coloredRows:
            self.coloredRows.add(row)
        self.table.setRowBackground(row, self.OK_COLOR)

    def set_row_error(self, row):
        """
        Set a row as done with error.

        Args:
            row (int): the row index
        """
        if row not in self.coloredRows:
            self.coloredRows.add(row)
        if row not in self.errorRows:
            self.errorRows.add(row)
        self.table.setRowBackground(row, self.ERROR_COLOR)

    def set_cell_ok(self, row, columnTitle):
        """
        Set a cell as OK. Remove it from the invalid cells set, change its
        color and remove the tooltip itf it exists.

        Args:
            row (int): row index
            columnTile (str): column header
        """
        if ((row < 0) or (row >= self.table.rowCount())
                or (columnTitle not in self.columns)):
            return
        column = self.columns.index(columnTitle)
        self.table.removeCellBackground(row, column)
        self.table.setCellToolTip(row, column, "")
        self.invalidCells.discard((row, column))

    def set_cell_error(self, row, columnTitle, msg):
        """
        Set a cell a containing an invalid value. Change its colors, add a
        tooltip containing the provided message and add it to the set of
        invalid cells.

        Args:
            row (int): row index
            columnTitle (str): column header
            msg (str): the error message
        """
        if ((row < 0) or (row >= self.table.rowCount())
                or (columnTitle not in self.columns)):
            return
        column = self.columns.index(columnTitle)
        self.table.setCellBackground(row, column, self.ERROR_COLOR)
        self.table.setCellToolTip(row, column, msg)
        self.invalidCells.add((row, column))

    def setVisualSettings(self, visualSettings):
        """
        Deal with the visual settings that the model saved.

        Args:
            visualSettings (dict): dictionnary containing some visual
                                   parameters that the view can deal with
        """
        # folding state
        if ("FoldedColumns" in visualSettings):
            f = list()
            for c in self.columns:
                if (c not in visualSettings["FoldedColumns"]):
                    f.append(False)
                    continue
                f.append(visualSettings["FoldedColumns"][c])
            self.table.setHeaderFoldingState(f)

    def getVisualSettings(self):
        """
        Used by the model to get some visualisation data that should be saved
        in rundex file. This data will be them handled by the setVisualSettings
        associated method.

        Returns:
            dict: visual settings dictionnay
        """
        vs = dict()
        # folding state
        vs["FoldedColumns"] = dict()
        f = self.table.getHeaderFoldingState()
        for i in range(len(self.columns)):
            if f[i]:
                vs["FoldedColumns"][self.columns[i]] = f[i]

        return vs

    def errorPopup(self, title, msg, details=None):
        """
        Display an error popup to inform the user.

        Args:
            title (str): popup title
            msg (str): popup message
            details (str): facultative detailed text
        """
        w = QMessageBox(QMessageBox.Critical, title, msg, QMessageBox.Ok, self)
        if details:
            w.setDetailedText(details)
        w.exec()

    def displayProcessingReport(self):
        """
        Display a popup listing row(s) that have been pushed to the errorRows
        set.
        """
        if self.errorRows:
            names = [str(r + 1) for r in self.errorRows]
            rows = ", ".join(names)
            self.errorPopup("Processing error(s)",
                            "Check logs for processing errors concerning rows: "
                            + rows)
