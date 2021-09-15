# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list


class CorrectionsModel:
    """
    The CorrectionsModel calculates Dead Time corrections and Background corrections.
    """

    def __init__(self, context: MuonContext):
        """Initialize the CorrectionsModel with empty data."""
        self._context = context
        self._data_context = context.data_context
        self._corrections_context = context.corrections_context

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

    """
    After the background corrections are performed on the Counts; the Group Asymmetry, Pair Asymmetry and Diff Asymmetry
    workspaces need to be recalculated. The following functions are used for this recalculation. These functions allow
    us to perform the recalculation of the asymmetries only for the necessary runs and groups that have just been
    count-corrected.
    """

    def calculate_asymmetry_workspaces_for(self, runs: list, groups: list) -> None:
        """Creates the asymmetry workspaces for the runs and groups provided using the Counts workspaces in the ADS."""
        for run, group in zip(runs, groups):
            self._create_asymmetry_workspace_for(run, group)

    def _create_asymmetry_workspace_for(self, run: str, group: str) -> None:
        """Creates the asymmetry workspace for a run and group using its corresponding Counts workspace in the ADS."""
        run_list = run_string_to_list(run)
        group_object = self._context.group_pair_context[group]
        if run_list is not None and group_object is not None:
            self._context.calculate_asymmetry_for(run_list, group_object, rebin=False)
            self._context.show_group(run_list, group_object, rebin=False)
            if self._context._do_rebin():
                self._context.calculate_asymmetry_for(run_list, group_object, rebin=True)
                self._context.show_group(run_list, group_object, rebin=True)

    def calculate_pairs_for(self, runs: list, groups: list) -> list:
        """Calculates the Pair Asymmetry workspaces which are concerned with the provided Runs and Groups."""
        self._context.update_phasequads()

        pairs = self._context.find_pairs_containing_groups(groups)
        self._calculate_pairs_for(self._remove_duplicates(runs), pairs)

        return [pair.name for pair in pairs]

    def _calculate_pairs_for(self, runs: list, pairs: list) -> None:
        """Calculates the Pair Asymmetry workspaces for the provided runs and pairs."""
        for run in runs:
            run_list = run_string_to_list(run)
            for pair_object in pairs:
                self._context.calculate_pair_for(run_list, pair_object)
                self._context.show_pair(run_list, pair_object)

    def calculate_diffs_for(self, runs: list, groups_and_pairs: list) -> None:
        """Calculates the Diff Asymmetry workspaces which are concerned with the provided Runs and Groups/Pairs."""
        diffs = self._context.find_diffs_containing_groups_or_pairs(groups_and_pairs)
        self._calculate_diffs_for(self._remove_duplicates(runs), diffs)

    def _calculate_diffs_for(self, runs: list, diffs: list) -> None:
        """Calculates the Diff Asymmetry workspaces for the provided runs and diffs."""
        for run in runs:
            run_list = run_string_to_list(run)
            for diff_object in diffs:
                self._context.calculate_diff_for(run_list, diff_object)
                self._context.show_diff(run_list, diff_object)

    @staticmethod
    def _remove_duplicates(list_with_duplicates: list) -> list:
        """Removes the duplicate entries in a list."""
        return list(dict.fromkeys(list_with_duplicates))
