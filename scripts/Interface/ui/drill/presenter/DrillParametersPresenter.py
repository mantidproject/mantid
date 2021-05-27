# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import *


class DrillParametersPresenter:

    """
    Reference to the QTableWidgetItem.
    """
    _item = None

    """
    Reference to the sample.
    """
    _sample = None

    """
    List of parameter names.
    """
    _parameters = None

    ERROR_COLOR = "#3fff0000"

    def __init__(self, item, sample):
        self._item = item
        self._item.setPresenter(self)
        self._sample = sample
        self._parameters = list()
        self._item.signals.dataChanged.connect(self.onDataChanged)

    def onDataChanged(self):
        """
        Triggered when the data changed in the view. This function propagates
        the changes to the model.
        """
        # remove all previously set parameters
        for name in self._parameters:
            self._sample.delParameter(name)
        self._parameters = list()
        self._item.signals.blockSignals(True)
        self._item.setData(Qt.BackgroundRole, None)
        self._item.setToolTip("")
        self._item.signals.blockSignals(False)

        # use the actual value
        value = self._item.text()
        for param in value.split(';'):
            if param and '=' not in param:
                self.onInvalid("Please provide semicolon separated key=value "
                               "pairs.")
                return
            try:
                name = param.split("=")[0].strip()
                value = param.split("=")[1].strip()
            except:
                self.onInvalid("Please provide semicolon separated key=value "
                               "pairs.")
                return
            if name in self._parameters:
                self.onInvalid("Same parameter provided several times. Only "
                               "the first value will be used.")
                continue
            parameter = self._sample.addParameter(name)
            parameter.invalid.connect(self.onInvalid)
            self._parameters.append(name)
            parameter.setValue(value)

    def onInvalid(self, msg):
        """
        Triggered when the parameter is invalid.

        Args:
            msg (str): error message associated to the invalid state
        """
        self._item.signals.blockSignals(True)
        brush = QBrush(QColor(self.ERROR_COLOR))
        self._item.setBackground(brush)
        self._item.setToolTip(msg)
        self._item.signals.blockSignals(False)
