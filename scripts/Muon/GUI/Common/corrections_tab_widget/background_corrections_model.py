# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass

from mantid.api import AlgorithmManager, CompositeFunction, FunctionFactory, IFunction, Workspace
from Muon.GUI.Common.contexts.corrections_context import (BACKGROUND_MODE_NONE, FLAT_BACKGROUND,
                                                          FLAT_BACKGROUND_AND_EXP_DECAY, RUNS_ALL, GROUPS_ALL)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.utilities.algorithm_utils import (run_Fit, run_clone_workspace, run_create_single_valued_workspace,
                                                       run_minus)
from Muon.GUI.Common.utilities.workspace_data_utils import x_limits_of_workspace

BACKGROUND_PARAM = "A0"
DEFAULT_A_VALUE = 1e6
MAX_ACCEPTABLE_CHI_SQUARED = 10.0
TEMPORARY_BACKGROUND_WORKSPACE_NAME = "__temp_background_workspace"


@dataclass
class BackgroundCorrectionData:
    """
    The background correction data associated with a specific group of a run.
    """
    uncorrected_workspace_name: str
    workspace_name: str
    start_x: float
    end_x: float
    flat_background: IFunction
    exp_decay: IFunction
    status: str = "No background correction"

    def __init__(self, start_x: float, end_x: float, flat_background: IFunction = None, exp_decay: IFunction = None):
        self.start_x = start_x
        self.end_x = end_x
        self.setup_functions(flat_background, exp_decay)

    def setup_functions(self, flat_background: IFunction = None, exp_decay: IFunction = None) -> None:
        """Sets up the functions to use by default when doing Auto corrections."""
        if flat_background is not None:
            self.flat_background = flat_background.clone()
        else:
            self.flat_background = FunctionFactory.createFunction("FlatBackground")

        if exp_decay is not None:
            self.exp_decay = exp_decay.clone()
        else:
            self.exp_decay = FunctionFactory.createFunction("ExpDecayMuon")
            self.exp_decay.setParameter("A", DEFAULT_A_VALUE)

    def setup_uncorrected_workspace(self, workspace_name: str) -> None:
        """Clones the Counts workspace into a hidden workspace in the ADS."""
        self.workspace_name = workspace_name
        self.uncorrected_workspace_name = f"__{workspace_name}_uncorrected"

        run_clone_workspace({"InputWorkspace": self.workspace_name, "OutputWorkspace": self.uncorrected_workspace_name})

    def reset(self) -> None:
        """Resets the background correction data by replacing the corrected data with the original uncorrected data."""
        self.setup_functions()
        run_clone_workspace({"InputWorkspace": self.uncorrected_workspace_name, "OutputWorkspace": self.workspace_name})

    def calculate_background(self, fit_function: IFunction) -> None:
        """Calculates the background in the counts workspace."""
        params = self._get_parameters_for_background_fit(fit_function)
        function, fit_status, chi_squared = run_Fit(params, AlgorithmManager.create("Fit"))

        self._handle_background_fit_output(function, fit_status, chi_squared)

    def _get_parameters_for_background_fit(self, fit_function: IFunction) -> dict:
        """Gets the parameters to use for the background Fit."""
        return {"Function": fit_function,
                "InputWorkspace": self.uncorrected_workspace_name,
                "StartX": self.start_x,
                "EndX": self.end_x,
                "CreateOutput": False,
                "CalcErrors": True}

    def _handle_background_fit_output(self, function: IFunction, fit_status: str, chi_squared: float) -> None:
        """Handles the output of the background fit."""
        if chi_squared > MAX_ACCEPTABLE_CHI_SQUARED:
            self.setup_functions()
            self.status = f"Correction skipped - chi squared is poor ({chi_squared:.3f})."
        elif "Failed to converge" in fit_status:
            self.setup_functions()
            self.status = f"Correction skipped - {fit_status}"
        else:
            if isinstance(function, CompositeFunction):
                self.flat_background = function.getFunction(0).clone()
                self.exp_decay = function.getFunction(1).clone()
            else:
                self.flat_background = function.clone()

            self.status = "Correction success"

    def perform_background_subtraction(self) -> None:
        """Performs the background subtraction on the counts workspace, and then generates the asymmetry workspace."""
        run_minus(self.uncorrected_workspace_name, self._create_background_workspace(), self.workspace_name)

    def _create_background_workspace(self) -> Workspace:
        """Creates the background workspace to use for the background subtraction."""
        return run_create_single_valued_workspace(self._get_parameters_for_creating_background_workspace())

    def _get_parameters_for_creating_background_workspace(self) -> dict:
        return {"DataValue": self.flat_background.getParameterValue(BACKGROUND_PARAM),
                "ErrorValue": self.flat_background.getError(BACKGROUND_PARAM),
                "OutputWorkspace": TEMPORARY_BACKGROUND_WORKSPACE_NAME}


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

    def set_show_rebin_data(self, show_rebin_data: bool) -> None:
        """Sets whether the rebin corrected data should be shown or not."""
        self._corrections_context.show_rebin_data = show_rebin_data

    def all_runs(self) -> list:
        """Returns a list of all loaded runs."""
        return self._context.get_runs(RUNS_ALL)

    def group_names(self) -> list:
        """Returns the group names found in the group/pair context."""
        return self._context.group_pair_context.group_names

    def set_start_x(self, run: str, group: str, rebin: bool, start_x: float) -> None:
        """Sets the Start X associated with the provided Run and Group."""
        run_group = tuple([run, group, rebin])
        if run_group in self._corrections_context.background_correction_data:
            self._corrections_context.background_correction_data[run_group].start_x = start_x

    def start_x(self, run: str, group: str, rebin: bool) -> float:
        """Returns the Start X associated with the provided Run and Group."""
        run_group = tuple([run, group, rebin])
        if run_group in self._corrections_context.background_correction_data:
            return self._corrections_context.background_correction_data[run_group].start_x

        raise RuntimeError(f"The provided run and group could not be found ({run}, {group}).")

    def set_end_x(self, run: str, group: str, rebin: bool, end_x: float) -> None:
        """Sets the End X associated with the provided Run and Group."""
        run_group = tuple([run, group, rebin])
        if run_group in self._corrections_context.background_correction_data:
            self._corrections_context.background_correction_data[run_group].end_x = end_x

    def end_x(self, run: str, group: str, rebin: bool) -> float:
        """Returns the End X associated with the provided Run and Group."""
        run_group = tuple([run, group, rebin])
        if run_group in self._corrections_context.background_correction_data:
            return self._corrections_context.background_correction_data[run_group].end_x

        raise RuntimeError(f"The provided run and group could not be found ({run}, {group}).")

    def all_runs_and_groups(self) -> tuple:
        """Returns all the runs and groups stored in the context. The list indices of the runs and groups correspond."""
        runs, groups, rebins = [], [], []
        for run_group in self._corrections_context.background_correction_data.keys():
            runs.append(run_group[0])
            groups.append(run_group[1])
            rebins.append(run_group[2])
        return runs, groups, rebins

    def clear_background_corrections_data(self) -> None:
        """Clears the background correction data dictionary."""
        self._corrections_context.background_correction_data = {}

    def populate_background_corrections_data(self) -> None:
        """Populates the background correction data dictionary when runs are initially loaded into the interface."""
        old_data = self._corrections_context.background_correction_data
        self.clear_background_corrections_data()

        self._populate_background_corrections_data_using_previous_data(old_data)

    def _populate_background_corrections_data_using_previous_data(self, previous_data: dict) -> None:
        """Populates the background correction data, whilst trying to reuse some of the previous data."""
        groups = self.group_names()
        for run in self._corrections_model.run_number_strings():
            for group in groups:
                self._add_background_correction_data_for(previous_data, run, group, rebin=False)
                if self._context._do_rebin():
                    self._add_background_correction_data_for(previous_data, run, group, rebin=True)

    def _add_background_correction_data_for(self, previous_data: dict, run: str, group: str, rebin: bool) -> None:
        """Add background correction data for the provided Run, Group and Rebin status."""
        run_group = tuple([run, group, rebin])
        background_data = self._create_background_correction_data(previous_data, run_group)

        workspace_name = self.get_counts_workspace_name(run, group, rebin)
        self._corrections_context.background_correction_data[run_group] = background_data
        self._corrections_context.background_correction_data[run_group].setup_uncorrected_workspace(workspace_name)

    def _create_background_correction_data(self, previous_data: dict, run_group: tuple) -> BackgroundCorrectionData:
        """Creates the BackgroundCorrectionData for a newly loaded data. It tries to reuse previous data."""
        run, group, rebin = run_group
        if run_group in previous_data:
            data = previous_data[run_group]
            return BackgroundCorrectionData(data.start_x, data.end_x, data.flat_background, data.exp_decay)
        else:
            for key, value in previous_data.items():
                if key[1] == group:
                    return BackgroundCorrectionData(value.start_x, value.end_x, value.flat_background, value.exp_decay)

        start_x, end_x = self.default_x_range(run, group, rebin)
        return BackgroundCorrectionData(start_x, end_x)

    def reset_background_subtraction_data(self) -> None:
        """Resets the calculated background functions and corrected counts data in the ADS."""
        for run_group in self._corrections_context.background_correction_data.keys():
            self._corrections_context.background_correction_data[run_group].reset()
        return self.all_runs_and_groups()

    def run_background_correction_for_all(self) -> None:
        """Runs the background corrections for all stored domains."""
        for run_group in self._corrections_context.background_correction_data.keys():
            self._run_background_correction(self._corrections_context.background_correction_data[run_group])
        return self.all_runs_and_groups()

    def run_background_correction_for(self, run: str, group: str, rebin: bool) -> None:
        """Calculates the background for some data using a Fit."""
        run_group = tuple([run, group, rebin])
        if run_group in self._corrections_context.background_correction_data:
            self._run_background_correction(self._corrections_context.background_correction_data[run_group])
        return [run], [group], [rebin]

    def _run_background_correction(self, correction_data: BackgroundCorrectionData) -> None:
        """Calculates the background for some data using a Fit."""
        fit_function = self._get_fit_function_for_background_fit(correction_data)

        correction_data.calculate_background(fit_function)
        correction_data.perform_background_subtraction()

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
        runs, groups, rebins, start_xs, end_xs, backgrounds, background_errors, statuses = \
            self._selected_correction_data_for(self._selected_runs(), self._selected_groups())
        return runs, groups, rebins, start_xs, end_xs, backgrounds, background_errors, statuses

    def _selected_correction_data_for(self, selected_runs: list, selected_groups: list) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs_list, groups_list, rebin_list, start_xs, end_xs, backgrounds, background_errors, statuses = [], [], [], [], \
                                                                                                         [], [], [], []
        for run_group, correction_data in self._corrections_context.background_correction_data.items():
            if run_group[0] in selected_runs and run_group[1] in selected_groups:
                rebinned = run_group[2]
                if rebinned and not self._corrections_context.show_rebin_data:
                    continue

                runs_list.append(run_group[0])
                groups_list.append(run_group[1])
                rebin_list.append(rebinned)
                start_xs.append(correction_data.start_x)
                end_xs.append(correction_data.end_x)
                backgrounds.append(correction_data.flat_background.getParameterValue(BACKGROUND_PARAM))
                background_errors.append(correction_data.flat_background.getError(BACKGROUND_PARAM))
                statuses.append(correction_data.status)
        return runs_list, groups_list, rebin_list, start_xs, end_xs, backgrounds, background_errors, statuses

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

    def get_counts_workspace_name(self, run_string: str, group: str, rebin: bool) -> str:
        """Returns the name of the counts workspace associated with the provided run string and group."""
        run_list = self._context.get_runs(run_string)
        workspace_list = []
        if len(run_list) > 0:
            workspace_list = self._context.group_pair_context.get_group_counts_workspace_names(run_list[0], [group], rebin)
        return workspace_list[0] if len(workspace_list) > 0 else None

    def default_x_range(self, run: str, group: str, rebin: bool) -> tuple:
        """Returns the x range to use by default for the background corrections. It is the second half of the data."""
        x_lower, x_upper = self.x_limits_of_workspace(run, group, rebin)
        x_mid = round((x_upper - x_lower)/2.0)
        return x_mid, x_upper

    def x_limits_of_workspace(self, run: str, group: str, rebin: bool) -> tuple:
        """Returns the x data limits of the workspace associated with the provided Run and Group."""
        return x_limits_of_workspace(self.get_counts_workspace_name(run, group, rebin))
