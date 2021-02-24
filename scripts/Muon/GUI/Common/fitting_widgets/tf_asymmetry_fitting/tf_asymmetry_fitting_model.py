# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import ConvertFitFunctionForMuonTFAsymmetry

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_model import GeneralFittingModel

from mantid import logger

DEFAULT_NORMALISATION = 0.0
NORMALISATION_FUNCTION_INDEX = "f0.f0.A0"
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
    def dataset_names(self, names):
        GeneralFittingModel.dataset_names.fset(self, names)
        self.recalculate_tf_asymmetry_functions()

    @property
    def tf_asymmetry_mode(self):
        return self._tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on):
        self._tf_asymmetry_mode = tf_asymmetry_on

    @property
    def tf_asymmetry_single_functions(self):
        return self._tf_asymmetry_single_functions

    @tf_asymmetry_single_functions.setter
    def tf_asymmetry_single_functions(self, tf_asymmetry_functions):
        self._tf_asymmetry_single_functions = tf_asymmetry_functions

    @property
    def tf_asymmetry_simultaneous_function(self):
        return self._tf_asymmetry_simultaneous_function

    @tf_asymmetry_simultaneous_function.setter
    def tf_asymmetry_simultaneous_function(self, tf_asymmetry_simultaneous_function):
        self._tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

    def reset_tf_asymmetry_functions(self):
        self.tf_asymmetry_single_functions = [None] * self.number_of_datasets
        self.tf_asymmetry_simultaneous_function = None

    def recalculate_tf_asymmetry_functions(self):
        if self.tf_asymmetry_mode:
            try:
                self._recalculate_tf_asymmetry_functions()
            except RuntimeError:
                self.reset_tf_asymmetry_functions()
                return False
        else:
            self.reset_tf_asymmetry_functions()
        return True

    def set_current_normalisation(self, value):
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                self._set_current_normalisation_in_tf_asymmetry_simultaneous_function(value)
            else:
                self._set_current_normalisation_in_tf_asymmetry_single_fit_function(value)

    def _set_current_normalisation_in_tf_asymmetry_single_fit_function(self, value):
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            current_tf_single_fit_function.setParameter(NORMALISATION_FUNCTION_INDEX, value)

    def _set_current_normalisation_in_tf_asymmetry_simultaneous_function(self, value):
        if self.tf_asymmetry_simultaneous_function is not None:
            self.tf_asymmetry_simultaneous_function.setParameter(
                f"f{self.current_dataset_index}.{NORMALISATION_FUNCTION_INDEX}", value)

    def current_normalisation(self):
        if self.current_dataset_index is not None:
            if self.simultaneous_fitting_mode:
                return self._current_normalisation_from_tf_asymmetry_simultaneous_function()
            else:
                return self._current_normalisation_from_tf_asymmetry_single_fit_function()
        else:
            return DEFAULT_NORMALISATION

    def automatically_update_function_name(self) -> None:
        """Attempt to update the function name automatically."""
        if self.function_name_auto_update:
            super().automatically_update_function_name()
            if self.tf_asymmetry_mode:
                self.function_name += TF_ASYMMETRY_FUNCTION_NAME_APPENDAGE

    def check_datasets_are_tf_asymmetry_compliant(self):
        non_compliant_workspaces = [item for item in self.dataset_names if "Group" not in item]
        return False if len(non_compliant_workspaces) > 0 else True

    # def perform_fit_or_plot_guess(self):
    #     if self._tf_asymmetry_mode:
    #         new_global_parameters = [str("f0.f1.f1." + item) for item in self.global_parameters]
    #     else:
    #         new_global_parameters = [item[9:] for item in self.global_parameters]
    #
    #     if not self.view.is_simul_fit:
    #         for index, fit_function in enumerate(self.single_fit_functions):
    #             fit_function = fit_function if fit_function else self.view.fit_object.clone()
    #             new_function = self._calculate_tf_asymmetry_fit_function(fit_function)
    #             self._fit_function[index] = new_function.clone()
    #
    #         func_str = str(self._fit_function[self.view.get_index_for_start_end_times()])
    #     else:
    #         new_function = self._calculate_tf_asymmetry_fit_function(self.simultaneous_fit_function)
    #         self._fit_function = [new_function.clone()]
    #         func_str = str(self._fit_function[0])

    def _recalculate_tf_asymmetry_functions(self):
        if self.simultaneous_fitting_mode:
            self._recalculate_tf_asymmetry_simultaneous_fit_function()
        else:
            self._recalculate_tf_asymmetry_single_fit_functions()

    def _recalculate_tf_asymmetry_single_fit_functions(self):
        self.tf_asymmetry_single_functions = [self._convert_to_tf_asymmetry_function(single_function,
                                                                                     [self.dataset_names[index]])
                                              for index, single_function in enumerate(self.single_fit_functions)]

    def _recalculate_tf_asymmetry_simultaneous_fit_function(self):
        self.tf_asymmetry_simultaneous_function = self._convert_to_tf_asymmetry_function(self.simultaneous_fit_function,
                                                                                         self.dataset_names)

    def _convert_to_tf_asymmetry_function(self, fit_function, workspace_names):
        if fit_function is None:
            return None

        parameters = self._get_parameters_for_tf_asymmetry_conversion(fit_function, workspace_names)
        return ConvertFitFunctionForMuonTFAsymmetry(StoreInADS=False, **parameters)

    def _get_parameters_for_tf_asymmetry_conversion(self, fit_function, workspace_names):
        return {"InputFunction": fit_function,
                "WorkspaceList": workspace_names,
                "Mode": "Construct" if self.tf_asymmetry_mode else "Extract",
                "CopyTies": False}

    def _current_normalisation_from_tf_asymmetry_single_fit_function(self):
        current_tf_single_fit_function = self.tf_asymmetry_single_functions[self.current_dataset_index]
        if current_tf_single_fit_function is not None:
            return current_tf_single_fit_function.getParameter(NORMALISATION_FUNCTION_INDEX)
        else:
            return DEFAULT_NORMALISATION

    def _current_normalisation_from_tf_asymmetry_simultaneous_function(self):
        if self.tf_asymmetry_simultaneous_function is not None:
            if self.number_of_datasets > 1:
                return self.tf_asymmetry_simultaneous_function.getParameter(
                    f"f{self.current_dataset_index}.{NORMALISATION_FUNCTION_INDEX}")
            else:
                return self.tf_asymmetry_simultaneous_function.getParameter(NORMALISATION_FUNCTION_INDEX)
        else:
            return DEFAULT_NORMALISATION

    # def update_plot_guess(self):
    #     pass
