from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common import thread_model


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
        self._multiple_files = False

        self._view.on_browse_clicked(self.browse_button_clicked)

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
        return filenames

    def browse_button_clicked(self):
        filenames = self.get_filenames_from_user()
        print("Loading started")
        if filenames:
            self.handle_load_thread_start(filenames)

    def handle_load_thread_start(self, filenames):
        self._view.disable_loading()
        self._load_thread = self.create_load_thread()
        self._load_thread.threadWrapperSetUp(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.loadData(filenames)
        self._load_thread.start()

    def handle_load_thread_finished(self):
        self._view.enable_loading()
        self._load_thread.threadWrapperTearDown(self.disable_loading, self.handle_load_thread_finished)
        self._load_thread.deleteLater()
        self._load_thread = None

        file_list = self._model.loaded_filenames
        self._view.set_file_edit(";".join(file_list))

    def clear_loaded_data(self):
        self._view.clear()
        self._model.clear()

    def disable_loading(self):
        self._view.disable_load_buttons()

    def enable_loading(self):
        self._view.enable_load_buttons()

    def enable_multiple_files(self, enabled):
        self._multiple_files = enabled
