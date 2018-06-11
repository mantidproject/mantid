#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from mantidqt.py3compat import Enum


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

    def _notify_observers(self, action, new_value=None, old_value=None):
        for observer in self.observers:
            observer.notify(action, new_value, old_value)

    def __setitem__(self, key, new_value):
        action = DictionaryAction.Set

        if key in self:
            old_value = self.__getitem__(key)
        else:
            old_value = None
            action = DictionaryAction.Create
        dict.__setitem__(self, key, new_value)

        self._notify_observers(action, new_value, old_value)

    def __delitem__(self, key):
        old_value = dict.__getitem__(self, key)
        dict.__delitem__(self, key)
        self._notify_observers(DictionaryAction.Removed, old_value=old_value)

    def clear(self):
        dict.clear(self)
        self._notify_observers(DictionaryAction.Clear)

    def pop(self, key, default=None):
        if key in self:
            old_value = dict.pop(self, key)
            self._notify_observers(DictionaryAction.Removed, old_value=old_value)
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
