# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common.plotting_widget.plotting_canvas_widget import PlottingCanvasWidget
from mantidqt.utils.qt import load_ui
from MultiPlotting.QuickEdit.quickEdit_widget import QuickEditWidget
from Muon.GUI.Common.plotting_widget.abstract_plotting_widget_view import AbstractPlottingWidgetView
import Muon.GUI.Common.message_box as message_box

from matplotlib.backends.qt_compat import is_pyqt5

if is_pyqt5():
    from matplotlib.backends.backend_qt5agg import FigureCanvas
else:
    from matplotlib.backends.backend_qt4agg import FigureCanvas

ui_plotting_view, _ = load_ui(__file__, "plotting_widget_view.ui")


class PlotWidgetView1(QtWidgets.QWidget, AbstractPlottingWidgetView, ui_plotting_view):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None, figure_widget=None):
        super().__init__(parent=parent)
        self.setupUi(self)
        self._plotting_widget = PlottingCanvasWidget(parent)

        canvas_layout = QtWidgets.QHBoxLayout(parent)
        canvas_layout.addWidget(self._plotting_widget.widget())
        self.canvasFrame.setLayout(canvas_layout)

    def plot_widget(self):
        return self._plotting_widget

    def get_plot_type(self): pass

    def is_data_rebinned(self): pass

    def plot_data(self, data): pass

    def clear_plot(self): pass

    def is_tiled_plot(self): pass

    def tiled_by(self): pass

    def on_rebin_options_changed(self, slot): pass

    def on_plot_type_changed(self, slot): pass

    def on_tiled_by_type_changed(self, slot): pass

    def on_plot_tiled_changed(self, slot): pass

    def on_external_plot_pressed(self, slot): pass
