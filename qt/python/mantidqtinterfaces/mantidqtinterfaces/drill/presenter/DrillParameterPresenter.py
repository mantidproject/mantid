# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import Qt


class DrillParameterPresenter:
    """
    Reference to the QTableWidgetItem.
    """

    _item = None

    """
    Reference to the sample model (DrillSample).
    """
    _sample = None

    """
    Reference to the parameter model.
    """
    _parameter = None

    ERROR_COLOR = "#3fff0000"

    def __init__(self, item, sample, parameter):
        self._item = item
        self._item.setPresenter(self)
        self._sample = sample
        self._parameter = parameter
        self._parameter.checked.connect(self.onChecked)
        self._item.signals.dataChanged.connect(self.onDataChanged)
        self._parameter.valueChanged.connect(self.onValueChanged)

    def onDataChanged(self):
        """
        Triggered when the data changed in the view. This function propagates
        the changes to the model.
        """
        value = self._item.text()
        if value == "":
            # if value is empty, delete the parameter and the presenter
            self._item.signals.blockSignals(True)
            self._item.setData(Qt.BackgroundRole, None)
            self._item.setToolTip("")
            self._item.signals.blockSignals(False)
            self._sample.delParameter(self._parameter.getName())
            self._parameter = None
            self._item.setPresenter(None)
        else:
            # else, propagate the value
            self._parameter.setValue(value)

    def onChecked(self):
        """
        Triggered when the parameter has been checked. This methods colors the
        table item accordingly.
        """
        valid = self._parameter.isValid()
        self._item.signals.blockSignals(True)
        if not valid:
            msg = self._parameter.getErrorMessage()
            brush = QBrush(QColor(self.ERROR_COLOR))
            self._item.setBackground(brush)
            self._item.setToolTip(msg)
        else:
            self._item.setData(Qt.BackgroundRole, None)
            self._item.setToolTip("")
        self._item.signals.blockSignals(False)

    def onValueChanged(self):
        """
        Triggered when the parameter value changed.
        """
        value = self._parameter.getValue()
        self._item.signals.blockSignals(True)
        self._item.setText(value)
        self._item.signals.blockSignals(False)
