# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObserver, GenericObservable
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_model import FittingPlotModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_view import FittingPlotView
from mantid.simpleapi import Fit, logger
from copy import deepcopy

PLOT_KWARGS = {"linestyle": "", "marker": "x", "markersize": "3"}
FAILED_STYLE_SHEET = """QProgressBar {border: 1px solid red; border-radius: 2px}"""
SUCCESS_STYLE_SHEET = """QProgressBar::chunk {background-color: #2de100; margin: 0.5px;}"""
IN_PROGRESS_STYLE_SHEET = """QProgressBar::chunk { width: 25px; margin: 0.5px;
                                        background-color: qlineargradient(spread:reflect, x1:0, y1:0, x2:0.5, y2:0,
                                                                          stop:0 white, stop:1 blue);}"""
EMPTY_STYLE_SHEET = """QProgressBar {color: blue;}"""


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
        self.fit_all_started_observer = GenericObserverWithArgPassing(self.do_fit_all)
        self.fit_all_done_notifier = GenericObservable()
        self.fit_started_observer = GenericObserver(self.set_progress_bar_to_in_progress)
        self.fit_complete_observer = GenericObserverWithArgPassing(self.set_final_state_progress_bar)

        self.setup_toolbar()

    def setup_toolbar(self):
        self.view.set_slot_for_display_all()
        self.view.set_slot_for_fit_toggled(self.fit_toggle)

    def add_workspace_to_plot(self, ws):
        axes = self.view.get_axes()
        for ax in axes:
            self.model.add_workspace_to_plot(ws, ax, PLOT_KWARGS)
        self.view.update_figure()
        self.set_progress_bar_zero()

    def remove_workspace_from_plot(self, ws):
        for ax in self.view.get_axes():
            self.model.remove_workspace_from_plot(ws, ax)
            self.view.remove_ws_from_fitbrowser(ws)
        self.view.update_figure()
        self.set_progress_bar_zero()

    def clear_plot(self):
        for ax in self.view.get_axes():
            self.model.remove_all_workspaces_from_plot(ax)
        self.view.clear_figure()
        self.view.update_fitbrowser()
        self.set_progress_bar_zero()

    def do_fit_all(self, ws_name_list, do_sequential=True):
        self.set_progress_bar_to_in_progress()
        fitprop_list = []
        prev_fitprop = self.view.read_fitprop_from_browser()
        final_status = None
        for ws_name in ws_name_list:
            logger.notice(f'Starting to fit workspace {ws_name}')
            fitprop = deepcopy(prev_fitprop)
            # update I/O workspace name
            fitprop['properties']['Output'] = ws_name
            fitprop['properties']['InputWorkspace'] = ws_name
            # do fit
            fit_output = Fit(**fitprop['properties'])
            # update results
            fitprop['status'] = fit_output.OutputStatus
            funcstr = str(fit_output.Function.fun)
            fitprop['properties']['Function'] = funcstr
            if "success" in fitprop['status'].lower() and do_sequential:
                # update function in prev fitprop to use for next workspace
                prev_fitprop['properties']['Function'] = funcstr
            # update last fit in fit browser and save setup
            self.view.update_browser(fit_output.OutputStatus, funcstr, ws_name)
            # append a deep copy to output list (will be initial parameters if not successful)
            fitprop_list.append(fitprop)
            final_status = fit_output.OutputStatus

        self.set_final_state_progress_bar(output_list=None, status=final_status)
        logger.notice('Sequential fitting finished.')
        self.fit_all_done_notifier.notify_subscribers(fitprop_list)

    def fit_toggle(self):
        """Toggle fit browser and tool on/off"""
        if self.view.is_fit_browser_visible():
            self.view.hide_fit_browser()
            self.view.hide_fit_progress_bar()
        else:
            self.view.show_fit_browser()  # if no data is plotted, the fit browser will fail to show
            if self.view.is_fit_browser_visible():
                self.view.show_fit_progress_bar()
        self.set_progress_bar_zero()

    # ==============================
    # Indeterminate Fit Progress Bar
    # ==============================

    def set_final_state_progress_bar(self, output_list, status=None):
        if not status:
            status = output_list[0]['status'].lower()
        if "success" in status:
            self.set_progress_bar_success(status)
        else:
            self.set_progress_bar_failed(status)

    def set_progress_bar_failed(self, status):
        self.view.set_progress_bar(status=status, minimum=0, maximum=100, value=0, style_sheet=FAILED_STYLE_SHEET)

    def set_progress_bar_success(self, status):
        self.view.set_progress_bar(status=status, minimum=0, maximum=100, value=100, style_sheet=SUCCESS_STYLE_SHEET)

    def set_progress_bar_to_in_progress(self):
        # indeterminate progress bar
        self.view.set_progress_bar(status="fitting...", minimum=0, maximum=0, value=0,
                                   style_sheet=IN_PROGRESS_STYLE_SHEET)

    def set_progress_bar_zero(self):
        self.view.set_progress_bar(status="", minimum=0, maximum=100, value=0, style_sheet=EMPTY_STYLE_SHEET)
