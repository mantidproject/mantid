# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List

from Muon.GUI.Common.plotting_widget.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plotting_widget.plotting_canvas_presenter import PlottingCanvasPresenter
from Muon.GUI.Common.plotting_widget.plotting_canvas_view import PlottingCanvasView


class PlottingCanvasWidget(object):

    def __init__(self, parent, view=None, model=None):

        self._view = view if view else PlottingCanvasView(parent)
        self._model = model if model else PlottingCanvasModel()
        self._presenter = PlottingCanvasPresenter(self._view, self._model)

    def widget(self):
        return self._view

    def plot_workspace_data(self, workspace_names: List[str], workspace_indicies: List[int],
                            errors: bool, overplot: bool):
        self._presenter.handle_plot_workspaces(workspace_names, workspace_indicies,
                                               errors, overplot)
