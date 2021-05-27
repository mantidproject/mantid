# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist
from Muon.GUI.Common.contexts.fitting_context import FittingContext


class BasicFittingContext(FittingContext):

    def __init__(self, fit_list=None):
        super(BasicFittingContext, self).__init__(fit_list)

        self._plot_guess: bool = False
        self._guess_workspace_name: str = ""

        self._fit_to_raw: bool = True

    @property
    def plot_guess(self) -> bool:
        """Returns true if plot guess is turned on."""
        return self._plot_guess

    @plot_guess.setter
    def plot_guess(self, plot_guess: bool) -> None:
        """Sets that the plot guess should or should not be plotted."""
        self._plot_guess = plot_guess

    @property
    def guess_workspace_name(self) -> str:
        """Returns the name of the currently selected guess workspace."""
        return self._guess_workspace_name

    @guess_workspace_name.setter
    def guess_workspace_name(self, workspace_name: str) -> None:
        """Set the name of the currently selected guess workspace."""
        self._guess_workspace_name = workspace_name if check_if_workspace_exist(workspace_name) else ""

    @property
    def fit_to_raw(self) -> bool:
        """Returns true if fit to raw is turned on."""
        return self._fit_to_raw

    @fit_to_raw.setter
    def fit_to_raw(self, fit_to_raw: bool) -> None:
        """Sets the fit to raw property."""
        self._fit_to_raw = fit_to_raw
