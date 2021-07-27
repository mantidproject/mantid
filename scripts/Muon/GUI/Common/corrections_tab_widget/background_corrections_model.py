# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass

from mantid.api import AlgorithmManager, FunctionFactory, IFunction, Workspace
from mantid.simpleapi import CreateWorkspace
from Muon.GUI.Common.ADSHandler.ADS_calls import add_ws_to_ads, retrieve_ws
from Muon.GUI.Common.contexts.corrections_context import (BACKGROUND_MODE_NONE, FLAT_BACKGROUND,
                                                          FLAT_BACKGROUND_AND_EXP_DECAY, RUNS_ALL, GROUPS_ALL)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.utilities.algorithm_utils import run_Fit, run_minus
from Muon.GUI.Common.utilities.run_string_utils import run_string_to_list
from Muon.GUI.Common.utilities.workspace_data_utils import x_limits_of_workspace
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

BACKGROUND_PARAM = "A0"
DEFAULT_A_VALUE = 1e6
MAX_ACCEPTABLE_CHI_SQUARED = 10.0


@dataclass
class BackgroundCorrectionData:
    """
    The background correction data associated with a specific group of a run.
    """
    uncorrected_counts_workspace: StaticWorkspaceWrapper
    start_x: float
    end_x: float
    flat_background: IFunction
    exp_decay: IFunction
    status: str = "No background correction"

    def __init__(self, start_x: float, end_x: float):
        self.start_x = start_x
        self.end_x = end_x
        self.setup_functions()

    def setup_functions(self):
        self.flat_background = FunctionFactory.createFunction("FlatBackground")
        self.exp_decay = FunctionFactory.createFunction("ExpDecayMuon")
        self.exp_decay.setParameter("A", DEFAULT_A_VALUE)

    def create_background_workspace(self) -> Workspace:
        background = self.flat_background.getParameterValue(BACKGROUND_PARAM)
        background_error = self.flat_background.getError(BACKGROUND_PARAM)
        background_workspace = CreateWorkspace(DataX=[0.0], DataY=[background], DataE=[background_error],
                                               StoreInADS=False)
        return background_workspace


