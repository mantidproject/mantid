from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget

def filter_for_extensions(extensions):
    """Filter for file browser"""
    str_list = ["*." + str(ext) for ext in extensions]
    return "Files (" + ", ".join(str_list) + ")"


class Observable:

    def __init__(self):
        self._subscribers = []

    def add_subscriber(self, observer):
        if observer not in self._subscribers:
            self._subscribers.append(observer)

    def delete_subscriber(self, observer):
        self._subscribers.remove(observer)

    def delete_subscribers(self):
        self._subscribers = []

    def count_subscribers(self):
        return len(self._subscribers)

    def notify_subscribers(self, arg=None):
        for observer in self._subscribers:
            observer.update(self, arg)


class InstrumentWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_time_zero_checkState_changed(self.handle_loaded_time_zero_checkState_change)
        self._view.on_first_good_data_checkState_changed(self.handle_loaded_first_good_data_checkState_change)
        self._view.on_dead_time_from_data_selected(self.handle_user_selects_dead_time_from_data)

        self._view.on_dead_time_browse_clicked(self.handle_dead_time_browse_clicked)

        self._view.on_instrument_changed(self.handle_instrument_changed)

        self.instrumentNotifier = InstrumentWidgetPresenter.InstrumentNotifier(self)

    class InstrumentNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer  # handle to containing class

        def notify_subscribers(self, arg=None):
            Observable.notify_subscribers(self)

    def handle_dead_time_browse_clicked(self):
        self._view.show_file_browser_and_return_selection(filter_for_extensions(['nxs']), [''], multiple_files=False)

    def update_view_from_model(self):
        self.handle_loaded_first_good_data_checkState_change()
        self.handle_loaded_time_zero_checkState_change()

    def handle_loaded_time_zero_checkState_change(self):
        if self._view.time_zero_state():
            time_zero = self._model.get_file_time_zero()
            self._view.set_time_zero(time_zero)
        else:
            time_zero = self._model.get_user_time_zero()
            self._view.set_time_zero(time_zero)

    def handle_loaded_first_good_data_checkState_change(self):
        if self._view.first_good_data_state():
            first_good_data = self._model.get_file_first_good_data()
            self._view.set_first_good_data(first_good_data)
        else:
            first_good_data = self._model.get_user_first_good_data()
            self._view.set_first_good_data(first_good_data)

    def dead_time_from_data_text(self, dead_times):
        mean = sum(dead_times) / len(dead_times)
        label = "From {} to {} (ave. {})".format(min(dead_times), max(dead_times), mean)
        return label

    def handle_user_selects_dead_time_from_data(self):
        print("HERE")
        dtc = self._model.get_dead_time_table_from_data()
        # assert isinstance(dtc, TableWorkspace)
        dead_times = dtc.toDict()['dead-time']
        print(dead_times)
        text = self.dead_time_from_data_text(dead_times)
        print(text)
        self._view.set_dead_time_label(text)

    def handle_instrument_changed(self):
        print("Instrument changed")
        self._model.clear_data()
        self.clear_view()
        self.instrumentNotifier.notify_subscribers()

    def clear_view(self):
        self._view.set_time_zero(0.0)
        self._view.set_first_good_data(0.0)
        self._view.set_combo_boxes_to_default()
        self._view.set_checkboxes_to_defualt()

    def show(self):
        self._view.show()
