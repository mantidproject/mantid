# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class Observer(object):
    """
    The Observer subscribes to Observables, when the Observable notifies its subscribers (the observers)
    they will call their update() method
    """

    def __init__(self):
        pass

    def update(self, observable, arg):
        """
        Called when the observed object is modified. You call an Observable object's notifyObservers method
        to notify all the object's observers of the change.
        """
        pass


class Observable(object):
    """
    The Observable is an object which may be subscribed to by Observers. It maintains a list of subscribers to it,
    and when needed, it will notify those subscribers.
    """

    def __init__(self):
        self._subscribers = []

    def add_subscriber(self, observer_instance):
        if not isinstance(observer_instance, Observer):
            raise AttributeError("Trying to add subscriber which is not Observer type.")
        if observer_instance not in self._subscribers:
            self._subscribers.append(observer_instance)

    def delete_subscriber(self, observer):
        self._subscribers.remove(observer)

    def delete_subscribers(self):
        self._subscribers = []

    def count_subscribers(self):
        return len(self._subscribers)

    def notify_subscribers(self, arg=None, **kwargs):
        for observer in self._subscribers:
            observer.update(self, arg, **kwargs)


class GenericObserver(Observer):
    def __init__(self, callback):
        Observer.__init__(self)
        self.callback = callback

    def update(self, observable, arg):
        self.callback()
