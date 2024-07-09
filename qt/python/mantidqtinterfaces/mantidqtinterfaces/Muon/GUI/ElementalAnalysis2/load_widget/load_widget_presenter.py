# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import Observable, GenericObserver

CO_ADD = "Co-Add"
SIMULTANEOUS = "Simultaneous"


class LoadWidgetPresenterEA(object):
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

        self._view.on_multiple_load_type_changed(self.handle_multiple_runs_option_changed)
        self._use_threading = True

        self.loadNotifier = Observable()

        self.disable_observer = GenericObserver(self.disable_loading)
        self.enable_observer = GenericObserver(self.enable_loading)

        self.update_view_from_model_observer = GenericObserver(self.update_view_from_model)

    def set_load_run_widget(self, widget):
        self.load_run_widget = widget
        self.load_run_widget.update_view_from_model([])

    def set_load_file_widget(self, widget):
        self.load_file_widget = widget
        self.load_file_widget.update_view_from_model([])

    def handle_multiple_runs_option_changed(self):
        selection = self._view.get_multiple_loading_state()
        if selection:
            self.load_file_widget.update_multiple_loading_behaviour(CO_ADD)
            self.load_run_widget.update_multiple_loading_behaviour(CO_ADD)
        else:
            self.load_file_widget.update_multiple_loading_behaviour(SIMULTANEOUS)
            self.load_run_widget.update_multiple_loading_behaviour(SIMULTANEOUS)
        self.load_run_widget.handle_run_changed_by_user()

    def enable_multiple_runs(self, enabled=True):
        self.load_run_widget.enable_multiple_runs(enabled)
        self.load_file_widget.enable_multiple_runs(enabled)

    def handle_file_widget_data_changed(self):
        self.load_run_widget.update_view_from_model(self._model.runs)
        self.loadNotifier.notify_subscribers()

    def handle_run_widget_data_changed(self):
        self._model.update_current_data()
        self.load_run_widget.update_view_from_model(self._model.runs)
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
        self.load_run_widget.set_current_instrument(self._model.instrument)
        self.loadNotifier.notify_subscribers()

    def update_view_from_model(self):
        self.load_run_widget.update_view_from_model(self._model.runs)

    @property
    def view(self):
        return self._view

    def set_current_instrument(self, instrument):
        self._model.instrument = instrument
        self.load_file_widget.set_current_instrument(instrument)
        self.load_run_widget.set_current_instrument(instrument)