class BackgroundCorrectionsModel:
    """
    The BackgroundCorrectionsModel calculates Background corrections.
    """

    def __init__(self, corrections_model: CorrectionsModel, context: MuonContext):
        """Initialize the BackgroundCorrectionsModel with empty data."""
        self._corrections_model = corrections_model
        self._context = context
        self._corrections_context = context.corrections_context

    def set_background_correction_mode(self, mode: str) -> None:
        """Sets the current background correction mode in the context."""
        self._corrections_context.background_corrections_mode = mode

    def is_background_mode_none(self) -> bool:
        """Returns true if the current background correction mode is none."""
        return self._corrections_context.background_corrections_mode == BACKGROUND_MODE_NONE

    def set_selected_function(self, selected_function: str) -> None:
        """Sets the currently selected function which is displayed in the function combo box."""
        self._corrections_context.selected_function = selected_function

    def set_selected_group(self, group: str) -> None:
        """Sets the currently selected Group in the context."""
        self._corrections_context.selected_group = group

    def set_show_all_runs(self, show_all: bool) -> None:
        """Sets whether all runs should be shown or not."""
        self._corrections_context.show_all_runs = show_all

    def all_runs(self) -> list:
        """Returns a list of all loaded runs."""
        return self._context.get_runs(RUNS_ALL)

    def group_names(self) -> list:
        """Returns the group names found in the group/pair context."""
        return self._context.group_pair_context.group_names

    def set_start_x(self, run: str, group: str, start_x: float) -> None:
        """Sets the Start X associated with the provided Run and Group."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            self._corrections_context.background_correction_data[run_group].start_x = start_x

    def start_x(self, run: str, group: str) -> float:
        """Returns the Start X associated with the provided Run and Group."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            return self._corrections_context.background_correction_data[run_group].start_x

        raise RuntimeError(f"The provided run and group could not be found ({run}, {group}).")

    def set_end_x(self, run: str, group: str, end_x: float) -> None:
        """Sets the End X associated with the provided Run and Group."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            self._corrections_context.background_correction_data[run_group].end_x = end_x

    def end_x(self, run: str, group: str) -> float:
        """Returns the End X associated with the provided Run and Group."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            return self._corrections_context.background_correction_data[run_group].end_x

        raise RuntimeError(f"The provided run and group could not be found ({run}, {group}).")

    def all_runs_and_groups(self) -> tuple:
        """Returns all the runs and groups stored in the context. The list indices of the runs and groups correspond."""
        runs, groups = [], []
        for run_group in self._corrections_context.background_correction_data.keys():
            runs.append(run_group[0])
            groups.append(run_group[1])
        return runs, groups

    def clear_background_corrections_data(self) -> None:
        """Clears the background correction data dictionary."""
        self._corrections_context.background_correction_data = {}

    def populate_background_corrections_data(self) -> None:
        """Populates the background correction data dictionary when runs are initially loaded into the interface."""
        groups = self.group_names()
        for run in self._corrections_model.run_number_strings():
            for group in groups:
                run_group = tuple([run, group])
                workspace_name = self.get_counts_workspace_name(run, group)
                if run_group not in self._corrections_context.background_correction_data:
                    start_x, end_x = self.default_x_range(run, group)
                    self._corrections_context.background_correction_data[run_group] = BackgroundCorrectionData(start_x,
                                                                                                               end_x)
                self._corrections_context.background_correction_data[run_group].uncorrected_counts_workspace = \
                    StaticWorkspaceWrapper(workspace_name, retrieve_ws(workspace_name))

    def reset_background_subtraction_data(self) -> None:
        """Resets the calculated background functions and corrected counts data in the ADS."""
        for run_group in self._corrections_context.background_correction_data.keys():
            correction_data = self._corrections_context.background_correction_data[run_group]
            correction_data.setup_functions()
            add_ws_to_ads(correction_data.uncorrected_counts_workspace.workspace_name,
                          correction_data.uncorrected_counts_workspace.workspace_copy())
        return self.all_runs_and_groups()

    def run_background_correction_for_all(self) -> None:
        """Runs the background corrections for all stored domains."""
        for run_group in self._corrections_context.background_correction_data.keys():
            self._run_background_correction(self._corrections_context.background_correction_data[run_group])
        return self.all_runs_and_groups()

    def run_background_correction_for(self, run: str, group: str) -> None:
        """Calculates the background for some data using a Fit."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            self._run_background_correction(self._corrections_context.background_correction_data[run_group])
        return [run], [group]

    def _run_background_correction(self, correction_data: BackgroundCorrectionData) -> None:
        """Calculates the background for some data using a Fit."""
        params = self._get_parameters_for_background_fit(correction_data)
        function, fit_status, chi_squared = run_Fit(params, AlgorithmManager.create("Fit"))

        self._handle_background_fit_output(correction_data, function, fit_status, chi_squared)
        self._perform_background_subtraction_on_counts(correction_data)

    @staticmethod
    def _perform_background_subtraction_on_counts(correction_data: BackgroundCorrectionData) -> None:
        """Performs the background subtraction on the counts workspace, and then generates the asymmetry workspace."""
        run_minus(correction_data.uncorrected_counts_workspace.workspace_copy(),
                  correction_data.create_background_workspace(),
                  correction_data.uncorrected_counts_workspace.workspace_name)

    def calculate_asymmetry_workspaces_for(self, runs: list, groups: list) -> None:
        """Creates the asymmetry workspaces for the runs and groups provided using the Counts workspaces in the ADS."""
        for run, group in zip(runs, groups):
            self._create_asymmetry_workspace_for(run, group)

    def _create_asymmetry_workspace_for(self, run: str, group: str) -> None:
        """Creates the asymmetry workspace for a run and group using its corresponding Counts workspace in the ADS."""
        run_list = run_string_to_list(run)
        group_object = self._context.group_pair_context[group]
        if run_list is not None and group_object is not None:
            self._context.calculate_asymmetry_for(run_list, group_object)
            self._context.show_group(run_list, group_object)

    def calculate_pairs_for(self, runs: list, groups: list) -> list:
        """Calculates the Pair Asymmetry workspaces which are concerned with the provided Runs and Groups."""
        self._context.update_phasequads()

        # Remove duplicates from the list
        runs = list(dict.fromkeys(runs))

        pairs = self._context.find_pairs_containing_groups(groups)
        self._calculate_pairs_for(runs, pairs)
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
        # Remove duplicates from the list
        runs = list(dict.fromkeys(runs))

        self._calculate_diffs_for(runs, self._context.find_diffs_containing_groups_or_pairs(groups_and_pairs))

    def _calculate_diffs_for(self, runs: list, diffs: list) -> None:
        """Calculates the Diff Asymmetry workspaces for the provided runs and diffs."""
        for run in runs:
            run_list = run_string_to_list(run)
            for diff_object in diffs:
                self._context.calculate_diff_for(run_list, diff_object)
                self._context.show_diff(run_list, diff_object)

    def _handle_background_fit_output(self, correction_data: BackgroundCorrectionData, function: IFunction,
                                      fit_status: str, chi_squared: float) -> None:
        """Handles the output of the background fit."""
        if chi_squared > MAX_ACCEPTABLE_CHI_SQUARED:
            correction_data.setup_functions()
            correction_data.status = f"Correction skipped - chi squared is poor ({chi_squared:.3f})."
        elif "Failed to converge" in fit_status:
            correction_data.setup_functions()
            correction_data.status = f"Correction skipped - {fit_status}"
        else:
            if self._corrections_context.selected_function == FLAT_BACKGROUND:
                correction_data.flat_background = function.clone()
            elif self._corrections_context.selected_function == FLAT_BACKGROUND_AND_EXP_DECAY:
                correction_data.flat_background = function.getFunction(0).clone()
                correction_data.exp_decay = function.getFunction(1).clone()

            correction_data.status = "Correction success"

    def _get_parameters_for_background_fit(self, correction_data: BackgroundCorrectionData) -> dict:
        """Gets the parameters to use for the background Fit."""
        return {"Function": self._get_fit_function_for_background_fit(correction_data),
                "InputWorkspace": correction_data.uncorrected_counts_workspace.workspace_copy(),
                "StartX": correction_data.start_x,
                "EndX": correction_data.end_x,
                "CreateOutput": False,
                "CalcErrors": True}

    def _get_fit_function_for_background_fit(self, correction_data: BackgroundCorrectionData) -> IFunction:
        """Returns the fit function to use for a background fit."""
        if self._corrections_context.selected_function == FLAT_BACKGROUND:
            return correction_data.flat_background
        elif self._corrections_context.selected_function == FLAT_BACKGROUND_AND_EXP_DECAY:
            composite = FunctionFactory.createFunction("CompositeFunction")
            composite.add(correction_data.flat_background)
            composite.add(correction_data.exp_decay)
            return composite

        raise RuntimeError("The selected background function is not recognised.")

    def selected_correction_data(self) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs, groups, start_xs, end_xs, backgrounds, background_errors, statuses = self._selected_correction_data_for(
            self._selected_runs(), self._selected_groups())
        return runs, groups, start_xs, end_xs, backgrounds, background_errors, statuses

    def _selected_correction_data_for(self, selected_runs: list, selected_groups: list) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs_list, groups_list, start_xs, end_xs, backgrounds, background_errors, statuses = [], [], [], [], [], [], []
        for run_group, correction_data in self._corrections_context.background_correction_data.items():
            if run_group[0] in selected_runs and run_group[1] in selected_groups:
                runs_list.append(run_group[0])
                groups_list.append(run_group[1])
                start_xs.append(correction_data.start_x)
                end_xs.append(correction_data.end_x)
                backgrounds.append(correction_data.flat_background.getParameterValue(BACKGROUND_PARAM))
                background_errors.append(correction_data.flat_background.getError(BACKGROUND_PARAM))
                statuses.append(correction_data.status)
        return runs_list, groups_list, start_xs, end_xs, backgrounds, background_errors, statuses

    def _selected_runs(self) -> list:
        """Returns a list containing the run number strings that are currently selected."""
        if self._corrections_context.show_all_runs:
            return self._corrections_model.run_number_strings()
        else:
            return [self._corrections_context.current_run_string]

    def _selected_groups(self) -> list:
        """Returns a list of selected group names."""
        selected_group = self._corrections_context.selected_group
        return self.group_names() if selected_group == GROUPS_ALL else [selected_group]

    def get_counts_workspace_name(self, run_string: str, group: str) -> str:
        """Returns the name of the counts workspace associated with the provided run string and group."""
        run_list = self._context.get_runs(run_string)
        workspace_list = []
        if len(run_list) > 0:
            workspace_list = self._context.group_pair_context.get_group_counts_workspace_names(run_list[0], [group])
        return workspace_list[0] if len(workspace_list) > 0 else None

    def default_x_range(self, run: str, group: str) -> tuple:
        """Returns the x range to use by default for the background corrections. It is the second half of the data."""
        x_lower, x_upper = self.x_limits_of_workspace(run, group)
        x_mid = round((x_upper - x_lower)/2.0)
        return x_mid, x_upper

    def x_limits_of_workspace(self, run: str, group: str) -> tuple:
        """Returns the x data limits of the workspace associated with the provided Run and Group."""
        return x_limits_of_workspace(self.get_counts_workspace_name(run, group))
