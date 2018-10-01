"""
The run information box of the home tab of Muon Analysis 2.0. This file
contains the view class HomeRunInfoWidgetView.
"""

from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui


class HomeRunInfoWidgetView(QtGui.QWidget):
    """
    The view handles adding lines of text to the run information box
    and clearing the box.
    """

    def __init__(self, parent=None):
        super(HomeRunInfoWidgetView, self).__init__(parent)

        self.setup_interface()

    def setup_interface(self):
        """Set up the interface."""
        self.setObjectName("RunInfoWidget")
        self.resize(500, 100)

        self.run_info_box = QtGui.QTextEdit(self)
        self.run_info_box.setObjectName("runInfoBox")
        # use a fixed width font for better aesthetics
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
        self.setStyleSheet("QGroupBox {"
                           "border: 1px solid grey;"
                           "border-radius: 10px;"
                           "margin-top: 1ex;"
                           "margin-right: 0ex}"
                           "QGroupBox:title {"
                           "subcontrol-origin: margin;"
                           "padding: 0 3px;"
                           "subcontrol-position: top center;"
                           "padding-top: -10px;"
                           "padding-bottom: 0px;"
                           "padding-right: 10px;"
                           "color: grey; }")
        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtGui.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def add_text_line(self, text):
        """Add line of text to the table."""
        self.run_info_box.append(text)

    def clear(self):
        """Clear the table of text completely."""
        self.run_info_box.clear()
