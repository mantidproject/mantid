# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import AlgorithmManager, logger
from mantid.api import CompositeFunction, IFunction, MultiDomainFunction
from mantid.simpleapi import CopyLogs, ConvertFitFunctionForMuonTFAsymmetry

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    create_fitted_workspace_name,
    create_multi_domain_fitted_workspace_name,
    get_group_or_pair_from_name,
)
from mantidqtinterfaces.Muon.GUI.Common.contexts.fitting_contexts.tf_asymmetry_fitting_context import TFAsymmetryFittingContext
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import DEFAULT_SINGLE_FIT_FUNCTION
from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import run_CalculateMuonAsymmetry
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

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

    def __init__(self, context: MuonContext, fitting_context: TFAsymmetryFittingContext):
        """Initialize the TFAsymmetryFittingModel with emtpy fit data."""
        super(TFAsymmetryFittingModel, self).__init__(context, fitting_context)

    @GeneralFittingModel.dataset_names.setter
    def dataset_names(self, names: list) -> None:
        """Sets the dataset names stored by the model. Resets the other fitting data."""
        GeneralFittingModel.dataset_names.fset(self, names)
        self.recalculate_tf_asymmetry_functions()

    @property
    def tf_asymmetry_mode(self) -> bool:
        """Returns true if TF Asymmetry fitting mode is currently active."""
        return self.fitting_context.tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on: bool) -> None:
        """Sets the TF Asymmetry mode as being on or off."""
        self.fitting_context.tf_asymmetry_mode = tf_asymmetry_on

    @property
    def tf_asymmetry_single_functions(self) -> list:
        """Returns the fit functions used for single TF Asymmetry fitting. Each function corresponds to a dataset."""
        return self.fitting_context.tf_asymmetry_single_functions

    @tf_asymmetry_single_functions.setter
    def tf_asymmetry_single_functions(self, tf_asymmetry_functions: list) -> None:
        """Sets the single TF Asymmetry fit functions stored in the model."""
        self.fitting_context.tf_asymmetry_single_functions = tf_asymmetry_functions

    @property
    def current_tf_asymmetry_single_function(self) -> IFunction:
        """Returns the currently selected TF Asymmetry fit function for single fitting."""
        return self.get_tf_asymmetry_single_function(self.fitting_context.current_dataset_index)

    def get_tf_asymmetry_single_function(self, dataset_index: int) -> IFunction:
        """Returns the TF Asymmetry fit function for single fitting for the specified dataset index."""
        if dataset_index is not None:
            return self.fitting_context.tf_asymmetry_single_functions[dataset_index]
        else:
            return DEFAULT_SINGLE_FIT_FUNCTION

    def update_tf_asymmetry_single_fit_function(self, dataset_index: int, fit_function: IFunction) -> None:
        """Updates the specified TF Asymmetry and ordinary single fit function."""
        self.fitting_context.tf_asymmetry_single_functions[dataset_index] = fit_function
        self.fitting_context.single_fit_functions[dataset_index] = self._get_normal_fit_function_from(fit_function)

    @property
    def tf_asymmetry_simultaneous_function(self) -> IFunction:
        """Returns the simultaneous TF Asymmetry fit function stored in the model."""
        return self.fitting_context.tf_asymmetry_simultaneous_function

    @tf_asymmetry_simultaneous_function.setter
    def tf_asymmetry_simultaneous_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Sets the simultaneous TF Asymmetry fit function stored in the model."""
        self.fitting_context.tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

    def update_tf_asymmetry_simultaneous_fit_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Updates the TF Asymmetry and normal simultaneous fit function based on the function from a TFA fit."""
        self.fitting_context.tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

        if isinstance(self.fitting_context.simultaneous_fit_function, MultiDomainFunction) and isinstance(
            tf_asymmetry_simultaneous_function, MultiDomainFunction
        ):
            self._update_parameters_of_multi_domain_simultaneous_function_from(tf_asymmetry_simultaneous_function)
        else:
            self.fitting_context.simultaneous_fit_function = self._get_normal_fit_function_from(tf_asymmetry_simultaneous_function)

    def _update_parameters_of_multi_domain_simultaneous_function_from(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Updates the parameters in the normal simultaneous function based on a TF Asymmetry simultaneous function."""
        for domain_index in range(
            min([tf_asymmetry_simultaneous_function.nFunctions(), self.fitting_context.simultaneous_fit_function.nFunctions()])
        ):
            tf_asymmetry_domain_function = tf_asymmetry_simultaneous_function.getFunction(domain_index)
            parameter_values, errors = self.get_fit_function_parameter_values(
                self._get_normal_fit_function_from(tf_asymmetry_domain_function)
            )

            simultaneous_function = self.fitting_context.simultaneous_fit_function.getFunction(domain_index)
            self._set_fit_function_parameter_values(simultaneous_function, parameter_values, errors)

    def get_domain_tf_asymmetry_fit_function(self, tf_simultaneous_function: IFunction, dataset_index: int) -> IFunction:
        """Returns the fit function in the TF Asymmetry simultaneous function corresponding to the specified index."""
        if self.fitting_context.number_of_datasets < 2 or not isinstance(tf_simultaneous_function, MultiDomainFunction):
            return tf_simultaneous_function

        index = dataset_index if dataset_index is not None else 0
        return tf_simultaneous_function.getFunction(index)

    def reset_tf_asymmetry_functions(self) -> None:
        """Resets the TF Asymmetry fit functions."""
        self.fitting_context.tf_asymmetry_single_functions = [None] * self.fitting_context.number_of_datasets
        self.fitting_context.tf_asymmetry_simultaneous_function = None

    def recalculate_tf_asymmetry_functions(self) -> bool:
        """Recalculates the TF Asymmetry functions based on the datasets and normal functions in the model."""
        tf_compliant, _ = self.check_datasets_are_tf_asymmetry_compliant()
        if self.fitting_context.tf_asymmetry_mode and tf_compliant:
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
        current_dataset_index = self.fitting_context.current_dataset_index
        if current_dataset_index is not None:
            if self.fitting_context.simultaneous_fitting_mode:
                self._set_normalisation_in_tf_asymmetry_simultaneous_function(
                    self.fitting_context.tf_asymmetry_simultaneous_function, current_dataset_index, value
                )
            else:
                self._set_normalisation_in_tf_asymmetry_single_fit_function(current_dataset_index, value)

    def _set_normalisation_for_dataset(self, dataset_name: str, value: float) -> None:
        """Sets the normalisation for the given dataset name."""
        dataset_names = self.fitting_context.dataset_names
        if dataset_name in dataset_names:
            dataset_index = dataset_names.index(dataset_name)
            if self.fitting_context.simultaneous_fitting_mode:
                self._set_normalisation_in_tf_asymmetry_simultaneous_function(
                    self.fitting_context.tf_asymmetry_simultaneous_function, dataset_index, value
                )
            else:
                self._set_normalisation_in_tf_asymmetry_single_fit_function(dataset_index, value)

    def _set_normalisation_in_tf_asymmetry_single_fit_function(self, function_index: int, value: float) -> None:
        """Sets the normalisation in the specified TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[function_index]
        if current_tf_single_fit_function is not None:
            current_tf_single_fit_function.setParameter(NORMALISATION_PARAMETER, value)

    def _set_normalisation_in_tf_asymmetry_simultaneous_function(
        self, tf_simultaneous_function: IFunction, domain_index: int, value: float
    ) -> None:
        """Sets the normalisation in the specified domain of the TF Asymmetry simultaneous function."""
        if tf_simultaneous_function is not None:
            if self.fitting_context.number_of_datasets > 1:
                tf_simultaneous_function.setParameter(
                    self._get_normalisation_function_parameter_for_simultaneous_domain(domain_index), value
                )
            else:
                tf_simultaneous_function.setParameter(NORMALISATION_PARAMETER, value)

    def current_normalisation(self) -> float:
        """Returns the normalisation of the current TF Asymmetry single function or simultaneous function."""
        if self.fitting_context.current_dataset_index is not None:
            if self.fitting_context.simultaneous_fitting_mode:
                return self._current_normalisation_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION

    def current_normalisation_error(self) -> float:
        """Returns the normalisation error of the current TF Asymmetry single function or simultaneous function."""
        if self.fitting_context.current_dataset_index is not None:
            if self.fitting_context.simultaneous_fitting_mode:
                return self._current_normalisation_error_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_error_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION_ERROR

    def is_current_normalisation_fixed(self) -> bool:
        """Returns true if the currently selected normalisation has its value fixed."""
        if self.fitting_context.current_dataset_index is not None:
            if self.fitting_context.simultaneous_fitting_mode:
                return self._is_normalisation_fixed_in_tf_asymmetry_simultaneous_mode(self.current_dataset_index)
            else:
                return self._is_normalisation_fixed_in_tf_asymmetry_single_fit_mode(self.current_dataset_index)
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def toggle_fix_current_normalisation(self, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value, or unfixes it."""
        if self.fitting_context.current_dataset_index is not None:
            if self.fitting_context.simultaneous_fitting_mode:
                self._toggle_fix_normalisation_in_tf_asymmetry_simultaneous_mode(self.current_dataset_index, is_fixed)
            else:
                self._toggle_fix_normalisation_in_tf_asymmetry_single_fit_mode(self.current_dataset_index, is_fixed)

    def update_parameter_value(self, full_parameter: str, value: float) -> None:
        """Update the value of a parameter in the TF Asymmetry fit functions."""
        super().update_parameter_value(full_parameter, value)

        if self.fitting_context.tf_asymmetry_mode:
            tf_asymmetry_full_parameter = f"{TF_ASYMMETRY_PREFIX_FUNCTION_INDEX}{full_parameter}"
            self._update_tf_asymmetry_parameter_value(self.fitting_context.current_dataset_index, tf_asymmetry_full_parameter, value)

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.fitting_context.function_name_auto_update:
            super().automatically_update_function_name()
            if self.fitting_context.tf_asymmetry_mode:
                self.fitting_context.function_name += TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE

    def check_datasets_are_tf_asymmetry_compliant(self, workspace_names: list = None) -> bool:
        """Returns true if the datasets stored in the model are compatible with TF Asymmetry mode."""
        workspace_names = self.fitting_context.dataset_names if workspace_names is None else workspace_names
        pair_names = [get_group_or_pair_from_name(name) for name in workspace_names if "Group" not in name]
        # Remove duplicates from the list
        pair_names = list(dict.fromkeys(pair_names))
        return len(pair_names) == 0, "'" + "', '".join(pair_names) + "'"

    def undo_previous_fit(self) -> None:
        """Undoes the previous fit using the saved undo data."""
        super().undo_previous_fit()

        if self.number_of_undos() > 0 and self.fitting_context.tf_asymmetry_mode:
            self.recalculate_tf_asymmetry_functions()
            self._set_all_normalisations(self.fitting_context.normalisations_for_undo.pop())
            self._toggle_all_normalisations_fixed(self.fitting_context.normalisations_fixed_for_undo.pop())

    def save_current_fit_function_to_undo_data(self) -> None:
        """Saves the current simultaneous fit function, and the single fit functions defined in the base class."""
        super().save_current_fit_function_to_undo_data()

        if self.fitting_context.tf_asymmetry_mode:
            self.fitting_context.normalisations_for_undo.append(self._get_all_normalisations())
            self.fitting_context.normalisations_fixed_for_undo.append(self._get_all_normalisations_fixed())

    def clear_undo_data(self) -> None:
        """Clears the saved simultaneous fit functions, and the saved single fit functions defined in the base class."""
        super().clear_undo_data()
        self.fitting_context.normalisations_for_undo = []
        self.fitting_context.normalisations_fixed_for_undo = []

    def get_fit_function_parameters(self) -> list:
        """Returns the names of the fit parameters in the fit functions."""
        parameters = super().get_fit_function_parameters()
        if self.fitting_context.tf_asymmetry_mode and self.fitting_context.number_of_datasets > 0:
            if self.fitting_context.simultaneous_fitting_mode:
                return self._add_normalisation_to_parameters_for_simultaneous_fitting(
                    parameters, self._get_normalisation_parameter_name_for_simultaneous_domain
                )
            else:
                return ["N0"] + parameters
        else:
            return parameters

    def _add_normalisation_to_parameters_for_simultaneous_fitting(self, parameters, get_normalisation_func):
        """Returns the names of the fit parameters in the fit functions including the normalisation for simultaneous."""
        number_of_datasets = self.fitting_context.number_of_datasets
        n_params_per_domain = int(len(parameters) / number_of_datasets)

        all_parameters = []
        for domain_index in range(number_of_datasets):
            all_parameters += [get_normalisation_func(domain_index)]
            all_parameters += parameters[domain_index * n_params_per_domain : (domain_index + 1) * n_params_per_domain]
        return all_parameters

    @staticmethod
    def _get_normalisation_parameter_name_for_simultaneous_domain(domain_index: int) -> str:
        """Returns the parameter name to use for a simultaneous normalisation parameter for a specific domain."""
        return f"f{domain_index}.N0"

    @staticmethod
    def _get_normalisation_function_parameter_for_simultaneous_domain(domain_index: int) -> str:
        """Returns the normalisation parameter for a specific domain as seen in the MultiDomainFunction."""
        return f"f{domain_index}.{NORMALISATION_PARAMETER}"

    def _get_normal_fit_function_from(self, tf_asymmetry_function: IFunction) -> IFunction:
        """Returns the ordinary fit function embedded within the TF Asymmetry fit function."""
        if isinstance(tf_asymmetry_function, CompositeFunction):
            domain_function = tf_asymmetry_function.getFunction(0)
            if isinstance(domain_function, CompositeFunction) and isinstance(domain_function.getFunction(1), CompositeFunction):
                return domain_function.getFunction(1).getFunction(1)
        return None

    def _update_tf_asymmetry_parameter_value(self, dataset_index: int, full_parameter: str, value: float) -> None:
        """Updates a parameters value within the current TF Asymmetry single function or simultaneous function."""
        if self.fitting_context.simultaneous_fitting_mode:
            domain_function = self.get_domain_tf_asymmetry_fit_function(
                self.fitting_context.tf_asymmetry_simultaneous_function, dataset_index
            )
        else:
            domain_function = self.get_tf_asymmetry_single_function(dataset_index)

        if domain_function is not None:
            domain_function.setParameter(full_parameter, value)

    def _recalculate_tf_asymmetry_functions(self) -> None:
        """Recalculates the TF Asymmetry single fit functions or simultaneous function."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._recalculate_tf_asymmetry_simultaneous_fit_function()
        else:
            self._recalculate_tf_asymmetry_single_fit_functions()

    def _recalculate_tf_asymmetry_single_fit_functions(self) -> None:
        """Recalculates the TF Asymmetry single fit functions."""
        tf_functions = []
        for index, single_function in enumerate(self.fitting_context.single_fit_functions):
            tf_functions.append(self._convert_to_tf_asymmetry_function(single_function, [self.fitting_context.dataset_names[index]]))
        self.tf_asymmetry_single_functions = tf_functions

    def _recalculate_tf_asymmetry_simultaneous_fit_function(self) -> None:
        """Recalculates the TF Asymmetry simultaneous function."""
        self.tf_asymmetry_simultaneous_function = self._convert_to_tf_asymmetry_function(
            self.fitting_context.simultaneous_fit_function, self.fitting_context.dataset_names
        )

    def _convert_to_tf_asymmetry_function(self, fit_function: IFunction, workspace_names: list) -> IFunction:
        """Converts a normal single fit or simultaneous function to a TF Asymmetry fit function."""
        if fit_function is None:
            return None

        parameters = self._get_parameters_for_tf_asymmetry_conversion(fit_function, workspace_names)
        return ConvertFitFunctionForMuonTFAsymmetry(StoreInADS=False, **parameters)

    def _get_parameters_for_tf_asymmetry_conversion(self, fit_function: IFunction, workspace_names: list) -> dict:
        """Returns the parameters used to convert a normal function to a TF Asymmetry fit function."""
        return {
            "InputFunction": fit_function,
            "WorkspaceList": workspace_names,
            "Mode": "Construct" if self.fitting_context.tf_asymmetry_mode else "Extract",
            "CopyTies": False,
        }

    def _set_all_normalisations(self, normalisations: list) -> None:
        """Sets the normalisations for each of the datasets."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._set_all_normalisations_for_simultaneous_fit_mode(normalisations)
        else:
            self._set_all_normalisations_for_single_fit_mode(normalisations)

    def _set_all_normalisations_for_single_fit_mode(self, normalisations: list) -> None:
        """Sets the normalisations within the TF Asymmetry single fit functions."""
        for index, normalisation in enumerate(normalisations):
            self._set_normalisation_in_tf_asymmetry_single_fit_function(index, normalisation)

    def _set_all_normalisations_for_simultaneous_fit_mode(self, normalisations: list) -> None:
        """Sets the normalisations within the TF Asymmetry simultaneous fit function."""
        tf_simultaneous_function = self.fitting_context.tf_asymmetry_simultaneous_function
        for index, normalisation in enumerate(normalisations):
            self._set_normalisation_in_tf_asymmetry_simultaneous_function(tf_simultaneous_function, index, normalisation)

    def _get_all_normalisations(self) -> list:
        """Returns the normalisations for each of the datasets."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._get_all_normalisations_for_simultaneous_fit_mode()
        else:
            return self._get_all_normalisations_for_single_fit_mode()

    def _get_all_normalisations_for_single_fit_mode(self) -> list:
        """Returns the normalisations within the TF Asymmetry single fit functions."""
        normalisations = []
        for dataset_index in range(self.fitting_context.number_of_datasets):
            tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[dataset_index]
            normalisations.append(self._get_normalisation_from_tf_asymmetry_single_fit_function(tf_single_fit_function))
        return normalisations

    def _get_all_normalisations_for_simultaneous_fit_mode(self) -> list:
        """Returns the normalisations within the TF Asymmetry simultaneous fit function."""
        normalisations = []
        tf_simultaneous_function = self.fitting_context.tf_asymmetry_simultaneous_function
        for dataset_index in range(self.fitting_context.number_of_datasets):
            normalisations.append(self._get_normalisation_from_tf_asymmetry_simultaneous_function(tf_simultaneous_function, dataset_index))
        return normalisations

    def _get_normalisation_from_tf_fit_function(self, tf_function: IFunction, function_index: int = 0) -> float:
        """Returns the normalisation in the specified TF Asymmetry fit function."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._get_normalisation_from_tf_asymmetry_simultaneous_function(tf_function, function_index)
        else:
            return self._get_normalisation_from_tf_asymmetry_single_fit_function(tf_function)

    def _current_normalisation_from_tf_asymmetry_single_fit_function(self) -> float:
        """Returns the normalisation in the currently selected TF Asymmetry single fit function."""
        current_tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[self.current_dataset_index]
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
        current_tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            return current_tf_single_fit_function.getError(NORMALISATION_PARAMETER)
        else:
            return DEFAULT_NORMALISATION_ERROR

    def _get_all_normalisations_fixed(self) -> list:
        """Returns whether the normalisation is fixed for each of the datasets."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._get_all_normalisations_fixed_for_simultaneous_fit_mode()
        else:
            return self._get_all_normalisations_fixed_for_single_fit_mode()

    def _get_all_normalisations_fixed_for_single_fit_mode(self) -> list:
        """Returns whether the normalisation is fixed for each of the datasets when in single fit mode."""
        return [
            self._is_normalisation_fixed_in_tf_asymmetry_single_fit_mode(dataset_index) for dataset_index in range(self.number_of_datasets)
        ]

    def _get_all_normalisations_fixed_for_simultaneous_fit_mode(self) -> list:
        """Returns whether the normalisation is fixed for each of the datasets in simultaneous fit mode."""
        return [
            self._is_normalisation_fixed_in_tf_asymmetry_simultaneous_mode(dataset_index)
            for dataset_index in range(self.number_of_datasets)
        ]

    def _is_normalisation_fixed_in_tf_asymmetry_single_fit_mode(self, dataset_index: int) -> bool:
        """Returns true if the currently selected normalisation in single fit mode has its value fixed."""
        current_tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[dataset_index]
        if current_tf_single_fit_function is not None:
            parameter_index = current_tf_single_fit_function.getParameterIndex(NORMALISATION_PARAMETER)
            return current_tf_single_fit_function.isFixed(parameter_index)
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def _toggle_all_normalisations_fixed(self, normalisations_fixed: list) -> None:
        """Toggle the fix state of each dataset using the corresponding booleans in the list provided."""
        if self.fitting_context.simultaneous_fitting_mode:
            self._toggle_all_normalisations_fixed_for_simultaneous_fit_mode(normalisations_fixed)
        else:
            self._toggle_all_normalisations_fixed_for_single_fit_mode(normalisations_fixed)

    def _toggle_all_normalisations_fixed_for_single_fit_mode(self, normalisations_fixed: list) -> None:
        """Sets the normalisations as fixed or unfixed within the TF Asymmetry single fit functions."""
        for index, normalisation_fixed in enumerate(normalisations_fixed):
            self._toggle_fix_normalisation_in_tf_asymmetry_single_fit_mode(index, normalisation_fixed)

    def _toggle_all_normalisations_fixed_for_simultaneous_fit_mode(self, normalisations_fixed: list) -> None:
        """Sets the normalisations as fixed or unfixed within the TF Asymmetry simultaneous fit function."""
        for index, normalisation_fixed in enumerate(normalisations_fixed):
            self._toggle_fix_normalisation_in_tf_asymmetry_simultaneous_mode(index, normalisation_fixed)

    def _toggle_fix_normalisation_in_tf_asymmetry_single_fit_mode(self, dataset_index: int, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value in single fit mode, or unfixes it."""
        current_tf_single_fit_function = self.fitting_context.tf_asymmetry_single_functions[dataset_index]
        if current_tf_single_fit_function is not None:
            if is_fixed:
                current_tf_single_fit_function.fixParameter(NORMALISATION_PARAMETER)
            else:
                current_tf_single_fit_function.freeParameter(NORMALISATION_PARAMETER)

    def _current_normalisation_from_tf_asymmetry_simultaneous_function(self) -> float:
        """Returns the normalisation in the current domain of the TF Asymmetry simultaneous fit function."""
        return self._get_normalisation_from_tf_asymmetry_simultaneous_function(
            self.fitting_context.tf_asymmetry_simultaneous_function, self.fitting_context.current_dataset_index
        )

    def _get_normalisation_from_tf_asymmetry_simultaneous_function(self, tf_simultaneous_function: IFunction, domain_index: int) -> float:
        """Returns the normalisation in the specified domain of the TF Asymmetry simultaneous fit function."""
        number_of_datasets = self.fitting_context.number_of_datasets
        if tf_simultaneous_function is None or domain_index >= number_of_datasets:
            return DEFAULT_NORMALISATION

        if number_of_datasets > 1:
            return tf_simultaneous_function.getParameterValue(
                self._get_normalisation_function_parameter_for_simultaneous_domain(domain_index)
            )
        else:
            return tf_simultaneous_function.getParameterValue(NORMALISATION_PARAMETER)

    def _current_normalisation_error_from_tf_asymmetry_simultaneous_function(self) -> float:
        """Returns the normalisation error in the current domain of the TF Asymmetry simultaneous fit function."""
        tf_simultaneous_function = self.fitting_context.tf_asymmetry_simultaneous_function
        if tf_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                return tf_simultaneous_function.getError(
                    self._get_normalisation_function_parameter_for_simultaneous_domain(self.fitting_context.current_dataset_index)
                )
            else:
                return tf_simultaneous_function.getError(NORMALISATION_PARAMETER)
        else:
            return DEFAULT_NORMALISATION_ERROR

    def _is_normalisation_fixed_in_tf_asymmetry_simultaneous_mode(self, dataset_index: int) -> bool:
        """Returns true if the currently selected normalisation in simultaneous fit mode has its value fixed."""
        tf_simultaneous_function = self.fitting_context.tf_asymmetry_simultaneous_function
        if tf_simultaneous_function is not None:
            full_parameter = NORMALISATION_PARAMETER
            if self.fitting_context.number_of_datasets > 1:
                full_parameter = f"f{dataset_index}." + full_parameter

            parameter_index = tf_simultaneous_function.getParameterIndex(full_parameter)
            return tf_simultaneous_function.isFixed(parameter_index)
        else:
            return DEFAULT_IS_NORMALISATION_FIXED

    def _toggle_fix_normalisation_in_tf_asymmetry_simultaneous_mode(self, dataset_index: int, is_fixed: bool) -> None:
        """Fixes the current normalisation to its current value in simultaneous mode, or unfixes it."""
        tf_simultaneous_function = self.fitting_context.tf_asymmetry_simultaneous_function
        if tf_simultaneous_function is not None:
            if self.fitting_context.number_of_datasets > 1:
                full_parameter = self._get_normalisation_function_parameter_for_simultaneous_domain(dataset_index)
            else:
                full_parameter = NORMALISATION_PARAMETER

            if is_fixed:
                tf_simultaneous_function.fixParameter(full_parameter)
            else:
                tf_simultaneous_function.freeParameter(full_parameter)

    def perform_fit(self) -> tuple:
        """Performs a fit. Allows the possibility of a TF Asymmetry fit in single fit mode and simultaneous fit mode."""
        if self.fitting_context.tf_asymmetry_mode:
            return self._do_tf_asymmetry_fit()
        else:
            return super().perform_fit()

    def _do_tf_asymmetry_fit(self) -> tuple:
        """Performs a TF Asymmetry fit in either the single fit mode or simultaneous fit mode."""
        if self.fitting_context.simultaneous_fitting_mode:
            params = self._get_parameters_for_tf_asymmetry_simultaneous_fit(
                self.fitting_context.dataset_names, self.fitting_context.tf_asymmetry_simultaneous_function
            )
            return self._do_tf_asymmetry_simultaneous_fit(params, self._get_global_parameters_for_tf_asymmetry_fit())
        else:
            params = self._get_parameters_for_tf_asymmetry_single_fit(self.current_dataset_name, self.current_tf_asymmetry_single_function)
            return self._do_tf_asymmetry_single_fit(params)

    def _do_tf_asymmetry_single_fit(self, parameters: dict) -> tuple:
        """Performs a TF Asymmetry fit in single fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = self._run_tf_asymmetry_fit(parameters)

        dataset_name = parameters["ReNormalizedWorkspaceList"]
        CopyLogs(InputWorkspace=dataset_name, OutputWorkspace=output_workspace, StoreInADS=False)
        self._add_single_fit_results_to_ADS_and_context(dataset_name, parameter_table, output_workspace, covariance_matrix)
        return function, fit_status, chi_squared

    def _do_tf_asymmetry_simultaneous_fit(self, parameters: dict, global_parameters: list) -> tuple:
        """Performs a TF Asymmetry fit in simultaneous fit mode."""
        output_workspace, parameter_table, function, fit_status, chi_squared, covariance_matrix = self._run_tf_asymmetry_fit(parameters)

        dataset_names = parameters["ReNormalizedWorkspaceList"]
        self._copy_logs(dataset_names, output_workspace)
        self._add_simultaneous_fit_results_to_ADS_and_context(
            dataset_names, parameter_table, output_workspace, covariance_matrix, global_parameters
        )
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

        fit_workspace_name, _ = create_fitted_workspace_name(dataset_name, self.fitting_context.function_name)
        params["OutputFitWorkspace"] = fit_workspace_name
        return params

    def _get_parameters_for_tf_asymmetry_simultaneous_fit(self, dataset_names: list, tf_simultaneous_function: IFunction) -> dict:
        """Returns the parameters to use for a simultaneous TF Asymmetry fit."""
        params = self._get_common_tf_asymmetry_parameters()
        params["InputFunction"] = str(tf_simultaneous_function) + self._construct_global_tie_appendage()
        params["ReNormalizedWorkspaceList"] = dataset_names
        params["UnNormalizedWorkspaceList"] = self._get_unnormalised_workspace_list(dataset_names)

        fit_workspace_name, _ = create_multi_domain_fitted_workspace_name(dataset_names[0], self.fitting_context.function_name)
        params["OutputFitWorkspace"] = fit_workspace_name
        return params

    def _get_common_tf_asymmetry_parameters(self) -> dict:
        """Returns the common parameters used for a TF Asymmetry fit."""
        params = {"StartX": self.current_start_x, "EndX": self.current_end_x, "Minimizer": self.fitting_context.minimizer}

        if self.fitting_context.exclude_range:
            params["Exclude"] = [self.current_exclude_start_x, self.current_exclude_end_x]

        if self._double_pulse_enabled():
            params.update(self._get_common_double_pulse_parameters())
        return params

    def _get_common_double_pulse_parameters(self) -> dict:
        """Returns the common parameters used for a TF Asymmetry fit in double pulse mode."""
        offset = self.context.gui_context["DoublePulseTime"]
        first_pulse_weighting, _ = self._get_pulse_weightings(offset, 2.2)

        return {"PulseOffset": offset, "EnableDoublePulse": True, "FirstPulseWeight": first_pulse_weighting}

    def _construct_global_tie_appendage(self) -> str:
        """Constructs the string which details the global parameter ties within a simultaneous TF Asymmetry fit."""
        if len(self.fitting_context.global_parameters) != 0 and self.fitting_context.number_of_datasets > 1:
            global_tie_appendage = str(self.fitting_context.simultaneous_fit_function).split(";")[-1]
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
        return [str(TF_ASYMMETRY_PREFIX_FUNCTION_INDEX + global_param) for global_param in self.fitting_context.global_parameters]

    def _get_unnormalised_workspace_list(self, normalised_workspaces: list) -> list:
        """Returns a list of unnormalised workspaces to be used within a TF Asymmetry fit."""
        return self.context.group_pair_context.get_unormalisised_workspace_list(normalised_workspaces)

    def _add_fit_to_context(
        self,
        input_workspace_names: list,
        output_workspaces: list,
        parameter_workspace: StaticWorkspaceWrapper,
        covariance_workspace: StaticWorkspaceWrapper,
        global_parameters: list = None,
    ) -> None:
        """Adds the results of a single/simultaneous tf asymmetry fit to the context."""
        self.context.fitting_context.add_fit_from_values(
            input_workspace_names,
            self.fitting_context.function_name,
            output_workspaces,
            parameter_workspace,
            covariance_workspace,
            global_parameters,
            self.fitting_context.tf_asymmetry_mode,
        )

    """
    Methods used by the Sequential Fitting Tab
    """

    def validate_sequential_fit(self, workspace_names: list) -> str:
        """Validates that the provided data is valid. It returns a message if the data is invalid."""
        message = super().validate_sequential_fit(workspace_names)
        if message != "":
            return message
        else:
            return self._check_tf_asymmetry_compliance(self._flatten_workspace_names(workspace_names))

    def _check_tf_asymmetry_compliance(self, workspace_names: list) -> str:
        """Checks that the workspace names provided are TF Asymmetry compliant."""
        tf_compliant, non_compliant_names = self.check_datasets_are_tf_asymmetry_compliant(workspace_names)
        if self.fitting_context.tf_asymmetry_mode and not tf_compliant:
            return (
                f"Only Groups can be fitted in TF Asymmetry mode. Please unselect the following Pairs/Diffs in "
                f"the grouping tab: {non_compliant_names}"
            )
        else:
            return ""

    def get_all_fit_function_parameter_values_for(self, fit_function: IFunction) -> list:
        """Returns the values of the fit function parameters. Also returns the normalisation for TF Asymmetry mode."""
        if self.fitting_context.tf_asymmetry_mode:
            if self.fitting_context.simultaneous_fitting_mode:
                return self._get_all_fit_function_parameter_values_for_tf_simultaneous_function(fit_function)
            else:
                return self._get_all_fit_function_parameter_values_for_tf_single_function(fit_function)
        else:
            return super().get_all_fit_function_parameter_values_for(fit_function)

    def _get_all_fit_function_parameter_values_for_tf_single_function(self, tf_single_function: IFunction) -> list:
        """Returns the required parameters values including normalisation from a TF asymmetry single function."""
        normal_single_function = self._get_normal_fit_function_from(tf_single_function)
        parameter_values, _ = self.get_fit_function_parameter_values(normal_single_function)
        return [self._get_normalisation_from_tf_fit_function(tf_single_function)] + parameter_values

    def _get_all_fit_function_parameter_values_for_tf_simultaneous_function(self, tf_simultaneous_function: IFunction) -> list:
        """Returns the required parameters values including normalisation from a TF asymmetry simultaneous function."""
        all_parameters = []
        for domain_index in range(self.fitting_context.number_of_datasets):
            all_parameters += [self._get_normalisation_from_tf_fit_function(tf_simultaneous_function, domain_index)]

            tf_domain_function = self.get_domain_tf_asymmetry_fit_function(tf_simultaneous_function, domain_index)
            normal_domain_function = self._get_normal_fit_function_from(tf_domain_function)
            parameter_values, _ = self.get_fit_function_parameter_values(normal_domain_function)
            all_parameters += parameter_values
        return all_parameters

    def get_all_fit_functions(self) -> list:
        """Returns all the fit functions for the current fitting mode."""
        if self.fitting_context.tf_asymmetry_mode:
            if self.fitting_context.simultaneous_fitting_mode:
                return [self.fitting_context.tf_asymmetry_simultaneous_function]
            else:
                return self.fitting_context.tf_asymmetry_single_functions
        else:
            return super().get_all_fit_functions()

    def get_all_fit_functions_for(self, display_type: str) -> list:
        """Returns all the fit functions for datasets with a name containing a string."""
        if self.fitting_context.tf_asymmetry_mode:
            return self.get_all_fit_functions()
        else:
            return super().get_all_fit_functions_for(display_type)

    def _parse_parameter_values(self, all_parameter_values: list):
        """Separate the parameter values into the normalisations and ordinary parameter values."""
        if not self.fitting_context.tf_asymmetry_mode:
            return all_parameter_values, []

        if not self.fitting_context.simultaneous_fitting_mode:
            return all_parameter_values[1:], all_parameter_values[:1]

        return self._parse_parameter_values_for_tf_asymmetry_simultaneous_mode(all_parameter_values)

    def _parse_parameter_values_for_tf_asymmetry_simultaneous_mode(self, all_parameter_values: list):
        """Separate the parameter values into the normalisations and ordinary parameter values."""
        n_params_per_domain = int(len(all_parameter_values) / self.fitting_context.number_of_datasets)

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

        super().update_ws_fit_function_parameters(dataset_names, parameter_values)

        if self.fitting_context.tf_asymmetry_mode:
            self._update_tf_fit_function_normalisation(dataset_names, normalisations)

    def _update_tf_fit_function_normalisation(self, dataset_names: list, normalisations: list) -> None:
        """Updates the normalisation values for the tf asymmetry functions."""
        if self.fitting_context.simultaneous_fitting_mode:
            for name, normalisation in zip(dataset_names, normalisations):
                self._set_normalisation_for_dataset(name, normalisation)
        else:
            self._set_normalisation_for_dataset(dataset_names[0], normalisations[0])

    def _update_fit_function_parameters_for_single_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in single fit mode."""
        super()._update_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

        if self.fitting_context.tf_asymmetry_mode:
            self._update_tf_fit_function_parameters_for_single_fit(dataset_names, parameter_values)

    def _update_tf_fit_function_parameters_for_single_fit(self, names: list, parameter_values: list):
        """Updates the tf asymmetry function parameters for the given dataset names if in single fit mode."""
        dataset_names = self.fitting_context.dataset_names
        for name in names:
            if name in dataset_names:
                tf_single_function = self.get_tf_asymmetry_single_function(dataset_names.index(name))
                self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_single_function), parameter_values)

    def _update_fit_function_parameters_for_simultaneous_fit(self, dataset_names: list, parameter_values: list) -> None:
        """Updates the function parameters for the given dataset names if in simultaneous fit mode."""
        super()._update_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)

        if self.fitting_context.tf_asymmetry_mode:
            self._update_tf_fit_function_parameters_for_simultaneous_fit(dataset_names, parameter_values)

    def _update_tf_fit_function_parameters_for_simultaneous_fit(self, names: list, parameter_values: list):
        """Updates the tf asymmetry function parameters for the given dataset names if in simultaneous fit mode."""
        dataset_names = self.fitting_context.dataset_names
        number_parameters_per_domain = int(len(parameter_values) / len(names))

        for name in names:
            if name in dataset_names:
                self._set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(
                    self.tf_asymmetry_simultaneous_function, dataset_names.index(name), parameter_values, number_parameters_per_domain
                )

    def perform_sequential_fit(self, workspaces: list, parameter_values: list, use_initial_values: bool = False) -> tuple:
        """Performs a sequential fit of the workspace names provided for the current fitting mode.

        :param workspaces: A list of lists of workspace names e.g. [[Row 1 workspaces], [Row 2 workspaces], etc...]
        :param parameter_values: A list of lists of parameter values e.g. [[Row 1 params], [Row 2 params], etc...]
        :param use_initial_values: If false the parameters at the end of each fit are passed on to the next fit.
        """
        if self.fitting_context.tf_asymmetry_mode:
            fitting_func = self._get_sequential_fitting_func_for_tf_asymmetry_fitting_mode()
        else:
            fitting_func = self._get_sequential_fitting_func_for_normal_fitting_mode()

        if not self.fitting_context.simultaneous_fitting_mode:
            workspaces = self._flatten_workspace_names(workspaces)

        return self._perform_sequential_fit_using_func(fitting_func, workspaces, parameter_values, use_initial_values)

    def _get_sequential_fitting_func_for_tf_asymmetry_fitting_mode(self):
        """Returns the fitting func to use when performing a fit in TF Asymmetry fitting mode."""
        if self.fitting_context.simultaneous_fitting_mode:
            return self._do_sequential_tf_asymmetry_simultaneous_fits
        else:
            return self._do_sequential_tf_asymmetry_fit

    def _do_sequential_tf_asymmetry_fit(
        self, row_index: int, workspace_name: str, parameter_values: list, functions: list, use_initial_values: bool = False
    ):
        """Performs a sequential fit of the TF Asymmetry single fit data."""
        tf_single_function = (
            functions[row_index - 1].clone()
            if not use_initial_values and row_index >= 1
            else self._get_tf_asymmetry_single_function_with_parameters(parameter_values)
        )

        params = self._get_parameters_for_tf_asymmetry_single_fit(workspace_name, tf_single_function)

        return self._do_tf_asymmetry_single_fit(params)

    def _get_tf_asymmetry_single_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        parameter_values, normalisations = self._parse_parameter_values(parameter_values)

        tf_single_function = self.current_tf_asymmetry_single_function.clone()
        self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_single_function), parameter_values)

        tf_single_function.setParameter(NORMALISATION_PARAMETER, normalisations[0])
        return tf_single_function

    def _do_sequential_tf_asymmetry_simultaneous_fits(
        self, row_index: int, workspace_names: list, parameter_values: list, functions: list, use_initial_values: bool = False
    ):
        """Performs a number of TF Asymmetry simultaneous fits, sequentially."""
        tf_simultaneous_function = (
            functions[row_index - 1].clone()
            if not use_initial_values and row_index >= 1
            else self._get_tf_asymmetry_simultaneous_function_with_parameters(parameter_values)
        )

        params = self._get_parameters_for_tf_asymmetry_simultaneous_fit(workspace_names, tf_simultaneous_function)

        return self._do_tf_asymmetry_simultaneous_fit(params, self._get_global_parameters_for_tf_asymmetry_fit())

    def _get_tf_asymmetry_simultaneous_function_with_parameters(self, parameter_values: list) -> IFunction:
        """Returns the current single fit function but with the parameter values provided."""
        parameter_values, normalisations = self._parse_parameter_values(parameter_values)
        number_parameters_per_domain = int(len(parameter_values) / self.fitting_context.number_of_datasets)

        tf_simultaneous_function = self.tf_asymmetry_simultaneous_function.clone()
        for dataset_index in range(self.fitting_context.number_of_datasets):
            self._set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(
                tf_simultaneous_function, dataset_index, parameter_values, number_parameters_per_domain
            )
            self._set_normalisation_in_tf_asymmetry_simultaneous_function(
                tf_simultaneous_function, dataset_index, normalisations[dataset_index]
            )

        return tf_simultaneous_function

    def _set_parameter_values_in_tf_asymmetry_simultaneous_function_domain(
        self, tf_simultaneous_function: IFunction, dataset_index: int, parameter_values: list, number_parameters_per_domain: int
    ) -> None:
        """Sets the parameter values within a domain of a TF Asymmetry simultaneous function."""
        parameter_values_for_domain = parameter_values[
            dataset_index * number_parameters_per_domain : (dataset_index + 1) * number_parameters_per_domain
        ]

        tf_domain_function = self.get_domain_tf_asymmetry_fit_function(tf_simultaneous_function, dataset_index)
        self._set_fit_function_parameter_values(self._get_normal_fit_function_from(tf_domain_function), parameter_values_for_domain)

    def _update_fit_functions_after_sequential_fit(self, workspaces: list, functions: list) -> None:
        """Updates the fit functions after a sequential fit has been run on the Sequential fitting tab."""
        if self.fitting_context.tf_asymmetry_mode:
            if self.fitting_context.simultaneous_fitting_mode:
                self._update_tf_asymmetry_simultaneous_fit_function_after_sequential(workspaces, functions)
            else:
                self._update_tf_asymmetry_single_fit_functions_after_sequential(workspaces, functions)
        else:
            super()._update_fit_functions_after_sequential_fit(workspaces, functions)

    def _update_tf_asymmetry_single_fit_functions_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the TF single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        dataset_names = self.fitting_context.dataset_names

        for workspace_index, workspace_name in enumerate(workspaces):
            if workspace_name in dataset_names:
                dataset_index = dataset_names.index(workspace_name)
                self.update_tf_asymmetry_single_fit_function(dataset_index, functions[workspace_index])

    def _update_tf_asymmetry_simultaneous_fit_function_after_sequential(self, workspaces: list, functions: list) -> None:
        """Updates the single fit functions after a sequential fit has been run on the Sequential fitting tab."""
        for fit_index, workspace_names in enumerate(workspaces):
            if self._are_same_workspaces_as_the_datasets(workspace_names):
                self.update_tf_asymmetry_simultaneous_fit_function(functions[fit_index])
                break
