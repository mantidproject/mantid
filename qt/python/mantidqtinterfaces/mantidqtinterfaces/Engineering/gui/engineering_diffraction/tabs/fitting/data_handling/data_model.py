# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import Load, logger, CreateEmptyTableWorkspace, GroupWorkspaces, DeleteWorkspace
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import \
    SampleLogsGroupWorkspace, write_table_row, _generate_workspace_name
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.workspace_record import \
    FittingWorkspaceRecordContainer
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
        self._fit_results = {}  # {WorkspaceName: fit_result_dict}
        self._fit_workspaces = None
        self._last_added = []  # List of workspace names loaded in the last load action.
        self._data_workspaces = FittingWorkspaceRecordContainer()
        self._sample_logs_workspace_group = SampleLogsGroupWorkspace()

    def restore_files(self, ws_names):
        self._data_workspaces.add_from_names_dict(ws_names)
        for ws_name in ws_names:
            try:
                ws = ADS.retrieve(ws_name)
                if ws.getNumberHistograms() == 1:
                    bgsubws = None
                    if self._data_workspaces[ws_name].bg_params:
                        bgsubws= ADS.retrieve(self._data_workspaces[ws_name].bgsub_ws_name)
                    self._last_added.append(ws_name)
                    self._data_workspaces[ws_name].loaded_ws = ws
                    self._data_workspaces[ws_name].bgsub_ws = bgsubws
                else:
                    logger.warning(
                        f"Invalid number of spectra in workspace {ws_name}. Skipping restoration of workspace.")
            except RuntimeError as e:
                logger.error(
                    f"Failed to restore workspace: {ws_name}. Error: {e}. \n Continuing loading of other files.")
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def load_files(self, filenames_string):
        self._last_added = []
        filenames = [name.strip() for name in filenames_string.split(",")]
        for filename in filenames:
            ws_name = _generate_workspace_name(filename)
            if ws_name not in self._data_workspaces.get_loaded_workpace_names():
                try:
                    if not ADS.doesExist(ws_name):
                        ws = Load(filename, OutputWorkspace=ws_name)
                    else:
                        ws = ADS.retrieve(ws_name)
                    if ws.getNumberHistograms() == 1:
                        self._last_added.append(ws_name)
                        self._data_workspaces.add(ws_name, loaded_ws=ws)
                    else:
                        logger.warning(
                            f"Invalid number of spectra in workspace {ws_name}. Skipping loading of file.")
                except RuntimeError as e:
                    logger.error(
                        f"Failed to load file: {filename}. Error: {e}. \n Continuing loading of other files.")
            else:
                logger.warning(f"File {ws_name} has already been loaded")
        self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    def get_loaded_ws_list(self):
        return list(self._data_workspaces.get_loaded_ws_dict().keys())

    def get_active_ws_name_list(self):
        return self._data_workspaces.get_active_ws_name_list()

    def get_active_ws(self, loaded_ws_name):
        return self._data_workspaces[loaded_ws_name].get_active_ws()

    def get_active_ws_name(self, loaded_ws_name):
        return self._data_workspaces.get_active_ws_name(loaded_ws_name)

    def get_active_ws_sorted_by_primary_log(self):
        active_ws_dict = self._data_workspaces.get_active_ws_dict()
        tof_ws_inds = [ind for ind, ws in enumerate(active_ws_dict.values()) if
                       ws.getAxis(0).getUnit().caption() == 'Time-of-flight']
        primary_log = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                                  "primary_log")
        sort_ascending = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                                     "sort_ascending")
        ws_name_list = list(active_ws_dict.keys())
        if primary_log:
            log_table = ADS.retrieve(primary_log)
            isort = argsort(array(log_table.column('avg')))
            ws_list_tof = [ws_name_list[iws] for iws in isort if iws in tof_ws_inds]
        else:
            ws_list_tof = ws_name_list
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
                        try:
                            dcen = self._convert_TOF_to_d(params_dict['Value'][irow], wsname)
                            dcen_er = self._convert_TOFerror_to_derror(params_dict['Error'][irow], dcen, wsname)
                            self._fit_results[wsname]['results'][key_d].append([dcen, dcen_er])
                        except (ValueError, RuntimeError) as e:
                            logger.warning(f"Unable to output {key_d} parameters for TOF={params_dict['Value'][irow]}: " + str(e))
                istart += nparams[ifunc]
            # append the cost function value (in this case always chisq/DOF) as don't let user change cost func
            # always last row in parameters table
            self._fit_results[wsname]['costFunction'] = params_dict['Value'][-1]
        self.create_fit_tables()

    def create_fit_tables(self):
        wslist = []  # ws to be grouped
        # extract fit parameters and errors
        nruns = len(self.get_loaded_ws_list())  # num of rows of output workspace
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
            for iws, wsname in enumerate(self.get_active_ws_name_list()):
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
        for iws, wsname in enumerate(self.get_active_ws_name_list()):
            if wsname in self._fit_results:
                row = [wsname, self._fit_results[wsname]['costFunction'],
                       self._fit_results[wsname]['status'], self._fit_results[wsname]['model']]
                write_table_row(model, row, iws)
            else:
                write_table_row(model, ['', nan, ''], iws)
        wslist += [model]
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        group_name = current_log_workspaces.name().split('_log')[0] + '_fits'
        self._fit_workspaces = GroupWorkspaces(wslist, OutputWorkspace=group_name)

    # handle ADS remove. name workspace has already been deleted
    def remove_workspace(self, name):
        ws_loaded = self._data_workspaces.get(name, None)
        if ws_loaded:
            bgsub_ws_name = self._data_workspaces[name].bgsub_ws_name
            removed = self._data_workspaces.pop(name).loaded_ws
            # deleting bg sub workspace will generate further remove_workspace event so ensure this is done after
            # removing record from _data_workspaces to avoid circular call
            if bgsub_ws_name:
                DeleteWorkspace(bgsub_ws_name)
            self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)
            return removed
        else:
            ws_loaded_name = self._data_workspaces.get_loaded_workspace_name_from_bgsub(name)
            if ws_loaded_name:
                removed = self._data_workspaces[ws_loaded_name].bgsub_ws
                self._data_workspaces[ws_loaded_name].bgsub_ws = None
                self._data_workspaces[ws_loaded_name].bgsub_ws_name = None
                self._data_workspaces[ws_loaded_name].bg_params = []
                return removed

    def replace_workspace(self, name, workspace):
        self._data_workspaces.replace_workspace(name, workspace)

    def update_workspace_name(self, old_name, new_name):
        if new_name not in self.get_all_workspace_names():
            self._data_workspaces.rename(old_name, new_name)
            current_log_values = self._sample_logs_workspace_group.get_log_values()
            if old_name in current_log_values:
                self._sample_logs_workspace_group.update_log_value(new_key=new_name, old_key=old_name)
        else:
            logger.warning(f"There already exists a workspace with name {new_name}.")
            self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)

    # handle ADS clear
    def clear_workspaces(self):
        self._data_workspaces.clear()
        self.set_log_workspaces_none()

    def delete_workspaces(self):
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        if current_log_workspaces:
            ws_name = current_log_workspaces.name()
            self._sample_logs_workspace_group.clear_log_workspaces()
            DeleteWorkspace(ws_name)
        removed_ws_list = []
        for ws_name in self._data_workspaces.get_loaded_workpace_names():
            removed_ws_list.extend(self.delete_workspace(ws_name))
        return removed_ws_list

    def delete_workspace(self, loaded_ws_name):
        removed = self._data_workspaces.pop(loaded_ws_name)
        removed_ws_list = [removed.loaded_ws]
        DeleteWorkspace(removed.loaded_ws)
        if removed.bgsub_ws:
            DeleteWorkspace(removed.bgsub_ws)
            removed_ws_list.append(removed.bgsub_ws)
            self._sample_logs_workspace_group.update_log_workspace_group(self._data_workspaces)
        return removed_ws_list

    def get_loaded_workspaces(self):
        return self._data_workspaces.get_loaded_ws_dict()

    def get_all_workspace_names(self):
        return self._data_workspaces.get_loaded_workpace_names() + self._data_workspaces.get_bgsub_workpace_names()

    def get_log_workspaces_name(self):
        current_log_workspaces = self._sample_logs_workspace_group.get_log_workspaces()
        return [ws.name() for ws in current_log_workspaces] if current_log_workspaces else ''

    def get_bgsub_workspaces(self):
        return self._data_workspaces.get_bgsub_ws_dict()

    def get_bgsub_workspace_names(self):
        return self._data_workspaces.get_bgsub_ws_name_dict()

    def get_bg_params(self):
        return self._data_workspaces.get_bg_params_dict()

    def get_fit_results(self):
        return self._fit_results

    def create_or_update_bgsub_ws(self, ws_name, bg_params):
        ws = self._data_workspaces[ws_name].loaded_ws
        ws_bg = self._data_workspaces[ws_name].bgsub_ws
        ws_bg_params = self._data_workspaces[ws_name].bg_params
        if not ws_bg or ws_bg_params == [] or bg_params[1:] != ws_bg_params[1:]:
            background = self.estimate_background(ws_name, *bg_params[1:])
            self._data_workspaces[ws_name].bg_params = bg_params
            bgsub_ws_name = ws_name + "_bgsub"
            bgsub_ws = Minus(LHSWorkspace=ws, RHSWorkspace=background, OutputWorkspace=bgsub_ws_name)
            self._data_workspaces[ws_name].bgsub_ws = bgsub_ws
            self._data_workspaces[ws_name].bgsub_ws_name = bgsub_ws_name
            DeleteWorkspace(background)
        else:
            logger.notice("Background workspace already calculated")

    def update_bgsub_status(self, ws_name, status):
        if self._data_workspaces[ws_name].bg_params:
            self._data_workspaces[ws_name].bg_params[0] = status

    def estimate_background(self, ws_name, niter, xwindow, doSGfilter):
        try:
            ws_bg = EnggEstimateFocussedBackground(InputWorkspace=ws_name, OutputWorkspace=ws_name + "_bg",
                                                   NIterations=niter, XWindow=xwindow, ApplyFilterSG=doSGfilter)
        except (ValueError, RuntimeError) as e:
            # ValueError when Niter not positive integer, RuntimeError when Window too small
            logger.error("Error on arguments supplied to EnggEstimateFocusedBackground: " + str(e))
            ws_bg = SetUncertainties(InputWorkspace=ws_name)  # copy data and zero errors
            ws_bg = Minus(LHSWorkspace=ws_bg, RHSWorkspace=ws_bg)  # workspace of zeros with same num spectra
        return ws_bg

    def plot_background_figure(self, ws_name):
        def on_draw(event):
            if event.canvas.signalsBlocked():
                # This stops infinite loop as draw() is called within this handle (and set signalsBlocked == True)
                # Resets signalsBlocked to False (default value)
                event.canvas.blockSignals(False)
            else:
                axes = event.canvas.figure.get_axes()
                data_line = next((line for line in axes[0].get_tracked_artists()), None)
                bg_line = next((line for line in axes[0].get_lines() if line not in axes[0].get_tracked_artists()),
                               None)
                bgsub_line = next((line for line in axes[1].get_tracked_artists()), None)
                if data_line and bg_line and bgsub_line:
                    event.canvas.blockSignals(True)  # this doesn't stop this handle being called again on canvas.draw()
                    bg_line.set_ydata(data_line.get_ydata() - bgsub_line.get_ydata())
                    event.canvas.draw()
                else:
                    # would like to close the fig at this point but this interferes with the mantid ADS observers when
                    # any of the tracked workspaces are deleted and causes mantid to hard crash - so just print warning
                    logger.warning(f"Inspect background figure {event.canvas.figure.number} has been invalidated - the "
                                   f"background curve will no longer be updated.")

        ws = self._data_workspaces[ws_name].loaded_ws
        ws_bgsub = self._data_workspaces[ws_name].bgsub_ws
        if ws_bgsub:
            fig, ax = subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [2, 1]},
                               subplot_kw={'projection': 'mantid'})
            bg = Minus(LHSWorkspace=ws_name, RHSWorkspace=ws_bgsub, StoreInADS=False)
            ax[0].plot(ws, 'x')
            ax[1].plot(ws_bgsub, 'x', label='background subtracted data')
            ax[0].plot(bg, '-r', label='background')
            ax[0].legend(fontsize=8.0)
            ax[1].legend(fontsize=8.0)
            fig.canvas.mpl_connect("draw_event", on_draw)
            fig.show()

    def get_last_added(self):
        return self._last_added

    def get_sample_log_from_ws(self, ws_name, log_name):
        return self._data_workspaces[ws_name].loaded_ws.getSampleDetails().getLogData(log_name).value

    def set_log_workspaces_none(self):
        # to be used in the event of Ads clear, as trying to reference the deleted grp ws results in an error
        self._sample_logs_workspace_group.clear_log_workspaces()

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
