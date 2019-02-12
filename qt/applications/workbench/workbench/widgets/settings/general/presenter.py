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

from mantid.kernel import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.general.view import GeneralSettingsView


class GeneralSettings(object):
    def __init__(self, parent, view=None):
        self.view = view if view else GeneralSettingsView(parent, self)
        self.setup_instrument_tab()

        self.setup_project_settings()

    def setup_instrument_tab(self):
        facilities = ConfigService.Instance().getFacilityNames()
        self.view.facility.addItems(facilities)
        default_facility = ConfigService.getFacility().name()
        self.view.facility.setCurrentIndex(self.view.facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

    def action_facility_changed(self, new_facility):
        ConfigService.setFacility(new_facility)
        self.view.instrument.clear()
        self.view.instrument.addItems(
            [str(instr) for instr in ConfigService.Instance().getFacility(new_facility).instruments()])

    def setup_project_settings(self):
        self.view.prompt_to_save_on_close.stateChanged.connect(self.action_prompt_save_on_close)
        self.view.prompt_to_save_editor_modified.stateChanged.connect(self.action_prompt_save_editor_modified)

    def action_prompt_save_on_close(self, state):
        CONF.prompt_save_on_close = state

    def action_prompt_save_editor_modified(self, state):
        CONF.prompt_save_editor_modified = state
