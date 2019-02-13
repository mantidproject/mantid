# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import absolute_import, unicode_literals

from mantid.kernel import ConfigService, config
from workbench.config import CONF
from workbench.widgets.settings.general.view import GeneralSettingsView


class GeneralSettings(object):
    def __init__(self, parent, view=None):
        self.view = view if view else GeneralSettingsView(parent, self)

        self.setup_facilities()
        self.setup_confirmations()

        self.load_current_setting_values()

    def setup_facilities(self):
        facilities = ConfigService.Instance().getFacilityNames()
        self.view.facility.addItems(facilities)

        default_facility = ConfigService.getFacility().name()
        self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

        default_instrument = ConfigService.getInstrument().name()
        self.view.instrument.setCurrentIndex(self.view.instrument.findText(default_instrument))
        self.action_instrument_changed(default_instrument)
        self.view.instrument.currentTextChanged.connect(self.action_instrument_changed)

        self.view.project_recovery_enabled.stateChanged.connect(self.action_project_recovery_enabled)
        self.view.time_between_recovery.valueChanged.connect(self.action_time_between_recovery)
        self.view.total_number_checkpoints.valueChanged.connect(self.action_total_number_checkpoints)

    def action_facility_changed(self, new_facility):
        ConfigService.setFacility(new_facility)
        self.view.instrument.clear()
        self.view.instrument.addItems(
            [instr.name() for instr in ConfigService.Instance().getFacility(new_facility).instruments()])

    def setup_confirmations(self):
        self.view.prompt_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self.view.prompt_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)

    def action_prompt_save_on_close(self, state):
        CONF.set('project', 'prompt_save_on_close', bool(state))

    def action_prompt_save_editor_modified(self, state):
        CONF.set('project', 'prompt_save_editor_modified', bool(state))

    def load_current_setting_values(self):
        self.view.prompt_save_on_close.setChecked(CONF.get('project', 'prompt_save_on_close'))
        self.view.prompt_save_editor_modified.setChecked(CONF.get('project', 'prompt_save_editor_modified'))

        self.view.project_recovery_enabled.setChecked(CONF.get('project/recovery', 'enabled'))
        self.view.time_between_recovery.setValue(int(CONF.get('project/recovery', 'time_between_recovery_checkpoints')))
        self.view.total_number_checkpoints.setValue(int(CONF.get('project/recovery', 'total_number_of_checkpoints')))

    def action_project_recovery_enabled(self, state):
        CONF.set('project/recovery', 'enabled', bool(state))

    def action_time_between_recovery(self, value):
        CONF.set('project/recovery', 'time_between_recovery_checkpoints', int(value))

    def action_total_number_checkpoints(self, value):
        CONF.set('project/recovery', 'total_number_of_checkpoints', int(value))

    def action_instrument_changed(self, new_instrument):
        print("Changing def instrument to", new_instrument)
        ConfigService.setString("default.instrument", new_instrument)
