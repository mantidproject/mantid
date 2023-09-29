# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


from mantid.simpleapi import (
    logger,
    AverageLogData,
    CreateEmptyTableWorkspace,
    GroupWorkspaces,
    DeleteWorkspace,
    DeleteTableRows,
    RenameWorkspace,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantid.api import AnalysisDataService as ADS

from os import path
from numpy import full, nan, max, mean, std


def write_table_row(ws_table, row, irow):
    if irow > ws_table.rowCount() - 1:
        ws_table.setRowCount(irow + 1)
    [ws_table.setCell(irow, icol, row[icol]) for icol in range(0, len(row))]


def _generate_workspace_name(filepath: str, suffix: str) -> str:
    wsname = path.splitext(path.split(filepath)[1])[0] + suffix
    return wsname


class SampleLogsGroupWorkspace(object):
    def __init__(self, suffix: str):
        self._log_names = []
        self._log_workspaces = None  # GroupWorkspace
        self._log_values = dict()  # {ws_name: {log_name: [avg, er]} }
        self._suffix = suffix
        self._run_info_name = "run_info" + self._suffix

    def create_log_workspace_group(self):
        # run information table
        run_info = self.make_runinfo_table()
        self._log_workspaces = GroupWorkspaces([run_info], OutputWorkspace="logs" + self._suffix)
        # a table per logs
        logs = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "logs")
        if logs:
            self._log_names = logs.split(",")
            for log in self._log_names:
                log_table_ws = self.make_log_table(log)
                self._log_workspaces.add(log_table_ws.name())

    def make_log_table(self, log):
        ws_log = CreateEmptyTableWorkspace(OutputWorkspace=log + self._suffix)
        ws_log.addColumn(type="float", name="avg")
        ws_log.addColumn(type="float", name="stdev")
        return ws_log

    def make_runinfo_table(self):
        run_info = CreateEmptyTableWorkspace(OutputWorkspace=self._run_info_name)
        run_info.addColumn(type="str", name="Instrument")
        run_info.addColumn(type="int", name="Run")
        run_info.addColumn(type="str", name="Bank")
        run_info.addColumn(type="float", name="uAmps")
        run_info.addColumn(type="str", name="Title")
        return run_info

    def update_log_workspace_group(self, data_workspaces=None):
        # both ws and name needed in event a ws is renamed and ws.name() is no longer correct

        if not data_workspaces:
            self.delete_logs()
            return

        if not self._log_workspaces:
            self.create_log_workspace_group()
        else:
            for log in self._log_names:
                if not ADS.doesExist(log + self._suffix):
                    log_table_ws = self.make_log_table(log)
                    self._log_workspaces.add(log_table_ws.name())
            if not ADS.doesExist(self._run_info_name):
                self.make_runinfo_table()
                self._log_workspaces.add(self._run_info_name)
        # update log tables
        self.remove_all_log_rows()
        for irow, (ws_name, ws) in enumerate(data_workspaces.get_loaded_ws_dict().items()):
            try:
                self.add_log_to_table(ws_name, ws, irow)
            except Exception as e:
                logger.warning(f"Unable to output log workspaces for workspace {ws_name}: " + str(e))

    def add_log_to_table(self, ws_name, ws, irow):
        # both ws and name needed in event a ws is renamed and ws.name() is no longer correct
        # make dict for run if doesn't exist
        if ws_name not in self._log_values:
            self._log_values[ws_name] = dict()
        # add run info
        run = ws.getRun()
        row = [
            ws.getInstrument().getFullName(),
            ws.getRunNumber(),
            str(run.getProperty("bankid").value),
            run.getProtonCharge(),
            ws.getTitle(),
        ]
        write_table_row(ADS.retrieve(self._run_info_name), row, irow)
        # add log data - loop over existing log workspaces not logs in settings as these might have changed
        nullLogValue = full(2, nan)  # default nan if can't read/average log data
        if run.getProtonCharge() > 0:
            for log in self._log_names:
                avg, stdev = nullLogValue
                if log in self._log_values[ws_name]:
                    avg, stdev = self._log_values[ws_name][log]  # already averaged
                elif run.hasProperty(log):
                    if run.hasProperty("proton_charge"):
                        avg, stdev = AverageLogData(ws_name, LogName=log, FixZero=False)
                    else:
                        # average filtered log values (excludes setup time)
                        log_series = run.getProperty(log)
                        avg = mean(log_series.filtered_value)
                        stdev = std(log_series.filtered_value)
                        logger.warning(f"{ws.name()} does not contain a proton charge log - an unweighted average has been performed.")
                self._log_values[ws_name][log] = [avg, stdev]  # update model dict (even if nan)
        else:
            self._log_values[ws_name] = {log: nullLogValue for log in self._log_names}
            logger.warning(f"{ws.name()} appears to be empty with zero integrated proton charge.")

        # write log values to table (nan if log could not be averaged)
        for log, avg_and_stdev in self._log_values[ws_name].items():
            write_table_row(ADS.retrieve(log + self._suffix), avg_and_stdev, irow)
        self.update_log_group_name()

    def remove_log_rows(self, row_numbers):
        DeleteTableRows(TableWorkspace=self._log_workspaces, Rows=list(row_numbers))
        self.update_log_group_name()

    def remove_all_log_rows(self):
        for ws in self._log_workspaces:
            ws.setRowCount(0)

    def delete_logs(self):
        if self._log_workspaces:
            ws_name = self._log_workspaces.name()
            self._log_workspaces = None
            DeleteWorkspace(ws_name)

    def update_log_group_name(self):
        name = self._generate_log_group_name()
        if not name:
            self.delete_logs()
            return
        RenameWorkspace(InputWorkspace=self._log_workspaces.name(), OutputWorkspace=name)

    def _generate_log_group_name(self) -> str:
        run_info = ADS.retrieve(self._run_info_name)
        if run_info.rowCount() > 0:
            runs = run_info.column("Run")
            name = f"{run_info.row(0)['Instrument']}_{min(runs)}-{max(runs)}_logs"
            return name + self._suffix
        return ""

    def get_log_values(self):
        return self._log_values

    def get_log_workspaces(self):
        return self._log_workspaces

    def update_log_value(self, new_key, old_key):
        self._log_values[new_key] = self._log_values.pop(old_key)

    def clear_log_workspaces(self):
        self._log_workspaces = None
