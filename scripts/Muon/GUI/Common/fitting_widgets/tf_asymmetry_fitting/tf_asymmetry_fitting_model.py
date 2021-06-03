# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.api import IFunction
from mantid.simpleapi import CopyLogs, ConvertFitFunctionForMuonTFAsymmetry

from Muon.GUI.Common.ADSHandler.workspace_naming import (check_phasequad_name, create_fitted_workspace_name,
                                                         create_multi_domain_fitted_workspace_name,
                                                         get_diff_asymmetry_name, get_group_asymmetry_name,
                                                         get_group_or_pair_from_name, get_pair_asymmetry_name,
                                                         get_pair_phasequad_name,
                                                         get_run_numbers_as_string_from_workspace_name)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import DEFAULT_SINGLE_FIT_FUNCTION
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from Muon.GUI.Common.utilities.algorithm_utils import run_CalculateMuonAsymmetry
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string

DEFAULT_NORMALISATION = 0.0
DEFAULT_NORMALISATION_ERROR = 0.0
DEFAULT_IS_NORMALISATION_FIXED = False
NORMALISATION_PARAMETER = "f0.f0.A0"
TF_ASYMMETRY_PREFIX_FUNCTION_INDEX = "f0.f1.f1."
TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE = ",TFAsymmetry"


