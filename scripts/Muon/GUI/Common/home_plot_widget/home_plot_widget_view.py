from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import pyqtSignal


class HomePlotWidgetView(QtGui.QWidget):

    def __init__(self, parent=None):
        super(HomePlotWidgetView, self).__init__(parent)
        self._button_height = 40

        self.setup_interface()



    def setup_interface(self):
        self.setObjectName("PlotWidget")
        self.resize(500, 100)

        self.plot_label = QtGui.QLabel(self)
        self.plot_label.setObjectName("plotLabel")
        self.plot_label.setText("Plot type : ")

        self.plot_selector = QtGui.QComboBox(self)
        self.plot_selector.setObjectName("plotSelector")
        self.plot_selector.addItems(["Asymmetry", "Counts"])

        self.plot_button = QtGui.QPushButton(self)
        self.plot_button.setObjectName("PlotButton")
        self.plot_button.setText("Plot")

        self.plot_selector.setMinimumHeight(self._button_height)
        self.plot_button.setMinimumHeight(self._button_height)

        self.plot_label.setBuddy(self.plot_button)
        self.plot_selector.setSizePolicy(QtGui.QSizePolicy.Fixed,
                                        QtGui.QSizePolicy.Fixed)
        self.plot_button.setSizePolicy(QtGui.QSizePolicy.Fixed,
                                         QtGui.QSizePolicy.Fixed)

        self.horizontal_layout = QtGui.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.plot_label)
        self.horizontal_layout.addWidget(self.plot_selector)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.plot_button)
        self.horizontal_layout.addSpacing(50)

        self.vertical_layout = QtGui.QVBoxLayout(self)
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout)

        self.group = QtGui.QGroupBox("Plotting")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                            "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: -10px;'
                           'padding-bottom: 0px;'
                            "padding-right: 10px;"
                           ' color: grey; }')
        # self.setStyleSheet('QGroupBox:title {'
        #                    'subcontrol-origin: margin;'
        #                    'subcontrol-position: top center;'
        #                    'padding-top: -50px;'
        #                    'padding-bottom: -50px;'
        #                    ' color: grey; }')

        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtGui.QVBoxLayout(self)
        self.widget_layout.addItem(self.horizontal_layout)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)
