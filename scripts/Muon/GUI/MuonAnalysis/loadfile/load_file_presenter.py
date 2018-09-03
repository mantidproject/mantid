from __future__ import (absolute_import, division, print_function)

import copy

from Muon.GUI.Common import thread_model
import Muon.GUI.Common.muon_file_utils as file_utils


class BrowseFileWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        # Whether to allow single or multiple files to be loaded
        self._multiple_files = True

        self._use_threading = True
        self._load_thread = None

        self._view.on_browse_clicked(self.on_browse_button_clicked)
        self._view.on_file_edit_changed(self.handle_file_changed_by_user)

    def show(self):
        self._view.show()

    def cancel(self):
        if self._load_thread is not None:
            self._load_thread.cancel()

    def create_load_thread(self):
        return thread_model.ThreadModel(self._model)

    def get_filenames_from_user(self):
        file_filter = file_utils.filter_for_extensions(["nxs"])
        directory = ""
        filenames = self._view.show_file_browser_and_return_selection(file_filter, [directory],
                                                                      multiple_files=self._multiple_files)
        # validate
        filenames = file_utils.parse_user_input_to_files(";".join(filenames))
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        return filenames

    def on_browse_button_clicked(self):
        filenames = self.get_filenames_from_user()
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        if filenames:
            self.handle_loading(filenames)

    def handle_file_changed_by_user(self):
        user_input = self._view.get_file_edit_text()
        filenames = file_utils.parse_user_input_to_files(user_input)
        filenames = file_utils.remove_duplicated_files_from_list(filenames)
        if not filenames:
            self._view.reset_edit_to_cached_value()
            return
        if not self._multiple_files and len(filenames) > 1:
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        self.handle_loading(filenames)

    def handle_loading(self, filenames):
        if self._use_threading:
            self.handle_load_thread_start(filenames)
        else:
            self.handle_load_no_threading(filenames)

    def handle_load_no_threading(self, filenames):
        self._view.notify_loading_started()
        self.disable_loading()
        self._model.loadData(filenames)
        try:
            self._model.execute()
        except ValueError as e:
            self._view.warning_popup(e.args[0])
        self.on_loading_finished()

    def handle_load_thread_start(self, filenames):
        if self._load_thread:
            return
        self._view.notify_loading_started()
        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.loadData(filenames)
        self._load_thread.start()

    def handle_load_thread_finished(self):
        self._load_thread.threadWrapperTearDown(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.deleteLater()
        self._load_thread = None
        self.on_loading_finished()

    def on_loading_finished(self):
        file_list = self._model.loaded_filenames
        self.set_file_edit(file_list)
        self._view.notify_loading_finished()
        self.enable_loading()
        self._model.add_directories_to_config_service(file_list)

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear()

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def enable_multiple_files(self, enabled):
        self._multiple_files = enabled

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def runs(self):
        return self._model.loaded_runs

    def set_file_edit(self, file_list):
        file_list = sorted(copy.copy(file_list))
        self._view.set_file_edit(";".join(file_list), False)

    # used by parent widget
    def update_view_from_model(self, file_list):
        self.set_file_edit(file_list)
