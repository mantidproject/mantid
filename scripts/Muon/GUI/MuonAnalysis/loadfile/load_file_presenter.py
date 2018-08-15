from __future__ import (absolute_import, division, print_function)

import sys

from Muon.GUI.Common import thread_model
import Muon.GUI.Common.muon_file_utils as fileUtils

def filter_for_extensions(extensions):
    str_list = ["*." + str(ext) for ext in extensions]
    return "Files (" + ", ".join(str_list) + ")"


# TODO : If files > 3, display "..." in the edit and change the tooltip to suggest to use the copy button

class BrowseFileWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._load_thread = None

        # Whether to allow single or multiple files to be loaded
        self._multiple_files = True

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
        file_filter = filter_for_extensions(["nxs"])
        directory = ""
        filenames = self._view.show_file_browser_and_return_selection(file_filter, [directory],
                                                                      multiple_files=self._multiple_files)
        filenames = self.validate_filenames_selection(filenames)
        print("Validate : ", self.validate_filenames_selection(filenames))
        return filenames

    def validate_filenames_selection(self, filenames):
        filenames = fileUtils.parse_user_input_to_files(";".join(filenames))
        filenames = fileUtils.remove_duplicated_files_from_list(filenames)
        return filenames

    def on_browse_button_clicked(self, threaded=True):
        filenames = self.get_filenames_from_user()
        print("Loading started")
        if not self._multiple_files and len(filenames) > 1:
            raise ValueError("Multiple files selected in single file mode")
        if filenames:
            self.handle_load_thread_start(filenames)

    def handle_load_thread_start(self, filenames):
        if self._load_thread:
            return
        print("Load thread started, Model : ", self._model.loaded_filenames)
        self._view.disable_loading()
        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.loadData(filenames)
        self._load_thread.start()

    def handle_load_thread_finished(self):
        print("Load thread finished : ", self._load_thread.currentThreadId(), " Model : ", self._model.loaded_filenames)
        self._load_thread.threadWrapperTearDown(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.deleteLater()
        self._load_thread = None
        self.on_loading_finished()

    def on_loading_finished(self):
        print("on_loading_finished")
        file_list = self._model.loaded_filenames
        print(file_list)
        self.set_file_edit(file_list)
        self._view.enable_loading()
        self._model.add_directories_to_config_service(file_list)


    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear()

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def enable_multiple_files(self, enabled):
        if enabled:
            print("Enabling multiple files")
        else:
            print("Disabling multiple files")
        self._multiple_files = enabled

    def get_loaded_filenames(self):
        return self._model.loaded_filenames

    @property
    def workspaces(self):
        return self._model.loaded_workspaces

    @property
    def runs(self):
        return self._model.loaded_runs

    def set_file_edit(self, file_list):
        display_text = len(file_list) > 10
        self._view.set_file_edit(";".join(file_list), display_text)

    def handle_file_changed_by_user(self):
        print("User has updated files! Model : ", self._model.loaded_filenames)
        user_input = self._view.get_file_edit_text()
        filenames = fileUtils.parse_user_input_to_files(user_input, [".nxs"])
        filenames = fileUtils.remove_duplicated_files_from_list(filenames)
        if not filenames:
            self._view.reset_edit_to_cached_value()
            return
        if not self._multiple_files and len(filenames) > 1:
            # TODO : fix this to only open at most one warning (opens two if user hits enter)
            self._view.warning_popup("Multiple files selected in single file mode")
            self._view.reset_edit_to_cached_value()
            return
        self.handle_load_thread_start(filenames)

    # used by parent widget
    def update_view_from_model(self, file_list):
        self.set_file_edit(file_list)