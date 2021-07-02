# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.corrections_tab_widget.background_corrections_model import BackgroundCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.background_corrections_view import (BackgroundCorrectionsView,
                                                                                BACKGROUND_MODE_NONE)


class BackgroundCorrectionsPresenter:
    """
    The BackgroundCorrectionsPresenter has a BackgroundCorrectionsView and BackgroundCorrectionsModel.
    """

    def __init__(self, view: BackgroundCorrectionsView,  model: BackgroundCorrectionsModel, corrections_presenter):
        """Initialize the DeadTimeCorrectionsPresenter. Sets up the slots and event observers."""
        self.view = view
        self.model = model
        self._corrections_presenter = corrections_presenter

        self.view.set_slot_for_mode_combo_box_changed(self.handle_mode_combo_box_changed)
        self.view.set_slot_for_group_combo_box_changed(self.handle_selected_group_changed)
        self.view.set_slot_for_show_all_runs(self.handle_show_all_runs_ticked)
        self.view.set_slot_for_start_x_changed(self.handle_start_x_changed)
        self.view.set_slot_for_end_x_changed(self.handle_end_x_changed)

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.set_background_correction_mode(BACKGROUND_MODE_NONE)

    def handle_instrument_changed(self) -> None:
        """User changes the selected instrument."""
        self.model.set_background_correction_mode(BACKGROUND_MODE_NONE)
        self.view.background_correction_mode = BACKGROUND_MODE_NONE

    def handle_runs_loaded(self) -> None:
        """Handles when new run numbers are loaded into the interface."""
        self.model.populate_background_corrections_data()

    def handle_groups_changed(self) -> None:
        """Handles when the selected groups have changed in the grouping tab."""
        self.view.populate_group_selector(self.model.group_names())

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        self._update_displayed_corrections_data()

    def handle_mode_combo_box_changed(self) -> None:
        """Handles when the background corrections mode is changed."""
        self.view.set_background_correction_options_visible(not self.view.is_mode_none())

        if self.view.is_mode_auto():
            self._handle_mode_auto_selected()

    def _handle_mode_auto_selected(self) -> None:
        """Handles when the background corrections mode is changed to 'Auto'."""
        pass

    def handle_selected_group_changed(self) -> None:
        """Handles when the selected group has changed."""
        self.model.set_selected_group(self.view.selected_group)
        self._update_displayed_corrections_data()

    def handle_show_all_runs_ticked(self) -> None:
        """Handles when the show all runs check box is ticked or unticked."""
        self.model.set_show_all_runs(self.view.show_all_runs)
        self._update_displayed_corrections_data()

    def handle_start_x_changed(self) -> None:
        """Handles when a Start X table cell is changed."""
        run, group = self.view.selected_run_and_group()
        self._check_start_x_is_valid_for(run, group)

        self.model.set_start_x(run, group, self.view.start_x(run, group))
        self.model.set_end_x(run, group, self.view.end_x(run, group))

    def handle_end_x_changed(self) -> None:
        """Handles when a End X table cell is changed."""
        run, group = self.view.selected_run_and_group()
        self._check_end_x_is_valid_for(run, group)

        self.model.set_start_x(run, group, self.view.start_x(run, group))
        self.model.set_end_x(run, group, self.view.end_x(run, group))

    def _update_displayed_corrections_data(self) -> None:
        """Updates the displayed corrections data using the data stored in the model."""
        runs, groups, start_xs, end_xs = self.model.selected_correction_data()
        self.view.populate_corrections_table(runs, groups, start_xs, end_xs)

    def _check_start_x_is_valid_for(self, run: str, group: str) -> None:
        """Checks that the new start X is valid. If it isn't, the start and end X is adjusted."""
        x_lower, x_upper = self.model.x_limits_of_workspace(run, group)

        view_start_x = self.view.start_x(run, group)
        view_end_x = self.view.end_x(run, group)
        model_start_x = self.model.start_x(run, group)

        if view_start_x < x_lower:
            self.view.set_start_x(run, group, x_lower)
        elif view_start_x > x_upper:
            if not self.model.is_equal_to_n_decimals(view_end_x, x_upper, 3):
                self.view.set_start_x(run, group, view_end_x)
                self.view.set_end_x(run, group, x_upper)
            else:
                self.view.set_start_x(run, group, model_start_x)
        elif view_start_x > view_end_x:
            self.view.set_start_x(run, group, view_end_x)
            self.view.set_end_x(run, group, view_start_x)
        elif view_start_x == view_end_x:
            self.view.set_start_x(run, group, model_start_x)

    def _check_end_x_is_valid_for(self, run: str, group: str) -> None:
        """Checks that the new end X is valid. If it isn't, the start and end X is adjusted."""
        x_lower, x_upper = self.model.x_limits_of_workspace(run, group)

        view_start_x = self.view.start_x(run, group)
        view_end_x = self.view.end_x(run, group)
        model_end_x = self.model.end_x(run, group)

        if view_end_x < x_lower:
            if not self.model.is_equal_to_n_decimals(view_start_x, x_lower, 3):
                self.view.set_start_x(run, group, x_lower)
                self.view.set_end_x(run, group, view_start_x)
            else:
                self.view.set_end_x(run, group, model_end_x)
        elif view_end_x > x_upper:
            self.view.set_end_x(run, group, x_upper)
        elif view_end_x < view_start_x:
            self.view.set_start_x(run, group, view_end_x)
            self.view.set_end_x(run, group, view_start_x)
        elif view_end_x == view_start_x:
            self.view.set_end_x(run, group, model_end_x)
