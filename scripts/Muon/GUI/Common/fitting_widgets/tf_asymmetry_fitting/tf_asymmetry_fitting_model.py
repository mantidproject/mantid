# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.api import IFunction, MultiDomainFunction
from mantid.simpleapi import CopyLogs, ConvertFitFunctionForMuonTFAsymmetry

from Muon.GUI.Common.ADSHandler.workspace_naming import (check_phasequad_name, create_fitted_workspace_name,
                                                         create_multi_domain_fitted_workspace_name,
                                                         get_diff_asymmetry_name, get_group_asymmetry_name,
                                                         get_pair_asymmetry_name, get_pair_phasequad_name)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import DEFAULT_SINGLE_FIT_FUNCTION
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.utilities.algorithm_utils import run_CalculateMuonAsymmetry
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

DEFAULT_NORMALISATION = 0.0
NORMALISATION_FUNCTION_INDEX = "f0.f0.A0"
TF_ASYMMETRY_PREFIX_FUNCTION_INDEX = "f0.f1.f1."
TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE = ",TFAsymmetry"


class TFAsymmetryFittingModel(GeneralFittingModel):
    """
    The TFAsymmetryFittingModel derives from GeneralFittingModel. It adds the ability to do TF Asymmetry fitting.
    """

    def __init__(self, context: MuonContext, is_frequency_domain: bool = False):
        """Initialize the TFAsymmetryFittingModel with emtpy fit data."""
        super(TFAsymmetryFittingModel, self).__init__(context, is_frequency_domain)

        self._tf_asymmetry_mode = False
        self._tf_asymmetry_single_functions = []
        self._tf_asymmetry_simultaneous_function = None

    @GeneralFittingModel.dataset_names.setter
    def dataset_names(self, names: list) -> None:
        """Sets the dataset names stored by the model. Resets the other fitting data."""
        GeneralFittingModel.dataset_names.fset(self, names)
        self.recalculate_tf_asymmetry_functions()

    @property
    def tf_asymmetry_mode(self) -> bool:
        """Returns true if TF Asymmetry fitting mode is currently active."""
        return self._tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on: bool) -> None:
        """Sets the TF Asymmetry mode as being on or off."""
        self._tf_asymmetry_mode = tf_asymmetry_on

    @property
    def tf_asymmetry_single_functions(self) -> list:
        """Returns the fit functions used for single TF Asymmetry fitting. Each function corresponds to a dataset."""
        return self._tf_asymmetry_single_functions

    @tf_asymmetry_single_functions.setter
    def tf_asymmetry_single_functions(self, tf_asymmetry_functions: list) -> None:
        """Sets the single TF Asymmetry fit functions stored in the model."""
        self._tf_asymmetry_single_functions = tf_asymmetry_functions

    @property
    def current_tf_asymmetry_single_function(self) -> IFunction:
        """Returns the currently selected TF Asymmetry fit function for single fitting."""
        if self.current_dataset_index is not None:
            return self.tf_asymmetry_single_functions[self.current_dataset_index]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    def update_current_single_fit_functions(self, fit_function: IFunction) -> None:
        """Updates the currently selected TF Asymmetry and ordinary single fit function."""
        self.tf_asymmetry_single_functions[self.current_dataset_index] = fit_function
        self.current_single_fit_function = self._get_normal_fit_function_from(fit_function)

    @property
    def tf_asymmetry_simultaneous_function(self) -> IFunction:
        """Returns the simultaneous TF Asymmetry fit function stored in the model."""
        return self._tf_asymmetry_simultaneous_function

    @tf_asymmetry_simultaneous_function.setter
    def tf_asymmetry_simultaneous_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Sets the simultaneous TF Asymmetry fit function stored in the model."""
        self._tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

    def update_simultaneous_fit_functions(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Updates the TF Asymmetry and normal simultaneous fit function based on the function from a TFA fit."""
        self.tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

        if self.number_of_datasets > 1:
            self._update_parameters_of_multi_domain_simultaneous_function_from(tf_asymmetry_simultaneous_function)
        else:
            self.simultaneous_fit_function = self._get_normal_fit_function_from(tf_asymmetry_simultaneous_function)

    def _update_parameters_of_multi_domain_simultaneous_function_from(self,
                                                                      tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Updates the parameters in the normal simultaneous function based on a TF Asymmetry simultaneous function."""
        for domain_index in range(tf_asymmetry_simultaneous_function.nFunctions()):
            tf_asymmetry_domain_function = tf_asymmetry_simultaneous_function.getFunction(domain_index)
            parameter_values = self.get_fit_function_parameter_values(self._get_normal_fit_function_from(
                tf_asymmetry_domain_function))

            self._set_fit_function_parameter_values(self.simultaneous_fit_function.getFunction(domain_index),
                                                    parameter_values)

    def current_domain_tf_asymmetry_fit_function(self) -> IFunction:
        """Returns the fit function in the TF Asymmetry simultaneous function corresponding to the current dataset."""
        if self.number_of_datasets < 2:
            return self.tf_asymmetry_simultaneous_function

        if self.current_dataset_index is not None:
            return self.tf_asymmetry_simultaneous_function.getFunction(self.current_dataset_index)
        else:
            return self.tf_asymmetry_simultaneous_function.getFunction(0)

    def reset_tf_asymmetry_functions(self) -> None:
        """Resets the TF Asymmetry fit functions."""
        self.tf_asymmetry_single_functions = [None] * self.number_of_datasets
        self.tf_asymmetry_simultaneous_function = None

    def recalculate_tf_asymmetry_functions(self) -> bool:
        """Recalculates the TF Asymmetry functions based on the datasets and normal functions in the model."""
        if self.tf_asymmetry_mode:
            try:
                self._recalculate_tf_asymmetry_functions()
            except RuntimeError:
                self.reset_tf_asymmetry_functions()
                logger.error("The input function was not of the form N*(1+f)+A*exp(-lambda*t)).")
                return False
        else:
            self.reset_tf_asymmetry_functions()
        return True

    def set_current_normalisation(self, value: float) -> None:
        """Sets the normalisation in the current TF Asymmetry single function or simultaneous function."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                self._set_current_normalisation_in_tf_asymmetry_simultaneous_function(value)
            else:
                self._set_current_normalisation_in_tf_asymmetry_single_fit_function(value)

    def _set_current_normalisation_in_tf_asymmetry_single_fit_function(self, value: float) -> None:
        """Sets the normalisation in the currently selected TF Asymmetry single  fit function."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            current_tf_single_fit_function.setParameter(NORMALISATION_FUNCTION_INDEX, value)

    def _set_current_normalisation_in_tf_asymmetry_simultaneous_function(self, value: float) -> None:
        """Sets the normalisation in the current domain of the TF Asymmetry simultaneous function."""
        if self.tf_asymmetry_simultaneous_function is not None:
            self.tf_asymmetry_simultaneous_function.setParameter(
                f"f{self.current_dataset_index}.{NORMALISATION_FUNCTION_INDEX}", value)

    def current_normalisation(self) -> float:
        """Returns the normalisation of the current TF Asymmetry single function or simultaneous function."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                return self._current_normalisation_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the TF Asymmetry fit functions."""
        super().update_parameter_value(full_parameter, value)

        if self.tf_asymmetry_mode:
            tf_asymmetry_full_parameter = f"{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}{full_parameter}"
            self._update_tf_asymmetry_parameter_value(tf_asymmetry_full_parameter, value)

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.function_name_auto_update:
            super().automatically_update_function_name()
            if self.tf_asymmetry_mode:
                self.function_name += TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE

    def check_datasets_are_tf_asymmetry_compliant(self) -> bool:
        """Returns true if the datasets stored in the model are compatible with TF Asymmetry mode."""
        non_compliant_workspaces = [item for item in self.dataset_names if "Group" not in item]
        return False if len(non_compliant_workspaces) > 0 else True

    @staticmethod
    def get_fit_function_parameter_values(fit_function: IFunction) -> list:
        """Get all the parameter values within a given fit function."""
        if fit_function is not None:
            return [fit_function.getParameterValue(i) for i in range(fit_function.nParams())]
        return []

    @staticmethod
    def _set_fit_function_parameter_values(fit_function: IFunction, parameter_values: list) -> None:
        """Set the parameter values within a fit function."""
        for i in range(fit_function.nParams()):
            fit_function.setParameter(i, parameter_values[i])

    def _get_normal_fit_function_from(self, tf_asymmetry_function: IFunction) -> IFunction:
        """Returns the ordinary fit function embedded within the TF Asymmetry fit function."""
        if tf_asymmetry_function is not None:
            return tf_asymmetry_function.getFunction(0).getFunction(1).getFunction(1)
        else:
            return tf_asymmetry_function

    def _update_tf_asymmetry_parameter_value(self, full_parameter: str, value: float) -> None:
        """Updates a parameters value within the current TF Asymmetry single function or simultaneous function."""
        if self.simultaneous_fitting_mode:
            current_domain_function = self.current_domain_tf_asymmetry_fit_function()
            if current_domain_function is not None:
                current_domain_function.setParameter(full_parameter, value)
        else:
            if self.current_tf_asymmetry_single_function is not None:
                self.current_tf_asymmetry_single_function.setParameter(full_parameter, value)

    def _recalculate_tf_asymmetry_functions(self) -> None:
        """Recalculates the TF Asymmetry single fit functions or simultaneous function."""
        if self.simultaneous_fitting_mode:
            self._recalculate_tf_asymmetry_simultaneous_fit_function()
        else:
            self._recalculate_tf_asymmetry_single_fit_functions()

    def _recalculate_tf_asymmetry_single_fit_functions(self) -> None:
        """Recalculates the TF Asymmetry single fit functions."""
        self.tf_asymmetry_single_functions = [self._convert_to_tf_asymmetry_function(single_function,
                                                                                     [self.dataset_names[index]])
                                              for index, single_function in enumerate(self.single_fit_functions)]

    def _recalculate_tf_asymmetry_simultaneous_fit_function(self) -> None:
        """Recalculates the TF Asymmetry simultaneous function."""
        self.tf_asymmetry_simultaneous_function = self._convert_to_tf_asymmetry_function(self.simultaneous_fit_function,
                                                                                         self.dataset_names)

    def _convert_to_tf_asymmetry_function(self, fit_function: IFunction, workspace_names: list) -> IFunction:
        """Converts a normal single fit or simultaneous function to a TF Asymmetry fit function."""
        if fit_function is None:
            return None

        parameters = self._get_parameters_for_tf_asymmetry_conversion(fit_function, workspace_names)
        return ConvertFitFunctionForMuonTFAsymmetry(StoreInADS=False, **parameters)

    def _get_parameters_for_tf_asymmetry_conversion(self, fit_function: IFunction, workspace_names: list) -> dict:
        """Returns the parameters used to convert a normal function to a TF Asymmetry fit function."""
        return {"InputFunction": fit_function,
                "WorkspaceList": workspace_names,
                "Mode": "Construct" if self.tf_asymmetry_mode else "Extract",
                "CopyTies": False}

    def _current_normalisation_from_tf_asymmetry_single_fit_function(self) -> float:
        """Returns the normalisation in the currently selected TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            return current_tf_single_fit_function.getParameterValue(NORMALISATION_FUNCTION_INDEX)
        else:
            return DEFAULT_NORMALISATION

    def _current_normalisation_from_tf_asymmetry_simultaneous_function(self) -> float:
        """Returns the normalisation in the current domain of the TF Asymmetry simultaneous fit function."""
        if self.tf_asymmetry_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                return self.tf_asymmetry_simultaneous_function.getParameterValue(
                    f"f{self.current_dataset_index}.{NORMALISATION_FUNCTION_INDEX}")
            else:
                return self.tf_asymmetry_simultaneous_function.getParameterValue(NORMALISATION_FUNCTION_INDEX)
        else:
            return DEFAULT_NORMALISATION

    def _get_active_tf_asymmetry_fit_function(self) -> IFunction:
        """Returns the fit function that is active and will be used for a fit."""
        if self.tf_asymmetry_mode:
            if self.simultaneous_fitting_mode:
                return self.tf_asymmetry_simultaneous_function
            else:
                return self.current_tf_asymmetry_single_function
        else:
            return super().get_active_fit_function()

    def perform_fit(self) -> tuple:
        """Performs a fit. Allows the possibility of a TF Asymmetry fit in single fit mode and simultaneous fit mode."""
        if self.tf_asymmetry_mode:
            return self._do_tf_asymmetry_fit()
        else:
            return super().perform_fit()

    def _do_tf_asymmetry_fit(self) -> tuple:
        """Performs a TF Asymmetry fit in either the single fit mode or simultaneous fit mode."""
        if self.simultaneous_fitting_mode:
            return self._do_tf_asymmetry_simultaneous_fit(self._get_parameters_for_tf_asymmetry_simultaneous_fit(),
                                                          self._get_global_parameters_for_tf_asymmetry_fit())
        else:
            return self._do_tf_asymmetry_single_fit(self._get_parameters_for_tf_asymmetry_single_fit())

    def _do_tf_asymmetry_single_fit(self, parameters: dict) -> tuple:
        """Performs a TF Asymmetry fit in single fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._run_tf_asymmetry_fit(parameters)

        CopyLogs(InputWorkspace=self.current_dataset_name, OutputWorkspace=output_workspace, StoreInADS=False)
        self._add_single_fit_results_to_ADS_and_context(self.current_dataset_name, parameter_table, output_workspace,
                                                        covariance_matrix)
        return function, fit_status, chi_squared

    def _do_tf_asymmetry_simultaneous_fit(self, parameters: dict, global_parameters: list) -> tuple:
        """Performs a TF Asymmetry fit in simultaneous fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._run_tf_asymmetry_fit(parameters)

        self._copy_logs(output_workspace)
        self._add_simultaneous_fit_results_to_ADS_and_context(parameter_table, output_workspace, covariance_matrix,
                                                              global_parameters)
        return function, fit_status, chi_squared

    def _run_tf_asymmetry_fit(self, parameters: dict) -> tuple:
        """Performs the TF Asymmetry fit using the parameters dict provided."""
        alg = AlgorithmManager.create("CalculateMuonAsymmetry")
        return run_CalculateMuonAsymmetry(parameters, alg)

    def _get_parameters_for_tf_asymmetry_single_fit(self) -> dict:
        """Returns the parameters to use for a single TF Asymmetry fit."""
        params = self._get_common_tf_asymmetry_parameters()
        params["InputFunction"] = self.current_tf_asymmetry_single_function.clone()
        params["ReNormalizedWorkspaceList"] = self.current_dataset_name
        params["UnNormalizedWorkspaceList"] = self._get_unnormalised_workspace_list([self.current_dataset_name])[0]

        fit_workspace_name, _ = create_fitted_workspace_name(self.current_dataset_name, self.function_name)
        params["OutputFitWorkspace"] = fit_workspace_name
        return params

    def _get_parameters_for_tf_asymmetry_simultaneous_fit(self) -> dict:
        """Returns the parameters to use for a simultaneous TF Asymmetry fit."""
        params = self._get_common_tf_asymmetry_parameters()
        params["InputFunction"] = str(self.tf_asymmetry_simultaneous_function) + self._construct_global_tie_appendage()
        params["ReNormalizedWorkspaceList"] = self.dataset_names
        params["UnNormalizedWorkspaceList"] = self._get_unnormalised_workspace_list(self.dataset_names)

        fit_workspace_name, _ = create_multi_domain_fitted_workspace_name(self.dataset_names[0], self.function_name)
        params["OutputFitWorkspace"] = fit_workspace_name
        return params

    def _get_common_tf_asymmetry_parameters(self) -> dict:
        """Returns the common parameters used for a TF Asymmetry fit."""
        params = {"StartX": self.current_start_x,
                  "EndX": self.current_end_x,
                  "Minimizer": self.minimizer}

        if self._double_pulse_enabled():
            params.update(self._get_common_double_pulse_parameters())
        return params

    def _get_common_double_pulse_parameters(self) -> dict:
        """Returns the common parameters used for a TF Asymmetry fit in double pulse mode."""
        offset = self.context.gui_context['DoublePulseTime']
        first_pulse_weighting, _ = self._get_pulse_weightings(offset, 2.2)

        return {"PulseOffset": offset,
                "EnableDoublePulse": True,
                "FirstPulseWeight": first_pulse_weighting}

    def _construct_global_tie_appendage(self) -> str:
        """Constructs the string which details the global parameter ties within a simultaneous TF Asymmetry fit."""
        if len(self.global_parameters) != 0 and self.number_of_datasets > 1:
            global_ties = [self._construct_tie_for_global_parameter(global_parameter)
                           for global_parameter in self.global_parameters]
            return f";ties=({','.join(global_ties)})"
        else:
            return ""

    def _construct_tie_for_global_parameter(self, global_parameter: str) -> str:
        """Returns the global tie representing a parameter being made global in a simultaneous fit."""
        global_tie = f"f0.{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}{global_parameter}"
        for domain_index in range(1, self.tf_asymmetry_simultaneous_function.nFunctions()):
            global_tie += f"=f{domain_index}.{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}{global_parameter}"
        return global_tie

    def _get_global_parameters_for_tf_asymmetry_fit(self) -> list:
        """Returns a list of global parameters in TF Asymmetry format."""
        return [str(TF_ASYMMETRY_PREFIX_FUNCTION_INDEX + global_param) for global_param in self.global_parameters]

    def _get_unnormalised_workspace_list(self, normalised_workspaces: list) -> list:
        """Returns a list of unnormalised workspaces to be used within a TF Asymmetry fit."""
        return self.context.group_pair_context.get_unormalisised_workspace_list(normalised_workspaces)

    """
    Methods used by the Sequential Fitting Tab
    """

    def update_ws_fit_function_parameters(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameter values for the given dataset names."""
        if self.simultaneous_fitting_mode:
            self._update_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)
        else:
            self._update_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

    def _update_fit_function_parameters_for_single_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in single fit mode."""
        for name in dataset_names:
            fit_function = self.get_single_fit_function_for(name)
            if fit_function is not None:
                self._set_fit_function_parameter_values(fit_function, parameter_values)

    def _update_fit_function_parameters_for_simultaneous_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in simultaneous fit mode."""
        for name in dataset_names:
            if name in self.dataset_names:
                if isinstance(self.simultaneous_fit_function, MultiDomainFunction):
                    function_index = self.dataset_names.index(name)
                    self._set_fit_function_parameter_values(self.simultaneous_fit_function.getFunction(function_index),
                                                            parameter_values)
                else:
                    self._set_fit_function_parameter_values(self.simultaneous_fit_function, parameter_values)

    def get_fit_workspace_names_from_groups_and_runs(self, runs: list, groups_and_pairs: list) -> list:
        """Returns the workspace names to use for the given runs and groups/pairs."""
        workspace_names = []
        for run in runs:
            for group_or_pair in groups_and_pairs:
                workspace_names += self._get_workspace_name_from_run_and_group_or_pair(run, group_or_pair)
        return workspace_names

    def _get_workspace_name_from_run_and_group_or_pair(self, run: str, group_or_pair: str) -> list:
        """Returns the workspace name to use for the given run and group/pair."""
        if check_phasequad_name(group_or_pair) and group_or_pair in self.context.group_pair_context.selected_pairs:
            return [get_pair_phasequad_name(self.context, group_or_pair, run, not self.fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_pairs:
            return [get_pair_asymmetry_name(self.context, group_or_pair, run, not self.fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_diffs:
            return [get_diff_asymmetry_name(self.context, group_or_pair, run, not self.fit_to_raw)]
        elif group_or_pair in self.context.group_pair_context.selected_groups:
            period_string = run_list_to_string(self.context.group_pair_context[group_or_pair].periods)
            return [get_group_asymmetry_name(self.context, group_or_pair, run, period_string, not self.fit_to_raw)]
        else:
            return []
