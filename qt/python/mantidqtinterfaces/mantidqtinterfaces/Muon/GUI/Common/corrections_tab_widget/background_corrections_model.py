# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from dataclasses import dataclass

from mantid.api import AlgorithmManager, CompositeFunction, FunctionFactory, IFunction, Workspace
from mantid.kernel import PhysicalConstants
from mantidqtinterfaces.Muon.GUI.Common.contexts.corrections_context import (
    BACKGROUND_MODE_NONE,
    BACKGROUND_MODE_AUTO,
    BACKGROUND_MODE_MANUAL,
    FLAT_BACKGROUND,
    FLAT_BACKGROUND_AND_EXP_DECAY,
    RUNS_ALL,
    GROUPS_ALL,
)
from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_context import MuonContext
from mantidqtinterfaces.Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import (
    run_Fit,
    run_clone_workspace,
    run_create_single_valued_workspace,
    run_minus,
)
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import run_string_to_list
from mantidqtinterfaces.Muon.GUI.Common.utilities.workspace_data_utils import x_limits_of_workspace

BACKGROUND_PARAM = "A0"
CORRECTION_SUCCESS_STATUS = "Correction success"
NO_CORRECTION_STATUS = "No background correction"
DEFAULT_A_VALUE = 1e6
DEFAULT_LAMBDA_VALUE = 1.0e-6 / PhysicalConstants.MuonLifetime
DEFAULT_USE_RAW = False
MAX_ACCEPTABLE_CHI_SQUARED = 10.0
TEMPORARY_BACKGROUND_WORKSPACE_NAME = "__temp_background_workspace"


