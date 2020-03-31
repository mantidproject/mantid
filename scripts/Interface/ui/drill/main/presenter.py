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
        self.view.set_header(self.model.get_supported_techniques())
        self.update_view_from_model()

    def on_add_row(self, position):
        self.model.add_row(position)

    def on_del_row(self, position):
        self.model.del_row(position)

    def on_data_changed(self, row, column, contents):
        self.model.change_data(row, column, contents)

    def on_process(self, rows):
        self.model.process(rows)

    def on_instrument_changed(self, instrument):
        self.model.set_instrument(instrument)
        self.update_view_from_model()

    def on_rundex_loaded(self, filename):
        self.model.set_rundex_data(filename)
        self.update_view_from_model()

    def on_rundex_saved(self, filename):
        self.model.export_rundex_data(filename)

    def update_view_from_model(self):
        self.view.set_table(self.model.get_columns(),
                            self.model.get_rows_contents())
        self.view.set_technique(self.model.get_technique())

