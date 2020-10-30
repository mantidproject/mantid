# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view_interface import PlottingCanvasViewInterface
from mantid import AnalysisDataService
from mantidqt.utils.observer_pattern import GenericObserver

DEFAULT_X_LIMITS = [0, 15]
DEFAULT_Y_LIMITS = [-1, 1]

class PlottingCanvasPresenter(PlottingCanvasPresenterInterface):

    def __init__(self, view: PlottingCanvasViewInterface, model: PlottingCanvasModel,
                 options_presenter: QuickEditPresenter, context):
        self._view = view
        self._model = model
        self._options_view = None
        self._options_presenter = options_presenter
        self._context = context
        # connection to quick edit widget
        self._setup_quick_edit_widget()
        self._setup_autoscale_observer()
        self._options_presenter.add_subplot("one")
        self._context.update_axis("one", 0)

    def remove_workspace_names_from_plot(self, workspace_names: List[str]):
        """Removes the input workspace names from the plot"""
        for workspace_name in workspace_names:
            try:
                workspace = AnalysisDataService.Instance().retrieve(workspace_name)
            except RuntimeError:
                continue
            self._view.remove_workspace_from_plot(workspace)
        self._view.redraw_figure()

    def remove_workspace_from_plot(self, workspace):
        """Removes all references to the input workspace from the plot"""
        self._view.remove_workspace_from_plot(workspace)
        self._view.redraw_figure()

    def replace_workspace_in_plot(self, workspace):
        """Replace specified workspace in the plot with a new and presumably updated instance"""
        self._view.replace_specified_workspace_in_plot(workspace)

    def replot_workspace_with_error_state(self, workspace_name, error_state):
        """Replot a workspace in the plot with a different error_state"""
        self._view.replot_workspace_with_error_state(workspace_name, error_state)

    def convert_plot_to_tiled_plot(self, keys, tiled_by):
        """Converts the current plot into a tiled plot specified by the keys and tiled by type
        In then replots the existing data on the new tiles"""
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self.create_tiled_plot(keys, tiled_by)
        self.plot_workspaces(workspaces, indices, hold_on=False, autoscale=False)

    def convert_plot_to_single_plot(self):
        """Converts the current plot into a single plot
        In then replots the existing data on the new tiles"""
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self.create_single_plot()
        self.plot_workspaces(workspaces, indices, hold_on=False, autoscale=False)

    def create_tiled_plot(self, keys, tiled_by):
        """Creates a blank tiled plot specified by the keys and tiled by type"""
        self._model.update_tiled_axis_map(keys, tiled_by)
        self._context.clear_subplots()
        self._options_presenter.clear_subplots()
        num_axes = len(keys) if len(keys) > 0 else 1
        self._view.create_new_plot_canvas(num_axes)

    def create_single_plot(self):
        """Creates a blank single plot """
        self._model.is_tiled = False
        self._context.clear_subplots()
        self._options_presenter.clear_subplots()

        self._options_presenter.add_subplot("one")
        self._context.update_axis("one", 0)
        print("bbbb")
        self._view.create_new_plot_canvas(1)

    def get_plotted_workspaces_and_indices(self):
        """Returns the workspace names and indices which are plotted in the figure """
        plotted_workspaces, indices = self._view.plotted_workspaces_and_indices
        return plotted_workspaces, indices

    def plot_guess_workspace(self, guess_ws_name: str):
        """Plots the guess workspace """
        fit_plot_information = self._model.create_plot_information_for_guess_ws(guess_ws_name)
        self._view.add_workspaces_to_plot([fit_plot_information])
        self._view.redraw_figure()

    def get_plot_axes(self):
        """Returns the matplotlib axes - needed for the external plot button"""
        return self._view.fig.axes

    def autoscale_y_axes(self):
        """Autoscales all y-axes in the figure using the existing x axis"""
        self._view.autoscale_y_axes()
        self._view.redraw_figure()

    def autoscale_selected_y_axis(self, axis_num):
        """Autoscales a selected y-axis in the figure using the existing x axis"""
        self._view.autoscale_selected_y_axis(axis_num)
        self._view.redraw_figure()

    def set_axis_limits(self, ax_num, xlims, ylims):
        """Sets the x and y limits for a specified axis in the figure"""
        self._view.set_axis_xlimits(ax_num, xlims)
        self._view.set_axis_ylimits(ax_num, ylims)

    def set_axis_title(self, ax_num, title):
        """Sets the title for a specified axis in the figure"""
        self._view.set_title(ax_num, title)

    def _handle_xlim_changed_in_quick_edit_options(self, xlims):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        print("boo", selected_subplots, list(self._context._subplots.keys()))

        for subplot in selected_subplots:
            self._view.set_axis_xlimits(subplot, xlims)
        self._view.redraw_figure()

    def _handle_ylim_changed_in_quick_edit_options(self, ylims):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        for subplot in selected_subplots:
            self._view.set_axis_ylimits(subplot, ylims)
        self._view.redraw_figure()

    def _set_axes_limits_and_titles(self, autoscale):
        xlims, ylims = self._get_axes_limits_from_quick_edit_widget()
        if xlims is None or ylims is None:
            self._view.set_axes_limits(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)
            self._view.autoscale_y_axes()
        else:
            self._view.set_axes_limits(xlims, ylims)
            # override y values
            if autoscale:
                self._view.autoscale_y_axes()
        titles = self._model.create_axes_titles()
        for axis_number, title in enumerate(titles):
            self._view.set_title(axis_number, title)
        self._update_quickedit_widget()
        self._view.redraw_figure()

