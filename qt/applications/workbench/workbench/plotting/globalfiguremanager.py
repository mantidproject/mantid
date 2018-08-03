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
from __future__ import absolute_import

# std imports
import atexit
import gc

# 3rdparty imports
import six

from mantidqt.py3compat import Enum
from .observabledictionary import DictionaryAction, ObservableDictionary


class FigureAction(Enum):
    Unknown = 0
    New = 1
    Closed = 2
    Renamed = 3
    OrderChanged = 4
    VisibilityChanged = 5


class GlobalFigureManagerObserver(object):
    def notify(self, action, key):
        """
        This method is called when a dictionary entry is added,
        removed or changed
        :param action: An enum with the type of dictionary action
        :param key: The key in the dictionary that was changed
        :param old_value: Old value(s) removed
        """
        gcf = GlobalFigureManager

        if action == DictionaryAction.Create:
            gcf.notify_observers(FigureAction.New, key)
        elif action == DictionaryAction.Set:
            gcf.notify_observers(FigureAction.Renamed, key)
        elif action == DictionaryAction.Removed:
            gcf.notify_observers(FigureAction.Closed, key)
        else:
            # Not expecting clear or update to be used, so we are
            # being lazy here and just updating the entire plot list
            gcf.notify_observers(FigureAction.Unknown, key)


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
    figs.add_observer(GlobalFigureManagerObserver())
    observers = []

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
        manager = cls.figs[num]
        manager.canvas.mpl_disconnect(manager._cidgcf)

        # There must be a good reason for the following careful
        # rebuilding of the activeQue; what is it?
        oldQue = cls._activeQue[:]
        cls._activeQue = []
        for f in oldQue:
            if f != manager:
                cls._activeQue.append(f)

        del cls.figs[num]
        manager.destroy()
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
        oldQue = cls._activeQue[:]
        cls._activeQue = []
        for m in oldQue:
            if m != manager:
                cls._activeQue.append(m)
        cls._activeQue.append(manager)
        cls.figs[manager.num] = manager
        cls.notify_observers(FigureAction.OrderChanged, manager.num)

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


atexit.register(GlobalFigureManager.destroy_all)
