# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets
import Muon.GUI.Common.message_box as message_box


class HomePlotWidgetView(QtWidgets.QWidget):

    @staticmethod
    def warning_popup(message):
        message_box.warning(str(message))

    def __init__(self, parent=None):
        super(HomePlotWidgetView, self).__init__(parent)
        self.plot_label = None
        self.plot_selector = None
        self.plot_button = None
        self.horizontal_layout = None
        self.vertical_layout = None
        self.group = None
        self.widget_layout = None

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("PlotWidget")
        self.resize(500, 100)

        self.plot_label = QtWidgets.QLabel(self)
        self.plot_label.setObjectName("plotLabel")
        self.plot_label.setText("Plot type : ")

        self.overlay = QtWidgets.QCheckBox("overlay",self)
        self.keep = QtWidgets.QCheckBox("append",self)
        self.raw = QtWidgets.QCheckBox("plot raw",self)
        self.raw.setChecked(True)

        self.plot_selector = QtWidgets.QComboBox(self)
        self.plot_selector.setObjectName("plotSelector")
        self.plot_selector.addItems(["Asymmetry", "Counts"])

        self.plot_button = QtWidgets.QPushButton(self)
        self.plot_button.setObjectName("PlotButton")
        self.plot_button.setText("Plot")

        self.plot_label.setBuddy(self.plot_button)
        self.plot_selector.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                                         QtWidgets.QSizePolicy.Fixed)
        self.plot_button.setSizePolicy(QtWidgets.QSizePolicy.Fixed,
                                       QtWidgets.QSizePolicy.Fixed)

        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.plot_label)
        self.horizontal_layout.addWidget(self.plot_selector)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.overlay)
        self.overlay.hide()
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.keep)
        self.keep.hide()
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.raw)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.plot_button)
        self.horizontal_layout.addSpacing(50)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)

        self.group = QtWidgets.QGroupBox("Plotting")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: 0px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)

        self.setLayout(self.widget_layout)

    # for docking
    def getLayout(self):
        return self.widget_layout

    def if_overlay(self):
        return self.overlay.isChecked()

    def if_keep(self):
        return self.keep.isChecked()

    def if_raw(self):
        return self.raw.isChecked()

    def set_raw_checkbox_state(self, state):
        self.raw.setChecked(state)

    def get_selected(self):
        return self.plot_selector.currentText()

    def on_plot_button_clicked(self, slot):
        self.plot_button.clicked.connect(slot)

    def on_rebin_options_changed(self, slot):
        self.raw.stateChanged.connect(slot)

    def on_plot_type_changed(self, slot):
        self.plot_selector.currentIndexChanged.connect(slot)
