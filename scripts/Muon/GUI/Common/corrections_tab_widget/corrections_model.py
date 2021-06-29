# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.corrections_context import CorrectionsContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string


class CorrectionsModel:
    """
    The CorrectionsModel calculates Dead Time corrections and Background corrections.
    """

    def __init__(self, data_context: MuonDataContext, corrections_context: CorrectionsContext):
        """Initialize the CorrectionsModel with empty data."""
        self._data_context = data_context
        self._corrections_context = corrections_context

    @property
    def number_of_run_strings(self) -> int:
        """Returns the number of runs currently loaded into the context."""
        return len(self._data_context.current_runs)

    def run_number_strings(self) -> list:
        """Returns a list of run number strings from the context. Its a string because of co-add mode."""
        return [run_list_to_string(run_list) for run_list in self._data_context.current_runs]

    def current_runs(self) -> list:
        """Returns the currently selected run number in a list. If co-add is selected, there are multiple runs."""
        for run_list in self._data_context.current_runs:
            if run_list_to_string(run_list) == self._corrections_context.current_run_string:
                return run_list
        return None

    def set_current_run_string(self, run_string: str) -> None:
        """Sets the currently selected run string shown in the view."""
        self._corrections_context.current_run_string = run_string if run_string != "" else None

    def current_run_string(self) -> str:
        """Returns the currently selected run string from the context."""
        return self._corrections_context.current_run_string
