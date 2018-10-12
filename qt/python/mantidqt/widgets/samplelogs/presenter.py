# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from .model import SampleLogsModel
from .view import SampleLogsView


class SampleLogs(object):
    def __init__(self, ws, parent=None, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else SampleLogsModel(ws)
        self.view = view if view else SampleLogsView(self,
                                                     parent,
                                                     self.model.get_name(),
                                                     self.model.isMD(),
                                                     self.model.getNumExperimentInfo())
        self.setup_table()

    def clicked(self):
        self.update_stats()
        self.plot_logs()

    def doubleClicked(self, i):
        # print the log, later this should display the data in a table workspace or something
        log_text = self.view.get_row_log_name(i.row())
        log = self.model.get_log(log_text)
        print('# {}'.format(log.name))
        print(log.valueAsPrettyStr())

    def changeExpInfo(self):
        selected_rows = self.view.get_selected_row_indexes()
        self.model.set_exp(self.view.get_exp())
        self.setup_table()
        self.view.set_selected_rows(selected_rows)
        self.update_stats()
        self.plot_logs()

    def update_stats(self):
        selected_rows = self.view.get_selected_row_indexes()
        if len(selected_rows) == 1:
            stats = self.model.get_statistics(self.view.get_row_log_name(selected_rows[0]))
            if stats:
                self.view.set_statistics(stats)
            else:
                self.view.clear_statistics()
        else:
            self.view.clear_statistics()

    def plot_logs(self):
        selected_rows = self.view.get_selected_row_indexes()
        to_plot = [row for row in selected_rows if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def setup_table(self):
        self.view.set_model(self.model.getItemModel())
