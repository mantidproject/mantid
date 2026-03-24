# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtCore import Qt

from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from mantidqt.widgets.observers.ads_observer import WorkspaceDisplayADSObserver
from .model import SampleLogsModel
from .view import SampleLogsView


class SampleLogs(ObservingPresenter):
    """ """

    def __init__(self, ws, parent=None, window_flags=Qt.Window, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model or SampleLogsModel(ws)
        self.view = view or SampleLogsView(
            self, parent, window_flags, self.model.get_name(), self.model.isMD(), self.model.getNumExperimentInfo()
        )
        self.container = self.view  # needed for the ObservingPresenter
        self.filtered = True
        self.show_timeROI = True
        self.setup_table()

        self.ads_observer = WorkspaceDisplayADSObserver(self)

        # connect to replace_signal signal to handle replacement of the workspace
        self.container.replace_signal.connect(self.action_replace_workspace)
        self.container.rename_signal.connect(self.action_rename_workspace)

    def update(self):
        """Update stats with selected log and replot the
        logs.  Finally update any other view aspects associated with the selected log.
        """
        self.log_changed()
        self.update_stats()
        self.plot_logs()

    def doubleClicked(self, i):
        """When a log is doubleClicked, print the log, later this should
        display the data in a table workspace or something
        """
        self.print_selected_logs()

    def log_changed(self):
        """Update interface aspects"""
        # determine if any of the logs might support filtering
        are_any_logs_filtered = False
        for row in self.view.get_selected_row_indexes():
            if self.model.get_is_log_filtered(self.view.get_row_log_name(row)):
                are_any_logs_filtered = True
                break
        self.view.set_log_controls(are_any_logs_filtered)

    def print_selected_logs(self):
        """Print all selected logs"""
        for row in self.view.get_selected_row_indexes():
            log = self.model.get_log(self.view.get_row_log_name(row))
            print("# {}".format(log.name))
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
            stats = self.model.get_statistics(self.view.get_row_log_name(selected_rows[0]), self.filtered)
            if stats:
                self.view.set_statistics(stats)
            else:
                self.view.clear_statistics()
        else:
            self.view.clear_statistics()

    def plot_logs(self):
        """Get all selected rows, check if plottable, then plot the logs"""
        to_plot = [row for row in self.view.get_selected_row_indexes() if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def new_plot_logs(self):
        """Get all selected rows, check if plottable, then plot the logs in new figure"""
        to_plot = [row for row in self.view.get_selected_row_indexes() if self.model.is_log_plottable(self.view.get_row_log_name(row))]
        self.view.new_plot_selected_logs(self.model.get_ws(), self.model.get_exp(), to_plot)

    def setup_table(self, search_key=""):
        """Set the model in the view to the one create from the model"""
        self.view.show_plot_and_stats(self.model.are_any_logs_plottable())
        self.view.set_model(self.model.getItemModel(search_key))
        self.show_timeROI = not self.model.get_timeroi().useAll()
        self.view.enable_timeROI(self.show_timeROI)

    def filtered_changed(self, state):
        self.filtered = state
        self.plot_logs()
        self.update_stats()

    def show_timeroi_changed(self, state):
        self.show_timeROI = state
        self.plot_logs()
        self.update_stats()

    def search_key_changed(self):
        """When the line edit is changed, print the logs that match the search key,
        and display the data.
        """
        self.update_table_with_matching_logs()

    def update_table_with_matching_logs(self):
        """Updates the table with logs matching the search key."""
        self.setup_table(self.view.line_edit.text())

    def action_replace_workspace(self, workspace_name, workspace):
        if self.model.workspace_equals(workspace_name):
            self.model.set_ws(workspace)
            self.setup_table()
            self.update()

    def action_rename_workspace(self, workspace_name):
        self.model.set_name(workspace_name)
