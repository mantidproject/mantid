# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import Observable


class GuiVariablesNotifier(Observable):
    def __init__(self, outer):
        Observable.__init__(self)
        self.outer = outer  # handle to containing class

    def notify_subscribers(self, *args, **kwargs):
        Observable.notify_subscribers(self, *args, **kwargs)


class MuonGuiContext(dict):
    def __init__(self, *args, **kwargs):
        super(MuonGuiContext, self).__init__(*args, **kwargs)
        self.gui_variables_notifier = GuiVariablesNotifier(self)
        self.gui_variable_non_calulation_notifier = GuiVariablesNotifier(self)

    def update_and_send_signal(self, *args, **kwargs):
        updated_items = {k: kwargs[k] for k in kwargs if k in self and kwargs[k] != self[k] or k not in self}
        equal_entries = 0
        for key, value in updated_items.items():
            try:
                if self[key].id() == value.id():
                    equal_entries += 1
            except (AttributeError, KeyError, RuntimeError):
                pass

        if (not updated_items and kwargs) or equal_entries == len(updated_items):
            return

        super(MuonGuiContext, self).update(*args, **kwargs)
        self.gui_variables_notifier.notify_subscribers(kwargs)

    def update_and_send_non_calculation_signal(self, *args, **kwargs):
        updated_items = {k: kwargs[k] for k in kwargs if k in self and kwargs[k] != self[k] or k not in self}
        if not updated_items and kwargs:
            return

        super(MuonGuiContext, self).update(*args, **kwargs)
        self.gui_variable_non_calulation_notifier.notify_subscribers(kwargs)

    def add_subscriber(self, observer):
        self.gui_variables_notifier.add_subscriber(observer)

    def add_non_calc_subscriber(self, observer):
        self.gui_variable_non_calulation_notifier.add_subscriber(observer)

    def remove_workspace_by_name(self, workspace_name):
        try:
            if self['DeadTimeTable'].name() == workspace_name:
                self.pop('DeadTimeTable')
                self['DeadTimeSource'] = 'FromFile'
        except KeyError:
            pass
