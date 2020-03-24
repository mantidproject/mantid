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

    def on_add_rows(self, position, content):
        print("on_add_rows: " + str(position) + str(content))

    def on_delete_rows(self, rows):
        print("on_delete_rows: " + str(rows))

    def on_data_changed(self, row, column, content):
        print("on_data_changed: " + str(row) + str(column) + str(content))

    def on_process(self, rows):
        print("on_process: " + str(rows))
        #self.model.process_on_thread(contents)

    def on_process_all(self):
        print("on_process_all")

    def on_instrument_changed(self, instrument):
        self.model.set_instrument(instrument)
        self.update_view_from_model()

    def update_view_from_model(self):
        self.view.set_table(self.model.get_columns())

