# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import absolute_import

# std imports
import atexit
import gc

# 3rd party imports
import six

from mantid.py3compat import Enum
from .observabledictionary import DictionaryAction, ObservableDictionary


class FigureAction(Enum):
    Update = 0
    New = 1
    Closed = 2
    Renamed = 3
    OrderChanged = 4
    VisibilityChanged = 5


class GlobalFigureManagerObserver(object):
    def __init__(self, figure_manager=None):
        """
        :param figure_manager: Figure manager that will be used to notify observers.
                               Injected on initialisation for easy mocking
        """
        self.figure_manager = figure_manager

    def notify(self, action, key):
        """
        This method is called when a dictionary entry is added,
        removed or changed
        :param action: An enum with the type of dictionary action
        :param key: The key in the dictionary that was changed
        :param old_value: Old value(s) removed
        """

        if action == DictionaryAction.Create:
            self.figure_manager.notify_observers(FigureAction.New, key)
        elif action == DictionaryAction.Set:
            self.figure_manager.notify_observers(FigureAction.Renamed, key)
        elif action == DictionaryAction.Removed:
            self.figure_manager.notify_observers(FigureAction.Closed, key)
        elif action == DictionaryAction.Update:
            self.figure_manager.notify_observers(FigureAction.Update, key)
        elif action == DictionaryAction.Clear:
            # On Clear notify the observers to close all of the figures
            # `figs.keys()` is safe to iterate and delete items at the same time
            # because `keys` returns a new list, not referencing the original dict
            for key in self.figure_manager.figs.keys():
                self.figure_manager.notify_observers(FigureAction.Closed, key)
        else:
            raise ValueError("Notifying for action {} is not supported".format(action))


class GlobalFigureManager(object):
    """
    Singleton to manage a set of integer-numbered figures. It replaces
    matplotlib._pylab_helpers.Gcf as the global figure manager.

    This class is never instantiated; it consists of two class
    attributes (a list and a dictionary), and a set of static
    methods that operate on those attributes, accessing them
    directly as class attributes.

    Attributes:

        *figs*:
          dictionary of the form {*num*: *manager*, ...}

        *_activeQue*:
          list of *managers*, with active one at the end

    """
    _activeQue = []
    figs = ObservableDictionary({})
    observers = []

    @classmethod
    def initialiseFiguresObserver(cls):
        """
        This is used to inject the GlobalFigureManager into the GlobalFigureManagerObserver
        as there is no way to reference the class' own name inside the class' own definition
        :return:
        """
        cls.figs.add_observer(GlobalFigureManagerObserver(cls))

    @classmethod
    def get_fig_manager(cls, num):
        """
        If figure manager *num* exists, make it the active
        figure and return the manager; otherwise return *None*.
        """
        manager = cls.figs.get(num, None)
        if manager is not None:
            cls.set_active(manager)
        return manager

    @classmethod
    def destroy(cls, num):
        """
        Try to remove all traces of figure *num*.

        In the interactive backends, this is bound to the
        window "destroy" and "delete" events.
        """
        if not cls.has_fignum(num):
            return
        current_fig_manager = cls.figs[num]
        current_fig_manager.canvas.mpl_disconnect(current_fig_manager._cidgcf)

        cls._remove_manager_if_present(current_fig_manager)

        del cls.figs[num]
        current_fig_manager.destroy()
        gc.collect(1)
        cls.notify_observers(FigureAction.OrderChanged, -1)

    @classmethod
    def destroy_fig(cls, fig):
        "*fig* is a Figure instance"
        num = None
        for manager in six.itervalues(cls.figs):
            if manager.canvas.figure == fig:
                num = manager.num
                break
        if num is not None:
            cls.destroy(num)

    @classmethod
    def destroy_all(cls):
        # this is need to ensure that gc is available in corner cases
        # where modules are being torn down after install with easy_install
        import gc  # noqa
        for manager in list(cls.figs.values()):
            manager.canvas.mpl_disconnect(manager._cidgcf)
            manager.destroy()

        cls._activeQue = []
        cls.figs.clear()
        gc.collect(1)

    @classmethod
    def has_fignum(cls, num):
        """
        Return *True* if figure *num* exists.
        """
        return num in cls.figs

    @classmethod
    def get_all_fig_managers(cls):
        """
        Return a list of figure managers.
        """
        return list(cls.figs.values())

    @classmethod
    def get_num_fig_managers(cls):
        """
        Return the number of figures being managed.
        """
        return len(cls.figs)

    @classmethod
    def get_active(cls):
        """
        Return the manager of the active figure, or *None*.
        """
        if len(cls._activeQue) == 0:
            return None
        else:
            return cls._activeQue[-1]

    @classmethod
    def set_active(cls, manager):
        """
        Make the figure corresponding to *manager* the active one.
        """
        cls._remove_manager_if_present(manager)
        cls._activeQue.append(manager)
        cls.figs[manager.num] = manager
        cls.notify_observers(FigureAction.OrderChanged, manager.num)

    @classmethod
    def _remove_manager_if_present(cls, manager):
        """
        Removes the manager from the active queue, if it is present in it.
        :param manager: Manager to be removed from the active queue
        :return:
        """
        try:
            del cls._activeQue[cls._activeQue.index(manager)]
        except ValueError:
            # the figure manager was not in the active queue - no need to delete anything
            pass

    @classmethod
    def draw_all(cls, force=False):
        """
        Redraw all figures registered with the pyplot
        state machine.
        """
        for f_mgr in cls.get_all_fig_managers():
            if force or f_mgr.canvas.figure.stale:
                f_mgr.canvas.draw_idle()

    # ------------------ Our additional interface -----------------

    @classmethod
    def last_active_values(cls):
        """
        Returns a dictionary where the keys are the plot numbers and
        the values are the last shown (active) order, the most recent
        being 1, the oldest being N, where N is the number of figure
        managers
        :return: A dictionary with the values as plot number and keys
                 as the opening order
        """
        last_shown_order_dict = {}
        num_figure_managers = len(cls._activeQue)

        for index in range(num_figure_managers):
            last_shown_order_dict[cls._activeQue[index].num] = num_figure_managers - index

        return last_shown_order_dict

    # ---------------------- Observer methods ---------------------
    # This is currently very simple as the only observer is
    # permanently registered to this class.

    @classmethod
    def add_observer(cls, observer):
        """
        Add an observer to this class - this can be any class with a
        notify() method
        :param observer: A class with a notify method
        """
        assert "notify" in dir(observer), "An observer must have a notify method"
        cls.observers.append(observer)

    @classmethod
    def notify_observers(cls, action, figure_number):
        """
        Calls notify method on all observers
        :param action: A FigureAction enum for the action called
        :param figure_number: The unique fig number (key in the dict)
        """
        for observer in cls.observers:
            observer.notify(action, figure_number)

    @classmethod
    def figure_title_changed(cls, figure_number):
        """
        Notify the observers that a figure title was changed
        :param figure_number: The unique number in GlobalFigureManager
        """
        cls.notify_observers(FigureAction.Renamed, figure_number)

    @classmethod
    def figure_visibility_changed(cls, figure_number):
        """
        Notify the observers that a figure was shown or hidden
        :param figure_number: The unique number in GlobalFigureManager
        """
        cls.notify_observers(FigureAction.VisibilityChanged, figure_number)


GlobalFigureManager.initialiseFiguresObserver()
atexit.register(GlobalFigureManager.destroy_all)
