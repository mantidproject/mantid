# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.plotting_widget.plotting_widget_view_interface import PlottingWidgetViewInterface
import Muon.GUI.Common.message_box as message_box

ui_plotting_view, _ = load_ui(__file__, "plotting_widget_view.ui")


class PlotWidgetView1(QtWidgets.QWidget, PlottingWidgetViewInterface, ui_plotting_view):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setupUi(self)

        self.tiled_by_combo

    def setup_plot_type_options(self, options):
        self.plot_type_combo.blockSignals(True)
        self.plot_type_combo.clear()
        for option in options:
            self.plot_type_combo.addItem(option)
        self.plot_type_combo.blockSignals(False)

    def setup_tiled_by_options(self, options):
        self.tiled_by_combo.blockSignals(True)
        self.tiled_by_combo.clear()
        for option in options:
            self.tiled_by_combo.addItem(option)
        self.tiled_by_combo.blockSignals(False)

    def add_canvas_widget(self, canvas_widget):
        canvas_layout = QtWidgets.QHBoxLayout(self)
        self.canvasFrame.setLayout(canvas_layout)
        canvas_layout.addWidget(canvas_widget)

    def get_plot_type(self):
        return self.plot_type_combo.currentText()

    def is_data_rebinned(self): pass

    def is_tiled_plot(self): pass

    def tiled_by(self):
        return self.tiled_by_combo.currentText()

    def on_plot_type_changed(self, slot):
        self.plot_type_combo.currentIndexChanged.connect(slot)

    def on_plot_tiled_checkbox_changed(self, slot):
        self.tiled_plot_checkbox.stateChanged.connect(slot)

    def on_tiled_by_type_changed(self, slot):
        self.tiled_by_combo.currentIndexChanged.connect(slot)

    def on_rebin_options_changed(self, slot): pass

    def on_external_plot_pressed(self, slot): pass
