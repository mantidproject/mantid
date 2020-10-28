# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QFileDialog

from ..view.DrillSettingsDialog import DrillSettingsDialog
from ..model.DrillModel import DrillModel


class DrillPresenter:

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

        # view signals connection
        self.view.instrumentChanged.connect(self.instrumentChanged)
        self.view.acquisitionModeChanged.connect(self.acquisitionModeChanged)
        self.view.cycleAndExperimentChanged.connect(
                self.model.setCycleAndExperiment)
        self.view.rowAdded.connect(self.model.addSample)
        self.view.rowDeleted.connect(self.model.deleteSample)
        self.view.dataChanged.connect(self.model.changeParameter)
        self.view.groupRequested.connect(self.onGroupRequested)
        self.view.ungroupRequested.connect(self.onUngroupRequested)
        self.view.setMaster.connect(self.onMasterRowRequested)
        self.view.process.connect(self.process)
        self.view.processStopped.connect(self.stopProcessing)
        self.view.loadRundex.connect(self.onLoad)
        self.view.saveRundex.connect(self.onSave)
        self.view.saveRundexAs.connect(self.onSaveAs)
        self.view.showSettings.connect(self.settingsWindow)

        # model signals connection
        self.model.processStarted.connect(self.view.set_row_processing)
        self.model.processSuccess.connect(self.view.set_row_done)
        self.model.processError.connect(self.view.set_row_error)
        self.model.progressUpdate.connect(
                lambda progress: self.view.set_progress(progress, 100)
                )
        self.model.processingDone.connect(self.processingDone)
        self.model.paramOk.connect(self.view.set_cell_ok)
        self.model.paramError.connect(self.view.set_cell_error)

        self.updateViewFromModel()

    def onGroupRequested(self, rows):
        """
        Triggered when the view request the creation of a group.

        Args:
            rows (list(int)): row indexes
        """
        group = self.model.groupSamples(rows)
        self.view.labelRowsInGroup(group, rows, None)

    def onUngroupRequested(self, rows):
        """
        Triggered when the view request the removing of row from their group(s).

        Args:
            rows (list(int)): row indexes
        """
        self.model.ungroupSamples(rows)
        self.view.labelRowsInGroup(None, rows, None)

    def onMasterRowRequested(self, row):
        group = self.model.setGroupMaster(row)
        if group:
            rows = self.model.getSamplesGroups()[group]
            self.view.labelRowsInGroup(group, rows, row)

    def process(self, rows):
        """
        Handles the row processing asked by the view. It forwards it to the
        model, takes care of the progress and the potential exceptions.

        Args:
            rows (list(int)): list of rows to be processed
        """
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

    def processingDone(self):
        """
        Forward the processing done signal to the view.
        """
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)
        self.view.displayProcessingReport()

    def instrumentChanged(self, instrument):
        """
        Forward the instrument changes to the model and update the view.

        Args:
            instrument (str): instrument name
        """
        self.model.setInstrument(instrument)
        self.model.resetIOFile()
        self.updateViewFromModel()

    def acquisitionModeChanged(self, mode):
        """
        Forward the acquisition mode changes to the model and update the view.

        Args:
            mode (str): acquisition mode name
        """
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
