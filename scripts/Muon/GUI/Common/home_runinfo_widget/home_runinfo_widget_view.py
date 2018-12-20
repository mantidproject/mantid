from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui


# TODO : Use a fixed-width font in the text edit to give better aesthetics


class HomeRunInfoWidgetView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(HomeRunInfoWidgetView, self).__init__(parent)

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("RunInfoWidget")
        self.resize(500, 100)

        self.run_info_box = QtGui.QTextEdit(self)
        self.run_info_box.setObjectName("runInfoBox")
        font = QtGui.QFont("Courier")
        self.run_info_box.setFont(font)
        self.run_info_box.setReadOnly(True)

        self.horizontal_layout_2 = QtGui.QHBoxLayout()
        self.horizontal_layout_2.setObjectName("horizontalLayout2")
        self.horizontal_layout_2.addWidget(self.run_info_box)

        self.vertical_layout = QtGui.QVBoxLayout()
        self.vertical_layout.setObjectName("verticalLayout")
        self.vertical_layout.addItem(self.horizontal_layout_2)

        self.group = QtGui.QGroupBox("Run Information")
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

        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtGui.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def clear(self):
        self.run_info_box.clear()

    def add_text_line(self, text):
        self.run_info_box.append(text)
