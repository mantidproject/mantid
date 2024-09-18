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
from mantidqt.utils.asynchronous import AsyncTask
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
        self.fit_all_started_notifier = GenericObservable()
        self.fit_all_done_notifier = GenericObservable()
        self.find_peaks_convolve_started_notifier = GenericObservable()
        self.find_peaks_convolve_done_notifier = GenericObservable()
        self.setup_toolbar()
        self.worker = None
        self.fitprop_list = None
        self.is_waiting_convolve_peaks = False
        self.view.set_subscriber_for_function_changed(self.handle_convolve_peaks_added)

    def setup_toolbar(self):
        self.view.set_slot_for_display_all()
        self.view.set_slot_for_fit_toggled(self.fit_toggle)
        self.view.set_slot_for_serial_fit(self.do_serial_fit)
        self.view.set_slot_for_seq_fit(self.do_seq_fit)
        self.view.set_slot_for_legend_toggled()
        self.view.set_slot_for_find_peaks_convolve(self.run_find_peaks_convolve)

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
        if len(self.model.get_plotted_workspaces()) == 0:
            self.view.set_find_peaks_convolve_button_status(False)

    def clear_plot(self):
        for ax in self.view.get_axes():
            self.model.remove_all_workspaces_from_plot(ax)
        self.view.clear_figure()
        self.view.update_fitbrowser()
        self.set_progress_bar_zero()
        self.view.set_find_peaks_convolve_button_status(False)

    def on_cancel_clicked(self):
        if self.worker:
            self.abort_worker()
            logger.warning("Fit cancelled therefore the fitpropertybrowser and output fit results may be out of sync.")
            self.fitprop_list = None
            self._finished()

    def abort_worker(self):
        self.worker.abort(interrupt=False)

    def enable_view_components(self, enable: bool):
        # enable / disable all widgets apart from the cancel button
        self.view.fit_browser.setEnabled(enable)
        self.view.dock_window.setEnabled(enable)
        self.view.plot_dock.setEnabled(enable)

    def update_browser(self):
        status = None
        function_string = None
        ws_name = None
        if self.fitprop_list:
            # update last fit in fit browser and save setup
            fitprop = self.fitprop_list[-1]
            function_string = fitprop["properties"]["Function"]
            status = fitprop["status"]
            ws_name = fitprop["properties"]["InputWorkspace"]
        self.view.update_browser(status=status, func_str=function_string, setup_name=ws_name)

    def do_serial_fit(self):
        self.fit_all_started_notifier.notify_subscribers(False)

    def do_seq_fit(self):
        self.fit_all_started_notifier.notify_subscribers(True)

    def run_find_peaks_convolve(self):
        self.is_waiting_convolve_peaks = False
        self.find_peaks_convolve_started_notifier.notify_subscribers()
        try:
            input_ws_name = self.view.fit_browser.workspaceName()
            func_wrapper_str = self.model.run_find_peaks_convolve(
                input_ws_name, self.view.fit_browser.defaultPeakType(), (self.view.fit_browser.startX(), self.view.fit_browser.endX())
            )
            if func_wrapper_str is not None:
                self.is_waiting_convolve_peaks = True
                self.view.fit_browser.loadFunction(func_wrapper_str)
            else:
                self.is_waiting_convolve_peaks = False
                self.find_peaks_convolve_done_notifier.notify_subscribers(False)
        except RuntimeError as err:
            logger.error(f"Failed to run FindPeaksConvolve for workspace:{input_ws_name}! Error:{err}")
            self.find_peaks_convolve_done_notifier.notify_subscribers(False)

    def do_fit_all_async(self, ws_names_list, do_sequential=True):
        previous_fit_browser = self.view.read_fitprop_from_browser()
        self.worker = AsyncTask(
            self.do_fit_all,
            (previous_fit_browser, ws_names_list, do_sequential),
            success_cb=self._on_worker_success,
            error_cb=self._on_worker_error,
            finished_cb=self._finished,
        )
        self.worker.start()

    def fit_completed(self, fit_props, loaded_ws_list, active_ws_list, log_workspace_name):
        if fit_props:
            self.model.update_fit(fit_props, loaded_ws_list, active_ws_list, log_workspace_name)

    # =======================
    # Fit Asynchronous Thread
    # =======================

    def _on_worker_success(self, async_success):
        self.fitprop_list = async_success.output

    def _on_worker_error(self, async_failure=None):
        error_message = str(async_failure)
        if "KeyboardInterrupt" not in error_message:
            logger.error(str(async_failure))

    def _finished(self, _=None):
        self.fit_all_done_notifier.notify_subscribers(self.fitprop_list)

    def do_fit_all(self, previous_fitprop, ws_name_list, do_sequential=True):
        prev_fitprop = previous_fitprop
        fitprop_list = []
        for ws_name in ws_name_list:
            logger.notice(f"Starting to fit workspace {ws_name}")
            fitprop = deepcopy(prev_fitprop)
            # update I/O workspace name
            fitprop["properties"]["Output"] = ws_name
            fitprop["properties"]["InputWorkspace"] = ws_name
            # do fit
            fit_output = Fit(**fitprop["properties"])
            # update results
            fitprop["status"] = fit_output.OutputStatus
            funcstr = str(fit_output.Function.fun)
            fitprop["properties"]["Function"] = funcstr
            if "success" in fitprop["status"].lower() and do_sequential:
                # update function in prev fitprop to use for next workspace
                prev_fitprop["properties"]["Function"] = funcstr
            # append a deep copy to output list (will be initial parameters if not successful)
            fitprop_list.append(fitprop)

        logger.notice(f'{"Serial" if not do_sequential else "Sequential"} fitting finished.')
        return fitprop_list

    # ==============================
    # Indeterminate Fit Progress Bar
    # ==============================

    def fit_toggle(self):
        """Toggle fit browser and tool on/off"""
        if self.view.is_fit_browser_visible():
            self.view.hide_fit_browser()
            self.view.hide_fit_progress_bar()
            self.view.set_find_peaks_convolve_button_status(False)
        else:
            self.view.show_fit_browser()  # if no data is plotted, the fit browser will fail to show
            if self.view.is_fit_browser_visible():
                self.view.show_fit_progress_bar()
                self.view.set_find_peaks_convolve_button_status(True)
        self.set_progress_bar_zero()

    def update_progress_bar(self):
        if self.fitprop_list:
            self.set_final_state_progress_bar(output_list=self.fitprop_list)
        else:
            self.set_progress_bar_zero()

    def set_final_state_progress_bar(self, output_list, status=None):
        if not status:
            status = output_list[-1]["status"].lower()
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
        self.view.set_progress_bar(status="fitting...", minimum=0, maximum=0, value=0, style_sheet=IN_PROGRESS_STYLE_SHEET)

    def set_progress_bar_zero(self):
        self.view.set_progress_bar(status="", minimum=0, maximum=100, value=0, style_sheet=EMPTY_STYLE_SHEET)

    def handle_convolve_peaks_added(self):
        if self.is_waiting_convolve_peaks:
            self.is_waiting_convolve_peaks = False
            self.find_peaks_convolve_done_notifier.notify_subscribers(True)
