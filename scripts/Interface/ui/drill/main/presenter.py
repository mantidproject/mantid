# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from .view import DrillView
from .model import DrillModel

class DrillPresenter:

    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.view.set_available_instruments(self.model.get_supported_techniques())

        # signals connections
        self.connect_view_signals()
        self.model.process_done.connect(self.on_process_done)
        self.model.process_error.connect(self.on_process_error)
        self.model.process_running.connect(self.on_process_running)
        self.model.processing_done.connect(self.on_processing_done)

        self.update_view_from_model()

    def connect_view_signals(self):
        self.view.instrument_changed.connect(self.on_instrument_changed)
        self.view.technique_changed.connect(self.on_technique_changed)
        self.view.row_added.connect(self.on_add_row)
        self.view.row_deleted.connect(self.on_del_row)
        self.view.data_changed.connect(self.on_data_changed)
        self.view.process.connect(self.on_process)
        self.view.process_stopped.connect(self.on_process_stop)
        self.view.rundex_loaded.connect(self.on_rundex_loaded)
        self.view.rundex_saved.connect(self.on_rundex_saved)

    def disconnect_view_signals(self):
        self.view.instrument_changed.disconnect()
        self.view.technique_changed.disconnect()
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
        self.model.process(rows)
        n, nmax = self.model.get_processing_progress()
        self.view.set_progress(n, nmax)
        self.view.set_disabled(True)

    def on_process_stop(self):
        self.model.stop_process();
        self.view.set_disabled(False)

    def on_instrument_changed(self, instrument):
        self.model.set_instrument(instrument)
        self.update_view_from_model()

    def on_technique_changed(self, technique):
        self.model.set_technique(technique)
        self.update_view_from_model()

    def on_rundex_loaded(self, filename):
        self.model.set_rundex_data(filename)
        self.update_view_from_model()

    def on_rundex_saved(self, filename):
        self.model.export_rundex_data(filename)

    def on_process_done(self, ref):
        n, nmax = self.model.get_processing_progress()
        self.view.set_progress(n, nmax)
        self.view.set_row_done(ref)

    def on_process_error(self, ref):
        self.view.set_row_error(ref)

    def on_process_running(self, ref):
        self.view.set_row_processing(ref)

    def on_processing_done(self):
        self.view.set_disabled(False)

    def update_view_from_model(self):
        self.view.set_available_techniques(
                self.model.get_available_techniques())
        self.view.set_technique(self.model.get_technique())
        # set table
        self.view.set_table(self.model.get_columns())
        rows_contents = self.model.get_rows_contents()
        self.view.fill_table(self.model.get_rows_contents())

        self.view.set_technique(self.model.get_technique())

