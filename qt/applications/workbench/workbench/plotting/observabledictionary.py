# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.py3compat import Enum


class DictionaryAction(Enum):
    Create = 0
    Set = 1
    Removed = 2
    Clear = 3
    Update = 4


class ObservableDictionary(dict):
    """
    Override parts of dictionary that deal with adding, removing
    or changing data in the dictionary to call an observer.
    """

    def __init__(self, value):
        super(ObservableDictionary, self).__init__(value)
        self.observers = []

    def add_observer(self, observer):
        """
        Add an observer to this class - this can be any class with a
        notify() method
        :param observer: A class with a notify method
        """
        self.observers.append(observer)

    def _notify_observers(self, action, key=-1):
        for observer in self.observers:
            observer.notify(action, key)

    def __setitem__(self, key, new_value):
        action = DictionaryAction.Set

        if key not in self:
            action = DictionaryAction.Create
        dict.__setitem__(self, key, new_value)

        self._notify_observers(action, key)

    def __delitem__(self, key):
        dict.__delitem__(self, key)
        self._notify_observers(DictionaryAction.Removed, key)

    def clear(self):
        dict.clear(self)
        self._notify_observers(DictionaryAction.Clear)

    def pop(self, key, default=None):
        if key in self:
            old_value = dict.pop(self, key)
            self._notify_observers(DictionaryAction.Removed, key)
            return old_value
        else:
            return dict.pop(self, key, default)

    def popitem(self):
        key, old_value = dict.popitem(self)
        self._notify_observers(DictionaryAction.Removed, old_value=old_value)
        return key, old_value

    def update(self, updated_dictionary):
        dict.update(self, updated_dictionary)
        self._notify_observers(DictionaryAction.Update)
