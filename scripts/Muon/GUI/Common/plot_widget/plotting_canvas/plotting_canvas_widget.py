# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from Muon.GUI.Common.plot_widget.QuickEdit.quickEdit_widget import QuickEditWidget
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter import PlottingCanvasPresenter
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view import PlottingCanvasView


class PlottingCanvasWidget(object):

    def __init__(self, parent, context):

        self._figure_options = QuickEditWidget(context.plotting_context, parent)
        self._plotting_view = PlottingCanvasView(self._figure_options.widget, context.plotting_context.min_y_range, context.plotting_context.y_axis_margin, parent)
        self._model = PlottingCanvasModel(context)
        self._presenter = PlottingCanvasPresenter(self._plotting_view, self._model, self._figure_options, context.plotting_context)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._plotting_view
