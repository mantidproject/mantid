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
from six import itervalues


class CurrentFigure(object):
    """A singleton manager of all figures created through it. Analogous to _pylab_helpers.Gcf.

    Each figure has a hold/active button attached to it:
      - active toggled = next plot operation should replace this figure
      - hold toggled = next plot operation should produce a new figure

    Attributes:

        *_active*:
          A reference to the figure that is set as active, can be None

        *figs*:
          dictionary of the form {*num*: *manager*, ...}
    """
    _active = None
    figs = {}
    observers = []

    @classmethod
    def get_fig_manager(cls, _):
        """
        If an active figure manager exists return it; otherwise
        return *None*.
        """
        return cls.get_active()

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

        if cls._active is None:
            print("Error: can not destroy figures while there are no active figures. "
                  "This should be fixed once the hold/active functionality is removed.")
            return

        if cls._active.num == num:
            cls._active = None
        del cls.figs[num]
        manager.destroy()
        gc.collect(1)
        cls.notify_observers()

    @classmethod
    def destroy_fig(cls, fig):
        "*fig* is a Figure instance"
        num = None
        for manager in itervalues(cls.figs):
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

        cls._active = None
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
        if cls._active is not None:
            cls._active.canvas.figure.clf()
        return cls._active

    @classmethod
    def set_active(cls, manager):
        """
        Make the figure corresponding to *manager* the active one.

        Notifies all other figures to disable their active status.
        """
        cls._active = manager
        active_num = manager.num
        cls.figs[active_num] = manager
        for manager in itervalues(cls.figs):
            if manager.num != active_num:
                manager.hold()
        cls.notify_observers()

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
    def get_figure_number_from_name(cls, figure_title):
        for num, figure_manager in cls.figs.items():
            if figure_manager.get_window_title() == figure_title:
                return num
        return None

    @classmethod
    def get_figure_manager_from_name(cls, figure_title):
        for figure_manager in cls.figs.values():
            if figure_manager.get_window_title() == figure_title:
                return figure_manager
        return None

    @classmethod
    def set_hold(cls, manager):
        """If this manager is active then set inactive"""
        if cls._active == manager:
            cls._active = None

    # ---------------------- Observer methods ---------------------
    # This is currently very simple as the only observer is
    # permanently registered to this class.

    @classmethod
    def add_observer(cls, observer):
        cls.observers.append(observer)

    @classmethod
    def notify_observers(cls):
        for observer in cls.observers:
            observer.notify()

atexit.register(CurrentFigure.destroy_all)
