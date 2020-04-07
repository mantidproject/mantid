# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.plotting_widget.abstract_plotting_widget_view import AbstractPlottingWidgetView
import Muon.GUI.Common.message_box as message_box

ui_plotting_view, _ = load_ui(__file__, "plotting_widget_view.ui")


class PlotWidgetView1(QtWidgets.QWidget, AbstractPlottingWidgetView, ui_plotting_view):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setupUi(self)

        self.plot_type_combo.addItems(["Asymmetry", "Counts"])
        self.tiled_by_combo.addItems(["Group/Pair", "Run"])

    def add_canvas_widget(self, canvas_widget):
        canvas_layout = QtWidgets.QHBoxLayout(self)
        self.canvasFrame.setLayout(canvas_layout)
        canvas_layout.addWidget(canvas_widget)

    def get_plot_type(self): pass

    def is_data_rebinned(self): pass

    def plot_data(self, data): pass

    def clear_plot(self): pass

    def is_tiled_plot(self): pass

    def tiled_by(self):
        return self.tiled_by_combo.currentText()

    def on_plot_type_changed(self, slot): pass

    def on_plot_tiled_checkbox_changed(self, slot):
        self.tiled_plot_checkbox.stateChanged.connect(slot)

    def on_tiled_by_type_changed(self, slot):
        self.tiled_by_combo.currentIndexChanged.connect(slot)

    def on_rebin_options_changed(self, slot): pass

    def on_external_plot_pressed(self, slot): pass
