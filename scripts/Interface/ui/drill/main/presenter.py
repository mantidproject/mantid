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

    def on_load_rundex_clicked(self):
        pass

    def on_load_settings_clicked(self):
        pass

    def on_insert_row(self):
        pass

    def on_erase_rows(self):
        pass

    def on_copy_rows_requested(self):
        pass

    def on_cut_rows(self):
        pass

    def on_paste_rows_requested(self):
        pass

    def on_save_rundex_clicked(self):
        pass

    def on_row_inserted(self):
        pass

    def on_rows_removed(self, rows):
        pass

    def on_data_changed(self, row, content):
        pass

    def on_process_clicked(self, contents):
        self.model.process_on_thread(contents)
