# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QMainWindow, QStatusBar


class StatusBarView(QMainWindow):
    def __init__(self, parent, central_widget, name, window_width=600, window_height=400):
        super(StatusBarView, self).__init__(parent)
        self.setCentralWidget(central_widget)
        self.setWindowTitle("{} - Mantid".format(name))
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        self.status_bar = QStatusBar(self)
        self.setStatusBar(self.status_bar)

        self.resize(window_width, window_height)
