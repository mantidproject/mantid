# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy.QtWidgets import QMessageBox
from qtpy.QtGui import QBrush, QColor
from qtpy.QtCore import Qt


class DrillParametersPresenter:
    """
    Reference to the table model (QTableWidget).
    """

    _table = None

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

    def __init__(self, table, item, sample):
        self._table = table
        self._item = item
        self._item.setPresenter(self)
        self._sample = sample
        self._parameters = list()
        self._item.signals.dataChanged.connect(self.onDataChanged)

    def addParameter(self, name):
        """
        Add a parameter name to the list.

        Args:
            name (str): new parameter name
        """
        self._parameters.append(name)
        parameter = self._sample.getParameter(name)
        parameter.checked.connect(self.onChecked)
        parameter.valueChanged.connect(self.onValueChanged)

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
        if value == "":
            # if value is empty, delete the prenter
            self._item.setPresenter(None)
            return
        text = ""
        parameters = dict()
        for param in value.split(";"):
            if param and "=" not in param:
                QMessageBox.warning(self._table, "Error", "Please provide semicolon separated key=value pairs.")
                break
            try:
                name = param.split("=")[0].strip()
                value = param.split("=")[1].strip()
            except:
                QMessageBox.warning(self._table, "Error", "Please provide semicolon separated key=value pairs.")
                break
            if name in parameters:
                QMessageBox.warning(
                    self._table, "Error", 'Parameter "{}" provided several times. Only the first one will be used.'.format(name)
                )
                break
            if self._table.itemFromName(self._sample.getIndex(), name):
                QMessageBox.warning(
                    self._table, "Error", 'Use the dedicated column to set the value of "{}". This one will be ignored.'.format(name)
                )
                break
            if isinstance(value, str) and value.lower() == "true":
                value = True
            if isinstance(value, str) and value.lower() == "false":
                value = False
            p = self._sample.addParameter(name)
            p.checked.connect(self.onChecked)
            p.setValue(value)
            text += str(name) + "=" + str(value) + ";"
        self._table.blockSignals(True)
        self._item.setText(text[:-1])
        self._table.blockSignals(False)

    def onChecked(self):
        """
        Triggered when one of the parameters has been checked. This methods
        colors the associated cell if invalid.
        """
        for name in self._parameters:
            p = self._sample.getParameter(name)
            if not p.isValid():
                msg = p.getErrorMessage()
                self.setInvalidItem(msg)
                return

    def setInvalidItem(self, msg):
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

    def onValueChanged(self):
        """
        Triggered when one of the parameter values changed.
        """
        text = ""
        for name in self._parameters:
            param = self._sample.getParameter(name)
            value = param.getValue()
            text += name
            text += "="
            text += str(value)
            text += ";"
        text = text[:-1]
        self._item.signals.blockSignals(True)
        self._item.setText(text)
        self._item.signals.blockSignals(False)
