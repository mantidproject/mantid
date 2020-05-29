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
        self.view.instrument_changed.connect(self.on_instrument_changed)
        self.view.acquisition_mode_changed.connect(self.on_acquisition_mode_changed)
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
        self.view.process.connect(self.on_process)
        self.view.process_stopped.connect(self.on_process_stop)
        self.view.rundex_loaded.connect(self.on_rundex_loaded)
        self.view.rundex_saved.connect(
                lambda filename: self.model.exportRundexData(filename)
                )
        self.view.show_settings.connect(self.on_settings_window)

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
        self.model.processing_done.connect(self.on_processing_done)
        self.model.param_ok.connect(
                lambda row, column: self.view.set_cell_ok(row, column)
                )
        self.model.param_error.connect(
                lambda row, column, msg: self.view.set_cell_error(
                    row, column, msg)
                )

        self.update_view_from_model()

    def show(self):
        self.view.show()

    def on_process(self, rows):
        try:
            self.model.process(rows)
        except Exception as e:
            self.view.processing_error(e.elements)
        else:
            self.view.set_disabled(True)
            self.view.set_progress(0, 100)

    def on_process_stop(self):
        self.model.stopProcess();
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)

    def on_instrument_changed(self, instrument):
        self.model.setInstrument(instrument)
        self.update_view_from_model()

    def on_acquisition_mode_changed(self, mode):
        self.model.setAcquisitionMode(mode)
        self.view.set_table(self.model.get_columns())
        rows_contents = self.model.get_rows_contents()
        self.view.fill_table(self.model.get_rows_contents())

    def on_rundex_loaded(self, filename):
        self.model.importRundexData(filename)
        self.update_view_from_model()

    def on_settings_window(self):
        sw = SansSettingsView(self.view)
        sw.setSettings(self.model.getSettings())
        sw.accepted.connect(
                lambda : self.model.setSettings(sw.getSettings())
                )
        sw.show()

    def on_processing_done(self):
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)

    def update_view_from_model(self):
        self.view.set_available_modes(
                self.model.getAvailableAcquisitionModes())
        self.view.set_acquisition_mode(self.model.getAcquisitionMode())
        # set table
        self.view.set_table(self.model.get_columns())
        rows_contents = self.model.get_rows_contents()
        self.view.fill_table(self.model.get_rows_contents())
