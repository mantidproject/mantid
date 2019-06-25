# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from qtpy import QtWidgets


class HomeTabView(QtWidgets.QWidget):

    def __init__(self, parent=None, widget_list=None):
        super(HomeTabView, self).__init__(parent)

        self._widget_list = widget_list

        self.splitter = None
        self.vertical_layout = None

        self.setup_interface()

    def setup_interface(self):
        self.setObjectName("HomeTab")
        self.setWindowTitle("Home Tab")
        self.resize(500, 100)

        self.vertical_layout = QtWidgets.QVBoxLayout()

        if self._widget_list:
            for i, widget in enumerate(self._widget_list):
                widget.setParent(self)
                self.vertical_layout.addWidget(widget)

        self.setLayout(self.vertical_layout)

    # for docking
    def getLayout(self):
        return self.vertical_layout
