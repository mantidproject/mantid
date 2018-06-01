""" The settings diagnostic tab which visualizes the SANS state object. """
from __future__ import (absolute_import, division, print_function)


import os
import json

from mantid.kernel import Logger
from ui.sans_isis.settings_diagnostic_tab import SettingsDiagnosticTab
from sans.gui_logic.gui_common import JSON_SUFFIX
from ui.sans_isis.work_handler import WorkHandler
from sans.gui_logic.models.create_state import create_state_for_row


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

    class CreateStateListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(SettingsDiagnosticPresenter.CreateStateListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_row_changed_finished(result)

        def on_processing_error(self, error):
            self._presenter.on_row_change_error(error)

    def __init__(self, parent_presenter):
        super(SettingsDiagnosticPresenter, self).__init__()
        self._view = None
        self._parent_presenter = parent_presenter
        # Logger
        self.gui_logger = Logger("SANS GUI LOGGER")
        self._work_handler = WorkHandler()

    def on_collapse(self):
        self._view.collapse()

    def on_expand(self):
        self._view.expand()

    def on_row_changed(self):
        self._view.set_processing(True)
        row_index = self._view.get_current_row()
        state_model, table_model, number_of_rows, instrument =\
            self._parent_presenter.get_state_model_table_workspace_number_of_rows_instrument()
        facility = self._parent_presenter._facility
        listener = SettingsDiagnosticPresenter.CreateStateListener(self)
        self._work_handler.process(listener, self.get_state, row_index, state_model, table_model, number_of_rows, instrument, facility)

    def on_row_changed_finished(self, state):
        if state:
            self.display_state_diagnostic_tree(state)
        self._view.set_processing(False)

    def on_row_change_error(self, error):
        self._view.set_processing(False)
        self.gui_logger.error(error[1].message)
        self._parent_presenter.display_warning_box('Warning', 'Unable to find files.', error[1].message)

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

    def get_state(self, index, state_model, table_model, number_of_rows, instrument, facility):
        return create_state_for_row(state_model, table_model, number_of_rows, instrument, facility, index, file_lookup=True)

    def display_state_diagnostic_tree(self, state):
        # Convert to dict before passing the state to the view
        if state is not None:
            state = state.property_manager
        self._view.set_tree(state)

    def on_save_state(self):
        # Get the save location
        save_location = self._view.get_save_location()
        # Check if it exists
        path_dir = os.path.dirname(save_location)
        if not path_dir:
            self.gui_logger.warning("The provided save location for the SANS state does not seem to exist. "
                                    "Please provide a validate path")
            return

        file_name, _ = os.path.splitext(save_location)
        full_file_path = file_name + JSON_SUFFIX

        row_index = self._view.get_current_row()
        state = self.get_state(row_index)
        serialized_state = state.property_manager
        with open(full_file_path, 'w') as f:
            json.dump(serialized_state, f, sort_keys=True, indent=4)
        self.gui_logger.information("The state for row {} has been saved to: {} ".format(row_index, full_file_path))

        # Update the file name in the UI
        self._view.set_save_location(full_file_path)
