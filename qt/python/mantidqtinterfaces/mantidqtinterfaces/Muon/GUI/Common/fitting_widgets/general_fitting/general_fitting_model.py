# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, IFunction, MultiDomainFunction
from mantid.simpleapi import RenameWorkspace, CopyLogs

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    create_covariance_matrix_name,
    create_fitted_workspace_name,
    create_multi_domain_fitted_workspace_name,
    create_parameter_table_name,
    get_group_or_pair_from_name,
    get_run_numbers_as_string_from_workspace_name,
)
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import GeneralFittingContext
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import BasicFittingModel
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import run_simultaneous_Fit
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


class GeneralFittingModel(BasicFittingModel):
    """
    The GeneralFittingModel derives from BasicFittingModel. It adds the ability to do simultaneous fitting.
    """

    def __init__(self, context: MuonContext, fitting_context: GeneralFittingContext):
        """Initialize the GeneralFittingModel with emtpy fit data."""
        super(GeneralFittingModel, self).__init__(context, fitting_context)

    def clear_simultaneous_fit_function(self) -> None:
        """Clears the simultaneous fit function."""
        self.fitting_context.simultaneous_fit_function = None

    @property
    def simultaneous_fit_function(self) -> IFunction:
        """Returns the simultaneous fit function stored in the model."""
        return self.fitting_context.simultaneous_fit_function

    @simultaneous_fit_function.setter
    def simultaneous_fit_function(self, fit_function: IFunction) -> None:
        """Sets the simultaneous fit function stored in the model."""
        self.fitting_context.simultaneous_fit_function = fit_function

    def current_domain_fit_function(self):
        """Returns the fit function in the simultaneous function corresponding to the currently displayed dataset."""
        simultaneous_function = self.fitting_context.simultaneous_fit_function
        if not isinstance(simultaneous_function, MultiDomainFunction):
            return simultaneous_function

        current_dataset_index = self.fitting_context.current_dataset_index
        return simultaneous_function.getFunction(current_dataset_index if current_dataset_index is not None else 0)

    @property
    def simultaneous_fitting_mode(self) -> bool:
        """Returns whether or not simultaneous fitting is currently active. If not, single fitting is active."""
        return self.fitting_context.simultaneous_fitting_mode

    @simultaneous_fitting_mode.setter
    def simultaneous_fitting_mode(self, enable_simultaneous: bool) -> None:
        """Sets whether or not simultaneous fitting is currently active in the model."""
        self.fitting_context.simultaneous_fitting_mode = enable_simultaneous

    @property
    def simultaneous_fit_by(self) -> str:
        """Returns the simultaneous fit by parameter stored in the model."""
        return self.fitting_context.simultaneous_fit_by

    @simultaneous_fit_by.setter
    def simultaneous_fit_by(self, simultaneous_fit_by: str) -> None:
        """Sets the simultaneous fit by parameter stored in the model."""
        self.fitting_context.simultaneous_fit_by = simultaneous_fit_by

    @property
    def simultaneous_fit_by_specifier(self) -> str:
        """Returns the simultaneous fit by specifier stored in the model."""
        return self.fitting_context.simultaneous_fit_by_specifier

    @simultaneous_fit_by_specifier.setter
    def simultaneous_fit_by_specifier(self, simultaneous_fit_by_specifier: str) -> None:
        """Sets the simultaneous fit by specifier stored in the model."""
        self.fitting_context.simultaneous_fit_by_specifier = simultaneous_fit_by_specifier

    @property
    def global_parameters(self) -> list:
        """Returns the global parameters stored in the model."""
        return self.fitting_context.global_parameters

    @global_parameters.setter
    def global_parameters(self, global_parameters: list) -> None:
        """Sets the global parameters stored in the model."""
        self.fitting_context.global_parameters = global_parameters

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the fit function."""
        if self.fitting_context.simultaneous_fitting_mode:
            current_domain_function = self.current_domain_fit_function()
            if current_domain_function is not None:
                current_domain_function.setParameter(full_parameter, value)
        else:
            super().update_parameter_value(full_parameter, value)

    def update_attribute_value(self, full_attribute: str, value: float) -> None:
        """Update the value of an attribute in the fit function."""
        if self.fitting_context.simultaneous_fitting_mode:
            current_domain_function = self.current_domain_fit_function()
            if current_domain_function is not None:
                current_domain_function.setAttributeValue(full_attribute, value)
        else:
            super().update_attribute_value(full_attribute, value)

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.fitting_context.function_name_auto_update:
            if self.fitting_context.simultaneous_fitting_mode:
                self.fitting_context.function_name = self._get_function_name(self.fitting_context.simultaneous_fit_function)
            else:
                super().automatically_update_function_name()

    def reset_fit_functions(self, new_functions: list) -> None:
        """Reset the fit functions stored by the model. Attempts to use the currently selected function."""
        if len(new_functions) == 0 or None in new_functions:
            self.fitting_context.simultaneous_fit_function = None
        elif len(new_functions) == 1:
            self.fitting_context.simultaneous_fit_function = new_functions[0]
        else:
            self._create_multi_domain_function_using(new_functions)
            self._add_global_ties_to_simultaneous_function()

        super().reset_fit_functions(new_functions)

    def number_of_undos(self) -> int:
        """Returns the number of previous single/simultaneous fits that are saved."""
        if self.fitting_context.simultaneous_fitting_mode:
            return len(self.fitting_context.simultaneous_fit_functions_for_undo)
        else:
            return super().number_of_undos()

    def undo_previous_fit(self) -> None:
        """Undoes the previous fit using the saved undo data."""
        if self.number_of_undos() > 0 and self.fitting_context.simultaneous_fitting_mode:
            self.fitting_context.undo_previous_fit()

            undo_simultaneous_fit_function = self.fitting_context.simultaneous_fit_functions_for_undo.pop()
            undo_simultaneous_fit_status = self.fitting_context.simultaneous_fit_statuses_for_undo.pop()
            undo_simultaneous_chi_squared = self.fitting_context.simultaneous_chi_squared_for_undo.pop()
            undo_global_parameters = self.fitting_context.global_parameters_for_undo.pop()

            self.fitting_context.simultaneous_fit_function = undo_simultaneous_fit_function
            self.fitting_context.fit_statuses = [undo_simultaneous_fit_status] * self.fitting_context.number_of_datasets
            self.fitting_context.chi_squared = [undo_simultaneous_chi_squared] * self.fitting_context.number_of_datasets
            self.fitting_context.global_parameters = undo_global_parameters
        else:
            super().undo_previous_fit()

    def save_current_fit_function_to_undo_data(self) -> None:
        """Saves the current simultaneous fit function, and the single fit functions defined in the base class."""
        if self.fitting_context.simultaneous_fitting_mode:
            self.fitting_context.simultaneous_fit_functions_for_undo.append(
                self._clone_function(self.fitting_context.simultaneous_fit_function)
            )
            self.fitting_context.simultaneous_fit_statuses_for_undo.append(self.current_fit_status)
            self.fitting_context.simultaneous_chi_squared_for_undo.append(self.current_chi_squared)
            self.fitting_context.global_parameters_for_undo.append(self.fitting_context.global_parameters)
        else:
            super().save_current_fit_function_to_undo_data()

    def clear_undo_data(self) -> None:
        """Clears the saved simultaneous fit functions, and the saved single fit functions defined in the base class."""
        super().clear_undo_data()
        self.fitting_context.simultaneous_fit_functions_for_undo = []
        self.fitting_context.simultaneous_fit_statuses_for_undo = []
        self.fitting_context.simultaneous_chi_squared_for_undo = []
        self.fitting_context.global_parameters_for_undo = []

    def _create_multi_domain_function_using(self, domain_functions: list) -> None:
        """Creates a new MultiDomainFunction using the provided functions corresponding to a domain each."""
        self.fitting_context.simultaneous_fit_function = MultiDomainFunction()
        for i, function in enumerate(domain_functions):
            self.fitting_context.simultaneous_fit_function.add(function)
            self.fitting_context.simultaneous_fit_function.setDomainIndex(i, i)

    def _get_new_functions_using_existing_datasets(self, new_dataset_names: list) -> list:
        """Returns the functions to use for the new datasets. It tries to use the existing functions if possible."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._get_new_domain_functions_using_existing_datasets(new_dataset_names)
        else:
            return super()._get_new_functions_using_existing_datasets(new_dataset_names)

    def _get_new_domain_functions_using_existing_datasets(self, new_dataset_names: list) -> list:
        """Returns the domain functions to use within a MultiDomainFunction for the new datasets."""
        dataset_names = self.fitting_context.dataset_names
        simultaneous_function = self.fitting_context.simultaneous_fit_function

        if len(dataset_names) == len(new_dataset_names) and simultaneous_function is not None:
            if isinstance(simultaneous_function, MultiDomainFunction):
                return [simultaneous_function.getFunction(i).clone() for i in range(simultaneous_function.nFunctions())]
            else:
                return [simultaneous_function.clone()]
        elif len(dataset_names) <= 1:
            return [self._clone_function(simultaneous_function) for _ in range(len(new_dataset_names))]
        else:
            return [self._get_new_domain_function_for(name) for name in new_dataset_names]

    def _get_new_domain_function_for(self, new_dataset_name: str) -> IFunction:
        """Returns the function to use for a specific domain when new datasets are loaded."""
        dataset_names = self.fitting_context.dataset_names
        simultaneous_function = self.fitting_context.simultaneous_fit_function

        if new_dataset_name in dataset_names and isinstance(simultaneous_function, MultiDomainFunction):
            return self._clone_function(simultaneous_function.getFunction(dataset_names.index(new_dataset_name)))
        else:
            return self._clone_function(self.current_domain_fit_function())

    def _add_global_ties_to_simultaneous_function(self) -> None:
        """Creates and adds ties to the simultaneous function to represent the global parameters."""
        assert self.simultaneous_fit_function is not None, "This method assumes the simultaneous fit function is not None."
        self._remove_non_existing_global_parameters()

        current_dataset_index = self.fitting_context.current_dataset_index

        for global_parameter in self.fitting_context.global_parameters:
            index = current_dataset_index if current_dataset_index is not None else 0
            self.simultaneous_fit_function.addTies(self._create_global_tie_string(index, global_parameter))

    def _create_global_tie_string(self, index: int, global_parameter: str) -> str:
        """Create a string to represent the tying of a global parameter."""
        ties = ["f" + str(i) + "." + global_parameter for i in range(self.simultaneous_fit_function.nFunctions()) if i != index]
        ties.append("f" + str(index) + "." + global_parameter)
        return "=".join(ties)

    def _remove_non_existing_global_parameters(self) -> None:
        """Removes non-existing parameters from the global parameters list."""
        self.global_parameters = [
            global_parameter
            for global_parameter in self.global_parameters
            if self.simultaneous_fit_function.hasParameter(f"f0.{global_parameter}")
        ]

    def _get_plot_guess_fit_function(self) -> IFunction:
        """Returns the fit function to evaluate when plotting a guess."""
        fit_function = self.get_active_fit_function()
        if fit_function is not None and self.fitting_context.simultaneous_fitting_mode:
            return fit_function.createEquivalentFunctions()[self.fitting_context.current_dataset_index]
        else:
            return fit_function

    def get_simultaneous_fit_by_specifiers_to_display_from_context(self) -> list:
        """Returns the simultaneous fit by specifiers to display in the view from the context."""
        if self.fitting_context.simultaneous_fit_by == "Run":
            return self._get_selected_runs()
        elif self.fitting_context.simultaneous_fit_by == "Group/Pair":
            return self._get_selected_groups_and_pairs()
        return []

    def get_fit_function_parameters(self) -> list:
        """Returns the names of the fit parameters in the fit functions."""
        if self.fitting_context.simultaneous_fitting_mode:
            simultaneous_function = self.fitting_context.simultaneous_fit_function
            if simultaneous_function is not None:
                return [simultaneous_function.parameterName(i) for i in range(simultaneous_function.nParams())]
            return []
        else:
            return super().get_fit_function_parameters()

    def get_active_fit_function(self) -> IFunction:
        """Returns the fit function that is active and will be used for a fit."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self.fitting_context.simultaneous_fit_function
        else:
            return super().get_active_fit_function()

    def get_active_workspace_names(self) -> list:
        """Returns the names of the workspaces that will be fitted. For simultaneous fitting, it is all loaded data."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self.fitting_context.dataset_names
        else:
            return super().get_active_workspace_names()

    def get_selected_runs_groups_and_pairs(self) -> tuple:
        """Returns the runs, groups and pairs that are currently selected."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode()
        else:
            return super().get_selected_runs_groups_and_pairs()

    def _get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode(self) -> tuple:
        """Returns the runs, groups and pairs that are currently selected for simultaneous fit mode."""
        runs, groups_and_pairs = super().get_selected_runs_groups_and_pairs()

        if self.fitting_context.simultaneous_fit_by == "Run":
            runs = self.simultaneous_fit_by_specifier
        elif self.fitting_context.simultaneous_fit_by == "Group/Pair":
            groups_and_pairs = [self.fitting_context.simultaneous_fit_by_specifier]
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
        return [get_run_numbers_as_string_from_workspace_name(workspace.workspace_name, instrument) for workspace in workspace_list]

    def perform_fit(self) -> tuple:
        """Performs a single or simultaneous fit and returns the resulting function, status and chi squared."""
        if self.fitting_context.simultaneous_fitting_mode:
            parameters = self._get_parameters_for_simultaneous_fit(
                self.fitting_context.dataset_names, self.fitting_context.simultaneous_fit_function
            )
            return self._do_simultaneous_fit(parameters, self.fitting_context.global_parameters)
        else:
            return super().perform_fit()

    def _do_simultaneous_fit(self, parameters: dict, global_parameters: list) -> tuple:
        """Performs a simultaneous fit and returns the resulting function, status and chi squared."""
        (
            output_group_workspace,
            parameter_table,
            function,
            fit_status,
            chi_squared,
            covariance_matrix,
        ) = self._do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameters)

        self._add_simultaneous_fit_results_to_ADS_and_context(
            parameters["InputWorkspace"], parameter_table, output_group_workspace, covariance_matrix, global_parameters
        )
        return function, fit_status, chi_squared

    def _do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(self, parameters: dict) -> tuple:
        """Performs a simultaneous fit and returns the resulting function, status and chi squared."""
        alg = self._create_fit_algorithm()
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = run_simultaneous_Fit(parameters, alg)

        self._copy_logs(parameters["InputWorkspace"], output_workspace)
        return output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix

    def _copy_logs(self, input_workspaces, output_workspace: str) -> None:
        """Copy the logs from the input workspace(s) to the output workspaces."""
        if self.fitting_context.number_of_datasets == 1:
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
        params["StartX"] = self.fitting_context.start_xs
        params["EndX"] = self.fitting_context.end_xs
        if self.fitting_context.exclude_range:
            params["Exclude"] = list(zip(self.fitting_context.exclude_start_xs, self.fitting_context.exclude_end_xs))
        return params

    def _add_simultaneous_fit_results_to_ADS_and_context(
        self, input_workspace_names: list, parameter_table, output_group_workspace, covariance_matrix, global_parameters: list
    ) -> None:
        """Adds the results of a simultaneous fit to the ADS and fitting context."""
        function_name = self.fitting_context.function_name

        output_workspace_wraps, directory = self._create_output_workspace_wraps(
            input_workspace_names, function_name, output_group_workspace
        )

        parameter_table_name, _ = create_parameter_table_name(input_workspace_names[0] + "+ ...", function_name)
        covariance_matrix_name, _ = create_covariance_matrix_name(input_workspace_names[0] + "+ ...", function_name)

        self._add_workspace_to_ADS(parameter_table, parameter_table_name, directory)
        self._add_workspace_to_ADS(covariance_matrix, covariance_matrix_name, directory)

        parameter_workspace_wrap = StaticWorkspaceWrapper(parameter_table_name, retrieve_ws(parameter_table_name))
        covariance_workspace_wrap = StaticWorkspaceWrapper(covariance_matrix_name, retrieve_ws(covariance_matrix_name))
        # the directory returns with a slash, so lets remove it
        self._add_workspaces_to_group([parameter_table_name, covariance_matrix_name], directory[:-1])

        self._add_fit_to_context(
            input_workspace_names, output_workspace_wraps, parameter_workspace_wrap, covariance_workspace_wrap, global_parameters
        )

    def _add_fit_to_context(
        self,
        input_workspace_names: list,
        output_workspaces: list,
        parameter_workspace: StaticWorkspaceWrapper,
        covariance_workspace: StaticWorkspaceWrapper,
        global_parameters: list = None,
    ) -> None:
        """Adds the results of a single or simultaneous fit to the context."""
        self.fitting_context.add_fit_from_values(
            input_workspace_names,
            self.fitting_context.function_name,
            output_workspaces,
            parameter_workspace,
            covariance_workspace,
            global_parameters,
        )

    def _get_names_in_group_workspace(self, group_name: str) -> list:
        """Returns the names of the workspaces existing within a group workspace."""
        if AnalysisDataService.doesExist(group_name):
            return AnalysisDataService.retrieve(group_name).getNames()
        else:
            return []

    def _create_output_workspace_wraps(self, input_workspace_names: list, function_name: str, output_group_workspace) -> tuple:
        """Returns a list of StaticWorkspaceWrapper objects containing the fitted output workspaces"""
        if self.fitting_context.number_of_datasets > 1:
            output_group_name, directory = create_multi_domain_fitted_workspace_name(input_workspace_names[0], function_name)
            output_workspace_wraps = self._create_output_workspace_wraps_for_a_multi_domain_fit(
                input_workspace_names, output_group_workspace, output_group_name
            )
        else:
            output_workspace_name, directory = create_fitted_workspace_name(input_workspace_names[0], function_name)
            self._add_workspace_to_ADS(output_group_workspace, output_workspace_name, directory)
            output_workspace_wraps = [StaticWorkspaceWrapper(output_workspace_name, retrieve_ws(output_workspace_name))]
        return output_workspace_wraps, directory

    def _create_output_workspace_wraps_for_a_multi_domain_fit(
        self, input_workspace_names: list, output_group_workspace, output_group_name: str
    ) -> list:
        """Returns a list of StaticWorkspaceWrapper objects containing the fitted output workspaces for many domains."""
        self._add_workspace_to_ADS(output_group_workspace, output_group_name, "")
        output_workspace_names = self._rename_members_of_fitted_workspace_group(input_workspace_names, output_group_name)

        return [StaticWorkspaceWrapper(workspace_name, retrieve_ws(workspace_name)) for workspace_name in output_workspace_names]

    def _rename_members_of_fitted_workspace_group(self, input_workspace_names: list, group_workspace: str) -> list:
        """Renames the output workspaces within a group workspace. The Fit algorithm returns an output group when
        doing a fit over multiple domains (i.e. for a simultaneous fit)."""
        self.context.ads_observer.observeRename(False)
        output_names = [
            self._rename_workspace(input_name, workspace_name)
            for input_name, workspace_name in zip(input_workspace_names, self._get_names_in_group_workspace(group_workspace))
        ]
        self.context.ads_observer.observeRename(True)

        return output_names

    def _rename_workspace(self, input_name: str, workspace_name: str) -> str:
        """Renames a resulting workspace from a simultaneous fit."""
        new_name, _ = create_fitted_workspace_name(input_name, self.fitting_context.function_name)
        new_name += "; Simultaneous"
        RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=new_name)
        return new_name

    """
    Methods used by the Sequential Fitting Tab
    """

    def get_runs_groups_and_pairs_for_fits(self, display_type: str) -> tuple:
        """Returns the runs and group/pairs corresponding to the selected dataset names."""
        if not self.fitting_context.simultaneous_fitting_mode:
            return super().get_runs_groups_and_pairs_for_fits(display_type)
        else:
            return self._get_runs_groups_and_pairs_for_simultaneous_fit(display_type)

    def _get_runs_groups_and_pairs_for_simultaneous_fit(self, display_type: str) -> tuple:
        """Returns the runs and group/pairs corresponding to the selected dataset names in simultaneous fitting mode."""
        if self.fitting_context.simultaneous_fit_by == "Run":
            return self._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs(display_type)
        elif self.fitting_context.simultaneous_fit_by == "Group/Pair":
            return self._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs(display_type)
        else:
            return [], [], []

    def _get_runs_groups_and_pairs_for_simultaneous_fit_by_runs(self, display_type: str):
        """Returns the runs and group/pairs for the selected data in simultaneous fit by runs mode."""
        runs = self._get_selected_runs()
        groups_and_pairs = [get_group_or_pair_from_name(name) for name in self.fitting_context.dataset_names]
        workspace_names = ["/".join(self.get_fit_workspace_names_from_groups_and_runs([run], groups_and_pairs)) for run in runs]
        return self._get_datasets_containing_string(display_type, workspace_names, runs, [";".join(groups_and_pairs)] * len(runs))

    def _get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs(self, display_type: str):
        """Returns the runs and group/pairs for the selected data in simultaneous fit by group/pairs mode."""
        runs = [
            get_run_numbers_as_string_from_workspace_name(name, self.context.data_context.instrument)
            for name in self.fitting_context.dataset_names
        ]
        groups_and_pairs = self._get_selected_groups_and_pairs()
        workspace_names = [
            "/".join(self.get_fit_workspace_names_from_groups_and_runs(runs, [group_and_pair])) for group_and_pair in groups_and_pairs
        ]
        return self._get_datasets_containing_string(
            display_type, workspace_names, [";".join(runs)] * len(groups_and_pairs), groups_and_pairs
        )

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        if self.fitting_context.simultaneous_fitting_mode:
            return [self.fitting_context.simultaneous_fit_function]
        else:
            return super().get_all_fit_functions()

    def get_all_fit_functions_for(self, display_type: str) -> list:
        """Returns all the fit functions for datasets with a name containing a string."""
        if self.fitting_context.simultaneous_fitting_mode:
            return [self.fitting_context.simultaneous_fit_function]
        else:
            return super().get_all_fit_functions_for(display_type)

    def update_ws_fit_function_parameters(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameter values for the given dataset names."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._update_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)
        else:
            super().update_ws_fit_function_parameters(dataset_names, parameter_values)

    def _update_fit_function_parameters_for_simultaneous_fit(self, _: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in simultaneous fit mode."""
        self._set_fit_function_parameter_values(self.fitting_context.simultaneous_fit_function, parameter_values)

    def perform_sequential_fit(self, workspaces: list, parameter_values: list, use_initial_values: bool = False):
        """Performs a sequential fit of the workspace names provided for the current fitting mode.

        :param workspaces: A list of lists of workspace names e.g. [[Row 1 workspaces], [Row 2 workspaces], etc...]
        :param parameter_values: A list of lists of parameter values e.g. [[Row 1 params], [Row 2 params], etc...]
        :param use_initial_values: If false the parameters at the end of each fit are passed on to the next fit.
        """
        fitting_func = self._get_sequential_fitting_func_for_normal_fitting_mode()
        if not self.fitting_context.simultaneous_fitting_mode:
            workspaces = self._flatten_workspace_names(workspaces)
        return self._perform_sequential_fit_using_func(fitting_func, workspaces, parameter_values, use_initial_values)

    def _do_sequential_simultaneous_fits(
        self, row_index: int, workspace_names: list, parameter_values: list, functions: list, use_initial_values: bool = False
    ):
        """Performs a number of simultaneous fits, sequentially."""
        simultaneous_function = (
            functions[row_index - 1].clone()
            if not use_initial_values and row_index >= 1
            else self._get_simultaneous_function_with_parameters(parameter_values)
        )

        params = self._get_parameters_for_simultaneous_fit(workspace_names, simultaneous_function)

        return self._do_simultaneous_fit(params, self.fitting_context.global_parameters)

    def _get_simultaneous_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current simultaneous function but with the parameter values provided."""
        simultaneous_function = self.fitting_context.simultaneous_fit_function.clone()
        self._set_fit_function_parameter_values(simultaneous_function, parameter_values)
        return simultaneous_function

    def _get_sequential_fitting_func_for_normal_fitting_mode(self):
        """Returns the fitting func to use when performing a fit in normal fitting mode."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._do_sequential_simultaneous_fits
        else:
            return self._do_sequential_fit

    def _update_fit_functions_after_sequential_fit(self, workspaces: list, functions: list) -> None:
        """Updates the fit functions after a sequential fit has been run on the Sequential fitting tab."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._update_simultaneous_fit_function_after_sequential(workspaces, functions)
        else:
            super()._update_fit_functions_after_sequential_fit(workspaces, functions)

    def _update_simultaneous_fit_function_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                self.fitting_context.simultaneous_fit_function = functions[fit_index]
                break

    def _are_same_workspaces_as_the_datasets(self, workspace_names: list) -> bool:
        """Checks that the workspace names provided are all in the loaded dataset names."""
        not_in_datasets = [name for name in workspace_names if name not in self.fitting_context.dataset_names]
        return False if len(not_in_datasets) > 0 else True

    def _update_fit_statuses_and_chi_squared_after_sequential_fit(self, workspaces, fit_statuses, chi_squared_list):
        """Updates the fit statuses and chi squared after a sequential fit."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._update_fit_statuses_and_chi_squared_for_simultaneous_mode(workspaces, fit_statuses, chi_squared_list)
        else:
            super()._update_fit_statuses_and_chi_squared_after_sequential_fit(workspaces, fit_statuses, chi_squared_list)

    def _update_fit_statuses_and_chi_squared_for_simultaneous_mode(
        self, workspaces: list, fit_statuses: list, chi_squared_list: list
    ) -> None:
        """Updates the fit statuses and chi squared after a sequential fit when in simultaneous fit mode."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                for workspace_name in workspace_names:
                    dataset_index = self.fitting_context.dataset_names.index(workspace_name)
                    self.fitting_context.fit_statuses[dataset_index] = fit_statuses[fit_index]
                    self.fitting_context.chi_squared[dataset_index] = chi_squared_list[fit_index]
                break
