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

    def __init__(self, view: PlottingCanvasView, model: PlottingCanvasModel, options_presenter : QuickEditPresenter):
        self._view = view
        self._model = model
        self._options_view = None
        self._options_presenter = options_presenter

    # interface implementation
    def plot_workspaces(self, workspace_names, workspace_indicies, overplot):

        workspace_plot_info_to_add, workspace_plot_info_to_remove = \
            self._model.get_workspaces_to_plot_and_remove(workspace_names, workspace_indicies,
                                                          self._view.plotted_workspace_information)

        if not overplot:
            self._view.remove_workspaces_from_plot(workspace_plot_info_to_remove)

        self._view.add_workspaces_to_plot(workspace_plot_info_to_add)

        self._set_axes_limits_and_titles()

    def remove_workspace_from_plot(self, workspace):
        self._view.remove_specified_workspace_from_plot(workspace)

    def replace_workspace_in_plot(self, workspace):
        self._view.replace_specified_workspace_in_plot(workspace)

    def convert_to_tiled_plot(self, keys, tiled_by):
        number_of_axes = len(keys)
        if number_of_axes <= 1:
            return
        self._model.update_tiled_axis_map(keys, tiled_by)
        # Update the axis selection for the data which is currently plotted
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self._view.create_new_plot_canvas(number_of_axes)

        workspace_plot_info, _ = self._model.get_workspaces_to_plot_and_remove(workspaces, indices, [])
        self._view.add_workspaces_to_plot(workspace_plot_info)

        self._set_axes_limits_and_titles()

    def convert_to_single_plot(self):
        if self._view.number_of_axes == 1:
            return
        self._model.is_tiled = False
        # Update the axis selection for the data which is currently plotted
        workspaces, indices = self._view.plotted_workspaces_and_indices
        self._view.create_new_plot_canvas(1)

        workspace_plot_info, _ = self._model.get_workspaces_to_plot_and_remove(workspaces, indices, [])
        self._view.add_workspaces_to_plot(workspace_plot_info)

        self._set_axes_limits_and_titles()

    def get_plotted_workspaces_and_indices(self):
        plotted_workspaces, indices = self._view.plotted_workspaces_and_indices
        return plotted_workspaces, indices

    def handle_axis_limits_changed(self):
        a = 3

    def autoscale_axes(self):
        pass

    def set_axis_limits(self, ax_num, xlims, ylims):
        pass

    # specific implementation details
    def get_axis_limits_from_edit_widget(self):
        pass

    def set_axis_limits_in_edit_widget(self):
        pass



    def _set_axes_limits_and_titles(self):
        xlims, ylims = self._model.get_axis_limits(self._view.fig.axes)
        self._view.set_titles(self._model.create_axes_titles())
        self._view.set_axes_limits(xlims, ylims)
        # Finally redraw the figure
        self._view.redraw_figure()