# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.corrections_tab_widget.background_corrections_model import BackgroundCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.background_corrections_presenter import BackgroundCorrectionsPresenter
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_model import DeadTimeCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_presenter import DeadTimeCorrectionsPresenter
from Muon.GUI.Common.thread_model import ThreadModel
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapperWithOutput

from qtpy.QtCore import QMetaObject, QObject, Slot


class CorrectionsPresenter(QObject):
    """
    The CorrectionsPresenter has a CorrectionsView and CorrectionsModel.
    """

    def __init__(self, view: CorrectionsView, model: CorrectionsModel, context: MuonContext):
        """Initialize the CorrectionsPresenter. Sets up the slots and event observers."""
        super(CorrectionsPresenter, self).__init__()
        self.view = view
        self.model = model

        self.thread_success = True

        self.dead_time_model = DeadTimeCorrectionsModel(model, context.data_context, context.corrections_context)
        self.dead_time_presenter = DeadTimeCorrectionsPresenter(self.view.dead_time_view, self.dead_time_model, self)

        self.background_model = BackgroundCorrectionsModel(model, context)
        self.background_presenter = BackgroundCorrectionsPresenter(self.view.background_view, self.background_model,
                                                                   self)

        self.initialize_model_options()

        self.view.set_slot_for_run_selector_changed(self.handle_run_selector_changed)

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.handle_ads_clear_or_remove_workspace_event)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)
        self.load_observer = GenericObserver(self.handle_runs_loaded)
        self.group_change_observer = GenericObserver(self.handle_groups_changed)
        self.pre_process_and_counts_calculated_observer = GenericObserver(self.handle_pre_process_and_counts_calculated)

        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.set_tab_warning_notifier = GenericObservable()
        self.perform_corrections_notifier = GenericObservable()
        self.asymmetry_pair_and_diff_calculations_finished_notifier = GenericObservable()

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.dead_time_presenter.initialize_model_options()
        self.background_presenter.initialize_model_options()

    def handle_ads_clear_or_remove_workspace_event(self, _: str = None) -> None:
        """Handle when there is a clear or remove workspace event in the ADS."""
        self.dead_time_presenter.handle_ads_clear_or_remove_workspace_event()
        self.handle_runs_loaded()

    def handle_instrument_changed(self) -> None:
        """User changes the selected instrument."""
        QMetaObject.invokeMethod(self, "_handle_instrument_changed")

    @Slot()
    def _handle_instrument_changed(self) -> None:
        """Handles when new run numbers are loaded from the GUI thread."""
        self.dead_time_presenter.handle_instrument_changed()
        self.background_presenter.handle_instrument_changed()

    def handle_runs_loaded(self) -> None:
        """Handles when new run numbers are loaded. QMetaObject is required so its executed on the GUI thread."""
        QMetaObject.invokeMethod(self, "_handle_runs_loaded")

    @Slot()
    def _handle_runs_loaded(self) -> None:
        """Handles when new run numbers are loaded from the GUI thread."""
        self.view.update_run_selector_combo_box(self.model.run_number_strings())
        self.model.set_current_run_string(self.view.current_run_string())

        if self.model.number_of_run_strings == 0:
            self.view.disable_view()
        else:
            self.view.enable_view()

    def handle_groups_changed(self) -> None:
        """Handles when the selected groups have changed in the grouping tab."""
        self.background_presenter.handle_groups_changed()

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        self.model.set_current_run_string(self.view.current_run_string())
        self.dead_time_presenter.handle_run_selector_changed()
        self.background_presenter.handle_run_selector_changed()

    def handle_pre_process_and_counts_calculated(self) -> None:
        """Handles when MuonPreProcess and counts workspaces have been calculated."""
        self.dead_time_presenter.handle_pre_process_and_counts_calculated()
        self.background_presenter.handle_pre_process_and_counts_calculated()

    def handle_thread_calculation_started(self) -> None:
        """Handles when a calculation on a thread has started."""
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = True

    def handle_background_corrections_for_all_finished(self) -> None:
        """Handle when the background corrections for all has finished."""
        self.enable_editing_notifier.notify_subscribers()
        if not self.thread_success:
            return

        self.background_presenter.handle_background_corrections_for_all_finished()

        corrected_runs_and_groups = self.thread_model_wrapper.result
        if corrected_runs_and_groups is not None:
            runs, groups = corrected_runs_and_groups
            self._perform_asymmetry_pairs_and_diffs_calculation(runs, groups)

    def handle_asymmetry_pairs_and_diffs_calc_finished(self) -> None:
        """Handle when the calculation of Asymmetry, Pairs and Diffs has finished finished."""
        self.enable_editing_notifier.notify_subscribers()
        self.asymmetry_pair_and_diff_calculations_finished_notifier.notify_subscribers()

    def handle_thread_error(self, error: str) -> None:
        """Handle when an error occurs while doing calculations on a thread."""
        self.disable_editing_notifier.notify_subscribers()
        self.thread_success = False
        self.view.warning_popup(error)

    def current_run_string(self) -> str:
        """Returns the currently selected run string."""
        return self.model.current_run_string()

    def _perform_asymmetry_pairs_and_diffs_calculation(self, *args) -> None:
        """Calculate the Asymmetry workspaces, Pairs and Diffs on a thread after background corrections are complete."""
        try:
            self.calculation_thread = self.create_calculation_thread(self._calculate_asymmetry_pairs_and_diffs, *args)
            self.calculation_thread.threadWrapperSetUp(self.handle_thread_calculation_started,
                                                       self.handle_asymmetry_pairs_and_diffs_calc_finished,
                                                       self.handle_thread_error)
            self.calculation_thread.start()
        except ValueError as error:
            self.view.warning_popup(error)

    def _calculate_asymmetry_pairs_and_diffs(self, runs: list, groups: list) -> None:
        """Calculates the Asymmetry workspaces, Pairs and Diffs only for the provided runs and groups."""
        # Calculates the Asymmetry workspaces for the corresponding runs and groups that have just been corrected
        self.model.calculate_asymmetry_workspaces_for(runs, groups)
        # Calculates the Pair Asymmetry workspaces for pairs formed from one or more groups which have been corrected
        pairs = self.model.calculate_pairs_for(runs, groups)
        # Calculates the Diff Asymmetry workspaces formed from one or more groups/pairs which have been corrected
        self.model.calculate_diffs_for(runs, groups + pairs)

    def create_calculation_thread(self, callback, *args) -> ThreadModel:
        """Create a thread for calculations."""
        self.thread_model_wrapper = ThreadModelWrapperWithOutput(callback, *args)
        return ThreadModel(self.thread_model_wrapper)

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        self.view.warning_popup(message)

    def set_tab_warning(self, message: str) -> None:
        """Sets a warning message as the tooltip of the corrections tab."""
        self.view.set_tab_warning(message)
