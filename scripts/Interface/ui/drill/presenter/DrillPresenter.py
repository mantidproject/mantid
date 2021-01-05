# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QFileDialog, QMessageBox

from ..view.DrillSettingsDialog import DrillSettingsDialog
from ..model.DrillModel import DrillModel
from .DrillContextMenuPresenter import DrillContextMenuPresenter


class DrillPresenter:

    """
    Set of invalid cells. This is used to avoid the submission of invalid
    parameters for processing.
    """
    _invalidCells = set()

    """
    Set of rows for which processing failed. This is used to display a report.
    """
    _processError = set()

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
        self._invalidCells = set()
        self._processError = set()

        # view signals connection
        self.view.instrumentChanged.connect(self.instrumentChanged)
        self.view.acquisitionModeChanged.connect(self.acquisitionModeChanged)
        self.view.cycleAndExperimentChanged.connect(
                self.model.setCycleAndExperiment)
        self.view.rowAdded.connect(self.model.addSample)
        self.view.rowDeleted.connect(self.model.deleteSample)
        self.view.dataChanged.connect(self.onDataChanged)
        self.view.groupRequested.connect(self.onGroupRequested)
        self.view.ungroupRequested.connect(self.onUngroupRequested)
        self.view.setMaster.connect(self.onMasterRowRequested)
        self.view.process.connect(self.onProcess)
        self.view.processGroup.connect(self.onProcessGroup)
        self.view.processAll.connect(self.onProcessAll)
        self.view.processStopped.connect(self.stopProcessing)
        self.view.newTable.connect(self.onNew)
        self.view.loadRundex.connect(self.onLoad)
        self.view.saveRundex.connect(self.onSave)
        self.view.saveRundexAs.connect(self.onSaveAs)
        self.view.showSettings.connect(self.settingsWindow)

        # model signals connection
        self.model.processStarted.connect(self.onProcessBegin)
        self.model.processSuccess.connect(self.onProcessSuccess)
        self.model.processError.connect(self.onProcessError)
        self.model.progressUpdate.connect(
                lambda progress: self.view.set_progress(progress, 100)
                )
        self.model.processingDone.connect(self.onProcessingDone)
        self.model.paramOk.connect(self.onParamOk)
        self.model.paramError.connect(self.onParamError)

        self.updateViewFromModel()

    def onDataChanged(self, row, column):
        """
        Triggered when the view notifies a change in the table.

        Args:
            row (int): row index
            column (int): column index
        """
        contents = self.view.getCellContents(row, column)
        if row in self._processError:
            self.view.unsetRowBackground(row)
            self._processError.remove(row)
        self.view.setWindowModified(True)
        if column == "CustomOptions":
            for option in contents.split(';'):
                if '=' not in option:
                    self.onParamError(row, column, "Bad")
                    return
                name = option.split("=")[0]
                value = option.split("=")[1]
                if value in ['true', 'True', 'TRUE']:
                    value = True
                if value in ['false', 'False', 'FALSE']:
                    value = False
                self.model.changeParameter(row, name, value)
        else:
            self.model.changeParameter(row, column, contents)

    def onGroupRequested(self, rows):
        """
        Triggered when the view request the creation of a group.

        Args:
            rows (list(int)): row indexes
        """
        group = self.model.groupSamples(rows)
        self.view.labelRowsInGroup(group, rows, None,
                                   "This row belongs to the sample group {}"
                                   .format(group),
                                   None)

    def onUngroupRequested(self, rows):
        """
        Triggered when the view request the removing of row from their group(s).

        Args:
            rows (list(int)): row indexes
        """
        groups = self.model.ungroupSamples(rows)
        self.view.labelRowsInGroup(None, rows, None)
        if groups:
            for group in groups:
                rows = self.model.getSamplesGroups()[group]
                self.view.labelRowsInGroup(group, rows, None, "This row "
                                           "belongs to the sample group {}"
                                           .format(group),
                                           None)

    def onMasterRowRequested(self, row):
        group = self.model.setGroupMaster(row)
        if group:
            rows = self.model.getSamplesGroups()[group]
            self.view.labelRowsInGroup(group, rows, row, None,
                                       "This is the master row of the group {}"
                                       .format(group))

    def onParamOk(self, row, columnName):
        """
        Triggered when a parameter is valid.

        Args:
            row (int): row index
            columnName (str): parameter name
        """
        if columnName not in self.view.columns:
            columnName = "CustomOptions"
        self._invalidCells.discard((row, columnName))
        self.view.setCellOk(row, columnName)

    def onParamError(self, row, columnName, msg):
        """
        Triggered when a parameter is invalid.

        Args:
            row (int): row index
            columnName (str): parameter name
            msg (str): error message
        """
        if columnName not in self.view.columns:
            columnName = "CustomOptions"
        self._invalidCells.add((row, columnName))
        self.view.setCellError(row, columnName, msg)

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
                QMessageBox.warning(self, "Error", "Please check the "
                                    "parameters value before processing.")
                return
        self._processError = set()
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

    def onProcessBegin(self, sample):
        """
        Triggered when the model signals that the processing of a specific
        sample started.

        Args:
            sample (int): sample index
        """
        self.view.setRowProcessing(sample)

    def onProcessError(self, sample):
        """
        Triggered when the model signals that the processing of a specific
        sample finished with an error.

        Args:
            sample (int): sample index
        """
        self._processError.add(sample)
        self.view.setRowError(sample)

    def onProcessSuccess(self, sample):
        """
        Triggered when the model signals that the processing of a specific
        sample finished with success.

        Args:
            sample (int): sample index
        """
        self.view.setRowDone(sample)

    def onProcessingDone(self):
        """
        Forward the processing done signal to the view.
        """
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)
        if self._processError:
            labels = [self.view.getRowLabel(row) for row in self._processError]
            w = QMessageBox(QMessageBox.Critical, "Processing error(s)",
                            "Unable to process the row(s) {}. Please check the "
                            "logs.".format(str(labels)[1:-1]),
                            QMessageBox.Ok, self.view)
            w.exec()

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
        self.updateViewFromModel()

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
        self.updateViewFromModel()

    def onLoad(self):
        """
        Triggered when the user want to load a file. This methods start a
        QDialog to get the file path from the user.
        """
        filename = QFileDialog.getOpenFileName(self.view, 'Load rundex', '.',
                                               "Rundex (*.mrd);;All (*.*)")
        if not filename[0]:
            return
        self.model.setIOFile(filename[0])
        self.model.importRundexData()
        self.view.setWindowModified(False)
        self.updateViewFromModel()

    def onSave(self):
        """
        Triggered when the user wants to save its data. This method starts a
        QDialog only if no file was previously used to load or save.
        """
        if self.model.getIOFile():
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
                                               "Rundex (*.mrd);;All (*.*)")
        if not filename[0]:
            return
        self.model.setIOFile(filename[0])
        self.model.setVisualSettings(self.view.getVisualSettings())
        self.model.exportRundexData()
        self.view.setWindowModified(False)

    def settingsWindow(self):
        """
        Show the setting window. This function creates a special dialog that
        generates automatically its fields on the basis of settings types. It
        also connects the differents signals to get validation of user inputs.
        """
        sw = DrillSettingsDialog(self.view)
        types, values, doc = self.model.getSettingsTypes()
        sw.initWidgets(types, values, doc)
        sw.setSettings(self.model.getSettings())
        self.model.paramOk.connect(
                lambda sample, param: sw.onSettingValidation(param, True)
                )
        self.model.paramError.connect(
                lambda sample, param, msg: sw.onSettingValidation(param, False,
                                                                  msg)
                )
        sw.valueChanged.connect(
                lambda p : self.model.checkParameter(p, sw.getSettingValue(p))
                )
        sw.accepted.connect(
                lambda : self.model.setSettings(sw.getSettings())
                )
        sw.show()

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
        self.updateViewFromModel()

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

    def updateViewFromModel(self):
        """
        Update the view (header and table) from the model.
        """
        # update the header
        self.view.set_available_modes(
                self.model.getAvailableAcquisitionModes())
        self.view.set_acquisition_mode(self.model.getAcquisitionMode())
        cycle, exp = self.model.getCycleAndExperiment()
        self.view.setCycleAndExperiment(cycle, exp)
        # update the table
        columns, tooltips = self.model.getColumnHeaderData()
        self.view.set_table(columns, tooltips)
        contents = self.model.getRowsContents()
        self.view.fill_table(contents)
        self._invalidCells = set()
        groups = self.model.getSamplesGroups()
        masters = self.model.getMasterSamples()
        for group in groups:
            if group in masters:
                master = masters[group]
            else:
                master = None
            self.view.labelRowsInGroup(group, groups[group], master)
        # set the visual settings if they exist
        vs = self.model.getVisualSettings()
        if vs:
            self.view.setVisualSettings(vs)
