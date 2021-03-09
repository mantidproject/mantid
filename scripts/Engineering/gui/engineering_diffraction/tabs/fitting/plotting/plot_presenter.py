# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObserver, GenericObservable
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_model import FittingPlotModel
from Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_view import FittingPlotView
from mantid.simpleapi import Fit, logger
from copy import deepcopy

PLOT_KWARGS = {"linestyle": "", "marker": "x", "markersize": "3"}


class FittingPlotPresenter(object):
    def __init__(self, parent, model=None, view=None):
        if view is None:
            self.view = FittingPlotView(parent)
        else:
            self.view = view
        if model is None:
            self.model = FittingPlotModel()
        else:
            self.model = model

        self.workspace_added_observer = GenericObserverWithArgPassing(self.add_workspace_to_plot)
        self.workspace_removed_observer = GenericObserverWithArgPassing(self.remove_workspace_from_plot)
        self.all_workspaces_removed_observer = GenericObserver(self.clear_plot)
        self.seq_fit_started_observer = GenericObserverWithArgPassing(self.do_sequential_fit)
        self.seq_fit_done_notifier = GenericObservable()

    def add_workspace_to_plot(self, ws):
        axes = self.view.get_axes()
        for ax in axes:
            self.model.add_workspace_to_plot(ws, ax, PLOT_KWARGS)
        self.view.update_figure()

    def remove_workspace_from_plot(self, ws):
        for ax in self.view.get_axes():
            self.model.remove_workspace_from_plot(ws, ax)
            self.view.remove_ws_from_fitbrowser(ws)
        self.view.update_figure()

    def clear_plot(self):
        for ax in self.view.get_axes():
            self.model.remove_all_workspaces_from_plot(ax)
        self.view.clear_figure()
        self.view.update_fitbrowser()

    def do_sequential_fit(self, ws_list):
        fitprop_list = []
        for ws in ws_list:
            logger.notice(f'Starting to fit workspace {ws}')
            fitprop = self.view.read_fitprop_from_browser()
            # update I/O workspace name
            fitprop['properties']['Output'] = ws
            fitprop['properties']['InputWorkspace'] = ws
            # do fit
            fit_output = Fit(**fitprop['properties'])
            fitprop['properties']['Function'] = str(fit_output.Function.fun)
            # save setup in fitprop browser (updates browser for next iteration of loop)
            self.view.update_browser(fit_output.OutputStatus, fitprop['properties']['Function'], ws)
            # append a deep copy to output list
            fitprop_list.append(deepcopy(fitprop))
        logger.notice('Sequential fitting finished.')
        self.seq_fit_done_notifier.notify_subscribers(fitprop_list)
