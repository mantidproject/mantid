# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

from mantidqt.utils.observer_pattern import GenericObserverWithArgPassing
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.fitting_ads_observer import FittingADSObserver


class GSAS2Presenter(object):
    def __init__(self, model, view, test=False):
        self.model = model
        self.view = view

        self.rb_num = None
        self.instrument = "ENGINX"
        self.current_plot_index = None
        self.latest_load_parameters = None
        self.focus_run_observer_gsas2 = GenericObserverWithArgPassing(self.view.set_default_gss_files)
        self.prm_filepath_observer_gsas2 = GenericObserverWithArgPassing(self.view.set_default_prm_files)
        self.ads_observer = FittingADSObserver(self.remove_workspace, self.clear_workspaces, self.replace_workspace, self.rename_workspace)
        if not test:
            self.connect_view_signals()

    def connect_view_signals(self):
        self.view.set_refine_clicked(self.on_refine_clicked)
        self.view.number_output_histograms_combobox.currentTextChanged.connect(self.on_plot_index_changed)

    def on_refine_clicked(self):
        self.clear_plot()
        load_params = self.view.get_load_parameters()
        project_name = self.view.get_project_name()
        refine_params = self.view.get_refinement_parameters()
        number_output_histograms = self.model.run_model(
            load_params, refine_params, project_name, self.rb_num, self.get_limits_if_same_load_parameters()
        )
        if number_output_histograms:
            self.plot_result(1)
            self.view.set_number_histograms(number_output_histograms)
            self.save_latest_load_parameters()
            self.model.load_focused_nxs_for_logs(load_params[2])

    def on_plot_index_changed(self, new_plot_index):
        if new_plot_index:
            if int(new_plot_index) != int(self.current_plot_index):
                self.plot_result(new_plot_index)

    def get_limits_if_same_load_parameters(self):
        new_load_params = self.view.get_load_parameters()
        if not new_load_params:
            return None
        if new_load_params != self.latest_load_parameters:
            return None
        current_limits = self.view.get_x_limits_from_line_edits()
        if current_limits:
            # Check current_limits != self.view.initial_x_limits upto 2 decimal places
            if not np.allclose(np.sort(np.array(current_limits, dtype=float), axis=None), self.view.initial_x_limits, atol=1e-2):
                return np.sort(np.array(current_limits, dtype=float), axis=0).tolist()
        return None

    # =================
    # Component Setters
    # =================

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def save_latest_load_parameters(self):
        self.latest_load_parameters = self.view.get_load_parameters()

    # ========
    # Plotting
    # ========

    def plot_result(self, output_histogram_index):
        self.clear_plot()
        axes = self.view.get_axes()
        plot_window_title = ""
        for ax in axes:
            plot_window_title = self.model.plot_result(int(output_histogram_index), ax)
        self.view.update_figure(plot_window_title)
        self.current_plot_index = output_histogram_index
        self.set_x_limits(output_histogram_index)

    def clear_plot(self):
        self.view.clear_figure()
        self.current_plot_index = None

    def set_x_limits(self, current_histogram_index):
        x_minimum = self.model.x_min[int(current_histogram_index) - 1]
        x_maximum = self.model.x_max[int(current_histogram_index) - 1]
        self.view.set_x_limits(x_minimum, x_maximum)

    # ========================
    # Sample Logs ADS observer
    # ========================

    def remove_workspace(self, ws_name):
        if ws_name in self.model.get_all_workspace_names():
            self.model.remove_workspace(ws_name)
        elif ws_name in self.model.get_log_workspaces_name():
            self.model.update_sample_log_workspace_group()

    def rename_workspace(self, old_name, new_name):
        # Note - ws.name() not updated yet so need to rely on new_name parameter
        # Also Note - ADS rename is always associated with a ADS replace so
        # rely on the replace to _repopulate_table. Prefer not to call twice
        # to avoid issue with legend entries doubling up
        if old_name in self.model.get_all_workspace_names():
            self.model.update_workspace_name(old_name, new_name)

    # handle ADS clear
    def clear_workspaces(self):
        self.model.clear_workspaces()

    def replace_workspace(self, name, workspace):
        if name in self.model.get_all_workspace_names():
            self.model.replace_workspace(name, workspace)
