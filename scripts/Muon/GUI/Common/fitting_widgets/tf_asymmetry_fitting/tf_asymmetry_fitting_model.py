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
        self._normalisations = []

    @GeneralFittingModel.dataset_names.setter
    def dataset_names(self, names):
        GeneralFittingModel.dataset_names.fset(self, names)
        self.recalculate_normalisations()

    @property
    def tf_asymmetry_mode(self):
        return self._tf_asymmetry_mode

    @tf_asymmetry_mode.setter
    def tf_asymmetry_mode(self, tf_asymmetry_on):
        self._tf_asymmetry_mode = tf_asymmetry_on

    @property
    def normalisations(self):
        return self._normalisations

    @normalisations.setter
    def normalisations(self, normalisations):
        self._normalisations = normalisations

    @property
    def current_normalisation(self):
        if self.current_dataset_index is not None:
            return self.normalisations[self.current_dataset_index]
        else:
            return DEFAULT_NORMALISATION

    def reset_normalisations(self):
        self.normalisations = [DEFAULT_NORMALISATION] * self.number_of_datasets

    def recalculate_normalisations(self):
        if self.tf_asymmetry_mode:
            try:
                self._recalculate_normalisations()
            except RuntimeError:
                self.normalisations = [DEFAULT_NORMALISATION] * self.number_of_datasets
                return False
        else:
            self.normalisations = [DEFAULT_NORMALISATION] * self.number_of_datasets
        return True

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

    def _recalculate_normalisations(self):
        if self.simultaneous_fitting_mode:
            self._recalculate_normalisations_for_simultaneous_fit_domains()
        else:
            self._recalculate_normalisations_for_single_fit_functions()

    def _recalculate_normalisations_for_single_fit_functions(self):
        self.normalisations = [self._calculate_normalisation_for_single_domain(index, single_function)
                               for index, single_function in enumerate(self.single_fit_functions)]

    def _calculate_normalisation_for_single_domain(self, dataset_index, fit_function):
        if fit_function is not None:
            parameters = self._get_parameters_for_tf_function_calculation(fit_function)
            parameters["WorkspaceList"] = [self.dataset_names[dataset_index]]
            tf_asymmetry_function = self._convert_to_tf_asymmetry_function(parameters)
            return tf_asymmetry_function.getParameter(NORMALISATION_FUNCTION_INDEX)
        else:
            return 0.0

    def _recalculate_normalisations_for_simultaneous_fit_domains(self):
        if self.number_of_datasets > 1:
            self.normalisations = self._calculate_normalisation_for_simultaneous_fit_function()
        else:
            self.normalisations = [self._calculate_normalisation_for_single_domain(self.simultaneous_fit_function)]

    def _calculate_normalisation_for_simultaneous_fit_function(self):
        if self.simultaneous_fit_function is not None:
            parameters = self._get_parameters_for_tf_function_calculation(self.simultaneous_fit_function)
            new_function = self._convert_to_tf_asymmetry_function(parameters)

            return [new_function.getParameter(f"f{domain_index}.{NORMALISATION_FUNCTION_INDEX}")
                    for domain_index in range(self.number_of_datasets)]
        else:
            return [DEFAULT_NORMALISATION] * self.number_of_datasets

    def _convert_to_tf_asymmetry_function(self, parameters):
        return ConvertFitFunctionForMuonTFAsymmetry(StoreInADS=False, **parameters)

    def _get_parameters_for_tf_function_calculation(self, fit_function):
        return {"InputFunction": fit_function,
                "WorkspaceList": self.get_active_workspace_names(),
                "Mode": "Construct" if self.tf_asymmetry_mode else "Extract",
                "CopyTies": False}

    # def update_plot_guess(self):
    #     pass
