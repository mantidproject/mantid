# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plotting_widget.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plotting_widget.plotting_canvas_view import PlottingCanvasView


class PlottingCanvasPresenter(object):

    def __init__(self, view: PlottingCanvasView, model: PlottingCanvasModel):
        self._view = view
        self._model = model

    def handle_plot_workspaces(self, workspace_names, workspace_indicies, errors,
                               overplot):
        if not overplot:
            self._view.clear_workspaces_from_plot()

        for workspace_name in workspace_names:
            workspace_plot_info = self._model.create_plot_information(workspace_name, 0, errors)
            self._view.add_workspaces_to_plot(workspace_plot_info)

        # Get axis limits from model
        xlims, ylims = self._model.get_axis_limits(self._view.fig.axes)
        self._view.set_axes_limits(xlims, ylims)

        # Finally redraw the figure
        self._view.redraw_figure()

    def handle_axis_limits_changed(self):
        pass