@dataclass
class BackgroundCorrectionData:
    """
    The background correction data associated with a specific group of a run.
    """

    uncorrected_workspace_name: str
    workspace_name: str
    uncorrected_rebin_workspace_name: str
    rebin_workspace_name: str

    use_raw: bool
    rebin_fixed_step: int

    start_x: float
    end_x: float
    flat_background: IFunction
    exp_decay: IFunction
    status: str = NO_CORRECTION_STATUS

    def __init__(
        self,
        use_raw: bool,
        rebin_fixed_step: int,
        start_x: float,
        end_x: float,
        flat_background: IFunction = None,
        exp_decay: IFunction = None,
    ):
        self.use_raw = use_raw
        self.rebin_fixed_step = rebin_fixed_step
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
            self.exp_decay.setParameter("Lambda", DEFAULT_LAMBDA_VALUE)
            self.exp_decay.fixParameter("Lambda")

    def setup_uncorrected_workspace(self, workspace_name: str, rebin_workspace_name: str) -> None:
        """Clones the Counts workspace into a hidden workspace in the ADS."""
        self.workspace_name = workspace_name
        self.uncorrected_workspace_name = f"__{workspace_name}_uncorrected"

        run_clone_workspace({"InputWorkspace": self.workspace_name, "OutputWorkspace": self.uncorrected_workspace_name})

        self.rebin_workspace_name = rebin_workspace_name
        if self.rebin_workspace_name is not None:
            self.uncorrected_rebin_workspace_name = f"__{rebin_workspace_name}_uncorrected"
            run_clone_workspace({"InputWorkspace": self.rebin_workspace_name, "OutputWorkspace": self.uncorrected_rebin_workspace_name})

    def reset(self) -> None:
        """Resets the background correction data by replacing the corrected data with the original uncorrected data."""
        self.setup_functions()
        run_clone_workspace({"InputWorkspace": self.uncorrected_workspace_name, "OutputWorkspace": self.workspace_name})

        if self.rebin_workspace_name is not None:
            run_clone_workspace({"InputWorkspace": self.uncorrected_rebin_workspace_name, "OutputWorkspace": self.rebin_workspace_name})

    def create_background_output_workspaces(self, fit_function: IFunction) -> tuple:
        """Creates the output workspaces for the currently stored background data."""
        params = self._get_parameters_for_background_fit(fit_function, create_output=True, max_iterations=0)
        _, parameter_table_name, _, _, _, covariance_matrix_name = run_Fit(params, AlgorithmManager.create("Fit"))
        return parameter_table_name, covariance_matrix_name

    def calculate_background(self, fit_function: IFunction) -> None:
        """Calculates the background in the counts workspace."""
        params = self._get_parameters_for_background_fit(fit_function, create_output=False)
        function, fit_status, chi_squared = run_Fit(params, AlgorithmManager.create("Fit"))

        self._handle_background_fit_output(function, fit_status, chi_squared)

    def _get_parameters_for_background_fit(self, fit_function: IFunction, create_output: bool, max_iterations: int = 500) -> dict:
        """Gets the parameters to use for the background Fit."""
        input_name = self.uncorrected_workspace_name if self._use_raw_data() else self.uncorrected_rebin_workspace_name
        return {
            "Function": str(fit_function),
            "InputWorkspace": input_name,
            "StartX": self.start_x,
            "EndX": self.end_x,
            "CreateOutput": create_output,
            "CalcErrors": True,
            "MaxIterations": max_iterations,
            "IgnoreInvalidData": True,
        }

    def _handle_background_fit_output(self, function: IFunction, fit_status: str, chi_squared: float) -> None:
        """Handles the output of the background fit."""
        if chi_squared > MAX_ACCEPTABLE_CHI_SQUARED:
            self.setup_functions()
            self.status = f"Correction skipped - chi squared is poor ({chi_squared:.3f})."
        elif "Failed to converge" in fit_status:
            self.setup_functions()
            self.status = f"Correction skipped - {fit_status}"
        else:
            self._handle_background_fit_output_success(function)

    def _handle_background_fit_output_success(self, function: IFunction) -> None:
        """Handles the output of a successful background fit."""
        if isinstance(function, CompositeFunction):
            background = function.getFunction(0).getParameterValue(BACKGROUND_PARAM)
            background_error = function.getFunction(0).getError(BACKGROUND_PARAM)
        else:
            background = function.getParameterValue(BACKGROUND_PARAM)
            background_error = function.getError(BACKGROUND_PARAM)

        if not self._use_raw_data():
            background /= self.rebin_fixed_step
            background_error /= self.rebin_fixed_step

        self.flat_background.setParameter(BACKGROUND_PARAM, background)
        self.flat_background.setError(BACKGROUND_PARAM, background_error)
        self.status = CORRECTION_SUCCESS_STATUS

    def perform_background_subtraction(self) -> None:
        """Performs the background subtraction on the counts workspace, and then generates the asymmetry workspace."""
        run_minus(self.uncorrected_workspace_name, self._create_background_workspace(rebin=False), self.workspace_name)
        if self.rebin_workspace_name is not None:
            run_minus(self.uncorrected_rebin_workspace_name, self._create_background_workspace(rebin=True), self.rebin_workspace_name)

    def _create_background_workspace(self, rebin: bool) -> Workspace:
        """Creates the background workspace to use for the background subtraction."""
        return run_create_single_valued_workspace(self._get_parameters_for_creating_background_workspace(rebin))

    def _get_parameters_for_creating_background_workspace(self, rebin: bool) -> dict:
        """Returns the parameters to use when creating a single valued background workspace."""
        background = self.flat_background.getParameterValue(BACKGROUND_PARAM)
        background_error = self.flat_background.getError(BACKGROUND_PARAM)
        if rebin:
            background *= self.rebin_fixed_step
            background_error *= self.rebin_fixed_step

        return {"DataValue": background, "ErrorValue": background_error, "OutputWorkspace": TEMPORARY_BACKGROUND_WORKSPACE_NAME}

    def _use_raw_data(self) -> bool:
        """Returns true if the raw data should be used to calculate the background."""
        return self.use_raw or self.rebin_fixed_step == 0 or self.rebin_workspace_name is None


