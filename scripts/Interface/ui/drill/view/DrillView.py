# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os

from qtpy.QtWidgets import QMainWindow, QMessageBox, QDialog, QMenu, QAction
from qtpy.QtCore import *
from qtpy import uic

from mantidqt.widgets import manageuserdirectories, instrumentselector
from mantid.kernel import config  # noqa
from mantidqt import icons
from mantidqt.interfacemanager import InterfaceManager

from .DrillExportDialog import DrillExportDialog
from ..presenter.DrillPresenter import DrillPresenter
from .DrillTableWidget import DrillTableWidget
from .DrillContextMenu import DrillContextMenu


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
        str: column name
    """
    dataChanged = Signal(int, str)

    """
    Sent when a new group is requested.
    """
    groupSelectedRows = Signal()

    """
    Sent when the removing of row(s) from their group is requested.
    """
    ungroupSelectedRows = Signal()

    """
    Sent when a row is set as master row.
    """
    setMasterRow = Signal()

    """
    Sent when the user asks to process the selected row(s).
    """
    process = Signal()

    """
    Sent when the user asks to process all rows.
    """
    processAll = Signal()

    """
    Sent when the user asks to process selected groups.
    """
    processGroup = Signal()

    """
    Sent when the user wants to stop the current processing.
    """
    processStopped = Signal()

    """
    Sent when the user asks for an empty table.
    """
    newTable = Signal()

    """
    Sent when the user asks for data loading from file.
    """
    loadRundex = Signal()

    """
    Sent when the user aks for data saving.
    """
    saveRundex = Signal()

    """
    Sent when the user aks for data saving in a new file.
    """
    saveRundexAs = Signal()

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

        self._presenter = DrillPresenter(self)

    def setup_header(self):
        """
        Setup the window header. Set the buttons icons and connect the signals.
        """
        self.actionNew.triggered.connect(self.newTable.emit)
        self.actionLoadRundex.triggered.connect(self.loadRundex.emit)
        self.actionSaveAs.triggered.connect(self.saveRundexAs.emit)
        self.actionSave.triggered.connect(self.saveRundex.emit)
        self.actionManageDirectories.triggered.connect(self.show_directory_manager)
        self.actionSettings.triggered.connect(self.showSettings.emit)
        self.actionClose.triggered.connect(self.close)
        self.actionAddRow.triggered.connect(self.add_row_after)
        self.actionDelRow.triggered.connect(self.del_selected_rows)
        self.actionCopyRow.triggered.connect(self.copySelectedCells)
        self.actionCutRow.triggered.connect(self.cutSelectedCells)
        self.actionPasteRow.triggered.connect(self.pasteCells)
        self.actionErase.triggered.connect(self.eraseSelectedCells)
        self.actionProcessRow.triggered.connect(self.process.emit)
        self.actionProcessGroup.triggered.connect(self.processGroup.emit)
        self.actionProcessAll.triggered.connect(self.processAll.emit)
        self.actionStopProcessing.triggered.connect(self.processStopped.emit)
        self.actionHelp.triggered.connect(self.helpWindow)

        self.instrumentselector = instrumentselector.InstrumentSelector(self)
        self.instrumentselector.setToolTip("Instrument")
        self.toolbar.insertWidget(0, self.instrumentselector, 0, Qt.AlignLeft)
        self.instrumentselector.instrumentSelectionChanged.connect(
                self.instrumentChanged.emit)

        self.modeSelector.currentTextChanged.connect(
                self.acquisitionModeChanged.emit)

        self.cycleNumber.editingFinished.connect(self._changeCycleOrExperiment)
        self.experimentId.editingFinished.connect(self._changeCycleOrExperiment)

        self.datadirs.setIcon(icons.get_icon("mdi.folder"))
        self.datadirs.clicked.connect(self.show_directory_manager)

        self.load.setIcon(icons.get_icon("mdi.file-import"))
        self.load.clicked.connect(self.loadRundex.emit)

        self.settings.setIcon(icons.get_icon("mdi.settings"))
        self.settings.clicked.connect(self.showSettings.emit)

        self.export.setIcon(icons.get_icon("mdi.application-export"))
        self.export.clicked.connect(self.showExportDialog)

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
        self.save.clicked.connect(self.saveRundex.emit)

        self.help.setIcon(icons.get_icon("mdi.help"))
        self.help.clicked.connect(self.helpWindow)

        self.fill.setIcon(icons.get_icon("mdi.arrow-expand-down"))
        self.fill.clicked.connect(self.automatic_filling)

        self.processRows.setIcon(icons.get_icon("mdi.play"))
        self.processRows.clicked.connect(self.process.emit)

        self.buttonProcessGroup.setIcon(icons.get_icon("mdi.skip-forward"))
        self.buttonProcessGroup.clicked.connect(self.processGroup.emit)

        self.buttonProcessAll.setIcon(icons.get_icon("mdi.fast-forward"))
        self.buttonProcessAll.clicked.connect(self.processAll.emit)

        self.stop.setIcon(icons.get_icon("mdi.stop"))
        self.stop.clicked.connect(self.processStopped.emit)

    def setup_table(self):
        """
        Setup the main table widget.
        """
        self.table.cellChanged.connect(
                lambda r,c: self.dataChanged.emit(r, self.columns[c]))
        self.table.setContextMenuPolicy(Qt.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.showContextMenu)

    def closeEvent(self, event):
        """
        Override QWidget::closeEvent. Called when the drill view is closed.
        Close all QDialog child.

        Args:
            event (QCloseEvent): the close event
        """
        children = self.findChildren(QDialog)
        for child in children:
            child.close()
        self._presenter.onClose()
        super(DrillView, self).closeEvent(event)

    ###########################################################################
    # actions                                                                 #
    ###########################################################################

    def _changeCycleOrExperiment(self):
        """
        Triggered when editing cycle number or experiment ID field.
        """
        cycle = self.cycleNumber.text()
        exp = self.experimentId.text()
        if (cycle and exp):
            self.cycleAndExperimentChanged.emit(cycle, exp)
        self.setWindowModified(True)

    def copySelectedCells(self):
        """
        Copy in the local buffer the content of the selected cells. The
        selection has to be valid, otherwise the buffer is not modified.
        If the selection concerns only empty cells, the buffer is not modified.
        """
        cells = self.table.getSelectedCells()
        if not cells:
            return
        shape = self.table.getSelectionShape()
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
        shape = self.table.getSelectionShape()
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
                                           self.buffer[int(i / shape[0])])
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

    def updateLabelsFromGroups(self, groups, masters):
        """
        Update all the row labels from the current groups.

        Args:
            groups (dict(str:set(int))): group name and rows
            master (dict(str:int)): group name and master row
        """
        for row in range(self.table.rowCount()):
            self.table.delRowLabel(row)
        for groupName,rows in groups.items():
            rowName = 1
            for row in sorted(rows):
                if groupName in masters and masters[groupName] == row:
                    _bold = True
                    _tooltip = "This is the master row of the group {}" \
                               .format(groupName)
                else:
                    _bold = False
                    _tooltip = "This row belongs to the sample group {}" \
                               .format(groupName)
                self.table.setRowLabel(row, groupName + str(rowName),
                                       _bold, _tooltip)
                rowName += 1

    def getRowLabel(self, row):
        """
        Get the visual label of a row.

        Args:
            row(int): row index

        Returns:
            str: row label
        """
        return self.table.getRowLabel(row)

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
        associated with this action. If a single row is selected, the increment
        will be propagated along that row. Otherwise, the increment is
        propagated along columns.
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
        # check if increment should append along columns
        columnIncrement = (len(self.table.getRowsFromSelectedCells()) > 1)
        if not cells:
            return
        # increment or copy the content of the previous cell
        for i in range(1, len(cells)):
            # if we increment along columns and this is a new column
            if columnIncrement and cells[i][1] != cells[i-1][1]:
                continue
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
        elif (event.key() == Qt.Key_G
                and event.modifiers() == Qt.ControlModifier):
            self.groupSelectedRows.emit()
        elif (event.key() == Qt.Key_G
                and event.modifiers() == Qt.ControlModifier | Qt.ShiftModifier):
            self.ungroupSelectedRows.emit()
        elif (event.key() == Qt.Key_M
                and event.modifiers() == Qt.ControlModifier):
            self.setMasterRow.emit()

    def show_directory_manager(self):
        """
        Open the Mantid user directories manager.
        """
        manageuserdirectories.ManageUserDirectories(self).exec_()

    def showContextMenu(self, pos):
        """
        Display the context menu.

        Args:
            pos (QPoint): mouse position in the widget frame
        """
        menu = DrillContextMenu(self.table.viewport().mapToGlobal(pos), self)
        self._presenter.onShowContextMenu(menu)

    def showExportDialog(self):
        """
        Open the export dialog.
        """
        dialog = DrillExportDialog(self)
        self._presenter.onShowExportDialog(dialog)
        dialog.show()

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
        self.table.setRowCount(0)
        self.table.setColumnCount(0)
        self.table.horizontalHeader().reset()
        self.table.setColumnCount(len(columns))
        self.table.setHorizontalHeaderLabels(columns)
        if tooltips:
            self.table.setColumnHeaderToolTips(tooltips)
        for i in range(len(columns)):
            self.table.setColumnHidden(i, False)
        self.menuAddRemoveColumn.aboutToShow.connect(
                lambda : self.setAddRemoveColumnMenu(columns))
        self.table.resizeColumnsToContents()
        self.setWindowModified(False)

    def getSelectedRows(self):
        """
        Get the list of selected row indexes. If the user did not select any
        full row, the selected rows are extracted from the selected cells.

        Returns:
            list(int): row indexes
        """
        rows = self.table.getSelectedRows()
        if not rows:
            rows = self.table.getRowsFromSelectedCells()
        return rows

    def getAllRows(self):
        """
        Get the list of all row indexes.

        Returns:
            list(int): row indexes
        """
        return self.table.getAllRows()

    def getCellContents(self, row, column):
        """
        Get the contents of a specific cell.

        Args:
            row (int): row index
            column (str): column name
        """
        return self.table.getCellContents(row, self.columns.index(column))

    def setAddRemoveColumnMenu(self, columns):
        """
        Fill the "add/remove column" menu. This function is triggered each time
        the menu is displayed to display a correct icon depending on the status
        of the column (hidden or not).

        Args:
            columns (list(str)): list of column titles
        """
        if self.menuAddRemoveColumn.receivers(QMenu.triggered):
            self.menuAddRemoveColumn.triggered.disconnect()
        self.menuAddRemoveColumn.clear()
        hidden = self.table.getHiddenColumns()
        for c in columns:
            action = QAction(c, self.menuAddRemoveColumn)
            if c in hidden:
                action.setIcon(icons.get_icon("mdi.close"))
            else:
                action.setIcon(icons.get_icon("mdi.check"))
            self.menuAddRemoveColumn.addAction(action)

        self.menuAddRemoveColumn.triggered.connect(
                lambda action: self.table.toggleColumnVisibility(action.text()))

    def setCellContents(self, row, column, value):
        """
        Set the contents of a specific cell.

        Args:
            row (int): row index
            column (str): column name
            value (str): column contents
        """
        if row >= self.table.rowCount() or column not in self.columns:
            return
        self.table.setCellContents(row, self.columns.index(column), value)

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
        self.export.setDisabled(state)
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
        self.buttonProcessGroup.setDisabled(state)
        self.buttonProcessAll.setDisabled(state)
        self.stop.setDisabled(not state)
        self.table.setDisabled(state)
        self.table.clearSelection()
        if state:
            self.table.setCursor(Qt.WaitCursor)
        else:
            self.table.setCursor(Qt.ArrowCursor)

    def unsetRowBackground(self, row):
        """
        Remove any background for a specific row.

        Args:
            row (int): row index
        """
        self.table.removeRowBackground(row)

    def setRowProcessing(self, row):
        """
        Set a row as currently processing.

        Args:
            row (int): the row index
        """
        self.table.setRowBackground(row, self.PROCESSING_COLOR)

    def setRowDone(self, row):
        """
        Set a row as done with success.

        Args:
            row (int): the row index
        """
        self.table.setRowBackground(row, self.OK_COLOR)

    def setRowError(self, row):
        """
        Set a row as done with error.

        Args:
            row (int): the row index
        """
        self.table.setRowBackground(row, self.ERROR_COLOR)

    def setCellOk(self, row, columnTitle):
        """
        Set a cell as OK. Change its color and remove the tooltip if it exists.

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

    def setCellError(self, row, columnTitle, msg):
        """
        Set a cell a containing an invalid value. Change its colors, add a
        tooltip containing the provided message.

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

    def setVisualSettings(self, visualSettings):
        """
        Deal with the visual settings that the model saved.

        Args:
            visualSettings (dict): dictionnary containing some visual
                                   parameters that the view can deal with
        """
        # folded columns
        if ("FoldedColumns" in visualSettings):
            if isinstance(visualSettings["FoldedColumns"], list):
                self.table.setFoldedColumns(visualSettings["FoldedColumns"])
            else:
                self.table.setFoldedColumns(
                        [c for c in visualSettings["FoldedColumns"]
                            if visualSettings["FoldedColumns"][c]]
                        )

        # hidden columns
        if ("HiddenColumns" in visualSettings):
            self.table.setHiddenColumns(visualSettings["HiddenColumns"])

        # columns order
        if ("ColumnsOrder" in visualSettings):
            self.table.setColumnsOrder(visualSettings["ColumnsOrder"])

    def getVisualSettings(self):
        """
        Used by the model to get some visualisation data that should be saved
        in rundex file. This data will be them handled by the setVisualSettings
        associated method.

        Returns:
            dict: visual settings dictionnay
        """
        vs = dict()

        # folded columns
        vs["FoldedColumns"] = self.table.getFoldedColumns()

        # hidden columns
        vs["HiddenColumns"] = self.table.getHiddenColumns()

        # columns order
        vs["ColumnsOrder"] = self.table.getColumnsOrder()

        return vs
