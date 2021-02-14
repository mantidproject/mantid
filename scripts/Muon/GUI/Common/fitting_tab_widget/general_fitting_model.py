# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid
from mantid.api import AnalysisDataService, FunctionFactory, MultiDomainFunction
from mantid.simpleapi import RenameWorkspace, ConvertFitFunctionForMuonTFAsymmetry, CalculateMuonAsymmetry, CopyLogs

from Muon.GUI.Common.ADSHandler.workspace_naming import *
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.fitting_tab_widget.basic_fitting_model import (BasicFittingModel, FitPlotInformation,
                                                                    FDA_GUESS_WORKSPACE, MA_GUESS_WORKSPACE, FDA_SUFFIX,
                                                                    MA_SUFFIX)
from Muon.GUI.Common.utilities.algorithm_utils import run_simultaneous_Fit
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

import math
from mantid import logger


class GeneralFittingModel(BasicFittingModel):
    def __init__(self, context):
        super(GeneralFittingModel, self).__init__(context)

        self._x_data_type = self._get_x_data_type()

        self._grppair_index = {}

        # This is a MultiDomainFunction if there are multiple domains in the function browser.
        self._simultaneous_fit_function = None
        self._simultaneous_fit_function_cache = None
        self._simultaneous_fitting_mode = False

        self._simultaneous_fit_by = ""
        self._simultaneous_fit_by_specifier = ""

        self._global_parameters = []
        self._tf_asymmetry_mode = False  # TEMPORARY

    def clear_simultaneous_fit_function(self):
        self.simultaneous_fit_function = None

    @property
    def simultaneous_fit_function(self):
        return self._simultaneous_fit_function

    @simultaneous_fit_function.setter
    def simultaneous_fit_function(self, fit_function):
        self._simultaneous_fit_function = fit_function

    @property
    def simultaneous_fit_function_cache(self):
        return self._simultaneous_fit_function_cache

    @simultaneous_fit_function_cache.setter
    def simultaneous_fit_function_cache(self, fit_function):
        self._simultaneous_fit_function_cache = fit_function

    def cache_the_current_fit_functions(self):
        self.simultaneous_fit_function_cache = self._clone_function(self.simultaneous_fit_function)
        super().cache_the_current_fit_functions()

    def clear_cached_fit_functions(self):
        self.simultaneous_fit_function_cache = None
        super().clear_cached_fit_functions()

    @property
    def simultaneous_fitting_mode(self):
        return self._simultaneous_fitting_mode

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, enable_simultaneous):
        self._simultaneous_fitting_mode = enable_simultaneous

    @property
    def simultaneous_fit_by(self):
        return self._simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, simultaneous_fit_by):
        self._simultaneous_fit_by = simultaneous_fit_by

    @property
    def simultaneous_fit_by_specifier(self):
        return self._simultaneous_fit_by_specifier

    @simultaneous_fit_by_specifier.setter
    def simultaneous_fit_by_specifier(self, simultaneous_fit_by_specifier):
        self._simultaneous_fit_by_specifier = simultaneous_fit_by_specifier

    @property
    def global_parameters(self):
        return self._global_parameters

    @global_parameters.setter
    def global_parameters(self, global_parameters):
        self._global_parameters = global_parameters

    @property
    def tf_asymmetry_mode(self):
        return self._tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_mode):
        self._tf_asymmetry_mode = tf_asymmetry_mode

    def automatically_update_function_name(self):
        if self.function_name_auto_update:
            self.function_name = self._get_function_name(self.simultaneous_fit_function)

    def use_cached_function(self):
        """Sets the current function as being the cached function."""
        self.simultaneous_fit_function = self.simultaneous_fit_function_cache
        super().use_cached_function()

    def reset_fit_functions(self):
        """Reset the fit functions stored by the model. Attempts to use the currently selected function."""
        if self.number_of_datasets == 0:
            self.simultaneous_fit_function = None

        if self.simultaneous_fit_function is not None:
            single_function = self._get_single_domain_reset_function()
            self.simultaneous_fit_function = single_function if self.number_of_datasets == 1 else \
                self._create_multi_domain_function_from(single_function)

        super().reset_fit_functions()

    def get_simultaneous_fit_by_specifiers_to_display_from_context(self):
        """Returns the simultaneous fit by specifiers to display in the view from the context."""
        if self.simultaneous_fit_by == "Run":
            return self._get_selected_runs()
        elif self.simultaneous_fit_by == "Group/Pair":
            return self._get_selected_groups_and_pairs()
        return []

    def get_workspace_names_to_display_from_context(self):
        """Returns the workspace names to display in the view based on the selected run and group/pair options."""
        runs, groups_and_pairs = self._get_selected_runs_groups_and_pairs()

        display_workspaces = []
        for group_and_pair in groups_and_pairs:
            display_workspaces += self._get_workspace_names_to_display_from_context(runs, group_and_pair)

        return self._sort_workspace_names(display_workspaces)

    def get_active_fit_function(self):
        """Returns the fit function that is active and will be used for a fit."""
        if self.simultaneous_fitting_mode:
            return self.simultaneous_fit_function
        else:
            return super().get_active_fit_function()

    def get_active_workspace_names(self):
        """Returns the names of the workspaces that will be fitted. For simultaneous fitting, it is all loaded data."""
        if self.simultaneous_fitting_mode:
            return self.dataset_names
        else:
            return super().get_active_workspace_names()

    def _get_selected_runs_groups_and_pairs(self):
        """Returns the runs, groups and pairs that are currently selected."""
        if self.simultaneous_fitting_mode:
            return self._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode()
        else:
            return self._get_selected_runs_groups_and_pairs_for_single_fit_mode()

    def _get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode(self):
        """Returns the runs, groups and pairs that are currently selected for simultaneous fit mode."""
        runs, groups_and_pairs = self._get_selected_runs_groups_and_pairs_for_single_fit_mode()

        if self.simultaneous_fit_by == "Run":
            runs = self.simultaneous_fit_by_specifier
        elif self.simultaneous_fit_by == "Group/Pair":
            groups_and_pairs = [self.simultaneous_fit_by_specifier]
        return runs, groups_and_pairs

    def _get_selected_runs_groups_and_pairs_for_single_fit_mode(self):
        """Returns the runs, groups and pairs to use for single fit mode."""
        return "All", self._get_selected_groups_and_pairs()

    def _get_selected_groups_and_pairs(self):
        """Returns the groups and pairs currently selected in the context."""
        return self.context.group_pair_context.selected_groups_and_pairs

    def _get_selected_runs(self):
        """Returns an ordered list of run numbers currently selected in the context."""
        if len(self.context.data_context.current_runs) > 1:
            run_numbers = self._get_selected_runs_from_run_list()
        else:
            run_numbers = self._get_selected_runs_from_workspace()
        run_numbers.sort()
        return run_numbers

    def _get_selected_runs_from_run_list(self):
        """Extract runs from run list of lists, which is in the format [ [run,...,runs],[runs],...,[runs] ]"""
        current_runs = self.context.data_context.current_runs
        return [str(run) for run_list in current_runs for run in run_list]

    def _get_selected_runs_from_workspace(self):
        """Extract runs from the output workspace of the data context."""
        instrument = self.context.data_context.instrument
        workspace_list = self.context.data_context.current_data["OutputWorkspace"]
        return [get_run_numbers_as_string_from_workspace_name(workspace.workspace_name, instrument)
                for workspace in workspace_list]

    def _get_x_data_type(self):
        """Returns the type of data in the x domain. Returns string "None" if it cannot be determined."""
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            return self.context._frequency_context.plot_type
        return "None"

    def _get_workspace_names_to_display_from_context(self, runs, group_and_pair):
        """Returns the workspace names for the given runs and group/pair to be displayed in the view."""
        return self.context.get_names_of_workspaces_to_fit(runs=runs, group_and_pair=group_and_pair,
                                                           rebin=not self.fit_to_raw, freq=self._x_data_type)

    def _sort_workspace_names(self, workspace_names):
        """Sort the workspace names and check the workspaces exist in the ADS."""
        workspace_names = list(set(self._check_data_exists(workspace_names)))
        if len(workspace_names) > 1:
            workspace_names.sort(key=self._workspace_list_sorter)
        return workspace_names

    def _workspace_list_sorter(self, workspace_name):
        """Used to sort a list of workspace names based on run number and group/pair name."""
        run_number = get_run_number_from_workspace_name(workspace_name, self.context.data_context.instrument)
        grp_pair_number = self._transform_grp_or_pair_to_float(workspace_name)
        return int(run_number) + grp_pair_number

    @staticmethod
    def _check_data_exists(workspace_names):
        """Returns only the workspace names that exist in the ADS."""
        return [workspace_name for workspace_name in workspace_names if AnalysisDataService.doesExist(workspace_name)]

    def _get_single_domain_reset_function(self):
        """Return a single domain function to use in a reset function based on the previously active function."""
        if not isinstance(self.simultaneous_fit_function, MultiDomainFunction):
            return self.simultaneous_fit_function

        functions = [function.clone() for function in self.simultaneous_fit_function.createEquivalentFunctions()]
        if self.current_dataset_index is not None:
            return self._clone_function(functions[self.current_dataset_index])
        else:
            return self._clone_function(functions[0])

    def _create_multi_domain_function_from(self, fit_function):
        """Create a MultiDomainFunction containing the same fit function for each domain."""
        return FunctionFactory.createInitializedMultiDomainFunction(str(fit_function), self.number_of_datasets)

    def perform_fit(self):
        if self.simultaneous_fitting_mode:
            function, fit_status, chi_squared = self._do_simultaneous_fit(self._get_parameters_for_simultaneous_fit(),
                                                                          self.global_parameters)
        else:
            function, fit_status, chi_squared = super().perform_fit()

        return function, fit_status, chi_squared

    def _do_simultaneous_fit(self, parameter_dict, global_parameters):
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        self._add_simultaneous_fit_results_to_ADS_and_context(function, parameter_table, output_workspace,
                                                              covariance_matrix, global_parameters)
        return function, fit_status, chi_squared

    def _do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(self, parameters_dict):
        alg = self._create_fit_algorithm()
        workspace, parameters, function, fit_status, chi_squared, covariance_matrix = run_simultaneous_Fit(
            parameters_dict, alg)

        self._copy_logs(workspace)
        return workspace, parameters, function, fit_status, chi_squared, covariance_matrix

    def _copy_logs(self, output_workspace):
        if self.number_of_datasets == 1:
            CopyLogs(InputWorkspace=self.current_dataset_name, OutputWorkspace=output_workspace, StoreInADS=False)
        else:
            self._copy_logs_for_all_datsets(output_workspace)

    def _copy_logs_for_all_datsets(self, output_group):
        for input_workspace, output in zip(self.dataset_names, AnalysisDataService.retrieve(output_group).getNames()):
            CopyLogs(InputWorkspace=input_workspace, OutputWorkspace=output, StoreInADS=False)

    def _get_parameters_for_simultaneous_fit(self):
        params = self._get_common_parameters()
        params['InputWorkspace'] = self.dataset_names
        params['StartX'] = self.start_xs
        params['EndX'] = self.end_xs
        return params

    def _add_simultaneous_fit_results_to_ADS_and_context(self, function, parameter_table, output_workspace,
                                                         covariance_matrix, global_parameters):
        if self.number_of_datasets > 1:
            workspace_names, table_name, table_directory = self._add_multiple_fit_workspaces_to_ADS(function,
                                                                                                    output_workspace,
                                                                                                    covariance_matrix)
        else:
            workspace_name, table_name, table_directory = self._add_single_fit_workspaces_to_ADS(
                self.current_dataset_name, output_workspace, covariance_matrix)
            workspace_names = [workspace_name]

        self.add_fit_to_context(self._add_workspace_to_ADS(parameter_table, table_name, table_directory), function,
                                self.dataset_names, workspace_names, global_parameters)

    def _add_multiple_fit_workspaces_to_ADS(self, function, output_workspace, covariance_matrix):
        suffix = self.function_name
        workspace_names, workspace_directory = create_multi_domain_fitted_workspace_name(self.dataset_names[0], suffix)
        table_name, table_directory = create_parameter_table_name(self.dataset_names[0] + "+ ...", suffix)

        self._add_workspace_to_ADS(output_workspace, workspace_names, "")
        workspace_name = self.rename_members_of_fitted_workspace_group(workspace_names, self.dataset_names, function)
        self._add_workspace_to_ADS(covariance_matrix, workspace_name[0] + "_CovarianceMatrix", table_directory)

        return workspace_names, table_name, table_directory

    ##############################
    ##  Old
    ##############################

    @property
    def stored_fit_functions(self):
        pass
        ##return list(self.ws_fit_function_map.values())

    # sequential fitting
    def evaluate_sequential_fit(self, workspaces, use_initial_values):
        # workspaces are stored as list of list [[Fit1 workspaces], [Fit2 workspaces], [Fit3 workspaces]]
        if not self.simultaneous_fitting_mode:
            # flatten the workspace list
            workspace_list = [workspace for fit_workspaces in workspaces for workspace in fit_workspaces]
            if self.tf_asymmetry_mode:
                function_object, output_status, output_chi_squared = self.do_sequential_tf_fit(workspace_list,
                                                                                               use_initial_values)
            else:
                function_object, output_status, output_chi_squared = self.do_sequential_fit(workspace_list,
                                                                                            use_initial_values)
        else:
            # in a simultaneous-sequential fit, each fit corresponds to a list of workspaces
            if self.tf_asymmetry_mode:
                function_object, output_status, output_chi_squared = \
                    self.do_sequential_simultaneous_tf_fit(workspaces, use_initial_values)
            else:
                function_object, output_status, output_chi_squared = \
                    self.do_sequential_simultaneous_fit(workspaces, use_initial_values)

        return function_object, output_status, output_chi_squared

    def do_sequential_fit(self, workspace_list, use_initial_values=False):
        function_object_list = []
        output_status_list = []
        output_chi_squared_list = []

        for i, input_workspace in enumerate(workspace_list):
            params = self.get_parameters_for_single_fit(input_workspace)

            if not use_initial_values and i >= 1:
                previous_values = self.get_fit_function_parameter_values(function_object_list[i - 1])
                self.set_fit_function_parameter_values(params['Function'],
                                                       previous_values)

            function_object, output_status, output_chi_squared = self.do_single_fit(params)

            function_object_list.append(function_object)
            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_object_list, output_status_list, output_chi_squared_list

    def do_sequential_simultaneous_fit(self, workspaces, use_initial_values=False):
        function_object_list = []
        output_status_list = []
        output_chi_squared_list = []

        # workspaces defines a list of lists [[ws1,ws2],[ws1,ws2]...]
        for i, workspace_list in enumerate(workspaces):
            params = self._get_parameters_for_simultaneous_fit(workspace_list)

            if not use_initial_values and i >= 1:
                previous_values = self.get_fit_function_parameter_values(function_object_list[i - 1])
                self.set_fit_function_parameter_values(params['Function'], previous_values)

            function_object, output_status, output_chi_squared = \
                self.do_simultaneous_fit(params, self.global_parameters)
            function_object_list.append(function_object)
            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_object_list, output_status_list, output_chi_squared_list

    def do_sequential_tf_fit(self, workspace_list, use_initial_values=False):
        function_object_list = []
        output_status_list = []
        output_chi_squared_list = []

        for i, input_workspace in enumerate(workspace_list):
            params = self.get_parameters_for_single_tf_fit(input_workspace)

            if not use_initial_values and i >= 1:
                previous_values = self.get_fit_function_parameter_values(function_object_list[i - 1])
                self.set_fit_function_parameter_values(params['InputFunction'],
                                                       previous_values)

            function_object, output_status, output_chi_squared = self.do_single_tf_fit(params)

            function_object_list.append(function_object)
            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_object_list, output_status_list, output_chi_squared_list

    def do_sequential_simultaneous_tf_fit(self, workspaces, use_initial_values=False):
        function_object_list = []
        output_status_list = []
        output_chi_squared_list = []

        for i, workspace_list in enumerate(workspaces):
            params = self.get_parameters_for_simultaneous_tf_fit(workspace_list)

            if not use_initial_values and i >= 1:
                previous_values = self.get_fit_function_parameter_values(function_object_list[i - 1])
                self.set_fit_function_parameter_values(params['InputFunction'],
                                                       previous_values)

            function_object, output_status, output_chi_squared = \
                self.do_simultaneous_tf_fit(params, self.global_parameters)

            function_object_list.append(function_object)
            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_object_list, output_status_list, output_chi_squared_list

    # workspace operations
    def rename_members_of_fitted_workspace_group(self, group_workspace, inputworkspace_list, function):
        self.context.ads_observer.observeRename(False)
        output_workspace_list = []
        for index, workspace_name in enumerate(AnalysisDataService.retrieve(group_workspace).getNames()):
            new_name, _ = create_fitted_workspace_name(inputworkspace_list[index], self.function_name)

            new_name += '; Simultaneous'
            output_workspace_list.append(new_name)
            RenameWorkspace(InputWorkspace=workspace_name,
                            OutputWorkspace=new_name)

        self.context.ads_observer.observeRename(True)

        return output_workspace_list

    def add_workspace_to_ADS(self, workspace, name, directory):
        self.context.ads_observer.observeRename(False)
        workspace_wrapper = MuonWorkspaceWrapper(workspace)
        workspace_wrapper.show(directory + name)
        self.context.ads_observer.observeRename(True)
        return workspace_wrapper

    def add_fit_to_context(self, parameter_workspace, function,
                           input_workspace, output_workspace_name, global_parameters=None):
        self.context.fitting_context.add_fit_from_values(
            parameter_workspace, self.function_name,
            input_workspace, output_workspace_name, global_parameters)

    def get_fit_function(self, dataset_name):
        if self.simultaneous_fitting_mode:
            return self.simultaneous_fit_function
        else:
            dataset_index = self.dataset_names.index(dataset_name)
            return self.single_fit_functions[dataset_index]

    def get_function_for_fitting(self, dataset_index):
        if self.simultaneous_fitting_mode:
            return self.simultaneous_fit_function
        else:
            return self.single_fit_functions[dataset_index]

    # This function creates a list of keys from a list of input workspace names
    def create_hashable_keys_for_workspace_names(self):
        list_of_workspace_lists = []
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            list_of_workspace_lists = [[item] for item in self.get_selected_workspace_list()]
        else:
            runs, group_and_pairs = self.get_runs_groups_and_pairs_for_fits()
            for run, group_and_pair in zip(runs, group_and_pairs):
                separated_runs, separated_groups_and_pairs = \
                    self.get_separated_runs_and_group_and_pairs(run, group_and_pair)
                list_of_workspace_lists += [
                    self.get_fit_workspace_names_from_groups_and_runs(separated_runs,
                                                                      separated_groups_and_pairs)]
        workspace_key_list = []
        for workspace_list in list_of_workspace_lists:
            workspace_key_list += [self.create_workspace_key(workspace_list)]

        return workspace_key_list

    def update_ws_fit_function_parameters(self, workspace_names, params):
        if self.simultaneous_fitting_mode:
            fit_function = self.simultaneous_fit_function
        else:
            fit_function = self.current_single_fit_function
        self.set_fit_function_parameter_values(fit_function, params)

    def update_tf_fit_function(self, workspaces, fit_function):
        if not self.simultaneous_fitting_mode:
            tf_asymmetry_parameters = self.get_params_for_single_tf_function_calculation(workspaces[0], fit_function)
        else:
            tf_asymmetry_parameters = self.get_params_for_multi_tf_function_calculation(workspaces, fit_function)

        fit_function = self.calculate_tf_function(tf_asymmetry_parameters)

    def get_params_for_single_tf_function_calculation(self, workspace, fit_function):
        return {
            'InputFunction': fit_function,
            'ReNormalizedWorkspaceList': workspace,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(
                [workspace])[0],
            'OutputFitWorkspace': "fit",
            'StartX': self.current_start_x,
            'EndX': self.current_end_x,
            'Minimizer': self.minimizer}

    def get_params_for_multi_tf_function_calculation(self, workspace_list, fit_function):
        return {
            'InputFunction': fit_function,
            'ReNormalizedWorkspaceList': workspace_list,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(
                workspace_list),
            'OutputFitWorkspace': "fit",
            'StartX': self.current_start_x,
            'EndX': self.current_end_x,
            'Minimizer': self.minimizer
        }

    def freq_type(self):
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            freq = self.context._frequency_context.plot_type
        else:
            freq = 'None'
        return freq

    def get_parameters_for_single_tf_fit(self, workspace):
        # workspace is the name of the input workspace
        fit_workspace_name, _ = create_fitted_workspace_name(workspace, self.function_name)
        parameters = {
            'InputFunction': self.current_single_fit_function,
            'ReNormalizedWorkspaceList': workspace,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(
                [workspace])[0],
            'OutputFitWorkspace': fit_workspace_name,
            'StartX': self.current_start_x,
            'EndX': self.current_end_x,
            'Minimizer': self.minimizer,
        }

        if self.double_pulse_enabled():
            offset = self.context.gui_context['DoublePulseTime']
            muon_halflife = 2.2
            decay = math.exp(-offset / muon_halflife)
            first_pulse_weighting = decay / (1 + decay)
            parameters.update({'PulseOffset': offset,
                               'EnableDoublePulse': True, 'FirstPulseWeight': first_pulse_weighting})

        return parameters

    def get_parameters_for_simultaneous_tf_fit(self, workspaces):
        # workspaces is a list containing the N workspaces which form the simultaneous fit
        # creates a workspace name based on the first entry in the workspace list
        fit_workspaces, _ = create_multi_domain_fitted_workspace_name(workspaces[0], self.function_name)

        parameters = {
            'InputFunction': self.simultaneous_fit_function,
            'ReNormalizedWorkspaceList': workspaces,
            'UnNormalizedWorkspaceList': self.context.group_pair_context.get_unormalisised_workspace_list(workspaces),
            'OutputFitWorkspace': fit_workspaces,
            'StartX': self.current_start_x,
            'EndX': self.current_end_x,
            'Minimizer': self.minimizer,
        }
        if self.double_pulse_enabled():
            offset = self.context.gui_context['DoublePulseTime']
            muon_halflife = 2.2
            decay = math.exp(-offset / muon_halflife)
            first_pulse_weighting = decay / (1 + decay)
            parameters.update({'PulseOffset': offset,
                               'EnableDoublePulse': True, 'FirstPulseWeight': first_pulse_weighting})

        return parameters

    def double_pulse_enabled(self):
        return 'DoublePulseEnabled' in self.context.gui_context and self.context.gui_context['DoublePulseEnabled']

    # get workspace information
    def get_selected_workspace_list(self):
        selected_workspaces = []
        selected_groups_and_pairs = self._get_selected_groups_and_pairs()
        for grp_and_pair in selected_groups_and_pairs:
            selected_workspaces += self.context.get_names_of_workspaces_to_fit(
                runs='All',
                group_and_pair=grp_and_pair,
                rebin=not self.fit_to_raw,
                freq=self.freq_type())

        selected_workspaces = list(
            set(self._check_data_exists(selected_workspaces)))
        selected_workspaces.sort(key=self._workspace_list_sorter)
        return selected_workspaces

    def get_runs_groups_and_pairs_for_fits(self):
        selected_workspaces = self.get_selected_workspace_list()
        runs = []
        groups_and_pairs = []
        if not self.simultaneous_fitting_mode:
            for workspace in selected_workspaces:
                runs += [get_run_numbers_as_string_from_workspace_name(workspace, self.context.data_context.instrument)]
                groups_and_pairs += [get_group_or_pair_from_name(workspace)]
            run_groups_and_pairs = list(zip(runs, groups_and_pairs))
            groups_and_pairs = [grp_pair for _, grp_pair in run_groups_and_pairs]
            runs = [run for run, _ in run_groups_and_pairs]
        else:
            fit_workspaces = {}
            if self.simultaneous_fit_by == "Run":
                for workspace in selected_workspaces:
                    run = get_run_numbers_as_string_from_workspace_name(workspace, self.context.data_context.instrument)
                    if run not in fit_workspaces:
                        fit_workspaces[run] = get_group_or_pair_from_name(workspace)
                    else:
                        fit_workspaces[run] += ";" + get_group_or_pair_from_name(workspace)
                runs = list(fit_workspaces.keys())
                runs.sort()
                groups_and_pairs = list(fit_workspaces.values())
            elif self.simultaneous_fit_by == "Group/Pair":
                for workspace in selected_workspaces:
                    group_or_pair = get_group_or_pair_from_name(workspace)
                    run = get_run_numbers_as_string_from_workspace_name(workspace, self.context.data_context.instrument)
                    if group_or_pair not in fit_workspaces:
                        fit_workspaces[group_or_pair] = run
                    else:
                        fit_workspaces[group_or_pair] += ";" + run
                runs = list(fit_workspaces.values())
                groups_and_pairs = list(fit_workspaces.keys())
            else:
                return

        return runs, groups_and_pairs

    def get_fit_workspace_names_from_groups_and_runs(self, runs, groups_and_pairs):
        workspace_names = []
        for run in runs:
            for group_or_pair in groups_and_pairs:
                if check_phasequad_name(
                        group_or_pair) and group_or_pair in self.context.group_pair_context.selected_pairs:
                    workspace_names += [get_pair_phasequad_name(self.context, group_or_pair, run,
                                                                not self.fit_to_raw)]
                elif group_or_pair in self.context.group_pair_context.selected_pairs:
                    workspace_names += [get_pair_asymmetry_name(self.context, group_or_pair, run,
                                                                not self.fit_to_raw)]
                elif group_or_pair in self.context.group_pair_context.selected_diffs:
                    workspace_names += [get_diff_asymmetry_name(self.context, group_or_pair, run,
                                                                not self.fit_to_raw)]
                elif group_or_pair in self.context.group_pair_context.selected_groups:
                    period_string = run_list_to_string(
                        self.context.group_pair_context[group_or_pair].periods)
                    workspace_names += [get_group_asymmetry_name(self.context, group_or_pair, run, period_string,
                                                                 not self.fit_to_raw)]

        return workspace_names

    # Converts the workspace group or pair name to a float which is used in sorting the workspace list
    # If there is only 1 group or pair selected this function returns 0 as the workspace list will be sorted by runs
    def _transform_grp_or_pair_to_float(self, workspace_name):
        grp_or_pair_name = get_group_or_pair_from_name(workspace_name)
        if grp_or_pair_name not in self._grppair_index:
            self._grppair_index[grp_or_pair_name] = len(self._grppair_index)

        grp_pair_values = list(self._grppair_index.values())
        if len(self._grppair_index) > 1:
            return ((self._grppair_index[grp_or_pair_name] - grp_pair_values[0])
                    / (grp_pair_values[-1] - grp_pair_values[0])) * 0.99
        else:
            return 0

    @staticmethod
    def get_fit_function_parameter_values(fit_function):
        if fit_function is None:
            return []
        number_of_parameters = fit_function.nParams()
        parameters = [fit_function.parameterName(i) for i in range(number_of_parameters)]
        parameter_values = [fit_function.getParameterValue(parameters[i]) for i in
                            range(number_of_parameters)]
        return parameter_values

    @staticmethod
    def set_fit_function_parameter_values(fit_function, vals):
        number_of_parameters = fit_function.nParams()
        parameters = [fit_function.parameterName(i) for i in
                      range(number_of_parameters)]
        for i in range(number_of_parameters):
            fit_function.setParameter(parameters[i], vals[i])

    @staticmethod
    def create_workspace_key(workspace_list):
        # Using a frozenset means the hash of the key is invariant to the order of the input workspace_list
        # which is a desired outcome
        return frozenset(workspace_list)

    @staticmethod
    def convert_to_tf_function(algorithm_parameters):
        return ConvertFitFunctionForMuonTFAsymmetry(StoreInADS=False, **algorithm_parameters)

    @staticmethod
    def calculate_tf_function(algorithm_parameters):
        return CalculateMuonAsymmetry(StoreInADS=False, **algorithm_parameters)

    @staticmethod
    def get_separated_runs_and_group_and_pairs(runs, group_and_pairs):
        separated_runs = runs.split(';')
        separated_group_and_pairs = group_and_pairs.split(';')
        return separated_runs, separated_group_and_pairs

    @staticmethod
    def get_separated_workspaces(workspace_names):
        separated_workspaces = workspace_names.split('-')
        return separated_workspaces

    @staticmethod
    def _check_data_exists(guess_selection):
        return [item for item in guess_selection if AnalysisDataService.doesExist(item)]
