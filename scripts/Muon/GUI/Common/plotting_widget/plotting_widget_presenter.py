# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'
FREQ_PLOT_TYPE = "Frequency "


class PlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model, context):
        """
        :param view: A reference to the QWidget object for plotting
        :param model: A refence to a model which contains the plotting logic
        :param context: A reference to the Muon context object
        """
        self._view = view
        self._model = model
        self.context = context
        self._view.on_plot_button_clicked(self.handle_data_updated)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self._view.on_plot_type_changed(self.handle_plot_type_changed)

        self.input_workspace_observer = GenericObserver(self.handle_data_updated)
        self.fit_observer = GenericObserver(self.handle_fit_completed)
        self.group_pair_observer = GenericObserver(self.handle_group_pair_to_plot_changed)
        self.plot_type_observer = GenericObserver(self.handle_group_pair_to_plot_changed)
        self.rebin_options_set_observer = GenericObserver(self.handle_rebin_options_set)
        self.plot_guess_observer = GenericObserver(self.handle_plot_guess_changed)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)

        self.plot_type_changed_notifier = GenericObservable()

        self._force_redraw = False
        if self.context._frequency_context:
            for ext in FREQUENCY_EXTENSIONS.keys():
                self._view.addItem(FREQ_PLOT_TYPE + FREQUENCY_EXTENSIONS[ext])
            self._view.addItem(FREQ_PLOT_TYPE + "All")
        self.instrument_observer = GenericObserver(self.handle_instrument_changed)
        self.keep = False

    def show(self):
        """
        Calls show on the view QtWidget
        """
        self._view.show()

    def update_view_from_model(self):
        """
        This is required in order for this presenter to match the base class interface
        """
        pass

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing. Checks whether the list of workspaces has changed before doing
        anything as workspaces being modified in place is handled by the ADS handler observer.
        """
        if self._model.plotted_workspaces == self.get_workspaces_to_plot(
                self.context.group_pair_context.selected,
                self._view.if_raw(), self._view.get_selected()):
            # If the workspace names have not changed the ADS observer should
            # handle any updates
            return

        self.plot_standard_workspaces()

    def handle_workspace_deleted_from_ads(self, workspace):
        if workspace in self._model.plotted_workspaces:
            self._model.remove_workspace_from_plot_by_name(workspace)

    def handle_workspace_replaced_in_ads(self, workspace):
        if not self._view.if_raw():
            if workspace in self._model.plotted_workspaces:
                self.plot_standard_workspaces()

    def handle_use_raw_workspaces_changed(self):
        """
        Handles the use raw workspaces being changed. If
        """
        if not self._view.if_raw() and not self.context._do_rebin():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup('No rebin options specified')
            return

        self.plot_standard_workspaces()

    def handle_plot_type_changed(self):
        """
        Handles the plot type being changed on the view
        """
        current_group_pair = self.context.group_pair_context[
            self.context.group_pair_context.selected]
        current_plot_type = self._view.get_selected()
        if isinstance(current_group_pair, MuonPair) and current_plot_type == COUNTS_PLOT_TYPE:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(ASYMMETRY_PLOT_TYPE)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace')
            return

        # force the plot to update
        self._force_redraw = True
        if self.context._frequency_context:
            self.context._frequency_context.plot_type = self._view.get_selected()[len(FREQ_PLOT_TYPE):]
        self.plot_type_changed_notifier.notify_subscribers()

        self.plot_standard_workspaces()

    def handle_group_pair_to_plot_changed(self):
        """
        Handles the selected group pair being changed on the view
        """
        if self.context.group_pair_context.selected == self._model.plotted_group:
            return
        self._model.plotted_group = self.context.group_pair_context.selected

        if not self._model.plot_figure:
            return

        self.plot_standard_workspaces()

    def plot_standard_workspaces(self):
        """
        Plots the standard list of workspaces in a new plot window, closing any existing plot windows.
        :return:
        """
        workspace_list = self.get_workspaces_to_plot(
            self.context.group_pair_context.selected, self._view.if_raw(),
            self._view.get_selected())

        self._model.plot(workspace_list, self.get_plot_title(), self.get_domain(),
                         self.context.window_title)

        # if we are redrawing, keep previous fit
        if self._force_redraw:
            self.handle_fit_completed()
        else:
            self.context.fitting_context.clear()
        self.handle_plot_guess_changed()
        # reset redraw flag
        self._force_redraw = False

        self._model.plotted_workspaces_inverse_binning = {
            workspace: self.context.group_pair_context.get_equivalent_group_pair(workspace)
            for workspace in workspace_list
            if self.context.group_pair_context.get_equivalent_group_pair(workspace)}

    def handle_fit_completed(self):
        """
        When a new fit is done adds the fit to the plotted workspaces if appropriate
        :return:
        """

        if self._model.plot_figure is None:
            return

        label = self._model.plot_figure.gca().get_ylabel()

        if self.context.fitting_context.number_of_fits <= 1:
            for workspace_name in self._model.plotted_fit_workspaces:
                self._model.remove_workpace_from_plot(workspace_name)
            self._model.plot_figure.gca().legend(prop=dict(size=7))

        if self.context.fitting_context.fit_list:
            current_fit = self.context.fitting_context.fit_list[-1]
            combined_ws_list = self._model.plotted_workspaces + list(
                self._model.plotted_workspaces_inverse_binning.values())
            list_of_output_workspaces_to_plot = [output for output, input in
                                                 zip(current_fit.output_workspace_names, current_fit.input_workspaces)
                                                 if input in combined_ws_list]
            list_of_output_workspaces_to_plot = list_of_output_workspaces_to_plot if list_of_output_workspaces_to_plot \
                else [current_fit.output_workspace_names[-1]]
        else:
            list_of_output_workspaces_to_plot = []

        for workspace_name in list_of_output_workspaces_to_plot:
            self._model.add_workspace_to_plot(workspace_name, 1, workspace_name + ': Fit')
            self._model.add_workspace_to_plot(workspace_name, 2, workspace_name + ': Diff')
        self._model.plot_figure.gca().set_ylabel(label)

        self._model.force_redraw()

    def get_workspaces_to_plot(self, current_group_pair, is_raw, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        if FREQ_PLOT_TYPE in plot_type:
            return self.get_freq_workspaces_to_plot(current_group_pair, plot_type)
        else:
            return self.get_time_workspaces_to_plot(current_group_pair, is_raw, plot_type)

    def get_freq_workspaces_to_plot(self, current_group_pair, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            runs = ""
            seperator = ""
            for run in self.context.data_context.current_runs:
                runs += seperator + str(run[0])
                seperator = ", "
            workspace_list = self.context.get_names_of_frequency_domain_workspaces_to_fit(
                runs, current_group_pair, True, plot_type[len(FREQ_PLOT_TYPE):])

            return workspace_list
        except AttributeError:
            return []

    def get_time_workspaces_to_plot(self, current_group_pair, is_raw, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names(
                    self.context.data_context.current_runs)
            else:
                workspace_list = self.context.group_pair_context[
                    current_group_pair].get_asymmetry_workspace_names_rebinned(
                    self.context.data_context.current_runs)

            if plot_type == COUNTS_PLOT_TYPE:
                workspace_list = [item.replace(ASYMMETRY_PLOT_TYPE, COUNTS_PLOT_TYPE)
                                  for item in workspace_list if ASYMMETRY_PLOT_TYPE in item]

            return workspace_list
        except AttributeError:
            return []

    def get_plot_title(self):
        """
        Generates a title for the plot based on current instrument group and run numbers
        :return:
        """
        flattened_run_list = [
            item for sublist in self.context.data_context.current_runs for item in sublist]
        return self.context.data_context.instrument + ' ' + run_list_to_string(flattened_run_list) + ' ' + \
            self.context.group_pair_context.selected

    def handle_rebin_options_set(self):
        if self.context._do_rebin():
            self._view.set_raw_checkbox_state(False)
        else:
            self._view.set_raw_checkbox_state(True)

    def get_domain(self):
        if FREQ_PLOT_TYPE in self._view.get_selected():
            return "Frequency"
        else:
            return "Time"

    def handle_instrument_changed(self):
        if self._model.plot_figure is not None:
            from matplotlib import pyplot as plt
            plt.close(self._model.plot_figure)

    def handle_plot_guess_changed(self):
        if self._model.plot_figure is None:
            return

        for guess in [ws for ws in self._model.plotted_fit_workspaces if '_guess' in ws]:
            self._model.remove_workpace_from_plot(guess)
            # refresh legend
            self._model.plot_figure.gca().legend(prop=dict(size=7))

        if self.context.fitting_context.plot_guess and self.context.fitting_context.guess_ws is not None:
            self._model.add_workspace_to_plot(self.context.fitting_context.guess_ws,
                                              workspace_index=1,
                                              label='Fit Function Guess')
        self._model.force_redraw()
