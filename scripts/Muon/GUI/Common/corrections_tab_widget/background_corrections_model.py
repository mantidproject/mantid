# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass

from mantid.api import FunctionFactory, IFunction
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws
from Muon.GUI.Common.contexts.corrections_context import (CorrectionsContext, BACKGROUND_MODE_NONE, RUNS_ALL,
                                                          GROUPS_ALL)
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel

DEFAULT_START_X = 5.0
DEFAULT_END_X = 15.0
DEFAULT_X_LOWER = 0.0
DEFAULT_X_UPPER = 100.0
X_OFFSET = 0.001


@dataclass
class BackgroundCorrectionData:
    """
    The background correction data associated with a specific group of a run.
    """
    start_x: float = DEFAULT_START_X
    end_x: float = DEFAULT_END_X
    flat_background: IFunction = FunctionFactory.createFunction("FlatBackground")
    exp_decay: IFunction = FunctionFactory.createFunction("ExpDecay")


class BackgroundCorrectionsModel:
    """
    The BackgroundCorrectionsModel calculates Background corrections.
    """

    def __init__(self, corrections_model: CorrectionsModel, context: MuonContext,
                 corrections_context: CorrectionsContext):
        """Initialize the BackgroundCorrectionsModel with empty data."""
        self._corrections_model = corrections_model
        self._context = context
        self._corrections_context = corrections_context

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
        return DEFAULT_START_X

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
        return DEFAULT_END_X

    @staticmethod
    def is_equal_to_n_decimals(value1: float, value2: float, n_decimals: int) -> bool:
        """Checks that two floats are equal up to n decimal places."""
        return f"{value1:.{n_decimals}f}" == f"{value2:.{n_decimals}f}"

    def populate_background_corrections_data(self) -> None:
        """Populates the background correction data dictionary when runs are initially loaded into the interface."""
        self._corrections_context.background_correction_data = {}
        groups = self.group_names()
        for run in self._corrections_model.run_number_strings():
            for group in groups:
                self._corrections_context.background_correction_data[tuple([run, group])] = BackgroundCorrectionData()

    def selected_correction_data(self) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs, groups, start_xs, end_xs, a0s, a0_errors = self._selected_correction_data_for(
            self._selected_runs(), self._selected_groups())
        return runs, groups, start_xs, end_xs, a0s, a0_errors

    def _selected_correction_data_for(self, selected_runs: list, selected_groups: list) -> tuple:
        """Returns lists of the selected correction data to display in the view."""
        runs_list, groups_list, start_xs, end_xs, a0s, a0_errors = [], [], [], [], [], []
        for run_group, correction_data in self._corrections_context.background_correction_data.items():
            if run_group[0] in selected_runs and run_group[1] in selected_groups:
                runs_list.append(run_group[0])
                groups_list.append(run_group[1])
                start_xs.append(correction_data.start_x)
                end_xs.append(correction_data.end_x)
                a0s.append(correction_data.flat_background.getParameterValue("A0"))
                a0_errors.append(correction_data.flat_background.getError("A0"))
        return runs_list, groups_list, start_xs, end_xs, a0s, a0_errors

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

    def _get_counts_workspace_name(self, run_string: str, group: str) -> str:
        """Returns the name of the counts workspace associated with the provided run string and group."""
        run_list = self._context.get_runs(run_string)
        if len(run_list) > 0:
            workspace_list = self._context.group_pair_context.get_group_counts_workspace_names(run_list[0], [group])
        return workspace_list[0] if len(workspace_list) > 0 else None

    def x_limits_of_workspace(self, run: str, group: str) -> tuple:
        """Returns the x data limits of the workspace associated with the provided Run and Group."""
        return self._x_limits_of_workspace(self._get_counts_workspace_name(run, group))

    @staticmethod
    def _x_limits_of_workspace(workspace_name: str) -> tuple:
        """Returns the x data limits of a provided workspace."""
        if workspace_name is not None and check_if_workspace_exist(workspace_name):
            x_data = retrieve_ws(workspace_name).dataX(0)
            if len(x_data) > 0:
                x_data.sort()
                x_lower, x_higher = x_data[0], x_data[-1]
                if x_lower == x_higher:
                    return x_lower - X_OFFSET, x_higher + X_OFFSET
                return x_lower, x_higher
        return DEFAULT_X_LOWER, DEFAULT_X_UPPER
