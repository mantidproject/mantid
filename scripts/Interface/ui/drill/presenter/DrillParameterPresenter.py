# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import *


class DrillParameterPresenter:

    """
    Reference to the QTableWidgetItem.
    """
    _item = None

    """
    Reference to the parameter model.
    """
    _parameter = None

    ERROR_COLOR = "#3fff0000"

    def __init__(self, item, parameter):
        self._item = item
        self._item.setPresenter(self)
        self._parameter = parameter
        self._parameter.valid.connect(self.onValid)
        self._parameter.invalid.connect(self.onInvalid)
        self._item.signals.dataChanged.connect(self.onDataChanged)

    def onDataChanged(self):
        """
        Triggered when the data changed in the view. This function propagates
        the changes to the model.
        """
        value = self._item.text()
        self._parameter.setValue(value)

    def onValid(self):
        """
        Triggered when the parameter is valid.
        """
        self._item.setData(0, Qt.BackgroundRole, None)
        self._item.setToolTip("")

    def onInvalid(self, msg):
        """
        Triggered when the parameter is invalid.

        Args:
            msg (str): error message associated to the invalid state
        """
        brush = QBrush(QColor(self.ERROR_COLOR))
        self._item.setBackground(brush)
        self._item.setToolTip(msg)
