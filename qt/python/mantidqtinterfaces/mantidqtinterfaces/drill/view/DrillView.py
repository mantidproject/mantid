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

from .DrillExportDialog import DrillExportDialog
from ..presenter.DrillPresenter import DrillPresenter
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
    Sent to remove the master row status.
    """
    unsetMasterRow = Signal()

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

    """
    Sent when the user asks for the increment fill function.
    """
    automaticFilling = Signal()

    def __init__(self, parent=None, window_flags=None):
        super(DrillView, self).__init__(parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        if window_flags:
            self.setWindowFlags(window_flags)
        self.here = os.path.dirname(os.path.realpath(__file__))

        # help
        self.assistant_process = QProcess(self)

        # setup ui
        uic.loadUi(os.path.join(self.here, "ui/main.ui"), self)
        self.setup_header()
        self.setup_table()
        self.setFocus()

        self.buffer = list()  # for cells cut-copy-paste
        self.bufferShape = tuple()  # (n_rows, n_columns) shape of self.buffer

        self._presenter = DrillPresenter(self, self.table)

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
        self.actionAddRow.triggered.connect(self.addRowAfter)
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
        self.instrumentselector.instrumentSelectionChanged.connect(self.instrumentChanged.emit)

        self.modeSelector.currentTextChanged.connect(self.acquisitionModeChanged.emit)

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
        self.addrow.clicked.connect(self.addRowsAfter)

        self.save.setIcon(icons.get_icon("mdi.file-export"))
        self.save.clicked.connect(self.saveRundex.emit)

        self.help.setIcon(icons.get_icon("mdi.help"))
        self.help.clicked.connect(self.helpWindow)

        self.fill.setIcon(icons.get_icon("mdi.arrow-expand-down"))
        self.fill.clicked.connect(self.automaticFilling.emit)

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
        self.table.setContextMenuPolicy(Qt.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.showContextMenu)

    def closeEvent(self, event):
        """
        Override QWidget::closeEvent. Called when the drill view is closed.
        Close all QDialog child.

        Args:
            event (QCloseEvent): the close event
        """
        if self._presenter.onClose():
            self.assistant_process.close()
            self.assistant_process.waitForFinished()
            children = self.findChildren(QDialog)
            for child in children:
                child.close()
            event.accept()
        else:
            event.ignore()

    ###########################################################################
    # actions                                                                 #
    ###########################################################################

    def _changeCycleOrExperiment(self):
        """
        Triggered when editing cycle number or experiment ID field.
        """
        cycle = self.cycleNumber.text()
        exp = self.experimentId.text()
        if cycle and exp:
            self.cycleAndExperimentChanged.emit(cycle, exp)

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
            QMessageBox.warning(self, "Selection error", "Please select adjacent cells")
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
                self.table.setCellContents(cells[i][0], cells[i][1], self.buffer[i])
        elif (self.bufferShape[0] == 1) and (shape[1] == self.bufferShape[1]) and (shape != (0, 0)):
            for i in range(len(cells)):
                self.table.setCellContents(cells[i][0], cells[i][1], self.buffer[int(i / shape[0])])
        elif self.buffer and shape != self.bufferShape and shape != (0, 0):
            QMessageBox.warning(
                self,
                "Paste error",
                "The selection does not correspond to the "
                + "clipboard contents ("
                + str(self.bufferShape[0])
                + " rows * "
                + str(self.bufferShape[1])
                + " columns)",
            )

    def eraseSelectedCells(self):
        """
        Erase the contents of the selected cells.
        """
        indexes = self.table.getSelectedCells()
        for r, c in indexes:
            self.table.eraseCell(r, c)

    def addRowAfter(self):
        """
        Add a single row after the current position (or the end of the table if
        nothing is selected).
        """
        position = self.table.getLastSelectedRow()
        if position == -1:
            position = self.table.getLastRow()
        self._presenter.onRowAdded(position + 1)

    def addRowsAfter(self):
        """
        Add several rows after the current position (or the end of the table if
        nothing is selected). The number of rows to add is given by the spinbox.
        """
        position = self.table.getLastSelectedRow()
        if position == -1:
            position = self.table.getLastRow()
        n = self.nrows.value()
        for i in range(n):
            self._presenter.onRowAdded(position + 1)
            position += 1

    def del_selected_rows(self):
        """
        Delete the selected rows.
        """
        rows = self.table.getSelectedRows()
        rows = sorted(rows, reverse=True)
        for row in rows:
            self.table.deleteRow(row)
            self.rowDeleted.emit(row)

    def helpWindow(self):
        """
        Popup the help window.
        """
        from mantidqt.gui_helper import show_interface_help

        show_interface_help("DrILL", self.assistant_process, area="ILL")

    def keyPressEvent(self, event):
        """
        Deal with key pressed events.

        Args:
            event (QPressEvent): the key event
        """
        if event.key() == Qt.Key_C and event.modifiers() == Qt.ControlModifier:
            self.copySelectedCells()
        elif event.key() == Qt.Key_X and event.modifiers() == Qt.ControlModifier:
            self.cutSelectedCells()
        elif event.key() == Qt.Key_V and event.modifiers() == Qt.ControlModifier:
            self.pasteCells()
        elif event.key() == Qt.Key_Delete:
            self.eraseSelectedCells()
        elif event.key() == Qt.Key_G and event.modifiers() == Qt.ControlModifier:
            self.groupSelectedRows.emit()
        elif event.key() == Qt.Key_G and event.modifiers() == Qt.ControlModifier | Qt.ShiftModifier:
            self.ungroupSelectedRows.emit()
        elif event.key() == Qt.Key_M and event.modifiers() == Qt.ControlModifier:
            self.setMasterRow.emit()
        elif event.key() == Qt.Key_M and event.modifiers() == Qt.ControlModifier | Qt.ShiftModifier:
            self.unsetMasterRow.emit()

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
        self.setDisabled(True)
        dialog = DrillExportDialog(self)
        dialog.finished.connect(lambda: self.setDisabled(False))
        self._presenter.onShowExportDialog(dialog)
        dialog.show()

    ###########################################################################
    # for model calls                                                         #
    ###########################################################################

    def setInstrument(self, instrument):
        """
        Set the instrument in the combobox.

        Args:
            instrument (str): instrument name
        """
        self.instrumentselector.setCurrentText(instrument)

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

    def getAcquisitionMode(self):
        """
        Get the selected acquistion mode.

        Returns:
            str: acquisition mode
        """
        return self.modeSelector.currentText()

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
        self.menuAddRemoveColumn.aboutToShow.connect(lambda: self.setAddRemoveColumnMenu(columns))
        self.table.resizeColumnsToContents()

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

        self.menuAddRemoveColumn.triggered.connect(lambda action: self.table.toggleColumnVisibility(action.text()))

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

    def setVisualSettings(self, visualSettings):
        """
        Deal with the visual settings that the model saved.

        Args:
            visualSettings (dict): dictionnary containing some visual
                                   parameters that the view can deal with
        """
        # folded columns
        if "FoldedColumns" in visualSettings:
            if isinstance(visualSettings["FoldedColumns"], list):
                self.table.setFoldedColumns(visualSettings["FoldedColumns"])
            else:
                self.table.setFoldedColumns([c for c in visualSettings["FoldedColumns"] if visualSettings["FoldedColumns"][c]])

        # hidden columns
        if "HiddenColumns" in visualSettings:
            self.table.setHiddenColumns(visualSettings["HiddenColumns"])

        # columns order
        if "ColumnsOrder" in visualSettings:
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
