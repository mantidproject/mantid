# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import re
import os

from qtpy.QtWidgets import QFileDialog, QMessageBox

from ..model.DrillModel import DrillModel
from ..model.configurations import RundexSettings
from .DrillExportPresenter import DrillExportPresenter
from .DrillContextMenuPresenter import DrillContextMenuPresenter
from .DrillSamplePresenter import DrillSamplePresenter
from .DrillSettingsPresenter import DrillSettingsPresenter


class DrillPresenter:

    """
    Set of invalid cells. This is used to avoid the submission of invalid
    parameters for processing.
    """
    _invalidCells = set()

    """
    Set of custom options. Used to keep an history of the previous values.
    """
    _customOptions = set()

    def __init__(self, view):
        """
        Initialize the presenter by giving a view and a model. This method
        connects all the view and model signals.

        Args:
            model (DrillModel): the model
            view (DrillView): the view
        """
        self.model = DrillModel()
        self.view = view
        self.view.setWindowTitle("Untitled [*]")
        self._invalidCells = set()
        self._customOptions = set()

        # view signals connection
        self.view.instrumentChanged.connect(self.instrumentChanged)
        self.view.acquisitionModeChanged.connect(self.acquisitionModeChanged)
        self.view.cycleAndExperimentChanged.connect(
                self.model.setCycleAndExperiment)
        self.view.rowAdded.connect(self.onRowAdded)
        self.view.rowDeleted.connect(self.model.deleteSample)
        self.view.groupSelectedRows.connect(self.onGroupSelectedRows)
        self.view.ungroupSelectedRows.connect(self.onUngroupSelectedRows)
        self.view.setMasterRow.connect(self.onSetMasterRow)
        self.view.process.connect(self.onProcess)
        self.view.processGroup.connect(self.onProcessGroup)
        self.view.processAll.connect(self.onProcessAll)
        self.view.processStopped.connect(self.stopProcessing)
        self.view.newTable.connect(self.onNew)
        self.view.loadRundex.connect(self.onLoad)
        self.view.saveRundex.connect(self.onSave)
        self.view.saveRundexAs.connect(self.onSaveAs)
        self.view.showSettings.connect(self.settingsWindow)
        self.view.automaticFilling.connect(self.onAutomaticFilling)

        # model signals connection
        self.model.progressUpdate.connect(
                lambda progress: self.view.set_progress(progress, 100)
                )
        self.model.processingDone.connect(self.onProcessingDone)
        self.model.newSample.connect(self.onNewSample)

        self._syncViewHeader()
        self._resetTable()
        self.model.addSample(0)

    def onRowAdded(self, position):
        """
        Triggered when a row is added to the table. This method add a sample to
        the model.

        Args:
            position (int): index of the new row
        """
        self.model.addSample(position)

    def onNewSample(self, sample):
        """
        Triggered when the model get a new sample.

        Args:
            sample (DrillSample): the new sample
        """
        self.view.table.addRow(sample.getIndex())
        DrillSamplePresenter(self.view.table, sample)

    def onAutomaticFilling(self):
        """
        Copy (and increment) the contents of the first selected cell in the
        other ones. The incremente value is found in the ui spinbox associated
        with this action. If a single row is selected, the increment will be
        propagated along that row. Otherwise, the increment is propagated along
        columns.
        """
        def inc(value, i):
            """
            Increment the value depending on its content. This function can
            increment numbers, numors (sum, range), names that end with a number
            and comma seprarated list of all these types.
            Some examples:
            ("1000", 1)               -> "1001"
            ("10,100,1000", 1)        -> "11,101,1001"
            ("10+20,100", 2)          -> "12+22,102"
            ("Sample_1,Sample10", 2)  -> "Sample_3,Sample12"
            ("Sample,sample10", 1)    -> "Sample,sample11"

            Args:
                value (str): a string to increment
                i (int): value of the increment

            Returns:
                str: A string that represents the incremented input value
            """
            if i == 0:
                return value
            if ',' in value:
                return ','.join([inc(e, i) for e in value.split(',')])
            if '+' in value:
                return '+'.join([inc(e, i) for e in value.split('+')])
            if ':' in value:
                l = value.split(':')
                try:
                    l = [int(e) for e in l]
                except:
                    return value
                if len(l) == 2:
                    if i > 0:
                        return str(l[1] + i) + ':' + str(l[1] + (l[1] - l[0]) + i)
                    else:
                        return str(l[0] - (l[1] - l[0]) + i) + ':' + str(l[0] + i)
                if len(l) == 3:
                    return inc(str(l[0]) + ':' + str(l[1]), i) + ':' + str(l[2])
                else:
                    return value

            if re.match(r"^-{,1}\d+$", value):
                if int(value) == 0:
                    return value
                if int(value) + i == 0:
                    return value
                return str(int(value) + i)
            suffix = re.search(r"\d+$", value)
            if suffix:
                n = suffix.group(0)
                ni = int(n) + i
                if ni < 0:
                    ni = 0
                return value[0:-len(n)] + str(ni)

            return value

        increment = self.view.increment.value()
        cells = self.view.table.getSelectedCells()
        # check if increment should append along columns
        columnIncrement = (len(self.view.table.getRowsFromSelectedCells()) > 1)
        if not cells:
            return
        # increment or copy the content of the previous cell
        for i in range(1, len(cells)):
            # if we increment along columns and this is a new column
            if columnIncrement and cells[i][1] != cells[i-1][1]:
                continue
            contents = self.view.table.getCellContents(cells[i-1][0],
                                                       cells[i-1][1])
            self.view.table.setCellContents(cells[i][0], cells[i][1],
                                            inc(contents, increment))

    def onGroupSelectedRows(self):
        """
        Triggered when the view request the creation of a group.
        """
        rows = self.view.table.getRowsFromSelectedCells()
        self.model.groupSamples(rows)

    def onUngroupSelectedRows(self):
        """
        Triggered when the view request the removing of row from their group(s).
        """
        rows = self.view.table.getRowsFromSelectedCells()
        self.model.ungroupSamples(rows)

    def onSetMasterRow(self):
        """
        Triggered when a master row is set for a group.
        """
        rows = self.view.table.getRowsFromSelectedCells()
        if len(rows) != 1:
            return
        row = rows[0]
        self.model.setGroupMaster(row)

    def onProcess(self):
        """
        Handles the processing of selected rows.
        """
        rows = self.view.getSelectedRows()
        if not rows:
            rows = self.view.getAllRows()
        self._process(rows)

    def onProcessGroup(self):
        """
        Handles the processing of selected groups.
        """
        groups = self.model.getSamplesGroups()
        selectedRows = self.view.getSelectedRows()
        rows = set()
        for row in selectedRows:
            for group in groups:
                if row in groups[group]:
                    rows.update(groups[group])
        self._process(rows)

    def onProcessAll(self):
        """
        Handles the processing of all rows.
        """
        rows = self.view.getAllRows()
        self._process(rows)

    def _process(self, rows):
        """
        Submit the processing.

        Args:
            rows (set(int)): row indexes
        """
        if not rows:
            return
        for cell in self._invalidCells:
            if cell[0] in rows:
                QMessageBox.warning(self.view, "Error", "Please check the "
                                    "parameters value before processing.")
                return
        self.view.set_disabled(True)
        self.view.set_progress(0, 100)
        self.model.process(rows)

    def stopProcessing(self):
        """
        Stop the current processing.
        """
        self.model.stopProcess()
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)

    def onProcessingDone(self):
        """
        Forward the processing done signal to the view.
        """
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)

    def instrumentChanged(self, instrument):
        """
        Forward the instrument changes to the model and update the view.

        Args:
            instrument (str): instrument name
        """
        if self.view.isWindowModified():
            self._saveDataQuestion()

        self.model.setInstrument(instrument)
        self.model.resetIOFile()
        self.view.setWindowTitle("Untitled [*]")
        self._syncViewHeader()
        self._resetTable()
        self.model.addSample(0)

    def acquisitionModeChanged(self, mode):
        """
        Forward the acquisition mode changes to the model and update the view.

        Args:
            mode (str): acquisition mode name
        """
        if self.view.isWindowModified():
            self._saveDataQuestion()

        self.model.setAcquisitionMode(mode)
        self.model.resetIOFile()
        self.view.setWindowTitle("Untitled [*]")
        self._syncViewHeader()
        self._resetTable()
        self.model.addSample(0)

    def onLoad(self):
        """
        Triggered when the user want to load a file. This methods start a
        QDialog to get the file path from the user.
        """
        filename = QFileDialog.getOpenFileName(self.view, 'Load rundex', '.',
                                               "Rundex (*.mrd);;All (*)")
        if not filename[0]:
            return
        self._resetTable()
        self.model.setIOFile(filename[0])
        self.view.setWindowTitle(os.path.split(filename[0])[1] + " [*]")
        self.model.importRundexData()
        self._syncViewHeader()

    def onSave(self):
        """
        Triggered when the user wants to save its data. This method starts a
        QDialog only if no file was previously used to load or save.
        """
        if self.model.getIOFile():
            self.model.setVisualSettings(self.view.getVisualSettings())
            self.model.exportRundexData()
            self.view.setWindowModified(False)
        else:
            self.onSaveAs()

    def onSaveAs(self):
        """
        Triggered when the user selects the "save as" function. This methods
        will open a dialog to select the file even if one has previously been
        used.
        """
        filename = QFileDialog.getSaveFileName(self.view, 'Save rundex',
                                               './*.mrd',
                                               "Rundex (*.mrd);;All (*)")
        if not filename[0]:
            return
        self.model.setIOFile(filename[0])
        self.view.setWindowTitle(os.path.split(filename[0])[1] + " [*]")
        self.model.setVisualSettings(self.view.getVisualSettings())
        self.model.exportRundexData()
        self.view.setWindowModified(False)

    def settingsWindow(self):
        """
        Show the setting window. This function creates a special dialog that
        generates automatically its fields on the basis of settings types. It
        also connects the differents signals to get validation of user inputs.
        """
        parameters = self.model.getParameters()
        presenter = DrillSettingsPresenter(self.view, parameters)

    def onShowExportDialog(self, dialog):
        exportModel = self.model.getExportModel()
        DrillExportPresenter(dialog, exportModel)

    def onShowContextMenu(self, menu):
        """
        Triggered when the user ask the context menu (right click).

        Args:
            menu (DrillContextMenu): view of the context menu
        """
        DrillContextMenuPresenter(self.view, self.model, menu)

    def onClose(self):
        """
        Triggered when the view is closed.
        """
        if self.view.isWindowModified():
            self._saveDataQuestion()

    def onNew(self):
        """
        Triggered when the user wants an empty table.
        """
        if self.view.isWindowModified():
            self._saveDataQuestion()
        self.model.clear()
        self.model.resetIOFile()
        self.view.setWindowTitle("Untitled [*]")
        self._syncViewHeader()
        self._resetTable()

    def _saveDataQuestion(self):
        """
        Open a dialog to ask the user if he wants to save the rundex file.
        """
        if self.view.isHidden():
            return
        q = QMessageBox.question(self.view, "DrILL: Unsaved data", "You have "
                                 "unsaved modifications, do you want to save "
                                 "them before?",
                                 QMessageBox.Yes | QMessageBox.No)
        if (q == QMessageBox.Yes):
            self.onSaveAs()

    def _syncViewHeader(self):
        availableModes = self.model.getAvailableAcquisitionModes()
        acquisitionMode = self.model.getAcquisitionMode()
        cycle, exp = self.model.getCycleAndExperiment()

        self.view.set_available_modes(availableModes)
        self.view.set_acquisition_mode(acquisitionMode)
        self.view.setCycleAndExperiment(cycle, exp)

    def _resetTable(self):
        """
        Reset the table header.
        """
        parameters = self.model.getParameters()
        acquisitionMode = self.model.getAcquisitionMode()
        columns = RundexSettings.COLUMNS[acquisitionMode]
        tooltips = list()
        for name in columns:
            for p in parameters:
                if p.getName() == name:
                    tooltips.append(p.getDocumentation())
        self.view.set_table(columns, tooltips)
