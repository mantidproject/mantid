# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from mantidqt.widgets import instrumentselector
from workbench.config import SAVE_STATE_VERSION
from workbench.widgets.settings.general.view import GeneralSettingsView
from workbench.widgets.settings.general.general_settings_model import GeneralSettingsModel
from workbench.widgets.settings.view_utilities.settings_view_utilities import filter_out_mousewheel_events_from_combo_or_spin_box

from qtpy.QtCore import Qt
from qtpy.QtGui import QFontDatabase


class GeneralSettings(object):
    """
    Presenter of the General settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the General settings, their events when changed should
    be handled here.
    """

    WINDOW_BEHAVIOUR = ["On top", "Floating"]

    def __init__(self, parent, view=None, settings_presenter=None, model=None):
        self.view = view if view else GeneralSettingsView(parent, self)
        self.parent = parent
        self.settings_presenter = settings_presenter
        self.model = model if model else GeneralSettingsModel()
        self.load_current_setting_values()

        self.setup_facilities_group()
        self.setup_checkbox_signals()
        self.setup_general_group()
        self.setup_layout_options()
        self.setup_confirmations()

    def setup_facilities_group(self):
        facilities = sorted(self.model.get_facility_names())
        if not facilities:
            return
        self.view.facility.addItems(facilities)
        self.view.instrument = instrumentselector.InstrumentSelector()
        self.view.horizontalLayout_4.replaceWidget(self.view.instrument_dummy, self.view.instrument)
        self.view.instrument_dummy.setVisible(False)

        try:
            default_facility = self.model.get_facility()
        except RuntimeError:
            default_facility = facilities[0]
        self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

        try:
            default_instrument = self.model.get_instrument()
        except RuntimeError:
            default_instrument = self.view.instrument.itemText(0)
        self.action_instrument_changed(default_instrument)
        self.view.instrument.currentTextChanged.connect(self.action_instrument_changed)

        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.facility)
        self.view.instrument.setFocusPolicy(Qt.StrongFocus)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.instrument)

    def update_facilities_group(self):
        default_facility = self.model.get_facility()
        if not self.view.facility.findText(default_facility) == -1:
            self.view.instrument.blockSignals(True)
            self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
            self.action_facility_changed(default_facility)
            self.view.instrument.blockSignals(False)

    def setup_general_group(self):
        self.view.window_behaviour.addItems(self.WINDOW_BEHAVIOUR)
        window_behaviour = self.model.get_window_behaviour()
        if window_behaviour in self.WINDOW_BEHAVIOUR:
            self.view.window_behaviour.setCurrentIndex(self.view.window_behaviour.findText(window_behaviour))

        self.view.window_behaviour.currentTextChanged.connect(self.action_window_behaviour_changed)
        self.view.main_font.clicked.connect(self.action_main_font_button_clicked)
        self.view.completion_enabled.stateChanged.connect(self.action_completion_enabled_modified)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.window_behaviour)

    def action_main_font_button_clicked(self):
        font = None
        font_string = self.model.get_font()
        if font_string is not None:
            font_string = self.model.get_font().split(",")
            if len(font_string) > 2:
                font = QFontDatabase().font(font_string[0], font_string[-1], int(font_string[1]))
        font_dialog = self.view.create_font_dialog(self.view, font)
        font_dialog.fontSelected.connect(self.action_font_selected)

    def action_font_selected(self, font):
        font_string = self.model.get_font()
        if font_string is None:
            font_string = ""
        if font_string != font.toString():
            self.model.set_font(font.toString())
            if self.settings_presenter is not None:
                self.settings_presenter.register_change_needs_restart("Main Font Selection")

    def action_window_behaviour_changed(self, text):
        self.model.set_window_behaviour(text)

    def action_completion_enabled_modified(self, state):
        self.model.set_completion_enabled(bool(state))

    def setup_checkbox_signals(self):
        self.view.show_invisible_workspaces.stateChanged.connect(self.action_show_invisible_workspaces)
        self.view.project_recovery_enabled.stateChanged.connect(self.action_project_recovery_enabled)
        self.view.time_between_recovery.valueChanged.connect(self.action_time_between_recovery)
        self.view.total_number_checkpoints.valueChanged.connect(self.action_total_number_checkpoints)
        self.view.crystallography_convention.stateChanged.connect(self.action_crystallography_convention)
        self.view.use_open_gl.stateChanged.connect(self.action_use_open_gl)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.time_between_recovery)
        filter_out_mousewheel_events_from_combo_or_spin_box(self.view.total_number_checkpoints)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        current_value = self.model.get_facility()
        if new_facility != current_value:
            self.model.set_facility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self.view.instrument.setFacility(new_facility)
        if new_facility != current_value:
            self.view.instrument.setCurrentIndex(0)

    def setup_confirmations(self):
        self.view.prompt_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self.view.prompt_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)
        self.view.prompt_deleting_workspaces.stateChanged.connect(self.action_prompt_deleting_workspace)
        self.view.use_notifications.stateChanged.connect(self.action_use_notifications_modified)

    def action_prompt_save_on_close(self, state):
        self.model.set_prompt_save_on_close(bool(state))

    def action_prompt_save_editor_modified(self, state):
        self.model.set_prompt_on_save_editor_modified(bool(state))

    def action_prompt_deleting_workspace(self, state):
        self.model.set_prompt_on_deleting_workspace(bool(state))
        if self.settings_presenter is not None:
            self.settings_presenter.register_change_needs_restart("Prompt when deleting workspaces")

    def action_use_notifications_modified(self, state):
        self.model.set_use_notifications("On" if bool(state) else "Off")

    def load_current_setting_values(self):
        self.view.prompt_save_on_close.setChecked(self.model.get_prompt_save_on_close())
        self.view.prompt_save_editor_modified.setChecked(self.model.get_prompt_on_save_editor_modified())
        self.view.prompt_deleting_workspaces.setChecked(self.model.get_prompt_on_deleting_workspace())

        # compare lower-case, because MantidPlot will save it as lower case,
        # but Python will have the bool's first letter capitalised
        pr_enabled = "true" == self.model.get_project_recovery_enabled().lower()
        pr_time_between_recovery = int(self.model.get_project_recovery_time_between_recoveries())
        pr_number_checkpoints = int(self.model.get_project_recovery_number_of_checkpoints())
        use_notifications_setting = "on" == self.model.get_use_notifications().lower()
        crystallography_convention = "Crystallography" == self.model.get_crystallography_convention()
        use_open_gl = "on" == self.model.get_use_opengl().lower()
        invisible_workspaces = "true" == self.model.get_show_invisible_workspaces().lower()
        completion_enabled = self.model.get_completion_enabled()

        self.view.project_recovery_enabled.setChecked(pr_enabled)
        self.view.time_between_recovery.setValue(pr_time_between_recovery)
        self.view.total_number_checkpoints.setValue(pr_number_checkpoints)
        self.view.use_notifications.setChecked(use_notifications_setting)
        self.view.crystallography_convention.setChecked(crystallography_convention)
        self.view.use_open_gl.setChecked(use_open_gl)
        self.view.show_invisible_workspaces.setChecked(invisible_workspaces)
        self.view.completion_enabled.setChecked(completion_enabled)

    def action_project_recovery_enabled(self, state):
        self.model.set_project_recovery_enabled(str(bool(state)))

    def action_time_between_recovery(self, value):
        self.model.set_project_recovery_time_between_recoveries(str(value))

    def action_total_number_checkpoints(self, value):
        self.model.set_project_recovery_number_of_checkpoints(str(value))

    def action_crystallography_convention(self, state):
        self.model.set_crystallography_convention("Crystallography" if state == Qt.Checked else "Inelastic")

    def action_instrument_changed(self, new_instrument):
        current_value = self.model.get_instrument()
        if new_instrument != current_value:
            self.model.set_instrument(new_instrument)

    def action_show_invisible_workspaces(self, state):
        self.model.set_show_invisible_workspaces(str(bool(state)))

    def action_use_open_gl(self, state):
        self.model.set_use_opengl("On" if bool(state) else "Off")

    def setup_layout_options(self):
        self.fill_layout_display()
        self.view.save_layout.clicked.connect(self.save_layout)
        self.view.load_layout.clicked.connect(self.load_layout)
        self.view.delete_layout.clicked.connect(self.delete_layout)

    def fill_layout_display(self):
        self.view.layout_display.clear()
        layout_dict = self.get_layout_dict()
        layout_list = sorted(layout_dict.keys())
        for item in layout_list:
            self.view.layout_display.addItem(item)

    def get_layout_dict(self):
        try:
            layout_list = self.model.get_user_layout(get_potential_update=True)
        except (KeyError, TypeError):
            layout_list = {}
        return layout_list

    def save_layout(self):
        filename = self.view.new_layout_name.text()
        if filename != "":
            layout_dict = self.get_layout_dict()
            layout_dict[filename] = self.parent.saveState(SAVE_STATE_VERSION)
            self.model.set_user_layout(layout_dict)
            self.view.new_layout_name.clear()
            self.fill_layout_display()
            self.parent.populate_layout_menu()

    def load_layout(self):
        item = self.view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            self.parent.restoreState(layout_dict[layout], SAVE_STATE_VERSION)

    def delete_layout(self):
        item = self.view.layout_display.currentItem()
        if hasattr(item, "text"):
            layout = item.text()
            layout_dict = self.get_layout_dict()
            layout_dict.pop(layout, None)
            self.model.set_user_layout(layout_dict)
            self.fill_layout_display()
            self.parent.populate_layout_menu()

    def focus_layout_box(self):
        # scroll the settings to the layout box. High yMargin ensures the box is always at the top of the window.
        self.view.scrollArea.ensureWidgetVisible(self.view.new_layout_name, yMargin=1000)
        self.view.new_layout_name.setFocus()

    def update_properties(self):
        self.load_current_setting_values()
        self.update_facilities_group()
        self.fill_layout_display()
