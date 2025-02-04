# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.widgets import instrumentselector
from workbench.config import SAVE_STATE_VERSION
from workbench.widgets.settings.base_classes.config_settings_presenter import SettingsPresenterBase
from workbench.widgets.settings.general.view import GeneralSettingsView
from workbench.widgets.settings.general.general_settings_model import GeneralSettingsModel
from workbench.widgets.settings.view_utilities.settings_view_utilities import filter_out_mousewheel_events_from_combo_or_spin_box

from qtpy.QtCore import Qt
from qtpy.QtGui import QFontDatabase


class GeneralSettings(SettingsPresenterBase):
    """
    Presenter of the General settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the General settings, their events when changed should
    be handled here.
    """

    WINDOW_BEHAVIOUR = ["On top", "Floating"]

    def __init__(self, parent, model: GeneralSettingsModel, view=None, settings_presenter=None):
        super().__init__(parent, model)
        self._view = view if view else GeneralSettingsView(parent, self)
        self.settings_presenter = settings_presenter
        self.load_current_setting_values()

        self.setup_facilities_group()
        self.setup_checkbox_signals()
        self.setup_general_group()
        self.setup_layout_options()
        self.setup_confirmations()

    def get_view(self):
        return self._view

    def setup_facilities_group(self):
        facilities = sorted(self._model.get_facility_names())
        if not facilities:
            return
        self._view.facility.addItems(facilities)
        self._view.instrument = instrumentselector.InstrumentSelector()
        self._view.horizontalLayout_4.replaceWidget(self._view.instrument_dummy, self._view.instrument)
        self._view.instrument_dummy.setVisible(False)

        try:
            default_facility = self._model.get_facility()
        except RuntimeError:
            default_facility = facilities[0]
        self._view.facility.setCurrentIndex(self._view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self._view.facility.currentTextChanged.connect(self.action_facility_changed)

        try:
            default_instrument = self._model.get_instrument()
        except RuntimeError:
            default_instrument = self._view.instrument.itemText(0)
        self.action_instrument_changed(default_instrument)
        self._view.instrument.currentTextChanged.connect(self.action_instrument_changed)

        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.facility)
        self._view.instrument.setFocusPolicy(Qt.StrongFocus)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.instrument)

    def update_facilities_group(self):
        default_facility = self._model.get_facility()
        if not self._view.facility.findText(default_facility) == -1:
            self._view.instrument.blockSignals(True)
            self._view.facility.setCurrentIndex(self._view.facility.findText(default_facility))
            self.action_facility_changed(default_facility)
            self._view.instrument.blockSignals(False)

    def setup_general_group(self):
        self._view.window_behaviour.addItems(self.WINDOW_BEHAVIOUR)
        window_behaviour = self._model.get_window_behaviour()
        if window_behaviour in self.WINDOW_BEHAVIOUR:
            self._view.window_behaviour.setCurrentIndex(self._view.window_behaviour.findText(window_behaviour))

        self._view.window_behaviour.currentTextChanged.connect(self.action_window_behaviour_changed)
        self._view.main_font.clicked.connect(self.action_main_font_button_clicked)
        self._view.completion_enabled.stateChanged.connect(self.action_completion_enabled_modified)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.window_behaviour)

    def action_main_font_button_clicked(self):
        font = None
        font_string = self._model.get_font()
        if font_string is not None:
            font_string = self._model.get_font().split(",")
            if len(font_string) > 2:
                font = QFontDatabase().font(font_string[0], font_string[-1], int(font_string[1]))
        font_dialog = self._view.create_font_dialog(self._view, font)
        font_dialog.fontSelected.connect(self.action_font_selected)

    def action_font_selected(self, font):
        font_string = self._model.get_font()
        if font_string is None:
            font_string = ""
        if font_string != font.toString():
            self._model.set_font(font.toString())
        self.notify_changes()

    def action_window_behaviour_changed(self, text):
        self._model.set_window_behaviour(text)
        self.notify_changes()

    def action_completion_enabled_modified(self, state):
        self._model.set_completion_enabled(bool(state))
        self.notify_changes()

    def setup_checkbox_signals(self):
        self._view.show_invisible_workspaces.stateChanged.connect(self.action_show_invisible_workspaces)
        self._view.project_recovery_enabled.stateChanged.connect(self.action_project_recovery_enabled)
        self._view.time_between_recovery.valueChanged.connect(self.action_time_between_recovery)
        self._view.total_number_checkpoints.valueChanged.connect(self.action_total_number_checkpoints)
        self._view.crystallography_convention.stateChanged.connect(self.action_crystallography_convention)
        self._view.use_open_gl.stateChanged.connect(self.action_use_open_gl)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.time_between_recovery)
        filter_out_mousewheel_events_from_combo_or_spin_box(self._view.total_number_checkpoints)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        self._model.set_facility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self._view.instrument.setFacility(new_facility)
        self.action_instrument_changed(self._view.instrument.currentText())
        self.notify_changes()

    def setup_confirmations(self):
        self._view.prompt_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self._view.prompt_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)
        self._view.prompt_deleting_workspaces.stateChanged.connect(self.action_prompt_deleting_workspace)
        self._view.use_notifications.stateChanged.connect(self.action_use_notifications_modified)

    def action_prompt_save_on_close(self, state):
        self._model.set_prompt_save_on_close(bool(state))
        self.notify_changes()

    def action_prompt_save_editor_modified(self, state):
        self._model.set_prompt_on_save_editor_modified(bool(state))
        self.notify_changes()

    def action_prompt_deleting_workspace(self, state):
        self._model.set_prompt_on_deleting_workspace(bool(state))
        self.notify_changes()

    def action_use_notifications_modified(self, state):
        self._model.set_use_notifications("On" if bool(state) else "Off")
        self.notify_changes()

    def load_current_setting_values(self):
        self._view.prompt_save_on_close.setChecked(self._model.get_prompt_save_on_close())
        self._view.prompt_save_editor_modified.setChecked(self._model.get_prompt_on_save_editor_modified())
        self._view.prompt_deleting_workspaces.setChecked(self._model.get_prompt_on_deleting_workspace())

        # compare lower-case, because MantidPlot will save it as lower case,
        # but Python will have the bool's first letter capitalised
        pr_enabled = "true" == self._model.get_project_recovery_enabled().lower()
        pr_time_between_recovery = int(self._model.get_project_recovery_time_between_recoveries())
        pr_number_checkpoints = int(self._model.get_project_recovery_number_of_checkpoints())
        use_notifications_setting = "on" == self._model.get_use_notifications().lower()
        crystallography_convention = "Crystallography" == self._model.get_crystallography_convention()
        use_open_gl = "on" == self._model.get_use_opengl().lower()
        invisible_workspaces = "true" == self._model.get_show_invisible_workspaces().lower()
        completion_enabled = self._model.get_completion_enabled()

        self._view.project_recovery_enabled.setChecked(pr_enabled)
        self._view.time_between_recovery.setValue(pr_time_between_recovery)
        self._view.total_number_checkpoints.setValue(pr_number_checkpoints)
        self._view.use_notifications.setChecked(use_notifications_setting)
        self._view.crystallography_convention.setChecked(crystallography_convention)
        self._view.use_open_gl.setChecked(use_open_gl)
        self._view.show_invisible_workspaces.setChecked(invisible_workspaces)
        self._view.completion_enabled.setChecked(completion_enabled)

    def action_project_recovery_enabled(self, state):
        self._model.set_project_recovery_enabled(str(bool(state)))
        self.notify_changes()

    def action_time_between_recovery(self, value):
        self._model.set_project_recovery_time_between_recoveries(str(value))
        self.notify_changes()

    def action_total_number_checkpoints(self, value):
        self._model.set_project_recovery_number_of_checkpoints(str(value))
        self.notify_changes()

    def action_crystallography_convention(self, state):
        self._model.set_crystallography_convention("Crystallography" if state == Qt.Checked else "Inelastic")
        self.notify_changes()

    def action_instrument_changed(self, new_instrument):
        self._model.set_instrument(new_instrument)
        self.notify_changes()

    def action_show_invisible_workspaces(self, state):
        self._model.set_show_invisible_workspaces("1" if state == Qt.Checked else "0")
        self.notify_changes()

    def action_use_open_gl(self, state):
        self._model.set_use_opengl("On" if bool(state) else "Off")
        self.notify_changes()

    def setup_layout_options(self):
        self.fill_layout_display()
        self._view.save_layout.clicked.connect(self.save_layout)
        self._view.load_layout.clicked.connect(self.load_layout)
        self._view.delete_layout.clicked.connect(self.delete_layout)

    def fill_layout_display(self):
        self._view.layout_display.clear()
        layout_dict = self.get_layout_dict()
        layout_list = sorted(layout_dict.keys())
        for item in layout_list:
            self._view.layout_display.addItem(item)

    def get_layout_dict(self):
        try:
            layout_list = self._model.get_user_layout(get_potential_update=True)
        except (KeyError, TypeError):
            layout_list = {}
        return layout_list

    def save_layout(self):
        filename = self._view.new_layout_name.text()
        if filename != "":
            layout_dict = self.get_layout_dict()
            layout_dict[filename] = self.parent().saveState(SAVE_STATE_VERSION)
            self._model.set_user_layout(layout_dict)
            self.notify_changes()
            self._view.new_layout_name.clear()
            self.fill_layout_display()
            self.parent().populate_layout_menu()

    def load_layout(self):
        item = self._view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            self.parent().restoreState(layout_dict[layout], SAVE_STATE_VERSION)

    def delete_layout(self):
        item = self._view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            layout_dict.pop(layout, None)
            self._model.set_user_layout(layout_dict)
            self.notify_changes()
            self.fill_layout_display()
            self.parent().populate_layout_menu()

    def focus_layout_box(self):
        # scroll the settings to the layout box. High yMargin ensures the box is always at the top of the window.
        self._view.scrollArea.ensureWidgetVisible(self._view.new_layout_name, yMargin=1000)
        self._view.new_layout_name.setFocus()

    def update_properties(self):
        self.load_current_setting_values()
        self.update_facilities_group()
        self.fill_layout_display()
