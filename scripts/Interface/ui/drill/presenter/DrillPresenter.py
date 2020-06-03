# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ..view.SansSettingsView import SansSettingsView


class DrillPresenter:

    def __init__(self, model, view):
        """
        Initialize the presenter by giving a view and a model. This method
        connects all the view and model signals.

        Args:
            model (DrillModel): the model
            view (DrillView): the view
        """
        self.model = model
        self.view = view
        self.view.set_available_instruments(self.model.getAvailableTechniques())

        # view signals connection
        self.view.instrument_changed.connect(self.instrumentChanged)
        self.view.acquisition_mode_changed.connect(self.acquisitionModeChanged)
        self.view.row_added.connect(
                lambda position: self.model.add_row(position)
                )
        self.view.row_deleted.connect(
                lambda position: self.model.del_row(position)
                )
        self.view.data_changed.connect(
                lambda row, column, contents: self.model.change_data(
                    row, column, contents)
                )
        self.view.process.connect(self.process)
        self.view.process_stopped.connect(self.stopProcessing)
        self.view.rundex_loaded.connect(self.rundexLoaded)
        self.view.rundex_saved.connect(
                lambda filename: self.model.exportRundexData(filename)
                )
        self.view.show_settings.connect(self.settingsWindow)

        # model signals connection
        self.model.process_started.connect(
                lambda row: self.view.set_row_processing(row)
                )
        self.model.process_done.connect(
                lambda row: self.view.set_row_done(row)
                )
        self.model.process_error.connect(
                lambda row: self.view.set_row_error(row)
                )
        self.model.progress_update.connect(
                lambda progress: self.view.set_progress(progress, 100)
                )
        self.model.processing_done.connect(self.processingDone)
        self.model.param_ok.connect(
                lambda row, param: self.view.set_cell_ok(row, param)
                )
        self.model.param_error.connect(
                lambda row, param, msg: self.view.set_cell_error(
                    row, param, msg)
                )

        self.updateViewFromModel()

    def show(self):
        """
        Show the view. This method is here for convenience.
        """
        self.view.show()

    def process(self, rows):
        """
        Handles the row processing asked by the view. It forwards it to the
        model, takes care of the progress and the potential exceptions.

        Args:
            rows (list(int)): list of rows to be processed
        """
        try:
            self.model.process(rows)
        except Exception as e:
            self.view.processing_error(e.elements)
        else:
            self.view.set_disabled(True)
            self.view.set_progress(0, 100)

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
        self.model.importRundexData(filename)
        self.updateViewFromModel()

    def settingsWindow(self):
        """
        Show the setting window.
        """
        sw = SansSettingsView(self.view)
        sw.setSettings(self.model.getSettings())
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
        # update the table
        self.view.set_table(self.model.get_columns())
        self.view.fill_table(self.model.get_rows_contents())
