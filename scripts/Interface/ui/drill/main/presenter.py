# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from .view import (DrillView, DrillEventListener)
from .model import DrillModel


class DrillPresenter(DrillEventListener):

    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.view.add_listener(self)
        self.update_view_from_model()

    def on_load_rundex_clicked(self):
        pass

    def on_load_settings_clicked(self):
        pass

    def on_insert_row(self):
        pass

    def on_erase_rows(self, rows):
        print("on_erase_rows: " + str(rows))

    def on_copy_rows_requested(self, rows):
        print("on_copy_rows_requested: " + str(rows))

    def on_cut_rows(self, rows):
        print("on_cut_rows: " + str(rows))

    def on_paste_rows_requested(self):
        print("on_paste_rows_requested")

    def on_save_rundex_clicked(self):
        pass

    def on_row_inserted(self):
        pass

    def on_rows_removed(self, rows):
        print("on_rows_removed: " + str(rows))

    def on_data_changed(self, row, content):
        pass

    def on_process_clicked(self, contents):
        self.model.process_on_thread(contents)

    def on_instrument_changed(self, instrument):
        self.model.set_instrument(instrument)
        self.update_view_from_model()

    def update_view_from_model(self):
        self.view.set_table(self.model.get_columns())

