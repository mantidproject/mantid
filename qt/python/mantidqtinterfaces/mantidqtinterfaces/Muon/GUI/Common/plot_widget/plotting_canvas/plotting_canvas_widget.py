# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import QuickEditWidget
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter import PlottingCanvasPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_view import PlottingCanvasView


class PlottingCanvasWidget(object):
    def __init__(self, parent, context, plot_model, figure_options=None):
        if figure_options:
            self._figure_options = figure_options
        else:
            self._figure_options = QuickEditWidget(context, parent)
        self._plotting_view = PlottingCanvasView(self._figure_options.widget, context.settings, parent)
        self._model = PlottingCanvasModel(plot_model)
        self._presenter = PlottingCanvasPresenter(self._plotting_view, self._model, self._figure_options, context)

    @property
    def get_quick_edit_info(self):
        selection = self._figure_options.get_selection()
        if len(selection) > 1:
            selection = "ALL"
        return (
            selection,
            self._figure_options.get_plot_x_range(),
            self._figure_options.autoscale,
            self._figure_options.get_plot_y_range(),
            self._figure_options.get_errors(),
        )

    def disable_plot_selection(self):
        self._figure_options.disable_plot_selection()

    def set_quick_edit_info(self, selection, x_range, auto, y_range, errors):
        self._presenter.set_subplot_selection(selection)
        self._presenter.set_plot_range(x_range)
        if auto:
            self._presenter.set_autoscale(auto)
        else:
            self._presenter.set_autoscale(auto)
            self._presenter.set_plot_y_range(y_range)
        self._presenter.set_errors(errors)

    @property
    def presenter(self):
        return self._presenter

    @property
    def widget(self):
        return self._plotting_view
