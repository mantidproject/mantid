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

    OK_COLOR = "#3f008744"
    ERROR_COLOR = "#3fd62d20"
    PROCESSING_COLOR = "#3fffa700"

    def __init__(self, parent=None):
        super(DrillTableWidget, self).__init__(parent)
        header = DrillHeaderView(self)
        header.setSectionsClickable(True)
        header.setHighlightSections(True)
        header.setSectionResizeMode(QHeaderView.Interactive)
        self.setHorizontalHeader(header)

        delegate = DrillItemDelegate(self)
        self.setItemDelegate(delegate)

    def setRowBackground(self, row, color):
        """
        Set the background color of an existing row. If the row cells do not
        contain items, they will be created by this method.

        Args:
            row (int): the row index
            color (str): the RBG or ARGB string color
        """
        brush = QBrush(QColor(color))
        if (row >= self.rowCount()):
            return
        for c in range(self.columnCount()):
            item = self.item(row, c)
            if not item:
                item = QTableWidgetItem()
                self.setItem(row, c, item)

            self.item(row, c).setBackground(brush)

    def setRowOkColor(self, row):
        """
        Set the OK_COLOR to an existing row.

        Args:
            row (int): the row index
        """
        self.setRowBackground(row, self.OK_COLOR)

    def setRowErrorColor(self, row):
        """
        Set the ERROR_COLOR to an existing row.

        Args:
            row (int): the row index
        """
        self.setRowBackground(row, self.ERROR_COLOR)

    def setRowProcessingColor(self, row):
        """
        Set the PROCESSING_COLOR to an existing row.

        Args:
            row (int): the row index
        """
        self.setRowBackground(row, self.PROCESSING_COLOR)

