# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QTableWidget, QTableWidgetItem, QHeaderView
from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import *

from .DrillHeaderView import DrillHeaderView
from .DrillItemDelegate import DrillItemDelegate

class DrillTableWidget(QTableWidget):
    """
    Widget based on QTableWidget, used in the DrILL interface. It mainly binds
    the custom header and delegate and expose some useful functions.
    """

    def __init__(self, parent=None):
        super(DrillTableWidget, self).__init__(parent)
        header = DrillHeaderView(self)
        header.setSectionsClickable(True)
        header.setHighlightSections(True)
        header.setSectionResizeMode(QHeaderView.Interactive)
        self.setHorizontalHeader(header)

        delegate = DrillItemDelegate(self)
        self.setItemDelegate(delegate)

