# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.contexts.corrections_context import BACKGROUND_MODE_NONE, FLAT_BACKGROUND
from Muon.GUI.Common.corrections_tab_widget.background_corrections_model import BackgroundCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.background_corrections_view import BackgroundCorrectionsView
from Muon.GUI.Common.utilities.workspace_data_utils import check_start_x_is_valid, check_end_x_is_valid


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
        self.view.set_slot_for_select_function_combo_box_changed(self.handle_select_function_combo_box_changed)
        self.view.set_slot_for_group_combo_box_changed(self.handle_selected_group_changed)
        self.view.set_slot_for_show_all_runs(self.handle_show_all_runs_ticked)
        self.view.set_slot_for_start_x_changed(self.handle_start_x_changed)
        self.view.set_slot_for_end_x_changed(self.handle_end_x_changed)

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.model.set_background_correction_mode(self.view.background_correction_mode)
        self.model.set_selected_function(self.view.selected_function)

    def handle_instrument_changed(self) -> None:
        """User changes the selected instrument."""
        self.model.set_background_correction_mode(BACKGROUND_MODE_NONE)
        self.model.set_selected_function(FLAT_BACKGROUND)
        self.view.background_correction_mode = BACKGROUND_MODE_NONE
        self.view.selected_function = FLAT_BACKGROUND
        self.model.clear_background_corrections_data()

    def handle_pre_process_and_counts_calculated(self) -> None:
        """Handles when MuonPreProcess and counts workspaces have been calculated."""
        self.model.populate_background_corrections_data()
        self._run_background_corrections_for_all()

    def handle_groups_changed(self) -> None:
        """Handles when the selected groups have changed in the grouping tab."""
        self.view.populate_group_selector(self.model.group_names())

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        self._update_displayed_corrections_data()

    def handle_mode_combo_box_changed(self) -> None:
        """Handles when the background corrections mode is changed."""
        self.model.set_background_correction_mode(self.view.background_correction_mode)
        self.view.set_background_correction_options_visible(not self.model.is_background_mode_none())
        self._run_background_corrections_for_all()

    def handle_select_function_combo_box_changed(self) -> None:
        """Handles when the selected function is changed."""
        self.model.set_selected_function(self.view.selected_function)
        self._run_background_corrections_for_all()

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
        self._handle_start_or_end_x_changed(self._get_new_x_range_when_start_x_changed)

    def handle_end_x_changed(self) -> None:
        """Handles when a End X table cell is changed."""
        self._handle_start_or_end_x_changed(self._get_new_x_range_when_end_x_changed)

    def handle_background_corrections_for_all_finished(self) -> None:
        """Handle when the background corrections has finished."""
        self._update_displayed_corrections_data()

    def _handle_start_or_end_x_changed(self, get_new_x_range) -> None:
        """Handles when a Start X or End X is changed using an appropriate getter to get the new x range."""
        runs, groups, rebins = self._selected_runs_and_groups()
        for run, group, rebin in zip(runs, groups, rebins):
            new_start_x, new_end_x = get_new_x_range(run, group, rebin)
            self._update_start_and_end_x_in_view_and_model(run, group, rebin, new_start_x, new_end_x)

        if len(runs) == 1:
            self._perform_background_corrections(self.model.run_background_correction_for, runs[0], groups[0], rebins[0])
        elif len(runs) > 1:
            self._perform_background_corrections(self.model.run_background_correction_for_all)

    def _get_new_x_range_when_start_x_changed(self, run: str, group: str, rebin: bool) -> tuple:
        """Returns the new x range for a domain when the start X has been changed."""
        return check_start_x_is_valid(self.model.get_counts_workspace_name(run, group, rebin),
                                      self.view.selected_start_x(), self.model.end_x(run, group, rebin),
                                      self.model.start_x(run, group, rebin))

    def _get_new_x_range_when_end_x_changed(self, run: str, group: str, rebin: bool) -> tuple:
        """Returns the new x range for a domain when the end X has been changed."""
        return check_end_x_is_valid(self.model.get_counts_workspace_name(run, group, rebin),
                                    self.model.start_x(run, group, rebin), self.view.selected_end_x(),
                                    self.model.end_x(run, group, rebin))

    def _run_background_corrections_for_all(self) -> None:
        """Runs the background corrections for all stored data in the corrections context."""
        if self.model.is_background_mode_none():
            self._perform_background_corrections(self.model.reset_background_subtraction_data)
        else:
            self._perform_background_corrections(self.model.run_background_correction_for_all)

    def _update_displayed_corrections_data(self) -> None:
        """Updates the displayed corrections data using the data stored in the model."""
        runs, groups, rebins, start_xs, end_xs, backgrounds, background_errors, statuses = \
            self.model.selected_correction_data()
        self.view.populate_corrections_table(runs, groups, rebins, start_xs, end_xs, backgrounds, background_errors, statuses)

    def _update_start_and_end_x_in_view_and_model(self, run: str, group: str, rebin: bool, start_x: float, end_x: float) -> None:
        """Updates the start and end x in the model using the provided values."""
        if self.view.is_run_group_displayed(run, group, rebin):
            self.view.set_start_x(run, group, rebin, start_x)
            self.view.set_end_x(run, group, rebin, end_x)
        self.model.set_start_x(run, group, rebin, start_x)
        self.model.set_end_x(run, group, rebin, end_x)

    def _perform_background_corrections(self, calculation_func, *args) -> None:
        """Creates a matrix workspace for each possible parameter combination to be used for fitting."""
        try:
            self.correction_calculation_thread = self._corrections_presenter.create_calculation_thread(calculation_func,
                                                                                                       *args)
            self.correction_calculation_thread.threadWrapperSetUp(
                self._corrections_presenter.handle_thread_calculation_started,
                self._corrections_presenter.handle_background_corrections_for_all_finished,
                self._corrections_presenter.handle_thread_error)
            self.correction_calculation_thread.start()
        except ValueError as error:
            self._corrections_presenter.warning_popup(error)

    def _selected_runs_and_groups(self) -> tuple:
        """Returns the runs and groups to apply the parameter changes to."""
        apply_to_all = self.view.apply_table_changes_to_all()
        return self.model.all_runs_and_groups() if apply_to_all else self.view.selected_run_and_group()
