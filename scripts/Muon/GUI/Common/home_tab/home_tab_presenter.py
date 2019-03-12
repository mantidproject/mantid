# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod
from Muon.GUI.Common.observer_pattern import Observer


class HomeTabSubWidget:
    __metaclass__ = ABCMeta
    """
    Abstract base class which all sub-widgets must inherit from. This is used to
    enforce a common interface so that the home tab can keep a list of sub-widgets without
    specifically naming each one. Since each sub-widget shares a common model (the context)
    all the home tab needs to do is instruct them to update from their own model.
    """

    @abstractmethod
    def update_view_from_model(self):
        # update from model
        pass


class HomeTabPresenter(object):

    def __init__(self, view, model, subwidgets):
        self._view = view
        self._model = model

        self._subwidgets = subwidgets

        self.instrumentObserver = HomeTabPresenter.InstrumentObserver(self)
        self.loadObserver = HomeTabPresenter.LoadObserver(self)
        self.groupingObserver = HomeTabPresenter.GroupingObserver(self)
        self.enable_observer = HomeTabPresenter.EnableWidgetObserver(self)
        self.disable_observer = HomeTabPresenter.DisableWidgetObserver(self)

        self.update_all_widgets()

    def show(self):
        self._view.show()

    def show_all_data(self):
        if self._model.is_data_loaded():
            self._model.show_all_data()

    def update_current_data(self):
        self._model.update_current_data()

    def update_all_widgets(self):
        """
        Update all widgets from the context.
        """
        for subwidget in self._subwidgets:
            subwidget.update_view_from_model()

    def enable_home_tab(self):
        self._view.setEnabled(True)

    def disable_home_tab(self):
        self._view.setEnabled(False)

    # ------------------------------------------------------------------------------------------------------------------
    # Observer / Observable
    # ------------------------------------------------------------------------------------------------------------------

    class InstrumentObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.update_all_widgets()

    class LoadObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.update_all_widgets()
            self.outer.show_all_data()

    class GroupingObserver(Observer):

        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.update_all_widgets()

    class EnableWidgetObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.enable_home_tab()

    class DisableWidgetObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, arg):
            self.outer.disable_home_tab()
