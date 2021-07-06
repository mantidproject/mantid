# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path

from mantid.simpleapi import Load, logger, EnggEstimateFocussedBackground, ConvertUnits, Minus, AverageLogData, \
    CreateEmptyTableWorkspace, GroupWorkspaces, DeleteWorkspace, DeleteTableRows, RenameWorkspace, CreateWorkspace
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from Engineering.gui.engineering_diffraction.tabs.common import path_handling
from mantid.api import AnalysisDataService as ADS
from mantid.api import TextAxis
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParams
from matplotlib.pyplot import subplots
from numpy import full, nan, max, array, vstack, argsort
from itertools import chain
from collections import defaultdict
from re import findall, sub


class FittingDataModel(object):
    def __init__(self):
        self._log_names = []
        self._log_workspaces = None
        self._log_values = dict()  # {ws_name: {log_name: [avg, er]} }
        self._loaded_workspaces = {}  # Map stores using {WorkspaceName: Workspace}
        self._bg_sub_workspaces = {}  # Map as above, this contains the workspaces with the background sub applied
        self._fit_results = {}  # {WorkspaceName: fit_result_dict}
        self._fit_workspaces = None
        self._last_added = []  # List of workspace names loaded in the last load action.
        self._bg_params = dict()  # {ws_name: [isSub, niter, xwindow, doSG]}

    def restore_files(self, ws_names):
        for ws_name in ws_names:
            try:
                ws = ADS.retrieve(ws_name)
                if ws.getNumberHistograms() == 1:
                    self._loaded_workspaces[ws_name] = ws
                    if self._bg_params[ws_name]:
                        self._bg_sub_workspaces[ws_name] = ADS.retrieve(ws_name + "_bgsub")
                    else:
                        self._bg_sub_workspaces[ws_name] = None
                    if ws_name not in self._bg_params:
                        self._bg_params[ws_name] = []
                    self._last_added.append(ws_name)
                else:
                    logger.warning(
                        f"Invalid number of spectra in workspace {ws_name}. Skipping restoration of workspace.")
            except RuntimeError as e:
                logger.error(
                    f"Failed to restore workspace: {ws_name}. Error: {e}. \n Continuing loading of other files.")
        self.update_log_workspace_group()

    def load_files(self, filenames_string, xunit):
        self._last_added = []
        filenames = [name.strip() for name in filenames_string.split(",")]
        for filename in filenames:
            ws_name = self._generate_workspace_name(filename, xunit)
            if ws_name not in self._loaded_workspaces:
                try:
                    if not ADS.doesExist(ws_name):
                        ws = Load(filename, OutputWorkspace=ws_name)
                        # temporary fix to ensure unit of ws matches unit box (which will soon be removed)
                        ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws_name, Target=xunit)
                    else:
                        ws = ADS.retrieve(ws_name)
                    if ws.getNumberHistograms() == 1:
                        self._loaded_workspaces[ws_name] = ws
                        if ws_name not in self._bg_sub_workspaces:
                            self._bg_sub_workspaces[ws_name] = None
                        if ws_name not in self._bg_params:
                            self._bg_params[ws_name] = []
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
        currentRunLogs = [l.name for l in run.getLogData()]
        nullLogValue = full(2, nan)  # default nan if can't read/average log data
        if run.getProtonCharge() > 0 and "proton_charge" in currentRunLogs:
            for log in self._log_names:
                if log in self._log_values[ws_name]:
                    avg, stdev = self._log_values[ws_name][log]  # already averaged
                elif log in currentRunLogs:
                    avg, stdev = AverageLogData(ws_name, LogName=log, FixZero=False)
                else:
                    avg, stdev = nullLogValue
                self._log_values[ws_name][log] = [avg, stdev]  # update model dict (even if nan)
        else:
            self._log_values[ws_name] = {log: nullLogValue for log in self._log_names}
            logger.warning(f"{ws.name()} does not contain a proton charge log - log values cannot be averaged.")

        # write log values to table (nan if log could not be averaged)
        for log, avg_and_stdev in self._log_values[ws_name].items():
            self.write_table_row(ADS.retrieve(log), avg_and_stdev, irow)
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

    def get_ws_list(self):
        return list(self._loaded_workspaces.keys())

    def get_ws_sorted_by_primary_log(self):
        ws_list = self.get_ws_list()
        tof_ws_inds = [ind for ind, ws in enumerate(ws_list) if
                       self._loaded_workspaces[ws].getAxis(0).getUnit().caption() == 'Time-of-flight']
        primary_log = get_setting(path_handling.INTERFACES_SETTINGS_GROUP, path_handling.ENGINEERING_PREFIX,
                                  "primary_log")
        sort_ascending = get_setting(path_handling.INTERFACES_SETTINGS_GROUP, path_handling.ENGINEERING_PREFIX,
                                     "sort_ascending")
        if primary_log:
            log_table = ADS.retrieve(primary_log)
            isort = argsort(array(log_table.column('avg')))
            ws_list_tof = [ws_list[iws] for iws in isort if iws in tof_ws_inds]
        else:
            ws_list_tof = ws_list
        if sort_ascending == 'false':
            # settings can only be saved as text
            ws_list_tof = ws_list_tof[::-1]
        return ws_list_tof

    def update_fit(self, fit_props):
        for fit_prop in fit_props:
            wsname = fit_prop['properties']['InputWorkspace']
            self._fit_results[wsname] = {'model': fit_prop['properties']['Function'],
                                         'status': fit_prop['status']}
            self._fit_results[wsname]['results'] = defaultdict(list)  # {function_param: [[Y1, E1], [Y2,E2],...] }
            self._fit_results[wsname]
            fnames = [x.split('=')[-1] for x in findall('name=[^,]*', fit_prop['properties']['Function'])]
            # get num params for each function (first elem empty as str begins with 'name=')
            # need to remove ties and constraints which are enclosed in ()
            nparams = [s.count('=') for s in
                       sub(r'=\([^)]*\)', '', fit_prop['properties']['Function']).split('name=')[1:]]
            params_dict = ADS.retrieve(fit_prop['properties']['Output'] + '_Parameters').toDict()
            # loop over rows in output workspace to get value and error for each parameter
            istart = 0
            for ifunc, fname in enumerate(fnames):
                for iparam in range(0, nparams[ifunc]):
                    irow = istart + iparam
                    key = '_'.join([fname, params_dict['Name'][irow].split('.')[-1]])  # funcname_param
                    self._fit_results[wsname]['results'][key].append([
                        params_dict['Value'][irow], params_dict['Error'][irow]])
                    if key in fit_prop['peak_centre_params']:
                        # param corresponds to a peak centre in TOF which we also need in dspacing
                        # add another entry into the results dictionary
                        key_d = key + "_dSpacing"
                        dcen = self._convert_TOF_to_d(params_dict['Value'][irow], wsname)
                        dcen_er = self._convert_TOFerror_to_derror(params_dict['Error'][irow], dcen, wsname)
                        self._fit_results[wsname]['results'][key_d].append([dcen, dcen_er])
                istart += nparams[ifunc]
            # append the cost function value (in this case always chisq/DOF) as don't let user change cost func
            # always last row in parameters table
            self._fit_results[wsname]['costFunction'] = params_dict['Value'][-1]
        self.create_fit_tables()

    def create_fit_tables(self):
        wslist = []  # ws to be grouped
        # extract fit parameters and errors
        nruns = len(self._loaded_workspaces.keys())  # num of rows of output workspace
        # get unique set of function parameters across all workspaces
        func_params = set(chain(*[list(d['results'].keys()) for d in self._fit_results.values()]))
        for param in func_params:
            # get max number of repeated func in a model (num columns of output workspace)
            nfuncs = max([len(d['results'][param]) for d in self._fit_results.values() if param in d['results']])
            # make output workspace
            ipeak = list(range(1, nfuncs + 1)) * nruns
            ws = CreateWorkspace(OutputWorkspace=param, DataX=ipeak, DataY=ipeak, NSpec=nruns)
            # axis for labels in workspace
            axis = TextAxis.create(nruns)
            for iws, wsname in enumerate(self._loaded_workspaces.keys()):
                wsname_bgsub = wsname + "_bgsub"
                if wsname_bgsub in self._fit_results:
                    wsname = wsname_bgsub
                if wsname in self._fit_results and param in self._fit_results[wsname]['results']:
                    fitvals = array(self._fit_results[wsname]['results'][param])
                    data = vstack((fitvals, full((nfuncs - fitvals.shape[0], 2), nan)))
                else:
                    data = full((nfuncs, 2), nan)
                ws.setY(iws, data[:, 0])
                ws.setE(iws, data[:, 1])
                # label row
                axis.setLabel(iws, wsname)
            ws.replaceAxis(1, axis)
            wslist += [ws]
        # table for model summary/info
        model = CreateEmptyTableWorkspace(OutputWorkspace='model')
        model.addColumn(type="str", name="Workspace")
        model.addColumn(type="float", name="chisq/DOF")  # always is for LM minimiser (users can't change)
        model.addColumn(type="str", name="status")
        model.addColumn(type="str", name="Model")
        for iws, wsname in enumerate(self._loaded_workspaces.keys()):
            if wsname in self._fit_results:
                row = [wsname, self._fit_results[wsname]['costFunction'],
                       self._fit_results[wsname]['status'], self._fit_results[wsname]['model']]
                self.write_table_row(model, row, iws)
            else:
                self.write_table_row(model, ['', nan, ''], iws)
        wslist += [model]
        group_name = self._log_workspaces.name().split('_log')[0] + '_fits'
        self._fit_workspaces = GroupWorkspaces(wslist, OutputWorkspace=group_name)

    def update_workspace_name(self, old_name, new_name):
        if new_name not in self._loaded_workspaces:
            self._loaded_workspaces[new_name] = self._loaded_workspaces.pop(old_name)
            if old_name in self._bg_sub_workspaces:
                self._bg_sub_workspaces[new_name] = self._bg_sub_workspaces.pop(old_name)
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

    def get_bgsub_workspaces(self):
        return self._bg_sub_workspaces

    def get_bg_params(self):
        return self._bg_params

    def get_fit_results(self):
        return self._fit_results

    def create_or_update_bgsub_ws(self, ws_name, bg_params):
        ws = self._loaded_workspaces[ws_name]
        ws_bg = self._bg_sub_workspaces[ws_name]
        if not ws_bg or self._bg_params[ws_name] == [] or bg_params[1:] != self._bg_params[ws_name][1:]:
            background = self.estimate_background(ws_name, *bg_params[1:])
            self._bg_params[ws_name] = bg_params
            bgsub_ws_name = ws_name + "_bgsub"
            bgsub_ws = Minus(LHSWorkspace=ws, RHSWorkspace=background, OutputWorkspace=bgsub_ws_name)
            self._bg_sub_workspaces[ws_name] = bgsub_ws
            DeleteWorkspace(background)
        else:
            logger.notice("Background workspace already calculated")

    def estimate_background(self, ws_name, niter, xwindow, doSGfilter):
        ws_bg = EnggEstimateFocussedBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name + "_bg",
                                               NIterations=niter, XWindow=xwindow, ApplyFilterSG=doSGfilter)
        return ws_bg

    def plot_background_figure(self, ws_name):
        ws = self._loaded_workspaces[ws_name]
        ws_bgsub = self._bg_sub_workspaces[ws_name]
        if ws_bgsub:
            fig, ax = subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [2, 1]},
                               subplot_kw={'projection': 'mantid'})
            bg = Minus(LHSWorkspace=ws_name, RHSWorkspace=ws_bgsub, StoreInADS=False)
            ax[0].plot(ws, 'x')
            ax[1].plot(ws_bgsub, 'x')
            ax[0].plot(bg, '-r')
            fig.show()

    def get_last_added(self):
        return self._last_added

    def get_sample_log_from_ws(self, ws_name, log_name):
        return self._loaded_workspaces[ws_name].getSampleDetails().getLogData(log_name).value

    def _convert_TOF_to_d(self, tof, ws_name):
        diff_consts = self._get_diff_constants(ws_name)
        return UnitConversion.run("TOF", "dSpacing", tof, 0, DeltaEModeType.Elastic, diff_consts)  # L1=0 (ignored)

    def _convert_TOFerror_to_derror(self, tof_error, d, ws_name):
        diff_consts = self._get_diff_constants(ws_name)
        difc = diff_consts[UnitParams.difc]
        difa = diff_consts[UnitParams.difa] if UnitParams.difa in diff_consts else 0
        return tof_error / (2 * difa * d + difc)

    def _get_diff_constants(self, ws_name):
        """
        Get diffractometer constants from workspace
        TOF = difc*d + difa*(d^2) + tzero
        """
        ws = ADS.retrieve(ws_name)
        si = ws.spectrumInfo()
        diff_consts = si.diffractometerConstants(0)  # output is a UnitParametersMap
        return diff_consts

    @staticmethod
    def write_table_row(ws_table, row, irow):
        if irow > ws_table.rowCount() - 1:
            ws_table.setRowCount(irow + 1)
        [ws_table.setCell(irow, icol, row[icol]) for icol in range(0, len(row))]

    @staticmethod
    def _generate_workspace_name(filepath, xunit):
        wsname = path.splitext(path.split(filepath)[1])[0]
        # remove unit from fname if present as will convert unit to xunit in combo box temporarily until it is removed
        # Once combo box removed we can get unit from workspace post-loading (and call RenameWorkspace)
        if wsname.endswith('_TOF') or wsname.endswith('_dSpacing'):
            wsname = '_'.join(wsname.split('_')[0:-1])
        return wsname + '_' + xunit