class TFAsymmetryFittingModel(GeneralFittingModel):
    """
    The TFAsymmetryFittingModel derives from GeneralFittingModel. It adds the ability to do TF Asymmetry fitting.
    """

    def __init__(self, context: MuonContext):
        """Initialize the TFAsymmetryFittingModel with emtpy fit data."""
        super(TFAsymmetryFittingModel, self).__init__(context)

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
        return self.get_tf_asymmetry_single_function(self.current_dataset_index)

    def get_tf_asymmetry_single_function(self, dataset_index: int) -> IFunction:
        """Returns the TF Asymmetry fit function for single fitting for the specified dataset index."""
        if dataset_index is not None:
            return self.tf_asymmetry_single_functions[dataset_index]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    def update_tf_asymmetry_single_fit_function(self, dataset_index: int, fit_function: IFunction) -> None:
        """Updates the specified TF Asymmetry and ordinary single fit function."""
        self.tf_asymmetry_single_functions[dataset_index] = fit_function
        self.single_fit_functions[dataset_index] = self._get_normal_fit_function_from(fit_function)

    @property
    def tf_asymmetry_simultaneous_function(self) -> IFunction:
        """Returns the simultaneous TF Asymmetry fit function stored in the model."""
        return self._tf_asymmetry_simultaneous_function

    @tf_asymmetry_simultaneous_function.setter
    def tf_asymmetry_simultaneous_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Sets the simultaneous TF Asymmetry fit function stored in the model."""
        self._tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

    def update_tf_asymmetry_simultaneous_fit_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
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

    def get_domain_tf_asymmetry_fit_function(self, tf_simultaneous_function: IFunction, dataset_index: int) -> IFunction:
        """Returns the fit function in the TF Asymmetry simultaneous function corresponding to the specified index."""
        if self.number_of_datasets < 2 or tf_simultaneous_function is None:
            return tf_simultaneous_function

        index = dataset_index if dataset_index is not None else 0
        return tf_simultaneous_function.getFunction(index)

    def reset_tf_asymmetry_functions(self) -> None:
        """Resets the TF Asymmetry fit functions."""
        self.tf_asymmetry_single_functions = [None] * self.number_of_datasets
        self.tf_asymmetry_simultaneous_function = None

    def recalculate_tf_asymmetry_functions(self) -> bool:
        """Recalculates the TF Asymmetry functions based on the datasets and normal functions in the model."""
        tf_compliant, _ = self.check_datasets_are_tf_asymmetry_compliant()
        if self.tf_asymmetry_mode and tf_compliant:
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
                self._set_normalisation_in_tf_asymmetry_simultaneous_function(self.tf_asymmetry_simultaneous_function,
                                                                              self.current_dataset_index, value)
            else:
                self._set_normalisation_in_tf_asymmetry_single_fit_function(self.current_dataset_index, value)

    def _set_normalisation_for_dataset(self, dataset_name: str, value: float) -> None:
        """Sets the normalisation for the given dataset name."""
        if dataset_name in self.dataset_names:
            dataset_index = self.dataset_names.index(dataset_name)
            if self.simultaneous_fitting_mode:
                self._set_normalisation_in_tf_asymmetry_simultaneous_function(self.tf_asymmetry_simultaneous_function,
                                                                              dataset_index, value)
            else:
                self._set_normalisation_in_tf_asymmetry_single_fit_function(dataset_index, value)

    def _set_normalisation_in_tf_asymmetry_single_fit_function(self, function_index: int, value: float) -> None:
        """Sets the normalisation in the specified TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[function_index]
        if current_tf_single_fit_function is not None:
            current_tf_single_fit_function.setParameter(NORMALISATION_PARAMETER, value)

    def _set_normalisation_in_tf_asymmetry_simultaneous_function(self, tf_simultaneous_function: IFunction,
                                                                 domain_index: int, value: float) -> None:
        """Sets the normalisation in the specified domain of the TF Asymmetry simultaneous function."""
        if tf_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                tf_simultaneous_function.setParameter(
                    self._get_normalisation_function_parameter_for_simultaneous_domain(domain_index), value)
            else:
                tf_simultaneous_function.setParameter(NORMALISATION_PARAMETER, value)

    def current_normalisation(self) -> float:
        """Returns the normalisation of the current TF Asymmetry single function or simultaneous function."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                return self._current_normalisation_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION

    def current_normalisation_error(self) -> float:
        """Returns the normalisation error of the current TF Asymmetry single function or simultaneous function."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                return self._current_normalisation_error_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_error_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION_ERROR

    def is_current_normalisation_fixed(self) -> bool:
        """Returns true if the currently selected normalisation has its value fixed."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                return self._is_current_normalisation_fixed_in_tf_asymmetry_simultaneous_mode()
            else:
                return self._is_current_normalisation_fixed_in_tf_asymmetry_single_fit_mode()
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def toggle_fix_current_normalisation(self, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value, or unfixes it."""
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                self._toggle_fix_current_normalisation_in_tf_asymmetry_simultaneous_mode(is_fixed)
            else:
                self._toggle_fix_current_normalisation_in_tf_asymmetry_single_fit_mode(is_fixed)

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the TF Asymmetry fit functions."""
        super().update_parameter_value(full_parameter, value)

        if self.tf_asymmetry_mode:
            tf_asymmetry_full_parameter = f"{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}{full_parameter}"
            self._update_tf_asymmetry_parameter_value(self.current_dataset_index, tf_asymmetry_full_parameter, value)

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.function_name_auto_update:
            super().automatically_update_function_name()
            if self.tf_asymmetry_mode:
                self.function_name += TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE

    def check_datasets_are_tf_asymmetry_compliant(self, workspace_names: list = None) -> bool:
        """Returns true if the datasets stored in the model are compatible with TF Asymmetry mode."""
        workspace_names = self.dataset_names if workspace_names is None else workspace_names
        pair_names = [get_group_or_pair_from_name(name) for name in workspace_names if "Group" not in name]
        # Remove duplicates from the list
        pair_names = list(dict.fromkeys(pair_names))
        return len(pair_names) == 0, "'" + "', '".join(pair_names) + "'"

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        if self.tf_asymmetry_mode:
            if self.simultaneous_fitting_mode:
                return [self.tf_asymmetry_simultaneous_function]
            else:
                return self.tf_asymmetry_single_functions
        else:
            return super().get_all_fit_functions()

    def get_fit_function_parameters(self) -> list:
        """Returns the names of the fit parameters in the fit functions."""
        parameters = super().get_fit_function_parameters()
        if self.tf_asymmetry_mode:
            if self.simultaneous_fitting_mode:
                return self._add_normalisation_to_parameters_for_simultaneous_fitting(
                    parameters, self._get_normalisation_parameter_name_for_simultaneous_domain)
            else:
                return ["N0"] + parameters
        else:
            return parameters

    def get_all_fit_function_parameter_values_for(self, fit_function: IFunction) -> list:
        """Returns the values of the fit function parameters. Also returns the normalisation for TF Asymmetry mode."""
        if self.tf_asymmetry_mode:
            if self.simultaneous_fitting_mode:
                return self._get_all_fit_function_parameter_values_for_tf_simultaneous_function(fit_function)
            else:
                return self._get_all_fit_function_parameter_values_for_tf_single_function(fit_function)
        else:
            return self.get_fit_function_parameter_values(fit_function)

    def _get_all_fit_function_parameter_values_for_tf_single_function(self, tf_single_function: IFunction) -> list:
        """Returns the required parameters values including normalisation from a TF asymmetry single function."""
        normal_single_function = self._get_normal_fit_function_from(tf_single_function)
        parameter_values = self.get_fit_function_parameter_values(normal_single_function)
        return [self._get_normalisation_from_tf_fit_function(tf_single_function)] + parameter_values

    def _get_all_fit_function_parameter_values_for_tf_simultaneous_function(self, tf_simultaneous_function: IFunction) -> list:
        """Returns the required parameters values including normalisation from a TF asymmetry simultaneous function."""
        all_parameters = []
        for domain_index in range(self.number_of_datasets):
            all_parameters += [self._get_normalisation_from_tf_fit_function(tf_simultaneous_function, domain_index)]

            tf_domain_function = self.get_domain_tf_asymmetry_fit_function(tf_simultaneous_function, domain_index)
            normal_domain_function = self._get_normal_fit_function_from(tf_domain_function)
            parameter_values = self.get_fit_function_parameter_values(normal_domain_function)
            all_parameters += parameter_values
        return all_parameters

    def _add_normalisation_to_parameters_for_simultaneous_fitting(self, parameters, get_normalisation_func):
        """Returns the names of the fit parameters in the fit functions including the normalisation for simultaneous."""
        n_params_per_domain = int(len(parameters) / self.number_of_datasets)

        all_parameters = []
        for domain_index in range(self.number_of_datasets):
            all_parameters += [get_normalisation_func(domain_index)]
            all_parameters += parameters[domain_index * n_params_per_domain: (domain_index + 1) * n_params_per_domain]
        return all_parameters

    @staticmethod
    def _get_normalisation_parameter_name_for_simultaneous_domain(domain_index: int) -> str:
        """Returns the parameter name to use for a simultaneous normalisation parameter for a specific domain."""
        return f"f{domain_index}.N0"

    @staticmethod
    def _get_normalisation_function_parameter_for_simultaneous_domain(domain_index: int) -> str:
        """Returns the normalisation parameter for a specific domain as seen in the MultiDomainFunction."""
        return f"f{domain_index}.{NORMALISATION_PARAMETER}"

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

    def _update_tf_asymmetry_parameter_value(self, dataset_index: int, full_parameter: str, value: float) -> None:
        """Updates a parameters value within the current TF Asymmetry single function or simultaneous function."""
        if self.simultaneous_fitting_mode:
            domain_function = self.get_domain_tf_asymmetry_fit_function(self.tf_asymmetry_simultaneous_function,
                                                                        dataset_index)
        else:
            domain_function = self.get_tf_asymmetry_single_function(dataset_index)

        if domain_function is not None:
            domain_function.setParameter(full_parameter, value)

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

    def _get_normalisation_from_tf_fit_function(self, tf_function: IFunction, function_index: int = 0) -> float:
        """Returns the normalisation in the specified TF Asymmetry fit function."""
        if self.simultaneous_fitting_mode:
            return self._get_normalisation_from_tf_asymmetry_simultaneous_function(tf_function, function_index)
        else:
            return self._get_normalisation_from_tf_asymmetry_single_fit_function(tf_function)

    def _current_normalisation_from_tf_asymmetry_single_fit_function(self) -> float:
        """Returns the normalisation in the currently selected TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        return self._get_normalisation_from_tf_asymmetry_single_fit_function(current_tf_single_fit_function)

    @staticmethod
    def _get_normalisation_from_tf_asymmetry_single_fit_function(tf_single_fit_function: IFunction) -> float:
        """Returns the normalisation in the specified TF Asymmetry single fit function."""
        if tf_single_fit_function is not None:
            return tf_single_fit_function.getParameterValue(NORMALISATION_PARAMETER)
        else:
            return DEFAULT_NORMALISATION

    def _current_normalisation_error_from_tf_asymmetry_single_fit_function(self) -> float:
        """Returns the normalisation error in the currently selected TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            return current_tf_single_fit_function.getError(NORMALISATION_PARAMETER)
        else:
            return DEFAULT_NORMALISATION_ERROR

    def _is_current_normalisation_fixed_in_tf_asymmetry_single_fit_mode(self) -> bool:
        """Returns true if the currently selected normalisation in single fit mode has its value fixed."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            parameter_index = current_tf_single_fit_function.getParameterIndex(NORMALISATION_PARAMETER)
            return current_tf_single_fit_function.isFixed(parameter_index)
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def _toggle_fix_current_normalisation_in_tf_asymmetry_single_fit_mode(self, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value in single fit mode, or unfixes it."""
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            if is_fixed:
                current_tf_single_fit_function.fixParameter(NORMALISATION_PARAMETER)
            else:
                current_tf_single_fit_function.freeParameter(NORMALISATION_PARAMETER)

    def _current_normalisation_from_tf_asymmetry_simultaneous_function(self) -> float:
        """Returns the normalisation in the current domain of the TF Asymmetry simultaneous fit function."""
        return self._get_normalisation_from_tf_asymmetry_simultaneous_function(self.tf_asymmetry_simultaneous_function,
                                                                               self.current_dataset_index)

    def _get_normalisation_from_tf_asymmetry_simultaneous_function(self, tf_simultaneous_function: IFunction,
                                                                   domain_index: int) -> float:
        """Returns the normalisation in the specified domain of the TF Asymmetry simultaneous fit function."""
        if tf_simultaneous_function is None or domain_index >= self.number_of_datasets:
            return DEFAULT_NORMALISATION

        if self.number_of_datasets > 1:
            return tf_simultaneous_function.getParameterValue(
                self._get_normalisation_function_parameter_for_simultaneous_domain(domain_index))
        else:
            return tf_simultaneous_function.getParameterValue(NORMALISATION_PARAMETER)

    def _current_normalisation_error_from_tf_asymmetry_simultaneous_function(self) -> float:
        """Returns the normalisation error in the current domain of the TF Asymmetry simultaneous fit function."""
        if self.tf_asymmetry_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                return self.tf_asymmetry_simultaneous_function.getError(
                    self._get_normalisation_function_parameter_for_simultaneous_domain(self.current_dataset_index))
            else:
                return self.tf_asymmetry_simultaneous_function.getError(NORMALISATION_PARAMETER)
        else:
            return DEFAULT_NORMALISATION_ERROR

    def _is_current_normalisation_fixed_in_tf_asymmetry_simultaneous_mode(self) -> bool:
        """Returns true if the currently selected normalisation in simultaneous fit mode has its value fixed."""
        if self.tf_asymmetry_simultaneous_function is not None:
            full_parameter = NORMALISATION_PARAMETER
            if self.number_of_datasets > 1:
                full_parameter = f"f{self.current_dataset_index}." + full_parameter

            parameter_index = self.tf_asymmetry_simultaneous_function.getParameterIndex(full_parameter)
            return self.tf_asymmetry_simultaneous_function.isFixed(parameter_index)
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def _toggle_fix_current_normalisation_in_tf_asymmetry_simultaneous_mode(self, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value in simultaneous mode, or unfixes it."""
        if self.tf_asymmetry_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                full_parameter = self._get_normalisation_function_parameter_for_simultaneous_domain(
                    self.current_dataset_index)
            else:
                full_parameter = NORMALISATION_PARAMETER

            if is_fixed:
                self.tf_asymmetry_simultaneous_function.fixParameter(full_parameter)
            else:
                self.tf_asymmetry_simultaneous_function.freeParameter(full_parameter)

    def perform_fit(self) -> tuple:
        """Performs a fit. Allows the possibility of a TF Asymmetry fit in single fit mode and simultaneous fit mode."""
        if self.tf_asymmetry_mode:
            return self._do_tf_asymmetry_fit()
        else:
            return super().perform_fit()

    def _do_tf_asymmetry_fit(self) -> tuple:
        """Performs a TF Asymmetry fit in either the single fit mode or simultaneous fit mode."""
        if self.simultaneous_fitting_mode:
            params = self._get_parameters_for_tf_asymmetry_simultaneous_fit(self.dataset_names,
                                                                            self.tf_asymmetry_simultaneous_function)
            return self._do_tf_asymmetry_simultaneous_fit(params, self._get_global_parameters_for_tf_asymmetry_fit())
        else:
            params = self._get_parameters_for_tf_asymmetry_single_fit(self.current_dataset_name,
                                                                      self.current_tf_asymmetry_single_function)
            return self._do_tf_asymmetry_single_fit(params)

    def _do_tf_asymmetry_single_fit(self, parameters: dict) -> tuple:
        """Performs a TF Asymmetry fit in single fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._run_tf_asymmetry_fit(parameters)

        dataset_name = parameters["ReNormalizedWorkspaceList"]
        CopyLogs(InputWorkspace=dataset_name, OutputWorkspace=output_workspace, StoreInADS=False)
        self._add_single_fit_results_to_ADS_and_context(dataset_name, parameter_table, output_workspace,
                                                        covariance_matrix)
        return function, fit_status, chi_squared

    def _do_tf_asymmetry_simultaneous_fit(self, parameters: dict, global_parameters: list) -> tuple:
        """Performs a TF Asymmetry fit in simultaneous fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = \
            self._run_tf_asymmetry_fit(parameters)

        dataset_names = parameters["ReNormalizedWorkspaceList"]
        self._copy_logs(dataset_names, output_workspace)
        self._add_simultaneous_fit_results_to_ADS_and_context(dataset_names, parameter_table, output_workspace,
                                                              covariance_matrix, global_parameters)
        return function, fit_status, chi_squared

    def _run_tf_asymmetry_fit(self, parameters: dict) -> tuple:
        """Performs the TF Asymmetry fit using the parameters dict provided."""
        alg = AlgorithmManager.create("CalculateMuonAsymmetry")
        return run_CalculateMuonAsymmetry(parameters, alg)

    def _get_parameters_for_tf_asymmetry_single_fit(self, dataset_name: str, tf_single_function: IFunction) -> dict:
        """Returns the parameters to use for a single TF Asymmetry fit."""
        params = self._get_common_tf_asymmetry_parameters()
        params["InputFunction"] = tf_single_function.clone()
        params["ReNormalizedWorkspaceList"] = dataset_name
        params["UnNormalizedWorkspaceList"] = self._get_unnormalised_workspace_list([dataset_name])[0]

        fit_workspace_name, _ = create_fitted_workspace_name(dataset_name, self.function_name)
        params["OutputFitWorkspace"] = fit_workspace_name
        return params

    def _get_parameters_for_tf_asymmetry_simultaneous_fit(self, dataset_names: list,
                                                          tf_simultaneous_function: IFunction) -> dict:
        """Returns the parameters to use for a simultaneous TF Asymmetry fit."""
        params = self._get_common_tf_asymmetry_parameters()
        params["InputFunction"] = str(tf_simultaneous_function) + self._construct_global_tie_appendage()
        params["ReNormalizedWorkspaceList"] = dataset_names
        params["UnNormalizedWorkspaceList"] = self._get_unnormalised_workspace_list(dataset_names)

        fit_workspace_name, _ = create_multi_domain_fitted_workspace_name(dataset_names[0], self.function_name)
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
            global_tie_appendage = str(self.simultaneous_fit_function).split(";")[-1]
            if global_tie_appendage[:6] == "ties=(" and global_tie_appendage[-1] == ")":
                tf_global_ties = self._convert_global_ties_for_tf_asymmetry_mode(global_tie_appendage[6:-1].split(","))
                return f";ties=({','.join(tf_global_ties)})"
        return ""

    def _convert_global_ties_for_tf_asymmetry_mode(self, global_ties: str) -> str:
        """Converts the global ties to the equivalent global ties in the TF Asymmetry function."""
        return [self._convert_global_tie_for_tf_asymmetry_mode(global_tie) for global_tie in global_ties]

    def _convert_global_tie_for_tf_asymmetry_mode(self, global_tie: str) -> str:
        """Converts a global tie to the equivalent global tie in the TF Asymmetry function."""
        tf_parameters = [self._convert_parameter_to_tf_asymmetry_mode(parameter) for parameter in global_tie.split("=")]
        return "=".join(tf_parameters)

    @staticmethod
    def _convert_parameter_to_tf_asymmetry_mode(parameter: str) -> str:
        """Converts a normal parameter to the equivalent parameter in the TF Asymmetry function."""
        split_parameter = parameter.split(".")
        return split_parameter[0] + f".{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}" + ".".join(split_parameter[1:])

    def _get_global_parameters_for_tf_asymmetry_fit(self) -> list:
        """Returns a list of global parameters in TF Asymmetry format."""
        return [str(TF_ASYMMETRY_PREFIX_FUNCTION_INDEX + global_param) for global_param in self.global_parameters]

    def _get_unnormalised_workspace_list(self, normalised_workspaces: list) -> list:
        """Returns a list of unnormalised workspaces to be used within a TF Asymmetry fit."""
        return self.context.group_pair_context.get_unormalisised_workspace_list(normalised_workspaces)

    def _add_fit_to_context(self, parameter_workspace, input_workspaces, output_workspaces,
                            global_parameters: list = None) -> None:
        """Adds the results of a fit to the context. Overrides the method in BasicFittingModel."""
        self.context.fitting_context.add_fit_from_values(parameter_workspace, self.function_name, input_workspaces,
                                                         output_workspaces, global_parameters, self.tf_asymmetry_mode)

    """
    Methods used by the Sequential Fitting Tab
    """

    def _parse_parameter_values(self, all_parameter_values: list):
        """Separate the parameter values into the normalisations and ordinary parameter values."""
        if not self.tf_asymmetry_mode:
            return all_parameter_values, []

        if not self.simultaneous_fitting_mode:
            return all_parameter_values[1:], all_parameter_values[:1]

        return self._parse_parameter_values_for_tf_asymmetry_simultaneous_mode(all_parameter_values)

    def _parse_parameter_values_for_tf_asymmetry_simultaneous_mode(self, all_parameter_values: list):
        """Separate the parameter values into the normalisations and ordinary parameter values."""
        n_params_per_domain = int(len(all_parameter_values) / self.number_of_datasets)

        parameter_values, normalisations = [], []
        for parameter_index in range(len(all_parameter_values)):
            if parameter_index % n_params_per_domain == 0:
                normalisations.append(all_parameter_values[parameter_index])
            else:
                parameter_values.append(all_parameter_values[parameter_index])

        return parameter_values, normalisations

    def update_ws_fit_function_parameters(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameter values for the given dataset names."""
        parameter_values, normalisations = self._parse_parameter_values(parameter_values)

        if self.simultaneous_fitting_mode:
            self._update_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)
        else:
            self._update_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

        if self.tf_asymmetry_mode:
            self._update_tf_fit_function_normalisation(dataset_names, normalisations)

    def _update_tf_fit_function_normalisation(self, dataset_names: list, normalisations: list) -> None:
        """Updates the normalisation values for the tf asymmetry functions."""
        if self.simultaneous_fitting_mode:
            for name, normalisation in zip(dataset_names, normalisations):
                self._set_normalisation_for_dataset(name, normalisation)
        else:
            self._set_normalisation_for_dataset(dataset_names[0], normalisations[0])

    def _update_fit_function_parameters_for_single_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in single fit mode."""
        for name in dataset_names:
            fit_function = self.get_single_fit_function_for(name)
            if fit_function is not None:
                self._set_fit_function_parameter_values(fit_function, parameter_values)

        if self.tf_asymmetry_mode:
            self._update_tf_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

    def _update_tf_fit_function_parameters_for_single_fit(self, dataset_names: list, parameter_values: list):
        """Updates the tf asymmetry function parameters for the given dataset names if in single fit mode."""
        for name in dataset_names:
            if name in self.dataset_names:
                tf_single_function = self.get_tf_asymmetry_single_function(self.dataset_names.index(name))
                self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_single_function),
                                                        parameter_values)

    def _update_fit_function_parameters_for_simultaneous_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in simultaneous fit mode."""
        self._set_fit_function_parameter_values(self.simultaneous_fit_function, parameter_values)

        if self.tf_asymmetry_mode:
            self._update_tf_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)

    def _update_tf_fit_function_parameters_for_simultaneous_fit(self, dataset_names: list, parameter_values: list):
        """Updates the tf asymmetry function parameters for the given dataset names if in simultaneous fit mode."""
        number_parameters_per_domain = int(len(parameter_values) / len(dataset_names))
        for name in dataset_names:
            if name in self.dataset_names:
                self._set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(
                    self.tf_asymmetry_simultaneous_function, self.dataset_names.index(name), parameter_values,
                    number_parameters_per_domain)

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

    def get_runs_groups_and_pairs_for_fits(self):
        """Returns the runs and group/pairs corresponding to the selected dataset names."""
        if not self.simultaneous_fitting_mode:
            return self._get_runs_groups_and_pairs_for_single_fit()
        else:
            return self._get_runs_groups_and_pairs_for_simultaneous_fit()

    def _get_runs_groups_and_pairs_for_single_fit(self) -> tuple:
        """Returns the runs and group/pairs corresponding to the selected dataset names in single fitting mode."""
        runs, groups_and_pairs = [], []
        for name in self.dataset_names:
            runs.append(get_run_numbers_as_string_from_workspace_name(name, self.context.data_context.instrument))
            groups_and_pairs.append(get_group_or_pair_from_name(name))
        return runs, groups_and_pairs

    def _get_runs_groups_and_pairs_for_simultaneous_fit(self) -> tuple:
        """Returns the runs and group/pairs corresponding to the selected dataset names in simultaneous fitting mode."""
        if self.simultaneous_fit_by == "Run":
            return self._get_runs_groups_and_pairs_for_simultaneous_fit_by_runs()
        elif self.simultaneous_fit_by == "Group/Pair":
            return self._get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs()
        else:
            return [], []

    def _get_runs_groups_and_pairs_for_simultaneous_fit_by_runs(self):
        """Returns the runs and group/pairs for the selected data in simultaneous fit by runs mode."""
        runs = self._get_selected_runs()
        groups_and_pairs = [get_group_or_pair_from_name(name) for name in self.dataset_names]
        return runs, [";".join(groups_and_pairs)] * len(runs)

    def _get_runs_groups_and_pairs_for_simultaneous_fit_by_groups_and_pairs(self):
        """Returns the runs and group/pairs for the selected data in simultaneous fit by group/pairs mode."""
        runs = [get_run_numbers_as_string_from_workspace_name(name, self.context.data_context.instrument)
                for name in self.dataset_names]
        groups_and_pairs = self._get_selected_groups_and_pairs()
        return [";".join(runs)] * len(groups_and_pairs), groups_and_pairs

    def perform_sequential_fit(self, workspaces: list, parameter_values: list, use_initial_values: bool = False):
        """Performs a sequential fit of the workspace names provided for the current fitting mode.

        :param workspaces: A list of lists of workspace names e.g. [[Row 1 workspaces], [Row 2 workspaces], etc...]
        :param parameter_values: A list of lists of parameter values e.g. [[Row 1 params], [Row 2 params], etc...]
        :param use_initial_values: If false the parameters at the end of each fit are passed on to the next fit.
        """
        if self.tf_asymmetry_mode:
            fitting_func = self._get_sequential_fitting_func_for_tf_asymmetry_fitting_mode()
        else:
            fitting_func = self._get_sequential_fitting_func_for_normal_fitting_mode()

        if not self.simultaneous_fitting_mode:
            workspaces = self._flatten_workspace_names(workspaces)

        functions, fit_statuses, chi_squared_list = self._evaluate_sequential_fit(
            fitting_func, workspaces, parameter_values, use_initial_values)

        self._update_fit_functions_after_sequential_fit(workspaces, functions)
        self._update_fit_statuses_and_chi_squared_after_sequential_fit(workspaces, fit_statuses, chi_squared_list)
        return functions, fit_statuses, chi_squared_list

    def _get_sequential_fitting_func_for_normal_fitting_mode(self):
        """Returns the fitting func to use when performing a fit in normal fitting mode."""
        if self.simultaneous_fitting_mode:
            return self._do_sequential_simultaneous_fits
        else:
            return self._do_sequential_fit

    def _get_sequential_fitting_func_for_tf_asymmetry_fitting_mode(self):
        """Returns the fitting func to use when performing a fit in TF Asymmetry fitting mode."""
        if self.simultaneous_fitting_mode:
            return self._do_sequential_tf_asymmetry_simultaneous_fits
        else:
            return self._do_sequential_tf_asymmetry_fit

    @staticmethod
    def _flatten_workspace_names(workspaces: list) -> list:
        """Provides a workspace name list of lists to be flattened if in single fitting mode."""
        return [workspace for fit_workspaces in workspaces for workspace in fit_workspaces]

    @staticmethod
    def _evaluate_sequential_fit(fitting_func, workspace_names: list, parameter_values: list,
                                 use_initial_values: bool = False):
        """Evaluates a sequential fit using the provided fitting func. The workspace_names is either a 1D or 2D list."""
        functions, fit_statuses, chi_squared_list = [], [], []

        for row_index, row_workspaces in enumerate(workspace_names):
            function, fit_status, chi_squared = fitting_func(row_index, row_workspaces, parameter_values[row_index],
                                                             functions, use_initial_values)

            functions.append(function)
            fit_statuses.append(fit_status)
            chi_squared_list.append(chi_squared)

        return functions, fit_statuses, chi_squared_list

    def _do_sequential_fit(self, row_index: int, workspace_name: str, parameter_values: list, functions: list,
                           use_initial_values: bool = False):
        """Performs a sequential fit of the single fit data."""
        single_function = functions[row_index - 1].clone() if not use_initial_values and row_index >= 1 else \
            self._get_single_function_with_parameters(parameter_values)

        params = self._get_parameters_for_single_fit(workspace_name, single_function)

        return self._do_single_fit(params)

    def _get_single_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        single_fit_function = self.current_single_fit_function.clone()
        self._set_fit_function_parameter_values(single_fit_function, parameter_values)
        return single_fit_function

    def _do_sequential_simultaneous_fits(self, row_index: int, workspace_names: list, parameter_values: list,
                                         functions: list, use_initial_values: bool = False):
        """Performs a number of simultaneous fits, sequentially."""
        simultaneous_function = functions[row_index - 1].clone() if not use_initial_values and row_index >= 1 else \
            self._get_simultaneous_function_with_parameters(parameter_values)

        params = self._get_parameters_for_simultaneous_fit(workspace_names, simultaneous_function)

        return self._do_simultaneous_fit(params, self.global_parameters)

    def _get_simultaneous_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current simultaneous function but with the parameter values provided."""
        simultaneous_function = self.simultaneous_fit_function.clone()
        self._set_fit_function_parameter_values(simultaneous_function, parameter_values)
        return simultaneous_function

    def _do_sequential_tf_asymmetry_fit(self, row_index: int, workspace_name: str, parameter_values: list,
                                        functions: list, use_initial_values: bool = False):
        """Performs a sequential fit of the TF Asymmetry single fit data."""
        tf_single_function = functions[row_index - 1].clone() if not use_initial_values and row_index >= 1 else \
            self._get_tf_asymmetry_single_function_with_parameters(parameter_values)

        params = self._get_parameters_for_tf_asymmetry_single_fit(workspace_name, tf_single_function)

        return self._do_tf_asymmetry_single_fit(params)

    def _get_tf_asymmetry_single_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        parameter_values, normalisations = self._parse_parameter_values(parameter_values)

        tf_single_function = self.current_tf_asymmetry_single_function.clone()
        self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_single_function),
                                                parameter_values)

        tf_single_function.setParameter(NORMALISATION_PARAMETER, normalisations[0])
        return tf_single_function

    def _do_sequential_tf_asymmetry_simultaneous_fits(self, row_index: int, workspace_names: list,
                                                      parameter_values: list, functions: list,
                                                      use_initial_values: bool = False):
        """Performs a number of TF Asymmetry simultaneous fits, sequentially."""
        tf_simultaneous_function = functions[row_index - 1].clone() if not use_initial_values and row_index >= 1 else \
            self._get_tf_asymmetry_simultaneous_function_with_parameters(parameter_values)

        params = self._get_parameters_for_tf_asymmetry_simultaneous_fit(workspace_names, tf_simultaneous_function)

        return self._do_tf_asymmetry_simultaneous_fit(params, self._get_global_parameters_for_tf_asymmetry_fit())

    def _get_tf_asymmetry_simultaneous_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        parameter_values, normalisations = self._parse_parameter_values(parameter_values)
        number_parameters_per_domain = int(len(parameter_values) / self.number_of_datasets)

        tf_simultaneous_function = self.tf_asymmetry_simultaneous_function.clone()
        for dataset_index in range(self.number_of_datasets):
            self._set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(
                tf_simultaneous_function, dataset_index, parameter_values, number_parameters_per_domain)
            self._set_normalisation_in_tf_asymmetry_simultaneous_function(tf_simultaneous_function, dataset_index,
                                                                          normalisations[dataset_index])

        return tf_simultaneous_function

    def _set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(self, tf_simultaneous_function: IFunction,
                                                                           dataset_index: int, parameter_values: list,
                                                                           number_parameters_per_domain: int) -> None:
        """Sets the parameter values within a domain of a TF Asymmetry simultaneous function."""
        parameter_values_for_domain = parameter_values[dataset_index * number_parameters_per_domain:
                                                       (dataset_index + 1) * number_parameters_per_domain]

        tf_domain_function = self.get_domain_tf_asymmetry_fit_function(tf_simultaneous_function, dataset_index)
        self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_domain_function),
                                                parameter_values_for_domain)

    def _update_fit_functions_after_sequential_fit(self, workspaces: list, functions: list) -> None:
        """Updates the fit functions after a sequential fit has been run on the Sequential fitting tab."""
        if self.tf_asymmetry_mode:
            if self.simultaneous_fitting_mode:
                self._update_tf_asymmetry_simultaneous_fit_function_after_sequential(workspaces, functions)
            else:
                self._update_tf_asymmetry_single_fit_functions_after_sequential(workspaces, functions)
        else:
            if self.simultaneous_fitting_mode:
                self._update_simultaneous_fit_function_after_sequential(workspaces, functions)
            else:
                self._update_single_fit_functions_after_sequential(workspaces, functions)

    def _update_single_fit_functions_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in self.dataset_names:
                dataset_index = self.dataset_names.index(workspace_name)
                self.single_fit_functions[dataset_index] = functions[workspace_index]

    def _update_tf_asymmetry_single_fit_functions_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the TF single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in self.dataset_names:
                dataset_index = self.dataset_names.index(workspace_name)
                self.update_tf_asymmetry_single_fit_function(dataset_index, functions[workspace_index])

    def _update_simultaneous_fit_function_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                self.simultaneous_fit_function = functions[fit_index]
                break

    def _update_tf_asymmetry_simultaneous_fit_function_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                self.update_tf_asymmetry_simultaneous_fit_function(functions[fit_index])
                break

    def _update_fit_statuses_and_chi_squared_after_sequential_fit(self, workspaces, fit_statuses, chi_squared_list):
        """Updates the fit statuses and chi squared after a sequential fit."""
        if self.simultaneous_fitting_mode:
            self._update_fit_statuses_and_chi_squared_for_simultaneous_mode(workspaces, fit_statuses, chi_squared_list)
        else:
            self._update_fit_statuses_and_chi_squared_for_single_mode(workspaces, fit_statuses, chi_squared_list)

    def _update_fit_statuses_and_chi_squared_for_single_mode(self, workspaces: list, fit_statuses: list,
                                                             chi_squared_list: list) -> None:
        """Updates the fit statuses and chi squared after a sequential fit when in single fit mode."""
        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in self.dataset_names:
                dataset_index = self.dataset_names.index(workspace_name)
                self.fit_statuses[dataset_index] = fit_statuses[workspace_index]
                self.chi_squared[dataset_index] = chi_squared_list[workspace_index]

    def _update_fit_statuses_and_chi_squared_for_simultaneous_mode(self, workspaces: list, fit_statuses: list,
                                                                   chi_squared_list: list) -> None:
        """Updates the fit statuses and chi squared after a sequential fit when in simultaneous fit mode."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                for workspace_name in workspace_names:
                    dataset_index = self.dataset_names.index(workspace_name)
                    self.fit_statuses[dataset_index] = fit_statuses[fit_index]
                    self.chi_squared[dataset_index] = chi_squared_list[fit_index]
                break

    def _are_same_workspaces_as_the_datasets(self, workspace_names: list) -> bool:
        """Checks that the workspace names provided are all in the loaded dataset names."""
        not_in_datasets = [name for name in workspace_names if name not in self.dataset_names]
        return False if len(not_in_datasets) > 0 else True
