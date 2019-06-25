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
    """
    """
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
        """When a log is clicked update stats with selected log and replot the
        logs
        """
        self.update_stats()
        self.plot_logs()

    def doubleClicked(self, i):
        """When a log is doubleClicked, print the log, later this should
        display the data in a table workspace or something
        """
        self.print_selected_logs()

    def print_selected_logs(self):
        """Print all selected logs"""
        for row in self.view.get_selected_row_indexes():
            log = self.model.get_log(self.view.get_row_log_name(row))
            print('# {}'.format(log.name))
            print(log.valueAsPrettyStr())

    def plot_clicked(self, event):
        """Check if figure is doubleClicked, then create new plot"""
        if event.dblclick:
            self.new_plot_logs()

    def changeExpInfo(self):
        """When a different experiment info is selected this updates
        everything
        """
        # get currently selected rows
        selected_rows = self.view.get_selected_row_indexes()
        # set experiment info of model to one from view
        self.model.set_exp(self.view.get_exp())
        # create a new table as eveything has changed
        self.setup_table()
        # reselect previously selected rows
        self.view.set_selected_rows(selected_rows)
        # update stats with different experiment info
        self.update_stats()
        # replot logs with different experiment info
        self.plot_logs()

    def update_stats(self):
        """Updates the stats for currently select row.

        If more then one row is selected then that stats are just cleared.
        """
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
        """Get all selected rows, check if plottable, then plot the logs"""
        to_plot = [row for row in self.view.get_selected_row_indexes()
                   if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def new_plot_logs(self):
        """Get all selected rows, check if plottable, then plot the logs in new figure"""
        to_plot = [row for row in self.view.get_selected_row_indexes()
                   if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.new_plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def setup_table(self):
        """Set the model in the view to the one create from the model"""
        self.view.set_model(self.model.getItemModel())
