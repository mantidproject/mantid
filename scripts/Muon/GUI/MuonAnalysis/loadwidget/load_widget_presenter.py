from __future__ import (absolute_import, division, print_function)


class LoadWidgetPresenter(object):
    """
    The load widget is responsible for combining data loaded from its two sub-widgets in a systematic way
    (either keeping a single workspace, or allowing multiple to be loaded at once).

     - It handles any additional load behaviours such as how to handle multiple runs.
     - Handles instrument change.

    Although there is duplication of code (in the models of the subwidgets) this allows them to be
    used standalone without this parent widget.
    """

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self.load_run_widget = None
        self.load_file_widget = None

        self.view.on_subwidget_loading_started(self.disable_loading)
        self.view.on_subwidget_loading_finished(self.enable_loading)

        self.view.on_file_widget_data_changed(self.handle_file_widget_data_changed)
        self.view.on_run_widget_data_changed(self.handle_run_widget_data_changed)

        self._view.on_multiple_loading_check_changed(self.handle_multiple_files_option_changed)

    def set_load_run_widget(self, widget):
        self.load_run_widget = widget
        self.load_run_widget.enable_multiple_files(False)

    def set_load_file_widget(self, widget):
        self.load_file_widget = widget
        self.load_file_widget.enable_multiple_files(False)

    def handle_multiple_files_option_changed(self):
        print("handling multiple files changed")
        if self._view.get_multiple_loading_state():
            self.enable_multiple_files(True)
        else:
            self.enable_multiple_files(False)

    def enable_multiple_files(self, enabled=True):
        self.load_run_widget.enable_multiple_files(enabled)
        self.load_file_widget.enable_multiple_files(enabled)

    def handle_file_widget_data_changed(self):
        print("handle_file_widget_data_changed")
        # get data from browse
        if not self._view.get_multiple_loading_state():
            self.clear_data()
        self.add_new_loaded_data_from_widget(self.load_file_widget)
        # store data (work out whether to append or clear then add)
        # clear browse.
        # update browse with the state of the model
        self.load_run_widget.update_model_and_view(self._model.runs)
        self.load_file_widget.update_model_and_view(self._model.filenames)

    def handle_run_widget_data_changed(self):
        print("handle_run_widget_data_changed")
        # get data from runs
        if not self._view.get_multiple_loading_state():
            self.clear_data()
        self.add_new_loaded_data_from_widget(self.load_run_widget)
        # store data (work out whether to append or clear then add)
        # clear runs.
        # update runs with the state of the model
        print(self._model.runs)
        self.load_run_widget.update_model_and_view(self._model.runs)
        self.load_file_widget.update_model_and_view(self._model.filenames)

    def add_new_loaded_data_from_widget(self, widget, overwrite=False):
        new_filenames = widget.get_loaded_filenames()
        print(new_filenames)
        for i, filename in enumerate(new_filenames):
            if self._model.is_filename_loaded(filename) and not overwrite:
                continue
            else:
                ws = widget.workspaces[i]
                run = widget.runs[i]
                self._model.add_muon_data(filename, ws, run)

    def handle_instrument_changed(self, new_instrument):
        # clear both run and browse
        # set new instrument in browse
        # set new instrument in run
        pass

    def disable_loading(self):
        print("Disabling loading")
        self.load_run_widget.disable_loading()
        self.load_file_widget.disable_loading()

    def enable_loading(self):
        print("Enabling loading")
        self.load_run_widget.enable_loading()
        self.load_file_widget.enable_loading()

    def show(self):
        self._view.show()

    def clear_data(self):
        self._model.clear_data()


    @property
    def view(self):
        return self._view

    def update_widget(self):
        pass