class BackgroundCorrectionsModel:
    """
    The BackgroundCorrectionsModel calculates Background corrections.
    """

    def __init__(self, corrections_model: CorrectionsModel, context: MuonContext):
        """Initialize the BackgroundCorrectionsModel with empty data."""
        self._corrections_model = corrections_model
        self._context = context
        self._corrections_context = context.corrections_context

    def do_rebin(self) -> bool:
        """Returns true if rebinned data exists in the ADS."""
        return self._context._do_rebin()

    def rebin_fixed_steps(self) -> int:
        """Returns the Rebin steps to use for Fixed Rebin mode."""
        return int(self._context.gui_context["RebinFixed"]) if "RebinFixed" in self._context.gui_context else 0

    def is_rebin_fixed_selected(self) -> bool:
        """Returns true if the Rebin Fixed mode is currently selected."""
        return self._context.gui_context["RebinType"] == "Fixed" if "RebinType" in self._context.gui_context else False

    def set_background_correction_mode(self, mode: str) -> None:
        """Sets the current background correction mode in the context."""
        self._corrections_context.background_corrections_mode = mode

    def is_background_mode_none(self) -> bool:
        """Returns true if the current background correction mode is none."""
        return self._corrections_context.background_corrections_mode == BACKGROUND_MODE_NONE

    def is_background_mode_auto(self) -> bool:
        """Returns true if the current background correction mode is auto."""
        return self._corrections_context.background_corrections_mode == BACKGROUND_MODE_AUTO

    def is_background_mode_manual(self) -> bool:
        """Returns true if the current background correction mode is manual."""
        return self._corrections_context.background_corrections_mode == BACKGROUND_MODE_MANUAL

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

    def set_use_raw(self, run: str, group: str, use_raw: bool) -> None:
        """Sets whether the background should be calculated using the raw data or rebinned data."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            self._corrections_context.background_correction_data[run_group].use_raw = use_raw

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

    def set_background(self, run: str, group: str, background: float) -> None:
        """Sets the Background associated with the provided Run and Group."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            correction_data = self._corrections_context.background_correction_data[run_group]
            correction_data.flat_background.setParameter(BACKGROUND_PARAM, background)
            correction_data.flat_background.setError(BACKGROUND_PARAM, 0.0)

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
        old_data = self._corrections_context.background_correction_data
        self.clear_background_corrections_data()

        self._populate_background_corrections_data_using_previous_data(old_data)

    def _populate_background_corrections_data_using_previous_data(self, previous_data: dict) -> None:
        """Populates the background correction data, whilst trying to reuse some of the previous data."""
        groups = self.group_names()
        for run in self._corrections_model.run_number_strings():
            for group in groups:
                self._add_background_correction_data_for(previous_data, run, group)

    def _add_background_correction_data_for(self, previous_data: dict, run: str, group: str) -> None:
        """Add background correction data for the provided Run and Group."""
        workspace_name = self.get_counts_workspace_name(run, group, False)
        rebin_workspace_name = self.get_counts_workspace_name(run, group, True) if self.do_rebin() else None

        if workspace_name is not None:
            run_group = tuple([run, group])
            background_data = self._create_background_correction_data(previous_data, run_group)

            self._corrections_context.background_correction_data[run_group] = background_data
            self._corrections_context.background_correction_data[run_group].setup_uncorrected_workspace(
                workspace_name, rebin_workspace_name
            )

    def _create_background_correction_data(self, previous_data: dict, run_group: tuple) -> BackgroundCorrectionData:
        """Creates the BackgroundCorrectionData for a newly loaded data. It tries to reuse previous data."""
        rebin_fixed, is_rebin_fixed = self.rebin_fixed_steps(), self.is_rebin_fixed_selected()

        run, group = run_group
        if run_group in previous_data:
            data = previous_data[run_group]
            return BackgroundCorrectionData(
                data.use_raw if is_rebin_fixed else DEFAULT_USE_RAW,
                rebin_fixed,
                data.start_x,
                data.end_x,
                data.flat_background,
                data.exp_decay,
            )
        else:
            for key, value in previous_data.items():
                if key[1] == group:
                    return BackgroundCorrectionData(
                        value.use_raw if is_rebin_fixed else DEFAULT_USE_RAW,
                        rebin_fixed,
                        value.start_x,
                        value.end_x,
                        value.flat_background,
                        value.exp_decay,
                    )

        start_x, end_x = self.default_x_range(run, group)
        return BackgroundCorrectionData(DEFAULT_USE_RAW, rebin_fixed, start_x, end_x)

    def create_background_output_workspaces_for(self, run: str, group: str) -> tuple:
        """Creates the parameter table and normalised covariance matrix for a Run/Group by performing a fit."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            correction_data = self._corrections_context.background_correction_data[run_group]
            fit_function = self._get_fit_function_for_background_fit(correction_data)
            return correction_data.create_background_output_workspaces(fit_function)
        else:
            return None, None

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

    def run_background_correction_for(self, run: str, group: str) -> None:
        """Calculates the background for some data using a Fit."""
        run_group = tuple([run, group])
        if run_group in self._corrections_context.background_correction_data:
            self._run_background_correction(self._corrections_context.background_correction_data[run_group])
        return [run], [group]

    def _run_background_correction(self, correction_data: BackgroundCorrectionData) -> None:
        """Calculates the background for some data using a Fit."""
        if self.is_background_mode_auto():
            fit_function = self._get_fit_function_for_background_fit(correction_data)
            correction_data.calculate_background(fit_function)
        elif self.is_background_mode_manual():
            correction_data.status = (
                CORRECTION_SUCCESS_STATUS
                if correction_data.flat_background.getParameterValue(BACKGROUND_PARAM) != 0.0
                else NO_CORRECTION_STATUS
            )

        correction_data.perform_background_subtraction()

    def _get_fit_function_for_background_fit(self, correction_data: BackgroundCorrectionData) -> IFunction:
        """Returns the fit function to use for a background fit."""
        correction_data = self._set_background_parameter_if_not_using_raw(correction_data)

        if self._corrections_context.selected_function == FLAT_BACKGROUND:
            return correction_data.flat_background
        elif self._corrections_context.selected_function == FLAT_BACKGROUND_AND_EXP_DECAY:
            composite = FunctionFactory.createFunction("CompositeFunction")
            composite.add(correction_data.flat_background)
            composite.add(correction_data.exp_decay)
            return composite

        raise RuntimeError("The selected background function is not recognised.")

    @staticmethod
    def _set_background_parameter_if_not_using_raw(correction_data: BackgroundCorrectionData) -> BackgroundCorrectionData:
        """Sets the background parameter to an appropriate value before doing a fit using rebinned data."""
        rebin_fixed_step = correction_data.rebin_fixed_step
        if not correction_data.use_raw and rebin_fixed_step != 0:
            background = correction_data.flat_background.getParameterValue(BACKGROUND_PARAM) * rebin_fixed_step
            background_error = correction_data.flat_background.getError(BACKGROUND_PARAM) * rebin_fixed_step
            correction_data.flat_background.setParameter(BACKGROUND_PARAM, background)
            correction_data.flat_background.setError(BACKGROUND_PARAM, background_error)
        return correction_data

    def get_warning_for_correction_tab(self) -> bool:
        """Returns a message if not all the background corrections were applied successfully."""
        if not self.is_background_mode_none():
            for correction_data in self._corrections_context.background_correction_data.values():
                if "skipped" in correction_data.status:
                    return "Background corrections skipped for some domains."
        return ""

    def any_negative_backgrounds(self) -> bool:
        """Returns true if a negative background exists in one of the background corrected domains."""
        if not self.is_background_mode_none():
            for correction_data in self._corrections_context.background_correction_data.values():
                if correction_data.flat_background.getParameterValue(BACKGROUND_PARAM) < 0.0:
                    return True
        return False

    def selected_correction_data(self) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs, groups, use_raws, start_xs, end_xs, backgrounds, background_errors, statuses = self._selected_correction_data_for(
            self._selected_runs(), self._selected_groups()
        )
        return runs, groups, use_raws, start_xs, end_xs, backgrounds, background_errors, statuses

    def _selected_correction_data_for(self, selected_runs: list, selected_groups: list) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs_list, groups_list, use_raws, start_xs, end_xs, backgrounds, background_errors, statuses = [], [], [], [], [], [], [], []
        for run_group, correction_data in self._corrections_context.background_correction_data.items():
            if run_group[0] in selected_runs and run_group[1] in selected_groups:
                runs_list.append(run_group[0])
                groups_list.append(run_group[1])
                use_raws.append(correction_data.use_raw)
                start_xs.append(correction_data.start_x)
                end_xs.append(correction_data.end_x)
                backgrounds.append(correction_data.flat_background.getParameterValue(BACKGROUND_PARAM))
                background_errors.append(correction_data.flat_background.getError(BACKGROUND_PARAM))
                statuses.append(correction_data.status)
        return runs_list, groups_list, use_raws, start_xs, end_xs, backgrounds, background_errors, statuses

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

    def get_counts_workspace_name(self, run_string: str, group: str, rebin: bool = False) -> str:
        """Returns the name of the counts workspace associated with the provided run string and group."""
        run_list = self._context.get_runs(run_string)
        workspace_list = []
        if len(run_list) > 0:
            workspace_list = self._context.group_pair_context.get_group_counts_workspace_names(run_list[0], [group], rebin)
        return workspace_list[0] if len(workspace_list) > 0 else None

    def default_x_range(self, run: str, group: str) -> tuple:
        """Returns the x range to use by default for the background corrections. It is the second half of the data."""
        run_list = run_string_to_list(run)
        last_good_data = self._context.last_good_data(run_list)

        if last_good_data != 0.0:
            return self._get_range_for_second_half_of_data(self._context.first_good_data(run_list), last_good_data)
        else:
            return self._get_x_range_from_counts_workspace(run, group)

    def _get_x_range_from_counts_workspace(self, run: str, group: str) -> tuple:
        """Get the default start and end X from the counts workspace corresponding to the provided run and group."""
        x_lower, x_upper = self.x_limits_of_workspace(run, group)
        return self._get_range_for_second_half_of_data(x_lower, x_upper)

    @staticmethod
    def _get_range_for_second_half_of_data(x_lower: float, x_upper: float) -> tuple:
        """Returns an x range representing the second half of the data range provided."""
        return (x_upper - x_lower) / 2.0 + x_lower, x_upper

    def x_limits_of_workspace(self, run: str, group: str) -> tuple:
        """Returns the x data limits of the workspace associated with the provided Run and Group."""
        return x_limits_of_workspace(self.get_counts_workspace_name(run, group, False))
