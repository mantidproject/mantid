# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, IFunction, MultiDomainFunction
from mantid.simpleapi import RenameWorkspace, CopyLogs

from Muon.GUI.Common.ADSHandler.workspace_naming import (create_fitted_workspace_name,
                                                         create_multi_domain_fitted_workspace_name,
                                                         create_parameter_table_name,
                                                         get_run_numbers_as_string_from_workspace_name)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from Muon.GUI.Common.utilities.algorithm_utils import run_simultaneous_Fit


class GeneralFittingModel(BasicFittingModel):
    """
    The GeneralFittingModel derives from BasicFittingModel. It adds the ability to do simultaneous fitting.
    """

    def __init__(self, context: MuonContext):
        """Initialize the GeneralFittingModel with emtpy fit data."""
        super(self.__class__, self).__init__(context)

        # This is a MultiDomainFunction if there are multiple domains in the function browser.
        self._simultaneous_fit_function = None
        self._simultaneous_fit_function_cache = None
        self._simultaneous_fitting_mode = False

        self._simultaneous_fit_by = ""
        self._simultaneous_fit_by_specifier = ""

        self._global_parameters = []

    def clear_simultaneous_fit_function(self) -> None:
        """Clears the simultaneous fit function."""
        self.simultaneous_fit_function = None

    @property
    def simultaneous_fit_function(self) -> IFunction:
        """Returns the simultaneous fit function stored in the model."""
        return self._simultaneous_fit_function

    @simultaneous_fit_function.setter
    def simultaneous_fit_function(self, fit_function: IFunction) -> None:
        """Sets the simultaneous fit function stored in the model."""
        if fit_function is not None and self.number_of_datasets == 0:
            raise RuntimeError(f"Cannot set a simultaneous fit function when there are no datasets in the model.")

        self._simultaneous_fit_function = fit_function

    def current_domain_fit_function(self):
        """Returns the fit function in the simultaneous function corresponding to the currently displayed dataset."""
        if not isinstance(self.simultaneous_fit_function, MultiDomainFunction):
            return self.simultaneous_fit_function

        index = self.current_dataset_index if self.current_dataset_index is not None else 0
        return self.simultaneous_fit_function.getFunction(index)

    @property
    def simultaneous_fit_function_cache(self) -> IFunction:
        """Returns the simultaneous fit function cache stored in the model."""
        return self._simultaneous_fit_function_cache

    @simultaneous_fit_function_cache.setter
    def simultaneous_fit_function_cache(self, fit_function: IFunction) -> None:
        """Sets the simultaneous fit function cache stored in the model."""
        if fit_function is not None and self.number_of_datasets == 0:
            raise RuntimeError(f"Cannot cache a simultaneous fit function when there are no datasets in the model.")

        self._simultaneous_fit_function_cache = fit_function

    def cache_the_current_fit_functions(self) -> None:
        """Caches the simultaneous fit function, and the single fit functions defined in the base class."""
        self.simultaneous_fit_function_cache = self._clone_function(self.simultaneous_fit_function)
        super().cache_the_current_fit_functions()

    def clear_cached_fit_functions(self) -> None:
        """Clears the simultaneous fit function cache, and the single fit function cache defined in the base class."""
        self.simultaneous_fit_function_cache = None
        super().clear_cached_fit_functions()

    @property
    def simultaneous_fitting_mode(self) -> bool:
        """Returns whether or not simultaneous fitting is currently active. If not, single fitting is active."""
        return self._simultaneous_fitting_mode

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, enable_simultaneous: bool) -> None:
        """Sets whether or not simultaneous fitting is currently active in the model."""
        self._simultaneous_fitting_mode = enable_simultaneous

    @property
    def simultaneous_fit_by(self) -> str:
        """Returns the simultaneous fit by parameter stored in the model."""
        return self._simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, simultaneous_fit_by: str) -> None:
        """Sets the simultaneous fit by parameter stored in the model."""
        self._simultaneous_fit_by = simultaneous_fit_by

    @property
    def simultaneous_fit_by_specifier(self) -> str:
        """Returns the simultaneous fit by specifier stored in the model."""
        return self._simultaneous_fit_by_specifier

    @simultaneous_fit_by_specifier.setter
    def simultaneous_fit_by_specifier(self, simultaneous_fit_by_specifier: str) -> None:
        """Sets the simultaneous fit by specifier stored in the model."""
        self._simultaneous_fit_by_specifier = simultaneous_fit_by_specifier

    @property
    def global_parameters(self) -> list:
        """Returns the global parameters stored in the model."""
        return self._global_parameters

    @global_parameters.setter
    def global_parameters(self, global_parameters: list) -> None:
        """Sets the global parameters stored in the model."""
        self._global_parameters = global_parameters

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the fit function."""
        if self.simultaneous_fitting_mode:
            current_domain_function = self.current_domain_fit_function()
            if current_domain_function is not None:
                current_domain_function.setParameter(full_parameter, value)
        else:
            super().update_parameter_value(full_parameter, value)

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.function_name_auto_update:
            if self.simultaneous_fitting_mode:
                self.function_name = self._get_function_name(self.simultaneous_fit_function)
            else:
                super().automatically_update_function_name()

    def use_cached_function(self) -> None:
        """Sets the current function as being the cached function."""
        self.simultaneous_fit_function = self.simultaneous_fit_function_cache
        super().use_cached_function()

    def reset_fit_functions(self, new_functions: list) -> None:
        """Reset the fit functions stored by the model. Attempts to use the currently selected function."""
        if len(new_functions) == 0 or None in new_functions:
            self.simultaneous_fit_function = None
        elif len(new_functions) == 1:
            self.simultaneous_fit_function = new_functions[0]
        else:
            self._create_multi_domain_function_using(new_functions)
            self._add_global_ties_to_simultaneous_function()

        super().reset_fit_functions(new_functions)

    def _create_multi_domain_function_using(self, domain_functions: list) -> None:
        """Creates a new MultiDomainFunction using the provided functions corresponding to a domain each."""
        self.simultaneous_fit_function = MultiDomainFunction()
        for i, function in enumerate(domain_functions):
            self.simultaneous_fit_function.add(function)
            self.simultaneous_fit_function.setDomainIndex(i, i)

    def _get_new_functions_using_existing_datasets(self, new_dataset_names: list) -> list:
        """Returns the functions to use for the new datasets. It tries to use the existing functions if possible."""
        if self.simultaneous_fitting_mode:
            return self._get_new_domain_functions_using_existing_datasets(new_dataset_names)
        else:
            return super()._get_new_functions_using_existing_datasets(new_dataset_names)

    def _get_new_domain_functions_using_existing_datasets(self, new_dataset_names: list) -> list:
        """Returns the domain functions to use within a MultiDomainFunction for the new datasets."""
        if len(self.dataset_names) == len(new_dataset_names) and self.simultaneous_fit_function is not None:
            if len(self.dataset_names) == 1:
                return [self.simultaneous_fit_function.clone()]
            else:
                return [self.simultaneous_fit_function.getFunction(i).clone()
                        for i in range(self.simultaneous_fit_function.nFunctions())]
        elif len(self.dataset_names) <= 1:
            return [self._clone_function(self.simultaneous_fit_function) for _ in range(len(new_dataset_names))]
        else:
            return [self._get_new_domain_function_for(name) for name in new_dataset_names]

    def _get_new_domain_function_for(self, new_dataset_name: str) -> IFunction:
        """Returns the function to use for a specific domain when new datasets are loaded."""
        if new_dataset_name in self.dataset_names and self.simultaneous_fit_function is not None:
            return self._clone_function(self.simultaneous_fit_function.getFunction(
                self.dataset_names.index(new_dataset_name)))
        else:
            return self._clone_function(self.current_domain_fit_function())

    def _add_global_ties_to_simultaneous_function(self) -> None:
        """Creates and adds ties to the simultaneous function to represent the global parameters."""
        index = self.current_dataset_index if self.current_dataset_index is not None else 0
        for global_parameter in self.global_parameters:
            self.simultaneous_fit_function.addTies(self._create_global_tie_string(index, global_parameter))

    def _create_global_tie_string(self, index: int, global_parameter: str) -> str:
        """Create a string to represent the tying of a global parameter."""
        ties = ["f" + str(i) + "." + global_parameter
                for i in range(self.simultaneous_fit_function.nFunctions()) if i != index]
        ties.append("f" + str(index) + "." + global_parameter)
        return "=".join(ties)

    def get_simultaneous_fit_by_specifiers_to_display_from_context(self) -> list:
        """Returns the simultaneous fit by specifiers to display in the view from the context."""
        if self.simultaneous_fit_by == "Run":
            return self._get_selected_runs()
        elif self.simultaneous_fit_by == "Group/Pair":
            return self._get_selected_groups_and_pairs()
        return []

    def get_fit_function_parameters(self) -> list:
        """Returns the names of the fit parameters in the fit functions."""
        if self.simultaneous_fitting_mode:
            if self.simultaneous_fit_function is not None:
                return [self.simultaneous_fit_function.parameterName(i)
                        for i in range(self.simultaneous_fit_function.nParams())]
            return []
        else:
            return super().get_fit_function_parameters()

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        if self.simultaneous_fitting_mode:
            return [self.simultaneous_fit_function]
        else:
            return super().get_all_fit_functions()

    def get_active_fit_function(self) -> IFunction:
        """Returns the fit function that is active and will be used for a fit."""
        if self.simultaneous_fitting_mode:
            return self.simultaneous_fit_function
        else:
            return super().get_active_fit_function()

    def get_active_workspace_names(self) -> list:
        """Returns the names of the workspaces that will be fitted. For simultaneous fitting, it is all loaded data."""
        if self.simultaneous_fitting_mode:
            return self.dataset_names
        else:
            return super().get_active_workspace_names()

    def get_selected_runs_groups_and_pairs(self) -> tuple:
        """Returns the runs, groups and pairs that are currently selected."""
        if self.simultaneous_fitting_mode:
            return self._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode()
        else:
            return super().get_selected_runs_groups_and_pairs()

    def _get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode(self) -> tuple:
        """Returns the runs, groups and pairs that are currently selected for simultaneous fit mode."""
        runs, groups_and_pairs = super().get_selected_runs_groups_and_pairs()

        if self.simultaneous_fit_by == "Run":
            runs = self.simultaneous_fit_by_specifier
        elif self.simultaneous_fit_by == "Group/Pair":
            groups_and_pairs = [self.simultaneous_fit_by_specifier]
        return runs, groups_and_pairs

    def _get_selected_runs(self) -> list:
        """Returns an ordered list of run numbers currently selected in the context."""
        if len(self.context.data_context.current_runs) > 1:
            run_numbers = self._get_selected_runs_from_run_list()
        else:
            run_numbers = self._get_selected_runs_from_workspace()
        run_numbers.sort()
        return run_numbers

    def _get_selected_runs_from_run_list(self) -> list:
        """Extract runs from run list of lists, which is in the format [ [run,...,runs],[runs],...,[runs] ]"""
        current_runs = self.context.data_context.current_runs
        return [str(run) for run_list in current_runs for run in run_list]

    def _get_selected_runs_from_workspace(self) -> list:
        """Extract runs from the output workspace of the data context."""
        instrument = self.context.data_context.instrument
        workspace_list = self.context.data_context.current_data["OutputWorkspace"]
        return [get_run_numbers_as_string_from_workspace_name(workspace.workspace_name, instrument)
                for workspace in workspace_list]

    def perform_fit(self) -> tuple:
        """Performs a single or simultaneous fit and returns the resulting function, status and chi squared."""
        if self.simultaneous_fitting_mode:
            return self._do_simultaneous_fit(self._get_parameters_for_simultaneous_fit(
                self.dataset_names, self.simultaneous_fit_function), self.global_parameters)
        else:
            return super().perform_fit()

    def _do_simultaneous_fit(self, parameters: dict, global_parameters: list) -> tuple:
        """Performs a simultaneous fit and returns the resulting function, status and chi squared."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameters)

        self._add_simultaneous_fit_results_to_ADS_and_context(parameters["InputWorkspace"], parameter_table,
                                                              output_workspace, covariance_matrix, global_parameters)
        return function, fit_status, chi_squared

    def _do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(self, parameters: dict) -> tuple:
        """Performs a simultaneous fit and returns the resulting function, status and chi squared."""
        alg = self._create_fit_algorithm()
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = run_simultaneous_Fit(
            parameters, alg)

        self._copy_logs(parameters["InputWorkspace"], output_workspace)
        return output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix

    def _copy_logs(self, input_workspaces, output_workspace: str) -> None:
        """Copy the logs from the input workspace(s) to the output workspaces."""
        if self.number_of_datasets == 1:
            CopyLogs(InputWorkspace=input_workspaces[0], OutputWorkspace=output_workspace, StoreInADS=False)
        else:
            self._copy_logs_for_all_datsets(input_workspaces, output_workspace)

    def _copy_logs_for_all_datsets(self, input_workspaces: list, output_group: str) -> None:
        """Copy the logs from the input workspaces to the output workspaces."""
        for input_workspace, output in zip(input_workspaces, self._get_names_in_group_workspace(output_group)):
            CopyLogs(InputWorkspace=input_workspace, OutputWorkspace=output, StoreInADS=False)

    def _get_parameters_for_simultaneous_fit(self, dataset_names: list, simultaneous_function: IFunction) -> dict:
        """Gets the parameters to use for a simultaneous fit."""
        params = self._get_common_parameters()
        params["Function"] = simultaneous_function
        params["InputWorkspace"] = dataset_names
        params["StartX"] = self.start_xs
        params["EndX"] = self.end_xs
        return params

    def _add_simultaneous_fit_results_to_ADS_and_context(self, input_workspace_names: list, parameter_table,
                                                         output_workspace, covariance_matrix,
                                                         global_parameters: list) -> None:
        """Adds the results of a simultaneous fit to the ADS and fitting context."""
        if self.number_of_datasets > 1:
            workspace_names, table_name, table_directory = self._add_multiple_fit_workspaces_to_ADS(
                input_workspace_names, output_workspace, covariance_matrix)
        else:
            workspace_name, table_name, table_directory = self._add_single_fit_workspaces_to_ADS(
                input_workspace_names[0], output_workspace, covariance_matrix)
            workspace_names = [workspace_name]

        self._add_fit_to_context(self._add_workspace_to_ADS(parameter_table, table_name, table_directory),
                                 input_workspace_names, workspace_names, global_parameters)

    def _add_multiple_fit_workspaces_to_ADS(self, input_workspace_names: list, output_workspace, covariance_matrix):
        """Adds the results of a simultaneous fit to the ADS and fitting context if multiple workspaces were fitted."""
        suffix = self.function_name
        workspace_names, workspace_directory = create_multi_domain_fitted_workspace_name(input_workspace_names[0],
                                                                                         suffix)
        table_name, table_directory = create_parameter_table_name(input_workspace_names[0] + "+ ...", suffix)

        self._add_workspace_to_ADS(output_workspace, workspace_names, "")
        workspace_names = self._rename_members_of_fitted_workspace_group(input_workspace_names, workspace_names)
        self._add_workspace_to_ADS(covariance_matrix, workspace_names[0] + "_CovarianceMatrix", table_directory)

        return workspace_names, table_name, table_directory

    def _get_names_in_group_workspace(self, group_name: str) -> list:
        """Returns the names of the workspaces existing within a group workspace."""
        if AnalysisDataService.doesExist(group_name):
            return AnalysisDataService.retrieve(group_name).getNames()
        else:
            return []

    def _rename_members_of_fitted_workspace_group(self, input_workspace_names: list, group_workspace: str) -> list:
        """Renames the fit result workspaces within a group workspace."""
        self.context.ads_observer.observeRename(False)
        output_names = [self._rename_workspace(input_name, workspace_name) for input_name, workspace_name in
                        zip(input_workspace_names, self._get_names_in_group_workspace(group_workspace))]
        self.context.ads_observer.observeRename(True)

        return output_names

    def _rename_workspace(self, input_name: str, workspace_name: str) -> str:
        """Renames a resulting workspace from a simultaneous fit."""
        new_name, _ = create_fitted_workspace_name(input_name, self.function_name)
        new_name += '; Simultaneous'
        RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=new_name)
        return new_name
