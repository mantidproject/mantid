# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter
from Muon.GUI.Common.plotting_widget.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plotting_widget.plotting_canvas_presenter_interface import PlottingCanvasPresenterInterface
from Muon.GUI.Common.plotting_widget.plotting_canvas_view import PlottingCanvasView


class PlottingCanvasPresenter(PlottingCanvasPresenterInterface):

    def __init__(self, view: PlottingCanvasView, model: PlottingCanvasModel, options_presenter: QuickEditPresenter):
        self._view = view
        self._model = model
        self._options_view = None
        self._options_presenter = options_presenter

        # connection to quick edit widget
        self._options_presenter.connect_errors_changed(self.handle_error_selection_changed)
        self._options_presenter.connect_x_range_changed(self.handle_xlim_changed_in_quick_edit_options)
        self._options_presenter.connect_y_range_changed(self.handle_ylim_changed_in_quick_edit_options)
        self._options_presenter.connect_autoscale_changed(self.handle_autoscale_requested_in_quick_edit_options)
        self._options_presenter.connect_plot_selection(self._handle_subplot_changed_in_quick_edit_widget)

    # Interface implementation
    def plot_workspaces(self, workspace_names, workspace_indices, overplot, autoscale=False):

        workspace_plot_info_to_add, workspace_plot_info_to_remove = \
            self._model.get_workspaces_to_plot_and_remove(workspace_names, workspace_indices,
                                                          self._view.plotted_workspace_information,
                                                          self._options_presenter.get_errors())

        if not overplot:
            self._view.remove_workspaces_from_plot(workspace_plot_info_to_remove)

        self._view.add_workspaces_to_plot(workspace_plot_info_to_add)

        self._set_axes_limits_and_titles(autoscale)
        self._update_quickedit_widget()

    def remove_workspace_from_plot(self, workspace):
        self._view.remove_specified_workspace_from_plot(workspace)

    def replace_workspace_in_plot(self, workspace):
        self._view.replace_specified_workspace_in_plot(workspace)

    def convert_plot_to_tiled_plot(self, keys, tiled_by):
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self.create_tiled_plot(keys, tiled_by)
        # Replot data on new axis
        self.plot_workspaces(workspaces, indices, False, False)

    def convert_plot_to_single_plot(self):
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self.create_single_plot()
        # Replot data on new axis
        self.plot_workspaces(workspaces, indices, False, False)

    def create_tiled_plot(self, keys, tiled_by):
        self._model.update_tiled_axis_map(keys, tiled_by)
        self._view.create_new_plot_canvas(len(keys))

    def create_single_plot(self):
        self._model.is_tiled = False
        self._view.create_new_plot_canvas(1)

    def get_plotted_workspaces_and_indices(self):
        plotted_workspaces, indices = self._view.plotted_workspaces_and_indices
        return plotted_workspaces, indices

    def autoscale_y_axes(self):
        self._view.autoscale_y_axes()
        self._view.redraw_figure()

    def autoscale_selected_y_axis(self, axis_num):
        self._view.autoscale_selected_y_axis(axis_num)
        self._view.redraw_figure()

    def set_axis_limits(self, ax_num, xlims, ylims):
        self._view.set_axis_xlimits(ax_num, xlims)
        self._view.set_axis_ylimits(ax_num, ylims)

    # Implementation of QuickEdit widget
    def _update_quickedit_widget(self):
        self._update_quick_widget_subplots()

    def _update_quick_widget_subplots(self):
        self._options_presenter.clear_subplots()
        for i in range(self._view.number_of_axes):
            self._options_presenter.add_subplot(str(i + 1))
        self._handle_subplot_changed_in_quick_edit_widget()

    def _handle_subplot_changed_in_quick_edit_widget(self):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        index = selected_subplots[0]
        xmin, xmax, ymin, ymax = self._view.get_axis_limits(index)
        self._options_presenter.set_plot_x_range([xmin, xmax])
        self._options_presenter.set_plot_y_range([ymin, ymax])

    def handle_xlim_changed_in_quick_edit_options(self, xlims):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        for subplot in selected_subplots:
            self._view.set_axis_xlimits(subplot, xlims)
        self._view.redraw_figure()

    def handle_ylim_changed_in_quick_edit_options(self, ylims):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        for subplot in selected_subplots:
            self._view.set_axis_ylimits(subplot, ylims)
        self._view.redraw_figure()

    def handle_autoscale_requested_in_quick_edit_options(self):
        selected_subplots = self._get_selected_subplots_from_quick_edit_widget()
        for subplot in selected_subplots:
            self.autoscale_selected_y_axis(subplot)

    def handle_error_selection_changed(self):
        plotted_workspaces, _ = self._view.plotted_workspaces_and_indices
        for workspace_name in plotted_workspaces:
            self._view.replot_workspace_with_error_state(workspace_name, self._options_presenter.get_errors())
        self._view.redraw_figure()

    def _get_selected_subplots_from_quick_edit_widget(self):
        subplots = self._options_presenter.get_selection()
        if len(subplots) > 0:
            selected_subplots = [ix - 1 for ix in list(map(int, subplots))]
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

    def _set_axes_limits_and_titles(self, autoscale):
        xlims, ylims = self._get_axes_limits_from_quick_edit_widget()
        if xlims is None or ylims is None:
            self._view.set_default_axes_limits()
        else:
            self._view.set_axes_limits(xlims, ylims)
            if autoscale:
                self._view.autoscale_y_axes()
        self._view.set_titles(self._model.create_axes_titles())
        # Finally redraw the figure
        self._view.redraw_figure()
