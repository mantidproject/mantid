# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QFileDialog

from mantid import ConfigService
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import ensure_widget_is_on_screen
from workbench.widgets.settings.categories.presenter import CategoriesSettings, CategoryProperties
from workbench.widgets.settings.fitting.presenter import FittingSettings, FittingProperties
from workbench.widgets.settings.general.presenter import GeneralSettings, GeneralProperties
from workbench.widgets.settings.plots.presenter import PlotSettings, PlotProperties
from workbench.widgets.settings.view import SettingsView
from workbench.widgets.settings.model import SettingsModel


class SettingsPresenter(object):
    ASK_BEFORE_CLOSE_TITLE = "Confirm exit"
    ASK_BEFORE_CLOSE_MESSAGE = "Are you sure you want to exit without applying the settings?"
    CHANGES_NEED_RESTART_TITLE = "Some changes require restart"
    CHANGES_NEED_RESTART_MESSAGE = "The following changes will be applied when the workbench is restarted:\n\n"

    SETTINGS_TABS = {
        "general_settings": "General",
        "categories_settings": "Categories",
        "plot_settings": "Plots",
        "fitting_settings": "Fitting",
    }

    def __init__(
        self, parent, view=None, model=None, general_settings=None, categories_settings=None, plot_settings=None, fitting_settings=None
    ):
        self.view = view if view else SettingsView(parent, self)
        self.model = model if model else SettingsModel()
        self.general_settings = general_settings if general_settings else GeneralSettings(parent, None, self)
        self.categories_settings = categories_settings if categories_settings else CategoriesSettings(parent)
        self.plot_settings = plot_settings if plot_settings else PlotSettings(parent)
        self.fitting_settings = fitting_settings if fitting_settings else FittingSettings(parent)
        self.parent = parent
        self.all_properties = []
        for properties in [CategoryProperties, FittingProperties, GeneralProperties, PlotProperties]:
            self.all_properties += [prop.value for prop in properties]

        self.view.sections.addItems(list(self.SETTINGS_TABS.values()))
        self.current = self.general_settings.view
        self.view.container.addWidget(self.general_settings.view)
        self.view.container.addWidget(self.categories_settings.view)
        self.view.container.addWidget(self.plot_settings.view)
        self.view.container.addWidget(self.fitting_settings.view)

        self.view.save_file_button.clicked.connect(self.action_save_settings_to_file)
        self.view.load_file_button.clicked.connect(self.action_load_settings_from_file)
        self.view.help_button.clicked.connect(self.action_open_help_window)
        self.ask_before_close = False

        self.changes_that_need_restart = []

    def show(self, modal=True):
        if modal:
            self.view.setWindowModality(Qt.WindowModal)

        self.view.show()
        ensure_widget_is_on_screen(self.view)
        self.current.show()

    def hide(self):
        self.view.hide()

    def action_section_changed(self, new_section_pos):
        """
        Selects the widget shown in the settings based on which section position is clicked.

        :param new_section_pos: The position of the section in the list widget
        """
        self.current.hide()
        new_view = self.current

        section_name = self.view.sections.item(new_section_pos).text()
        for settings, name in self.SETTINGS_TABS.items():
            if name == section_name:
                new_view = getattr(self, settings).view

        if self.current != new_view:
            self.view.container.replaceWidget(self.current, new_view)
            self.current = new_view

        self.current.show()

    def action_open_help_window(self):
        InterfaceManager().showHelpPage("qthelp://org.mantidproject/doc/workbench/settings.html")

    def view_closing(self):
        """
        Saves the mantid settings and updates updates the parent
        """
        if not self.ask_before_close or self.view.ask_before_close():
            ConfigService.saveConfig(ConfigService.getUserFilename())
            self.parent.config_updated()
            self.view.close()
            if self.changes_that_need_restart:
                self.view.notify_changes_need_restart(self.changes_that_need_restart)
            return True
        else:
            # try to stop the close action
            return False

    def action_save_settings_to_file(self):
        ConfigService.saveConfig(ConfigService.getUserFilename())
        filepath = self.view.get_properties_filename(accept_mode=QFileDialog.AcceptSave, file_mode=QFileDialog.AnyFile)
        if filepath:
            self.model.save_settings_to_file(filepath, self.all_properties)

    def action_load_settings_from_file(self):
        filepath = self.view.get_properties_filename(accept_mode=QFileDialog.AcceptOpen, file_mode=QFileDialog.ExistingFile)
        if filepath:
            self.model.load_settings_from_file(filepath, self.all_properties)
            self._update_all_properties()

    def register_change_needs_restart(self, change_description):
        self.changes_that_need_restart.append(change_description)

    def _update_all_properties(self):
        for settings in self.SETTINGS_TABS.keys():
            getattr(self, settings).update_properties()
