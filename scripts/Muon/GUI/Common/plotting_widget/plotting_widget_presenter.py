# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import re
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS
from Muon.GUI.Common.plotting_widget.workspace_finder import WorkspaceFinder

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'
FREQ_PLOT_TYPE = "Frequency "
workspace_index = 0
default_xlimits = {"Frequency": [0, 50], "Time": [0, 15]}


class PlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model, context):
        """
        :param view: A reference to the QWidget object for plotting
        :param model: A reference to a model which contains the plotting logic
        :param context: A reference to the Muon context object
        """
        self._view = view
        self._model = model
        self.context = context
        self.workspace_finder = WorkspaceFinder(self.context)

        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self._view.on_plot_type_changed(self.handle_plot_type_changed)
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_changed_on_view)
        self._view.on_plot_tiled_changed(self.handle_plot_tiled_changed)

        self.input_workspace_observer = GenericObserver(self.handle_data_updated)
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)
        self.fit_observer = GenericObserver(self.handle_fit_completed)
        self.removed_group_pair_observer = GenericObserver(self.handle_removed_group_or_pair_to_plot)
        self.rebin_options_set_observer = GenericObserver(self.handle_rebin_options_set)
        self.plot_guess_observer = GenericObserver(self.handle_plot_guess_changed)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self.plot_type_changed_notifier = GenericObservable()

        self.connect_xlim_changed_in_figure_view(self.handle_x_axis_limits_changed_in_figure_view)
        self.connect_ylim_changed_in_figure_view(self.handle_y_axis_limits_changed_in_figure_view)
        self._view.plot_options.connect_errors_changed(self.handle_error_selection_changed)
        self._view.plot_options.connect_x_range_changed(self.handle_xlim_changed_in_options_view)
        self._view.plot_options.connect_y_range_changed(self.handle_ylim_changed_in_options_view)
        self._view.plot_options.connect_autoscale_changed(self.handle_autoscale_requested_in_view)
        self._view.plot_options.connect_plot_selection(self.handle_subplot_changed_in_options)

        if self.context._frequency_context:
            for ext in FREQUENCY_EXTENSIONS.keys():
                self._view.addItem(FREQ_PLOT_TYPE + FREQUENCY_EXTENSIONS[ext])
            self._view.addItem(FREQ_PLOT_TYPE + "All")
        self.instrument_observer = GenericObserver(self.handle_instrument_changed)

    def show(self):
        """
        Calls show on the view QtWidget
        """
        self._view.show()

    def update_view_from_model(self):
        """
        Needed so we can match the base class interface
        """
        pass

    def update_options_view_from_model(self):
        """
        Updates the view from the model, which includes
        axis limits and subplot list
        """
        xmin, xmax = self.get_x_lim_from_subplot(0)
        ymin, ymax = self.get_y_lim_from_subplot(0)
        self._view.plot_options.set_plot_x_range([xmin, xmax])
        self._view.plot_options.set_plot_y_range([ymin, ymax])

        self._view.plot_options.clear_subplots()
        for i in range(self._model.number_of_axes):
            self._view.plot_options.add_subplot(str(i + 1))

    # ------------------------------------------------------------------------------------------------------------------
    # Handle user making plotting related changes from GUI
    # ------------------------------------------------------------------------------------------------------------------

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing. Checks whether the list of workspaces has changed before doing
        anything as workspaces being modified in place is handled by the ADS handler observer.
        """
        if self._model.plotted_workspaces == \
                self.workspace_finder.get_workspace_list_to_plot(self._view.if_raw(), self._view.get_selected()):
            # If the workspace names have not changed the ADS observer should
            # handle any updates
            return

        if self._view.is_tiled_plot():
            self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)

        self.plot_all_selected_workspaces()

    def handle_workspace_deleted_from_ads(self, workspace):
        """
        Handles the workspace being deleted from ads
        """
        if self._view.is_tiled_plot():
            self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)
            self.plot_all_selected_workspaces()
            return

        if workspace.name() in self._model.plotted_workspaces:
            self._model.workspace_deleted_from_ads(workspace, self._view.get_axes())
            self._view.set_fig_titles(self.get_plot_title())

            self._model.autoscale_axes(self._view.get_axes(), self.get_x_limits())
            self._view.force_redraw()

    def handle_workspace_replaced_in_ads(self, workspace):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in ads
        """
        if workspace in self._model.plotted_workspaces:
            ax = self.get_workspace_plot_axis(workspace)
            self._model.replace_workspace_plot(workspace, ax)
            self._model.autoscale_axes(self._view.get_axes(), self.get_x_limits())
            self._view.force_redraw()

    def handle_use_raw_workspaces_changed(self):
        """
        Handles the use raw workspaces being changed.
        """
        if not self._view.if_raw() and not self.context._do_rebin():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup('No rebin options specified')
            return
        self.plot_all_selected_workspaces()

    def handle_plot_type_changed(self):
        """
        Handles the plot type being changed on the view
        """
        current_plot_type = self._view.get_selected()
        if len(self.context.group_pair_context.selected_pairs) != 0 and current_plot_type == COUNTS_PLOT_TYPE:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(ASYMMETRY_PLOT_TYPE)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, remove pairs from analysis and retry')
            return
        if self.context._frequency_context:
            self.context._frequency_context.plot_type = self._view.get_selected()[len(FREQ_PLOT_TYPE):]
        self.plot_type_changed_notifier.notify_subscribers()
        self._model.clear_plot_model(self._view.get_axes())
        self.plot_all_selected_workspaces()

    def handle_error_selection_changed(self, error_state):
        """
        Handles error checkbox being changed on the view
        """
        for workspace in self._model.plotted_workspaces:
            ax = self.get_workspace_plot_axis(workspace)
            label = self.get_workspace_legend_label(workspace)
            plot_kwargs = {'distribution': True, 'autoscale_on_update': False, 'label': label}
            self._model.replot_workspace(workspace, ax, error_state, plot_kwargs)

        self._view.force_redraw()

    def handle_xlim_changed_in_options_view(self, xlims):
        selected_axes = self.get_selected_axes()
        for ax in selected_axes:
            self._model.set_axis_xlim(ax, xlims)
        self._view.force_redraw()

    def handle_ylim_changed_in_options_view(self, ylims):
        selected_axes = self.get_selected_axes()
        for ax in selected_axes:
            self._model.set_axis_ylim(ax, ylims)
        self._view.force_redraw()

    def handle_autoscale_requested_in_view(self):
        subplots = self._view.plot_options.get_selection()
        self._view.update_toolbar()
        if len(subplots) > 0:
            xlims = self._view.plot_options.get_plot_x_range()
            indices = [ix - 1 for ix in list(map(int, subplots))]
            axes = [self._view.get_axes()[index] for index in indices]

            ymin, ymax = self._model.autoscale_axes(axes, xlims)
            self._view.plot_options.set_plot_y_range([ymin, ymax])
            self._view.force_redraw()

    def handle_x_axis_limits_changed_in_figure_view(self, axis):
        subplots = self._view.plot_options.get_selection()
        if len(subplots) > 1 or len(subplots) == 0:
            return
        if subplots[0].isdigit():
            if axis == self._view.get_axes()[int(subplots[0]) - 1]:
                xmin, xmax = axis.get_xlim()
                self._view.plot_options.set_plot_x_range([xmin, xmax])

    def handle_y_axis_limits_changed_in_figure_view(self, axis):
        subplots = self._view.plot_options.get_selection()
        if len(subplots) > 1 or len(subplots) == 0:
            return
        if subplots[0].isdigit():
            if axis == self._view.get_axes()[int(subplots[0]) - 1]:
                ymin, ymax = axis.get_ylim()
                self._view.plot_options.set_plot_y_range([ymin, ymax])

    def handle_subplot_changed_in_options(self):
        subplots = self._view.plot_options.get_selection()
        if len(subplots) == 0 or len(subplots) > 1:
            return
        if subplots[0].isdigit():
            index = int(subplots[0]) - 1
            xmin, xmax = self.get_x_lim_from_subplot(index)
            ymin, ymax = self.get_y_lim_from_subplot(index)

            self._view.plot_options.set_plot_x_range([xmin, xmax])
            self._view.plot_options.set_plot_y_range([ymin, ymax])

    def handle_plot_tiled_changed(self, state):
        """
        Handles the plot tiled checkbox being changed in the view
        """
        if state == 2:  # tiled plot
            self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)
            self.plot_all_selected_workspaces()
        if state == 0:  # not tiled plot
            self.new_plot_figure(num_axes=1)
            self._model.number_of_axes = 1
            self.plot_all_selected_workspaces()

        self.connect_xlim_changed_in_figure_view(self.handle_x_axis_limits_changed_in_figure_view)
        self.connect_ylim_changed_in_figure_view(self.handle_y_axis_limits_changed_in_figure_view)

    def handle_added_or_removed_group_or_pair_to_plot(self, is_added):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        """
        if is_added:
            self.handle_added_group_or_pair_to_plot()
        else:
            self.handle_removed_group_or_pair_to_plot()

    def handle_added_group_or_pair_to_plot(self):
        """
        Handles a group or pair being added from the view
        """
        if len(self.context.group_pair_context.selected_pairs) != 0 and self._view.get_selected() == COUNTS_PLOT_TYPE:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(ASYMMETRY_PLOT_TYPE)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')

        # if tiled by group, we will need to recreate the tiles
        if self._view.is_tiled_plot() and self._view.get_tiled_by_type() == 'group':
            self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)
            self.plot_all_selected_workspaces()
            self.connect_xlim_changed_in_figure_view(self.handle_x_axis_limits_changed_in_figure_view)
            self.connect_ylim_changed_in_figure_view(self.handle_y_axis_limits_changed_in_figure_view)
        else:  # add plots to existing axes
            self.plot_all_selected_workspaces()

        return

    def handle_removed_group_or_pair_to_plot(self):
        """
        Handles a group or pair being removed in grouping widget analysis table
        """
        if self._view.is_tiled_plot() and self._view.get_tiled_by_type():
            self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)
            self.plot_all_selected_workspaces()
            self.connect_xlim_changed_in_figure_view(self.handle_x_axis_limits_changed_in_figure_view)
            self.connect_ylim_changed_in_figure_view(self.handle_y_axis_limits_changed_in_figure_view)
            return

        workspace_list = self.workspace_finder.get_workspace_list_to_plot(self._view.if_raw(),
                                                                          self._view.get_selected())
        for ws in self._model.plotted_workspaces:
            if ws not in workspace_list:
                self._model.remove_workspace_from_plot(ws, self._view.get_axes())

        self._model.autoscale_axes(self._view.get_axes(), self.get_x_limits())
        self._view.force_redraw()

    def handle_fit_completed(self):
        """
        When a new fit is done adds the fit to the plotted workspaces if appropriate
        """
        if self.context.fitting_context.number_of_fits <= 1:
            for workspace_name in self._model.plotted_fit_workspaces:
                self._model.remove_workspace_from_plot(workspace_name, self._view.get_axes())

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
            label = self.get_workspace_legend_label(workspace_name)
            ax = self.get_workspace_plot_axis(workspace_name)
            fit_function = current_fit.fit_function_name

            self._model.add_workspace_to_plotted_fit_workspaces(workspace_name)
            self._model.add_workspace_to_plot(ax, workspace_name, [1],
                                              errors=False,
                                              plot_kwargs={'distribution': True, 'autoscale_on_update': False,
                                                           'label': label + fit_function + ': Fit'})
            self._model.add_workspace_to_plot(ax, workspace_name, [2],
                                              errors=False,
                                              plot_kwargs={'distribution': True, 'autoscale_on_update': False,
                                                           'label': label + fit_function + ': Diff'})
        self._view.force_redraw()

    def handle_rebin_options_set(self):
        if self.context._do_rebin():
            self._view.set_raw_checkbox_state(False)
        else:
            self._view.set_raw_checkbox_state(True)

    def handle_tiled_by_changed_on_view(self, index):
        if self._view.is_tiled_plot():
            if index == 0:
                self.update_model_tile_plot_positions()
            else:
                self.update_model_tile_plot_positions()
            self.new_plot_figure(self._model.number_of_axes)
            self.plot_all_selected_workspaces()
            self.connect_xlim_changed_in_figure_view(self.handle_x_axis_limits_changed_in_figure_view)
            self.connect_ylim_changed_in_figure_view(self.handle_y_axis_limits_changed_in_figure_view)

    def handle_instrument_changed(self):
        self.new_plot_figure(num_axes=1)

    def handle_plot_guess_changed(self):

        for guess in [ws for ws in self._model.plotted_fit_workspaces if '_guess' in ws]:
            self._model.remove_workspace_from_plot(guess, self._view.get_axes())

        if self.context.fitting_context.plot_guess and self.context.fitting_context.guess_ws is not None:
            self._model.add_workspace_to_plotted_fit_workspaces(self.context.fitting_context.guess_ws)
            for i in range(self._model.number_of_axes):
                self._model.add_workspace_to_plot(self._view.get_axes()[i], self.context.fitting_context.guess_ws,
                                                  workspace_indices=[1], errors=False,
                                                  plot_kwargs={'distribution': True, 'autoscale_on_update': False,
                                                               'label': 'Fit Function Guess'})
        self._view.force_redraw()

    # Every time a new figure is generated these signals will have
    # to be reconnected (e.g switching to tiled plotting).
    def connect_xlim_changed_in_figure_view(self, slot):
        self._view.on_x_lims_changed_in_view(slot)

    def connect_ylim_changed_in_figure_view(self, slot):
        self._view.on_y_lims_changed_in_view(slot)

    # ------------------------------------------------------------------------------------------------------------------
    # Plotting controls
    # ------------------------------------------------------------------------------------------------------------------

    def plot_all_selected_workspaces(self):
        """
        Plots all the selected workspaces (from grouping tab) in the plot window,
        clearing any previous plots
        """

        errors = self._view.plot_options.get_errors()
        workspace_list = self.workspace_finder.get_workspace_list_to_plot(self._view.if_raw(),
                                                                          self._view.get_selected())

        for ws in self._model.plotted_workspaces:
            if ws not in workspace_list:
                self._model.remove_workspace_from_plot(ws, self._view.get_axes())

        for workspace in workspace_list:
            if workspace not in self._model.plotted_workspaces:
                ax = self.get_workspace_plot_axis(workspace)
                label = self.get_workspace_legend_label(workspace)
                self._model.add_workspace_to_plot(ax, workspace, [0], errors=errors,
                                                  plot_kwargs={'distribution': True, 'autoscale_on_update': False,
                                                               'label': label})
                self._model.add_workspace_to_plotted_workspaces(workspace)

        # scale the axis and set title
        self._view.set_fig_titles(self.get_plot_title())
        self._model.autoscale_axes(self._view.get_axes(), self.get_x_limits())
        self.update_options_view_from_model()
        self._view.force_redraw()

        self._model.plotted_workspaces_inverse_binning = {
            workspace: self.context.group_pair_context.get_equivalent_group_pair(workspace)
            for workspace in workspace_list
            if self.context.group_pair_context.get_equivalent_group_pair(workspace)}

    def update_model_tile_plot_positions(self):
        """
        Updates tile dictionary in the model, which maps workspaces to
        the axis which they are to be plotted on
        """
        self._model.tiled_plot_positions.clear()
        self._model.clear_plot_model(self._view.get_axes())
        self.context.fitting_context.clear()

        if self._view.get_tiled_by_type() == 'run':  # tile by run
            flattened_run_list = [item for sublist in self.context.data_context.current_runs for item in sublist]
            instrument = self.context.data_context.instrument
            for i, run in enumerate(flattened_run_list):
                self._model.tiled_plot_positions[instrument + str(run)] = i
        else:  # tile by group or pair
            for i, grppair in enumerate(self.context.group_pair_context.selected_groups +
                                        self.context.group_pair_context.selected_pairs):
                self._model.tiled_plot_positions[grppair] = i

        self._model.number_of_axes = len(self._model.tiled_plot_positions)

    def new_plot_figure(self, num_axes):
        self._model.clear_plot_model(self._view.get_axes())
        self.context.fitting_context.clear()
        self._view.new_plot_figure(num_axes)

    def get_plot_title(self):
        """
        Generates a title for the plot based on current instrument group and run numbers
        :return: Plot titles
        """
        flattened_run_list = [
            item for sublist in self.context.data_context.current_runs for item in sublist]
        instrument = self.context.data_context.instrument
        plot_titles = []

        if self._view.is_tiled_plot():
            if self._view.get_tiled_by_type() == 'run':
                for run in flattened_run_list:
                    plot_titles.append(instrument + str(run))
            else:
                for grouppair in self.context.group_pair_context.selected_groups + \
                                 self.context.group_pair_context.selected_pairs:
                    title = self.context.data_context.instrument + ' ' + run_list_to_string(flattened_run_list) + \
                            ' ' + str(grouppair)
                    plot_titles.append(title)
            return plot_titles
        else:
            return [self.context.data_context.instrument + ' ' + run_list_to_string(flattened_run_list)]

    def get_workspace_legend_label(self, workspace_name):
        """
        Generates a label for the workspace which is used in the plot
        :return: workspace label
        """
        if self._view.is_tiled_plot():
            if self._view.get_tiled_by_type() == 'group':
                label = self.context.data_context.instrument + self._get_run_number_from_workspace(workspace_name)
            else:
                label = self._get_group_or_pair_from_workspace_name(workspace_name)
        else:
            label = self.context.data_context.instrument + self._get_run_number_from_workspace(workspace_name) + \
                    '; ' + self._get_group_or_pair_from_workspace_name(workspace_name)

        if not self._view.if_raw():
            label = label + '; Rebin'

        if FREQ_PLOT_TYPE in self._view.get_selected():
            label = workspace_name

        return label

    def get_workspace_plot_axis(self, workspace_name):
        """
        Returns the axis which the workspace will be plotted to
        :return: axis
        """
        if self._view.is_tiled_plot():
            tiled_key = self.get_tiled_key(workspace_name)
            position = self._model.tiled_plot_positions[tiled_key]
            ax = self._view.get_axes()[position]
        else:
            ax = self._view.get_axes()[0]
        return ax

    def get_tiled_key(self, workspace_name):
        if self._view.get_tiled_by_type() == 'group':
            tiled_key = self._get_group_or_pair_from_workspace_name(workspace_name)
        else:
            tiled_key = self.context.data_context.instrument + self._get_run_number_from_workspace(workspace_name)
        return tiled_key

    def get_domain(self):
        if FREQ_PLOT_TYPE in self._view.get_selected():
            return "Frequency"
        else:
            return "Time"

    def get_x_lim_from_subplot(self, subplot):
        left, right = self._view.get_axes()[subplot].get_xlim()
        return left, right

    def get_y_lim_from_subplot(self, subplot):
        bottom, top = self._view.get_axes()[subplot].get_ylim()
        return bottom, top

    def get_x_limits(self):
        xlims = self._view.plot_options.get_plot_x_range()
        if xlims[1] - xlims[0] > 0:
            return xlims
        else:
            return default_xlimits[self.get_domain()]

    def get_selected_axes(self):
        subplots = self._view.plot_options.get_selection()
        if len(subplots) > 0:
            indices = [ix - 1 for ix in list(map(int, subplots))]
            selected_axes = [self._view.get_axes()[index] for index in indices]
            return selected_axes
        else:
            return []  # no subplots are avaiable

    # Note: These methods should be implemented as lower level properties
    # as currently they are specialised methods dependent on the workspace name format.

    # The number following the instrument name is the run
    def _get_run_number_from_workspace(self, workspace_name):
        instrument = self.context.data_context.instrument
        run = re.findall(r'%s(\d+)' % instrument, workspace_name)
        return run[0]

    # the string following either 'Pair Asym; %s' or  Group; "
    def _get_group_or_pair_from_workspace_name(self, workspace_name):
        for grppair in self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs:
            grp = re.findall(r'%s' % grppair, workspace_name)
            if len(grp) > 0:
                return grp[0]
