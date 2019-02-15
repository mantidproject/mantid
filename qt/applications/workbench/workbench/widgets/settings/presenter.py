# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt

from mantid import ConfigService
from workbench.widgets.settings.general.presenter import GeneralSettings
from workbench.widgets.settings.view import SettingsView


class SettingsPresenter(object):
    ASK_BEFORE_CLOSE_TITLE = "Confirm exit"
    ASK_BEFORE_CLOSE_MESSAGE = "Are you sure you want to exit without applying the settings?"

    def __init__(self, parent, view=None):
        self.view = view if view else SettingsView(parent, self)
        self.general_settings = GeneralSettings(parent)
        self.parent = parent

        self.current = self.general_settings.view
        self.view.container.addWidget(self.general_settings.view)

        self.view.save_settings_button.clicked.connect(self.action_save_settings_button)

        self.ask_before_close = False

    def show(self, modal=True):
        if modal:
            self.view.setWindowModality(Qt.WindowModal)

        self.view.show()
        self.current.show()

    def hide(self):
        self.view.hide()

    def action_current_row_changed(self, new_pos):
        self.current.hide()
        # if 0 == new_pos:
        new_view = self.general_settings.view
        # else:  # 1 == new_pos
        #     new_view = self.plots_settings_view
        # elif 2 == new_pos:
        #     new_view = self.view.plots_settings_view
        #     pass

        if self.current != new_view:
            self.view.container.replaceWidget(self.current, new_view)
            self.current = new_view

        self.current.show()

    def action_save_settings_button(self):
        if not self.ask_before_close or self.view.ask_before_close():
            ConfigService.saveConfig(ConfigService.getUserFilename())
            self.view.close()

    def refresh_workspaces(self):
        self.parent.workspacewidget.refresh_workspaces()
