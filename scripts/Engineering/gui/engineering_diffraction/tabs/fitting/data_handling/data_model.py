# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path

from mantid.simpleapi import Load, logger, EnggEstimateFocussedBackground, ConvertUnits, Plus, Minus, AverageLogData, \
    CreateEmptyTableWorkspace, GroupWorkspaces, DeleteWorkspace, DeleteTableRows, RenameWorkspace
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from Engineering.gui.engineering_diffraction.tabs.common import path_handling
from mantid.api import AnalysisDataService as ADS
from matplotlib.pyplot import subplots
from numpy import full, nan


class FittingDataModel(object):
    def __init__(self):
        self._log_names = []
        self._log_workspaces = None
        self._log_values = dict()  # {ws_name: {log_name: [avg, er]} }
        self._loaded_workspaces = {}  # Map stores using {WorkspaceName: Workspace}
        self._background_workspaces = {}
        self._last_added = []  # List of workspace names loaded in the last load action.
        self._bg_params = dict()  # {ws_name: [isSub, niter, xwindow, doSG]}

    def load_files(self, filenames_string, xunit):
        self._last_added = []
        filenames = [name.strip() for name in filenames_string.split(",")]
        for filename in filenames:
            ws_name = self._generate_workspace_name(filename, xunit)
            if ws_name not in self._loaded_workspaces:
                try:
                    if not ADS.doesExist(ws_name):
                        ws = Load(filename, OutputWorkspace=ws_name)
                        if xunit != "TOF":
                            ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws_name, Target=xunit)
                    else:
                        ws = ADS.retrieve(ws_name)
                    if ws.getNumberHistograms() == 1:
                        self._loaded_workspaces[ws_name] = ws
                        if ws_name not in self._background_workspaces:
                            self._background_workspaces[ws_name] = None
                        self._last_added.append(ws_name)
                    else:
                        logger.warning(
                            f"Invalid number of spectra in workspace {ws_name}. Skipping loading of file.")
                except RuntimeError as e:
                    logger.error(
                        f"Failed to load file: {filename}. Error: {e}. \n Continuing loading of other files.")
            else:
                logger.warning(f"File {ws_name} has already been loaded")
        self.update_log_workspace_group()

    def create_log_workspace_group(self):
        # run information table
        run_info = self.make_runinfo_table()
        self._log_workspaces = GroupWorkspaces([run_info], OutputWorkspace='logs')
        # a table per logs
        logs = get_setting(path_handling.INTERFACES_SETTINGS_GROUP, path_handling.ENGINEERING_PREFIX, "logs")
        if logs:
            self._log_names = logs.split(',')
            for log in self._log_names:
                self.make_log_table(log)
                self._log_workspaces.add(log)

    def make_log_table(self, log):
        ws_log = CreateEmptyTableWorkspace(OutputWorkspace=log)
        ws_log.addColumn(type="float", name="avg")
        ws_log.addColumn(type="float", name="stdev")
        return ws_log

    def make_runinfo_table(self):
        run_info = CreateEmptyTableWorkspace()
        run_info.addColumn(type="str", name="Instrument")
        run_info.addColumn(type="int", name="Run")
        run_info.addColumn(type="int", name="Bank")
        run_info.addColumn(type="float", name="uAmps")
        run_info.addColumn(type="str", name="Title")
        return run_info

    def update_log_workspace_group(self):
        # both ws and name needed in event a ws is renamed and ws.name() is no longer correct
        if not self._log_workspaces:
            self.create_log_workspace_group()
        else:
            for log in self._log_names:
                if not ADS.doesExist(log):
                    self.make_log_table(log)
                    self._log_workspaces.add(log)
            if not ADS.doesExist("run_info"):
                self.make_runinfo_table()
                self._log_workspaces.add("run_info")
        # update log tables
        for irow, (ws_name, ws) in enumerate(self._loaded_workspaces.items()):
            self.add_log_to_table(ws_name, ws, irow)  # rename write_log_row

    def add_log_to_table(self, ws_name, ws, irow):
        # both ws and name needed in event a ws is renamed and ws.name() is no longer correct
        # make dict for run if doesn't exist
        if ws_name not in self._log_values:
            self._log_values[ws_name] = dict()
        # add run info
        run = ws.getRun()
        row = [ws.getInstrument().getFullName(), ws.getRunNumber(), run.getProperty('bankid').value,
               run.getProtonCharge(), ws.getTitle()]
        self.write_table_row(ADS.retrieve("run_info"), row, irow)
        # add log data - loop over existing log workspaces not logs in settings as these might have changed
        for log in self._log_names:
            if log in self._log_values[ws_name]:
                avg, stdev = self._log_values[ws_name][log]
            else:
                try:
                    avg, stdev = AverageLogData(ws_name, LogName=log, FixZero=False)
                    self._log_values[ws_name][log] = [avg, stdev]
                except RuntimeError:
                    avg, stdev = full(2, nan)
                    logger.error(
                        f"File {ws.name()} does not contain log {log}")
            self.write_table_row(ADS.retrieve(log), [avg, stdev], irow)
        self.update_log_group_name()

    def remove_log_rows(self, row_numbers):
        DeleteTableRows(TableWorkspace=self._log_workspaces, Rows=list(row_numbers))
        self.update_log_group_name()

    def clear_logs(self):
        if self._log_workspaces:
            ws_name = self._log_workspaces.name()
            self._log_workspaces = None
            DeleteWorkspace(ws_name)

    def update_log_group_name(self):
        run_info = ADS.retrieve('run_info')
        if run_info.rowCount() > 0:
            runs = run_info.column('Run')
            name = f"{run_info.row(0)['Instrument']}_{min(runs)}-{max(runs)}_logs"
            if not name == self._log_workspaces.name():
                RenameWorkspace(InputWorkspace=self._log_workspaces.name(), OutputWorkspace=name)
        else:
            self.clear_logs()

    def update_workspace_name(self, old_name, new_name):
        if new_name not in self._loaded_workspaces:
            self._loaded_workspaces[new_name] = self._loaded_workspaces.pop(old_name)
            if old_name in self._background_workspaces:
                self._background_workspaces[new_name] = self._background_workspaces.pop(old_name)
            if old_name in self._bg_params:
                self._bg_params[new_name] = self._bg_params.pop(old_name)
            if old_name in self._log_values:
                self._log_values[new_name] = self._log_values.pop(old_name)
        else:
            logger.warning(f"There already exists a workspace with name {new_name}.")

    def get_loaded_workspaces(self):
        return self._loaded_workspaces

    def get_log_workspaces_name(self):
        return [ws.name() for ws in self._log_workspaces] if self._log_workspaces else ''

    def get_background_workspaces(self):
        return self._background_workspaces

    def get_bg_params(self):
        return self._bg_params

    def do_background_subtraction(self, ws_name, bg_params):
        ws = self._loaded_workspaces[ws_name]
        ws_bg = self._background_workspaces[ws_name]
        bg_changed = False
        if ws_bg and bg_params[1:] != self._bg_params[ws_name][1:]:
            # add bg back on to data (but don't change bgsub status)
            self.undo_background_subtraction(ws_name, isBGsub=bg_params[0])
            bg_changed = True
        if bg_changed or not ws_bg:
            # re-evaluate background (or evaluate for first time)
            self._bg_params[ws_name] = bg_params
            ws_bg = self.estimate_background(ws_name, *bg_params[1:])
        # update bg sub status before Minus (updates plot which repopulates table)
        self._bg_params[ws_name][0] = bg_params[0]
        Minus(LHSWorkspace=ws, RHSWorkspace=ws_bg, OutputWorkspace=ws_name)

    def undo_background_subtraction(self, ws_name, isBGsub=False):
        self._bg_params[ws_name][0] = isBGsub  # must do this before plotting which refreshes table
        Plus(LHSWorkspace=ws_name, RHSWorkspace=self.get_background_workspaces()[ws_name],
             OutputWorkspace=ws_name)

    def estimate_background(self, ws_name, niter, xwindow, doSGfilter):
        ws_bg = EnggEstimateFocussedBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name + "_bg",
                                               NIterations=niter, XWindow=xwindow, ApplyFilterSG=doSGfilter)
        self._background_workspaces[ws_name] = ws_bg
        return ws_bg

    def plot_background_figure(self, ws_name):
        ws = self._loaded_workspaces[ws_name]
        ws_bg = self._background_workspaces[ws_name]
        if ws_bg:
            fig, ax = subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [2, 1]},
                               subplot_kw={'projection': 'mantid'})
            tmp = Plus(LHSWorkspace=ws_name, RHSWorkspace=ws_bg, StoreInADS=False)
            ax[0].plot(tmp, 'x')
            ax[0].plot(ws_bg, '-r')
            ax[1].plot(ws, 'x')
            fig.show()

    def get_last_added(self):
        return self._last_added

    def get_sample_log_from_ws(self, ws_name, log_name):
        return self._loaded_workspaces[ws_name].getSampleDetails().getLogData(log_name).value

    @staticmethod
    def write_table_row(ws_table, row, irow):
        if irow > ws_table.rowCount() - 1:
            ws_table.setRowCount(irow + 1)
        [ws_table.setCell(irow, icol, row[icol]) for icol in range(0, len(row))]

    @staticmethod
    def _generate_workspace_name(filepath, xunit):
        return path.splitext(path.split(filepath)[1])[0] + '_' + xunit
