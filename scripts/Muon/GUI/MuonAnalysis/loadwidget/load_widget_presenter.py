from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.observer_pattern import Observer, Observable


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

        self.view.on_clear_button_clicked(self.clear_data_and_view)

        self._view.on_multiple_loading_check_changed(self.handle_multiple_files_option_changed)

        self.instrumentObserver = LoadWidgetPresenter.InstrumentObserver(self)
        self.loadNotifier = LoadWidgetPresenter.LoadNotifier(self)

    def set_load_run_widget(self, widget):
        self.load_run_widget = widget
        self.load_run_widget.enable_multiple_files(False)

    def set_load_file_widget(self, widget):
        self.load_file_widget = widget
        self.load_file_widget.enable_multiple_files(False)

    def handle_multiple_files_option_changed(self):
        if self._view.get_multiple_loading_state():
            self.enable_multiple_files(True)
        else:
            self.enable_multiple_files(False)

        selection = self._view.get_multiple_loading_combo_text()

        if selection == "Co-Add":
            self.load_file_widget.update_multiple_loading_behaviour("Co-Add")
            self.load_run_widget.update_multiple_loading_behaviour("Co-Add")

        if selection == "Simultaneous":
            self.load_file_widget.update_multiple_loading_behaviour("Simultaneous")
            self.load_run_widget.update_multiple_loading_behaviour("Simultaneous")

        self.clear_data_and_view()

    def enable_multiple_files(self, enabled=True):
        self.load_run_widget.enable_multiple_files(enabled)
        self.load_file_widget.enable_multiple_files(enabled)

    def handle_file_widget_data_changed(self):
        self.load_run_widget.update_view_from_model(self._model.runs)
        self.load_file_widget.update_view_from_model(self._model.filenames)
        self.loadNotifier.notify_subscribers()

    def handle_run_widget_data_changed(self):
        self.load_run_widget.update_view_from_model(self._model.runs)
        self.load_file_widget.update_view_from_model(self._model.filenames)
        self.loadNotifier.notify_subscribers()

    def disable_loading(self):
        self.load_run_widget.disable_loading()
        self.load_file_widget.disable_loading()
        self._view.disable_loading()

    def enable_loading(self):
        self.load_run_widget.enable_loading()
        self.load_file_widget.enable_loading()
        self._view.enable_loading()

    def show(self):
        self._view.show()

    def clear_data(self):
        self._model.clear_data()

    def clear_data_and_view(self):
        self._model.clear_data()
        self.handle_run_widget_data_changed()
        self.handle_run_widget_data_changed()
        self.load_run_widget.set_current_instrument("Instrument")
        self.loadNotifier.notify_subscribers()

    @property
    def view(self):
        return self._view

    def set_current_instrument(self, instrument):
        self._model._loaded_data_store.instrument = instrument
        self.load_file_widget.set_current_instrument(instrument)
        self.load_run_widget.set_current_instrument(instrument)

    def update_new_instrument(self, instrument):
        self.clear_data_and_view()
        self.set_current_instrument(instrument)

    class LoadNotifier(Observable):
        """
        Notify when loaded data changes from file widget or run widget, or when clear button is pressed.
        """

        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, arg=None):
            Observable.notify_subscribers(self, arg)

    class InstrumentObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self, observable, arg):
            print("update called : ", arg)
            self.outer.update_new_instrument(arg)
