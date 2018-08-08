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

    def set_load_run_widget(self, widget):
        self.load_run_widget = widget

    def set_load_file_widget(self, widget):
        self.load_file_widget = widget

    def enable_multiple_files(self, enabled):
        self.load_run_widget.enable_multiple_files(enabled)
        self.load_file_widget.enable_multiple_files(enabled)

    def load_data_from_browse(self):
        # clear run
        # get data from browse
        # store data (work out whether to append or clear then add)
        # clear browse.
        pass

    def load_data_from_run_widget(self):
        # clear browse
        # get data from runs
        # store data (work out whether to append or clear then add)
        # clear runs.
        pass

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

    @property
    def view(self):
        return self._view
