# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import GeneralFittingContext


class TFAsymmetryFittingContext(GeneralFittingContext):

    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(TFAsymmetryFittingContext, self).__init__(allow_double_pulse_fitting)

        self._tf_asymmetry_mode: bool = False

        self._tf_asymmetry_single_functions: list = []
        self._tf_asymmetry_simultaneous_function: IFunction = None

        self._normalisations_for_undo: list = []
        self._normalisations_fixed_for_undo: list = []

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
    def tf_asymmetry_simultaneous_function(self) -> IFunction:
        """Returns the simultaneous TF Asymmetry fit function stored in the model."""
        return self._tf_asymmetry_simultaneous_function

    @tf_asymmetry_simultaneous_function.setter
    def tf_asymmetry_simultaneous_function(self, tf_asymmetry_simultaneous_function: IFunction) -> None:
        """Sets the simultaneous TF Asymmetry fit function stored in the model."""
        self._tf_asymmetry_simultaneous_function = tf_asymmetry_simultaneous_function

    @property
    def normalisations_for_undo(self) -> list:
        """Returns the previous normalisations that can be used when undoing."""
        return self._normalisations_for_undo

    @normalisations_for_undo.setter
    def normalisations_for_undo(self, normalisations: list) -> None:
        """Sets the previous normalisations that can be used when undoing."""
        self._normalisations_for_undo = normalisations

    @property
    def normalisations_fixed_for_undo(self) -> list:
        """Returns the previous booleans whether the normalisations were fixed that can be used when undoing."""
        return self._normalisations_fixed_for_undo

    @normalisations_fixed_for_undo.setter
    def normalisations_fixed_for_undo(self, normalisations_fixed: list) -> None:
        """Sets the previous booleans for if the normalisations are fixed that can be used when undoing."""
        self._normalisations_fixed_for_undo = normalisations_fixed
