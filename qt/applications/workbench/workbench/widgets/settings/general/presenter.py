# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from __future__ import absolute_import, unicode_literals

from mantid.kernel import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.general.view import GeneralSettingsView


class GeneralSettings(object):
    """
    Presenter of the General settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the General settings, their events when changed should
    be handled here.
    """

    INSTRUMENT = "default.instrument"
    SHOW_INVISIBLE_WORKSPACES = "MantidOptions.InvisibleWorkspaces"
    PR_NUMBER_OF_CHECKPOINTS = "projectRecovery.numberOfCheckpoints"
    PR_TIME_BETWEEN_RECOVERY = "projectRecovery.secondsBetween"
    PR_RECOVERY_ENABLED = "projectRecovery.enabled"
    PROMPT_SAVE_EDITOR_MODIFIED = 'project/prompt_save_editor_modified'
    PROMPT_SAVE_ON_CLOSE = 'project/prompt_save_on_close'

    def __init__(self, parent, view=None):
        self.view = view if view else GeneralSettingsView(parent, self)
        self.load_current_setting_values()

        self.setup_facilities_group()
        self.setup_checkbox_signals()

        self.setup_confirmations()

    def setup_facilities_group(self):
        facilities = ConfigService.getFacilityNames()
        self.view.facility.addItems(facilities)

        default_facility = ConfigService.getFacility().name()
        self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

        default_instrument = ConfigService.getInstrument().name()
        self.view.instrument.setCurrentIndex(self.view.instrument.findText(default_instrument))
        self.action_instrument_changed(default_instrument)
        self.view.instrument.currentTextChanged.connect(self.action_instrument_changed)

    def setup_checkbox_signals(self):
        self.view.show_invisible_workspaces.setChecked(
            "true" == ConfigService.getString(self.SHOW_INVISIBLE_WORKSPACES).lower())

        self.view.show_invisible_workspaces.stateChanged.connect(self.action_show_invisible_workspaces)
        self.view.project_recovery_enabled.stateChanged.connect(self.action_project_recovery_enabled)
        self.view.time_between_recovery.valueChanged.connect(self.action_time_between_recovery)
        self.view.total_number_checkpoints.valueChanged.connect(self.action_total_number_checkpoints)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        ConfigService.setFacility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self.view.instrument.clear()
        self.view.instrument.addItems(
            [instr.name() for instr in ConfigService.getFacility(new_facility).instruments()])

    def setup_confirmations(self):
        self.view.prompt_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self.view.prompt_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)

    def action_prompt_save_on_close(self, state):
        CONF.set(self.PROMPT_SAVE_ON_CLOSE, bool(state))

    def action_prompt_save_editor_modified(self, state):
        CONF.set(self.PROMPT_SAVE_EDITOR_MODIFIED, bool(state))

    def load_current_setting_values(self):
        self.view.prompt_save_on_close.setChecked(CONF.get(self.PROMPT_SAVE_ON_CLOSE))
        self.view.prompt_save_editor_modified.setChecked(CONF.get(self.PROMPT_SAVE_EDITOR_MODIFIED))

        # compare lower-case, because MantidPlot will save it as lower case,
        # but Python will have the bool's first letter capitalised
        pr_enabled = ("true" == ConfigService.getString(self.PR_RECOVERY_ENABLED).lower())
        pr_time_between_recovery = int(ConfigService.getString(self.PR_TIME_BETWEEN_RECOVERY))
        pr_number_checkpoints = int(ConfigService.getString(self.PR_NUMBER_OF_CHECKPOINTS))

        self.view.project_recovery_enabled.setChecked(pr_enabled)
        self.view.time_between_recovery.setValue(pr_time_between_recovery)
        self.view.total_number_checkpoints.setValue(pr_number_checkpoints)

    def action_project_recovery_enabled(self, state):
        ConfigService.setString(self.PR_RECOVERY_ENABLED, str(bool(state)))

    def action_time_between_recovery(self, value):
        ConfigService.setString(self.PR_TIME_BETWEEN_RECOVERY, str(value))

    def action_total_number_checkpoints(self, value):
        ConfigService.setString(self.PR_NUMBER_OF_CHECKPOINTS, str(value))

    def action_instrument_changed(self, new_instrument):
        ConfigService.setString(self.INSTRUMENT, new_instrument)

    def action_show_invisible_workspaces(self, state):
        ConfigService.setString(self.SHOW_INVISIBLE_WORKSPACES, str(bool(state)))
