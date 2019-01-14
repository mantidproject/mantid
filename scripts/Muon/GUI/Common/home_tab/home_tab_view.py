# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtGui, QtCore


class HomeTabView(QtGui.QWidget):

    def __init__(self, parent=None,widget_list=None):
        super(HomeTabView, self).__init__(parent)

        self._widget_list = widget_list

        self.splitter = None
        self.vertical_layout = None

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("HomeTab")
        self.setWindowTitle("Home Tab")
        self.resize(500, 100)

        self.splitter = QtGui.QSplitter(QtCore.Qt.Vertical)

        self.vertical_layout = QtGui.QVBoxLayout()

        if self._widget_list:
            for i, widget in enumerate(self._widget_list):
                self.splitter.addWidget(widget)
                self.splitter.setCollapsible(i, False)

        self.splitter.setHandleWidth(2)
        self.setStyleSheet("QSplitter::handle {background-color: darkBlue}")

        self.vertical_layout.addWidget(self.splitter)
        self.setLayout(self.vertical_layout)

    # for docking
    def getLayout(self):
        return self.vertical_layout
