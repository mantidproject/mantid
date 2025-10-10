# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from itertools import chain
from numpy import full, nan, max, array, vstack, mean, round
from collections import defaultdict

from mantid import FunctionFactory
from mantid.api import TextAxis
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParams
from mantid.simpleapi import logger, CreateEmptyTableWorkspace, GroupWorkspaces, CreateWorkspace, FindPeaksConvolve, SaveNexus
from mantid.api import AnalysisDataService as ADS
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs import write_table_row
from mantid.api import CompositeFunction
from mantid.fitfunctions import FunctionWrapper
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import wsname_in_instr_run_ceria_group_ispec_unit_format
from os import path, makedirs


class FittingPlotModel(object):
    def __init__(self):
        self.plotted_workspaces = set()
        self._fit_results = {}  # {WorkspaceName: fit_result_dict}
        self._fit_workspaces = None
        self._rb_num = None

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
            ws_is_tof = self._ws_is_tof(wsname)
            # loop over rows in output workspace to get value and error for each parameter
            istart = 0
            for ifunc, fname in enumerate(fnames):
                for iparam in range(0, nparams[ifunc]):
                    irow = istart + iparam
                    key = "_".join([fname, params_dict["Name"][irow].split(".")[-1]])  # funcname_param
                    self._fit_results[wsname]["results"][key].append([params_dict["Value"][irow], params_dict["Error"][irow]])
                    if key in fit_prop["peak_centre_params"] and ws_is_tof:
                        # param corresponds to a peak centre check if units are TOF
                        # if so add another entry into the results dictionary in dSpacing
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
        if fit_func.hasParameter("FWHM"):
            return  # Avoid re-calculating of FWHM if the fit function already has it

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
        summary_tables = self.create_bank_fit_summary_tables_by_run(self.get_fit_results(), active_ws_list)
        wslist.extend(summary_tables.values())

    def create_bank_fit_summary_tables_by_run(self, fit_results, active_ws_list):
        """
        Create one TableWorkspace per run, summarizing fit parameters for each bank.
        For use in Texture Analysis Pipeline
        Includes cost function as 'chi2' column. If only one function is used, column names are simplified.

        :param fit_results: Dictionary of fit results (usually self._fit_results)
        :param active_ws_list: List of active workspace names
        :return: Dict of {run_number: TableWorkspace}
        """
        # get dictionary correspondence between runs and banks/prefix/group with k:v structures defined below
        # run: [(one_spec, ws name), (another_spec, ws name)...]
        # run: prefix_string
        # run: group
        run_to_banks, run_to_prefix, run_to_group = self._create_metadata_dicts(active_ws_list, fit_results)

        # create a dict to store summary tables
        summary_tables = {}
        for run, bank_ws_pairs in run_to_banks.items():
            # sort the spectra in order of ispec
            bank_ws_pairs.sort(key=lambda x: int(x[0]))
            banks_sorted = [ws for _, ws in bank_ws_pairs]

            # extract fit parameters
            all_params = sorted(set(param for ws in banks_sorted if ws in fit_results for param in fit_results[ws]["results"].keys()))

            # determine if there are any duplicate functions (if there aren't just use eg. X0 rather than f1_X0)
            func_names = {param.split("_")[0] for param in all_params if "_" in param}
            use_short_names = len(func_names) == 1

            # make a list of parameter labels and get a unique "peak" label
            param_labels, fit_peak = self._get_param_labels_and_peak_label(all_params, use_short_names, fit_results, banks_sorted)

            # read metadata for run
            prefix = run_to_prefix[run]
            grouping = run_to_group[run]
            table_name = f"{prefix}{run}_{fit_peak}_{grouping}_Fit_Parameters"

            # create a table with the correct columns and data types
            table = self.create_texture_output_table(table_name, param_labels)

            # populate table
            for ws in banks_sorted:
                row = [ws]
                for param in all_params:
                    if param in fit_results.get(ws, {}).get("results", {}):
                        val_err = fit_results[ws]["results"][param][0]
                        row.extend(val_err)
                    else:
                        row.extend([nan, nan])

                chi2 = fit_results.get(ws, {}).get("costFunction", nan)
                row.append(chi2)

                table.addRow(row)

            self._save_files(table_name, "FitParameters", grouping)
            summary_tables[run] = table

        return summary_tables

    @staticmethod
    def create_texture_output_table(table_name, param_labels):
        table = CreateEmptyTableWorkspace(OutputWorkspace=table_name)
        table.addColumn("str", "Bank")

        for param_label in param_labels:
            table.addColumn("double", param_label)
            table.addColumn("double", f"{param_label}_Error")

        # Add chi2 column
        table.addColumn("double", "chi2")

        return table

    @staticmethod
    def _get_param_labels_and_peak_label(all_params, use_short_names, fit_results, banks_sorted):
        param_labels = []
        fit_peak = ""
        for param in all_params:
            if use_short_names and "_" in param:
                param_label = param.split("_", 1)[1]
                param_labels.append(param_label)
                # if we only have one X0 in the table, we would like a unique file name
                if param_label == "X0":
                    x0s = [fit_results[ws]["results"][param][0][0] for ws in banks_sorted]
                    fit_peak = str(round(mean(x0s), 2))
            else:
                param_labels.append(param)
        return param_labels, fit_peak

    @staticmethod
    def _create_metadata_dicts(active_ws_list, fit_results):
        print(active_ws_list)
        run_to_banks = defaultdict(list)
        run_to_prefix = {}
        run_to_group = {}

        for wsname in active_ws_list:
            if wsname in fit_results.keys():
                if wsname_in_instr_run_ceria_group_ispec_unit_format(wsname):
                    # ws generated through the GUI pipeline should have names with all the metadata present
                    # (due to shared run objects for ws of extracted single spectra this is more reliable than the logs)
                    prefix, run_number, _, grouping, bank_id, *_ = wsname.split("_")
                    prefix += "_"
                else:
                    # if the name isn't in expected format, try and extract the metadata from the logs
                    # failing that, just set defaults
                    try:
                        ws = ADS.retrieve(wsname)
                        run_number = str(ws.getRun().getLogData("run_number").value)
                        prefix = wsname.split(run_number)[0]
                        bank_id = int(ws.getRun().getLogData("bankid").value.split()[-1])  # e.g., "group 1"
                    except Exception:
                        run_number = "unknown"
                        prefix = ""
                        bank_id = 9999
                    try:
                        ws = ADS.retrieve(wsname)
                        grouping = str(ws.getRun().getLogData("Grouping").value)
                    except RuntimeError:
                        grouping = ""

                run_to_banks[run_number].append((bank_id, wsname))
                run_to_prefix[run_number] = prefix
                run_to_group[run_number] = grouping

        return run_to_banks, run_to_prefix, run_to_group

    def _save_files(self, ws, dir_name, peak, grouping=""):
        root_dir = output_settings.get_output_path()
        save_dirs = [path.join(root_dir, dir_name, peak)]
        if self._rb_num:
            if grouping == "":
                # grouping is "" if we can't read group from log data - in this case we don't provide separate dir
                save_dirs.append(path.join(root_dir, "User", self._rb_num, dir_name, peak))
            else:
                # otherwise it is convenient for the user to have separate directories
                save_dirs.append(path.join(root_dir, "User", self._rb_num, dir_name, grouping, peak))
        for save_dir in save_dirs:
            if not path.exists(save_dir):
                makedirs(save_dir)
            SaveNexus(InputWorkspace=ws, Filename=path.join(save_dir, ws + ".nxs"))

    def get_fit_results(self):
        return self._fit_results

    def run_find_peaks_convolve(self, input_ws_name, peak_type, x_range):
        """
        Run FindPeaksConvolve algorithm with default values for ENGIN-X
        :param input_ws_name: Name of the input workspace to run against
        :param peak_type: Name of the selected peak type
        :param x_range: min and max values of the fitting range
        :return string: Function wrapper string for the peaks detected via the algorithm, None otherwise
        """
        if not ADS.doesExist(input_ws_name):
            logger.error(f"{input_ws_name} does not exist!")
            return None
        fit_prop_ws = ADS.retrieve(input_ws_name)
        groupWs = FindPeaksConvolve(
            InputWorkspace=fit_prop_ws,
            OutputWorkspace="FindPeaksConvolve_" + input_ws_name,
            IOverSigmaThreshold=10,
            EstimatedPeakExtent=200,
            StoreInADS=True,
        )
        peak_x_values = dict()
        peak_y_values = dict()
        for tab_ws in groupWs:
            if tab_ws.getName() == "PeakCentre":
                peak_x_values = self._re_organize_keys_find_peaks_convolve(tab_ws)
            elif tab_ws.getName() == "PeakYPosition":
                peak_y_values = self._re_organize_keys_find_peaks_convolve(tab_ws)

        if peak_x_values is None or peak_y_values is None:
            logger.error("Failed extracting columns from FindPeakConvolve output")
            return None

        return self._get_func_wrapper_str_for_peak_x_y(peak_x_values, peak_y_values, peak_type, fit_prop_ws, x_range)

    def _get_func_wrapper_str_for_peak_x_y(self, peak_x_values, peak_y_values, peak_type, fit_prop_ws, x_range):
        if set(peak_x_values.keys()) == set(peak_y_values.keys()):
            logger.notice(f"Adding {len(peak_x_values)} peaks found via FindPeaksConvolve")
            func_wrapper = None
            for col, x_value in peak_x_values.items():
                if x_value < x_range[0] or x_value > x_range[1]:
                    continue
                y_value = peak_y_values[col]
                peak_func = FunctionFactory.Instance().createPeakFunction(peak_type)
                peak_func.setCentre(x_value)
                peak_func.setHeight(y_value)
                peak_func.setMatrixWorkspace(fit_prop_ws, 0, 0, 0)
                f_wrapper = FunctionWrapper(peak_func)
                if func_wrapper is None:
                    func_wrapper = f_wrapper
                else:
                    func_wrapper += f_wrapper
            if func_wrapper is not None:
                return str(func_wrapper)
            else:
                logger.error("No peak functions were created from the results of FindPeaksConvolve")
        else:
            logger.error("Incompatible columns returned from FindPeaksConvolve!")
        return None

    def _re_organize_keys_find_peaks_convolve(self, table_ws):
        if table_ws.rowCount() == 1:
            table_dict = table_ws.row(0)
            table_dict.pop("SpecIndex", None)
            return {col_name.split("_")[-1]: value for col_name, value in table_dict.items()}
        return None

    def _ws_is_tof(self, wsname):
        return ADS.retrieve(wsname).getXDimension().getUnits() == "microsecond"

    def set_rb_num(self, rb_num):
        self._rb_num = rb_num
