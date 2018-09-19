from __future__ import (absolute_import, division, print_function)


class Observer(object):
    """
    The Observer observes (or subscribes) Observables, when the Observable notifies its subscribers (the observers)
    they will cal their update() method
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
    The Observable is an object which may be observed by Observers. It maintains a list of observers (aka subscribers)
    to it, and when needed, it will notify those subscribers.
    """

    def __init__(self):
        self._subscribers = []

    def add_subscriber(self, observer):
        if not isinstance(observer, Observer):
            raise AttributeError("Trying to add subscriber which is not Observer type.")
        if observer not in self._subscribers:
            self._subscribers.append(observer)

    def delete_subscriber(self, observer):
        self._subscribers.remove(observer)

    def delete_subscribers(self):
        self._subscribers = []

    def count_subscribers(self):
        return len(self._subscribers)

    def notify_subscribers(self, arg=None, **kwargs):
        for observer in self._subscribers:
            observer.update(self, arg, **kwargs)
