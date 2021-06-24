# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction
from Muon.GUI.Common.contexts.fitting_contexts.basic_fitting_context import SINGLE_FITS_KEY
from Muon.GUI.Common.contexts.fitting_contexts.general_fitting_context import (GeneralFittingContext,
                                                                               SIMULTANEOUS_FITS_KEY)

TF_SINGLE_FITS_KEY = "TFSingleFits"
TF_SIMULTANEOUS_FITS_KEY = "TFSimultaneousFits"


class TFAsymmetryFittingContext(GeneralFittingContext):

    def __init__(self, allow_double_pulse_fitting: bool = False):
        super(TFAsymmetryFittingContext, self).__init__(allow_double_pulse_fitting)

        # A list of FitInformation's detailing all the TF Asymmetry fits that have happened including the fits that have
        # been overridden by an updated fit. The last TF Asymmetry fit performed is at the end of the list corresponding
        # to its key, and undoing will remove it.
        self._fit_history[TF_SINGLE_FITS_KEY] = []
        self._fit_history[TF_SIMULTANEOUS_FITS_KEY] = []

        self._tf_asymmetry_mode: bool = False

        self._tf_asymmetry_single_functions: list = []
        self._tf_asymmetry_simultaneous_function: IFunction = None

        self._normalisations_for_undo: list = []
        self._normalisations_fixed_for_undo: list = []

    def all_latest_fits(self):
        """Returns the latest unique fits for all fitting modes."""
        latest_tf_single_fits = self._latest_unique_fits_in(self._fit_history[TF_SINGLE_FITS_KEY])
        latest_tf_simultaneous_fits = self._latest_unique_fits_in(self._fit_history[TF_SIMULTANEOUS_FITS_KEY])
        return super().all_latest_fits() + latest_tf_single_fits + latest_tf_simultaneous_fits

    @property
    def active_fit_history(self):
        """Returns the fit history for the currently active fitting mode."""
        if self.tf_asymmetry_mode:
            return self._fit_history[TF_SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else TF_SINGLE_FITS_KEY]
        else:
            return self._fit_history[SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else SINGLE_FITS_KEY]

    @active_fit_history.setter
    def active_fit_history(self, fit_history: list) -> None:
        """Sets the fit history for the currently active fitting mode."""
        if self.tf_asymmetry_mode:
            self._fit_history[TF_SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else TF_SINGLE_FITS_KEY] = fit_history
        else:
            self._fit_history[SIMULTANEOUS_FITS_KEY if self.simultaneous_fitting_mode else SINGLE_FITS_KEY] = fit_history

    def clear(self, removed_fits: list = []):
        """Removes all the stored Fits from the context when an ADS clear event happens."""
        if len(removed_fits) == 0:
            removed_fits = self.all_latest_fits()

        self._fit_history[TF_SINGLE_FITS_KEY] = []
        self._fit_history[TF_SIMULTANEOUS_FITS_KEY] = []

        self._normalisations_for_undo = []
        self._normalisations_fixed_for_undo = []

        super().clear(removed_fits)

    def remove_workspace_by_name(self, workspace_name: str) -> None:
        """Remove a Fit from the history when an ADS delete event happens on one of its output workspaces."""
        self.remove_fit_by_name(self._fit_history[TF_SINGLE_FITS_KEY], workspace_name)
        self.remove_fit_by_name(self._fit_history[TF_SIMULTANEOUS_FITS_KEY], workspace_name)
        super().remove_workspace_by_name(workspace_name)

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
