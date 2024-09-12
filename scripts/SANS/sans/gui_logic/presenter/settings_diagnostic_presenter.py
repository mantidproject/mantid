# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""The settings diagnostic tab which visualizes the SANS state object."""

import json
import os

from ui.sans_isis.settings_diagnostic_tab import SettingsDiagnosticTab

from mantid.kernel import Logger
from mantid import UsageService
from mantid.kernel import FeatureType
from sans.gui_logic.gui_common import JSON_SUFFIX
from sans.state.AllStates import AllStates
from sans.state.Serializer import Serializer


class SettingsDiagnosticPresenter(object):
    class ConcreteSettingsDiagnosticTabListener(SettingsDiagnosticTab.SettingsDiagnosticTabListener):
        def __init__(self, presenter):
            super(SettingsDiagnosticPresenter.ConcreteSettingsDiagnosticTabListener, self).__init__()
            self._presenter = presenter

        def on_row_changed(self):
            self._presenter.on_row_changed()

        def on_update_rows(self):
            self._presenter.on_update_rows()

        def on_collapse(self):
            self._presenter.on_collapse()

        def on_expand(self):
            self._presenter.on_expand()

        def on_save_state_to_file(self):
            self._presenter.on_save_state()

    def __init__(self, parent_presenter):
        self._view = None
        self._parent_presenter = parent_presenter
        # Logger
        self.gui_logger = Logger("SANS GUI LOGGER")

    def on_collapse(self):
        self._view.collapse()

    def on_expand(self):
        self._view.expand()

    def on_row_changed(self):
        try:
            row_index = self._view.get_current_row()
            state = self.get_state(row_index)
            if state:
                self.display_state_diagnostic_tree(state)
        except RuntimeError as e:
            self.gui_logger.error(str(e))
            self._parent_presenter.display_warning_box("Warning", "Unable to find files.", str(e))

    def on_update_rows(self):
        """
        Update the row selection in the combobox
        """
        current_row_index = self._view.get_current_row()
        valid_row_indices = self._parent_presenter.get_row_indices()

        new_row_index = -1
        if current_row_index in valid_row_indices:
            new_row_index = current_row_index
        elif len(valid_row_indices) > 0:
            new_row_index = valid_row_indices[0]

        self._view.update_rows(valid_row_indices)

        if new_row_index != -1:
            self.set_row(new_row_index)
            self.on_row_changed()

    def set_row(self, index):
        self._view.set_row(index)

    def set_view(self, view):
        if view:
            self._view = view

            # Set up row selection listener
            listener = SettingsDiagnosticPresenter.ConcreteSettingsDiagnosticTabListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._set_default_gui()

    def _set_default_gui(self):
        self._view.update_rows([])
        self.display_state_diagnostic_tree(state=None)

    def get_state(self, index) -> AllStates:
        return self._parent_presenter.get_state_for_row(index)

    def display_state_diagnostic_tree(self, state):
        # Convert to dict before passing the state to the view
        dict_vals = None

        if state:
            state = Serializer.to_json(state)
            dict_vals = json.loads(state)  # We intentionally do not use serializer to get a dict type back

        self._view.set_tree(dict_vals)

    def on_save_state(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Settings Diagnostics - Save JSON"], False)
        # Get the save location
        save_location = self._view.get_save_location()
        # Check if it exists
        path_dir = os.path.dirname(save_location)
        if not path_dir:
            self.gui_logger.warning(
                "The provided save location for the SANS state does not seem to exist. " "Please provide a validate path"
            )
            return

        file_name, _ = os.path.splitext(save_location)
        full_file_path = file_name + JSON_SUFFIX

        row_index = self._view.get_current_row()
        state = self.get_state(row_index)
        Serializer.save_file(state, full_file_path)
        self.gui_logger.information("The state for row {} has been saved to: {} ".format(row_index, full_file_path))

        # Update the file name in the UI
        self._view.set_save_location(full_file_path)
