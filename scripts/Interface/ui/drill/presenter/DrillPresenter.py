# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ..view.SansSettingsView import SansSettingsView

class DrillPresenter:

    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.view.set_available_instruments(self.model.getAvailableTechniques())

        # signals connections
        self.connect_view_signals()
        self.model.process_started.connect(self.on_process_started)
        self.model.process_done.connect(self.on_process_done)
        self.model.process_error.connect(self.on_process_error)
        self.model.progress_update.connect(self.on_progress)
        self.model.processing_done.connect(self.on_processing_done)
        self.model.param_ok.connect(self.on_param_ok)
        self.model.param_error.connect(self.on_param_error)

        self.update_view_from_model()

    def show(self):
        self.view.show()

    def connect_view_signals(self):
        self.view.instrument_changed.connect(self.on_instrument_changed)
        self.view.acquisition_mode_changed.connect(self.on_acquisition_mode_changed)
        self.view.row_added.connect(self.on_add_row)
        self.view.row_deleted.connect(self.on_del_row)
        self.view.data_changed.connect(self.on_data_changed)
        self.view.process.connect(self.on_process)
        self.view.process_stopped.connect(self.on_process_stop)
        self.view.rundex_loaded.connect(self.on_rundex_loaded)
        self.view.rundex_saved.connect(self.on_rundex_saved)
        self.view.show_settings.connect(self.on_settings_window)

    def disconnect_view_signals(self):
        self.view.instrument_changed.disconnect()
        self.view.acquisition_mode_changed.disconnect()
        self.view.row_added.disconnect()
        self.view.row_deleted.disconnect()
        self.view.data_changed.disconnect()
        self.view.process.disconnect()
        self.view.process_stopped.disconnect()
        self.view.rundex_loaded.disconnect()
        self.view.rundex_saved.disconnect()

    def on_add_row(self, position):
        self.model.add_row(position)

    def on_del_row(self, position):
        self.model.del_row(position)

    def on_data_changed(self, row, column, contents):
        self.model.change_data(row, column, contents)

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

    def on_rundex_saved(self, filename):
        self.model.exportRundexData(filename)

    def on_settings_window(self):
        sw = SansSettingsView(self.view)
        sw.setSettings(self.model.getSettings())
        sw.accepted.connect(
                lambda : self.model.setSettings(sw.getSettings())
                )
        sw.show()

    def on_process_started(self, row):
        self.view.set_row_processing(row)

    def on_process_done(self, ref):
        self.view.set_row_done(ref)

    def on_process_error(self, ref):
        self.view.set_row_error(ref)

    def on_progress(self, progress):
        self.view.set_progress(progress, 100)

    def on_processing_done(self):
        self.view.set_disabled(False)
        self.view.set_progress(0, 100)

    def on_param_ok(self, row, column):
        self.view.set_cell_ok(row, column)

    def on_param_error(self, row, column, msg):
        self.view.set_cell_error(row, column, msg)

    def update_view_from_model(self):
        self.view.set_available_modes(
                self.model.getAvailableAcquisitionModes())
        self.view.set_acquisition_mode(self.model.getAcquisitionMode())
        # set table
        self.view.set_table(self.model.get_columns())
        rows_contents = self.model.get_rows_contents()
        self.view.fill_table(self.model.get_rows_contents())
