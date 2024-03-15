# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_widget import FittingDataWidget
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_presenter import FittingPlotPresenter
from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing, GenericObserver


class FittingPresenter(object):
    def __init__(self, view):
        self.view = view
        self.data_widget = FittingDataWidget(self.view, view=self.view.get_data_widget())
        self.plot_widget = FittingPlotPresenter(self.view, view=self.view.get_plot_widget())

        # Plot added/removed observer/notifiers
        self.data_widget.presenter.plot_removed_notifier.add_subscriber(self.plot_widget.workspace_removed_observer)
        self.data_widget.presenter.plot_added_notifier.add_subscriber(self.plot_widget.workspace_added_observer)
        self.data_widget.presenter.all_plots_removed_notifier.add_subscriber(self.plot_widget.all_workspaces_removed_observer)

        # Fit started observer/notifiers
        self.fit_all_started_observer = GenericObserverWithArgPassing(self.fit_all_started)
        self.plot_widget.fit_all_started_notifier.add_subscriber(self.fit_all_started_observer)

        self.fit_started_observer = GenericObserver(self.fit_started)
        self.plot_widget.view.fit_browser.fit_started_notifier.add_subscriber(self.fit_started_observer)

        # Fit ended observer/notifiers
        self.fit_all_done_observer = GenericObserverWithArgPassing(self.fit_all_done)
        self.plot_widget.fit_all_done_notifier.add_subscriber(self.fit_all_done_observer)

        self.fit_complete_observer = GenericObserverWithArgPassing(self.fit_done)
        self.plot_widget.view.fit_browser.fit_notifier.add_subscriber(self.fit_complete_observer)

        self.find_peaks_convolve_started_observer = GenericObserver(self.find_peaks_convolve_started)
        self.plot_widget.find_peaks_convolve_started_notifier.add_subscriber(self.find_peaks_convolve_started_observer)

        self.find_peaks_convolve_completed_observer = GenericObserverWithArgPassing(self.find_peaks_convolve_completed)
        self.plot_widget.find_peaks_convolve_done_notifier.add_subscriber(self.find_peaks_convolve_completed_observer)

        self.connect_view_signals()

    def fit_all_started(self, do_sequential):
        if do_sequential:
            ws_name_list = self.data_widget.get_sorted_active_ws_list()
        else:
            ws_name_list = self.data_widget.get_active_ws_list()
        # "all" refers to sequential/serial fit triggered in the data widget
        self.plot_widget.set_progress_bar_to_in_progress()
        self.disable_view(fit_all=True)
        self.plot_widget.do_fit_all_async(ws_name_list, do_sequential)

    def fit_all_done(self, fit_props):
        # "all" refers to sequential/serial fit triggered in the data widget
        self.plot_widget.update_browser()
        self.plot_widget.update_progress_bar()
        self.enable_view(fit_all=True)
        self.plot_widget.fit_completed(
            fit_props,
            self.data_widget.presenter.get_loaded_ws_list(),
            self.data_widget.presenter.get_active_ws_list(),
            self.data_widget.presenter.get_log_ws_group_name(),
        )

    def fit_started(self):
        # triggered in the fit property browser
        self.plot_widget.set_progress_bar_to_in_progress()
        self.disable_view()

    def fit_done(self, fit_props):
        # triggered in the fit property browser
        self.enable_view()
        if fit_props:
            self.plot_widget.set_final_state_progress_bar(fit_props)
            self.plot_widget.fit_completed(
                fit_props,
                self.data_widget.presenter.get_loaded_ws_list(),
                self.data_widget.presenter.get_active_ws_list(),
                self.data_widget.presenter.get_log_ws_group_name(),
            )
        else:
            self.plot_widget.set_final_state_progress_bar(None, status="Failed, invalid fit.")

    def find_peaks_convolve_started(self):
        self.plot_widget.set_progress_bar_to_in_progress()
        self.disable_view()

    def find_peaks_convolve_completed(self, is_success):
        if is_success:
            self.plot_widget.set_progress_bar_success(status="success")
        else:
            self.plot_widget.set_progress_bar_failed(status="failed")
        self.enable_view()

    def disable_view(self, _=None, fit_all=False):
        self.data_widget.view.setEnabled(False)
        self.plot_widget.enable_view_components(False)
        if fit_all:
            self.plot_widget.view.show_cancel_button(True)

    def enable_view(self, _=None, fit_all=False):
        self.data_widget.view.setEnabled(True)
        self.plot_widget.enable_view_components(True)
        if fit_all:
            self.plot_widget.view.show_cancel_button(False)

    def connect_view_signals(self):
        self.plot_widget.view.set_cancel_clicked(self.on_cancel_clicked)

    def on_cancel_clicked(self):
        self.plot_widget.on_cancel_clicked()
        self.enable_view()
        self.plot_widget.set_progress_bar_zero()
