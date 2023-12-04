# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from itertools import chain
from numpy import full, nan, max, array, vstack
from collections import defaultdict

from mantid import FunctionFactory
from mantid.api import TextAxis
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParams
from mantid.simpleapi import logger, CreateEmptyTableWorkspace, GroupWorkspaces, CreateWorkspace
from mantid.api import AnalysisDataService as ADS
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import write_table_row
from mantid.api import CompositeFunction


class FittingPlotModel(object):
    def __init__(self):
        self.plotted_workspaces = set()
        self._fit_results = {}  # {WorkspaceName: fit_result_dict}
        self._fit_workspaces = None

    # ===============
    # Plotting
    # ===============

    def get_plotted_workspaces(self):
        return self.plotted_workspaces

    def add_workspace_to_plot(self, ws, ax, plot_kwargs):
        ax.plot(ws, **plot_kwargs)
        self.plotted_workspaces.add(ws)

    def remove_workspace_from_plot(self, ws, ax):
        if ws in self.plotted_workspaces:
            self._remove_workspace_from_plot(ws, ax)
            self.plotted_workspaces.remove(ws)
        if not self.plotted_workspaces:
            ax.cla()

    @staticmethod
    def _remove_workspace_from_plot(ws, ax):
        ax.remove_workspace_artists(ws)

    def remove_all_workspaces_from_plot(self, ax):
        ax.cla()
        self.plotted_workspaces.clear()

    # ===============
    # Fitting
    # ===============

    def update_fit(self, fit_props, loaded_ws_list, active_ws_list, log_workspace_name):
        for fit_prop in fit_props:
            wsname = fit_prop["properties"]["InputWorkspace"]
            self._fit_results[wsname] = {"model": fit_prop["properties"]["Function"], "status": fit_prop["status"]}
            self._fit_results[wsname]["results"] = defaultdict(list)  # {function_param: [[Y1, E1], [Y2, E2],...] }
            fit_functions = FunctionFactory.createInitialized(fit_prop["properties"]["Function"])
            if isinstance(fit_functions, CompositeFunction):
                fnames = [func.name() for func in fit_functions]
                nparams = [func.nParams() for func in fit_functions]
            else:
                fnames = [fit_functions.name()]
                nparams = [fit_functions.nParams()]
            params_dict = ADS.retrieve(fit_prop["properties"]["Output"] + "_Parameters").toDict()
            # loop over rows in output workspace to get value and error for each parameter
            istart = 0
            for ifunc, fname in enumerate(fnames):
                for iparam in range(0, nparams[ifunc]):
                    irow = istart + iparam
                    key = "_".join([fname, params_dict["Name"][irow].split(".")[-1]])  # funcname_param
                    self._fit_results[wsname]["results"][key].append([params_dict["Value"][irow], params_dict["Error"][irow]])
                    if key in fit_prop["peak_centre_params"]:
                        # param corresponds to a peak centre in TOF which we also need in dspacing
                        # add another entry into the results dictionary
                        key_d = key + "_dSpacing"
                        try:
                            dcen = self._convert_TOF_to_d(params_dict["Value"][irow], wsname)
                            dcen_er = self._convert_TOFerror_to_derror(params_dict["Error"][irow], dcen, wsname)
                            self._fit_results[wsname]["results"][key_d].append([dcen, dcen_er])
                        except (ValueError, RuntimeError) as e:
                            logger.warning(f"Unable to output {key_d} parameters for TOF={params_dict['Value'][irow]}: " + str(e))
                istart += nparams[ifunc]
            # append the cost function value (in this case always chisq/DOF) as don't let user change cost func
            # always last row in parameters table
            self._fit_results[wsname]["costFunction"] = params_dict["Value"][-1]
            self._calculate_fwhm_values(wsname, fit_functions)
        self.create_fit_tables(loaded_ws_list, active_ws_list, log_workspace_name)

    def _setup_peak_func_and_extract_fwhm(self, ws_name, peak_func_names, fit_func):
        fit_func_name = fit_func.name()
        if fit_func_name in peak_func_names:
            peak_func = FunctionFactory.Instance().createPeakFunction(fit_func_name)
            [peak_func.setParameter(i_param, fit_func.getParameterValue(i_param)) for i_param in range(fit_func.nParams())]
            key_fwhm = fit_func_name + "_fwhm"
            self._fit_results[ws_name]["results"][key_fwhm].append([peak_func.fwhm(), 0.0])

    def _calculate_fwhm_values(self, ws_name, fit_functions):
        peak_func_names = FunctionFactory.getPeakFunctionNames()
        if isinstance(fit_functions, CompositeFunction):
            for func in fit_functions:
                self._setup_peak_func_and_extract_fwhm(ws_name, peak_func_names, func)
        else:
            self._setup_peak_func_and_extract_fwhm(ws_name, peak_func_names, fit_functions)

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

    def create_fit_tables(self, loaded_ws_list, active_ws_list, log_workspace_name):
        wslist = []  # ws to be grouped
        # extract fit parameters and errors
        nruns = len(loaded_ws_list)  # num of rows of output workspace
        # get unique set of function parameters across all workspaces
        func_params = set(chain(*[list(d["results"].keys()) for d in self._fit_results.values()]))
        for param in func_params:
            # get max number of repeated func in a model (num columns of output workspace)
            nfuncs = max([len(d["results"][param]) for d in self._fit_results.values() if param in d["results"]])
            # make output workspace
            ipeak = list(range(1, nfuncs + 1)) * nruns
            ws = CreateWorkspace(OutputWorkspace=param, DataX=ipeak, DataY=ipeak, NSpec=nruns)
            # axis for labels in workspace
            axis = TextAxis.create(nruns)
            for iws, wsname in enumerate(active_ws_list):
                if wsname in self._fit_results and param in self._fit_results[wsname]["results"]:
                    fitvals = array(self._fit_results[wsname]["results"][param])
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
        model = CreateEmptyTableWorkspace(OutputWorkspace="model")
        model.addColumn(type="str", name="Workspace")
        model.addColumn(type="float", name="chisq/DOF")  # always is for LM minimiser (users can't change)
        model.addColumn(type="str", name="status")
        model.addColumn(type="str", name="Model")
        for iws, wsname in enumerate(active_ws_list):
            if wsname in self._fit_results:
                row = [
                    wsname,
                    self._fit_results[wsname]["costFunction"],
                    self._fit_results[wsname]["status"],
                    self._fit_results[wsname]["model"],
                ]
                write_table_row(model, row, iws)
            else:
                write_table_row(model, ["", nan, ""], iws)
        wslist += [model]
        group_name = log_workspace_name.split("_log")[0] + "_fits"
        self._fit_workspaces = GroupWorkspaces(wslist, OutputWorkspace=group_name)

    def get_fit_results(self):
        return self._fit_results
