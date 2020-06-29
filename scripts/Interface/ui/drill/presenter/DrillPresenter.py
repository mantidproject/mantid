# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ..view.SettingsDialog import SettingsDialog
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
        self.view.set_available_instruments(self.model.getAvailableTechniques())

        # view signals connection
        self.view.instrumentChanged.connect(self.instrumentChanged)
        self.view.acquisitionModeChanged.connect(self.acquisitionModeChanged)
        self.view.rowAdded.connect(self.model.addSample)
        self.view.rowDeleted.connect(self.model.deleteSample)
        self.view.dataChanged.connect(self.model.changeParameter)
        self.view.process.connect(self.process)
        self.view.processStopped.connect(self.stopProcessing)
        self.view.rundexLoaded.connect(self.rundexLoaded)
        self.view.rundexSaved.connect(self.rundexSaved)
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

    def instrumentChanged(self, instrument):
        """
        Forward the instrument changes to the model and update the view.

        Args:
            instrument (str): instrument name
        """
        self.model.setInstrument(instrument)
        self.updateViewFromModel()

    def acquisitionModeChanged(self, mode):
        """
        Forward the acquisition mode changes to the model and update the view.

        Args:
            mode (str): acquisition mode name
        """
        self.model.setAcquisitionMode(mode)
        self.updateViewFromModel()

    def rundexLoaded(self, filename):
        """
        Forward the rundex file loading to the model and update the view.

        Args:
            filename (str): rundex file path
        """
        try:
            self.model.importRundexData(filename)
            self.updateViewFromModel()
        except Exception as ex:
            self.view.errorPopup("Import error",
                                 "Unable to open {0}".format(filename),
                                 str(ex))

    def rundexSaved(self, filename):
        """
        Forward the rundex file saving to the model. This method transmit also
        some potential visual settings that the view wants to save.

        Args:
            filename (str): rundex file path
        """
        vs = self.view.getVisualSettings()
        self.model.exportRundexData(filename, vs)

    def settingsWindow(self):
        """
        Show the setting window. This function creates a special dialog that
        generates automatically its fields on the basis of settings types. It
        also connects the differents signals to get validation of user inputs.
        """
        sw = SettingsDialog(self.view)
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
        self.view.setRundexFile(self.model.rundexFile)
        # update the header
        self.view.set_available_modes(
                self.model.getAvailableAcquisitionModes())
        self.view.set_acquisition_mode(self.model.getAcquisitionMode())
        # update the table
        columns, tooltips = self.model.getColumnHeaderData()
        self.view.set_table(columns, tooltips)
        self.view.fill_table(self.model.get_rows_contents())
        self.model.setSampleNames(self.view.getRowNames())
        # set the visual settings if they exist
        vs = self.model.getVisualSettings()
        if vs:
            self.view.setVisualSettings(vs)
