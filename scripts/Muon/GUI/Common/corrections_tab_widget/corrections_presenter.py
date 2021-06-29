# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.corrections_view import CorrectionsView
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_model import DeadTimeCorrectionsModel
from Muon.GUI.Common.corrections_tab_widget.dead_time_corrections_presenter import DeadTimeCorrectionsPresenter

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

        self.dead_time_model = DeadTimeCorrectionsModel(model, context.data_context, context.corrections_context)
        self.dead_time_presenter = DeadTimeCorrectionsPresenter(self.view.dead_time_view, self.dead_time_model, self)

        self.initialize_model_options()

        self.view.set_slot_for_run_selector_changed(self.handle_run_selector_changed)

        self.update_view_from_model_observer = GenericObserverWithArgPassing(
            self.handle_ads_clear_or_remove_workspace_event)
        self.instrument_changed_observer = GenericObserver(self.handle_instrument_changed)
        self.load_observer = GenericObserver(self.handle_runs_loaded)
        self.corrections_complete_observer = GenericObserver(self.handle_corrections_complete)

        self.enable_editing_notifier = GenericObservable()
        self.disable_editing_notifier = GenericObservable()
        self.perform_corrections_notifier = GenericObservable()

    def initialize_model_options(self) -> None:
        """Initialise the model with the default fitting options."""
        self.dead_time_presenter.initialize_model_options()

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

    def handle_run_selector_changed(self) -> None:
        """Handles when the run selector is changed."""
        self.model.set_current_run_string(self.view.current_run_string())
        self.dead_time_presenter.handle_run_selector_changed()

    def handle_corrections_complete(self) -> None:
        """When the corrections have been calculated, update the displayed correction data."""
        self.dead_time_presenter.handle_corrections_complete()

    def current_run_string(self) -> str:
        """Returns the currently selected run string."""
        return self.model.current_run_string()

    def warning_popup(self, message: str) -> None:
        """Displays a warning message."""
        self.view.warning_popup(message)
