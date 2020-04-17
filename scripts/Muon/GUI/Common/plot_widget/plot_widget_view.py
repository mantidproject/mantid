# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui
from Muon.GUI.Common.plot_widget.plot_widget_view_interface import PlotWidgetViewInterface
import Muon.GUI.Common.message_box as message_box

ui_plotting_view, _ = load_ui(__file__, "plotting_widget_view.ui")


class PlotWidgetView(QtWidgets.QWidget, PlotWidgetViewInterface, ui_plotting_view):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setupUi(self)

    def setup_plot_type_options(self, options):
        """
        Setup the options which are displayed in the plot type combo box
        """
        self.plot_type_combo.blockSignals(True)
        self.plot_type_combo.clear()
        for option in options:
            self.plot_type_combo.addItem(option)
        self.plot_type_combo.blockSignals(False)

    def setup_tiled_by_options(self, options):
        """
        Setup to options which are shown in the tiled plots combo box
        """
        self.tiled_by_combo.blockSignals(True)
        self.tiled_by_combo.clear()
        for option in options:
            self.tiled_by_combo.addItem(option)
        self.tiled_by_combo.blockSignals(False)

    def add_canvas_widget(self, canvas_widget):
        """
        Adds the canvas widget (where the plotting will occur) to the widget view.
        """
        canvas_layout = QtWidgets.QHBoxLayout(self)
        self.canvasFrame.setLayout(canvas_layout)
        canvas_layout.addWidget(canvas_widget)

    def get_plot_type(self):
        """
        Returns the current plot type
        """
        return self.plot_type_combo.currentText()

    def is_tiled_plot(self):
        """
        Checks if tiled plot is currently requested
        """
        return self.tiled_plot_checkbox.isChecked()

    def is_raw_plot(self):
        """
        Checks if plotting raw data
        """
        return self.plot_raw_checkbox.isChecked()

    def tiled_by(self):
        """
        Returns the option which is currently selected in the 'tiled by' combo box
        """
        return self.tiled_by_combo.currentText()

    def on_plot_type_changed(self, slot):
        """
        Connect the plot_type combo box to the input slot
        """
        self.plot_type_combo.currentIndexChanged.connect(slot)

    def on_plot_tiled_checkbox_changed(self, slot):
        """
        Connect the tiled_plot checkbox to the input slot
        """
        self.tiled_plot_checkbox.stateChanged.connect(slot)

    def on_tiled_by_type_changed(self, slot):
        """
        Connect the tiled_by combo box to the input slot
        """
        self.tiled_by_combo.currentIndexChanged.connect(slot)

    def on_rebin_options_changed(self, slot):
        """
        Connect the plot_rebin checkbox to the input slot
        """
        self.plot_raw_checkbox.clicked.connect(slot)

    def on_external_plot_pressed(self, slot):
        """
        Connect the external plot button to the input slot
        """
        self.external_plot_button.clicked.connect(slot)

    def set_raw_checkbox_state(self, state):
        """
        Sets the raw checkbox state, which can be controlled externally through the home tab.
        """
        self.plot_raw_checkbox.setChecked(state)

    def set_plot_type(self, plot_type: str):
        """
        Sets the plot type to the input str
        """
        self.plot_type_combo.blockSignals(True)
        self.plot_type_combo.setCurrentText(plot_type)
        self.plot_type_combo.blockSignals(False)
