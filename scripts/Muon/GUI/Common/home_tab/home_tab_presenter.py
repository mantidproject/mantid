from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod

class HomeTabSubWidget:
    __metaclass__ = ABCMeta
    """Abstract base class which all sub-widgets must inherit from. This is used to
    enforce a common interface so that the home tab can keep a list of sub-widgets without
    specifically naming each one. Since each sub-widget shares a common model (the context)
    all the home tab needs to do is instruct them to update from their own model"""

    @abstractmethod
    def update_view_from_model(self):
        # update from model
        pass

class Observer:

    def update(observable, arg):
        '''Called when the observed object is
        modified. You call an Observable object's
        notifyObservers method to notify all the
        object's observers of the change.'''
        pass

class HomeTabPresenter(object):

    def __init__(self, view, model,
                 # instrument_widget=None,
                 # grouping_widget=None,
                 # plot_widget=None,
                 # run_info_widget=None,
                 subwidgets = []
                 ):
        self._view = view
        self._model = model

        self._subwidgets = subwidgets

        # self._instrument_widget = instrument_widget
        # self._grouping_widget = grouping_widget
        # self._plot_widget = plot_widget
        # self._run_info_widget = run_info_widget

        self.instrumentObserver = HomeTabPresenter.InstrumentObserver(self)


    def show(self):
        self._view.show()

    def update_all_widgets(self):
        """Update all widgets from the context"""
        for subwidget in self._subwidgets:
            subwidget.update_view_from_model()


    class InstrumentObserver(Observer):

        def __init__(self, outer):
            self.outer = outer

        def update(self,observable, arg):
            print("Bee's breakfast time!")
            self.outer.update_all_widgets()