#    def _set_axes_limits_and_titles(self, autoscale):
#        xlims, ylims = self._get_axes_limits_from_quick_edit_widget()
#        if xlims is None or ylims is None:
#            self._view.set_axes_limits(DEFAULT_X_LIMITS, DEFAULT_Y_LIMITS)
#            self._view.autoscale_y_axes()
#        else:
#           self._view.set_axes_limits(xlims, ylims)
#
#        if autoscale:
#            self._options_presenter.disable_yaxis_changer()
#            self._view.autoscale_y_axes()
#        else:
#            self._options_presenter.enable_yaxis_changer()#
#
#        titles = self._model.create_axes_titles()
#        for axis_number, title in enumerate(titles):
#            self._view.set_title(axis_number, title)
#        self._update_quickedit_widget()
#        self._view.redraw_figure()


    def _setup_quick_edit_widget(self):
        self._options_presenter.connect_errors_changed(self.handle_error_selection_changed)
        self._options_presenter.connect_x_range_changed(self._handle_xlim_changed_in_quick_edit_options)
        self._options_presenter.connect_y_range_changed(self._handle_ylim_changed_in_quick_edit_options)
        self._options_presenter.connect_autoscale_changed(self._handle_autoscale_y_axes)
        self._options_presenter.connect_plot_selection(self._handle_subplot_changed_in_quick_edit_widget)

    def _setup_autoscale_observer(self):
        self.uncheck_autoscale_observer = GenericObserver(self._options_presenter.uncheck_autoscale)
        self._view.add_uncheck_autoscale_subscriber(self.uncheck_autoscale_observer)

        self.enable_autoscale_observer = GenericObserver(self._options_presenter.enable_autoscale)
        self._view.add_enable_autoscale_subscriber(self.enable_autoscale_observer)

        self.disable_autoscale_observer = GenericObserver(self._options_presenter.disable_autoscale)
        self._view.add_disable_autoscale_subscriber(self.disable_autoscale_observer)

    # Interface implementation
    def plot_workspaces(self, workspace_names: List[str], workspace_indices: List[int], hold_on: bool,
                        autoscale: bool):
        """Plots the input workspace names and indices in the figure window
        If hold_on is True the existing workspaces plotted in the figure are kept"""
        # Create workspace information named tuple from input list
        workspace_plot_info = self._model.create_workspace_plot_information(workspace_names, workspace_indices,
                                                                            self._options_presenter.get_errors())
        if not hold_on:
            # Remove data which is currently plotted and not in the new workspace_plot_info
            workspaces_info_to_remove = [plot_info for plot_info in self._view.plotted_workspace_information
                                         if plot_info not in workspace_plot_info]
            self._view.remove_workspace_info_from_plot(workspaces_info_to_remove)

        # Add workspace info which is currently not plotted
        workspace_info_to_add = [plot_info for plot_info in workspace_plot_info if plot_info
                                 not in self._view.plotted_workspace_information]
        self._view.add_workspaces_to_plot(workspace_info_to_add)
        # check if to force autoscale
        if self._options_presenter.autoscale:
            autoscale = True
        self._set_axes_limits_and_titles(autoscale)

    def get_plot_x_range(self):
        """Returns the x range of the first plot
        :return: a tuple contained the start and end ranges"""
        return self._options_presenter.get_plot_x_range()

    def set_plot_range(self, range):
        """Sets the x range of all the plots"""
        self._options_presenter.set_plot_x_range(range)
        self._handle_xlim_changed_in_quick_edit_options(range)

    # Implementation of QuickEdit widget
    def _update_quickedit_widget(self):
        self._update_quick_widget_subplots_menu()
        self._handle_subplot_changed_in_quick_edit_widget()

    def _update_quick_widget_subplots_menu(self):
        #self._options_presenter.clear_subplots()
        for i, title in  enumerate(self._model.create_axes_titles()):
            self._options_presenter.add_subplot(title)
            self._context.update_axis(title, i)

    def _handle_subplot_changed_in_quick_edit_widget(self):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        if selected_subplots == []:
            return
        index = selected_subplots[0]
        xmin, xmax, ymin, ymax = self._view.get_axis_limits(index)
        self._options_presenter.set_plot_x_range([xmin, xmax])
        self._options_presenter.set_plot_y_range([ymin, ymax])

    def _handle_autoscale_y_axes(self):
        if not self._options_presenter.autoscale:
            self._options_presenter.enable_yaxis_changer()
            return
        self._options_presenter.disable_yaxis_changer()
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        if len(selected_subplots) == 1:
            self.autoscale_selected_y_axis(selected_subplots[0])
        else:
            self.autoscale_y_axes()

        xmin, xmax, ymin, ymax = self._view.get_axis_limits(selected_subplots[0])
        self._options_presenter.set_plot_x_range([xmin, xmax])
        self._options_presenter.set_plot_y_range([ymin, ymax])

    def handle_error_selection_changed(self):
        plotted_workspaces, _ = self._view.plotted_workspaces_and_indices
        for workspace_name in plotted_workspaces:
            self.replot_workspace_with_error_state(workspace_name, self._options_presenter.get_errors())

    def _get_selected_subplots_from_quick_edit_widget(self):
        subplots = self._options_presenter.get_selection()
        print("waa", subplots)
        if len(subplots) > 0:
            selected_subplots = [self._context.get_axis(name) for name in subplots]
            print("mff", selected_subplots)
            return selected_subplots
        else:
            return []  # no subplots are available

    def _get_axes_limits_from_quick_edit_widget(self):
        xlims = self._options_presenter.get_plot_x_range()
        ylims = self._options_presenter.get_plot_y_range()
        if xlims[0] == xlims[1] or ylims[0] == ylims[1]:
            return None, None
        else:
            return xlims, ylims
