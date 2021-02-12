# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view_interface import PlottingCanvasViewInterface
from mantid import AnalysisDataService
from mantidqt.utils.observer_pattern import GenericObserver


class PlottingCanvasPresenter(PlottingCanvasPresenterInterface):

    def __init__(self, view: PlottingCanvasViewInterface, model: PlottingCanvasModel,
                 options_presenter: QuickEditWidget, context):
        self._view = view
        self._model = model
        self._options_view = None
        self._context = context
        self._options_presenter = options_presenter
        # connection to quick edit widget
        self._setup_quick_edit_widget()
        self._setup_autoscale_observer()
        self._options_presenter.add_subplot("one")
        self._context.update_axis("one", 0)

    # general plotting
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

    def clear_subplots(self):
        self._context.clear_subplots()
        self._options_presenter.clear_subplots()

    def create_tiled_plot(self, keys, tiled_by):
        """Creates a blank tiled plot specified by the keys and tiled by type"""
        self._model.update_tiled_axis_map(keys, tiled_by)
        self.clear_subplots()
        num_axes = len(keys) if len(keys) > 0 else 1
        self._view.create_new_plot_canvas(num_axes)

    def create_single_plot(self):
        """Creates a blank single plot """
        self._model.is_tiled = False
        self.clear_subplots()
        self._options_presenter.add_subplot("one")
        self._context.update_axis("one", 0)
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

    def set_axis_limits(self, ax_num, xlims, ylims):
        """Sets the x and y limits for a specified axis in the figure"""
        self._view.set_axis_xlimits(ax_num, xlims)
        self._view.set_axis_ylimits(ax_num, ylims)

    def set_axis_title(self, ax_num, title):
        """Sets the title for a specified axis in the figure"""
        self._view.set_title(ax_num, title)


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

    def _set_axes_limits_and_titles(self, autoscale):
        xlims = self._context.default_xlims
        ylims = self._context.default_ylims

        selected_subplots, _ = self._get_selected_subplots_from_quick_edit_widget()
        if len(selected_subplots) == 1:
            xlim = self._context.get_xlim(selected_subplots[0])
            ylim = self._context.get_ylim(selected_subplots[0])
        self._view.set_axes_limits(xlims, ylims)
        # override y values 
        if autoscale:
                self._view.autoscale_y_axes()
        titles = self._model.create_axes_titles()
        for axis_number, title in enumerate(titles):
            self._view.set_title(axis_number, title)
        self._update_quickedit_widget()
        self._view.redraw_figure()

    """ Quick edit"""
    def _setup_quick_edit_widget(self):
        self._options_presenter.connect_errors_changed(self.handle_error_selection_changed)
        self._options_presenter.connect_x_range_changed(self._handle_xlim_changed_in_quick_edit_options)
        self._options_presenter.connect_y_range_changed(self._handle_ylim_changed_in_quick_edit_options)
        self._options_presenter.connect_autoscale_changed(self._handle_autoscale_y_axes)
        self._options_presenter.connect_plot_selection(self._handle_subplot_changed_in_quick_edit_widget)
        self.range_changed_observer = GenericObserver(self.update_range)
        self._view.add_range_changed_subscriber(self.range_changed_observer)

    def _handle_subplot_changed_in_quick_edit_widget(self):
        selected_subplots, indicies = self._get_selected_subplots_from_quick_edit_widget()

        if selected_subplots == []:
            return
        # it is either all or just one plot
        name = selected_subplots[0]
        xlim = self._context.get_xlim(selected_subplots[0])
        ylim = self._context.get_ylim(selected_subplots[0])
        autoscale = self._context.get_autoscale_state(selected_subplots[0])
        error = self._context.get_error_state(selected_subplots[0])
        # if all
        if len(selected_subplots)>1:
            xlim = self._context.get_xlim_all
            ylim = self._context.get_ylim_all
            autoscale = self._context.get_autoscale_all
            error = self._context.get_error_all
            print("boo",error)
        #update the quick edit
        self._options_presenter.set_plot_x_range(xlim)
        self._options_presenter.set_plot_y_range(ylim)
        self.set_autoscale(autoscale)
        self.set_errors(error)
        # update the plots and contexts
        for subplot, index in zip(selected_subplots,indicies):
            self._view.set_axis_xlimits(index, xlim)
            self._view.set_axis_ylimits(index, ylim)
            self._context.update_xlim(subplot, xlim)
            self._context.update_ylim(subplot, ylim)
            self._context.update_autoscale_state(subplot, autoscale)
            self._context.update_error_state(subplot, error)

        self._view.redraw_figure()

    def _get_selected_subplots_from_quick_edit_widget(self):
        subplots = self._options_presenter.get_selection()
        if len(subplots) > 0:
            indicies = [self._context.get_axis(name) for name in subplots]
            return subplots, indicies
        else:
            return [], [] # no subplots are available

    def _get_axes_limits_from_quick_edit_widget(self):
        xlims = self._options_presenter.get_plot_x_range()
        ylims = self._options_presenter.get_plot_y_range()
        if xlims[0] == xlims[1] or ylims[0] == ylims[1]:
            return None, None
        else:
            return xlims, ylims


    """x or y range changed"""
    def _handle_xlim_changed_in_quick_edit_options(self, xlims):
        selected_subplots, indicies = self._get_selected_subplots_from_quick_edit_widget()
        if len(selected_subplots)>1:
            self._context.update_xlim_all(xlims)
        for subplot, index in zip(selected_subplots,indicies):
            self._view.set_axis_xlimits(index, xlims)
            self._context.update_xlim(subplot, xlims)
        if self._view.autoscale_state:
            self._handle_autoscale_y_axes()
        self._view.redraw_figure()


    def _handle_ylim_changed_in_quick_edit_options(self, ylims):
        selected_subplots, indicies = self._get_selected_subplots_from_quick_edit_widget()
        if len(selected_subplots)>1:
            self._context.update_ylim_all(ylims)
        for subplot, index in zip(selected_subplots,indicies):
            self._view.set_axis_ylimits(index, ylims)
            self._context.update_ylim(subplot, ylims)

        self._view.redraw_figure()

    def get_plot_x_range(self):
        """Returns the x range of the first plot
        :return: a tuple contained the start and end ranges"""
        return self._options_presenter.get_plot_x_range()

    def set_plot_range(self, range):
        """Sets the x range of all the plots"""
        self._options_presenter.set_plot_x_range(range)
        self._handle_xlim_changed_in_quick_edit_options(range)

    def _update_quickedit_widget(self):
        self._update_quick_widget_subplots_menu()
        self._handle_subplot_changed_in_quick_edit_widget()

    def _update_quick_widget_subplots_menu(self):
        subplots = self._options_presenter.get_all_subplots
        for i, title in  enumerate(self._model.create_axes_titles()):
            if title not in subplots:
                self._options_presenter.add_subplot(title)
                self._context.update_axis(title, i)

    def update_range(self):
        # get the updated values first
        # changing the selection first changes the result
        xlim_list = self._view.get_xlim_list
        ylim_list = self._view.get_ylim_list
        for axis, name in enumerate(self._context._subplots.keys()):
            self._context.update_xlim(name, xlim_list[axis])
            self._context.update_ylim(name, ylim_list[axis])

        # force all to be not selected
        selection_index = self._options_presenter.get_selection_index()
        if selection_index==0:
            self._options_presenter.set_selection_by_index(1)

        # Logic to only update the quick edit once
        selection = self._options_presenter.get_selection()
        x_values = self._context.get_xlim(selection[0])       
        y_values = self._context.get_ylim(selection[0])       
        self._options_presenter.set_plot_x_range(x_values)
        self._options_presenter.set_plot_y_range(y_values)

    """ autoscale """

    def _setup_autoscale_observer(self):
        self.uncheck_autoscale_observer = GenericObserver(self.uncheck_autoscale)
        self._view.add_uncheck_autoscale_subscriber(self.uncheck_autoscale_observer)

        self.enable_autoscale_observer = GenericObserver(self.enable_autoscale)
        self._view.add_enable_autoscale_subscriber(self.enable_autoscale_observer)

        self.disable_autoscale_observer = GenericObserver(self.disable_autoscale)
        self._view.add_disable_autoscale_subscriber(self.disable_autoscale_observer)

    def uncheck_autoscale(self):
        self._options_presenter.set_autoscale(False)

    def enable_autoscale(self):
        self._options_presenter.enable_autoscale()

    def disable_autoscale(self):
        self._options_presenter.disable_autoscale()

    def set_autoscale(self, state:bool):
        self._options_presenter.set_autoscale(state)
        if state:
            self._options_presenter.disable_yaxis_changer()
        else:
            self._options_presenter.enable_yaxis_changer()

    def _handle_autoscale_y_axes(self):
        # update the state of autoscale
        selected_subplots, axes = self._get_selected_subplots_from_quick_edit_widget()
        state = self._options_presenter.autoscale
        for plot in selected_subplots:
            self._context.update_autoscale_state(plot, state)
        if len(selected_subplots)>1:
            self._context.set_autoscale_all = state
        if not state:
            self._options_presenter.enable_yaxis_changer()
            return
        # get min and max
        ymin = 9.9e9
        ymax = -9.9e9
        for axis_num, plot in zip(axes, selected_subplots):
           self._view.autoscale_selected_y_axis(axis_num)
           _, _, y_min, y_max = self._view.get_axis_limits(axis_num)
           if ymin > y_min:
               ymin = y_min
           if y_max > ymax:
               ymax = y_max
        # update y vales
        for plot in selected_subplots:   
           self._context.update_ylim(plot, [ymin,ymax])    
        if len(selected_subplots)>1:
            self._context.update_ylim_all([ymin,ymax])

        self._options_presenter.set_plot_y_range([ymin,ymax])
        self._options_presenter.disable_yaxis_changer()
        self._view.redraw_figure()

    def autoscale_y_axes(self):
        """Autoscales all y-axes in the figure using the existing x axis"""
        self._view.autoscale_y_axes()
        self._view.redraw_figure()     
        
    """ error checkbox"""
    def set_errors(self, state:bool):
        print("moo", state)
        self._options_presenter.set_errors(state)
        self._options_presenter.set_errors(state)

    def handle_error_selection_changed(self):
        # update the state of autoscale
        selected_subplots, _ = self._get_selected_subplots_from_quick_edit_widget()
        state = self._options_presenter.get_errors()
        plotted_workspaces, plot_indicies = self._view.plotted_workspaces_and_indices

        if len(selected_subplots)>1:
            self._context.set_error_all(state)
            print(":P",state)

        for plot in selected_subplots:
            self._context.update_error_state(plot, state)
            axis = self._context.get_axis(plot)
            for workspace_name in plotted_workspaces:
                index = self._model._get_workspace_plot_axis(workspace_name)
                if axis == index:
                    print(workspace_name, "WE")
                    self.replot_workspace_with_error_state(workspace_name, state)


