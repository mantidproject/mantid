# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QFileDialog

from mantid import ConfigService
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import ensure_widget_is_on_screen
from workbench.widgets.settings.categories.presenter import CategoriesSettings
from workbench.widgets.settings.categories.categories_settings_model import CategoryProperties, CategoriesSettingsModel
from workbench.widgets.settings.fitting.presenter import FittingSettings
from workbench.widgets.settings.fitting.fitting_settings_model import FittingProperties, FittingSettingsModel
from workbench.widgets.settings.general.presenter import GeneralSettings
from workbench.widgets.settings.general.general_settings_model import GeneralProperties, GeneralUserConfigProperties, GeneralSettingsModel
from workbench.widgets.settings.plots.presenter import PlotSettings
from workbench.widgets.settings.plots.model import PlotProperties, PlotsSettingsModel
from workbench.widgets.settings.view import SettingsView
from workbench.widgets.settings.model import SettingsModel


class SettingsPresenter(object):
    SETTINGS_TABS = {
        "general_settings": "General",
        "categories_settings": "Categories",
        "plot_settings": "Plots",
        "fitting_settings": "Fitting",
    }

    def __init__(
        self, parent, view=None, model=None, general_settings=None, categories_settings=None, plot_settings=None, fitting_settings=None
    ):
        categories_model = CategoriesSettingsModel()
        fitting_model = FittingSettingsModel()
        general_model = GeneralSettingsModel()
        plots_model = PlotsSettingsModel()
        self.view = view if view else SettingsView(parent, self)
        self.model = model if model else SettingsModel([categories_model, fitting_model, general_model, plots_model])
        self.general_settings = (
            general_settings if general_settings else GeneralSettings(parent, settings_presenter=self, model=general_model)
        )
        self.categories_settings = categories_settings if categories_settings else CategoriesSettings(parent, model=categories_model)
        self.plot_settings = plot_settings if plot_settings else PlotSettings(parent, model=plots_model)
        self.fitting_settings = fitting_settings if fitting_settings else FittingSettings(parent, model=fitting_model)
        self.parent = parent
        self.all_properties = []
        for properties in [CategoryProperties, FittingProperties, GeneralProperties, GeneralUserConfigProperties, PlotProperties]:
            self.all_properties += [prop.value for prop in properties]

        self.view.sections.addItems(list(self.SETTINGS_TABS.values()))
        self.current = self.general_settings.get_view()
        self.view.sections.setCurrentRow(0)
        self.view.container.addWidget(self.general_settings.get_view())
        self.view.container.addWidget(self.categories_settings.get_view())
        self.view.container.addWidget(self.plot_settings.get_view())
        self.view.container.addWidget(self.fitting_settings.get_view())

        self.plot_settings.subscribe_parent_presenter(self)
        self.categories_settings.subscribe_parent_presenter(self)
        self.general_settings.subscribe_parent_presenter(self)
        self.fitting_settings.subscribe_parent_presenter(self)

        self.view.okay_button.clicked.connect(self.action_okay_button_pushed)
        self.view.apply_button.clicked.connect(self.action_apply_button_pushed)

        self.view.save_file_button.clicked.connect(self.action_save_settings_to_file)
        self.view.load_file_button.clicked.connect(self.action_load_settings_from_file)
        self.view.help_button.clicked.connect(self.action_open_help_window)

        self.model.register_property_which_needs_a_restart(str(GeneralUserConfigProperties.FONT.value))
        self.model.register_property_which_needs_a_restart(str(GeneralUserConfigProperties.PROMPT_ON_DELETING_WORKSPACE.value))

        self.update_apply_button()

    def show(self, modal=True):
        if modal:
            self.view.setWindowModality(Qt.WindowModal)

        self.view.show()
        ensure_widget_is_on_screen(self.view)
        self.current.show()

    def hide(self):
        self.view.hide()

    def action_okay_button_pushed(self):
        changes_that_need_restart = self.model.potential_changes_that_need_a_restart()
        self.model.apply_all_settings()
        self.view.notify_changes_need_restart(changes_that_need_restart)
        self.view_closing()

    def action_apply_button_pushed(self):
        changes_that_need_restart = self.model.potential_changes_that_need_a_restart()
        self.model.apply_all_settings()
        self.parent.config_updated()
        self.view.notify_changes_need_restart(changes_that_need_restart)
        self.update_apply_button()

    def changes_updated(self, unsaved_changes: bool):
        self.view.apply_button.setEnabled(unsaved_changes)

    def update_apply_button(self):
        unsaved_changes = self.model.unsaved_changes() != {}
        self.view.apply_button.setEnabled(unsaved_changes)

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
                new_view = getattr(self, settings).get_view()

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
        unsaved_changes = self.model.unsaved_changes()
        if not unsaved_changes or self.view.ask_before_close(unsaved_changes):
            ConfigService.saveConfig(ConfigService.getUserFilename())
            self.parent.config_updated()
            self.view.close()
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

    def _update_all_properties(self):
        for settings in self.SETTINGS_TABS.keys():
            getattr(self, settings).update_properties()
