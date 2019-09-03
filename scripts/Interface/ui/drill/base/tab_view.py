# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import (QWidget, QTabWidget)


class TabView(QTabWidget):

    def __init__(self):
        QTabWidget.__init__(self)
        self.addTab(QWidget(),'Settings')
        self.addTab(QWidget(), 'I(Q)')
        self.addTab(QWidget(), 'Export')
        self.addTab(QWidget(), 'Plot')
