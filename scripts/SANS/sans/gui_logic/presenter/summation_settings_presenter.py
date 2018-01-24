class SummationSettingsPresenter(object):
    def __init__(self, summation_settings, view, parent_view):
        self._summation_settings = summation_settings
        self.view = view
        self.parent_view = parent_view
        self._connect_to_view(view)
        self._refresh()

    def _connect_to_view(self, view):
        view.binningTypeChanged.connect(self._handle_binning_type_changed)
        view.preserveEventsChanged.connect(self._handle_overlay_event_workspaces_changed)
        view.additionalTimeShiftsChanged.connect(self._handle_additional_time_shifts_changed)
        view.binSettingsChanged.connect(self._handle_bin_settings_changed)

    def _handle_additional_time_shifts_changed(self):
        self._save_additional_time_shifts_to_model()

    def _handle_bin_settings_changed(self):
        self._save_bin_settings_to_model()

    def _handle_binning_type_changed(self, type_of_binning):
        self._switch_binning_type(type_of_binning)
        self._refresh()

    def _switch_binning_type(self, type_of_binning):
        self._summation_settings.set_histogram_binning_type(type_of_binning)

    def _handle_overlay_event_workspaces_changed(self, is_enabled):
        assert self._summation_settings.has_overlay_event_workspaces()
        if is_enabled:
            self._summation_settings.enable_overlay_event_workspaces()
        else:
            self._summation_settings.disable_overlay_event_workspaces()
        self._refresh()

    def _save_bin_settings_to_model(self):
        if self._summation_settings.has_bin_settings():
            self._summation_settings.bin_settings = self.view.bin_settings()

    def _save_additional_time_shifts_to_model(self):
        if self._summation_settings.has_additional_time_shifts():
            self._summation_settings.additional_time_shifts = self.view.additional_time_shifts()

    def settings(self):
        return self._summation_settings

    def _refresh(self):
        self.view.draw_settings(self._summation_settings)
