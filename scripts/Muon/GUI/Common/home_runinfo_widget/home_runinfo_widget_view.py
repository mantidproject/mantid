# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets, QtGui


class HomeRunInfoWidgetView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(HomeRunInfoWidgetView, self).__init__(parent)

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("RunInfoWidget")
        self.resize(500, 100)

        self.run_info_box = QtWidgets.QTextEdit(self)
        self.run_info_box.setObjectName("runInfoBox")
        font = QtGui.QFont("Courier")
        self.run_info_box.setFont(font)
        self.run_info_box.setReadOnly(True)

        self.horizontal_layout_2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout_2.setObjectName("horizontalLayout2")
        self.horizontal_layout_2.addWidget(self.run_info_box)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout_2)

        self.group = QtWidgets.QGroupBox("Run Information")
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

    def clear(self):
        self.run_info_box.clear()

    def add_text_line(self, text):
        self.run_info_box.append(text)
